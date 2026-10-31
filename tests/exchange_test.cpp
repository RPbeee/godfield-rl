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
using Resources = std::array<int,3>;
const Config config = [] {
    Config c; c.miracles = c.elements = c.chance = c.reflection = c.disease = c.harms = true;
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL exchange: " << message << '\n'; std::exit(1); } }
Action exchange(Resources allocation, std::uint8_t base = 0) {
    Action a{ActionKind::exchange,base,0,no_slot}; a.extra_count = 3;
    for (int i = 0; i < 3; ++i) a.extras[i] = static_cast<std::uint8_t>(allocation[i]);
    return a;
}
Resources resources(const Player& p) { return {p.hp,p.mp,p.money}; }
void set(Player& p, Resources r) { p.hp = r[0]; p.mp = r[1]; p.money = r[2]; }
Resources integers(const std::array<std::uint8_t,3>& r) { return {r[0],r[1],r[2]}; }
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own = {2}, std::initializer_list<CardId> enemy = {120}, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy);
    check(env.restore(s), "restore explicit exchange fixture"); return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "resource allocation and public history roundtrip exactly"); }
void reject(Env& env, Action a) { const auto old = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == old, "bad allocation/params/ownership rejects atomically"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "script earned exchange gift"); }
void catalog_minimum_and_allocation_boundaries() {
    check(exchange_card_count == 1 && exchange_catalog().size() == 1 && exchange_index(2) == 0 && static_cast<int>(Kind::trade) == 5,
        "one trade EXCHANGE card with stable added kind");
    const auto* card = find_exchange_card(2);
    check(card && card->card.kind == Kind::trade && !card->card.attack && !card->card.boost && !card->card.defense
        && !card->card.hp && !card->card.mp && card->card.price == 10 && card->card.weight == 20 && card->card.discardable,
        "exchange2 retains ordinary fields and is discardable");
    for (CardId id : {CardId{0},CardId{1},CardId{3},CardId{244},CardId{65535}}) check(!find_exchange_card(id), "no buy/sell/revival effects silently included");
    Config minimum; minimum.exchanges = true; auto narrow = fixture({2},{120},minimum);
    check(std::string_view(narrow.profile()) == exchange_ruleset && narrow.snapshot().version == 37
        && narrow.options().exchange_slots == 1 && narrow.options().discard_slots == 1 && !narrow.options().utility_slots
        && !narrow.options().weapon_bases && narrow.options().pray,
        "dependency-free main exchange uses dedicated role and keeps weapon-based prayer");
    draws(narrow); const auto result = narrow.step(exchange({10,20,40}));
    check(result.valid && result.exchanged && result.exchange_before == Resources{40,10,20} && result.exchange_after == Resources{10,20,40}
        && resources(narrow.state().players[0]) == Resources{10,20,40} && resources(narrow.state().players[1]) == Resources{40,10,20}
        && narrow.state().phase == Phase::main && narrow.state().pending == Pending{} && narrow.snapshot().draw_index == 1
        && narrow.state().players[0].hand_count == 1 && narrow.state().players[0].hand[0] == 120,
        "exchange reallocates only its owner immediately and consumes one card for one replacement"); roundtrip(narrow);
    constexpr std::array<Resources,5> before{{{1,0,0},{40,10,20},{33,33,33},{99,99,0},{99,99,99}}};
    constexpr std::array<Resources,5> after{{{1,0,0},{1,0,69},{99,0,0},{0,99,99},{99,99,99}}};
    for (int i = 0; i < 5; ++i) {
        auto env = fixture(); auto s = env.snapshot(); set(s.state.players[0],before[i]); check(env.restore(s), "restore allocation total boundary");
        const auto r = env.step(exchange(after[i]));
        check(r.valid && r.exchanged && r.exchange_before == before[i] && r.exchange_after == after[i]
            && resources(env.state().players[0]) == after[i] && env.state().last_exchange.id == 2
            && env.state().last_exchange.seat == 0 && integers(env.state().last_exchange.before) == before[i]
            && integers(env.state().last_exchange.after) == after[i], "totals1/70/99/198/297 preserve all resources exactly"); roundtrip(env);
    }
    auto discarded = fixture({2},{120},minimum);
    check(discarded.step({ActionKind::discard,no_slot,1,no_slot}).valid && !discarded.state().players[0].hand_count
        && discarded.state().last_exchange.id == 0, "discarding trade card earns no replacement or allocation event"); roundtrip(discarded);
    auto old_cfg = config; old_cfg.exchanges = false; Env old(old_cfg); const auto old_state = old.snapshot(); const CardId id[] = {2};
    check(!old.script_draws(id) && old.snapshot() == old_state, "old pool excludes exchange2");
}
void malformed_allocations_and_private_prefixes() {
    auto env = fixture({2,120}); reject(env,exchange({10,20,39})); reject(env,exchange({10,20,41})); reject(env,exchange({0,100,0})); reject(env,exchange({255,0,0}));
    auto action = exchange({10,20,40}); action.extra_count = 2; reject(env,action);
    action = exchange({10,20,40}); action.extra_count = 4; reject(env,action);
    action = exchange({10,20,40}); action.extras[3] = 1; reject(env,action);
    action = exchange({10,20,40}); action.target = 0; reject(env,action);
    action = exchange({10,20,40}); action.slots = 1; reject(env,action);
    action = exchange({10,20,40}); action.counter_count = 1; reject(env,action);
    action = exchange({10,20,40}); action.counter_order = 1; reject(env,action);
    reject(env,exchange({10,20,40},1)); reject(env,exchange({10,20,40},2));
    reject(env,{ActionKind::use,0,0,0}); reject(env,{ActionKind::attack,0,0,1});
    auto free = Action{ActionKind::cast_ordered,0,0,1}; free.extra_count = 1; free.extras[0] = 1; reject(env,free);
    auto a = fixture(), b = fixture(); auto changed = b.snapshot(); changed.rng_state ^= 0x9e3779b97f4a7c15ULL;
    check(b.restore(changed), "restore alternative hidden RNG"); const CardId first[] = {120}, second[] = {177};
    check(a.script_draws(first) && b.script_draws(second) && a.observe() == b.observe()
        && a.legal(exchange({10,20,40})) == b.legal(exchange({10,20,40})),
        "future RNG and replacement prefix do not leak into allocation observation or legality");
}
void hp_zero_disease_flags_and_later_mana_weapon() {
    auto dead = fixture(); auto s = dead.snapshot(); s.state.players[0].disease = Disease::heaven;
    s.state.players[0].glory = s.state.players[0].dark_cloud = true; s.state.players[0].abilities[0] = 1;
    check(dead.restore(s), "restore Heaven/harms/holdings owner"); draws(dead); const auto result = dead.step(exchange({0,50,20}));
    check(result.exchanged && !result.disease_tick && dead.done() && dead.state().winner == 1 && dead.state().players[0].dead
        && dead.state().players[0].hp == 0 && dead.state().players[0].disease == Disease::heaven
        && dead.state().players[0].glory && dead.state().players[0].dark_cloud && dead.state().players[0].abilities[0] == 1
        && dead.snapshot().draw_index == 0 && dead.state().players[0].pending_deals == 1,
        "HP0 allocation finalizes death, skips Heaven heal and replacement, preserves status/holdings"); roundtrip(dead);
    for (Disease disease : {Disease::hell,Disease::heaven}) {
        auto env = fixture(); s = env.snapshot(); s.state.players[0].disease = disease; s.state.players[0].glory = true;
        check(env.restore(s), "restore positive allocation disease tick"); const std::uint8_t roll[] = {99}; check(env.script_disease_rolls(roll), "avoid natural worsening");
        const auto ticked = env.step(exchange({1,69,0}));
        check(ticked.exchanged && ticked.disease_tick && env.state().last_exchange.after[0] == 1
            && env.state().players[0].hp == (disease == Disease::hell ? 0 : 6) && env.state().players[0].glory,
            "owner disease ticks after redistribution, so historical afterHP differs from currentHP"); roundtrip(env);
    }
    auto env = fixture({2,57}); draws(env); env.step(exchange({20,50,0}));
    for (int turns = 0; env.state().actor != 0 && turns < 3; ++turns) check(env.step({ActionKind::pray}).valid, "advance opponent without changing ownerMP");
    check(env.state().actor == 0 && env.state().players[0].hand[0] == 57 && env.state().players[0].mp == 50,
        "earned replacement leaves next weapon and reallocated MP intact");
    const auto weapon = env.step({ActionKind::attack,0,0,1});
    check(weapon.mana_weapon_attack && weapon.mana_weapon_mp_spent == 50 && weapon.mana_weapon_power == 100
        && env.state().last_exchange.after[1] == 50 && env.state().players[0].mp == 0,
        "later57 uses newly allocatedMP while old exchange history remains immutable"); roundtrip(env);
}
void public_history_strictness_and_sampler_replay() {
    auto env = fixture(); const auto before = env.observe(); check(before[1017] == 1.f && before[1018] == 1.f, "new enabled and own exchange2 positions");
    env.step(exchange({10,20,40})); const auto exact = env.snapshot(); const auto obs = env.observe();
    check(obs[1034] == 1.f && obs[1036] == 40/99.f && obs[1037] == 10/99.f && obs[1038] == 20/99.f
        && obs[1039] == 10/99.f && obs[1040] == 20/99.f && obs[1041] == 40/99.f && obs[1042] == 70/297.f,
        "public exchange event preserves resource ordering and normalized total");
    for (int error = 0; error < 9; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.last_exchange.id = 1;
        if (error == 1) bad.state.last_exchange.seat = 255;
        if (error == 2) bad.state.last_exchange.after[0] = 100;
        if (error == 3) bad.state.last_exchange.before[1] = 11;
        if (error == 4) bad.state.last_exchange.after[2] = 41;
        if (error == 5) bad.config.exchanges = false;
        if (error == 6) bad.version = 24;
        if (error == 7) bad.state.last_exchange.before = {0,50,20};
        if (error == 8) { bad.state.last_exchange.before = {}; bad.state.last_exchange.after = {}; }
        check(!env.restore(bad) && env.snapshot() == exact, "malformed public exchange record/profile/schema rejected atomically");
    }
    auto unit = fixture(); auto s = unit.snapshot(); set(s.state.players[0],{1,0,0}); check(unit.restore(s), "restore total1 allocation sampler");
    std::array<bool,3> seen{}; std::uint64_t policy = 0xabba;
    for (int i = 0; i < 3000; ++i) {
        const auto action = unit.sample_action(policy); check(unit.legal(action), "low-total sampler never emits out-of-range split");
        if (action.kind == ActionKind::exchange) {
            check(action.extras[0]+action.extras[1]+action.extras[2] == 1, "total1 allocation conserved");
            for (int j = 0; j < 3; ++j) if (action.extras[j]) seen[j] = true;
        }
    }
    check(std::all_of(seen.begin(),seen.end(),[](bool x){return x;}), "sampler reaches all three total1 allocations");
    auto old_cfg = config; old_cfg.exchanges = false; Env old(old_cfg); const auto old_obs = old.observe();
    check(std::all_of(old_obs.begin()+1017,old_obs.end(),[](float x){return x == 0.f;}), "old profiles new26 features zero");
    std::uint64_t exchanges = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+113000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "exchange seeded state/observation replay");
            for (float x : a.observe()) check(std::isfinite(x), "resource allocation observations finite");
            const auto action = a.sample_action(pa), twin = b.sample_action(pb);
            check(action == twin && pa == pb && a.legal(action), "allocation sampler deterministic and legal");
            const auto ra = a.step(action), rb = b.step(twin);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.exchanged == rb.exchanged
                && ra.exchange_before == rb.exchange_before && ra.exchange_after == rb.exchange_after,
                "exchange resource deltas and reward replay"); exchanges += ra.exchanged; roundtrip(a);
        }
    }
    check(exchanges, "natural pool exercises exchange2");
}
}
int main() {
    catalog_minimum_and_allocation_boundaries(); malformed_allocations_and_private_prefixes(); hp_zero_disease_flags_and_later_mana_weapon(); public_history_strictness_and_sampler_replay();
    std::cout << "Exchange passed (resource boundaries/HP0/disease/history/next57,1000 replay rollouts).\n";
}
