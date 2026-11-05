#pragma once
#include "godfield/official_state.hpp"

namespace godfield::official {
// Received HP presentation only. This does not execute a Match effect or
// produce death, revival, inventory, turn, guardian or random state changes.
enum class HpDisplayEventKind : std::uint8_t { deal_damage, boost_hp };
struct HpDisplayEvent {
    HpDisplayEventKind kind = HpDisplayEventKind::deal_damage;
    // Signed safe wire integer. Negative amounts are accepted as arithmetic
    // inputs, not asserted to be regular server-generated events.
    std::int64_t amount = 0;
    // Presence matters: explicit player 0 is a real roster ID; only nullopt
    // falls back to the caller-supplied current target. A raw wire playerId=0
    // sentinel must be normalized by the caller, never inferred here.
    std::optional<PlayerId> player_id;
    bool operator==(const HpDisplayEvent&) const = default;
};
struct HpPlayerView {
    PlayerId id = 0;
    std::int64_t hp = 0;
    bool operator==(const HpPlayerView&) const = default;
};
struct HpDisplayView {
    std::vector<HpPlayerView> players; // Input roster order, IDs and HP only.
    std::optional<PlayerId> current_target;
    bool operator==(const HpDisplayView&) const = default;
};
class HpDisplay {
public:
    // Validate the supplied snapshot, then retain only roster IDs and HP. Safe
    // initial HP outside0..99 is retained, as in the received client display;
    // clipping occurs only when an HP event is applied to that player.
    // No Fog/ending gate is applied to the received arithmetic. The result is
    // a caller-owned display context, not a public player observation.
    explicit HpDisplay(const SnapshotState& board, std::optional<PlayerId> current_target = {});
    // Target changes and event batches reject unknown/unsafe IDs atomically.
    // The caller resolves target changes from other received events. In
    // particular, a new self-use must not inherit an earlier attack's target.
    void set_current_target(std::optional<PlayerId> player_id);
    // Apply in received order; the whole batch commits only after every event
    // succeeds. dealDamage subtracts amount; boostHP adds amount; each clips
    // to 0..99 independently, matching the client eV resource helper.
    void apply(std::span<const HpDisplayEvent> events);
    [[nodiscard]] HpDisplayView view() const { return view_; }
private:
    HpDisplayView view_;
    [[nodiscard]] std::size_t require_player(PlayerId player_id) const;
    void apply_one(const HpDisplayEvent& event);
};
[[nodiscard]] std::string hp_display_json(const HpDisplayView& view);
}
