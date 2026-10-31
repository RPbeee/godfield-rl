#pragma once
#include "godfield/legacy_env.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace godfield::learning {
inline constexpr std::uint32_t checkpoint_version = 1;
inline constexpr std::size_t policy_feature_count = 256;
inline constexpr std::size_t value_feature_count = 32;
inline constexpr std::uint32_t max_candidates = 32;
using PolicyFeatures = std::array<double, policy_feature_count>;
using ValueFeatures = std::array<double, value_feature_count>;
struct Policy { PolicyFeatures weights{}; bool operator==(const Policy&) const = default; };
struct Model { Policy policy{}; ValueFeatures value{}; bool operator==(const Model&) const = default; };
enum class Opponent : std::uint32_t { random, snapshot };
struct TrainConfig {
    std::uint64_t seed = 42;
    std::uint32_t candidates = 12, batch_size = 64, snapshot_interval = 1024, max_decisions = 512;
    double learning_rate = 0.01, value_learning_rate = 0.01, entropy = 0.01, gamma = 1.0, clip_norm = 5.0;
    Opponent opponent = Opponent::snapshot;
    bool operator==(const TrainConfig&) const = default;
};
struct Candidate { legacy::Action action{}; PolicyFeatures features{}; bool operator==(const Candidate&) const = default; };
struct Decision {
    std::vector<Candidate> candidates;
    std::vector<double> probabilities;
    ValueFeatures value_features{};
    double value = 0;
    std::size_t selected = 0;
    std::uint8_t actor = 0;
};
struct Checkpoint {
    TrainConfig config{};
    Model model{};
    Policy opponent{};
    PolicyFeatures policy_m{}, policy_v{}, policy_gradient{};
    ValueFeatures value_m{}, value_v{}, value_gradient{};
    std::uint64_t episodes = 0, updates = 0, optimizer_steps = 0, rng = 0, pending_decisions = 0;
    std::uint64_t wins = 0, losses = 0, draws = 0, truncations = 0, decisions = 0, learned_decisions = 0, checksum = 0;
    std::uint32_t pending_episodes = 0;
    bool operator==(const Checkpoint&) const = default;
};
struct EpisodeResult {
    std::uint8_t learner_seat = 0;
    std::uint32_t decisions = 0, learning_decisions = 0;
    double reward = 0;
    bool terminated = false, truncated = false;
};
struct Evaluation {
    std::uint64_t games = 0, wins = 0, losses = 0, draws = 0, truncations = 0, decisions = 0, checksum = 0;
    double win_rate() const noexcept { return games ? static_cast<double>(wins) / games : 0; }
    bool operator==(const Evaluation&) const = default;
};
legacy::Config environment_config(std::uint32_t max_decisions = 512);
void validate_config(const TrainConfig& config);
Checkpoint initialize(TrainConfig config = {});
// Policy inputs use only the acting player's observation, proposed action and public seat.
PolicyFeatures action_features(const legacy::Observation& observation, legacy::Action action, std::uint8_t actor);
ValueFeatures state_features(const legacy::Observation& observation);
// Samples a deduplicated bank, not an exhaustive legal-action enumeration.
// Only policy_rng is advanced; environment RNG and future tickets stay untouched.
std::vector<Candidate> candidate_actions(const legacy::Env& env, std::uint64_t& policy_rng, std::uint32_t count);
std::vector<double> probabilities(const Policy& policy, const std::vector<Candidate>& candidates);
PolicyFeatures policy_gradient(const Decision& decision, double advantage, double entropy_coefficient);
Decision decide(const legacy::Env& env, const Model& model, std::uint64_t& policy_rng, std::uint32_t count, bool greedy = false);
double terminal_reward(const legacy::StepResult& result, std::uint8_t seat);
// One complete episode. Pending batch gradients are intentionally retained on return.
EpisodeResult train_episode(Checkpoint& checkpoint);
void train(Checkpoint& checkpoint, std::uint64_t additional_episodes);
// Seat-balanced evaluation against native hierarchical random; never updates training state.
Evaluation evaluate(const Checkpoint& checkpoint, std::uint64_t games, std::uint64_t seed = 1000000000, bool greedy = false);
void save_checkpoint(const Checkpoint& checkpoint, const std::filesystem::path& path);
Checkpoint load_checkpoint(const std::filesystem::path& path);
}
