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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = c.revival = c.item_removal = c.hp_absorption = c.self_damage = c.pestle_mortar = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL pestle/mortar: " << message << '\n'; std::exit(1); } }
Action attack(std::uint8_t target = 1, std::uint16_t slots = 0) { return {ActionKind::attack,0,slots,target}; }
Action accept(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action counter(std::uint16_t mask, std::initializer_list<int> order) {
    auto a = accept(mask); for (int slot : order) a.counter_order |= std::uint64_t(slot)<<(4*a.counter_count++); return a;
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy);
    check(env.restore(s), "restore explicit pestle/mortar fixture"); return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "pestle origin/target/marker/tape/history roundtrip exactly"); }
void reject(Env& env, Action a) { const auto before = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == before, "noncanonical pestle/mortar action rejects atomically"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "isolate earned gifts from target selection"); }
void destination(Env& env, std::uint8_t seat) { const std::uint8_t seats[] = {seat}; check(env.script_pestle_targets(seats), "script private no-mortar target"); }
Config minimum() { Config c; c.elements = c.chance = c.harms = c.celestial = c.pestle_mortar = true; return c; }

void metadata_minimum_and_target_selection() {
    const auto* pestle = find_pestle_card(99); const auto* mortar = find_mortar_card(245);
    check(pestle_card_count == 1 && mortar_card_count == 1 && pestle_catalog().size() == 1 && mortar_catalog().size() == 1
        && pestle_index(99) == 0 && mortar_index(245) == 0 && pestle && mortar,
        "strict one-pestle/one-mortar catalog and local indices");
    check(pestle->element == Element::light && pestle->mortar_amount == 999 && pestle->card.kind == Kind::weapon
        && pestle->card.attack == 30 && !pestle->card.boost && !pestle->card.defense && !pestle->card.hp && !pestle->card.mp
        && pestle->card.price == 5 && pestle->card.weight == 1 && !pestle->card.discardable,
        "99 is guaranteed standalone Light30, boosted to999 only by the mortar selection");
    check(mortar->card.kind == Kind::sundry && !mortar->card.attack && !mortar->card.boost && !mortar->card.defense
        && !mortar->card.hp && !mortar->card.mp && !mortar->card.price && mortar->card.weight == 1 && !mortar->card.discardable,
        "245 has no manual combat/resource role or ordinary discard effect");
    for (CardId id : {CardId{0},CardId{53},CardId{98},CardId{244},CardId{65535}})
        check(!find_pestle_card(id) && !find_mortar_card(id), "no neighboring or other special inclusion");
    for (std::uint8_t chosen = 0; chosen < 2; ++chosen) for (std::uint8_t requested = 0; requested < 2; ++requested) {
        auto env = fixture({99},{120},minimum()); auto s = env.snapshot(); s.state.players[0].mp = 0; check(env.restore(s), "restore no-cost minimum pestle");
        draws(env); destination(env,chosen); const auto rng = env.snapshot().rng_state;
        check(std::string_view(env.profile()) == pestle_mortar_ruleset && env.snapshot().version == 35
            && env.options().weapon_bases == 1 && env.options().pestle_slots == 1 && !env.options().boost_slots && !env.options().pray,
            "minimum profile exposes standalone weapon without a miracle dependency");
        const auto r = env.step(attack(requested)); check(r.pestle_attack && r.pestle_target == chosen && !r.pestle_mortar && r.pestle_damage == 30
            && env.snapshot().pestle_target_index == 1 && env.snapshot().rng_state == rng && !r.chance_attempt
            && env.state().last_pestle.id == 99 && env.state().last_pestle.target == chosen && !env.state().last_pestle.mortar,
            "no-mortar target overrides either requested seat, consumes exactly one target tape and no hit roll");
        if (chosen == 0) check(r.pestle_resolved && r.pestle_residual == 30 && r.pestle_target_lost == 30
            && env.state().players[0].hp == 10 && env.snapshot().draw_index == 1, "random self target auto-resolves and earns only original gift");
        else {
            check(!r.pestle_resolved && env.state().pending.pestle_card == 99 && !env.state().pending.mortar_card
                && env.state().pending.amount == 30 && env.state().pending.element == Element::light,
                "random opponent target waits for an ordinary server response"); roundtrip(env);
            check(env.step(accept()).pestle_residual == 30 && env.state().players[1].hp == 10, "unlocked ordinary30 survives response path");
        }
        roundtrip(env);
    }
    auto singleton = fixture({99},{120},minimum()); draws(singleton); const auto rng = singleton.snapshot().rng_state;
    singleton.step(attack()); check(singleton.snapshot().rng_state != rng, "default no-mortar selection consumes regular RNG"); roundtrip(singleton);
    auto old_cfg = config; old_cfg.pestle_mortar = false; Env old(old_cfg); const auto before = old.snapshot();
    for (CardId id : {CardId{99},CardId{245}}) { const CardId ids[] = {id}; check(!old.script_draws(ids) && old.snapshot() == before, "old29 pools exclude99/245"); }
    const std::uint8_t target[] = {0}; check(!old.script_pestle_targets(target) && old.snapshot() == before && old.script_pestle_targets({}),
        "disabled profile rejects nonempty target tape but permits empty clear");
}

