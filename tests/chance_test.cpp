#include "godfield/legacy_env.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace godfield::legacy;
void check(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL chance: " << message << '\n'; std::exit(1); }
}
Action attack(int base = 0, std::uint16_t extras = 0, int target = 1) {
    return {ActionKind::attack, static_cast<std::uint8_t>(base), extras, static_cast<std::uint8_t>(target)};
}
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend, no_slot, slots, no_slot}; }
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0);
    std::copy(ids.begin(), ids.end(), p.hand.begin());
    p.hand_count = static_cast<std::uint8_t>(ids.size());
    p.pending_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy,
            Config config = Config{512, 10, false, true, true}) {
    Env env(config);
    auto snapshot = env.snapshot();
    snapshot.state = State{};
    snapshot.scripted_draws.fill(0); snapshot.draw_count = snapshot.draw_index = 0;
    snapshot.scripted_hits.fill(0); snapshot.hit_count = snapshot.hit_index = 0;
    hand(snapshot.state.players[0], own); hand(snapshot.state.players[1], enemy);
    check(env.restore(snapshot), "restore valid chance fixture");
    return env;
}
void script(Env& env, std::initializer_list<CardId> draws, std::initializer_list<std::uint8_t> rolls) {
    check(env.script_draws({draws.begin(), draws.size()}), "install supported draw tape");
    check(env.script_hit_rolls({rolls.begin(), rolls.size()}), "install valid hit tape");
}
void reject(Env& env, Action action, const char* message) {
    const auto original = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid, message);
    check(env.snapshot() == original, "invalid action preserves both tapes and RNG");
}
void metadata_and_profiles() {
    check(chance_card_count == 12 && chance_catalog().size() == 12, "pinned chance allowlist size");
    int weight = 0, count25 = 0, count50 = 0, count75 = 0;
    for (const auto& entry : chance_catalog()) {
        check(find_chance_card(entry.card.id) == &entry, "chance ID lookup");
        check(!find_card(entry.card.id) && !find_element_card(entry.card.id)
            && !find_miracle(entry.card.id), "chance IDs disjoint from prior pools");
        check(entry.card.kind == Kind::weapon && !entry.card.discardable && entry.card.boost == 0,
            "simple chance weapons have no erased booster/discard role");
        check(entry.element >= Element::fire && entry.element <= Element::stone, "only supported colored chance weapons");
        weight += entry.card.weight;
        count25 += entry.hit_rate == 25; count50 += entry.hit_rate == 50; count75 += entry.hit_rate == 75;
    }
    check(weight == 12 && count25 == 2 && count50 == 3 && count75 == 7, "pinned weights and percentages");
    check(find_chance_card(113)->card.defense == 6 && find_chance_card(113)->element == Element::wood,
        "chance dual-role weapon retains DEF6 and wood element");
    for (CardId id : {CardId{0}, CardId{5}, CardId{58}, CardId{101}, CardId{102},
                      CardId{103}, CardId{104}, CardId{111}, CardId{117}, CardId{65535}})
        check(!find_chance_card(id), "unsupported effects have no chance fallback");
    bool threw = false;
    try { Env invalid(Config{512, 10, false, false, true}); }
    catch (const std::invalid_argument&) { threw = true; }
    check(threw, "chance requires element semantics explicitly enabled");
    auto chance = fixture({100}, {123});
    auto combined = fixture({100, 216}, {123}, Config{512, 10, true, true, true});
    check(std::string_view(chance.profile()) == chance_ruleset
        && std::string_view(combined.profile()) == chance_miracles_ruleset, "profiles distinguish miracle flag");
    check(chance.snapshot().version == 35, "shared counter extensions snapshot schema version");

    for (const Config config : {Config{}, Config{512, 10, true}, Config{512, 10, false, true},
                                Config{512, 10, true, true}}) {
        Env disabled(config);
        const auto original = disabled.snapshot();
        const CardId chance_draw[] = {100};
        const std::uint8_t hit_roll[] = {1};
        check(!disabled.script_draws(chance_draw) && disabled.snapshot() == original, "disabled chance draw rejected atomically");
        check(!disabled.script_hit_rolls(hit_roll) && disabled.snapshot() == original,
            "disabled chance tape rejected atomically");
        check(disabled.script_hit_rolls({}), "empty chance tape may be cleared when disabled");
        auto invalid = original;
        invalid.state.players[invalid.state.actor].hand[0] = 100;
        check(!disabled.restore(invalid) && disabled.snapshot() == original, "same-config chance hand rejected when disabled");
        invalid = original; invalid.scripted_hits[0] = 1; invalid.hit_count = 1;
        check(!disabled.restore(invalid) && disabled.snapshot() == original, "same-config hit tape rejected when disabled");
        invalid = original; invalid.state.last_chance = LastChance{100, 0, 1, 75, true};
        check(!disabled.restore(invalid) && disabled.snapshot() == original, "disabled public chance event rejected");
    }
}
void boundary_outcomes_and_consumption() {
    for (CardId id : {CardId{100}, CardId{109}, CardId{110}}) {
        const auto& card = *find_chance_card(id);
        for (int roll : {1, card.hit_rate, card.hit_rate + 1, 100}) {
            auto env = fixture({id}, {120});
            script(env, {5}, {static_cast<std::uint8_t>(roll), 100});
            const auto before_rng = env.snapshot().rng_state;
            const auto result = env.step(attack());
            const bool hit = roll <= card.hit_rate;
            check(result.valid && result.chance_attempt && result.chance_hit == hit, "threshold hit/miss event result");
            check(env.snapshot().hit_index == 1, "one attack consumes exactly one hit outcome");
            check(env.state().last_chance == LastChance{id, 0, 1, static_cast<std::uint8_t>(card.hit_rate), hit},
                "last event records public artifact, seats, percent and outcome");
            check(env.state().players[1].hp == 40 && env.state().players[1].hand[0] == 120,
                "damage and defender consumption do not occur before response");
            if (hit) {
                check(env.state().phase == Phase::response && env.state().actor == 1,
                    "hit waits for one defense response");
                check(env.state().pending.hit_rate == card.hit_rate && env.state().pending.amount == card.card.attack
                    && env.state().pending.element == card.element, "pending hit preserves public attack data");
                check(env.state().players[0].hand_count == 0 && env.state().players[0].pending_deals == 1
                    && env.snapshot().draw_index == 0, "hit consumes weapon and defers acquisition");
                check(env.snapshot().rng_state == before_rng, "scripted hit does not consume future seeded RNG");
                Env resumed(Config{512, 10, false, true, true});
                check(resumed.restore(env.snapshot()), "already-rolled pending hit restorable");
                const auto resolved = env.step(defend());
                const auto replayed = resumed.step(defend());
                check(resolved.valid && replayed.valid && !resolved.chance_attempt && !resolved.chance_hit,
                    "response does not make another chance attempt");
                check(env.snapshot() == resumed.snapshot() && env.snapshot().hit_index == 1,
                    "restored response does not reroll");
                check(env.state().players[1].hp == 40 - card.card.attack, "hit applies full power after acceptance");
            } else {
                check(env.state().phase == Phase::main && env.state().actor == 1 && env.state().turns == 1,
                    "miss skips response and ends the normal turn");
                check(env.state().pending == Pending{} && env.state().players[0].pending_deals == 0,
                    "miss clears pending damage and completed gift budget");
                check(env.state().players[1].hp == 40, "miss does not damage defender");
            }
            check(env.state().players[0].hand_count == 1 && env.state().players[0].hand[0] == 5
                && env.snapshot().draw_index == 1, "both hit and miss eventually replace consumed weapon once");
            check(env.state().last_chance.hit == hit && env.snapshot().hit_index == 1,
                "public outcome survives ordinary turn completion");
        }
    }
}
void targeting_and_legality() {
    auto self = fixture({100}, {123});
    auto enemy = self;
    script(self, {5}, {1}); script(enemy, {5}, {1});
    check(self.step(attack(0, 0, 0)).valid && enemy.step(attack(0, 0, 1)).valid,
        "both valid target inputs accepted for legacy all-enemy chance attack");
    check(self.snapshot() == enemy.snapshot() && self.state().pending.target == 1
        && self.state().players[0].hp == 40, "chance attack normalizes self selection to enemy rather than self-harm");

    auto invalid = fixture({100, 9, 84}, {123});
    script(invalid, {5}, {1});
    reject(invalid, attack(0, 2), "chance base forbids ordinary boost extras");
    reject(invalid, attack(0, 4), "chance base forbids colored boost extras");
    reject(invalid, attack(1, 1), "chance weapon cannot be appended as a booster");
    reject(invalid, attack(0, 0, 2), "chance target still rejects nonexistent seat");
    reject(invalid, {ActionKind::discard, no_slot, 1, no_slot}, "chance weapon cannot be discarded");
    reject(invalid, {ActionKind::pray}, "chance weapon in hand prevents pray");

    auto regular = fixture({58}, {123});
    script(regular, {5}, {100});
    check(regular.step(attack()).valid && regular.snapshot().hit_index == 0,
        "guaranteed regular attack does not consume hit tape");
    check(regular.state().pending.hit_rate == 0 && regular.state().last_chance == LastChance{},
        "guaranteed attack keeps historical zero-percent sentinel and no chance event");

    auto defense = fixture({69}, {113});
    script(defense, {5, 120}, {100});
    defense.step(attack());
    check(defense.options().defense_slots == 1 && defense.step(defend(1)).valid,
        "wood chance weapon DEF6 may defend stone attack");
    check(defense.state().players[1].hp == 40 && defense.snapshot().hit_index == 0
        && defense.state().last_chance == LastChance{}, "defensive use of chance artifact does not roll to defend");
}
void tapes_snapshots_and_privacy() {
    auto env = fixture({100}, {123});
    script(env, {5}, {1, 100});
    const auto original = env.snapshot();
    for (std::uint8_t bad : {std::uint8_t{0}, std::uint8_t{101}, std::uint8_t{255}}) {
        const std::uint8_t tape[] = {bad};
        check(!env.script_hit_rolls(tape) && env.snapshot() == original, "invalid percentage outcome prefix rejected atomically");
    }
    std::array<std::uint8_t, 65> oversized; oversized.fill(1);
    check(!env.script_hit_rolls(oversized) && env.snapshot() == original, "oversized hit tape rejected atomically");
    auto invalid = original; invalid.hit_index = 3;
    check(!env.restore(invalid) && env.snapshot() == original, "hit tape index checked");
    invalid = original; invalid.scripted_hits[63] = 1;
    check(!env.restore(invalid) && env.snapshot() == original, "unused hit tape tail must be zero");
    invalid = original; invalid.version = 3;
    check(!env.restore(invalid) && env.snapshot() == original, "previous snapshot schema rejected");

    auto hidden = original; hidden.scripted_hits[0] = 100; hidden.scripted_hits[1] = 1;
    hidden.rng_state ^= 0x9e3779b97f4a7c15ULL;
    Env other(Config{512, 10, false, true, true}); check(other.restore(hidden), "restore different future rolls");
    check(env.observe() == other.observe() && env.options() == other.options(), "future hit outcomes and RNG are hidden");
    check(env.legal(attack()) == other.legal(attack()), "future hit outcomes do not affect current legality");

    check(env.step(attack()).valid, "pending hit fixture");
    const auto pending = env.snapshot();
    invalid = pending; invalid.state.pending.hit_rate = 50;
    check(!env.restore(invalid) && env.snapshot() == pending, "pending percent must match public chance event");
    invalid = pending; invalid.state.last_chance.hit = false;
    check(!env.restore(invalid) && env.snapshot() == pending, "a pending chance attack cannot have a miss event");
    invalid = pending; invalid.state.last_chance.id = 101;
    check(!env.restore(invalid) && env.snapshot() == pending, "excluded artifact cannot become a public chance event");

    auto public_miss = fixture({100}, {123});
    script(public_miss, {120}, {100, 1});
    public_miss.step(attack());
    const auto last = public_miss.state().last_chance;
    check(public_miss.step({ActionKind::pray}).valid && public_miss.state().last_chance == last
        && public_miss.snapshot().hit_index == 1, "normal later action retains public event without consuming future roll");
}
void observations_and_cutoffs() {
    auto env = fixture({100, 110, 58}, {123});
    script(env, {5}, {1});
    auto obs = env.observe();
    check(obs[186] == .75f && obs[187] == .25f && obs[188] == 0.f && obs[203] == 1.f,
        "own percentage rates and chance-enabled flag represented");
    check(obs[204] == 0.f && obs[202] == 0.f, "no event or pending rate before chance attack");
    env.step(attack()); obs = env.observe();
    check(obs[202] == .75f && obs[204] == 1.f && obs[205] == 1.f
        && obs[206] == 100 / 363.f && obs[207] == .75f && obs[208] == 0.f && obs[209] == 1.f,
        "response exposes public chance event in acting-seat coordinates");
    check(obs[16] == 123 / 363.f && obs[165] == 1.f && obs[169] == .5f,
        "chance extension preserves prior hand and incoming-element features");

    Env disabled(Config{512, 10, true, true, false});
    const auto plain = disabled.observe();
    check(std::all_of(plain.begin() + 186, plain.end(), [](float x) { return x == 0.f; }),
        "old profile chance observation extension all zero");

    for (std::uint8_t roll : {std::uint8_t{1}, std::uint8_t{100}}) {
        auto limited = fixture({100}, {123}, Config{1, 10, false, true, true});
        script(limited, {5}, {roll});
        const auto result = limited.step(attack());
        check(result.truncated && !result.terminated && result.rewards == std::array<float, 2>{},
            "chance decision limit does not invent a winner");
        check(limited.state().phase == (roll == 1 ? Phase::response : Phase::main),
            "hit and miss preserve distinct states at horizon");
        Env resumed(Config{1, 10, false, true, true});
        check(resumed.restore(limited.snapshot()), "hit/miss truncated states both restorable");
    }

    auto reversed = fixture({123}, {116}, Config{2, 10, false, true, true});
    auto snapshot = reversed.snapshot();
    snapshot.state.order = {1, 0}; snapshot.state.actor = snapshot.state.turn_owner = 1;
    snapshot.state.players[0].hp = 1;
    check(reversed.restore(snapshot), "restore reversed-seat lethal chance fixture");
    script(reversed, {5}, {1});
    check(reversed.step(attack(0, 0, 1)).valid && reversed.state().pending.target == 0
        && reversed.state().last_chance.source == 1 && reversed.state().last_chance.target == 0,
        "self-selected chance attack normalizes symmetrically for seat one");
    const auto won = reversed.step(defend());
    check(won.terminated && !won.truncated && reversed.state().winner == 1
        && won.rewards == std::array<float, 2>{-1.f, 1.f},
        "chance lethal at horizon takes precedence with fixed-seat rewards");
}
void seeded_rollouts() {
    std::uint64_t attempts = 0, hits = 0, misses = 0;
    for (bool miracles_enabled : {false, true}) for (std::uint64_t seed = 0; seed < 500; ++seed) {
        const Config config{256, 10, miracles_enabled, true, true};
        Env a(config), b(config); a.reset(seed); b.reset(seed);
        const std::uint8_t initial_roll[] = {75};
        check(a.script_hit_rolls(initial_roll) && b.script_hit_rolls(initial_roll), "seeded rolls use same finite prefix");
        std::uint64_t pa = seed + 7000, pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "chance replay includes hit tape and RNG");
            for (float value : a.observe()) check(std::isfinite(value), "chance observation finite");
            const auto action = a.sample_action(pa), action_b = b.sample_action(pb);
            check(action == action_b && pa == pb && a.legal(action), "sampled chance action legal and reproducible");
            const auto ra = a.step(action), rb = b.step(action_b);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.chance_attempt == rb.chance_attempt
                && ra.chance_hit == rb.chance_hit, "chance result replay");
            attempts += ra.chance_attempt;
            hits += ra.chance_attempt && ra.chance_hit;
            misses += ra.chance_attempt && !ra.chance_hit;
            Env resumed(config);
            check(resumed.restore(a.snapshot()) && resumed.snapshot() == a.snapshot(), "every chance state restorable");
        }
    }
    check(attempts > 0 && hits > 0 && misses > 0 && attempts == hits + misses,
        "rollouts exercise scripted-prefix exhaustion and both hit outcomes");
}
}
int main() {
    metadata_and_profiles(); boundary_outcomes_and_consumption(); targeting_and_legality();
    tapes_snapshots_and_privacy(); observations_and_cutoffs(); seeded_rollouts();
    std::cout << "Chance checks passed (percent boundaries, miss/hit flow, hidden tapes, 1000 replay rollouts).\n";
}
