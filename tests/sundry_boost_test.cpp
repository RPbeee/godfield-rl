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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = c.revival = c.item_removal = c.hp_absorption = c.self_damage = c.pestle_mortar = c.selling = c.buying = c.hp_gamble = c.magic_bounce = c.sundry_boost = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL sundry boost: " << message << '\n'; std::exit(1); } }
Action attack(std::uint16_t slots = 0, std::uint8_t target = 1) { return {ActionKind::attack,0,slots,target}; }
Action accept(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action ordered(std::initializer_list<std::uint8_t> refs, std::uint8_t target = 1) {
    Action a{ActionKind::attack_ordered,0,0,target}; a.extra_count = static_cast<std::uint8_t>(refs.size()); std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0; p.incoming_sale = p.incoming_buy = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy);
    check(env.restore(s), "restore explicit SUNDRY238 fixture"); return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "ordinary/ordered boost provenance and budgets roundtrip exactly"); }
void reject(Env& env, Action a) { const auto before = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == before, "unsupported238 role/reference rejects atomically"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "isolate physical boost gift counts"); }

void metadata_minimum_base_prayer_and_discard() {
    const auto* card = find_sundry_boost_card(238);
    check(sundry_boost_card_count == 1 && sundry_boost_catalog().size() == 1 && sundry_boost_index(238) == 0 && card
        && card->card.kind == Kind::sundry && !card->card.attack && card->card.boost == 10 && !card->card.defense
        && !card->card.hp && !card->card.mp && card->card.price == 15 && card->card.weight == 2 && card->card.discardable,
        "strict238-only SUNDRY pure boost10 metadata");
    for (CardId id : {CardId{0},CardId{133},CardId{197},CardId{237},CardId{239},CardId{65535}}) check(!find_sundry_boost_card(id), "no ordinary armor, MAGIC, FREE or gamble inclusion");
    Config minimum; minimum.sundry_boost = true; auto alone = fixture({238},{120},minimum); draws(alone);
    check(std::string_view(alone.profile()) == sundry_boost_ruleset && alone.snapshot().version == 37 && alone.options().sundry_boost_slots == 1
        && alone.options().boost_slots == 1 && alone.options().discard_slots == 1 && !alone.options().weapon_bases
        && !alone.options().utility_slots && alone.options().pray, "independent pure extra does not count as WEAPON or prevent prayer");
    reject(alone,{ActionKind::use,0,0,0}); reject(alone,attack()); reject(alone,ordered({0}));
    const auto prayed = alone.step({ActionKind::pray}); check(prayed.valid && !prayed.sundry_boost_cards_used && alone.snapshot().draw_index == 1
        && alone.state().players[0].hand[0] == 238, "prayer retains booster and earns only its normal gift"); roundtrip(alone);
    auto discard = fixture({238},{120},minimum); draws(discard); check(discard.step({ActionKind::discard,no_slot,1,no_slot}).valid
        && !discard.state().players[0].hand_count && !discard.snapshot().draw_index, "discard is not an attack and earns no replacement"); roundtrip(discard);
    auto base = fixture({5,238},{120},minimum); auto s = base.snapshot(); s.state.players[0].mp = 0; check(base.restore(s), "restore0MP ordinary boost"); draws(base);
    const auto used = base.step(attack(2)); check(used.sundry_boost_cards_used == 1 && base.state().pending.amount == 11
        && base.state().players[0].pending_deals == 2 && base.state().players[0].mp == 0 && !base.state().players[0].hand_count,
        "ordinary guaranteed weapon adds10, consumes one physical copy, pays no MP"); roundtrip(base);
    check(!base.step(accept()).sundry_boost_cards_used && base.state().players[1].hp == 29 && base.snapshot().draw_index == 2,
        "boost counted only at attack, not again at resolution; base and extra each earn one gift"); roundtrip(base);
}

