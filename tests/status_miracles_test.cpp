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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = c.revival = c.item_removal = c.hp_absorption = c.self_damage = c.pestle_mortar = c.selling = c.buying = c.hp_gamble = c.magic_bounce = c.sundry_boost = c.status_miracles = true;
    return c;
}();
Config minimum() { Config c; c.miracles = c.elements = c.chance = c.disease = c.harms = c.celestial = c.status_miracles = true; return c; }
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL status miracles: " << message << '\n'; std::exit(1); } }
Action use(std::uint8_t target = 1) { return {ActionKind::use,0,0,target}; }
Action accept(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action freecast(int base, std::initializer_list<std::uint8_t> refs, std::uint8_t target = 1) {
    Action a{ActionKind::cast_ordered,static_cast<std::uint8_t>(base),0,target}; a.extra_count = static_cast<std::uint8_t>(refs.size()); std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
Action defense(std::initializer_list<std::uint8_t> refs) {
    Action a{ActionKind::defend_ordered,no_slot,0,no_slot}; a.extra_count = static_cast<std::uint8_t>(refs.size()); std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0; p.incoming_sale = p.incoming_buy = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy); s.state.players[0].mp = 99;
    if (!env.restore(s)) {
        std::cerr << "Fixture own:"; for (const auto id : own) std::cerr << ' ' << id;
        std::cerr << " enemy:"; for (const auto id : enemy) std::cerr << ' ' << id;
        std::cerr << '\n'; check(false, "restore explicit direct-status fixture");
    }
    return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "status payload, holdings, response provenance and budgets roundtrip"); }
void reject(Env& env, Action a) { const auto before = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == before, "unsupported status role/reference rejects atomically"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "isolate deferred status gifts"); }
void no_worsen(Env& env) { std::array<std::uint8_t,64> rolls{}; rolls.fill(99); check(env.script_disease_rolls(rolls), "keep deterministic natural disease ticks"); }
void target(Env& env, std::uint8_t seat) { const std::uint8_t seats[] = {seat}; check(env.script_bounce_targets(seats), "fix magic bounce destination"); }
void set(Player& p, CardId id, std::uint64_t n) {
    if (const int i = status_miracle_index(id); i >= 0) p.status_abilities[i] = static_cast<std::uint32_t>(n);
    else if (const int i = miracle_index(id); i >= 0) p.abilities[i] = static_cast<std::uint32_t>(n);
    else if (const int i = offensive_miracle_index(id); i >= 0) p.abilities[miracle_count+i] = static_cast<std::uint32_t>(n);
    else if (const int i = defensive_miracle_index(id); i >= 0) p.defensive_abilities[i] = static_cast<std::uint32_t>(n);
    else if (const int i = additive_miracle_index(id); i >= 0) p.additive_abilities[i] = n;
    else if (id == 214) p.doubling_ability = n;
    else if (id == 209) p.wide_ability = n;
    else if (id == 221) p.abilities[hp_absorption_ability_index] = static_cast<std::uint32_t>(n);
    else check(false,"unknown retained fixture type");
}

