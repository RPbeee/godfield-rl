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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = c.revival = c.item_removal = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL item removal: " << message << '\n'; std::exit(1); } }
Action use(std::uint8_t target = 1) { return {ActionKind::use,0,0,target}; }
Action accept() { return {ActionKind::defend,no_slot,0,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action ordered(std::initializer_list<std::uint8_t> refs) {
    Action a{ActionKind::defend_ordered,no_slot,0,no_slot}; a.extra_count = static_cast<std::uint8_t>(refs.size()); std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy);
    check(env.restore(s), "restore explicit item-removal fixture"); return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "item-removal response/history/tape roundtrip exactly"); }
void reject(Env& env, Action a) { const auto before = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == before, "unsupported role/response rejects atomically"); }
void draws(Env& env, CardId id = 120) { std::array<CardId,64> ids{}; ids.fill(id); check(env.script_draws(ids), "script earned gifts independently of removal randomness"); }
void ticket(Env& env, std::uint64_t rank) { const std::uint64_t ids[] = {rank}; check(env.script_item_removal_tickets(ids), "set raw physical-slot removal ticket"); }

void metadata_minimum_and_rng() {
    check(item_removal_card_count == 1 && item_removal_catalog().size() == 1 && item_removal_index(240) == 0, "only positive-weight SUNDRY240 in scope");
    const auto* card = find_item_removal_card(240);
    check(card && card->amount == 1 && card->card.kind == Kind::sundry && !card->card.attack && !card->card.boost
        && !card->card.defense && !card->card.hp && !card->card.mp && card->card.price == 10
        && card->card.weight == 3 && card->card.discardable, "240 removes one physical card without a resource cost");
    for (CardId id : {CardId{0},CardId{241},CardId{245},CardId{249},CardId{65535}}) check(!find_item_removal_card(id), "no implicit mortar/creature/other utility inclusion");
    Config minimum; minimum.item_removal = true; auto empty = fixture({240},{120},minimum); draws(empty); ticket(empty,123);
    auto s = empty.snapshot(); s.state.players[0].mp = 0; check(empty.restore(s), "minimum configuration allows0MP use"); const auto before = empty.snapshot();
    check(std::string_view(empty.profile()) == item_removal_ruleset && empty.snapshot().version == 37
        && empty.options().item_removal_slots == 1 && empty.options().utility_slots == 1
        && empty.options().discard_slots == 1 && empty.options().pray, "independent gate exposes utility/discard without weapon/MP restrictions");
    const auto result = empty.step(use(0));
    check(result.valid && result.item_removal_attempt && !result.item_removed && !result.removed_item
        && result.item_removed_from == 0 && !result.item_removal_population && result.removed_item_slot == no_slot
        && empty.snapshot().item_removal_index == 0 && empty.snapshot().rng_state == before.rng_state
        && empty.snapshot().draw_index == 1 && empty.state().players[0].hand_count == 1
        && empty.state().players[0].hand[0] == 120 && empty.state().players[0].mp == 0,
        "self240 is consumed before empty check; no removal RNG but original gift is still paid"); roundtrip(empty);
    auto single = fixture({240,177},{120},minimum); draws(single); const auto rng = single.snapshot().rng_state;
    check(single.step(use(0)).removed_item == 177 && single.snapshot().rng_state != rng,
        "default singleton draw still consumes RNG although its result is certain"); roundtrip(single);
    auto taped = fixture({240,177},{120},minimum); draws(taped); ticket(taped,std::numeric_limits<std::uint64_t>::max());
    const auto one = taped.step(use(0)); check(one.item_removed && one.item_removal_population == 1 && one.removed_item_slot == 0
        && taped.snapshot().item_removal_index == 1 && taped.snapshot().draw_index == 1
        && taped.state().players[0].hand_count == 1, "raw modulo-one ticket consumed exactly once; victim earns no gift"); roundtrip(taped);
    auto discarded = fixture({240},{120},minimum); check(discarded.step({ActionKind::discard,no_slot,1,no_slot}).valid
        && !discarded.state().last_item_removal.id && !discarded.state().players[0].hand_count,
        "discard removes utility without activating it or earning a replacement"); roundtrip(discarded);
    auto old_cfg = config; old_cfg.item_removal = false; Env old(old_cfg); const auto old_state = old.snapshot(); const CardId id[] = {240};
    check(!old.script_draws(id) && old.snapshot() == old_state, "disabled profile excludes240 from pool/script");
    const std::uint64_t nonempty[] = {0}; check(!old.script_item_removal_tickets(nonempty) && old.snapshot() == old_state,
        "disabled profile rejects nonempty private tape atomically");
    check(old.script_item_removal_tickets({}), "disabled profile permits empty tape clear");
}

