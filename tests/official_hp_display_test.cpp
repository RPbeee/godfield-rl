#include "godfield/official_hp_display.hpp"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace godfield::official;
void check(bool value,const char* message) {
    if (!value) { std::cerr << "FAIL official HP display: " << message << '\n'; std::exit(1); }
}
template<class F> void rejects(F&& function,const char* message) {
    bool caught = false;
    try { function(); } catch (const std::invalid_argument&) { caught = true; }
    check(caught,message);
}
SnapshotState board() {
    SnapshotState state;
    for (PlayerId id : {PlayerId{0},PlayerId{1},PlayerId{7}}) {
        PlayerState player; player.id = id; state.players.push_back(player);
    }
    return state;
}
void apply_event(HpDisplay& display,const HpDisplayEvent& event) {
    const std::array events{event}; display.apply(events);
}
void unchanged_on_failure(HpDisplay& display,const std::vector<HpDisplayEvent>& events) {
    const auto before = display.view(); const auto json = hp_display_json(before);
    rejects([&] { display.apply(events); },"invalid received HP batch must reject");
    check(display.view() == before && hp_display_json(display.view()) == json,
          "failed received HP batch committed a prefix");
}

void v66_received_absorb_event_order() {
    // wire-game-updates-v66-absorb.json update2: player1 at HP40 receives
    // dealDamage12 (wire fallback target1), then explicit boostHP12/player1.
    // This checks received display arithmetic, not Match's body/death order.
    auto state = board(); HpDisplay display(state,1);
    apply_event(display,{HpDisplayEventKind::deal_damage,12,{}});
    check(display.view().players[1].hp == 28,"v66 received damage12 renders HP40 to28");
    const auto after_damage = display;
    apply_event(display,{HpDisplayEventKind::boost_hp,12,PlayerId{1}});
    check(display.view().players[1].hp == 40 && display.view().players[0].hp == 40
          && display.view().players[2].hp == 40,"v66 boost12 restores only player1 to40");
    check(after_damage.view().players[1].hp == 28 && state.players[1].hp == 40,
          "received display owns its HP state and leaves the supplied board/copy unchanged");
    HpDisplay batch(state,1);
    const std::array events{HpDisplayEvent{HpDisplayEventKind::deal_damage,12,{}},
                           HpDisplayEvent{HpDisplayEventKind::boost_hp,12,PlayerId{1}}};
    batch.apply(events);
    check(batch.view() == display.view(),"v66 combined batch matches ordered event checkpoints");
}

void v66_twice_and_following_self_recovery_context() {
    // wire-game-updates-v66-twice.json update2: self14 targets player2,
    // while the later CPU191 boostHP5 omits playerId and targets player1.
    // The caller interprets advanceGF/useAttackItems to update this context;
    // this reducer is not a decoder for those non-HP events.
    SnapshotState state;
    PlayerState cpu; cpu.id=1;
    PlayerState viewer; viewer.id=2;
    state.players={cpu,viewer};
    HpDisplay display(state,2);
    apply_event(display,{HpDisplayEventKind::deal_damage,3,{}});
    check(display.view().players[1].hp == 37,"first received self14 hit reaches37");
    display.set_current_target(2); // nextAttack14/setTarget2 before hit2.
    apply_event(display,{HpDisplayEventKind::deal_damage,3,{}});
    check(display.view().players[1].hp == 34,"second received self14 hit reaches34");
    display.set_current_target(1); // Caller resolves CPU191 implicit self.
    apply_event(display,{HpDisplayEventKind::boost_hp,5,{}});
    check(display.view().players[0].hp == 45 && display.view().players[1].hp == 34,
          "following implicit-self recovery must not inherit the viewer2 target");
}

