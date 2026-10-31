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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = c.revival = c.item_removal = c.hp_absorption = c.self_damage = c.pestle_mortar = c.selling = c.buying = c.hp_gamble = c.magic_bounce = c.sundry_boost = c.status_miracles = c.dying_weapon = true;
    return c;
}();
Config minimum() { Config c; c.elements = c.chance = c.harms = c.celestial = c.dying_weapon = true; return c; }
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL dying weapon: " << message << '\n'; std::exit(1); } }
Action attack(std::uint8_t target = 1, std::uint16_t slots = 0) { return {ActionKind::attack,0,slots,target}; }
Action accept(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action ordered(std::initializer_list<std::uint8_t> refs) {
    Action a{ActionKind::attack_ordered,0,0,1}; a.extra_count = static_cast<std::uint8_t>(refs.size()); std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
Action counter(std::uint16_t slots, int counter_slot) { auto a = accept(slots); a.counter_count = 1; a.counter_order = static_cast<std::uint64_t>(counter_slot); return a; }
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0; p.incoming_sale = p.incoming_buy = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy);
    if (!env.restore(s)) { std::cerr << "Fixture own:"; for (auto id : own) std::cerr << ' ' << id; std::cerr << " enemy:"; for (auto id : enemy) std::cerr << ' ' << id; std::cerr << '\n'; check(false,"restore explicit117 fixture"); }
    return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "ordinary117 chance, death/queue and private state roundtrip"); }
void reject(Env& env, Action a) { const auto before = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == before, "unsupported117 role, extras or type rejects atomically"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "isolate ordinary gifts from retained death items"); }
void roll(Env& env, std::uint8_t value) { const std::uint8_t values[] = {value}; check(env.script_hit_rolls(values), "script117 hit boundary"); }
void bounce_to(Env& env, std::uint8_t seat) { const std::uint8_t values[] = {seat}; check(env.script_bounce_targets(values), "script physical117 bounce"); }
int copies(const Player& p, CardId id) { return static_cast<int>(std::count(p.hand.begin(),p.hand.begin()+p.hand_count,id)); }

void metadata_minimum_hit_miss_and_forced() {
    const auto* c = find_dying_weapon_card(117);
    check(dying_weapon_card_count == 1 && dying_weapon_catalog().size() == 1 && dying_weapon_index(117) == 0 && c
        && c->card.kind == Kind::weapon && c->card.attack == 1 && !c->card.boost && !c->card.defense && !c->card.hp && !c->card.mp
        && c->card.price == 10 && c->card.weight == 1 && !c->card.discardable && c->element == Element::light && c->hit_rate == 75 && c->dying_amount == 30,
        "strict117 ordinary LightATK1/75percent with inactive death30 metadata");
    for (CardId id : {CardId{0},CardId{104},CardId{111},CardId{116},CardId{118},CardId{244},CardId{65535}}) check(!find_dying_weapon_card(id), "no other chance/revival cards included");
    for (bool hit : {false,true}) for (std::uint8_t specified : {std::uint8_t{0},std::uint8_t{1}}) {
        auto env = fixture({117},{120},minimum()); auto s = env.snapshot(); s.state.players[0].mp = 0; check(env.restore(s), "restore zeroMP ordinary117"); draws(env); roll(env,hit ? 75 : 76);
        check(std::string_view(env.profile()) == dying_weapon_ruleset && env.snapshot().version == 37 && env.options().dying_weapon_slots == 1
            && env.options().weapon_bases == 1 && !env.options().pray && !env.options().discard_slots && !env.options().utility_slots,
            "minimal celestial-based profile treats117 only as nondiscardable weapon");
        const auto obs = env.observe(); check(obs[1345] == 1 && obs[1346] == 1 && obs[285] == 5.f/6 && obs[186] == .75f, "own membership, Light and hit-rate observation are public only to holder");
        reject(env,{ActionKind::pray}); reject(env,{ActionKind::use,0,0,0}); reject(env,{ActionKind::discard,no_slot,1,no_slot});
        const auto r = env.step(attack(specified)); check(r.valid && r.dying_weapon_attack && r.chance_attempt && r.chance_hit == hit && !r.chance_forced
            && env.state().players[0].mp == 0 && env.snapshot().hit_index == 1 && env.state().last_chance.id == 117 && env.state().last_chance.source == 0
            && env.state().last_chance.target == 1 && env.state().last_chance.hit == hit && env.state().last_chance.hit_rate == 75,
            "75hits/76misses, specified self normalizes enemy, normal use spends noMP and records one chance event");
        if (hit) { check(env.state().phase == Phase::response && env.state().pending.amount == 1 && env.state().pending.element == Element::light
                && env.state().pending.hit_rate == 75 && env.state().pending.target == 1 && env.state().players[0].pending_deals == 1 && env.observe()[1362] == 1,
                "ordinary active117 never uses death30 and awaits one response"); roundtrip(env); env.step(accept()); }
        check(env.state().players[1].hp == (hit ? 39 : 40) && env.snapshot().draw_index == 1 && env.state().players[0].hand[0] == 120
            && !env.observe()[1362] && env.observe()[1363] == 1 && env.observe()[1364] == hit, "hit/miss each consume117 and earn exactly one normal gift"); roundtrip(env);
    }
    auto forced = fixture({117},{120}); auto s = forced.snapshot(); s.state.players[1].dark_cloud = true; check(forced.restore(s), "restore DarkCloud force hit"); roll(forced,100);
    const auto rng = forced.snapshot().rng_state; const auto r = forced.step(attack(0)); check(r.dying_weapon_attack && r.chance_forced && r.chance_hit && !forced.snapshot().hit_index
        && forced.snapshot().rng_state == rng && forced.state().last_chance.forced && forced.observe()[1365] == 1, "DarkCloud short-circuits RNG and future ticket even for self-specified attack"); roundtrip(forced);
    Config bad; bad.dying_weapon = true; bool threw = false; try { Env invalid(bad); } catch (const std::invalid_argument&) { threw = true; } check(threw,"117 requires celestial profile explicitly");
}

