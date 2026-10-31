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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = c.revival = c.item_removal = c.hp_absorption = c.self_damage = c.pestle_mortar = c.selling = c.buying = c.hp_gamble = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL HP gamble: " << message << '\n'; std::exit(1); } }
Action use(std::uint8_t target = 1) { return {ActionKind::use,0,0,target}; }
Action accept() { return {ActionKind::defend,no_slot,0,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action ordered(std::initializer_list<std::uint8_t> refs) {
    Action a{ActionKind::defend_ordered,no_slot,0,no_slot}; a.extra_count = static_cast<std::uint8_t>(refs.size()); std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0; p.incoming_sale = p.incoming_buy = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy);
    check(env.restore(s), "restore explicit HP-gamble fixture"); return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "signed gamble/event/reflect/tape roundtrip exactly"); }
void reject(Env& env, Action a) { const auto before = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == before, "illegal gamble/type/prefix action rejects atomically"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "isolate original gamble gift"); }
void roll(Env& env, std::uint8_t bit) { const std::uint8_t bits[] = {bit}; check(env.script_hp_gamble_rolls(bits), "script one binary gamble draw"); }

void metadata_minimum_signs_caps_and_self() {
    const auto* card = find_hp_gamble_card(237);
    check(hp_gamble_card_count == 1 && hp_gamble_catalog().size() == 1 && hp_gamble_index(237) == 0 && card && card->amount == 10
        && card->card.kind == Kind::sundry && !card->card.attack && !card->card.boost && !card->card.defense && !card->card.hp
        && !card->card.mp && card->card.price == 1 && card->card.weight == 2 && card->card.discardable,
        "237 is a one-draw SUNDRY gamble, not an ordinary heal or damage weapon");
    for (CardId id : {CardId{0},CardId{236},CardId{238},CardId{244},CardId{65535}}) check(!find_hp_gamble_card(id), "strict237-only scope");
    Config minimum; minimum.hp_gamble = true;
    for (std::uint8_t bit = 0; bit < 2; ++bit) for (std::uint8_t target = 0; target < 2; ++target) {
        auto env = fixture({237},{120},minimum); auto s = env.snapshot(); s.state.players[0].mp = 0; check(env.restore(s), "restore independent0MP gamble"); draws(env); roll(env,bit);
        const auto rng = env.snapshot().rng_state;
        check(std::string_view(env.profile()) == hp_gamble_ruleset && env.snapshot().version == 36 && env.options().hp_gamble_slots == 1
            && env.options().utility_slots == 1 && env.options().discard_slots == 1 && env.options().pray, "minimum profile exposes use/discard without prayer or MP interference");
        const auto used = env.step(use(target)); const int delta = bit ? 10 : -10;
        check(used.hp_gamble_used && used.hp_gamble_card == 237 && used.hp_gamble_delta == delta && env.snapshot().hp_gamble_index == 1
            && env.snapshot().rng_state == rng && env.state().last_hp_gamble.source == 0 && env.state().last_hp_gamble.target == target,
            "one launch draw0=-10/1=+10 becomes a public immutable decision");
        if (target == 0) check(used.hp_gamble_resolved && used.hp_gamble_change == delta && env.state().players[0].hp == 40+delta
            && env.snapshot().draw_index == 1 && env.state().last_hp_gamble.turn == env.state().turns, "self target resolves in one action then pays one original gift");
        else {
            check(!used.hp_gamble_resolved && env.state().pending.kind == Effect::hp_gamble && env.state().pending.hp_gamble_card == 237
                && env.state().pending.amount == delta && env.state().pending.element == Element::none && !env.state().last_hp_gamble.resolved
                && env.state().last_hp_gamble.turn == env.state().turns+1 && env.observe()[1269] == (bit ? 1.f : -1.f),
                "other target waits for response with signed public None payload, not future hidden outcome"); roundtrip(env);
            const auto resolved = env.step(accept()); check(resolved.hp_gamble_resolved && resolved.hp_gamble_delta == delta && resolved.hp_gamble_change == delta
                && env.state().players[1].hp == 40+delta && env.snapshot().draw_index == 1, "response applies the same selected sign without a second draw");
        }
        check(env.state().players[0].mp == 0 && env.state().players[0].money == 20, "gamble has no MP or wallet effect"); roundtrip(env);
    }
    for (int hp : {90,95,99}) {
        auto capped = fixture({237},{120},minimum); auto s = capped.snapshot(); s.state.players[1].hp = hp; check(capped.restore(s), "restore positive cap boundary"); roll(capped,1);
        capped.step(use()); const auto r = capped.step(accept()); check(r.hp_gamble_change == std::min(10,99-hp) && capped.state().players[1].hp == std::min(99,hp+10),
            "positive result clips actual healing at99, including zero change at cap"); roundtrip(capped);
    }
    auto lethal = fixture({237},{120},minimum); auto s = lethal.snapshot(); s.state.players[1].hp = 5; check(lethal.restore(s), "restore negative overkill boundary"); roll(lethal,0);
    lethal.step(use()); const auto death = lethal.step(accept()); check(death.hp_gamble_change == -5 && death.terminated && lethal.state().winner == 0
        && !lethal.state().players[1].dead, "negative uses normal clipped HP loss and independent config does not enable unrelated dead tracking"); roundtrip(lethal);
    auto discarded = fixture({237},{120},minimum); roll(discarded,1); check(discarded.step({ActionKind::discard,no_slot,1,no_slot}).valid
        && !discarded.snapshot().hp_gamble_index && !discarded.state().last_hp_gamble.id, "discard neither rolls nor earns a gift"); roundtrip(discarded);
}

