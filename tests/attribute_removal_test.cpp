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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = true;
    return c;
}();
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL attribute removal: " << message << '\n'; std::exit(1); }
}
Action attack() { return {ActionKind::attack,0,0,1}; }
Action use() { return {ActionKind::use,0,0,1}; }
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action special(int base, std::uint16_t slots = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(base),slots,no_slot}; }
Action retained(int index, std::uint16_t slots = 0) { return {ActionKind::defend_cast,static_cast<std::uint8_t>(index),slots,no_slot}; }
void hand(Player& p, std::initializer_list<CardId> cards) {
    p.hand.fill(0); std::copy(cards.begin(),cards.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(cards.size());
    p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy);
    check(env.restore(snapshot), "restore explicit removal fixture"); return env;
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "removed-attribute snapshot exact roundtrip");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "invalid conditional removal response preserves complete state and random streams");
}
void hit(Env& env) { const std::uint8_t rolls[] = {1,100}; check(env.script_hit_rolls(rolls), "script hit and future unconsumed roll"); }
void land(Env& env, std::uint8_t seat) { const std::uint8_t seats[] = {seat}; check(env.script_bounce_targets(seats), "script landing after attribute removal"); }
void metadata_and_profile() {
    check(attribute_removal_card_count == 1 && attribute_removal_catalog().size() == 1, "single removal artifact allowlist");
    const auto* item = find_attribute_removal_card(195);
    check(item && item->card.kind == Kind::armor && item->card.attack == 0 && item->card.boost == 0
        && item->card.defense == 0 && item->card.hp == 0 && item->card.mp == 0
        && item->card.price == 15 && item->card.weight == 3 && item->card.discardable,
        "old REMOVE_ATTRIBUTE remains zero-DFS protector with weight3 and price15");
    for (CardId id : {CardId{0},CardId{65},CardId{85},CardId{118},CardId{196},CardId{210},CardId{65535}})
        check(!find_attribute_removal_card(id), "no fallback to setter, zero-defense armor, ANY or magic");
    auto env = fixture({195}, {120});
    check(std::string_view(env.profile()) == attribute_removal_ruleset && env.snapshot().version == 37
        && env.options().weapon_bases == 0 && env.options().boost_slots == 0 && env.options().attribute_slots == 0
        && env.options().utility_slots == 0 && env.options().discard_slots == 1,
        "removal profile identifies separate response-only role and permits ordinary discard");
    check(env.step({ActionKind::discard,no_slot,1,no_slot}).valid && env.state().players[0].hand_count == 0,
        "discarding removal item earns no replacement or special effect"); roundtrip(env);
    auto bad_config = config; bad_config.elements = bad_config.chance = bad_config.disease = bad_config.celestial = false;
    bad_config.offensive_miracles = bad_config.defensive_miracles = bad_config.attribute_change = false;
    bool threw = false; try { Env invalid(bad_config); } catch (const std::invalid_argument&) { threw = true; }
    check(threw, "attribute removal requires elements explicitly enabled");
    Config minimal; minimal.elements = minimal.attribute_removal = true;
    auto standalone = fixture({58}, {195},minimal); standalone.step(attack());
    check(standalone.step(defend(1)).attribute_removed && standalone.state().players[1].hp == 39,
        "removal works without enabling miracles or special response catalogs"); roundtrip(standalone);
    auto disabled_config = config; disabled_config.attribute_removal = false; Env disabled(disabled_config);
    const auto original = disabled.snapshot(); const CardId draw[] = {195};
    check(!disabled.script_draws(draw) && disabled.snapshot() == original, "disabled removal draw rejected atomically");
    auto bad = original; bad.state.players[bad.state.actor].hand[0] = 195;
    check(!disabled.restore(bad) && disabled.snapshot() == original, "disabled removal hand rejected");
    bad = original; bad.version = 11; check(!disabled.restore(bad), "previous shared fixture schema rejected");
}
void ordinary_defense_and_multiple_removals() {
    constexpr std::array<CardId,7> attacks{5,58,59,60,69,62,63};
    for (int color = 0; color < 7; ++color) {
        auto env = fixture({attacks[color]}, {195}); env.step(attack());
        check(env.options().removal_slots == 1 && env.legal(defend(1)), "all seven incoming weapon elements allow removal alone");
        const auto result = env.step(defend(1));
        check(result.valid && result.attribute_removed && result.attribute_removals == 1
            && result.removed_from == static_cast<Element>(color) && !result.dark_execution
            && env.state().players[1].hp == (color == 4 ? 38 : 39),
            "zero-defense removal preserves ordinary power and eliminates final dark execution"); roundtrip(env);
    }
    auto fire = fixture({58}, {195,122,120}); fire.step(attack());
    reject(fire,defend(2)); reject(fire,defend(4));
    check(fire.legal(defend(3)) && fire.legal(defend(5)), "removal permits formerly incompatible fire/neutral normal armor");
    const auto blocked = fire.step(defend(3));
    check(blocked.attribute_removals == 1 && fire.state().players[1].hp == 40,
        "ordinary armor subtracts power after removal without itself becoming damage");
    auto duplicate = fixture({58}, {195,195,120}); duplicate.step(attack());
    check(duplicate.options().removal_slots == 3, "duplicate removal cards remain distinct slot candidates");
    const auto twice = duplicate.step(defend(7));
    check(twice.attribute_removals == 2 && twice.removed_from == Element::fire
        && duplicate.state().players[1].hand_count == 3 && duplicate.state().players[1].hp == 40,
        "two removals are idempotent but consume and replace both copies"); roundtrip(duplicate);
    auto dark = fixture({97}, {195}); dark.step(attack());
    check(!dark.step(defend(1)).dark_execution && dark.state().players[1].hp == 30,
        "ATK10 dark weapon becomes ordinary ATK10 rather than instant death");
}
void conditional_weapon_specials() {
    for (CardId weapon : {CardId{58},CardId{70},CardId{63},CardId{100}}) {
        for (int mode = 0; mode < 4; ++mode) {
            const CardId response = mode == 0 ? 39 : mode == 1 ? 20 : 215;
            auto env = fixture({weapon}, {195,response}); hit(env); land(env,1);
            auto snapshot = env.snapshot();
            if (mode == 3) snapshot.state.players[1].defensive_abilities[1] = 1;
            check(env.restore(snapshot), "restore conditional special with removal candidate"); env.step(attack());
            reject(env,mode == 3 ? retained(1) : special(1));
            const auto action = mode == 3 ? retained(1,1) : special(1,1);
            check(env.legal(action), "selected removal enables physical special against previously incompatible attribute");
            const auto result = env.step(action);
            check(result.attribute_removed && result.attribute_removals == 1,
                "conditional physical reaction consumes selected removal first");
            if (mode == 0) {
                check(result.reflected && env.state().pending.element == Element::neutral
                    && env.state().pending.attribute_removed, "physical reflection keeps removed attribute on original attack");
                roundtrip(env); env.step(defend());
            } else if (mode == 1) check(result.bounced && result.bounce_self && !result.dark_execution,
                "physical self-bounce resolves neutralized effect immediately");
            else check(result.weapon_blocked && env.state().players[1].hp == 40,
                "hand or held weapon block can follow removal");
            roundtrip(env);
        }
    }
    auto chain = fixture({100,39}, {195,39}); hit(chain); chain.step(attack()); const auto chance = chain.state().last_chance;
    chain.step(special(1,1)); roundtrip(chain);
    check(chain.state().pending.hit_rate == 75 && chain.state().last_chance == chance && chain.snapshot().hit_index == 1,
        "percent weapon reflection preserves original event without rerolling");
    const auto again = chain.step(special(0));
    check(again.reflected && !again.attribute_removed && chain.state().pending.attribute_removed
        && chain.state().last_chance == chance && chain.snapshot().hit_index == 1,
        "later physical reflection needs no second removal and does not clear the durable flag");
    roundtrip(chain); chain.step(defend()); check(chain.state().players[1].hp == 39, "twice-returned percent weapon uses original ordinary damage");
    Config independent; independent.elements = independent.chance = independent.reflection = independent.attribute_removal = true;
    auto narrow = fixture({100,39}, {195,39},independent); hit(narrow); narrow.step(attack());
    check(narrow.step(special(1,1)).reflected, "independent removal profile reflects percent weapon without defensive miracles");
    roundtrip(narrow); check(narrow.step(special(0)).reflected, "independent percent weapon can be reflected again");
    check(narrow.state().players[0].pending_deals == 2 && narrow.state().pending.attribute_removed,
        "original percent weapon owner legitimately has multiple deferred gifts in physical chain");
    roundtrip(narrow); narrow.step(defend()); roundtrip(narrow);
}
void magic_payload_and_setter_provenance() {
    auto glory = fixture({200,120}, {195,210}); hit(glory); land(glory,0); glory.step(use());
    const auto original = glory.snapshot(); const auto result = glory.step(special(1,1));
    check(result.magic_bounced && result.removed_from == Element::light && glory.state().pending.attribute_removed
        && glory.state().pending.miracle == 200 && glory.state().pending.element == Element::neutral
        && glory.state().pending.hit_rate == 25 && glory.state().pending.harm == Harm::glory
        && glory.state().players[0].mp == original.state.players[0].mp
        && glory.state().last_chance == original.state.last_chance && glory.snapshot().hit_index == 1,
        "removed chance miracle keeps ID, original MP payment, hit event and Glory payload"); roundtrip(glory);
    check(glory.legal(defend(1)), "removed light magic may be blocked by ordinary neutral armor");
    glory.step(defend(1)); check(!glory.state().players[0].glory && glory.state().players[0].hp == 40,
        "full defense prevents originally unblocked light Glory payload after neutralization");
    auto dark = fixture({207}, {195,210}); land(dark,1); dark.step(use());
    const auto returned = dark.step(special(1,1));
    check(returned.magic_bounced && returned.bounce_self && returned.attribute_removed
        && !returned.dark_execution && dark.state().players[1].hp == 35,
        "dark magic removal survives self-bounce and suppresses dark execution"); roundtrip(dark);
    auto utility = fixture({216}, {195,210}); land(utility,0); utility.step(use()); utility.step(special(1,1));
    check(utility.state().pending.kind == Effect::heal && utility.state().pending.attribute_removed,
        "already-neutral healing miracle may still consume removal and bounce"); roundtrip(utility);
    utility.step(defend()); check(utility.state().players[0].hp == 50, "removal never reduces heal amount");
    auto incompatible_type = fixture({198}, {195,39,215}); incompatible_type.step(use());
    reject(incompatible_type,special(1,1)); reject(incompatible_type,special(2,1));
    auto changed = fixture({43,65}, {195,39});
    changed.step({ActionKind::attack_set_fire,0,2,1}); changed.step(special(1,1));
    check(changed.state().pending.attribute_card == 65 && changed.state().pending.attribute_removed
        && changed.state().pending.harm == Harm::dark_cloud, "setter, removal and original status provenance coexist");
    roundtrip(changed); changed.step(defend());
    check(changed.state().players[0].dark_cloud && changed.state().players[0].hp == 29,
        "removed setter attack retains base-only power and reflected on-damage status"); roundtrip(changed);
}
void glory_sampler_privacy_and_validation() {
    auto env = fixture({100}, {195,120,122,39,20,215,210}); hit(env); env.step(attack());
    auto snapshot = env.snapshot(); snapshot.state.players[1].defensive_abilities = {1,1};
    check(env.restore(snapshot), "restore retained candidates alongside broad removal choices");
    const auto obs = env.observe();
    check(obs[375] == 1.f && obs[376] == 1.f && obs[377] == 0.f && obs[392] == 0.f,
        "new hand removal flag and current removal state represented");
    for (bool glory : {false,true}) {
        snapshot = env.snapshot(); snapshot.state.players[1].glory = glory;
        check(env.restore(snapshot), "restore ordinary/Glory removal sampling variant");
        if (glory) { reject(env,special(3,1)); reject(env,retained(1,1)); check(env.legal(defend(1)), "Glory allows removal alone"); }
        for (std::uint64_t seed = 0; seed < 2000; ++seed) {
            auto rng = seed; const auto action = env.sample_action(rng);
            check(env.legal(action), "sampler conditionally includes removal or falls back to a legal response");
            auto branch = env; check(branch.step(action).valid, "sampled removal combination executes"); roundtrip(branch);
        }
    }
    auto hidden = env.snapshot(); hidden.state.players[0].defensive_abilities = {7,8}; hidden.rng_state ^= 0x9e3779b97f4a7c15ULL;
    hidden.scripted_hits[1] = 1; Env other(config);
    check(other.restore(hidden) && other.observe() == env.observe() && other.options() == env.options(),
        "hidden retained abilities, future hit roll and RNG stay private");
    auto private_hand = fixture({5}, {195}); const auto visible = private_hand.observe(); const auto choices = private_hand.options();
    auto swapped = private_hand.snapshot(); swapped.state.players[1].hand[0] = 120;
    check(private_hand.restore(swapped) && private_hand.observe() == visible && private_hand.options() == choices,
        "opponent removal-card identity is not exposed by the new hand flags");
    auto chain = fixture({100}, {195,39}); hit(chain); chain.step(attack()); chain.step(special(1,1));
    const auto exact = chain.snapshot(); check(chain.observe()[392] == 1.f, "pending removal flag public during reflected response");
    for (int error = 0; error < 4; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.pending.element = Element::fire;
        if (error == 1) bad.state.pending.attribute_removed = false;
        if (error == 2) --bad.state.players[1].pending_deals;
        if (error == 3) bad.config.attribute_removal = false;
        check(!chain.restore(bad) && chain.snapshot() == exact,
            "invalid current attribute, missing removal provenance, missing gift or profile rejected atomically");
    }
    auto initial = fixture({5}, {195}); initial.step(attack()); auto forged = initial.snapshot();
    forged.state.pending.attribute_removed = true;
    check(!initial.restore(forged), "unresolved removal cannot precede any response-chain decision");
    auto disabled_config = config; disabled_config.attribute_removal = false; Env disabled(disabled_config); const auto old = disabled.observe();
    check(std::all_of(old.begin()+375,old.end(),[](float x){return x==0.f;}), "old profile new removal suffix zero");
}
void replay() {
    std::uint64_t events = 0, copies = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+55000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "removal game trajectory replay");
            for (float x : a.observe()) check(std::isfinite(x), "removal observations finite");
            const auto action = a.sample_action(pa), copy = b.sample_action(pb);
            check(action == copy && pa == pb && a.legal(action), "removal policy deterministic and legal");
            const auto ra = a.step(action), rb = b.step(copy);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.attribute_removed == rb.attribute_removed
                && ra.attribute_removals == rb.attribute_removals && ra.removed_from == rb.removed_from,
                "removal event metadata replay");
            events += ra.attribute_removed; copies += ra.attribute_removals; roundtrip(a);
        }
    }
    check(events && copies, "seeded games actually consume newly drawn removal artifacts");
}
}
int main() {
    metadata_and_profile(); ordinary_defense_and_multiple_removals(); conditional_weapon_specials();
    magic_payload_and_setter_provenance(); glory_sampler_privacy_and_validation(); replay();
    std::cout << "Attribute removal checks passed (conditional reactions, percent/magic provenance, 1000 replay rollouts).\n";
}