void metadata_minimum_first_and_recast() {
    constexpr std::array<CardId,5> ids{202,208,211,212,224}; constexpr std::array<int,5> costs{3,5,5,6,15};
    check(status_miracle_count == 5 && status_miracles().size() == 5 && status_retained_type_ids.size() == 27
        && retained_type_ids.size() == 21 && hp_absorption_retained_type_ids.size() == 22 && main_ability_capacity == 16 && total_miracle_count == 15,
        "five separate status holdings preserve all old main/forget capacities");
    for (int i = 0; i < 5; ++i) {
        const auto& m = status_miracles()[i]; check(m.id == ids[i] && status_miracle_index(m.id) == i && find_status_miracle(m.id) == &m
            && m.cost == costs[i] && m.price == (i == 2 || i == 4 ? 15 : 10) && m.weight == 1
            && m.element == (i == 1 ? Element::dark : Element::neutral), "exact sorted status catalog costs, prices and typed attributes");
        auto env = fixture({m.id},{186},minimum()); draws(env); no_worsen(env);
        check(std::string_view(env.profile()) == status_miracles_ruleset && env.snapshot().version == 36
            && env.options().status_slots == 1 && env.options().utility_slots == 1 && env.options().pray && !env.options().weapon_bases,
            "minimal profile exposes standalone status as utility, never weapon");
        const auto r = env.step(use()); const auto& p = env.state().pending;
        check(r.valid && r.status_first_use && !r.status_recast && r.spell_first_use && !r.spell_recast && r.spell_mp_spent == m.cost
            && r.status_miracle_card == m.id && r.status_source == 0 && r.status_target == 1 && env.state().players[0].status_abilities[i] == 1
            && env.state().players[0].mp == 99-m.cost && env.state().players[0].pending_deals == 1 && !env.state().players[0].hand_count
            && p.kind == Effect::status && !p.amount && p.miracle == m.id && p.element == m.element && p.cure == m.cure && p.disease == m.disease
            && p.harm == m.harm && !p.from_ability && !p.hit_rate && !p.harm_card && !p.status_card,
            "initial use pays once, retains separate copy, and creates exact zero-amount MAGIC status"); roundtrip(env);
        check(env.options().defense_slots == 1 && env.legal(accept(1)), "minimal status profile accepts ordinary armor even without defensive miracles");
        const auto applied = env.step(accept(1)); check(applied.valid && applied.status_resolved && applied.status_miracle_card == m.id
            && applied.status_source == 0 && applied.status_target == 1 && !applied.counter_queued && env.snapshot().draw_index == 2,
            "ordinary DEF cannot suppress status or create damage-counter effects; both physical cards earn gifts");
        check(env.state().players[1].hp == 40 && (i != 1 || env.state().players[1].dark_cloud)
            && (i != 3 || env.state().players[1].disease == Disease::cold) && (i != 4 || env.state().players[1].disease == Disease::heaven),
            "DARK208 is a direct flag, never dark damage execution"); roundtrip(env);
        auto held = fixture({120},{120},minimum()); auto s = held.snapshot(); s.state.players[0].status_abilities[i] = 1;
        s.state.players[0].mp = m.cost; check(held.restore(s), "restore exact affordable held status"); draws(held);
        check(held.options().status_ability_slots == (1u<<i) && !held.options().ability_slots, "new held options do not leak into old16bit main mask");
        const auto cast = held.step({ActionKind::cast_status,static_cast<std::uint8_t>(i),0,1});
        check(cast.status_recast && cast.spell_recast && !cast.status_first_use && !cast.spell_first_use && cast.spell_mp_spent == m.cost
            && held.state().players[0].status_abilities[i] == 1 && held.state().players[0].mp == 0 && held.state().players[0].retained_deals == 1
            && held.state().pending.from_ability, "recast pays MP and schedules one retained gift without acquiring duplicate"); roundtrip(held);
        held.step(accept()); roundtrip(held);
    }
    for (CardId id : {CardId{0},CardId{198},CardId{210},CardId{216},CardId{221},CardId{65535}}) check(!find_status_miracle(id), "no implicit utility/offensive/defensive inclusion");
    for (int absent = 0; absent < 3; ++absent) {
        auto bad = minimum(); if (absent == 0) bad.miracles = false; if (absent == 1) bad.disease = false; if (absent == 2) bad.celestial = false;
        bool threw = false; try { Env invalid(bad); } catch (const std::invalid_argument&) { threw = true; } check(threw,"status profile dependencies explicit");
    }
}

