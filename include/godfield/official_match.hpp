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
inline constexpr const char* match_ruleset = "official-execution-subset-v4";
inline constexpr const char* match_scope = "experimental:156-models;not-official-server-equivalence";
inline constexpr std::uint32_t match_snapshot_version = 4;
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
struct ReflectionFrame {
    PlayerId defender = 0;
    std::vector<CardId> models;
    std::vector<ItemToken> tokens; // Privileged consumption evidence for this response.
    bool operator==(const ReflectionFrame&) const = default;
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
    std::vector<ReflectionFrame> reflections;
    bool operator==(const EffectFrame&) const = default;
};
struct MatchSnapshot {
    std::uint32_t version = match_snapshot_version;
    MatchConfig config;
    SnapshotState state;
    PlayerId owner = 0;
    std::optional<PlayerId> actor;
    MatchPhase phase = MatchPhase::finished;
    std::vector<EffectFrame> queue; // One payload; reflections transfer it without cloning/recasting.
    std::vector<std::uint32_t> gifts;
    std::uint64_t rng = 0, decisions = 0, next_token = 0;
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
