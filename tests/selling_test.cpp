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
const Config config = [] {
    Config c; c.miracles = c.elements = c.chance = c.reflection = c.disease = c.harms = true;
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = c.revival = c.item_removal = c.hp_absorption = c.self_damage = c.pestle_mortar = c.selling = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL selling: " << message << '\n'; std::exit(1); } }
Action sell(std::uint8_t target = 1, std::uint8_t item = 1, std::uint8_t base = 0) { return {ActionKind::sell,base,static_cast<std::uint16_t>(1u<<item),target}; }
Action accept() { return {ActionKind::defend,no_slot,0,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action ordered(std::initializer_list<std::uint8_t> refs) {
    Action a{ActionKind::defend_ordered,no_slot,0,no_slot}; a.extra_count = static_cast<std::uint8_t>(refs.size()); std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0; p.incoming_sale = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy);
    check(env.restore(s), "restore explicit selling fixture"); return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "sale product/receipt/budget/history roundtrip exactly"); }
void reject(Env& env, Action a) { const auto before = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == before, "invalid sale/response rejects atomically"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "isolate original SELL gift count"); }
bool no_retained(const Player& p) {
    return std::all_of(p.abilities.begin(),p.abilities.end(),[](auto x){return !x;})
        && std::all_of(p.defensive_abilities.begin(),p.defensive_abilities.end(),[](auto x){return !x;})
        && std::all_of(p.additive_abilities.begin(),p.additive_abilities.end(),[](auto x){return !x;}) && !p.doubling_ability && !p.wide_ability;
}

void metadata_minimum_and_all_physical_products() {
    const auto* card = find_selling_card(3);
    check(selling_card_count == 1 && selling_catalog().size() == 1 && selling_index(3) == 0 && card
        && card->card.kind == Kind::trade && !card->card.attack && !card->card.boost && !card->card.defense && !card->card.hp
        && !card->card.mp && card->card.price == 5 && card->card.weight == 20 && card->card.discardable,
        "SELL3 has independent TRADE metadata and no ordinary combat/MP cost");
    check(sale_price_count == 216 && sale_prices().size() == 216 && max_sale_price == 30
        && sale_price(245) == 0 && sale_price(216) == 15 && sale_price(220) == 25 && sale_price(221) == 15
        && sale_price(4) == -1 && sale_price(65535) == -1 && !find_selling_card(4), "complete separate price metadata distinguishes zero from unsupported BUY4");
    Config minimum; minimum.selling = true; auto env = fixture({3,56},{120},minimum); draws(env);
    check(std::string_view(env.profile()) == selling_ruleset && env.snapshot().version == 35 && env.options().sell_slots == 1
        && env.options().sale_item_slots == 3 && env.options().discard_slots == 1 && !env.options().utility_slots,
        "minimum SELL profile exposes base/product masks without unrelated dependencies");
    const auto used = env.step(sell()); check(used.sale_used && !used.sale_resolved && env.state().pending.kind == Effect::sell
        && env.state().pending.element == Element::none && env.state().pending.sale_card == 3 && env.state().pending.sale_item == 56
        && env.state().pending.amount == 30 && env.state().players[0].pending_deals == 1 && env.state().players[0].hand_count == 0,
        "base and product disappear before response, but only SELL base earns a gift"); roundtrip(env);
    const auto r = env.step(accept()); check(r.sale_resolved && r.sale_delivered && r.sale_item == 56 && r.sale_price == 30
        && r.sale_money_lost == 20 && r.sale_mp_lost == 10 && !r.sale_hp_lost && r.sale_money_gained == 30
        && env.state().players[0].money == 50 && env.state().players[1].money == 0 && env.state().players[1].mp == 0
        && env.state().players[1].hp == 40 && env.state().players[1].hand[1] == 56 && env.snapshot().draw_index == 1,
        "forced price pays YEN then MP then HP, delivers product, credits current source, pays one original gift"); roundtrip(env);
    std::uint64_t magic_products = 0;
    for (const auto& price : sale_prices()) {
        auto all = fixture({3,price.id},{120}); auto s = all.snapshot(); s.state.players[0].mp = 0;
        check(all.restore(s), "restore arbitrary physical product at0MP"); draws(all); check(all.step(sell()).valid, "every supported physical hand card can be sold");
        const auto result = all.step(accept()); check(result.sale_resolved && result.sale_price == price.price && result.sale_delivered
            && result.sale_item == price.id && all.state().players[1].hand[1] == price.id && all.state().players[1].hand_count == 2
            && all.state().players[0].hand_count == 1 && all.state().players[0].mp == 0 && all.snapshot().draw_index == 1
            && no_retained(all.state().players[0]) && no_retained(all.state().players[1]),
            "selling any physical product never casts, learns, triggers passives, or earns a product gift"); roundtrip(all);
        magic_products += std::find(hp_absorption_retained_type_ids.begin(),hp_absorption_retained_type_ids.end(),price.id) != hp_absorption_retained_type_ids.end();
    }
    check(magic_products == 22, "all22 physical MAGIC prices and no-acquisition behavior covered");
    auto old_cfg = config; old_cfg.selling = false; Env old(old_cfg); const auto before = old.snapshot(); const CardId ids[] = {3};
    check(!old.script_draws(ids) && old.snapshot() == before, "old30 profiles do not admit SELL3");
}

