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
constexpr Config config{512,10,true,true,true,true,true,true,true,true};
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL offensive miracles: " << message << '\n'; std::exit(1); }
}
Action use(int slot = 0, int target = 1) { return {ActionKind::use,static_cast<std::uint8_t>(slot),0,static_cast<std::uint8_t>(target)}; }
Action cast(int index, int target = 1) { return {ActionKind::cast,static_cast<std::uint8_t>(index),0,static_cast<std::uint8_t>(target)}; }
Action defend(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
void hand(Player& p, std::initializer_list<CardId> cards) {
    p.hand.fill(0); std::copy(cards.begin(),cards.end(),p.hand.begin());
    p.hand_count = static_cast<std::uint8_t>(cards.size()); p.pending_deals = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy) {
    Env env(config); auto snapshot = env.snapshot(); snapshot.state = State{};
    hand(snapshot.state.players[0],own); hand(snapshot.state.players[1],enemy);
    check(env.restore(snapshot), "restore explicit offensive miracle fixture"); return env;
}
void roundtrip(const Env& env) {
    Env resumed(env.snapshot().config);
    check(resumed.restore(env.snapshot()) && resumed.snapshot() == env.snapshot(), "exact offensive miracle checkpoint roundtrip");
}
void reject(Env& env, Action action) {
    const auto before = env.snapshot();
    check(!env.legal(action) && !env.step(action).valid && env.snapshot() == before,
        "invalid spell is rejected atomically including counts, MP, tapes and RNG");
}
void ready(Env& env, int mp, bool retained = false, int index = 0) {
    auto snapshot = env.snapshot(); snapshot.state.players[0].mp = mp;
    if (retained) snapshot.state.players[0].abilities[index] = 1;
    check(env.restore(snapshot), "restore exact spell MP and retained count");
    const CardId draw[] = {120}; check(env.script_draws(draw), "install spell replacement outcome");
    const std::uint8_t hit[] = {1}; check(env.script_hit_rolls(hit), "install guaranteed test hit roll");
}
void metadata_and_profile() {
    constexpr std::array<CardId,13> ids{198,199,200,203,204,205,206,207,218,219,222,223,226};
    constexpr std::array<int,13> amounts{4,6,1,8,5,10,10,5,9,13,25,16,30};
    constexpr std::array<int,13> costs{2,3,3,4,4,4,5,5,7,8,12,12,30};
    constexpr std::array<int,13> rates{0,0,25,0,75,25,0,0,75,50,0,75,75};
    constexpr std::array<Element,13> elements{Element::water,Element::wood,Element::light,Element::stone,
        Element::fire,Element::light,Element::fire,Element::dark,Element::water,Element::stone,
        Element::water,Element::fire,Element::water};
    check(offensive_miracle_count == 13 && offensive_miracles().size() == ids.size(), "pinned 13-spell allowlist");
    check(miracle_count == 2 && miracles().size() == 2 && miracle_index(216) == 0 && miracle_index(220) == 1,
        "original two miracle slots and catalog unchanged");
    int probabilistic = 0;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        const auto& m = offensive_miracles()[i];
        check(m.id == ids[i] && m.amount == amounts[i] && m.cost == costs[i] && m.hit_rate == rates[i]
            && m.element == elements[i] && m.weight == 1 && m.harm == (m.id == 200 ? Harm::glory : Harm::none),
            "historical spell IDs, attributes, amounts, costs and rates fixed");
        check(find_offensive_miracle(m.id) == &m && offensive_miracle_index(m.id) == static_cast<int>(i)
            && !find_miracle(m.id) && !find_celestial_card(m.id) && !find_card(m.id), "separate catalog and stable appended index");
        probabilistic += m.hit_rate != 0;
    }
    check(probabilistic == 7, "seven probabilistic offensive miracles");
    for (CardId id : {CardId{0},CardId{197},CardId{201},CardId{202},CardId{208},CardId{216},CardId{220},CardId{65535}})
        check(!find_offensive_miracle(id) && offensive_miracle_index(id) == -1, "unsupported spell effects have no fallback");
    for (int missing = 0; missing < 2; ++missing) {
        auto bad = config;
        if (missing == 0) bad.miracles = false; else bad.celestial = false;
        bool threw = false; try { Env invalid(bad); } catch (const std::invalid_argument&) { threw = true; }
        check(threw, "offensive spells require miracles and celestial semantics explicitly enabled");
    }
    auto env = fixture({198}, {120});
    check(std::string_view(env.profile()) == offensive_miracles_ruleset && env.snapshot().version == 37
        && env.state().players[0].abilities.size() == main_ability_capacity, "new spell profile, schema and total retained kinds");
    roundtrip(env);
    auto disabled_config = config; disabled_config.offensive_miracles = false; Env disabled(disabled_config);
    const auto original = disabled.snapshot();
    check(!disabled.script_draws(ids) && disabled.snapshot() == original, "disabled spell pool rejected atomically");
    auto bad = original; bad.state.players[bad.state.actor].hand[0] = 198;
    check(!disabled.restore(bad) && disabled.snapshot() == original, "disabled spell hand rejected");
    bad = original; bad.state.players[0].abilities[2] = 1;
    check(!disabled.restore(bad) && disabled.snapshot() == original, "disabled appended retained kind rejected");
    bad = env.snapshot(); bad.version = 8; check(!env.restore(bad), "old schema rejected without migration");
}
void first_use_recast_and_affordability() {
    for (const auto& m : offensive_miracles()) {
        const int index = 2 + offensive_miracle_index(m.id);
        auto first = fixture({m.id}, {120}); ready(first,m.cost);
        check(first.options().utility_slots == 1 && first.options().weapon_bases == 0 && first.options().pray,
            "hand spell is affordable utility, not weapon; pray remains permitted");
        reject(first,{ActionKind::attack,0,0,1});
        reject(first,{ActionKind::use,0,1,1});
        const auto start = first.step(use());
        check(start.valid && start.spell_first_use && !start.spell_recast && first.state().players[0].mp == 0
            && first.state().players[0].abilities[index] == 1 && first.state().players[0].hand_count == 0
            && first.state().players[0].pending_deals == 1, "first use pays exact cost, acquires kind and defers one replacement");
        check(first.state().phase == Phase::response && first.state().pending.miracle == m.id
            && first.state().pending.kind == Effect::damage && first.state().pending.amount == m.amount
            && first.state().pending.element == m.element && !first.state().pending.from_ability
            && first.state().pending.harm_card == 0 && first.state().pending.harm == m.harm,
            "spell pending retains exact magic and harm provenance");
        check(first.options().reaction_slots == 0, "physical reactions cannot respond to offensive miracle");
        roundtrip(first); const auto resolved = first.step(defend());
        check(resolved.valid && resolved.dark_execution == (m.element == Element::dark)
            && first.state().players[1].hp == (m.element == Element::dark ? 0 : 40-m.amount)
            && first.state().players[1].glory == (m.harm == Harm::glory)
            && first.snapshot().draw_index == 1 && first.state().players[0].hand_count == 1,
            "spell resolution applies damage/status and distributes one replacement");
        roundtrip(first);

        auto retained = fixture({120}, {120}); ready(retained,m.cost,true,index);
        check(retained.options().ability_slots == (1u << index), "affordable retained kind uses stable appended bit");
        const auto again = retained.step(cast(index));
        check(again.valid && !again.spell_first_use && again.spell_recast
            && retained.state().players[0].mp == 0 && retained.state().players[0].abilities[index] == 1
            && retained.state().players[0].hand_count == 1 && retained.state().pending.from_ability,
            "retained cast pays cost, preserves multiplicity and consumes no hand card");
        check(retained.observe()[302] == 1.f && retained.observe()[303] == m.id/363.f
            && retained.observe()[304] == 1.f, "retained pending origin is public in new observation suffix");
        roundtrip(retained); retained.step(defend());
        check(retained.state().players[0].hand_count == 2 && retained.snapshot().draw_index == 1,
            "retained offensive cast grows hand by one"); roundtrip(retained);

        auto poor = fixture({m.id}, {120}); ready(poor,m.cost-1,true,index);
        check(poor.options().utility_slots == 0 && poor.options().ability_slots == 0,
            "insufficient MP removes first-use and retained candidates"); reject(poor,use()); reject(poor,cast(index));
    }
}
void chance_misses_forcing_and_self() {
    for (const auto& m : offensive_miracles()) {
        const int index = 2+offensive_miracle_index(m.id);
        if (!m.hit_rate) {
            auto self = fixture({m.id}, {120}); ready(self,m.cost);
            const auto result = self.step(use(0,0));
            check(result.valid && self.state().phase == Phase::main && !result.chance_attempt
                && self.state().players[0].hp == (m.element == Element::dark ? 0 : 40-m.amount),
                "nonrandom spell permits immediate self damage and dark self execution"); roundtrip(self);
            continue;
        }
        for (int roll : {m.hit_rate,m.hit_rate+1}) {
            auto env = fixture({m.id}, {120}); ready(env,m.cost);
            const std::uint8_t prefix[] = {static_cast<std::uint8_t>(roll)};
            check(env.script_hit_rolls(prefix), "script exact offensive chance boundary");
            const auto result = env.step(use(0,0)); const bool hit = roll <= m.hit_rate;
            check(result.chance_attempt && result.chance_hit == hit && result.spell_first_use
                && env.state().last_chance.id == m.id && env.state().last_chance.target == 1
                && env.state().players[0].abilities[index] == 1 && env.state().players[0].mp == 0,
                "chance spell normalizes enemy target and retains acquisition/cost on miss");
            check(hit ? env.state().phase == Phase::response
                      : (env.state().phase == Phase::main && env.snapshot().draw_index == 1 && !env.state().players[1].glory),
                "miss skips response and status but still replaces consumed spell"); roundtrip(env);
        }
        auto forced = fixture({m.id}, {120}); ready(forced,m.cost);
        auto snapshot = forced.snapshot(); snapshot.state.players[1].dark_cloud = true;
        check(forced.restore(snapshot), "restore DarkCloud spell recipient");
        const std::uint8_t miss[] = {100}; check(forced.script_hit_rolls(miss), "install preserved forced roll");
        const auto rng = forced.snapshot().rng_state; const auto result = forced.step(use(0,0));
        check(result.chance_forced && result.chance_hit && forced.snapshot().hit_index == 0
            && forced.snapshot().rng_state == rng, "DarkCloud forces magic with no hit tape or RNG use"); roundtrip(forced);
    }
}
void defense_counts_and_capacity() {
    auto armored = fixture({198}, {166,158,39,20}); ready(armored,2); armored.step(use());
    check(armored.legal(defend(1)) && armored.legal(defend(2)) && !armored.options().reaction_slots,
        "water spell admits fire/light armor and excludes physical reflect/bounce");
    reject(armored,{ActionKind::defend,2,0,no_slot});
    const auto blocked = armored.step(defend(1));
    check(!blocked.dark_execution && armored.state().players[1].hp == 40, "ordinary armor fully blocks matching spell");
    auto glory = fixture({198}, {122,128}); ready(glory,2);
    auto snapshot = glory.snapshot(); snapshot.state.players[1].glory = true;
    check(glory.restore(snapshot), "restore Glory spell defender"); glory.step(use());
    check(glory.options().defense_limit == 1 && glory.legal(defend(1)), "Glory limits magic defense to one artifact");
    reject(glory,defend(3));
    auto dark = fixture({207}, {158}); ready(dark,5); dark.step(use());
    check(!dark.step(defend(1)).dark_execution && dark.state().players[1].hp == 40,
        "complete defense prevents dark offensive miracle execution");
    auto both = fixture({207}, {120}); ready(both,5); snapshot = both.snapshot();
    snapshot.state.players[0].hp = 1; snapshot.state.players[0].disease = Disease::cold;
    check(both.restore(snapshot), "restore dying owner of dark offensive miracle");
    const std::uint8_t natural[] = {99}; check(both.script_disease_rolls(natural), "script owner disease tick after magic");
    both.step(use()); const auto draw = both.step(defend());
    check(draw.dark_execution && draw.disease_tick && draw.terminated && both.state().winner == -1
        && draw.rewards == std::array<float,2>{0.f,0.f}, "dark spell death still precedes owner tick and simultaneous learning draw");
    roundtrip(both);
    auto cured = fixture({200}, {235}); ready(cured,3); cured.step(use()); cured.step(defend());
    check(cured.state().players[1].glory && cured.step(use(0,1)).valid && !cured.state().players[1].glory,
        "Glory acquired from magic is removable by lower cure");

    auto heaven = fixture({200}, {120}); ready(heaven,3); snapshot = heaven.snapshot();
    snapshot.state.players[1].disease = Disease::heaven; snapshot.state.players[1].worse_chance = 75;
    check(heaven.restore(snapshot), "restore Heaven recipient of magic Glory"); heaven.step(use());
    const auto fell = heaven.step(defend());
    check(fell.harm_heaven_falls == 1 && heaven.state().players[1].hp == 0
        && !heaven.state().players[1].glory && heaven.state().players[1].worse_chance == 0,
        "magic Glory invokes Heaven fall before new flag attachment"); roundtrip(heaven);

    auto duplicates = fixture({198,198}, {120}); ready(duplicates,4); duplicates.step(use()); duplicates.step(defend());
    snapshot = duplicates.snapshot(); snapshot.state.order = {0,1}; snapshot.state.order_index = 0;
    snapshot.state.actor = snapshot.state.turn_owner = 0;
    check(duplicates.restore(snapshot) && duplicates.step(use()).spell_first_use,
        "second copy is another first-use acquisition rather than retained cast");
    check(duplicates.state().players[0].abilities[2] == 2 && duplicates.state().players[0].mp == 0,
        "natural duplicate spell acquisitions increment count and pay cost independently");
    duplicates.step(defend()); roundtrip(duplicates);

    auto full = fixture({120}, {120}); snapshot = full.snapshot();
    snapshot.state.players[0].hand.fill(120); snapshot.state.players[0].hand_count = 16;
    snapshot.state.players[0].abilities[2] = 2; snapshot.state.players[0].mp = 2;
    check(full.restore(snapshot), "restore duplicate retained spell with full hand");
    const CardId overflow[] = {198}; check(full.script_draws(overflow), "script overflowing new spell draw"); full.step(cast(2));
    check(full.state().players[0].hand_count == 16 && full.state().players[0].pending_deals == 1,
        "full-hand retained offensive response permits 16 plus one deferred gift"); roundtrip(full);
    full.step(defend());
    check(full.state().players[0].hand_count == 16 && full.state().players[0].abilities[2] == 2
        && full.snapshot().draw_index == 1 && std::all_of(full.state().players[0].hand.begin(),full.state().players[0].hand.end(),[](CardId id){return id==120;}),
        "full-hand recast consumes overflow draw without changing duplicates or inventory"); roundtrip(full);
    auto limit = fixture({198}, {120}); snapshot = limit.snapshot(); snapshot.state.players[0].abilities[14] = config.max_decisions;
    check(limit.restore(snapshot), "restore fixture at retained count limit"); reject(limit,use());
    snapshot.state.players[0].abilities[0] = 1;
    check(!limit.restore(snapshot), "overflow sum across old and appended kinds rejected");
}
void restoration_observation_and_sampler() {
    auto env = fixture({198,200}, {120}); ready(env,10,true,2);
    auto snapshot = env.snapshot(); snapshot.state.players[0].abilities[0] = 2; snapshot.state.players[0].abilities[1] = 3;
    check(env.restore(snapshot), "restore old and new retained counts"); const auto obs = env.observe();
    check(obs[129] == 2.f/512 && obs[130] == 3.f/512 && obs[131] == 1.f && obs[301] == 1.f
        && obs[305] == 1.f/512 && obs[318] == 4/30.f && obs[319] == 1/30.f
        && obs[132] == 1.f && obs[148] == 2/99.f, "old observation offsets preserved and appended spell features represented");
    reject(env,cast(15)); reject(env,cast(255)); reject(env,cast(2,2));
    auto malformed = cast(2); malformed.slots = 1; reject(env,malformed);
    for (std::uint64_t seed = 0; seed < 512; ++seed) {
        auto rng = seed; const auto action = env.sample_action(rng); check(env.legal(action), "spell-aware main sampler legal");
        if ((action.kind == ActionKind::use && action.base < 2) || (action.kind == ActionKind::cast && action.base >= 2))
            check(action.target == 1, "sampler aims offensive first use and retained cast at enemy");
        if (action.kind == ActionKind::cast && action.base < 2) check(action.target == 0, "old healing/money casts still target self");
    }
    env.step(use()); const auto pending = env.snapshot(); const auto visible = env.observe();
    check(visible[302] == 1.f && visible[303] == 198/363.f && visible[304] == 0.f,
        "public pending spell identity and first-use origin represented");
    for (int kind = 0; kind < 4; ++kind) {
        auto bad = pending;
        if (kind == 0) ++bad.state.pending.amount;
        if (kind == 1) bad.state.pending.element = Element::fire;
        if (kind == 2) bad.state.pending.miracle = 197;
        if (kind == 3) bad.state.players[0].abilities[2] = 0;
        check(!env.restore(bad) && env.snapshot() == pending, "invalid spell amount, attribute, ID or ownership rejected atomically");
    }
    auto hidden = pending; hidden.state.players[0].abilities[14] = 7; hidden.scripted_hits[0] = 99;
    hidden.rng_state ^= 0x9e3779b97f4a7c15ULL;
    Env other(config); check(other.restore(hidden) && other.observe() == visible && other.options() == env.options(),
        "enemy retained spells, future hit tape and RNG remain hidden to defender");
    auto disabled_config = config; disabled_config.offensive_miracles = false; Env disabled(disabled_config);
    const auto old = disabled.observe();
    check(std::all_of(old.begin()+301,old.end(),[](float x){return x==0.f;}), "old profile new spell suffix all zero");
}
void replay() {
    std::uint64_t first = 0, recasts = 0, chance = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env a(config), b(config); a.reset(seed); b.reset(seed); std::uint64_t pa = seed+25000, pb = pa;
        while (!a.done()) {
            check(a.snapshot() == b.snapshot() && a.observe() == b.observe(), "offensive spell trajectory replay");
            for (float x : a.observe()) check(std::isfinite(x), "spell observations finite");
            const auto action = a.sample_action(pa), copy = b.sample_action(pb);
            check(action == copy && pa == pb && a.legal(action), "spell hierarchical policy deterministic and legal");
            const auto ra = a.step(action), rb = b.step(copy);
            check(ra.valid && rb.valid && ra.rewards == rb.rewards && ra.spell_first_use == rb.spell_first_use
                && ra.spell_recast == rb.spell_recast && ra.chance_hit == rb.chance_hit
                && ra.dark_execution == rb.dark_execution, "spell transition and metrics reproducible");
            first += ra.spell_first_use; recasts += ra.spell_recast; chance += ra.chance_attempt; roundtrip(a);
        }
    }
    check(first && recasts && chance, "natural seeded games exercise spell acquisition, recast and chance");
}
}
int main() {
    metadata_and_profile(); first_use_recast_and_affordability(); chance_misses_forcing_and_self();
    defense_counts_and_capacity(); restoration_observation_and_sampler(); replay();
    std::cout << "Offensive miracle checks passed (13 spells, MP/retention, defense/chance/status, 1000 replay rollouts).\n";
}