void extras_defense_reactions_and_no_reroll() {
    auto env = fixture({117,11,65,197,214,209,239,238},{120}); roll(env,75);
    for (int slot = 1; slot < 8; ++slot) { reject(env,attack(1,static_cast<std::uint16_t>(1u<<slot))); reject(env,ordered({static_cast<std::uint8_t>(slot)})); }
    reject(env,{ActionKind::attack_set_fire,0,4,1}); reject(env,{ActionKind::attack_set_neutral,0,6,1});
    auto light = fixture({117},{120,158,39,20,215,40,210,185,196,195}); roll(light,75); light.step(attack());
    reject(light,accept(1)); reject(light,accept(2)); reject(light,special(2)); reject(light,special(3)); reject(light,special(4));
    reject(light,special(5)); reject(light,special(6)); reject(light,special(7)); check(light.legal(special(8)) && light.legal(accept(512)), "Light only permits universal reaction or actual195 removal in current pool with noDarkarmor");
    const auto removed = light.step(accept(513)); check(removed.attribute_removed && light.state().players[1].hp == 40 && light.snapshot().hit_index == 1,
        "195 converts Light toNE and ordinaryNE armor can fully defend raw1 without reroll"); roundtrip(light);
    for (CardId card : {CardId{20},CardId{39},CardId{215},CardId{196}}) {
        auto returned = fixture({117},{195,card}); roll(returned,75); bounce_to(returned,0); draws(returned); returned.step(attack());
        const auto r = returned.step(special(1,card == 196 ? 0 : 1)); check(r.valid && !r.dying_weapon_attack && (r.bounced || r.reflected || r.weapon_blocked)
            && returned.snapshot().hit_index == 1 && returned.state().last_chance.source == 0 && returned.state().last_chance.target == 1,
            "195 permits20/39/215;196 accepts rawLight and none replay117 launch or chance roll");
        if (card != 215) { check(returned.state().pending.amount == 1 && returned.state().pending.source == 1 && returned.state().pending.target == 0
                && returned.state().turn_owner == 0 && !returned.state().pending.miracle, "returned117 keeps original raw and immutable chance owner"); roundtrip(returned); returned.step(accept()); }
        check(returned.state().players[0].hp == (card == 215 ? 40 : 39), "returned ordinary117 damages only current target"); roundtrip(returned);
    }
    auto self = fixture({117},{195,20}); draws(self); roll(self,75); bounce_to(self,1); self.step(attack()); const auto bounced = self.step(special(1,1));
    check(bounced.bounce_self && self.state().players[1].hp == 39 && self.snapshot().hit_index == 1 && !bounced.dying_weapon_attack,
        "self bounce resolves once without revisiting enemy normalization or chance"); roundtrip(self);
}

