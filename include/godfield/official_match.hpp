#pragma once
#include "godfield/official_state.hpp"
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace godfield::official {
// Local executable subset. Public client arithmetic is source-checked; turn,
// gift and death scheduling below is an explicit experimental contract.
inline constexpr const char* match_ruleset = "official-execution-subset-v11";
inline constexpr const char* match_scope = "experimental:204-models;not-official-server-equivalence";
inline constexpr const char* bounce_target_rule = "experimental:uniform-all-alive-in-input-order;includes-self-and-teammates";
inline constexpr const char* chance_rule = "experimental:targetless-all-alive-enemies-in-input-order;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll;skip-dead;cancel-on-origin-death";
inline constexpr const char* counter_rule = "experimental:actual-hp-loss;survivors-only;selection-order-fifo-before-next-target;skip-dead-source-or-target;single-target;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll";
inline constexpr const char* resource_counter_rule = "experimental:mp-self-target-with-response;cp-target-incoming-source;debit-current-target-then-full-credit-current-source;resource-debt-no-new-counter";
inline constexpr const char* damage_effect_rule = "experimental:positive-nondark-remainder;current-source-side-effect-before-target-damage;counter-basis-target-damage-only";
inline constexpr const char* flash_rule = "experimental:positive-target-hp-loss-including-lethal;idempotent-until-explicit-cure;counter-to-incoming-source;one-card-response;no-tick";
inline constexpr const char* darkcloud_rule = "experimental:force-percent-hit-without-draw-at-target-activation;redirects-no-recheck;idempotent-until-all-cure;no-tick";
inline constexpr std::uint32_t match_snapshot_version = 11;
[[nodiscard]] bool match_supported(CardId id) noexcept;
[[nodiscard]] std::vector<CardId> match_catalog();
[[nodiscard]] int match_gift_weight() noexcept;
struct MatchConfig {
    std::uint32_t max_decisions = 512;
    std::uint8_t initial_hand = 9, hand_limit = 18;
    int initial_hp = 40, initial_mp = 10, initial_cp = 20;
    bool operator==(const MatchConfig&) const = default;
};
struct PlayerSetup {
    PlayerId id = 0;
    Team team = Team::solo;
    std::string name;
};
enum class MatchPhase : std::uint8_t { main, response, finished };
enum class CommandKind : std::uint8_t { play, defend, discard, pray };
struct Command {
    PlayerId actor = 0;
    CommandKind kind = CommandKind::play;
    std::vector<ItemToken> items;
    std::optional<PlayerId> target;
    bool operator==(const Command&) const = default;
};
struct RedirectFrame {
    PlayerId defender = 0;
    std::vector<CardId> models;
    std::vector<ItemToken> tokens; // Privileged consumption evidence for this response.
    PlayerId target = 0; // Derived reflection target or audited local bounce outcome.
    bool operator==(const RedirectFrame&) const = default;
};
struct EffectFrame {
    PlayerId origin = 0, source = 0, target = 0;
    std::vector<CardId> models;
    std::vector<ItemToken> tokens; // Privileged origin evidence, not part of the policy view.
    Category category = Category::weapons;
    Attribute element;
    std::int64_t power = 0;
    Ability ability = Ability::none;
    int amount = 0;
    PlayerId initial_target = 0;
    std::uint64_t redirect_rng = 0; // Privileged RNG at launch, for exact redirect replay.
    std::vector<RedirectFrame> redirects; // Reaction kind is derived from models and incoming payload.
    int hit_rate = 0; // Original display percentage; an active chance frame has already hit.
    bool is_counter = false;
    bool forced_hit = false; // DarkCloud bypassed the original percentage draw.
    bool operator==(const EffectFrame&) const = default;
};
// Non-recursive privileged launch anchor and accepted response journal. Current
// targets/cursor/models are derived again when the batch is restored.
struct ActionBatch {
    SnapshotState initial_state;
    PlayerId owner = 0;
    std::uint64_t rng = 0, decisions = 0, next_token = 0, chance_rolls = 0, chance_hits = 0;
    std::uint64_t counter_created = 0, counter_started = 0, counter_rolls = 0, counter_hits = 0;
    std::uint64_t chance_forced_hits = 0, counter_forced_hits = 0;
    Command attack;
    std::vector<CardId> models;
    std::vector<PlayerId> targets;
    std::size_t cursor = 0; // Next initial target; counter frames do not advance it.
    bool chance = false;
    std::size_t counters_created = 0;
    std::vector<Command> responses;
    bool operator==(const ActionBatch&) const = default;
};
struct MatchSnapshot {
    std::uint32_t version = match_snapshot_version;
    MatchConfig config;
    SnapshotState state;
    PlayerId owner = 0;
    std::optional<PlayerId> actor;
    MatchPhase phase = MatchPhase::finished;
    std::vector<EffectFrame> queue; // One payload; redirects transfer it without cloning/recasting.
    std::optional<ActionBatch> batch;
    std::vector<EffectFrame> counters; // Reserved FIFO, never part of a policy observation.
    std::vector<std::uint32_t> gifts;
    std::uint64_t rng = 0, decisions = 0, next_token = 0;
    std::uint64_t chance_rolls = 0, chance_hits = 0; // Actual percentage draws/successes, excluding forced hits.
    std::uint64_t counter_created = 0, counter_started = 0, counter_rolls = 0, counter_hits = 0;
    std::uint64_t chance_forced_hits = 0, counter_forced_hits = 0; // Activated without environmental RNG.
    bool terminated = false, truncated = false;
    std::vector<double> rewards; // Fixed player vector order, never actor-relative.
    std::vector<PlayerId> winners;
    bool operator==(const MatchSnapshot&) const = default;
};
struct MatchStep {
    MatchPhase phase = MatchPhase::finished;
    std::optional<PlayerId> actor;
    bool terminated = false, truncated = false;
    std::uint64_t decisions = 0;
    std::vector<double> rewards;
    std::vector<PlayerId> winners;
    bool operator==(const MatchStep&) const = default;
};
enum class MatchErrorCode { invalid, unsupported };
class MatchError : public std::runtime_error {
public:
    MatchErrorCode code;
    MatchError(MatchErrorCode c, const char* message) : std::runtime_error(message), code(c) {}
};
// Actual public-client resource update semantics, without implicit death/turns.
enum class Resource : std::uint8_t { hp, mp, cp };
void change_resource(PlayerState& player, Resource resource, std::int64_t delta);
class Match {
public:
    explicit Match(MatchConfig config = {}); // Initializes two solo players, seed42.
    void reset(std::span<const PlayerSetup> players, std::uint64_t seed);
    void load_scene(SnapshotState state, PlayerId owner, std::uint64_t seed);
    void restore(const MatchSnapshot& snapshot); // Validates completely, then replaces atomically.
    [[nodiscard]] const MatchSnapshot& snapshot() const noexcept { return data_; }
    [[nodiscard]] bool done() const noexcept { return data_.phase == MatchPhase::finished; }
    [[nodiscard]] bool legal(const Command& command) const noexcept;
    [[nodiscard]] Command sample_action(std::uint64_t& policy_rng) const;
    MatchStep step(const Command& command); // Errors leave all state, RNG and counters unchanged.
    [[nodiscard]] std::string observe(std::optional<PlayerId> viewer) const;
private:
    MatchConfig config_;
    MatchSnapshot data_;
};
}
