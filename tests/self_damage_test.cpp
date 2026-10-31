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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = c.revival = c.item_removal = c.hp_absorption = c.self_damage = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL self damage: " << message << '\n'; std::exit(1); } }
Action attack(std::uint8_t target = 1, std::uint16_t slots = 0) { return {ActionKind::attack,0,slots,target}; }
Action accept(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action counter(std::initializer_list<int> slots) {
    auto a = accept(); for (int slot : slots) { a.slots |= static_cast<std::uint16_t>(1u<<slot); a.counter_order |= std::uint64_t(slot)<<(4*a.counter_count++); } return a;
}
Action ordered(std::initializer_list<std::uint8_t> refs, std::uint8_t target = 1) {
    Action a{ActionKind::attack_ordered,0,0,target}; a.extra_count = static_cast<std::uint8_t>(refs.size()); std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy);
    check(env.restore(s), "restore explicit self-damage fixture"); return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "self-damage/death/queue/history roundtrip exactly"); }
void reject(Env& env, Action a) { const auto before = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == before, "unsupported self-damage role rejects atomically"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "isolate original gifts from passive consumption"); }

void metadata_minimum_zero_and_clipping() {
    const auto* card = find_self_damage_card(53);
    check(self_damage_card_count == 1 && self_damage_catalog().size() == 1 && self_damage_index(53) == 0
        && card && card->card.kind == Kind::weapon && card->card.attack == 14 && !card->card.boost
        && !card->card.defense && !card->card.hp && !card->card.mp && card->card.price == 1 && card->card.weight == 1
        && !card->card.discardable, "strict53-only WEAPON ATK14 metadata");
    for (CardId id : {CardId{0},CardId{26},CardId{42},CardId{57},CardId{221},CardId{65535}}) check(!find_self_damage_card(id), "no implicit absorption/MAGICAL or other inclusion");
    Config minimum; minimum.self_damage = true; auto env = fixture({53},{145},minimum); auto s = env.snapshot(); s.state.players[0].mp = 0;
    check(env.restore(s), "restore minimum independent0MP weapon"); draws(env);
    check(std::string_view(env.profile()) == self_damage_ruleset && env.snapshot().version == 36 && env.options().self_damage_slots == 1
        && env.options().weapon_bases == 1 && !env.options().utility_slots && !env.options().boost_slots
        && !env.options().discard_slots && !env.options().pray, "minimum profile needs no dependencies and retains WEAPON/prayer roles");
    reject(env,{ActionKind::use,0,0,1}); reject(env,{ActionKind::discard,no_slot,1,no_slot});
    env.step(attack()); check(env.state().pending.self_damage_card == 53 && !env.state().pending.miracle, "unresolved53 keeps original WEAPON marker"); roundtrip(env);
    const auto r = env.step(accept(1)); check(r.self_damage_resolved && r.self_damage_card == 53 && r.self_damage_amount == 9
        && r.self_damage_source == 0 && r.self_damage_target == 1 && r.self_damage_source_lost == 9 && r.self_damage_target_lost == 9
        && env.state().players[0].hp == 31 && env.state().players[1].hp == 31 && env.state().players[0].mp == 0
        && env.snapshot().draw_index == 2, "ordinary defense reduces both raw charges and both consumed cards earn their existing gifts"); roundtrip(env);
    auto blocked = fixture({53},{186},minimum); blocked.step(attack()); const auto zero = blocked.step(accept(1));
    check(zero.self_damage_resolved && !zero.self_damage_amount && !zero.self_damage_source_lost && !zero.self_damage_target_lost
        && blocked.state().last_self_damage.id == 53 && blocked.state().players[0].hp == 40 && blocked.state().players[1].hp == 40,
        "full defense publishes a zero-residual event with no self damage"); roundtrip(blocked);
    for (int hp : {40,20,5}) {
        auto self = fixture({53},{120},minimum); s = self.snapshot(); s.state.players[0].hp = hp; check(self.restore(s), "restore same-player double payment");
        const auto result = self.step(attack(0)); const int first = std::min(hp,14), second = std::min(hp-first,14);
        check(result.self_damage_source_lost == first && result.self_damage_target_lost == second
            && result.self_damage_source == 0 && result.self_damage_target == 0 && self.state().players[0].hp == hp-first-second,
            "same alias receives sequential individually clipped debits rather than one combined or one skipped debit"); roundtrip(self);
    }
    auto draw = fixture({53},{120},minimum); s = draw.snapshot(); s.state.players[0].hp = s.state.players[1].hp = 1;
    check(draw.restore(s), "restore no-tracking simultaneous lethal action"); draw.step(attack()); const auto end = draw.step(accept());
    check(end.terminated && draw.state().winner == -1 && end.rewards == std::array<float,2>{0,0}
        && !draw.state().players[0].dead && !draw.state().players[1].dead, "independent53 creates a valid zero-reward draw without enabling old dead tracking"); roundtrip(draw);
    auto old_cfg = config; old_cfg.self_damage = false; Env old(old_cfg); const auto before = old.snapshot(); const CardId ids[] = {53};
    check(!old.script_draws(ids) && old.snapshot() == before, "old profiles exclude53");
}

