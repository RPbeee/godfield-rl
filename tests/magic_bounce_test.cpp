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
    c.celestial = c.offensive_miracles = c.defensive_miracles = c.attribute_change = c.attribute_removal = c.counters = c.counter_extensions = c.money_counter = c.magic_reflection = c.any_reflection = c.mana_weapon = c.magic_block = c.additive_miracles = c.doubling_miracles = c.wide_miracles = c.magic_free = c.forgetting = c.exchanges = c.revival = c.item_removal = c.hp_absorption = c.self_damage = c.pestle_mortar = c.selling = c.buying = c.hp_gamble = c.magic_bounce = true;
    return c;
}();
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL magic bounce: " << message << '\n'; std::exit(1); } }
Action use() { return {ActionKind::use,0,0,1}; }
Action accept(std::uint16_t slots = 0) { return {ActionKind::defend,no_slot,slots,no_slot}; }
Action bounce(int slot, std::uint16_t prefix = 0) { return {ActionKind::defend,static_cast<std::uint8_t>(slot),prefix,no_slot}; }
Action ordered(std::initializer_list<std::uint8_t> refs) {
    Action a{ActionKind::defend_ordered,no_slot,0,no_slot}; a.extra_count = static_cast<std::uint8_t>(refs.size()); std::copy(refs.begin(),refs.end(),a.extras.begin()); return a;
}
void hand(Player& p, std::initializer_list<CardId> ids) {
    p.hand.fill(0); std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count = static_cast<std::uint8_t>(ids.size()); p.pending_deals = p.retained_deals = 0; p.incoming_sale = p.incoming_buy = 0;
}
Env fixture(std::initializer_list<CardId> own, std::initializer_list<CardId> enemy, Config cfg = config) {
    Env env(cfg); auto s = env.snapshot(); s.state = State{}; hand(s.state.players[0],own); hand(s.state.players[1],enemy);
    check(env.restore(s), "restore explicit physical magic-bounce fixture"); return env;
}
void roundtrip(const Env& env) { Env copy(env.snapshot().config); check(copy.restore(env.snapshot()) && copy.snapshot() == env.snapshot(), "physical magic bounce/history/retained budgets roundtrip exactly"); }
void reject(Env& env, Action a) { const auto before = env.snapshot(); check(!env.legal(a) && !env.step(a).valid && env.snapshot() == before, "illegal physical stage/prefix/type rejects atomically"); }
void draws(Env& env) { std::array<CardId,64> ids{}; ids.fill(120); check(env.script_draws(ids), "isolate deferred reaction gifts"); }
void target(Env& env, std::uint8_t seat) { const std::uint8_t seats[] = {seat}; check(env.script_bounce_targets(seats), "script bounce destination"); }