void v68_received_low_hp_absorb_order() {
    // wire-game-updates-v68.json update5->6, viewer1 has no revival passive.
    // Both received amounts are7 although initial HP is3. A damage event's
    // nominal amount must not be replaced by the clipped HP loss of3.
    auto state=board();state.players[1].hp=3;HpDisplay display(state,1);
    apply_event(display,{HpDisplayEventKind::deal_damage,7,{}});
    check(display.view().players[1].hp==0,"v68 low-HP damage clips3 to0");
    apply_event(display,{HpDisplayEventKind::boost_hp,7,PlayerId{1}});
    check(display.view().players[1].hp==7,"v68 nominal boost7 follows damage and reaches7");
    check(state.players[1].hp==3,"v68 received projection keeps its source checkpoint immutable");
}

void explicit_zero_and_current_target_presence() {
    HpDisplay display(board(),7);
    apply_event(display,{HpDisplayEventKind::deal_damage,3,PlayerId{0}});
    check(display.view().players[0].hp == 37 && display.view().players[2].hp == 40,
          "explicit player0 must not fall back to current target7");
    apply_event(display,{HpDisplayEventKind::deal_damage,4,{}});
    check(display.view().players[2].hp == 36,"absent player falls back to target7");
    display.set_current_target(0);
    apply_event(display,{HpDisplayEventKind::boost_hp,2,{}});
    check(display.view().players[0].hp == 39 && display.view().current_target == 0,
          "present current target0 is a real player");
    display.set_current_target({});
    unchanged_on_failure(display,{{HpDisplayEventKind::boost_hp,1,{}}});
    apply_event(display,{HpDisplayEventKind::boost_hp,1,PlayerId{1}});
    check(display.view().players[1].hp == 41,"explicit ID works without any fallback context");
    const auto before = display.view();
    rejects([&] { display.set_current_target(99); },"unknown fallback player rejected");
    rejects([&] { display.set_current_target(max_wire_integer+1); },"unsafe fallback player rejected");
    check(display.view() == before,"failed target change must retain prior context");
}

void clipping_and_synthetic_signed_boundaries() {
    HpDisplay display(board(),0);
    constexpr auto limit = static_cast<std::int64_t>(max_wire_integer);
    // Signed/huge amounts are synthetic arithmetic boundaries, not captured
    // normal event examples or evidence for server-generated negative damage.
    apply_event(display,{HpDisplayEventKind::boost_hp,limit,{}});
    check(display.view().players[0].hp == 99,"large safe heal clips99 without overflow");
    apply_event(display,{HpDisplayEventKind::deal_damage,limit,{}});
    check(display.view().players[0].hp == 0,"large safe damage clips0");
    apply_event(display,{HpDisplayEventKind::deal_damage,-limit,{}});
    check(display.view().players[0].hp == 99,"signed damage follows source subtraction");
    apply_event(display,{HpDisplayEventKind::boost_hp,-limit,{}});
    check(display.view().players[0].hp == 0,"signed boost follows source addition");
    apply_event(display,{HpDisplayEventKind::boost_hp,0,{}});
    check(display.view().players[0].hp == 0,"zero is accepted without changing HP");
    auto cap = board(); cap.players[0].hp = 95;
    HpDisplay damage_first(cap,0),heal_first(cap,0);
    const std::array d_h{HpDisplayEvent{HpDisplayEventKind::deal_damage,12,{}},
                         HpDisplayEvent{HpDisplayEventKind::boost_hp,12,{}}};
    const std::array h_d{d_h[1],d_h[0]}; damage_first.apply(d_h); heal_first.apply(h_d);
    check(damage_first.view().players[0].hp == 95 && heal_first.view().players[0].hp == 87,
          "synthetic cap probe preserves each event's independent clip and order");
    auto outside = board(); outside.players[0].hp = 100; outside.players[1].hp = -5;
    HpDisplay received(outside,0);
    check(received.view().players[0].hp == 100 && received.view().players[1].hp == -5,
          "safe initial HP outside0..99 is retained until its first event");
    apply_event(received,{HpDisplayEventKind::deal_damage,2,{}});
    apply_event(received,{HpDisplayEventKind::boost_hp,2,PlayerId{1}});
    check(received.view().players[0].hp == 98 && received.view().players[1].hp == 0,
          "initial100 minus2 reaches98 while initial negative HP clips only after its event");
    auto extreme = board(); extreme.players[0].hp = limit; extreme.players[1].hp = -limit;
    HpDisplay bounded(extreme);
    apply_event(bounded,{HpDisplayEventKind::boost_hp,limit,PlayerId{0}});
    apply_event(bounded,{HpDisplayEventKind::boost_hp,-limit,PlayerId{1}});
    check(bounded.view().players[0].hp == 99 && bounded.view().players[1].hp == 0,
          "sum of two safe signed input extremes is safe before clipping");
}

