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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = c.revival = c.item_removal = c.hp_absorption = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL HP absorption: " << message << '\n'; std::exit(1); } }
Action attack(std::uint8_t target = 1, std::uint16_t slots = 0) { return {ActionKind::attack,0,slots,target}; }
Action use(std::uint8_t target = 1) { return {ActionKind::use,0,0,target}; }
Action accept(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action ordered(std::initializer_list<std::uint8_t> refs, std::uint8_t target = 1) {
    Action a{ActionKind::attack_ordered,0,0,target}; a.extra_count = static_cast<std::uint8_t>(refs.size()); std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy);
    check(env.restore(s), "restore explicit absorption fixture"); return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "absorption provenance/holding/history roundtrip exactly"); }
void reject(Env& env, Action a) { const auto before = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == before, "unsupported absorption action rejects atomically"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "isolate replacement draws"); }
void hit(Env& env, std::uint8_t roll) { const std::uint8_t values[] = {roll}; check(env.script_hit_rolls(values), "set chance boundary roll"); }

void minimum_gate_ability_slot_and_pool() {
    check(hp_absorption_card_count == 3 && hp_absorption_catalog().size() == 3 && hp_absorption_miracle_count == 1
        && hp_absorption_miracles().size() == 1 && retained_type_ids.size() == 21 && hp_absorption_retained_type_ids.size() == 22,
        "strict three-weapon/one-miracle scope and old21 retained list preserved");
    const std::array<CardId,3> ids{26,42,103}; const std::array<int,3> attacks{7,11,3};
    for (int i = 0; i < 3; ++i) {
        const auto* w = find_hp_absorption_card(ids[i]);
        check(w && hp_absorption_index(ids[i]) == i && w->card.kind == Kind::weapon && w->card.attack == attacks[i]
            && !w->card.boost && !w->card.defense && !w->card.hp && !w->card.mp && w->card.weight == 1
            && !w->card.discardable && w->hit_rate == (i == 2 ? 75 : 0)
            && w->element == (i == 2 ? Element::wood : Element::neutral), "absorber weapon metadata has no hidden ordinary extras");
    }
    const auto* miracle = find_hp_absorption_miracle(221);
    check(miracle && miracle->amount == 10 && miracle->cost == 10 && miracle->weight == 1 && miracle->element == Element::light
        && hp_absorption_miracle_index(221) == 0 && !find_offensive_miracle(221)
        && !find_hp_absorption_card(221) && !find_hp_absorption_miracle(26), "221 belongs to separate catalog, not old offensive13");
    Config minimum; minimum.miracles = minimum.elements = minimum.chance = minimum.harms = minimum.celestial = minimum.hp_absorption = true;
    auto env = fixture({221},{120},minimum); auto s = env.snapshot(); s.state.players[0].abilities[hp_absorption_ability_index] = 1;
    check(env.restore(s), "221 retained index15 is independent of old offensive profile");
    check(total_miracle_count == 15 && total_retained_miracle_count == 17 && main_ability_capacity == 16
        && hp_absorption_ability_index == 15 && std::string_view(env.profile()) == hp_absorption_ruleset && env.snapshot().version == 37,
        "old15/17 constants frozen; new221 gets appended index15 and dedicated profile");
    check(env.options().ability_slots == (1u<<15) && env.options().hp_absorption_slots == 1 && env.options().utility_slots == 1,
        "bit15 and hand221 utility work with offensive_miracles disabled");
    check(env.step({ActionKind::cast,15,0,1}).valid && env.state().pending.miracle == 221
        && env.state().pending.hp_absorption_card == 221 && env.state().players[0].mp == 0,
        "cast15 pays MP10 and preserves MAGIC plus absorption provenance"); roundtrip(env);
    for (int missing = 0; missing < 2; ++missing) {
        auto bad = minimum; if (!missing) bad.miracles = false; else bad.celestial = false;
        bool threw = false; try { Env invalid(bad); } catch (const std::invalid_argument&) { threw = true; }
        check(threw, "HP absorption dependencies enforced");
    }
    auto old_cfg = config; old_cfg.hp_absorption = false; Env old(old_cfg); const auto before = old.snapshot();
    for (CardId id : {CardId{26},CardId{42},CardId{103},CardId{221}}) {
        const CardId ids[] = {id}; check(!old.script_draws(ids) && old.snapshot() == before, "old pool excludes all four absorbers");
    }
    auto forged = before; forged.state.players[0].abilities[15] = 1;
    check(!old.restore(forged) && old.snapshot() == before, "disabled append slot must stay zero");
    auto legacy = fixture({120},{120},minimum); auto invalid = legacy.snapshot(); invalid.state.players[0].abilities[2] = 1;
    check(!legacy.restore(invalid), "new221 permission does not enable old offensive13 holdings");
}

