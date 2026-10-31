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
constexpr Config config{512, 10, true, true, true, true, true, true, true};
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL celestial: " << message << '\n'; std::exit(1); }
}
Action attack(int base = 0, std::uint16_t extras = 0, int target = 1) {
    return {ActionKind::attack, static_cast<std::uint8_t>(base), extras, static_cast<std::uint8_t>(target)};
}
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend, no_slot, slots, no_slot}; }
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(), ids.end(), p.hand.begin());
    p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy) {
    Env env(config); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0], own); hand(snapshot.state.players[1], enemy);
    check(env.restore(snapshot), "restore explicit celestial fixture"); return env;
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "exact celestial snapshot roundtrip");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "illegal celestial action rejected without state or RNG mutation");
}
Element composed(Element a, Element b) {
    if (a == Element::light) return b;
    if (b == Element::light) return a;
    return a == b ? a : Element::neutral;
}
bool compatible(Element incoming, Element armor) {
    switch (incoming) {
    case Element::none: return false;
    case Element::neutral: case Element::dark: return true;
    case Element::fire: return armor == Element::water || armor == Element::light;
    case Element::water: return armor == Element::fire || armor == Element::light;
    case Element::wood: return armor == Element::stone || armor == Element::light;
    case Element::stone: return armor == Element::wood || armor == Element::light;
    case Element::light: return armor == Element::dark;
    }
    return false;
}
void catalog_and_dependencies() {
    constexpr std::array<CardId, 18> ids{62,63,70,75,76,82,83,87,88,89,93,96,97,101,104,111,158,176};
    check(celestial_card_count == 18 && celestial_catalog().size() == ids.size(), "pinned celestial allowlist size");
    int light = 0, dark = 0, armor = 0, chance = 0, harm = 0;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const auto& entry = celestial_catalog()[i];
        check(entry.card.id == ids[i] && find_celestial_card(ids[i]) == &entry, "pinned sorted IDs and exact lookup");
        check(!find_card(ids[i]) && !find_element_card(ids[i]) && !find_chance_card(ids[i])
            && !find_harm_card(ids[i]) && !find_disease_card(ids[i]) && !find_reaction_card(ids[i]),
            "new pool disjoint from all prior artifact pools");
        check(entry.card.weight == 1 && entry.card.hp == 0 && entry.card.mp == 0,
            "celestial cards preserve unit weights and no erased utility effects");
        check(entry.card.discardable == (entry.card.kind == Kind::armor), "weapon discard restriction preserved");
        light += entry.element == Element::light; dark += entry.element == Element::dark;
        armor += entry.card.kind == Kind::armor; chance += entry.hit_rate != 0; harm += entry.harm != Harm::none;
    }
    check(light == 10 && dark == 8 && armor == 2 && chance == 3 && harm == 1, "pinned celestial roles");
    check(find_celestial_card(62)->card.boost == 1 && find_celestial_card(89)->card.boost == 5
        && find_celestial_card(70)->card.attack == 2 && find_celestial_card(70)->harm == Harm::glory,
        "light/dark boost and Glory source metadata");
    check(find_celestial_card(158)->card.defense == 6 && find_celestial_card(176)->card.defense == 10,
        "only light armor included; no invented dark defense fixture");
    check(find_celestial_card(101)->hit_rate == 50 && find_celestial_card(101)->card.attack == 2
        && find_celestial_card(104)->hit_rate == 25 && find_celestial_card(104)->card.attack == 3
        && find_celestial_card(111)->hit_rate == 75 && find_celestial_card(111)->card.attack == 6,
        "pinned historical celestial chance percentages and powers");
    for (CardId id : {CardId{0}, CardId{43}, CardId{65}, CardId{85}, CardId{102}, CardId{191}, CardId{200}, CardId{65535}})
        check(!find_celestial_card(id), "unsupported override, counter, miracle and special effects excluded");
    for (int missing = 0; missing < 3; ++missing) {
        auto bad = config;
        if (missing == 0) { bad.elements = false; bad.disease = false; }
        if (missing == 1) bad.chance = false;
        if (missing == 2) bad.harms = false;
        bool threw = false; try { Env invalid(bad); } catch (const std::invalid_argument&) { threw = true; }
        check(threw, "celestial requires elements, chance and harms explicitly enabled");
    }
    auto env = fixture({70}, {158});
    check(std::string_view(env.profile()) == celestial_ruleset && env.snapshot().version == 37,
        "explicit legacy celestial profile and schema");
    roundtrip(env);
    const auto main = env.snapshot(); env.step(attack());
    auto invalid_harm = env.snapshot(); invalid_harm.state.pending.harm_card = 62;
    const auto original_pending = env.snapshot();
    check(!env.restore(invalid_harm) && env.snapshot() == original_pending,
        "plain light weapon cannot forge Glory provenance");
    check(env.restore(main), "resume main celestial catalog fixture");
    Env minimal(Config{512, 10, false, true, true, false, false, true, true});
    check(minimal.script_draws(ids), "celestial dependencies do not require miracles, reflection or disease");
    roundtrip(minimal);

    auto disabled_config = config; disabled_config.celestial = false; Env disabled(disabled_config);
    const auto original = disabled.snapshot();
    check(!disabled.script_draws(ids) && disabled.snapshot() == original, "disabled new draw pool rejected atomically");
    auto bad = original; bad.state.players[bad.state.actor].hand[0] = 62;
    check(!disabled.restore(bad) && disabled.snapshot() == original, "disabled celestial hand rejected");
    auto ordinary = fixture({5}, {120}); ordinary.step(attack());
    auto pending = ordinary.snapshot(); pending.config.celestial = false;
    for (auto element : {Element::light, Element::dark}) {
        pending.state.pending.element = element;
        check(!disabled.restore(pending) && disabled.snapshot() == original, "disabled celestial pending enum rejected");
    }
    bad = env.snapshot(); bad.version = 7;
    check(!env.restore(bad), "previous snapshot schema rejected");
}
void defense_matrix_and_composition() {
    // The strict pool contains no dark defense card; Light<-Dark is therefore
    // a source table rule rather than a reachable artificial defense fixture.
    constexpr std::array<Element, 6> defenses{Element::neutral,Element::fire,Element::water,
        Element::wood,Element::stone,Element::light};
    for (int incoming = 0; incoming < 7; ++incoming) {
        auto env = fixture({5}, {120,122,123,124,125,158}); env.step(attack());
        auto snapshot = env.snapshot(); snapshot.state.pending.element = static_cast<Element>(incoming);
        check(env.restore(snapshot), "restore source-table incoming element fixture"); roundtrip(env);
        for (unsigned mask = 0; mask < 64; ++mask) {
            Element combined = Element::light;
            for (int i = 0; i < 6; ++i) if (mask & (1u << i)) combined = composed(combined, defenses[i]);
            const bool valid = !mask || compatible(static_cast<Element>(incoming), combined);
            check(env.legal(defend(mask)) == valid, "7x6 reachable defense matrix and all composed subsets");
            if (valid) {
                check((mask & env.options().defense_slots) == mask, "candidate filter retains every legal composed choice");
                auto branch = env; check(branch.step(defend(mask)).valid, "legal celestial defense executes"); roundtrip(branch);
            } else reject(env, defend(mask));
        }
    }
    constexpr std::array<CardId, 7> bases{5,58,59,60,69,62,63};
    constexpr std::array<CardId, 7> boosts{9,84,95,68,81,62,89};
    for (int base = 0; base < 7; ++base) for (int first = 0; first < 7; ++first) for (int second = 0; second < 7; ++second) {
        auto env = fixture({bases[base], boosts[first], boosts[second]}, {120});
        check(env.step(attack(0, 6)).valid, "all ordinary celestial boosts compose");
        const auto expected = composed(composed(static_cast<Element>(base), static_cast<Element>(first)), static_cast<Element>(second));
        check(env.state().pending.element == expected, "light identity and sticky neutral composition across all seven attributes");
        roundtrip(env);
        auto reversed = fixture({bases[base], boosts[second], boosts[first]}, {120}); reversed.step(attack(0, 6));
        check(reversed.state().pending.element == expected, "ordinary boost composition remains order-independent");
    }
}
void dark_resolution() {
    auto partial = fixture({97}, {158}); partial.step(attack());
    const auto killed = partial.step(defend(1));
    check(killed.dark_execution && killed.terminated && partial.state().players[1].hp == 0
        && killed.rewards == std::array<float,2>{1.f,-1.f}, "positive residual dark damage executes after partial defense");
    auto full = fixture({97}, {176}); full.step(attack()); const auto blocked = full.step(defend(1));
    check(!blocked.dark_execution && !blocked.terminated && full.state().players[1].hp == 40,
        "fully blocked dark attack neither damages nor executes");
    auto zero = fixture({63}, {118}); zero.step(attack());
    check(zero.step(defend(1)).dark_execution && zero.state().players[1].hp == 0,
        "zero-strength armor is legal against dark and does not prevent execution");
    auto self = fixture({63}, {120}); const auto self_result = self.step(attack(0, 0, 0));
    check(self_result.dark_execution && self_result.terminated && self.state().winner == 1,
        "self-target dark attack resolves immediately and executes source");
    auto mixed = fixture({63,9}, {120}); mixed.step(attack(0,2)); const auto mixed_result = mixed.step(defend());
    check(!mixed_result.dark_execution && mixed.state().players[1].hp == 38,
        "dark mixed with neutral boost loses execution together with dark attribute");
    auto light_boost = fixture({63,62}, {120}); light_boost.step(attack(0,2));
    check(light_boost.state().pending.element == Element::dark && light_boost.step(defend()).dark_execution,
        "light boost preserves dark attribute and execution");
    auto reflected = fixture({63,9}, {39}); reflected.step(attack(0,2));
    check(reflected.step({ActionKind::defend,0,0,no_slot}).reflected, "neutralized dark attack may use existing neutral reflector");
    const auto reflected_result = reflected.step(defend());
    check(!reflected_result.dark_execution && reflected.state().players[0].hp == 38,
        "reflected neutralized dark attack does not recover execution");
    roundtrip(partial); roundtrip(full); roundtrip(zero); roundtrip(self); roundtrip(mixed); roundtrip(reflected);
}
void glory_source_and_cures() {
    constexpr std::array<CardId,7> boosts{9,84,95,68,81,62,89};
    for (int index = 0; index < 7; ++index) {
        auto env = fixture({70,boosts[index]}, {120}); env.step(attack(0,2));
        check(env.state().pending.harm_card == 70 && env.state().pending.harm == Harm::glory
            && env.state().pending.element == static_cast<Element>(index), "Glory source payload survives light substitution");
        roundtrip(env); const auto result = env.step(defend());
        check(result.harm_new_flags == 1 && env.state().players[1].glory,
            "positive damage applies Glory even after final attribute changes");
        roundtrip(env);
    }
    auto blocked = fixture({70,84}, {176}); blocked.step(attack(0,2)); const auto prevented = blocked.step(defend(1));
    check(!prevented.harm_applications && !blocked.state().players[1].glory,
        "light armor full block of fire-substituted Glory prevents status");
    auto reflected = fixture({70,9}, {39}); reflected.step(attack(0,2));
    check(reflected.step({ActionKind::defend,0,0,no_slot}).reflected, "neutralized Glory attack is reflectable");
    roundtrip(reflected); reflected.step(defend());
    check(reflected.state().players[0].glory && !reflected.state().players[1].glory
        && reflected.state().players[0].hp == 37, "reflected Glory applies to original owner");
    auto natural = fixture({70}, {235}); natural.step(attack()); natural.step(defend());
    check(natural.state().players[1].glory && natural.step({ActionKind::use,0,0,1}).valid
        && !natural.state().players[1].glory, "naturally acquired Glory can be removed by lower medicine");
    auto heaven = fixture({70}, {120}); auto snapshot = heaven.snapshot();
    snapshot.state.players[1].disease = Disease::heaven; snapshot.state.players[1].worse_chance = 50;
    check(heaven.restore(snapshot), "restore Heaven Glory recipient"); heaven.step(attack());
    const auto fell = heaven.step(defend());
    check(fell.harm_heaven_falls == 1 && heaven.state().players[1].hp == 0
        && !heaven.state().players[1].glory, "Glory source invokes Heaven fall before attaching new harm");
    roundtrip(heaven);
}
void chance_variants() {
    for (CardId id : {CardId{101},CardId{104},CardId{111}}) {
        const auto& record = *find_celestial_card(id);
        for (int roll : {1,record.hit_rate,record.hit_rate+1,100}) {
            auto env = fixture({id,62}, {120});
            const std::uint8_t prefix[] = {static_cast<std::uint8_t>(roll)};
            check(env.script_hit_rolls(prefix), "install celestial chance boundary roll");
            reject(env, attack(0,2));
            const auto result = env.step(attack(0,0,0));
            const bool hit = roll <= record.hit_rate;
            check(result.chance_attempt && result.chance_hit == hit && !result.chance_forced
                && env.snapshot().hit_index == 1 && env.state().last_chance.id == id,
                "all celestial chance weapons use original percentages and normalized enemy target");
            roundtrip(env);
            if (hit) {
                check(env.state().pending.element == record.element && env.state().pending.target == 1,
                    "hit retains celestial element and target");
                const auto resolved = env.step(defend());
                check(resolved.dark_execution == (record.element == Element::dark), "dark chance hit executes; light hit uses ordinary damage");
                roundtrip(env);
            }
        }
        auto forced = fixture({id}, {120}); auto snapshot = forced.snapshot(); snapshot.state.players[1].dark_cloud = true;
        check(forced.restore(snapshot), "restore forced celestial chance target");
        const std::uint8_t miss[] = {100}; check(forced.script_hit_rolls(miss), "retain unconsumed forced roll");
        const auto rng = forced.snapshot().rng_state; const auto result = forced.step(attack());
        check(result.chance_forced && result.chance_hit && forced.snapshot().hit_index == 0
            && forced.snapshot().rng_state == rng, "DarkCloud forces celestial variants without tape or RNG use");
        roundtrip(forced);
    }
}
void observation_and_privacy() {
    auto env = fixture({70,63,58}, {120}); const auto own = env.observe();
    check(own[283] == 1.f && own[285] == 5/6.f && own[286] == 1.f && own[287] == 1/6.f,
        "new hand suffix represents all seven attributes");
    check(own[169] == 0.f && own[170] == 0.f && own[171] == 1/4.f,
        "legacy hand scalar preserves prior attributes and zeroes light/dark");
    env.step(attack()); const auto pending = env.observe();
    check(pending[284] == 5/6.f && std::all_of(pending.begin()+164,pending.begin()+169,[](float x){return x==0.f;}),
        "light pending uses suffix without overwriting legacy five-way onehot");
    check(pending[169] == 0.f && pending[170] == 0.f, "pending light does not overwrite own hand scalar slots");
    auto dark = fixture({63}, {120}); dark.step(attack()); check(dark.observe()[284] == 1.f, "dark pending suffix explicit");
    auto bad = dark.snapshot(); bad.state.pending.element = static_cast<Element>(255);
    const auto original = dark.snapshot(); check(!dark.restore(bad) && dark.snapshot() == original, "unknown extended element rejected atomically");

    auto hidden = fixture({5}, {70}); const auto visible = hidden.observe(); const auto options = hidden.options();
    auto snapshot = hidden.snapshot(); snapshot.state.players[1].hand[0] = 63;
    check(hidden.restore(snapshot) && hidden.observe() == visible && hidden.options() == options,
        "enemy celestial IDs and hand attributes remain private");
    auto chance = fixture({101}, {158}); const std::uint8_t rolls[] = {1,100};
    check(chance.script_hit_rolls(rolls) && chance.step(attack()).chance_hit, "install public celestial chance hit with future roll");
    auto future = chance.snapshot(); future.scripted_hits[1] = 1; future.rng_state ^= 0x9e3779b97f4a7c15ULL;
    Env concealed(config); check(concealed.restore(future) && concealed.observe() == chance.observe()
        && concealed.options() == chance.options(), "celestial chance future tape and RNG remain hidden");
    auto disabled_config = config; disabled_config.celestial = false; Env disabled(disabled_config);
    const auto old = disabled.observe();
    check(std::all_of(old.begin()+283,old.end(),[](float x){return x==0.f;}), "disabled celestial suffix all zero");
}
void replay() {
    std::uint64_t celestial_plays = 0, glory = 0, executions = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed);
        std::uint64_t pa = seed+17500, pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "celestial seeded trajectory equality");
            for (float value : a.observe()) check(std::isfinite(value) && value >= 0.f && value <= 1.f,
                "celestial observation finite and normalized");
            const auto action = a.sample_action(pa), copy = b.sample_action(pb);
            check(action == copy && pa == pb && a.legal(action), "celestial policy sampling legal and deterministic");
            if (action.kind == ActionKind::attack && find_celestial_card(a.state().players[a.state().actor].hand[action.base])) ++celestial_plays;
            const auto ra = a.step(action), rb = b.step(copy);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.chance_hit == rb.chance_hit
                && ra.chance_forced == rb.chance_forced && ra.dark_execution == rb.dark_execution
                && ra.harm_new_flags == rb.harm_new_flags, "celestial transition metrics replay");
            glory += ra.harm_new_flags; executions += ra.dark_execution; roundtrip(a);
        }
    }
    check(celestial_plays && glory && executions, "seed suite exercises natural celestial cards, status acquisition and dark execution");
}
}
int main() {
    catalog_and_dependencies(); defense_matrix_and_composition(); dark_resolution();
    glory_source_and_cures(); chance_variants(); observation_and_privacy(); replay();
    std::cout << "Celestial checks passed (legacy matrix, light composition, dark execution, Glory, 1000 replay rollouts).\n";
}
