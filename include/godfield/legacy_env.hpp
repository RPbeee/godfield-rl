#pragma once
#include "godfield/catalog.hpp"
#include "godfield/miracles.hpp"
#include "godfield/elements.hpp"
#include "godfield/chance.hpp"
#include "godfield/reflection.hpp"
#include "godfield/disease.hpp"
#include "godfield/harms.hpp"
#include "godfield/celestial.hpp"
#include "godfield/offensive_miracles.hpp"
#include "godfield/defensive_miracles.hpp"
#include "godfield/attribute_change.hpp"
#include "godfield/attribute_removal.hpp"
#include "godfield/counter.hpp"
#include "godfield/counter_extensions.hpp"
#include "godfield/money_counter.hpp"
#include "godfield/magic_reflection.hpp"
#include "godfield/any_reflection.hpp"
#include "godfield/mana_weapon.hpp"
#include "godfield/magic_block.hpp"
#include "godfield/additive_miracles.hpp"
#include "godfield/doubling_miracles.hpp"
#include "godfield/wide_miracles.hpp"
#include "godfield/magic_free.hpp"
#include "godfield/forgetting.hpp"
#include "godfield/exchange.hpp"
#include "godfield/revival.hpp"
#include "godfield/item_removal.hpp"
#include "godfield/hp_absorption.hpp"
#include "godfield/self_damage.hpp"
#include "godfield/pestle_mortar.hpp"
#include "godfield/selling.hpp"
#include "godfield/buying.hpp"
#include "godfield/hp_gamble.hpp"
#include "godfield/magic_bounce.hpp"
#include "godfield/sundry_boost.hpp"
#include "godfield/status_miracles.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace godfield::legacy {
inline constexpr const char* ruleset = "legacy-neutral-subset-v1";
inline constexpr const char* miracles_ruleset = "legacy-neutral-miracles-subset-v2";
inline constexpr const char* elements_ruleset = "legacy-elements-subset-v3";
inline constexpr const char* elements_miracles_ruleset = "legacy-elements-miracles-subset-v3";
inline constexpr const char* chance_ruleset = "legacy-elements-chance-subset-v4";
inline constexpr const char* chance_miracles_ruleset = "legacy-elements-miracles-chance-subset-v4";
inline constexpr const char* reflection_ruleset = "legacy-weapon-reactions-subset-v5";
inline constexpr const char* disease_ruleset = "legacy-disease-subset-v6";
inline constexpr const char* harms_ruleset = "legacy-harms-subset-v7";
inline constexpr const char* celestial_ruleset = "legacy-celestial-subset-v8";
inline constexpr const char* offensive_miracles_ruleset = "legacy-offensive-miracles-subset-v9";
inline constexpr const char* defensive_miracles_ruleset = "legacy-defensive-miracles-subset-v10";
inline constexpr const char* attribute_change_ruleset = "legacy-attribute-change-subset-v11";
inline constexpr const char* attribute_removal_ruleset = "legacy-attribute-removal-subset-v12";
inline constexpr const char* counter_ruleset = "legacy-counter-subset-v13";
inline constexpr const char* counter_extensions_ruleset = "legacy-counter-extensions-subset-v14";
inline constexpr const char* money_counter_ruleset = "legacy-money-counter-subset-v15";
inline constexpr const char* magic_reflection_ruleset = "legacy-magic-reflection-subset-v16";
inline constexpr const char* any_reflection_ruleset = "legacy-any-reflection-subset-v17";
inline constexpr const char* mana_weapon_ruleset = "legacy-mana-weapon-subset-v18";
inline constexpr const char* magic_block_ruleset = "legacy-magic-block-subset-v19";
inline constexpr const char* additive_miracles_ruleset = "legacy-additive-miracles-subset-v20";
inline constexpr const char* doubling_miracles_ruleset = "legacy-doubling-miracles-subset-v21";
inline constexpr const char* wide_miracles_ruleset = "legacy-wide-miracles-subset-v22";
inline constexpr const char* magic_free_ruleset = "legacy-magic-free-subset-v23";
inline constexpr const char* forgetting_ruleset = "legacy-forgetting-subset-v24";
inline constexpr const char* exchange_ruleset = "legacy-exchange-subset-v25";
inline constexpr const char* revival_ruleset = "legacy-revival-subset-v26";
inline constexpr const char* item_removal_ruleset = "legacy-item-removal-subset-v27";
inline constexpr const char* hp_absorption_ruleset = "legacy-hp-absorption-subset-v28";
inline constexpr const char* self_damage_ruleset = "legacy-self-damage-subset-v29";
inline constexpr const char* pestle_mortar_ruleset = "legacy-pestle-mortar-subset-v30";
inline constexpr const char* selling_ruleset = "legacy-selling-subset-v31";
inline constexpr const char* buying_ruleset = "legacy-buying-subset-v32";
inline constexpr const char* hp_gamble_ruleset = "legacy-hp-gamble-subset-v33";
inline constexpr const char* magic_bounce_ruleset = "legacy-magic-bounce-subset-v34";
inline constexpr const char* sundry_boost_ruleset = "legacy-sundry-boost-subset-v35";
inline constexpr const char* status_miracles_ruleset = "legacy-status-miracles-subset-v36";
// Main-turn slots remain stable. Defensive kinds have their own local indices.
inline constexpr int total_miracle_count = miracle_count + offensive_miracle_count;
inline constexpr int total_retained_miracle_count = total_miracle_count + defensive_miracle_count;
inline constexpr int hp_absorption_ability_index = total_miracle_count;
inline constexpr int main_ability_capacity = total_miracle_count + 1;
static_assert(main_ability_capacity <= 16);
inline constexpr int hand_capacity = 16;
inline constexpr int counter_queue_capacity = 2 * hand_capacity;
inline constexpr std::int64_t counter_damage_limit = std::int64_t{480} << counter_queue_capacity;
inline constexpr std::int64_t doubling_damage_limit = std::int64_t{480} << 16;
inline constexpr std::int64_t doubling_counter_damage_limit = doubling_damage_limit << counter_queue_capacity;
inline constexpr std::int64_t magic_free_damage_limit = std::int64_t{480} << 17;
inline constexpr std::int64_t magic_free_counter_damage_limit = magic_free_damage_limit << counter_queue_capacity;
inline constexpr int base_observation_size = 128;
inline constexpr int miracles_observation_size = 164;
inline constexpr int elements_observation_size = 186;
inline constexpr int chance_observation_size = 210;
inline constexpr int reflection_observation_size = 236;
inline constexpr int disease_observation_size = 258;
inline constexpr int harms_observation_size = 283;
inline constexpr int celestial_observation_size = 301;
inline constexpr int offensive_miracles_observation_size = 334;
inline constexpr int defensive_miracles_observation_size = 356;
inline constexpr int attribute_change_observation_size = 375;
inline constexpr int attribute_removal_observation_size = 393;
inline constexpr int counter_observation_size = 515;
inline constexpr int counter_extensions_observation_size = 661;
inline constexpr int money_counter_observation_size = 679;
inline constexpr int magic_reflection_observation_size = 696;
inline constexpr int any_reflection_observation_size = 713;
inline constexpr int mana_weapon_observation_size = 732;
inline constexpr int magic_block_observation_size = 750;
inline constexpr int additive_miracles_observation_size = 903;
inline constexpr int doubling_miracles_observation_size = 924;
inline constexpr int wide_miracles_observation_size = 945;
inline constexpr int magic_free_observation_size = 993;
inline constexpr int forgetting_observation_size = 1017;
inline constexpr int exchange_observation_size = 1043;
inline constexpr int revival_observation_size = 1062;
inline constexpr int item_removal_observation_size = 1085;
inline constexpr int hp_absorption_observation_size = 1110;
inline constexpr int self_damage_observation_size = 1135;
inline constexpr int pestle_mortar_observation_size = 1175;
inline constexpr int selling_observation_size = 1221;
inline constexpr int buying_observation_size = 1251;
inline constexpr int hp_gamble_observation_size = 1278;
inline constexpr int magic_bounce_observation_size = 1300;
inline constexpr int sundry_boost_observation_size = 1317;
inline constexpr int observation_size = 1345;
inline constexpr int legacy_attack_extra_capacity = 64;
inline constexpr int attack_extra_capacity = 65;
inline constexpr std::uint8_t no_slot = 255;
enum class ActionKind : std::uint8_t {
    pray, attack, defend, use, discard, cast, defend_cast,
    attack_set_fire, attack_set_water, attack_set_neutral, cast_additive, attack_ordered, cast_ordered, defend_ordered, exchange, sell, buy, buy_decline, buy_accept, cast_status
};
enum class Phase : std::uint8_t { main, response, purchase };
enum class Effect : std::uint8_t { none, damage, heal, mana, money, cure, harm, absorb_money, forget, remove_item, sell, buy, hp_gamble, status };
struct Config {
    std::uint32_t max_decisions = 512;
    std::uint8_t initial_hand = 10;
    bool miracles = false;
    bool elements = false;
    bool chance = false;
    bool reflection = false;
    bool disease = false;
    bool harms = false;
    bool celestial = false;
    bool offensive_miracles = false;
    bool defensive_miracles = false;
    bool attribute_change = false;
    bool attribute_removal = false;
    bool counters = false;
    bool counter_extensions = false;
    bool money_counter = false;
    bool magic_reflection = false;
    bool any_reflection = false;
    bool mana_weapon = false;
    bool magic_block = false;
    bool additive_miracles = false;
    bool doubling_miracles = false;
    bool wide_miracles = false;
    bool magic_free = false;
    bool forgetting = false;
    bool exchanges = false;
    bool revival = false;
    bool item_removal = false;
    bool hp_absorption = false;
    bool self_damage = false;
    bool pestle_mortar = false;
    bool selling = false;
    bool buying = false;
    bool hp_gamble = false;
    bool magic_bounce = false;
    bool sundry_boost = false;
    bool status_miracles = false;
    bool operator==(const Config&) const = default;
};
struct Action {
    ActionKind kind = ActionKind::pray;
    std::uint8_t base = no_slot; // defend: hand reaction slot; defend_cast: defensive kind index.
    std::uint16_t slots = 0; // Extra attack slots, or defense/discard slots.
    std::uint8_t target = no_slot;
    std::uint8_t counter_count = 0;
    std::uint64_t counter_order = 0; // Hand slots in low-to-high 4-bit nibbles.
    std::uint8_t extra_count = 0;
    std::array<std::uint8_t, attack_extra_capacity> extras{}; // 0..15 hand,16/17 held additive kind,18 held doubling,19 held wide.
    bool operator==(const Action&) const = default;
};
struct Player {
    int hp = 40, mp = 10, money = 20;
    std::array<CardId, hand_capacity> hand{};
    std::uint8_t hand_count = 0;
    std::uint64_t pending_deals = 0;
    std::array<std::uint32_t, main_ability_capacity> abilities{};
    std::array<std::uint32_t, defensive_miracle_count> defensive_abilities{};
    std::array<std::uint32_t, status_miracle_count> status_abilities{};
    std::array<std::uint64_t, additive_miracle_count> additive_abilities{};
    std::uint64_t doubling_ability = 0;
    std::uint64_t wide_ability = 0;
    std::uint64_t retained_deals = 0; // Pending gifts from held casts, tracked with response magic support.
    Disease disease = Disease::none;
    std::uint8_t worse_chance = 0; // Internal legacy counter, not a policy feature.
    bool glory = false, dark_cloud = false;
    bool dead = false; // v13: finalized death, distinct from provisional HP0.
    std::uint8_t incoming_sale = 0; // Physical delivery credit, separate from gift-backed item expenditure.
    std::uint8_t incoming_buy = 0;
    bool operator==(const Player&) const = default;
};
struct Pending {
    Effect kind = Effect::none;
    std::uint8_t source = no_slot, target = no_slot;
    std::int64_t amount = 0;
    CardId miracle = 0;
    bool from_ability = false;
    Element element = Element::neutral;
    std::uint8_t hit_rate = 0; // 0 is the historical guaranteed-hit sentinel.
    CardId status_card = 0;
    Disease disease = Disease::none;
    Cure cure = Cure::none;
    CardId harm_card = 0;
    Harm harm = Harm::none;
    CardId attribute_card = 0; // Canonical last setter among the selected extras.
    bool attribute_removed = false; // Current attack was neutralized by a response.
    std::uint8_t origin_source = no_slot; // Immutable item owner in the counter profile.
    CardId counter_card = 0;
    CardId mana_weapon = 0; // Base57 provenance; never a MAGIC or inherited by a counter.
    std::uint8_t mana_spent = 0;
    CardId wide_miracle = 0; // WIDE modifier provenance; base classification stays WEAPON.
    CardId free_card = 0; // Free carrier for a standalone MAGIC cast.
    CardId forget_card = 0; // Untyped SUNDRY forgetting payload.
    CardId item_removal_card = 0; // Untyped SUNDRY hand-removal payload.
    CardId hp_absorption_card = 0; // Original absorption weapon or MAGIC221; never inherited by counters.
    CardId self_damage_card = 0; // Base53 provenance; never a MAGIC or inherited by counters.
    CardId pestle_card = 0, mortar_card = 0; // Original99/selected245; not inherited by counters.
    CardId sale_card = 0, sale_item = 0; // Single physical product consumed when SELL3 starts.
    CardId buy_card = 0;
    CardId hp_gamble_card = 0; // Signed amount chosen once at launch, before any response.
    bool operator==(const Pending&) const = default;
};
struct LastChance {
    CardId id = 0;
    std::uint8_t source = no_slot, target = no_slot, hit_rate = 0;
    bool hit = false;
    bool forced = false;
    CardId wide_miracle = 0; // Completed wide attack; id remains its original weapon base.
    bool operator==(const LastChance&) const = default;
};
struct AttackComposition {
    CardId base = 0;
    std::uint8_t initial_mp = 0, count = 0;
    std::array<CardId, attack_extra_capacity> items{};
    std::uint64_t held_mask = 0;
    bool held_tail = false; // Ref64; avoids shifting a uint64 by64.
    std::array<std::uint64_t, additive_miracle_count> initial_additive{};
    std::uint64_t initial_doubling = 0;
    std::uint64_t initial_wide = 0;
    bool operator==(const AttackComposition&) const = default;
};
struct FreeCastComposition {
    CardId magic = 0;
    std::uint8_t initial_mp = 0;
    bool from_ability = false;
    std::uint8_t count = 0;
    std::array<CardId, hand_capacity> items{};
    bool operator==(const FreeCastComposition&) const = default;
};
struct LastForget {
    CardId id = 0;
    std::uint8_t source = no_slot, target = no_slot;
    CardId removed = 0;
    std::uint64_t population = 0; // Private pre-removal total, excluded from observations.
    bool operator==(const LastForget&) const = default;
};
struct LastExchange {
    CardId id = 0;
    std::uint8_t seat = no_slot;
    std::array<std::uint8_t, 3> before{}, after{}; // HP,MP,YEN allocation, before owner tick.
    bool operator==(const LastExchange&) const = default;
};
struct LastRevival {
    CardId id = 0;
    std::uint8_t seat = no_slot;
    bool operator==(const LastRevival&) const = default;
};
struct LastItemRemoval {
    CardId id = 0;
    std::uint8_t source = no_slot, target = no_slot;
    CardId removed = 0;
    std::uint8_t population = 0, slot = no_slot; // Private pre-removal hand count and slot.
    bool operator==(const LastItemRemoval&) const = default;
};
struct LastHPAbsorption {
    CardId id = 0;
    std::uint8_t source = no_slot, target = no_slot;
    std::int64_t amount = 0; // Positive residual, or zero when fully defended.
    std::uint8_t healing = 0; // Actual HP gained before target damage and owner tick.
    bool operator==(const LastHPAbsorption&) const = default;
};
struct LastSelfDamage {
    CardId id = 0;
    std::uint8_t source = no_slot, target = no_slot;
    std::int64_t amount = 0;
    std::uint8_t source_damage = 0, target_damage = 0; // Two consecutive clipped HP losses.
    bool operator==(const LastSelfDamage&) const = default;
};
struct LastPestle {
    CardId id = 0;
    std::uint8_t source = no_slot, target = no_slot; // Immutable initial selection, before reactions.
    CardId mortar = 0;
    int amount = 0;
    bool operator==(const LastPestle&) const = default;
};
struct LastSale {
    CardId id = 0, item = 0;
    std::uint8_t source = no_slot, target = no_slot;
    int price = 0;
    bool delivered = false;
    std::uint8_t money_lost = 0, mp_lost = 0, hp_lost = 0, money_gained = 0;
    std::uint32_t turn = 0; // Completed turn of this historical transaction.
    bool operator==(const LastSale&) const = default;
};
enum class BuyOutcome : std::uint8_t { none, empty, self, insufficient, offered, declined, purchased };
struct LastBuy {
    CardId id = 0, item = 0;
    std::uint8_t source = no_slot, target = no_slot;
    int price = 0;
    std::uint8_t population = 0, slot = no_slot; // Private physical selection.
    BuyOutcome outcome = BuyOutcome::none;
    bool delivered = false;
    std::uint8_t money_lost = 0, money_gained = 0;
    std::uint32_t turn = 0; // Future completion while offered; completed turn otherwise.
    bool operator==(const LastBuy&) const = default;
};
struct LastHPGamble {
    CardId id = 0;
    std::uint8_t source = no_slot, target = no_slot; // Immutable launch endpoints.
    int delta = 0; // Public signed draw, before a response or reflection.
    bool resolved = false;
    std::uint8_t resolved_source = no_slot, resolved_target = no_slot;
    int change = 0; // Actual clipped target HP change, before revival and owner tick.
    std::uint32_t turn = 0; // Future completion while active; completed turn otherwise.
    bool operator==(const LastHPGamble&) const = default;
};
struct State {
    std::array<Player, 2> players{};
    std::array<std::uint8_t, 2> order{0, 1};
    std::uint8_t order_index = 0, actor = 0, turn_owner = 0;
    Phase phase = Phase::main;
    Pending pending{};
    AttackComposition attack_composition{};
    FreeCastComposition free_cast{};
    std::array<Pending, counter_queue_capacity> counter_queue{};
    std::uint8_t counter_count = 0, counter_generated = 0, counter_started = 0;
    LastExchange last_exchange{};
    LastRevival last_revival{};
    LastItemRemoval last_item_removal{};
    LastHPAbsorption last_hp_absorption{};
    LastSelfDamage last_self_damage{};
    LastPestle last_pestle{};
    LastSale last_sale{};
    LastBuy last_buy{};
    LastHPGamble last_hp_gamble{};
    LastForget last_forget{};
    LastChance last_chance{}; // Public event; no hidden roll value.
    std::uint32_t reaction_count = 0; // Current unresolved chain only.
    struct LastReaction {
        CardId id = 0;
        std::uint8_t source = no_slot, target = no_slot;
        WeaponReaction reaction = WeaponReaction::none;
        bool from_ability = false;
        CardId free_card = 0; // Unconsumed suffix of free210/215.
        bool operator==(const LastReaction&) const = default;
    } last_reaction{};
    struct LastDiseaseTick {
        std::uint8_t seat = no_slot;
        Disease before = Disease::none, after = Disease::none;
        bool worsened = false;
        int damage = 0, healing = 0;
        bool operator==(const LastDiseaseTick&) const = default;
    } last_tick{};
    std::uint32_t decisions = 0, turns = 0;
    bool terminated = false, truncated = false;
    int winner = -1;
    bool operator==(const State&) const = default;
};
struct Snapshot {
    std::uint64_t format_tag = 0x67666c6567616379ULL; // gflegacy; in-process fixture format.
    std::uint32_t version = 36;
    Config config{};
    State state{};
    std::uint64_t rng_state = 0;
    std::array<CardId, 64> scripted_draws{};
    std::uint8_t draw_count = 0, draw_index = 0;
    std::array<std::uint8_t, 64> scripted_hits{};
    std::uint8_t hit_count = 0, hit_index = 0;
    std::array<std::uint8_t, 64> scripted_targets{};
    std::uint8_t target_count = 0, target_index = 0;
    std::array<std::uint8_t, 64> scripted_diseases{};
    std::uint8_t disease_count = 0, disease_index = 0;
    std::array<std::uint64_t, 64> scripted_forgets{};
    std::uint8_t forget_count = 0, forget_index = 0;
    std::array<std::uint64_t, 64> scripted_item_removals{};
    std::uint8_t item_removal_count = 0, item_removal_index = 0;
    std::array<std::uint8_t, 64> scripted_pestle_targets{};
    std::uint8_t pestle_target_count = 0, pestle_target_index = 0;
    std::array<std::uint64_t, 64> scripted_buys{};
    std::uint8_t buy_count = 0, buy_index = 0;
    std::array<std::uint8_t, 64> scripted_hp_gambles{};
    std::uint8_t hp_gamble_count = 0, hp_gamble_index = 0;
    bool operator==(const Snapshot&) const = default;
};
struct Options {
    std::uint16_t weapon_bases = 0, boost_slots = 0, defense_slots = 0;
    std::uint16_t utility_slots = 0, discard_slots = 0;
    std::uint16_t ability_slots = 0;
    std::uint16_t reaction_slots = 0;
    std::uint16_t defensive_ability_slots = 0; // Local defensive kinds, not hand slots.
    std::uint16_t attribute_slots = 0; // Attribute-only attack extras, never ATK boosts.
    std::uint16_t removal_slots = 0; // Attribute removal cards usable in response slots.
    std::uint16_t counter_slots = 0; // Counter cards, still subject to composed defense legality.
    std::uint16_t additive_slots = 0, additive_ability_slots = 0;
    std::uint16_t doubling_slots = 0, doubling_ability_slots = 0;
    std::uint16_t wide_slots = 0, wide_ability_slots = 0;
    std::uint16_t magic_free_slots = 0, free_cast_slots = 0, free_ability_slots = 0;
    std::uint16_t free_additive_ability_slots = 0, free_defensive_slots = 0, free_defensive_ability_slots = 0;
    std::uint16_t forget_slots = 0;
    std::uint16_t exchange_slots = 0;
    std::uint16_t item_removal_slots = 0;
    std::uint16_t hp_absorption_slots = 0; // Main-hand membership; combine with other role masks/legality.
    std::uint16_t self_damage_slots = 0; // Main-hand membership; combine with weapon/legality masks.
    std::uint16_t pestle_slots = 0, mortar_slots = 0; // Main-hand membership.
    std::uint16_t sell_slots = 0, sale_item_slots = 0; // Select a SELL base and a distinct physical product.
    std::uint16_t buy_slots = 0, purchase_choices = 0; // Purchase choices bit0=NO, bit1=YES.
    std::uint16_t hp_gamble_slots = 0;
    std::uint16_t magic_bounce_slots = 0; // Hand membership in active main/response; consult reaction_slots and legal().
    std::uint16_t sundry_boost_slots = 0; // Main-hand238 membership, also in ordinary boost_slots.
    std::uint16_t status_slots = 0, status_ability_slots = 0, free_status_ability_slots = 0;
    bool pray = false;
    std::uint8_t defense_limit = 0;
    bool operator==(const Options&) const = default;
};
struct StepResult {
    bool ordered_attack = false;
    std::uint8_t ordered_components = 0, additive_first_uses = 0, additive_recasts = 0;
    std::uint16_t additive_mp_spent = 0;
    std::uint8_t doubling_first_uses = 0, doubling_recasts = 0;
    std::uint16_t doubling_mp_spent = 0;
    std::uint8_t wide_first_uses = 0, wide_recasts = 0;
    std::uint16_t wide_mp_spent = 0;
    bool wide_attack = false;
    std::uint8_t magic_free_cards = 0;
    std::uint16_t magic_free_mp_saved = 0, spell_mp_spent = 0;
    bool magic_free_suffix = false, free_cast = false, ordered_defense = false;
    bool forget_attempt = false, forget_removed = false;
    CardId forgotten_miracle = 0;
    std::uint8_t forgotten_from = no_slot;
    std::uint64_t forget_population = 0;
    std::array<std::uint8_t, 2> revivals{}; // Automatic, without replacement gifts.
    bool item_removal_attempt = false, item_removed = false;
    CardId removed_item = 0;
    std::uint8_t item_removed_from = no_slot, item_removal_population = 0, removed_item_slot = no_slot;
    bool hp_absorption_resolved = false;
    std::int64_t hp_absorption_amount = 0;
    std::uint8_t hp_absorption_healing = 0, hp_absorption_source = no_slot, hp_absorption_target = no_slot;
    CardId hp_absorption_card = 0;
    bool self_damage_resolved = false;
    std::int64_t self_damage_amount = 0;
    CardId self_damage_card = 0;
    std::uint8_t self_damage_source = no_slot, self_damage_target = no_slot;
    std::uint8_t self_damage_source_lost = 0, self_damage_target_lost = 0;
    bool pestle_attack = false, pestle_mortar = false, pestle_resolved = false;
    std::uint8_t pestle_target = no_slot, pestle_target_lost = 0;
    std::uint16_t pestle_damage = 0;
    std::int64_t pestle_residual = 0;
    std::uint8_t pestle_resolved_source = no_slot, pestle_resolved_target = no_slot, mortar_discards_ignored = 0;
    bool sale_used = false, sale_resolved = false, sale_delivered = false;
    CardId sale_card = 0, sale_item = 0;
    int sale_price = 0;
    std::uint8_t sale_source = no_slot, sale_target = no_slot;
    std::uint8_t sale_money_lost = 0, sale_mp_lost = 0, sale_hp_lost = 0, sale_money_gained = 0;
    bool buy_used = false, buy_attempt = false, buy_offered = false, buy_decided = false, buy_accepted = false, buy_delivered = false;
    CardId buy_item = 0;
    int buy_price = 0;
    std::uint8_t buy_source = no_slot, buy_target = no_slot, buy_population = 0, buy_slot = no_slot;
    std::uint8_t buy_money_lost = 0, buy_money_gained = 0;
    BuyOutcome buy_outcome = BuyOutcome::none;
    bool hp_gamble_used = false, hp_gamble_resolved = false;
    CardId hp_gamble_card = 0;
    int hp_gamble_delta = 0, hp_gamble_change = 0;
    std::uint8_t hp_gamble_source = no_slot, hp_gamble_target = no_slot;
    bool exchanged = false;
    std::array<int, 3> exchange_before{}, exchange_after{};
    bool valid = false;
    std::uint8_t actor = 0;
    std::array<float, 2> rewards{};
    bool terminated = false, truncated = false;
    bool status_first_use = false, status_recast = false, status_resolved = false;
    CardId status_miracle_card = 0;
    std::uint8_t status_source = no_slot, status_target = no_slot;
    bool chance_attempt = false, chance_hit = false;
    bool chance_forced = false;
    bool reflected = false, bounced = false, bounce_self = false;
    bool disease_tick = false, disease_worsened = false;
    int disease_damage = 0, disease_healing = 0;
    std::uint32_t harm_applications = 0, harm_new_flags = 0, harm_heaven_falls = 0;
    bool glory_defense = false;
    bool dark_execution = false;
    bool spell_first_use = false, spell_recast = false;
    bool defensive_first_use = false, defensive_recast = false;
    bool magic_bounced = false, weapon_blocked = false, magic_blocked = false;
    bool magic_reflected = false, any_reflected = false;
    bool magic_card_bounced = false;
    CardId magic_bounce_card = 0;
    std::uint8_t sundry_boost_cards_used = 0; // Physical238 attack extras; final raw may be reset or doubled later.
    bool counter_cancelled = false; // Launched item reflected to an already dead target.
    bool mana_weapon_attack = false;
    std::uint8_t mana_weapon_mp_spent = 0;
    std::uint16_t mana_weapon_power = 0;
    bool attribute_attack = false;
    std::uint8_t attribute_cards_used = 0;
    Element attribute_element = Element::neutral;
    bool attribute_removed = false;
    std::uint8_t attribute_removals = 0;
    Element removed_from = Element::neutral;
    std::uint8_t counter_queued = 0, counter_launched = 0, counter_skipped = 0;
    bool counter_resolved = false;
    std::uint8_t counter_missed = 0, counter_auto_resolved = 0;
    std::uint8_t counter_chance_attempts = 0, counter_chance_hits = 0, counter_chance_misses = 0;
    std::uint8_t counter_chance_forced = 0, counter_hidden_checks = 0;
    std::uint32_t counter_mp_gained = 0, counter_harm_applications = 0;
    std::uint8_t counter_money_absorptions = 0;
    std::uint32_t counter_money_gained = 0, counter_money_lost = 0;
    std::uint32_t counter_mp_lost = 0, counter_hp_lost = 0;
};
using Observation = std::array<float, observation_size>;
class Env {
public:
    explicit Env(Config config = {});
    void reset(std::uint64_t seed) noexcept;
    const State& state() const noexcept { return snapshot_.state; }
    const char* profile() const noexcept {
        if (snapshot_.config.status_miracles) return status_miracles_ruleset;
        if (snapshot_.config.sundry_boost) return sundry_boost_ruleset;
        if (snapshot_.config.magic_bounce) return magic_bounce_ruleset;
        if (snapshot_.config.hp_gamble) return hp_gamble_ruleset;
        if (snapshot_.config.buying) return buying_ruleset;
        if (snapshot_.config.selling) return selling_ruleset;
        if (snapshot_.config.pestle_mortar) return pestle_mortar_ruleset;
        if (snapshot_.config.self_damage) return self_damage_ruleset;
        if (snapshot_.config.hp_absorption) return hp_absorption_ruleset;
        if (snapshot_.config.item_removal) return item_removal_ruleset;
        if (snapshot_.config.revival) return revival_ruleset;
        if (snapshot_.config.exchanges) return exchange_ruleset;
        if (snapshot_.config.forgetting) return forgetting_ruleset;
        if (snapshot_.config.magic_free) return magic_free_ruleset;
        if (snapshot_.config.wide_miracles) return wide_miracles_ruleset;
        if (snapshot_.config.doubling_miracles) return doubling_miracles_ruleset;
        if (snapshot_.config.additive_miracles) return additive_miracles_ruleset;
        if (snapshot_.config.magic_block) return magic_block_ruleset;
        if (snapshot_.config.mana_weapon) return mana_weapon_ruleset;
        if (snapshot_.config.any_reflection) return any_reflection_ruleset;
        if (snapshot_.config.magic_reflection) return magic_reflection_ruleset;
        if (snapshot_.config.money_counter) return money_counter_ruleset;
        if (snapshot_.config.counter_extensions) return counter_extensions_ruleset;
        if (snapshot_.config.counters) return counter_ruleset;
        if (snapshot_.config.attribute_removal) return attribute_removal_ruleset;
        if (snapshot_.config.attribute_change) return attribute_change_ruleset;
        if (snapshot_.config.defensive_miracles) return defensive_miracles_ruleset;
        if (snapshot_.config.offensive_miracles) return offensive_miracles_ruleset;
        if (snapshot_.config.celestial) return celestial_ruleset;
        if (snapshot_.config.harms) return harms_ruleset;
        if (snapshot_.config.disease) return disease_ruleset;
        if (snapshot_.config.reflection) return reflection_ruleset;
        if (snapshot_.config.chance) return snapshot_.config.miracles ? chance_miracles_ruleset : chance_ruleset;
        if (snapshot_.config.elements) return snapshot_.config.miracles ? elements_miracles_ruleset : elements_ruleset;
        return snapshot_.config.miracles ? miracles_ruleset : ruleset;
    }
    bool done() const noexcept { return state().terminated || state().truncated; }
    Snapshot snapshot() const noexcept { return snapshot_; }
    bool restore(const Snapshot& snapshot) noexcept;
    // Prefix draw outcomes for fixtures; after the prefix, continue seeded draws.
    bool script_draws(std::span<const CardId> draws) noexcept;
    bool script_hit_rolls(std::span<const std::uint8_t> rolls) noexcept;
    bool script_bounce_targets(std::span<const std::uint8_t> targets) noexcept;
    bool script_pestle_targets(std::span<const std::uint8_t> targets) noexcept;
    bool script_disease_rolls(std::span<const std::uint8_t> rolls) noexcept;
    bool script_forget_tickets(std::span<const std::uint64_t> tickets) noexcept;
    bool script_buy_tickets(std::span<const std::uint64_t> tickets) noexcept;
    bool script_hp_gamble_rolls(std::span<const std::uint8_t> rolls) noexcept;
    bool script_item_removal_tickets(std::span<const std::uint64_t> tickets) noexcept;
    bool legal(Action action) const noexcept;
    Options options() const noexcept;
    Observation observe() const noexcept;
    StepResult step(Action action) noexcept;
    // Hierarchical random policy, not uniform over all semantic actions.
    Action sample_action(std::uint64_t& policy_rng) const noexcept;
private:
    Snapshot snapshot_{};
    struct CompositionPlan {
        int amount = 0, cost = 0, additive_cost = 0, doubling_cost = 0, wide_cost = 0;
        Element element = Element::neutral;
        CardId setter = 0;
        std::uint8_t physical = 1, held = 0, first_uses = 0, setters = 0, mana_spent = 0;
        std::array<std::uint64_t, additive_miracle_count> acquired{};
        std::uint64_t acquired_doubling = 0, acquired_wide = 0;
        std::uint8_t additive_recasts = 0, doubling_first_uses = 0, doubling_recasts = 0;
        std::uint8_t wide_first_uses = 0, wide_recasts = 0;
        bool wide = false;
        int free_index = -1;
        CardId free_card = 0;
        std::uint16_t mp_saved = 0;
        std::uint8_t free_cards = 0;
    };
    struct FreeCastPlan {
        FreeCastComposition record{};
        std::uint16_t slots = 0;
        std::uint8_t hand_base = no_slot;
    };
    struct DefensePlan {
        Action action{};
        std::array<std::uint8_t, hand_capacity> prefix{};
        std::uint8_t prefix_count = 0;
        CardId free_card = 0;
        std::uint16_t mp_saved = 0;
        bool suffix = false;
    };
    bool prepare_exchange(Action action, LastExchange& record) const noexcept;
    bool prepare_sale(Action action, CardId& item, int& price) const noexcept;
    bool prepare_free_cast(Action action, FreeCastPlan& plan) const noexcept;
    bool prepare_ordered_defense(Action action, DefensePlan& plan) const noexcept;
    bool replay_composition(const AttackComposition& record, CompositionPlan& plan) const noexcept;
    bool prepare_ordered(Action action, AttackComposition& record, CompositionPlan& plan, std::uint16_t& slots) const noexcept;
    bool supported(CardId id) const noexcept;
    int trade_price(CardId id) const noexcept;
    const Card* artifact(CardId id) const noexcept;
    Element element_of(CardId id) const noexcept;
    int chance_rate(CardId id) const noexcept;
    CardId attribute_component(Action action) const noexcept;
    std::uint8_t hit_roll() noexcept;
    std::uint8_t bounce_target() noexcept;
    std::uint8_t disease_roll() noexcept;
    void apply_disease(int seat, Disease disease) noexcept;
    void apply_harm(int seat, Harm harm, StepResult& result) noexcept;
    void cure_disease(int seat, Cure cure) noexcept;
    void forget_one(StepResult& result) noexcept;
    void offer_buy(StepResult& result) noexcept;
    void decide_buy(bool accepted, StepResult& result) noexcept;
    void remove_item(StepResult& result) noexcept;
    CardId draw() noexcept;
    void deal(int seat, std::uint64_t count) noexcept;
    void consume(int seat, std::uint16_t slots) noexcept;
    void finish_turn(StepResult& result) noexcept;
    void finish_item(StepResult& result) noexcept;
    void finalize_deaths(StepResult& result) noexcept;
    void resolve(int defense, StepResult& result) noexcept;
    void launch_pending(CardId card, StepResult& result) noexcept;
};
}