void duplicate15_boundary_and_canonical_defense() {
    Config minimum; minimum.sundry_boost = true; auto env = fixture({5},{120},minimum); auto s = env.snapshot(); auto& owner = s.state.players[0];
    owner.hand.fill(238); owner.hand[0] = 5; owner.hand_count = 16; owner.mp = 0; check(env.restore(s), "restore15 physical booster copies"); draws(env);
    const auto r = env.step(attack(0xfffe)); check(r.sundry_boost_cards_used == 15 && env.state().pending.amount == 151
        && env.state().players[0].pending_deals == 16 && !env.state().players[0].hand_count, "all15 copies independently add10 and consume distinct slots"); roundtrip(env);
    env.step(accept()); check(env.snapshot().draw_index == 16 && env.state().players[0].hand_count == 16 && env.state().winner == 0,
        "16 consumed physical cards yield16 normal gifts after lethal resolution"); roundtrip(env);
    auto defense = fixture({5},{238}); defense.step(attack()); check(!defense.options().sundry_boost_slots && !defense.options().defense_slots
        && !defense.options().reaction_slots, "response membership stays0 and canonical DEF-less238 is excluded");
    reject(defense,accept(1)); reject(defense,special(0)); check(defense.step(accept()).valid, "empty ordinary acceptance remains legal"); roundtrip(defense);
    auto magic = fixture({198},{238,119}); magic.step({ActionKind::use,0,0,1}); reject(magic,special(1,1));
    check(magic.options().magic_bounce_slots == 2 && !magic.options().sundry_boost_slots,
        "canonical pure-booster exclusion also holds in incompatible ordinary prefix before MAGIC special");
}

void mana_paid_reset_free_double_and_wide() {
    for (bool before : {false,true}) {
        auto env = before ? fixture({57,238,197},{120}) : fixture({57,197,238},{120}); const auto r = env.step(ordered({1,2}));
        check(r.sundry_boost_cards_used == 1 && r.additive_mp_spent == 2 && r.mana_weapon_mp_spent == 8
            && env.state().pending.amount == (before ? 18 : 28) && env.state().players[0].mp == 0,
            "paid197 resets57 after payment, so preceding238 is lost and following238 remains"); roundtrip(env);
    }
    auto zero = fixture({57,238},{120}); auto s = zero.snapshot(); s.state.players[0].mp = 0; check(zero.restore(s), "restore empty mana weapon with pure extra");
    check(zero.step(attack(2)).sundry_boost_cards_used == 1 && zero.state().pending.amount == 10 && zero.state().pending.mana_spent == 0,
        "MP0 weapon57 remains valid and receives ordinary boost10"); roundtrip(zero);
    for (bool before : {false,true}) {
        auto env = before ? fixture({56,238,214},{120}) : fixture({56,214,238},{120}); const auto r = env.step(ordered({1,2}));
        check(r.sundry_boost_cards_used == 1 && r.doubling_mp_spent == 6 && env.state().pending.amount == (before ? 80 : 70)
            && env.state().players[0].mp == 4, "ordinary boost position relative to214 determines80 versus70"); roundtrip(env);
    }
    for (bool doubling : {false,true}) {
        auto env = doubling ? fixture({57,238,214,239},{120}) : fixture({57,238,197,239},{120}); const auto r = env.step(ordered({1,2,3}));
        check(r.sundry_boost_cards_used == 1 && r.magic_free_cards == 1 && !r.additive_mp_spent && !r.doubling_mp_spent
            && r.mana_weapon_mp_spent == 10 && env.state().pending.amount == (doubling ? 60 : 32),
            "free MAGIC never resets57:20+10 then free197+2 or free214x2; final remainingMP burns once"); roundtrip(env);
    }
    auto wide = fixture({56,238,209},{39}); const std::uint8_t hit[] = {100}; check(wide.script_hit_rolls(hit), "force100percent WIDE prefix"); draws(wide);
    const auto r = wide.step(ordered({1,2},0)); check(r.sundry_boost_cards_used == 1 && r.wide_attack && wide.state().pending.amount == 40
        && wide.state().pending.target == 1 && !wide.state().pending.miracle && wide.state().players[0].mp == 5,
        "WIDE normalizes enemy while preserving weapon boost and typed provenance"); roundtrip(wide);
    check(wide.step(special(0)).reflected && wide.snapshot().hit_index == 1, "boosted wide weapon reflects without hit reroll"); roundtrip(wide);
}

