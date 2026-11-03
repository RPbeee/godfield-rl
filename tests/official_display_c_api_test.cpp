#include "godfield/official_display_c_api.h"
#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
void check(bool ok,const char* what){if(!ok){std::cerr<<"FAIL display C API: "<<what<<'\n';std::exit(1);}}
std::string view(const gfo_display* display){
    std::size_t required=0;
    check(gfo_display_view_json(display,nullptr,0,&required)==0&&required>1,"query immutable display length");
    std::vector<char> bytes(required+2,'Z');
    check(gfo_display_view_json(display,bytes.data()+1,required,&required)==0
        &&bytes.front()=='Z'&&bytes.back()=='Z'&&bytes[required]=='\0',"exact output buffer and NUL canaries");
    return bytes.data()+1;
}
gfo_state* board(std::int64_t gf=17){
    gfo_state_meta meta{};meta.gf=gf;
    std::array<gfo_state_player,2> players{};players[1].id=7;
    gfo_state* out=nullptr;
    check(gfo_state_create(&meta,players.data(),players.size(),nullptr,0,nullptr,0,&out)==0,"create typed read-only source board");
    return out;
}
}
int main(){
    check(gfo_display_api_version()==1&&gfo_state_abi_version()==1&&gfo_display_event_size()==88&&sizeof(gfo_display_event)==88
        &&offsetof(gfo_display_event,attack)==0&&offsetof(gfo_display_event,player_id)==72
        &&offsetof(gfo_display_event,kind)==80&&offsetof(gfo_display_event,reserved)==81,"additive API1 event layout preserves State ABI1");
    auto* state=board();gfo_display* original=nullptr;
    check(gfo_display_create(state,0,&original)==0&&original,"display owns copied state and active head");
    gfo_state_destroy(state);
    const std::string initial="{\"schema\":\"official-active-display-view-v1\",\"gf\":17,\"reversed\":false,\"panes\":{\"left\":0,\"right\":null},\"selected_side\":null,\"attack\":null}";
    check(view(original)==initial,"real player0 initializes left pane after source handle is destroyed");
    gfo_display* sentinel=reinterpret_cast<gfo_display*>(uintptr_t{1});
    check(gfo_display_create(nullptr,0,&sentinel)==-1&&sentinel==reinterpret_cast<gfo_display*>(uintptr_t{1}),"create NULL preserves out");
    state=board();
    check(gfo_display_create(state,0,nullptr)==-1&&gfo_display_create(state,999,&sentinel)==-2
        &&sentinel==reinterpret_cast<gfo_display*>(uintptr_t{1}),"unknown initial turn player is not inferred");gfo_state_destroy(state);
    std::array<uint16_t,1> models{{58}};
    std::array<gfo_display_event,3> events{};
    events[0].attack.models=models.data();events[0].attack.model_count=1;events[0].attack.player_id=7;
    events[0].attack.atk=20;events[0].attack.guardian=9;events[0].attack.reversed=1;
    events[0].attack.target_player_id=999;events[0].attack.buying_model=235;
    events[1].kind=1;events[1].player_id=0;events[2].kind=2;events[2].player_id=7;
    gfo_display* first=nullptr;gfo_display* second=nullptr;gfo_display* sequential=nullptr;gfo_display* batch=nullptr;
    check(gfo_display_apply(original,events.data(),1,&first)==0,"typed nextAttack copies model column");
    const auto first_view=view(first);
    check(first_view.find("\"panes\":{\"left\":null,\"right\":7}")!=std::string::npos
        &&first_view.find("\"source\":7")!=std::string::npos&&first_view.find("\"power\":20")!=std::string::npos
        &&first_view.find("\"from_mp\":true")!=std::string::npos&&first_view.find("\"target\":null")!=std::string::npos
        &&first_view.find("\"buying_model\":null")!=std::string::npos,"incremental nextAttack ignores fresh target and buying restoration");
    check(gfo_display_apply(first,&events[1],1,&second)==0&&view(second).find("\"panes\":{\"left\":0,\"right\":7}")!=std::string::npos
        &&view(second).find("\"selected_side\":\"left\"")!=std::string::npos&&view(second).find("\"target\":0")!=std::string::npos,
        "setTargetPlayer0 is a real lookup and displayed target, not an absent sentinel");
    check(gfo_display_apply(second,&events[2],1,&sequential)==0&&gfo_display_apply(original,events.data(),3,&batch)==0
        &&view(sequential)==view(batch),"ordered batch and one-event clones agree");
    const std::string advanced="{\"schema\":\"official-active-display-view-v1\",\"gf\":18,\"reversed\":false,\"panes\":{\"left\":7,\"right\":null},\"selected_side\":null,\"attack\":null}";
    check(view(batch)==advanced&&view(original)==initial,"advanceGF clears attack and reversals without mutating original");
    models[0]=6;events[0].attack.atk=999;
    check(view(first)==first_view,"returned handle does not borrow caller model storage");
    std::array<char,8> short_output{};short_output.fill('Q');const auto old_output=short_output;std::size_t required=777;
    check(gfo_display_view_json(first,short_output.data(),short_output.size(),&required)==-4&&short_output==old_output&&required==777,
        "short display output preserves bytes and size");
    check(gfo_display_view_json(nullptr,short_output.data(),8,&required)==-1
        &&gfo_display_view_json(first,nullptr,1,&required)==-1
        &&gfo_display_view_json(first,short_output.data(),8,nullptr)==-1&&short_output==old_output&&required==777,"view NULL checks have no partial output");
    for(unsigned error=0;error<12;++error){auto bad=events[0];
        if(error<7)bad.reserved[error]=1;if(error==7)bad.kind=3;if(error==8)bad.player_id=7;
        if(error==9)bad.attack.reversed=2;if(error==10)bad.attack.guardian=11;
        if(error==11){bad.attack.models=reinterpret_cast<const uint16_t*>(uintptr_t{1});bad.attack.model_count=std::numeric_limits<std::size_t>::max();}
        check(gfo_display_apply(first,&bad,1,&sentinel)==-2&&sentinel==reinterpret_cast<gfo_display*>(uintptr_t{1})&&view(first)==first_view,
            "bad event discriminants, padding and impossible extents are atomic");
    }
    for(std::size_t byte=0;byte<sizeof(gfo_state_active_attack);++byte){gfo_display_event bad{};bad.kind=1;
        reinterpret_cast<unsigned char*>(&bad.attack)[byte]=1;
        check(gfo_display_apply(first,&bad,1,&sentinel)==-2&&sentinel==reinterpret_cast<gfo_display*>(uintptr_t{1}),
            "all72 inactive attack bytes including pointer are rejected before dereference");}
    std::array<gfo_display_event,2> fails_late{{events[2],{}}};fails_late[1].kind=1;fails_late[1].player_id=999;
    check(gfo_display_apply(original,fails_late.data(),2,&sentinel)==-2&&sentinel==reinterpret_cast<gfo_display*>(uintptr_t{1})&&view(original)==initial,
        "valid prefix GF change is rolled back when a later lookup fails");
    check(gfo_display_apply(original,reinterpret_cast<const gfo_display_event*>(uintptr_t{1}),std::numeric_limits<std::size_t>::max(),&sentinel)==-2
        &&sentinel==reinterpret_cast<gfo_display*>(uintptr_t{1}),"impossible batch count rejects before pointer access");
    check(gfo_display_apply(nullptr,nullptr,0,&sentinel)==-1&&gfo_display_apply(original,nullptr,1,&sentinel)==-1
        &&gfo_display_apply(original,nullptr,0,nullptr)==-1&&sentinel==reinterpret_cast<gfo_display*>(uintptr_t{1}),"apply NULL variants preserve out");
    gfo_display* clone=nullptr;check(gfo_display_apply(original,nullptr,0,&clone)==0&&view(clone)==initial,"zero events returns a separate owner");
    gfo_display_destroy(original);check(view(clone)==initial,"clone survives source display destruction");
    state=board(9007199254740991LL);gfo_display* exhausted=nullptr;
    check(gfo_display_create(state,0,&exhausted)==0&&gfo_display_apply(exhausted,&events[2],1,&sentinel)==-2
        &&sentinel==reinterpret_cast<gfo_display*>(uintptr_t{1})&&view(exhausted).find("\"gf\":9007199254740991")!=std::string::npos,
        "GF safe-integer exhaustion never produces a partial new owner");gfo_state_destroy(state);
    for(auto* display:{first,second,sequential,batch,clone,exhausted})gfo_display_destroy(display);
    gfo_display_destroy(nullptr);std::cout<<"official display C API passed\n";
}
