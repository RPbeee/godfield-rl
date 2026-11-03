#pragma once
#include "godfield/official_state.hpp"
#include <array>
#include <variant>

namespace godfield::official {
// A deliberately partial client presentation reducer, never a Match checkpoint
// or an authoritative server-state/event interpreter.
struct NextAttack { ActiveAttackState attack; bool operator==(const NextAttack&) const = default; };
struct SetTargetPlayer { PlayerId player_id = 0; bool operator==(const SetTargetPlayer&) const = default; };
struct AdvanceGF { PlayerId player_id = 0; bool operator==(const AdvanceGF&) const = default; };
using ActiveDisplayEvent = std::variant<NextAttack,SetTargetPlayer,AdvanceGF>;
enum class DisplaySide : std::uint8_t { left, right };
struct ActiveDisplayView {
    std::int64_t gf = 0;
    bool reversed = false;
    std::array<std::optional<PlayerId>,2> panes;
    std::optional<DisplaySide> selected_side;
    std::optional<ActiveAttackView> attack;
    bool operator==(const ActiveDisplayView&) const = default;
};
class ActiveDisplay {
public:
    // Fresh initialization uses the head attack, or the supplied current turn
    // player for a board without an attack. Ending skips both identity lookups.
    ActiveDisplay(SnapshotState board, std::span<const ActiveAttackState> attacks = {}, PlayerId turn_player = 0);
    // Whole batch commits atomically. Event effects are limited to the panes,
    // attack cards/summary and GF; no HP/hand/curse/death/phase/RNG is generated.
    void apply(std::span<const ActiveDisplayEvent> events);
    [[nodiscard]] ActiveDisplayView view() const { return view_; }
private:
    SnapshotState context_; // Only roster IDs are retained; no private board data.
    ActiveDisplayView view_;
    void apply_one(const ActiveDisplayEvent& event);
    void require_player(PlayerId player_id) const;
};
[[nodiscard]] std::string active_display_json(const ActiveDisplayView& view);
}