void self_payment_order_caps_and_passive_delivery() {
    auto self = fixture({3,44},{120}); auto s = self.snapshot(); s.state.players[0].money = 3; s.state.players[0].mp = 4;
    check(self.restore(s), "restore self selling debt across three resources"); draws(self); const auto r = self.step(sell(0));
    check(r.sale_resolved && r.sale_delivered && r.sale_source == 0 && r.sale_target == 0 && r.sale_money_lost == 3
        && r.sale_mp_lost == 4 && r.sale_hp_lost == 3 && r.sale_money_gained == 10
        && self.state().players[0].money == 10 && self.state().players[0].mp == 0 && self.state().players[0].hp == 37
        && self.state().players[0].hand[0] == 44 && self.state().players[0].incoming_sale == 0 && self.snapshot().draw_index == 1,
        "self SELL charges before receiving and crediting: YEN3/MP4/HP40 becomes10/0/37"); roundtrip(self);
    auto capped = fixture({3,44},{120}); s = capped.snapshot(); s.state.players[0].money = 98; check(capped.restore(s), "restore capped seller wallet");
    capped.step(sell()); const auto gain = capped.step(accept()); check(gain.sale_money_gained == 1 && gain.sale_money_lost == 10
        && capped.state().players[0].money == 99, "source credit uses actual cap delta, independent of full target charge"); roundtrip(capped);
    for (bool same_seat : {false,true}) {
        auto revived = fixture({3,244},{120}); s = revived.snapshot(); auto& target = s.state.players[same_seat ? 0 : 1];
        target.hp = 5; target.money = target.mp = 0; check(revived.restore(s), "restore delivered244 after lethal price payment"); draws(revived);
        StepResult result = revived.step(sell(same_seat ? 0 : 1)); if (!same_seat) result = revived.step(accept());
        const int seat = same_seat ? 0 : 1; check(result.sale_hp_lost == 5 && result.sale_delivered && result.revivals[seat] == 1
            && revived.state().players[seat].hp == 10 && !revived.state().players[seat].dead && revived.snapshot().draw_index == 1
            && revived.state().players[seat].incoming_sale == 0,
            "commodity244 arrives after price death but before death hook and revives without a product gift"); roundtrip(revived);
    }
    auto mortar = fixture({3,245},{120}); draws(mortar); mortar.step(sell()); const auto free = mortar.step(accept());
    check(free.sale_price == 0 && !free.sale_money_lost && !free.sale_mp_lost && !free.sale_hp_lost && !free.sale_money_gained
        && free.sale_delivered && mortar.state().players[1].hand[1] == 245 && mortar.state().players[0].hand[0] == 120,
        "non-discardable zero-price245 can be sold and physically moves without resource effects"); roundtrip(mortar);
}

