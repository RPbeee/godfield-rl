#include "godfield/legacy_env.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <stdexcept>

namespace {
using namespace godfield::legacy;
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL legacy: " << message << '\n'; std::exit(1); }
}
constexpr std::uint16_t bit(int slot) { return static_cast<std::uint16_t>(1u << slot); }
Action attack(int base, std::uint16_t extras = 0, int target = 1) {
    return {ActionKind::attack, static_cast<std::uint8_t>(base), extras, static_cast<std::uint8_t>(target)};
}
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend, no_slot, slots, no_slot}; }
Action use(int base, int target = 0) {
    return {ActionKind::use, static_cast<std::uint8_t>(base), 0, static_cast<std::uint8_t>(target)};
}
Action discard(std::uint16_t slots) { return {ActionKind::discard, no_slot, slots, no_slot}; }
void hand(Player& player, std::initializer_list<CardId> ids) {
    player.hand.fill(0);
    std::copy(ids.begin(), ids.end(), player.hand.begin());
    player.hand_count = static_cast<std::uint8_t>(ids.size());
    player.pending_deals = 0;
}
Env fixture(std::initializer_list<CardId> first, std::initializer_list<CardId> second,
            std::uint64_t seed = 1) {
    Env env;
    auto snapshot = env.snapshot();
    snapshot.state = State{};
    snapshot.rng_state = seed;
    snapshot.scripted_draws.fill(0);
    snapshot.draw_count = snapshot.draw_index = 0;
    hand(snapshot.state.players[0], first);
    hand(snapshot.state.players[1], second);
    check(env.restore(snapshot), "restore valid main fixture");
    return env;
}
void script(Env& env, std::initializer_list<CardId> ids) {
    check(env.script_draws({ids.begin(), ids.size()}), "install valid scripted draws");
}
void reject(Env& env, Action action, const char* message) {
    const auto before = env.snapshot();
    check(!env.legal(action), message);
    check(!env.step(action).valid, message);
    check(env.snapshot() == before, "invalid action preserves state, draws and RNG");
}
void catalog_and_initialization() {
    check(catalog().size() == 60, "pinned 60-card allowlist");
    int weight = 0;
    for (const auto& card : catalog()) {
        check(find_card(card.id) == &card, "catalog ID lookup");
        check(card.weight > 0, "supported pool has positive weights");
        weight += card.weight;
        check(card.discardable != (card.kind == Kind::weapon), "weapon discard prohibition");
    }
    check(weight == 225, "pinned subset weight total");
    for (CardId id : {CardId{0}, CardId{20}, CardId{58}, CardId{197}, CardId{65535}})
        check(!find_card(id), "excluded IDs have no fallback");
    check(find_card(9)->attack == 1 && find_card(9)->boost == 1, "boost weapon also has base attack");
    check(find_card(11)->attack == 2 && find_card(11)->defense == 6, "dual-role weapon");
    check(find_card(133)->boost == 5 && find_card(133)->defense == 3, "armor can boost attack");
    check(find_card(118)->defense == 0 && find_card(118)->kind == Kind::armor, "zero-defense armor exists");
    Env env;
    env.reset(42);
    check(env.state().players[0].hp == 40 && env.state().players[0].mp == 10
        && env.state().players[0].money == 20, "legacy initial resources");
    for (const auto& p : env.state().players) {
        check(p.hand_count == 10, "initial hand separated from capacity");
        for (int i = 0; i < p.hand_count; ++i) check(find_card(p.hand[i]), "draws only supported cards");
        for (int i = p.hand_count; i < hand_capacity; ++i) check(p.hand[i] == 0, "unused hand tail cleared");
    }
    check(env.state().order[0] != env.state().order[1], "initial order permutation");
    check(env.state().actor == env.state().order[0], "initial actor follows order");
    bool threw = false;
    try { Env invalid(Config{0, 10}); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "zero decision budget rejected");
    threw = false;
    try { Env invalid(Config{512, 17}); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "initial hand beyond capacity rejected");
}
void legal_combinations() {
    auto env = fixture({5, 9, 10, 11, 118, 133, 227, 231}, {120});
    const auto options = env.options();
    check((options.weapon_bases & bit(1)) && (options.boost_slots & bit(1)), "boost weapon has both attack roles");
    check((options.boost_slots & bit(5)) && !(options.weapon_bases & bit(5)), "armor boosts but cannot be base");
    check(!options.pray && !(options.discard_slots & bit(0)), "weapon prevents pray and discard");
    check(env.legal(attack(0, bit(1) | bit(5))), "base plus weapon and armor boosts");
    check(env.legal(attack(1)), "boost weapon can attack alone");
    reject(env, attack(0, bit(2)), "two ordinary base weapons cannot combine");
    reject(env, attack(0, bit(0)), "base cannot consume itself again as extra");
    reject(env, attack(4), "armor is not a base weapon");
    reject(env, attack(8), "empty slot rejected");
    reject(env, attack(16), "slot beyond capacity rejected");
    reject(env, attack(0, 0, 2), "nonexistent target rejected");
    reject(env, use(6, 2), "utility target out of bounds rejected");
    reject(env, {ActionKind::pray}, "pray while holding any weapon rejected");
    reject(env, discard(bit(3)), "dual-role weapon cannot be discarded");
    reject(env, discard(0), "empty discard rejected");
    reject(env, defend(), "defense outside response rejected");
    reject(env, {static_cast<ActionKind>(255)}, "unknown action kind rejected");
    auto malformed = use(6);
    malformed.slots = bit(7);
    reject(env, malformed, "unused action fields must be canonical");
}
void consumption_and_defense() {
    // Source-derived cases R02/R04/R09: queue attack, consume every selected
    // armor including overdefense, then append scripted draws in player order.
    auto env = fixture({5, 9, 10}, {186, 120});
    script(env, {228, 231, 120, 132});
    const auto began = env.step(attack(0, bit(1)));
    check(began.valid && !began.terminated, "begin combined attack");
    const auto& s = env.state();
    check(s.phase == Phase::response && s.actor == 1 && s.turn_owner == 0, "reaction actor separate from turn owner");
    check(s.pending.kind == Effect::damage && s.pending.amount == 2, "base attack plus boost");
    check(s.players[0].hand_count == 1 && s.players[0].hand[0] == 10
        && s.players[0].pending_deals == 2, "attack consumption compacts and defers two draws");
    check(env.snapshot().draw_index == 0, "no acquisition before response");
    const auto response = env.step(defend(bit(0) | bit(1)));
    check(response.valid && env.state().players[1].hp == 40, "overdefense blocks damage");
    check(env.state().players[0].hand_count == 3 && env.state().players[0].hand[0] == 10
        && env.state().players[0].hand[1] == 228 && env.state().players[0].hand[2] == 231,
        "remaining attacker hand preserved, draws appended");
    check(env.state().players[1].hand_count == 2 && env.state().players[1].hand[0] == 120
        && env.state().players[1].hand[1] == 132, "all selected armor consumed even after full block");
    check(env.snapshot().draw_index == 4, "all scheduled draws consumed");
    check(env.state().actor == 1 && env.state().turn_owner == 1 && env.state().phase == Phase::main,
        "return to scheduled normal actor");

    auto zero = fixture({5}, {118});
    script(zero, {5, 120});
    zero.step(attack(0));
    check(zero.options().defense_slots == bit(0), "zero-defense armor remains selectable");
    check(zero.step(defend(bit(0))).valid && zero.state().players[1].hp == 39,
        "zero-defense armor consumed without reducing damage");
    check(zero.state().players[1].hand[0] == 120, "zero-defense armor receives replacement");

    auto dual = fixture({36}, {11});
    dual.step(attack(0));
    check(dual.options().defense_slots == bit(0), "dual-role weapon may defend");
    check(dual.step(defend(bit(0))).valid && dual.state().players[1].hp == 36,
        "dual-role weapon uses defense value instead of attack value");

    auto duplicates = fixture({9, 9}, {120});
    check(duplicates.step(attack(0, bit(1))).valid && duplicates.state().pending.amount == 2,
        "same CardId in distinct slots is two owned instances");
}
void prayer_discard_and_capacity() {
    auto env = fixture({120, 227, 132, 231}, {120});
    script(env, {5});
    check(env.step(discard(bit(1) | bit(3))).valid, "multiple nonweapons discard");
    const auto& p = env.state().players[0];
    check(p.hand_count == 2 && p.hand[0] == 120 && p.hand[1] == 132, "discard compacts without refill");
    check(env.snapshot().draw_index == 0 && p.pending_deals == 0, "discard grants no draws");

    auto praying = fixture({120}, {120});
    script(praying, {5});
    check(praying.options().pray && praying.step({ActionKind::pray}).valid, "pray without weapon legal");
    check(praying.state().players[0].hand_count == 2 && praying.state().players[0].hand[1] == 5,
        "pray adds one artifact rather than fills capacity");

    auto full = fixture({}, {120});
    auto snapshot = full.snapshot();
    auto& first = snapshot.state.players[0];
    first.hand.fill(120); first.hand_count = 16;
    check(full.restore(snapshot), "restore full hand");
    script(full, {5, 6});
    check(full.step({ActionKind::pray}).valid, "full hand without weapon may pray");
    check(full.state().players[0].hand_count == 16 && full.snapshot().draw_index == 1,
        "overflow draw still sampled then dropped");
    check(std::all_of(full.state().players[0].hand.begin(), full.state().players[0].hand.end(),
        [](CardId id) { return id == 120; }), "overflow does not overwrite hand");

    auto high = fixture({}, {120});
    snapshot = high.snapshot();
    snapshot.state.players[0].hand.fill(120);
    snapshot.state.players[0].hand_count = 16;
    snapshot.state.players[0].hand[15] = 9;
    check(high.restore(snapshot), "restore highest slot fixture");
    check(high.legal(attack(15)) && (high.options().weapon_bases & bit(15)), "slot fifteen attack addressable");
    reject(high, discard(bit(15)), "highest weapon slot still discard-prohibited");
    check(high.step(discard(0x7fff)).valid && high.state().players[0].hand_count == 1
        && high.state().players[0].hand[0] == 9, "wide mask discard retains final unselected slot");
}
void full_width_masks() {
    auto env = fixture({5}, {});
    auto snapshot = env.snapshot();
    snapshot.state.players[1].hand.fill(120);
    snapshot.state.players[1].hand_count = 16;
    check(env.restore(snapshot), "restore sixteen armor fixture");
    std::array<CardId, 17> draws; draws.fill(120);
    check(env.script_draws(draws), "script seventeen acquisition outcomes");
    env.step(attack(0));
    check(env.options().defense_slots == 0xffff && env.legal(defend(0xffff)),
        "all sixteen defense slots addressable");
    check(env.step(defend(0xffff)).valid && env.snapshot().draw_index == 17,
        "full-width defense consumes and replaces every selected artifact");
    check(env.state().players[1].hand_count == 16 && env.state().players[1].hp == 40,
        "sixteen-armor block respects hand capacity");

    auto boosts = fixture({}, {120});
    snapshot = boosts.snapshot();
    snapshot.state.players[0].hand.fill(9);
    snapshot.state.players[0].hand_count = 16;
    check(boosts.restore(snapshot), "restore sixteen boost weapons");
    check(boosts.step(attack(15, 0x7fff)).valid && boosts.state().pending.amount == 16,
        "highest-slot base plus fifteen distinct boost instances");
    check(boosts.state().players[0].pending_deals == 16 && boosts.state().players[0].hand_count == 0,
        "sixteen-card attack preserves full acquisition budget");
    Env resumed;
    check(resumed.restore(boosts.snapshot()), "maximum-width attack snapshot restorable");
}
void utility_and_terminal() {
    auto self = fixture({227, 231}, {120});
    auto snapshot = self.snapshot();
    snapshot.state.players[0].hp = 98;
    snapshot.state.players[0].mp = 98;
    check(self.restore(snapshot), "restore near-cap fixture");
    check(self.step(use(0)).valid && self.state().players[0].hp == 99,
        "self HP recovery capped and immediate");
    check(self.state().phase == Phase::main && self.state().actor == 1, "self use ends scheduled turn");

    auto mana = fixture({231}, {120});
    snapshot = mana.snapshot(); snapshot.state.players[0].mp = 98;
    check(mana.restore(snapshot), "restore mana cap fixture");
    check(mana.step(use(0)).valid && mana.state().players[0].mp == 99, "mana capped at 99");

    auto other = fixture({227}, {120});
    check(other.step(use(0, 1)).valid && other.state().phase == Phase::response,
        "other-player recovery waits for response");
    reject(other, defend(bit(0)), "ordinary armor cannot defend recovery");
    check(other.step(defend()).valid && other.state().players[1].hp == 45,
        "empty response accepts recovery");

    auto self_hit = fixture({5}, {120});
    check(self_hit.step(attack(0, 0, 0)).valid && self_hit.state().players[0].hp == 39
        && self_hit.state().phase == Phase::main, "self attack resolves without defense decision");

    auto lethal = fixture({56}, {120});
    snapshot = lethal.snapshot(); snapshot.state.players[1].hp = 1;
    check(lethal.restore(snapshot), "restore lethal fixture");
    script(lethal, {5});
    lethal.step(attack(0));
    const auto won = lethal.step(defend());
    check(won.valid && won.terminated && !won.truncated && lethal.state().winner == 0,
        "lethal resolves winner");
    check(won.rewards[0] == 1.f && won.rewards[1] == -1.f, "fixed-seat terminal rewards");
    check(lethal.state().players[0].hand_count == 1 && lethal.state().players[0].hand[0] == 5,
        "survivor scheduled draw precedes final result");
    reject(lethal, {ActionKind::pray}, "terminal state rejects further actions");

    Env limited(Config{1, 10});
    snapshot = limited.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0], {5}); hand(snapshot.state.players[1], {120});
    check(limited.restore(snapshot), "restore short horizon fixture");
    const auto cut = limited.step(attack(0));
    check(cut.truncated && !cut.terminated && limited.state().phase == Phase::response,
        "time limit can stop while awaiting reaction");
    check(cut.rewards[0] == 0.f && cut.rewards[1] == 0.f, "truncation does not assign winner");
}
void restore_and_privacy() {
    auto env = fixture({5, 9, 227}, {120, 231});
    script(env, {5, 120});
    const auto original = env.snapshot();
    auto invalid = original;
    invalid.format_tag ^= 1;
    check(!env.restore(invalid) && env.snapshot() == original, "foreign snapshot format rejected atomically");
    invalid = original; ++invalid.version;
    check(!env.restore(invalid) && env.snapshot() == original, "unknown snapshot version rejected atomically");
    invalid = original;
    invalid.state.players[0].hand[0] = 58;
    check(!env.restore(invalid) && env.snapshot() == original, "unsupported ID restore atomic");
    invalid = original; invalid.state.players[0].hand[15] = 5;
    check(!env.restore(invalid) && env.snapshot() == original, "noncanonical hand tail rejected");
    invalid = original; invalid.state.order = {0, 0};
    check(!env.restore(invalid) && env.snapshot() == original, "duplicate order rejected");
    invalid = original; invalid.state.actor = 1;
    check(!env.restore(invalid) && env.snapshot() == original, "main actor must follow schedule");
    invalid = original; invalid.draw_index = invalid.draw_count + 1;
    check(!env.restore(invalid) && env.snapshot() == original, "script index bounds checked");
    invalid = original; invalid.config.initial_hand = 17;
    check(!env.restore(invalid) && env.snapshot() == original, "invalid snapshot config rejected");
    invalid = original; invalid.state.players[0].pending_deals = 16;
    check(!env.restore(invalid) && env.snapshot() == original, "main fixture cannot carry stale acquisition budget");
    const CardId unsupported[] = {58};
    check(!env.script_draws(unsupported) && env.snapshot() == original, "unsupported script rejected atomically");
    std::array<CardId, 65> oversized_script; oversized_script.fill(5);
    check(!env.script_draws(oversized_script) && env.snapshot() == original, "oversized draw prefix rejected atomically");

    auto hidden = original;
    hand(hidden.state.players[1], {56, 56});
    hidden.rng_state ^= 0x9e3779b97f4a7c15ULL;
    hidden.scripted_draws[0] = 56;
    Env other; check(other.restore(hidden), "restore different hidden state");
    check(other.observe() == env.observe() && other.options() == env.options(),
        "opponent hand identity and future draws hidden");

    auto recovery = fixture({227}, {120});
    auto damage = fixture({18}, {120});
    recovery.step(use(0, 1)); damage.step(attack(0));
    check(recovery.state().pending.amount == damage.state().pending.amount,
        "equal-valued pending effects fixture");
    check(recovery.observe() != damage.observe(), "recovery and damage distinguished in observation");

    auto response = env;
    response.step(attack(0));
    const auto pending = response.snapshot();
    auto bad_pending = pending;
    bad_pending.state.pending.source = 1;
    check(!response.restore(bad_pending) && response.snapshot() == pending, "pending source must be turn owner");
    bad_pending = pending; bad_pending.state.pending.amount = -1;
    check(!response.restore(bad_pending) && response.snapshot() == pending, "negative pending amount rejected");
    bad_pending = pending; bad_pending.state.players[0].pending_deals = 16;
    check(!response.restore(bad_pending) && response.snapshot() == pending, "response cannot exceed original inventory budget");
    bad_pending = pending; bad_pending.state.players[1].pending_deals = 1;
    check(!response.restore(bad_pending) && response.snapshot() == pending, "unacted response target has no acquisition budget");
}
void scheduling_and_rollouts() {
    bool repeated_actor = false;
    for (std::uint64_t seed = 0; seed < 64; ++seed) {
        auto env = fixture({120}, {120}, seed);
        script(env, {120, 120});
        check(env.step({ActionKind::pray}).valid && env.state().actor == 1, "first scheduled turn goes to next seat");
        check(env.step({ActionKind::pray}).valid, "second scheduled prayer");
        repeated_actor |= env.state().actor == 1;
        check(env.state().order_index == 0 && env.state().turns == 2,
            "new two-player cycle begins after two turns");
    }
    check(repeated_actor, "cycle shuffle permits same seat in consecutive normal turns");

    std::uint64_t completed = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a, b;
        a.reset(seed); b.reset(seed);
        std::uint64_t policy_a = seed + 10000, policy_b = policy_a;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot(), "full seeded state replay");
            check(a.observe() == b.observe() && a.options() == b.options(), "observation and options replay");
            for (float value : a.observe()) check(std::isfinite(value), "finite observation");
            const auto action_a = a.sample_action(policy_a), action_b = b.sample_action(policy_b);
            check(action_a == action_b && policy_a == policy_b, "hierarchical policy replay");
            check(a.legal(action_a), "sampled semantic action legal");
            const auto ra = a.step(action_a), rb = b.step(action_b);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards, "replay step result");
            check(ra.rewards[0] == -ra.rewards[1], "zero-sum result");
            check(!(ra.terminated && ra.truncated), "termination and truncation mutually exclusive");
            for (const auto& player : a.state().players) {
                check(player.hp >= 0 && player.hp <= 99 && player.mp >= 0 && player.mp <= 99,
                    "resource invariants");
                check(player.hand_count <= hand_capacity, "capacity invariant");
                for (int i = 0; i < player.hand_count; ++i) check(find_card(player.hand[i]), "active IDs supported");
                for (int i = player.hand_count; i < hand_capacity; ++i) check(player.hand[i] == 0, "canonical compact hand tail");
            }
            Env resumed;
            check(resumed.restore(a.snapshot()) && resumed.snapshot() == a.snapshot(), "every produced state restorable");
        }
        check(a.snapshot() == b.snapshot(), "complete trajectory replay");
        check(!a.options().pray && !a.options().weapon_bases && !a.options().defense_slots,
            "no terminal action options");
        ++completed;
    }
    check(completed == 1000, "1000 bounded seeded rollouts complete");
}
}
int main() {
    catalog_and_initialization(); legal_combinations(); consumption_and_defense();
    prayer_discard_and_capacity(); full_width_masks(); utility_and_terminal(); restore_and_privacy(); scheduling_and_rollouts();
    std::cout << "Legacy checks passed (source-derived neutral fixtures + 1000 replay rollouts).\n";
}