void fixed_seat_search_duplicate_mortars_and_tape() {
    for (int placement = 0; placement < 3; ++placement) {
        auto env = placement == 0 ? fixture({99,245},{120},minimum())
            : placement == 1 ? fixture({99},{245},minimum()) : fixture({99,245,245},{245},minimum());
        draws(env); destination(env,1); const auto rng = env.snapshot().rng_state; const auto r = env.step(attack(1));
        const int chosen = placement == 1 ? 1 : 0;
        check(r.pestle_mortar && r.pestle_target == chosen && r.pestle_damage == 999 && env.state().last_pestle.mortar == 245
            && env.snapshot().pestle_target_index == 0 && env.snapshot().rng_state == rng,
            "first245 holder in fixed seat order wins; locked target consumes no tape/RNG");
        if (chosen == 0) check(r.pestle_resolved && env.done() && env.state().winner == 1
            && env.state().players[0].hand[0] == 245 && env.state().players[0].hand_count == (placement == 2 ? 2 : 1),
            "self999 is immediate and mortar copies are never consumed");
        else {
            check(env.state().phase == Phase::response && env.state().pending.pestle_card == 99 && env.state().pending.mortar_card == 245
                && env.state().pending.amount == 999 && env.state().players[1].hand[0] == 245,
                "opponent999 retains a response phase rather than assuming client no-defense behavior"); roundtrip(env); env.step(accept());
        }
        roundtrip(env);
    }
    auto second_owner = fixture({245},{99,245}); auto s = second_owner.snapshot(); s.state.order = {1,0}; s.state.actor = s.state.turn_owner = 1;
    check(second_owner.restore(s), "restore seat1 owner with mortar in both seats"); const auto r = second_owner.step(attack(1));
    check(r.pestle_target == 0 && second_owner.state().actor == 0 && second_owner.state().pending.amount == 999,
        "fixed participating-seat order is independent of current owner/order"); roundtrip(second_owner);
    auto exhausted = fixture({99},{99}); draws(exhausted); destination(exhausted,1); exhausted.step(attack()); exhausted.step(accept());
    Env fallback(config); check(fallback.restore(exhausted.snapshot()) && fallback.script_pestle_targets({}), "restore ordinary RNG control after prefix exhaustion");
    const auto a = exhausted.step(attack(0)), b = fallback.step(attack(0));
    check(a.pestle_target == b.pestle_target && exhausted.state() == fallback.state() && exhausted.snapshot().rng_state == fallback.snapshot().rng_state
        && exhausted.snapshot().pestle_target_index == 1, "exhausted target prefix resumes RNG without reusing last seat"); roundtrip(exhausted);
}