void cure_rank_heaven_and_owner_tick() {
    for (bool all : {false,true}) for (Disease disease : {Disease::none,Disease::cold,Disease::fever,Disease::hell,Disease::heaven}) {
        auto env = fixture({static_cast<CardId>(all ? 211 : 202)},{120}); auto s = env.snapshot(); auto& t = s.state.players[1];
        t.disease = disease; t.worse_chance = disease == Disease::none ? 0 : 37; t.glory = t.dark_cloud = true; check(env.restore(s), "restore cure boundary disease/harms");
        env.step(use()); env.step(accept()); const auto& p = env.state().players[1]; const bool cleared = all || disease == Disease::cold || disease == Disease::fever;
        check(p.hp == 40 && p.disease == (cleared ? Disease::none : disease) && p.worse_chance == (cleared || disease == Disease::none ? 0 : 37)
            && !p.glory && p.dark_cloud == !all, "lower clears Cold/Fever+Glory only; all clears Heaven/Hell and both flags without falling"); roundtrip(env);
    }
    for (Disease before : {Disease::none,Disease::cold,Disease::fever,Disease::hell}) {
        auto env = fixture({212},{120}); auto s = env.snapshot(); s.state.players[1].disease = before; s.state.players[1].worse_chance = before == Disease::none ? 0 : 31;
        check(env.restore(s), "restore repeated direct Cold rank"); env.step(use()); env.step(accept());
        check(env.state().players[1].disease == static_cast<Disease>(static_cast<int>(before)+1) && !env.state().players[1].worse_chance,
            "Cold direct application advances one rank and resets worsening counter"); roundtrip(env);
    }
    for (CardId id : {CardId{208},CardId{212},CardId{224}}) for (bool revive : {false,true}) {
        auto env = revive ? fixture({id},{244}) : fixture({id},{120}); auto s = env.snapshot(); s.state.players[1].disease = Disease::heaven;
        s.state.players[1].worse_chance = 37; s.state.players[1].glory = true; check(env.restore(s), "restore Heaven before direct harmful status"); draws(env);
        env.step(use()); const auto r = env.step(accept()); check(env.state().players[1].hp == (revive ? 10 : 0) && r.revivals[1] == revive
            && env.state().players[1].disease == Disease::heaven && !env.state().players[1].worse_chance && env.state().players[1].glory
            && !env.state().players[1].dark_cloud, "Heaven falls before incoming status, leaves payload unapplied, then optional passive revival"); roundtrip(env);
    }
    auto cold = fixture({212},{120}); no_worsen(cold); const auto r = cold.step(use(0)); check(r.status_resolved && r.disease_tick && r.disease_damage == 1
        && cold.state().players[0].hp == 39 && cold.state().players[0].disease == Disease::cold && cold.state().players[0].worse_chance == 1,
        "self Cold applies before exactly one immediate owner tick"); roundtrip(cold);
    auto heaven = fixture({224},{120}); no_worsen(heaven); const auto h = heaven.step(use(0)); check(h.status_resolved && h.disease_tick && h.disease_healing == 5
        && heaven.state().players[0].hp == 45 && heaven.state().players[0].disease == Disease::heaven, "first self Heaven heals5 on same turn"); roundtrip(heaven);
}

