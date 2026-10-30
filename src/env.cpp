#include "godfield/env.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace godfield {
std::uint64_t Rng::next() noexcept {
    auto z = (state_ += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}
std::uint32_t Rng::bounded(std::uint32_t bound) noexcept {
    if (!bound) return 0;
    const auto threshold = (std::uint64_t{0} - bound) % bound;
    for (;;) { auto x = next(); if (x >= threshold) return x % bound; }
}
Env::Env(Config config) : config_(config) {
    if (!config_.max_decisions) throw std::invalid_argument("max_decisions must be positive");
    reset(0);
}
void Env::refill(int player) noexcept {
    // Uniform with replacement over the explicitly synthetic card pool.
    for (auto& id : state_.players[player].hand)
        if (!id) id = static_cast<std::uint8_t>(1 + rng_.bounded(cards.size() - 1));
}
void Env::reset(std::uint64_t seed) noexcept {
    rng_ = Rng(seed);
    state_ = State{};
    refill(0);
    refill(1);
    state_.actor = state_.attacker = static_cast<std::uint8_t>(rng_.bounded(2));
}
std::uint16_t Env::eligible_slots() const noexcept {
    std::uint16_t slots = 0;
    const auto kind = state_.phase == Phase::main ? Kind::weapon : Kind::armor;
    for (int i = 0; i < hand_size; ++i)
        if (cards[state_.players[state_.actor].hand[i]].kind == kind) slots |= 1u << i;
    return slots;
}
bool Env::legal(int action) const noexcept {
    if (done() || action < 0 || action >= action_count) return false;
    if (action == 0) return true;
    const auto& p = state_.players[state_.actor];
    if (action >= 512) return state_.phase == Phase::main && p.hand[action - 512] != 0;
    const auto slots = static_cast<std::uint16_t>(action);
    if ((slots & eligible_slots()) == slots) return true;
    if (state_.phase != Phase::main || !std::has_single_bit(slots)) return false;
    const auto& c = cards[p.hand[std::countr_zero(slots)]];
    return (c.kind == Kind::heal || c.kind == Kind::mana || c.kind == Kind::spell_heal)
        && c.cost <= p.mp;
}
ActionList Env::legal_actions() const noexcept {
    ActionList out;
    if (done()) return out;
    out.values[out.count++] = 0;
    const auto slots = eligible_slots();
    for (auto subset = slots; subset; subset = (subset - 1) & slots)
        out.values[out.count++] = subset;
    if (state_.phase == Phase::main) {
        const auto& p = state_.players[state_.actor];
        for (int i = 0; i < hand_size; ++i) {
            const auto& c = cards[p.hand[i]];
            if ((c.kind == Kind::heal || c.kind == Kind::mana || c.kind == Kind::spell_heal)
                    && c.cost <= p.mp) out.values[out.count++] = 1u << i;
            if (p.hand[i]) out.values[out.count++] = 512 + i;
        }
    }
    return out;
}
std::array<std::uint8_t, action_count> Env::action_mask() const noexcept {
    std::array<std::uint8_t, action_count> mask{};
    const auto actions = legal_actions();
    for (std::size_t i = 0; i < actions.count; ++i) mask[actions.values[i]] = 1;
    return mask;
}
std::array<float, observation_size> Env::observe() const noexcept {
    std::array<float, observation_size> obs{};
    const auto& me = state_.players[state_.actor];
    const auto& other = state_.players[1 - state_.actor];
    obs[0] = me.hp / 99.f; obs[1] = me.mp / 99.f;
    obs[2] = other.hp / 99.f; obs[3] = other.mp / 99.f;
    obs[4] = state_.phase == Phase::main; obs[5] = state_.phase == Phase::defense;
    obs[6] = state_.pending_damage / 90.f;
    obs[7] = static_cast<float>(state_.decisions) / config_.max_decisions;
    obs[8] = state_.actor == state_.attacker;
    obs[9] = std::count_if(other.hand.begin(), other.hand.end(), [](auto id) { return id != 0; }) / 9.f;
    for (int i = 0; i < hand_size; ++i) {
        const auto& c = cards[me.hand[i]];
        const int offset = 10 + i * 7;
        if (c.kind != Kind::empty) obs[offset + static_cast<int>(c.kind) - 1] = 1.f;
        obs[offset + 5] = c.power / 10.f;
        obs[offset + 6] = c.cost / 20.f;
    }
    return obs;
}
void Env::consume(std::uint16_t slots) noexcept {
    for (int i = 0; i < hand_size; ++i)
        if (slots & (1u << i)) state_.players[state_.actor].hand[i] = 0;
}
void Env::next_turn(int player) noexcept {
    state_.actor = state_.attacker = static_cast<std::uint8_t>(player);
    state_.phase = Phase::main;
    state_.pending_damage = 0;
    ++state_.turns;
}
StepResult Env::step(int action) noexcept {
    StepResult result;
    result.actor = state_.actor;
    result.terminated = state_.terminated;
    result.truncated = state_.truncated;
    if (!legal(action)) return result; // Invalid actions do not alter state or RNG.
    result.valid = true;
    const int actor = state_.actor;
    auto& p = state_.players[actor];
    if (state_.phase == Phase::defense) {
        int defense = 0;
        for (int i = 0; i < hand_size; ++i)
            if (action & (1u << i)) defense += cards[p.hand[i]].power;
        consume(static_cast<std::uint16_t>(action));
        p.hp = std::max(0, p.hp - std::max(0, state_.pending_damage - defense));
        if (p.hp == 0) {
            state_.terminated = true;
            state_.winner = 1 - actor;
            result.rewards[actor] = -1.f;
            result.rewards[1 - actor] = 1.f;
        } else {
            refill(state_.attacker);
            refill(actor);
        }
        next_turn(actor);
    } else if (action == 0) {
        next_turn(1 - actor);
    } else if (action >= 512) {
        p.hand[action - 512] = 0;
        refill(actor);
        next_turn(1 - actor);
    } else {
        const auto first = cards[p.hand[std::countr_zero(static_cast<unsigned>(action))]];
        if (first.kind == Kind::weapon) {
            int damage = 0;
            for (int i = 0; i < hand_size; ++i)
                if (action & (1u << i)) damage += cards[p.hand[i]].power;
            consume(static_cast<std::uint16_t>(action));
            state_.pending_damage = damage;
            state_.phase = Phase::defense;
            state_.actor = 1 - actor;
        } else {
            p.mp -= first.cost;
            if (first.kind == Kind::mana) p.mp = std::min(99, p.mp + first.power);
            else p.hp = std::min(99, p.hp + first.power);
            consume(static_cast<std::uint16_t>(action));
            refill(actor);
            next_turn(1 - actor);
        }
    }
    ++state_.decisions;
    if (!state_.terminated && state_.decisions >= config_.max_decisions) state_.truncated = true;
    result.terminated = state_.terminated;
    result.truncated = state_.truncated;
    return result;
}
}