void setters_types_reactions_counter_and_status() {
    for (bool neutral : {false,true}) {
        auto compressed = fixture({5,65,238},{120}), traced = fixture({5,65,238},{120});
        const auto r = compressed.step({neutral ? ActionKind::attack_set_neutral : ActionKind::attack_set_fire,0,6,1});
        traced.step(neutral ? ordered({1,2}) : ordered({2,1}));
        check(r.sundry_boost_cards_used == 1 && compressed.state().pending.amount == 11 && traced.state().pending.amount == 11
            && compressed.state().pending.element == (neutral ? Element::neutral : Element::fire)
            && traced.state().pending.element == compressed.state().pending.element && compressed.state().pending.attribute_card == 65,
            "new NE boost is a valid setter-neutralization witness and ordered/canonical results agree"); roundtrip(compressed); roundtrip(traced);
    }
    auto percent = fixture({100,238},{120}); reject(percent,attack(2)); reject(percent,ordered({1}));
    auto pestle = fixture({99,238},{245}); reject(pestle,attack(2)); reject(pestle,ordered({1}));
    auto spell = fixture({198,238},{120}); reject(spell,{ActionKind::use,0,2,1}); reject(spell,ordered({1}));
    for (CardId reaction : {CardId{20},CardId{39},CardId{215},CardId{196}}) {
        auto env = fixture({5,238},{reaction}); draws(env); const std::uint8_t seat[] = {0}; check(env.script_bounce_targets(seat), "keep physical bounce on enemy"); env.step(attack(2));
        const auto r = env.step(special(0)); check((r.bounced || r.reflected || r.weapon_blocked) && !r.magic_bounced && !r.sundry_boost_cards_used,
            "ordinary boosted WEAPON supports20/39/215/196 and never becomes typedMAGIC");
        if (!r.weapon_blocked) { check(env.state().pending.amount == 11, "returned weapon keeps full boosted amount"); roundtrip(env); env.step(accept()); }
        roundtrip(env);
    }
    auto counter = fixture({5,238},{190}); draws(counter); counter.step(attack(2)); auto selected = accept(1); selected.counter_count = 1;
    counter.step(selected); check(counter.state().pending.counter_card == 190 && counter.state().pending.amount == 22,
        "counter derives twice boosted raw11 without carrying extra item provenance"); roundtrip(counter); counter.step(accept());
    check(counter.state().players[0].hp == 18 && counter.snapshot().draw_index == 3, "original physical boost plus counter each pay normal gifts once"); roundtrip(counter);
    auto absorb = fixture({26,238},{120}); absorb.step(attack(2)); const auto a = absorb.step(accept()); check(a.hp_absorption_amount == 17
        && absorb.state().players[0].hp == 57 && absorb.state().players[1].hp == 23, "absorption uses boosted residual17"); roundtrip(absorb);
    auto self = fixture({53,238,244},{120}); auto s = self.snapshot(); s.state.players[0].hp = 20; s.state.players[0].disease = Disease::hell;
    check(self.restore(s), "restore boosted self-death revival and owner tick"); draws(self); const std::uint8_t roll[] = {99}; check(self.script_disease_rolls(roll), "ordinary Hell tick");
    const auto paid = self.step(attack(2,0)); check(paid.sundry_boost_cards_used == 1 && paid.self_damage_amount == 24
        && paid.self_damage_source_lost == 20 && !paid.self_damage_target_lost && paid.revivals[0] == 1 && paid.disease_tick
        && self.state().players[0].hp == 5 && self.snapshot().draw_index == 2,
        "boosted self alias clips, revives once, ticks once and preserves two original gifts"); roundtrip(self);
    auto cold = fixture({34,238},{244}); s = cold.snapshot(); s.state.players[1].hp = 5; check(cold.restore(s), "restore boosted Cold weapon before target revival"); draws(cold);
    cold.step(attack(2)); const auto status = cold.step(accept()); check(status.revivals[1] == 1 && cold.state().players[1].hp == 10
        && cold.state().players[1].disease == Disease::cold && cold.snapshot().draw_index == 2, "ordinary boost keeps status payload before HP reduction and244"); roundtrip(cold);
}