void revival_seat_order_alias_and_owner_tick() {
    for (int owner = 0; owner < 2; ++owner) for (int cards = 0; cards < 4; ++cards) {
        auto env = fixture({},{}); auto s = env.snapshot();
        for (int seat = 0; seat < 2; ++seat) {
            s.state.players[seat].hp = 1;
            if (seat == owner) hand(s.state.players[seat],cards & (1<<seat) ? std::initializer_list<CardId>{53,244} : std::initializer_list<CardId>{53});
            else hand(s.state.players[seat],cards & (1<<seat) ? std::initializer_list<CardId>{244} : std::initializer_list<CardId>{});
        }
        s.state.order = {static_cast<std::uint8_t>(owner),static_cast<std::uint8_t>(1-owner)}; s.state.actor = s.state.turn_owner = static_cast<std::uint8_t>(owner);
        check(env.restore(s), "restore both near-death seats and arbitrary owner"); draws(env); env.step(attack(static_cast<std::uint8_t>(1-owner))); const auto r = env.step(accept());
        const bool first = cards & 1, second = cards == 3;
        check(r.self_damage_source_lost == 1 && r.self_damage_target_lost == 1 && r.revivals[0] == first && r.revivals[1] == second
            && env.state().players[0].hp == (first ? 10 : 0) && env.state().players[1].hp == (second ? 10 : 0),
            "both charges finish before seat0-to1 revival scan, regardless of source/target ordering");
        if (cards == 2) check(env.done() && env.state().winner == -1 && env.state().players[1].hand[0] == 244,
            "seat1-only passive cannot revive after seat0 finalized death");
        if (cards == 1) check(env.done() && env.state().winner == 0, "seat0-only passive wins both owner orientations");
        if (cards == 3) check(!env.done() && env.snapshot().draw_index == 1, "both passives consume no replacement gifts");
        roundtrip(env);
    }
    auto alias = fixture({53,244,244},{120}); auto s = alias.snapshot(); s.state.players[0].hp = 5; check(alias.restore(s), "restore alias with two passive copies"); draws(alias);
    const auto one = alias.step(attack(0)); check(one.self_damage_source_lost == 5 && !one.self_damage_target_lost && one.revivals[0] == 1
        && alias.state().players[0].hp == 10 && alias.state().players[0].hand[0] == 244 && alias.snapshot().draw_index == 1,
        "same-alias two debits form one death edge and consume only one244"); roundtrip(alias);
    for (int copies : {1,2}) {
        auto env = copies == 1 ? fixture({53,244},{120}) : fixture({53,244,244},{120}); s = env.snapshot(); s.state.players[0].hp = 1;
        s.state.players[0].disease = Disease::heaven; s.state.players[0].worse_chance = 100;
        check(env.restore(s), "restore revived owner followed by Heaven worsening"); draws(env); const std::uint8_t roll[] = {0}; check(env.script_disease_rolls(roll), "force one owner Heaven tick");
        env.step(attack()); const auto r = env.step(accept()); check(r.revivals[0] == copies && r.disease_tick && r.disease_worsened
            && env.snapshot().disease_index == 1 && env.state().players[0].hp == (copies == 2 ? 10 : 0)
            && r.self_damage_source_lost == 1 && r.self_damage_target_lost == 14,
            "item revival precedes exactly one owner tick and optional second death revival"); roundtrip(env);
    }
}

