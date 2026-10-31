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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = c.revival = c.item_removal = c.hp_absorption = c.self_damage = c.pestle_mortar = c.selling = c.buying = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL buying: " << message << '\n'; std::exit(1); } }
Action buy(std::uint8_t target = 1, std::uint8_t base = 0) { return {ActionKind::buy,base,0,target}; }
Action accept() { return {ActionKind::defend,no_slot,0,no_slot}; }
Action decision(bool yes) { return {yes ? ActionKind::buy_accept : ActionKind::buy_decline}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0; p.incoming_sale = p.incoming_buy = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy);
    check(env.restore(s), "restore explicit buying fixture"); return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "buy offer/actor/slot/history/receipt roundtrip exactly"); }
void reject(Env& env, Action a) { const auto before = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == before, "noncanonical purchase or incompatible action rejects atomically"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "isolate original BUY gift"); }
void ticket(Env& env, std::uint64_t rank) { const std::uint64_t ids[] = {rank}; check(env.script_buy_tickets(ids), "script raw physical-slot purchase ticket"); }
bool no_retained(const Player& p) {
    return std::all_of(p.abilities.begin(),p.abilities.end(),[](auto x){return !x;})
        && std::all_of(p.defensive_abilities.begin(),p.defensive_abilities.end(),[](auto x){return !x;})
        && std::all_of(p.additive_abilities.begin(),p.additive_abilities.end(),[](auto x){return !x;}) && !p.doubling_ability && !p.wide_ability;
}

void metadata_minimum_and_yes_no() {
    const auto* card = find_buying_card(4);
    check(buying_card_count == 1 && buying_catalog().size() == 1 && buying_index(4) == 0 && card && card->card.kind == Kind::trade
        && !card->card.attack && !card->card.boost && !card->card.defense && !card->card.hp && !card->card.mp
        && card->card.price == 5 && card->card.weight == 20 && card->card.discardable && !find_buying_card(3), "strict BUY4 TRADE metadata");
    check(sale_price_count == 216 && sale_price(4) == -1 && total_miracle_count == 15 && main_ability_capacity == 16,
        "old216 price metadata and main-ability indices remain frozen");
    Config minimum; minimum.buying = true;
    for (bool yes : {false,true}) {
        auto env = fixture({4},{120,177},minimum); draws(env); ticket(env,1);
        check(std::string_view(env.profile()) == buying_ruleset && env.snapshot().version == 36 && env.options().buy_slots == 1
            && !env.options().purchase_choices && env.options().discard_slots == 1 && env.options().pray, "minimum independent BUY role and stable profile");
        check(env.step(buy()).buy_used && env.state().pending.kind == Effect::buy && env.state().pending.buy_card == 4
            && env.state().pending.element == Element::none && env.state().actor == 1 && env.state().players[0].pending_deals == 1,
            "BUY consumes once and waits for target's response before random selection"); roundtrip(env);
        const auto offered = env.step(accept()); check(offered.buy_attempt && offered.buy_offered && offered.buy_item == 177 && offered.buy_price == 10
            && offered.buy_slot == 1 && offered.buy_population == 2 && env.state().phase == Phase::purchase && env.state().actor == 0
            && env.state().turn_owner == 0 && env.state().turns == 0 && env.snapshot().draw_index == 0 && env.snapshot().buy_index == 1
            && env.state().last_buy.outcome == BuyOutcome::offered && env.state().last_buy.turn == 1,
            "selection creates funded immutable offer without running owner tick, gifts, or advancing original turn"); roundtrip(env);
        const auto opts = env.options(); check(opts.purchase_choices == 3 && !opts.weapon_bases && !opts.utility_slots && !opts.discard_slots
            && !opts.magic_free_slots && !opts.buy_slots && !opts.sell_slots, "purchase exposes exactly NO/YES choices");
        const auto r = env.step(decision(yes)); check(r.buy_decided && r.buy_accepted == yes && r.buy_delivered == yes
            && r.buy_item == 177 && r.buy_price == 10 && r.buy_outcome == (yes ? BuyOutcome::purchased : BuyOutcome::declined)
            && env.state().players[0].money == (yes ? 10 : 20) && env.state().players[1].money == (yes ? 30 : 20)
            && env.state().players[1].hand_count == (yes ? 1 : 2) && env.state().players[0].hand_count == (yes ? 2 : 1)
            && env.snapshot().buy_index == 1 && env.snapshot().draw_index == 1 && env.state().turns == 1,
            "YES transfers one card and pays wallets, NO transfers nothing; neither reselects or adds a product gift"); roundtrip(env);
    }
    auto old_cfg = config; old_cfg.buying = false; Env old(old_cfg); const auto before = old.snapshot(); const CardId ids[] = {4};
    const std::uint64_t raw[] = {0}; check(!old.script_draws(ids) && !old.script_buy_tickets(raw) && old.snapshot() == before
        && old.script_buy_tickets({}), "old31 excludes4 and nonempty buy tape but permits clear");
}