void residual_defense_cap_self_and_overkill() {
    for (CardId id : {CardId{26},CardId{42}}) {
        const int power = id == 26 ? 7 : 11;
        auto env = fixture({id},{120}); draws(env); env.step(attack()); const auto r = env.step(accept(1));
        check(r.hp_absorption_resolved && r.hp_absorption_card == id && r.hp_absorption_amount == power-1
            && r.hp_absorption_healing == power-1 && r.hp_absorption_source == 0 && r.hp_absorption_target == 1
            && env.state().players[0].hp == 40+power-1 && env.state().players[1].hp == 40-power+1
            && env.snapshot().draw_index == 2, "ordinary defense reduces the shared positive residual before healing/damage"); roundtrip(env);
        auto blocked = fixture({id},{177}); blocked.step(attack()); const auto zero = blocked.step(accept(1));
        check(zero.hp_absorption_resolved && zero.hp_absorption_amount == 0 && zero.hp_absorption_healing == 0
            && blocked.state().last_hp_absorption.id == id && blocked.state().players[0].hp == 40
            && blocked.state().players[1].hp == 40, "full defense still publishes resolved absorption event with zero residual"); roundtrip(blocked);
    }
    for (int initial : {40,98,99}) {
        auto self = fixture({26},{120}); auto s = self.snapshot(); s.state.players[0].hp = initial;
        check(self.restore(s), "restore self cap case"); const auto r = self.step(attack(0));
        const int healing = std::min(99-initial,7);
        check(r.hp_absorption_resolved && r.hp_absorption_source == 0 && r.hp_absorption_target == 0
            && r.hp_absorption_amount == 7 && r.hp_absorption_healing == healing
            && self.state().players[0].hp == initial+healing-7,
            "self absorption heals first and then damages; cap can cause a net HP loss"); roundtrip(self);
    }
    auto overkill = fixture({42,44},{244}); auto s = overkill.snapshot(); s.state.players[0].hp = 1; s.state.players[1].hp = 1;
    check(overkill.restore(s), "restore oneHP target and raw22 absorption"); draws(overkill); overkill.step(attack(1,2)); const auto r = overkill.step(accept());
    check(r.hp_absorption_amount == 22 && r.hp_absorption_healing == 22 && overkill.state().players[0].hp == 23
        && r.revivals[1] == 1 && overkill.state().players[1].hp == 10 && overkill.snapshot().draw_index == 2,
        "absorb raw residual rather than target's actual oneHP loss; revival follows subtraction and earns no gift"); roundtrip(overkill);
}

