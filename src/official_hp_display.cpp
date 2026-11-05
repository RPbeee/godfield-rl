#include "godfield/official_hp_display.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace godfield::official {
std::size_t HpDisplay::require_player(PlayerId id) const {
    if (id > max_wire_integer) throw std::invalid_argument("unsafe HP display player");
    for (std::size_t i = 0; i < view_.players.size(); ++i)
        if (view_.players[i].id == id) return i;
    throw std::invalid_argument("unknown HP display player");
}
HpDisplay::HpDisplay(const SnapshotState& board, std::optional<PlayerId> current_target) {
    validate_state(board);
    for (const auto& player : board.players) view_.players.push_back({player.id,player.hp});
    set_current_target(current_target);
}
void HpDisplay::set_current_target(std::optional<PlayerId> id) {
    if (id) (void)require_player(*id);
    view_.current_target = id;
}
void HpDisplay::apply_one(const HpDisplayEvent& event) {
    constexpr auto limit = static_cast<std::int64_t>(max_wire_integer);
    if (event.amount < -limit || event.amount > limit)
        throw std::invalid_argument("unsafe HP display amount");
    std::int64_t delta;
    switch (event.kind) {
    case HpDisplayEventKind::deal_damage: delta = -event.amount; break;
    case HpDisplayEventKind::boost_hp: delta = event.amount; break;
    default: throw std::invalid_argument("unknown HP display event kind");
    }
    const auto target = event.player_id ? event.player_id : view_.current_target;
    if (!target) throw std::invalid_argument("HP display event needs a player or current target");
    auto& player = view_.players[require_player(*target)];
    // Initial HP and delta are safe wire integers; even their sum is bounded
    // by2*max_wire_integer, well inside int64. Each event clips its own result.
    player.hp = std::clamp(player.hp + delta,std::int64_t{0},std::int64_t{99});
}
void HpDisplay::apply(std::span<const HpDisplayEvent> events) {
    auto next = *this;
    for (const auto& event : events) next.apply_one(event);
    view_ = std::move(next.view_);
}
std::string hp_display_json(const HpDisplayView& view) {
    std::string out = "{\"schema\":\"official-hp-display-view-v1\",\"players\":[";
    for (std::size_t i = 0; i < view.players.size(); ++i) {
        if (i) out += ',';
        const auto& player = view.players[i];
        out += "{\"id\":" + std::to_string(player.id) + ",\"hp\":" + std::to_string(player.hp) + '}';
    }
    out += "],\"current_target\":";
    out += view.current_target ? std::to_string(*view.current_target) : "null";
    out += '}';
    return out;
}
}
