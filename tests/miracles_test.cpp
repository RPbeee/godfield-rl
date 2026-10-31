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
void check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL miracles: " << message << '\n'; std::exit(1); }
}
Action use(int slot, int target = 0) {
    return {ActionKind::use, static_cast<std::uint8_t>(slot), 0, static_cast<std::uint8_t>(target)};
}
Action cast(int ability, int target = 0) {
    return {ActionKind::cast, static_cast<std::uint8_t>(ability), 0, static_cast<std::uint8_t>(target)};
}
Action accept() { return {ActionKind::defend, no_slot, 0, no_slot}; }
void hand(Player& player, std::initializer_list<CardId> cards) {
    player.hand.fill(0);
    std::copy(cards.begin(), cards.end(), player.hand.begin());
    player.hand_count = static_cast<std::uint8_t>(cards.size());
    player.pending_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy,
            Config config = Config{512, 10, true}) {
    Env env(config);
    auto snapshot = env.snapshot();
    snapshot.state = State{};
    snapshot.scripted_draws.fill(0);
    snapshot.draw_count = snapshot.draw_index = 0;
    hand(snapshot.state.players[0], own);
    hand(snapshot.state.players[1], enemy);
    check(env.restore(snapshot), "restore valid miracle fixture");
    return env;
}
void script(Env& env, std::initializer_list<CardId> cards) {
    check(env.script_draws({cards.begin(), cards.size()}), "script supported outcome prefix");
}
void reject(Env& env, Action action, const char* message) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid, message);
    check(env.snapshot() == before, "rejected action preserves resources, abilities, draws and RNG");
}
void metadata_and_profiles() {
    check(miracle_count == 2 && miracles().size() == 2, "two supported stable ability slots");
    check(miracle_index(216) == 0 && miracle_index(220) == 1 && miracle_index(197) == -1,
        "legacy miracle IDs mapped explicitly");
    check(!find_miracle(0) && !find_miracle(197) && !find_miracle(65535), "unsupported miracles have no fallback");
    check(find_miracle(216)->effect == MiracleEffect::heal && find_miracle(216)->amount == 10
        && find_miracle(216)->cost == 6 && find_miracle(216)->weight == 1, "pinned healing miracle metadata");
    check(find_miracle(220)->effect == MiracleEffect::money && find_miracle(220)->amount == 15
        && find_miracle(220)->cost == 8 && find_miracle(220)->weight == 1, "pinned money miracle metadata");
    Env plain;
    check(std::string_view(plain.profile()) == ruleset, "default profile unchanged");
    auto enabled = fixture({216}, {120});
    check(std::string_view(enabled.profile()) == miracles_ruleset, "opt-in profile has distinct identity");
    const auto original = plain.snapshot();
    const CardId miracle_prefix[] = {216};
    check(!plain.script_draws(miracle_prefix) && plain.snapshot() == original,
        "default profile rejects miracle scripted acquisition");
    auto bad = original; bad.state.players[bad.state.actor].hand[0] = 216;
    check(!plain.restore(bad) && plain.snapshot() == original, "default profile rejects miracle hand");
    bad = original; bad.state.players[0].abilities[0] = 1;
    check(!plain.restore(bad) && plain.snapshot() == original, "default profile rejects retained miracle");
    bad = original;
    const auto source = bad.state.turn_owner;
    bad.state.actor = static_cast<std::uint8_t>(1 - source);
    bad.state.phase = Phase::response;
    bad.state.players[source].pending_deals = 1;
    bad.state.pending = Pending{Effect::money, source, bad.state.actor, 15};
    check(!plain.restore(bad) && plain.snapshot() == original, "default profile rejects a new money effect without changing config");
    check(!plain.restore(enabled.snapshot()) && plain.snapshot() == original, "profile mismatch restore is atomic");
    bad = enabled.snapshot(); bad.version = 1;
    const auto before = enabled.snapshot();
    check(!enabled.restore(bad) && enabled.snapshot() == before, "old snapshot version not silently migrated");

    // Golden reset observed through the pre-miracle local gl ABI v1 binary.
    // This regression checks the default curriculum, not official gameplay.
    plain.reset(42);
    constexpr std::array<CardId, 10> expected{173, 120, 126, 228, 22, 120, 126, 169, 33, 45};
    check(plain.state().actor == 0 && plain.state().players[0].hand_count == expected.size()
        && std::equal(expected.begin(), expected.end(), plain.state().players[0].hand.begin()),
        "default seed42 initial trajectory preserved");
    check(plain.options().weapon_bases == 784 && plain.options().utility_slots == 8
        && plain.options().discard_slots == 239 && plain.options().ability_slots == 0,
        "default seed42 option masks preserved");
    for (const auto& player : plain.state().players) check(player.abilities == std::array<std::uint32_t, main_ability_capacity>{},
        "default reset has no retained abilities");
}
void first_use_and_exact_cost() {
    for (int index = 0; index < miracle_count; ++index) {
        const auto& miracle = miracles()[index];
        auto env = fixture({miracle.id, 120}, {120});
        auto state = env.snapshot();
        state.state.players[0].mp = miracle.cost;
        check(env.restore(state), "restore exactly affordable miracle");
        script(env, {5});
        check(env.options().utility_slots & 1, "exactly affordable hand miracle in utility options");
        const auto result = env.step(use(0));
        check(result.valid && !result.terminated && !result.truncated, "first hand miracle use accepted");
        const auto& p = env.state().players[0];
        check(p.mp == 0 && p.abilities[index] == 1, "first use pays once and retains one copy");
        check(p.hand_count == 2 && p.hand[0] == 120 && p.hand[1] == 5,
            "first use consumes hand copy, compacts and earns one replacement");
        check(env.snapshot().draw_index == 1, "first use consumes one draw outcome");
        if (index == 0) check(p.hp == 50 && p.money == 20, "healing effect changes only HP");
        else check(p.hp == 40 && p.money == 35, "money effect changes only money");
        check(env.state().actor == 1 && env.state().phase == Phase::main, "self miracle resolves before next scheduled turn");

        auto poor = fixture({miracle.id}, {120});
        state = poor.snapshot(); state.state.players[0].mp = miracle.cost - 1;
        check(poor.restore(state), "restore unaffordable hand miracle");
        check(!(poor.options().utility_slots & 1), "unaffordable hand miracle omitted");
        reject(poor, use(0), "MP shortage rejects first use atomically");
    }
}
void retained_cast_and_duplicates() {
    auto env = fixture({216, 216, 120}, {120});
    auto snapshot = env.snapshot(); snapshot.state.players[0].mp = 99;
    check(env.restore(snapshot), "restore duplicate miracle copies");
    script(env, {5});
    check(env.step(use(0)).valid && env.state().players[0].abilities[0] == 1,
        "first duplicate retained independently");
    snapshot = env.snapshot();
    snapshot.state.order = {0, 1}; snapshot.state.order_index = 0;
    snapshot.state.actor = snapshot.state.turn_owner = 0;
    check(env.restore(snapshot), "fixture schedules owner for second initial use");
    script(env, {5});
    check(env.step(use(0)).valid && env.state().players[0].abilities[0] == 2,
        "second same-ID hand copy increments retained count");

    for (int repetition = 0; repetition < 3; ++repetition) {
        snapshot = env.snapshot();
        snapshot.state.order = {0, 1}; snapshot.state.order_index = 0;
        snapshot.state.actor = snapshot.state.turn_owner = 0;
        check(env.restore(snapshot), "fixture schedules repeated retained cast");
        const auto before = env.state().players[0];
        check(env.options().ability_slots & 1, "retained affordable ability exposed");
        script(env, {120});
        check(env.step(cast(0)).valid, "retained cast accepted");
        const auto& after = env.state().players[0];
        check(after.abilities[0] == 2 && after.mp == before.mp - 6, "cast retains duplicate copies and pays MP");
        check(after.hand_count == before.hand_count + 1
            && std::equal(before.hand.begin(), before.hand.begin() + before.hand_count, after.hand.begin()),
            "cast does not consume hand; earns one appended artifact");
        check(env.snapshot().draw_index == 1, "retained cast earns exactly one draw");
    }

    auto poor = fixture({120}, {120});
    snapshot = poor.snapshot(); snapshot.state.players[0].abilities = {1, 1};
    snapshot.state.players[0].mp = 5;
    check(poor.restore(snapshot), "restore known but unaffordable abilities");
    check(poor.options().ability_slots == 0, "ability options respect current MP");
    reject(poor, cast(0), "retained healing cast rejects insufficient MP");
    reject(poor, cast(1), "retained money cast rejects insufficient MP");
    snapshot.state.players[0].mp = 8;
    check(poor.restore(snapshot), "restore exact money cast budget");
    check(poor.options().ability_slots == 3, "both retained abilities affordable with MP8");
    check(poor.step(cast(1)).valid && poor.state().players[0].mp == 0
        && poor.state().players[0].money == 35 && poor.state().players[0].abilities[1] == 1,
        "money retained cast pays exact MP and preserves ability");
}
void reactions_and_caps() {
    for (int index = 0; index < miracle_count; ++index) {
        const auto& miracle = miracles()[index];
        auto env = fixture({miracle.id}, {120});
        auto snapshot = env.snapshot();
        snapshot.state.players[1].hp = 98;
        snapshot.state.players[1].money = 98;
        check(env.restore(snapshot), "restore receiver near cap");
        script(env, {5});
        check(env.step(use(0, 1)).valid && env.state().phase == Phase::response,
            "other-target first use waits for acceptance");
        check(env.state().pending.miracle == miracle.id && !env.state().pending.from_ability,
            "pending first-use provenance retained");
        check(env.state().players[0].abilities[index] == 1 && env.state().players[0].mp == 10 - miracle.cost,
            "first use learns and pays before response");
        check(env.state().players[0].pending_deals == 1 && env.snapshot().draw_index == 0,
            "replacement deferred until response");
        check(env.options().defense_slots == 0, "armor unavailable against utility miracle");
        reject(env, {ActionKind::defend, no_slot, 1, no_slot}, "armor cannot cancel utility miracle");
        Env resumed(Config{512, 10, true});
        check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "pending initial-use state restorable");
        check(env.step(accept()).valid, "utility response accepted");
        check(env.state().players[0].hp == 40 && env.state().players[0].money == 20,
            "other-target effect does not change caster resources beyond MP");
        check(index == 0 ? env.state().players[1].hp == 99 : env.state().players[1].money == 99,
            "utility applies to selected receiver with cap99");
    }

    auto money = fixture({220}, {120});
    auto heal = fixture({216}, {120});
    money.step(use(0, 1)); heal.step(use(0, 1));
    auto money_obs = money.observe();
    check(money_obs[128] == 1.f && heal.observe()[128] == 0.f, "money pending effect gets dedicated extension flag");
    check(money_obs[16] == 120 / 363.f, "money flag does not overwrite first hand-card feature");
}
void full_hand_retained_cast() {
    auto env = fixture({}, {120});
    auto snapshot = env.snapshot();
    auto& caster = snapshot.state.players[0];
    caster.hand.fill(120); caster.hand_count = 16;
    caster.abilities[0] = 1;
    check(env.restore(snapshot), "restore retained ability with full hand");
    script(env, {216});
    check(env.step(cast(0, 1)).valid, "cast from full hand to other seat");
    check(env.state().players[0].hand_count == 16 && env.state().players[0].pending_deals == 1
        && env.state().pending.from_ability, "cast acquisition does not require a consumed hand slot");
    Env resumed(Config{512, 10, true});
    check(resumed.restore(env.snapshot()), "response with hand16 plus one future draw is valid");
    check(env.step(accept()).valid && resumed.step(accept()).valid, "both original and resumed cast resolve");
    check(env.snapshot() == resumed.snapshot(), "full-hand retained-cast continuation replay");
    check(env.snapshot().draw_index == 1 && env.state().players[0].hand_count == 16
        && env.state().players[0].abilities[0] == 1,
        "overflow draw consumed then discarded without automatically learning it");
}
void malformed_actions_and_snapshots() {
    auto env = fixture({216, 220}, {120});
    reject(env, cast(0), "unowned retained ability rejected");
    reject(env, cast(2), "unknown stable ability index rejected");
    reject(env, cast(255), "ability sentinel is not a slot");
    reject(env, cast(0, 2), "invalid ability target rejected");
    auto snapshot = env.snapshot(); snapshot.state.players[0].abilities = {1, 1};
    check(env.restore(snapshot), "restore both known abilities");
    reject(env, cast(0, 2), "owned cast still rejects nonexistent target");
    reject(env, cast(2), "owned cast still rejects unknown ability index");
    auto malformed = cast(0); malformed.slots = 1;
    reject(env, malformed, "cast cannot carry hand-selection extras");
    reject(env, {ActionKind::attack, 0, 0, 1}, "hand miracle is not an ordinary weapon");

    const auto original = env.snapshot();
    check(original.version == 37, "shared snapshot schema includes counter extensions");
    auto invalid = original; invalid.state.players[0].abilities[0] = 513;
    check(!env.restore(invalid) && env.snapshot() == original, "retained count above profile budget rejected");
    invalid = original; invalid.state.players[0].abilities = {512, 1};
    check(!env.restore(invalid) && env.snapshot() == original, "total retained counts bounded without overflow");
    invalid = original; invalid.state.players[0].abilities = {512, 0};
    check(env.restore(invalid), "restore retained count at exact supported limit");
    check(!(env.options().utility_slots & 1) && !(env.options().utility_slots & 2),
        "first learning excluded once total ability capacity reached");
    reject(env, use(0), "learning healing copy beyond capacity rejected atomically");
    reject(env, use(1), "learning another type beyond total capacity rejected atomically");
    check(env.legal(cast(0)), "capacity limit does not block already-known cast");

    auto huge = fixture({216}, {120}, Config{std::numeric_limits<std::uint32_t>::max(), 10, true});
    auto huge_state = huge.snapshot();
    huge_state.state.players[0].abilities[0] = std::numeric_limits<std::uint32_t>::max();
    check(huge.restore(huge_state), "largest supported retained count represented");
    reject(huge, use(0), "uint32 retained-count wraparound prevented");
    const auto huge_before = huge.snapshot();
    huge_state.state.players[0].abilities[1] = 1;
    check(!huge.restore(huge_state) && huge.snapshot() == huge_before,
        "uint32 total overflow cannot bypass count validation");

    auto response = fixture({216}, {120});
    response.step(use(0, 1));
    const auto pending = response.snapshot();
    invalid = pending; invalid.state.pending.miracle = 220;
    check(!response.restore(invalid) && response.snapshot() == pending, "miracle ID/effect mismatch rejected");
    invalid = pending; invalid.state.pending.from_ability = true;
    // A first-use pending can be observationally equivalent to an ability cast,
    // so do not require rejecting this origin change without historical evidence.
    invalid.state.players[0].pending_deals = 2;
    check(!response.restore(invalid) && response.snapshot() == pending, "retained cast must schedule one acquisition");
}
void observation_privacy() {
    auto env = fixture({216, 220, 120}, {120});
    const auto before = env.observe();
    check(before[131] == 1.f && before[132] == 1.f && before[133] == 1.f && before[134] == 0.f,
        "profile and hand miracle tags represented");
    check(before[148] == 6 / 99.f && before[149] == 8 / 99.f && before[150] == 0.f,
        "own hand costs represented without changing legacy prefix");
    auto own = env.snapshot(); own.state.players[0].abilities = {1, 2};
    check(env.restore(own), "restore own retained counts");
    const auto known = env.observe();
    check(known != before && known[129] == 1 / 512.f && known[130] == 2 / 512.f,
        "own retained counts exposed in documented normalized features");
    auto hidden = env.snapshot(); hidden.state.players[1].abilities = {10, 20};
    Env other(Config{512, 10, true}); check(other.restore(hidden), "restore different opponent knowledge");
    check(other.observe() == known && other.options() == env.options(),
        "opponent retained abilities do not leak through observations/options");
    for (int index = 0; index < miracle_count; ++index)
        check(other.legal(cast(index)) == env.legal(cast(index)), "opponent abilities do not affect own cast legality");
}
void seeded_rollouts() {
    std::uint64_t learned = 0, casted = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(Config{256, 10, true}), b(Config{256, 10, true});
        a.reset(seed); b.reset(seed);
        std::uint64_t pa = seed + 1000, pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "opt-in seeded replay");
            for (float value : a.observe()) check(std::isfinite(value), "finite miracle observation");
            const auto action = a.sample_action(pa), action_b = b.sample_action(pb);
            check(action == action_b && pa == pb && a.legal(action), "opt-in hierarchical policy legal and reproducible");
            if (action.kind == ActionKind::use && find_miracle(a.state().players[a.state().actor].hand[action.base])) ++learned;
            if (action.kind == ActionKind::cast) ++casted;
            const auto ra = a.step(action), rb = b.step(action_b);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards, "opt-in transitions replay");
            check(!(ra.terminated && ra.truncated), "termination precedence maintained");
            Env resumed(Config{256, 10, true});
            check(resumed.restore(a.snapshot()) && resumed.snapshot() == a.snapshot(), "all generated miracle states restorable");
        }
    }
    check(learned > 0 && casted > 0, "seeded opt-in runs exercise initial and retained casts");
}
}
int main() {
    metadata_and_profiles(); first_use_and_exact_cost(); retained_cast_and_duplicates();
    reactions_and_caps(); full_hand_retained_cast(); malformed_actions_and_snapshots();
    observation_privacy(); seeded_rollouts();
    std::cout << "Miracle checks passed (first-use/retained casts, profile boundaries, privacy, 1000 replay rollouts).\n";
}
