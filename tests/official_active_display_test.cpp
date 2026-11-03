#include "godfield/official_display.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace godfield::official;
void check(bool value,const char* message) {
    if(!value){std::cerr<<"FAIL official active display: "<<message<<'\n';std::exit(1);}
}
template<class E,class F>void throws_as(F&& function,const char* message) {
    bool caught=false;try{function();}catch(const E&){caught=true;}check(caught,message);
}
SnapshotState board() {
    SnapshotState state;state.gf=17;state.update_count=42;
    for(PlayerId id:{PlayerId{0},PlayerId{7},PlayerId{22}}) {
        PlayerState player;player.id=id;player.name="P"+std::to_string(id);
        player.user_id="private-user-"+std::to_string(id);player.hp=40;player.mp=10;player.cp=20;
        state.players.push_back(std::move(player));
    }
    return state;
}
ActiveAttackState frame(std::vector<CardId> models={6},bool reversed=false) {
    ActiveAttackState attack;attack.player_id=0;attack.target_player_id=7;
    attack.models=std::move(models);attack.reversed=reversed;return attack;
}
ActiveDisplay fresh(const SnapshotState& state,const ActiveAttackState& attack,PlayerId turn=0) {
    const std::array<ActiveAttackState,1> attacks{attack};return ActiveDisplay(state,attacks,turn);
}
void apply_event(ActiveDisplay& display,const ActiveDisplayEvent& event) {
    const std::array<ActiveDisplayEvent,1> events{event};display.apply(events);
}
void unchanged_on_failure(ActiveDisplay& display,const std::vector<ActiveDisplayEvent>& events) {
    const auto before=display.view();const auto json=active_display_json(before);
    throws_as<std::invalid_argument>([&]{display.apply(events);},"invalid event batch must reject");
    check(display.view()==before && active_display_json(display.view())==json,"failed batch partially committed display state");
}

void fresh_initialization_and_zero_target_distinction() {
    const auto state=board();ActiveDisplay empty(state,{},7);const auto idle=empty.view();
    check(idle.gf==17 && !idle.reversed && idle.panes[0]==7 && !idle.panes[1]
          && !idle.selected_side && !idle.attack,"fresh no-attack initializes only the turn player's left pane");
    auto attack=frame({5});attack.target_player_id=0;attack.buying_model=207;
    auto display=fresh(state,attack,22);const auto initial=display.view();
    check(initial.panes[0]==0 && !initial.panes[1] && !initial.selected_side && initial.attack
          && !initial.attack->target && !initial.attack->buying_model,"fresh wire target0 and commodity are omitted despite real player0");
    apply_event(display,SetTargetPlayer{0});
    check(display.view().selected_side==DisplaySide::left && display.view().attack->target==0
          && !display.view().attack->buying_model,"incremental explicit target0 is a real source player rather than the fresh sentinel");
    attack.target_player_id=7;attack.buying_model=235;
    const auto foreign=fresh(state,attack).view();
    check(foreign.panes[0]==0 && foreign.panes[1]==7 && foreign.selected_side==DisplaySide::right
          && foreign.attack->target==7 && foreign.attack->buying_model==235,"fresh positive target restores target and commodity");
    throws_as<std::invalid_argument>([&]{ActiveDisplay bad(state,{},99);},"unknown initial turn player rejected");
    check(fresh(state,attack,99).view()==foreign,"fresh attack does not look up an unused turn player");
    auto missing_zero=state;missing_zero.players.erase(missing_zero.players.begin());
    throws_as<std::invalid_argument>([&]{ActiveDisplay bad(missing_zero);},"initial turn0 still requires a real roster entry");
}

void next_attack_clears_both_panes_and_ignored_wire_fields() {
    auto first=frame({5});first.buying_model=235;auto display=fresh(board(),first);
    apply_event(display,SetTargetPlayer{22});
    auto next=frame({6,9},true);next.player_id=7;next.target_player_id=999;next.buying_model=296;
    apply_event(display,NextAttack{next});const auto view=display.view();
    check(view.gf==17 && view.reversed && !view.panes[0] && view.panes[1]==7
          && !view.selected_side && view.attack && view.attack->source==7 && view.attack->models==std::vector<CardId>({6,9})
          && !view.attack->target && !view.attack->buying_model,"nextAttack clears old target/commodity and creates only the reversed source pane");
    auto canonical=next;canonical.target_player_id=0;canonical.buying_model=0;
    canonical.atk=-123;canonical.hit_rate=-456;canonical.mp=789;canonical.cp=-987;
    auto other=fresh(board(),first);apply_event(other,NextAttack{canonical});
    check(other.view()==view && active_display_json(other.view())==active_display_json(view),
          "unused nextAttack target/commodity and unpainted numbers leaked into the display");
    const auto before=next;ActiveDisplayEvent event=NextAttack{next};apply_event(display,event);
    check(std::get<NextAttack>(event).attack==before,"nextAttack mutated the caller's received frame");
    apply_event(display,NextAttack{frame({})});
    check(display.view().attack->prayer && display.view().panes[0]==0 && !display.view().panes[1]
          && !display.view().reversed && !display.view().selected_side,"prayer nextAttack remains a source-only pane and resets reversal");
}