void discard_skip_and_forced_removal_history() {
    auto noop = fixture({245},{120}); draws(noop); const auto before = noop.snapshot();
    check(noop.options().mortar_slots == 1 && noop.options().discard_slots == 1 && noop.options().pray && !noop.options().utility_slots,
        "245 may be selected for source-compatible discard even though actual removal is skipped"); reject(noop,{ActionKind::use,0,0,0}); reject(noop,attack());
    const auto r = noop.step({ActionKind::discard,no_slot,1,no_slot}); check(r.valid && r.mortar_discards_ignored == 1
        && noop.state().players[0].hand_count == 1 && noop.state().players[0].hand[0] == 245 && noop.state().turns == before.state.turns+1
        && noop.state().decisions == before.state.decisions+1 && noop.snapshot().draw_index == 0,
        "all-mortar discard is a legal turn-ending no-op with no gift"); roundtrip(noop);
    auto mixed = fixture({245,120,245},{120}); draws(mixed); const auto m = mixed.step({ActionKind::discard,no_slot,7,no_slot});
    check(m.mortar_discards_ignored == 2 && mixed.state().players[0].hand_count == 2 && mixed.state().players[0].hand[0] == 245
        && mixed.state().players[0].hand[1] == 245 && !mixed.snapshot().draw_index, "mixed discard consumes only non-mortar cards without compensating gifts"); roundtrip(mixed);
    auto tick = fixture({245},{120}); auto s = tick.snapshot(); s.state.players[0].hp = 10; s.state.players[0].disease = Disease::hell;
    check(tick.restore(s), "restore no-op discard owner tick"); const std::uint8_t roll[] = {99}; check(tick.script_disease_rolls(roll), "avoid Hell worsening");
    check(tick.step({ActionKind::discard,no_slot,1,no_slot}).disease_tick && tick.state().players[0].hp == 5 && tick.state().players[0].hand[0] == 245,
        "ignored discard still advances owner disease exactly once"); roundtrip(tick);
    auto history = fixture({99,240},{245,244}); draws(history); history.step(attack()); history.step(accept());
    check(history.state().players[1].hp == 10 && history.state().players[1].hand[0] == 245 && history.state().last_pestle.mortar == 245,
        "mortar survives999 while244 revives its holder");
    for (int turns = 0; history.state().actor != 0; ++turns) { check(turns < 3, "reach owner0 in duel order"); check(history.step({ActionKind::pray}).valid, "pass harmless armor-only opposing turn"); }
    const std::uint64_t first[] = {0}; check(history.script_item_removal_tickets(first), "force first mortar physical slot");
    const CardId next[] = {99,120,120,120}; check(history.script_draws(next), "earned240 gift supplies future pestle");
    history.step({ActionKind::use,0,0,1}); const auto swept = history.step(accept());
    check(swept.removed_item == 245 && history.state().last_pestle.mortar == 245,
        "240 truly removes245 despite discard skip and leaves historical locked event valid"); roundtrip(history);
    for (int turns = 0; history.state().actor != 0; ++turns) { check(turns < 3, "reach later pestle turn"); check(history.step({ActionKind::pray}).valid, "pass after forced mortar removal"); }
    int base = 0; while (base < history.state().players[0].hand_count && history.state().players[0].hand[base] != 99) ++base;
    destination(history,1); const auto ordinary = history.step({ActionKind::attack,static_cast<std::uint8_t>(base),0,0});
    check(ordinary.pestle_attack && !ordinary.pestle_mortar && ordinary.pestle_damage == 30 && history.snapshot().pestle_target_index == 1,
        "next pestle after240 re-searches and becomes ordinary random30"); roundtrip(history);
}

