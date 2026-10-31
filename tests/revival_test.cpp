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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = c.revival = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL revival: " << message << '\n'; std::exit(1); } }
Action attack(std::uint8_t base = 0, std::uint8_t target = 1) { return {ActionKind::attack,base,0,target}; }
Action accept() { return {ActionKind::defend,no_slot,0,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action exchange_zero() { Action a{ActionKind::exchange,0,0,no_slot}; a.extra_count = 3; a.extras[1] = 50; a.extras[2] = 20; return a; }
Action counter(std::uint16_t slots) {
    auto a = Action{ActionKind::defend,no_slot,slots,no_slot};
    for (int i = 0; i < 16; ++i) if (slots & (1u<<i)) a.counter_order |= std::uint64_t(i) << (4*a.counter_count++);
    return a;
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy);
    check(env.restore(s), "restore explicit revival fixture"); return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "revival/dead/queue/history roundtrip exactly"); }
void reject(Env& env, Action a) { const auto old = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == old, "passive revival rejects manual role atomically"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "script gift counts without revival replacements"); }
void metadata_minimum_and_first_copy() {
    check(revival_card_count == 1 && revival_catalog().size() == 1 && revival_index(244) == 0, "only passive244 in scope");
    const auto* card = find_revival_card(244);
    check(card && card->amount == 10 && card->card.kind == Kind::sundry && !card->card.attack && !card->card.boost
        && !card->card.defense && !card->card.hp && !card->card.mp && card->card.price == 10
        && card->card.weight == 1 && card->card.discardable, "244 has passiveHP10 and no ordinary combat/MP effect");
    Config minimum; minimum.revival = true; auto env = fixture({244},{120},minimum);
    check(std::string_view(env.profile()) == revival_ruleset && env.snapshot().version == 36 && env.options().discard_slots == 1
        && !env.options().utility_slots && !env.options().weapon_bases && env.options().pray,
        "minimum profile has passive/discard role without unrelated dependencies"); reject(env,{ActionKind::use,0,0,0}); reject(env,attack());
    auto discard = fixture({244},{120},minimum); check(discard.step({ActionKind::discard,no_slot,1,no_slot}).valid
        && !discard.state().players[0].hand_count && !discard.state().last_revival.id, "discard is not manual revival and earns no gift"); roundtrip(discard);
    auto first = fixture({244,56,120,244,177},{120},minimum); auto s = first.snapshot(); s.state.players[0].hp = 1;
    check(first.restore(s), "restore lethal self weapon with two244 copies"); draws(first); const auto result = first.step(attack(1,0));
    check(result.revivals == std::array<std::uint8_t,2>{1,0} && first.state().players[0].hp == 10
        && !first.state().players[0].dead && first.state().players[0].hand_count == 4
        && first.state().players[0].hand[0] == 120 && first.state().players[0].hand[1] == 244
        && first.state().players[0].hand[2] == 177 && first.snapshot().draw_index == 1,
        "first244 only is consumed, other copy retained, and only weapon's gift is paid"); roundtrip(first);
    auto none = fixture({56},{120},minimum); s = none.snapshot(); s.state.players[0].hp = 1;
    check(none.restore(s), "restore minimum profile lethal action without244"); none.step(attack(0,0));
    check(none.state().players[0].dead && none.done() && none.state().winner == 1,
        "revival-only profile still tracks finalized death when no passive exists"); roundtrip(none);
    auto pending = fixture({56},{244},minimum); pending.step(attack()); reject(pending,{ActionKind::defend,no_slot,1,no_slot}); reject(pending,special(0));
    const auto active = pending.snapshot();
    for (int seat = 0; seat < 2; ++seat) {
        auto bad = active; bad.state.players[seat].hp = 0; bad.state.players[seat].dead = true;
        check(!pending.restore(bad) && pending.snapshot() == active,
            "minimum revival profile rejects finalized dead source or actor on original active response");
    }
    auto short_cfg = minimum; short_cfg.max_decisions = 1;
    auto limited = fixture({56,244},{120},short_cfg); s = limited.snapshot(); s.state.players[0].hp = 1;
    check(limited.restore(s), "restore last-decision automatic revival"); const CardId replacement[] = {244};
    check(limited.script_draws(replacement), "ordinary earned gift may draw a new passive244");
    const auto last_step = limited.step(attack(0,0));
    check(last_step.revivals[0] == 1 && limited.state().truncated && !limited.state().terminated
        && limited.state().players[0].hp == 10 && limited.state().players[0].hand_count == 1
        && limited.state().players[0].hand[0] == 244 && limited.snapshot().draw_index == 1,
        "revival finishes before decision truncation and only original earned gift can replace244"); roundtrip(limited);
    auto old_cfg = config; old_cfg.revival = false; Env old(old_cfg); const auto old_state = old.snapshot(); const CardId ids[] = {244};
    check(!old.script_draws(ids) && old.snapshot() == old_state, "old profiles exclude244");
}
void exchange_and_single_tick_death_edges() {
    auto env = fixture({2,244},{120}); draws(env); const auto result = env.step(exchange_zero());
    check(result.exchanged && result.revivals[0] == 1 && env.state().players[0].hp == 10 && !env.state().players[0].dead
        && env.state().last_exchange.after[0] == 0 && env.snapshot().draw_index == 1 && !env.done(),
        "HP0 redistribution revives before turn end and history remains allocationHP0"); roundtrip(env);
    for (int copies : {1,2}) {
        auto heavenly = copies == 1 ? fixture({2,244},{120}) : fixture({2,244,244},{120}); auto s = heavenly.snapshot();
        s.state.players[0].disease = Disease::heaven; s.state.players[0].worse_chance = 100;
        check(heavenly.restore(s), "restore item death followed by Heaven tick death"); const std::uint8_t roll[] = {0};
        check(heavenly.script_disease_rolls(roll), "force one Heaven worsening"); draws(heavenly);
        const auto r = heavenly.step(exchange_zero());
        check(r.revivals[0] == copies && r.disease_tick && r.disease_worsened && r.disease_damage == 10
            && heavenly.state().players[0].hp == (copies == 2 ? 10 : 0) && heavenly.snapshot().disease_index == 1
            && heavenly.snapshot().draw_index == (copies == 2 ? 1 : 0),
            "item revival precedes one disease tick; post-tick revival does not tick again"); roundtrip(heavenly);
    }
    auto hell = fixture({244},{120}); auto s = hell.snapshot(); s.state.players[0].hp = 1; s.state.players[0].disease = Disease::hell;
    s.state.players[0].glory = s.state.players[0].dark_cloud = true;
    check(hell.restore(s), "restore Hell post-tick revival"); const std::uint8_t roll[] = {99}; check(hell.script_disease_rolls(roll), "avoid worsening"); draws(hell);
    const auto tick = hell.step({ActionKind::pray});
    check(tick.revivals[0] == 1 && tick.disease_damage == 1 && hell.state().players[0].hp == 10
        && hell.state().players[0].disease == Disease::hell && hell.state().players[0].worse_chance == 1
        && hell.state().players[0].glory && hell.state().players[0].dark_cloud && hell.snapshot().draw_index == 1,
        "post-tick revival keeps disease/counter/harms and does not rerun Hell"); roundtrip(hell);
}
void provisional_heal_dark_and_simultaneous_seat_order() {
    for (bool block_heal : {false,true}) {
        auto env = fixture({216},{175,static_cast<CardId>(block_heal ? 185 : 210),244}); auto s = env.snapshot(); s.state.players[1].disease = Disease::heaven;
        check(env.restore(s), "restore provisional Heaven target and244"); draws(env); const std::uint8_t target[] = {1};
        check(env.script_bounce_targets(target), "self-bounce heals provisional target"); env.step({ActionKind::use,0,0,1}); const auto r = env.step(special(1,1));
        check(env.state().players[1].hp == 10 && !env.state().players[1].dead && r.revivals[1] == block_heal
            && (block_heal ? env.state().players[1].hand_count == 2 : env.state().players[1].hand[0] == 244)
            && env.snapshot().draw_index == 3,
            "heal rescues provisionalHP0 before244; cancelling heal triggers244 only at item end"); roundtrip(env);
    }
    auto dark = fixture({63},{244}); draws(dark); dark.step(attack()); const auto execution = dark.step(accept());
    check(execution.dark_execution && execution.revivals[1] == 1 && dark.state().players[1].hp == 10
        && !dark.state().players[1].hand_count && dark.snapshot().draw_index == 1,
        "DARK execution occurs then244 rescues without a replacement gift"); roundtrip(dark);
    for (int cards = 0; cards < 4; ++cards) {
        auto env = fixture({36},{175,196}); auto s = env.snapshot(); s.state.players[0].hp = 1; s.state.players[1].disease = Disease::heaven;
        if (cards & 1) hand(s.state.players[0],{36,244});
        if (cards & 2) hand(s.state.players[1],{175,196,244});
        check(env.restore(s), "restore simultaneousHP0 source case"); draws(env); env.step(attack());
        check(env.step(special(1,1)).revivals == std::array<std::uint8_t,2>{0,0}
            && env.state().players[1].hp == 0 && !env.state().players[1].dead,
            "Heaven prefix remains provisional before returned damage completes"); roundtrip(env);
        const auto finish = env.step(accept());
        const bool first_revives = (cards & 1) != 0, second_revives = cards == 3;
        check(finish.revivals[0] == first_revives && finish.revivals[1] == second_revives
            && env.state().players[0].hp == (first_revives ? 10 : 0) && env.state().players[1].hp == (second_revives ? 10 : 0),
            "seat0 dead flag changes eligibility for seat1 using not-dead count, not currentHP count");
        if (cards == 2) check(env.done() && env.state().winner == -1 && env.state().players[1].hand[0] == 244,
            "seat1-only244 remains unused after seat0 finalized death reduces alive count to1");
        if (cards == 1) check(env.done() && env.state().winner == 0, "seat0-only244 wins asymmetric simultaneous death");
        if (cards == 3) check(!env.done() && env.state().last_revival.seat == 1, "both244 can revive while both not-dead flags persist");
        roundtrip(env);
    }
}
void repeated_counter_deaths_and_last_survivor() {
    auto env = fixture({36,244,244},{190,190,244}); auto s = env.snapshot(); s.state.players[0].hp = s.state.players[1].hp = 1;
    check(env.restore(s), "restore repeated counter deaths with multiple passive copies"); draws(env); env.step(attack());
    const auto original = env.step(counter(3));
    check(original.revivals[1] == 1 && original.counter_queued == 2 && env.state().pending.counter_card == 190
        && env.state().players[1].hp == 10 && !env.state().players[1].dead,
        "target revives before the first queued counter is launched"); roundtrip(env);
    const auto first = env.step(accept());
    check(first.revivals[0] == 1 && env.state().players[0].hp == 10 && env.state().players[0].hand[0] == 244
        && env.state().pending.counter_card == 190 && env.snapshot().draw_index == 0,
        "first lethal counter consumes one244 and leaves next counter and copy for later"); roundtrip(env);
    const auto second = env.step(accept());
    check(second.revivals[0] == 1 && !env.done() && env.state().players[0].hp == 10 && env.state().players[1].hp == 10
        && env.snapshot().draw_index == 3 && env.state().players[0].hand_count == 1 && env.state().players[1].hand_count == 2,
        "second new death uses next244; final gifts total original weapon plus two counter cards only"); roundtrip(env);
    auto last = fixture({36,244},{190}); s = last.snapshot(); s.state.players[0].hp = s.state.players[1].hp = 1;
    check(last.restore(s), "restore dead-source counter and last survivor244"); last.step(attack()); last.step(counter(1));
    check(last.state().players[1].dead && last.state().pending.source == 1, "reserved counter still runs from finalized dead source"); roundtrip(last);
    const auto draw = last.step(accept());
    check(draw.revivals == std::array<std::uint8_t,2>{0,0} && last.done() && last.state().winner == -1
        && last.state().players[0].hand[0] == 244 && last.state().players[0].dead,
        "last survivor cannot use244 when pre-death not-dead count is already1"); roundtrip(last);
}
void mana_money_and_dead_source_return() {
    for (bool revive : {false,true}) {
        auto env = revive ? fixture({36},{193,244}) : fixture({36},{193}); auto s = env.snapshot();
        s.state.players[1].hp = 1; s.state.players[1].mp = 0;
        check(env.restore(s), "restore selfMP counter with optional source revival"); draws(env); env.step(attack());
        const auto result = env.step(counter(1));
        check(result.revivals[1] == revive && result.counter_auto_resolved == revive && result.counter_skipped == !revive
            && result.counter_mp_gained == (revive ? 20 : 0) && env.state().players[1].mp == (revive ? 20 : 0)
            && env.state().players[1].hp == (revive ? 10 : 0),
            "193 checks target death after source revival; revived self resolves and finalized dead self skips"); roundtrip(env);
    }
    for (bool revive_source : {false,true}) {
        auto env = revive_source ? fixture({36,244},{194,244}) : fixture({36,244},{194}); auto s = env.snapshot();
        s.state.players[0].hp = s.state.players[1].hp = 1; s.state.players[0].mp = s.state.players[0].money = 0;
        check(env.restore(s), "restore194 HP payment death with optional source revival"); draws(env); env.step(attack());
        const auto launch = env.step(counter(1));
        check(launch.revivals[1] == revive_source && env.state().pending.counter_card == 194
            && env.state().players[1].dead == !revive_source,
            "194 remains queued from either revived or already-dead source"); roundtrip(env);
        const auto paid = env.step(accept());
        check(paid.counter_money_absorptions == 1 && paid.counter_money_gained == 10 && paid.counter_hp_lost == 1
            && !paid.counter_money_lost && !paid.counter_mp_lost && env.state().players[1].money == 30
            && paid.revivals[0] == revive_source && env.state().players[0].hp == (revive_source ? 10 : 0),
            "194 credits even a dead source then HP payment invokes last-survivor revival eligibility");
        if (!revive_source) check(env.done() && env.state().winner == -1 && env.state().players[0].hand[0] == 244,
            "dead-source194 kills final survivor without consuming its unusable244");
        else check(!env.done() && env.snapshot().draw_index == 2,
            "revived194 source lets target revive and both receive only original card gifts");
        roundtrip(env);
    }
    for (bool revive_source : {false,true}) {
        auto env = revive_source ? fixture({36,196,244},{190,244}) : fixture({36,196,244},{190}); auto s = env.snapshot();
        s.state.players[0].hp = s.state.players[1].hp = 1;
        check(env.restore(s), "restore196 return to dead or revived counter source"); draws(env); env.step(attack()); env.step(counter(1));
        const auto returned = env.step(special(0));
        check(returned.any_reflected && returned.counter_cancelled == !revive_source && !returned.counter_resolved
            && returned.revivals == std::array<std::uint8_t,2>{0,0} && env.state().players[0].hand[0] == 244,
            "196 cancels only finalized-dead target and leaves living reflector244 untouched");
        if (revive_source) {
            check(env.state().pending.counter_card == 190 && env.state().pending.target == 1 && !env.done(),
                "return to revived source stays a live counter response"); roundtrip(env);
            const auto resolved = env.step(accept()); check(!resolved.revivals[1] && env.state().players[1].dead,
                "source used its244 earlier and cannot revive second lethal return");
        }
        check(env.done() && env.state().winner == 0 && env.state().players[0].hp == 1
            && env.state().players[0].hand[0] == 244 && env.snapshot().draw_index == 2,
            "cancelled or later-resolved return pays only weapon/196 gifts and keeps unused244"); roundtrip(env);
    }
}
void history_privacy_strictness_and_seeded_replay() {
    auto env = fixture({56,244},{120}); auto s = env.snapshot(); s.state.players[0].hp = 1; check(env.restore(s), "restore history fixture"); env.step(attack(0,0)); const auto exact = env.snapshot();
    for (int error = 0; error < 5; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.last_revival.id = 243;
        if (error == 1) bad.state.last_revival.seat = 255;
        if (error == 2) bad.state.last_revival.id = 0;
        if (error == 3) bad.config.revival = false;
        if (error == 4) bad.state.players[0].dead = true;
        check(!env.restore(bad) && env.snapshot() == exact, "bad passive history/profile/dead flag rejected atomically");
    }
    auto changed = exact; changed.state.players[0].hp = 20;
    check(env.restore(changed) && env.state().last_revival.id == 244, "historical revival does not force currentHP10 or ownership of consumed card"); roundtrip(env);
    auto hidden = fixture({36},{244}); const auto visible = hidden.observe(); s = hidden.snapshot(); s.state.players[1].hand[0] = 120;
    check(hidden.restore(s) && hidden.observe() == visible && hidden.legal(attack()), "opponent passive possession stays hidden from observation/legal");
    auto own = fixture({244},{120}); check(own.observe()[1043] == 1.f && own.observe()[1044] == 1.f, "new flag and own244 feature offsets");
    auto old_cfg = config; old_cfg.revival = false; Env old(old_cfg); const auto old_obs = old.observe();
    check(std::all_of(old_obs.begin()+1043,old_obs.end(),[](float x){return x == 0.f;}), "old profiles new19 features zero");
    std::uint64_t revivals = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+114000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "revival seed state/observation replay");
            for (float x : a.observe()) check(std::isfinite(x), "passive revival observations finite");
            const auto action = a.sample_action(pa), twin = b.sample_action(pb);
            check(action == twin && pa == pb && a.legal(action), "revival sampler deterministic/legal");
            const auto ra = a.step(action), rb = b.step(twin);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.revivals == rb.revivals,
                "seat-wise passive events and rewards replay"); revivals += ra.revivals[0] + ra.revivals[1]; roundtrip(a);
        }
    }
    check(revivals, "natural pool exercises automatic244");
}
}
int main() {
    metadata_minimum_and_first_copy(); exchange_and_single_tick_death_edges(); provisional_heal_dark_and_simultaneous_seat_order();
    repeated_counter_deaths_and_last_survivor(); mana_money_and_dead_source_return(); history_privacy_strictness_and_seeded_replay();
    std::cout << "Revival passed (seat order/death edges/no gifts/queue/tick/provisional,1000 replay rollouts).\n";
}