void invalid_events_rollback_the_entire_batch() {
    HpDisplay display(board(),1);
    const auto valid = HpDisplayEvent{HpDisplayEventKind::deal_damage,12,PlayerId{1}};
    const std::vector<HpDisplayEvent> bad{
        {HpDisplayEventKind::boost_hp,1,PlayerId{99}},
        {HpDisplayEventKind::boost_hp,1,max_wire_integer+1},
        {static_cast<HpDisplayEventKind>(255),1,PlayerId{1}},
        {HpDisplayEventKind::boost_hp,static_cast<std::int64_t>(max_wire_integer)+1,PlayerId{1}},
        {HpDisplayEventKind::deal_damage,-static_cast<std::int64_t>(max_wire_integer)-1,PlayerId{1}},
        {HpDisplayEventKind::deal_damage,std::numeric_limits<std::int64_t>::min(),PlayerId{1}},
        {HpDisplayEventKind::boost_hp,std::numeric_limits<std::int64_t>::max(),PlayerId{1}}
    };
    for (const auto& event : bad) unchanged_on_failure(display,{valid,event});
    unchanged_on_failure(display,{{HpDisplayEventKind::boost_hp,1,PlayerId{99}}});
}

void constructor_validation_and_view_ownership() {
    auto duplicate = board(); duplicate.players[1].id = 0;
    rejects([&] { HpDisplay invalid(duplicate); },"duplicate initial IDs rejected");
    auto unsafe = board(); unsafe.players[0].id = max_wire_integer+1;
    rejects([&] { HpDisplay invalid(unsafe); },"unsafe initial ID rejected");
    auto hp = board(); hp.players[0].hp = static_cast<std::int64_t>(max_wire_integer)+1;
    rejects([&] { HpDisplay invalid(hp); },"unsafe initial HP rejected");
    rejects([&] { HpDisplay invalid(board(),99); },"unknown initial target rejected");
    HpDisplay display(board(),1); auto copy = display.view(); copy.players[1].hp = 0; copy.current_target = 7;
    check(display.view().players[1].hp == 40 && display.view().current_target == 1,
          "view returns an owning copy rather than mutable reducer state");
    const auto expected = std::string{"{\"schema\":\"official-hp-display-view-v1\",\"players\":[{\"id\":0,\"hp\":40},{\"id\":1,\"hp\":40},{\"id\":7,\"hp\":40}],\"current_target\":1}"};
    check(hp_display_json(display.view()) == expected,"HP projection serializes only its dedicated schema/IDs/HP/context");
}

void private_state_noninterference_and_no_lifecycle_generation() {
    const auto plain = board(); auto rich = plain;
    rich.gf = 99; rich.update_count = 123; rich.tiebreak_gf = 1; rich.is_over = true;
    for (auto& player : rich.players) {
        player.name = "private-name"; player.user_id = "private-user"; player.alias = "private-alias";
        player.mp = 99; player.cp = 0; player.curses = {Curse::fog,Curse::dream};
        player.guardian = Guardian::neptune; player.confusion_count = 3; player.dead = true;
        player.items = {{1,208,{},false},{2,112,{},false},{3,135,CardId{135},false}};
    }
    const auto original = rich;
    HpDisplay a(plain,1),b(rich,1);
    check(a.view() == b.view(),"private board fields,ending,Fog and dead flags do not affect HP context");
    const std::array events{HpDisplayEvent{HpDisplayEventKind::deal_damage,99,PlayerId{1}},
                           HpDisplayEvent{HpDisplayEventKind::boost_hp,5,PlayerId{1}}};
    a.apply(events); b.apply(events);
    check(a.view() == b.view() && b.view().players[1].hp == 5 && rich == original,
          "HP0 then received heal neither invokes passives/death nor alters supplied private state");
    const auto json = hp_display_json(b.view());
    for (const auto* hidden : {"private-","items","token","guardian","dead","curse","mp","cp","gf"})
        check(json.find(hidden) == std::string::npos,"HP JSON retained unrelated/private board data");
}