void full_target_refusal_and_untyped_reactions() {
    auto full = fixture({3,56},{120}); auto s = full.snapshot(); s.state.players[1].hand.fill(120); s.state.players[1].hand_count = 16;
    check(full.restore(s), "restore full target hand"); draws(full); full.step(sell()); const auto lost = full.step(accept());
    check(lost.sale_resolved && !lost.sale_delivered && lost.sale_money_lost == 20 && lost.sale_mp_lost == 10
        && lost.sale_money_gained == 30 && full.state().players[1].hand_count == 16 && full.state().players[1].incoming_sale == 0
        && std::all_of(full.state().players[1].hand.begin(),full.state().players[1].hand.end(),[](auto id){return id == 120;})
        && full.state().players[0].hand_count == 1 && full.snapshot().draw_index == 1,
        "full target refuses direct delivery but price and source credit still apply with no rollback"); roundtrip(full);
    auto reflected = fixture({3,56},{120,195,39,20,40,210,215,185,196,239}); s = reflected.snapshot();
    s.state.players[0].money = s.state.players[0].mp = 0; s.state.players[1].money = 98;
    check(reflected.restore(s), "restore reflected transaction with asymmetric wallets"); draws(reflected); reflected.step(sell());
    check(reflected.options().reaction_slots == (1u<<8) && !reflected.options().defense_slots && !reflected.options().removal_slots
        && !reflected.options().counter_slots && !reflected.options().defensive_ability_slots, "None trade exposes only196 reaction");
    for (int slot = 0; slot < 8; ++slot) reject(reflected,special(slot));
    reject(reflected,{ActionKind::defend,no_slot,1,no_slot}); reject(reflected,special(8,1)); reject(reflected,special(8,2)); reject(reflected,ordered({0,8}));
    check(reflected.legal(accept()), "empty acceptance remains legal for forced sale"); roundtrip(reflected);
}

