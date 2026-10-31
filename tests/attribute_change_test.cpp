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
#include <unordered_map>

namespace {
using namespace godfield::legacy;
constexpr Config config{512,10,true,true,true,true,true,true,true,true,true,true};
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL attribute change: " << message << '\n'; std::exit(1); }
}
Action attack(ActionKind kind = ActionKind::attack, std::uint16_t slots = 0, int base = 0, int target = 1) {
    return {kind,static_cast<std::uint8_t>(base),slots,static_cast<std::uint8_t>(target)};
}
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
void hand(Player& p, std::initializer_list<CardId> cards) {
    p.hand.fill(0); std::copy(cards.begin(),cards.end(),p.hand.begin());
    p.hand_count = static_cast<std::uint8_t>(cards.size()); p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy);
    check(env.restore(snapshot), "restore explicit attribute-change fixture"); return env;
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "attribute snapshot exact roundtrip");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "illegal compressed attribute action preserves all state, RNG and fixture tapes");
}
int merge(int a, int b) {
    if (a == 5) return b;
    if (b == 5) return a;
    return a == b ? a : 0;
}
// Independent exact permutation semantics: each recurrence chooses the LAST
// selected component. The two setters overwrite; ordinary boosts only merge.
std::unordered_map<std::uint64_t,std::uint8_t> permutations;
std::uint8_t possible(std::uint64_t counts, int base) {
    if (!counts) return static_cast<std::uint8_t>(1u << base);
    const auto key = counts | (static_cast<std::uint64_t>(base) << 36);
    if (auto found = permutations.find(key); found != permutations.end()) return found->second;
    unsigned outcomes = 0;
    for (int symbol = 0; symbol < 9; ++symbol) {
        if (!(counts & (std::uint64_t{15} << (symbol*4)))) continue;
        const auto prefix = counts - (std::uint64_t{1} << (symbol*4));
        const auto before = possible(prefix,base);
        if (symbol >= 7) outcomes |= 1u << (symbol-6);
        else for (int attribute = 0; attribute < 7; ++attribute)
            if (before & (1u << attribute)) outcomes |= 1u << merge(attribute,symbol);
    }
    return permutations.emplace(key,static_cast<std::uint8_t>(outcomes)).first->second;
}
unsigned compressed(std::uint64_t counts) {
    unsigned outcomes = 0;
    for (int setter = 7; setter < 9; ++setter) {
        if (!(counts & (std::uint64_t{15} << (setter*4)))) continue;
        outcomes |= 1u << (setter-6);
        for (int boost = 0; boost < 7; ++boost)
            if (boost != 5 && boost != setter-6 && (counts & (std::uint64_t{15} << (boost*4)))) outcomes |= 1u;
    }
    return outcomes;
}
void enumerate_multisets(int first, int remaining, std::uint64_t counts, std::uint64_t& checked) {
    if (counts & ((std::uint64_t{15} << 28) | (std::uint64_t{15} << 32))) {
        const auto expected = compressed(counts);
        for (int base = 0; base < 7; ++base) {
            check(possible(counts,base) == expected, "three-result compression equals every actual permutation of setters and boosts");
            ++checked;
        }
    }
    if (!remaining) return;
    for (int next = first; next < 9; ++next)
        enumerate_multisets(next,remaining-1,counts+(std::uint64_t{1} << (4*next)),checked);
}
void metadata_and_compression_proof() {
    check(attribute_change_card_count == 2 && attribute_change_catalog().size() == 2, "strict two-setter catalog");
    const auto* fire = find_attribute_change_card(65); const auto* water = find_attribute_change_card(85);
    check(fire && fire->element == Element::fire && fire->card.attack == 2 && fire->card.boost == 0
        && water && water->element == Element::water && water->card.attack == 5 && water->card.boost == 0,
        "setters retain standalone ATK2/5 but add no damage as extra components");
    for (const auto& item : attribute_change_catalog())
        check(item.card.kind == Kind::weapon && item.card.defense == 0 && item.card.hp == 0 && item.card.mp == 0
            && item.card.price == 15 && item.card.weight == 1 && !item.card.discardable,
            "setter price, draw weight and independent roles pinned");
    for (CardId id : {CardId{0},CardId{58},CardId{84},CardId{195},CardId{196},CardId{65535}})
        check(!find_attribute_change_card(id), "attribute removal and unsupported roles not silently included");
    std::uint64_t checked = 0; enumerate_multisets(0,8,0,checked);
    check(checked == 125125, "complete bounded abstract multiset exploration includes duplicates and all seven base attributes");
    auto env = fixture({65,85}, {120});
    check(std::string_view(env.profile()) == attribute_change_ruleset && env.snapshot().version == 36,
        "new attribute profile and in-process schema identified");
    check(env.options().weapon_bases == 3 && env.options().attribute_slots == 3
        && env.options().boost_slots == 0 && !env.options().pray, "setter extra mask separate from pure attack boosts");
    for (int slot = 0; slot < 2; ++slot) {
        auto branch = env; const auto result = branch.step(attack(ActionKind::attack,0,slot));
        check(result.valid && !result.attribute_attack && !result.attribute_cards_used
            && branch.state().pending.amount == (slot ? 5 : 2) && !branch.state().pending.attribute_card,
            "setter standalone attack preserves original power without synthetic modifier event"); roundtrip(branch);
    }
    auto bad_cfg = config; bad_cfg.elements = bad_cfg.chance = bad_cfg.disease = bad_cfg.celestial = false;
    bad_cfg.offensive_miracles = bad_cfg.defensive_miracles = false;
    bool threw = false; try { Env invalid(bad_cfg); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "attribute change requires elements even without later feature profiles");
    Env minimal(Config{512,10,false,true,false,false,false,false,false,false,false,true});
    const CardId ids[] = {65,85}; check(minimal.script_draws(ids), "attribute setters can be enabled independently of miracles or light/dark");
    roundtrip(minimal);
    auto disabled_cfg = config; disabled_cfg.attribute_change = false; Env disabled(disabled_cfg); const auto original = disabled.snapshot();
    check(!disabled.script_draws(ids) && disabled.snapshot() == original, "disabled setter draws rejected atomically");
    auto bad = original; bad.state.players[bad.state.actor].hand[0] = 65;
    check(!disabled.restore(bad) && disabled.snapshot() == original, "disabled setter hand rejected");
    for (auto kind : {ActionKind::attack_set_fire,ActionKind::attack_set_water,ActionKind::attack_set_neutral}) reject(disabled,attack(kind));
}
void concrete_engine_compression() {
    constexpr std::array<CardId,7> bases{5,58,59,60,69,62,63};
    constexpr std::array<int,7> powers{1,1,1,1,2,1,1};
    constexpr std::array<CardId,9> parts{9,84,95,68,81,62,89,65,85};
    constexpr std::array<int,9> boosts{1,5,8,2,4,1,5,0,0};
    constexpr std::array<ActionKind,3> kinds{ActionKind::attack_set_neutral,ActionKind::attack_set_fire,ActionKind::attack_set_water};
    for (int base = 0; base < 7; ++base) for (int a = 0; a < 9; ++a) for (int b = 0; b < 9; ++b) for (int c = 0; c < 9; ++c) {
        if (a < 7 && b < 7 && c < 7) continue;
        auto env = fixture({bases[base],parts[a],parts[b],parts[c]}, {120});
        const std::array<int,3> symbols{a,b,c};
        std::uint64_t counts = 0; for (int symbol : symbols) counts += std::uint64_t{1} << (symbol*4);
        const auto actual = possible(counts,base);
        for (int result = 0; result < 3; ++result) {
            const auto action = attack(kinds[result],14);
            const bool valid = actual & (1u << result);
            check(env.legal(action) == valid, "compressed concrete action represents exactly a realizable full ordering");
            if (!valid) { reject(env,action); continue; }
            CardId canonical = 0; int setter_count = 0;
            for (int slot = 0; slot < 3; ++slot) if (symbols[slot] >= 7) {
                ++setter_count; const int color = symbols[slot]-6;
                bool witness = result == color;
                if (!result) for (int other : symbols) if (other < 7 && other != 5 && other != color) witness = true;
                if (!canonical && witness) canonical = parts[symbols[slot]];
            }
            auto branch = env; const auto event = branch.step(action);
            check(event.valid && event.attribute_attack && event.attribute_cards_used == setter_count
                && event.attribute_element == static_cast<Element>(result)
                && branch.state().pending.element == static_cast<Element>(result)
                && branch.state().pending.attribute_card == canonical
                && branch.state().pending.amount == powers[base]+boosts[a]+boosts[b]+boosts[c]
                && branch.state().players[0].hand_count == 0 && branch.state().players[0].pending_deals == 4,
                "canonical provenance, zero setter extra power and all selected consumption preserved");
            roundtrip(branch);
        }
    }
}
void invalid_witnesses_and_full_capacity() {
    for (auto cards : {std::initializer_list<CardId>{85,65,84},std::initializer_list<CardId>{5,65,62},
                       std::initializer_list<CardId>{5,65,85}}) {
        auto env = fixture(cards,{120}); reject(env,attack(ActionKind::attack_set_neutral,6));
        reject(env,attack(ActionKind::attack,6));
    }
    auto no_extra = fixture({65,95}, {120});
    reject(no_extra,attack(ActionKind::attack_set_fire));
    check(no_extra.step(attack(ActionKind::attack,2)).valid && no_extra.state().pending.element == Element::neutral,
        "setter base is not a later extra witness and still uses ordinary composition for ordinary boosts");
    for (CardId id : {CardId{100},CardId{198}}) {
        auto env = fixture({id,65}, {120}); reject(env,attack(ActionKind::attack_set_fire,2));
    }
    auto malformed = fixture({5,65,9}, {120});
    reject(malformed,attack(ActionKind::attack_set_fire,3));
    reject(malformed,attack(ActionKind::attack_set_fire,0x8002));
    reject(malformed,attack(ActionKind::attack_set_water,6));
    reject(malformed,attack(ActionKind::attack_set_fire,2,255));
    reject(malformed,attack(ActionKind::attack_set_fire,2,0,2));
    auto canonical = fixture({5,85,65,9}, {120}); canonical.step(attack(ActionKind::attack_set_neutral,14));
    check(canonical.state().pending.attribute_card == 85, "neutral chooses first slot among multiple valid setter witnesses");
    auto full = fixture({5,65,65,65,65,65,65,65,85,85,85,85,85,85,85,9}, {120});
    std::array<CardId,16> draws{}; draws.fill(120); check(full.script_draws(draws), "script all full-capacity replacements");
    const auto result = full.step(attack(ActionKind::attack_set_neutral,0xfffe));
    check(result.attribute_cards_used == 14 && full.state().pending.amount == 2 && full.state().pending.attribute_card == 65
        && full.state().players[0].pending_deals == 16, "15 extras including duplicate setter individuals do not add setter ATK");
    roundtrip(full); full.step(defend());
    check(full.snapshot().draw_index == 16 && full.state().players[0].hand_count == 16,
        "all sixteen consumed slots earn exactly sixteen replacements"); roundtrip(full);
}
void payloads_reactions_and_restore() {
    for (CardId base : {CardId{43},CardId{61},CardId{70}}) {
        auto env = fixture({base,65}, {120}); env.step(attack(ActionKind::attack_set_fire,2)); roundtrip(env);
        check(env.state().pending.attribute_card == 65 && env.state().pending.element == Element::fire,
            "setter marker makes changed attribute valid for original status payload");
        const auto before = env.snapshot(); env.step(defend());
        check(base != 43 || env.state().players[1].dark_cloud, "DarkCloud source payload retained after override from neutral to fire");
        check(base != 61 || env.state().players[1].disease == Disease::cold, "cold source payload retained after stone to fire override");
        check(base != 70 || env.state().players[1].glory, "Glory source payload retained after light to fire override");
        roundtrip(env);
        Env restored(config); check(restored.restore(before), "restore changed status attack before response");
        auto bad = before; bad.state.pending.attribute_card = 195;
        check(!restored.restore(bad) && restored.snapshot() == before, "unsupported marker rejected atomically");
        bad = before; bad.state.pending.element = Element::water;
        check(!restored.restore(bad), "marker permits only its own color or neutral");
        bad = before; bad.state.pending.attribute_card = 0;
        if (base != 70) check(!restored.restore(bad), "changed non-light payload needs override provenance");
        bad = before; bad.state.pending.from_ability = true;
        check(!restored.restore(bad), "weapon attribute marker cannot claim retained magic origin");
    }
    auto returned = fixture({43,65,9}, {39}); returned.step(attack(ActionKind::attack_set_neutral,6));
    check(returned.step({ActionKind::defend,0,0,no_slot}).reflected, "neutralized modified weapon admits ordinary physical reflection");
    check(returned.state().pending.attribute_card == 65 && returned.state().pending.harm == Harm::dark_cloud,
        "reflection preserves override marker and original on-damage payload"); roundtrip(returned);
    returned.step(defend()); check(returned.state().players[0].dark_cloud && returned.state().players[0].hp == 28,
        "returned modified attack applies original power and payload to new target"); roundtrip(returned);
    auto dark = fixture({63,65}, {120}); dark.step(attack(ActionKind::attack_set_fire,2));
    check(!dark.step(defend()).dark_execution && dark.state().players[1].hp == 39,
        "dark base loses execution when final override attribute is fire");
    auto fire = fixture({5,65}, {215}); fire.step(attack(ActionKind::attack_set_fire,2));
    reject(fire,{ActionKind::defend,0,0,no_slot});
    auto neutral = fixture({5,65,9}, {215}); neutral.step(attack(ActionKind::attack_set_neutral,6));
    check(neutral.step({ActionKind::defend,0,0,no_slot}).weapon_blocked, "neutral override result remains blockable as weapon"); roundtrip(neutral);
}
void observations_privacy_and_replay() {
    auto env = fixture({5,85,65,9,62}, {120}); const auto obs = env.observe();
    check(obs[356] == 1.f && obs[357] == 0.f && obs[358] == 2/6.f && obs[359] == 1/6.f,
        "setter hand suffix separate from ordinary attribute features");
    unsigned sampled = 0;
    for (std::uint64_t seed = 0; seed < 2000; ++seed) {
        auto rng = seed; const auto action = env.sample_action(rng); check(env.legal(action), "attribute-aware main sampler legal");
        if (action.kind >= ActionKind::attack_set_fire && action.kind <= ActionKind::attack_set_neutral)
            sampled |= 1u << (static_cast<int>(action.kind)-7);
    }
    check(sampled == 7, "sampler reaches all three compressed outcomes");
    env.step(attack(ActionKind::attack_set_neutral,14)); const auto pending = env.observe();
    check(pending[373] == 85/363.f && pending[374] == 1.f, "pending canonical override provenance publicly represented");
    // Retain the same hand count in the public information while swapping ID.
    auto original = env.snapshot(); original.state.players[0].hand[0] = 85;
    original.rng_state ^= 0x9e3779b97f4a7c15ULL;
    Env other(config); check(other.restore(original) && other.observe() == env.observe() && other.options() == env.options(),
        "opponent unplayed hand setter identity remains private");
    auto disabled_cfg = config; disabled_cfg.attribute_change = false; Env disabled(disabled_cfg); const auto old = disabled.observe();
    check(std::all_of(old.begin()+356,old.end(),[](float x){return x==0.f;}), "old modes new attribute suffix zero");
    auto bad = env.snapshot(); bad.config.attribute_change = false;
    const auto disabled_before = disabled.snapshot();
    check(!disabled.restore(bad) && disabled.snapshot() == disabled_before, "disabled override marker rejected");
    std::uint64_t changed = 0, setters = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+45000, pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "attribute-change game trajectory replay");
            for (float x : a.observe()) check(std::isfinite(x), "attribute observations finite");
            const auto action = a.sample_action(pa), copy = b.sample_action(pb);
            check(action == copy && pa == pb && a.legal(action), "attribute policy deterministic and legal");
            const auto ra = a.step(action), rb = b.step(copy);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.attribute_attack == rb.attribute_attack
                && ra.attribute_cards_used == rb.attribute_cards_used && ra.attribute_element == rb.attribute_element,
                "attribute event metadata replay");
            changed += ra.attribute_attack; setters += ra.attribute_cards_used; roundtrip(a);
        }
    }
    check(changed && setters, "seeded games exercise newly drawn setters and compressed actions");
}
}
int main() {
    metadata_and_compression_proof(); concrete_engine_compression(); invalid_witnesses_and_full_capacity();
    payloads_reactions_and_restore(); observations_privacy_and_replay();
    std::cout << "Attribute change checks passed (125125 exhaustive compression cases, canonical markers, 1000 replay rollouts).\n";
}
