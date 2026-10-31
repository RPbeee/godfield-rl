#include "godfield/official_state.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using namespace godfield::official;
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL official state: "<<message<<'\n';std::exit(1);}}
template<class F>void throws(F&& f,const char* message){bool yes=false;try{f();}catch(const std::exception&){yes=true;}check(yes,message);}
CardId image(std::string_view name){for(const auto& c:catalog())if(name==c.image_name)return c.id;check(false,"known model exists");return 0;}
PlayerState person(PlayerId id,Team team=Team::solo){PlayerState p;p.id=id;p.team=team;p.name="player"+std::to_string(id);return p;}
SnapshotState fixture(){SnapshotState s;s.gf=123;s.tiebreak_gf=200;s.update_count=42;s.players={person(0,Team::one),person(7,Team::one),person(9,Team::two),person(12),person(88,Team::two)};
    s.players[0].user_id="own-user";s.players[0].name="self";s.players[0].alias="ignored-own-alias";s.players[0].curses={Curse::fog};
    s.players[2].alias="public foe";s.players[2].user_id="private-foe";s.players[2].items={{0,image("spring"),image("aura"),true},{1,image("bronze-club"),{},false}};
    s.players[4].dead=true;s.players[4].confusion_count=5;
    s.players[0].items={{0,image("spring"),image("bronze-club"),false},{1,image("bronze-club"),image("spring"),false},{2,image("spring"),{},true}};return s;}
std::vector<bool> readiness(const SnapshotState& s){std::vector<bool> a;for(ItemToken i=0;i<4;++i){a.push_back(can_append_hand(s,0,{},i));std::array<ItemToken,1> one{i};a.push_back(selection_ready(s,0,one));}a.push_back(selection_ready(s,0,{}));return a;}

void relations_and_fresh_views(){
    auto s=fixture();validate_state(s);check(find_player(s,0)==&s.players[0]&&!find_player(s,999)&&resolve_viewer(s,"own-user")==0&&!resolve_viewer(s,"absent"),"zero arbitrary IDs and optional viewer identity lookup");
    check(!is_enemy(s.players[0],s.players[0])&&!is_enemy(s.players[0],s.players[1])&&is_enemy(s.players[0],s.players[2])&&is_enemy(s.players[0],s.players[3]),"team ally/other team/solo enemy relations");
    check(alive_players(s)==std::vector<PlayerId>{0,7,9,12}&&enemy_players(s,0)==std::vector<PlayerId>{9,12},"not-dead filtering preserves fixed snapshot order");
    check(ending_display_candidates(s)==std::vector<PlayerId>{0,7},"first alive team's display group is not a completed outcome even with enemies alive");
    s.players[0].dead=true;check(ending_display_candidates(s)==std::vector<PlayerId>{0,7},"ending display includes dead teammates");s=fixture();
    const auto before=s;const auto v=project_view(s,0);check(s==before&&v.viewer==0&&v.gf==123&&v.tiebreak_gf==200&&!v.is_over,"projection pure and carries public frame metadata only");
    check(v.players[0].display_name=="self"&&v.players[2].display_name=="public foe"&&!v.players[0].fogged&&!v.players[1].fogged
        &&v.players[2].fogged&&v.players[3].fogged&&!v.players[4].fogged&&!v.players[2].status&&v.players[1].status&&v.players[4].status&&!v.players[4].status->confused,"fresh fog hides alive enemy panes, preserves allies/dead and alias-visible name");
    check(v.players[2].used_models==std::vector<CardId>{image("aura")},"foreign used visible models public while hidden true model and tokens remain absent");
    check(v.hand.size()==3&&v.hand[0].token==0&&v.hand[0].face==image("bronze-club")&&v.hand[0].reference==v.hand[0].face&&v.hand[0].disguised
        &&v.hand[1].token==1&&!v.hand[1].used&&v.hand[2].token==2&&v.hand[2].used,"fresh hand sorted unused first/visible face, tooltip remains perceived model");
    const auto spectator=project_view(s,{});check(spectator.hand.empty()&&!spectator.viewer&&std::all_of(spectator.players.begin(),spectator.players.end(),[](const auto& p){return p.status&&!p.fogged;}),"spectator has no own hand and no viewer fog overlay");
    for(bool over:{false,true}){auto x=s;x.is_over=over;x.players[0].dead=!over;const auto shown=project_view(x,0);check(shown.players[2].status&&!shown.players[2].fogged
        &&shown.hand[0].face==image("bronze-club")&&shown.hand[0].reference==image("spring"),"death/game over restores resources and own true reference without changing face");}
    auto solo=s;for(auto& p:solo.players)p.team=Team::solo;check(ending_display_candidates(solo)==std::vector<PlayerId>{0},"solo display picks first alive, not every survivor");
    for(auto& p:solo.players)p.dead=true;check(alive_players(solo).empty()&&ending_display_candidates(solo).empty(),"all-dead fresh snapshot has no display candidates");
    SnapshotState empty;validate_state(empty);check(project_view(empty,{}).players.empty(),"empty display snapshot allowed independently from playable game");
    throws([&]{(void)project_view(empty,0);},"nonexistent viewer cannot borrow another hand");
}