void reflected_source_provisional_heaven_and_counter() {
    auto reflected = fixture({26},{39}); auto s = reflected.snapshot(); s.state.players[0].hp = 30; s.state.players[1].hp = 50;
    check(reflected.restore(s), "restore reflected absorption sources"); draws(reflected); reflected.step(attack()); reflected.step(special(0));
    check(reflected.state().pending.hp_absorption_card == 26 && reflected.state().pending.source == 1, "physical reflection preserves marker and changes current source"); roundtrip(reflected);
    const auto r = reflected.step(accept()); check(r.hp_absorption_source == 1 && r.hp_absorption_target == 0
        && reflected.state().players[1].hp == 57 && reflected.state().players[0].hp == 23,
        "heal current reflector rather than immutable original owner"); roundtrip(reflected);
    auto heavenly = fixture({26},{175,39,244}); s = heavenly.snapshot(); s.state.players[1].disease = Disease::heaven;
    check(heavenly.restore(s), "restore Heaven reflector and unused revival"); draws(heavenly); heavenly.step(attack()); const auto prefix = heavenly.step(special(1,1));
    check(!prefix.hp_absorption_resolved && heavenly.state().players[1].hp == 0 && !heavenly.state().players[1].dead,
        "Heaven prefix drops reflector to provisionalHP0 before returned effect"); roundtrip(heavenly);
    const auto saved = heavenly.step(accept()); check(saved.hp_absorption_healing == 7 && heavenly.state().players[1].hp == 7
        && !saved.revivals[1] && heavenly.state().players[1].hand[0] == 244 && !heavenly.state().players[1].dead,
        "absorption rescues provisional source before death finalization without consuming244"); roundtrip(heavenly);
    auto counter = fixture({26},{190}); s = counter.snapshot(); s.state.players[0].hp = 20; check(counter.restore(s), "restore counter after absorbing original hit");
    draws(counter); counter.step(attack()); Action c = accept(1); c.counter_count = 1; c.counter_order = 0;
    const auto original = counter.step(c); check(original.hp_absorption_healing == 7 && counter.state().players[0].hp == 27
        && counter.state().pending.counter_card == 190 && counter.state().pending.amount == 14
        && !counter.state().pending.hp_absorption_card, "queued counter derives raw residual but drops absorption marker"); roundtrip(counter);
    const auto back = counter.step(accept()); check(!back.hp_absorption_resolved && counter.state().players[0].hp == 13
        && counter.state().players[1].hp == 33 && counter.state().last_hp_absorption.id == 26,
        "counter is ordinary damage and does not heal its source or overwrite last absorption"); roundtrip(counter);
    auto bounce = fixture({26},{20}); s = bounce.snapshot(); s.state.players[1].hp = 98;
    check(bounce.restore(s), "restore near-cap reflector self-bounce"); const std::uint8_t destination[] = {1};
    check(bounce.script_bounce_targets(destination), "force current source as bounce target"); bounce.step(attack());
    const auto self = bounce.step(special(0)); check(self.bounced && self.bounce_self && self.hp_absorption_resolved
        && self.hp_absorption_source == 1 && self.hp_absorption_target == 1 && self.hp_absorption_healing == 1
        && bounce.state().players[1].hp == 92, "20 self-bounce performs capped heal before its own raw damage"); roundtrip(bounce);
    auto blocked = fixture({26},{215}); blocked.step(attack()); const auto block = blocked.step(special(0));
    check(block.weapon_blocked && !block.hp_absorption_resolved && !blocked.state().last_hp_absorption.id,
        "215 cancels absorption effect completely rather than resolving a zero-residual event"); roundtrip(blocked);
}

