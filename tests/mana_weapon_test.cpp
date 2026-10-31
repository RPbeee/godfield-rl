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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = true;
    return c;
}();
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL mana weapon: " << message << '\n'; std::exit(1); }
}
Action attack(std::uint16_t slots = 0, std::uint8_t target = 1) { return {ActionKind::attack,0,slots,target}; }
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action counter(std::uint16_t slots) {
    auto a = defend(slots);
    for (int slot = 0; slot < 16; ++slot) if (slots & (1u<<slot)) {
        a.counter_order |= std::uint64_t(slot) << (4*a.counter_count); ++a.counter_count;
    }
    return a;
}
void hand(Player& p, std::initializer_list<CardId> cards) {
    p.hand.fill(0); std::copy(cards.begin(),cards.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(cards.size());
    p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, int mp = 10, Config cfg = config) {
    Env env(cfg); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy); snapshot.state.players[0].mp = mp;
    check(env.restore(snapshot), "restore explicit mana weapon fixture"); return env;
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "mana provenance survives exact snapshot roundtrip");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "invalid mana weapon composition is rejected before MP, hand, RNG or gifts mutate");
}
void gifts(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "script deferred mana weapon gifts"); }
void metadata_scope_and_zero() {
    check(mana_weapon_card_count == 1 && mana_weapon_catalog().size() == 1, "single57 closed catalog");
    const auto* card = find_mana_weapon_card(57);
    check(card && card->multiplier == 2 && card->card.kind == Kind::weapon && !card->card.attack && !card->card.boost
        && !card->card.defense && !card->card.hp && !card->card.mp && card->card.price == 10
        && card->card.weight == 1 && !card->card.discardable, "57 is dynamic MP-times-two weapon, not static ATK or extra boost");
    for (CardId id : {CardId{0},CardId{56},CardId{58},CardId{197},CardId{217},CardId{65535}})
        check(!find_mana_weapon_card(id), "no added magic role silently included");
    Config narrow; narrow.mana_weapon = true;
    auto env = fixture({57}, {120},0,narrow);
    check(std::string_view(env.profile()) == mana_weapon_ruleset && env.snapshot().version == 36
        && env.options().weapon_bases == 1 && !env.options().boost_slots && !env.options().pray
        && !env.options().discard_slots && !env.options().utility_slots,
        "dependency-free profile preserves weapon role and no-pray even at MP0");
    reject(env,{ActionKind::pray}); reject(env,{ActionKind::use,0,0,1}); reject(env,{ActionKind::discard,no_slot,1,no_slot});
    gifts(env); const auto result = env.step(attack());
    check(result.valid && result.mana_weapon_attack && !result.mana_weapon_mp_spent && !result.mana_weapon_power
        && env.state().phase == Phase::response && env.state().pending.kind == Effect::damage
        && env.state().pending.amount == 0 && env.state().pending.mana_weapon == 57 && !env.state().pending.mana_spent
        && env.state().players[0].pending_deals == 1 && env.snapshot().draw_index == 0,
        "zero power is an explicit response item with deferred gift"); roundtrip(env);
    check(env.step(defend()).valid && env.state().players[1].hp == 40 && env.snapshot().draw_index == 1,
        "zero power accept costs no HP and still completes weapon use"); roundtrip(env);
    auto self = fixture({57}, {120},0,narrow);
    check(self.step(attack(0,0)).valid && self.state().phase == Phase::main && self.state().players[0].hp == 40,
        "self-target zero attack resolves without response"); roundtrip(self);
    auto old_cfg = config; old_cfg.mana_weapon = false; Env old(old_cfg); const auto before = old.snapshot();
    const CardId draw[] = {57}; check(!old.script_draws(draw) && old.snapshot() == before, "old pool rejects57 draw without state change");
    auto bad = before; bad.state.players[bad.state.actor].hand[0] = 57;
    check(!old.restore(bad) && old.snapshot() == before, "old profile rejects57 hand");
    auto extra = fixture({5,57}, {120}); reject(extra,attack(2));
}
void power_boosts_and_setters() {
    for (int mp : {0,1,10,49,99}) {
        auto env = fixture({57}, {120},mp); const auto obs = env.observe();
        check(std::abs(obs[17] - mp*2/30.f) < 1e-6f && obs[713] == 1.f && obs[714] == 1.f,
            "own dynamic ATK uses current MP and appended own57 features");
        const auto result = env.step(attack());
        check(result.mana_weapon_attack && result.mana_weapon_mp_spent == mp && result.mana_weapon_power == 2*mp
            && env.state().pending.amount == 2*mp && env.state().pending.mana_spent == mp && env.state().players[0].mp == 0
            && env.observe()[730] == 1.f && std::abs(env.observe()[731]-mp/99.f) < 1e-6f,
            "all current MP spent once before response and represented as public provenance"); roundtrip(env);
    }
    auto maximum = fixture({57}, {120},99); auto snapshot = maximum.snapshot();
    auto& p = snapshot.state.players[0]; p.hand.fill(170); p.hand[0] = 57; p.hand_count = 16;
    check(maximum.restore(snapshot), "restore15 maximum ordinary boosts"); gifts(maximum);
    check(maximum.step(attack(0xfffe)).mana_weapon_power == 423 && maximum.state().pending.amount == 423
        && maximum.state().players[0].pending_deals == 16 && maximum.state().players[0].mp == 0,
        "maximal99MP plus15 ordinary15boost gives423 and consumes16 cards"); roundtrip(maximum);
    maximum.step(defend()); check(maximum.snapshot().draw_index == 16 && maximum.state().players[1].dead,
        "maximum power clips HP and distributes all consumed cards"); roundtrip(maximum);
    auto self = fixture({57,9}, {120},3); check(self.step(attack(2,0)).mana_weapon_power == 7
        && self.state().players[0].hp == 33 && self.state().players[0].mp == 0,
        "self attack combines ordinary boost and spends MP before damage"); roundtrip(self);
    for (ActionKind kind : {ActionKind::attack_set_fire,ActionKind::attack_set_neutral}) {
        auto setter = fixture({57,65,9}, {196},3);
        const auto result = setter.step({kind,0,static_cast<std::uint16_t>(kind == ActionKind::attack_set_fire ? 2 : 6),1});
        check(result.mana_weapon_attack && result.attribute_attack && result.mana_weapon_power == (kind == ActionKind::attack_set_fire ? 6 : 7)
            && setter.state().pending.attribute_card == 65 && setter.state().pending.mana_spent == 3
            && setter.state().pending.element == (kind == ActionKind::attack_set_fire ? Element::fire : Element::neutral),
            "setter changes color but contributes no ATK to dynamic weapon"); roundtrip(setter);
        setter.step(special(0)); check(setter.state().pending.mana_weapon == 57 && setter.state().players[0].mp == 0,
            "ANY preserves setter and spent-MP provenance"); roundtrip(setter);
    }
    for (int mp : {0,3}) {
        auto removed = fixture({57,65}, {195,39},mp);
        removed.step({ActionKind::attack_set_fire,0,2,1});
        check(removed.step(special(1,1)).reflected && removed.state().pending.attribute_removed
            && removed.state().pending.element == Element::neutral && removed.state().pending.mana_weapon == 57
            && removed.state().pending.amount == 2*mp && removed.state().pending.attribute_card == 65,
            "actual195 permits physical reflection of setter-colored57, including zero power"); roundtrip(removed);
    }
}
void weapon_reactions_and_counters() {
    for (CardId reaction : {CardId{39},CardId{20},CardId{196}}) {
        auto env = fixture({57,39}, {173,reaction},3); const std::uint8_t bounce[] = {0};
        check(env.script_bounce_targets(bounce), "script physical bounce back to original source"); env.step(attack());
        const auto returned = env.step(special(1,1));
        check(returned.valid && (returned.reflected || returned.bounced) && !returned.magic_reflected
            && env.state().pending.amount == 6 && env.state().pending.mana_spent == 3 && env.state().pending.mana_weapon == 57
            && env.state().players[0].mp == 0 && env.state().players[1].mp == 10,
            "ordinary prefix cannot reduce returned dynamic weapon or cause another MP payment"); roundtrip(env);
        env.step(special(0)); check(env.state().pending.amount == 6 && env.state().players[0].mp == 0,
            "second physical reflection preserves original MP-derived power"); roundtrip(env);
        env.step(defend()); roundtrip(env);
    }
    auto types = fixture({57}, {40,210,215,171},0); types.step(attack());
    reject(types,special(0)); reject(types,special(1));
    check(types.legal(special(2)) && types.step(special(2)).weapon_blocked,
        "zero damage retains weapon type for215 and rejects MAGIC-only40/210"); roundtrip(types);
    auto held = fixture({57}, {120},0); auto snapshot = held.snapshot(); snapshot.state.players[1].defensive_abilities[1] = 1;
    check(held.restore(snapshot), "restore held215 against zero damage"); held.step(attack());
    check(held.step({ActionKind::defend_cast,1,0,no_slot}).weapon_blocked && held.state().players[1].mp == 4,
        "held215 still pays MP to block zero power weapon"); roundtrip(held);
    for (int mp : {0,3}) {
        auto env = fixture({57}, {190},mp); env.step(attack()); const auto result = env.step(counter(1));
        if (!mp) check(!result.counter_queued && !env.state().counter_generated && env.state().players[1].hp == 40,
            "zero raw residual cannot reserve a counter");
        else {
            check(result.counter_queued == 1 && env.state().pending.amount == 12 && env.state().pending.counter_card == 190
                && !env.state().pending.mana_weapon && !env.state().pending.mana_spent,
                "counter derives raw post-boost amount but does not inherit mana-weapon provenance"); roundtrip(env); env.step(defend());
            check(env.state().players[0].hp == 28, "counter raw12 applies after initial MP spent");
        } roundtrip(env);
    }
    auto restore_mp = fixture({57}, {193},3); restore_mp.step(attack());
    check(restore_mp.step(counter(1)).counter_mp_gained == 12 && restore_mp.state().players[1].mp == 22,
        "193 MP restoration occurs after mana frame finishes, without mutating original spent amount"); roundtrip(restore_mp);
    auto provisional = fixture({57}, {175,196},0); snapshot = provisional.snapshot(); snapshot.state.players[1].disease = Disease::heaven;
    check(provisional.restore(snapshot), "restore Heaven before zero-damage weapon prefix"); provisional.step(attack());
    check(provisional.step(special(1,1)).any_reflected && provisional.state().players[1].hp == 0
        && !provisional.state().players[1].dead && provisional.state().pending.amount == 0,
        "zero weapon does not omit ordinary prefix side effects or finalize Heaven death mid-chain"); roundtrip(provisional);
    provisional.step(defend()); check(provisional.state().players[1].dead && provisional.state().players[0].hp == 40,
        "zero reflected payload finishes pending side-effect death without damage"); roundtrip(provisional);
}
void strict_snapshots_privacy_and_replay() {
    auto env = fixture({57}, {39},3); env.step(attack()); const auto exact = env.snapshot();
    for (int error = 0; error < 11; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.pending.mana_weapon = 0;
        if (error == 1) bad.state.pending.mana_weapon = 58;
        if (error == 2) bad.state.pending.mana_spent = 100;
        if (error == 3) bad.state.pending.amount = 5;
        if (error == 4) bad.state.pending.amount = 424;
        if (error == 5) bad.state.players[0].mp = 1;
        if (error == 6) bad.state.pending.miracle = 198;
        if (error == 7) bad.state.pending.hit_rate = 50;
        if (error == 8) bad.config.mana_weapon = false;
        if (error == 9) { bad.state.pending.mana_spent = 0; bad.state.pending.amount = 423; }
        if (error == 10) bad.state.pending.element = Element::fire;
        check(!env.restore(bad) && env.snapshot() == exact, "impossible mana source/spent/power/type/profile restored atomically rejected");
    }
    auto zero = fixture({57}, {39},0); zero.step(attack()); roundtrip(zero); zero.step(special(0)); roundtrip(zero);
    const auto returned = zero.snapshot(); auto bad = returned; bad.state.pending.mana_weapon = 0;
    check(!zero.restore(bad) && zero.snapshot() == returned, "zero damage requires57 marker even after reflection");
    auto enemy = fixture({5}, {57}); const auto visible = enemy.observe(); auto changed = enemy.snapshot();
    changed.state.players[1].hand[0] = 120; changed.rng_state ^= 0x9e3779b97f4a7c15ULL;
    check(enemy.restore(changed) && enemy.observe() == visible, "enemy unplayed57 and future RNG remain private");
    auto old_cfg = config; old_cfg.mana_weapon = false; Env old(old_cfg); const auto old_obs = old.observe();
    check(std::all_of(old_obs.begin()+713,old_obs.end(),[](float x){return x == 0.f;}), "old profiles appended19 features zero");
    std::uint64_t attacks = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+106000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "seeded dynamic weapon trajectories replay exactly");
            for (float x : a.observe()) check(std::isfinite(x), "dynamic power observation finite");
            const auto action = a.sample_action(pa), twin = b.sample_action(pb);
            check(action == twin && pa == pb && a.legal(action), "mana weapon sampler reproducible and legal");
            const auto ra = a.step(action), rb = b.step(twin);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.mana_weapon_attack == rb.mana_weapon_attack
                && ra.mana_weapon_mp_spent == rb.mana_weapon_mp_spent && ra.mana_weapon_power == rb.mana_weapon_power,
                "dynamic power events and final reward replay"); attacks += ra.mana_weapon_attack; roundtrip(a);
        }
    }
    check(attacks, "natural pool trajectories exercise57");
}
}
int main() {
    metadata_scope_and_zero(); power_boosts_and_setters(); weapon_reactions_and_counters(); strict_snapshots_privacy_and_replay();
    std::cout << "Mana weapon passed (zero/spent/boost/type/counter/provenance,1000 replay rollouts).\n";
}