void private_state_noninterference_and_visible_selection(){
    auto s=fixture();const auto observed=project_view(s,0);const auto public_json=view_json(observed);const auto choices=readiness(s);
    auto hidden=s;hidden.update_count=800;hidden.players[0].user_id="different-auth-id";hidden.players[0].items[0].model=image("sun-amulet");hidden.players[0].items[1].model=image("wall");
    auto& enemy=hidden.players[2];enemy.name="secret raw name";enemy.user_id="secret transport user";enemy.hp=-50;enemy.mp=900;enemy.cp=-4;
    enemy.curses={Curse::heaven,Curse::dream};enemy.guardian=Guardian::mars;enemy.confusion_count=999;enemy.items[0].model=image("bronze-club");enemy.items.resize(1);
    for(ItemToken n=1;n<80;++n)enemy.items.push_back({n,image(n%2?"sun-amulet":"bronze-club"),{},false});
    check(project_view(hidden,0)==observed&&view_json(project_view(hidden,0))==public_json&&readiness(hidden)==choices,"truth/disguise, private UID, hidden resources/harms, foreign unused size/order and update version cannot leak through view/queries");
    auto count=s;count.players[0].confusion_count=2;const auto v=project_view(count,0);count.players[0].confusion_count=999;
    check(project_view(count,0)==v,"raw confusion duration hidden; only bool is visible");
    auto public_change=s;public_change.players[2].items[0].fake_model=image("spring");check(project_view(public_change,0)!=observed,"foreign performed visible miracle change is public, not a privacy violation");
    check(can_append_hand(s,0,{},0)&&selection_ready(s,0,std::array<ItemToken,1>{0}),"disguised true miracle selected as apparent weapon");
    s.players[0].mp=6;check(!selection_ready(s,0,std::array<ItemToken,1>{1}),"perceived Spring uses displayed MP7 cost, not hidden weapon cost");
    s.players[0].mp=7;check(selection_ready(s,0,std::array<ItemToken,1>{1}),"perceived displayed cost can become ready exactly at7MP");
    check(!can_append_hand(s,0,std::array<ItemToken,1>{0},0)&&!selection_ready(s,0,std::array<ItemToken,2>{0,0})&&!selection_ready(s,0,std::array<ItemToken,1>{99}),"physical tokens unique and owned independently from duplicate model identities");
    auto pray=fixture();pray.players[0].items={{0,image("bronze-club"),image("aqua-shoes"),false}};
    check(selection_ready(pray,0,{}),"prayer depends on displayed weapon absence only");pray.players[0].items[0].fake_model.reset();check(!selection_ready(pray,0,{}),"a visible weapon prevents empty main prayer");
    pray.players[0].items={{0,image("discard"),{},false},{1,image("aqua-shoes"),{},false}};
    check(!selection_ready(pray,0,std::array<ItemToken,1>{0})&&selection_ready(pray,0,std::array<ItemToken,2>{0,1}),"discard/sacrifice require an actual selected extra");
    pray.players[0].items[0].model=image("sell");check(!selection_ready(pray,0,std::array<ItemToken,1>{0})&&selection_ready(pray,0,std::array<ItemToken,2>{0,1}),"sell requires exactly one physical goods selection");
    pray.players[0].items={{0,image("wall"),{},true},{1,image("spiritual-doll"),{},false}};pray.players[0].mp=0;
    ResponseContext response;response.flash=true;
    check(selection_ready(pray,0,{},response)&&can_append_hand(pray,0,std::array<ItemToken,1>{0},1,response)
        &&selection_ready(pray,0,std::array<ItemToken,2>{0,1},response),"empty response and FREE retained wall allowed without MP; caller flash is not trusted");
    pray.players[0].curses={Curse::flash};response.flash=false;
    check(!can_append_hand(pray,0,std::array<ItemToken,1>{0},1,response)&&!selection_ready(pray,0,std::array<ItemToken,2>{0,1},response),"own visible flash overrides false caller bit before FREE exception");
    pray.players[0].dead=true;check(!selection_ready(pray,0,{},response)&&!can_append_hand(pray,0,{},0,response),"dead viewer cannot select cards");
}

