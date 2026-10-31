#include "godfield/legacy_env.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string_view>

namespace {
using namespace godfield::legacy;
constexpr Config config{512, 10, true, true, true, true, true, true};
void check(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL harms: " << message << '\n'; std::exit(1); }
}
Action attack(int base = 0, std::uint16_t extras = 0, int target = 1) {
    return {ActionKind::attack, static_cast<std::uint8_t>(base), extras, static_cast<std::uint8_t>(target)};
}
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend, no_slot, slots, no_slot}; }
Action reaction(int base, std::uint16_t prefix = 0) {
    return {ActionKind::defend, static_cast<std::uint8_t>(base), prefix, no_slot};
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(), ids.end(), p.hand.begin());
    p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy) {
    Env env(config); auto snapshot = env.snapshot(); snapshot.state = State{};
    snapshot.scripted_draws.fill(0); snapshot.draw_count = snapshot.draw_index = 0;
    snapshot.scripted_hits.fill(0); snapshot.hit_count = snapshot.hit_index = 0;
    snapshot.scripted_targets.fill(0); snapshot.target_count = snapshot.target_index = 0;
    snapshot.scripted_diseases.fill(0); snapshot.disease_count = snapshot.disease_index = 0;
    hand(snapshot.state.players[0], own); hand(snapshot.state.players[1], enemy);
    check(env.restore(snapshot), "restore valid harm fixture"); return env;
}
void flags(Env& env, int seat, bool glory, bool dark_cloud) {
    auto snapshot = env.snapshot(); snapshot.state.players[seat].glory = glory;
    snapshot.state.players[seat].dark_cloud = dark_cloud;
    check(env.restore(snapshot), "restore fixture-only harm flags");
}
void draws(Env& env, std::initializer_list<CardId> ids) {
    check(env.script_draws({ids.begin(), ids.size()}), "script supported artifact outcomes");
}
void hits(Env& env, std::initializer_list<std::uint8_t> rolls) {
    check(env.script_hit_rolls({rolls.begin(), rolls.size()}), "script supported chance outcomes");
}
void reject(Env& env, Action action, const char* message) {
    const auto original = env.snapshot(); check(!env.legal(action) && !env.step(action).valid, message);
    check(env.snapshot() == original, "illegal harm-sensitive action preserves all state and RNG");
}
void catalog_and_profile_boundaries() {
    check(harm_card_count == 1 && harm_catalog().size() == 1, "single supported on-damage harm source");
    const auto* card = find_harm_card(43);
    check(card && card->card.attack == 11 && card->card.weight == 1 && card->harm == Harm::dark_cloud
        && card->element == Element::neutral && !card->on_defense && !card->card.discardable,
        "legacy DarkCloud weapon metadata");
    for (CardId id : {CardId{0}, CardId{37}, CardId{70}, CardId{191}, CardId{200}, CardId{208}, CardId{65535}})
        check(!find_harm_card(id), "Glory light/counter sources and other unsupported harms excluded");
    auto enabled = fixture({43}, {120});
    check(std::string_view(enabled.profile()) == harms_ruleset && enabled.snapshot().version == 36,
        "new harm profile and snapshot version identified");
    for (const Config disabled_config : {Config{}, Config{512, 10, true, true, true, true, true}}) {
        Env disabled(disabled_config); const auto original = disabled.snapshot();
        const CardId harm_draw[] = {43};
        check(!disabled.script_draws(harm_draw) && disabled.snapshot() == original, "disabled harm acquisition rejected atomically");
        auto bad = original; bad.state.players[0].glory = true;
        check(!disabled.restore(bad) && disabled.snapshot() == original, "disabled Glory fixture rejected");
        bad = original; bad.state.players[0].dark_cloud = true;
        check(!disabled.restore(bad) && disabled.snapshot() == original, "disabled DarkCloud fixture rejected");
        bad = original; bad.state.last_chance = LastChance{100, 0, 1, 75, true, true};
        check(!disabled.restore(bad) && disabled.snapshot() == original, "forced public chance event requires harm profile");
    }
    const auto original = enabled.snapshot(); auto old = original; old.version = 6;
    check(!enabled.restore(old) && enabled.snapshot() == original, "old snapshot schema rejected");
    Env standalone(Config{512, 10, false, false, false, false, false, true});
    auto standalone_state = standalone.snapshot(); standalone_state.state = State{};
    hand(standalone_state.state.players[0], {43}); hand(standalone_state.state.players[1], {120});
    standalone_state.state.players[1].glory = true;
    check(standalone.restore(standalone_state) && standalone.step(attack()).valid
        && standalone.options().defense_limit == 1, "harms opt-in independently supports neutral harm and Glory");
    const auto standalone_result = standalone.step(defend());
    check(standalone_result.valid && standalone_result.glory_defense
        && standalone_result.harm_new_flags == 1 && standalone.state().players[1].dark_cloud,
        "standalone harm profile resolves without enabling unrelated feature pools");
    Env standalone_resume(standalone.snapshot().config);
    check(standalone_resume.restore(standalone.snapshot()), "standalone harm trajectory restorable");
}
void glory_legal_response_and_sampling() {
    auto env = fixture({5}, {120, 126, 118, 39, 20}); flags(env, 1, true, false);
    check(env.options().defense_limit == 0, "defense limit not active on normal action");
    env.step(attack());
    check(env.options().defense_limit == 1 && env.options().defense_slots == 7
        && env.options().reaction_slots == 24, "Glory limits composed response without hiding component candidates");
    check(env.legal(defend()) && env.legal(defend(1)) && env.legal(defend(4))
        && env.legal(reaction(3)) && env.legal(reaction(4)), "Glory permits empty or one ordinary/special artifact");
    reject(env, defend(3), "Glory rejects two ordinary armor pieces");
    reject(env, defend(5), "zero-defense armor still counts toward one-artifact limit");
    reject(env, reaction(3, 1), "Glory rejects ordinary prefix plus reflector");
    reject(env, reaction(4, 2), "Glory rejects ordinary prefix plus bounce weapon");
    for (std::uint64_t seed = 0; seed < 1024; ++seed) {
        auto policy = seed;
        const auto before = env.snapshot(); const auto action = env.sample_action(policy);
        check(env.legal(action) && std::popcount(action.slots) + (action.base != no_slot) <= 1,
            "hierarchical sampler never creates multiple artifacts under Glory");
        check(env.snapshot() == before, "sampling does not mutate harm state");
        auto branch = env; const auto result = branch.step(action);
        check(result.valid && result.glory_defense, "Glory response metric accompanies valid constrained response");
        Env resumed(config); check(resumed.restore(branch.snapshot()), "sampled Glory response state restorable");
    }

    auto actor = fixture({5, 9}, {120, 126}); flags(actor, 0, true, false);
    check(actor.step(attack(0, 2)).valid && actor.state().pending.amount == 2,
        "Glory does not restrict ordinary attack boosts");
    check(actor.options().defense_limit == 16 && actor.legal(defend(3)),
        "response uses acting defender's Glory flag rather than turn owner's flag");
}
void dark_cloud_forcing_and_saved_rolls() {
    auto forced = fixture({100}, {236}); flags(forced, 1, false, true);
    draws(forced, {120, 120}); hits(forced, {100});
    const auto before_rng = forced.snapshot().rng_state;
    const auto result = forced.step(attack(0, 0, 0));
    check(result.valid && result.chance_attempt && result.chance_hit && result.chance_forced,
        "actual enemy DarkCloud forces hit after target normalization");
    check(forced.snapshot().hit_index == 0 && forced.snapshot().rng_state == before_rng,
        "forced hit consumes neither scripted roll nor game RNG");
    check(forced.state().last_chance == LastChance{100, 0, 1, 75, true, true}
        && forced.state().pending.hit_rate == 75, "forced event preserves original chance percent and actual seats");
    Env resumed(config); check(resumed.restore(forced.snapshot()), "forced pending hit snapshot restorable without a consumed roll");
    forced.step(defend());
    check(forced.state().players[1].hp == 39 && forced.state().actor == 1, "forced chance hit resolves ordinary damage");
    check(forced.step({ActionKind::use, 0, 0, 1}).valid && !forced.state().players[1].dark_cloud
        && forced.state().last_chance.forced && forced.snapshot().hit_index == 0,
        "full cure clears target flag but retains historical forced event and saved roll");
    check(resumed.restore(forced.snapshot()), "past forced event does not require current DarkCloud flag");
    auto next = forced.snapshot(); next.state.order = {0, 1}; next.state.order_index = 0;
    next.state.actor = next.state.turn_owner = 0;
    hand(next.state.players[0], {100}); hand(next.state.players[1], {120});
    check(forced.restore(next), "fixture schedules another chance weapon after cure");
    const auto missed = forced.step(attack());
    check(missed.chance_attempt && !missed.chance_hit && !missed.chance_forced
        && forced.snapshot().hit_index == 1 && forced.state().phase == Phase::main,
        "next unforced chance consumes saved roll100 and misses");

    auto source_only = fixture({100}, {123}); flags(source_only, 0, false, true); hits(source_only, {100});
    const auto source_result = source_only.step(attack(0, 0, 0));
    check(source_result.chance_attempt && !source_result.chance_hit && !source_result.chance_forced
        && source_only.snapshot().hit_index == 1, "source DarkCloud does not force attack against clean actual enemy");
    auto certain = fixture({5}, {120}); flags(certain, 1, false, true); hits(certain, {100});
    const auto certain_result = certain.step(attack());
    check(!certain_result.chance_attempt && !certain_result.chance_forced && certain.snapshot().hit_index == 0
        && certain.state().last_chance == LastChance{}, "guaranteed attack is not reclassified as a forced chance attempt");
}
void on_damage_duplicate_and_heaven() {
    for (bool already : {false, true}) {
        auto env = fixture({43}, {120}); flags(env, 1, false, already);
        auto snapshot = env.snapshot(); snapshot.state.players[1].disease = Disease::cold;
        snapshot.state.players[1].worse_chance = 50;
        check(env.restore(snapshot), "restore disease plus harm target");
        env.step(attack());
        check(env.state().pending.harm_card == 43 && env.state().pending.harm == Harm::dark_cloud,
            "pending harm provenance retained");
        const auto result = env.step(defend());
        check(result.harm_applications == 1 && result.harm_new_flags == (already ? 0u : 1u)
            && result.harm_heaven_falls == 0, "new and duplicate harm applications distinguished");
        check(env.state().players[1].dark_cloud && env.state().players[1].disease == Disease::cold
            && env.state().players[1].worse_chance == 50 && env.state().players[1].hp == 29,
            "harm application leaves non-Heaven disease and worsening counter unchanged");
    }
    auto blocked = fixture({43}, {186}); blocked.step(attack());
    const auto block = blocked.step(defend(1));
    check(!blocked.state().players[1].dark_cloud && block.harm_applications == 0
        && blocked.state().players[1].hp == 40, "full block prevents on-damage harm");

    for (bool already : {false, true}) {
        auto env = fixture({43}, {120}); flags(env, 1, false, already);
        auto snapshot = env.snapshot(); snapshot.state.players[1].disease = Disease::heaven;
        snapshot.state.players[1].worse_chance = 50;
        check(env.restore(snapshot), "restore Heaven harm recipient"); env.step(attack());
        const auto fell = env.step(defend());
        check(fell.terminated && fell.harm_applications == 1 && fell.harm_new_flags == 0
            && fell.harm_heaven_falls == 1, "Heaven check precedes both new and duplicate harm attachment");
        check(env.state().players[1].hp == 0 && env.state().players[1].worse_chance == 0
            && env.state().players[1].dark_cloud == already,
            "Heaven dies without adding a new flag or erasing an existing flag");
    }
    auto reflected = fixture({43}, {39}); reflected.step(attack()); reflected.step(reaction(0));
    Env resumed(config); check(resumed.restore(reflected.snapshot()), "reflected harm-bearing pending attack restorable");
    const auto applied = reflected.step(defend());
    check(applied.harm_applications == 1 && reflected.state().players[0].dark_cloud
        && !reflected.state().players[1].dark_cloud && reflected.state().players[0].hp == 29,
        "reflected on-damage harm applies to new target");
}
void cure_interactions() {
    auto lower = fixture({235}, {120}); flags(lower, 0, true, true);
    auto snapshot = lower.snapshot(); snapshot.state.players[0].disease = Disease::hell;
    check(lower.restore(snapshot), "restore severe disease and both harms");
    const std::uint8_t roll[] = {99}; check(lower.script_disease_rolls(roll), "script nonworsening owner tick");
    check(lower.step({ActionKind::use, 0, 0, 0}).valid && !lower.state().players[0].glory
        && lower.state().players[0].dark_cloud && lower.state().players[0].disease == Disease::hell
        && lower.state().players[0].worse_chance == 1 && lower.state().players[0].hp == 35,
        "lower cure clears Glory independently of severe disease and leaves DarkCloud");
    auto full = fixture({236}, {120}); flags(full, 0, true, true);
    snapshot = full.snapshot(); snapshot.state.players[0].disease = Disease::heaven;
    snapshot.state.players[0].worse_chance = 100; check(full.restore(snapshot), "restore full-cure Heaven fixture");
    check(full.script_disease_rolls(roll), "retain future natural roll");
    const auto cured = full.step({ActionKind::use, 0, 0, 0});
    check(cured.valid && !full.state().players[0].glory && !full.state().players[0].dark_cloud
        && full.state().players[0].disease == Disease::none && full.state().players[0].worse_chance == 0
        && !cured.disease_tick && full.snapshot().disease_index == 0,
        "full cure clears both flags and disease before any natural roll");
}
void restoration_and_observations() {
    auto env = fixture({43}, {120}); flags(env, 0, true, false); flags(env, 1, false, true);
    auto obs = env.observe();
    check(obs[258] == 1.f && obs[259] == 0.f && obs[260] == 0.f && obs[261] == 1.f
        && obs[262] == 1.f && obs[267] == 1.f, "public own/opponent flags and own hand harm effect represented");
    env.step(attack()); obs = env.observe();
    check(obs[265] == 1.f && obs[266] == 43 / 363.f && obs[263] == 1.f,
        "pending DarkCloud and unconstrained defender limit represented");
    const auto original = env.snapshot(); auto bad = original;
    bad.state.pending.harm = Harm::glory;
    check(!env.restore(bad) && env.snapshot() == original, "harm payload must match source artifact");
    bad = original; bad.state.pending.harm_card = 0;
    check(!env.restore(bad) && env.snapshot() == original, "pending harm needs source provenance");
    bad = original; bad.state.pending.harm = static_cast<Harm>(255);
    check(!env.restore(bad) && env.snapshot() == original, "unknown harm enum rejected");

    auto forced = fixture({100}, {120}); flags(forced, 1, true, true); hits(forced, {100});
    forced.step(attack()); const auto pending = forced.snapshot();
    const auto visible = forced.observe();
    check(visible[263] == 1 / 16.f && visible[264] == 1.f && visible[258] == 1.f
        && visible[259] == 1.f, "Glory defense limit and public forced-hit flag exposed");
    bad = pending; bad.state.last_chance.hit = false;
    check(!forced.restore(bad) && forced.snapshot() == pending, "forced public chance record cannot be a miss");
    auto hidden = pending; hidden.scripted_hits[0] = 1; hidden.rng_state ^= 0x9e3779b97f4a7c15ULL;
    Env other(config); check(other.restore(hidden), "restore hidden-roll variant of forced hit");
    check(other.observe() == forced.observe() && other.options() == forced.options(), "forcing does not reveal the preserved future roll");

    Env disabled(Config{512, 10, true, true, true, true, true});
    const auto old = disabled.observe();
    check(std::all_of(old.begin() + 258, old.end(), [](float value) { return value == 0.f; }),
        "prior profile harm observation suffix all zero");

    auto hidden_hand = fixture({5}, {43});
    const auto hidden_visible = hidden_hand.observe(); const auto hidden_options = hidden_hand.options();
    auto swapped = hidden_hand.snapshot(); swapped.state.players[1].hand[0] = 120;
    check(hidden_hand.restore(swapped) && hidden_hand.observe() == hidden_visible
        && hidden_hand.options() == hidden_options, "opponent hand harm feature remains hidden");
}
void replay() {
    std::uint64_t applications = 0, forced_hits = 0, glory_responses = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed);
        auto initial = a.snapshot();
        // Glory has no supported acquisition source in this strict pool;
        // injection tests the documented old-bot constraint only.
        initial.state.players[0].glory = (seed % 2) == 0;
        initial.state.players[1].glory = (seed % 3) == 0;
        initial.state.players[1].dark_cloud = (seed % 5) == 0;
        check(a.restore(initial) && b.restore(initial), "seeded test-only public harm initialization");
        const std::uint8_t rolls[] = {100, 1};
        check(a.script_hit_rolls(rolls) && b.script_hit_rolls(rolls), "seeded finite chance prefix");
        std::uint64_t pa = seed + 15000, pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "harm trajectory replay");
            for (float value : a.observe()) check(std::isfinite(value), "harm observation finite");
            const auto action = a.sample_action(pa), action_b = b.sample_action(pb);
            check(action == action_b && pa == pb && a.legal(action), "harm-aware sampler legal and reproducible");
            const auto ra = a.step(action), rb = b.step(action_b);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.chance_forced == rb.chance_forced
                && ra.harm_applications == rb.harm_applications && ra.harm_new_flags == rb.harm_new_flags
                && ra.harm_heaven_falls == rb.harm_heaven_falls && ra.glory_defense == rb.glory_defense,
                "harm result metrics replay");
            applications += ra.harm_applications; forced_hits += ra.chance_forced; glory_responses += ra.glory_defense;
            Env resumed(config); check(resumed.restore(a.snapshot()) && resumed.snapshot() == a.snapshot(),
                "all harm-profile states restore including forced historical events");
        }
    }
    check(applications > 0 && forced_hits > 0 && glory_responses > 0,
        "seeded fixtures exercise harm, forced chance and Glory-constrained response");
}
}
int main() {
    catalog_and_profile_boundaries(); glory_legal_response_and_sampling();
    dark_cloud_forcing_and_saved_rolls(); on_damage_duplicate_and_heaven();
    cure_interactions(); restoration_and_observations(); replay();
    std::cout << "Harm checks passed (Glory constraints, forced chances, cures/Heaven, 1000 replay rollouts).\n";
}