void ordinary_responses_reactions_and_revival() {
    auto defended = fixture({99},{245,195,186,244}); draws(defended); defended.step(attack()); const auto r = defended.step(accept(6));
    check(r.pestle_resolved && r.pestle_residual == 969 && r.pestle_target_lost == 40 && r.revivals[1] == 1
        && defended.state().players[1].hp == 10 && defended.state().players[1].hand[0] == 245 && defended.snapshot().draw_index == 3,
        "server accepts195 plus ordinaryDEF30 against locked999; mortar survives and244 earns no gift"); roundtrip(defended);
    auto zero = fixture({99},{195,186}); destination(zero,1); zero.step(attack()); const auto stopped = zero.step(accept(3));
    check(stopped.pestle_resolved && stopped.pestle_residual == 0 && !stopped.pestle_target_lost && zero.state().players[1].hp == 40,
        "unlocked30 can be completely defended through195 and records zero residual"); roundtrip(zero);
    for (bool universal : {false,true}) {
        auto reflected = universal ? fixture({99,244},{245,196}) : fixture({99,244},{245,195,39}); draws(reflected); reflected.step(attack());
        reflected.step(universal ? special(1) : special(2,2));
        check(reflected.state().pending.pestle_card == 99 && reflected.state().pending.mortar_card == 245 && reflected.state().pending.amount == 999
            && reflected.state().pending.source == 1 && reflected.state().pending.target == 0 && reflected.state().last_pestle.source == 0
            && reflected.state().last_pestle.target == 1 && reflected.state().players[1].hand[0] == 245,
            "reflection keeps initial locked seat and999 while current source/target change"); roundtrip(reflected);
        const auto finish = reflected.step(accept()); check(finish.pestle_resolved_source == 1 && finish.pestle_resolved_target == 0
            && finish.pestle_residual == 999 && finish.revivals[0] == 1 && reflected.state().players[0].hp == 10,
            "returned999 neither re-searches mortar nor resets damage to30"); roundtrip(reflected);
    }
    auto block = fixture({99},{245,195,215,40,210,185}); block.step(attack());
    reject(block,special(2)); reject(block,special(3)); reject(block,special(4)); reject(block,special(5));
    const auto blocked = block.step(special(2,2)); check(blocked.weapon_blocked && !blocked.pestle_resolved
        && block.state().players[0].hp == 40 && block.state().players[1].hp == 40 && block.state().players[1].hand[0] == 245,
        "195 makes215 valid; matching WEAPON block cancels locked999 while MAGIC reactions remain invalid"); roundtrip(block);
    for (int copies : {1,2}) {
        auto env = copies == 1 ? fixture({99,245,244},{120}) : fixture({99,245,244,244},{120}); auto s = env.snapshot();
        s.state.players[0].disease = Disease::heaven; s.state.players[0].worse_chance = 100; check(env.restore(s), "restore self999 then owner Heaven tick"); draws(env);
        const std::uint8_t roll[] = {0}; check(env.script_disease_rolls(roll), "force one post-revival owner tick"); const auto result = env.step(attack());
        check(result.pestle_target == 0 && result.pestle_target_lost == 40 && result.revivals[0] == copies
            && result.disease_tick && env.snapshot().disease_index == 1 && env.state().players[0].hand[0] == 245
            && env.state().players[0].hp == (copies == 2 ? 10 : 0) && env.snapshot().draw_index == (copies == 2 ? 1 : 0),
            "self999 revival occurs before exactly one disease tick; only original99 gift can be drawn"); roundtrip(env);
    }
}

