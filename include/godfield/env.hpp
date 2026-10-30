#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace godfield {
inline constexpr int hand_size = 9;
inline constexpr int action_count = 521;
inline constexpr int observation_size = 73;
inline constexpr const char* ruleset = "synthetic-neutral-duel-v1";
enum class Kind : std::uint8_t { empty, weapon, armor, heal, mana, spell_heal };
struct Card { Kind kind; int power; int cost; };
// Synthetic curriculum cards; these IDs are NOT official artifact IDs.
inline constexpr std::array<Card, 11> cards{{
    {Kind::empty, 0, 0}, {Kind::weapon, 3, 0}, {Kind::weapon, 6, 0},
    {Kind::weapon, 10, 0}, {Kind::armor, 3, 0}, {Kind::armor, 6, 0},
    {Kind::armor, 10, 0}, {Kind::heal, 5, 0}, {Kind::heal, 10, 0},
    {Kind::mana, 5, 0}, {Kind::spell_heal, 10, 6}
}};
struct Config { std::uint32_t max_decisions = 512; };
struct Player {
    int hp = 40;
    int mp = 20;
    std::array<std::uint8_t, hand_size> hand{};
};
enum class Phase : std::uint8_t { main, defense };
struct State {
    std::array<Player, 2> players{};
    std::uint8_t actor = 0;
    std::uint8_t attacker = 0;
    Phase phase = Phase::main;
    int pending_damage = 0;
    std::uint32_t decisions = 0;
    std::uint32_t turns = 0;
    bool terminated = false;
    bool truncated = false;
    int winner = -1;
};
struct StepResult {
    bool valid = false;
    std::uint8_t actor = 0; // Player that supplied the action, before transition.
    std::array<float, 2> rewards{}; // Fixed player coordinates, not next-actor relative.
    bool terminated = false;
    bool truncated = false;
};
struct ActionList {
    std::array<std::uint16_t, action_count> values{};
    std::size_t count = 0;
};
class Rng {
public:
    explicit Rng(std::uint64_t seed = 0) : state_(seed) {}
    std::uint64_t next() noexcept;
    std::uint32_t bounded(std::uint32_t bound) noexcept;
private:
    std::uint64_t state_;
};
class Env {
public:
    explicit Env(Config config = {});
    void reset(std::uint64_t seed) noexcept;
    const State& state() const noexcept { return state_; }
    bool done() const noexcept { return state_.terminated || state_.truncated; }
    bool legal(int action) const noexcept;
    ActionList legal_actions() const noexcept;
    std::array<std::uint8_t, action_count> action_mask() const noexcept;
    std::array<float, observation_size> observe() const noexcept;
    StepResult step(int action) noexcept;
private:
    Config config_;
    State state_{};
    Rng rng_{};
    std::uint16_t eligible_slots() const noexcept;
    void consume(std::uint16_t slots) noexcept;
    void refill(int player) noexcept;
    void next_turn(int player) noexcept;
};
}