void metadata_all_six_and_dual_roles() {
    constexpr std::array<CardId,6> ids{21,119,121,127,134,140};
    check(magic_bounce_card_count == 6 && magic_bounce_catalog().size() == 6, "six physical FLICK_MAGIC cards only");
    for (int i = 0; i < 6; ++i) {
        const auto* item = find_magic_bounce_card(ids[i]);
        check(item && magic_bounce_index(ids[i]) == i && item->card.kind == (i ? Kind::armor : Kind::weapon)
            && item->card.attack == (i ? 0 : 5) && item->card.boost == (i ? 0 : 5) && item->card.defense == (i ? i-1 : 0)
            && !item->card.hp && !item->card.mp && item->card.price == (i ? 5 : 10) && item->card.weight == 1
            && item->card.discardable == bool(i), "normal weapon/boost and ordinary armor0..4 roles match catalog");
        auto env = fixture({198},{ids[i]}); auto s = env.snapshot(); s.state.players[1].mp = 0; check(env.restore(s), "restore0MP physical bounce"); draws(env); target(env,0); env.step(use());
        check(env.options().reaction_slots == 1 && env.options().magic_bounce_slots == 1 && !(env.options().defense_slots & 1),
            "matching MAGIC forces special base, never ordinary armor opt-out"); reject(env,accept(1));
        const auto r = env.step(bounce(0)); check(r.magic_card_bounced && r.magic_bounce_card == ids[i] && r.bounced && r.magic_bounced && !r.bounce_self
            && !r.reflected && !r.magic_reflected && !r.spell_first_use && !r.spell_recast && !r.defensive_first_use && !r.defensive_recast
            && env.state().players[1].mp == 0 && !env.state().players[1].defensive_abilities[0] && env.state().pending.amount == 4
            && env.state().pending.miracle == 198 && env.state().pending.source == 1 && env.state().pending.target == 0 && env.state().turn_owner == 0,
            "physical bounce consumes one card, returns original magic, and costs no MP or retention acquisition"); roundtrip(env);
        check(env.step(accept()).valid && env.state().players[0].hp == 36, "physical card DEF never reduces redirected magic"); roundtrip(env);
        auto ordinary = fixture({36},{ids[i]}); ordinary.step({ActionKind::attack,0,0,1});
        check(!ordinary.options().reaction_slots && ordinary.options().magic_bounce_slots == 1, "physical membership is distinct from matching reaction legality");
        if (i) {
            check(ordinary.step(accept(1)).valid && ordinary.state().players[1].hp == 30+i-1,
                "non-MAGIC uses ordinary armor DEF0..4, including legal zero defense"); roundtrip(ordinary);
        } else reject(ordinary,accept(1));
    }
    auto pair = fixture({21,21},{120}); pair.step({ActionKind::attack,0,2,1});
    check(pair.state().pending.amount == 10 && !pair.state().pending.miracle, "base21 counts ATK5 once and second21 contributes boost5 only"); roundtrip(pair);
    auto extra = fixture({5,21},{119}); extra.step({ActionKind::attack,0,2,1}); check(extra.state().pending.amount == 6 && !extra.state().pending.miracle,
        "additional21 does not confer MAGIC classification or reaction payload to original weapon");
    check(extra.step(accept(1)).valid && extra.state().players[1].hp == 34, "119 is plainDEF0 against weapon with21 boost"); roundtrip(extra);
    for (CardId id : {CardId{0},CardId{20},CardId{39},CardId{40},CardId{210},CardId{196},CardId{65535}})
        check(!find_magic_bounce_card(id), "old weapon bounce/reflect/ANY and held magic210 stay separate");
    auto profile = fixture({119},{120}); check(std::string_view(profile.profile()) == magic_bounce_ruleset && profile.snapshot().version == 36,
        "new profile and checkpoint schema explicit");
    for (int missing = 0; missing < 2; ++missing) {
        Config bad; bad.magic_bounce = true; if (!missing) bad.miracles = true; else bad.reflection = true;
        bool threw = false; try { Env invalid(bad); } catch (const std::invalid_argument&) { threw = true; }
        check(threw, "physical magic bounce explicitly requires miracles and reflection");
    }
    auto old_cfg = config; old_cfg.magic_bounce = false; Env old(old_cfg); const auto before = old.snapshot();
    check(!old.script_draws(ids) && old.snapshot() == before, "old33 profile pool rejects six new IDs");
}

void prefixes_forcing_glory_and_unused_free() {
    auto prefix = fixture({206},{122,119}); draws(prefix); target(prefix,0); prefix.step(use());
    check(prefix.legal(bounce(1,1)) && !prefix.legal(accept(1)), "incompatible Fire armor allowed before MAGIC stage but not ordinary defense");
    const auto r = prefix.step(bounce(1,1)); check(r.magic_card_bounced && prefix.state().pending.amount == 10
        && prefix.state().players[1].pending_deals == 2 && prefix.state().players[1].hand_count == 0,
        "prefix consumed before bounce, original raw retained"); roundtrip(prefix); prefix.step(accept()); check(prefix.state().players[0].hp == 30, "prefix never subtracts redirected raw");
    auto light = fixture({221},{120,119}); target(light,0); light.step(use()); check(light.step(bounce(1,1)).magic_card_bounced,
        "Light MAGIC also permits ordinary incompatible NE prefix before stage"); roundtrip(light);
    auto staged = fixture({216},{119,121}); target(staged,0); staged.step(use()); reject(staged,bounce(1,1)); reject(staged,accept(1));
    check(staged.step(ordered({0})).magic_card_bounced && staged.state().players[1].hand[0] == 121,
        "first matching physical bounce fires; other staged card cannot be opted out as ordinary prefix"); roundtrip(staged);
    auto unused = fixture({216},{119,239}); draws(unused); target(unused,0); unused.step(use()); const auto suffix = unused.step(ordered({0,1}));
    check(suffix.magic_card_bounced && suffix.magic_free_suffix && !suffix.magic_free_cards && !suffix.magic_free_mp_saved
        && unused.state().players[1].hand[0] == 239 && !unused.state().last_reaction.free_card && !unused.state().last_reaction.from_ability,
        "physical bounce leaves FREE suffix unconsumed and never becomes a free held spell"); roundtrip(unused);
    auto glory = fixture({198},{120,119,239}); auto s = glory.snapshot(); s.state.players[1].glory = true;
    check(glory.restore(s), "restore Glory physical responder"); target(glory,0); glory.step(use()); reject(glory,bounce(1,1)); reject(glory,ordered({1,2}));
    check(glory.step(bounce(1)).magic_card_bounced, "Glory permits one physical card but counts prefix and unused suffix"); roundtrip(glory);
    auto counter = fixture({198},{190,119}); target(counter,0); counter.step(use()); auto selected = bounce(1,1); selected.counter_count = 1;
    const auto consumed = counter.step(selected); check(consumed.magic_card_bounced && !consumed.counter_queued
        && counter.state().players[1].pending_deals == 2, "counter prefix before physical bounce consumes but never reserves a counter"); roundtrip(counter);
}