void physical_slots_duplicates_and_full_hand() {
    const std::array<CardId,4> original{120,177,120,145};
    for (std::uint64_t chosen = 0; chosen < original.size(); ++chosen) {
        auto env = fixture({240},{120,177,120,145}); draws(env); ticket(env,chosen+4*123); env.step(use());
        const auto response = env.snapshot(); check(env.state().pending.kind == Effect::remove_item && env.state().pending.element == Element::none
            && !env.state().pending.amount && !env.state().pending.miracle && env.state().pending.item_removal_card == 240,
            "opponent removal has untyped zero-amount pending payload with dedicated provenance"); roundtrip(env);
        const auto r = env.step(accept()); check(r.item_removed && r.item_removal_population == 4 && r.removed_item_slot == chosen
            && r.removed_item == original[chosen] && r.item_removed_from == 1 && env.state().players[1].hand_count == 3
            && env.snapshot().draw_index == 1 && env.state().players[0].hand_count == 1,
            "uniform physical-slot selection, one victim loss, one actor replacement");
        int next = 0; for (int i = 0; i < 4; ++i) if (i != static_cast<int>(chosen))
            check(env.state().players[1].hand[next++] == original[i], "consume selected duplicate instance and preserve remaining order");
        check(env.state().last_item_removal.source == 0 && env.state().last_item_removal.target == 1
            && env.state().last_item_removal.population == 4 && env.state().last_item_removal.slot == chosen,
            "historical record captures pre-removal population and actual slot"); roundtrip(env);
        auto bad = response; bad.state.pending.element = Element::neutral; check(!env.restore(bad), "untyped removal cannot masquerade asNE");
    }
    const std::array<CardId,16> roles{5,120,216,210,197,214,209,239,2,244,240,196,195,190,241,55};
    for (int chosen = 0; chosen < 16; ++chosen) {
        auto env = fixture({240},{120}); auto s = env.snapshot(); auto& p = s.state.players[1]; p.hand_count = 16;
        p.hand = roles;
        check(env.restore(s), "restore full physical hand"); ticket(env,chosen); env.step(use()); const auto r = env.step(accept());
        check(r.item_removal_population == 16 && r.removed_item_slot == chosen && r.removed_item == roles[chosen]
            && env.state().players[1].hand_count == 15,
            "every physical hand index0..15 and passive/TRADE/MAGIC/special/weapon role is selectable"); roundtrip(env);
    }
    auto retained = fixture({240},{216}); auto s = retained.snapshot(); s.state.players[1].abilities[0] = 2;
    check(retained.restore(s), "restore physical miracle and independent retained copies"); ticket(retained,0); retained.step(use()); retained.step(accept());
    check(!retained.state().players[1].hand_count && retained.state().players[1].abilities[0] == 2,
        "physical miracle loss never decrements retained copies"); roundtrip(retained);
}