void reflection_none_free_glory_and_no_reroll() {
    for (std::uint8_t bit = 0; bit < 2; ++bit) {
        auto env = fixture({237},{120,195,39,20,40,210,215,185,196,239}); draws(env); roll(env,bit); env.step(use());
        check(!env.options().defense_slots && !env.options().removal_slots && !env.options().counter_slots
            && env.options().reaction_slots == (1u<<8), "None response offers only unconditional196");
        for (int slot = 0; slot < 8; ++slot) reject(env,special(slot));
        reject(env,{ActionKind::defend,no_slot,1,no_slot}); reject(env,special(8,1)); reject(env,special(8,2)); reject(env,ordered({0,8}));
        const auto redirected = env.step(ordered({8,9})); check(redirected.any_reflected && redirected.magic_free_suffix && !redirected.magic_free_cards
            && env.state().pending.hp_gamble_card == 237 && env.state().pending.amount == (bit ? 10 : -10)
            && env.state().pending.source == 1 && env.state().pending.target == 0 && env.state().last_hp_gamble.source == 0
            && env.state().last_hp_gamble.target == 1 && env.snapshot().hp_gamble_index == 1, "reflection keeps initial record, selected sign and unused suffix"); roundtrip(env);
        const auto result = env.step(accept()); check(result.hp_gamble_source == 1 && result.hp_gamble_target == 0
            && result.hp_gamble_change == (bit ? 10 : -10) && env.state().last_hp_gamble.resolved_source == 1
            && env.state().last_hp_gamble.resolved_target == 0 && env.state().last_hp_gamble.source == 0 && env.state().last_hp_gamble.target == 1
            && env.snapshot().hp_gamble_index == 1, "resolution records current reflector endpoints without changing launch history or rolling again"); roundtrip(env);
    }
    auto chain = fixture({237,196},{196}); roll(chain,0); chain.step(use()); chain.step(special(0)); chain.step(special(0)); roundtrip(chain);
    check(chain.step(accept()).hp_gamble_target == 1 && chain.snapshot().hp_gamble_index == 1, "two reflections return original target while preserving one draw"); roundtrip(chain);
    auto glory = fixture({237},{196,239}); auto s = glory.snapshot(); s.state.players[1].glory = true; check(glory.restore(s), "restore Glory responder"); roll(glory,1);
    glory.step(use()); reject(glory,ordered({0,1})); check(glory.step(special(0)).any_reflected, "Glory counts unused suffix and permits lone196");
    auto invalid = fixture({237,239},{120}); reject(invalid,{ActionKind::attack,0,0,1});
    Action free{ActionKind::cast_ordered,0,0,1}; free.extra_count = 1; free.extras[0] = 1; reject(invalid,free);
    auto added = fixture({5,237},{120}); Action ordered_attack{ActionKind::attack_ordered,0,0,1}; ordered_attack.extra_count = 1; ordered_attack.extras[0] = 1; reject(added,ordered_attack);
}

