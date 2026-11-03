#include "godfield/official_display.hpp"
#include <stdexcept>
#include <string_view>
#include <utility>

namespace godfield::official {
void ActiveDisplay::require_player(PlayerId id) const {
    if (id>max_wire_integer || !find_player(context_,id)) throw std::invalid_argument("unknown received display player");
}
ActiveDisplay::ActiveDisplay(SnapshotState board,std::span<const ActiveAttackState> attacks,PlayerId turn_player) {
    validate_state(board);
    for(const auto& player:board.players){PlayerState identity;identity.id=player.id;context_.players.push_back(std::move(identity));}
    if (turn_player>max_wire_integer) throw std::invalid_argument("unsafe display turn player");
    view_.gf=board.gf;
    view_.attack=project_active_attack(board,attacks);
    if (board.is_over) return;
    if (!view_.attack) { require_player(turn_player); view_.panes[0]=turn_player; return; }
    const auto& attack=*view_.attack; view_.reversed=attack.reversed;
    const unsigned source=view_.reversed?1:0;
    view_.panes[source]=attack.source;
    if (attack.target) {
        const auto target=*attack.target;
        const auto side=target==attack.source?source:1-source;
        view_.panes[side]=target;view_.selected_side=static_cast<DisplaySide>(side);
    }
}
void ActiveDisplay::apply_one(const ActiveDisplayEvent& event) {
    if (const auto* next=std::get_if<NextAttack>(&event)) {
        validate_active_attack(next->attack);
        // nextAttack invokes fu, not the fresh caller's target/commodity restore.
        auto frame=next->attack;frame.target_player_id=0;frame.buying_model=0;
        auto projected=project_active_attack(context_,std::span{&frame,1}); // No fresh ending gate in the receiver.
        ActiveDisplayView changed;changed.gf=view_.gf;changed.attack=std::move(projected);
        changed.reversed=changed.attack->reversed;
        changed.panes[changed.reversed?1:0]=changed.attack->source;
        view_=std::move(changed);return;
    }
    if (const auto* target=std::get_if<SetTargetPlayer>(&event)) {
        require_player(target->player_id);
        const unsigned source=view_.reversed?1:0;
        const auto side=view_.panes[source] && *view_.panes[source]==target->player_id?source:1-source;
        view_.panes[side]=target->player_id;
        view_.selected_side=static_cast<DisplaySide>(side);
        if (view_.attack) view_.attack->target=target->player_id;
        return;
    }
    const auto& advance=std::get<AdvanceGF>(event);
    require_player(advance.player_id);
    if (view_.gf>=static_cast<std::int64_t>(max_wire_integer)) throw std::invalid_argument("display GF exhausted");
    ActiveDisplayView changed;changed.gf=view_.gf+1;changed.panes[0]=advance.player_id;
    view_=std::move(changed);
}
void ActiveDisplay::apply(std::span<const ActiveDisplayEvent> events) {
    auto next=*this;
    for (const auto& event:events) next.apply_one(event);
    view_=std::move(next.view_);
}
std::string active_display_json(const ActiveDisplayView& view) {
    std::string out="{\"schema\":\"official-active-display-view-v1\",\"gf\":"+std::to_string(view.gf);
    out+=",\"reversed\":";out+=view.reversed?"true":"false";
    const auto pane=[&](std::optional<PlayerId> value){out+=value?std::to_string(*value):"null";};
    out+=",\"panes\":{\"left\":";pane(view.panes[0]);out+=",\"right\":";pane(view.panes[1]);out+="},\"selected_side\":";
    if(!view.selected_side)out+="null";else out+=*view.selected_side==DisplaySide::left?"\"left\"":"\"right\"";
    out+=",\"attack\":";
    auto frame=active_attack_json(view.attack);
    constexpr std::string_view prefix="{\"schema\":\"official-active-attack-view-v1\",\"attack\":";
    if(!frame.starts_with(prefix))throw std::logic_error("unexpected active attack serializer schema");
    frame.erase(0,prefix.size());frame.pop_back();out+=frame;out+='}';return out;
}
}