void tape_exhaustion_and_action_canonicality() {
    auto empty = fixture({240},{}); draws(empty); ticket(empty,13); const auto initial_rng = empty.snapshot().rng_state;
    empty.step(use()); const auto none = empty.step(accept());
    check(none.item_removal_attempt && !none.item_removed && !none.item_removal_population
        && empty.snapshot().item_removal_index == 0 && empty.snapshot().rng_state == initial_rng,
        "empty opponent hand consumes no removal ticket or RNG"); roundtrip(empty);
    auto first = fixture({240},{120,240,177}); draws(first); ticket(first,0); first.step(use()); first.step(accept());
    check(first.state().actor == 1 && first.snapshot().item_removal_index == 1, "one scripted ticket exhausted by first utility");
    Env fallback(config); check(fallback.restore(first.snapshot()) && fallback.script_item_removal_tickets({}), "clear exhausted ticket in control");
    first.step(use(0)); fallback.step(use(0)); const auto a = first.step(accept()), b = fallback.step(accept());
    check(a.item_removed && a.removed_item == b.removed_item && a.removed_item_slot == b.removed_item_slot
        && first.snapshot().rng_state == fallback.snapshot().rng_state && first.state() == fallback.state()
        && first.snapshot().item_removal_index == 1,
        "exhausted prefix resumes regular RNG exactly rather than reusing last ticket"); roundtrip(first);
    auto env = fixture({240,120},{120});
    auto invalid = use(); invalid.slots = 2; reject(env,invalid);
    invalid = use(); invalid.base = no_slot; reject(env,invalid);
    invalid = use(); invalid.target = no_slot; reject(env,invalid);
    invalid = use(); invalid.counter_count = 1; reject(env,invalid);
    invalid = use(); invalid.counter_order = 1; reject(env,invalid);
    invalid = use(); invalid.extra_count = 1; invalid.extras[0] = 1; reject(env,invalid);
    invalid = use(); invalid.extras[64] = 1; reject(env,invalid);
}

void response_roles_reflections_and_unused_free() {
    auto env = fixture({240,120,177},{120,195,39,20,40,210,215,185,196,239}); draws(env); ticket(env,1); env.step(use());
    check(!env.options().defense_slots && !env.options().removal_slots && !env.options().counter_slots
        && !env.options().defensive_ability_slots && env.options().reaction_slots == (1u<<8), "untyped item permits only196 as reaction");
    for (int slot = 0; slot < 8; ++slot) reject(env,special(slot));
    reject(env,{ActionKind::defend,no_slot,1,no_slot}); reject(env,special(8,1)); reject(env,special(8,2)); reject(env,ordered({0,8}));
    const auto reflected = env.step(ordered({8,9}));
    check(reflected.any_reflected && reflected.magic_free_suffix && !reflected.magic_free_cards && !reflected.magic_free_mp_saved
        && env.state().pending.kind == Effect::remove_item && env.state().pending.source == 1 && env.state().pending.target == 0
        && env.state().players[1].hand_count == 9 && env.snapshot().item_removal_index == 0,
        "196 is consumed before selection, unused FREE remains, and removal waits for final target"); roundtrip(env);
    const auto finish = env.step(accept()); check(finish.removed_item == 177 && finish.removed_item_slot == 1 && finish.item_removal_population == 2
        && env.state().last_item_removal.source == 1 && env.state().players[0].hand_count == 2
        && env.state().players[0].hand[0] == 120 && env.snapshot().draw_index == 2,
        "returned effect uses original owner's post240 hand and preserves both earned gifts"); roundtrip(env);
    auto chain = fixture({240,196,120},{196,177}); draws(chain); ticket(chain,0); chain.step(use()); chain.step(special(0));
    check(chain.step(special(0)).any_reflected && chain.state().reaction_count == 2, "repeated196 preserves SUNDRY provenance and owner"); roundtrip(chain);
    const auto final = chain.step(accept()); check(final.item_removal_population == 1 && final.removed_item == 177
        && chain.state().players[0].hand_count == 3 && chain.state().players[1].hand_count == 1
        && chain.snapshot().draw_index == 3, "196 cards are no longer selectable and victim loss does not erase reaction gifts"); roundtrip(chain);
    auto glory = fixture({240},{196,239}); auto s = glory.snapshot(); s.state.players[1].glory = true;
    check(glory.restore(s), "restore Glory utility defender"); glory.step(use()); reject(glory,ordered({0,1}));
    check(glory.step(special(0)).any_reflected, "Glory permits lone196, counts unused FREE in two-selection rejection");
    auto no_magic = fixture({240,239},{120}); Action free{ActionKind::cast_ordered,0,0,1}; free.extra_count = 1; free.extras[0] = 1; reject(no_magic,free);
    reject(no_magic,{ActionKind::cast,0,0,1}); reject(no_magic,{ActionKind::attack,0,0,1});
    auto extra = fixture({5,240},{120}); Action attack{ActionKind::attack_ordered,0,0,1}; attack.extra_count = 1; attack.extras[0] = 1; reject(extra,attack);
}