void stage_prefix_free_and_provisional_zero() {
    for (CardId card : {CardId{119},CardId{40},CardId{185},CardId{210},CardId{196}}) {
        auto env = fixture({212},{card}); draws(env); no_worsen(env); target(env,0); env.step(use()); const auto r = env.step(special(0));
        check(r.valid && (r.magic_bounced || r.reflected || r.magic_blocked) && !r.status_first_use && !r.status_recast
            && env.state().players[0].status_abilities[3] == 1 && env.state().players[0].mp == 93,
            "all matching MAGIC stages consume original status without paying/acquiring it again");
        if (card == 185) check(env.state().players[0].disease == Disease::none && env.state().players[1].disease == Disease::none && !r.status_resolved, "MAGIC block cancels pure status");
        else { roundtrip(env); const auto applied = env.step(accept()); check(applied.status_resolved && applied.status_source == 1 && applied.status_target == 0
            && env.state().players[0].disease == Disease::cold && env.state().players[0].hp == 39, "redirected status resolves at current target then original-owner tick"); }
        roundtrip(env);
    }
    auto wrong = fixture({212},{20,39,215,202}); wrong.step(use()); reject(wrong,special(0)); reject(wrong,special(1)); reject(wrong,special(2));
    reject(wrong,accept(8)); reject(wrong,special(3));
    auto stripped = fixture({208},{195}); stripped.step(use()); const auto applied = stripped.step(accept(1));
    check(applied.attribute_removed && applied.status_resolved && stripped.state().players[1].dark_cloud && stripped.state().players[1].hp == 40,
        "195 changes current attribute, never direct DarkCloud payload or HP"); roundtrip(stripped);
    auto prefix = fixture({212},{120,119}); target(prefix,0); prefix.step(use()); check(prefix.step(special(1,1)).magic_card_bounced,
        "status MAGIC allows ordinary prefix before mandatory physical bounce"); roundtrip(prefix);
    auto counter = fixture({212},{190}); counter.step(use()); auto a = accept(1); a.counter_count = 1; const auto cr = counter.step(a);
    check(cr.valid && cr.status_resolved && !cr.counter_queued && counter.state().players[1].disease == Disease::cold,
        "counter prefix consumed but zero damage status never schedules retaliation"); roundtrip(counter);
    auto suffix = fixture({212},{119,239}); target(suffix,0); suffix.step(use()); const auto sf = suffix.step(defense({0,1}));
    check(sf.magic_card_bounced && sf.magic_free_suffix && !sf.magic_free_cards && suffix.state().players[1].hand[0] == 239,
        "FREE after physical MAGIC stage remains unconsumed"); roundtrip(suffix);
    auto glory = fixture({212},{120,119}); auto s = glory.snapshot(); s.state.players[1].glory = true; check(glory.restore(s), "restore one-card Glory limit");
    glory.step(use()); reject(glory,special(1,1)); check(glory.legal(special(1)), "Glory permits one stage card and excludes prefix plus stage");
    for (bool revive : {false,true}) {
        auto fall = revive ? fixture({211},{175,119,244}) : fixture({211},{175,119}); s = fall.snapshot(); s.state.players[1].disease = Disease::heaven;
        check(fall.restore(s), "restore Heaven prefix cured after self bounce"); target(fall,1); const auto before = fall.state().players[1].mp; fall.step(use());
        const auto r = fall.step(special(1,1)); check(r.status_resolved && r.bounce_self && fall.state().players[1].disease == Disease::none
            && fall.state().players[1].hp == (revive ? 10 : 0) && r.revivals[1] == revive && fall.state().players[1].mp == before,
            "175prefix falls toHP0, returned cure removes disease without healing, only244 rescues"); roundtrip(fall);
    }
}