void reactions_heaven_and_marker_consumption() {
    auto reflected = fixture({53},{39}); draws(reflected); reflected.step(attack()); reflected.step(special(0));
    check(reflected.state().pending.self_damage_card == 53 && reflected.state().pending.source == 1, "39 preserves marker and changes current source"); roundtrip(reflected);
    const auto r = reflected.step(accept()); check(r.self_damage_source == 1 && r.self_damage_target == 0
        && reflected.state().players[0].hp == 26 && reflected.state().players[1].hp == 26,
        "reflected self debit charges reflector before returned target"); roundtrip(reflected);
    auto heavenly = fixture({53},{175,39,244}); auto s = heavenly.snapshot(); s.state.players[1].disease = Disease::heaven;
    check(heavenly.restore(s), "restore provisionalHP0 self-damage source"); draws(heavenly); heavenly.step(attack()); heavenly.step(special(1,1));
    check(heavenly.state().players[1].hp == 0 && !heavenly.state().players[1].dead, "prefix creates provisional source before reflected effect"); roundtrip(heavenly);
    const auto paid = heavenly.step(accept()); check(!paid.self_damage_source_lost && paid.self_damage_target_lost == 14
        && paid.revivals[1] == 1 && heavenly.state().players[0].hp == 26 && heavenly.state().players[1].hp == 10,
        "zeroHP source still damages target;244 happens only after both debits"); roundtrip(heavenly);
    auto bounce = fixture({53},{20,244}); s = bounce.snapshot(); s.state.players[1].hp = 20; check(bounce.restore(s), "restore self-bounce alias clipping");
    const std::uint8_t target[] = {1}; check(bounce.script_bounce_targets(target), "bounce to new source"); bounce.step(attack()); const auto self = bounce.step(special(0));
    check(self.bounced && self.bounce_self && self.self_damage_source_lost == 14 && self.self_damage_target_lost == 6
        && self.revivals[1] == 1 && bounce.state().players[1].hp == 10, "20 self-bounce double-debits its new source before a single revival"); roundtrip(bounce);
    auto block = fixture({53},{215,40,210}); block.step(attack()); reject(block,special(1)); reject(block,special(2)); const auto stopped = block.step(special(0));
    check(stopped.weapon_blocked && !stopped.self_damage_resolved && !block.state().last_self_damage.id
        && block.state().players[0].hp == 40 && block.state().players[1].hp == 40,
        "WEAPON block cancels self damage completely; MAGIC reactions do not match53"); roundtrip(block);
}