void dead_hook_never_fires_revival_and_counter_queue() {
    for (int cards = 0; cards < 4; ++cards) {
        auto env = fixture({53,117},{117}); auto s = env.snapshot(); if (cards&1) hand(s.state.players[0],{53,117,244}); if (cards&2) hand(s.state.players[1],{117,244});
        s.state.players[0].hp = s.state.players[1].hp = 1; check(env.restore(s), "restore simultaneousHP0 duel with117 and all244 combinations"); draws(env);
        env.step(attack()); const auto r = env.step(accept()); const bool first = cards&1, second = cards == 3;
        check(r.revivals[0] == first && r.revivals[1] == second && env.state().players[0].hp == (first ? 10 : 0)
            && env.state().players[1].hp == (second ? 10 : 0) && copies(env.state().players[0],117) == 1 && copies(env.state().players[1],117) == 1
            && !r.dying_weapon_attack && !r.counter_queued && !env.state().last_chance.id && !env.snapshot().hit_index,
            "244pre-death fixedseat order never reaches117 post-dead alive>=2;117 remains unconsumed and creates no death chance or gift"); roundtrip(env);
    }
    for (CardId id : {CardId{190},CardId{193},CardId{194}}) {
        auto env = fixture({117},{195,id,117}); auto s = env.snapshot(); s.state.players[1].hp = 1; s.state.players[1].mp = 0;
        check(env.restore(s), "restore dying counter owner holding unused117"); draws(env); roll(env,75); env.step(attack());
        const auto r = env.step(counter(3,1)); check(r.counter_queued == 1 && env.state().players[1].dead && copies(env.state().players[1],117) == 1 && !r.dying_weapon_attack
            && env.snapshot().hit_index == 1 && env.state().last_chance.id == 117, "dead117 holder contributes no extra death queue or roll beyond selected ordinary counter");
        if (id == 193) check(r.counter_skipped == 1 && !r.counter_launched && env.state().players[1].mp == 0, "dead193 self counter skips; retained117 does not revive its source");
        else { roundtrip(env); const auto applied = env.step(accept()); check(!applied.dying_weapon_attack && env.state().players[0].hp == (id == 190 ? 38 : 40)
                && (id != 194 || (env.state().players[1].money == 21 && env.state().players[0].money == 19)), "dead counter source still acts or receives194money without triggering117"); }
        check(copies(env.state().players[1],117) == 1, "dead unused117 survives queue resolution without gifting"); roundtrip(env);
    }
    auto cancel = fixture({117,196},{195,190,117}); auto s = cancel.snapshot(); s.state.players[1].hp = 1; check(cancel.restore(s), "restore dead counter source return");
    draws(cancel); roll(cancel,75); cancel.step(attack()); cancel.step(counter(3,1)); const auto r = cancel.step(special(0));
    check(r.counter_cancelled && !r.counter_resolved && copies(cancel.state().players[1],117) == 1 && cancel.snapshot().hit_index == 1,
        "196 returns counter to finalized dead source;117 cannot reopen cancelled item or create death attack"); roundtrip(cancel);
    auto tick = fixture({5,117},{120}); s = tick.snapshot(); s.state.players[0].hp = 1; s.state.players[0].disease = Disease::cold; check(tick.restore(s), "restore Cold owner with unused117");
    const std::uint8_t disease[] = {99}; check(tick.script_disease_rolls(disease), "avoid worsening tick"); draws(tick); tick.step(attack()); const auto death = tick.step(accept());
    check(death.disease_tick && tick.state().players[0].dead && copies(tick.state().players[0],117) == 1 && !tick.state().last_chance.id && !death.dying_weapon_attack
        && !tick.snapshot().draw_index, "disease death after originalitem retains117, creates no chance event and suppresses dead owner gift"); roundtrip(tick);
}