void reflected_payment_and_receipt_terminal_credit() {
    for (bool same_seat : {false,true}) {
        Config minimum; minimum.selling = true; auto small = fixture({3,44},{120},minimum); auto snapshot = small.snapshot();
        const int seat = same_seat ? 0 : 1; snapshot.state.players[seat].hp = 1;
        snapshot.state.players[seat].money = snapshot.state.players[seat].mp = 0;
        check(small.restore(snapshot), "restore minimum nontracking few-card terminal recipient"); draws(small);
        auto result = small.step(sell(static_cast<std::uint8_t>(seat))); if (!same_seat) result = small.step(accept());
        check(result.sale_delivered && result.sale_hp_lost == 1 && small.done() && small.state().players[seat].hp == 0
            && !small.state().players[seat].dead && small.state().players[seat].incoming_sale == 1
            && small.state().players[seat].hand_count <= 2 && small.state().last_sale.turn == small.state().turns,
            "successful delivery to eventual dead recipient retains credit even without old dead tracking or capacity pressure"); roundtrip(small);
        const auto exact = small.snapshot(); auto missing = exact; missing.state.players[seat].incoming_sale = 0;
        check(!small.restore(missing) && small.snapshot() == exact,
            "few-card terminal delivery credit cannot be erased even when generic hand capacity would still fit");
    }
    auto env = fixture({3,56},{196,239}); auto s = env.snapshot(); s.state.players[0].money = s.state.players[0].mp = 0; s.state.players[1].money = 98;
    check(env.restore(s), "restore current-source reflected income"); draws(env); env.step(sell());
    const auto redirect = env.step(ordered({0,1})); check(redirect.any_reflected && redirect.magic_free_suffix && !redirect.magic_free_cards
        && env.state().pending.sale_item == 56 && env.state().pending.amount == 30 && env.state().pending.source == 1 && env.state().pending.target == 0,
        "196 keeps immutable original product/price and leaves FREE suffix unconsumed"); roundtrip(env);
    const auto chain = env.snapshot();
    for (int error = 0; error < 5; ++error) {
        auto bad = chain;
        if (error == 0) ++bad.state.players[0].pending_deals;
        if (error == 1) ++bad.state.players[1].pending_deals;
        if (error == 2) bad.state.players[0].retained_deals = 1;
        if (error == 3) bad.state.players[1].retained_deals = 1;
        if (error == 4) { bad.state.players[0].hand.fill(120); bad.state.players[0].hand[15] = 0; bad.state.players[0].hand_count = 15; }
        check(!env.restore(bad) && env.snapshot() == chain, "active sale chain has exact physical-only gift parity and original owner's nongift product gap");
    }
    const auto r = env.step(accept()); check(r.sale_source == 1 && r.sale_target == 0 && r.sale_item == 56 && r.sale_hp_lost == 30
        && r.sale_money_gained == 1 && env.state().players[0].hp == 10 && env.state().players[0].hand[0] == 56
        && env.state().players[1].money == 99 && env.state().players[1].hand[0] == 239 && env.snapshot().draw_index == 2,
        "reflected SELL charges original owner, returns product there, and credits current reflector"); roundtrip(env);
    auto glory = fixture({3,44},{196,239}); s = glory.snapshot(); s.state.players[1].glory = true;
    check(glory.restore(s), "restore Glory trader"); glory.step(sell()); reject(glory,ordered({0,1})); check(glory.step(special(0)).any_reflected, "Glory counts unconsumed FREE suffix but permits lone196");
    auto terminal = fixture({3,56,196},{196}); s = terminal.snapshot(); auto& recipient = s.state.players[1];
    recipient.hand.fill(120); recipient.hand[0] = 196; recipient.hand_count = 16; recipient.hp = 1; recipient.mp = recipient.money = 0;
    check(terminal.restore(s), "restore16-card future terminal recipient with pending reflector gift"); draws(terminal);
    terminal.step(sell()); terminal.step(special(0)); terminal.step(special(0)); roundtrip(terminal);
    check(terminal.state().players[1].hand_count == 15 && terminal.state().players[1].pending_deals == 1
        && terminal.state().pending.target == 1, "two reflections return trade to initial target with one earned replacement still owed");
    const auto delivered = terminal.step(accept()); check(delivered.sale_delivered && delivered.sale_hp_lost == 1 && terminal.done()
        && terminal.state().players[1].hand_count == 16 && terminal.state().players[1].hand[15] == 56
        && terminal.state().players[1].incoming_sale == 1 && terminal.state().players[1].pending_deals == 1
        && terminal.state().players[1].dead && terminal.state().last_sale.turn == terminal.state().turns
        && terminal.snapshot().draw_index == 2, "dead terminal recipient keeps distinct receipt credit without conflating physical spent or losing its earned gift"); roundtrip(terminal);
    const auto exact = terminal.snapshot();
    for (int error = 0; error < 6; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.players[1].incoming_sale = 0;
        if (error == 1) bad.state.players[1].incoming_sale = 2;
        if (error == 2) bad.state.last_sale.delivered = false;
        if (error == 3) bad.state.last_sale.target = 0;
        if (error == 4) --bad.state.last_sale.turn;
        if (error == 5) bad.state.players[0].incoming_sale = 1;
        check(!terminal.restore(bad) && terminal.snapshot() == exact, "invalid receipt credit, target, delivery, or completed-turn nonce rejects atomically");
    }
}