void chance_boundary_setter_ordered_and_large_raw() {
    for (std::uint8_t roll : {std::uint8_t{75},std::uint8_t{76}}) {
        auto env = fixture({103},{144}); hit(env,roll); draws(env); const auto attempt = env.step(attack(0));
        check(attempt.chance_attempt && attempt.chance_hit == (roll == 75) && !attempt.hp_absorption_resolved,
            "103 checks75percent boundary and normalizes self request to enemy");
        if (roll == 75) {
            check(env.state().pending.hp_absorption_card == 103 && env.state().pending.amount == 3 && env.state().pending.hit_rate == 75,
                "103 hit has fixed3 and its own absorption provenance"); roundtrip(env);
            const auto zero = env.step(accept(1)); check(zero.hp_absorption_resolved && !zero.hp_absorption_amount
                && !zero.hp_absorption_healing, "opposite Stone defense can fully stop chance absorption");
        } else check(!env.state().last_hp_absorption.id && env.state().players[0].hp == 40 && env.state().players[1].hp == 40,
            "miss causes no absorption event or resource changes");
        roundtrip(env);
    }
    auto forced = fixture({103},{196}); auto s = forced.snapshot(); s.state.players[1].dark_cloud = true;
    check(forced.restore(s), "restore forced chance absorber"); hit(forced,100); forced.step(attack()); forced.step(special(0));
    const auto reflected = forced.step(accept()); check(reflected.hp_absorption_source == 1 && reflected.hp_absorption_amount == 3
        && forced.snapshot().hit_index == 0 && forced.state().last_chance.forced,
        "forced chance and196 reflection keep original successful hit without reroll"); roundtrip(forced);
    auto percent_self = fixture({103},{195,20}); s = percent_self.snapshot(); s.state.players[1].hp = 98;
    check(percent_self.restore(s), "restore percentage attack after removal and self-bounce"); hit(percent_self,75); draws(percent_self);
    const std::uint8_t self_target[] = {1}; check(percent_self.script_bounce_targets(self_target), "self-bounce after attribute removal");
    percent_self.step(attack(0)); const auto self = percent_self.step(special(1,1));
    check(self.attribute_removed && self.removed_from == Element::wood && self.bounced && self.bounce_self
        && self.hp_absorption_card == 103 && self.hp_absorption_source == 1 && self.hp_absorption_target == 1
        && self.hp_absorption_amount == 3 && self.hp_absorption_healing == 1 && percent_self.state().players[1].hp == 96
        && percent_self.snapshot().hit_index == 1 && percent_self.state().last_chance.id == 103
        && percent_self.state().last_chance.source == 0 && percent_self.state().last_chance.target == 1,
        "195 permits20 to self-bounce103; cap99 precedes raw3 and immutable hit is never rerolled"); roundtrip(percent_self);
    auto setter = fixture({26,65},{148}); draws(setter); check(setter.step({ActionKind::attack_set_fire,0,2,1}).valid
        && setter.state().pending.hp_absorption_card == 26 && setter.state().pending.element == Element::fire,
        "setter changes current attribute while keeping original absorption payload"); roundtrip(setter);
    const auto residual = setter.step(accept(1)); check(residual.hp_absorption_amount == 2 && residual.hp_absorption_healing == 2,
        "newFire7 versus WaterDEF5 heals final residual2"); roundtrip(setter);
    auto order = fixture({26,197},{120}); draws(order); order.step(ordered({1}));
    check(order.state().pending.hp_absorption_card == 26 && order.state().pending.amount == 9
        && order.state().pending.element == Element::neutral && !order.state().pending.miracle,
        "ordered additional MAGIC retains original WEAPON absorption type"); roundtrip(order);
    check(order.step(accept()).hp_absorption_healing == 9, "ordered residual is absorbed once");
    auto large = fixture({26},{244}); s = large.snapshot(); s.state.players[0].mp = 99; s.state.players[0].hp = 1; s.state.players[0].doubling_ability = 1;
    check(large.restore(s), "restore large retained multiplier absorption"); Action multiply{ActionKind::attack_ordered,0,0,1}; multiply.extra_count = 16;
    std::fill_n(multiply.extras.begin(),16,18); draws(large); large.step(multiply); roundtrip(large);
    const auto enormous = large.step(accept()); check(enormous.hp_absorption_amount == (7LL<<16) && enormous.hp_absorption_healing == 98
        && large.state().players[0].hp == 99 && enormous.revivals[1] == 1,
        "int64 raw amount survives multiplier composition while actual HP healing clips safely to99"); roundtrip(large);
}

