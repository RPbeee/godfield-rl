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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL forgetting: " << message << '\n'; std::exit(1); } }
Action use(std::uint8_t target = 1) { return {ActionKind::use,0,0,target}; }
Action accept() { return {ActionKind::defend,no_slot,0,no_slot}; }
Action any(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action defense(std::initializer_list<std::uint8_t> refs) {
    Action a{ActionKind::defend_ordered,no_slot,0,no_slot}; a.extra_count = static_cast<std::uint8_t>(refs.size()); std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
void hand(Player& p, std::initializer_list<CardId> cards) {
    p.hand.fill(0); std::copy(cards.begin(),cards.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(cards.size()); p.pending_deals = p.retained_deals = 0;
}
void set(Player& p, CardId id, std::uint64_t n) {
    if (const int i = miracle_index(id); i >= 0) p.abilities[i] = static_cast<std::uint32_t>(n);
    else if (const int i = offensive_miracle_index(id); i >= 0) p.abilities[miracle_count+i] = static_cast<std::uint32_t>(n);
    else if (const int i = defensive_miracle_index(id); i >= 0) p.defensive_abilities[i] = static_cast<std::uint32_t>(n);
    else if (const int i = additive_miracle_index(id); i >= 0) p.additive_abilities[i] = n;
    else if (id == 214) p.doubling_ability = n;
    else if (id == 209) p.wide_ability = n;
    else check(false, "unsupported test holding ID");
}
std::uint64_t count(const Player& p, CardId id) {
    if (const int i = miracle_index(id); i >= 0) return p.abilities[i];
    if (const int i = offensive_miracle_index(id); i >= 0) return p.abilities[miracle_count+i];
    if (const int i = defensive_miracle_index(id); i >= 0) return p.defensive_abilities[i];
    if (const int i = additive_miracle_index(id); i >= 0) return p.additive_abilities[i];
    if (id == 214) return p.doubling_ability;
    if (id == 209) return p.wide_ability;
    check(false,"unknown test holding ID"); return 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto snapshot = env.snapshot(); snapshot.state = State{}; hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy);
    check(env.restore(snapshot), "restore explicit forgetting fixture"); return env;
}
void roundtrip(const Env& env) { Env resumed(env.snapshot().config); check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "forgotten holdings and historical records roundtrip exactly"); }
void reject(Env& env, Action a) { const auto old = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == old, "untyped forgetting action rejects illegal defense/type atomically"); }
void ticket(Env& env, std::uint64_t n) { const std::uint64_t ids[] = {n}; check(env.script_forget_tickets(ids), "set raw weighted-rank ticket"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "isolate forgetting RNG from gift draws"); }
void metadata_empty_and_singleton_rng() {
    check(forgetting_card_count == 1 && forgetting_catalog().size() == 1 && forgetting_index(241) == 0,
        "only positive-weight forgetting241 in scope");
    const auto* item = find_forgetting_card(241);
    check(item && item->amount == 1 && item->card.kind == Kind::sundry && item->card.price == 10 && item->card.weight == 3
        && !item->card.attack && !item->card.boost && !item->card.defense && !item->card.hp && !item->card.mp && item->card.discardable,
        "SUNDRY forgets one, costs no MP and does not become a retained miracle");
    for (CardId id : {CardId{0},CardId{239},CardId{240},CardId{250},CardId{65535}}) check(!find_forgetting_card(id), "no implicit creature/other utility inclusion");
    Config minimal; minimal.miracles = minimal.forgetting = true;
    auto empty = fixture({241}, {120},minimal); draws(empty); ticket(empty,999); const auto before = empty.snapshot();
    check(std::string_view(empty.profile()) == forgetting_ruleset && empty.snapshot().version == 35
        && empty.options().forget_slots == 1 && empty.options().utility_slots == 1 && empty.options().discard_slots == 1 && empty.options().pray,
        "minimum profile exposes utility/discard and keeps weapon-based prayer");
    const auto none = empty.step(use(0));
    check(none.valid && none.forget_attempt && !none.forget_removed && none.forget_population == 0 && !none.forgotten_miracle
        && empty.snapshot().forget_index == 0 && empty.snapshot().rng_state == before.rng_state
        && empty.state().players[0].mp == 10 && empty.state().last_forget.id == 241 && !empty.state().last_forget.removed,
        "empty self target consumes no ticket/RNG while normal gift still occurs"); roundtrip(empty);
    auto singleton = fixture({241}, {120},minimal); auto snapshot = singleton.snapshot(); set(snapshot.state.players[0],216,1);
    check(singleton.restore(snapshot), "restore singleton holding"); draws(singleton); const auto rng = singleton.snapshot().rng_state;
    check(singleton.step(use(0)).forgotten_miracle == 216 && singleton.snapshot().rng_state != rng,
        "singleton still consumes default RNG even though its outcome is certain"); roundtrip(singleton);
    auto taped = fixture({241}, {120},minimal); snapshot = taped.snapshot(); set(snapshot.state.players[0],216,1);
    check(taped.restore(snapshot), "restore taped singleton"); draws(taped); ticket(taped,std::numeric_limits<std::uint64_t>::max());
    check(taped.step(use(0)).forget_removed && taped.snapshot().forget_index == 1 && !count(taped.state().players[0],216),
        "raw modulo-one ticket is consumed once and removes final copy"); roundtrip(taped);
    auto mp0 = fixture({241}, {120}); snapshot = mp0.snapshot(); snapshot.state.players[0].mp = 0; check(mp0.restore(snapshot), "restore0MP forgetting");
    check(mp0.step(use()).valid && mp0.state().pending.kind == Effect::forget && mp0.state().pending.element == Element::none
        && mp0.state().pending.forget_card == 241 && !mp0.state().pending.amount && !mp0.state().pending.miracle,
        "forgetting is an untyped zero-amount response payload at0MP"); roundtrip(mp0);
    auto old_cfg = config; old_cfg.forgetting = false; Env old(old_cfg); const auto old_state = old.snapshot(); const CardId id[] = {241};
    check(!old.script_draws(id) && old.snapshot() == old_state, "old profile pool excludes241");
}
void all_weighted_families_duplicates_and_large_total() {
    for (std::size_t chosen = 0; chosen < retained_type_ids.size(); ++chosen) {
        auto env = fixture({241}, {120}); auto snapshot = env.snapshot();
        for (CardId id : retained_type_ids) set(snapshot.state.players[1],id,1);
        check(env.restore(snapshot), "restore one copy of every retained family"); ticket(env,chosen); env.step(use()); const auto result = env.step(accept());
        check(result.forget_attempt && result.forget_removed && result.forget_population == 21
            && result.forgotten_miracle == retained_type_ids[chosen] && result.forgotten_from == 1
            && env.state().last_forget.removed == retained_type_ids[chosen] && env.state().last_forget.source == 0
            && env.state().last_forget.target == 1 && env.state().last_forget.population == 21,
            "canonical ascending-ID weighted rank crosses every storage family boundary");
        for (CardId id : retained_type_ids) check(count(env.state().players[1],id) == (id == retained_type_ids[chosen] ? 0 : 1),
            "forget decrements only selected copy");
        roundtrip(env);
    }
    for (std::uint64_t rank = 0; rank < 4; ++rank) {
        auto env = fixture({241}, {120}); auto snapshot = env.snapshot(); set(snapshot.state.players[1],197,3); set(snapshot.state.players[1],216,1);
        check(env.restore(snapshot), "restore weighted duplicates"); ticket(env,rank); env.step(use()); const auto result = env.step(accept());
        check(result.forgotten_miracle == (rank < 3 ? 197 : 216) && count(env.state().players[1],197) == (rank < 3 ? 2 : 3),
            "three identical copies occupy three rank buckets and only one is removed"); roundtrip(env);
    }
    auto cfg = config; cfg.max_decisions = std::numeric_limits<std::uint32_t>::max(); auto big = fixture({241}, {120},cfg);
    auto snapshot = big.snapshot(); set(snapshot.state.players[1],197,1ULL<<32); set(snapshot.state.players[1],216,1);
    check(big.restore(snapshot), "restore legitimate weighted population above uint32"); ticket(big,1ULL<<32); big.step(use());
    const auto result = big.step(accept());
    check(result.forget_population == (1ULL<<32)+1 && result.forgotten_miracle == 216 && count(big.state().players[1],197) == (1ULL<<32),
        "uint64 total and ticket select bucket beyond2^32 without truncation"); roundtrip(big);
}
void none_defense_any_return_and_free_reference() {
    auto env = fixture({241}, {120,195,39,20,40,210,215,185,196,239}); auto snapshot = env.snapshot(); set(snapshot.state.players[0],216,1);
    check(env.restore(snapshot), "restore original caster holding for returned forget"); draws(env); ticket(env,0); env.step(use());
    check(!env.options().defense_slots && !env.options().removal_slots && !env.options().counter_slots && env.options().reaction_slots == (1u<<8),
        "untyped payload offers only unconditional196 reaction");
    for (int slot = 0; slot < 8; ++slot) reject(env,any(slot));
    reject(env,{ActionKind::defend,no_slot,1,no_slot}); reject(env,any(8,1)); reject(env,any(8,2)); reject(env,defense({0,8}));
    const auto response = env.step(defense({8,9}));
    check(response.any_reflected && response.magic_free_suffix && !response.magic_free_cards && !response.magic_free_mp_saved
        && env.state().pending.element == Element::none && env.state().pending.source == 1 && env.state().pending.target == 0,
        "196 first may leave unused FREE suffix while preserving untyped returned effect"); roundtrip(env);
    const auto done = env.step(accept());
    check(done.forgotten_miracle == 216 && done.forgotten_from == 0 && env.state().last_forget.source == 1
        && !count(env.state().players[0],216) && env.state().players[0].hand_count == 1 && env.state().players[0].hand[0] == 120,
        "return removes from original caster and still pays its outstanding physical gift"); roundtrip(env);
    auto glory = fixture({241}, {196,239}); snapshot = glory.snapshot(); snapshot.state.players[1].glory = true;
    check(glory.restore(snapshot), "restore Glory untyped responder"); glory.step(use()); reject(glory,defense({0,1}));
    check(glory.step(any(0)).any_reflected, "Glory permits lone196"); roundtrip(glory);
    auto held = fixture({241}, {239,197}); snapshot = held.snapshot(); set(snapshot.state.players[1],197,1);
    check(held.restore(snapshot), "restore target with acquired and unused hand197"); ticket(held,0); held.step(use()); held.step(accept());
    Action recast{ActionKind::cast_ordered,16,0,1}; recast.extra_count = 1; recast.extras[0] = 0; reject(held,recast);
    reject(held,{ActionKind::cast_additive,0,0,1});
    Action learn{ActionKind::cast_ordered,1,0,1}; learn.extra_count = 1; learn.extras[0] = 0;
    check(held.step(learn).additive_first_uses == 1 && count(held.state().players[1],197) == 1,
        "forgotten held reference fails even with FREE, but untouched hand copy can learn again"); roundtrip(held);
    auto not_magic = fixture({241,239}, {120}); Action free{ActionKind::cast_ordered,0,0,1}; free.extra_count = 1; free.extras[0] = 1; reject(not_magic,free);
    auto extra = fixture({5,241}, {120}); Action attack{ActionKind::attack_ordered,0,0,1}; attack.extra_count = 1; attack.extras[0] = 1; reject(extra,attack);
}
void past_ability_reaction_survives_forgetting() {
    auto env = fixture({216}, {241,239}); auto snapshot = env.snapshot(); set(snapshot.state.players[1],210,1); snapshot.state.players[1].mp = 0;
    check(env.restore(snapshot), "restore free held210 with future forgetting241"); draws(env); env.step(use()); const std::uint8_t bounce[] = {0};
    check(env.script_bounce_targets(bounce), "return heal to original caster"); env.step(defense({20,1})); env.step(accept());
    check(env.state().actor == 1 && env.state().last_reaction.id == 210 && env.state().last_reaction.from_ability
        && env.state().last_reaction.free_card == 239 && env.state().players[1].hand[2] == 120,
        "held210's earned replacement was distributed before later forgetting");
    ticket(env,0); const auto result = env.step(use(1));
    check(result.forgotten_miracle == 210 && !count(env.state().players[1],210) && env.state().last_reaction.id == 210
        && env.state().players[1].hand[1] == 120,
        "forget removes last210 while keeping paid gift and its historical free reaction"); roundtrip(env);
}
void privacy_tapes_strictness_and_seeded_replay() {
    auto a = fixture({241}, {120}), b = fixture({241}, {120}); auto sa = a.snapshot(), sb = b.snapshot();
    for (auto* state : {&sa.state,&sb.state}) { set(state->players[1],197,1); set(state->players[1],216,1); }
    check(a.restore(sa) && b.restore(sb), "restore equivalent ticket privacy states"); ticket(a,0); ticket(b,1);
    check(a.observe() == b.observe() && a.legal(use()) == b.legal(use()), "future forget ticket not visible through observation/legal");
    a.step(use()); b.step(use()); check(a.observe() == b.observe() && a.legal(accept()) == b.legal(accept()), "unresolved forget never reveals future choice");
    check(a.step(accept()).forgotten_miracle == 197 && b.step(accept()).forgotten_miracle == 216,
        "only applied removal outcome becomes public");
    auto small = fixture({241}, {120}), large = fixture({241}, {120}); sa = small.snapshot(); sb = large.snapshot();
    set(sa.state.players[0],197,9); set(sb.state.players[0],197,99); check(small.restore(sa) && large.restore(sb), "restore different hidden populations");
    draws(small); draws(large); ticket(small,0); ticket(large,0); small.step(use(0)); large.step(use(0));
    check(small.state().last_forget.population != large.state().last_forget.population && small.observe() == large.observe(),
        "public removal ID does not disclose historical population or remaining opponent holdings");
    const auto exact = small.snapshot();
    for (int error = 0; error < 6; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.last_forget.removed = 241;
        if (error == 1) bad.state.last_forget.population = 0;
        if (error == 2) bad.state.last_forget.source = 255;
        if (error == 3) bad.state.last_forget.population = 16ULL*config.max_decisions+1;
        if (error == 4) bad.scripted_forgets[bad.forget_count] = 7;
        if (error == 5) bad.config.forgetting = false;
        check(!small.restore(bad) && small.snapshot() == exact, "malformed forget history/private tape/profile rejects atomically");
    }
    auto current = fixture({241}, {120}); current.step(use()); const auto pending = current.snapshot();
    auto wrong = pending; wrong.state.pending.element = Element::neutral;
    check(!current.restore(wrong) && current.snapshot() == pending, "forget cannot masquerade as neutral typed effect");
    auto old_cfg = config; old_cfg.forgetting = false; Env old(old_cfg); const auto obs = old.observe();
    check(std::all_of(obs.begin()+993,obs.end(),[](float x){return x == 0.f;}), "old profiles added24 features zero");
    auto own = fixture({241}, {120}); check(own.observe()[993] == 1.f && own.observe()[994] == 1.f, "new flag and own241 positions stable");
    std::uint64_t attempts = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env x(config), y(config); x.reset(seed); y.reset(seed); std::uint64_t px = seed+112000,py = px;
        while (!x.done()) {
            check(x.snapshot() == y.snapshot() && x.observe() == y.observe(), "forgetting seeded state/observation replay");
            for (float value : x.observe()) check(std::isfinite(value), "forgetting observations finite");
            const auto action = x.sample_action(px), twin = y.sample_action(py);
            check(action == twin && px == py && x.legal(action), "forgetting sampler deterministic/legal");
            const auto rx = x.step(action), ry = y.step(twin);
            check(rx.valid && ry.valid && rx.rewards == ry.rewards && rx.forget_attempt == ry.forget_attempt
                && rx.forget_removed == ry.forget_removed && rx.forgotten_miracle == ry.forgotten_miracle
                && rx.forget_population == ry.forget_population, "forget sampling/outcome/reward replay"); attempts += rx.forget_attempt; roundtrip(x);
        }
    }
    check(attempts, "natural pool exercises forgetting");
}
}
int main() {
    metadata_empty_and_singleton_rng(); all_weighted_families_duplicates_and_large_total(); none_defense_any_return_and_free_reference();
    past_ability_reaction_survives_forgetting(); privacy_tapes_strictness_and_seeded_replay();
    std::cout << "Forgetting passed (21weighted families/uint64/None/empty/history/privacy,1000 replay rollouts).\n";
}
