#pragma once
#include "godfield/official_rules.hpp"
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace godfield::official {
using PlayerId = std::uint64_t;
using ItemToken = std::uint64_t;
inline constexpr std::uint64_t max_wire_integer = 9007199254740991ULL;
inline constexpr std::size_t player_capacity = 9;
inline constexpr std::size_t visible_hand_capacity = 18; // Fresh renderer domain, not a server physical-hand rule.
enum class Team : std::uint8_t { solo, one, two, three, four };

struct ItemInstance {
    ItemToken token = 0;
    CardId model = 0;
    std::optional<CardId> fake_model;
    bool used = false;
    bool operator==(const ItemInstance&) const = default;
};
struct PlayerState {
    PlayerId id = 0;
    std::optional<std::string> user_id;
    std::string name;
    std::optional<std::string> alias;
    Team team = Team::solo;
    std::int64_t hp = 40, mp = 10, cp = 20;
    std::vector<Curse> curses;
    Guardian guardian = Guardian::none;
    std::int64_t confusion_count = 0;
    bool dead = false;
    std::vector<ItemInstance> items;
    bool operator==(const PlayerState&) const = default;
};
// Typed state for a decoded fresh snapshot; this is not a transition engine.
// Attacks/events, ratings, incremental history and action timing are not projected.
struct SnapshotState {
    std::vector<PlayerState> players;
    std::int64_t gf = 0;
    std::optional<std::int64_t> tiebreak_gf;
    bool is_over = false;
    std::uint64_t update_count = 0;
    bool operator==(const SnapshotState&) const = default;
};
struct StatusPane {
    std::int64_t hp = 0, mp = 0, cp = 0;
    std::vector<Curse> curses; // Visible icon set in canonical enum order.
    Guardian guardian = Guardian::none;
    bool confused = false;
    bool operator==(const StatusPane&) const = default;
};
struct HandItemView {
    ItemToken token = 0;
    CardId face = 0;
    CardId reference = 0; // Own tooltip: true model only after death/game over or without disguise.
    bool disguised = false, used = false;
    bool operator==(const HandItemView&) const = default;
};
struct PlayerView {
    PlayerId id = 0;
    std::string display_name; // Name available through normal/used-miracle panels, not raw identity.
    Team team = Team::solo; // Also visible through the used-miracle panel while fogged.
    bool dead = false, fogged = false;
    std::optional<StatusPane> status;
    std::vector<CardId> used_models; // Visible model IDs, sorted; no foreign physical tokens.
    bool operator==(const PlayerView&) const = default;
};
struct SnapshotView {
    std::optional<PlayerId> viewer;
    std::vector<PlayerView> players;
    std::vector<HandItemView> hand; // Stable visible-model order, used last; own inventory only.
    std::int64_t gf = 0;
    std::optional<std::int64_t> tiebreak_gf;
    bool is_over = false;
    bool operator==(const SnapshotView&) const = default;
};

// Received public display frame. Separate from board state and executable Match
// effects: IDs here are models, never inventory tokens or a future effect queue.
struct ActiveAttackState {
    PlayerId player_id = 0, target_player_id = 0;
    Guardian guardian = Guardian::none;
    std::vector<CardId> models;
    std::int64_t atk = 0, hit_rate = 0, mp = 0, cp = 0;
    CardId buying_model = 0;
    bool reversed = false;
    bool operator==(const ActiveAttackState&) const = default;
};
struct ActiveAttackCardView {
    CardId model = 0;
    std::optional<std::int64_t> atk, hit_rate, mp, cp;
    bool from_mp = false;
    std::optional<Curse> curse;
    bool operator==(const ActiveAttackCardView&) const = default;
};
struct ActiveAttackView {
    PlayerId source = 0;
    Guardian guardian = Guardian::none;
    std::vector<CardId> models;
    std::vector<ActiveAttackCardView> cards; // Visible special-card values, without hidden override-presence bits.
    bool reversed = false;
    std::vector<CardId> summary_models; // Client removes one leading207 only for a compound column.
    bool prayer = false, is_attack = false;
    std::optional<std::int64_t> power, hit_rate; // Present only for an actually drawn attack summary.
    std::optional<Category> incoming_category;
    Attribute incoming_element;
    std::optional<PlayerId> target; // Fresh caller displays a target only when wire ID>0.
    std::optional<CardId> buying_model; // Requires both a displayed target and a positive commodity ID.
    bool operator==(const ActiveAttackView&) const = default;
};

void validate_active_attack(const ActiveAttackState& attack);
// Only the head is inspected; no18-model hand bound is imposed on public frames.
// Ending suppresses the display before roster lookup. Otherwise the source and
// any positive target ID must exist, as required by the fresh client caller.
// This is not actor/timing authorization or a pending Match import.
[[nodiscard]] std::optional<ActiveAttackView> project_active_attack(const SnapshotState& board, std::span<const ActiveAttackState> attacks);
[[nodiscard]] std::string active_attack_json(const std::optional<ActiveAttackView>& view);

void validate_state(const SnapshotState& state);
[[nodiscard]] const PlayerState* find_player(const SnapshotState& state, PlayerId id) noexcept;
[[nodiscard]] std::optional<PlayerId> resolve_viewer(const SnapshotState& state, std::string_view user_id);
[[nodiscard]] bool is_enemy(const PlayerState& first, const PlayerState& second) noexcept;
[[nodiscard]] std::vector<PlayerId> alive_players(const SnapshotState& state);
[[nodiscard]] std::vector<PlayerId> enemy_players(const SnapshotState& state, PlayerId player);
// Actual client ending display, not an end-of-game decision. Can return dead teammates.
[[nodiscard]] std::vector<PlayerId> ending_display_candidates(const SnapshotState& state);
[[nodiscard]] SnapshotView project_view(const SnapshotState& state, std::optional<PlayerId> viewer);
[[nodiscard]] CardId face_model(const ItemInstance& item);
// These query only the viewer's displayed models. They do not validate actor,
// turn, target, exchange allocation, server acceptance or hidden true effects.
// Response flash is derived from the viewer's curses; the supplied flash bit is ignored.
[[nodiscard]] bool can_append_hand(const SnapshotState& state, PlayerId viewer, std::span<const ItemToken> selected, ItemToken next,
                                   std::optional<ResponseContext> response = std::nullopt);
[[nodiscard]] bool selection_ready(const SnapshotState& state, PlayerId viewer, std::span<const ItemToken> selected,
                                   std::optional<ResponseContext> response = std::nullopt);
[[nodiscard]] std::string view_json(const SnapshotView& view);
}