void ordered_setter_free_double_and_wide() {
    auto boosted = fixture({53,44},{120}); draws(boosted); boosted.step(attack(1,2)); const auto boost = boosted.step(accept());
    check(boost.self_damage_amount == 25 && boost.self_damage_source_lost == 25 && boost.self_damage_target_lost == 25,
        "ordinary boost raises both charges while consuming original extras"); roundtrip(boosted);
    auto setter = fixture({53,65},{148}); setter.step({ActionKind::attack_set_fire,0,2,1});
    check(setter.state().pending.self_damage_card == 53 && setter.state().pending.element == Element::fire, "setter preserves self-damage provenance"); roundtrip(setter);
    check(setter.step(accept(1)).self_damage_amount == 9, "WaterDEF5 reduces Fire14 before both charges"); roundtrip(setter);
    auto add = fixture({53,197},{120}); add.step(ordered({1})); check(add.state().pending.self_damage_card == 53
        && add.state().pending.amount == 16 && !add.state().pending.miracle, "additive MAGIC keeps original WEAPON self effect"); roundtrip(add);
    check(add.step(accept()).self_damage_amount == 16, "additional magic raw amount charges both players once");
    auto free = fixture({53,214,239},{120}); auto s = free.snapshot(); s.state.players[0].mp = 0; check(free.restore(s), "restore free doubled53 at0MP");
    const auto cast = free.step(ordered({1,2})); check(cast.doubling_first_uses == 1 && cast.magic_free_cards == 1 && !cast.doubling_mp_spent
        && free.state().pending.self_damage_card == 53 && free.state().pending.amount == 28, "FREE doubling retains self marker and costs no MP"); roundtrip(free);
    check(free.step(accept()).self_damage_amount == 28, "free composition resolves original self effect");
    auto wide = fixture({53,209},{39}); const std::uint8_t hit[] = {100}; check(wide.script_hit_rolls(hit), "100percent WIDE consumes hit roll");
    wide.step(ordered({1},0)); check(wide.state().pending.target == 1 && wide.state().pending.self_damage_card == 53
        && wide.state().pending.wide_miracle == 209, "WIDE normalizes original self request but retains original WEAPON payload"); roundtrip(wide);
    wide.step(special(0)); const auto returned = wide.step(accept()); check(returned.self_damage_source == 1 && returned.self_damage_amount == 14
        && wide.snapshot().hit_index == 1 && wide.state().last_chance.source == 0 && wide.state().last_chance.target == 1,
        "NE WIDE can reflect without reroll and current-source self debit changes seats"); roundtrip(wide);
    auto large = fixture({53},{120}); s = large.snapshot(); s.state.players[0].hp = s.state.players[1].hp = 99;
    s.state.players[0].mp = 99; s.state.players[0].doubling_ability = 1; check(large.restore(s), "restore int64 multiplier double death");
    Action many{ActionKind::attack_ordered,0,0,1}; many.extra_count = 16; std::fill_n(many.extras.begin(),16,18);
    large.step(many); roundtrip(large); const auto huge = large.step(accept());
    check(huge.self_damage_amount == (14LL<<16) && huge.self_damage_source_lost == 99 && huge.self_damage_target_lost == 99
        && large.done() && large.state().winner == -1, "large raw remains int64 while separate actual deltas clip to99"); roundtrip(large);
    auto no_extra = fixture({5,53},{120}); reject(no_extra,attack(1,2)); reject(no_extra,ordered({1}));
}

