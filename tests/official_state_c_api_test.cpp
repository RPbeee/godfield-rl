#include "godfield/official_state_c_api.h"
#include <array>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
void check(bool ok, const char* text) { if (!ok) { std::cerr << "FAIL state C API: " << text << '\n'; std::exit(1); } }
std::string view(gfo_state* state, uint8_t present = 1, uint64_t viewer = 0) {
    size_t required = 999;
    check(gfo_state_view_json(state, present, viewer, nullptr, 0, &required) == 0 && required > 1, "query view length");
    std::vector<char> bytes(required + 2, 'Z');
    check(gfo_state_view_json(state, present, viewer, bytes.data() + 1, required, &required) == 0, "write UTF8 view");
    check(bytes.front() == 'Z' && bytes.back() == 'Z' && bytes[required] == '\0', "exact output boundary and NUL");
    return {bytes.data() + 1};
}
std::string active(gfo_state* state) {
    size_t required=0;
    check(gfo_state_active_attack_json(state,nullptr,0,&required)==0&&required>1,"query separate active attack length");
    std::vector<char> bytes(required+2,'Z');
    check(gfo_state_active_attack_json(state,bytes.data()+1,required,&required)==0
        &&bytes.front()=='Z'&&bytes.back()=='Z'&&bytes[required]=='\0',"active attack exact output boundary and terminator");
    return bytes.data()+1;
}
}
int main() {
    check(gfo_state_abi_version() == 1 && gfo_state_item_size() == sizeof(gfo_state_item)
        && gfo_state_player_size() == sizeof(gfo_state_player) && gfo_state_meta_size() == sizeof(gfo_state_meta)
        && gfo_state_response_size() == sizeof(gfo_state_response)
        &&gfo_state_active_attack_size()==72&&sizeof(gfo_state_active_attack)==72
        &&offsetof(gfo_state_active_attack,model_count)==8&&offsetof(gfo_state_active_attack,player_id)==16
        &&offsetof(gfo_state_active_attack,atk)==32&&offsetof(gfo_state_active_attack,buying_model)==64
        &&offsetof(gfo_state_active_attack,guardian)==66&&offsetof(gfo_state_active_attack,reserved)==68,"ABI sizes");
    const char name[] = {'A', '\0', 'B'};
    const char uid[] = {'u', '\0', 'i'};
    gfo_state_meta meta{};
    gfo_state_player p{}; p.name = name; p.name_size = 3; p.user_id = uid; p.user_id_size = 3;
    p.hp = 40; p.mp = 10; p.cp = 20; p.item_count = 2;
    std::array<gfo_state_item, 2> items{{{0, 6, 0, 0, {}}, {3, 9, 0, 0, {}}}};
    gfo_state* handle = nullptr;
    check(gfo_state_create(&meta, &p, 1, items.data(), items.size(), nullptr, 0, &handle) == 0 && handle, "copy immutable frame");
    const auto initial = view(handle);
    check(initial.find("A\\u0000B") != std::string::npos && initial.find("\"token\":0") != std::string::npos, "NUL-safe UTF8 string and token0");
    p.hp = 0; items[0].model = 53;
    check(view(handle) == initial, "create owns all caller records/strings");
    uint8_t found = 9; uint64_t id = 99;
    check(gfo_state_resolve_viewer(handle, uid, 3, &found, &id) == 0 && found == 1 && id == 0, "NUL user ID and player0");
    check(gfo_state_resolve_viewer(handle, "absent", 6, &found, &id) == 0 && found == 0, "explicit no user match");
    std::array<char, 8> short_buffer{}; short_buffer.fill('Q'); const auto old_buffer = short_buffer; size_t needed = 787;
    check(gfo_state_view_json(handle, 1, 0, short_buffer.data(), short_buffer.size(), &needed) == -4
        && short_buffer == old_buffer && needed == 787, "short leaves every output unchanged");
    check(gfo_state_view_json(handle, 1, 123, short_buffer.data(), 8, &needed) == -2 && short_buffer == old_buffer && needed == 787, "bad viewer transactional output");
    check(gfo_state_view_json(handle, 0, 1, nullptr, 0, &needed) == -2 && needed == 787, "spectator must be explicit canonical selector");
    check(gfo_state_view_json(handle, 0, 0, nullptr, 1, &needed) == -1, "NULL output storage");
    check(view(handle, 0, 0).find("\"viewer\":null") != std::string::npos, "spectator");
    const uint64_t first = 0;
    uint8_t result = 177;
    check(gfo_state_can_append(handle, 0, &first, 1, 3, nullptr, &result) == 0 && result == 1, "physical token lookup into visible main kernel");
    check(gfo_state_selection_ready(handle, 0, &first, 1, nullptr, &result) == 0 && result == 1, "selection ready");
    result = 177;
    for (size_t count : {size_t{19}, std::numeric_limits<size_t>::max()}) {
        check(gfo_state_can_append(handle, 0, &first, count, 3, nullptr, &result) == -2 && result == 177, "count rejected before dereference");
        check(gfo_state_selection_ready(handle, 0, &first, count, nullptr, &result) == -2 && result == 177, "ready count rejected before dereference");
    }
    gfo_state_response bad{}; bad.incoming_element = -1; bad.reserved[3] = 1;
    check(gfo_state_selection_ready(handle, 0, nullptr, 0, &bad, &result) == -2 && result == 177, "response zero padding");
    gfo_state* sentinel = reinterpret_cast<gfo_state*>(uintptr_t{1});
    p.hp = 40; items[0].model = 6;
    auto invalid = p; invalid.team = 5;
    check(gfo_state_create(&meta, &invalid, 1, items.data(), 2, nullptr, 0, &sentinel) == -2 && sentinel == reinterpret_cast<gfo_state*>(uintptr_t{1}), "create cannot replace output on bad input");
    const char broken_utf[] = {char(0xc0), char(0x80)};
    invalid = p; invalid.name = broken_utf; invalid.name_size = 2;
    check(gfo_state_create(&meta, &invalid, 1, items.data(), 2, nullptr, 0, &sentinel) == -2, "overlong UTF8 rejected");
    invalid.name = "\xed\xa0\x80"; invalid.name_size = 3;
    check(gfo_state_create(&meta, &invalid, 1, items.data(), 2, nullptr, 0, &sentinel) == -2, "UTF8 surrogate rejected");
    invalid = p; invalid.name = "X"; invalid.name_size = std::numeric_limits<size_t>::max();
    check(gfo_state_create(&meta, &invalid, 1, items.data(), 2, nullptr, 0, &sentinel) == -2
        && sentinel == reinterpret_cast<gfo_state*>(uintptr_t{1}), "impossible string extent rejected before dereference");
    found = 177; id = 888;
    check(gfo_state_resolve_viewer(handle, "X", std::numeric_limits<size_t>::max(), &found, &id) == -2
        && found == 177 && id == 888, "impossible viewer string extent preserves outputs");
    auto duplicate_items = items; duplicate_items[1].token = duplicate_items[0].token;
    check(gfo_state_create(&meta, &p, 1, duplicate_items.data(), 2, nullptr, 0, &sentinel) == -2, "duplicate token rejected");
    invalid = p; invalid.item_count = 1;
    check(gfo_state_create(&meta, &invalid, 1, items.data(), 2, nullptr, 0, &sentinel) == -2, "no ignored flat records");
    items[0].model = 0;
    check(gfo_state_create(&meta, &p, 1, items.data(), 2, nullptr, 0, &sentinel) == -2, "UI model0 placeholder rejected");
    check(view(handle) == initial, "all failures preserve prior live frame");
    check(active(handle)=="{\"schema\":\"official-active-attack-view-v1\",\"attack\":null}","old board-only creation has no active attack");
    std::array<gfo_state_player,2> attack_people{};attack_people[1].id=900;
    attack_people[0].item_count=2;attack_people[1].item_offset=2;
    const std::array<gfo_state_item,2> attack_items{{{0,6,0,0,{}},{3,9,0,0,{}}}};
    gfo_state* attack_board=nullptr;
    check(gfo_state_create(&meta,attack_people.data(),2,attack_items.data(),2,nullptr,0,&attack_board)==0,"active display board has known source0 and target900");
    const auto attack_initial=view(attack_board);
    std::array<uint16_t,1> attack_models{{6}};
    gfo_state_active_attack attack{};attack.models=attack_models.data();attack.model_count=1;
    attack.target_player_id=900;attack.atk=7;attack.hit_rate=75;attack.mp=-3;attack.cp=4;attack.buying_model=235;attack.guardian=9;attack.reversed=1;
    gfo_state* attached=nullptr;
    check(gfo_state_with_active_attack(attack_board,&attack,&attached)==0&&attached,"attach returns a separate immutable owner");
    const auto attack_view=active(attached);
    check(attack_view.find("\"source\":0")!=std::string::npos&&attack_view.find("\"target\":900")!=std::string::npos
        &&attack_view.find("\"buying_model\":235")!=std::string::npos&&attack_view.find("\"guardian\":\"earth\"")!=std::string::npos
        &&attack_view.find("\"models\":[6]")!=std::string::npos&&attack_view.find("\"power\":7")!=std::string::npos
        &&attack_view.find("\"hit_rate\":75")!=std::string::npos&&attack_view.find("\"mp\":-3")==std::string::npos,
        "source0 and known positive target remain public DTO values with composed attack summary");
    auto unused_scalars=attack;unused_scalars.mp=9007199254740991LL;unused_scalars.cp=-9007199254740991LL;
    gfo_state* unused_scalar_clone=nullptr;
    check(gfo_state_with_active_attack(attack_board,&unused_scalars,&unused_scalar_clone)==0
        &&active(unused_scalar_clone)==attack_view&&view(unused_scalar_clone)==attack_initial,
        "unrendered MP/CP changes never enter public attack JSON or board view");gfo_state_destroy(unused_scalar_clone);
    attack_models[0]=235;attack.atk=99;
    check(active(attached)==attack_view&&view(attached)==attack_initial&&active(attack_board).find("\"attack\":null")!=std::string::npos,
        "model buffer is copied and attachment never alters source board or old fresh view");
    std::array<char,8> active_short{};active_short.fill('Q');const auto active_old=active_short;needed=787;
    check(gfo_state_active_attack_json(attached,active_short.data(),active_short.size(),&needed)==-4
        &&active_short==active_old&&needed==787,"short active JSON is fully atomic");
    check(gfo_state_active_attack_json(nullptr,active_short.data(),8,&needed)==-1
        &&gfo_state_active_attack_json(attached,nullptr,1,&needed)==-1
        &&gfo_state_active_attack_json(attached,active_short.data(),8,nullptr)==-1
        &&active_short==active_old&&needed==787,"active JSON NULL checks preserve companion outputs");
    for(bool missing_source:{false,true}){
        auto missing=attack;if(missing_source)missing.player_id=123;else missing.target_player_id=123;
        gfo_state* unknown=nullptr;
        check(gfo_state_with_active_attack(attack_board,&missing,&unknown)==0,"typed attach owns unknown endpoint evidence before fresh caller lookup");
        check(gfo_state_active_attack_json(unknown,active_short.data(),active_short.size(),&needed)==-2
            &&active_short==active_old&&needed==787,"unknown source or positive target fails caller lookup with every output unchanged");gfo_state_destroy(unknown);
    }
    attack.atk=0;
    for(unsigned error=0;error<12;++error){auto bad_attack=attack;
        if(error<4)bad_attack.reserved[error]=1;
        if(error==4)bad_attack.guardian=11;if(error==5)bad_attack.reversed=2;if(error==6)bad_attack.buying_model=297;
        if(error==7)bad_attack.player_id=9007199254740992ULL;if(error==8)bad_attack.target_player_id=9007199254740992ULL;
        if(error==9)bad_attack.atk=9007199254740992LL;if(error==10)bad_attack.hit_rate=-9007199254740992LL;
        if(error==11){bad_attack.model_count=std::numeric_limits<size_t>::max();bad_attack.models=reinterpret_cast<const uint16_t*>(uintptr_t{1});}
        sentinel=reinterpret_cast<gfo_state*>(uintptr_t{1});
        check(gfo_state_with_active_attack(attached,&bad_attack,&sentinel)==-2&&sentinel==reinterpret_cast<gfo_state*>(uintptr_t{1})
            &&active(attached)==attack_view&&view(attached)==attack_initial,"bad attack fields, padding or impossible count never read/commit partial output");
    }
    attack.models=nullptr;sentinel=reinterpret_cast<gfo_state*>(uintptr_t{1});
    check(gfo_state_with_active_attack(handle,&attack,&sentinel)==-1&&sentinel==reinterpret_cast<gfo_state*>(uintptr_t{1})
        &&gfo_state_with_active_attack(nullptr,nullptr,&sentinel)==-1&&gfo_state_with_active_attack(handle,nullptr,nullptr)==-1,"attach NULL input variants are atomic");
    std::array<uint16_t,19> long_models{};long_models.fill(6);attack={};attack.models=long_models.data();attack.model_count=long_models.size();
    gfo_state* long_attack=nullptr;check(gfo_state_with_active_attack(handle,&attack,&long_attack)==0,"public attack model columns are not limited to18 inventory slots");
    check(active(long_attack).find("\"target\":null")!=std::string::npos&&active(long_attack).find("\"buying_model\":null")!=std::string::npos,"wire target0 is hidden by display semantics, unlike source0");
    gfo_state_destroy(long_attack);
    for(uint16_t model:{uint16_t{0},uint16_t{297}}){attack.models=&model;attack.model_count=1;sentinel=reinterpret_cast<gfo_state*>(uintptr_t{1});
        check(gfo_state_with_active_attack(handle,&attack,&sentinel)==-2&&sentinel==reinterpret_cast<gfo_state*>(uintptr_t{1}),"attack models remain in official catalog namespace");}
    gfo_state* cleared=nullptr;check(gfo_state_with_active_attack(attached,nullptr,&cleared)==0,"nullable clear makes another independent board clone");
    gfo_state_destroy(attached);
    check(view(cleared)==attack_initial&&active(cleared).find("\"attack\":null")!=std::string::npos,"cleared clone survives destruction of its source");gfo_state_destroy(cleared);
    auto over_meta=meta;over_meta.is_over=1;gfo_state* over_board=nullptr;gfo_state* over_attack=nullptr;attack={};
    check(gfo_state_create(&over_meta,nullptr,0,nullptr,0,nullptr,0,&over_board)==0
        &&gfo_state_with_active_attack(over_board,&attack,&over_attack)==0&&active(over_attack).find("\"attack\":null")!=std::string::npos,
        "ending hides the independently attached attack without rejecting its DTO");gfo_state_destroy(over_attack);gfo_state_destroy(over_board);
    gfo_state_destroy(attack_board);gfo_state_destroy(handle); gfo_state_destroy(nullptr);
    std::cout << "official state C API passed\n";
}