void free_refs_capacity_and_restore() {
    for (int i = 0; i < 5; ++i) for (bool held : {false,true}) {
        const auto& m = status_miracles()[i]; auto env = held ? fixture({239},{120}) : fixture({m.id,239},{120}); auto s = env.snapshot();
        s.state.players[0].mp = 0; if (held) s.state.players[0].status_abilities[i] = 1; check(env.restore(s), "restore0MP new FREE status reference"); draws(env);
        check(env.options().free_status_ability_slots == (held ? 1u<<i : 0), "held FREE status mask uses separate five local bits");
        const auto r = env.step(held ? freecast(48+i,{0}) : freecast(0,{1})); check(r.valid && r.free_cast && r.status_first_use == !held && r.status_recast == held
            && r.spell_first_use == !held && r.spell_recast == held && !r.spell_mp_spent && r.magic_free_mp_saved == m.cost && r.magic_free_cards == 1
            && env.state().players[0].mp == 0 && env.state().players[0].status_abilities[i] == 1 && env.state().pending.free_card == 239
            && env.state().pending.miracle == m.id && env.state().players[0].pending_deals == 2,
            "refs48..52 map to new storage; FREE physical and retained use preserve exact acquisition, MP and gifts"); roundtrip(env);
        env.step(accept()); roundtrip(env);
    }
    auto unsafe = fixture({5,212,239},{120}); reject(unsafe,freecast(48,{2})); reject(unsafe,freecast(53,{2})); reject(unsafe,freecast(47,{2}));
    reject(unsafe,{ActionKind::cast_status,5,0,1}); reject(unsafe,{ActionKind::cast,16,0,1});
    Action extra{ActionKind::attack_ordered,0,0,1}; extra.extra_count = 1; extra.extras[0] = 1; reject(unsafe,extra);
    auto cap_cfg = config; cap_cfg.max_decisions = 8; auto capped = fixture({212},{120},cap_cfg); auto s = capped.snapshot();
    s.state.players[0].abilities[0] = 3; s.state.players[0].defensive_abilities[0] = 2; s.state.players[0].status_abilities[0] = 3;
    check(capped.restore(s) && !capped.legal(use()), "shared main+defensive+status acquisition cap excludes ninth first use"); reject(capped,use());
    check(capped.legal({ActionKind::cast_status,0,0,1}), "quota does not prevent already-retained status reuse");
    auto bad = s; ++bad.state.players[0].status_abilities[4]; check(!capped.restore(bad) && capped.snapshot() == s, "over-cap status count rejects atomically");
    auto full_cfg = minimum(); full_cfg.magic_reflection = full_cfg.reflection = true;
    auto full = fixture({120},{40,40},full_cfg); s = full.snapshot();
    s.state.players[0].hand.fill(120); s.state.players[0].hand_count = 16; s.state.players[0].status_abilities[3] = 1; check(full.restore(s), "minimum held status with full hand");
    draws(full); full.step({ActionKind::cast_status,3,0,1}); check(full.state().players[0].pending_deals == 1 && full.state().players[0].retained_deals == 1, "full hand cast reserves retained gift"); roundtrip(full);
    full.step(special(0)); roundtrip(full); full.step(accept()); roundtrip(full);
    auto exact_env = fixture({208},{196}); exact_env.step(use()); const auto exact = exact_env.snapshot();
    for (int error = 0; error < 9; ++error) {
        bad = exact;
        if (error == 0) bad.state.pending.amount = 1;
        if (error == 1) bad.state.pending.kind = Effect::damage;
        if (error == 2) bad.state.pending.harm = Harm::glory;
        if (error == 3) bad.state.pending.cure = Cure::all;
        if (error == 4) bad.state.pending.disease = Disease::cold;
        if (error == 5) bad.state.pending.element = Element::neutral;
        if (error == 6) bad.state.pending.miracle = 211;
        if (error == 7) bad.state.pending.harm_card = 70;
        if (error == 8) bad.state.players[0].status_abilities.fill(0), bad.state.pending.from_ability = true;
        check(!exact_env.restore(bad) && exact_env.snapshot() == exact, "exact status provenance rejects altered amount, type, payload, element, source or holding atomically");
    }
}