void target_updates_keep_existing_opposite_pane_and_commodity() {
    auto attack=frame({5});attack.buying_model=235;auto display=fresh(board(),attack);
    apply_event(display,SetTargetPlayer{0});auto view=display.view();
    check(view.panes[0]==0 && view.panes[1]==7 && view.selected_side==DisplaySide::left
          && view.attack->target==0 && view.attack->buying_model==235,"self-target selects source without removing the previous opposite pane or commodity");
    apply_event(display,SetTargetPlayer{22});view=display.view();
    check(view.panes[1]==22 && view.selected_side==DisplaySide::right && view.attack->target==22
          && view.attack->buying_model==235,"foreign target replaces only opposite pane and retains commodity");
    attack.reversed=true;auto reverse=fresh(board(),attack);
    apply_event(reverse,SetTargetPlayer{0});
    check(reverse.view().panes[0]==7 && reverse.view().panes[1]==0
          && reverse.view().selected_side==DisplaySide::right,"reversed self-target selects the right source side");
    apply_event(reverse,SetTargetPlayer{22});
    check(reverse.view().panes[0]==22 && reverse.view().panes[1]==0
          && reverse.view().selected_side==DisplaySide::left,"reversed foreign target replaces the left opposite pane");
    unchanged_on_failure(reverse,{SetTargetPlayer{99}});
    unchanged_on_failure(reverse,{SetTargetPlayer{max_wire_integer+1}});
}

void advance_gf_resets_attack_and_left_source() {
    auto attack=frame({5},true);attack.buying_model=235;auto display=fresh(board(),attack);
    apply_event(display,AdvanceGF{22});const auto view=display.view();
    check(view.gf==18 && !view.reversed && view.panes[0]==22 && !view.panes[1]
          && !view.selected_side && !view.attack,"advanceGF increments once and removes all old attack/target/buy/reversal data");
    apply_event(display,SetTargetPlayer{0});
    check(display.view().panes[0]==22 && display.view().panes[1]==0
          && display.view().selected_side==DisplaySide::right && !display.view().attack,
          "target update can select real0 against a turn pane without inventing an attack");
    apply_event(display,AdvanceGF{0});
    check(display.view().gf==19 && display.view().panes[0]==0 && !display.view().panes[1],"advanceGF recognizes real player0");
    unchanged_on_failure(display,{AdvanceGF{99}});
    auto exhausted=board();exhausted.gf=static_cast<std::int64_t>(max_wire_integer);
    ActiveDisplay full(exhausted);
    unchanged_on_failure(full,{NextAttack{frame()},AdvanceGF{7}});
}

void ordered_batches_copy_resume_and_ending_events() {
    auto attack=frame({5});attack.buying_model=207;
    const std::vector<ActiveDisplayEvent> events{SetTargetPlayer{0},NextAttack{frame({112},true)},
        SetTargetPlayer{22},SetTargetPlayer{0},AdvanceGF{7},NextAttack{frame({58})},SetTargetPlayer{7}};
    auto combined=fresh(board(),attack),sequential=combined;
    combined.apply(events);
    for(const auto& event:events) {
        auto resumed=sequential;apply_event(resumed,event);apply_event(sequential,event);
        check(resumed.view()==sequential.view(),"copy resumed reducer disagrees at an event boundary");
    }
    check(combined.view()==sequential.view() && active_display_json(combined.view())==active_display_json(sequential.view()),
          "whole-batch and ordered single-event execution differ");
    const auto before=combined.view();combined.apply({});check(combined.view()==before,"empty event batch changed the display");
    auto ended=board();ended.is_over=true;auto unknown=frame();unknown.player_id=99;unknown.target_player_id=88;
    auto display=fresh(ended,unknown,77);
    check(display.view().gf==17 && !display.view().attack && !display.view().panes[0] && !display.view().panes[1],
          "fresh ending suppresses both attack and turn identity lookups");
    apply_event(display,SetTargetPlayer{0});
    check(!display.view().panes[0] && display.view().panes[1]==0 && display.view().selected_side==DisplaySide::right && !display.view().attack,"aT updates the opposite pane even without a source; later overlay errors are outside this component");
    apply_event(display,NextAttack{frame({6})});
    check(display.view().attack && display.view().panes[0]==0,"received nextAttack is not blocked by the fresh ending gate");
    apply_event(display,SetTargetPlayer{7});apply_event(display,AdvanceGF{22});
    check(display.view().gf==18 && display.view().panes[0]==22 && !display.view().attack,
          "received target/advance events remain enabled after an ending initialization");
}