void counter_dead_targets_sources_and_late_money() {
    auto skipped = fixture({53},{190}); auto initial = skipped.snapshot(); initial.state.players[0].hp = 1;
    check(skipped.restore(initial), "restore original self death before queued counter targets it"); draws(skipped); skipped.step(attack());
    const auto absent = skipped.step(counter({0})); check(absent.self_damage_source_lost == 1 && absent.counter_queued == 1
        && absent.counter_skipped == 1 && !absent.counter_launched && skipped.state().players[0].dead
        && skipped.state().players[1].hp == 26 && skipped.state().winner == 1 && skipped.snapshot().draw_index == 1,
        "original source finalizes dead before counter pop, so its queued incoming counter skips without another debit"); roundtrip(skipped);
    auto cancel = fixture({53,196,244},{190}); auto s = cancel.snapshot(); s.state.players[0].hp = 20; s.state.players[1].hp = 1;
    check(cancel.restore(s), "restore dead-source counter after self damage"); draws(cancel); cancel.step(attack()); const auto queued = cancel.step(counter({0}));
    check(queued.self_damage_source_lost == 14 && queued.self_damage_target_lost == 1 && cancel.state().players[0].hp == 6
        && cancel.state().players[1].dead && cancel.state().pending.counter_card == 190 && cancel.state().pending.amount == 28
        && !cancel.state().pending.self_damage_card, "counter survives dead origin but never inherits self marker"); roundtrip(cancel);
    const auto stopped = cancel.step(special(0)); check(stopped.counter_cancelled && !stopped.counter_resolved && !stopped.self_damage_resolved
        && cancel.state().players[0].hp == 6 && cancel.state().players[0].hand[0] == 244 && cancel.state().winner == 0,
        "196 cancels return to finalized-dead counter source without a new self debit or passive use"); roundtrip(cancel);
    for (bool revive : {false,true}) {
        auto mana = revive ? fixture({53},{193,244}) : fixture({53},{193}); s = mana.snapshot(); s.state.players[1].hp = 1; s.state.players[1].mp = 0;
        check(mana.restore(s), "restore193 auto after self-damage original"); mana.step(attack()); const auto r = mana.step(counter({0}));
        check(r.counter_auto_resolved == revive && r.counter_skipped == !revive && r.counter_mp_gained == (revive ? 28 : 0)
            && mana.state().players[1].mp == (revive ? 28 : 0), "193 auto checks revival/dead status after both original charges"); roundtrip(mana);
    }
    for (bool revive : {false,true}) {
        auto money = revive ? fixture({53,244},{194,244}) : fixture({53,244},{194}); s = money.snapshot(); s.state.players[0].hp = 20;
        s.state.players[0].mp = s.state.players[0].money = 0; s.state.players[1].hp = 1;
        check(money.restore(s), "restore194 with original source weakened by its own14 payment"); money.step(attack()); money.step(counter({0})); roundtrip(money);
        const auto paid = money.step(accept()); check(paid.counter_money_gained == 14 && paid.counter_hp_lost == 6
            && !paid.self_damage_resolved && money.state().players[1].money == 34 && paid.revivals[0] == revive
            && money.state().players[0].hp == (revive ? 10 : 0), "194 credits even dead source then spends remaining6HP;244 needs a second living seat");
        if (!revive) check(money.state().winner == -1 && money.state().players[0].hand[0] == 244,
            "dead194 source kills last survivor and leaves unusable244 unconsumed"); roundtrip(money);
    }
    auto repeated = fixture({53,244,244},{190}); s = repeated.snapshot(); s.state.players[0].hp = 1;
    check(repeated.restore(s), "restore repeated item death and later counter death"); draws(repeated); repeated.step(attack()); const auto first = repeated.step(counter({0}));
    check(first.revivals[0] == 1 && repeated.state().players[0].hp == 10 && !repeated.state().pending.self_damage_card,
        "first item self death revives before counter starts"); roundtrip(repeated);
    const auto second = repeated.step(accept()); check(second.revivals[0] == 1 && !second.self_damage_resolved
        && repeated.state().players[0].hp == 10 && repeated.snapshot().draw_index == 2,
        "later counter death is a new edge and consumes second244 without additional original self damage"); roundtrip(repeated);
}

