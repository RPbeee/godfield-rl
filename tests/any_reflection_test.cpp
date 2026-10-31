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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = true;
    return c;
}();
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL any reflection: " << message << '\n'; std::exit(1); }
}
Action attack() { return {ActionKind::attack,0,0,1}; }
Action use() { return {ActionKind::use,0,0,1}; }
Action defend(std::uint16_t slots = 0, std::initializer_list<int> order = {}) {
    Action a{ActionKind::defend,no_slot,slots,no_slot}; a.counter_count = static_cast<std::uint8_t>(order.size());
    int shift = 0; for (int slot : order) { a.counter_order |= std::uint64_t(slot)<<shift; shift += 4; } return a;
}
Action reflect(int base, std::uint16_t slots = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(base),slots,no_slot}; }
void hand(Player& p, std::initializer_list<CardId> cards) {
    p.hand.fill(0); std::copy(cards.begin(),cards.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(cards.size());
    p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy);
    check(env.restore(snapshot), "restore explicit ANY reflection fixture"); return env;
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "ANY snapshot preserves origin and current frame");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "invalid ANY prefix/category preserves complete state, gifts and all random streams");
}
void hits(Env& env) { const std::uint8_t roll[] = {1,100}; check(env.script_hit_rolls(roll), "script original public hit and preserved future roll"); }
void gifts(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "script deferred ANY gifts"); }
void metadata_and_all_weapon_attributes() {
    check(any_reflection_card_count == 1 && any_reflection_catalog().size() == 1, "single ANY artifact scope");
    const auto* card = find_any_reflection_card(196);
    check(card && card->card.kind == Kind::armor && !card->card.attack && !card->card.boost && !card->card.defense
        && !card->card.hp && !card->card.mp && card->card.price == 10 && card->card.weight == 1 && card->card.discardable,
        "ANY196 is a zero-DFS response artifact with no MP or holding role");
    for (CardId id : {CardId{0},CardId{39},CardId{40},CardId{195},CardId{210},CardId{65535}})
        check(!find_any_reflection_card(id), "no fallback to category-specific or removal effects");
    auto main = fixture({196}, {120});
    check(std::string_view(main.profile()) == any_reflection_ruleset && main.snapshot().version == 37
        && main.options().discard_slots == 1 && !main.options().weapon_bases && !main.options().utility_slots,
        "new profile/schema and discard-only main role"); reject(main,use());
    for (CardId id : {CardId{5},CardId{58},CardId{59},CardId{60},CardId{69},CardId{62},CardId{63},CardId{100}}) {
        auto env = fixture({id}, {196}); hits(env); env.step(attack()); const auto origin = env.snapshot();
        check(env.options().reaction_slots == 1 && !env.options().defense_slots, "ANY cannot opt out through ordinary slots"); reject(env,defend(1));
        const auto result = env.step(reflect(0));
        check(result.any_reflected && result.reflected && !result.magic_reflected
            && env.state().pending.amount == origin.state.pending.amount && env.state().pending.element == origin.state.pending.element
            && env.state().pending.source == 1 && env.state().pending.target == 0
            && env.state().pending.origin_source == 0 && env.snapshot().hit_index == origin.hit_index
            && env.state().last_chance == origin.state.last_chance && env.state().players[1].mp == 10,
            "standalone ANY returns all seven weapon attributes and percent hit without cost or reroll");
        roundtrip(env); env.step(defend());
        check(id != 63 || env.state().players[0].dead, "dark weapon stays dark after ANY"); roundtrip(env);
    }
    auto old_cfg = config; old_cfg.any_reflection = false; Env old(old_cfg); const auto original = old.snapshot();
    const CardId id[] = {196}; check(!old.script_draws(id) && old.snapshot() == original, "old pool excludes196");
    auto bad = original; bad.state.players[bad.state.actor].hand[0] = 196;
    check(!old.restore(bad) && old.snapshot() == original, "disabled ANY hand rejected");
    auto missing = config; missing.reflection = missing.defensive_miracles = missing.magic_reflection = false;
    bool threw = false; try { Env invalid(missing); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "ANY requires reflection explicitly enabled");
    Config narrow; narrow.reflection = narrow.any_reflection = true;
    auto minimum = fixture({5}, {196},narrow); minimum.step(attack()); check(minimum.step(reflect(0)).any_reflected,
        "ANY independently reflects ordinary weapon without miracle or element dependencies"); roundtrip(minimum);
}
void non_weapon_payloads_and_prefix_rules() {
    for (CardId id : {CardId{216},CardId{220},CardId{200},CardId{207},CardId{227},CardId{231},CardId{235}}) {
        auto env = fixture({id}, {196}); auto snapshot = env.snapshot(); snapshot.state.players[0].hp = 30;
        if (id == 235) snapshot.state.players[0].disease = Disease::cold;
        check(env.restore(snapshot), "restore diverse reflected effects"); hits(env); env.step(use()); const auto original = env.snapshot();
        const auto reflected = env.step(reflect(0));
        check(reflected.any_reflected && !reflected.magic_reflected && env.state().pending.kind == original.state.pending.kind
            && env.state().pending.status_card == original.state.pending.status_card
            && env.state().pending.miracle == original.state.pending.miracle
            && env.state().pending.harm == original.state.pending.harm && env.snapshot().hit_index == original.hit_index
            && env.state().players[0].mp == original.state.players[0].mp,
            "ANY preserves magic, sundry, cure and status provenance rather than converting type"); roundtrip(env); env.step(defend());
        if (id == 216) check(env.state().players[0].hp == 40, "magic heal returns full10");
        if (id == 220) check(env.state().players[0].money == 35, "magic money returns full15");
        if (id == 200) check(env.state().players[0].glory && env.state().players[0].hp == 29, "returned percent magic keeps Glory");
        if (id == 207) check(env.state().players[0].dead, "returned dark magic still executes");
        if (id == 227) check(env.state().players[0].hp == 35, "non-MAGIC heal returns full5");
        if (id == 231) check(env.state().players[0].mp == 15, "non-MAGIC MP returns full5");
        if (id == 235) check(env.state().players[0].disease == Disease::none, "non-MAGIC cure clears original user's Cold before tick");
        roundtrip(env);
    }
    for (CardId weapon : {CardId{58},CardId{62},CardId{63}}) {
        auto env = fixture({weapon}, {120,195,196}); env.step(attack()); reject(env,reflect(2,1));
        check(env.legal(reflect(2)) && env.legal(reflect(2,2)), "colored weapon allows standalone196 or actual removal prefix, not unrelated prefix");
        env.step(reflect(2,2)); check(env.state().pending.attribute_removed && env.state().pending.element == Element::neutral,
            "selected195 is preserved through non-MAGIC ANY reflection"); roundtrip(env);
    }
    auto magic = fixture({206}, {122,196}); magic.step(use());
    check(magic.step(reflect(1,1)).any_reflected && magic.state().pending.amount == 10,
        "MAGIC permits incompatible ordinary prefix before ANY but returns original payload"); roundtrip(magic);
    auto staged = fixture({5}, {196,39}); staged.step(attack()); reject(staged,reflect(1,1)); reject(staged,defend(1));
    check(staged.legal(reflect(0)), "196 cannot be ordinary prefix before another special");
    auto spent = fixture({5}, {190,196}); spent.step(attack());
    auto counter_prefix = reflect(1,1); counter_prefix.counter_count = 1;
    const auto special = spent.step(counter_prefix);
    check(special.any_reflected && !special.counter_queued && !spent.state().counter_generated
        && spent.state().players[1].pending_deals == 2 && spent.state().pending.amount == 1,
        "ordered190 prefix is spent but generates no counter when ANY terminates the defense"); roundtrip(spent);
    auto glory = fixture({5}, {120,196}); auto snapshot = glory.snapshot(); snapshot.state.players[1].glory = true;
    check(glory.restore(snapshot), "restore Glory ANY defender"); glory.step(attack());
    check(glory.legal(reflect(1)), "Glory allows lone ANY"); reject(glory,reflect(1,1));
}
void reflected_counters_origin_harms_and_credit() {
    for (CardId counter : {CardId{190},CardId{187},CardId{191},CardId{192}}) {
        auto env = fixture({5,196}, {counter}); hits(env); env.step(attack()); env.step(defend(1,{0})); const auto original = env.snapshot();
        const auto returned = env.step(reflect(0));
        check(returned.any_reflected && !returned.counter_resolved && !returned.counter_cancelled
            && env.state().pending.counter_card == counter && env.state().pending.origin_source == 1
            && env.state().pending.source == 0 && env.state().pending.target == 1 && env.state().reaction_count == 1
            && env.state().counter_generated == original.state.counter_generated && env.state().counter_started == original.state.counter_started
            && env.snapshot().hit_index == original.hit_index && env.state().last_chance == original.state.last_chance,
            "reflected counter changes current coordinates but not immutable origin, generation or hit record"); roundtrip(env);
        env.step(defend());
        if (counter == 190) check(env.state().players[1].hp == 37, "Stone counter returns original2 to its owner");
        if (counter == 187) check(env.state().players[1].hp == 38, "Fire75 counter is not rerolled on returned target");
        if (counter == 191) check(env.state().players[1].glory && !env.state().players[0].glory, "direct Glory follows reflected counter target");
        if (counter == 192) check(env.state().players[1].dark_cloud && !env.state().players[0].dark_cloud, "direct DarkCloud stays harm instead of dark attack");
        roundtrip(env);
    }
    for (bool twice : {false,true}) {
        auto env = fixture({36,196}, {194,196}); env.step(attack()); env.step(defend(1,{0})); env.step(reflect(0)); roundtrip(env);
        if (twice) { env.step(reflect(0)); roundtrip(env); }
        env.step(defend());
        check(env.state().players[0].money == (twice ? 10 : 30) && env.state().players[1].money == (twice ? 30 : 10),
            "194 credit belongs to reflected current source rather than immutable original card user"); roundtrip(env);
    }
    auto prefix = fixture({5,195,196,39,171,215}, {190}); prefix.step(attack()); prefix.step(defend(1,{0}));
    reject(prefix,reflect(1,1));
    check(prefix.legal(reflect(1)), "counter allows first196 only, not195-before196 counter-guard bypass");
    prefix.step(reflect(1)); roundtrip(prefix);
    auto heaven = fixture({5,196}, {191}); auto snapshot = heaven.snapshot(); snapshot.state.players[1].disease = Disease::heaven;
    check(heaven.restore(snapshot), "restore Heaven original counter source"); heaven.step(attack()); heaven.step(defend(1,{0})); heaven.step(reflect(0));
    check(heaven.step(defend()).harm_heaven_falls == 1 && heaven.state().players[1].dead,
        "returned direct harm invokes Heaven fall on final target"); roundtrip(heaven);
}
void dead_return_cancellation_and_queue() {
    for (CardId counter : {CardId{190},CardId{187},CardId{194}}) {
        auto env = fixture({36,196}, {counter}); auto snapshot = env.snapshot(); snapshot.state.players[1].hp = 1;
        check(env.restore(snapshot), "restore dead queued counter source"); hits(env); env.step(attack()); env.step(defend(1,{0}));
        const auto chance = env.state().last_chance; const auto index = env.snapshot().hit_index;
        const auto cancelled = env.step(reflect(0));
        check(cancelled.any_reflected && cancelled.counter_cancelled && !cancelled.counter_resolved
            && !cancelled.counter_skipped && !cancelled.counter_hidden_checks && env.done() && env.state().winner == 0
            && env.state().players[0].hp == 40 && env.state().players[0].money == 20
            && env.state().players[1].money == 20 && env.snapshot().hit_index == index && env.state().last_chance == chance,
            "return to already-dead source ends launched current counter without payment, reroll or double-counting queued skip"); roundtrip(env);
    }
    auto remaining = fixture({5,196}, {190,190}); auto snapshot = remaining.snapshot(); snapshot.state.players[1].hp = 1;
    check(remaining.restore(snapshot), "restore cancelled item with older pending queue"); remaining.step(attack()); remaining.step(defend(3,{0,1}));
    const auto next = remaining.step(reflect(0));
    check(next.counter_cancelled && !next.counter_resolved && next.counter_launched == 1 && !next.counter_skipped
        && remaining.state().pending.counter_card == 190 && remaining.state().pending.source == 1
        && remaining.state().pending.target == 0 && remaining.state().reaction_count == 0,
        "cancellation advances to next reserved dead-source item with reset local reaction count"); roundtrip(remaining);
    remaining.step(defend()); check(remaining.state().players[0].hp == 38, "next counter still executes after prior cancelled return"); roundtrip(remaining);
    auto automatic = fixture({5,196}, {190,193}); snapshot = automatic.snapshot(); snapshot.state.players[1].hp = 1;
    check(automatic.restore(snapshot), "restore dead selfMP entry after cancelled current"); automatic.step(attack()); automatic.step(defend(3,{0,1}));
    const auto skip = automatic.step(reflect(0));
    check(skip.counter_cancelled && !skip.counter_resolved && skip.counter_skipped == 1 && !skip.counter_auto_resolved,
        "cancelled current and later193 dead-target skip remain distinct counters"); roundtrip(automatic);
}
void full_held_chain_and_provisional_heal() {
    Config narrow; narrow.reflection = narrow.miracles = narrow.any_reflection = true;
    auto env = fixture({196}, {196},narrow); auto snapshot = env.snapshot();
    for (auto& p : snapshot.state.players) { p.hand.fill(196); p.hand_count = 16; }
    snapshot.state.players[0].abilities[0] = 1; snapshot.state.players[0].mp = 99;
    check(env.restore(snapshot), "restore minimum-flag full hand held-cast chain"); gifts(env); env.step({ActionKind::cast,0,0,1});
    for (int n = 0; n < 32; ++n) {
        const auto returned = env.step(reflect(0));
        check(returned.any_reflected && !returned.magic_reflected && env.snapshot().draw_index == 0,
            "full32 ANY chain never pays response MP or distributes early"); roundtrip(env);
    }
    check(env.state().players[0].pending_deals == 17 && env.state().players[0].retained_deals == 1,
        "ANY minimum cfg tracks original held gift separately from sixteen consumed reflectors");
    env.step(defend()); check(env.snapshot().draw_index == 33 && env.state().players[0].mp == 93,
        "all33 outcomes including held-cast overflow consumed after final heal"); roundtrip(env);
    auto mixed = fixture({216,196}, {196}); snapshot = mixed.snapshot();
    for (auto& p : snapshot.state.players) { p.defensive_abilities[0] = 1; p.mp = 99; }
    check(mixed.restore(snapshot), "restore ANY and retained210 mixed chain");
    const std::uint8_t targets[] = {1,0}; check(mixed.script_bounce_targets(targets), "script mixed defensive return targets");
    mixed.step(use()); mixed.step(reflect(0)); roundtrip(mixed);
    for (int i = 0; i < 2; ++i) {
        check(mixed.step({ActionKind::defend_cast,0,0,no_slot}).magic_bounced,
            "retained210 can continue an ANY-returned original miracle"); roundtrip(mixed);
    }
    check(mixed.step(reflect(0)).any_reflected && mixed.state().reaction_count == 4
        && mixed.state().players[0].retained_deals == 1 && mixed.state().players[1].retained_deals == 1,
        "ANY mixed chain accepts retained gifts independently of physical consumption"); roundtrip(mixed);
    mixed.step(defend()); roundtrip(mixed);
    auto heal = fixture({227,196}, {175,196}); snapshot = heal.snapshot(); snapshot.state.players[1].disease = Disease::heaven;
    check(heal.restore(snapshot), "restore non-MAGIC heal and Heaven prefix"); heal.step(use()); heal.step(reflect(1,1));
    check(!heal.state().players[1].dead && heal.state().players[1].hp == 0, "Heaven prefix death remains provisional before item end"); roundtrip(heal);
    check(!heal.step(reflect(0)).counter_cancelled, "HP0 with dead=false is not cancelled as finalized death"); roundtrip(heal);
    heal.step(defend()); check(heal.state().players[1].hp == 5 && !heal.state().players[1].dead,
        "ANY returns sundry heal to rescue provisional HP0 before finalization"); roundtrip(heal);
}
void snapshots_privacy_and_replay() {
    auto env = fixture({5,196}, {187}); hits(env); env.step(attack()); env.step(defend(1,{0})); env.step(reflect(0)); const auto exact = env.snapshot();
    for (int error = 0; error < 5; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.pending.origin_source = 0;
        if (error == 1) { bad.state.last_chance.source = 0; bad.state.last_chance.target = 1; }
        if (error == 2) bad.state.last_reaction.from_ability = true;
        if (error == 3) { bad.state.reaction_count = 3; bad.state.decisions = 10; }
        if (error == 4) bad.config.any_reflection = false;
        check(!env.restore(bad) && env.snapshot() == exact,
            "forged counter origin/hit origin/ANY ownership/physical reaction budget/profile rejected atomically");
    }
    auto own = fixture({196}, {120}); check(own.observe()[696] == 1.f && own.observe()[697] == 1.f,
        "new ANY flag and own hand feature represented");
    auto hidden = fixture({5}, {196}); const auto visible = hidden.observe(); auto swap = hidden.snapshot(); swap.state.players[1].hand[0] = 120;
    swap.rng_state ^= 0x9e3779b97f4a7c15ULL;
    check(hidden.restore(swap) && hidden.observe() == visible, "unplayed opponent ANY and future randomness remain hidden");
    auto old_cfg = config; old_cfg.any_reflection = false; Env old(old_cfg); const auto oldobs = old.observe();
    check(std::all_of(oldobs.begin()+696,oldobs.end(),[](float x){return x==0.f;}), "old profiles new17 features remain zero");
    std::uint64_t reflected = 0, cancelled = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+105000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "ANY seeded state trajectory replay");
            for (float x : a.observe()) check(std::isfinite(x), "ANY observations finite");
            const auto chosen = a.sample_action(pa), copy = b.sample_action(pb);
            check(chosen == copy && pa == pb && a.legal(chosen), "ANY sampler produces reproducible legal category/prefix choices");
            const auto ra = a.step(chosen), rb = b.step(copy);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.any_reflected == rb.any_reflected
                && ra.counter_cancelled == rb.counter_cancelled && ra.reflected == rb.reflected,
                "ANY and cancellation metrics replay"); reflected += ra.any_reflected; cancelled += ra.counter_cancelled; roundtrip(a);
        }
    }
    check(reflected, "natural games exercise newly drawn ANY artifact"); (void)cancelled;
}
}
int main() {
    metadata_and_all_weapon_attributes(); non_weapon_payloads_and_prefix_rules(); reflected_counters_origin_harms_and_credit();
    dead_return_cancellation_and_queue(); full_held_chain_and_provisional_heal(); snapshots_privacy_and_replay();
    std::cout << "ANY reflection passed (counter origin/dead cancellation/credit, full32, 1000 replay rollouts).\n";
}