void magic221_free_holding_forgetting_and_reactions() {
    auto learn = fixture({221},{195}); draws(learn); check(learn.step(use()).valid && learn.state().players[0].abilities[15] == 1
        && learn.state().players[0].mp == 0 && learn.state().pending.miracle == 221 && learn.state().pending.hp_absorption_card == 221,
        "hand221 first use learns appended retained slot and pays10MP"); roundtrip(learn);
    const auto hit = learn.step(accept(1)); check(hit.hp_absorption_card == 221 && hit.hp_absorption_amount == 10
        && hit.hp_absorption_healing == 10, "195neutralization preserves MAGIC absorption rather than cancelling effect"); roundtrip(learn);
    auto free = fixture({239},{40}); auto s = free.snapshot(); s.state.players[0].abilities[15] = 1; s.state.players[0].mp = 0;
    check(free.restore(s), "restore held221 at0MP with carrier"); Action cast{ActionKind::cast_ordered,47,0,1}; cast.extra_count = 1; cast.extras[0] = 0;
    check(free.step(cast).valid && free.state().pending.miracle == 221 && free.state().pending.hp_absorption_card == 221
        && free.state().pending.from_ability && free.state().players[0].retained_deals == 1,
        "FREE ref47 accepts new held MAGIC without changing old32..46 indices"); roundtrip(free);
    check(free.step(special(0)).magic_reflected, "MAGIC reflection handles221"); roundtrip(free);
    const auto returned = free.step(accept()); check(returned.hp_absorption_source == 1 && returned.hp_absorption_target == 0
        && free.state().players[0].mp == 0 && free.state().players[0].abilities[15] == 1,
        "returned221 heals new source and neither refunds MP nor reacquires miracle"); roundtrip(free);
    for (bool revive : {false,true}) {
        auto heavenly = revive ? fixture({221},{175,210,244}) : fixture({221},{175,210}); s = heavenly.snapshot();
        s.state.players[1].disease = Disease::heaven; check(heavenly.restore(s), "restore221 Heaven self-bounce and optional passive");
        draws(heavenly); const std::uint8_t landing[] = {1}; check(heavenly.script_bounce_targets(landing), "self-bounce221 after Heaven prefix");
        heavenly.step(use()); const auto bounced = heavenly.step(special(1,1));
        check(bounced.magic_bounced && bounced.bounce_self && bounced.hp_absorption_card == 221
            && bounced.hp_absorption_source == 1 && bounced.hp_absorption_target == 1
            && bounced.hp_absorption_amount == 10 && bounced.hp_absorption_healing == 10
            && heavenly.state().players[1].hp == (revive ? 10 : 0) && bounced.revivals[1] == revive
            && heavenly.state().players[1].dead == !revive && heavenly.done() == !revive
            && heavenly.state().players[1].mp == 5 && heavenly.state().players[0].abilities[15] == 1,
            "temporaryHP0 self absorption heals10 then damages back to0;244 alone rescues after the item"); roundtrip(heavenly);
    }
    auto block = fixture({221},{185}); block.step(use()); const auto cancelled = block.step(special(0));
    check(cancelled.magic_blocked && !cancelled.hp_absorption_resolved && !block.state().last_hp_absorption.id
        && block.state().players[0].mp == 0 && block.state().players[0].abilities[15] == 1,
        "MAGIC block cancels221 healing/damage while retaining paid MP and first acquisition"); roundtrip(block);
    auto forgotten = fixture({241},{120}); s = forgotten.snapshot(); s.state.players[1].abilities[15] = 1;
    check(forgotten.restore(s), "restore new221 in forgetting inventory"); const std::uint64_t ticket[] = {0}; check(forgotten.script_forget_tickets(ticket), "select only retained221");
    forgotten.step(use()); const auto gone = forgotten.step(accept()); check(gone.forgotten_miracle == 221 && gone.forget_population == 1
        && forgotten.state().players[1].abilities[15] == 0, "new221 is included in weighted forgetting and removed from correct storage slot"); roundtrip(forgotten);
}

