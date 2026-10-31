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
    if (!value) { std::cerr << "FAIL reflection: " << message << '\n'; std::exit(1); }
}
constexpr std::uint16_t bit(int slot) { return static_cast<std::uint16_t>(1u << slot); }
Action attack(int base = 0, std::uint16_t extras = 0, int target = 1) {
    return {ActionKind::attack, static_cast<std::uint8_t>(base), extras, static_cast<std::uint8_t>(target)};
}
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend, no_slot, slots, no_slot}; }
Action reaction(int base, std::uint16_t prefix = 0) {
    return {ActionKind::defend, static_cast<std::uint8_t>(base), prefix, no_slot};
}
void hand(Player& p, std::initializer_list<CardId> cards) {
    p.hand.fill(0);
    std::copy(cards.begin(), cards.end(), p.hand.begin());
    p.hand_count = static_cast<std::uint8_t>(cards.size());
    p.pending_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy,
            Config config = Config{512, 10, false, false, false, true}) {
    Env env(config);
    auto snapshot = env.snapshot();
    snapshot.state = State{};
    snapshot.scripted_draws.fill(0); snapshot.draw_count = snapshot.draw_index = 0;
    snapshot.scripted_hits.fill(0); snapshot.hit_count = snapshot.hit_index = 0;
    snapshot.scripted_targets.fill(0); snapshot.target_count = snapshot.target_index = 0;
    hand(snapshot.state.players[0], own); hand(snapshot.state.players[1], enemy);
    check(env.restore(snapshot), "restore valid reaction fixture");
    return env;
}
void draw_script(Env& env, std::initializer_list<CardId> draws) {
    check(env.script_draws({draws.begin(), draws.size()}), "install supported draw outcomes");
}
void target_script(Env& env, std::initializer_list<std::uint8_t> targets) {
    check(env.script_bounce_targets({targets.begin(), targets.size()}), "install bounce target outcomes");
}
void reject(Env& env, Action action, const char* message) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid, message);
    check(env.snapshot() == before, "illegal reaction leaves resources, tapes and RNG unchanged");
}
void metadata_and_profile_boundaries() {
    check(reaction_card_count == 2 && reaction_catalog().size() == 2, "exact two-card reaction allowlist");
    check(find_reaction_card(20)->reaction == WeaponReaction::bounce
        && find_reaction_card(20)->card.attack == 5 && find_reaction_card(20)->card.weight == 6,
        "pinned bounce weapon metadata");
    check(find_reaction_card(39)->reaction == WeaponReaction::reflect
        && find_reaction_card(39)->card.attack == 10 && find_reaction_card(39)->card.weight == 3,
        "pinned reflect weapon metadata");
    for (CardId id : {CardId{0}, CardId{21}, CardId{27}, CardId{40}, CardId{54}, CardId{195}, CardId{196}, CardId{65535}})
        check(!find_reaction_card(id), "magic reactions, blockers, attribute removers and unknown IDs excluded");
    for (const auto& record : reaction_catalog())
        check(record.card.kind == Kind::weapon && !record.card.discardable && record.card.boost == 0,
            "reaction weapons retain weapon restrictions");
    auto enabled = fixture({39}, {20});
    check(std::string_view(enabled.profile()) == reflection_ruleset && enabled.snapshot().version == 35,
        "reaction profile and snapshot schema identified");

    for (const Config config : {Config{}, Config{512, 10, true}, Config{512, 10, true, true},
                                Config{512, 10, true, true, true}}) {
        Env disabled(config);
        const auto original = disabled.snapshot();
        const CardId special_draw[] = {39};
        const std::uint8_t bounce_target[] = {0};
        check(!disabled.script_draws(special_draw) && disabled.snapshot() == original,
            "disabled special draw rejected atomically");
        check(!disabled.script_bounce_targets(bounce_target) && disabled.snapshot() == original,
            "disabled bounce tape rejected atomically");
        check(disabled.script_bounce_targets({}), "empty target prefix clear allowed when disabled");
        auto invalid = original; invalid.state.players[invalid.state.actor].hand[0] = 20;
        check(!disabled.restore(invalid) && disabled.snapshot() == original,
            "disabled profile rejects special weapon in same-config snapshot");
        invalid = original; invalid.target_count = 1; invalid.scripted_targets[0] = 0;
        check(!disabled.restore(invalid) && disabled.snapshot() == original,
            "disabled profile rejects even all-zero nonempty target tape");
        invalid = original; invalid.state.last_reaction = State::LastReaction{39, 0, 1, WeaponReaction::reflect};
        check(!disabled.restore(invalid) && disabled.snapshot() == original, "disabled public reaction event rejected");
    }
    const auto original = enabled.snapshot();
    auto invalid = original; invalid.version = 4;
    check(!enabled.restore(invalid) && enabled.snapshot() == original, "old snapshot schema rejected without migration");
}
void reflect_and_consumption() {
    auto env = fixture({36}, {39, 120});
    draw_script(env, {5, 120});
    check(env.step(attack()).valid, "open NE attack for reflection");
    check(env.options().reaction_slots == bit(0) && env.options().defense_slots == bit(1),
        "ordinary defense and reaction candidate roles separated");
    const auto result = env.step(reaction(0));
    check(result.valid && result.reflected && !result.bounced && !result.bounce_self,
        "reflect result flags");
    check(env.state().actor == 0 && env.state().turn_owner == 0
        && env.state().pending.source == 1 && env.state().pending.target == 0,
        "reaction changes pending source/target while preserving normal turn owner");
    check(env.state().pending.amount == 10 && env.state().reaction_count == 1,
        "reflected amount preserved and active depth counted");
    check(env.state().players[0].pending_deals == 1 && env.state().players[1].pending_deals == 1
        && env.state().players[1].hand_count == 1 && env.state().players[1].hand[0] == 120,
        "both seats retain deferred acquisition budgets");
    check(env.snapshot().draw_index == 0 && env.state().turns == 0, "no acquisition or normal turn advancement during chain");
    Env resumed(Config{512, 10, false, false, false, true});
    check(resumed.restore(env.snapshot()), "reflected pending attack snapshot restorable");
    check(env.step(defend()).valid && resumed.step(defend()).valid && env.snapshot() == resumed.snapshot(),
        "reflected attack continuation reproducible after restore");
    check(env.state().players[0].hp == 30 && env.state().players[1].hp == 40,
        "original attacker receives returned damage");
    check(env.state().players[0].hand[0] == 5 && env.state().players[1].hand[1] == 120
        && env.snapshot().draw_index == 2, "both gifts distributed in fixed-seat order after chain");
    check(env.state().actor == 1 && env.state().turn_owner == 1 && env.state().reaction_count == 0,
        "normal order resumes and active chain count clears");
    check(env.state().last_reaction == State::LastReaction{39, 1, 0, WeaponReaction::reflect},
        "public last event persists after resolution");

    auto prefix = fixture({36}, {186, 39, 120});
    draw_script(prefix, {5, 120, 132});
    prefix.step(attack());
    check(prefix.step(reaction(1, bit(0))).valid && prefix.state().pending.amount == 10,
        "ordinary armor before reflect does not reduce redirected original power");
    check(prefix.state().players[1].hand_count == 1 && prefix.state().players[1].hand[0] == 120
        && prefix.state().players[1].pending_deals == 2 && prefix.snapshot().draw_index == 0,
        "ordinary prefix and special consumed, later unrelated hand artifact retained");
    prefix.step(defend());
    check(prefix.state().players[0].hp == 30 && prefix.state().players[1].hand_count == 3
        && prefix.snapshot().draw_index == 3, "wasted full-block armor still earns its replacement");
}
void bounce_target_branches() {
    for (std::uint8_t target : {std::uint8_t{0}, std::uint8_t{1}}) {
        auto env = fixture({36}, {20});
        draw_script(env, {5, 120}); target_script(env, {target});
        const auto before_rng = env.snapshot().rng_state;
        env.step(attack());
        const auto result = env.step(reaction(0));
        check(result.valid && result.bounced && !result.reflected && result.bounce_self == (target == 1),
            "bounce target result flags");
        check(env.snapshot().target_index == 1 && env.snapshot().rng_state == before_rng,
            "scripted bounce consumes one outcome without consuming seeded RNG");
        check(env.state().last_reaction == State::LastReaction{20, 1, target, WeaponReaction::bounce},
            "bounce last event reports actual target including self");
        if (target == 1) {
            check(env.state().phase == Phase::main && env.state().players[1].hp == 30
                && env.state().players[0].hp == 40, "self-selected bounce applies immediately without another response");
            check(env.state().reaction_count == 0 && env.snapshot().draw_index == 2,
                "self bounce ends chain and distributes both budgets");
        } else {
            check(env.state().phase == Phase::response && env.state().actor == 0
                && env.state().pending.source == 1 && env.state().pending.target == 0,
                "other-selected bounce awaits its new defender");
            check(env.snapshot().draw_index == 0 && env.state().reaction_count == 1,
                "nonself bounce keeps gifts deferred");
            check(env.step(defend()).valid && env.state().players[0].hp == 30,
                "other-selected bounce damages original attacker after acceptance");
        }
    }
}
void legality_and_nonreaction_use() {
    auto env = fixture({36}, {39, 20, 120});
    target_script(env, {1}); env.step(attack());
    reject(env, reaction(0, bit(0)), "special cannot also be an ordinary prefix slot");
    reject(env, reaction(0, bit(1)), "second special cannot be treated as ordinary prefix");
    reject(env, reaction(2), "ordinary armor cannot be designated as special base");
    reject(env, reaction(16), "special base slot beyond capacity rejected");
    reject(env, defend(bit(0)), "special cannot silently appear in plain defense mask");
    auto malformed = reaction(0); malformed.target = 0;
    reject(env, malformed, "reaction target cannot be supplied by policy");
    reject(env, attack(), "normal attack forbidden during reaction phase");

    for (CardId special : {CardId{20}, CardId{39}}) {
        auto normal = fixture({special, 9}, {120});
        target_script(normal, {0});
        check(normal.step(attack(0, bit(1))).valid && normal.state().pending.amount == (special == 20 ? 6 : 11),
            "special weapon can be ordinary base with legal boost");
        check(normal.state().reaction_count == 0 && normal.state().last_reaction == State::LastReaction{}
            && normal.snapshot().target_index == 0, "offensive use does not trigger defensive reaction or target RNG");
    }
    auto discard = fixture({39}, {120});
    reject(discard, {ActionKind::discard, no_slot, 1, no_slot}, "special weapon non-discardable");
    reject(discard, {ActionKind::pray}, "special weapon still prevents prayer");

    for (CardId base : {CardId{58}, CardId{59}, CardId{60}, CardId{69}}) {
        auto colored = fixture({base}, {39, 20}, Config{512, 10, false, true, false, true});
        colored.step(attack());
        check(colored.options().reaction_slots == 0, "NE reaction cannot redirect colored attack");
        reject(colored, reaction(0), "NE reflect rejects colored attack");
        reject(colored, reaction(1), "NE bounce rejects colored attack");
    }
    for (CardId item : {CardId{227}, CardId{216}, CardId{220}}) {
        auto utility = fixture({item}, {39, 20}, Config{512, 10, true, false, false, true});
        check(utility.step({ActionKind::use, 0, 0, 1}).valid, "utility response fixture");
        check(utility.options().reaction_slots == 0, "NE reactions do not target utility effects");
        reject(utility, reaction(0), "reflect rejects recovery and money");
    }
}
void long_chain_and_horizon() {
    auto full_width = fixture({5}, {});
    auto wide = full_width.snapshot();
    wide.state.players[1].hand.fill(120); wide.state.players[1].hand[15] = 39;
    wide.state.players[1].hand_count = 16;
    check(full_width.restore(wide), "restore slot15 special with fifteen ordinary armor pieces");
    std::array<CardId, 17> wide_draws; wide_draws.fill(120);
    check(full_width.script_draws(wide_draws), "script full-width reaction gift budget");
    full_width.step(attack());
    check(full_width.options().reaction_slots == bit(15) && full_width.step(reaction(15, 0x7fff)).valid
        && full_width.state().players[1].pending_deals == 16 && full_width.state().pending.amount == 1,
        "slot15 special plus all preceding armor consumes sixteen slots while keeping original attack power");
    check(full_width.step(defend()).valid && full_width.snapshot().draw_index == 17,
        "full-width reaction distributes every consumed artifact once");

    auto env = fixture({}, {});
    auto snapshot = env.snapshot();
    snapshot.state.players[0].hand.fill(39); snapshot.state.players[0].hand[0] = 5;
    snapshot.state.players[1].hand.fill(39);
    snapshot.state.players[0].hand_count = snapshot.state.players[1].hand_count = 16;
    check(env.restore(snapshot), "restore maximum inventory chain fixture");
    std::array<CardId, 32> draws; draws.fill(120);
    check(env.script_draws(draws), "script final chain gifts");
    env.step(attack());
    for (std::uint32_t depth = 1; depth <= 31; ++depth) {
        check(env.step(reaction(0)).valid && env.state().reaction_count == depth,
            "inventory permits all thirty-one redirects without invented chain cap");
        check(env.state().pending.amount == 1 && env.state().turn_owner == 0 && env.state().turns == 0
            && env.snapshot().draw_index == 0, "long chain preserves power, original owner and deferred gifts");
        Env resumed(Config{512, 10, false, false, false, true});
        check(resumed.restore(env.snapshot()), "every long-chain intermediate state restorable");
    }
    check(env.options().reaction_slots == 0 && env.state().players[0].pending_deals == 16
        && env.state().players[1].pending_deals == 16, "chain ends by exhausted actual inventory");
    check(env.step(defend()).valid && env.state().players[0].hp == 39
        && env.snapshot().draw_index == 32 && env.state().reaction_count == 0,
        "final response ends chain and deals all thirty-two gifts");

    auto limited = fixture({5, 39}, {39}, Config{3, 10, false, false, false, true});
    limited.step(attack()); limited.step(reaction(0));
    const auto result = limited.step(reaction(0));
    check(result.truncated && !result.terminated && limited.state().phase == Phase::response
        && limited.state().reaction_count == 2 && limited.state().turns == 0,
        "decision limit truncates unresolved chain without ending the game or dealing gifts");
    Env resumed(Config{3, 10, false, false, false, true});
    check(resumed.restore(limited.snapshot()), "truncated chain restorable");
}
void terminal_outcomes() {
    auto reflected = fixture({56}, {39}, Config{3, 10, false, false, false, true});
    auto snapshot = reflected.snapshot(); snapshot.state.players[0].hp = 1;
    check(reflected.restore(snapshot), "restore lethal reflection fixture");
    draw_script(reflected, {120});
    reflected.step(attack()); reflected.step(reaction(0));
    const auto result = reflected.step(defend());
    check(result.terminated && !result.truncated && reflected.state().winner == 1
        && result.rewards == std::array<float, 2>{-1.f, 1.f},
        "original turn owner may die from reflection with fixed-seat rewards");
    check(reflected.state().reaction_count == 0 && reflected.snapshot().draw_index == 1
        && reflected.state().players[1].hand[0] == 120, "only survivor receives pending gift after reflected lethal");

    auto bounced = fixture({56}, {20}, Config{2, 10, false, false, false, true});
    snapshot = bounced.snapshot(); snapshot.state.players[1].hp = 1;
    check(bounced.restore(snapshot), "restore self-bounce lethal fixture");
    draw_script(bounced, {120}); target_script(bounced, {1});
    bounced.step(attack());
    const auto self_death = bounced.step(reaction(0));
    check(self_death.terminated && !self_death.truncated && self_death.bounce_self
        && bounced.state().winner == 0 && self_death.rewards == std::array<float, 2>{1.f, -1.f},
        "self bounce lethal resolves immediately and takes precedence at horizon");
}
void tapes_restore_and_observations() {
    auto env = fixture({36}, {39, 20});
    target_script(env, {0, 1});
    const auto original = env.snapshot();
    const std::uint8_t invalid_target[] = {2};
    check(!env.script_bounce_targets(invalid_target) && env.snapshot() == original,
        "invalid bounce target prefix rejected atomically");
    std::array<std::uint8_t, 65> too_many{};
    check(!env.script_bounce_targets(too_many) && env.snapshot() == original,
        "oversized bounce target prefix rejected atomically");
    auto invalid = original; invalid.target_index = 3;
    check(!env.restore(invalid) && env.snapshot() == original, "target tape cursor checked");
    invalid = original; invalid.scripted_targets[63] = 1;
    check(!env.restore(invalid) && env.snapshot() == original, "unused target tape tail cleared");
    auto hidden = original; hidden.scripted_targets[0] = 1; hidden.scripted_targets[1] = 0;
    hidden.rng_state ^= 0x9e3779b97f4a7c15ULL;
    Env other(Config{512, 10, false, false, false, true}); check(other.restore(hidden), "restore future target variant");
    check(env.observe() == other.observe() && env.options() == other.options(), "future bounce targets and RNG hidden");
    env.step(attack());
    const auto before = env.observe();
    check(before[210] == 1 / 3.f && before[211] == 2 / 3.f && before[226] == 1.f,
        "acting hand reaction types and enabled flag exposed");
    check(env.step(reaction(0)).valid, "observe redirected state");
    const auto active = env.observe();
    check(active[227] == 1 / 32.f && active[228] == 0.f && active[229] == 1.f
        && active[230] == 39 / 363.f && active[231] == 1.f && active[232] == 0.f
        && active[233] == 0.f && active[234] == 1.f && active[235] == 0.f,
        "redirect depth, source/owner distinction and public last reaction in actor coordinates");
    const auto pending = env.snapshot();
    invalid = pending; invalid.state.reaction_count = 0;
    check(!env.restore(invalid) && env.snapshot() == pending, "changed pending source needs positive chain count");
    invalid = pending; invalid.state.players[1].pending_deals = 0;
    check(!env.restore(invalid) && env.snapshot() == pending, "redirector must retain its consumption budget");
    invalid = pending; invalid.state.pending.source = invalid.state.pending.target;
    check(!env.restore(invalid) && env.snapshot() == pending, "self-target redirected response cannot remain unresolved");

    auto disabled = fixture({5}, {120}, Config{});
    disabled.step(attack());
    const auto plain = disabled.observe();
    check(std::all_of(plain.begin() + 210, plain.end(), [](float x) { return x == 0.f; }),
        "old profile reaction observation extension stays zero");
}
void seeded_rollouts() {
    std::uint64_t reflects = 0, bounces = 0, self_bounces = 0;
    for (bool other_features : {false, true}) for (std::uint64_t seed = 0; seed < 500; ++seed) {
        const Config config{256, 10, other_features, other_features, other_features, true};
        Env a(config), b(config); a.reset(seed); b.reset(seed);
        const std::uint8_t targets[] = {0, 1};
        check(a.script_bounce_targets(targets) && b.script_bounce_targets(targets), "same finite bounce prefix for replay");
        std::uint64_t pa = seed + 9000, pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "reaction seeded snapshot replay");
            for (float value : a.observe()) check(std::isfinite(value), "reaction observation finite");
            const auto action = a.sample_action(pa), action_b = b.sample_action(pb);
            check(action == action_b && pa == pb && a.legal(action), "sampled reaction action legal and reproducible");
            const auto ra = a.step(action), rb = b.step(action_b);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.reflected == rb.reflected
                && ra.bounced == rb.bounced && ra.bounce_self == rb.bounce_self, "reaction step result replay");
            reflects += ra.reflected; bounces += ra.bounced; self_bounces += ra.bounce_self;
            Env restored(config);
            check(restored.restore(a.snapshot()) && restored.snapshot() == a.snapshot(), "every reaction-profile state restorable");
        }
    }
    check(reflects > 0 && bounces > 0 && self_bounces > 0, "rollouts exercise reflected, random and self landing paths");
}
}
int main() {
    metadata_and_profile_boundaries(); reflect_and_consumption(); bounce_target_branches();
    legality_and_nonreaction_use(); long_chain_and_horizon(); terminal_outcomes();
    tapes_restore_and_observations(); seeded_rollouts();
    std::cout << "Reflection checks passed (redirect chains, bounce targets, budgets/privacy, 1000 replay rollouts).\n";
}