void whole_batch_validation_and_atomic_rollback() {
    auto attack=frame({5},true);attack.buying_model=207;auto display=fresh(board(),attack);
    const auto valid=ActiveDisplayEvent{NextAttack{frame({112})}};
    for(unsigned error=0;error<8;++error) {
        auto bad=frame({6});
        if(error==0)bad.player_id=99;
        if(error==1)bad.player_id=max_wire_integer+1;
        if(error==2)bad.target_player_id=max_wire_integer+1;
        if(error==3)bad.models={0};
        if(error==4)bad.models={297};
        if(error==5)bad.buying_model=999;
        if(error==6)bad.guardian=static_cast<Guardian>(11);
        if(error==7)bad.atk=static_cast<std::int64_t>(max_wire_integer)+1;
        unchanged_on_failure(display,{valid,NextAttack{bad}});
    }
    unchanged_on_failure(display,{AdvanceGF{7},SetTargetPlayer{99}});
    unchanged_on_failure(display,{valid,AdvanceGF{max_wire_integer+1}});
    auto overflow=frame({58});overflow.atk=static_cast<std::int64_t>(max_wire_integer);
    overflow.models.insert(overflow.models.end(),11,231);
    const auto before=display.view();const std::vector<ActiveDisplayEvent> events{valid,NextAttack{overflow}};
    throws_as<std::overflow_error>([&]{display.apply(events);},"checked composition overflow must reject the batch");
    check(display.view()==before,"late numeric overflow committed an earlier event");
    auto malformed=board();malformed.players.push_back(malformed.players.front());
    throws_as<std::invalid_argument>([&]{ActiveDisplay invalid(malformed);},"initial roster duplicate IDs rejected");
}

void roster_private_data_and_unused_raw_noninterference() {
    const auto state=board();auto hidden=state;
    hidden.update_count=999;hidden.tiebreak_gf=50;
    for(auto& p:hidden.players) {
        p.user_id="different-secret-user";p.alias="hidden alias";p.name="changed name";p.team=Team::two;
        p.hp=1;p.mp=99;p.cp=0;p.guardian=Guardian::moon;p.confusion_count=999;
        p.curses={Curse::heaven,Curse::fog,Curse::dream};
        p.items={{0,58,6,false},{7,235,{},true}};
    }
    const auto original=hidden;
    ActiveDisplay first(state,{},7),second(hidden,{},7);
    const std::vector<ActiveDisplayEvent> events{NextAttack{frame({6,9})},SetTargetPlayer{22},AdvanceGF{0},NextAttack{frame({187})},SetTargetPlayer{7}};
    first.apply(events);second.apply(events);
    check(first.view()==second.view() && hidden==original,"private board fields affected public event reduction or caller input was mutated");
    const auto json=active_display_json(first.view());
    check(json.find("private")==std::string::npos && json.find("secret")==std::string::npos && json.find("hidden alias")==std::string::npos
          && json.find("\"items\"")==std::string::npos && json.find("\"confusion_count\"")==std::string::npos,
          "display serializer leaked retained raw/private board fields");
    check(json.starts_with("{\"schema\":\"official-active-display-view-v1\",\"gf\":18,")
          && json.find("\"panes\":{\"left\":0,\"right\":7}")!=std::string::npos,
          "display uses its own schema and sanitized panes");
}

void head_only_large_columns_and_strict_initialization() {
    auto bad=frame({0});bad.player_id=max_wire_integer+1;
    const std::array<ActiveAttackState,2> attacks{frame({6}),bad};
    ActiveDisplay display(board(),attacks,99);
    check(display.view().attack && display.view().attack->models==std::vector<CardId>{6},"fresh display inspected a tail frame or unused turn player");
    auto large=frame(std::vector<CardId>(19,6));large.target_player_id=999;
    apply_event(display,NextAttack{large});
    check(display.view().attack->cards.size()==19 && display.view().attack->power==19 && !display.view().attack->target,
          "received columns use no physical18-card cap and do not restore event target");
    auto ended=board();ended.is_over=true;
    const std::array<ActiveAttackState,1> malformed{bad};
    throws_as<std::invalid_argument>([&]{ActiveDisplay invalid(ended,malformed);},"ending skips identity lookup but not inspected-head strict wire validation");
    throws_as<std::invalid_argument>([&]{ActiveDisplay invalid(ended,{},max_wire_integer+1);},"unsafe turn ID rejected even when fresh ending hides it");
    auto maximum=board();maximum.gf=static_cast<std::int64_t>(max_wire_integer)-1;ActiveDisplay last(maximum);
    apply_event(last,AdvanceGF{0});check(last.view().gf==static_cast<std::int64_t>(max_wire_integer),"last safe GF increment is accepted");
    unchanged_on_failure(last,{AdvanceGF{0}});
}
}
int main() {
    fresh_initialization_and_zero_target_distinction();
    next_attack_clears_both_panes_and_ignored_wire_fields();
    target_updates_keep_existing_opposite_pane_and_commodity();
    advance_gf_resets_attack_and_left_source();
    ordered_batches_copy_resume_and_ending_events();
    whole_batch_validation_and_atomic_rollback();
    roster_private_data_and_unused_raw_noninterference();
    head_only_large_columns_and_strict_initialization();
    std::cout<<"Official active display passed (8 boundary/noninterference groups; partial received UI state, no authoritative replay).\n";
}