void revival_owner_tick_and_decision_boundaries() {
    auto revived = fixture({237,244},{120}); auto s = revived.snapshot(); s.state.players[0].hp = 5; s.state.players[0].disease = Disease::hell;
    s.state.players[0].glory = s.state.players[0].dark_cloud = true; check(revived.restore(s), "restore self price-free gamble death before owner tick"); draws(revived); roll(revived,0);
    const std::uint8_t disease[] = {99}; check(revived.script_disease_rolls(disease), "avoid Hell worsening"); const auto r = revived.step(use(0));
    check(r.hp_gamble_change == -5 && r.revivals[0] == 1 && r.disease_tick && r.disease_damage == 5 && revived.state().players[0].hp == 5
        && revived.state().players[0].disease == Disease::hell && revived.state().players[0].glory && revived.state().players[0].dark_cloud
        && revived.snapshot().draw_index == 1 && revived.snapshot().disease_index == 1,
        "negative effect clips before244 revival, then owner tick runs once and leaves status/harms intact"); roundtrip(revived);
    auto other = fixture({237},{244}); s = other.snapshot(); s.state.players[1].hp = 3; check(other.restore(s), "restore opponent passive boundary"); roll(other,0);
    other.step(use()); const auto saved = other.step(accept()); check(saved.hp_gamble_change == -3 && saved.revivals[1] == 1
        && other.state().players[1].hp == 10 && !other.state().players[1].hand_count, "other237 death consumes244 without a replacement gift"); roundtrip(other);
    auto heaven = fixture({237,244},{120}); s = heaven.snapshot(); s.state.players[0].hp = 95;
    s.state.players[0].disease = Disease::heaven; s.state.players[0].worse_chance = 100; check(heaven.restore(s), "restore positive cap then Heaven fall"); draws(heaven); roll(heaven,1);
    const std::uint8_t fall[] = {0}; check(heaven.script_disease_rolls(fall), "force Heaven worsening"); const auto h = heaven.step(use(0));
    check(h.hp_gamble_change == 4 && h.disease_tick && h.revivals[0] == 1 && heaven.state().players[0].hp == 10
        && heaven.state().last_hp_gamble.change == 4 && heaven.state().players[0].disease == Disease::heaven,
        "historical healing is pre-tick cap delta, independent of later Heaven death and revival"); roundtrip(heaven);
    Config short_cfg; short_cfg.hp_gamble = true; short_cfg.max_decisions = 1;
    auto pending = fixture({237},{120},short_cfg); roll(pending,0); pending.step(use()); check(pending.done() && pending.state().truncated
        && !pending.state().terminated && pending.state().phase == Phase::response && !pending.state().last_hp_gamble.resolved,
        "last decision may retain an unresolved public signed result"); roundtrip(pending);
    auto final_self = fixture({237},{120},short_cfg); roll(final_self,1); const auto last = final_self.step(use(0));
    check(last.hp_gamble_resolved && last.truncated && !last.terminated && final_self.state().turns == 1, "self resolution finishes once before common truncation tail"); roundtrip(final_self);
    auto terminal = fixture({237},{120},short_cfg); s = terminal.snapshot(); s.state.players[0].hp = 1; check(terminal.restore(s), "restore last-decision terminal self gamble"); roll(terminal,0);
    const auto end = terminal.step(use(0)); check(end.terminated && !end.truncated && terminal.state().winner == 1, "terminal death has priority over decision truncation"); roundtrip(terminal);
}