void market_strict_refs_privacy_and_replay() {
    check(sale_price_count == 216 && sale_price(238) == -1, "old static price table remains fixed");
    auto sale = fixture({3,238},{120}); draws(sale); sale.step({ActionKind::sell,0,2,1}); const auto sold = sale.step(accept());
    check(sold.sale_price == 15 && sold.sale_delivered && !sold.sundry_boost_cards_used && sale.state().players[1].hand[1] == 238,
        "selling238 uses price15 but does not count as attack use"); roundtrip(sale);
    auto buy = fixture({4},{238}); draws(buy); const std::uint64_t item[] = {0}; check(buy.script_buy_tickets(item), "select238 commodity"); buy.step({ActionKind::buy,0,0,1});
    check(buy.step(accept()).buy_price == 15 && !buy.options().sundry_boost_slots, "purchase price fallback is15 and attack membership is0 in purchase");
    const auto bought = buy.step({ActionKind::buy_accept}); check(bought.buy_delivered && !bought.sundry_boost_cards_used && buy.state().players[0].hand[0] == 238,
        "buying238 delivers without activating an attack effect"); roundtrip(buy);
    auto invalid = fixture({57,238},{120}); reject(invalid,ordered({1,1})); reject(invalid,ordered({20})); reject(invalid,ordered({65}));
    auto malformed = ordered({1}); malformed.extras[64] = 1; reject(invalid,malformed);
    auto trace = fixture({56,238},{120}); trace.step(ordered({1})); const auto exact = trace.snapshot();
    for (int error = 0; error < 4; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.attack_composition.items[0] = 44;
        if (error == 1) bad.state.attack_composition.held_mask = 1;
        if (error == 2) ++bad.state.pending.amount;
        if (error == 3) ++bad.state.attack_composition.initial_mp;
        check(!trace.restore(bad) && trace.snapshot() == exact, "ordered replay rejects modified boost value, held ref, amount or MP atomically");
    }
    auto old_cfg = config; old_cfg.sundry_boost = false; Env disabled(old_cfg); const auto before = disabled.snapshot(); const CardId id[] = {238};
    check(!disabled.script_draws(id) && disabled.snapshot() == before, "old34 pools reject238");
    auto old_trace = exact; old_trace.config = old_cfg; check(!disabled.restore(old_trace) && disabled.snapshot() == before,
        "disabled profile rejects consumed238 in ordered trace even when no such hand card remains");
    auto hidden = fixture({5},{238}); auto swapped = hidden.snapshot(); swapped.state.players[1].hand[0] = 120; const auto visible = hidden.observe();
    check(hidden.restore(swapped) && hidden.observe() == visible && hidden.legal(attack()), "opponent booster identity remains private");
    auto old = fixture({120},{120},old_cfg), current = fixture({120},{120}); const auto old_obs = old.observe(), new_obs = current.observe();
    check(std::equal(old_obs.begin(),old_obs.begin()+1300,new_obs.begin()) && std::all_of(old_obs.begin()+1300,old_obs.end(),[](float x){return x == 0.f;}),
        "old34 prefix1300 fixed and17new feature tail zero"); auto own = fixture({238},{120}); check(own.observe()[1300] == 1.f && own.observe()[1301] == 1.f, "new flag/own238 offsets fixed");
    std::uint64_t used = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env x(config), y(config); x.reset(seed); y.reset(seed); std::uint64_t px = seed+350000, py = px;
        while (!x.done()) {
            check(x.snapshot() == y.snapshot() && x.observe() == y.observe(), "SUNDRY boost seeded state/observation replay");
            for (float value : x.observe()) check(std::isfinite(value), "boost observations finite");
            const auto a = x.sample_action(px), b = y.sample_action(py); check(a == b && px == py && x.legal(a), "pure boost sampler deterministic/legal");
            const auto rx = x.step(a), ry = y.step(b); check(rx.valid && ry.valid && rx.rewards == ry.rewards
                && rx.sundry_boost_cards_used == ry.sundry_boost_cards_used, "physical-copy usage and rewards replay exactly");
            used += rx.sundry_boost_cards_used; roundtrip(x);
        }
    }
    check(used, "natural pool and sampler actually use238");
}
}
int main() {
    metadata_minimum_base_prayer_and_discard(); duplicate15_boundary_and_canonical_defense(); mana_paid_reset_free_double_and_wide();
    setters_types_reactions_counter_and_status(); market_strict_refs_privacy_and_replay();
    std::cout << "SUNDRY boost passed (15copies/57/DOUBLE/FREE/WIDE/setter/types/status/market/privacy,1000 replay rollouts).\n";
}