void gift_accounting_revival_and_historical_free() {
    for (bool erase_revive : {false,true}) {
        auto env = fixture({240,244,120},{120}); auto s = env.snapshot(); s.state.players[0].hp = 1; s.state.players[0].disease = Disease::hell;
        s.state.players[0].glory = s.state.players[0].dark_cloud = true; check(env.restore(s), "restore victim244 before owner's lethal disease tick");
        draws(env,244); ticket(env,erase_revive ? 0 : 1); const std::uint8_t rolls[] = {99}; check(env.script_disease_rolls(rolls), "isolate one Hell tick");
        const auto r = env.step(use(0)); check(r.item_removed && r.disease_tick && r.revivals[0] == !erase_revive
            && env.state().players[0].hp == (erase_revive ? 0 : 10) && env.done() == erase_revive
            && env.snapshot().draw_index == (erase_revive ? 0 : 1) && env.state().players[0].glory && env.state().players[0].dark_cloud,
            "remove passive before tick; retained244 rescues once, new gift244 cannot rescue an already finalized death"); roundtrip(env);
    }
    auto env = fixture({216},{240,239}); auto s = env.snapshot(); s.state.players[1].defensive_abilities[0] = 1; s.state.players[1].mp = 0;
    check(env.restore(s), "restore prior free held210 reaction and future self removal"); draws(env); env.step(use());
    const std::uint8_t targets[] = {0}; check(env.script_bounce_targets(targets), "bounce back before later240"); env.step(ordered({20,1})); env.step(accept());
    check(env.state().actor == 1 && env.state().last_reaction.free_card == 239 && env.state().players[1].hand_count == 3,
        "held reaction's earned gift is already paid before new utility turn"); ticket(env,0);
    const auto r = env.step(use(1)); check(r.removed_item == 239 && env.state().players[1].defensive_abilities[0] == 1
        && env.state().last_reaction.free_card == 239 && env.state().players[1].pending_deals == 0
        && env.state().players[1].retained_deals == 0 && env.snapshot().draw_index == 3,
        "erase unused historical carrier without forgetting holding, undoing gifts, or invalidating past reaction"); roundtrip(env);
}

