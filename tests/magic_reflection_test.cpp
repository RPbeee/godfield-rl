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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = true;
    return c;
}();
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL magic reflection: " << message << '\n'; std::exit(1); }
}
Action use() { return {ActionKind::use,0,0,1}; }
Action defend(std::uint16_t mask = 0) { return {ActionKind::defend,no_slot,mask,no_slot}; }
Action reflect(int slot, std::uint16_t mask = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),mask,no_slot}; }
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size());
    p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy);
    check(env.restore(snapshot), "restore explicit magic-reflection fixture"); return env;
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "magic-reflected snapshot exact roundtrip");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "invalid staged reflector action preserves state, gifts, MP and RNG");
}
void gifts(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "script all deferred magic chain gifts"); }
void metadata_and_dual_roles() {
    check(magic_reflection_card_count == 4 && magic_reflection_catalog().size() == 4, "four-card magic reflection scope");
    constexpr std::array<CardId,4> ids{40,171,174,178}; constexpr std::array<int,4> defense{0,9,10,11};
    for (int i = 0; i < 4; ++i) {
        const auto* item = find_magic_reflection_card(ids[i]);
        check(item && magic_reflection_catalog()[i].card.id == ids[i] && item->card.attack == (i ? 0 : 10)
            && item->card.defense == defense[i] && item->card.boost == 0 && item->card.hp == 0 && item->card.mp == 0
            && item->card.price == (i ? 10 : 15) && item->card.weight == 1
            && item->card.discardable == bool(i), "standalone weapon and ordinary armor roles preserved");
        auto env = fixture({198}, {ids[i]}); env.step(use());
        check(env.options().reaction_slots == 1 && !(env.options().defense_slots & 1),
            "matching MAGIC forces staged reflection rather than ordinary armor use"); reject(env,defend(1));
        const auto response = env.step(reflect(0));
        check(response.magic_reflected && response.reflected && !response.spell_first_use && !response.spell_recast
            && env.state().players[1].mp == 10 && env.state().pending.amount == 4
            && env.state().pending.source == 1 && env.state().pending.target == 0
            && env.state().pending.miracle == 198 && env.state().turn_owner == 0,
            "reflection returns unmodified magic without MP or new spell acquisition");
        roundtrip(env); env.step(defend()); check(env.state().players[0].hp == 36, "reflector's ordinary DEF does not reduce returned damage"); roundtrip(env);
        auto ordinary = fixture({36}, {ids[i]}); ordinary.step({ActionKind::attack,0,0,1});
        check(!ordinary.options().reaction_slots, "magic reflector does not reflect a weapon attack");
        if (i) {
            check(ordinary.legal(defend(1)), "armor reflector keeps ordinary DEF against non-MAGIC"); ordinary.step(defend(1));
            check(ordinary.state().players[1].hp == (i == 1 ? 39 : 40), "DEF9/10/11 applies to ordinary weapon damage"); roundtrip(ordinary);
        } else reject(ordinary,defend(1));
    }
    auto weapon = fixture({40}, {120});
    check(weapon.step({ActionKind::attack,0,0,1}).valid && weapon.state().pending.amount == 10,
        "weapon40 remains a normal ATK10 first weapon"); roundtrip(weapon);
    for (CardId id : {CardId{0},CardId{20},CardId{39},CardId{119},CardId{196},CardId{210},CardId{65535}})
        check(!find_magic_reflection_card(id), "weapon reflection, magic bounce and ANY not silently included");
    auto env = fixture({171}, {120});
    check(std::string_view(env.profile()) == magic_reflection_ruleset && env.snapshot().version == 35,
        "explicit new reflection profile and schema");
    for (int missing = 0; missing < 2; ++missing) {
        auto bad = config; if (!missing) { bad.reflection = false; bad.defensive_miracles = false; } else {
            bad.miracles = bad.offensive_miracles = bad.defensive_miracles = false;
        }
        bool threw = false; try { Env invalid(bad); } catch (const std::invalid_argument&) { threw = true; }
        check(threw, "magic reflection requires reflection and miracles explicitly enabled");
    }
    auto old_cfg = config; old_cfg.magic_reflection = false; Env old(old_cfg); const auto before = old.snapshot();
    check(!old.script_draws(ids) && old.snapshot() == before, "old profile draw pool unchanged");
    auto bad = before; bad.state.players[bad.state.actor].hand[0] = 171;
    check(!old.restore(bad) && old.snapshot() == before, "disabled reflector hand rejected atomically");
    bad = env.snapshot(); bad.version = 15; check(!env.restore(bad), "previous common checkpoint schema rejected");
}
void magic_effects_prefixes_and_chance() {
    for (CardId spell : {CardId{216},CardId{220},CardId{200},CardId{207}}) {
        auto env = fixture({spell}, {171}); auto snapshot = env.snapshot();
        snapshot.state.players[0].hp = snapshot.state.players[1].hp = 30;
        check(env.restore(snapshot), "restore reflected utility/Glory/dark magic fixture");
        const std::uint8_t rolls[] = {1,100}; check(env.script_hit_rolls(rolls), "script original chance and preserved next roll");
        env.step(use()); const auto origin = env.snapshot(); const auto result = env.step(reflect(0));
        check(result.magic_reflected && env.state().players[0].mp == origin.state.players[0].mp
            && env.state().players[0].abilities == origin.state.players[0].abilities
            && env.snapshot().hit_index == origin.hit_index && env.state().last_chance == origin.state.last_chance,
            "reflection neither repays/reacquires original miracle nor rerolls original hit"); roundtrip(env);
        env.step(defend());
        if (spell == 216) check(env.state().players[0].hp == 40 && env.state().players[1].hp == 30, "healing returns full10 to original user");
        if (spell == 220) check(env.state().players[0].money == 35 && env.state().players[1].money == 20, "money miracle returns full15");
        if (spell == 200) check(env.state().players[0].glory && env.state().players[0].hp == 29, "light chance magic retains Glory on final target");
        if (spell == 207) check(env.state().players[0].dead && env.state().winner == 1, "dark miracle retains execution after magic reflection");
        roundtrip(env);
    }
    auto forced = fixture({200}, {171}); auto snapshot = forced.snapshot(); snapshot.state.players[1].dark_cloud = true;
    check(forced.restore(snapshot), "restore forced original magic hit"); const std::uint8_t miss[] = {100}; check(forced.script_hit_rolls(miss), "retain unconsumed forced roll");
    forced.step(use()); forced.step(reflect(0));
    check(forced.state().last_chance.forced && forced.snapshot().hit_index == 0 && forced.state().pending.target == 0,
        "original forced chance is not reevaluated on clean returned target"); roundtrip(forced);
    auto prefix = fixture({206}, {122,40}); prefix.step(use());
    const auto reflected = prefix.step(reflect(1,1));
    check(reflected.magic_reflected && prefix.state().pending.amount == 10 && prefix.state().players[1].pending_deals == 2,
        "incompatible ordinary Fire prefix is consumed before full original Fire magic reflection"); roundtrip(prefix);
    prefix.step(defend()); check(prefix.state().players[0].hp == 30, "prefix cannot reduce returned magic power");
    auto staged = fixture({216}, {171,40}); staged.step(use()); reject(staged,reflect(1,1)); reject(staged,defend(1));
    check(staged.legal(reflect(0)), "earlier matching reflector cannot be consumed as an ordinary prefix before another reflector");
    auto glory = fixture({198}, {120,171}); snapshot = glory.snapshot(); snapshot.state.players[1].glory = true;
    check(glory.restore(snapshot), "restore Glory magic reflector"); glory.step(use());
    check(glory.legal(reflect(1)), "Glory permits single magic reflector"); reject(glory,reflect(1,1));
    auto nonreflect = fixture({216}, {171,186}); nonreflect.step(use());
    check(nonreflect.legal(defend(2)), "other ordinary armor may accept magic without using staged reflector");
    nonreflect.step(defend(2)); check(nonreflect.state().players[1].hp == 50 && nonreflect.state().players[1].hand[0] == 171,
        "unselected staged reflector remains while ordinary armor does not reduce heal amount"); roundtrip(nonreflect);
}
void minimum_dependencies_and_full_inventory() {
    Config narrow; narrow.miracles = narrow.reflection = narrow.magic_reflection = true;
    auto small = fixture({171}, {174},narrow); auto snapshot = small.snapshot(); snapshot.state.players[0].abilities[0] = 1;
    snapshot.state.players[0].mp = 99; check(small.restore(snapshot), "restore minimum flags held miracle");
    small.step({ActionKind::cast,0,0,1}); check(small.state().players[0].retained_deals == 1, "minimum magic flag tracks original held gift"); roundtrip(small);
    small.step(reflect(0)); roundtrip(small); small.step(reflect(0)); roundtrip(small); small.step(defend());
    check(small.state().players[0].mp == 93 && small.state().players[1].hp == 50,
        "two magic reflections work without defensive_miracles, elements or other flags"); roundtrip(small);
    auto full = fixture({171}, {174},narrow); snapshot = full.snapshot();
    for (auto& p : snapshot.state.players) { p.hand.fill(171); p.hand_count = 16; }
    snapshot.state.players[0].abilities[0] = 1; snapshot.state.players[0].mp = 99;
    check(full.restore(snapshot), "restore both full inventories with original retained heal"); gifts(full); full.step({ActionKind::cast,0,0,1});
    for (int n = 0; n < 32; ++n) {
        check(full.step(reflect(0)).magic_reflected && full.snapshot().draw_index == 0 && full.state().turns == 0,
            "thirty-two inventory-limited reflections defer all gifts and turn boundary"); roundtrip(full);
    }
    check(full.state().players[0].pending_deals == 17 && full.state().players[0].retained_deals == 1
        && full.state().players[1].pending_deals == 16 && full.state().reaction_count == 32,
        "original full held cast plus consumed16 reflectors legitimately schedules17 gifts");
    full.step(defend());
    check(full.snapshot().draw_index == 33 && full.state().players[0].hand_count == 16 && full.state().players[1].hand_count == 16
        && !full.state().players[0].retained_deals, "all33 draw outcomes consumed including one overflow"); roundtrip(full);
}
void mixed_long_chain_and_provisional_heal() {
    auto env = fixture({171}, {171}); auto snapshot = env.snapshot();
    for (auto& p : snapshot.state.players) { p.mp = 99; p.defensive_abilities[0] = 1; }
    snapshot.state.players[0].abilities[2] = 1; check(env.restore(snapshot), "restore mixed physical/held magic response chain");
    std::array<std::uint8_t,38> seats{}; for (int n = 0; n < 38; ++n) seats[n] = n%2;
    check(env.script_bounce_targets(seats), "script retained bounces to other seat"); gifts(env); env.step({ActionKind::cast,2,0,1});
    env.step(reflect(0)); roundtrip(env); env.step(reflect(0)); roundtrip(env);
    for (int n = 0; n < 38; ++n) { check(env.step({ActionKind::defend_cast,0,0,no_slot}).magic_bounced, "mixed chain pays each retained bounce once"); roundtrip(env); }
    check(env.state().reaction_count == 40 && env.state().players[0].pending_deals == 21
        && env.state().players[1].pending_deals == 20 && env.state().players[0].retained_deals == 20
        && env.state().players[0].mp == 2 && env.state().players[1].mp == 4,
        "two physical reflections plus38 held bounces exceed32 reactions while retaining proven MP gift bound20");
    env.step(defend()); check(env.snapshot().draw_index == 41, "all41 mixed response gifts drawn only after final item"); roundtrip(env);
    auto heal = fixture({216,171}, {175,171}); snapshot = heal.snapshot(); snapshot.state.players[1].disease = Disease::heaven;
    check(heal.restore(snapshot), "restore Heaven prefix before returned healing"); heal.step(use()); heal.step(reflect(1,1));
    check(heal.state().players[1].hp == 0 && !heal.state().players[1].dead, "prefix Heaven fall remains provisional during magic return"); roundtrip(heal);
    heal.step(reflect(0)); roundtrip(heal); heal.step(defend());
    check(heal.state().players[1].hp == 10 && !heal.state().players[1].dead && !heal.done(),
        "second magic reflection restores heal to provisional HP0 target before finalization"); roundtrip(heal);
}
void counter_prefix_types_snapshots_and_replay() {
    auto prefix = fixture({198}, {190,171}); prefix.step(use()); auto action = reflect(1,1); action.counter_count = 1;
    const auto result = prefix.step(action);
    check(result.magic_reflected && !result.counter_queued && prefix.state().players[1].pending_deals == 2,
        "counter prefix before magic reflection consumes without reserving attack"); roundtrip(prefix);
    auto counter = fixture({5,195,171}, {190}); counter.step({ActionKind::attack,0,0,1});
    auto selected = defend(1); selected.counter_count = 1; counter.step(selected);
    reject(counter,reflect(1,1));
    check(counter.legal(defend(3)) && !counter.step(defend(3)).magic_reflected,
        "non-MAGIC counter allows staged armor's ordinary defense after removal but cannot be reflected"); roundtrip(counter);
    auto env = fixture({198}, {171}); env.step(use()); env.step(reflect(0)); const auto exact = env.snapshot();
    check(env.observe()[679] == 1.f, "magic reflection feature suffix enabled");
    for (int error = 0; error < 4; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.last_reaction.from_ability = true;
        if (error == 1) bad.state.last_reaction.reaction = WeaponReaction::bounce;
        if (error == 2) bad.state.players[0].abilities[2] = 0;
        if (error == 3) bad.config.magic_reflection = false;
        check(!env.restore(bad) && env.snapshot() == exact, "forged reflection category/origin ownership/profile rejected atomically");
    }
    auto own = fixture({40,171,174,178}, {120}); const auto features = own.observe();
    check(features[680] == .25f && features[681] == .5f && features[682] == .75f && features[683] == 1.f,
        "own reflector IDs exposed as four stable local kinds");
    auto hidden = fixture({5}, {171}); const auto visible = hidden.observe(); auto swap = hidden.snapshot(); swap.state.players[1].hand[0] = 178;
    swap.rng_state ^= 0x9e3779b97f4a7c15ULL;
    check(hidden.restore(swap) && hidden.observe() == visible, "opponent reflector identity and game RNG private");
    auto old_cfg = config; old_cfg.magic_reflection = false; Env old(old_cfg); const auto oldobs = old.observe();
    check(std::all_of(oldobs.begin()+679,oldobs.end(),[](float x){return x==0.f;}), "old profiles new17 features remain zero");
    std::uint64_t reflected = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+95000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "magic reflection seeded replay");
            for (float x : a.observe()) check(std::isfinite(x), "magic reflection observations finite");
            const auto chosen = a.sample_action(pa), copy = b.sample_action(pb);
            check(chosen == copy && pa == pb && a.legal(chosen), "staged response sampler legal and reproducible");
            const auto ra = a.step(chosen), rb = b.step(copy);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.magic_reflected == rb.magic_reflected
                && ra.reflected == rb.reflected, "magic-reflection events replay"); reflected += ra.magic_reflected; roundtrip(a);
        }
    }
    check(reflected, "natural games actually use new magic reflectors");
}
}
int main() {
    metadata_and_dual_roles(); magic_effects_prefixes_and_chance(); minimum_dependencies_and_full_inventory();
    mixed_long_chain_and_provisional_heal(); counter_prefix_types_snapshots_and_replay();
    std::cout << "Magic reflection passed (dual roles, minimal fullhand17, mixed40 chain, 1000 replay rollouts).\n";
}