void market_price_and_rng_prefixes() {
    check(sale_price(237) == -1 && sale_price_count == 216, "old static market table remains frozen");
    auto sale = fixture({3,237},{120}); roll(sale,1); draws(sale); sale.step({ActionKind::sell,0,2,1}); const auto sold = sale.step(accept());
    check(sold.sale_price == 1 && sold.sale_delivered && sale.state().players[1].hand[1] == 237 && !sale.snapshot().hp_gamble_index
        && !sale.state().last_hp_gamble.id, "selling237 uses new price fallback and never rolls/casts commodity"); roundtrip(sale);
    auto bought = fixture({4},{237}); roll(bought,0); draws(bought); const std::uint64_t select[] = {0}; check(bought.script_buy_tickets(select), "select237 purchase");
    bought.step({ActionKind::buy,0,0,1}); check(bought.step(accept()).buy_price == 1, "BUY offer price supports237 fallback"); const auto purchase = bought.step({ActionKind::buy_accept});
    check(purchase.buy_delivered && bought.state().players[0].hand[0] == 237 && !bought.snapshot().hp_gamble_index && !bought.state().last_hp_gamble.id,
        "purchasing237 is physical transfer and does not consume gamble tape"); roundtrip(bought);
    auto random = fixture({237},{120}); const auto rng = random.snapshot().rng_state; random.step(use());
    check(random.snapshot().rng_state != rng && (random.state().pending.amount == -10 || random.state().pending.amount == 10), "unscripted launch consumes default binary RNG exactly at use"); roundtrip(random);
    auto exhausted = fixture({237},{237}); draws(exhausted); roll(exhausted,1); exhausted.step(use()); exhausted.step(accept());
    Env control(config); check(control.restore(exhausted.snapshot()) && control.script_hp_gamble_rolls({}), "clear exhausted prefix in control");
    const auto a = exhausted.step(use(0)), b = control.step(use(0)); check(a.hp_gamble_delta == b.hp_gamble_delta && exhausted.state() == control.state()
        && exhausted.snapshot().rng_state == control.snapshot().rng_state && exhausted.snapshot().hp_gamble_index == 1,
        "exhausted binary prefix resumes regular RNG without repeating earlier outcome"); roundtrip(exhausted);
    auto old_cfg = config; old_cfg.hp_gamble = false; Env old(old_cfg); const auto before = old.snapshot(); const CardId id[] = {237}; const std::uint8_t bit[] = {1};
    check(!old.script_draws(id) && !old.script_hp_gamble_rolls(bit) && old.snapshot() == before && old.script_hp_gamble_rolls({}),
        "disabled profile excludes237/nonempty tape and permits clear");
}