void empty_self_insufficient_zero_price_and_rng() {
    auto empty = fixture({4},{}); draws(empty); ticket(empty,999); const auto rng = empty.snapshot().rng_state;
    empty.step(buy()); const auto none = empty.step(accept()); check(none.buy_attempt && !none.buy_offered && none.buy_outcome == BuyOutcome::empty
        && !none.buy_item && !none.buy_population && none.buy_slot == no_slot && empty.snapshot().buy_index == 0
        && empty.snapshot().rng_state == rng && empty.snapshot().draw_index == 1, "empty target consumes no choice RNG and needs no purchase input"); roundtrip(empty);
    auto singleton = fixture({4},{120}); draws(singleton); const auto initial_rng = singleton.snapshot().rng_state;
    singleton.step(buy()); singleton.step(accept()); check(singleton.snapshot().rng_state != initial_rng
        && singleton.state().phase == Phase::purchase, "singleton choice still consumes default RNG"); roundtrip(singleton);
    auto insufficient = fixture({4},{56}); auto s = insufficient.snapshot(); s.state.players[0].money = 29;
    s.state.players[0].mp = s.state.players[0].hp = 99; check(insufficient.restore(s), "restore insufficient wallet despite plentiful MP/HP"); draws(insufficient); ticket(insufficient,0);
    insufficient.step(buy()); const auto declined = insufficient.step(accept()); check(declined.buy_attempt && !declined.buy_offered
        && declined.buy_outcome == BuyOutcome::insufficient && declined.buy_item == 56 && declined.buy_price == 30
        && insufficient.state().players[0].money == 29 && insufficient.state().players[0].mp == 99 && insufficient.state().players[0].hp == 99
        && insufficient.state().players[1].hand[0] == 56 && insufficient.snapshot().buy_index == 1,
        "insufficient wallet ends after revealing selected item and never spends MP/HP"); roundtrip(insufficient); reject(insufficient,decision(true)); reject(insufficient,decision(false));
    auto self = fixture({4,44},{120}); draws(self); ticket(self,0); s = self.snapshot(); s.state.players[0].money = 99; check(self.restore(s), "restore self selection");
    const auto sampled = self.step(buy(0)); check(sampled.buy_attempt && !sampled.buy_offered && sampled.buy_outcome == BuyOutcome::self
        && sampled.buy_item == 44 && self.state().players[0].money == 99 && self.state().players[0].hand[0] == 44
        && self.state().players[0].hand_count == 2 && self.snapshot().buy_index == 1 && self.snapshot().draw_index == 1,
        "source-compatible self BUY only samples and ends without approval, transfer, or wallet changes"); roundtrip(self);
    auto free = fixture({4},{245}); s = free.snapshot(); s.state.players[0].money = 0; check(free.restore(s), "restore zero-price mortar offer"); draws(free); ticket(free,0);
    free.step(buy()); check(free.step(accept()).buy_offered && free.state().last_buy.price == 0, "zero wallet can receive an optional zero-price offer");
    const auto acquired = free.step(decision(true)); check(acquired.buy_accepted && acquired.buy_delivered && !acquired.buy_money_lost && !acquired.buy_money_gained
        && free.state().players[0].hand[0] == 245 && !free.state().players[1].hand_count, "245 purchases physically transfer despite ordinary discard immunity"); roundtrip(free);
}