void all_magic_payloads_chance_and_attribute_removal() {
    for (CardId id : {CardId{216},CardId{220},CardId{200},CardId{207},CardId{221}}) for (std::uint8_t landing = 0; landing < 2; ++landing) {
        auto env = fixture({id},{119}); auto s = env.snapshot(); s.state.players[0].hp = s.state.players[1].hp = 30;
        check(env.restore(s), "restore physical bounce for heal/money/chance/dark/absorb"); draws(env); target(env,landing);
        const std::uint8_t hit[] = {1,100}; check(env.script_hit_rolls(hit), "script original hit then unused next roll"); env.step(use()); const auto original = env.snapshot();
        const auto r = env.step(bounce(0)); check(r.magic_card_bounced && r.bounce_self == bool(landing)
            && env.snapshot().hit_index == original.hit_index && env.state().last_chance == original.state.last_chance
            && env.state().players[0].mp == original.state.players[0].mp && env.state().players[0].abilities == original.state.players[0].abilities,
            "new physical bounce never repays/reacquires original magic or rerolls chance");
        if (!landing) { roundtrip(env); env.step(accept()); }
        const auto& recipient = env.state().players[landing];
        if (id == 216) check(recipient.hp == 40, "heal reaches actual bounce target in full");
        if (id == 220) check(recipient.money == 35, "money payload uses actual bounce target");
        if (id == 200) check(recipient.glory && recipient.hp == 29, "chance Glory payload remains intact");
        if (id == 207) check(recipient.hp == 0 && recipient.dead, "Dark execution remains typed MAGIC after bounce");
        if (id == 221) check(recipient.hp == (landing ? 30 : 20) && env.state().players[1].hp == (landing ? 30 : 40),
            "absorption uses current source, with self heal-before-damage when target is source");
        roundtrip(env);
    }
    auto forced = fixture({200},{119}); auto s = forced.snapshot(); s.state.players[1].dark_cloud = true;
    check(forced.restore(s), "restore original forced MAGIC hit"); const std::uint8_t miss[] = {100}; check(forced.script_hit_rolls(miss), "forced hit leaves next roll untouched");
    target(forced,0); forced.step(use()); forced.step(bounce(0)); check(forced.snapshot().hit_index == 0 && forced.state().last_chance.forced,
        "DarkCloud applies only to original normalized enemy hit, not bounce destination"); roundtrip(forced);
    auto removed = fixture({207},{195,119}); draws(removed); target(removed,1); removed.step(use()); const auto self = removed.step(bounce(1,1));
    check(self.magic_card_bounced && self.attribute_removed && self.bounce_self && !self.dark_execution && removed.state().players[1].hp == 35,
        "195 alters original Dark MAGIC before self-bounce and preserves ordinary raw5"); roundtrip(removed);
}