void privacy_strict_restore_and_replay() {
    auto a = fixture({237},{120}), b = fixture({237},{120}); roll(a,0); roll(b,1);
    check(a.observe() == b.observe() && a.options() == b.options() && a.legal(use()) && b.legal(use()), "future binary result is private before use"); a.step(use()); b.step(use());
    check(a.observe() != b.observe() && a.observe()[1269] == -1.f && b.observe()[1269] == 1.f,
        "launch exposes sign before response, matching public decidedHP command");
    const auto exact = a.snapshot();
    for (int error = 0; error < 12; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.pending.amount = 0;
        if (error == 1) bad.state.pending.amount = -9;
        if (error == 2) bad.state.pending.hp_gamble_card = 236;
        if (error == 3) bad.state.pending.element = Element::neutral;
        if (error == 4) bad.state.pending.miracle = 216;
        if (error == 5) bad.state.last_hp_gamble.delta = 10;
        if (error == 6) bad.state.last_hp_gamble.resolved = true;
        if (error == 7) bad.state.last_hp_gamble.turn = 0;
        if (error == 8) ++bad.state.players[0].pending_deals;
        if (error == 9) bad.state.players[0].retained_deals = 1;
        if (error == 10) bad.state.last_hp_gamble.target = 0;
        if (error == 11) { bad.state.players[1].hp = 0; bad.state.players[1].disease = Disease::heaven; }
        check(!a.restore(bad) && a.snapshot() == exact, "bad signed payload, live event, parity, or impossible provisional target rejects atomically");
    }
    const std::uint8_t invalid[] = {2}; std::array<std::uint8_t,65> oversized{};
    check(!a.script_hp_gamble_rolls(invalid) && !a.script_hp_gamble_rolls(oversized) && a.snapshot() == exact, "invalid bit/oversize tape reject atomically");
    auto bad_tail = exact; bad_tail.scripted_hp_gambles[bad_tail.hp_gamble_count] = 1; check(!a.restore(bad_tail) && a.snapshot() == exact, "unused private tape tail must be zero");
    a.step(accept()); const auto history = a.snapshot();
    for (int error = 0; error < 8; ++error) {
        auto bad = history;
        if (error == 0) bad.state.last_hp_gamble.id = 236;
        if (error == 1) bad.state.last_hp_gamble.change = 0;
        if (error == 2) bad.state.last_hp_gamble.change = 1;
        if (error == 3) bad.state.last_hp_gamble.change = -11;
        if (error == 4) bad.state.last_hp_gamble.resolved_source = no_slot;
        if (error == 5) bad.state.last_hp_gamble.resolved_target = bad.state.last_hp_gamble.resolved_source;
        if (error == 6) bad.state.last_hp_gamble.turn = bad.state.turns+1;
        if (error == 7) bad.state.last_hp_gamble.resolved = false;
        check(!a.restore(bad) && a.snapshot() == history, "impossible historical sign/actual endpoints/turn status rejects atomically");
    }
    auto self = fixture({237},{120}); roll(self,1); self.step(use(0)); const auto self_history = self.snapshot(); auto shifted = self_history;
    shifted.state.last_hp_gamble.resolved_source = shifted.state.last_hp_gamble.resolved_target = 1;
    check(!self.restore(shifted) && self.snapshot() == self_history, "initial self event has no reaction path to a different resolved seat");
    auto old_cfg = config; old_cfg.hp_gamble = false; auto old = fixture({120},{120},old_cfg), current = fixture({120},{120});
    const auto old_obs = old.observe(), new_obs = current.observe(); check(std::equal(old_obs.begin(),old_obs.begin()+1251,new_obs.begin())
        && std::all_of(old_obs.begin()+1251,old_obs.end(),[](float x){return x == 0.f;}), "old32 prefix1251 and zero27 added feature tail preserved");
    auto own = fixture({237},{120}); check(own.observe()[1251] == 1.f && own.observe()[1252] == 1.f, "new flag and own237 offsets fixed");
    std::uint64_t uses = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env x(config), y(config); x.reset(seed); y.reset(seed); std::uint64_t px = seed+330000, py = px;
        while (!x.done()) {
            check(x.snapshot() == y.snapshot() && x.observe() == y.observe(), "signed gamble seeded state/observation replay");
            for (float value : x.observe()) check(std::isfinite(value), "signed observations remain finite");
            const auto ax = x.sample_action(px), ay = y.sample_action(py); check(ax == ay && px == py && x.legal(ax), "gamble-enabled sampler deterministic/legal");
            const auto rx = x.step(ax), ry = y.step(ay); check(rx.valid && ry.valid && rx.rewards == ry.rewards
                && rx.hp_gamble_used == ry.hp_gamble_used && rx.hp_gamble_resolved == ry.hp_gamble_resolved
                && rx.hp_gamble_delta == ry.hp_gamble_delta && rx.hp_gamble_change == ry.hp_gamble_change,
                "chosen sign, clipped effect and reward replay exactly"); uses += rx.hp_gamble_used; roundtrip(x);
        }
    }
    check(uses, "natural pool and sampler exercise237");
}
}
int main() {
    metadata_minimum_signs_caps_and_self(); reflection_none_free_glory_and_no_reroll(); revival_owner_tick_and_decision_boundaries();
    market_price_and_rng_prefixes(); privacy_strict_restore_and_replay();
    std::cout << "HP gamble passed (public launch sign/None reflection/clips/revival/tick/trade/tape/history,1000 replay rollouts).\n";
}