void validation_renderer_bounds_and_json(){
    auto s=fixture();for(int error=0;error<11;++error){auto bad=s;if(error==0)bad.players.push_back(bad.players[0]);if(error==1)bad.players[0].id=max_wire_integer+1;
        if(error==2)bad.players[0].team=static_cast<Team>(5);if(error==3)bad.players[0].guardian=static_cast<Guardian>(11);if(error==4)bad.players[0].hp=std::numeric_limits<std::int64_t>::min();
        if(error==5)bad.players[0].items.push_back(bad.players[0].items[0]);if(error==6)bad.players[0].items[0].model=0;if(error==7)bad.players[0].items[0].fake_model=297;
        if(error==8)bad.players[0].items[0].token=max_wire_integer+1;if(error==9)bad.players[0].curses={Curse::none};if(error==10)bad.update_count=max_wire_integer+1;
        throws([&]{validate_state(bad);},"invalid IDs/tokens/enums/models/unsafe JS integers rejected");}
    SnapshotState nine;for(unsigned i=0;i<9;++i)nine.players.push_back(person(i));validate_state(nine);nine.players.push_back(person(9));throws([&]{validate_state(nine);},"maximum nine players enforced");
    auto many=s;many.players[0].items.clear();for(unsigned i=0;i<19;++i)many.players[0].items.push_back({i,image("bronze-club"),{},false});
    validate_state(many);throws([&]{(void)project_view(many,0);},"own19-card fresh renderer domain rejected without pretending server capacity proven");
    throws([&]{(void)selection_ready(many,0,{});},"hand queries also enforce own rendering domain");check(project_view(many,7).players.size()==5,"foreign unused count does not become a validity oracle");
    auto unicode=s;unicode.players[0].name=std::string("name\0\"\\\n",8)+"日本語";const auto json=view_json(project_view(unicode,0));
    check(json.find("\\u0000")!=std::string::npos&&json.find("\\\"")!=std::string::npos&&json.find("\\\\")!=std::string::npos&&json.find("日本語")!=std::string::npos
        &&json.find("own-user")==std::string::npos&&json.find("private-foe")==std::string::npos&&json.find("update_count")==std::string::npos,"JSON escapes controls/quotes and omits transport IDs/version");
    for(const auto bad:{std::string("\xc0\x80",2),std::string("\xed\xa0\x80",3),std::string("\xf4\x90\x80\x80",4),std::string("\xe3",1)}){
        auto x=s;x.players[0].name=bad;throws([&]{validate_state(x);},"invalid UTF8 input rejected rather than emitting broken JSON");}
    auto curses=s;curses.players[0].curses={Curse::cold,Curse::fog,Curse::fever,Curse::cold};
    check(project_view(curses,0).players[0].status->curses==std::vector<Curse>{Curse::fever,Curse::fog},"fresh wire Set deduplicates before last-distinct-disease normalization");
}
}
int main(){relations_and_fresh_views();private_state_noninterference_and_visible_selection();validation_renderer_bounds_and_json();std::cout<<"Official fresh state passed (teams/view identity/fog/dream noninterference/token selection/renderer domain/strict transport).\n";}