void canonicality_history_privacy_and_replay() {
    auto env = fixture({3,44,245},{120}); reject(env,{ActionKind::use,0,0,1});
    auto a = sell(); a.slots = 0; reject(env,a); a = sell(); a.slots = 1; reject(env,a); a = sell(); a.slots = 6; reject(env,a);
    a = sell(); a.base = 1; reject(env,a); a = sell(); a.target = no_slot; reject(env,a);
    a = sell(); a.counter_count = 1; reject(env,a); a = sell(); a.extra_count = 1; reject(env,a); a = sell(); a.extras[64] = 1; reject(env,a);
    auto pending = fixture({3,44},{120}); pending.step(sell()); const auto exact = pending.snapshot();
    for (int error = 0; error < 8; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.pending.sale_card = 0;
        if (error == 1) bad.state.pending.sale_item = 4;
        if (error == 2) bad.state.pending.amount = 9;
        if (error == 3) bad.state.pending.element = Element::neutral;
        if (error == 4) bad.state.pending.miracle = 221;
        if (error == 5) bad.state.pending.counter_card = 190;
        if (error == 6) bad.state.players[0].pending_deals = 2;
        if (error == 7) bad.state.players[1].incoming_sale = 1;
        check(!pending.restore(bad) && pending.snapshot() == exact, "trade price/type/gift and premature delivery metadata reject atomically");
    }
    pending.step(accept()); const auto history = pending.snapshot();
    for (int error = 0; error < 7; ++error) {
        auto bad = history;
        if (error == 0) bad.state.last_sale.id = 4;
        if (error == 1) bad.state.last_sale.source = no_slot;
        if (error == 2) bad.state.last_sale.target = no_slot;
        if (error == 3) bad.state.last_sale.price = 9;
        if (error == 4) bad.state.last_sale.money_gained = 11;
        if (error == 5) bad.state.last_sale.turn = bad.state.turns+1;
        if (error == 6) bad.state.last_sale.item = 4;
        check(!pending.restore(bad) && pending.snapshot() == history, "invalid historical product/price/deltas/turn rejects atomically");
    }
    auto changed = history; changed.state.players[0].money = 1; changed.state.players[1].hp = 20;
    check(pending.restore(changed), "historical sale deltas do not constrain later current resources"); roundtrip(pending);
    auto hidden_a = fixture({3,44},{245}), hidden_b = fixture({3,44},{120});
    check(hidden_a.observe() == hidden_b.observe() && hidden_a.options() == hidden_b.options() && hidden_a.legal(sell()) && hidden_b.legal(sell()),
        "unknown opponent product contents cannot change forced-selling observation or legal choices");
    auto own = fixture({3,245},{120}); check(own.observe()[1175] == 1.f && own.observe()[1176] == 1.f
        && own.observe()[1192] == 5.f/max_sale_price && own.observe()[1193] == 0.f, "own seller and per-hand price features include legitimate zero price");
    auto old_cfg = config; old_cfg.selling = false; auto old = fixture({120},{120},old_cfg), extended = fixture({120},{120});
    const auto previous = old.observe(), current = extended.observe(); check(std::equal(previous.begin(),previous.begin()+1175,current.begin())
        && std::all_of(previous.begin()+1175,previous.end(),[](float x){return x == 0.f;}), "old30 feature prefix1175 remains identical and46feature suffix zero");
    std::uint64_t sales = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env x(config), y(config); x.reset(seed); y.reset(seed); std::uint64_t px = seed+310000, py = px;
        while (!x.done()) {
            check(x.snapshot() == y.snapshot() && x.observe() == y.observe(), "selling seeded state/observation replay");
            for (float value : x.observe()) check(std::isfinite(value), "sale/debt/receipt observations finite");
            const auto ax = x.sample_action(px), ay = y.sample_action(py); check(ax == ay && px == py && x.legal(ax), "selling base/product sampler deterministic/legal");
            const auto rx = x.step(ax), ry = y.step(ay); check(rx.valid && ry.valid && rx.rewards == ry.rewards && rx.sale_used == ry.sale_used
                && rx.sale_resolved == ry.sale_resolved && rx.sale_delivered == ry.sale_delivered && rx.sale_item == ry.sale_item
                && rx.sale_money_lost == ry.sale_money_lost && rx.sale_mp_lost == ry.sale_mp_lost && rx.sale_hp_lost == ry.sale_hp_lost,
                "source credit, recipient payment, delivery and receipt replay exactly"); sales += rx.sale_resolved; roundtrip(x);
        }
    }
    check(sales, "natural pool and policy exercise SELL3");
}
}
int main() {
    metadata_minimum_and_all_physical_products(); self_payment_order_caps_and_passive_delivery();
    full_target_refusal_and_untyped_reactions(); reflected_payment_and_receipt_terminal_credit(); canonicality_history_privacy_and_replay();
    std::cout << "Selling passed (all216 products/MC prices/debt order/reflection/full target/terminal receipt/privacy,1000 replay rollouts).\n";
}