void markets_privacy_restore_and_replay() {
    check(sale_price_count == 216 && sale_price(117) == -1, "old frozen sale price table excludes new117");
    auto sale = fixture({3,117},{120}); draws(sale); sale.step({ActionKind::sell,0,2,1}); const auto sold = sale.step(accept()); check(sold.sale_delivered && sold.sale_price == 10
        && !sold.dying_weapon_attack && copies(sale.state().players[1],117) == 1 && !sale.state().last_chance.id, "sale delivers117 at price10 without ordinary or death activation"); roundtrip(sale);
    auto buy = fixture({4},{117}); draws(buy); const std::uint64_t zero[] = {0}; check(buy.script_buy_tickets(zero), "select117 commodity"); buy.step({ActionKind::buy,0,0,1});
    check(buy.step(accept()).buy_price == 10 && !buy.options().dying_weapon_slots, "purchase offers price10 and hides main-only membership duringapproval");
    const auto bought = buy.step({ActionKind::buy_accept}); check(bought.buy_delivered && !bought.dying_weapon_attack && copies(buy.state().players[0],117) == 1 && !buy.state().last_chance.id,
        "buying117 neither attacks nor creates pending death behavior"); roundtrip(buy);
    auto remove = fixture({240},{117}); draws(remove); check(remove.script_item_removal_tickets(zero), "force sole117 removal"); remove.step({ActionKind::use,0,0,1});
    const auto swept = remove.step(accept()); check(swept.removed_item == 117 && !swept.dying_weapon_attack && !copies(remove.state().players[1],117) && !remove.state().last_chance.id,
        "240 removes nondiscardable117 without invoking passive death behavior"); roundtrip(remove);
    auto pending = fixture({117},{196}); roll(pending,75); pending.step(attack()); const auto exact = pending.snapshot();
    for (int error = 0; error < 6; ++error) {
        auto bad = exact; if (error == 0) bad.state.pending.amount = 30; if (error == 1) bad.state.pending.hit_rate = 100;
        if (error == 2) bad.state.last_chance.id = 111; if (error == 3) bad.state.pending.element = Element::neutral;
        if (error == 4) bad.state.pending.miracle = 198; if (error == 5) bad.state.last_chance.hit = false;
        check(!pending.restore(bad) && pending.snapshot() == exact, "ordinary117 provenance rejects forced death30, alteredrate/ID/attribute/type or miss atomically");
    }
    auto old_cfg = config; old_cfg.dying_weapon = false; auto old = fixture({120},{120},old_cfg), current = fixture({120},{120});
    const auto old_obs = old.observe(), new_obs = current.observe(); check(std::equal(old_obs.begin(),old_obs.begin()+1345,new_obs.begin())
        && std::all_of(old_obs.begin()+1345,old_obs.end(),[](float x){return x == 0.f;}), "old36 prefix1345 preserved and23new tail zero");
    const auto before = old.snapshot(); const CardId ids[] = {117}; check(!old.script_draws(ids) && old.snapshot() == before, "old profile pool rejects117 atomically");
    auto hidden = fixture({120},{117}); auto s = hidden.snapshot(); const auto obs = hidden.observe(); s.state.players[1].hand[0] = 120;
    check(hidden.restore(s) && hidden.observe() == obs, "opponent held117 identity remains private before use");
    auto x = fixture({117},{120}), y = fixture({117},{120}); roll(x,1); roll(y,100); check(x.observe() == y.observe() && x.options().dying_weapon_slots == y.options().dying_weapon_slots
        && x.legal(attack()) == y.legal(attack()), "future hit tape cannot leak through observations/options/legal");
    std::uint64_t uses = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+370000, pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "new117 seeded state and observation replay");
            for (float value : a.observe()) check(std::isfinite(value), "117 observations finite");
            const auto aa = a.sample_action(pa), ab = b.sample_action(pb); check(aa == ab && pa == pb && a.legal(aa), "chance-only117 sampler deterministic and legal");
            const auto ra = a.step(aa), rb = b.step(ab); check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.dying_weapon_attack == rb.dying_weapon_attack,
                "ordinary117 usage and rewards replay exactly"); uses += ra.dying_weapon_attack; roundtrip(a);
        }
    }
    check(uses,"new natural pool and policy exercise117 ordinary attack");
}
}
int main() {
    metadata_minimum_hit_miss_and_forced(); extras_defense_reactions_and_no_reroll(); dead_hook_never_fires_revival_and_counter_queue(); markets_privacy_restore_and_replay();
    std::cout << "Dying weapon passed (Light1/75percent/duel-death-unreachable/244/counters/reactions/market/privacy,1000 replay rollouts).\n";
}