void heaven_provisional_and_mixed_reactions() {
    auto self = fixture({216},{175,119,244}); auto s = self.snapshot(); s.state.players[1].disease = Disease::heaven;
    check(self.restore(s), "restore Heaven prefix into physical self-heal"); draws(self); target(self,1); self.step(use()); const auto rescued = self.step(bounce(1,1));
    check(rescued.magic_card_bounced && rescued.bounce_self && !rescued.revivals[1] && self.state().players[1].hp == 10
        && !self.state().players[1].dead && self.state().players[1].hand[0] == 244,
        "physical self-bounce heal rescues provisionalHP0 before automatic244"); roundtrip(self);
    auto chain = fixture({216,196},{175,119}); s = chain.snapshot(); s.state.players[1].disease = Disease::heaven;
    check(chain.restore(s), "restore physical bounce followed by196 return to provisional target"); draws(chain); target(chain,0); chain.step(use()); chain.step(bounce(1,1));
    check(chain.state().players[1].hp == 0 && !chain.state().players[1].dead && chain.state().pending.source == 1,
        "other landing keeps Heaven source provisional during unresolved chain"); roundtrip(chain);
    check(chain.step(bounce(0)).any_reflected, "196 can return incoming MAGIC after physical bounce"); roundtrip(chain); chain.step(accept());
    check(chain.state().players[1].hp == 10 && !chain.state().players[1].dead, "returned healing reaches provisional target before death hook"); roundtrip(chain);
    auto mixed = fixture({119},{119}); s = mixed.snapshot(); for (auto& p : s.state.players) { p.mp = 99; p.defensive_abilities[0] = 1; }
    s.state.players[0].abilities[2] = 1; check(mixed.restore(s), "restore physical plus held210 chain"); draws(mixed);
    std::array<std::uint8_t,40> others{}; for (int i = 0; i < 40; ++i) others[i] = static_cast<std::uint8_t>(i%2);
    check(mixed.script_bounce_targets(others), "force all mixed bounces to other seat"); mixed.step({ActionKind::cast,2,0,1});
    for (int i = 0; i < 2; ++i) { check(mixed.step(bounce(0)).magic_card_bounced, "first two physical cards consumed without MP"); roundtrip(mixed); }
    for (int i = 0; i < 38; ++i) { const auto r = mixed.step({ActionKind::defend_cast,0,0,no_slot}); check(r.magic_bounced && !r.magic_card_bounced, "held210 remains separately accounted"); roundtrip(mixed); }
    check(mixed.state().reaction_count == 40 && mixed.state().players[0].retained_deals == 20 && mixed.state().players[0].pending_deals == 21
        && mixed.state().players[1].pending_deals == 20 && mixed.state().players[0].mp == 2 && mixed.state().players[1].mp == 4,
        "physical and held response budgets combine without artificial32-chain cap"); mixed.step(accept());
    check(mixed.snapshot().draw_index == 41, "all mixed gifts paid once after final item"); roundtrip(mixed);
}

void minimum_fullhand32_and_counter_nonmagic() {
    Config narrow; narrow.miracles = narrow.reflection = narrow.magic_bounce = true;
    auto env = fixture({119},{119},narrow); auto s = env.snapshot(); for (auto& p : s.state.players) { p.hand.fill(119); p.hand_count = 16; }
    s.state.players[0].abilities[0] = 1; s.state.players[0].mp = 99; check(env.restore(s), "restore minimal both-full inventories and held heal"); draws(env);
    std::array<std::uint8_t,32> destinations{}; for (int i = 0; i < 32; ++i) destinations[i] = static_cast<std::uint8_t>(i%2);
    check(env.script_bounce_targets(destinations), "force32 physical bounces"); env.step({ActionKind::cast,0,0,1}); roundtrip(env);
    for (int i = 0; i < 32; ++i) { check(env.step(bounce(0)).magic_card_bounced && env.snapshot().draw_index == 0, "physical inventory-limited chain defers gifts"); roundtrip(env); }
    check(env.state().reaction_count == 32 && env.state().players[0].pending_deals == 17 && env.state().players[0].retained_deals == 1
        && env.state().players[1].pending_deals == 16 && env.state().players[0].mp == 93 && env.state().players[1].mp == 10,
        "minimum flag tracks original full-hand held gift17 while every physical response costs0MP"); env.step(accept());
    check(env.snapshot().draw_index == 33 && env.state().players[0].hand_count == 16 && env.state().players[1].hand_count == 16
        && !env.state().players[0].retained_deals, "all33 draws including overflow consumed and retained debt clears"); roundtrip(env);
    auto counter = fixture({56,195,119,244},{190}); counter.step({ActionKind::attack,0,0,1}); auto selected = accept(1); selected.counter_count = 1;
    counter.step(selected); reject(counter,bounce(1,1)); check(counter.step(accept(3)).valid && !counter.state().last_reaction.id
        && counter.state().players[0].hp == 10, "COUNTER stays non-MAGIC:195 permits ordinaryDEF0 but physical magic bounce never fires"); roundtrip(counter);
    auto none = fixture({237},{119}); const std::uint8_t negative[] = {0}; check(none.script_hp_gamble_rolls(negative), "prepare None gamble"); none.step(use()); reject(none,bounce(0)); reject(none,accept(1));
    auto trade = fixture({4},{119}); trade.step({ActionKind::buy,0,0,1}); reject(trade,bounce(0));
}

