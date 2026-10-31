#include "godfield/legacy_env.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {
using namespace godfield::legacy;
const Config config = [] {
    Config c; c.miracles = c.elements = c.chance = c.reflection = c.disease = c.harms = true;
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = true;
    return c;
}();
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL additive miracles: " << message << '\n'; std::exit(1); }
}
Action ordered(std::initializer_list<std::uint8_t> refs, std::uint8_t base = 0, std::uint8_t target = 1) {
    Action a{ActionKind::attack_ordered,base,0,target}; a.extra_count = static_cast<std::uint8_t>(refs.size());
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
    check(env.restore(snapshot), "restore explicit additive fixture"); return env;
}
void abilities(Env& env, std::uint64_t fire, std::uint64_t light) {
    auto snapshot = env.snapshot(); snapshot.state.players[0].additive_abilities = {fire,light};
    check(env.restore(snapshot), "restore initial additional holdings");
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "ordered composition and holds roundtrip exactly");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "invalid ordered refs/cost/type rejected before MP, holdings, hand, gifts and RNG mutate");
}
void gifts(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "script64 deferred gift prefix"); }
void metadata_standalone_and_local_masks() {
    check(additive_miracle_count == 2 && additive_miracles().size() == 2
        && additive_miracle_index(197) == 0 && additive_miracle_index(217) == 1,
        "two additional miracle stable local indices");
    for (int i = 0; i < 2; ++i) {
        const auto& card = additive_miracles()[i];
        check(card.id == (i ? 217 : 197) && card.amount == (i ? 10 : 2) && card.cost == (i ? 7 : 2)
            && card.weight == 1 && card.element == (i ? Element::light : Element::fire), "exact additive data");
        auto env = fixture({card.id}, {196}); gifts(env);
        check(std::string_view(env.profile()) == additive_miracles_ruleset && env.snapshot().version == 35
            && env.options().additive_slots == 1 && !env.options().boost_slots && !env.options().additive_ability_slots,
            "new hand/additional ability masks stay separate from ordinary boost and old main masks");
        check(env.observe()[750] == 1.f && env.observe()[751] == (i+1)/2.f,
            "additional flag and own hand local kind features have stable suffix positions");
        const auto result = env.step({ActionKind::use,0,0,1});
        check(result.valid && result.additive_first_uses == 1 && !result.additive_recasts && result.additive_mp_spent == card.cost
            && !result.spell_first_use && !result.spell_recast && !result.ordered_attack
            && env.state().pending.miracle == card.id && env.state().pending.amount == card.amount
            && env.state().players[0].additive_abilities[i] == 1 && env.state().players[0].mp == 10-card.cost
            && env.state().attack_composition == AttackComposition{}, "standalone hand spell is MAGIC and acquires once"); roundtrip(env);
        check(env.observe()[774] == (i+1)/2.f, "current standalone additive MAGIC kind is public");
        env.step(special(0)); check(env.state().pending.miracle == card.id && env.state().players[0].mp == 10-card.cost,
            "standalone MAGIC survives ANY without another payment or acquisition"); roundtrip(env);
        env.step(defend()); roundtrip(env);
        auto held = fixture({120}, {152}); abilities(held,i ? 0 : 1,i ? 1 : 0);
        check(held.options().additive_ability_slots == (1u<<i) && !held.options().ability_slots,
            "held additional kinds use own local mask");
        const auto cast = held.step({ActionKind::cast_additive,static_cast<std::uint8_t>(i),0,1});
        check(cast.additive_recasts == 1 && !cast.additive_first_uses && !cast.spell_first_use && !cast.spell_recast
            && held.state().pending.from_ability && held.state().players[0].retained_deals == 1,
            "standalone held cast retains count and earns one retained gift"); roundtrip(held);
        check(held.step(special(0)).magic_blocked, "additional standalone magic is blockable"); roundtrip(held);
    }
    auto low = fixture({197}, {120},1); reject(low,{ActionKind::use,0,0,1});
    auto paid = fixture({217}, {120},7); check(paid.step({ActionKind::use,0,0,1}).valid && paid.state().players[0].mp == 0,
        "standalone exact MP boundary accepted"); roundtrip(paid);
    auto self = fixture({197}, {120}); check(self.step({ActionKind::use,0,0,0}).valid && self.state().players[0].hp == 38
        && self.state().attack_composition == AttackComposition{}, "self standalone MAGIC resolves immediately"); roundtrip(self);
    for (CardId id : {CardId{0},CardId{198},CardId{209},CardId{214},CardId{216},CardId{65535}})
        check(!find_additive_miracle(id) && additive_miracle_index(id) == -1, "no implicit additive role for other spells");
    auto old_cfg = config; old_cfg.additive_miracles = false; Env old(old_cfg); const auto original = old.snapshot();
    const CardId ids[] = {197,217}; check(!old.script_draws(ids) && old.snapshot() == original, "old pool excludes both spells");
    Config minimum; minimum.miracles = minimum.elements = minimum.chance = minimum.harms = minimum.celestial = minimum.additive_miracles = true;
    auto narrow = fixture({5}, {120},99,minimum); abilities(narrow,1,0); auto repeated = ordered({16}); repeated.extra_count = 49;
    std::fill_n(repeated.extras.begin(),49,16);
    check(narrow.step(repeated).additive_recasts == 49 && narrow.state().players[0].retained_deals == 49,
        "minimal dependency profile tracks49 held extras without defensive or reflection flags"); roundtrip(narrow);
    minimum.celestial = false; bool threw = false;
    try { Env invalid(minimum); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "additive miracles explicitly require celestial prerequisites");
}
void sequential_learning_and_atomic_rejection() {
    auto env = fixture({5,197}, {120}); gifts(env);
    const auto result = env.step(ordered({1,16}));
    check(result.ordered_attack && result.ordered_components == 2 && result.additive_first_uses == 1 && result.additive_recasts == 1
        && result.additive_mp_spent == 4 && !result.spell_first_use && !result.spell_recast
        && env.state().players[0].additive_abilities[0] == 1 && env.state().players[0].mp == 6
        && env.state().players[0].pending_deals == 3 && env.state().players[0].retained_deals == 1
        && env.state().pending.amount == 5 && !env.state().pending.miracle,
        "hand learns before later held ref, mixed usage counters and gifts each reflect actual uses");
    check(env.state().attack_composition.base == 5 && env.state().attack_composition.initial_mp == 10
        && env.state().attack_composition.count == 2 && env.state().attack_composition.items[0] == 197
        && env.state().attack_composition.items[1] == 197 && env.state().attack_composition.held_mask == 2
        && !env.state().attack_composition.initial_additive[0], "immutable ordered IDs and pre-acquisition holdings captured"); roundtrip(env);
    const auto public_trace = env.observe();
    check(public_trace[769] == 1.f && public_trace[770] == 5/363.f && public_trace[771] == 2/64.f
        && public_trace[772] == 10/99.f && public_trace[773] == 4/99.f && public_trace[775] == 197/363.f
        && public_trace[776] == 0.f && public_trace[777] == 197/363.f && public_trace[778] == 1.f,
        "public ordered IDs and hand/held flags preserve selection order without private pre-holdings");
    env.step(defend()); check(env.snapshot().draw_index == 3 && env.state().attack_composition == AttackComposition{},
        "composition clears only after item ends and all3 gifts are drawn"); roundtrip(env);
    auto inverse = fixture({5,197}, {120}); reject(inverse,ordered({16,1}));
    auto duplicate = fixture({5,197}, {120}); reject(duplicate,ordered({1,1})); reject(duplicate,ordered({0}));
    for (std::uint8_t reference : {std::uint8_t{2},std::uint8_t{17},std::uint8_t{18},std::uint8_t{255}}) reject(duplicate,ordered({reference}));
    auto same_ids = fixture({5,197,197}, {120}); const auto twice = same_ids.step(ordered({1,2,16}));
    check(twice.additive_first_uses == 2 && twice.additive_recasts == 1 && same_ids.state().players[0].additive_abilities[0] == 2,
        "same ID physical instances are distinct and holding duplicates are not charges"); roundtrip(same_ids);
    auto short_cfg = config; short_cfg.max_decisions = 1;
    auto many = fixture({5}, {120},99,short_cfg); auto snapshot = many.snapshot();
    snapshot.state.players[0].hand.fill(197); snapshot.state.players[0].hand[0] = 5; snapshot.state.players[0].hand_count = 16;
    check(many.restore(snapshot), "restore15 simultaneous physical first uses"); auto fifteen = ordered({}); fifteen.extra_count = 15;
    for (int i = 0; i < 15; ++i) fifteen.extras[i] = static_cast<std::uint8_t>(i+1);
    check(many.step(fifteen).additive_first_uses == 15 && many.state().players[0].additive_abilities[0] == 15
        && many.state().truncated && many.state().decisions == 1,
        "one decision may acquire15 holdings and truncate with a valid unresolved composition"); roundtrip(many);
    auto capped = fixture({5,197}, {120},10,short_cfg); abilities(capped,15,0); reject(capped,ordered({1}));
    check(capped.step(ordered({16})).additive_recasts == 1,
        "acquisition cap forbids new hand learning but permits existing held use"); roundtrip(capped);
    auto unavailable = fixture({5}, {120},99); abilities(unavailable,1,0); auto fifty = ordered({16}); fifty.extra_count = 50;
    std::fill_n(fifty.extras.begin(),50,16); reject(unavailable,fifty);
    auto malformed = fixture({5,9}, {120});
    auto action = ordered({1}); action.slots = 2; reject(malformed,action);
    action = ordered({1}); action.counter_count = 1; reject(malformed,action);
    action = ordered({1}); action.extras[1] = 1; reject(malformed,action);
    action = ordered({1}); action.extra_count = 65; reject(malformed,action);
    reject(malformed,ordered({}));
    auto plain = Action{ActionKind::attack,0,0,1}; plain.extra_count = 1; plain.extras[0] = 1; reject(malformed,plain);
    for (CardId base : {CardId{100},CardId{197},CardId{198},CardId{120},CardId{227}}) {
        auto wrong = fixture({base,9}, {120}); reject(wrong,ordered({1}));
    }
    auto magic = fixture({197,9}, {120}); reject(magic,{ActionKind::use,0,2,1});
    auto forbidden = fixture({5,216}, {120}); reject(forbidden,ordered({1}));
}
void magical_reset_order_and_attribute_order() {
    for (bool reverse : {false,true}) {
        auto env = fixture({57,44,197}, {39}); const auto result = env.step(reverse ? ordered({2,1}) : ordered({1,2}));
        check(result.mana_weapon_attack && result.additive_mp_spent == 2 && result.mana_weapon_mp_spent == 8
            && result.mana_weapon_power == (reverse ? 29 : 18) && env.state().pending.amount == (reverse ? 29 : 18)
            && env.state().players[0].mp == 0, "paid MAGIC resets57 before its own amount and loses only earlier boosts"); roundtrip(env);
        const auto record = env.state().attack_composition; env.step(special(0));
        check(env.state().attack_composition == record && env.state().pending.amount == (reverse ? 29 : 18)
            && env.state().players[0].mp == 0 && env.state().players[1].mp == 10,
            "physical reflection preserves frozen reset order and original payments"); roundtrip(env);
    }
    for (bool reverse : {false,true}) {
        auto env = fixture({57}, {120}); abilities(env,1,1);
        const auto result = env.step(reverse ? ordered({17,16}) : ordered({16,17}));
        check(result.additive_recasts == 2 && result.additive_mp_spent == 9 && result.mana_weapon_mp_spent == 1
            && env.state().pending.amount == (reverse ? 4 : 12), "197/217 order changes last MAGICAL reset contribution"); roundtrip(env);
    }
    auto exact = fixture({57}, {120},2); abilities(exact,1,0);
    check(exact.step(ordered({16})).mana_weapon_power == 2 && exact.state().players[0].mp == 0,
        "last paid197 can leave no MP while its own2 ATK remains"); roundtrip(exact);
    for (bool setter_last : {false,true}) {
        auto env = fixture({5,65,197}, {196}); const auto result = env.step(setter_last ? ordered({2,1}) : ordered({1,2}));
        check(result.ordered_attack && env.state().pending.element == Element::fire,
            "same-color197 and setter preserveFire irrespective order"); roundtrip(env);
        auto water = fixture({5,85,197}, {196}); water.step(setter_last ? ordered({2,1}) : ordered({1,2}));
        check(water.state().pending.element == (setter_last ? Element::water : Element::neutral),
            "ordered Fire197 after Water setter neutralizes; later setter restoresWater"); roundtrip(water);
    }
    auto self = fixture({57,44,197}, {120}); check(self.step(ordered({2,1},0,0)).valid && self.state().players[0].hp == 11
        && self.state().players[0].mp == 0 && self.state().attack_composition == AttackComposition{},
        "ordered self-target applies final29 then clears trace"); roundtrip(self);
}
void maximum_components_power_and_derived_counter() {
    auto env = fixture({56}, {120},99); auto snapshot = env.snapshot(); auto& p = snapshot.state.players[0];
    p.hand.fill(170); p.hand[0] = 56; p.hand_count = 16; p.additive_abilities[0] = 1;
    check(env.restore(snapshot), "restore15 boost hand and repeated held197"); gifts(env);
    auto action = ordered({}); action.extra_count = 64;
    for (int i = 0; i < 15; ++i) action.extras[i] = static_cast<std::uint8_t>(i+1);
    std::fill(action.extras.begin()+15,action.extras.begin()+legacy_attack_extra_capacity,16);
    const auto rng = env.snapshot().rng_state; const auto result = env.step(action);
    check(result.ordered_components == 64 && result.additive_recasts == 49 && result.additive_mp_spent == 98
        && env.state().pending.amount == 353 && env.state().players[0].mp == 1
        && env.state().players[0].pending_deals == 65 && env.state().players[0].retained_deals == 49
        && env.state().attack_composition.held_mask == (std::numeric_limits<std::uint64_t>::max()<<15),
        "64 extras consume16 physical cards and49 repeated held uses, yielding65 gifts"); roundtrip(env);
    env.step(defend()); check(env.snapshot().draw_index == 64 && env.snapshot().rng_state != rng
        && env.state().players[0].hand_count == 16 && env.state().players[0].hand[15] == 120,
        "65th overflow draw resumes seeded RNG after64 fixture outcomes"); roundtrip(env);
    auto maximum = fixture({56}, {120},99); snapshot = maximum.snapshot();
    auto& maxp = snapshot.state.players[0]; maxp.hand.fill(170); maxp.hand[0] = 56; maxp.hand_count = 16; maxp.additive_abilities[1] = 1;
    check(maximum.restore(snapshot), "restore maximum normal-weapon contribution"); action.extra_count = 29; action.extras.fill(0);
    for (int i = 0; i < 15; ++i) action.extras[i] = static_cast<std::uint8_t>(i+1);
    std::fill_n(action.extras.begin()+15,14,17); maximum.step(action);
    check(maximum.state().pending.amount == 395, "base30 +225 boosts +140 cost-bounded magic stays below480"); roundtrip(maximum);
    auto counter_env = fixture({5,197}, {190}); counter_env.step(ordered({1,16})); auto response = defend(1); response.counter_count = 1;
    const auto queued = counter_env.step(response);
    check(queued.counter_queued == 1 && counter_env.state().pending.counter_card == 190 && counter_env.state().pending.amount == 10
        && counter_env.state().attack_composition == AttackComposition{} && !counter_env.state().pending.miracle,
        "counter uses combined raw residual then becomes independent of composition and MAGIC components"); roundtrip(counter_env);
}
void weapon_vs_magic_reactions_and_restore() {
    for (CardId reaction : {CardId{39},CardId{196},CardId{215}}) {
        auto env = fixture({5,197}, {reaction}); env.step(ordered({1,16})); const auto record = env.state().attack_composition;
        const auto result = env.step(special(0));
        check(result.valid && (result.reflected || result.weapon_blocked) && !result.magic_reflected,
            "adding MAGIC does not reclassify original WEAPON for physical reaction/block");
        if (!result.weapon_blocked) check(env.state().attack_composition == record && env.state().pending.amount == 5,
            "source swap leaves immutable composition intact");
        roundtrip(env);
    }
    auto types = fixture({5,197}, {40,210,185}); types.step(ordered({1}));
    reject(types,special(0)); reject(types,special(1)); reject(types,special(2));
    auto env = fixture({57,44,197}, {39}); env.step(ordered({2,1})); const auto exact = env.snapshot();
    for (int error = 0; error < 13; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.attack_composition.base = 5;
        if (error == 1) bad.state.attack_composition.initial_mp = 9;
        if (error == 2) std::swap(bad.state.attack_composition.items[0],bad.state.attack_composition.items[1]);
        if (error == 3) bad.state.attack_composition.items[2] = 197;
        if (error == 4) bad.state.attack_composition.held_mask = 1;
        if (error == 5) bad.state.players[0].additive_abilities[0] = 0;
        if (error == 6) bad.state.pending.amount = 30;
        if (error == 7) bad.state.players[0].mp = 1;
        if (error == 8) bad.state.attack_composition.count = 65;
        if (error == 9) bad.state.attack_composition.initial_additive[0] = std::numeric_limits<std::uint64_t>::max();
        if (error == 10) bad.state.attack_composition.held_mask = 1ull<<63;
        if (error == 11) bad.state.pending.miracle = 197;
        if (error == 12) bad.config.additive_miracles = false;
        check(!env.restore(bad) && env.snapshot() == exact, "forged ordered trace/cost/reset/ownership/tail/profile rejected atomically");
    }
    auto chain = fixture({5,197,39}, {39}); chain.step(ordered({1,16})); chain.step(special(0)); roundtrip(chain);
    const auto chain_exact = chain.snapshot(); auto forged_chain = chain_exact;
    forged_chain.state.reaction_count = 3; forged_chain.state.decisions = 10;
    check(!chain.restore(forged_chain) && chain.snapshot() == chain_exact,
        "initial held-extra gifts cannot masquerade as physical weapon-reflection consumption");
    for (int seat = 0; seat < 2; ++seat) {
        auto bad = chain_exact; ++bad.state.players[seat].retained_deals;
        check(!chain.restore(bad) && chain.snapshot() == chain_exact,
            "unresolved weapon chain retained gifts are exactly initial held extras, not response casts");
    }
    auto hidden = fixture({5}, {197}); const auto visible = hidden.observe(); auto changed = hidden.snapshot();
    changed.state.players[1].hand[0] = 120; changed.state.players[1].additive_abilities[0] = 1; changed.rng_state ^= 0x9e3779b97f4a7c15ULL;
    check(hidden.restore(changed) && hidden.observe() == visible, "opponent unplayed spells/full holdings and future RNG remain private");
    auto old_cfg = config; old_cfg.additive_miracles = false; Env old(old_cfg); const auto oldobs = old.observe();
    check(std::all_of(oldobs.begin()+750,oldobs.end(),[](float x){return x == 0.f;}), "old profiles new153 suffix zero");
}
void seeded_replay() {
    std::uint64_t ordered_attacks = 0, spells = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+108000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "additive seeded state and observation replay");
            for (float x : a.observe()) check(std::isfinite(x), "composition observation finite");
            const auto action = a.sample_action(pa), twin = b.sample_action(pb);
            check(action == twin && pa == pb && a.legal(action), "ordered sampler produces identical legal refs");
            const auto ra = a.step(action), rb = b.step(twin);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.ordered_attack == rb.ordered_attack
                && ra.additive_first_uses == rb.additive_first_uses && ra.additive_recasts == rb.additive_recasts
                && ra.additive_mp_spent == rb.additive_mp_spent, "additional spell count/cost/reward events replay");
            ordered_attacks += ra.ordered_attack; spells += ra.additive_first_uses + ra.additive_recasts; roundtrip(a);
        }
    }
    check(ordered_attacks && spells, "natural pool exercises ordered and standalone additional magic");
}
}
int main() {
    metadata_standalone_and_local_masks(); sequential_learning_and_atomic_rejection(); magical_reset_order_and_attribute_order();
    maximum_components_power_and_derived_counter(); weapon_vs_magic_reactions_and_restore(); seeded_replay();
    std::cout << "Additive miracles passed (ordered learning/reset/64 refs/65 gifts/type/restore,1000 replay rollouts).\n";
}