void privacy_tapes_snapshot_and_replay() {
    auto a = fixture({240},{120,177}), b = fixture({240},{120,177}); ticket(a,0); ticket(b,1);
    check(a.observe() == b.observe() && a.options() == b.options(), "future slot tape is private before use"); a.step(use()); b.step(use());
    check(a.observe() == b.observe() && a.options() == b.options() && a.legal(accept()) == b.legal(accept()), "unresolved effect exposes no chosen card or slot");
    check(a.step(accept()).removed_item == 120 && b.step(accept()).removed_item == 177, "only applied victim ID becomes public");
    auto history = a.snapshot(); auto altered = history; altered.state.last_item_removal.population = 16; altered.state.last_item_removal.slot = 15;
    Env hidden(config); check(hidden.restore(altered) && hidden.observe() == a.observe() && hidden.options() == a.options(),
        "past population and slot are private metadata, not current hand constraints");
    for (int error = 0; error < 10; ++error) {
        auto bad = history;
        if (error == 0) bad.state.last_item_removal.id = 241;
        if (error == 1) bad.state.last_item_removal.source = no_slot;
        if (error == 2) bad.state.last_item_removal.target = no_slot;
        if (error == 3) bad.state.last_item_removal.population = 17;
        if (error == 4) bad.state.last_item_removal.slot = bad.state.last_item_removal.population;
        if (error == 5) bad.state.last_item_removal.removed = 249;
        if (error == 6) bad.state.last_item_removal.population = 0;
        if (error == 7) bad.scripted_item_removals[bad.item_removal_count] = 7;
        if (error == 8) bad.item_removal_index = bad.item_removal_count + 1;
        if (error == 9) bad.config.item_removal = false;
        check(!a.restore(bad) && a.snapshot() == history, "malformed history/tape/disabled profile rejects atomically");
    }
    auto pending = fixture({240},{120}); pending.step(use()); const auto exact = pending.snapshot();
    for (int error = 0; error < 8; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.pending.element = Element::neutral;
        if (error == 1) bad.state.pending.amount = 1;
        if (error == 2) bad.state.pending.miracle = 216;
        if (error == 3) bad.state.pending.counter_card = 190;
        if (error == 4) bad.state.pending.item_removal_card = 0;
        if (error == 5) bad.state.pending.item_removal_card = 241;
        if (error == 6) bad.state.pending.forget_card = 241;
        if (error == 7) bad.state.pending.attribute_removed = true;
        check(!pending.restore(bad) && pending.snapshot() == exact, "untyped removal cannot impersonate damage/MAGIC/counter");
    }
    std::array<std::uint64_t,65> oversized{}; check(!pending.script_item_removal_tickets(oversized) && pending.snapshot() == exact, "oversize private tape rejected atomically");
    auto old_cfg = config; old_cfg.item_removal = false; Env old(old_cfg); const auto old_obs = old.observe();
    check(std::all_of(old_obs.begin()+1062,old_obs.end(),[](float x){return x == 0.f;}), "old26 profiles keep all23 appended features zero");
    auto own = fixture({240},{120}); check(own.observe()[1062] == 1.f && own.observe()[1063] == 1.f, "new flag and own240 positions stable");
    std::uint64_t attempts = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env x(config), y(config); x.reset(seed); y.reset(seed); std::uint64_t px = seed+270000, py = px;
        while (!x.done()) {
            check(x.snapshot() == y.snapshot() && x.observe() == y.observe(), "item-removal seeded state and observation replay");
            for (float value : x.observe()) check(std::isfinite(value), "item-removal observation remains finite");
            const auto ax = x.sample_action(px), ay = y.sample_action(py); check(ax == ay && px == py && x.legal(ax), "sampler remains deterministic/legal");
            const auto rx = x.step(ax), ry = y.step(ay); check(rx.valid && ry.valid && rx.rewards == ry.rewards
                && rx.item_removal_attempt == ry.item_removal_attempt && rx.item_removed == ry.item_removed
                && rx.removed_item == ry.removed_item && rx.removed_item_slot == ry.removed_item_slot
                && rx.item_removal_population == ry.item_removal_population, "seeded selection/removal/reward replay");
            attempts += rx.item_removal_attempt; roundtrip(x);
        }
    }
    check(attempts, "natural pool and sampler exercise item removal");
}
}
int main() {
    metadata_minimum_and_rng(); physical_slots_duplicates_and_full_hand(); tape_exhaustion_and_action_canonicality(); response_roles_reflections_and_unused_free();
    gift_accounting_revival_and_historical_free(); privacy_tapes_snapshot_and_replay();
    std::cout << "Item removal passed (physical duplicates/None/empty/revival/gifts/history/privacy,1000 replay rollouts).\n";
}
