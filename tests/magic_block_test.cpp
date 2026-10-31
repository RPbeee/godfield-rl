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
const Config config = [] {
    Config c; c.miracles = c.elements = c.chance = c.reflection = c.disease = c.harms = true;
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = true;
    return c;
}();
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL magic block: " << message << '\n'; std::exit(1); }
}
Action attack(std::uint16_t slots = 0) { return {ActionKind::attack,0,slots,1}; }
Action use() { return {ActionKind::use,0,0,1}; }
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action block(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
void hand(Player& p, std::initializer_list<CardId> cards) {
    p.hand.fill(0); std::copy(cards.begin(),cards.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(cards.size());
    p.pending_deals = p.retained_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy);
    check(env.restore(snapshot), "restore explicit magic block fixture"); return env;
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "magic block event survives exact snapshot roundtrip");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "invalid matching blocker opt-out or type rejected atomically");
}
void gifts(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "script delayed block gifts"); }
void metadata_and_ordinary_roles() {
    constexpr std::array<CardId,8> ids{27,54,55,152,165,182,183,185};
    constexpr std::array<int,8> atk{7,15,15,0,0,0,0,0}, dfs{0,0,0,6,8,13,14,15};
    check(magic_block_card_count == 8 && magic_block_catalog().size() == 8, "eight positive-weight blockers only");
    Config narrow; narrow.magic_block = true;
    for (int i = 0; i < 8; ++i) {
        const auto* card = find_magic_block_card(ids[i]);
        check(card && magic_block_catalog()[i].card.id == ids[i] && card->card.attack == atk[i]
            && card->card.boost == (i == 2 ? 15 : 0) && card->card.defense == dfs[i]
            && !card->card.hp && !card->card.mp && card->card.weight == 1 && card->card.discardable == (i >= 3)
            && card->card.price == (i == 0 || i >= 5 ? 15 : 10), "blocker dual roles and original card fields preserved");
        auto env = fixture({ids[i]}, {120},narrow);
        check(std::string_view(env.profile()) == magic_block_ruleset && env.snapshot().version == 36,
            "dependency-free blocker profile and common schema19");
        if (i < 3) {
            check(env.options().weapon_bases == 1 && !env.options().discard_slots && !env.options().pray,
                "weapon blockers remain undiscardable attack bases");
            check(env.step(attack()).valid && env.state().pending.amount == atk[i], "block weapon retains normal attack power");
        } else {
            check(env.options().discard_slots == 1 && !env.options().weapon_bases && env.options().pray,
                "armor blockers retain ordinary main discard and pray roles");
            auto ordinary = fixture({36}, {ids[i]},narrow); ordinary.step(attack());
            check(!ordinary.options().reaction_slots && ordinary.legal(defend(1)), "non-MAGIC uses ordinary armor without forced block");
            ordinary.step(defend(1)); check(ordinary.state().players[1].hp == 40-std::max(0,10-dfs[i]),
                "printed DEF applies against non-MAGIC weapon"); roundtrip(ordinary);
        }
        roundtrip(env);
    }
    for (CardId id : {CardId{0},CardId{26},CardId{40},CardId{57},CardId{171},CardId{196},CardId{312},CardId{313},CardId{65535}})
        check(!find_magic_block_card(id), "zero-weight and other reaction effects not implicitly included");
    auto extra = fixture({5,55}, {120},narrow);
    check(extra.options().boost_slots == 2 && extra.step(attack(2)).valid && extra.state().pending.amount == 16,
        "weapon55 retains INCREASE_ATK15 as an extra"); roundtrip(extra);
    auto old_cfg = config; old_cfg.magic_block = false; Env old(old_cfg); const auto original = old.snapshot();
    check(!old.script_draws(ids) && old.snapshot() == original, "old pool rejects all new blockers atomically");
    auto bad = original; bad.state.players[bad.state.actor].hand[0] = 152;
    check(!old.restore(bad) && old.snapshot() == original, "disabled hand blocker rejected");
}
void forced_magic_block_and_all_payloads() {
    constexpr std::array<CardId,8> ids{27,54,55,152,165,182,183,185};
    for (CardId id : ids) {
        auto env = fixture({198}, {id}); gifts(env); env.step(use()); const auto before = env.snapshot();
        check(env.options().reaction_slots == 1 && !env.options().defense_slots,
            "matching MAGIC requires base-only blocker, not ordinary DEF opt-out"); reject(env,defend(1));
        const auto result = env.step(block(0));
        check(result.magic_blocked && !result.weapon_blocked && !result.reflected && !result.bounced
            && !result.spell_first_use && !result.spell_recast && env.state().players[1].hp == 40
            && env.state().players[1].mp == 10 && env.state().players[0].mp == before.state.players[0].mp
            && env.state().players[0].abilities == before.state.players[0].abilities
            && env.state().last_reaction.id == id && env.state().last_reaction.reaction == WeaponReaction::block
            && env.state().last_reaction.source == 1 && env.state().last_reaction.target == 1
            && !env.state().last_reaction.from_ability && env.observe()[749] == 1.f && env.snapshot().draw_index == 2,
            "all8 blockers cancel magic without their DFS, MP, acquisition or reflection and distribute once"); roundtrip(env);
    }
    for (CardId spell : {CardId{198},CardId{200},CardId{206},CardId{207},CardId{216},CardId{220}}) {
        auto env = fixture({spell}, {185}); auto snapshot = env.snapshot(); snapshot.state.players[1].hp = 30;
        check(env.restore(snapshot), "restore heal/damage/glory/dark/money victim"); const std::uint8_t rolls[] = {1,100};
        check(env.script_hit_rolls(rolls), "script chance hit and reserved future roll"); env.step(use()); const auto paid = env.snapshot();
        const auto result = env.step(block(0));
        check(result.magic_blocked && env.state().players[1].hp == 30 && env.state().players[1].money == 20
            && !env.state().players[1].glory && !env.state().players[1].dead && !result.dark_execution
            && env.state().players[0].mp == paid.state.players[0].mp && env.state().players[0].abilities == paid.state.players[0].abilities
            && env.snapshot().hit_index == paid.hit_index && env.state().last_chance == paid.state.last_chance,
            "block all MAGIC effects after original payment/acquisition/hit without reroll or target side effect"); roundtrip(env);
    }
    Config minimum; minimum.miracles = minimum.magic_block = true;
    auto narrow = fixture({216}, {152},minimum); narrow.step(use());
    check(narrow.step(block(0)).magic_blocked && !narrow.snapshot().config.reflection,
        "magic block events do not require unrelated reflection flag"); roundtrip(narrow);
    auto held = fixture({120}, {152}); auto snapshot = held.snapshot(); snapshot.state.players[0].abilities[0] = 1;
    check(held.restore(snapshot), "restore held heal before block"); held.step({ActionKind::cast,0,0,1});
    check(held.step(block(0)).magic_blocked && held.state().players[0].abilities[0] == 1 && held.state().players[0].mp == 4,
        "blocking original held magic keeps ability and MP cost"); roundtrip(held);
    Config full_cfg; full_cfg.miracles = full_cfg.elements = full_cfg.counters = full_cfg.magic_block = true;
    auto full = fixture({120}, {152},full_cfg); snapshot = full.snapshot();
    snapshot.state.players[0].hand.fill(120); snapshot.state.players[0].hand_count = 16;
    snapshot.state.players[0].abilities[0] = 1;
    check(full.restore(snapshot), "restore full-hand held cast with counters but no reflection or defensive miracles"); gifts(full);
    full.step({ActionKind::cast,0,0,1});
    check(full.state().players[0].pending_deals == 1 && full.state().players[0].retained_deals == 1,
        "magic block independently tracks full-hand original held gift"); roundtrip(full);
    check(full.step(block(0)).magic_blocked && full.snapshot().draw_index == 2
        && full.state().players[0].hand_count == 16 && full.state().players[0].mp == 4
        && !full.state().players[0].pending_deals && !full.state().players[0].retained_deals,
        "block completes held full-hand overflow draw and defender replacement with narrow dependencies"); roundtrip(full);
}
void prefixes_glory_death_and_nonmagic() {
    auto env = fixture({206}, {122,185}); env.step(use());
    check(env.legal(block(1,1)) && env.step(block(1,1)).magic_blocked,
        "MAGIC block permits incompatible Fire armor prefix without attribute matching"); roundtrip(env);
    auto removal = fixture({207}, {195,185}); gifts(removal); removal.step(use());
    const auto removed = removal.step(block(1,1));
    check(removed.magic_blocked && removed.attribute_removed && removal.state().players[1].hp == 40
        && removal.snapshot().draw_index == 3,
        "actual195 prefix is consumed before blocking dark MAGIC without changing its type"); roundtrip(removal);
    auto stages = fixture({216}, {152,196,171,185}); stages.step(use());
    reject(stages,block(3,1)); reject(stages,block(1,1)); reject(stages,block(2,1));
    check(stages.legal(block(0)), "matching blocker cannot be ordinary prefix before another special");
    auto glory = fixture({206}, {122,185}); auto snapshot = glory.snapshot(); snapshot.state.players[1].glory = true;
    check(glory.restore(snapshot), "restore Glory defender"); glory.step(use());
    check(glory.legal(block(1)), "Glory permits lone blocker"); reject(glory,block(1,1));
    auto status = fixture({206}, {175,185}); snapshot = status.snapshot(); snapshot.state.players[0].disease = Disease::cold;
    check(status.restore(snapshot), "restore prefix effect and owner tick"); const std::uint8_t disease_roll[] = {99};
    check(status.script_disease_rolls(disease_roll), "prevent owner worsening"); gifts(status); status.step(use());
    const auto effect = status.step(block(1,1));
    check(effect.magic_blocked && effect.disease_tick && status.state().players[0].hp == 39
        && status.state().players[1].hp == 40 && status.state().players[1].disease == Disease::fever
        && status.snapshot().disease_index == 1 && status.snapshot().draw_index == 3,
        "prefix sickness stays, owner ticks once, all3 gifts arrive after block"); roundtrip(status);
    auto heaven = fixture({216}, {175,185}); snapshot = heaven.snapshot(); snapshot.state.players[1].disease = Disease::heaven;
    check(heaven.restore(snapshot), "restore provisional Heaven before blocked heal"); heaven.step(use());
    check(heaven.step(block(1,1)).magic_blocked && heaven.state().players[1].dead && heaven.state().winner == 0,
        "block cancels heal but preserves prefix Heaven death until item finalization"); roundtrip(heaven);
    auto consumed = fixture({198}, {190,185}); consumed.step(use()); auto response = block(1,1); response.counter_count = 1;
    check(consumed.step(response).magic_blocked && !consumed.state().counter_generated,
        "ordered counter prefix consumed before special creates no queued counter"); roundtrip(consumed);
    for (CardId item : {CardId{5},CardId{57},CardId{227},CardId{231},CardId{235}}) {
        auto nonmagic = fixture({item}, {185}); nonmagic.step(item == 5 || item == 57 ? attack() : use());
        reject(nonmagic,block(0)); check(nonmagic.legal(defend(1)), "non-MAGIC armor role remains available"); roundtrip(nonmagic);
    }
    auto counter_env = fixture({36,182}, {194}); counter_env.step(attack()); auto claim = defend(1); claim.counter_count = 1;
    counter_env.step(claim); reject(counter_env,block(0));
    check(counter_env.step(defend(1)).counter_money_absorptions == 1 && counter_env.state().players[0].money == 10,
        "COUNTER is not MAGIC and ordinary blocker DEF cannot cancel non-damage194 claim"); roundtrip(counter_env);
}
void observation_strictness_and_replay() {
    auto own = fixture({27,54,55,152,165,182,183,185}, {120}); const auto obs = own.observe();
    check(obs[732] == 1.f, "new enabled feature");
    for (int i = 0; i < 8; ++i) check(obs[733+i] == (i+1)/8.f, "own blocker local ordinal features stable");
    auto env = fixture({198}, {185}); env.step(use()); env.step(block(0)); const auto exact = env.snapshot();
    for (int error = 0; error < 5; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.last_reaction.reaction = WeaponReaction::reflect;
        if (error == 1) bad.state.last_reaction.from_ability = true;
        if (error == 2) bad.state.last_reaction.target = 0;
        if (error == 3) bad.config.magic_block = false;
        if (error == 4) bad.version = 18;
        check(!env.restore(bad) && env.snapshot() == exact, "forged blocker event/type/owner/profile/schema rejected atomically");
    }
    auto hidden = fixture({5}, {185}); const auto visible = hidden.observe(); auto changed = hidden.snapshot();
    changed.state.players[1].hand[0] = 120; changed.rng_state ^= 0x9e3779b97f4a7c15ULL;
    check(hidden.restore(changed) && hidden.observe() == visible, "unplayed opposing block artifact and future RNG private");
    auto old_cfg = config; old_cfg.magic_block = false; Env old(old_cfg); const auto oldobs = old.observe();
    check(std::all_of(oldobs.begin()+732,oldobs.end(),[](float x){return x == 0.f;}), "old profiles append18 zeros");
    std::uint64_t blocks = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+107000,pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "magic blocker seeded trajectory replay");
            for (float x : a.observe()) check(std::isfinite(x), "magic blocker observations finite");
            const auto action = a.sample_action(pa), twin = b.sample_action(pb);
            check(action == twin && pa == pb && a.legal(action), "magic block sampler legal and reproducible");
            const auto ra = a.step(action), rb = b.step(twin);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.magic_blocked == rb.magic_blocked
                && ra.weapon_blocked == rb.weapon_blocked, "block metrics replay without weapon misclassification");
            blocks += ra.magic_blocked; roundtrip(a);
        }
    }
    check(blocks, "natural pool trajectories exercise magic blockers");
}
}
int main() {
    metadata_and_ordinary_roles(); forced_magic_block_and_all_payloads(); prefixes_glory_death_and_nonmagic(); observation_strictness_and_replay();
    std::cout << "Magic block passed (mandatory/type/prefix/death/dual roles,1000 replay rollouts).\n";
}
