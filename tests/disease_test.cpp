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
void check(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL disease: " << message << '\n'; std::exit(1); }
}
Action attack(int base = 0, std::uint16_t extras = 0, int target = 1) {
    return {ActionKind::attack, static_cast<std::uint8_t>(base), extras, static_cast<std::uint8_t>(target)};
}
Action use(int base = 0, int target = 0) {
    return {ActionKind::use, static_cast<std::uint8_t>(base), 0, static_cast<std::uint8_t>(target)};
}
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend, no_slot, slots, no_slot}; }
Action reflect(int base, std::uint16_t prefix = 0) {
    return {ActionKind::defend, static_cast<std::uint8_t>(base), prefix, no_slot};
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(), ids.end(), p.hand.begin());
    p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy,
            Config config = Config{512, 10, false, true, false, false, true}) {
    Env env(config); auto snapshot = env.snapshot(); snapshot.state = State{};
    snapshot.scripted_draws.fill(0); snapshot.draw_count = snapshot.draw_index = 0;
    snapshot.scripted_hits.fill(0); snapshot.hit_count = snapshot.hit_index = 0;
    snapshot.scripted_targets.fill(0); snapshot.target_count = snapshot.target_index = 0;
    snapshot.scripted_diseases.fill(0); snapshot.disease_count = snapshot.disease_index = 0;
    hand(snapshot.state.players[0], own); hand(snapshot.state.players[1], enemy);
    check(env.restore(snapshot), "restore valid disease fixture"); return env;
}
void condition(Env& env, int seat, Disease disease, std::uint8_t chance = 0, int hp = 40) {
    auto snapshot = env.snapshot();
    snapshot.state.players[seat].disease = disease; snapshot.state.players[seat].worse_chance = chance;
    snapshot.state.players[seat].hp = hp;
    check(env.restore(snapshot), "restore disease and worsening counter");
}
void script(Env& env, std::initializer_list<CardId> draws, std::initializer_list<std::uint8_t> diseases) {
    check(env.script_draws({draws.begin(), draws.size()}), "script supported draws");
    check(env.script_disease_rolls({diseases.begin(), diseases.size()}), "script natural worsening outcomes");
}
void reject(Env& env, Action action, const char* message) {
    const auto original = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid, message);
    check(env.snapshot() == original, "invalid action preserves diseases, all tapes and RNG");
}
void catalog_and_profiles() {
    check(disease_card_count == 8 && disease_catalog().size() == 8, "eight strict disease records");
    int total_weight = 0;
    for (const auto& record : disease_catalog()) {
        check(find_disease_card(record.card.id) == &record, "disease ID lookup");
        check(!find_card(record.card.id) && !find_element_card(record.card.id)
            && !find_miracle(record.card.id) && !find_chance_card(record.card.id)
            && !find_reaction_card(record.card.id), "disease pool is disjoint from prior pools");
        check(record.disease != Disease::none || record.cure != Cure::none, "no erased disease effect");
        total_weight += record.card.weight;
    }
    check(total_weight == 24, "pinned disease weights");
    check(find_disease_card(34)->disease == Disease::cold && find_disease_card(34)->card.attack == 9,
        "Cold-on-damage NE weapon metadata");
    check(find_disease_card(175)->on_defense && find_disease_card(175)->disease == Disease::fever
        && find_disease_card(175)->card.defense == 10 && find_disease_card(175)->element == Element::fire,
        "Fever-on-defense armor metadata");
    check(find_disease_card(234)->card.mp == 20 && find_disease_card(234)->disease == Disease::heaven,
        "Heaven MP item metadata");
    check(find_disease_card(235)->cure == Cure::lower && find_disease_card(236)->cure == Cure::all,
        "distinct partial and complete cure metadata");
    for (CardId id : {CardId{0}, CardId{37}, CardId{43}, CardId{46}, CardId{197}, CardId{237}, CardId{65535}})
        check(!find_disease_card(id), "harms, unsupported miracles and random items excluded");
    auto enabled = fixture({34}, {120});
    check(std::string_view(enabled.profile()) == disease_ruleset && enabled.snapshot().version == 37,
        "disease profile and snapshot schema identified");
    bool threw = false;
    try { Env invalid(Config{512, 10, false, false, false, false, true}); }
    catch (const std::invalid_argument&) { threw = true; }
    check(threw, "disease palette requires element semantics for its colored records");
    for (const Config config : {Config{}, Config{512, 10, true, true, true, true}}) {
        Env disabled(config); const auto before = disabled.snapshot();
        const CardId new_card[] = {34}; const std::uint8_t roll[] = {0};
        check(!disabled.script_draws(new_card) && disabled.snapshot() == before, "disabled disease artifact rejected");
        check(!disabled.script_disease_rolls(roll) && disabled.snapshot() == before, "disabled nonempty zero disease tape rejected");
        check(disabled.script_disease_rolls({}), "disabled empty tape clear permitted");
        auto bad = before; bad.state.players[0].disease = Disease::cold;
        check(!disabled.restore(bad) && disabled.snapshot() == before, "disabled disease state rejected atomically");
        bad = before; bad.scripted_diseases[0] = 0; bad.disease_count = 1;
        check(!disabled.restore(bad) && disabled.snapshot() == before, "disabled disease tape snapshot rejected");
    }
    const auto before = enabled.snapshot(); auto old = before; old.version = 5;
    check(!enabled.restore(old) && enabled.snapshot() == before, "old snapshot version not silently migrated");
}
void natural_ticks_and_thresholds() {
    struct TickCase { Disease disease; int hp; int damage; int healing; };
    constexpr std::array<TickCase, 4> cases{{
        {Disease::cold, 39, 1, 0}, {Disease::fever, 38, 2, 0},
        {Disease::hell, 35, 5, 0}, {Disease::heaven, 45, 0, 5}
    }};
    for (const auto& c : cases) {
        auto env = fixture({120}, {120}); condition(env, 0, c.disease);
        script(env, {120}, {0});
        const auto result = env.step({ActionKind::pray});
        check(result.valid && result.disease_tick && !result.disease_worsened
            && result.disease_damage == c.damage && result.disease_healing == c.healing,
            "legacy disease tick reports its fixed effect");
        check(env.state().players[0].hp == c.hp && env.state().players[0].worse_chance == 1,
            "chance zero consumes a failing roll then increments to one");
        check(env.snapshot().disease_index == 1 && env.state().last_tick.seat == 0
            && env.state().last_tick.before == c.disease && env.state().last_tick.after == c.disease,
            "natural tick recorded once for normal turn owner");
    }
    for (std::uint8_t roll : {std::uint8_t{0}, std::uint8_t{1}}) {
        auto env = fixture({120}, {120}); condition(env, 0, Disease::cold, 1);
        script(env, {120}, {roll}); env.step({ActionKind::pray});
        const bool worse = roll == 0;
        check(env.state().players[0].disease == (worse ? Disease::fever : Disease::cold)
            && env.state().players[0].hp == (worse ? 38 : 39)
            && env.state().players[0].worse_chance == (worse ? 0 : 2),
            "natural worsening uses roll below current counter, not a fixed five percent");
    }
    auto upper = fixture({120}, {120}); condition(upper, 0, Disease::cold, 99);
    script(upper, {120}, {99}); upper.step({ActionKind::pray});
    check(upper.state().players[0].worse_chance == 100 && upper.state().players[0].disease == Disease::cold,
        "failed ninety-nine percent roll reaches exact counter100");
    for (Disease disease : {Disease::cold, Disease::fever, Disease::hell, Disease::heaven}) {
        auto env = fixture({120}, {120}); condition(env, 0, disease, 100);
        script(env, {120}, {99}); const auto result = env.step({ActionKind::pray});
        check(result.disease_tick && result.disease_worsened && env.snapshot().disease_index == 1,
            "counter100 consumes a roll and always worsens");
        check(env.state().players[0].worse_chance == 0, "worsening resets legacy counter");
        if (disease == Disease::cold) check(env.state().players[0].disease == Disease::fever
            && env.state().players[0].hp == 38, "Cold upgrade applies Fever damage immediately");
        if (disease == Disease::fever) check(env.state().players[0].disease == Disease::hell
            && env.state().players[0].hp == 35, "Fever upgrade applies Hell damage immediately");
        if (disease == Disease::hell) check(env.state().players[0].disease == Disease::heaven
            && env.state().players[0].hp == 45, "Hell upgrade applies Heaven healing immediately");
        if (disease == Disease::heaven) check(result.terminated && env.state().players[0].hp == 0
            && env.state().winner == 1 && env.snapshot().draw_index == 0,
            "Heaven worsens to death before healing or acquiring replacement");
    }
    auto capped = fixture({120}, {120}); condition(capped, 0, Disease::heaven, 0, 98);
    script(capped, {120}, {99}); capped.step({ActionKind::pray});
    check(capped.state().players[0].hp == 99, "Heaven healing respects HP cap");
}
void weapon_status_and_reapplication() {
    for (Disease old : {Disease::none, Disease::cold, Disease::fever, Disease::hell, Disease::heaven}) {
        auto env = fixture({34}, {120});
        condition(env, 1, old, old == Disease::none ? 0 : 50);
        env.step(attack());
        check(env.state().pending.status_card == 34 && env.state().pending.disease == Disease::cold,
            "on-damage status provenance carried while awaiting defense");
        const auto result = env.step(defend());
        const auto& target = env.state().players[1];
        if (old == Disease::heaven) check(result.terminated && target.hp == 0 && target.worse_chance == 0,
            "any disease reapplied to Heaven causes death");
        else {
            const auto expected = old == Disease::none ? Disease::cold
                : static_cast<Disease>(static_cast<int>(old) + 1);
            check(target.disease == expected && target.worse_chance == 0 && target.hp == 31,
                "same or lower disease promotes existing level and resets counter");
        }
        check(!result.disease_tick && env.snapshot().disease_index == 0,
            "new disease on nonowner does not tick on attacker's turn");
    }
    auto skip = fixture({30}, {120}); condition(skip, 1, Disease::cold, 30);
    skip.step(attack()); skip.step(defend());
    check(skip.state().players[1].disease == Disease::hell && skip.state().players[1].hp == 32
        && skip.state().players[1].worse_chance == 0, "higher incoming disease skips weaker levels");

    auto blocked = fixture({34}, {186}); condition(blocked, 1, Disease::heaven, 50);
    blocked.step(attack()); blocked.step(defend(1));
    check(blocked.state().players[1].hp == 40 && blocked.state().players[1].disease == Disease::heaven
        && blocked.state().players[1].worse_chance == 50, "full defense prevents on-damage application and Heaven reapplication death");

    auto boosted = fixture({34, 9}, {120}); boosted.step(attack(0, 2));
    Env resumed(Config{512, 10, false, true, false, false, true});
    check(boosted.state().pending.amount == 10 && resumed.restore(boosted.snapshot()),
        "disease weapon can carry ordinary boosts and retain valid status provenance");
    boosted.step(defend());
    check(boosted.state().players[1].hp == 30 && boosted.state().players[1].disease == Disease::cold,
        "boosted damage and on-damage condition both resolve");

    auto self = fixture({34}, {120}); script(self, {120}, {0});
    const auto result = self.step(attack(0, 0, 0));
    check(result.disease_tick && self.state().players[0].hp == 30
        && self.state().players[0].disease == Disease::cold && self.state().players[0].worse_chance == 1,
        "self status attack is followed by owner's new disease tick");
}
void medicines_and_direct_application() {
    for (Disease old : {Disease::cold, Disease::fever, Disease::hell, Disease::heaven}) {
        auto lower = fixture({235}, {120}); condition(lower, 0, old, 20);
        script(lower, {120}, {99}); const auto result = lower.step(use());
        if (old == Disease::cold || old == Disease::fever)
            check(lower.state().players[0].disease == Disease::none && lower.state().players[0].worse_chance == 0
                && !result.disease_tick && lower.snapshot().disease_index == 0,
                "lower cure removes only lower disease and prevents that turn's tick");
        else check(lower.state().players[0].disease == old && lower.state().players[0].worse_chance == 21
            && result.disease_tick && lower.snapshot().disease_index == 1,
            "lower cure does not reset severe disease counter before ordinary tick");
        auto all = fixture({236}, {120}); condition(all, 0, old, 100);
        script(all, {120}, {0}); const auto cured = all.step(use());
        check(cured.valid && all.state().players[0].disease == Disease::none
            && all.state().players[0].worse_chance == 0 && all.state().players[0].hp == 40
            && !cured.disease_tick && all.snapshot().disease_index == 0,
            "complete cure removes Heaven without treating none as disease reapplication");
    }
    auto other = fixture({236}, {}); condition(other, 1, Disease::hell, 60);
    check(other.step(use(0, 1)).valid && other.state().pending.kind == Effect::cure
        && other.state().pending.amount == 0 && other.state().pending.cure == Cure::all,
        "targeted cure has a zero-amount response distinct from damage");
    const auto obs = other.observe();
    check(obs[257] == 1.f && obs[247] == 1.f && obs[17] == 0.f,
        "cure indicator does not corrupt empty hand features");
    Env resumed(Config{512, 10, false, true, false, false, true});
    check(resumed.restore(other.snapshot()), "zero-amount cure response snapshot valid");
    check(other.step(defend()).valid && other.state().players[1].disease == Disease::none
        && other.state().players[1].worse_chance == 0, "other-target cure applies after acceptance");

    auto heaven = fixture({234}, {120}); script(heaven, {120}, {99});
    const auto result = heaven.step(use());
    check(result.disease_tick && heaven.state().players[0].mp == 30 && heaven.state().players[0].hp == 45
        && heaven.state().players[0].disease == Disease::heaven && heaven.state().players[0].worse_chance == 1,
        "direct MP item applies Heaven before owner's end-turn tick");
    auto repeated = fixture({234}, {120}); condition(repeated, 0, Disease::heaven, 20);
    script(repeated, {120}, {0}); const auto died = repeated.step(use());
    check(died.terminated && repeated.state().players[0].hp == 0 && repeated.state().players[0].mp == 30
        && !died.disease_tick && repeated.snapshot().disease_index == 0,
        "direct Heaven reapplication kills before natural tick and cannot revive itself");
}
void reflection_defense_and_owner_tick() {
    const Config combined{512, 10, false, true, false, true, true};
    auto reflected = fixture({34}, {39}, combined); script(reflected, {120, 120}, {0});
    reflected.step(attack());
    const auto redirect = reflected.step(reflect(0));
    check(redirect.reflected && !redirect.disease_tick && reflected.snapshot().disease_index == 0
        && reflected.state().pending.status_card == 34 && reflected.state().pending.disease == Disease::cold,
        "reflection preserves on-damage status without ticking during the chain");
    Env resumed(combined); check(resumed.restore(reflected.snapshot()), "status-bearing reflected response valid");
    const auto result = reflected.step(defend());
    check(result.disease_tick && reflected.state().players[0].hp == 30
        && reflected.state().players[0].disease == Disease::cold
        && reflected.state().players[0].worse_chance == 1, "reflected status damages and then ticks original turn owner");

    auto armor = fixture({5}, {175}, combined);
    armor.step(attack()); const auto blocked = armor.step(defend(1));
    check(armor.state().players[1].hp == 40 && armor.state().players[1].disease == Disease::fever
        && armor.state().players[1].worse_chance == 0 && !blocked.disease_tick,
        "defense disease applies even after full block, without ticking nonowner");

    auto prefix = fixture({5}, {175, 39}, combined); condition(prefix, 1, Disease::heaven);
    script(prefix, {120}, {0}); prefix.step(attack());
    check(prefix.step(reflect(1, 1)).valid && prefix.state().phase == Phase::response
        && prefix.state().players[1].hp == 0 && prefix.state().players[1].disease == Disease::heaven
        && prefix.state().players[1].worse_chance == 0,
        "Heaven disease armor can provisionally kill redirector before source death processing");
    check(resumed.restore(prefix.snapshot()), "provisional HP0 Heaven redirect chain restorable");
    check(prefix.step(defend()).terminated && prefix.state().winner == 0
        && prefix.state().players[0].hp == 39 && prefix.snapshot().disease_index == 0,
        "provisional dead redirector is finalized after remaining attack, no off-owner tick");

    auto both = fixture({5, 175, 39}, {175, 39}, combined);
    condition(both, 0, Disease::heaven); condition(both, 1, Disease::heaven);
    script(both, {120}, {0}); both.step(attack());
    check(both.step(reflect(1, 1)).valid && both.step(reflect(1, 1)).valid
        && both.state().players[0].hp == 0 && both.state().players[1].hp == 0
        && both.state().phase == Phase::response,
        "two Heaven defense prefixes can leave both players provisionally HP0 during a chain");
    check(resumed.restore(both.snapshot()), "both provisionally dead redirected response restorable");
    const auto both_died = both.step(defend());
    check(both_died.terminated && both.state().winner == -1 && !both_died.disease_tick
        && both_died.rewards == std::array<float, 2>{} && both.snapshot().draw_index == 0
        && both.snapshot().disease_index == 0,
        "deferred double-death chain finalizes without gifts, natural tick or invented survivor");

    auto chain = fixture({5, 39}, {39}, combined); condition(chain, 0, Disease::cold);
    script(chain, {120, 120, 120}, {0}); chain.step(attack());
    check(!chain.step(reflect(0)).disease_tick && !chain.step(reflect(0)).disease_tick
        && chain.snapshot().disease_index == 0, "multiple reactions do not multiply disease ticks");
    check(chain.step(defend()).disease_tick && chain.state().players[0].hp == 39
        && chain.snapshot().disease_index == 1 && chain.state().last_tick.seat == 0,
        "only normal owner ticks once after entire chain");
}
void terminal_order_and_draws() {
    auto draw = fixture({56}, {120}, Config{2, 10, false, true, false, false, true});
    condition(draw, 0, Disease::cold, 0, 1);
    auto snapshot = draw.snapshot(); snapshot.state.players[1].hp = 1;
    check(draw.restore(snapshot), "restore simultaneous death fixture"); script(draw, {120}, {0});
    draw.step(attack()); const auto result = draw.step(defend());
    check(result.terminated && !result.truncated && draw.state().winner == -1
        && draw.state().players[0].hp == 0 && draw.state().players[1].hp == 0
        && result.rewards == std::array<float, 2>{}, "enemy death followed by owner disease death is a terminal learning draw");
    check(draw.snapshot().draw_index == 0 && draw.snapshot().disease_index == 1,
        "no dead player receives scheduled artifact after simultaneous death");
    Env restored(Config{2, 10, false, true, false, false, true});
    check(restored.restore(draw.snapshot()), "terminated no-winner simultaneous death snapshot valid");

    auto heaven = fixture({56}, {120}); condition(heaven, 0, Disease::heaven);
    snapshot = heaven.snapshot(); snapshot.state.players[1].hp = 1;
    check(heaven.restore(snapshot), "restore win-before-Heaven-tick fixture"); script(heaven, {120}, {99});
    heaven.step(attack()); const auto won = heaven.step(defend());
    check(won.terminated && heaven.state().winner == 0 && heaven.state().players[0].hp == 45
        && heaven.snapshot().disease_index == 1 && heaven.snapshot().draw_index == 1,
        "owner Heaven still heals after enemy dies, before distribution and winner finalization");

    auto dead_owner = fixture({56}, {39}, Config{512, 10, false, true, false, true, true});
    condition(dead_owner, 0, Disease::heaven, 0, 1); script(dead_owner, {120}, {0});
    dead_owner.step(attack()); dead_owner.step(reflect(0)); const auto reflected_death = dead_owner.step(defend());
    check(reflected_death.terminated && dead_owner.state().winner == 1
        && dead_owner.state().players[0].hp == 0 && !reflected_death.disease_tick
        && dead_owner.snapshot().disease_index == 0, "already dead normal owner skips Heaven tick and cannot revive");
}
void restoration_tapes_and_privacy() {
    auto env = fixture({34}, {120}); condition(env, 0, Disease::cold, 30);
    script(env, {120}, {0, 99}); const auto original = env.snapshot();
    for (std::uint8_t value : {std::uint8_t{100}, std::uint8_t{255}}) {
        const std::uint8_t invalid_roll[] = {value};
        check(!env.script_disease_rolls(invalid_roll) && env.snapshot() == original,
            "out-of-range natural roll rejected atomically");
    }
    std::array<std::uint8_t, 65> too_many{};
    check(!env.script_disease_rolls(too_many) && env.snapshot() == original, "oversized disease tape rejected atomically");
    auto bad = original; bad.state.players[0].disease = static_cast<Disease>(255);
    check(!env.restore(bad) && env.snapshot() == original, "unknown disease enum rejected");
    bad = original; bad.state.players[0].worse_chance = 101;
    check(!env.restore(bad) && env.snapshot() == original, "natural counter above guaranteed threshold rejected");
    bad = original; bad.state.players[1].worse_chance = 1;
    check(!env.restore(bad) && env.snapshot() == original, "disease-none requires zero worsening counter");
    bad = original; bad.disease_index = 3;
    check(!env.restore(bad) && env.snapshot() == original, "disease tape cursor checked");
    bad = original; bad.scripted_diseases[63] = 99;
    check(!env.restore(bad) && env.snapshot() == original, "unused disease tape tail canonical");
    bad = original; bad.state.last_tick = State::LastDiseaseTick{0, Disease::cold, Disease::heaven, false, 0, 5};
    check(!env.restore(bad) && env.snapshot() == original, "non-worsened public event cannot change disease level");
    bad = original; bad.state.last_tick = State::LastDiseaseTick{0, Disease::cold, Disease::hell, true, 5, 0};
    check(!env.restore(bad) && env.snapshot() == original, "natural worsening public event cannot skip levels");
    bad = original; bad.state.last_tick = State::LastDiseaseTick{0, Disease::cold, Disease::cold, false, 2, 0};
    check(!env.restore(bad) && env.snapshot() == original, "Cold tick public damage cannot exceed one");
    bad = original; bad.state.last_tick = State::LastDiseaseTick{0, Disease::heaven, Disease::heaven, false, 0, 6};
    check(!env.restore(bad) && env.snapshot() == original, "ordinary Heaven public healing cannot exceed five");
    bad = original; bad.state.last_tick = State::LastDiseaseTick{0, Disease::heaven, Disease::heaven, true, 0, 0};
    check(!env.restore(bad) && env.snapshot() == original, "Heaven worsening death public event requires positive actual damage");
    auto historical = original;
    historical.state.last_tick = State::LastDiseaseTick{1, Disease::hell, Disease::heaven, true, 0, 5};
    check(env.restore(historical), "consistent historical tick need not equal current player disease");
    check(env.restore(original), "restore original fixture after historical event check");
    auto hidden = original;
    hidden.state.players[0].worse_chance = 90;
    hidden.scripted_diseases[0] = 99; hidden.scripted_diseases[1] = 0;
    hidden.rng_state ^= 0x9e3779b97f4a7c15ULL;
    Env other(Config{512, 10, false, true, false, false, true});
    check(other.restore(hidden), "restore hidden future worsening and private counter variant");
    check(other.observe() == env.observe() && other.options() == env.options(),
        "private worsening counter, future tape and RNG do not leak to policy");
    reject(env, attack(0, 1), "invalid attack does not consume natural worsening tape");

    auto pending = fixture({34}, {120}); pending.step(attack()); const auto response = pending.snapshot();
    bad = response; bad.state.pending.disease = Disease::hell;
    check(!pending.restore(bad) && pending.snapshot() == response, "status effect must match original artifact");
    bad = response; bad.state.pending.status_card = 0;
    check(!pending.restore(bad) && pending.snapshot() == response, "status provenance required for pending disease");
    auto hp_zero = original; hp_zero.state.players[0].hp = 0;
    check(!env.restore(hp_zero), "nonterminal main state cannot contain provisionally dead player");
}
void observations_and_replay() {
    auto env = fixture({34}, {120}); condition(env, 0, Disease::cold); condition(env, 1, Disease::hell);
    const auto obs = env.observe();
    check(obs[237] == 1.f && obs[244] == 1.f && obs[248] == 1.f,
        "acting and opponent public disease states represented");
    script(env, {120}, {99}); env.step(attack()); const auto incoming = env.observe();
    check(incoming[246] == 1 / 4.f && incoming[256] == 34 / 363.f,
        "pending on-damage disease and source artifact represented");
    const auto result = env.step(defend()); const auto after = env.observe();
    check(result.disease_tick && after[249] == 1.f && after[252] == 1 / 4.f
        && after[253] == 1 / 4.f && after[254] == 1 / 99.f,
        "public last owner tick represented without raw roll");
    Env disabled; const auto old = disabled.observe();
    check(std::all_of(old.begin() + 236, old.end(), [](float value) { return value == 0.f; }),
        "old profiles have an all-zero disease suffix");

    std::uint64_t ticks = 0;
    for (bool features : {false, true}) for (std::uint64_t seed = 0; seed < 500; ++seed) {
        const Config config{256, 10, features, true, features, features, true};
        Env a(config), b(config); a.reset(seed); b.reset(seed);
        const std::uint8_t rolls[] = {0, 99};
        check(a.script_disease_rolls(rolls) && b.script_disease_rolls(rolls), "seeded finite disease prefixes");
        std::uint64_t pa = seed + 12000, pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "disease trajectory replay");
            for (float x : a.observe()) check(std::isfinite(x), "finite disease observation");
            const auto action = a.sample_action(pa), action_b = b.sample_action(pb);
            check(action == action_b && pa == pb && a.legal(action), "sampled disease action legal and reproducible");
            const auto ra = a.step(action), rb = b.step(action_b);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.disease_tick == rb.disease_tick
                && ra.disease_worsened == rb.disease_worsened && ra.disease_damage == rb.disease_damage
                && ra.disease_healing == rb.disease_healing, "disease results replay");
            ticks += ra.disease_tick;
            Env resumed(config); check(resumed.restore(a.snapshot()) && resumed.snapshot() == a.snapshot(),
                "every disease-profile state restorable including draws and temporary dead chains");
        }
    }
    check(ticks > 0, "seeded rollouts exercise actual natural disease effects");
}
}
int main() {
    catalog_and_profiles(); natural_ticks_and_thresholds(); weapon_status_and_reapplication();
    medicines_and_direct_application(); reflection_defense_and_owner_tick(); terminal_order_and_draws();
    restoration_tapes_and_privacy(); observations_and_replay();
    std::cout << "Disease checks passed (legacy progression, cures/status/redirects, terminal draws, 1000 replay rollouts).\n";
}
