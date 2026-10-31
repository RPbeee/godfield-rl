#include "godfield/legacy_env.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string_view>

namespace {
using namespace godfield::legacy;
const Config config = [] {
    Config c; c.miracles = c.elements = c.chance = c.reflection = c.disease = c.harms = true;
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = true;
    return c;
}();
void check(bool ok, const char* text) { if (!ok) { std::cerr << "FAIL wide miracles: " << text << '\n'; std::exit(1); } }
Action ordered(std::initializer_list<std::uint8_t> refs, std::uint8_t target = 1) {
    Action a{ActionKind::attack_ordered,0,0,target}; a.extra_count = static_cast<std::uint8_t>(refs.size());
    std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
void hand(Player& p, std::initializer_list<CardId> cards) {
    p.hand.fill(0); std::copy(cards.begin(),cards.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(cards.size());
    p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, int mp = 10, Config cfg = config) {
    Env env(cfg); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy); snapshot.state.players[0].mp = mp;
    check(env.restore(snapshot), "restore wide fixture"); return env;
}
void hold(Env& env, std::uint64_t count) { auto s = env.snapshot(); s.state.players[0].wide_ability = count; check(env.restore(s), "restore initial held209"); }
void roundtrip(const Env& env) { Env resumed(env.snapshot().config); check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "wide/current/past chance provenance roundtrip"); }
void reject(Env& env, Action action) { const auto before = env.snapshot(); check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before, "bad WIDE action rejected before costs, ownership, RNG or gifts mutate"); }
void rolls(Env& env) { const std::uint8_t hits[] = {100,1}; check(env.script_hit_rolls(hits), "script100 guaranteed hit then next roll"); }
void metadata_learning_and_minimum_profile() {
    check(wide_miracle_count == 1 && wide_miracles().size() == 1 && wide_miracle_index(209) == 0
        && find_wide_miracle(209)->cost == 5 && find_wide_miracle(209)->hit_rate == 100
        && find_wide_miracle(209)->element == Element::neutral && find_wide_miracle(209)->weight == 1,
        "209 has positive-weight neutral5MP WIDE100 role");
    auto env = fixture({5,209}, {120});
    check(std::string_view(env.profile()) == wide_miracles_ruleset && env.snapshot().version == 37
        && env.options().wide_slots == 2 && !env.options().wide_ability_slots && !(env.options().utility_slots & 2),
        "hand WIDE is extra-only and uses independent masks"); reject(env,{ActionKind::use,1,0,1}); reject(env,{ActionKind::cast_additive,3,0,1});
    rolls(env); const auto result = env.step(ordered({1,19},0));
    check(result.valid && result.wide_attack && result.wide_first_uses == 1 && result.wide_recasts == 1 && result.wide_mp_spent == 10
        && !result.additive_first_uses && !result.additive_recasts && !result.doubling_first_uses && !result.doubling_recasts
        && env.state().players[0].wide_ability == 1 && env.state().players[0].mp == 0
        && env.state().players[0].pending_deals == 3 && env.state().players[0].retained_deals == 1
        && env.state().pending.source == 0 && env.state().pending.target == 1 && env.state().actor == 1
        && env.state().pending.wide_miracle == 209 && env.state().pending.hit_rate == 100
        && env.snapshot().hit_index == 1,
        "same-action learns then repeats209; repeated WIDE still produces one enemy item and one final roll"); roundtrip(env);
    check(env.state().attack_composition.initial_wide == 0 && env.state().last_chance.wide_miracle == 209
        && env.state().last_chance.id == 5 && env.state().last_chance.source == 0 && env.state().last_chance.target == 1
        && env.state().last_chance.hit && env.state().last_chance.hit_rate == 100,
        "public chance records original weapon plus WIDE provenance rather than classifying it as209 MAGIC");
    auto inverse = fixture({5,209}, {120}); reject(inverse,ordered({19,1})); reject(inverse,ordered({1,1}));
    auto low = fixture({5,209}, {120},4); reject(low,ordered({1}));
    Config minimum; minimum.miracles = minimum.elements = minimum.chance = minimum.harms = minimum.celestial = minimum.additive_miracles = minimum.wide_miracles = true;
    auto narrow = fixture({5}, {120},5,minimum); hold(narrow,1); reject(narrow,ordered({18}));
    check(narrow.options().wide_ability_slots == 1 && narrow.step(ordered({19},0)).wide_attack
        && narrow.state().phase == Phase::response && narrow.state().pending.target == 1,
        "ref19 works with doubling disabled and self input remains enemy-directed"); roundtrip(narrow);
    auto old_cfg = config; old_cfg.wide_miracles = false; Env old(old_cfg); const auto original = old.snapshot(); const CardId id[] = {209};
    check(!old.script_draws(id) && old.snapshot() == original, "older profile pool rejects209");
    auto repeated = fixture({5}, {120},99); hold(repeated,1); auto twenty = ordered({19}); twenty.extra_count = 20;
    std::fill_n(twenty.extras.begin(),20,19); reject(repeated,twenty); twenty.extra_count = 19; twenty.extras[19] = 0;
    rolls(repeated); const auto nineteen = repeated.step(twenty);
    check(nineteen.wide_recasts == 19 && nineteen.wide_mp_spent == 95 && repeated.snapshot().hit_index == 1
        && repeated.state().players[0].retained_deals == 19 && repeated.observe()[944] == 1.f,
        "nineteen paid WIDE uses fit99MP and normalize one final enemy command/one roll"); roundtrip(repeated);
    auto capped_cfg = config; capped_cfg.max_decisions = 1;
    auto capped = fixture({5,209}, {120},10,capped_cfg); auto cap = capped.snapshot();
    cap.state.players[0].additive_abilities[0] = 13; cap.state.players[0].doubling_ability = 1; cap.state.players[0].wide_ability = 1;
    check(capped.restore(cap), "restore combined four-kind acquisition cap"); reject(capped,ordered({1}));
    check(capped.step(ordered({19})).wide_recasts == 1 && capped.state().truncated,
        "four-kind cap denies first use while existing held WIDE remains usable"); roundtrip(capped);
}
void guaranteed_roll_darkcloud_and_past_event() {
    for (bool cloud : {false,true}) {
        auto env = fixture({5,209}, {39}); auto snapshot = env.snapshot(); snapshot.state.players[1].dark_cloud = cloud;
        check(env.restore(snapshot), "restore actual WIDE target DarkCloud"); rolls(env);
        const auto result = env.step(ordered({1},0));
        check(result.chance_attempt && result.chance_hit && result.chance_forced == cloud && env.snapshot().hit_index == (cloud ? 0 : 1)
            && env.state().last_chance.forced == cloud,
            "100 percent consumes one roll except actual enemy DarkCloud short-circuit"); roundtrip(env);
        const auto chance = env.state().last_chance; const auto index = env.snapshot().hit_index;
        const auto reflected = env.step(special(0));
        check(reflected.reflected && !reflected.wide_attack && !reflected.wide_first_uses && !reflected.wide_recasts
            && env.state().pending.hit_rate == 100 && env.state().pending.wide_miracle == 209
            && env.state().last_chance == chance && env.snapshot().hit_index == index,
            "NE WIDE physical reflection requires no195 and keeps original hit/coordinates without reroll"); roundtrip(env);
        env.step(defend()); check(env.state().attack_composition == AttackComposition{} && env.state().last_chance == chance,
            "past WIDE event persists after trace and current item clear"); roundtrip(env);
    }
    auto own_cloud = fixture({5,209}, {120}); auto snapshot = own_cloud.snapshot(); snapshot.state.players[0].dark_cloud = true;
    check(own_cloud.restore(snapshot), "restore only requested self seat Cloud"); rolls(own_cloud); own_cloud.step(ordered({1},0));
    check(!own_cloud.state().last_chance.forced && own_cloud.snapshot().hit_index == 1,
        "requested self Cloud does not force attack normalized to clean enemy"); roundtrip(own_cloud);
}
void attributes_reactions_zero_and_type() {
    for (bool setter_last : {false,true}) {
        auto env = fixture({58,65,209}, {195,39}); rolls(env);
        env.step(setter_last ? ordered({2,1}) : ordered({1,2}));
        check(env.state().pending.element == (setter_last ? Element::fire : Element::neutral),
            "209 resets toNE at its own position; a subsequent setter restoresFire"); roundtrip(env);
        if (setter_last) { reject(env,special(1)); check(env.step(special(1,1)).reflected && env.state().pending.attribute_removed,
            "colored completed WIDE needs actual195 before physical reflection"); }
        else check(env.step(special(1)).reflected, "alreadyNE completed WIDE reflects without195");
        check(env.state().pending.hit_rate == 100 && env.state().pending.wide_miracle == 209,
            "attribute removal never removes percent or WIDE provenance"); roundtrip(env);
    }
    for (CardId reaction : {CardId{20},CardId{215},CardId{196}}) {
        auto env = fixture({5,209}, {reaction}); const std::uint8_t targets[] = {0}; check(env.script_bounce_targets(targets), "script enemy bounce");
        env.step(ordered({1})); const auto result = env.step(special(0));
        check(result.valid && (result.bounced || result.weapon_blocked || result.any_reflected) && !result.magic_reflected,
            "NE completed WIDE remains WEAPON for20/215/196"); roundtrip(env);
    }
    auto types = fixture({5,209}, {40,210,185}); types.step(ordered({1})); reject(types,special(0)); reject(types,special(1)); reject(types,special(2));
    auto zero = fixture({57,209}, {39},5); rolls(zero); const auto result = zero.step(ordered({1},0));
    check(result.mana_weapon_attack && result.mana_weapon_mp_spent == 0 && result.wide_mp_spent == 5
        && result.mana_weapon_power == 0 && result.chance_hit && zero.state().phase == Phase::response
        && zero.state().pending.amount == 0 && zero.state().pending.mana_weapon == 57 && zero.state().pending.hit_rate == 100,
        "exact5MP MAGICAL resets to legal zero while100percent still targets enemy and rolls"); roundtrip(zero);
    zero.step(special(0)); roundtrip(zero); zero.step(defend());
    check(zero.state().players[0].hp == 40 && zero.state().players[1].hp == 40,
        "reflected guaranteed zero percent payload causes no HP damage"); roundtrip(zero);
    auto counter = fixture({5,209}, {190}); counter.step(ordered({1})); auto response = defend(1); response.counter_count = 1;
    check(counter.step(response).counter_queued == 1 && counter.state().pending.counter_card == 190
        && !counter.state().pending.wide_miracle && !counter.state().pending.hit_rate,
        "derived counter does not inherit guaranteed rate or WIDE marker"); roundtrip(counter);
    auto next_chance = fixture({5,209}, {187}); rolls(next_chance); next_chance.step(ordered({1}));
    check(next_chance.step(response).counter_chance_hits == 1 && next_chance.snapshot().hit_index == 2
        && next_chance.state().last_chance.id == 187 && !next_chance.state().last_chance.wide_miracle
        && next_chance.state().last_chance.source == 1 && next_chance.state().last_chance.target == 0,
        "later187 launch overwrites past WIDE event with its own immutable origin and ordinary75percent record"); roundtrip(next_chance);
    for (CardId base : {CardId{30},CardId{43},CardId{70}}) {
        auto payload = fixture({base,209}, {39}); payload.step(ordered({1})); const auto before = payload.state().pending;
        check((base != 30 || before.disease == Disease::hell) && (base != 43 || before.harm == Harm::dark_cloud)
            && (base != 70 || before.harm == Harm::glory), "WIDE preserves base disease/harm payload despite derived percent"); roundtrip(payload);
        payload.step(special(0)); check(payload.state().pending.status_card == before.status_card
            && payload.state().pending.harm_card == before.harm_card && payload.state().pending.harm == before.harm
            && payload.state().pending.disease == before.disease,
            "physical return preserves WIDE base payload and its provenance"); roundtrip(payload);
    }
}
void strictness_observation_and_seeded_replay() {
    auto env = fixture({5,209}, {39}); rolls(env); env.step(ordered({1})); env.step(special(0)); const auto exact = env.snapshot();
    for (int error = 0; error < 12; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.pending.wide_miracle = 0;
        if (error == 1) bad.state.pending.wide_miracle = 214;
        if (error == 2) bad.state.pending.hit_rate = 99;
        if (error == 3) bad.state.last_chance.wide_miracle = 0;
        if (error == 4) bad.state.last_chance.id = 209;
        if (error == 5) bad.state.last_chance.hit = false;
        if (error == 6) { bad.state.last_chance.source = 1; bad.state.last_chance.target = 0; }
        if (error == 7) bad.state.attack_composition.initial_wide = 1;
        if (error == 8) bad.state.players[0].wide_ability = 0;
        if (error == 9) bad.state.attack_composition.items[0] = 197;
        if (error == 10) bad.config.wide_miracles = false;
        if (error == 11) bad.state.pending.amount = 0;
        check(!env.restore(bad) && env.snapshot() == exact, "forged rate/guarantee/source/trace/holdings/profile rejects atomically");
    }
    env.step(defend()); const auto past = env.snapshot(); auto bad = past; bad.state.last_chance.wide_miracle = 0;
    check(!env.restore(bad) && env.snapshot() == past, "past base-rate0/guaranteed event still requires209 provenance after trace clear");
    auto own = fixture({209}, {120}); const auto obs = own.observe(); check(obs[924] == 1.f && obs[925] == 1.f, "new flag/own209 positions");
    auto active = fixture({5,209}, {120}); active.step(ordered({1}));
    check(active.observe()[942] == 1.f && active.observe()[943] == 1.f && active.observe()[944] == 1/19.f,
        "current and past WIDE flags and public use count represented");
    auto hidden = fixture({5}, {209}); const auto visible = hidden.observe(); auto changed = hidden.snapshot();
    changed.state.players[1].hand[0] = 120; changed.state.players[1].wide_ability = 1;
    check(hidden.restore(changed) && hidden.observe() == visible, "opponent unplayed209 and private full holdings not leaked");
    auto old_cfg = config; old_cfg.wide_miracles = false; Env old(old_cfg); const auto oldobs = old.observe();
    check(std::all_of(oldobs.begin()+924,oldobs.end(),[](float x){return x == 0.f;}), "old profiles new21-feature suffix zero");
    std::uint64_t attacks = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+110000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "WIDE seed state/observation replay");
            for (float x : a.observe()) check(std::isfinite(x), "WIDE observations finite");
            const auto action = a.sample_action(pa), twin = b.sample_action(pb);
            check(action == twin && pa == pb && a.legal(action), "WIDE ordered sampler deterministic and legal");
            const auto ra = a.step(action), rb = b.step(twin);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.wide_attack == rb.wide_attack
                && ra.wide_first_uses == rb.wide_first_uses && ra.wide_recasts == rb.wide_recasts && ra.wide_mp_spent == rb.wide_mp_spent,
                "WIDE cast/cost/guarantee/reward events replay"); attacks += ra.wide_attack; roundtrip(a);
        }
    }
    check(attacks, "natural pool exercises WIDE use");
}
}
int main() {
    metadata_learning_and_minimum_profile(); guaranteed_roll_darkcloud_and_past_event(); attributes_reactions_zero_and_type(); strictness_observation_and_seeded_replay();
    std::cout << "Wide miracles passed (guaranteed roll/target/type/zero/order/provenance,1000 replay rollouts).\n";
}