void every_product_and_price_fallback() {
    auto inspect = [](CardId id, int price) {
        auto env = fixture({4},{id}); auto s = env.snapshot(); s.state.players[0].money = 99; s.state.players[0].mp = 0;
        check(env.restore(s), "restore funded0MP buyer for every commodity"); draws(env); ticket(env,std::numeric_limits<std::uint64_t>::max());
        env.step(buy()); check(env.step(accept()).buy_offered && env.state().last_buy.item == id && env.state().last_buy.price == price,
            "all217 supported physical commodity IDs have known offer prices"); roundtrip(env);
        const auto r = env.step(decision(true)); check(r.buy_delivered && r.buy_money_lost == price && r.buy_money_gained == price
            && env.state().players[0].money == 99-price && env.state().players[0].mp == 0 && env.state().players[0].hand[0] == id
            && env.state().players[0].hand_count == 2 && env.state().players[1].hand_count == 0 && env.snapshot().draw_index == 1
            && no_retained(env.state().players[0]) && no_retained(env.state().players[1]),
            "buying physical MAGIC/passive/special/trade is a transfer, never a cast or extra gift"); roundtrip(env);
    };
    for (const auto& price : sale_prices()) inspect(price.id,price.price); inspect(4,5);
    auto sale = fixture({3,4},{120}); draws(sale); check(sale.observe()[1193] == 5.f/max_sale_price, "old selling own-price feature uses BUY4 fallback");
    sale.step({ActionKind::sell,0,2,1}); const auto r = sale.step(accept()); check(r.sale_resolved && r.sale_price == 5 && r.sale_delivered
        && sale.state().players[1].hand[1] == 4 && sale_price(4) == -1, "SELL can trade new BUY4 while frozen old price API still returns unknown"); roundtrip(sale);
    auto capped = fixture({4},{44}); auto s = capped.snapshot(); s.state.players[1].money = 98; check(capped.restore(s), "restore cap99 seller income"); ticket(capped,0);
    capped.step(buy()); capped.step(accept()); const auto gain = capped.step(decision(true)); check(gain.buy_money_lost == 10 && gain.buy_money_gained == 1
        && capped.state().players[0].money == 10 && capped.state().players[1].money == 99, "buyer pays full price even when seller credit clips to99"); roundtrip(capped);
}