void counter_limits_dead_sources_auto_money_and_cancel() {
    auto cfg = minimum(); cfg.counters = cfg.attribute_removal = true;
    auto env = fixture({99},{245,195,190},cfg); env.step(attack()); const auto queued = env.step(accept(6));
    check(queued.counter_queued == 1 && env.state().pending.counter_card == 190 && env.state().pending.amount == 1998
        && !env.state().pending.pestle_card && !env.state().pending.mortar_card && env.state().players[1].dead,
        "minimal noMF/noDOUBLE profile permits999-derived dead-source1998 counter and drops pestle provenance"); roundtrip(env);
    env.step(accept()); check(env.done() && env.state().winner == -1, "dead source's large counter still executes"); roundtrip(env);
    auto old_cfg = cfg; old_cfg.pestle_mortar = false; auto old = fixture({56},{190},old_cfg); old.step(attack()); old.step(accept(1));
    const auto old_exact = old.snapshot(); auto inflated = old_exact; inflated.state.pending.amount = 1998;
    check(!old.restore(inflated) && old.snapshot() == old_exact, "old29 counter bound remains480-based");
    for (bool revive : {false,true}) {
        auto mana = revive ? fixture({99},{245,195,193,244}) : fixture({99},{245,195,193}); auto s = mana.snapshot(); s.state.players[1].mp = 0;
        check(mana.restore(s), "restore large autoMP counter"); mana.step(attack()); const auto result = mana.step(counter(6,{2}));
        check(result.counter_auto_resolved == revive && result.counter_skipped == !revive && result.counter_mp_gained == (revive ? 99 : 0)
            && mana.state().players[1].mp == (revive ? 99 : 0), "193 uses1998 but caps at99 only after source revival, otherwise dead-self skip"); roundtrip(mana);
    }
    for (bool revive : {false,true}) {
        auto money = revive ? fixture({99,244},{245,195,194,244}) : fixture({99,244},{245,195,194}); money.step(attack()); money.step(counter(6,{2})); roundtrip(money);
        const auto paid = money.step(accept()); check(paid.counter_money_gained == 79 && paid.counter_money_lost == 20
            && paid.counter_mp_lost == 10 && paid.counter_hp_lost == 40 && money.state().players[1].money == 99
            && paid.revivals[0] == revive && money.state().players[0].hp == (revive ? 10 : 0),
            "194 credits even dead source up to99 then charges target money/MP/HP; last-survivor244 remains conditional");
        if (!revive) check(money.state().winner == -1 && money.state().players[0].hand[0] == 244, "dead creditor causes last-survivor draw without consuming unusable244"); roundtrip(money);
    }
    auto cancel = fixture({99,196,244},{245,195,190}); cancel.step(attack()); cancel.step(counter(6,{2})); const auto returned = cancel.step(special(0));
    check(returned.counter_cancelled && !returned.counter_resolved && !returned.pestle_resolved && cancel.state().winner == 0
        && cancel.state().players[0].hp == 40 && cancel.state().players[0].hand[0] == 244,
        "196 returns1998 to dead counter origin and cancels without inheriting999 marker or spending244"); roundtrip(cancel);
}

