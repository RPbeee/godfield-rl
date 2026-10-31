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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL magic free: " << message << '\n'; std::exit(1); } }
Action row(ActionKind kind, std::initializer_list<std::uint8_t> refs, std::uint8_t base = 0, std::uint8_t target = 1) {
    Action a{kind,base,0,target}; a.extra_count = static_cast<std::uint8_t>(refs.size()); std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
Action attack(std::initializer_list<std::uint8_t> refs) { return row(ActionKind::attack_ordered,refs); }
Action cast(std::uint8_t base, std::initializer_list<std::uint8_t> refs) { return row(ActionKind::cast_ordered,refs,base); }
Action defense(std::initializer_list<std::uint8_t> refs) { return row(ActionKind::defend_ordered,refs,no_slot,no_slot); }
Action accept() { return {ActionKind::defend,no_slot,0,no_slot}; }
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, int mp = 10, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy); s.state.players[0].mp = mp;
    check(env.restore(s), "restore explicit FREE fixture"); return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "FREE ownership/trace/large counters restore exactly"); }
void reject(Env& env, Action a) { const auto old = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == old, "bad FREE adjacency/reference/cost/type is atomic"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "script overflow gift prefix"); }
void away(Env& env) { const std::uint8_t seat[] = {static_cast<std::uint8_t>(1-env.state().actor)}; check(env.script_bounce_targets(seat), "force next bounce to the other seat"); }
Env incoming(std::initializer_list<CardId> defender, int mp = 0) {
    auto env = fixture({216},defender); auto s = env.snapshot(); s.state.players[1].mp = mp; s.state.players[1].defensive_abilities[0] = 1;
    check(env.restore(s), "restore held210 responder"); env.step({ActionKind::use,0,0,1}); return env;
}
void catalog_roles_and_main_casts() {
    check(magic_free_card_count == 5 && magic_free_catalog().size() == 5 && static_cast<int>(Kind::sundry) == 4,
        "five FREE artifacts and explicit SUNDRY kind");
    constexpr std::array<CardId,5> ids{47,153,172,180,239}; constexpr std::array<int,5> dfs{0,6,9,12,0};
    for (int i = 0; i < 5; ++i) {
        const auto* item = find_magic_free_card(ids[i]);
        check(item && magic_free_index(ids[i]) == i && item->card.attack == (i ? 0 : 12) && item->card.defense == dfs[i]
            && item->card.boost == 0 && item->card.weight == (i == 4 ? 2 : 1) && item->card.price == (i == 4 ? 5 : 20)
            && item->element == Element::neutral, "FREE catalog fields and local ordering exact");
    }
    auto weapon = fixture({47}, {120}); check(weapon.step({ActionKind::attack,0,0,1}).valid && weapon.state().pending.amount == 12,
        "FREE weapon in first position remains ordinary ATK12"); roundtrip(weapon);
    auto armor = fixture({36}, {153}); armor.step({ActionKind::attack,0,0,1}); const auto ordinary = armor.step(defense({0}));
    check(ordinary.ordered_defense && ordinary.magic_free_cards == 1 && !ordinary.magic_free_mp_saved
        && armor.state().players[1].hp == 36, "FREE armor is ordinary DEF6 when no MAGIC precedes it"); roundtrip(armor);
    auto free = fixture({197,239}, {171},0); const auto result = free.step(cast(0,{1}));
    check(result.valid && result.free_cast && result.magic_free_cards == 1 && result.magic_free_mp_saved == 2
        && result.additive_first_uses == 1 && !result.additive_mp_spent && free.state().players[0].mp == 0
        && free.state().pending.miracle == 197 && free.state().pending.free_card == 239
        && free.state().free_cast.magic == 197 && free.state().free_cast.count == 1,
        "hand MAGIC can be acquired and cast at0MP with consumed FREE suffix"); roundtrip(free);
    const auto frozen = free.state().free_cast; free.step({ActionKind::defend,0,0,no_slot});
    check(free.state().free_cast == frozen && free.state().pending.free_card == 239 && free.state().players[0].mp == 0,
        "magic reflection preserves free-cast trace and does not pay or reacquire"); roundtrip(free);
    auto held = fixture({239}, {120},0); auto s = held.snapshot(); s.state.players[0].abilities[0] = 1;
    s.state.players[0].hand.fill(239); s.state.players[0].hand_count = 16; check(held.restore(s), "restore held216 and16 FREE cards"); draws(held);
    auto all = cast(32,{}); all.extra_count = 16; for (int i = 0; i < 16; ++i) all.extras[i] = static_cast<std::uint8_t>(i);
    const auto full = held.step(all);
    check(full.free_cast && full.magic_free_cards == 16 && full.magic_free_mp_saved == 6 && full.spell_recast && !full.spell_mp_spent
        && held.state().players[0].pending_deals == 17 && held.state().players[0].retained_deals == 1
        && held.state().free_cast.count == 16 && held.state().free_cast.from_ability,
        "held base leaves all16 physical slots for meaningful consumed FREE extras and17 gifts"); roundtrip(held);
    held.step(accept()); check(held.snapshot().draw_index == 17 && held.state().players[0].hand_count == 16,
        "main held free-cast distributes17 outcomes including overflow"); roundtrip(held);
    auto bad = fixture({214,239}, {120},0); reject(bad,cast(0,{1}));
    auto bad_ref = fixture({197,120}, {120},0); reject(bad_ref,cast(0,{1})); reject(bad_ref,cast(16,{1}));
    auto old_cfg = config; old_cfg.magic_free = false; Env old(old_cfg); const auto before = old.snapshot();
    check(!old.script_draws(ids) && old.snapshot() == before, "old profiles exclude all FREE artifacts");
}
void sixty_five_extras_and_magical_order() {
    auto env = fixture({56}, {120},99); auto s = env.snapshot(); auto& p = s.state.players[0];
    p.hand.fill(170); p.hand[0] = 56; p.hand[15] = 239; p.hand_count = 16; p.additive_abilities[0] = 1;
    check(env.restore(s), "restore maximal component count"); draws(env);
    auto a = attack({}); a.extra_count = 65;
    for (int i = 0; i < 14; ++i) a.extras[i] = static_cast<std::uint8_t>(i+1);
    a.extras[14] = 16; a.extras[15] = 15; std::fill(a.extras.begin()+16,a.extras.end(),16);
    const auto rng = env.snapshot().rng_state; const auto result = env.step(a);
    check(result.valid && result.additive_recasts == 50 && result.additive_mp_spent == 98 && result.magic_free_mp_saved == 2
        && result.magic_free_cards == 1 && env.state().pending.amount == 340 && env.state().players[0].mp == 1
        && env.state().players[0].pending_deals == 66 && env.state().players[0].retained_deals == 50
        && env.state().attack_composition.items[64] == 197 && env.state().attack_composition.held_tail,
        "65th ref has its own held bit;50 casts plus16 physical uses yield66 gifts"); roundtrip(env);
    check(env.observe()[966] == 197/363.f && env.observe()[967] == 1.f, "65th public trace ID and bit represented without shift64");
    env.step(accept()); check(env.snapshot().draw_index == 64 && env.snapshot().rng_state != rng
        && env.state().players[0].hand_count == 16, "66-gift overflow resumes seeded draws after64 prefix"); roundtrip(env);
    for (bool doubled : {false,true}) {
        auto magic = fixture({57,44,static_cast<CardId>(doubled ? 214 : 197),239}, {120},10);
        const auto value = magic.step(attack({1,2,3}));
        check(value.mana_weapon_power == (doubled ? 62 : 33) && value.mana_weapon_mp_spent == 10
            && value.magic_free_mp_saved == (doubled ? 6 : 2) && magic.state().players[0].mp == 0,
            "free MAGIC does not reset MAGICAL and preserves preceding boost"); roundtrip(magic);
    }
    auto mana = fixture({57}, {120},99); s = mana.snapshot(); auto& mp = s.state.players[0];
    mp.hand.fill(170); mp.hand[0] = 57; mp.hand[15] = 239; mp.hand_count = 16; mp.doubling_ability = 1;
    check(mana.restore(s), "restore strongest MAGICAL free-double order"); a = attack({18}); a.extra_count = 17;
    for (int i = 0; i < 14; ++i) a.extras[i+1] = static_cast<std::uint8_t>(i+1);
    a.extras[15] = 18; a.extras[16] = 15;
    const auto power = mana.step(a);
    check(power.mana_weapon_power == 1164 && power.mana_weapon_mp_spent == 93 && power.doubling_recasts == 2
        && power.doubling_mp_spent == 6 && power.magic_free_mp_saved == 6,
        "paid214 then14 boosts then free214 reaches1164, exceeding earlier954/969 estimates"); roundtrip(mana);
    auto raw = fixture({56}, {120},99); s = raw.snapshot(); auto& rp = s.state.players[0];
    rp.hand.fill(170); rp.hand[0] = 56; rp.hand[15] = 239; rp.hand_count = 16; rp.additive_abilities[0] = 1; rp.doubling_ability = 1;
    check(raw.restore(s), "restore seventeen-doubling maximum"); a = attack({}); a.extra_count = 33;
    for (int i = 0; i < 14; ++i) a.extras[i] = static_cast<std::uint8_t>(i+1);
    a.extras[14] = 16; a.extras[15] = 18; a.extras[16] = 15; std::fill_n(a.extras.begin()+17,16,18);
    raw.step(a); check(raw.state().pending.amount == 31719424 && raw.state().pending.amount > doubling_damage_limit,
        "one free plus16 paid multipliers exceed previous conservative bound and stay within new bound"); roundtrip(raw);
}
void defense_suffix_prefix_precedence_and_glory() {
    auto env = incoming({210,239}); away(env); const auto first = env.step(defense({0,1}));
    check(first.valid && first.ordered_defense && first.magic_free_suffix && !first.magic_free_cards && first.magic_free_mp_saved == 5
        && !first.spell_mp_spent && env.state().players[1].defensive_abilities[0] == 2
        && env.state().players[1].hand_count == 1 && env.state().players[1].hand[0] == 239
        && env.state().last_reaction.free_card == 239 && !env.state().last_reaction.from_ability,
        "hand210 is acquired for free while its causal suffix remains unconsumed"); roundtrip(env);
    auto prefix = incoming({153,239}); away(prefix); const auto used = prefix.step(defense({0,20,1}));
    check(used.valid && used.magic_free_mp_saved == 5 && used.magic_free_cards == 1 && used.magic_free_suffix
        && prefix.state().players[1].hand_count == 1 && prefix.state().players[1].hand[0] == 239,
        "FREE at index0 is excluded from prepass; later suffix can waive held210"); roundtrip(prefix);
    auto blocked = incoming({153,172,239}); reject(blocked,defense({0,1,20,2}));
    auto paid = incoming({153,172,239},5); away(paid); const auto wasted = paid.step(defense({0,1,20,2}));
    check(wasted.valid && !wasted.magic_free_mp_saved && wasted.spell_mp_spent == 5
        && paid.state().players[1].mp == 0 && paid.state().last_reaction.free_card == 0,
        "first later FREE grants preceding ordinary piece; later suffix cannot waive210"); roundtrip(paid);
    auto noncanonical = incoming({239,239}); reject(noncanonical,defense({20,0,1})); reject(noncanonical,defense({0,20,0}));
    auto glory = incoming({239},5); auto s = glory.snapshot(); s.state.players[1].glory = true;
    check(glory.restore(s), "restore Glory selection limit"); reject(glory,defense({20,0})); away(glory);
    check(glory.step(defense({20})).valid && glory.state().players[1].mp == 0,
        "Glory counts causal unused FREE selection; single paid held210 remains allowed"); roundtrip(glory);
    auto counter = incoming({190,239}); away(counter);
    check(counter.step(defense({0,20,1})).valid && !counter.state().counter_generated,
        "counter prefix is consumed but not reserved when free special fires"); roundtrip(counter);
    auto heaven = incoming({175,239}); s = heaven.snapshot(); s.state.players[1].disease = Disease::heaven;
    check(heaven.restore(s), "restore Heaven prefix before free210"); const std::uint8_t self[] = {1};
    check(heaven.script_bounce_targets(self), "bounce heal back to provisional HP0 responder");
    const auto rescue = heaven.step(defense({0,20,1}));
    check(rescue.valid && rescue.bounce_self && rescue.magic_free_mp_saved == 5 && heaven.state().players[1].hp == 10
        && !heaven.state().players[1].dead, "free self-bounce rescues Heaven prefix HP0 before finalization"); roundtrip(heaven);
    auto weapon = fixture({5}, {239}); s = weapon.snapshot(); s.state.players[1].mp = 0; s.state.players[1].defensive_abilities[1] = 1;
    check(weapon.restore(s), "restore free held215"); weapon.step({ActionKind::attack,0,0,1});
    const auto stop = weapon.step(defense({21,0}));
    check(stop.valid && stop.weapon_blocked && stop.magic_free_suffix && stop.magic_free_mp_saved == 6
        && !stop.spell_mp_spent && weapon.state().last_reaction.free_card == 239,
        "215 can block at0MP with unconsumed FREE suffix"); roundtrip(weapon);
}
void long_free_response_chain_and_large_distribution() {
    constexpr std::uint32_t responses = 131074;
    auto cfg = config; cfg.max_decisions = responses+2;
    auto env = fixture({239,239}, {239},0,cfg); auto s = env.snapshot();
    s.state.players[0].abilities[0] = 1;
    for (auto& p : s.state.players) { p.defensive_abilities[0] = 1; p.mp = 0; }
    check(env.restore(s), "restore zero-MP reusable FREE chain"); draws(env);
    check(env.step(cast(32,{0})).valid, "start free held216 with one FREE left in each hand");
    for (std::uint32_t i = 0; i < responses; ++i) {
        away(env); const auto r = env.step(defense({20,0}));
        check(r.valid && r.magic_bounced && r.magic_free_suffix && !r.magic_free_cards && r.magic_free_mp_saved == 5 && !r.spell_mp_spent,
            "reusable unconsumed FREE suffix supports each0MP held210 response");
        if (i == 65534 || i == 65535) roundtrip(env);
    }
    check(env.state().players[0].retained_deals > 65535 && env.state().players[1].retained_deals > 65535
        && env.state().players[0].pending_deals > 65535 && env.state().reaction_count == responses
        && env.snapshot().draw_index == 0 && env.state().players[0].hand[0] == 239 && env.state().players[1].hand[0] == 239,
        "both seats pass uint16 boundary through actual play without consumption or early gifts"); roundtrip(env);
    Env truncated(cfg); check(truncated.restore(env.snapshot()), "fork long pre-limit snapshot"); away(truncated);
    check(truncated.step(defense({20,0})).valid && truncated.state().truncated && truncated.state().phase == Phase::response
        && truncated.snapshot().draw_index == 0, "decision limit freezes large unresolved chain without artificial chain cap"); roundtrip(truncated);
    check(env.step(accept()).valid && env.state().truncated && env.state().phase == Phase::main
        && env.state().players[0].hand_count == 16 && env.state().players[1].hand_count == 16
        && !env.state().players[0].pending_deals && !env.state().players[1].retained_deals && env.snapshot().draw_index == 64,
        "accept resolves and distributes every large overflow outcome before main-phase truncation"); roundtrip(env);
}
void free_cast_mp_history_and_uint32_limit() {
    auto env = fixture({239,239}, {239},10); auto s = env.snapshot();
    s.state.players[0].abilities[0] = 1;
    for (auto& p : s.state.players) p.defensive_abilities[0] = 1;
    s.state.players[1].mp = 0; check(env.restore(s), "restore free main cast with owner10MP");
    env.step(cast(32,{0})); away(env); env.step(defense({20,0})); const auto first = env.snapshot();
    auto bad = first; bad.state.players[0].mp = 9;
    check(!env.restore(bad) && env.snapshot() == first, "R1 owner has not responded and cannot spend even1MP");
    bad = first; bad.state.pending.attribute_removed = true;
    check(!env.restore(bad) && env.snapshot() == first, "initial FREE gift cannot masquerade as195 removal consumption");
    Env paid(config), free(config); check(paid.restore(first) && free.restore(first), "fork paid and free owner response");
    away(paid); check(paid.step(defense({20})).spell_mp_spent == 5 && paid.state().players[0].mp == 5,
        "R2 owner paid210 decreases exactly5MP"); roundtrip(paid);
    away(free); check(free.step(defense({20,0})).magic_free_mp_saved == 5 && free.state().players[0].mp == 10,
        "R2 free210 preserves initial ownerMP"); roundtrip(free);
    const auto second = free.snapshot(); bad = second; bad.state.players[0].mp = 0;
    check(!free.restore(bad) && free.snapshot() == second, "one owner response cannot pay10MP");
    auto weapon = fixture({5,197,239}, {39},0); weapon.step(attack({1,2})); weapon.step({ActionKind::defend,0,0,no_slot});
    const auto weapon_exact = weapon.snapshot(); bad = weapon_exact; bad.state.pending.attribute_removed = true;
    check(!weapon.restore(bad) && weapon.snapshot() == weapon_exact,
        "ordered attack's initial physical components cannot replace a later195 charge");
    auto limit_cfg = config; limit_cfg.max_decisions = std::numeric_limits<std::uint32_t>::max(); Env limit(limit_cfg);
    auto maximal = first; maximal.config = limit_cfg; auto& state = maximal.state;
    state.decisions = limit_cfg.max_decisions; state.reaction_count = limit_cfg.max_decisions-1; state.truncated = true;
    state.actor = 1; state.pending.source = 0; state.pending.target = 1; state.last_reaction.source = 0; state.last_reaction.target = 1;
    state.players[0].retained_deals = 1ULL + state.reaction_count/2;
    state.players[1].retained_deals = (std::uint64_t{state.reaction_count}+1)/2;
    state.players[0].pending_deals = state.players[0].retained_deals+1;
    state.players[1].pending_deals = state.players[1].retained_deals;
    check(limit.restore(maximal), "UINT32_MAX decisions and R=MAX-1 fit uint64 pending sums without wrap"); roundtrip(limit);
    const auto exact = limit.snapshot(); bad = exact; bad.state.pending.attribute_removed = true;
    check(!limit.restore(bad) && limit.snapshot() == exact, "R+2/removal budget remains uint64 at maximum decisions");
    Config minimum; minimum.miracles = minimum.elements = minimum.chance = minimum.harms = minimum.celestial = minimum.additive_miracles = minimum.magic_free = true;
    auto narrow = fixture({5,197,239}, {120},0,minimum);
    check(narrow.step(attack({1,2})).magic_free_mp_saved == 2 && narrow.state().pending.amount == 3,
        "minimum dependencies support FREE without doubling/wide/defensive/reflection flags"); roundtrip(narrow);
}
void strictness_privacy_and_seeded_replay() {
    auto env = fixture({197,239}, {120},0); env.step(cast(0,{1})); const auto exact = env.snapshot();
    for (int error = 0; error < 8; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.pending.free_card = 0;
        if (error == 1) bad.state.free_cast.magic = 217;
        if (error == 2) bad.state.free_cast.items[0] = 120;
        if (error == 3) bad.state.free_cast.items[1] = 239;
        if (error == 4) bad.state.free_cast.count = 17;
        if (error == 5) bad.state.players[0].mp = 1;
        if (error == 6) bad.config.magic_free = false;
        if (error == 7) bad.state.pending.free_card = 214;
        check(!env.restore(bad) && env.snapshot() == exact, "free main-cast trace/payment/profile tamper rejects atomically");
    }
    auto own = fixture({239}, {120}); check(own.observe()[945] == 1.f && own.observe()[946] == 1.f, "FREE flag and local fifth-kind hand scalar");
    auto hidden = fixture({5}, {239}); const auto obs = hidden.observe(); auto changed = hidden.snapshot(); changed.state.players[1].hand[0] = 120;
    check(hidden.restore(changed) && hidden.observe() == obs, "opponent unplayed FREE remains private");
    auto old_cfg = config; old_cfg.magic_free = false; Env old(old_cfg); const auto oldobs = old.observe();
    check(std::all_of(oldobs.begin()+945,oldobs.end(),[](float x){return x == 0.f;}), "old profiles FREE suffix zero");
    std::uint64_t free_cards = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+111000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "FREE seeded state/observation replay");
            for (float x : a.observe()) check(std::isfinite(x), "FREE observations finite");
            const auto action = a.sample_action(pa), twin = b.sample_action(pb);
            check(action == twin && pa == pb && a.legal(action), "FREE sampler selects complete legal ordered action");
            const auto ra = a.step(action), rb = b.step(twin);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.magic_free_cards == rb.magic_free_cards
                && ra.magic_free_mp_saved == rb.magic_free_mp_saved && ra.magic_free_suffix == rb.magic_free_suffix,
                "FREE cost/consumption/unconsumed suffix/reward replay"); free_cards += ra.magic_free_cards; roundtrip(a);
        }
    }
    check(free_cards, "natural pool exercises new FREE cards");
}
}
int main() {
    catalog_roles_and_main_casts(); sixty_five_extras_and_magical_order(); defense_suffix_prefix_precedence_and_glory();
    long_free_response_chain_and_large_distribution(); free_cast_mp_history_and_uint32_limit(); strictness_privacy_and_seeded_replay();
    std::cout << "Magic free passed (65refs/66gifts/order/unconsumedsuffix/131074chain,1000 replay rollouts).\n";
}
