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
constexpr Config config{512,10,true,true,true,true,true,true,true,true,true};
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL defensive miracles: " << message << '\n'; std::exit(1); }
}
Action attack() { return {ActionKind::attack,0,0,1}; }
Action use() { return {ActionKind::use,0,0,1}; }
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action special(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action retained(int index, std::uint16_t prefix = 0) { return {ActionKind::defend_cast,static_cast<std::uint8_t>(index),prefix,no_slot}; }
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin());
    p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy);
    check(env.restore(snapshot), "restore explicit defensive miracle fixture"); return env;
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "defensive magic snapshot roundtrip");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "invalid defensive magic preserves cards, counts, MP, gifts and all random streams");
}
void target(Env& env, std::initializer_list<std::uint8_t> seats) {
    check(env.script_bounce_targets({seats.begin(),seats.size()}), "install defensive magic bounce targets");
}
void metadata_and_profile() {
    check(defensive_miracle_count == 2 && defensive_miracles().size() == 2
        && defensive_miracle_index(210) == 0 && defensive_miracle_index(215) == 1,
        "two separate stable defensive ability slots");
    const auto* bounce = find_defensive_miracle(210); const auto* block = find_defensive_miracle(215);
    check(bounce && bounce->cost == 5 && bounce->weight == 1 && bounce->kind == DefensiveMiracleKind::bounce_magic
        && block && block->cost == 6 && block->weight == 1 && block->kind == DefensiveMiracleKind::block_weapon,
        "exact legacy bounce-magic and block-weapon metadata");
    for (CardId id : {CardId{0},CardId{40},CardId{119},CardId{171},CardId{196},CardId{198},CardId{216},CardId{65535}})
        check(!find_defensive_miracle(id) && defensive_miracle_index(id) == -1, "unsupported counter, ANY and physical magic reactions excluded");
    for (int missing = 0; missing < 2; ++missing) {
        auto bad = config; if (missing == 0) bad.offensive_miracles = false; else bad.reflection = false;
        bool threw = false; try { Env invalid(bad); } catch (const std::invalid_argument&) { threw = true; }
        check(threw, "defensive miracles require offensive profile and reflection semantics");
    }
    auto env = fixture({210,215}, {120});
    check(std::string_view(env.profile()) == defensive_miracles_ruleset && env.snapshot().version == 36
        && env.state().players[0].abilities.size() == main_ability_capacity && env.state().players[0].defensive_abilities.size() == 2,
        "new profile/schema preserves old fifteen kinds and separate defensive pair");
    check(env.options().utility_slots == 0 && env.options().ability_slots == 0
        && env.options().discard_slots == 3 && env.options().pray,
        "defensive-only spells cannot be empty-cast in main; discard and pray remain available");
    reject(env,use()); reject(env,{ActionKind::cast,0,0,0}); reject(env,retained(0));
    const auto dropped = env.step({ActionKind::discard,no_slot,3,no_slot});
    check(dropped.valid && env.state().players[0].mp == 10 && env.state().players[0].hand_count == 0
        && env.state().players[0].defensive_abilities == std::array<std::uint32_t,2>{},
        "discard does not acquire, pay MP or earn gifts"); roundtrip(env);
    auto disabled_config = config; disabled_config.defensive_miracles = false; Env disabled(disabled_config);
    const auto original = disabled.snapshot(); const CardId draws[] = {210,215};
    check(!disabled.script_draws(draws) && disabled.snapshot() == original, "disabled defensive pool rejected atomically");
    auto bad = original; bad.state.players[0].defensive_abilities[0] = 1;
    check(!disabled.restore(bad) && disabled.snapshot() == original, "disabled defensive retained kind rejected");
    bad = original; bad.state.players[0].retained_deals = 1;
    check(!disabled.restore(bad) && disabled.snapshot() == original, "disabled v10 gift accounting rejected");
    bad = original; bad.state.players[bad.state.actor].hand[0] = 210;
    check(!disabled.restore(bad) && disabled.snapshot() == original, "disabled defensive hand rejected");
}
void block_first_recast_and_limits() {
    for (bool ability : {false,true}) {
        auto env = fixture({43}, ability ? std::initializer_list<CardId>{120} : std::initializer_list<CardId>{215});
        auto snapshot = env.snapshot(); snapshot.state.players[1].mp = 6;
        if (ability) snapshot.state.players[1].defensive_abilities[1] = 2;
        check(env.restore(snapshot), "restore exact block budget and ownership"); env.step(attack());
        check(env.options().defensive_ability_slots == (ability ? 2 : 0)
            && env.options().reaction_slots == (ability ? 0 : 1), "hand and retained block masks separate");
        const auto result = env.step(ability ? retained(1) : special(0));
        check(result.valid && result.weapon_blocked && !result.magic_bounced && !result.reflected
            && result.defensive_first_use == !ability && result.defensive_recast == ability
            && result.spell_first_use == !ability && result.spell_recast == ability,
            "block emits defensive and shared spell acquisition metrics");
        check(env.state().players[1].hp == 40 && !env.state().players[1].dark_cloud
            && env.state().players[1].mp == 0 && env.state().players[1].defensive_abilities[1] == (ability ? 2u : 1u)
            && env.state().players[1].hand_count == (ability ? 2 : 1), "block prevents damage/status and distributes response gift");
        check(env.state().last_reaction.id == 215 && env.state().last_reaction.source == 1
            && env.state().last_reaction.target == 1 && env.state().last_reaction.from_ability == ability
            && env.observe()[354] == 1.f && env.observe()[353] == float(ability), "public block event uses documented observation convention");
        roundtrip(env);
    }
    auto poor = fixture({5}, {215}); auto snapshot = poor.snapshot(); snapshot.state.players[1].mp = 5;
    snapshot.state.players[1].defensive_abilities[1] = 1;
    check(poor.restore(snapshot), "restore unaffordable first-use and retained block"); poor.step(attack());
    check(!poor.options().reaction_slots && !poor.options().defensive_ability_slots, "MP shortage removes both defensive routes");
    reject(poor,special(0)); reject(poor,retained(1)); reject(poor,retained(2)); reject(poor,retained(255));

    for (CardId id : {CardId{58},CardId{63},CardId{70},CardId{100}}) {
        auto env = fixture({id}, {215}); const std::uint8_t hit[] = {1}; check(env.script_hit_rolls(hit), "force colored weapon hit");
        env.step(attack()); check(!env.options().reaction_slots, "block weapon rejects colored, dark, light and colored chance attacks");
        reject(env,special(0));
    }
    for (CardId id : {CardId{198},CardId{216},CardId{227}}) {
        auto env = fixture({id}, {215}); env.step(use()); reject(env,special(0));
    }
    auto prefix = fixture({43}, {118,215}); prefix.step(attack());
    const auto result = prefix.step(special(1,1));
    check(result.weapon_blocked && prefix.state().players[1].hand_count == 2,
        "zero-defense ordinary prefix and block both consume and earn replacement");
    auto glory = fixture({5}, {120,215}); snapshot = glory.snapshot(); snapshot.state.players[1].glory = true;
    snapshot.state.players[1].defensive_abilities[1] = 1; check(glory.restore(snapshot), "restore Glory defensive caster"); glory.step(attack());
    check(glory.legal(special(1)) && glory.legal(retained(1)), "Glory permits one hand or retained defensive spell");
    reject(glory,special(1,1)); reject(glory,retained(1,1));
    auto limit = fixture({5}, {215}); snapshot = limit.snapshot(); snapshot.state.players[1].abilities[0] = config.max_decisions;
    check(limit.restore(snapshot), "restore shared acquisition bound across all seventeen kinds"); limit.step(attack());
    reject(limit,special(0));
    auto dying_owner = fixture({43}, {215}); snapshot = dying_owner.snapshot();
    snapshot.state.players[0].hp = 1; snapshot.state.players[0].disease = Disease::cold;
    check(dying_owner.restore(snapshot), "restore weapon owner who dies on blocked turn boundary");
    const std::uint8_t natural[] = {99}; check(dying_owner.script_disease_rolls(natural), "script blocked-turn owner tick");
    dying_owner.step(attack()); const auto ended = dying_owner.step(special(0));
    check(ended.weapon_blocked && ended.disease_tick && ended.disease_damage == 1 && ended.terminated
        && dying_owner.state().winner == 1 && dying_owner.snapshot().disease_index == 1,
        "block ends original owner's turn and ticks that owner exactly once before winner selection");
    roundtrip(dying_owner);
}
void bouncing_magic_and_provenance() {
    for (CardId spell : {CardId{198},CardId{200},CardId{207},CardId{216},CardId{220}}) {
        for (bool ability : {false,true}) for (std::uint8_t landing : {std::uint8_t{0},std::uint8_t{1}}) {
            auto env = fixture({spell}, ability ? std::initializer_list<CardId>{120} : std::initializer_list<CardId>{210});
            auto snapshot = env.snapshot(); snapshot.state.players[0].hp = 30; snapshot.state.players[1].hp = 30;
            snapshot.state.players[1].mp = 5;
            if (ability) snapshot.state.players[1].defensive_abilities[0] = 2;
            check(env.restore(snapshot), "restore exact magic bounce fixture");
            const std::uint8_t hit[] = {1,100}; check(env.script_hit_rolls(hit), "install original hit and preserved future roll");
            target(env,{landing}); env.step(use()); const auto origin = env.snapshot();
            const auto result = env.step(ability ? retained(0) : special(0));
            check(result.valid && result.magic_bounced && result.bounced && result.bounce_self == (landing == 1)
                && result.defensive_first_use == !ability && result.defensive_recast == ability,
                "magic bounce acquires or reuses with selected landing");
            check(env.state().players[1].mp == 0 && env.state().players[1].defensive_abilities[0] == (ability ? 2u : 1u)
                && env.state().players[0].mp == origin.state.players[0].mp
                && env.state().players[0].abilities == origin.state.players[0].abilities
                && env.snapshot().hit_index == origin.hit_index && env.state().last_chance == origin.state.last_chance,
                "bounce preserves original spell payment/ownership and never rerolls chance");
            if (landing == 0) {
                check(env.state().phase == Phase::response && env.state().actor == 0
                    && env.state().pending.source == 1 && env.state().pending.target == 0
                    && env.state().pending.miracle == spell && env.state().turn_owner == 0,
                    "bounced pending uses new source but original caster remains owner");
                if (ability) {
                    const auto exact = env.snapshot(); auto missing = exact;
                    missing.state.players[1].retained_deals = 0;
                    check(!env.restore(missing) && env.snapshot() == exact,
                        "unresolved retained defensive event requires its scheduled retained gift");
                }
                roundtrip(env); env.step(defend());
            }
            const int destination = landing;
            const int amount = spell == 198 ? 4 : spell == 200 ? 1 : spell == 207 ? 5 : 10;
            if (spell == 220) check(env.state().players[destination].money == 35, "money miracle applies full original value to bounce target");
            else check(env.state().players[destination].hp == (spell == 207 ? 0 : spell == 216 ? 40 : 30-amount),
                "original damage/heal value and dark execution reach final bounce target");
            if (spell == 200) check(env.state().players[destination].glory, "Glory magic follows final bounced target");
            roundtrip(env);
        }
    }
    for (CardId id : {CardId{5},CardId{227},CardId{231},CardId{236}}) {
        auto env = fixture({id}, {210}); env.step(id == 5 ? attack() : use());
        check(!env.options().reaction_slots, "magic bounce excludes weapon and sundry utilities"); reject(env,special(0));
    }
    auto prefix = fixture({198}, {166,210}); target(prefix,{0}); prefix.step(use()); prefix.step(special(1,1));
    check(prefix.state().pending.amount == 4 && prefix.state().players[1].hand_count == 0
        && prefix.state().players[1].pending_deals == 2, "ordinary prefix consumed but bounced magic keeps original power");
    roundtrip(prefix); prefix.step(defend()); check(prefix.state().players[0].hp == 36, "prefix defense is discarded from returned original magic");
}
void utility_prefix_and_provisional_zero() {
    for (CardId id : {CardId{216},CardId{220},CardId{227},CardId{231},CardId{235}}) {
        auto env = fixture({id}, {186}); auto snapshot = env.snapshot(); snapshot.state.players[1].hp = 30;
        snapshot.state.players[1].disease = Disease::cold;
        check(env.restore(snapshot), "restore utility prefix fixture"); env.step(use());
        check(env.legal(defend(1)), "v10 allows ordinary armor response to non-damage utility"); env.step(defend(1));
        check(env.state().players[1].hand_count == 1, "utility armor response consumes and replaces armor");
        if (id == 216) check(env.state().players[1].hp == 40, "DEF30 does not reduce HP+10 magic");
        if (id == 220) check(env.state().players[1].money == 35, "DEF30 does not reduce money+15 magic");
        if (id == 227) check(env.state().players[1].hp == 35, "DEF30 does not reduce sundry HP+5");
        if (id == 231) check(env.state().players[1].mp == 15, "DEF30 does not reduce sundry MP+5");
        if (id == 235) check(env.state().players[1].disease == Disease::none, "ordinary armor does not cancel cure");
        roundtrip(env);
    }
    for (std::uint8_t landing : {std::uint8_t{0},std::uint8_t{1}}) {
        auto env = fixture({216}, {175,210}); auto snapshot = env.snapshot();
        snapshot.state.players[1].disease = Disease::heaven;
        check(env.restore(snapshot), "restore Heaven defender with disease prefix and magic bounce");
        target(env,{landing}); env.step(use()); env.step(special(1,1));
        if (landing == 0) {
            check(env.state().players[1].hp == 0 && env.state().phase == Phase::response,
                "Heaven prefix fall remains provisional during returned heal response"); roundtrip(env);
            env.step(defend()); check(env.state().terminated && env.state().winner == 0,
                "unhealed provisional death finalized after response chain");
        } else check(!env.done() && env.state().players[1].hp == 10,
            "self-bounced heal revives provisional HP0 before death finalization");
        roundtrip(env);
    }
    auto disease = fixture({234}, {175}); disease.step(use()); disease.step(defend(1));
    check(disease.state().players[1].mp == 30 && disease.state().players[1].disease == Disease::heaven,
        "utility prefix disease occurs before MP gain and independent Heaven application"); roundtrip(disease);
    auto revived_owner = fixture({216,175,210}, {210}); auto snapshot = revived_owner.snapshot();
    snapshot.state.players[0].mp = 20; snapshot.state.players[0].disease = Disease::heaven;
    check(revived_owner.restore(snapshot), "restore Heaven owner for returned-heal self rescue");
    const std::uint8_t natural[] = {99}; check(revived_owner.script_disease_rolls(natural), "script revived owner's only natural tick");
    target(revived_owner,{0,0}); revived_owner.step(use()); revived_owner.step(special(0));
    const auto revived = revived_owner.step(special(1,1));
    check(revived.magic_bounced && revived.bounce_self && revived.disease_tick && revived.disease_healing == 5
        && revived_owner.state().players[0].hp == 15 && revived_owner.snapshot().disease_index == 1
        && !revived_owner.done(), "returned heal revives Heaven owner before exactly one Heaven healing tick");
    roundtrip(revived_owner);
}
void magic_bounce_incompatible_prefix() {
    // MAGIC skips the final ordinary-defense attribute test when 210 fires.
    // Thus the component prefix mask must include armor that is unusable alone.
    for (bool ability : {false,true}) for (CardId spell : {CardId{206},CardId{205}}) {
        auto env = fixture({spell}, ability ? std::initializer_list<CardId>{122,120}
                                           : std::initializer_list<CardId>{122,120,210});
        auto snapshot = env.snapshot();
        if (ability) snapshot.state.players[1].defensive_abilities[0] = 1;
        check(env.restore(snapshot), "restore incompatible magic-prefix fixture");
        const std::uint8_t hit[] = {1}; check(env.script_hit_rolls(hit), "force light magic prefix test hit");
        target(env,{0}); env.step(use());
        check((env.options().defense_slots & 3) == 3, "magic bounce keeps fire and neutral armor prefix candidates");
        reject(env,defend(1)); reject(env,defend(2)); reject(env,defend(3));
        for (std::uint16_t mask : {std::uint16_t{1},std::uint16_t{2},std::uint16_t{3}}) {
            auto branch = env; const auto result = branch.step(ability ? retained(0,mask) : special(2,mask));
            check(result.valid && result.magic_bounced && branch.state().pending.amount == 10,
                "incompatible ordinary prefix is consumed before original magic bounce");
            roundtrip(branch); branch.step(defend());
            check(branch.state().players[0].hp == 30, "incompatible prefix never reduces bounced spell amount");
        }
        bool sampled_bounce = false;
        for (std::uint64_t seed = 0; seed < 1000; ++seed) {
            auto rng = seed; const auto action = env.sample_action(rng);
            check(env.legal(action), "sampler distinguishes expanded prefix candidates from ordinary legal defense");
            auto branch = env; const auto result = branch.step(action);
            check(result.valid, "sampled expanded-prefix response executes legally");
            sampled_bounce |= result.magic_bounced; roundtrip(branch);
        }
        check(sampled_bounce, "expanded-prefix sampling includes valid magic bounce");
        snapshot = env.snapshot(); snapshot.state.players[1].glory = true;
        check(env.restore(snapshot), "restore Glory with expanded magic-prefix candidates");
        check(env.legal(ability ? retained(0) : special(2)), "Glory still permits lone magic bounce");
        reject(env,ability ? retained(0,1) : special(2,1));
        for (std::uint64_t seed = 0; seed < 1000; ++seed) {
            auto rng = seed; check(env.legal(env.sample_action(rng)), "Glory sampler avoids incompatible ordinary defense and multi-artifact bounce");
        }
    }
}
void long_retained_chain_and_truncation() {
    auto env = fixture({120}, {120}); auto snapshot = env.snapshot();
    for (auto& p : snapshot.state.players) {
        p.hand.fill(120); p.hand_count = 16; p.mp = 99; p.defensive_abilities[0] = 1;
    }
    snapshot.state.players[0].abilities[2] = 1;
    check(env.restore(snapshot), "restore full inventories and maximum MP for long chain");
    std::array<CardId,64> gifts{}; gifts.fill(120); check(env.script_draws(gifts), "script all deferred overflow gifts");
    std::array<std::uint8_t,38> seats{};
    for (std::size_t i = 0; i < seats.size(); ++i) seats[i] = i % 2;
    check(env.script_bounce_targets(seats), "script 38 alternating magic returns");
    check(env.step({ActionKind::cast,2,0,1}).valid, "launch original retained attack from full hand");
    const auto origin = env.snapshot(); auto missing = origin; missing.state.players[0].retained_deals = 0;
    check(!env.restore(missing) && env.snapshot() == origin, "initial held spell requires exactly one retained-origin gift");
    for (int i = 0; i < 38; ++i) {
        check(env.legal(retained(0)), "MP-supported chain is not capped at hand16 or reaction32");
        const auto result = env.step(retained(0)); check(result.magic_bounced && result.defensive_recast, "long chain reuses defensive spell");
        check(env.snapshot().draw_index == 0 && env.state().turns == 0, "all gifts and owner tick deferred throughout chain");
        roundtrip(env);
    }
    check(env.state().reaction_count == 38 && env.state().players[0].pending_deals == 20
        && env.state().players[1].pending_deals == 19 && env.state().players[0].retained_deals == 20
        && env.state().players[1].retained_deals == 19 && !env.options().defensive_ability_slots,
        "38 returns tracked with >16 pending gifts until both MP budgets exhausted");
    env.step(defend());
    check(env.snapshot().draw_index == 39 && env.state().players[0].hand_count == 16
        && env.state().players[1].hand_count == 16 && env.state().players[0].mp == 2
        && env.state().players[1].mp == 4 && env.state().reaction_count == 0,
        "long chain pays each cast once and consumes all 39 overflow outcomes at final distribution");
    for (const auto& p : env.state().players) check(!p.pending_deals && !p.retained_deals, "resolved retained gift accounting cleared");
    roundtrip(env);
    auto cfg = config; cfg.max_decisions = 2;
    auto truncated = fixture({198}, {210},cfg); target(truncated,{0}); truncated.step(use());
    check(truncated.step(special(0)).truncated && truncated.state().phase == Phase::response
        && truncated.state().turns == 0, "learning cap may freeze unresolved magic chain without distribution"); roundtrip(truncated);
    reject(truncated,defend());
}
void observations_privacy_and_replay() {
    auto env = fixture({5}, {210,215}); auto snapshot = env.snapshot();
    snapshot.state.players[1].defensive_abilities = {2,3}; check(env.restore(snapshot), "restore defensive observation fixture"); env.step(attack());
    const auto obs = env.observe();
    check(obs[334] == 1.f && obs[335] == 2.f/512 && obs[336] == 3.f/512
        && obs[337] == .5f && obs[338] == 1.f, "new defensive counts and hand kind suffix");
    for (std::uint64_t seed = 0; seed < 1024; ++seed) {
        auto rng = seed; const auto action = env.sample_action(rng); check(env.legal(action), "response sampler includes only legal affordable magic choices");
        auto branch = env; check(branch.step(action).valid, "sampled mixed ordinary/retained defensive response executes"); roundtrip(branch);
    }
    auto hidden = env.snapshot(); hidden.state.players[0].defensive_abilities = {9,8};
    hidden.rng_state ^= 0x9e3779b97f4a7c15ULL;
    Env other(config); check(other.restore(hidden) && other.observe() == env.observe() && other.options() == env.options(),
        "opponent defensive ability counts and future randomness private");
    auto invalid = env.snapshot(); invalid.state.players[1].defensive_abilities[0] = config.max_decisions;
    check(!env.restore(invalid), "acquisition sum includes both defensive and original abilities");
    invalid = env.snapshot(); invalid.state.players[1].retained_deals = 1;
    check(!env.restore(invalid), "retained gifts cannot exceed scheduled gifts");
    auto first = fixture({198}, {120}); first.step(use());
    const auto first_pending = first.snapshot(); invalid = first_pending;
    invalid.state.players[0].retained_deals = 1;
    check(!first.restore(invalid) && first.snapshot() == first_pending,
        "initial hand-used spell cannot claim retained-origin gift");
    auto disabled_config = config; disabled_config.defensive_miracles = false; Env disabled(disabled_config); const auto old = disabled.observe();
    check(std::all_of(old.begin()+334,old.end(),[](float x){return x==0.f;}), "old mode defensive suffix remains zero");
    std::uint64_t acquired = 0, recast = 0, bounce = 0, block = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+35000, pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "defensive spell trajectory deterministic");
            for (float x : a.observe()) check(std::isfinite(x), "all defensive observations finite");
            const auto action = a.sample_action(pa), copy = b.sample_action(pb);
            check(action == copy && pa == pb && a.legal(action), "defensive policy deterministic and legal");
            const auto ra = a.step(action), rb = b.step(copy);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.defensive_first_use == rb.defensive_first_use
                && ra.defensive_recast == rb.defensive_recast && ra.magic_bounced == rb.magic_bounced
                && ra.weapon_blocked == rb.weapon_blocked, "defensive transition metrics replay");
            acquired += ra.defensive_first_use; recast += ra.defensive_recast; bounce += ra.magic_bounced; block += ra.weapon_blocked;
            roundtrip(a);
        }
    }
    check(acquired && recast && bounce && block, "natural seeded games exercise both defensive abilities and retained reuse");
}
}
int main() {
    metadata_and_profile(); block_first_recast_and_limits(); bouncing_magic_and_provenance();
    utility_prefix_and_provisional_zero(); magic_bounce_incompatible_prefix();
    long_retained_chain_and_truncation(); observations_privacy_and_replay();
    std::cout << "Defensive miracle checks passed (magic bounce, weapon block, 38-return full-hand chain, 1000 replay rollouts).\n";
}
