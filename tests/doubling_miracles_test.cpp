#include "godfield/legacy_env.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using namespace godfield::legacy;
const Config config = [] {
    Config c; c.miracles = c.elements = c.chance = c.reflection = c.disease = c.harms = true;
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL doubling miracles: " << message << '\n'; std::exit(1); } }
Action ordered(std::initializer_list<std::uint8_t> refs) {
    Action a{ActionKind::attack_ordered,0,0,1}; a.extra_count = static_cast<std::uint8_t>(refs.size());
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
    check(env.restore(snapshot), "restore explicit doubling fixture"); return env;
}
void holds(Env& env, std::uint64_t doubling, std::uint64_t fire = 0, std::uint64_t light = 0) {
    auto snapshot = env.snapshot(); snapshot.state.players[0].doubling_ability = doubling;
    snapshot.state.players[0].additive_abilities = {fire,light}; check(env.restore(snapshot), "restore independent doubling holdings");
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config); check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "double trace and int64 counter roundtrip");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot(); check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "bad multiplier reference/order/cost/type rejects atomically");
}
void metadata_roles_learning_and_masks() {
    check(doubling_miracle_count == 1 && doubling_miracles().size() == 1 && doubling_miracle_index(214) == 0
        && find_doubling_miracle(214)->cost == 6 && find_doubling_miracle(214)->weight == 1
        && find_doubling_miracle(214)->element == Element::neutral && !find_doubling_miracle(209),
        "214 is the single neutral6MP modifier catalog");
    auto env = fixture({5,214}, {120},12);
    check(std::string_view(env.profile()) == doubling_miracles_ruleset && env.snapshot().version == 37
        && env.options().doubling_slots == 2 && !env.options().doubling_ability_slots
        && !(env.options().utility_slots & 2) && !(env.options().boost_slots & 2),
        "214 is a modifier-only hand role, not standalone utility or free boost");
    check(env.observe()[903] == 1.f && env.observe()[905] == 1.f,
        "new enabled and own214 hand feature positions stable");
    reject(env,{ActionKind::use,1,0,1}); reject(env,{ActionKind::cast_additive,2,0,1});
    const auto result = env.step(ordered({1,18}));
    check(result.doubling_first_uses == 1 && result.doubling_recasts == 1 && result.doubling_mp_spent == 12
        && !result.additive_first_uses && !result.additive_recasts && !result.additive_mp_spent,
        "doubling events are distinct from197/217 events");
}
void sequential_boundaries_and_order() {
    auto env = fixture({5,214}, {120},12); const auto result = env.step(ordered({1,18}));
    check(result.valid && result.doubling_first_uses == 1 && result.doubling_recasts == 1
        && env.state().pending.amount == 4 && env.state().players[0].mp == 0 && env.state().players[0].doubling_ability == 1
        && env.state().players[0].pending_deals == 3 && env.state().players[0].retained_deals == 1
        && !env.state().attack_composition.initial_doubling && env.state().attack_composition.held_mask == 2,
        "hand acquisition precedes repeat held18 and each paid use doubles then earns its own gift"); roundtrip(env);
    check(env.observe()[921] == 2/16.f, "public active trace counts both first-use and held doubling");
    auto reverse = fixture({5,214}, {120},12); reject(reverse,ordered({18,1})); reject(reverse,ordered({1,1}));
    auto insufficient = fixture({5,214}, {120},11); reject(insufficient,ordered({1,18}));
    auto low = fixture({5,214}, {120},5); reject(low,ordered({1}));
    auto exact = fixture({5,214}, {120},6); check(exact.step(ordered({1})).doubling_mp_spent == 6 && exact.state().pending.amount == 2,
        "exact6MP boundary accepted"); roundtrip(exact);
    for (bool doubled_last : {false,true}) {
        auto a = fixture({18}, {120}); holds(a,1,1,0);
        a.step(doubled_last ? ordered({16,18}) : ordered({18,16}));
        check(a.state().pending.amount == (doubled_last ? 14 : 12) && a.state().players[0].mp == 2,
            "197 and214 do not commute"); roundtrip(a);
        auto boost = fixture({18,44}, {120}); holds(boost,1);
        boost.step(doubled_last ? ordered({1,18}) : ordered({18,1}));
        check(boost.state().pending.amount == (doubled_last ? 32 : 21), "ordinary boost and214 order preserved"); roundtrip(boost);
    }
    auto immutable_refs = fixture({5}, {120},15); holds(immutable_refs,1,1,1);
    const auto mixed = immutable_refs.step(ordered({16,17,18}));
    check(mixed.additive_recasts == 2 && mixed.additive_mp_spent == 9 && mixed.doubling_recasts == 1 && mixed.doubling_mp_spent == 6
        && immutable_refs.state().pending.amount == 26, "16/17 retain their old meanings;18 adds independent214"); roundtrip(immutable_refs);
    auto old_cfg = config; old_cfg.doubling_miracles = false; Env old(old_cfg); const auto before = old.snapshot(); const CardId id[] = {214};
    check(!old.script_draws(id) && old.snapshot() == before, "old pools exclude214");
    auto bad = before; bad.state.players[bad.state.actor].doubling_ability = 1;
    check(!old.restore(bad) && old.snapshot() == before, "disabled doubling holdings rejected");
}
void maximum_normal_and_magical_reset() {
    auto env = fixture({56}, {190},99); auto snapshot = env.snapshot(); auto& p = snapshot.state.players[0];
    p.hand.fill(170); p.hand[0] = 56; p.hand_count = 16; p.additive_abilities[0] = 1; p.doubling_ability = 1;
    check(env.restore(snapshot), "restore maximum static weapon composition");
    auto a = ordered({}); a.extra_count = 32;
    for (int i = 0; i < 15; ++i) a.extras[i] = static_cast<std::uint8_t>(i+1);
    a.extras[15] = 16; std::fill_n(a.extras.begin()+16,16,18);
    const auto result = env.step(a);
    check(result.doubling_recasts == 16 && result.doubling_mp_spent == 96 && result.additive_recasts == 1
        && env.state().pending.amount == 16842752 && env.state().players[0].mp == 1
        && env.state().players[0].pending_deals == 33 && env.state().players[0].retained_deals == 17,
        "(30+15*15+2)*2^16 is attainable at98MP below conservative new raw bound"); roundtrip(env);
    auto counter = defend(1); counter.counter_count = 1; const auto launch = env.step(counter);
    check(launch.counter_queued == 1 && env.state().pending.amount == 33685504 && env.state().pending.counter_card == 190
        && env.state().attack_composition == AttackComposition{} && env.state().players[1].dead,
        "real large raw launches its doubled counter from the dead source without inheriting composition"); roundtrip(env);
    env.step(defend()); check(env.done() && env.state().winner == -1, "real large counter clips victim HP without integer wrap"); roundtrip(env);
    auto overflow_action = fixture({5}, {120},99); holds(overflow_action,1); auto seventeen = ordered({18}); seventeen.extra_count = 17;
    std::fill_n(seventeen.extras.begin(),17,18); reject(overflow_action,seventeen);
    auto mana = fixture({57}, {120},99); snapshot = mana.snapshot(); auto& mp = snapshot.state.players[0];
    mp.hand.fill(170); mp.hand[0] = 57; mp.hand_count = 16; mp.doubling_ability = 1;
    check(mana.restore(snapshot), "restore maximal MAGICAL reset with multiplier"); a = ordered({18}); a.extra_count = 16;
    for (int i = 1; i < 16; ++i) a.extras[i] = static_cast<std::uint8_t>(i);
    const auto power = mana.step(a);
    check(power.mana_weapon_power == 597 && power.mana_weapon_mp_spent == 93 && power.doubling_mp_spent == 6
        && mana.state().pending.amount == 597 && mana.state().players[0].mp == 0,
        "MAGICAL resets after6MP payment then doubles to372 before225 boosts"); roundtrip(mana);
    auto reset = fixture({57}, {120},18); holds(reset,1); reset.step(ordered({18,18,18}));
    check(reset.state().pending.amount == 0 && reset.state().pending.mana_weapon == 57,
        "each later paid214 resets MAGICAL; exhausting18MP yields a legal zero payload"); roundtrip(reset);
}
void reaction_type_and_modifier_attribute() {
    for (CardId reaction : {CardId{39},CardId{196},CardId{215}}) {
        auto env = fixture({5,214,39}, {reaction},12); env.step(ordered({1,18})); const auto record = env.state().attack_composition;
        const auto result = env.step(special(0));
        check(result.valid && (result.reflected || result.weapon_blocked) && !result.magic_reflected
            && !result.doubling_first_uses && !result.doubling_recasts && !result.doubling_mp_spent,
            "modifier preserves WEAPON reaction type and is not paid or counted again on response");
        if (!result.weapon_blocked) {
            check(env.state().attack_composition == record && env.state().pending.amount == 4
                && env.state().players[0].mp == 0 && env.state().players[0].doubling_ability == 1,
                "returned doubled raw and acquisition provenance remain frozen"); roundtrip(env);
            env.step(special(0)); roundtrip(env);
        }
        roundtrip(env);
    }
    auto magic_only = fixture({5,214}, {40,210,185}); magic_only.step(ordered({1}));
    reject(magic_only,special(0)); reject(magic_only,special(1)); reject(magic_only,special(2));
    for (bool setter_last : {false,true}) {
        auto env = fixture({58,85,214}, {120}); env.step(setter_last ? ordered({2,1}) : ordered({1,2}));
        check(env.state().pending.element == (setter_last ? Element::water : Element::neutral),
            "neutral214 participates in attribute composition; later setter can restoreWater"); roundtrip(env);
    }
}
void conservative_int64_checkpoint_boundary() {
    auto env = fixture({5}, {190},99); holds(env,1); auto snapshot = env.snapshot();
    snapshot.state.players[1].hand.fill(190); snapshot.state.players[1].hand_count = 16;
    check(env.restore(snapshot), "restore16 counter reservation input"); auto action = ordered({18}); action.extra_count = 16;
    std::fill_n(action.extras.begin(),16,18); env.step(action);
    auto response = defend(0xffff); response.counter_count = 16; response.counter_order = 0xfedcba9876543210ULL; env.step(response); roundtrip(env);
    // This is a conservative checkpoint-bound fixture, not a claim that a
    // high-power32-stage chain survives the HP99 and dead-target rules.
    snapshot = env.snapshot(); auto& s = snapshot.state;
    s.players[0].hand.fill(0); s.players[0].hand_count = 0; s.players[0].pending_deals = 32;
    s.counter_generated = 32; s.counter_started = 1; s.counter_count = 31; s.counter_queue.fill(Pending{});
    for (int i = 0; i < 31; ++i) {
        auto item = s.pending; item.amount = 2; item.source = item.origin_source = static_cast<std::uint8_t>(i < 16 ? 0 : 1);
        item.target = static_cast<std::uint8_t>(1-item.source); s.counter_queue[i] = item;
    }
    s.pending.amount = doubling_counter_damage_limit;
    check(env.restore(snapshot), "conservative int64 fullbound with32 physical counter charges restores"); roundtrip(env);
    const auto exact = env.snapshot(); auto bad = exact; bad.state.pending.amount += 2;
    check(!env.restore(bad) && env.snapshot() == exact, "even counter amount just above mathematical upper bound rejected");
    check(env.state().pending.amount > std::numeric_limits<std::int32_t>::max()
        && std::isfinite(env.observe()[923]) && env.observe()[923] == 1.f,
        "new public int64 scale represents the conservative fullbound");
    const auto done = env.step(defend());
    check(done.counter_resolved && done.counter_skipped == 31 && env.done() && env.state().winner == -1
        && env.state().players[0].hp == 0, "large checkpoint value clips HP then skips all dead targets without overflow"); roundtrip(env);
}
void acquisition_cap_strictness_privacy_and_replay() {
    auto capped_cfg = config; capped_cfg.max_decisions = 1;
    auto cap = fixture({5,214}, {120},12,capped_cfg); holds(cap,0,15,0); reject(cap,ordered({1}));
    auto allowed = fixture({5,214}, {120},12,capped_cfg); holds(allowed,0,14,0);
    check(allowed.step(ordered({1,18})).doubling_first_uses == 1 && allowed.state().truncated,
        "combined three-kind acquisition cap permits final new214 and held repeat"); roundtrip(allowed);
    auto env = fixture({57,214}, {39},12); env.step(ordered({1,18})); const auto exact = env.snapshot();
    for (int error = 0; error < 8; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.attack_composition.initial_doubling = 1;
        if (error == 1) bad.state.players[0].doubling_ability = 0;
        if (error == 2) bad.state.attack_composition.items[0] = 197;
        if (error == 3) bad.state.pending.amount = 1;
        if (error == 4) bad.state.attack_composition.held_mask = 3;
        if (error == 5) bad.state.attack_composition.initial_mp = 11;
        if (error == 6) bad.config.doubling_miracles = false;
        if (error == 7) bad.state.attack_composition.initial_doubling = std::numeric_limits<std::uint64_t>::max();
        check(!env.restore(bad) && env.snapshot() == exact, "double trace/ownership/cost/reset/profile tampering rejects atomically");
    }
    auto hidden = fixture({5}, {214}); const auto visible = hidden.observe(); auto changed = hidden.snapshot();
    changed.state.players[1].hand[0] = 120; changed.state.players[1].doubling_ability = 1;
    check(hidden.restore(changed) && hidden.observe() == visible, "unplayed opponent multiplier and exact holdings stay private");
    auto old_cfg = config; old_cfg.doubling_miracles = false; Env old(old_cfg); const auto oldobs = old.observe();
    check(std::all_of(oldobs.begin()+903,oldobs.end(),[](float x){return x == 0.f;}), "old profiles new21 features zero");
    std::uint64_t doubled = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+109000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "doubling seeded trajectory replay");
            for (float x : a.observe()) check(std::isfinite(x), "large power observations remain finite");
            const auto action = a.sample_action(pa), twin = b.sample_action(pb);
            check(action == twin && pa == pb && a.legal(action), "doubler ordered sampler legal and deterministic");
            const auto ra = a.step(action), rb = b.step(twin);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.doubling_first_uses == rb.doubling_first_uses
                && ra.doubling_recasts == rb.doubling_recasts && ra.doubling_mp_spent == rb.doubling_mp_spent,
                "doubling count/cost/reward events replay"); doubled += ra.doubling_first_uses + ra.doubling_recasts; roundtrip(a);
        }
    }
    check(doubled, "natural pool exercises multiplier casts");
}
}
int main() {
    metadata_roles_learning_and_masks(); sequential_boundaries_and_order(); maximum_normal_and_magical_reset();
    reaction_type_and_modifier_attribute(); conservative_int64_checkpoint_boundary(); acquisition_cap_strictness_privacy_and_replay();
    std::cout << "Doubling miracles passed (order/MP/MAGICAL/largecounter/strict trace,1000 replay rollouts).\n";
}
