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
void check(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL elements: " << message << '\n'; std::exit(1); }
}
constexpr std::uint16_t bit(int slot) { return static_cast<std::uint16_t>(1u << slot); }
Action attack(int base, std::uint16_t extras = 0, int target = 1) {
    return {ActionKind::attack, static_cast<std::uint8_t>(base), extras, static_cast<std::uint8_t>(target)};
}
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend, no_slot, slots, no_slot}; }
void hand(Player& p, std::initializer_list<CardId> cards) {
    p.hand.fill(0);
    std::copy(cards.begin(), cards.end(), p.hand.begin());
    p.hand_count = static_cast<std::uint8_t>(cards.size());
    p.pending_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy,
            Config config = Config{512, 10, false, true}) {
    Env env(config);
    auto snap = env.snapshot();
    snap.state = State{};
    snap.scripted_draws.fill(0);
    snap.draw_count = snap.draw_index = 0;
    hand(snap.state.players[0], own);
    hand(snap.state.players[1], enemy);
    check(env.restore(snap), "restore valid element fixture");
    return env;
}
void reject(Env& env, Action action, const char* message) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid, message);
    check(env.snapshot() == before, "illegal action preserves all state and RNG");
}
void metadata_and_profile_boundaries() {
    check(element_catalog().size() == 54 && element_card_count == 54, "pinned four-element allowlist size");
    int weapons = 0, armor = 0, weight = 0;
    for (const auto& record : element_catalog()) {
        check(find_element_card(record.card.id) == &record, "element ID lookup");
        check(!find_card(record.card.id) && !find_miracle(record.card.id), "separate catalogs have disjoint IDs");
        check(record.element >= Element::fire && record.element <= Element::stone, "only four supported colored elements");
        check(record.card.weight > 0, "positive element draw weights");
        weapons += record.card.kind == Kind::weapon;
        armor += record.card.kind == Kind::armor;
        weight += record.card.weight;
        check(record.card.discardable != (record.card.kind == Kind::weapon), "weapon discard restriction retained");
    }
    check(weapons == 23 && armor == 31 && weight == 54, "pinned element roles and weights");
    for (CardId id : {CardId{0}, CardId{5}, CardId{61}, CardId{62}, CardId{63}, CardId{65},
                      CardId{85}, CardId{100}, CardId{175}, CardId{184}, CardId{195}, CardId{65535}})
        check(!find_element_card(id), "status, light/dark, override, random and unknown rows excluded");
    check(find_element_card(78)->card.attack == 4 && find_element_card(78)->card.defense == 4,
        "elemental weapon retains independent defense role");

    Env neutral;
    Env miracles_only(Config{512, 10, true, false});
    auto elements_only = fixture({58}, {123});
    auto combined = fixture({58, 216}, {123}, Config{512, 10, true, true});
    check(std::string_view(elements_only.profile()) == elements_ruleset, "elements-only profile identity");
    check(std::string_view(combined.profile()) == elements_miracles_ruleset, "combined profile identity");
    const CardId element_draw[] = {58};
    for (auto* disabled : {&neutral, &miracles_only}) {
        const auto original = disabled->snapshot();
        check(!disabled->script_draws(element_draw) && disabled->snapshot() == original,
            "elements-disabled scripted draw rejected atomically");
        auto invalid = original;
        invalid.state.players[invalid.state.actor].hand[0] = 58;
        check(!disabled->restore(invalid) && disabled->snapshot() == original,
            "same-config colored hand rejected when elements disabled");
        invalid = original;
        const auto source = invalid.state.turn_owner;
        invalid.state.actor = static_cast<std::uint8_t>(1 - source);
        invalid.state.phase = Phase::response;
        invalid.state.players[source].pending_deals = 1;
        invalid.state.pending = Pending{Effect::damage, source, invalid.state.actor, 1, 0, false, Element::fire};
        check(!disabled->restore(invalid) && disabled->snapshot() == original,
            "same-config colored pending rejected when elements disabled");
    }
    const auto before = elements_only.snapshot();
    const CardId miracle_draw[] = {216};
    check(!elements_only.script_draws(miracle_draw) && elements_only.snapshot() == before,
        "elements flag does not implicitly enable miracles");
    auto invalid = before; invalid.state.players[0].abilities[0] = 1;
    check(!elements_only.restore(invalid) && elements_only.snapshot() == before,
        "elements-only snapshot rejects retained miracle");
    invalid = before; invalid.version = 2;
    check(!elements_only.restore(invalid) && elements_only.snapshot() == before,
        "previous snapshot schema rejected without migration");
    check(before.version == 36, "shared counter extensions snapshot schema version");
    check(!elements_only.restore(combined.snapshot()), "combined profile snapshot cannot cross into elements-only profile");
}
void all_defense_pairs() {
    constexpr std::array<CardId, 5> attackers{5, 58, 59, 60, 69};
    constexpr std::array<CardId, 5> defenders{120, 122, 123, 124, 125};
    constexpr std::array<Element, 5> colors{Element::neutral, Element::fire, Element::water, Element::wood, Element::stone};
    for (int a = 0; a < 5; ++a) for (int d = 0; d < 5; ++d) {
        auto env = fixture({attackers[a]}, {defenders[d]});
        check(env.step(attack(0)).valid && env.state().pending.element == colors[a],
            "base attack preserves source element");
        const bool allowed = a == 0 || (a == 1 && d == 2) || (a == 2 && d == 1)
            || (a == 3 && d == 4) || (a == 4 && d == 3);
        check(env.options().defense_slots == (allowed ? 1 : 0), "all 5x5 defense option pairs match expected relation");
        check(env.legal(defend()), "receiving without armor is always legal");
        if (allowed) {
            check(env.step(defend(1)).valid, "compatible elemental defense accepted");
            check(env.state().players[1].hp == (a == 4 ? 39 : 40), "defense power applied after compatibility check");
        } else {
            reject(env, defend(1), "incompatible defense rejects before consumption");
            check(env.step(defend()).valid && env.state().players[1].hp == (a == 4 ? 38 : 39),
                "accepting incompatible incoming damage applies full power");
        }
    }
}
void attack_composition() {
    struct Case {
        CardId base, first, second;
        Element expected;
        int amount;
    };
    constexpr std::array<Case, 8> cases{{
        {58, 84, 94, Element::fire, 14},
        {58, 95, 94, Element::neutral, 17},
        {58, 94, 95, Element::neutral, 17},
        {58, 9, 0, Element::neutral, 2},
        {5, 84, 0, Element::neutral, 6},
        {69, 68, 0, Element::neutral, 4},
        {60, 68, 74, Element::wood, 6},
        {58, 133, 0, Element::neutral, 6}
    }};
    for (const auto& c : cases) {
        auto env = c.second ? fixture({c.base, c.first, c.second}, {123})
                            : fixture({c.base, c.first}, {123});
        const auto selected = static_cast<std::uint16_t>(c.second ? bit(1) | bit(2) : bit(1));
        check((env.options().boost_slots & selected) == selected, "mixed-element boosts stay available");
        check(env.step(attack(0, selected)).valid, "mixed-element composition legal");
        check(env.state().pending.element == c.expected && env.state().pending.amount == c.amount,
            "same element stays colored; any mismatch including NE becomes sticky NE");
        Env resumed(Config{512, 10, false, true});
        check(resumed.restore(env.snapshot()), "composed colored/neutral response snapshot restorable");
    }
    auto boosted_base = fixture({84}, {123});
    check(boosted_base.step(attack(0)).valid && boosted_base.state().pending.element == Element::fire
        && boosted_base.state().pending.amount == 5, "elemental boost weapon also works as a base");
    auto ordinary = fixture({58, 59}, {123});
    reject(ordinary, attack(0, bit(1)), "two ordinary colored base weapons cannot combine");
    auto self = fixture({58}, {123});
    check(self.step(attack(0, 0, 0)).valid && self.state().players[0].hp == 39
        && self.state().phase == Phase::main && self.state().pending.element == Element::neutral,
        "colored self-attack resolves directly and clears pending element");
}
void defense_composition_and_dual_role() {
    auto neutral = fixture({5}, {120, 122, 123, 124, 125});
    const std::array<CardId, 6> draws{120, 120, 120, 120, 120, 120};
    check(neutral.script_draws(draws), "script multi-defense replacement");
    neutral.step(attack(0));
    check(neutral.options().defense_slots == 31 && neutral.step(defend(31)).valid,
        "NE attack permits mixed-element armor subset");
    check(neutral.state().players[1].hp == 40 && neutral.snapshot().draw_index == 6,
        "all selected mixed armor consumed, including overdefense");

    auto fire = fixture({58}, {123, 186, 129});
    fire.step(attack(0));
    check(fire.options().defense_slots == (bit(0) | bit(2)), "fire allows only water armor candidates");
    reject(fire, defend(bit(0) | bit(1)), "water plus NE armor cannot defend fire, even with excess raw defense");
    check(fire.step(defend(bit(0) | bit(2))).valid && fire.state().players[1].hp == 40,
        "multiple same-compatible-element armor combine");

    auto dual = fixture({59}, {78});
    dual.step(attack(0));
    check(dual.options().defense_slots == 1 && dual.step(defend(1)).valid
        && dual.state().players[1].hp == 40, "fire dual-role weapon defends water with its defense value");
    auto forbidden = fixture({78}, {123});
    reject(forbidden, {ActionKind::discard, no_slot, 1, no_slot}, "elemental dual-role weapon remains non-discardable");
}
void full_width_and_restore_validation() {
    auto env = fixture({}, {123});
    auto snapshot = env.snapshot();
    snapshot.state.players[0].hand.fill(84);
    snapshot.state.players[0].hand_count = 16;
    check(env.restore(snapshot), "restore sixteen elemental boost weapons");
    check(env.step(attack(15, 0x7fff)).valid && env.state().pending.element == Element::fire
        && env.state().pending.amount == 80, "slot15 base and all other colored extras retain element and power");
    Env resumed(Config{512, 10, false, true});
    check(resumed.restore(env.snapshot()), "full-width elemental response restorable");
    const auto original = env.snapshot();
    auto invalid = original; invalid.state.pending.element = static_cast<Element>(255);
    check(!env.restore(invalid) && env.snapshot() == original, "unknown element rejected atomically");
    invalid = original; invalid.state.players[1].hand[0] = 63;
    check(!env.restore(invalid) && env.snapshot() == original, "excluded dark artifact rejected in element profile");
    const CardId excluded[] = {65};
    check(!env.script_draws(excluded) && env.snapshot() == original, "order-dependent attribute override excluded");

    auto heal = fixture({227}, {123});
    check(heal.step({ActionKind::use, 0, 0, 1}).valid, "utility response fixture");
    const auto heal_before = heal.snapshot();
    invalid = heal_before; invalid.state.pending.element = Element::fire;
    check(!heal.restore(invalid) && heal.snapshot() == heal_before, "utility effects cannot carry attack element");
}
void observations_and_privacy() {
    auto env = fixture({58, 59, 60, 69, 120}, {123});
    const auto own = env.observe();
    check(own[185] == 1.f && own[169] == 1 / 4.f && own[170] == 2 / 4.f
        && own[171] == 3 / 4.f && own[172] == 4 / 4.f && own[173] == 0,
        "own hand element features in documented extension");
    check(own[164] == 1.f && std::count(own.begin() + 164, own.begin() + 169, 1.f) == 1,
        "enabled main phase has one neutral incoming-element tag");
    auto hidden = env.snapshot();
    hand(hidden.state.players[1], {122}); hidden.rng_state ^= 0x9e3779b97f4a7c15ULL;
    Env other(Config{512, 10, false, true}); check(other.restore(hidden), "restore hidden enemy-element variant");
    check(other.observe() == own && other.options() == env.options(), "opponent hand element not leaked");
    env.step(attack(0));
    const auto response = env.observe();
    check(response[165] == 1.f && response[164] == 0.f && response[169] == 2 / 4.f,
        "response shows incoming fire and acting defender's own water");
    check(response[16] == 123 / 363.f, "element extension does not overwrite first hand prefix");

    Env plain;
    Env miracle(Config{512, 10, true, false});
    for (const auto* disabled : {&plain, &miracle}) {
        const auto features = disabled->observe();
        check(std::all_of(features.begin() + 164, features.end(), [](float x) { return x == 0.f; }),
            "elements-disabled extension is all zero");
    }
}
void combined_profile_and_replay() {
    auto combined = fixture({58, 216}, {123}, Config{512, 10, true, true});
    check(combined.step({ActionKind::use, 1, 0, 1}).valid && combined.state().pending.miracle == 216
        && combined.state().pending.element == Element::neutral, "neutral miracle coexists with colored hand");
    check(combined.options().defense_slots == 0, "colored armor does not defend neutral utility miracle");
    Env resumed(Config{512, 10, true, true});
    check(resumed.restore(combined.snapshot()), "combined miracle response state restorable");
    check(combined.step(defend()).valid && combined.state().players[1].hp == 50,
        "combined profile neutral miracle resolves normally");

    for (bool enable_miracles : {false, true}) {
        std::uint64_t colored_actions = 0;
        for (std::uint64_t seed = 0; seed < 500; ++seed) {
            const Config config{256, 10, enable_miracles, true};
            Env a(config), b(config); a.reset(seed); b.reset(seed);
            std::uint64_t pa = seed + 4000, pb = pa;
            while (!a.done()) {
                check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "element trajectory reproducible");
                for (float x : a.observe()) check(std::isfinite(x), "element observation finite");
                const auto action = a.sample_action(pa), action_b = b.sample_action(pb);
                check(action == action_b && pa == pb && a.legal(action), "element sampled action legal and reproducible");
                const auto ra = a.step(action), rb = b.step(action_b);
                check(ra.valid && rb.valid && ra.rewards == rb.rewards, "element step results reproducible");
                colored_actions += a.state().pending.element != Element::neutral;
                Env restored(config);
                check(restored.restore(a.snapshot()) && restored.snapshot() == a.snapshot(), "every element-profile state restorable");
            }
        }
        check(colored_actions > 0, "rollouts exercise pending colored attacks");
    }
}
}
int main() {
    metadata_and_profile_boundaries(); all_defense_pairs(); attack_composition();
    defense_composition_and_dual_role(); full_width_and_restore_validation();
    observations_and_privacy(); combined_profile_and_replay();
    std::cout << "Element checks passed (5x5 compatibility, composition, profiles/privacy, 1000 replay rollouts).\n";
}