void duplicate_slot_privacy_and_phase_canonicality() {
    auto a = fixture({4,99,3,216,239},{120,44,120}), b = fixture({4,99,3,216,239},{120,44,120}); ticket(a,0); ticket(b,2);
    check(a.observe() == b.observe() && a.options() == b.options(), "future physical-slot ticket private before BUY"); a.step(buy()); b.step(buy());
    check(a.observe() == b.observe() && a.legal(accept()) == b.legal(accept()), "unresolved choice remains private to environment"); a.step(accept()); b.step(accept());
    check(a.state().last_buy.slot == 0 && b.state().last_buy.slot == 2 && a.observe() == b.observe() && a.options() == b.options(),
        "identical revealed ID/price does not expose which duplicate physical slot was chosen"); roundtrip(a); roundtrip(b);
    reject(a,{ActionKind::attack,0,0,1}); // Actual99 is present: guards the old special-case legal() hole.
    reject(a,{ActionKind::use,2,0,0}); reject(a,{ActionKind::sell,1,1,1}); reject(a,{ActionKind::discard,no_slot,8,no_slot});
    reject(a,buy()); reject(a,accept()); reject(a,{ActionKind::pray});
    for (int error = 0; error < 7; ++error) {
        auto malformed = decision(true);
        if (error == 0) malformed.base = 0;
        if (error == 1) malformed.target = 0;
        if (error == 2) malformed.slots = 1;
        if (error == 3) malformed.counter_count = 1;
        if (error == 4) malformed.counter_order = 1;
        if (error == 5) malformed.extra_count = 1;
        if (error == 6) malformed.extras[64] = 1;
        reject(a,malformed);
    }
    const auto raw_rng = a.snapshot().rng_state; const auto ra = a.step(decision(true)), rb = b.step(decision(true));
    check(ra.buy_item == 120 && rb.buy_item == 120 && a.state().players[1].hand[0] == 44 && a.state().players[1].hand[1] == 120
        && b.state().players[1].hand[0] == 120 && b.state().players[1].hand[1] == 44,
        "YES deletes the fixed selected physical copy rather than first matching ID");
    check(a.snapshot().buy_index == 1 && b.snapshot().buy_index == 1 && a.snapshot().rng_state != raw_rng,
        "approval uses no second choice but normal deferred gifts still consume draw RNG"); roundtrip(a); roundtrip(b);
}

void reflection_actor_owner_tick_and_receipt() {
    auto reflected = fixture({4,44},{196}); auto s = reflected.snapshot(); s.state.players[0].hp = 1; s.state.players[0].disease = Disease::hell;
    check(reflected.restore(s), "restore original owner disease and reflected buyer"); draws(reflected); ticket(reflected,0); const std::uint8_t roll[] = {99};
    check(reflected.script_disease_rolls(roll), "force one ordinary Hell tick"); reflected.step(buy()); reflected.step(special(0));
    check(reflected.state().pending.source == 1 && reflected.state().pending.target == 0 && reflected.state().actor == 0,
        "196 redirects BUY but original owner remains0"); roundtrip(reflected);
    const auto offer = reflected.step(accept()); check(offer.buy_offered && reflected.state().phase == Phase::purchase && reflected.state().actor == 1
        && reflected.state().turn_owner == 0 && reflected.snapshot().disease_index == 0, "offer actor switches to current buyer, not original owner"); roundtrip(reflected);
    const auto done = reflected.step(decision(true)); check(done.buy_source == 1 && done.buy_target == 0 && done.buy_item == 44
        && done.disease_tick && reflected.snapshot().disease_index == 1 && reflected.state().players[0].hp == 0
        && reflected.state().players[1].hp == 40 && reflected.state().winner == 1, "approval ticks original owner exactly once after reflected purchase"); roundtrip(reflected);
    auto revival = fixture({4},{244}); s = revival.snapshot(); s.state.players[0].hp = 1; s.state.players[0].disease = Disease::hell;
    check(revival.restore(s), "restore bought passive before owner disease"); draws(revival); ticket(revival,0); check(revival.script_disease_rolls(roll), "tick after purchased244");
    revival.step(buy()); revival.step(accept()); const auto saved = revival.step(decision(true));
    check(saved.buy_delivered && saved.disease_tick && saved.revivals[0] == 1 && revival.state().players[0].hp == 10
        && revival.state().players[0].hand[0] == 120 && revival.snapshot().draw_index == 1 && revival.state().players[0].incoming_buy == 0,
        "new244 is usable at owner tick before original gift, with no replacement gift of its own"); roundtrip(revival);
    auto terminal = fixture({4},{44}); s = terminal.snapshot(); auto& owner = s.state.players[0]; owner.hand.fill(120); owner.hand[0] = 4; owner.hand_count = 16;
    owner.hp = 1; owner.disease = Disease::hell; check(terminal.restore(s), "restore initially full buyer before funded receipt and death"); draws(terminal); ticket(terminal,0);
    check(terminal.script_disease_rolls(roll), "tick after physical receipt"); terminal.step(buy()); terminal.step(accept()); const auto purchased = terminal.step(decision(true));
    check(purchased.buy_delivered && terminal.done() && terminal.state().players[0].hand_count == 16 && terminal.state().players[0].hand[15] == 44
        && terminal.state().players[0].incoming_buy == 1 && !terminal.state().players[0].incoming_sale && terminal.state().players[0].pending_deals == 1
        && terminal.state().last_buy.turn == terminal.state().turns && terminal.snapshot().draw_index == 0,
        "buyer/source gets separate credit when purchased card fills16 and owner disease prevents gift clearing"); roundtrip(terminal);
    const auto exact = terminal.snapshot();
    for (int error = 0; error < 6; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.players[0].incoming_buy = 0;
        if (error == 1) bad.state.players[0].incoming_buy = 2;
        if (error == 2) bad.state.last_buy.delivered = false;
        if (error == 3) --bad.state.last_buy.turn;
        if (error == 4) bad.state.players[1].incoming_buy = 1;
        if (error == 5) bad.state.players[0].incoming_sale = 1;
        check(!terminal.restore(bad) && terminal.snapshot() == exact, "receipt type, buyer seat, latest-turn nonce and delivered proof reject atomically");
    }
}