void forgetting_market_privacy_and_replay() {
    for (std::size_t rank = 0; rank < status_retained_type_ids.size(); ++rank) {
        auto env = fixture({241},{120}); auto s = env.snapshot(); for (CardId id : status_retained_type_ids) set(s.state.players[1],id,1);
        check(env.restore(s), "restore27 ascending retained families"); const std::uint64_t ticket[] = {rank}; check(env.script_forget_tickets(ticket), "select weighted status-inclusive rank");
        env.step(use()); const auto r = env.step(accept()); check(r.forget_population == 27 && r.forgotten_miracle == status_retained_type_ids[rank],
            "forget distribution includes new status types in canonical ID order without changing old lists"); roundtrip(env);
    }
    auto forget = fixture({241,212,239},{120}); auto s = forget.snapshot(); s.state.players[0].status_abilities[3] = 1; check(forget.restore(s), "restore owned status to forget");
    const std::uint64_t zero[] = {0}; check(forget.script_forget_tickets(zero), "forget final new status copy"); forget.step(use(0));
    check(!forget.state().players[0].status_abilities[3], "forget removes final separate status count"); roundtrip(forget);
    for (const auto& m : status_miracles()) {
        check(sale_price(m.id) == -1 && sale_price_count == 216, "new status prices use profile fallback, old table frozen");
        auto sale = fixture({3,m.id},{120}); draws(sale); sale.step({ActionKind::sell,0,2,1}); const auto sold = sale.step(accept());
        check(sold.sale_delivered && sold.sale_price == m.price && !sold.status_first_use && !sold.status_resolved
            && sale.state().players[1].status_abilities == std::array<std::uint32_t,5>{}, "selling physical status never acquires, pays MP or activates it"); roundtrip(sale);
        auto buy = fixture({4},{m.id}); draws(buy); const std::uint64_t ticket[] = {0}; check(buy.script_buy_tickets(ticket), "select status commodity");
        buy.step({ActionKind::buy,0,0,1}); const auto offer = buy.step(accept()); check(offer.buy_price == m.price && buy.state().phase == Phase::purchase, "status price fallback supports purchase");
        const auto bought = buy.step({ActionKind::buy_accept}); check(bought.buy_delivered && !bought.status_first_use && !bought.status_resolved
            && buy.state().players[0].status_abilities == std::array<std::uint32_t,5>{}, "purchase changes hand only, no new ability or payload"); roundtrip(buy);
    }
    auto old_cfg = config; old_cfg.status_miracles = false; auto old = fixture({120},{120},old_cfg), current = fixture({120},{120});
    const auto old_obs = old.observe(), new_obs = current.observe();
    check(std::equal(old_obs.begin(),old_obs.begin()+1317,new_obs.begin()), "new empty profile preserves all old observation features"); check(std::all_of(old_obs.begin()+1317,old_obs.end(),[](float x){return x == 0.f;}), "disabled status suffix allzero");
    const auto old_state = old.snapshot(); const CardId ids[] = {202,208,211,212,224}; check(!old.script_draws(ids) && old.snapshot() == old_state, "old pool rejects new status cards atomically");
    reject(old,{ActionKind::cast_status,0,0,1});
    auto privacy = fixture({120},{212}); s = privacy.snapshot(); s.state.players[1].status_abilities[0] = 7; s.state.players[1].hand[0] = 224;
    const auto obs = privacy.observe(); check(privacy.restore(s) && privacy.observe() == obs, "enemy physical identities and retained status counts remain private");
    auto own = fixture({212},{120}); s = own.snapshot(); s.state.players[0].status_abilities[3] = 3; check(own.restore(s), "restore own status count features");
    check(own.observe()[1317] == 1.f && own.observe()[1318] == 4.f/5 && own.observe()[1337] == 3.f/config.max_decisions
        && own.observe()[1016] == 3.f/(16.f*config.max_decisions), "new hand/count features and old own total include new holdings");
    std::uint64_t uses = 0, resolved = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env x(config), y(config); x.reset(seed); y.reset(seed); std::uint64_t px = seed+360000, py = px;
        while (!x.done()) {
            check(x.snapshot() == y.snapshot() && x.observe() == y.observe(), "status seeded snapshots and private observations replay");
            for (float value : x.observe()) check(std::isfinite(value), "status observations finite");
            const auto a = x.sample_action(px), b = y.sample_action(py); check(a == b && px == py && x.legal(a), "new standalone/FREE status sampler deterministic and legal");
            const auto rx = x.step(a), ry = y.step(b); check(rx.valid && ry.valid && rx.rewards == ry.rewards && rx.status_first_use == ry.status_first_use
                && rx.status_recast == ry.status_recast && rx.status_resolved == ry.status_resolved, "new status event and reward replay exact");
            uses += rx.status_first_use + rx.status_recast; resolved += rx.status_resolved; roundtrip(x);
        }
    }
    check(uses && resolved, "natural profile and policy actually use and resolve new statuses");
}
}
int main() {
    metadata_minimum_first_and_recast(); cure_rank_heaven_and_owner_tick(); stage_prefix_free_and_provisional_zero();
    free_refs_capacity_and_restore(); forgetting_market_privacy_and_replay();
    std::cout << "Status miracles passed (five separate holdings/typed zeroMAGIC/cures/Heaven/FREE/forget/market/privacy,1000 replay rollouts).\n";
}