void privacy_histories_restore_and_replay() {
    auto hidden_a = fixture({26},{120}), hidden_b = fixture({26},{120}); auto s = hidden_b.snapshot(); s.state.players[1].abilities[15] = 99;
    check(hidden_b.restore(s) && hidden_a.observe() == hidden_b.observe() && hidden_a.options() == hidden_b.options(),
        "opponent new221 holdings stay private and do not alter current legal choices");
    auto own = fixture({221},{120}); s = own.snapshot(); s.state.players[0].abilities[15] = 3; check(own.restore(s), "restore new own holding observation");
    check(own.observe()[1085] == 1.f && own.observe()[1086] == 1.f && own.observe()[1102] == 3.f/config.max_decisions,
        "dedicated new features expose own membership and index15 count");
    auto old_cfg = config; old_cfg.hp_absorption = false; Env old(old_cfg); const auto obs = old.observe();
    check(std::all_of(obs.begin()+1085,obs.end(),[](float x){return x == 0.f;}), "old27 profiles keep all25 new features zero");
    auto prefix_new = fixture({120},{120}), prefix_old = fixture({120},{120},old_cfg); s = prefix_new.snapshot();
    s.state.players[0].abilities[15] = 3; check(prefix_new.restore(s), "restore appended own holding without old abilities");
    const auto new_obs = prefix_new.observe(), old_obs = prefix_old.observe();
    for (int i = 0; i < 1085; ++i) if (i != 1016)
        check(new_obs[i] == old_obs[i], "221 count never spills into old13 individual ability/hand features");
    check(new_obs[1016] == 3.f/(16.f*config.max_decisions) && old_obs[1016] == 0.f,
        "existing all-retained own aggregate includes221 while old13 individual counts remain unchanged");
    auto env = fixture({26},{120}); env.step(attack()); const auto exact = env.snapshot();
    for (int error = 0; error < 8; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.pending.hp_absorption_card = 42;
        if (error == 1) bad.state.pending.hp_absorption_card = 221;
        if (error == 2) bad.state.pending.status_card = 175;
        if (error == 3) bad.state.pending.harm_card = 70;
        if (error == 4) bad.state.pending.element = Element::dark;
        if (error == 5) bad.state.pending.mana_weapon = 57;
        if (error == 6) bad.state.pending.counter_card = 190;
        if (error == 7) bad.state.pending.amount = 6;
        check(!env.restore(bad) && env.snapshot() == exact, "inconsistent base/type/status/Dark/raw provenance rejects atomically");
    }
    env.step(accept()); const auto history = env.snapshot();
    for (int error = 0; error < 6; ++error) {
        auto bad = history;
        if (error == 0) bad.state.last_hp_absorption.id = 5;
        if (error == 1) bad.state.last_hp_absorption.source = no_slot;
        if (error == 2) bad.state.last_hp_absorption.target = no_slot;
        if (error == 3) bad.state.last_hp_absorption.amount = -1;
        if (error == 4) bad.state.last_hp_absorption.healing = 8;
        if (error == 5) bad.state.last_hp_absorption.amount = magic_free_damage_limit+1;
        check(!env.restore(bad) && env.snapshot() == history, "impossible public absorption record rejects atomically");
    }
    std::uint64_t events = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env x(config), y(config); x.reset(seed); y.reset(seed); std::uint64_t px = seed+280000, py = px;
        while (!x.done()) {
            check(x.snapshot() == y.snapshot() && x.observe() == y.observe(), "seeded absorption state/observation replay");
            for (float value : x.observe()) check(std::isfinite(value), "absorption observation stays finite");
            const auto a = x.sample_action(px), b = y.sample_action(py); check(a == b && px == py && x.legal(a), "new slot sampler deterministic/legal");
            const auto rx = x.step(a), ry = y.step(b); check(rx.valid && ry.valid && rx.rewards == ry.rewards
                && rx.hp_absorption_resolved == ry.hp_absorption_resolved && rx.hp_absorption_amount == ry.hp_absorption_amount
                && rx.hp_absorption_healing == ry.hp_absorption_healing && rx.hp_absorption_source == ry.hp_absorption_source,
                "absorption outcomes and clipped healing replay exactly"); events += rx.hp_absorption_resolved; roundtrip(x);
        }
    }
    check(events, "natural pool/sampler exercises absorption");
}
}
int main() {
    minimum_gate_ability_slot_and_pool(); residual_defense_cap_self_and_overkill(); reflected_source_provisional_heaven_and_counter();
    chance_boundary_setter_ordered_and_large_raw(); magic221_free_holding_forgetting_and_reactions(); privacy_histories_restore_and_replay();
    std::cout << "HP absorption passed (raw/cap/self/reflect/Heaven/counter/221/FREE/privacy,1000 replay rollouts).\n";
}