void truncation_and_strict_offer_restore() {
    Config short_cfg; short_cfg.buying = true; short_cfg.max_decisions = 2; auto limited = fixture({4},{120},short_cfg); ticket(limited,0);
    limited.step(buy()); const auto offer = limited.step(accept()); check(offer.buy_offered && limited.done() && limited.state().truncated
        && !limited.state().terminated && limited.state().phase == Phase::purchase && !limited.options().purchase_choices
        && limited.state().players[0].pending_deals == 1 && limited.state().turns == 0,
        "decision2 can truncate with a complete immutable offer before any original gift/tick"); roundtrip(limited); reject(limited,decision(true));
    short_cfg.max_decisions = 1; auto unselected = fixture({4},{120},short_cfg); ticket(unselected,0); unselected.step(buy());
    check(unselected.done() && unselected.state().phase == Phase::response && !unselected.snapshot().buy_index && !unselected.state().last_buy.id,
        "decision1 truncates before selection without consuming hidden ticket"); roundtrip(unselected);
    for (bool yes : {false,true}) {
        short_cfg.max_decisions = 3; auto final_choice = fixture({4},{120},short_cfg); draws(final_choice); ticket(final_choice,0);
        final_choice.step(buy()); final_choice.step(accept()); const auto settled = final_choice.step(decision(yes));
        check(settled.buy_decided && settled.actor == 0 && settled.truncated && !settled.terminated && final_choice.done()
            && final_choice.state().phase == Phase::main && final_choice.state().decisions == 3 && final_choice.state().turns == 1
            && final_choice.snapshot().draw_index == 1 && final_choice.state().players[0].pending_deals == 0
            && final_choice.state().players[0].money == (yes ? 19 : 20),
            "decision3 settlement follows common step tail: one finish, one gift, then truncation for both YES and NO"); roundtrip(final_choice);
    }
    auto env = fixture({4},{120,177}); ticket(env,0); env.step(buy()); env.step(accept()); const auto exact = env.snapshot();
    for (int error = 0; error < 13; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.actor = 1;
        if (error == 1) bad.state.last_buy.slot = 1;
        if (error == 2) bad.state.last_buy.population = 1;
        if (error == 3) bad.state.last_buy.price = 2;
        if (error == 4) bad.state.players[0].money = 0;
        if (error == 5) bad.state.last_buy.outcome = BuyOutcome::purchased;
        if (error == 6) bad.state.last_buy.turn = 0;
        if (error == 7) bad.state.pending.miracle = 216;
        if (error == 8) bad.state.players[0].retained_deals = 1;
        if (error == 9) ++bad.state.players[0].pending_deals;
        if (error == 10) bad.state.decisions = 1;
        if (error == 11) { bad.state.players[0].hand.fill(120); bad.state.players[0].hand_count = 16; }
        if (error == 12) bad.state.players[1].hp = 0;
        check(!env.restore(bad) && env.snapshot() == exact, "strict offer actor/slot/funds/nonce/parity/phase/arrival validation rejects atomically");
    }
    std::array<std::uint64_t,65> too_many{}; check(!env.script_buy_tickets(too_many) && env.snapshot() == exact, "oversize tape rejects without mutating live offer");
    auto bad_tail = exact; bad_tail.scripted_buys[bad_tail.buy_count] = 1;
    check(!env.restore(bad_tail) && env.snapshot() == exact, "hidden tape unused tail remains canonical");
    env.step(decision(true)); const auto completed = env.snapshot(); auto undelivered = completed; undelivered.state.last_buy.delivered = false;
    check(!env.restore(undelivered) && env.snapshot() == completed, "purchased history must include successful delivery because every canonical buyer had space");
    auto collision = completed;
    collision.state.last_sale.id = 3; collision.state.last_sale.item = 120; collision.state.last_sale.source = 0; collision.state.last_sale.target = 1;
    collision.state.last_sale.price = 1; collision.state.last_sale.delivered = true; collision.state.last_sale.money_lost = 1;
    collision.state.last_sale.money_gained = 1; collision.state.last_sale.turn = collision.state.last_buy.turn;
    check(!env.restore(collision) && env.snapshot() == completed, "two different completed trade events cannot claim one original turn"); roundtrip(env);
}

