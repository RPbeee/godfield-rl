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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = true;
    return c;
}();
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL counter: " << message << '\n'; std::exit(1); }
}
Action attack() { return {ActionKind::attack,0,0,1}; }
Action use() { return {ActionKind::use,0,0,1}; }
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action special(int base, std::uint16_t slots = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(base),slots,no_slot}; }
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size());
    p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy);
    check(env.restore(snapshot), "restore explicit counter fixture"); return env;
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "counter pipeline snapshot exact roundtrip");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "invalid counter response preserves pipeline, gifts, cards and random streams");
}
void draws(Env& env) { std::array<CardId,64> prefix{}; prefix.fill(120); check(env.script_draws(prefix), "script deferred pipeline gifts"); }
void metadata_and_profile() {
    check(counter_card_count == 1 && counter_catalog().size() == 1, "single counter artifact scope");
    const auto* item = find_counter_card(190);
    check(item && item->card.kind == Kind::armor && item->card.attack == 0 && item->card.boost == 0
        && item->card.defense == 0 && item->card.hp == 0 && item->card.mp == 0
        && item->card.price == 10 && item->card.weight == 1 && item->card.discardable
        && item->element == Element::stone && item->multiplier == 2,
        "counter190 is Stone x2 with no fabricated ordinary defense");
    for (CardId id : {CardId{0},CardId{184},CardId{185},CardId{187},CardId{188},CardId{189},CardId{191},CardId{192},CardId{194},CardId{65535}})
        check(!find_counter_card(id), "other seven counter types and unknown IDs excluded");
    auto env = fixture({190}, {120});
    check(std::string_view(env.profile()) == counter_ruleset && env.snapshot().version == 37
        && counter_queue_capacity == 32 && counter_damage_limit == (std::int64_t{480}<<32),
        "counter profile, schema and mathematically derived capacity/bound");
    check(env.options().weapon_bases == 0 && env.options().discard_slots == 1 && !env.state().players[0].dead,
        "counter only responds or discards and starts alive"); roundtrip(env);
    auto bad_cfg = config; bad_cfg.elements = bad_cfg.chance = bad_cfg.disease = bad_cfg.celestial = false;
    bad_cfg.offensive_miracles = bad_cfg.defensive_miracles = bad_cfg.attribute_change = bad_cfg.attribute_removal = false;
    bool threw = false; try { Env invalid(bad_cfg); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "counter requires elements without unrelated feature dependencies");
    auto disabled_cfg = config; disabled_cfg.counters = false; Env disabled(disabled_cfg); const auto original = disabled.snapshot();
    const CardId counter_draw[] = {190}; check(!disabled.script_draws(counter_draw) && disabled.snapshot() == original, "disabled counter draw atomically rejected");
    auto bad = original; bad.state.players[bad.state.actor].hand[0] = 190;
    check(!disabled.restore(bad), "disabled counter hand rejected");
    bad = original; bad.state.players[0].dead = true; bad.state.players[0].hp = 0;
    check(!disabled.restore(bad), "old profiles cannot import finalized death flags");
    bad = original; bad.state.counter_generated = 1;
    check(!disabled.restore(bad), "old profiles cannot import counter bookkeeping");
    bad = env.snapshot(); bad.version = 12; check(!env.restore(bad), "old in-process schema rejected");
}
void residual_and_multiple_fifo() {
    auto blocked = fixture({36}, {186,190}); blocked.step(attack());
    const auto zero = blocked.step(defend(3));
    check(zero.valid && zero.counter_queued == 0 && zero.counter_launched == 0
        && blocked.state().players[1].hp == 40 && blocked.state().players[1].hand_count == 2,
        "full block consumes counter without generating attack"); roundtrip(blocked);
    auto partial = fixture({36}, {126,190}); partial.step(attack()); const auto queued = partial.step(defend(3));
    check(queued.counter_queued == 1 && queued.counter_launched == 1 && partial.state().pending.amount == 16
        && partial.state().players[1].hp == 32 && partial.state().pending.counter_card == 190
        && partial.state().pending.origin_source == 1 && partial.state().turn_owner == 0,
        "counter doubles raw residual after ordinary defense and keeps separate item origin");
    roundtrip(partial); check(partial.step(defend()).counter_resolved && partial.state().players[0].hp == 24,
        "counter damage resolves without doubling original defender's HP loss"); roundtrip(partial);

    auto multiple = fixture({5}, {190,190,190}); draws(multiple); multiple.step(attack());
    const auto start = multiple.step(defend(7));
    check(start.counter_queued == 3 && start.counter_launched == 1 && !start.counter_resolved
        && multiple.state().counter_generated == 3 && multiple.state().counter_started == 1
        && multiple.state().counter_count == 2 && multiple.state().players[1].hp == 39,
        "three selected same-kind counters queue three attacks after one incoming hit");
    for (int n = 0; n < 3; ++n) {
        check(multiple.state().pending.amount == 2 && multiple.state().pending.counter_card == 190
            && multiple.state().pending.source == 1 && multiple.state().pending.target == 0
            && multiple.state().pending.origin_source == 1 && multiple.state().actor == 0
            && multiple.state().turn_owner == 0 && multiple.state().turns == 0 && multiple.snapshot().draw_index == 0,
            "each queued counter item is served before owner tick or gifts"); roundtrip(multiple);
        const auto result = multiple.step(defend());
        check(result.counter_resolved && result.counter_queued == 0 && result.counter_launched == (n < 2 ? 1 : 0),
            "FIFO advances exactly once per answered counter item");
    }
    check(multiple.state().players[0].hp == 34 && multiple.state().players[1].hp == 39
        && multiple.state().turns == 1 && multiple.snapshot().draw_index == 4
        && !multiple.state().counter_count && !multiple.state().counter_generated && !multiple.state().counter_started,
        "pipeline clears only at turn boundary and pays each used artifact one gift"); roundtrip(multiple);
}
void re_counter_types_and_payload() {
    auto env = fixture({5,195,190}, {190,195}); env.step(attack()); env.step(defend(1));
    check(env.state().pending.amount == 2 && env.state().pending.origin_source == 1, "first counter origin is original defender");
    const auto doubled = env.step(defend(3));
    check(doubled.attribute_removed && doubled.counter_queued == 1 && doubled.counter_launched == 1
        && doubled.counter_resolved && env.state().pending.amount == 4
        && env.state().pending.element == Element::stone && !env.state().pending.attribute_removed
        && env.state().pending.origin_source == 0 && env.state().pending.source == 0 && env.state().actor == 1,
        "server-accepted re-counter creates a new Stone item rather than inheriting removed incoming attribute");
    roundtrip(env); env.step(defend(1));
    check(env.state().players[0].hp == 38 && env.state().players[1].hp == 35, "re-counter damage and removals applied once per item"); roundtrip(env);
    auto fifo = fixture({5,195,190}, {190,190,195}); fifo.step(attack()); fifo.step(defend(3));
    fifo.step(defend(3));
    check(fifo.state().pending.amount == 2 && fifo.state().pending.origin_source == 1
        && fifo.state().counter_count == 1 && fifo.state().counter_queue[0].amount == 4
        && fifo.state().counter_queue[0].origin_source == 0,
        "new re-counter appends behind older queued attack instead of replacing or jumping ahead");
    roundtrip(fifo); fifo.step(defend());
    check(fifo.state().pending.amount == 4 && fifo.state().pending.origin_source == 0,
        "FIFO eventually advances from older original counter to new re-counter");
    fifo.step(defend(1)); check(fifo.state().players[0].hp == 36 && fifo.state().players[1].hp == 35,
        "mixed-generation queue applies raw values in original FIFO order"); roundtrip(fifo);
    auto types = fixture({5,195,39,20,215,210}, {190});
    auto snapshot = types.snapshot(); snapshot.state.players[0].defensive_abilities = {1,1};
    check(types.restore(snapshot), "restore defensive alternatives against counter"); types.step(attack()); types.step(defend(1));
    for (int slot = 1; slot < 5; ++slot) reject(types,special(slot,1));
    reject(types,{ActionKind::defend_cast,0,1,no_slot}); reject(types,{ActionKind::defend_cast,1,1,no_slot});
    check(types.legal(defend(1)), "counter can be neutralized and normally defended but remains neither weapon nor magic");
    types.step(defend(1)); roundtrip(types);

    auto payload = fixture({43,65}, {195,190});
    payload.step({ActionKind::attack_set_fire,0,2,1}); payload.step(defend(3));
    const auto& counter = payload.state().pending;
    check(counter.counter_card == 190 && counter.amount == 22 && counter.element == Element::stone
        && counter.harm == Harm::none && !counter.harm_card && counter.disease == Disease::none
        && !counter.status_card && !counter.attribute_card && !counter.attribute_removed && !counter.miracle && !counter.hit_rate
        && payload.state().players[1].dark_cloud, "new counter carries no original status, setter, removal or magic metadata");
    roundtrip(payload); payload.step(defend());
    check(!payload.state().players[0].dark_cloud && payload.state().players[0].hp == 18, "original status does not spread through counter attack");
}
void specials_and_non_damage() {
    for (int mode = 0; mode < 4; ++mode) {
        auto env = fixture({mode == 3 ? CardId{198} : CardId{36}}, {190,39,20,215,210});
        const std::uint8_t seats[] = {1}; check(env.script_bounce_targets(seats), "script terminal special after counter prefix");
        env.step(mode == 3 ? use() : attack());
        const int slot = mode == 0 ? 1 : mode == 1 ? 2 : mode == 2 ? 3 : 4;
        const auto result = env.step(special(slot,1));
        check(result.valid && !result.counter_queued && !result.counter_launched
            && env.state().counter_generated == 0, "special prefix counter is consumed but cannot trigger before reflection/bounce/block");
        if (mode == 0) { roundtrip(env); env.step(defend()); }
        roundtrip(env);
    }
    for (CardId id : {CardId{216},CardId{220},CardId{227},CardId{231},CardId{235}}) {
        auto env = fixture({id}, {190}); env.step(use()); const auto result = env.step(defend(1));
        check(result.valid && !result.counter_queued && !env.state().counter_generated,
            "non-damage response may consume counter without generating damage"); roundtrip(env);
    }
    auto incompatible = fixture({58}, {190}); incompatible.step(attack()); reject(incompatible,defend(1));
    auto compatible = fixture({60}, {190}); compatible.step(attack());
    check(compatible.step(defend(1)).counter_queued == 1, "Stone counter participates in final defense attribute against Wood"); roundtrip(compatible);
    auto glory = fixture({5}, {190,190}); auto snapshot = glory.snapshot(); snapshot.state.players[1].glory = true;
    check(glory.restore(snapshot), "restore Glory counter defender"); glory.step(attack()); reject(glory,defend(3));
    check(glory.legal(defend(1)), "Glory counts every counter as one selected artifact");
}
void finalized_death_skip_and_owner_tick() {
    Config narrow; narrow.elements = narrow.counters = true;
    auto dead_source = fixture({36}, {190},narrow); auto snapshot = dead_source.snapshot(); snapshot.state.players[1].hp = 1;
    check(dead_source.restore(snapshot), "restore low-HP counter user"); dead_source.step(attack()); dead_source.step(defend(1));
    check(dead_source.state().players[1].dead && dead_source.state().players[1].hp == 0 && !dead_source.done()
        && dead_source.state().pending.source == 1 && dead_source.state().pending.amount == 20,
        "dead counter source still attacks using raw ten damage rather than one actual lost HP");
    roundtrip(dead_source); dead_source.step(defend());
    check(dead_source.state().winner == 0 && dead_source.state().players[0].hp == 20,
        "dead source counter executes before final winner selection"); roundtrip(dead_source);
    auto draw = fixture({36}, {190},narrow); snapshot = draw.snapshot();
    snapshot.state.players[0].hp = snapshot.state.players[1].hp = 1;
    check(draw.restore(snapshot), "restore simultaneous counter death without disease feature"); draw.step(attack()); draw.step(defend(1));
    const auto tied = draw.step(defend());
    check(tied.terminated && draw.state().winner == -1 && tied.rewards == std::array<float,2>{0.f,0.f}
        && draw.state().players[0].dead && draw.state().players[1].dead,
        "counter alone can finalize both deaths and award a learning draw"); roundtrip(draw);
    auto large = fixture({56,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170}, {190},narrow);
    snapshot = large.snapshot(); snapshot.state.players[0].hp = snapshot.state.players[1].hp = 99;
    check(large.restore(snapshot), "restore naturally large ordinary boost attack");
    check(large.step({ActionKind::attack,0,0xfffe,1}).valid, "maximum hand of ordinary boosts executes");
    large.step(defend(1));
    check(large.state().pending.amount == 510 && large.state().players[1].dead,
        "natural counter exceeds former480 bound and doubles raw255 despite HP99 clipping");
    roundtrip(large); check(large.step(defend()).terminated && large.state().winner == -1,
        "large natural counter finalizes simultaneous death without overflow"); roundtrip(large);
    auto skipped = fixture({56}, {190,190}); snapshot = skipped.snapshot(); snapshot.state.players[0].hp = 1;
    snapshot.state.players[1].hp = 99; check(skipped.restore(snapshot), "restore queued counter target who dies before second entry");
    skipped.step(attack()); skipped.step(defend(3)); const auto killed = skipped.step(defend());
    check(killed.terminated && killed.counter_resolved && killed.counter_skipped == 1
        && !killed.counter_launched && skipped.state().winner == 1 && skipped.state().players[1].hp == 69,
        "queue skips entries whose target is finalized dead without another response"); roundtrip(skipped);
    auto disease = fixture({5}, {190,190}); draws(disease); snapshot = disease.snapshot();
    snapshot.state.players[0].disease = Disease::cold; snapshot.state.players[1].disease = Disease::hell;
    check(disease.restore(snapshot), "restore owner-only disease test across queue");
    const std::uint8_t natural[] = {99}; check(disease.script_disease_rolls(natural), "script single owner natural tick");
    disease.step(attack()); disease.step(defend(3)); disease.step(defend());
    check(disease.snapshot().disease_index == 0 && disease.snapshot().draw_index == 0 && disease.state().turns == 0,
        "intermediate counter resolution neither ticks nor distributes");
    const auto last = disease.step(defend());
    check(last.disease_tick && disease.snapshot().disease_index == 1 && disease.state().players[0].hp == 35
        && disease.state().players[1].hp == 39 && disease.state().players[1].worse_chance == 0
        && disease.snapshot().draw_index == 3 && disease.state().turns == 1,
        "owner ticks once after whole queue; counter users do not steal the turn tick"); roundtrip(disease);
}
void snapshot_bounds_observations_and_truncation() {
    auto env = fixture({5}, {190,190,190}); env.step(attack()); env.step(defend(7)); const auto original = env.snapshot();
    const auto obs = env.observe();
    check(obs[393] == 1.f && obs[410] == 190/363.f && obs[411] == 0.f && obs[414] == 2/32.f
        && obs[415] == 3/32.f && obs[416] == 1/32.f && obs[417] == 1.f
        && obs[419] == 0.f && obs[420] == 1.f && obs[421] == 2/480.f,
        "current and future public counter queue represented in fixed actor coordinates");
    for (int error = 0; error < 10; ++error) {
        auto bad = original;
        if (error == 0) bad.state.pending.origin_source = 0;
        if (error == 1) bad.state.pending.amount = 3;
        if (error == 2) bad.state.pending.amount = 0;
        if (error == 3) bad.state.pending.amount = counter_damage_limit+2;
        if (error == 4) ++bad.state.counter_generated;
        if (error == 5) bad.state.counter_queue[0].counter_card = 191;
        if (error == 6) bad.state.counter_queue[0].origin_source = 0;
        if (error == 7) bad.state.counter_queue[2] = bad.state.counter_queue[0];
        if (error == 8) bad.state.players[0].dead = true;
        if (error == 9) bad.state.pending.miracle = 216;
        check(!env.restore(bad) && env.snapshot() == original, "invalid origin, counter amount, queue bookkeeping/tail, death or magic rejected atomically");
    }
    auto huge = fixture({5}, {190}); huge.step(attack()); huge.step(defend(1)); auto snapshot = huge.snapshot();
    snapshot.state.pending.amount = counter_damage_limit;
    snapshot.state.counter_generated = snapshot.state.counter_started = counter_queue_capacity;
    snapshot.state.decisions = 33;
    for (auto& p : snapshot.state.players) { p.hand.fill(0); p.hand_count = 0; p.pending_deals = 16; p.retained_deals = 0; }
    ++snapshot.state.players[0].pending_deals; snapshot.state.players[0].retained_deals = 1;
    snapshot.state.players[0].abilities[2] = 1;
    check(huge.restore(snapshot), "maximum conservative int64 counter amount accepted as explicit fixture");
    const auto result = huge.step(defend());
    check(result.terminated && huge.state().players[0].hp == 0 && huge.state().players[0].dead,
        "int64 damage clips HP safely without signed narrowing overflow"); roundtrip(huge);
    auto cfg = config; cfg.max_decisions = 2; auto truncated = fixture({5}, {190,190},cfg);
    truncated.step(attack());
    check(truncated.step(defend(3)).truncated && truncated.state().counter_count == 1
        && truncated.state().pending.counter_card == 190 && truncated.state().turns == 0,
        "learning limit freezes current counter and queued entry without finishing turn"); roundtrip(truncated); reject(truncated,defend());
}
void privacy_and_replay() {
    auto private_hand = fixture({5}, {190}); const auto obs = private_hand.observe(); const auto opts = private_hand.options();
    auto snapshot = private_hand.snapshot(); snapshot.state.players[1].hand[0] = 120;
    check(private_hand.restore(snapshot) && private_hand.observe() == obs && private_hand.options() == opts,
        "unplayed opponent counter artifact remains private");
    auto disabled_cfg = config; disabled_cfg.counters = false; Env disabled(disabled_cfg); const auto old = disabled.observe();
    check(std::all_of(old.begin()+393,old.end(),[](float x){return x==0.f;}), "previous profile counter suffix zero");
    std::uint64_t queued = 0, launched = 0, resolved = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+65000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "counter seeded trajectory replay");
            for (float x : a.observe()) check(std::isfinite(x), "large counter observations remain finite");
            const auto action = a.sample_action(pa), copy = b.sample_action(pb);
            check(action == copy && pa == pb && a.legal(action), "counter sampler deterministic and legal");
            const auto ra = a.step(action), rb = b.step(copy);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.counter_queued == rb.counter_queued
                && ra.counter_launched == rb.counter_launched && ra.counter_skipped == rb.counter_skipped
                && ra.counter_resolved == rb.counter_resolved, "counter transition metrics replay");
            queued += ra.counter_queued; launched += ra.counter_launched; resolved += ra.counter_resolved; roundtrip(a);
        }
    }
    check(queued && launched && resolved, "natural seeded games exercise counter creation, response and resolution");
}
}
int main() {
    metadata_and_profile(); residual_and_multiple_fifo(); re_counter_types_and_payload(); specials_and_non_damage();
    finalized_death_skip_and_owner_tick(); snapshot_bounds_observations_and_truncation(); privacy_and_replay();
    std::cout << "Counter checks passed (raw residual/FIFO/death/skip/int64, 1000 replay rollouts).\n";
}