void copy_resume_empty_batch_and_input_nonmutation() {
    HpDisplay batch(board(),7),single = batch;
    const std::vector<HpDisplayEvent> events{
        {HpDisplayEventKind::deal_damage,5,PlayerId{0}},
        {HpDisplayEventKind::boost_hp,70,{}},
        {HpDisplayEventKind::deal_damage,99,PlayerId{1}},
        {HpDisplayEventKind::boost_hp,7,PlayerId{1}}
    };
    const auto original = events;
    batch.apply(events);
    for (const auto& event : events) {
        auto resumed = single; apply_event(resumed,event); apply_event(single,event);
        check(resumed.view() == single.view(),"copied context cannot resume at a received event boundary");
    }
    check(batch.view() == single.view() && events == original,"batch/order execution or caller-owned events changed");
    const auto before = batch.view(); batch.apply({});
    check(batch.view() == before,"empty batch changed HP/context");
}

void pinned_client_hp_vectors(const char* path) {
    // Reuse outputs of the actual pinned client's c5.eV method. These are
    // client arithmetic vectors, not additional official game observations.
    std::ifstream input(path);
    check(bool(input),"open pinned resource vectors");
    std::string line; std::size_t rows=0,hp_rows=0;
    while (std::getline(input,line)) {
        if (line.empty() || line.front()=='#') continue;
        std::istringstream fields(line); char kind;
        std::int64_t hp,mp,cp,resource,delta,expected_hp,expected_mp,expected_cp;
        check(bool(fields>>kind>>hp>>mp>>cp>>resource>>delta>>expected_hp>>expected_mp>>expected_cp)
              && kind=='R',"parse pinned resource row");
        fields>>std::ws; check(fields.eof(),"no trailing resource vector fields");
        ++rows;
        if (resource!=0) continue; // MP/CP debt cascade is outside this API.
        ++hp_rows;
        auto state=board(); state.players[0].hp=hp;
        HpDisplay boost(state,0),damage(state,0);
        apply_event(boost,{HpDisplayEventKind::boost_hp,delta,{}});
        apply_event(damage,{HpDisplayEventKind::deal_damage,-delta,{}});
        check(boost.view().players[0].hp==expected_hp && damage.view()==boost.view(),
              "both HP event branches match actual pinned client arithmetic");
        check(boost.view().players[1].hp==40 && boost.view().players[2].hp==40,
              "client-vector projection changes only its resolved player");
    }
    check(rows==7898 && hp_rows==2617,"all pinned HP vectors were compared");
}
}
int main(int argc,char** argv) {
    check(argc==2,"expected pinned resource vector path");
    v66_received_absorb_event_order();
    v66_twice_and_following_self_recovery_context();
    v68_received_low_hp_absorb_order();
    explicit_zero_and_current_target_presence();
    clipping_and_synthetic_signed_boundaries();
    invalid_events_rollback_the_entire_batch();
    constructor_validation_and_view_ownership();
    private_state_noninterference_and_no_lifecycle_generation();
    copy_resume_empty_batch_and_input_nonmutation();
    pinned_client_hp_vectors(argv[1]);
    std::cout << "official HP display: 10 groups; 2617 pinned HP vectors x2 branches; received v66 HP40->28->40 and v68 HP3->0->7; explicit caller target changes,not a full event decoder\n";
}
