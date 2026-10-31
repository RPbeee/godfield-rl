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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = true;
    return c;
}();
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL money counter: " << message << '\n'; std::exit(1); }
}
Action attack() { return {ActionKind::attack,0,0,1}; }
Action defend(std::uint16_t slots = 0, std::initializer_list<int> order = {}, int base = no_slot) {
    Action a{ActionKind::defend,static_cast<std::uint8_t>(base),slots,no_slot};
    a.counter_count = static_cast<std::uint8_t>(order.size()); int shift = 0;
    for (int slot : order) { a.counter_order |= std::uint64_t(slot)<<shift; shift += 4; }
    return a;
}
void hand(Player& p, std::initializer_list<CardId> cards) {
    p.hand.fill(0); std::copy(cards.begin(),cards.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(cards.size());
    p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy);
    check(env.restore(snapshot), "restore explicit money counter fixture"); return env;
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "money counter snapshot exact roundtrip");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "illegal absorption response preserves wallets, resources, queue and RNG");
}
void metadata_and_profile() {
    check(money_counter_card_count == 1 && money_counter_catalog().size() == 1, "single new absorption counter scope");
    const auto* card = find_money_counter_card(194);
    check(card && card->element == Element::neutral && card->multiplier == 1 && card->card.kind == Kind::armor
        && !card->card.attack && !card->card.boost && !card->card.defense && !card->card.hp && !card->card.mp
        && card->card.price == 10 && card->card.weight == 1 && card->card.discardable,
        "194 requests raw residual, has no ordinary defense, MP cost or holding role");
    for (CardId id : {CardId{0},CardId{188},CardId{189},CardId{190},CardId{193},CardId{220},CardId{65535}})
        check(!find_money_counter_card(id), "new lookup does not erase other counter or money-miracle semantics");
    auto env = fixture({194}, {120});
    check(std::string_view(env.profile()) == money_counter_ruleset && env.snapshot().version == 37
        && env.options().discard_slots == 1 && !env.options().weapon_bases && !env.options().utility_slots,
        "absorption profile and response-only artifact role explicit");
    reject(env,{ActionKind::use,0,0,1});
    check(env.step({ActionKind::discard,no_slot,1,no_slot}).valid && env.state().players[0].money == 20
        && env.state().players[0].hand_count == 0, "discard does not trigger absorption or earn replacement"); roundtrip(env);
    auto bad = config; bad.counter_extensions = false;
    bool threw = false; try { Env invalid(bad); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "money counter requires ordered counter extensions");
    Config narrow; narrow.elements = narrow.chance = narrow.harms = narrow.celestial = narrow.counters = narrow.counter_extensions = narrow.money_counter = true;
    auto independent = fixture({5}, {194},narrow); independent.step(attack()); independent.step(defend(1,{0})); roundtrip(independent);
    independent.step(defend()); check(independent.state().players[0].money == 19 && independent.state().players[1].money == 21,
        "minimum dependencies execute absorption without magic, disease or reflection profiles"); roundtrip(independent);
    auto old_cfg = config; old_cfg.money_counter = false; Env old(old_cfg); const auto original = old.snapshot();
    const CardId id[] = {194}; check(!old.script_draws(id) && old.snapshot() == original, "old counter pool cannot acquire194");
    auto forged = original; forged.state.players[forged.state.actor].hand[0] = 194;
    check(!old.restore(forged) && old.snapshot() == original, "disabled money counter hand rejected atomically");
    forged = env.snapshot(); forged.version = 14; check(!env.restore(forged), "old common snapshot schema rejected");
}
void payment_cascade_and_credit_cap() {
    struct Case { int cash,mp,hp,recipient; };
    constexpr std::array<Case,8> cases{{{20,10,40,20},{10,0,40,20},{9,1,40,20},{9,0,40,20},
        {0,20,40,20},{5,3,40,95},{0,0,4,99},{99,99,99,0}}};
    for (const auto test : cases) {
        auto env = fixture({36}, {194}); auto snapshot = env.snapshot();
        snapshot.state.players[0].money = test.cash; snapshot.state.players[0].mp = test.mp; snapshot.state.players[0].hp = test.hp;
        snapshot.state.players[1].money = test.recipient;
        check(env.restore(snapshot), "restore resource payment boundary fixture"); env.step(attack()); const auto queued = env.step(defend(1,{0}));
        check(queued.counter_queued == 1 && !queued.counter_money_absorptions && env.state().pending.kind == Effect::absorb_money
            && env.state().pending.amount == 10 && env.state().pending.counter_card == 194
            && env.state().players[1].money == test.recipient, "credit/debt waits for actual counter response, not reservation");
        roundtrip(env);
        const int cash_lost = std::min(10,test.cash), mp_lost = std::min(10-cash_lost,test.mp);
        const int hp_lost = std::min(10-cash_lost-mp_lost,test.hp), credit = std::min(10,99-test.recipient);
        const auto result = env.step(defend());
        check(result.counter_money_absorptions == 1 && result.counter_money_gained == unsigned(credit)
            && result.counter_money_lost == unsigned(cash_lost) && result.counter_mp_lost == unsigned(mp_lost)
            && result.counter_hp_lost == unsigned(hp_lost) && env.state().players[0].money == test.cash-cash_lost
            && env.state().players[0].mp == test.mp-mp_lost && env.state().players[0].hp == test.hp-hp_lost
            && env.state().players[1].money == test.recipient+credit,
            "full requested credit precedes cash, MP then HP payment with independent caps");
        check(!result.dark_execution && !env.state().players[0].glory && !env.state().players[0].dark_cloud,
            "HP payment is neither dark damage nor a copied status payload"); roundtrip(env);
    }
}
void defense_and_counter_type() {
    auto env = fixture({36,186,195,190,194}, {194}); auto snapshot = env.snapshot();
    snapshot.state.players[0].money = 5; snapshot.state.players[0].mp = 3;
    check(env.restore(snapshot), "restore DEF30 and removal before absorption"); env.step(attack()); env.step(defend(1,{0}));
    const auto result = env.step(defend(15,{2,3}));
    check(result.counter_money_absorptions == 1 && result.counter_money_gained == 10
        && result.counter_money_lost == 5 && result.counter_mp_lost == 3 && result.counter_hp_lost == 2
        && !result.counter_queued && env.state().players[0].hp == 38,
        "DEF30/removal do not reduce bill; non-damage absorption cannot trigger selected counters"); roundtrip(env);
    auto partial = fixture({36}, {126,194}); partial.step(attack()); partial.step(defend(3,{1}));
    check(partial.state().pending.amount == 8 && partial.state().players[1].hp == 32,
        "ordinary defense on original attack reduces r before absorption reservation");
    partial.step(defend()); check(partial.state().players[0].money == 12 && partial.state().players[1].money == 28,
        "reserved post-defense request remains unchanged at execution"); roundtrip(partial);
    auto blocked = fixture({36}, {186,194}); blocked.step(attack()); const auto zero = blocked.step(defend(3,{1}));
    check(!zero.counter_queued && !zero.counter_money_absorptions && blocked.state().players[0].money == 20
        && blocked.state().players[1].money == 20, "zero original residual consumes194 but reserves no absorption");
    auto prefix = fixture({36}, {194,39}); prefix.step(attack()); const auto returned = prefix.step(defend(1,{0},1));
    check(returned.reflected && !returned.counter_queued && prefix.state().players[1].money == 20,
        "absorption counter prefix before special is consumed without money effect"); roundtrip(prefix);
    auto types = fixture({36,195,39,20,215,210}, {194}); snapshot = types.snapshot(); snapshot.state.players[0].defensive_abilities = {1,1};
    check(types.restore(snapshot), "restore type-specific reactions to absorption"); types.step(attack()); types.step(defend(1,{0}));
    for (int base = 1; base < 5; ++base) reject(types,defend(1,{},base));
    reject(types,{ActionKind::defend_cast,0,1,no_slot}); reject(types,{ActionKind::defend_cast,1,1,no_slot});
    check(types.legal(defend(1)), "absorption remains counter type after optional removal");
}
void dead_source_dead_target_and_order() {
    auto corpse = fixture({36}, {194}); auto snapshot = corpse.snapshot();
    snapshot.state.players[1].hp = 1; snapshot.state.players[1].money = 95;
    check(corpse.restore(snapshot), "restore dead absorption beneficiary"); corpse.step(attack()); corpse.step(defend(1,{0}));
    check(corpse.state().players[1].dead && corpse.state().pending.source == 1, "dead counter source still waits for living debtor response");
    roundtrip(corpse); const auto credit = corpse.step(defend());
    check(credit.counter_money_gained == 4 && credit.counter_money_lost == 10
        && corpse.state().players[1].money == 99 && corpse.state().players[1].dead
        && corpse.state().winner == 0, "dead source receives capped requested credit without resurrection"); roundtrip(corpse);
    Config narrow; narrow.elements = narrow.chance = narrow.harms = narrow.celestial = narrow.counters = narrow.counter_extensions = narrow.money_counter = true;
    auto both = fixture({36}, {194},narrow); snapshot = both.snapshot();
    snapshot.state.players[0].hp = snapshot.state.players[1].hp = 1;
    snapshot.state.players[0].money = snapshot.state.players[0].mp = 0;
    check(both.restore(snapshot), "restore dead source and debtor killed by non-damage payment");
    both.step(attack()); both.step(defend(1,{0})); const auto tied = both.step(defend());
    check(tied.counter_money_gained == 10 && tied.counter_hp_lost == 1 && tied.terminated
        && tied.rewards == std::array<float,2>{0.f,0.f} && both.state().winner == -1
        && both.state().players[0].dead && both.state().players[1].dead,
        "dead beneficiary still receives full credit while non-damage debt finalizes both-dead draw"); roundtrip(both);
    auto heaven = fixture({36,175}, {194}); snapshot = heaven.snapshot(); snapshot.state.players[0].disease = Disease::heaven;
    check(heaven.restore(snapshot), "restore Heaven debtor with defense-status prefix"); heaven.step(attack()); heaven.step(defend(1,{0}));
    const auto fell = heaven.step(defend(1));
    check(fell.counter_money_absorptions == 1 && fell.counter_hp_lost == 0 && !fell.disease_tick
        && heaven.state().players[0].money == 10 && heaven.state().players[0].hp == 0
        && heaven.state().players[0].dead && heaven.state().winner == 1,
        "Heaven prefix fall remains billable then finalizes dead without a restoring end-turn tick"); roundtrip(heaven);
    auto skipped = fixture({5}, {190,194}); snapshot = skipped.snapshot(); snapshot.state.players[0].hp = 1;
    check(skipped.restore(snapshot), "restore debtor killed before queued absorption"); skipped.step(attack()); skipped.step(defend(3,{0,1}));
    const auto skip = skipped.step(defend());
    check(skip.counter_skipped == 1 && !skip.counter_money_absorptions && skipped.state().players[1].money == 20,
        "dead target skips absorption before either credit or payment"); roundtrip(skipped);
    for (bool money_first : {false,true}) {
        auto env = fixture({5}, {194,187}); snapshot = env.snapshot();
        snapshot.state.players[0].hp = 1; snapshot.state.players[0].money = snapshot.state.players[0].mp = 0;
        check(env.restore(snapshot), "restore payment-death order fixture"); const std::uint8_t hit[] = {1}; check(env.script_hit_rolls(hit), "script fire after/before lethal payment");
        env.step(attack()); env.step(money_first ? defend(3,{0,1}) : defend(3,{1,0}));
        const auto end = env.step(defend());
        check(env.state().winner == 1 && env.state().players[1].money == (money_first ? 21 : 20)
            && end.counter_money_absorptions == (money_first ? 1 : 0)
            && end.counter_hidden_checks == (money_first ? 1 : 0),
            "mixed order distinguishes payment credit before death from dead-target skip"); roundtrip(env);
    }
}
void raw_large_integer_and_snapshots() {
    auto large = fixture({56,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170}, {194});
    auto snapshot = large.snapshot(); snapshot.state.players[0].hp = snapshot.state.players[0].mp = snapshot.state.players[0].money = 99;
    snapshot.state.players[1].hp = 99; check(large.restore(snapshot), "restore unclipped raw255 money request");
    large.step({ActionKind::attack,0,0xfffe,1}); large.step(defend(1,{0}));
    check(large.state().pending.amount == 255 && large.state().players[1].dead, "request uses raw255, not defender's99 actual lost HP"); roundtrip(large);
    const auto paid = large.step(defend());
    check(paid.counter_money_gained == 79 && paid.counter_money_lost == 99 && paid.counter_mp_lost == 99
        && paid.counter_hp_lost == 57 && large.state().players[0].hp == 42 && large.state().winner == 0,
        "natural oversized absorption preserves full requested credit and sequential resource payment"); roundtrip(large);
    auto huge = fixture({5}, {194}); huge.step(attack()); huge.step(defend(1,{0})); snapshot = huge.snapshot();
    snapshot.state.pending.amount = counter_damage_limit; snapshot.state.counter_generated = snapshot.state.counter_started = 32;
    snapshot.state.decisions = 33;
    for (auto& p : snapshot.state.players) { p.hand.fill(0); p.hand_count = 0; p.pending_deals = 16; p.retained_deals = 0; }
    ++snapshot.state.players[0].pending_deals; snapshot.state.players[0].retained_deals = 1; snapshot.state.players[0].abilities[2] = 1;
    snapshot.state.players[0].money = snapshot.state.players[0].mp = snapshot.state.players[0].hp = 99;
    snapshot.state.players[1].money = 0;
    check(huge.restore(snapshot), "restore conservative int64 maximum absorption request");
    const auto clipped = huge.step(defend());
    check(clipped.counter_money_gained == 99 && clipped.counter_money_lost == 99 && clipped.counter_mp_lost == 99
        && clipped.counter_hp_lost == 99 && huge.state().players[0].dead, "int64 payment remainder clips each resource safely without narrowing overflow"); roundtrip(huge);
    auto env = fixture({5}, {194}); env.step(attack()); env.step(defend(1,{0})); const auto exact = env.snapshot();
    for (int error = 0; error < 6; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.pending.kind = Effect::money;
        if (error == 1) bad.state.pending.amount = 0;
        if (error == 2) bad.state.pending.counter_card = 193;
        if (error == 3) bad.state.pending.target = bad.state.pending.source;
        if (error == 4) bad.state.pending.hit_rate = 75;
        if (error == 5) bad.config.money_counter = false;
        check(!env.restore(bad) && env.snapshot() == exact, "wrong money effect, request, card, target, rate or profile rejected atomically");
    }
}
void observations_privacy_and_replay() {
    auto main = fixture({194}, {120}); check(main.observe()[661] == 1.f && main.observe()[662] == 1.f, "new hand194 feature distinct from other counter kinds");
    auto env = fixture({5}, {194,190}); env.step(attack()); env.step(defend(3,{1,0}));
    check(env.observe()[533] == 194/363.f && env.observe()[534] == 7/6.f,
        "queued absorption uses appended effect7 while old Effect/6 observation contract is maintained"); roundtrip(env);
    env.step(defend()); check(env.observe()[678] == 1.f, "current absorption is identified in new suffix"); roundtrip(env);
    auto hidden = env.snapshot(); hidden.rng_state ^= 0x9e3779b97f4a7c15ULL; hidden.state.players[1].defensive_abilities = {7,8};
    Env other(config); check(other.restore(hidden) && other.observe() == env.observe() && other.options() == env.options(),
        "future RNG and opponent retained counts not exposed by absorption");
    auto private_hand = fixture({5}, {194}); const auto obs = private_hand.observe(); auto swap = private_hand.snapshot(); swap.state.players[1].hand[0] = 190;
    check(private_hand.restore(swap) && private_hand.observe() == obs, "opponent unplayed194 remains private");
    auto old_cfg = config; old_cfg.money_counter = false; Env old(old_cfg); const auto oldobs = old.observe();
    check(std::all_of(oldobs.begin()+661,oldobs.end(),[](float value){return value==0.f;}), "old profiles new18feature suffix zero");
    std::uint64_t absorbed = 0,gained = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+85000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "money counter seeded replay");
            for (float value : a.observe()) check(std::isfinite(value), "money queue observations finite");
            const auto action = a.sample_action(pa), copy = b.sample_action(pb);
            check(action == copy && pa == pb && a.legal(action), "money counter sampler legal and reproducible");
            const auto ra = a.step(action), rb = b.step(copy);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.counter_money_absorptions == rb.counter_money_absorptions
                && ra.counter_money_gained == rb.counter_money_gained && ra.counter_money_lost == rb.counter_money_lost
                && ra.counter_mp_lost == rb.counter_mp_lost && ra.counter_hp_lost == rb.counter_hp_lost,
                "money counter actual resource deltas replay"); absorbed += ra.counter_money_absorptions; gained += ra.counter_money_gained; roundtrip(a);
        }
    }
    check(absorbed && gained, "natural games actually execute newly drawn money counters");
}
}
int main() {
    metadata_and_profile(); payment_cascade_and_credit_cap(); defense_and_counter_type();
    dead_source_dead_target_and_order(); raw_large_integer_and_snapshots(); observations_privacy_and_replay();
    std::cout << "Money counter checks passed (credit/cash/MP/HP, corpse/order/int64, 1000 replay rollouts).\n";
}