void privacy_canonicality_restore_and_replay() {
    auto hidden_a = fixture({99},{245}), hidden_b = fixture({99},{120}); destination(hidden_a,0); destination(hidden_b,1);
    check(hidden_a.observe() == hidden_b.observe() && hidden_a.options() == hidden_b.options()
        && hidden_a.legal(attack(0)) && hidden_a.legal(attack(1)) && hidden_b.legal(attack(0)) && hidden_b.legal(attack(1)),
        "secret enemy mortar and future target tape cannot alter observation/options/legality before selection");
    auto extra = fixture({99,44,65,197},{120}); reject(extra,attack(1,2)); reject(extra,{ActionKind::attack_set_fire,0,4,1});
    Action ordered{ActionKind::attack_ordered,0,0,1}; ordered.extra_count = 1; ordered.extras[0] = 3; reject(extra,ordered);
    reject(extra,{ActionKind::use,0,0,1}); reject(extra,{ActionKind::attack,0,0,no_slot});
    const std::uint8_t invalid[] = {2}; const auto pre = extra.snapshot(); check(!extra.script_pestle_targets(invalid) && extra.snapshot() == pre, "invalid target seat tape rejects atomically");
    std::array<std::uint8_t,65> oversized{}; check(!extra.script_pestle_targets(oversized) && extra.snapshot() == pre, "oversized private tape rejects atomically");
    auto pending = fixture({99},{245}); pending.step(attack()); const auto exact = pending.snapshot();
    for (int error = 0; error < 10; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.pending.pestle_card = 0;
        if (error == 1) bad.state.pending.mortar_card = 99;
        if (error == 2) bad.state.pending.amount = 30;
        if (error == 3) bad.state.pending.hp_absorption_card = 26;
        if (error == 4) bad.state.pending.self_damage_card = 53;
        if (error == 5) bad.state.pending.miracle = 221;
        if (error == 6) bad.state.pending.hit_rate = 100;
        if (error == 7) bad.state.players[1].hand[0] = 120;
        if (error == 8) { bad.state.players[0].hand[0] = 245; bad.state.players[0].hand_count = 1; }
        if (error == 9) bad.state.last_pestle.target = 0;
        check(!pending.restore(bad) && pending.snapshot() == exact, "inconsistent999 payload or selected-mortar provenance rejects atomically");
    }
    pending.step(accept()); const auto history = pending.snapshot();
    for (int error = 0; error < 7; ++error) {
        auto bad = history;
        if (error == 0) bad.state.last_pestle.id = 98;
        if (error == 1) bad.state.last_pestle.source = no_slot;
        if (error == 2) bad.state.last_pestle.target = no_slot;
        if (error == 3) bad.state.last_pestle.mortar = 244;
        if (error == 4) bad.state.last_pestle.amount = 30;
        if (error == 5) bad.scripted_pestle_targets[bad.pestle_target_count] = 1;
        if (error == 6) bad.pestle_target_index = bad.pestle_target_count+1;
        check(!pending.restore(bad) && pending.snapshot() == history, "invalid historic locked selection or private tape rejects atomically");
    }
    auto old_cfg = config; old_cfg.pestle_mortar = false; auto old = fixture({120},{120},old_cfg), current = fixture({120},{120});
    const auto a = old.observe(), b = current.observe(); check(std::equal(a.begin(),a.begin()+1135,b.begin())
        && std::all_of(a.begin()+1135,a.end(),[](float x){return x == 0.f;}), "old29 prefix1135 and zero40 feature tail preserved");
    auto own = fixture({99,245},{120}); check(own.observe()[1135] == 1.f && own.observe()[1136] == 1.f && own.observe()[1153] == 1.f,
        "new flag and own hand99/245 offsets fixed");
    std::uint64_t attempts = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env x(config), y(config); x.reset(seed); y.reset(seed); std::uint64_t px = seed+300000, py = px;
        while (!x.done()) {
            check(x.snapshot() == y.snapshot() && x.observe() == y.observe(), "pestle target/response seeded replay");
            for (float value : x.observe()) check(std::isfinite(value), "999 and counter observations remain finite");
            const auto ax = x.sample_action(px), ay = y.sample_action(py); check(ax == ay && px == py && x.legal(ax), "pestle standalone/no-op discard sampler deterministic/legal");
            const auto rx = x.step(ax), ry = y.step(ay); check(rx.valid && ry.valid && rx.rewards == ry.rewards
                && rx.pestle_attack == ry.pestle_attack && rx.pestle_target == ry.pestle_target && rx.pestle_mortar == ry.pestle_mortar
                && rx.pestle_resolved == ry.pestle_resolved && rx.pestle_residual == ry.pestle_residual
                && rx.mortar_discards_ignored == ry.mortar_discards_ignored, "immutable selection and current resolution outcomes replay exactly");
            attempts += rx.pestle_attack; roundtrip(x);
        }
    }
    check(attempts, "natural pool and policy exercise99");
}
}
int main() {
    metadata_minimum_and_target_selection(); fixed_seat_search_duplicate_mortars_and_tape(); discard_skip_and_forced_removal_history();
    ordinary_responses_reactions_and_revival(); counter_limits_dead_sources_auto_money_and_cancel(); privacy_canonicality_restore_and_replay();
    std::cout << "Pestle/mortar passed (fixed/random target/999/server defense/no-op discard/counter/history/privacy,1000 replay rollouts).\n";
}
