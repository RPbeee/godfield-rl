#include "godfield/env.hpp"
#include "godfield/c_api.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
using namespace godfield;
void transitions() {
    bool attack_seen = false, heal_seen = false, mana_seen = false, spell_seen = false;
    for (std::uint64_t seed = 0; seed < 200; ++seed) {
        Env env; env.reset(seed);
        for (int action = 0; action < action_count; ++action) {
            env.reset(seed);
            if (!env.legal(action)) continue;
            const auto before = env.state();
            const int actor = before.actor;
            const int enemy = 1 - actor;
            const auto result = env.step(action);
            check(result.valid && result.actor == actor, "action actor");
            check(env.state().decisions == 1, "decision count");
            if (action >= 512 || action == 0) {
                check(env.state().actor == enemy && env.state().phase == Phase::main, "pass/discard ends turn");
                if (action >= 512) for (int i = 0; i < hand_size; ++i)
                    if (i != action - 512) check(env.state().players[actor].hand[i] == before.players[actor].hand[i], "discard only replaces selected slot");
                continue;
            }
            int first = 0;
            while (!(action & (1 << first))) ++first;
            const auto card = cards[before.players[actor].hand[first]];
            if (card.kind == Kind::weapon) {
                attack_seen = true;
                int damage = 0;
                for (int i = 0; i < hand_size; ++i) if (action & (1 << i)) {
                    damage += cards[before.players[actor].hand[i]].power;
                    check(env.state().players[actor].hand[i] == 0, "attack refill deferred");
                }
                check(env.state().actor == enemy && env.state().phase == Phase::defense, "defender acts");
                check(env.state().pending_damage == damage, "combined attack power");
                const auto defending = env;
                const auto defenses = env.legal_actions();
                for (std::size_t k = 0; k < defenses.count; ++k) {
                    auto branch = defending;
                    const auto defense = defenses.values[k];
                    int armor = 0;
                    for (int i = 0; i < hand_size; ++i) if (defense & (1 << i)) armor += cards[before.players[enemy].hand[i]].power;
                    const auto r = branch.step(defense);
                    const int hp = std::max(0, 40 - std::max(0, damage - armor));
                    check(branch.state().players[enemy].hp == hp, "damage after selected armor");
                    check(branch.state().actor == enemy && branch.state().phase == Phase::main, "defender gets next main turn");
                    check(r.terminated == (hp == 0), "lethal detection");
                    check(r.rewards[actor] == (hp == 0 ? 1.f : 0.f), "fixed-seat win reward");
                    check(r.rewards[enemy] == -r.rewards[actor], "zero-sum rewards");
                }
            } else {
                check(env.state().actor == enemy, "utility ends turn");
                check(env.state().players[actor].mp == 20 - card.cost + (card.kind == Kind::mana ? card.power : 0), "utility MP");
                check(env.state().players[actor].hp == 40 + (card.kind == Kind::mana ? 0 : card.power), "utility HP");
                heal_seen |= card.kind == Kind::heal;
                mana_seen |= card.kind == Kind::mana;
                spell_seen |= card.kind == Kind::spell_heal;
            }
        }
    }
    check(attack_seen && heal_seen && mana_seen && spell_seen, "all supported effects exercised");
}
void rollouts() {
    std::uint64_t wins = 0, truncations = 0;
    for (std::uint64_t seed = 0; seed < 10000; ++seed) {
        Env a, b; a.reset(seed); b.reset(seed);
        Rng policy(seed + 123);
        while (!a.done()) {
            check(a.observe() == b.observe() && a.action_mask() == b.action_mask(), "seed replay");
            const auto obs = a.observe();
            for (float x : obs) check(std::isfinite(x) && x >= 0 && x <= 1, "normalized observation");
            const auto list = a.legal_actions();
            const auto mask = a.action_mask();
            std::array<bool, action_count> seen{};
            for (std::size_t j = 0; j < list.count; ++j) {
                check(!seen[list.values[j]], "no duplicate legal actions"); seen[list.values[j]] = true;
            }
            for (int action = -1; action <= action_count; ++action) {
                const bool expected = action >= 0 && action < action_count && mask[action];
                check(a.legal(action) == expected, "mask and validation agree");
                if (!expected && action == -1) {
                    auto rejected = a;
                    check(!rejected.step(action).valid, "illegal rejected");
                    check(rejected.observe() == a.observe(), "illegal leaves state intact");
                    auto branch = a;
                    rejected.step(list.values[0]); branch.step(list.values[0]);
                    check(rejected.observe() == branch.observe(), "illegal leaves RNG intact");
                }
            }
            const auto action = list.values[policy.bounded(static_cast<std::uint32_t>(list.count))];
            auto ra = a.step(action); auto rb = b.step(action);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards, "replay transition");
            check(!(ra.terminated && ra.truncated), "terminal vs truncation");
            for (const auto& p : a.state().players) {
                check(p.hp >= 0 && p.hp <= 99 && p.mp >= 0 && p.mp <= 99, "resource bounds");
                for (auto id : p.hand) check(id < cards.size(), "card bounds");
            }
        }
        wins += a.state().terminated; truncations += a.state().truncated;
        check(a.legal_actions().count == 0, "no terminal actions");
        const auto obs = a.observe();
        check(!a.step(0).valid && a.observe() == obs, "terminal freeze");
    }
    check(wins > 0 && wins + truncations == 10000, "outcome accounting");
    Env limited(Config{1});
    check(limited.step(0).truncated && !limited.state().terminated, "time limit has no winner");
}
void privacy() {
    // Different opponent cards with identical public information must produce
    // identical policy input and masks. Only own hand and public stats enter observe().
    Env a; a.reset(8);
    auto b = a;
    auto& other = const_cast<State&>(b.state()).players[1 - b.state().actor];
    for (auto& card : other.hand) card = card == 1 ? 2 : 1;
    check(a.observe() == b.observe() && a.action_mask() == b.action_mask(), "opponent hand hidden");
}
void edge_cases() {
    Env a;
    auto& s = const_cast<State&>(a.state()); // Controlled fixtures, no production mutation API.
    auto& p = s.players[s.actor];
    p.hand.fill(1);
    check(a.legal_actions().count == 521 && a.legal(511), "all weapon subsets fit fixed action storage");
    a.step(511);
    check(a.state().pending_damage == 27, "nine-card attack");
    auto& defense = const_cast<State&>(a.state()).players[a.state().actor];
    defense.hand.fill(4);
    check(a.legal_actions().count == 512 && !a.legal(512), "all defense subsets, no discard");
    a.step(511);
    check(a.state().players[a.state().actor].hp == 40, "full block");
    auto& utility = const_cast<State&>(a.state()).players[a.state().actor];
    utility.hand.fill(10); utility.mp = 5;
    check(!a.legal(1), "spell rejected with insufficient MP");
    utility.mp = 6; utility.hp = 95;
    check(a.legal(1) && !a.legal(3), "utility is singleton");
    const int seat = a.state().actor;
    a.step(1);
    check(a.state().players[seat].hp == 99 && a.state().players[seat].mp == 0, "healing cap and exact cost");
    Env last(Config{2});
    auto& terminal = const_cast<State&>(last.state());
    terminal.players[terminal.actor].hand.fill(3);
    terminal.players[1 - terminal.actor].hp = 1;
    last.step(511);
    const auto result = last.step(0);
    check(result.terminated && !result.truncated, "lethal on decision limit takes precedence");
}
void batch_api() {
    check(gf_abi_version() == 1 && gf_action_count() == action_count && gf_observation_size() == observation_size, "ABI dimensions");
    check(!gf_create(0, 2) && !gf_create(2, 0), "invalid config");
    auto* batch = gf_create(2, 2); check(batch != nullptr, "batch creation");
    std::vector<float> obs(2 * observation_size), before(obs.size());
    std::vector<std::uint8_t> masks(2 * action_count);
    std::uint8_t actors[2], term[2], trunc[2]; float rewards[4]; std::int32_t actions[2]{0, -1};
    check(gf_reset_at(batch, 0, 42) == 0 && gf_reset_at(batch, 1, 43) == 0, "batch seeds");
    check(gf_observe(batch, 1, obs.data(), masks.data(), actors) == -2, "reject wrong buffer count");
    check(gf_observe(batch, 2, before.data(), masks.data(), actors) == 0, "batch observation");
    check(gf_step(batch, 2, actions, rewards, term, trunc) == -3, "batch rejects illegal row");
    gf_observe(batch, 2, obs.data(), masks.data(), actors);
    check(obs == before, "batch rejection atomic");
    actions[1] = 0;
    check(gf_step(batch, 2, actions, rewards, term, trunc) == 0, "batch step");
    check(gf_step(batch, 2, actions, rewards, term, trunc) == 0 && trunc[0] && trunc[1], "batch truncation");
    actions[0] = actions[1] = -1;
    check(gf_step(batch, 2, actions, rewards, term, trunc) == 0, "done rows ignore actions");
    check(std::all_of(rewards, rewards + 4, [](float x) { return x == 0; }), "no repeated reward");
    gf_destroy(batch); gf_destroy(nullptr);
}
int main() {
    transitions(); rollouts(); privacy(); edge_cases(); batch_api();
    std::cout << "All checks passed (10000 seeded rollouts + transition/ABI/privacy checks).\n";
}