void privacy_strict_restore_and_seeded_replay() {
    auto hidden = fixture({5},{53}); const auto visible = hidden.observe(); auto s = hidden.snapshot(); s.state.players[1].hand[0] = 120;
    check(hidden.restore(s) && hidden.observe() == visible && hidden.legal(attack()), "opponent53 remains hidden until played");
    auto own = fixture({53},{120}); check(own.observe()[1110] == 1.f && own.observe()[1111] == 1.f, "self-damage feature offsets fixed");
    auto old_cfg = config; old_cfg.self_damage = false; auto old = fixture({120},{120},old_cfg), extended = fixture({120},{120});
    const auto obs = old.observe(), added = extended.observe(); check(std::all_of(obs.begin()+1110,obs.end(),[](float x){return x == 0.f;})
        && std::equal(obs.begin(),obs.begin()+1110,added.begin()), "old28 zero tail and unchanged existing1110 features");
    auto pending = fixture({53},{120}); pending.step(attack()); const auto exact = pending.snapshot();
    for (int error = 0; error < 9; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.pending.self_damage_card = 26;
        if (error == 1) bad.state.pending.hp_absorption_card = 26;
        if (error == 2) bad.state.pending.mana_weapon = 57;
        if (error == 3) bad.state.pending.miracle = 221;
        if (error == 4) bad.state.pending.counter_card = 190;
        if (error == 5) bad.state.pending.status_card = 175;
        if (error == 6) bad.state.pending.element = Element::dark;
        if (error == 7) bad.state.pending.hit_rate = 75;
        if (error == 8) bad.state.pending.amount = 13;
        check(!pending.restore(bad) && pending.snapshot() == exact, "impossible self/absorb/MAGICAL/MAGIC/counter/status/Dark/rate provenance rejects atomically");
    }
    pending.step(accept()); const auto history = pending.snapshot();
    for (int error = 0; error < 8; ++error) {
        auto bad = history;
        if (error == 0) bad.state.last_self_damage.id = 26;
        if (error == 1) bad.state.last_self_damage.source = no_slot;
        if (error == 2) bad.state.last_self_damage.target = no_slot;
        if (error == 3) bad.state.last_self_damage.amount = -1;
        if (error == 4) bad.state.last_self_damage.source_damage = 15;
        if (error == 5) bad.state.last_self_damage.target_damage = 15;
        if (error == 6) bad.state.last_self_damage.amount = magic_free_damage_limit+1;
        if (error == 7) { bad.state.last_self_damage.target = bad.state.last_self_damage.source; bad.state.last_self_damage.source_damage = 5; bad.state.last_self_damage.target_damage = 5; }
        check(!pending.restore(bad) && pending.snapshot() == history, "invalid raw/actual ranges or impossible alias second charge reject atomically");
    }
    auto historical = history; historical.state.players[0].hp = 70;
    check(pending.restore(historical), "historical actual deltas need not match current HP after later effects"); roundtrip(pending);
    std::uint64_t events = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env x(config), y(config); x.reset(seed); y.reset(seed); std::uint64_t px = seed+290000, py = px;
        while (!x.done()) {
            check(x.snapshot() == y.snapshot() && x.observe() == y.observe(), "self-damage seeded state/observation replay");
            for (float value : x.observe()) check(std::isfinite(value), "self-damage observations finite");
            const auto a = x.sample_action(px), b = y.sample_action(py); check(a == b && px == py && x.legal(a), "self-damage sampler deterministic/legal");
            const auto rx = x.step(a), ry = y.step(b); check(rx.valid && ry.valid && rx.rewards == ry.rewards
                && rx.self_damage_resolved == ry.self_damage_resolved && rx.self_damage_amount == ry.self_damage_amount
                && rx.self_damage_source_lost == ry.self_damage_source_lost && rx.self_damage_target_lost == ry.self_damage_target_lost,
                "raw and two independently clipped actual charges replay exactly"); events += rx.self_damage_resolved; roundtrip(x);
        }
    }
    check(events, "natural pool and policy exercise self damage");
}
}
int main() {
    metadata_minimum_zero_and_clipping(); revival_seat_order_alias_and_owner_tick(); reactions_heaven_and_marker_consumption();
    ordered_setter_free_double_and_wide(); counter_dead_targets_sources_and_late_money(); privacy_strict_restore_and_seeded_replay();
    std::cout << "Self damage passed (two charges/alias/seat revival/typed reactions/counters/privacy,1000 replay rollouts).\n";
}