void old_prefix_and_seeded_replay() {
    auto old_cfg = config; old_cfg.buying = false; auto old = fixture({120},{120},old_cfg), current = fixture({120},{120});
    const auto old_obs = old.observe(), new_obs = current.observe(); check(std::equal(old_obs.begin(),old_obs.begin()+1221,new_obs.begin())
        && std::all_of(old_obs.begin()+1221,old_obs.end(),[](float x){return x == 0.f;}), "old31 feature prefix1221 remains fixed and30 added features are zero");
    auto own = fixture({4},{120}); check(own.observe()[1221] == 1.f && own.observe()[1222] == 1.f, "new BUY own-card offsets fixed");
    std::uint64_t attempts = 0, decisions = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env x(config), y(config); x.reset(seed); y.reset(seed); std::uint64_t px = seed+320000, py = px;
        while (!x.done()) {
            check(x.snapshot() == y.snapshot() && x.observe() == y.observe(), "three-phase buying seeded state/observation replay");
            for (float value : x.observe()) check(std::isfinite(value), "offer and receipt observations finite");
            const auto a = x.sample_action(px), b = y.sample_action(py); check(a == b && px == py && x.legal(a), "purchase-only and main samplers deterministic/legal");
            const auto rx = x.step(a), ry = y.step(b); check(rx.valid && ry.valid && rx.rewards == ry.rewards
                && rx.buy_attempt == ry.buy_attempt && rx.buy_offered == ry.buy_offered && rx.buy_accepted == ry.buy_accepted
                && rx.buy_item == ry.buy_item && rx.buy_slot == ry.buy_slot && rx.buy_money_lost == ry.buy_money_lost,
                "physical selection, approvals, transfers and money outcomes replay exactly");
            attempts += rx.buy_attempt; decisions += rx.buy_decided; roundtrip(x);
        }
    }
    check(attempts && decisions, "natural pool and policy exercise selection plus purchase decisions");
}
}
int main() {
    metadata_minimum_and_yes_no(); empty_self_insufficient_zero_price_and_rng(); every_product_and_price_fallback();
    duplicate_slot_privacy_and_phase_canonicality(); reflection_actor_owner_tick_and_receipt(); truncation_and_strict_offer_restore(); old_prefix_and_seeded_replay();
    std::cout << "Buying passed (all217 products/offer phase/physical duplicates/price fallback/receipts/truncation/privacy,1000 replay rollouts).\n";
}