void market_privacy_strictness_and_replay() {
    for (const auto& card : magic_bounce_catalog()) {
        const auto id = card.card.id; check(sale_price(id) == -1, "old216 static price table remains frozen");
        auto sale = fixture({3,id},{120}); draws(sale); target(sale,0); sale.step({ActionKind::sell,0,2,1}); const auto sold = sale.step(accept());
        check(sold.sale_price == card.card.price && sold.sale_delivered && !sold.magic_card_bounced && !sale.snapshot().target_index,
            "physical magic-bounce commodity sells at new price without reacting"); roundtrip(sale);
        auto buy = fixture({4},{id}); draws(buy); target(buy,0); const std::uint64_t ticket[] = {0}; check(buy.script_buy_tickets(ticket), "select new commodity");
        buy.step({ActionKind::buy,0,0,1}); check(buy.step(accept()).buy_price == card.card.price, "new commodity BUY price fallback");
        check(buy.options().magic_bounce_slots == 0 && buy.step({ActionKind::buy_accept}).buy_delivered && !buy.snapshot().target_index,
            "purchase phase offers no physical-reaction membership or side effects"); roundtrip(buy);
    }
    auto a = fixture({216},{119}), b = fixture({216},{119}); target(a,0); target(b,1); a.step(use()); b.step(use());
    check(a.observe() == b.observe() && a.options() == b.options(), "future bounce destination tape remains private");
    a.step(bounce(0)); const auto exact = a.snapshot(); check(a.observe()[1295] == 1.f && a.observe()[1296] == 119.f/363.f,
        "new last-physical-bounce features expose actual public reaction ID");
    for (int error = 0; error < 6; ++error) {
        auto bad = exact;
        if (error == 0) bad.state.last_reaction.reaction = WeaponReaction::reflect;
        if (error == 1) bad.state.last_reaction.from_ability = true;
        if (error == 2) bad.state.last_reaction.free_card = 239;
        if (error == 3) bad.state.pending.miracle = 0;
        if (error == 4) bad.state.players[0].abilities[0] = 0;
        if (error == 5) bad.state.last_reaction.id = 20;
        check(!a.restore(bad) && a.snapshot() == exact, "wrong category/held origin/free marker/MAGIC provenance rejects atomically");
    }
    auto hidden = fixture({5},{119}); auto swapped = hidden.snapshot(); swapped.state.players[1].hand[0] = 140; const auto visible = hidden.observe();
    check(hidden.restore(swapped) && hidden.observe() == visible, "opponent physical bounce identity private");
    auto old_cfg = config; old_cfg.magic_bounce = false; auto old = fixture({120},{120},old_cfg), current = fixture({120},{120});
    const auto old_obs = old.observe(), new_obs = current.observe(); check(std::equal(old_obs.begin(),old_obs.begin()+1278,new_obs.begin())
        && std::all_of(old_obs.begin()+1278,old_obs.end(),[](float x){return x == 0.f;}), "old33 prefix1278 fixed and new22 features zero");
    std::uint64_t used = 0;
    for (std::uint64_t seed = 0; seed < 1000; ++seed) {
        Env x(config), y(config); x.reset(seed); y.reset(seed); std::uint64_t px = seed+340000, py = px;
        while (!x.done()) {
            check(x.snapshot() == y.snapshot() && x.observe() == y.observe(), "physical magic bounce seeded state/observation replay");
            for (float value : x.observe()) check(std::isfinite(value), "new bounce observations finite");
            const auto ax = x.sample_action(px), ay = y.sample_action(py); check(ax == ay && px == py && x.legal(ax), "staged physical bounce sampler deterministic/legal");
            const auto rx = x.step(ax), ry = y.step(ay); check(rx.valid && ry.valid && rx.rewards == ry.rewards && rx.magic_card_bounced == ry.magic_card_bounced
                && rx.magic_bounce_card == ry.magic_bounce_card && rx.bounce_self == ry.bounce_self, "physical stage identity and destination replay");
            used += rx.magic_card_bounced; roundtrip(x);
        }
    }
    check(used, "natural pool and sampler actually use physical magic bounce");
}
}
int main() {
    metadata_all_six_and_dual_roles(); prefixes_forcing_glory_and_unused_free(); all_magic_payloads_chance_and_attribute_removal();
    heaven_provisional_and_mixed_reactions(); minimum_fullhand32_and_counter_nonmagic(); market_privacy_strictness_and_replay();
    std::cout << "Physical magic bounce passed (six roles/stages/payloads/Heaven/fullhand32/mixed40/market/privacy,1000 replay rollouts).\n";
}
