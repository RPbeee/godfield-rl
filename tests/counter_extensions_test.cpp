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
const Config config = [] {
    Config c; c.miracles = c.elements = c.chance = c.reflection = c.disease = c.harms = true;
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = true;
    return c;
}();
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL counter extensions: " << message << '\n'; std::exit(1); }
}
Action attack() { return {ActionKind::attack,0,0,1}; }
Action defend(std::uint16_t mask = 0, std::initializer_list<int> order = {}, int base = no_slot) {
    Action a{ActionKind::defend,static_cast<std::uint8_t>(base),mask,no_slot};
    a.counter_count = static_cast<std::uint8_t>(order.size()); int offset = 0;
    for (int slot : order) { a.counter_order |= std::uint64_t(slot) << offset; offset += 4; }
    return a;
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size());
    p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy);
    check(env.restore(snapshot), "restore explicit extended counter fixture"); return env;
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "extended counter pipeline roundtrip");
}
void reject(Env& env, Action action) {
    const auto original = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == original,
        "invalid counter order preserves all cards, pipeline, MP, RNG and tapes");
}
void rolls(Env& env, std::initializer_list<std::uint8_t> values) {
    check(env.script_hit_rolls({values.begin(),values.size()}), "script future counter hit outcomes");
}
void gifts(Env& env) { std::array<CardId,64> values{}; values.fill(120); check(env.script_draws(values), "script deferred ordered counter gifts"); }
void metadata_and_order_validation() {
    check(counter_extension_card_count == 4 && counter_extensions_catalog().size() == 4
        && counter_card_count == 1 && counter_catalog()[0].card.id == 190,
        "old190 catalog unchanged and four new counter variants separate");
    constexpr std::array<CardId,4> ids{187,191,192,193};
    constexpr std::array<Element,4> elements{Element::fire,Element::light,Element::dark,Element::neutral};
    for (int i = 0; i < 4; ++i) {
        const auto& item = counter_extensions_catalog()[i];
        check(item.card.id == ids[i] && find_counter_extension_card(ids[i]) == &item && item.element == elements[i]
            && item.card.kind == Kind::armor && !item.card.attack && !item.card.boost && !item.card.defense
            && !item.card.hp && !item.card.mp && item.card.price == 10 && item.card.weight == 1 && item.card.discardable,
            "new variant IDs, attributes and independent zero-DFS roles pinned");
    }
    const auto& fire = *find_counter_extension_card(187); const auto& mp = *find_counter_extension_card(193);
    check(fire.effect == CounterExtensionEffect::damage && fire.multiplier == 1 && fire.hit_rate == 75
        && fire.target == CounterExtensionTarget::all_enemies && mp.effect == CounterExtensionEffect::mana
        && mp.multiplier == 2 && mp.target == CounterExtensionTarget::self,
        "fire75 uses raw r and MP counter uses self2r");
    check(find_counter_extension_card(191)->harm == Harm::glory && find_counter_extension_card(192)->harm == Harm::dark_cloud,
        "direct Glory and DarkCloud variants retain harm payloads");
    for (CardId id : {CardId{0},CardId{188},CardId{189},CardId{190},CardId{194},CardId{65535}})
        check(!find_counter_extension_card(id), "fog, illusion, absorption and old190 excluded from new lookup");
    auto env = fixture({5}, {187,190,191,192,193,120});
    check(std::string_view(env.profile()) == counter_extensions_ruleset && env.snapshot().version == 37,
        "new extended-order profile and schema identified");
    auto malformed = attack(); malformed.counter_count = 1; reject(env,malformed);
    env.step(attack());
    reject(env,defend(31)); reject(env,defend(31,{0,1,2,3})); reject(env,defend(31,{0,1,2,3,3}));
    reject(env,defend(31,{0,1,2,3,5})); reject(env,defend(1,{0,1}));
    malformed = defend(1,{0}); malformed.counter_order |= std::uint64_t{1}<<4; reject(env,malformed);
    malformed = defend(1,{0}); malformed.counter_count = 17; reject(env,malformed);
    check(env.legal(defend(31,{4,0,2,1,3})), "all selected heterogeneous counters admit an explicit complete permutation");
    auto special_prefix = fixture({5}, {190,39}); special_prefix.step(attack());
    reject(special_prefix,defend(1,{},1));
    const auto returned = special_prefix.step(defend(1,{0},1));
    check(returned.reflected && !returned.counter_queued, "counter order required even when special prefix only consumes the counter");
    roundtrip(special_prefix);
    for (int missing = 0; missing < 2; ++missing) {
        auto bad = config; if (!missing) bad.counters = false; else bad.celestial = false;
        bool threw = false; try { Env invalid(bad); } catch (const std::invalid_argument&) { threw = true; }
        check(threw, "extensions require old counter and celestial semantics");
    }
    auto old_cfg = config; old_cfg.counter_extensions = false; Env old(old_cfg); const auto original = old.snapshot();
    check(!old.script_draws(ids) && old.snapshot() == original, "old profile does not acquire new counter IDs");
    auto prior = fixture({5}, {190}); auto snap = prior.snapshot(); snap.config.counter_extensions = false;
    Env legacy(snap.config); check(legacy.restore(snap), "old190 profile retains original hand fixture");
    legacy.step(attack()); check(legacy.step(defend(1)).counter_queued == 1,
        "old190-only action accepts its original zero-order encoding"); roundtrip(legacy);
    Config narrow; narrow.elements = narrow.chance = narrow.harms = narrow.celestial = narrow.counters = narrow.counter_extensions = true;
    auto independent = fixture({5}, {192,187},narrow); rolls(independent,{100}); independent.step(attack());
    independent.step(defend(3,{0,1})); roundtrip(independent); independent.step(defend()); roundtrip(independent);
    check(independent.state().pending.counter_card == 187 && independent.state().last_chance.forced,
        "minimum valid extension dependencies support direct harm and forced fire without other feature flags");
    independent.step(defend()); roundtrip(independent);
}
void dark_cloud_then_fire_order() {
    for (bool cloud_first : {false,true}) {
        auto env = fixture({5}, {192,187}); rolls(env,{76}); gifts(env); env.step(attack());
        const auto start = env.step(cloud_first ? defend(3,{0,1}) : defend(3,{1,0}));
        if (cloud_first) {
            check(start.counter_queued == 2 && env.state().pending.counter_card == 192
                && env.state().pending.kind == Effect::harm && env.snapshot().hit_index == 0,
                "queued fire is not rolled before earlier direct DarkCloud response");
            roundtrip(env); const auto applied = env.step(defend());
            check(applied.counter_harm_applications == 1 && applied.counter_chance_attempts == 1
                && applied.counter_chance_hits == 1 && applied.counter_chance_forced == 1
                && env.state().pending.counter_card == 187 && env.snapshot().hit_index == 0,
                "earlier DarkCloud forces later fire at pop without consuming future roll76");
            roundtrip(env); env.step(defend());
        } else {
            check(start.counter_missed == 1 && start.counter_chance_attempts == 1
                && start.counter_chance_misses == 1 && env.snapshot().hit_index == 1
                && env.state().pending.counter_card == 192, "fire first consumes76 and misses before direct DarkCloud");
            roundtrip(env); env.step(defend());
        }
        check(env.state().players[0].dark_cloud && env.state().players[0].hp == (cloud_first ? 39 : 40)
            && env.state().players[1].hp == 39 && env.state().turns == 1,
            "same selected mask and opposite orders produce distinct HP/RNG outcomes"); roundtrip(env);
    }
    for (int roll : {75,76}) {
        auto env = fixture({36}, {187}); auto snap = env.snapshot(); snap.state.players[1].hp = 1;
        check(env.restore(snap), "restore dead-source fire counter test"); rolls(env,{static_cast<std::uint8_t>(roll)});
        env.step(attack()); const auto result = env.step(defend(1,{0}));
        check(result.counter_chance_attempts == 1 && env.state().players[1].dead
            && env.snapshot().hit_index == 1, "reserved fire rolls even when original damage finalized its source dead");
        if (roll == 75) {
            check(env.state().pending.counter_card == 187 && env.state().pending.amount == 10
                && env.state().last_chance.source == 1 && env.state().last_chance.target == 0,
                "alive-target hit from dead source exposes correct origin event"); roundtrip(env); env.step(defend());
        } else check(result.counter_missed == 1 && env.done(), "dead-source miss advances and ends turn");
        roundtrip(env);
    }
}
void direct_harms_and_glory_order() {
    for (CardId card : {CardId{191},CardId{192}}) {
        auto env = fixture({5,195,186}, {card}); env.step(attack()); env.step(defend(1,{0}));
        check(env.state().pending.kind == Effect::harm && env.state().pending.amount == 0,
            "direct harm counter is not attack damage");
        const auto effect = env.step(defend(3));
        check(effect.counter_harm_applications == 1 && !effect.dark_execution && env.state().players[0].hp == 40
            && (card == 191 ? env.state().players[0].glory : env.state().players[0].dark_cloud),
            "removal plus DEF30 cannot cancel direct harm or turn DarkCloud into dark execution"); roundtrip(env);
        for (bool duplicate : {false,true}) {
            auto heaven = fixture({5}, {card}); auto snapshot = heaven.snapshot();
            snapshot.state.players[0].disease = Disease::heaven;
            if (card == 191) snapshot.state.players[0].glory = duplicate; else snapshot.state.players[0].dark_cloud = duplicate;
            check(heaven.restore(snapshot), "restore Heaven direct-harm recipient"); heaven.step(attack()); heaven.step(defend(1,{0}));
            const auto fall = heaven.step(defend());
            check(fall.harm_heaven_falls == 1 && heaven.state().players[0].dead && heaven.state().winner == 1
                && (card == 191 ? heaven.state().players[0].glory : heaven.state().players[0].dark_cloud) == duplicate,
                "Heaven fall precedes new and duplicate direct counter harm attachment"); roundtrip(heaven);
        }
    }
    for (bool glory_first : {false,true}) {
        auto env = fixture({5,195,120}, {191,190}); env.step(attack());
        env.step(glory_first ? defend(3,{0,1}) : defend(3,{1,0}));
        if (glory_first) {
            env.step(defend()); check(env.state().pending.counter_card == 190 && env.options().defense_limit == 1,
                "earlier Glory restricts later counter response immediately"); reject(env,defend(3)); env.step(defend(1));
        } else {
            check(env.options().defense_limit == 16 && env.legal(defend(3)), "counter before Glory permits two-piece ordinary response");
            env.step(defend(3)); env.step(defend());
        }
        check(env.state().players[0].glory && env.state().players[0].hp == (glory_first ? 38 : 39),
            "mixed queue order changes real intermediate choice and resulting health"); roundtrip(env);
    }
}
void mp_auto_and_full_order() {
    for (bool dies : {false,true}) {
        auto env = fixture({36}, {193}); auto snapshot = env.snapshot(); snapshot.state.players[1].mp = 90;
        snapshot.state.players[1].hp = dies ? 1 : 40; check(env.restore(snapshot), "restore capped/self-dead MP counter");
        env.step(attack()); const auto result = env.step(defend(1,{0}));
        check(result.counter_queued == 1 && result.counter_auto_resolved == (dies ? 0 : 1)
            && result.counter_skipped == (dies ? 1 : 0) && result.counter_mp_gained == (dies ? 0u : 9u)
            && env.state().players[1].mp == (dies ? 90 : 99) && env.state().phase == Phase::main,
            "193 auto-executes selfMP+20 capped99 after damage, or skips finalized dead owner"); roundtrip(env);
    }
    auto prefix = fixture({36}, {193,39}); auto snap = prefix.snapshot(); snap.state.players[1].mp = 90;
    check(prefix.restore(snap), "restore MP prefix before reflection"); prefix.step(attack());
    check(prefix.step(defend(1,{0},1)).reflected && prefix.state().players[1].mp == 90,
        "MP counter prefix does not restore MP when special prevents reservation"); roundtrip(prefix);
    auto sixteen = fixture({5}, {193,193,193,193,193,193,193,193,193,193,193,193,193,193,193,193});
    snap = sixteen.snapshot(); snap.state.players[1].mp = 90; check(sixteen.restore(snap), "restore full16 self-MP counters");
    sixteen.step(attack()); Action all = defend(0xffff); all.counter_count = 16; all.counter_order = 0xfedcba9876543210ULL;
    const auto resolved = sixteen.step(all);
    check(resolved.valid && resolved.counter_queued == 16 && resolved.counter_auto_resolved == 16
        && resolved.counter_mp_gained == 9 && sixteen.state().players[1].mp == 99,
        "sixteen packed nibbles including slot15 validate without shift64 and auto-resolve in order"); roundtrip(sixteen);
}
void hidden_dead_target_rolls_and_reservation() {
    std::array<Observation,2> seen{}; std::array<State,2> states{};
    for (int branch = 0; branch < 2; ++branch) {
        auto env = fixture({100}, {195,190,187}); gifts(env); auto snapshot = env.snapshot(); snapshot.state.players[0].hp = 1;
        check(env.restore(snapshot), "restore target who dies before reserved fire"); rolls(env,{1,static_cast<std::uint8_t>(branch ? 100 : 1)});
        env.step(attack()); const auto public_event = env.state().last_chance;
        env.step(defend(7,{1,2})); const auto end = env.step(defend());
        check(end.counter_hidden_checks == 1 && !end.counter_chance_attempts && !end.counter_chance_hits
            && !end.counter_chance_misses && !end.counter_chance_forced && !end.counter_missed
            && end.counter_skipped == 1 && env.snapshot().hit_index == 2
            && env.state().last_chance == public_event && !end.chance_attempt,
            "dead-target fire consumes a roll but exposes neither its hit nor miss through public events or metrics");
        seen[branch] = env.observe(); states[branch] = env.state(); roundtrip(env);
    }
    check(seen[0] == seen[1] && states[0] == states[1], "hidden dead-target hit versus miss has identical complete public state");
    auto forced_dead = fixture({100}, {195,190,187}); auto corpse = forced_dead.snapshot();
    corpse.state.players[0].hp = 1; corpse.state.players[0].dark_cloud = true;
    check(forced_dead.restore(corpse), "restore future dead target already carrying DarkCloud"); rolls(forced_dead,{1,100});
    forced_dead.step(attack()); const auto last_public = forced_dead.state().last_chance;
    forced_dead.step(defend(7,{1,2})); const auto hidden_forced = forced_dead.step(defend());
    check(hidden_forced.counter_hidden_checks == 1 && !hidden_forced.counter_chance_forced
        && !hidden_forced.counter_chance_attempts && forced_dead.snapshot().hit_index == 1
        && forced_dead.state().last_chance == last_public,
        "dead-target DarkCloud check short-circuits RNG while withholding its forced-hit event"); roundtrip(forced_dead);
    auto no_enemy = fixture({36,187,195}, {190}); auto snapshot = no_enemy.snapshot(); snapshot.state.players[1].hp = 1;
    check(no_enemy.restore(snapshot), "restore dead incoming counter source for alive-enemy reservation"); rolls(no_enemy,{1});
    no_enemy.step(attack()); no_enemy.step(defend(1,{0}));
    const auto no_queue = no_enemy.step(defend(3,{0}));
    check(no_queue.counter_queued == 0 && no_enemy.snapshot().hit_index == 0 && no_enemy.state().winner == 0,
        "187 cannot reserve against a source already finalized dead, unlike direct-target counters"); roundtrip(no_enemy);
}
void snapshots_privacy_and_replay() {
    auto env = fixture({5}, {191,187,193}); rolls(env,{1}); env.step(attack()); env.step(defend(7,{0,1,2}));
    const auto exact = env.snapshot(); const auto obs = env.observe();
    check(obs[515] == 1.f && obs[532] == 1.f && obs[533] == 187/363.f
        && obs[534] == 1/6.f && obs[535] == .75f && obs[536] == 0.f,
        "current direct harm and queued unrolled fire variant features public");
    auto future = exact; future.scripted_hits[0] = 100; future.rng_state ^= 0x9e3779b97f4a7c15ULL;
    Env concealed(config); check(concealed.restore(future) && concealed.observe() == obs && concealed.options() == env.options(),
        "unrolled queue fire outcomes and RNG remain hidden");
    for (int error = 0; error < 5; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.counter_queue[0].hit_rate = 25;
        if (error == 1) bad.state.pending.harm = Harm::dark_cloud;
        if (error == 2) bad.state.counter_queue[1].source = bad.state.counter_queue[1].target = 0;
        if (error == 3) bad.state.counter_queue[1].kind = Effect::damage;
        if (error == 4) bad.state.pending.amount = 1;
        check(!env.restore(bad) && env.snapshot() == exact, "forged fire rate/direct harm/selfMP owner/effect/amount rejected atomically");
    }
    auto private_hand = fixture({5}, {187}); const auto view = private_hand.observe(); auto swap = private_hand.snapshot();
    swap.state.players[1].hand[0] = 193; check(private_hand.restore(swap) && private_hand.observe() == view,
        "opponent unplayed mixed counter kind not exposed by new hand feature");
    auto old_cfg = config; old_cfg.counter_extensions = false; Env old(old_cfg); const auto oldobs = old.observe();
    check(std::all_of(oldobs.begin()+515,oldobs.end(),[](float x){return x==0.f;}), "old profiles extended suffix zero");
    std::uint64_t ordered = 0, attempts = 0, auto_mp = 0, harms = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+75000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "ordered mixed-counter seeded replay");
            for (float x : a.observe()) check(std::isfinite(x), "all mixed-counter observations finite");
            const auto action = a.sample_action(pa), copy = b.sample_action(pb);
            check(action == copy && pa == pb && a.legal(action), "ordered response sampler reproduces only legal complete permutations");
            ordered += action.counter_count;
            const auto ra = a.step(action), rb = b.step(copy);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.counter_queued == rb.counter_queued
                && ra.counter_missed == rb.counter_missed && ra.counter_auto_resolved == rb.counter_auto_resolved
                && ra.counter_chance_hits == rb.counter_chance_hits && ra.counter_hidden_checks == rb.counter_hidden_checks,
                "mixed-counter transition metrics deterministic");
            attempts += ra.counter_chance_attempts; auto_mp += ra.counter_auto_resolved; harms += ra.counter_harm_applications; roundtrip(a);
        }
    }
    check(ordered && attempts && auto_mp && harms, "natural games exercise ordered counters, rolled fire, automatic MP and direct harms");
}
}
int main() {
    metadata_and_order_validation(); dark_cloud_then_fire_order(); direct_harms_and_glory_order();
    mp_auto_and_full_order(); hidden_dead_target_rolls_and_reservation(); snapshots_privacy_and_replay();
    std::cout << "Counter extensions passed (mixed order, direct harms/selfMP, hidden rolls, 1000 replay rollouts).\n";
}
