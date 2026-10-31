#include "godfield/legacy_env.hpp"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace {
struct alignas(64) Stats {
    std::uint64_t decisions = 0;
    std::uint64_t wins[2]{};
    std::uint64_t draws = 0, truncations = 0;
    std::uint64_t miracle_first_uses = 0, miracle_recasts = 0;
    std::uint64_t miracle_mp_spent = 0, learned_miracles = 0;
    std::uint64_t miracle_hp_gained = 0, miracle_money_gained = 0;
    std::uint64_t elemental_attacks = 0, neutral_attacks = 0;
    std::uint64_t effective_elemental_defenses = 0, elemental_damage_reduced = 0;
    std::uint64_t chance_attempts = 0, chance_hits = 0, chance_misses = 0;
    std::uint64_t weapon_reactions = 0, reflections = 0, bounces = 0, self_bounces = 0;
    std::uint64_t disease_ticks = 0, disease_worsenings = 0, tick_damage = 0, tick_healing = 0;
    std::uint64_t forced_chance_hits = 0, harm_applications = 0, harm_new_flags = 0;
    std::uint64_t harm_heaven_falls = 0, glory_defense_decisions = 0;
    std::uint64_t light_attacks = 0, dark_attacks = 0, dark_executions = 0;
    std::uint64_t offensive_miracle_first_uses = 0, offensive_miracle_recasts = 0;
    std::uint64_t offensive_miracle_attacks = 0, offensive_miracle_resolutions = 0;
    std::uint64_t offensive_miracle_mp_spent = 0, offensive_learned_miracles = 0;
    std::uint64_t offensive_miracle_hp_damage = 0, offensive_miracle_chance_attempts = 0;
    std::uint64_t defensive_miracle_first_uses = 0, defensive_miracle_recasts = 0;
    std::uint64_t defensive_miracle_mp_spent = 0, defensive_learned_miracles = 0;
    std::uint64_t magic_bounces = 0, magic_self_bounces = 0, weapon_blocks = 0;
    std::uint64_t attribute_attacks = 0, attribute_cards_used = 0;
    std::uint64_t attribute_fire_attacks = 0, attribute_water_attacks = 0, attribute_neutral_attacks = 0;
    std::uint64_t attribute_weapon_base_uses = 0;
    std::uint64_t attribute_removal_uses = 0, attribute_removal_cards_used = 0;
    std::array<std::uint64_t, 7> removed_from_elements{};
    std::uint64_t removal_weapon_responses = 0, removal_magic_responses = 0;
    std::uint64_t counter_removal_responses = 0, removal_utility_responses = 0;
    std::uint64_t removal_weapon_reactions = 0, removal_magic_bounces = 0, removal_weapon_blocks = 0;
    std::uint64_t neutralized_weapon_reactions = 0, neutralized_percentage_weapon_reactions = 0;
    std::uint64_t counters_queued = 0, counters_launched = 0, counters_skipped = 0, counters_resolved = 0;
    std::uint64_t counter_defense_decisions = 0, counter_cards_used = 0;
    std::uint64_t counter_queue_remaining = 0, counter_active_remaining = 0;
    std::uint64_t counter_missed = 0, counter_auto_resolved = 0;
    std::uint64_t counter_chance_attempts = 0, counter_chance_hits = 0, counter_chance_misses = 0;
    std::uint64_t counter_chance_forced = 0, counter_hidden_checks = 0;
    std::uint64_t counter_mp_gained = 0, counter_harm_applications = 0;
    std::uint64_t counter_money_absorptions = 0, counter_money_gained = 0, counter_money_lost = 0;
    std::uint64_t counter_mp_lost = 0, counter_hp_lost = 0;
    std::uint64_t magic_reflections = 0, removal_magic_reflections = 0;
    std::uint64_t any_reflections = 0, any_weapon_reflections = 0, any_magic_reflections = 0;
    std::uint64_t any_counter_reflections = 0, any_utility_reflections = 0, removal_any_reflections = 0;
    std::uint64_t counters_cancelled = 0;
    std::uint64_t mana_weapon_attacks = 0, mana_weapon_mp_spent = 0, mana_weapon_power_total = 0;
    std::uint64_t mana_weapon_zero_mp_attacks = 0, mana_weapon_zero_power_attacks = 0, mana_weapon_setter_attacks = 0;
    std::uint64_t magic_blocks = 0, magic_damage_blocks = 0, magic_utility_blocks = 0, removal_magic_blocks = 0;
    std::uint64_t additive_first_uses = 0, additive_recasts = 0, additive_mp_spent = 0, additive_learned = 0;
    std::uint64_t additive_single_attacks = 0, additive_single_resolutions = 0, additive_single_hp_damage = 0;
    std::uint64_t additive_extra_uses = 0, ordered_attacks = 0, ordered_components = 0;
    std::uint64_t mana_weapon_zero_burn_attacks = 0;
    std::uint64_t doubling_first_uses = 0, doubling_recasts = 0, doubling_mp_spent = 0, doubling_learned = 0;
    std::uint64_t doubling_attacks = 0, doubling_uses = 0;
    std::uint64_t wide_first_uses = 0, wide_recasts = 0, wide_mp_spent = 0, wide_learned = 0;
    std::uint64_t wide_attacks = 0, wide_uses = 0, wide_chance_attempts = 0, wide_chance_hits = 0;
    std::uint64_t wide_forced_hits = 0, wide_weapon_reactions = 0, wide_removal_responses = 0;
    std::uint64_t magic_free_cards = 0, magic_free_attack_cards = 0, magic_free_cast_cards = 0, magic_free_defense_cards = 0;
    std::uint64_t magic_free_mp_saved = 0, magic_free_attack_mp_saved = 0, magic_free_cast_mp_saved = 0, magic_free_defense_mp_saved = 0;
    std::uint64_t magic_free_suffixes = 0, free_casts = 0, ordered_defenses = 0;
    std::uint64_t forgetting_uses = 0, forgetting_attempts = 0, forgetting_removed = 0, forgetting_empty = 0;
    std::uint64_t forgetting_self_targets = 0, forgetting_opponent_targets = 0, forgetting_reflections = 0;
    std::uint64_t forgetting_utility = 0, forgetting_offensive = 0, forgetting_defensive = 0;
    std::uint64_t forgetting_additive = 0, forgetting_doubling = 0, forgetting_wide = 0, retained_inventory = 0;
    std::uint64_t exchanges = 0, exchange_noops = 0, exchange_suicides = 0;
    std::uint64_t exchange_tick_damage = 0, exchange_tick_healing = 0;
    std::array<std::uint64_t, 3> exchange_up{}, exchange_down{};
    std::uint64_t revivals = 0, revival_steps = 0, revival_multiple_steps = 0, revival_both_seats = 0;
    std::array<std::uint64_t, 2> revivals_by_seat{};
    std::uint64_t revival_hp_restored = 0, exchange_revival_steps = 0;
    std::uint64_t item_removal_uses = 0, item_removal_attempts = 0, items_removed = 0, item_removal_empty = 0;
    std::uint64_t item_removal_self_targets = 0, item_removal_opponent_targets = 0, item_removal_reflections = 0;
    std::uint64_t item_removed_weapons = 0, item_removed_armor = 0, item_removed_miracles = 0, item_removed_other = 0;
    std::uint64_t item_removed_revivals = 0;
    std::uint64_t hp_absorption_resolutions = 0, hp_absorption_weapon_resolutions = 0, hp_absorption_miracle_resolutions = 0;
    std::uint64_t hp_absorption_self_resolutions = 0, hp_absorption_zero_residual = 0, hp_absorption_zero_healing = 0;
    std::uint64_t hp_absorption_healing = 0, hp_absorption_capped = 0, hp_absorption_dark_resolutions = 0, hp_absorption_raw_max = 0;
    std::uint64_t hp_absorption_miracle_first_uses = 0, hp_absorption_miracle_recasts = 0, hp_absorption_miracle_mp_spent = 0;
    std::uint64_t hp_absorption_learned_miracles = 0, hp_absorption_miracle_attacks = 0, hp_absorption_miracle_hp_damage = 0;
    std::uint64_t forgetting_hp_absorption = 0;
    std::uint64_t self_damage_attacks = 0, self_damage_resolutions = 0, self_damage_self_resolutions = 0, self_damage_zero_residual = 0, self_damage_zero_source_loss = 0, self_damage_source_lost = 0, self_damage_target_lost = 0, self_damage_capped_source = 0, self_damage_capped_target = 0, self_damage_raw_max = 0, self_damage_reflections = 0, self_damage_revival_steps = 0;
    std::uint64_t pestle_attacks = 0, pestle_mortar_locked_attacks = 0, pestle_random_target_attacks = 0, pestle_initial_self_targets = 0, pestle_initial_opponent_targets = 0, pestle_resolutions = 0, pestle_self_resolutions = 0, pestle_zero_residual = 0, pestle_target_lost = 0, pestle_capped = 0, pestle_raw_max = 0, pestle_reflections = 0, pestle_blocks = 0, pestle_revival_steps = 0, mortar_discards_ignored = 0, item_removed_mortars = 0;
    std::uint64_t sales = 0, sale_resolutions = 0, sale_delivered = 0, sale_delivery_failures = 0, sale_self_resolutions = 0, sale_opponent_resolutions = 0, sale_zero_price = 0, sale_price_total = 0, sale_money_lost = 0, sale_mp_lost = 0, sale_hp_lost = 0, sale_money_gained = 0, sale_reflections = 0, sale_revival_steps = 0, sale_weapons = 0, sale_armor = 0, sale_miracles = 0, sale_other = 0;
    std::uint64_t buy_uses = 0, buy_attempts = 0, buy_empty = 0, buy_self_selections = 0, buy_insufficient = 0, buy_offers = 0, buy_decisions = 0, buy_declines = 0, buy_purchases = 0, buy_delivered = 0, buy_delivery_failures = 0, buy_money_spent = 0, buy_money_gained = 0, buy_zero_price = 0, buy_reflections = 0, buy_active_offers = 0, buy_revival_steps = 0, bought_weapons = 0, bought_armor = 0, bought_miracles = 0, bought_other = 0, sold_buy_cards = 0;
    std::uint64_t hp_gamble_uses = 0, hp_gamble_heal_rolls = 0, hp_gamble_damage_rolls = 0, hp_gamble_initial_self_targets = 0, hp_gamble_initial_opponent_targets = 0, hp_gamble_resolutions = 0, hp_gamble_healing_resolutions = 0, hp_gamble_damage_resolutions = 0, hp_gamble_self_resolutions = 0, hp_gamble_healed = 0, hp_gamble_damaged = 0, hp_gamble_zero_change = 0, hp_gamble_capped_heals = 0, hp_gamble_capped_damage = 0, hp_gamble_reflections = 0, hp_gamble_revival_steps = 0, sold_hp_gambles = 0, bought_hp_gambles = 0;
    std::uint64_t magic_card_bounces = 0, magic_card_self_bounces = 0, magic_card_damage_bounces = 0, magic_card_utility_bounces = 0, magic_card_held_spell_bounces = 0, magic_card_free_spell_bounces = 0, magic_card_removal_bounces = 0, magic_bounce_weapon_attacks = 0, magic_bounce_boost_uses = 0, magic_bounce_ordinary_cards_used = 0, sold_magic_bounce_cards = 0, bought_magic_bounce_cards = 0;
    std::array<std::uint64_t, 6> magic_card_bounces_by_id{};
    std::uint64_t sundry_boost_attacks = 0, sundry_boost_cards_used = 0, sundry_boost_plain_attacks = 0, sundry_boost_ordered_attacks = 0, sundry_boost_mana_attacks = 0, sundry_boost_mana_resets = 0, sundry_boost_resets_after_boost = 0, sundry_boost_double_attacks = 0, sundry_boost_wide_attacks = 0, sundry_boost_reaction_steps = 0, sundry_boost_counters_queued = 0, sundry_boost_discarded_cards = 0, sundry_boost_sold_cards = 0, sundry_boost_bought_cards = 0, sundry_boost_removed_cards = 0, sundry_boost_raw_max = 0, sundry_boost_raw_checked = 0, sundry_boost_self_attacks = 0;
    std::array<std::uint64_t, 7> sundry_boost_elements{};
    std::uint64_t status_first_uses = 0, status_recasts = 0, status_mp_spent = 0, status_learned = 0, status_casts = 0, status_resolutions = 0, status_self_resolutions = 0, status_lower_cures = 0, status_all_cures = 0, status_dark_cloud_applications = 0, status_cold_applications = 0, status_heaven_applications = 0, status_heaven_falls = 0, status_disease_changes = 0, status_diseases_removed = 0, status_glory_cleared = 0, status_dark_cloud_cleared = 0, status_dark_cloud_new = 0, status_hp_lost = 0, status_free_uses = 0, status_reflections = 0, status_bounces = 0, status_blocks = 0, status_held_resolutions = 0, forgetting_status_miracles = 0, status_retained_inventory = 0, sold_status_miracles = 0, bought_status_miracles = 0, removed_status_miracles = 0;
    std::uint64_t checksum = 0;
    std::exception_ptr error;
};

std::uint64_t number(const char* text) {
    std::uint64_t value = 0;
    const std::string_view input(text);
    const auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), value);
    if (error != std::errc{} || end != input.data() + input.size())
        throw std::invalid_argument("arguments must be unsigned decimal integers");
    return value;
}

std::uint64_t mix(std::uint64_t value) noexcept {
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

const godfield::legacy::Card* artifact(godfield::legacy::CardId id) noexcept {
    if (const auto* neutral = godfield::legacy::find_card(id)) return neutral;
    if (const auto* elemental = godfield::legacy::find_element_card(id)) return &elemental->card;
    if (const auto* chance = godfield::legacy::find_chance_card(id)) return &chance->card;
    if (const auto* reaction = godfield::legacy::find_reaction_card(id)) return &reaction->card;
    if (const auto* disease = godfield::legacy::find_disease_card(id)) return &disease->card;
    if (const auto* harm = godfield::legacy::find_harm_card(id)) return &harm->card;
    if (const auto* setter = godfield::legacy::find_attribute_change_card(id)) return &setter->card;
    if (const auto* remover = godfield::legacy::find_attribute_removal_card(id)) return &remover->card;
    if (const auto* counter = godfield::legacy::find_counter_card(id)) return &counter->card;
    if (const auto* counter = godfield::legacy::find_counter_extension_card(id)) return &counter->card;
    if (const auto* counter = godfield::legacy::find_money_counter_card(id)) return &counter->card;
    if (const auto* mirror = godfield::legacy::find_magic_reflection_card(id)) return &mirror->card;
    if (const auto* mirror = godfield::legacy::find_any_reflection_card(id)) return &mirror->card;
    if (const auto* mana = godfield::legacy::find_mana_weapon_card(id)) return &mana->card;
    if (const auto* blocker = godfield::legacy::find_magic_block_card(id)) return &blocker->card;
    if (const auto* free = godfield::legacy::find_magic_free_card(id)) return &free->card;
    if (const auto* forgetting = godfield::legacy::find_forgetting_card(id)) return &forgetting->card;
    if (const auto* exchange = godfield::legacy::find_exchange_card(id)) return &exchange->card;
    if (const auto* revival = godfield::legacy::find_revival_card(id)) return &revival->card;
    if (const auto* removal = godfield::legacy::find_item_removal_card(id)) return &removal->card;
    if (const auto* absorption = godfield::legacy::find_hp_absorption_card(id)) return &absorption->card;
    if (const auto* self = godfield::legacy::find_self_damage_card(id)) return &self->card;
    if (const auto* pestle = godfield::legacy::find_pestle_card(id)) return &pestle->card;
    if (const auto* mortar = godfield::legacy::find_mortar_card(id)) return &mortar->card;
    if (const auto* selling = godfield::legacy::find_selling_card(id)) return &selling->card;
    if (const auto* buying = godfield::legacy::find_buying_card(id)) return &buying->card;
    if (const auto* gamble = godfield::legacy::find_hp_gamble_card(id)) return &gamble->card;
    if (const auto* bounce = godfield::legacy::find_magic_bounce_card(id)) return &bounce->card;
    if (const auto* boost = godfield::legacy::find_sundry_boost_card(id)) return &boost->card;
    const auto* celestial = godfield::legacy::find_celestial_card(id);
    return celestial ? &celestial->card : nullptr;
}

int chance_rate(godfield::legacy::CardId id) noexcept {
    if (const auto* chance = godfield::legacy::find_chance_card(id)) return chance->hit_rate;
    if (const auto* absorption = godfield::legacy::find_hp_absorption_card(id)) return absorption->hit_rate;
    if (const auto* celestial = godfield::legacy::find_celestial_card(id)) return celestial->hit_rate;
    if (const auto* miracle = godfield::legacy::find_offensive_miracle(id)) return miracle->hit_rate;
    return 0;
}

godfield::legacy::Element chance_element(godfield::legacy::CardId id) noexcept {
    if (const auto* pestle = godfield::legacy::find_pestle_card(id)) return pestle->element;
    if (const auto* elemental = godfield::legacy::find_element_card(id)) return elemental->element;
    if (const auto* absorption = godfield::legacy::find_hp_absorption_card(id)) return absorption->element;
    if (const auto* absorption = godfield::legacy::find_hp_absorption_miracle(id)) return absorption->element;
    if (const auto* disease = godfield::legacy::find_disease_card(id)) return disease->element;
    if (const auto* harm = godfield::legacy::find_harm_card(id)) return harm->element;
    if (const auto* additive = godfield::legacy::find_additive_miracle(id)) return additive->element;
    if (const auto* chance = godfield::legacy::find_chance_card(id)) return chance->element;
    if (const auto* celestial = godfield::legacy::find_celestial_card(id)) return celestial->element;
    if (const auto* miracle = godfield::legacy::find_offensive_miracle(id)) return miracle->element;
    if (const auto* setter = godfield::legacy::find_attribute_change_card(id)) return setter->element;
    if (const auto* counter = godfield::legacy::find_counter_card(id)) return counter->element;
    if (const auto* counter = godfield::legacy::find_counter_extension_card(id)) return counter->element;
    if (const auto* counter = godfield::legacy::find_money_counter_card(id)) return counter->element;
    if (godfield::legacy::find_magic_reflection_card(id) || godfield::legacy::find_any_reflection_card(id)
        || godfield::legacy::find_mana_weapon_card(id) || godfield::legacy::find_magic_block_card(id))
        return godfield::legacy::Element::neutral;
    return godfield::legacy::Element::neutral;
}

bool attribute_attack(godfield::legacy::ActionKind kind) noexcept {
    using godfield::legacy::ActionKind;
    return kind == ActionKind::attack_set_fire || kind == ActionKind::attack_set_water
        || kind == ActionKind::attack_set_neutral;
}

godfield::legacy::Element combine_element(godfield::legacy::Element left, godfield::legacy::Element right) noexcept {
    using godfield::legacy::Element;
    if (left == Element::light) return right;
    if (right == Element::light || left == right) return left;
    return Element::neutral;
}

struct SpellView {
    godfield::legacy::CardId id = 0;
    int index = -1;
    godfield::legacy::Effect effect = godfield::legacy::Effect::none;
    godfield::legacy::Element element = godfield::legacy::Element::neutral;
    int amount = 0, cost = 0, hit_rate = 0;
    bool defensive = false, additive = false, hp_absorption = false, status = false;
};

SpellView spell_view(godfield::legacy::CardId id) noexcept {
    using namespace godfield::legacy;
    if (const auto* status = find_status_miracle(id))
        return {id, status_miracle_index(id), Effect::status, status->element,
                0, status->cost, 0, false, false, false, true};
    if (const auto* absorption = find_hp_absorption_miracle(id))
        return {id, hp_absorption_ability_index, Effect::damage, absorption->element,
                absorption->amount, absorption->cost, 0, false, false, true};
    if (const auto* utility = find_miracle(id))
        return {id, miracle_index(id), utility->effect == MiracleEffect::heal ? Effect::heal : Effect::money,
                Element::neutral, utility->amount, utility->cost, 0};
    if (const auto* offensive = find_offensive_miracle(id))
        return {id, miracle_count + offensive_miracle_index(id), Effect::damage,
                offensive->element, offensive->amount, offensive->cost, offensive->hit_rate};
    if (const auto* defensive = find_defensive_miracle(id))
        return {id, defensive_miracle_index(id), Effect::none, Element::neutral,
                0, defensive->cost, 0, true};
    if (const auto* additive = find_additive_miracle(id))
        return {id, additive_miracle_index(id), Effect::damage, additive->element,
                additive->amount, additive->cost, 0, false, true};
    return {};
}

godfield::legacy::CardId spell_id_at(int index) noexcept {
    using namespace godfield::legacy;
    if (index == hp_absorption_ability_index) return hp_absorption_miracles()[0].id;
    if (index < 0 || index >= total_miracle_count) return 0;
    return index < miracle_count ? miracles()[index].id : offensive_miracles()[index - miracle_count].id;
}

std::uint64_t retained_count(const godfield::legacy::Player& player, godfield::legacy::CardId id) noexcept {
    using namespace godfield::legacy;
    if (const auto index = status_miracle_index(id); index >= 0) return player.status_abilities[index];
    if (find_hp_absorption_miracle(id)) return player.abilities[hp_absorption_ability_index];
    if (const auto index = miracle_index(id); index >= 0) return player.abilities[index];
    if (const auto index = offensive_miracle_index(id); index >= 0) return player.abilities[miracle_count + index];
    if (const auto index = defensive_miracle_index(id); index >= 0) return player.defensive_abilities[index];
    if (const auto index = additive_miracle_index(id); index >= 0) return player.additive_abilities[index];
    if (doubling_miracle_index(id) >= 0) return player.doubling_ability;
    if (wide_miracle_index(id) >= 0) return player.wide_ability;
    return 0;
}
std::span<const godfield::legacy::CardId> retained_ids(bool absorption, bool status) noexcept {
    if (status) return godfield::legacy::status_retained_type_ids;
    if (absorption) return godfield::legacy::hp_absorption_retained_type_ids;
    return godfield::legacy::retained_type_ids;
}
std::uint64_t retained_total(const godfield::legacy::Player& player,
                             std::span<const godfield::legacy::CardId> ids) noexcept {
    std::uint64_t count = 0;
    for (const auto id : ids) count += retained_count(player, id);
    return count;
}

}

int main(int argc, char** argv) {
    try {
        if (argc > 5) throw std::invalid_argument("too many arguments");
        const std::uint64_t games = argc > 1 ? number(argv[1]) : 100000;
        const std::uint64_t threads = argc > 2 ? number(argv[2]) : 1;
        const std::uint64_t seed = argc > 3 ? number(argv[3]) : 42;
        const std::string_view selected_profile = argc > 4 ? argv[4] : "neutral";
        constexpr std::array<std::string_view, 36> profiles{
            "neutral", "miracles", "elements", "chance", "reflection", "disease", "harms", "celestial", "offensive-miracles", "defensive-miracles", "attribute-change", "attribute-removal", "counter", "counter-extensions", "money-counter", "magic-reflection", "any-reflection", "mana-weapon", "magic-block", "additive-miracles", "doubling-miracles", "wide-miracles", "magic-free", "forgetting", "exchange", "revival", "item-removal", "hp-absorption", "self-damage", "pestle-mortar", "selling", "buying", "hp-gamble", "magic-bounce", "sundry-boost", "status-miracles"};
        const auto position = std::find(profiles.begin(), profiles.end(), selected_profile);
        if (position == profiles.end())
            throw std::invalid_argument("profile must be neutral, miracles, elements, chance, reflection, disease, harms, celestial, offensive-miracles, defensive-miracles, attribute-change, attribute-removal, counter, counter-extensions, money-counter, magic-reflection, any-reflection, mana-weapon, magic-block, additive-miracles, doubling-miracles, wide-miracles, magic-free, forgetting, exchange, revival, item-removal, hp-absorption, self-damage, pestle-mortar, selling, buying, hp-gamble, magic-bounce, sundry-boost, or status-miracles");
        const auto level = position - profiles.begin();
        const godfield::legacy::Config config{512, 10, level >= 1, level >= 2,
            level >= 3, level >= 4, level >= 5, level >= 6, level >= 7, level >= 8, level >= 9, level >= 10, level >= 11, level >= 12, level >= 13, level >= 14, level >= 15, level >= 16, level >= 17, level >= 18, level >= 19, level >= 20, level >= 21, level >= 22, level >= 23, level >= 24, level >= 25, level >= 26, level >= 27, level >= 28, level >= 29, level >= 30, level >= 31, level >= 32, level >= 33, level >= 34, level >= 35};
        const auto retained_order = retained_ids(config.hp_absorption, config.status_miracles);
        const std::string_view ruleset_name = godfield::legacy::Env(config).profile();
        int catalog_weight = 0;
        for (const auto& card : godfield::legacy::catalog()) catalog_weight += card.weight;
        if (config.miracles) for (const auto& card : godfield::legacy::miracles()) catalog_weight += card.weight;
        if (config.elements) for (const auto& card : godfield::legacy::element_catalog()) catalog_weight += card.card.weight;
        if (config.chance) for (const auto& card : godfield::legacy::chance_catalog()) catalog_weight += card.card.weight;
        if (config.reflection) for (const auto& card : godfield::legacy::reaction_catalog()) catalog_weight += card.card.weight;
        if (config.disease) for (const auto& card : godfield::legacy::disease_catalog()) catalog_weight += card.card.weight;
        if (config.harms) for (const auto& card : godfield::legacy::harm_catalog()) catalog_weight += card.card.weight;
        if (config.celestial) for (const auto& card : godfield::legacy::celestial_catalog()) catalog_weight += card.card.weight;
        if (config.offensive_miracles) for (const auto& miracle : godfield::legacy::offensive_miracles()) catalog_weight += miracle.weight;
        if (config.defensive_miracles) for (const auto& miracle : godfield::legacy::defensive_miracles()) catalog_weight += miracle.weight;
        if (config.attribute_change) for (const auto& card : godfield::legacy::attribute_change_catalog()) catalog_weight += card.card.weight;
        if (config.attribute_removal) for (const auto& card : godfield::legacy::attribute_removal_catalog()) catalog_weight += card.card.weight;
        if (config.counters) for (const auto& card : godfield::legacy::counter_catalog()) catalog_weight += card.card.weight;
        if (config.counter_extensions) for (const auto& card : godfield::legacy::counter_extensions_catalog()) catalog_weight += card.card.weight;
        if (config.money_counter) for (const auto& card : godfield::legacy::money_counter_catalog()) catalog_weight += card.card.weight;
        if (config.magic_reflection) for (const auto& card : godfield::legacy::magic_reflection_catalog()) catalog_weight += card.card.weight;
        if (config.any_reflection) for (const auto& card : godfield::legacy::any_reflection_catalog()) catalog_weight += card.card.weight;
        if (config.mana_weapon) for (const auto& card : godfield::legacy::mana_weapon_catalog()) catalog_weight += card.card.weight;
        if (config.magic_block) for (const auto& card : godfield::legacy::magic_block_catalog()) catalog_weight += card.card.weight;
        if (config.additive_miracles) for (const auto& miracle : godfield::legacy::additive_miracles()) catalog_weight += miracle.weight;
        if (config.doubling_miracles) for (const auto& miracle : godfield::legacy::doubling_miracles()) catalog_weight += miracle.weight;
        if (config.wide_miracles) for (const auto& miracle : godfield::legacy::wide_miracles()) catalog_weight += miracle.weight;
        if (config.magic_free) for (const auto& card : godfield::legacy::magic_free_catalog()) catalog_weight += card.card.weight;
        if (config.forgetting) for (const auto& card : godfield::legacy::forgetting_catalog()) catalog_weight += card.card.weight;
        if (config.exchanges) for (const auto& card : godfield::legacy::exchange_catalog()) catalog_weight += card.card.weight;
        if (config.revival) for (const auto& card : godfield::legacy::revival_catalog()) catalog_weight += card.card.weight;
        if (config.item_removal) for (const auto& card : godfield::legacy::item_removal_catalog()) catalog_weight += card.card.weight;
        if (config.hp_absorption) {
            for (const auto& card : godfield::legacy::hp_absorption_catalog()) catalog_weight += card.card.weight;
            for (const auto& miracle : godfield::legacy::hp_absorption_miracles()) catalog_weight += miracle.weight;
        }
        if (config.self_damage) for (const auto& card : godfield::legacy::self_damage_catalog()) catalog_weight += card.card.weight;
        if (config.pestle_mortar) {
            for (const auto& card : godfield::legacy::pestle_catalog()) catalog_weight += card.card.weight;
            for (const auto& card : godfield::legacy::mortar_catalog()) catalog_weight += card.card.weight;
        }
        if (config.selling) for (const auto& card : godfield::legacy::selling_catalog()) catalog_weight += card.card.weight;
        if (config.buying) for (const auto& card : godfield::legacy::buying_catalog()) catalog_weight += card.card.weight;
        if (config.hp_gamble) for (const auto& card : godfield::legacy::hp_gamble_catalog()) catalog_weight += card.card.weight;
        if (config.magic_bounce) for (const auto& card : godfield::legacy::magic_bounce_catalog()) catalog_weight += card.card.weight;
        if (config.sundry_boost) for (const auto& card : godfield::legacy::sundry_boost_catalog()) catalog_weight += card.card.weight;
        if (config.status_miracles) for (const auto& card : godfield::legacy::status_miracles()) catalog_weight += card.weight;
        const auto trade_price = [&config](godfield::legacy::CardId id) noexcept {
            if (config.status_miracles) if (const auto* card = godfield::legacy::find_status_miracle(id)) return card->price;
            if (config.sundry_boost) if (const auto* card = godfield::legacy::find_sundry_boost_card(id)) return card->card.price;
            if (config.magic_bounce) if (const auto* card = godfield::legacy::find_magic_bounce_card(id)) return card->card.price;
            if (config.hp_gamble) if (const auto* card = godfield::legacy::find_hp_gamble_card(id)) return card->card.price;
            if (config.buying) if (const auto* card = godfield::legacy::find_buying_card(id)) return card->card.price;
            return godfield::legacy::sale_price(id);
        };
        auto glory_sources = config.harms ? std::count_if(godfield::legacy::harm_catalog().begin(),
            godfield::legacy::harm_catalog().end(), [](const auto& card) {
                return card.harm == godfield::legacy::Harm::glory;
            }) : 0;
        if (config.celestial)
            glory_sources += std::count_if(godfield::legacy::celestial_catalog().begin(),
                godfield::legacy::celestial_catalog().end(), [](const auto& card) {
                    return card.harm == godfield::legacy::Harm::glory;
                });
        if (config.offensive_miracles)
            glory_sources += std::count_if(godfield::legacy::offensive_miracles().begin(),
                godfield::legacy::offensive_miracles().end(), [](const auto& miracle) {
                    return miracle.harm == godfield::legacy::Harm::glory;
                });
        if (config.counter_extensions)
            glory_sources += std::count_if(godfield::legacy::counter_extensions_catalog().begin(),
                godfield::legacy::counter_extensions_catalog().end(), [](const auto& counter) {
                    return counter.harm == godfield::legacy::Harm::glory;
                });
        if (!games || !threads || threads > 256)
            throw std::invalid_argument("games must be positive; threads must be in [1,256]");
        const auto maximum = std::numeric_limits<std::uint64_t>::max();
        if (games - 1 > maximum - seed)
            throw std::invalid_argument("game seed range overflows uint64");
        // HP/resource deltas and applied armor reductions fit 512 per decision.
        // Counter raw damage may exceed 32 bits, but is never summed as a statistic.
        // v21 MAGICAL power can reach597, and v23 uses conservative1194.
        // Their per-decision statistic guards are1024 and2048 respectively.
        // Queue events are bounded by the 32-card counter queue and also fit 512.
        if (games > maximum / (static_cast<std::uint64_t>(config.max_decisions) * (config.magic_free ? 2048 : config.doubling_miracles ? 1024 : 512)))
            throw std::invalid_argument("maximum statistics count overflows uint64");

        std::vector<Stats> stats(threads);
        std::vector<std::jthread> workers;
        workers.reserve(threads);
        const auto start = std::chrono::steady_clock::now();
        for (std::uint64_t worker = 0; worker < threads; ++worker) {
            workers.emplace_back([&, worker] {
                auto& output = stats[worker];
                try {
                    godfield::legacy::Env env(config);
                    const auto quotient = games / threads;
                    const auto remainder = games % threads;
                    const auto first = quotient * worker + std::min(worker, remainder);
                    const auto assigned = quotient + (worker < remainder ? 1 : 0);
                    for (std::uint64_t local = 0; local < assigned; ++local) {
                        const auto game_seed = seed + first + local;
                        env.reset(game_seed);
                        auto policy_rng = mix(game_seed ^ 0xa0761d6478bd642fULL);
                        std::array<float, 2> terminal_rewards{};
                        const auto learned_at_start = output.miracle_first_uses;
                        const auto forgotten_at_start = output.forgetting_removed;
                        std::uint64_t forgetting_digest = 0, exchange_digest = 0, revival_digest = 0, item_removal_digest = 0, hp_absorption_digest = 0, self_damage_digest = 0, pestle_digest = 0, sale_digest = 0, buy_digest = 0, hp_gamble_digest = 0, magic_card_bounce_digest = 0, sundry_boost_digest = 0, status_digest = 0;
                        bool sundry_boost_pending = false;
                        std::uint64_t counter_order_digest = 0;
                        std::uint64_t magic_reflection_digest = 0, game_magic_reflections = 0;
                        std::uint64_t any_reflection_digest = 0, game_any_reflections = 0;
                        std::uint64_t mana_weapon_digest = 0, game_mana_weapon_attacks = 0;
                        std::uint64_t magic_block_digest = 0, game_magic_blocks = 0;
                        std::uint64_t additive_action_digest = 0, doubling_action_digest = 0, wide_action_digest = 0, magic_free_digest = 0;
                        while (!env.done()) {
                            const auto action = env.sample_action(policy_rng);
                            if (config.counter_extensions && action.counter_count)
                                counter_order_digest = mix(counter_order_digest ^ action.counter_order
                                    ^ (static_cast<std::uint64_t>(action.counter_count) << 56)
                                    ^ static_cast<std::uint64_t>(env.state().decisions));
                            const auto& before = env.state();
                            const auto actor = before.actor;
                            const auto& player = before.players[actor];
                            const bool sale_action = action.kind == godfield::legacy::ActionKind::sell;
                            const bool incoming_sale = before.pending.sale_card != 0;
                            const auto sale_source = incoming_sale ? before.pending.source : actor;
                            const auto sale_target = incoming_sale ? before.pending.target : action.target;
                            godfield::legacy::CardId sold_item = incoming_sale ? before.pending.sale_item : 0;
                            if (sale_action) for (int slot = 0; slot < player.hand_count; ++slot)
                                if (action.slots & (1u << slot)) sold_item = player.hand[slot];
                            const int sold_price = sold_item ? trade_price(sold_item) : -1;
                            const std::array<int, 3> sale_target_before = (sale_action || incoming_sale) && sale_target < 2
                                ? std::array<int, 3>{before.players[sale_target].money, before.players[sale_target].mp, before.players[sale_target].hp}
                                : std::array<int, 3>{};
                            const int sale_source_money_before = (sale_action || incoming_sale) && sale_source < 2
                                ? before.players[sale_source].money : 0;
                            const int sale_target_hand_before = (sale_action || incoming_sale) && sale_target < 2
                                ? before.players[sale_target].hand_count : 0;
                            const bool buy_action = action.kind == godfield::legacy::ActionKind::buy;
                            const bool purchase_before = before.phase == godfield::legacy::Phase::purchase;
                            const bool purchase_action = action.kind == godfield::legacy::ActionKind::buy_decline
                                || action.kind == godfield::legacy::ActionKind::buy_accept;
                            const bool incoming_buy = before.pending.buy_card != 0;
                            const auto buy_source = incoming_buy ? before.pending.source : actor;
                            const auto buy_target = incoming_buy ? before.pending.target : action.target;
                            const auto offer_before = before.last_buy;
                            std::array<godfield::legacy::CardId, godfield::legacy::hand_capacity> buy_hand_before{};
                            std::uint8_t buy_population_before = 0;
                            if ((buy_action || incoming_buy) && buy_target < 2) {
                                const auto& owner = before.players[buy_target];
                                for (int slot = 0; slot < owner.hand_count; ++slot) {
                                    if (buy_action && buy_source == buy_target && slot == action.base) continue;
                                    buy_hand_before[buy_population_before++] = owner.hand[slot];
                                }
                            }
                            const int buyer_money_before = (buy_action || incoming_buy) && buy_source < 2
                                ? before.players[buy_source].money : 0;
                            const int buyer_hand_before = (buy_action || incoming_buy) && buy_source < 2
                                ? before.players[buy_source].hand_count : 0;
                            const int seller_money_before = (buy_action || incoming_buy) && buy_target < 2
                                ? before.players[buy_target].money : 0;
                            const bool hp_gamble_action = action.kind == godfield::legacy::ActionKind::use
                                && action.base < player.hand_count && godfield::legacy::find_hp_gamble_card(player.hand[action.base]);
                            const auto incoming_hp_gamble = before.pending.hp_gamble_card;
                            const auto gamble_source = incoming_hp_gamble ? before.pending.source : actor;
                            const auto gamble_target = incoming_hp_gamble ? before.pending.target : action.target;
                            const int gamble_target_hp_before = (hp_gamble_action || incoming_hp_gamble) && gamble_target < 2
                                ? before.players[gamble_target].hp : 0;
                            const int gamble_delta_before = incoming_hp_gamble ? static_cast<int>(before.pending.amount) : 0;
                            const auto gamble_history_before = before.last_hp_gamble;
                            const bool exchange_action = action.kind == godfield::legacy::ActionKind::exchange;
                            const std::array<int, 3> resources_before{player.hp, player.mp, player.money};
                            const auto decisions_before = before.decisions;
                            const auto turn_owner_before = before.turn_owner;
                            std::array<int, 2> revival_cards_before{};
                            int discarded_revival_cards = 0;
                            if (config.revival) for (int seat = 0; seat < 2; ++seat)
                                for (int slot = 0; slot < before.players[seat].hand_count; ++slot)
                                    if (godfield::legacy::find_revival_card(before.players[seat].hand[slot])) {
                                        ++revival_cards_before[seat];
                                        discarded_revival_cards += seat == actor
                                            && action.kind == godfield::legacy::ActionKind::discard && (action.slots & (1u << slot));
                                    }
                            const bool item_removal_use = action.kind == godfield::legacy::ActionKind::use
                                && action.base < player.hand_count && godfield::legacy::find_item_removal_card(player.hand[action.base]);
                            const auto incoming_item_removal = before.pending.item_removal_card;
                            const auto removal_source = incoming_item_removal ? before.pending.source : actor;
                            const auto removal_target = incoming_item_removal ? before.pending.target : action.target;
                            std::array<godfield::legacy::CardId, godfield::legacy::hand_capacity> removal_hand_before{};
                            std::uint8_t removal_population_before = 0;
                            if ((incoming_item_removal || item_removal_use) && removal_target < 2) {
                                const auto& victim = before.players[removal_target];
                                for (int slot = 0; slot < victim.hand_count; ++slot) {
                                    // Self240 is consumed before its own selection.
                                    if (item_removal_use && removal_target == actor && slot == action.base) continue;
                                    removal_hand_before[removal_population_before++] = victim.hand[slot];
                                }
                            }
                            const bool forget_use = action.kind == godfield::legacy::ActionKind::use
                                && action.base < player.hand_count && godfield::legacy::find_forgetting_card(player.hand[action.base]);
                            const auto incoming_forget = before.pending.forget_card;
                            const auto forget_source = incoming_forget ? before.pending.source : actor;
                            const auto forget_target = incoming_forget ? before.pending.target : action.target;
                            std::array<std::uint64_t, godfield::legacy::status_retained_type_ids.size()> forget_counts_before{};
                            if ((incoming_forget || forget_use) && forget_target < 2)
                                for (std::size_t i = 0; i < retained_order.size(); ++i)
                                    forget_counts_before[i] = retained_count(before.players[forget_target], retained_order[i]);
                            const auto forget_population_before = (incoming_forget || forget_use) && forget_target < 2
                                ? retained_total(before.players[forget_target], retained_order) : 0;
                            const bool ordered_defense = action.kind == godfield::legacy::ActionKind::defend_ordered;
                            const bool free_cast_action = action.kind == godfield::legacy::ActionKind::cast_ordered;
                            std::uint16_t response_slots = action.slots;
                            godfield::legacy::CardId ordered_defensive_id = 0, ordered_block_id = 0;
                            bool ordered_defensive_held = false, ordered_any_reflection = false;
                            if (ordered_defense) for (int i = 0; i < action.extra_count; ++i) {
                                const auto ref = action.extras[i];
                                const auto id = ref < player.hand_count ? player.hand[ref]
                                    : ref == 20 || ref == 21 ? godfield::legacy::defensive_miracles()[ref - 20].id : 0;
                                if (ref < player.hand_count) response_slots |= static_cast<std::uint16_t>(1u << ref);
                                if (godfield::legacy::find_defensive_miracle(id)) {
                                    ordered_defensive_id = id;
                                    ordered_defensive_held = ref >= 20;
                                }
                                ordered_any_reflection |= godfield::legacy::find_any_reflection_card(id) != nullptr;
                                if (before.pending.miracle && godfield::legacy::find_magic_block_card(id)) ordered_block_id = id;
                            }
                            const bool any_reflection_action = ordered_any_reflection
                                || (action.kind == godfield::legacy::ActionKind::defend
                                    && action.base < player.hand_count
                                    && godfield::legacy::find_any_reflection_card(player.hand[action.base]));
                            const bool magic_block_action = ordered_block_id
                                || (action.kind == godfield::legacy::ActionKind::defend
                                    && action.base < player.hand_count
                                    && godfield::legacy::find_magic_block_card(player.hand[action.base]));
                            const auto selected_magic_block = ordered_block_id ? ordered_block_id
                                : magic_block_action ? player.hand[action.base] : 0;
                            const bool setter_attack = attribute_attack(action.kind);
                            const bool ordered_attack = action.kind == godfield::legacy::ActionKind::attack_ordered;
                            const bool weapon_attack = action.kind == godfield::legacy::ActionKind::attack || setter_attack || ordered_attack;
                            const auto attack_id = weapon_attack && action.base < player.hand_count
                                ? player.hand[action.base] : 0;
                            const bool setter_base = weapon_attack && godfield::legacy::find_attribute_change_card(attack_id);
                            const bool mana_weapon_base = weapon_attack && godfield::legacy::find_mana_weapon_card(attack_id);
                            const bool chance_attack = chance_rate(attack_id) > 0;
                            const auto probabilistic_element = chance_element(attack_id);
                            auto ordered_element = chance_element(attack_id);
                            int ordered_setters = 0;
                            bool ordered_used_magic = false;
                            if (ordered_attack) for (int i = 0; i < action.extra_count; ++i) {
                                const auto ref = action.extras[i];
                                godfield::legacy::CardId id = 0;
                                if (ref < player.hand_count) id = player.hand[ref];
                                else if (ref >= 16 && ref < 16 + godfield::legacy::additive_miracle_count)
                                    id = godfield::legacy::additive_miracles()[ref - 16].id;
                                else if (config.doubling_miracles && ref == 18) id = godfield::legacy::doubling_miracles()[0].id;
                                else if (config.wide_miracles && ref == 19) id = godfield::legacy::wide_miracles()[0].id;
                                else throw std::runtime_error("invalid sampled ordered reference");
                                ordered_used_magic |= godfield::legacy::find_additive_miracle(id)
                                    || godfield::legacy::find_doubling_miracle(id) || godfield::legacy::find_wide_miracle(id);
                                if (ordered_used_magic && godfield::legacy::find_magic_free_card(id)) continue;
                                if (const auto* setter = godfield::legacy::find_attribute_change_card(id)) {
                                    ordered_element = setter->element;
                                    ++ordered_setters;
                                } else ordered_element = combine_element(ordered_element, chance_element(id));
                            }
                            const bool additive_recast = action.kind == godfield::legacy::ActionKind::cast_additive;
                            const bool defensive_recast = action.kind == godfield::legacy::ActionKind::defend_cast;
                            const bool status_recast = action.kind == godfield::legacy::ActionKind::cast_status;
                            const bool free_cast_held = free_cast_action && action.base >= 16;
                            const bool recast = action.kind == godfield::legacy::ActionKind::cast || defensive_recast
                                || additive_recast || status_recast || free_cast_held || ordered_defensive_held;
                            const auto free_cast_id = !free_cast_action ? 0
                                : action.base < player.hand_count ? player.hand[action.base]
                                : action.base == 16 || action.base == 17 ? godfield::legacy::additive_miracles()[action.base - 16].id
                                : action.base >= 48 && action.base < 48 + godfield::legacy::status_miracle_count
                                    ? godfield::legacy::status_miracles()[action.base - 48].id
                                : action.base >= 32 ? spell_id_at(action.base - 32) : 0;
                            const auto used_miracle = spell_view(free_cast_action ? free_cast_id
                                : ordered_defense ? ordered_defensive_id
                                : status_recast ? godfield::legacy::status_miracles()[action.base].id
                                : additive_recast ? godfield::legacy::additive_miracles()[action.base].id
                                : defensive_recast ? godfield::legacy::defensive_miracles()[action.base].id
                                : recast ? spell_id_at(action.base)
                                : (action.kind == godfield::legacy::ActionKind::use
                                    || action.kind == godfield::legacy::ActionKind::defend)
                                    && action.base < player.hand_count ? player.hand[action.base] : 0);
                            const bool damage_cast = used_miracle.effect == godfield::legacy::Effect::damage;
                            const bool offensive_cast = damage_cast && !used_miracle.additive && !used_miracle.hp_absorption;
                            const int mana_before = player.mp;
                            const int ability_slot = used_miracle.index;
                            const auto known_before = ability_slot < 0 || used_miracle.additive ? 0 : used_miracle.status
                                ? player.status_abilities[ability_slot] : used_miracle.defensive
                                ? player.defensive_abilities[ability_slot] : player.abilities[ability_slot];
                            const auto wide_known_before = player.wide_ability;
                            const auto doubling_known_before = player.doubling_ability;
                            const auto additive_known_before = player.additive_abilities[0] + player.additive_abilities[1];

                            const bool elemental_response = before.phase == godfield::legacy::Phase::response
                                && before.pending.kind == godfield::legacy::Effect::damage
                                && before.pending.element != godfield::legacy::Element::neutral;
                            const std::int64_t incoming_damage = before.pending.amount;
                            // Capture before step: state() references the mutable environment.
                            const auto incoming_element = before.pending.element;
                            const auto incoming_miracle = before.pending.miracle;
                            const auto incoming_magic_free = before.pending.free_card;
                            const auto* incoming_status = godfield::legacy::find_status_miracle(incoming_miracle);
                            const auto incoming_absorption = before.pending.hp_absorption_card;
                            const auto incoming_self_damage = before.pending.self_damage_card;
                            const auto incoming_pestle = before.pending.pestle_card;
                            const auto incoming_wide = before.pending.wide_miracle;
                            const bool incoming_magic = incoming_miracle != 0;
                            const auto incoming_kind = before.pending.kind;
                            const auto incoming_harm = before.pending.harm;
                            const bool incoming_from_ability = before.pending.from_ability;
                            const auto incoming_origin = before.pending.origin_source;
                            const auto incoming_hit_rate = before.pending.hit_rate;
                            const bool incoming_percentage = incoming_hit_rate != 0;
                            const bool incoming_neutralized = before.pending.attribute_removed;
                            const bool incoming_response = before.phase == godfield::legacy::Phase::response;
                            const auto incoming_counter_id = before.pending.counter_card;
                            const bool incoming_counter = incoming_counter_id != 0;
                            const bool incoming_money_counter = before.pending.kind == godfield::legacy::Effect::absorb_money;
                            const auto incoming_money_source = before.pending.source;
                            const auto incoming_money_target = before.pending.target;
                            const int counter_source_money_before = incoming_money_counter && incoming_money_source < 2
                                ? before.players[incoming_money_source].money : 0;
                            const int counter_target_money_before = incoming_money_counter && incoming_money_target < 2
                                ? before.players[incoming_money_target].money : 0;
                            int selected_counters = 0;
                            if (incoming_response && (action.kind == godfield::legacy::ActionKind::defend
                                || action.kind == godfield::legacy::ActionKind::defend_cast || ordered_defense))
                                for (int slot = 0; slot < player.hand_count; ++slot)
                                    if ((response_slots & (1u << slot))
                                        && (godfield::legacy::find_counter_card(player.hand[slot])
                                            || godfield::legacy::find_counter_extension_card(player.hand[slot])
                                            || godfield::legacy::find_money_counter_card(player.hand[slot])))
                                        ++selected_counters;
                            int selected_defense = 0;
                            if (before.phase == godfield::legacy::Phase::response && before.pending.kind == godfield::legacy::Effect::damage && (action.kind == godfield::legacy::ActionKind::defend
                                || action.kind == godfield::legacy::ActionKind::defend_cast || ordered_defense)) {
                                for (int slot = 0; slot < player.hand_count; ++slot)
                                    if (response_slots & (1u << slot))
                                        if (const auto* card = artifact(player.hand[slot])) selected_defense += card->defense;
                            }

                            // A self-targeted miracle resolves immediately; an
                            // opponent-targeted one resolves on the response step.
                            const bool miracle_response = before.phase == godfield::legacy::Phase::response
                                && before.pending.miracle != 0;
                            const auto effect_target = miracle_response ? before.pending.target : action.target;
                            const auto resolving_spell = miracle_response ? spell_view(before.pending.miracle) : used_miracle;
                            const bool resolution_candidate = !any_reflection_action && (miracle_response
                                || (used_miracle.id && !used_miracle.defensive
                                    && !used_miracle.hit_rate && action.target == actor));
                            const bool resolving_offensive_miracle = resolution_candidate
                                && resolving_spell.effect == godfield::legacy::Effect::damage && !resolving_spell.additive && !resolving_spell.hp_absorption;
                            const int hp_before = resolution_candidate && effect_target < 2
                                ? before.players[effect_target].hp : 0;
                            const int money_before = resolution_candidate && effect_target < 2
                                ? before.players[effect_target].money : 0;
                            const bool pestle_action = attack_id && godfield::legacy::find_pestle_card(attack_id);
                            std::uint8_t first_mortar_seat = godfield::legacy::no_slot;
                            int selected_mortar_discards = 0;
                            if (config.pestle_mortar) for (std::uint8_t seat = 0; seat < 2; ++seat) {
                                const auto& owner = before.players[seat];
                                for (int slot = 0; slot < owner.hand_count; ++slot) {
                                    if (!godfield::legacy::find_mortar_card(owner.hand[slot])) continue;
                                    if (!owner.dead && first_mortar_seat == godfield::legacy::no_slot) first_mortar_seat = seat;
                                    if (seat == actor && action.kind == godfield::legacy::ActionKind::discard
                                        && (action.slots & (1u << slot))) ++selected_mortar_discards;
                                }
                            }
                            godfield::legacy::CardId selected_magic_bounce = 0;
                            int ordinary_magic_bounce_cards = 0, magic_bounce_extra_uses = 0;
                            if (config.magic_bounce) {
                                if (incoming_response && (action.kind == godfield::legacy::ActionKind::defend || ordered_defense)) {
                                    for (int slot = 0; slot < player.hand_count; ++slot) {
                                        const bool selected = (response_slots & (1u << slot))
                                            || (!ordered_defense && action.base == slot);
                                        if (selected && godfield::legacy::find_magic_bounce_card(player.hand[slot])) {
                                            if (incoming_magic) selected_magic_bounce = player.hand[slot];
                                            else ++ordinary_magic_bounce_cards;
                                        }
                                    }
                                }
                                if (weapon_attack) {
                                    if (ordered_attack) {
                                        for (int i = 0; i < action.extra_count; ++i) {
                                            const auto slot = action.extras[i];
                                            if (slot < player.hand_count && player.hand[slot] == 21) ++magic_bounce_extra_uses;
                                        }
                                    } else for (int slot = 0; slot < player.hand_count; ++slot)
                                        if ((action.slots & (1u << slot)) && player.hand[slot] == 21) ++magic_bounce_extra_uses;
                                }
                            }
                            int selected_sundry_boosts = 0, discarded_sundry_boosts = 0;
                            const bool incoming_sundry_boost = sundry_boost_pending && incoming_response && !incoming_counter;
                            const auto extra_id = [&player](std::uint8_t ref) -> godfield::legacy::CardId {
                                if (ref < player.hand_count) return player.hand[ref];
                                if (ref == 16 || ref == 17) return godfield::legacy::additive_miracles()[ref - 16].id;
                                if (ref == 18) return godfield::legacy::doubling_miracles()[0].id;
                                if (ref == 19) return godfield::legacy::wide_miracles()[0].id;
                                throw std::runtime_error("invalid sampled physical boost reference");
                            };
                            if (config.sundry_boost) {
                                if (weapon_attack && ordered_attack) for (int i = 0; i < action.extra_count; ++i)
                                    selected_sundry_boosts += extra_id(action.extras[i]) == 238;
                                for (int slot = 0; slot < player.hand_count; ++slot) if (player.hand[slot] == 238) {
                                    if (weapon_attack && !ordered_attack && (action.slots & (1u << slot))) ++selected_sundry_boosts;
                                    if (action.kind == godfield::legacy::ActionKind::discard && (action.slots & (1u << slot))) ++discarded_sundry_boosts;
                                }
                            }
                            std::int64_t replayed_boost_raw = 0;
                            int replayed_mana_left = mana_before, boost_mana_resets = 0, resets_after_boost = 0;
                            bool boost_has_double = false, boost_has_wide = false;
                            auto boost_element = godfield::legacy::Element::neutral;
                            if (selected_sundry_boosts) {
                                const auto* base = artifact(attack_id);
                                if (!base || base->kind != godfield::legacy::Kind::weapon || chance_rate(attack_id))
                                    throw std::runtime_error("SUNDRY boost attached to an invalid base");
                                replayed_boost_raw = mana_weapon_base ? 2 * mana_before : base->attack;
                                boost_element = ordered_attack ? ordered_element : chance_element(attack_id);
                                if (ordered_attack) {
                                    int free_predecessor = -2;
                                    for (int i = 0; i < action.extra_count; ++i)
                                        if (godfield::legacy::find_magic_free_card(extra_id(action.extras[i]))) { free_predecessor = i - 1; break; }
                                    bool saw_magic = false, boost_since_reset = false;
                                    for (int i = 0; i < action.extra_count; ++i) {
                                        const auto id = extra_id(action.extras[i]);
                                        const auto* additive = godfield::legacy::find_additive_miracle(id);
                                        const auto* doubling = godfield::legacy::find_doubling_miracle(id);
                                        const auto* wide = godfield::legacy::find_wide_miracle(id);
                                        if (additive || doubling || wide) {
                                            const int cost = additive ? additive->cost : doubling ? doubling->cost : wide->cost;
                                            const bool paid = i != free_predecessor;
                                            if (paid) replayed_mana_left -= cost;
                                            if (mana_weapon_base && paid) {
                                                ++boost_mana_resets;
                                                resets_after_boost += boost_since_reset;
                                                boost_since_reset = false;
                                                replayed_boost_raw = 2 * replayed_mana_left;
                                            }
                                            saw_magic = true;
                                            if (additive) replayed_boost_raw += additive->amount;
                                            if (doubling) { replayed_boost_raw *= 2; boost_has_double = true; }
                                            if (wide) boost_has_wide = true;
                                        } else if (!(saw_magic && godfield::legacy::find_magic_free_card(id))) {
                                            if (const auto* card = artifact(id)) replayed_boost_raw += card->boost;
                                            else throw std::runtime_error("unknown sampled physical boost item");
                                            boost_since_reset |= id == 238;
                                        }
                                    }
                                } else {
                                    for (int slot = 0; slot < player.hand_count; ++slot) if (action.slots & (1u << slot)) {
                                        const auto* card = artifact(player.hand[slot]);
                                        if (!card) throw std::runtime_error("unknown plain physical boost item");
                                        replayed_boost_raw += card->boost;
                                        boost_element = combine_element(boost_element, chance_element(player.hand[slot]));
                                    }
                                    if (setter_attack) boost_element = action.kind == godfield::legacy::ActionKind::attack_set_fire
                                        ? godfield::legacy::Element::fire : action.kind == godfield::legacy::ActionKind::attack_set_water
                                        ? godfield::legacy::Element::water : godfield::legacy::Element::neutral;
                                }
                                if (replayed_mana_left < 0 || replayed_boost_raw < 0)
                                    throw std::runtime_error("invalid replayed SUNDRY boost resource state");
                            }
                            const auto status_before = before.players;
                            std::uint64_t status_known_before = 0;
                            if (config.status_miracles) for (const auto count : player.status_abilities) status_known_before += count;
                            const auto result = env.step(action);
                            if (!result.valid)
                                throw std::runtime_error("sample_action generated an illegal action");
                            const auto& after = env.state();
                            if (result.spell_first_use != (used_miracle.id != 0 && !used_miracle.additive && !recast)
                                || result.spell_recast != (used_miracle.id != 0 && !used_miracle.additive && recast))
                                throw std::runtime_error("inconsistent spell use event");
                            if (result.defensive_first_use != (used_miracle.defensive && !recast)
                                || result.defensive_recast != (used_miracle.defensive && recast))
                                throw std::runtime_error("inconsistent defensive spell use event");
                            // Non-self magic bounce keeps the spell pending. Its
                            // resolution can finish the turn or launch a new counter.
                            const bool resolving_miracle = resolution_candidate
                                && (!miracle_response || after.phase == godfield::legacy::Phase::main
                                    || (config.counters && !incoming_counter && after.pending.counter_card != 0))
                                && (!result.magic_bounced || result.bounce_self) && !result.magic_reflected && !result.any_reflected && !result.magic_blocked;
                            // Forgetting can decrease the acting seat's inventory;
                            // add that removed copy back before counting new learning.
                            const bool actor_forgot = result.forget_removed && result.forgotten_from == actor;
                            const auto additive_learned = after.players[actor].additive_abilities[0]
                                + after.players[actor].additive_abilities[1]
                                + (actor_forgot && godfield::legacy::find_additive_miracle(result.forgotten_miracle) ? 1u : 0u)
                                - additive_known_before;
                            const auto additive_uses = result.additive_first_uses + result.additive_recasts;
                            if (result.ordered_attack != ordered_attack
                                || result.ordered_components != (ordered_attack ? action.extra_count : 0)
                                || additive_learned != result.additive_first_uses
                                || (!config.additive_miracles && (additive_uses || result.additive_mp_spent))
                                || (!config.additive_miracles && !config.doubling_miracles && result.ordered_attack)
                                || (used_miracle.additive && (additive_uses != 1 || result.additive_mp_spent != (free_cast_action ? 0 : used_miracle.cost)))
                                || (!ordered_attack && !used_miracle.additive && additive_uses))
                                throw std::runtime_error("inconsistent additive miracle event");
                            if ((ordered_attack || used_miracle.additive)
                                && mana_before - after.players[actor].mp != result.additive_mp_spent + result.doubling_mp_spent + result.wide_mp_spent + result.mana_weapon_mp_spent)
                                throw std::runtime_error("additive MP accounting mismatch");
                            const auto doubling_learned = after.players[actor].doubling_ability
                                + (actor_forgot && godfield::legacy::find_doubling_miracle(result.forgotten_miracle) ? 1u : 0u)
                                - doubling_known_before;
                            const auto doubling_uses = result.doubling_first_uses + result.doubling_recasts;
                            if (doubling_learned != result.doubling_first_uses
                                || result.doubling_mp_spent > doubling_uses * 6
                                || (!config.magic_free && result.doubling_mp_spent != doubling_uses * 6)
                                || doubling_uses > (config.magic_free ? 17 : 16)
                                || (doubling_uses && !ordered_attack)
                                || (!config.doubling_miracles && (doubling_uses || result.doubling_mp_spent)))
                                throw std::runtime_error("inconsistent doubling miracle event");
                            output.doubling_first_uses += result.doubling_first_uses;
                            output.doubling_recasts += result.doubling_recasts;
                            output.doubling_mp_spent += result.doubling_mp_spent;
                            output.doubling_learned += doubling_learned;
                            output.doubling_attacks += doubling_uses != 0;
                            output.doubling_uses += doubling_uses;
                            output.miracle_first_uses += result.doubling_first_uses;
                            output.miracle_recasts += result.doubling_recasts;
                            output.miracle_mp_spent += result.doubling_mp_spent;
                            output.learned_miracles += doubling_learned;
                            if (config.doubling_miracles && ordered_attack) {
                                doubling_action_digest = mix(doubling_action_digest ^ after.decisions
                                    ^ (static_cast<std::uint64_t>(action.base) << 32)
                                    ^ (static_cast<std::uint64_t>(action.target) << 40)
                                    ^ (static_cast<std::uint64_t>(action.extra_count) << 48));
                                for (int i = 0; i < action.extra_count; ++i)
                                    doubling_action_digest = mix(doubling_action_digest ^ action.extras[i]);
                                doubling_action_digest = mix(doubling_action_digest
                                    ^ static_cast<std::uint64_t>(result.doubling_first_uses)
                                    ^ (static_cast<std::uint64_t>(result.doubling_recasts) << 8)
                                    ^ (static_cast<std::uint64_t>(result.doubling_mp_spent) << 16));
                            }
                            const auto wide_learned = after.players[actor].wide_ability
                                + (actor_forgot && godfield::legacy::find_wide_miracle(result.forgotten_miracle) ? 1u : 0u)
                                - wide_known_before;
                            const auto wide_uses = result.wide_first_uses + result.wide_recasts;
                            if (wide_learned != result.wide_first_uses
                                || result.wide_mp_spent > wide_uses * 5
                                || (!config.magic_free && result.wide_mp_spent != wide_uses * 5)
                                || wide_uses > (config.magic_free ? 20 : 19)
                                || result.wide_attack != (wide_uses != 0)
                                || (result.wide_attack && (!ordered_attack || !result.chance_attempt || !result.chance_hit
                                    || after.last_chance.wide_miracle != 209 || after.last_chance.id != attack_id))
                                || (!config.wide_miracles && (wide_uses || result.wide_mp_spent || result.wide_attack)))
                                throw std::runtime_error("inconsistent wide miracle event");
                            output.wide_first_uses += result.wide_first_uses;
                            output.wide_recasts += result.wide_recasts;
                            output.wide_mp_spent += result.wide_mp_spent;
                            output.wide_learned += wide_learned;
                            output.wide_attacks += result.wide_attack;
                            output.wide_uses += wide_uses;
                            output.wide_chance_attempts += result.wide_attack && result.chance_attempt;
                            output.wide_chance_hits += result.wide_attack && result.chance_hit;
                            output.wide_forced_hits += result.wide_attack && result.chance_forced;
                            output.miracle_first_uses += result.wide_first_uses;
                            output.miracle_recasts += result.wide_recasts;
                            output.miracle_mp_spent += result.wide_mp_spent;
                            output.learned_miracles += wide_learned;
                            if (config.wide_miracles && ordered_attack) {
                                wide_action_digest = mix(wide_action_digest ^ after.decisions
                                    ^ (static_cast<std::uint64_t>(action.base) << 32)
                                    ^ (static_cast<std::uint64_t>(action.target) << 40)
                                    ^ (static_cast<std::uint64_t>(action.extra_count) << 48));
                                for (int i = 0; i < action.extra_count; ++i)
                                    wide_action_digest = mix(wide_action_digest ^ action.extras[i]);
                                wide_action_digest = mix(wide_action_digest ^ result.wide_first_uses
                                    ^ (static_cast<std::uint64_t>(result.wide_recasts) << 8)
                                    ^ (static_cast<std::uint64_t>(result.wide_mp_spent) << 16)
                                    ^ (static_cast<std::uint64_t>(result.wide_attack) << 32));
                            }
                            output.additive_first_uses += result.additive_first_uses;
                            output.additive_recasts += result.additive_recasts;
                            output.additive_mp_spent += result.additive_mp_spent;
                            output.additive_learned += additive_learned;
                            output.additive_single_attacks += used_miracle.additive;
                            output.additive_extra_uses += ordered_attack ? additive_uses : 0;
                            output.ordered_attacks += result.ordered_attack;
                            output.ordered_components += result.ordered_components;
                            output.miracle_first_uses += result.additive_first_uses;
                            output.miracle_recasts += result.additive_recasts;
                            output.miracle_mp_spent += result.additive_mp_spent;
                            output.learned_miracles += additive_learned;
                            if (config.additive_miracles && (ordered_attack || used_miracle.additive)) {
                                additive_action_digest = mix(additive_action_digest
                                    ^ static_cast<std::uint64_t>(after.decisions)
                                    ^ (static_cast<std::uint64_t>(action.kind) << 32)
                                    ^ (static_cast<std::uint64_t>(action.base) << 40)
                                    ^ (static_cast<std::uint64_t>(action.target) << 48)
                                    ^ (static_cast<std::uint64_t>(action.extra_count) << 56));
                                for (int i = 0; i < action.extra_count; ++i)
                                    additive_action_digest = mix(additive_action_digest ^ action.extras[i]);
                                additive_action_digest = mix(additive_action_digest
                                    ^ static_cast<std::uint64_t>(result.additive_first_uses)
                                    ^ (static_cast<std::uint64_t>(result.additive_recasts) << 8)
                                    ^ (static_cast<std::uint64_t>(result.additive_mp_spent) << 16));
                            }
                            if (result.mana_weapon_attack != mana_weapon_base
                                || (result.mana_weapon_attack && (!config.mana_weapon || used_miracle.id
                                    || result.mana_weapon_mp_spent + result.additive_mp_spent + result.doubling_mp_spent + result.wide_mp_spent != mana_before || after.players[actor].mp != 0
                                    || result.mana_weapon_power > (config.magic_free ? 1194 : config.doubling_miracles ? 597 : 423) || result.counter_auto_resolved))
                                || (!result.mana_weapon_attack && (result.mana_weapon_mp_spent || result.mana_weapon_power)))
                                throw std::runtime_error("inconsistent mana weapon event");
                            if (result.mana_weapon_attack) {
                                ++output.mana_weapon_attacks;
                                ++game_mana_weapon_attacks;
                                output.mana_weapon_mp_spent += result.mana_weapon_mp_spent;
                                output.mana_weapon_power_total += result.mana_weapon_power;
                                output.mana_weapon_zero_mp_attacks += mana_before == 0;
                                output.mana_weapon_zero_burn_attacks += result.mana_weapon_mp_spent == 0;
                                output.mana_weapon_zero_power_attacks += result.mana_weapon_power == 0;
                                output.mana_weapon_setter_attacks += result.attribute_attack;
                                // Base57 starts neutral; ordinary boosts cannot
                                // replace that neutral element. Setter outcomes
                                // remain available even if a self attack clears pending.
                                const auto element = result.attribute_attack ? result.attribute_element
                                    : godfield::legacy::Element::neutral;
                                mana_weapon_digest = mix(mana_weapon_digest
                                    ^ static_cast<std::uint64_t>(after.decisions)
                                    ^ (static_cast<std::uint64_t>(attack_id) << 32)
                                    ^ (static_cast<std::uint64_t>(action.slots) << 16));
                                mana_weapon_digest = mix(mana_weapon_digest
                                    ^ static_cast<std::uint64_t>(action.kind)
                                    ^ (static_cast<std::uint64_t>(actor) << 8)
                                    ^ (static_cast<std::uint64_t>(action.target) << 16)
                                    ^ (static_cast<std::uint64_t>(element) << 24)
                                    ^ (static_cast<std::uint64_t>(result.mana_weapon_mp_spent) << 32)
                                    ^ (static_cast<std::uint64_t>(result.mana_weapon_power) << 40));
                            }
                            if (result.free_cast != free_cast_action || result.ordered_defense != ordered_defense
                                || (!config.magic_free && (result.magic_free_cards || result.magic_free_mp_saved
                                    || result.magic_free_suffix || result.free_cast || result.ordered_defense))
                                || (result.magic_free_suffix && !ordered_defense))
                                throw std::runtime_error("inconsistent MAGIC_FREE event");
                            output.magic_free_cards += result.magic_free_cards;
                            output.magic_free_mp_saved += result.magic_free_mp_saved;
                            output.magic_free_suffixes += result.magic_free_suffix;
                            output.free_casts += result.free_cast;
                            output.ordered_defenses += result.ordered_defense;
                            if (incoming_response) {
                                output.magic_free_defense_cards += result.magic_free_cards;
                                output.magic_free_defense_mp_saved += result.magic_free_mp_saved;
                            } else if (free_cast_action) {
                                output.magic_free_cast_cards += result.magic_free_cards;
                                output.magic_free_cast_mp_saved += result.magic_free_mp_saved;
                            } else {
                                output.magic_free_attack_cards += result.magic_free_cards;
                                output.magic_free_attack_mp_saved += result.magic_free_mp_saved;
                            }
                            if (config.magic_free && (ordered_attack || free_cast_action || ordered_defense || result.magic_free_cards)) {
                                magic_free_digest = mix(magic_free_digest ^ after.decisions
                                    ^ (static_cast<std::uint64_t>(action.kind) << 32)
                                    ^ (static_cast<std::uint64_t>(action.base) << 40)
                                    ^ (static_cast<std::uint64_t>(action.target) << 48));
                                for (int i = 0; i < action.extra_count; ++i) magic_free_digest = mix(magic_free_digest ^ action.extras[i]);
                                magic_free_digest = mix(magic_free_digest ^ result.magic_free_cards
                                    ^ (static_cast<std::uint64_t>(result.magic_free_mp_saved) << 8)
                                    ^ (static_cast<std::uint64_t>(result.magic_free_suffix) << 24)
                                    ^ (static_cast<std::uint64_t>(result.spell_mp_spent) << 32));
                            }
                            const bool expected_forget_attempt = (forget_use && action.target == actor)
                                || (incoming_forget && !result.any_reflected);
                            if (result.forget_attempt != expected_forget_attempt
                                || (result.forget_attempt && (!config.forgetting || result.forgotten_from != forget_target
                                    || result.forget_population != forget_population_before
                                    || result.forget_removed != (result.forget_population != 0)))
                                || (result.forget_removed && (std::find(retained_order.begin(),
                                    retained_order.end(), result.forgotten_miracle)
                                        == retained_order.end()
                                    || retained_count(after.players[forget_target], result.forgotten_miracle) + 1
                                        != forget_counts_before[std::find(retained_order.begin(),
                                            retained_order.end(), result.forgotten_miracle)
                                            - retained_order.begin()]))
                                || (!result.forget_removed && result.forgotten_miracle))
                                throw std::runtime_error("inconsistent forgetting event");
                            output.forgetting_uses += forget_use;
                            output.forgetting_attempts += result.forget_attempt;
                            output.forgetting_removed += result.forget_removed;
                            output.forgetting_empty += result.forget_attempt && !result.forget_removed;
                            output.forgetting_self_targets += result.forget_attempt && forget_source == forget_target;
                            output.forgetting_opponent_targets += result.forget_attempt && forget_source != forget_target;
                            output.forgetting_reflections += incoming_forget && result.any_reflected;
                            if (result.forget_removed) {
                                output.forgetting_utility += godfield::legacy::find_miracle(result.forgotten_miracle) != nullptr;
                                output.forgetting_offensive += godfield::legacy::find_offensive_miracle(result.forgotten_miracle) != nullptr;
                                output.forgetting_defensive += godfield::legacy::find_defensive_miracle(result.forgotten_miracle) != nullptr;
                                output.forgetting_additive += godfield::legacy::find_additive_miracle(result.forgotten_miracle) != nullptr;
                                output.forgetting_doubling += godfield::legacy::find_doubling_miracle(result.forgotten_miracle) != nullptr;
                                output.forgetting_wide += godfield::legacy::find_wide_miracle(result.forgotten_miracle) != nullptr;
                                output.forgetting_hp_absorption += godfield::legacy::find_hp_absorption_miracle(result.forgotten_miracle) != nullptr;
                                output.forgetting_status_miracles += godfield::legacy::find_status_miracle(result.forgotten_miracle) != nullptr;
                            }
                            if (config.forgetting && (forget_use || incoming_forget)) {
                                forgetting_digest = mix(forgetting_digest ^ after.decisions
                                    ^ (static_cast<std::uint64_t>(forget_source) << 32)
                                    ^ (static_cast<std::uint64_t>(forget_target) << 40)
                                    ^ (static_cast<std::uint64_t>(result.any_reflected) << 48)
                                    ^ (static_cast<std::uint64_t>(result.forget_attempt) << 49));
                                forgetting_digest = mix(forgetting_digest ^ result.forget_population);
                                forgetting_digest = mix(forgetting_digest ^ result.forgotten_miracle);
                            }
                            const bool expected_item_removal = (item_removal_use && action.target == actor)
                                || (incoming_item_removal && !result.any_reflected);
                            if (result.item_removal_attempt != expected_item_removal
                                || (result.item_removal_attempt && (!config.item_removal || result.item_removed_from != removal_target
                                    || result.item_removal_population != removal_population_before
                                    || result.item_removed != (removal_population_before != 0)))
                                || (result.item_removed && (result.removed_item_slot >= removal_population_before
                                    || result.removed_item != removal_hand_before[result.removed_item_slot]))
                                || (!result.item_removed && (result.removed_item || result.removed_item_slot != godfield::legacy::no_slot)))
                                throw std::runtime_error("inconsistent item removal event");
                            output.item_removal_uses += item_removal_use;
                            output.item_removal_attempts += result.item_removal_attempt;
                            output.items_removed += result.item_removed;
                            output.item_removal_empty += result.item_removal_attempt && !result.item_removed;
                            output.item_removal_self_targets += result.item_removal_attempt && removal_source == removal_target;
                            output.item_removal_opponent_targets += result.item_removal_attempt && removal_source != removal_target;
                            output.item_removal_reflections += incoming_item_removal && result.any_reflected;
                            if (result.item_removed) {
                                if (const auto* card = artifact(result.removed_item)) {
                                    if (card->kind == godfield::legacy::Kind::weapon) ++output.item_removed_weapons;
                                    else if (card->kind == godfield::legacy::Kind::armor) ++output.item_removed_armor;
                                    else ++output.item_removed_other;
                                } else ++output.item_removed_miracles;
                                output.item_removed_revivals += godfield::legacy::find_revival_card(result.removed_item) != nullptr;
                                output.item_removed_mortars += godfield::legacy::find_mortar_card(result.removed_item) != nullptr;
                                output.sundry_boost_removed_cards += godfield::legacy::find_sundry_boost_card(result.removed_item) != nullptr;
                                output.removed_status_miracles += godfield::legacy::find_status_miracle(result.removed_item) != nullptr;
                            }
                            if (config.item_removal && (item_removal_use || incoming_item_removal)) {
                                item_removal_digest = mix(item_removal_digest ^ after.decisions
                                    ^ (static_cast<std::uint64_t>(removal_source) << 32)
                                    ^ (static_cast<std::uint64_t>(removal_target) << 40)
                                    ^ (static_cast<std::uint64_t>(result.any_reflected) << 48)
                                    ^ (static_cast<std::uint64_t>(result.item_removal_attempt) << 49));
                                item_removal_digest = mix(item_removal_digest ^ result.removed_item
                                    ^ (static_cast<std::uint64_t>(result.item_removal_population) << 16)
                                    ^ (static_cast<std::uint64_t>(result.removed_item_slot) << 24));
                            }
                            if (result.hp_absorption_resolved) {
                                const auto id = result.hp_absorption_card;
                                const bool magic = godfield::legacy::find_hp_absorption_miracle(id) != nullptr;
                                const auto element = incoming_response
                                    ? (result.attribute_removed ? godfield::legacy::Element::neutral : incoming_element)
                                    : ordered_attack ? ordered_element : result.attribute_attack ? result.attribute_element
                                    : used_miracle.id ? used_miracle.element : chance_element(attack_id);
                                const auto raw_limit = config.magic_free ? godfield::legacy::magic_free_damage_limit
                                    : config.doubling_miracles ? godfield::legacy::doubling_damage_limit : std::int64_t{480};
                                if (!config.hp_absorption || (!magic && !godfield::legacy::find_hp_absorption_card(id))
                                    || result.hp_absorption_source > 1 || result.hp_absorption_target > 1 || incoming_counter
                                    || result.hp_absorption_amount < 0 || result.hp_absorption_amount > raw_limit
                                    || result.hp_absorption_healing > result.hp_absorption_amount
                                    || result.magic_blocked || result.weapon_blocked || result.magic_reflected || result.any_reflected
                                    || (result.bounced && !result.bounce_self)
                                    || (element == godfield::legacy::Element::dark && result.hp_absorption_healing))
                                    throw std::runtime_error("inconsistent HP absorption resolution");
                                if (incoming_response && (id != incoming_absorption
                                    || result.hp_absorption_amount != std::max<std::int64_t>(0,
                                        incoming_damage - (result.bounced || result.reflected ? 0 : selected_defense))))
                                    throw std::runtime_error("HP absorption residual accounting mismatch");
                                ++output.hp_absorption_resolutions;
                                output.hp_absorption_weapon_resolutions += !magic;
                                output.hp_absorption_miracle_resolutions += magic;
                                output.hp_absorption_self_resolutions += result.hp_absorption_source == result.hp_absorption_target;
                                output.hp_absorption_zero_residual += result.hp_absorption_amount == 0;
                                output.hp_absorption_zero_healing += result.hp_absorption_healing == 0;
                                output.hp_absorption_healing += result.hp_absorption_healing;
                                output.hp_absorption_dark_resolutions += element == godfield::legacy::Element::dark;
                                output.hp_absorption_capped += element != godfield::legacy::Element::dark
                                    && result.hp_absorption_healing < result.hp_absorption_amount;
                                // Raw residual can be huge; record its maximum, never sum it.
                                output.hp_absorption_raw_max = std::max(output.hp_absorption_raw_max,
                                    static_cast<std::uint64_t>(result.hp_absorption_amount));
                                hp_absorption_digest = mix(hp_absorption_digest ^ after.decisions
                                    ^ (static_cast<std::uint64_t>(id) << 32)
                                    ^ (static_cast<std::uint64_t>(result.hp_absorption_source) << 48)
                                    ^ (static_cast<std::uint64_t>(result.hp_absorption_target) << 56));
                                hp_absorption_digest = mix(hp_absorption_digest ^ static_cast<std::uint64_t>(result.hp_absorption_amount));
                                hp_absorption_digest = mix(hp_absorption_digest ^ result.hp_absorption_healing);
                            } else if (result.hp_absorption_card || result.hp_absorption_amount || result.hp_absorption_healing) {
                                throw std::runtime_error("HP absorption payload without a resolution");
                            }
                            const bool self_damage_attack = attack_id && godfield::legacy::find_self_damage_card(attack_id);
                            output.self_damage_attacks += self_damage_attack;
                            output.self_damage_reflections += incoming_self_damage && (result.reflected || result.bounced);
                            if (result.self_damage_resolved) {
                                const auto raw_limit = config.magic_free ? godfield::legacy::magic_free_damage_limit
                                    : config.doubling_miracles ? godfield::legacy::doubling_damage_limit : std::int64_t{480};
                                if (!config.self_damage || !godfield::legacy::find_self_damage_card(result.self_damage_card)
                                    || result.self_damage_source > 1 || result.self_damage_target > 1 || incoming_counter
                                    || result.self_damage_amount < 0 || result.self_damage_amount > raw_limit
                                    || result.self_damage_source_lost > result.self_damage_amount
                                    || result.self_damage_target_lost > result.self_damage_amount
                                    || (result.self_damage_source == result.self_damage_target
                                        && (result.self_damage_source_lost + result.self_damage_target_lost > 99
                                            || (result.self_damage_source_lost < result.self_damage_amount
                                                && result.self_damage_target_lost)))
                                    || result.magic_blocked || result.weapon_blocked || result.magic_reflected || result.any_reflected
                                    || (result.bounced && !result.bounce_self))
                                    throw std::runtime_error("inconsistent self damage resolution");
                                if (incoming_response && (result.self_damage_card != incoming_self_damage
                                    || result.self_damage_amount != std::max<std::int64_t>(0,
                                        incoming_damage - (result.bounced || result.reflected ? 0 : selected_defense))))
                                    throw std::runtime_error("self damage residual accounting mismatch");
                                ++output.self_damage_resolutions;
                                output.self_damage_self_resolutions += result.self_damage_source == result.self_damage_target;
                                output.self_damage_zero_residual += result.self_damage_amount == 0;
                                output.self_damage_zero_source_loss += result.self_damage_source_lost == 0;
                                output.self_damage_source_lost += result.self_damage_source_lost;
                                output.self_damage_target_lost += result.self_damage_target_lost;
                                output.self_damage_capped_source += result.self_damage_source_lost < result.self_damage_amount;
                                output.self_damage_capped_target += result.self_damage_target_lost < result.self_damage_amount;
                                output.self_damage_raw_max = std::max(output.self_damage_raw_max,
                                    static_cast<std::uint64_t>(result.self_damage_amount));
                                output.self_damage_revival_steps += result.revivals[0] || result.revivals[1];
                                self_damage_digest = mix(self_damage_digest ^ after.decisions ^ result.self_damage_card
                                    ^ (static_cast<std::uint64_t>(result.self_damage_source) << 32)
                                    ^ (static_cast<std::uint64_t>(result.self_damage_target) << 40));
                                self_damage_digest = mix(self_damage_digest ^ static_cast<std::uint64_t>(result.self_damage_amount)
                                    ^ (static_cast<std::uint64_t>(result.self_damage_source_lost) << 48)
                                    ^ (static_cast<std::uint64_t>(result.self_damage_target_lost) << 56));
                            } else if (result.self_damage_card || result.self_damage_amount
                                || result.self_damage_source_lost || result.self_damage_target_lost) {
                                throw std::runtime_error("self damage payload without resolution");
                            }
                            if (result.pestle_attack != pestle_action || result.mortar_discards_ignored != selected_mortar_discards)
                                throw std::runtime_error("inconsistent pestle selection or ignored mortar discard event");
                            output.mortar_discards_ignored += result.mortar_discards_ignored;
                            if (result.pestle_attack) {
                                const bool locked = first_mortar_seat != godfield::legacy::no_slot;
                                if (!config.pestle_mortar || result.pestle_target > 1
                                    || result.pestle_mortar != locked || result.pestle_damage != (locked ? 999 : 30)
                                    || (locked && result.pestle_target != first_mortar_seat)
                                    || after.last_pestle.id != attack_id || after.last_pestle.source != actor
                                    || after.last_pestle.target != result.pestle_target
                                    || after.last_pestle.mortar != (locked ? 245 : 0)
                                    || after.last_pestle.amount != result.pestle_damage)
                                    throw std::runtime_error("pestle initial target accounting mismatch");
                                ++output.pestle_attacks;
                                output.pestle_mortar_locked_attacks += locked;
                                output.pestle_random_target_attacks += !locked;
                                output.pestle_initial_self_targets += result.pestle_target == actor;
                                output.pestle_initial_opponent_targets += result.pestle_target != actor;
                            }
                            output.pestle_reflections += incoming_pestle && (result.reflected || result.bounced);
                            output.pestle_blocks += incoming_pestle && result.weapon_blocked;
                            if (result.pestle_resolved) {
                                if (!config.pestle_mortar || (!pestle_action && !incoming_pestle) || incoming_counter
                                    || result.pestle_resolved_source > 1 || result.pestle_resolved_target > 1
                                    || result.pestle_residual < 0 || result.pestle_residual > 999
                                    || result.pestle_target_lost > result.pestle_residual
                                    || result.reflected || result.weapon_blocked || (result.bounced && !result.bounce_self))
                                    throw std::runtime_error("inconsistent pestle resolution");
                                const auto expected_residual = incoming_response
                                    ? std::max<std::int64_t>(0, incoming_damage - (result.bounced ? 0 : selected_defense))
                                    : static_cast<std::int64_t>(result.pestle_damage);
                                if (result.pestle_residual != expected_residual)
                                    throw std::runtime_error("pestle residual accounting mismatch");
                                ++output.pestle_resolutions;
                                output.pestle_self_resolutions += result.pestle_resolved_source == result.pestle_resolved_target;
                                output.pestle_zero_residual += result.pestle_residual == 0;
                                output.pestle_target_lost += result.pestle_target_lost;
                                output.pestle_capped += result.pestle_target_lost < result.pestle_residual;
                                output.pestle_raw_max = std::max(output.pestle_raw_max, static_cast<std::uint64_t>(result.pestle_residual));
                                output.pestle_revival_steps += result.revivals[0] || result.revivals[1];
                            } else if (result.pestle_residual || result.pestle_target_lost) {
                                throw std::runtime_error("pestle payload without resolution");
                            }
                            if (config.pestle_mortar && (pestle_action || incoming_pestle || result.mortar_discards_ignored)) {
                                pestle_digest = mix(pestle_digest ^ after.decisions
                                    ^ (static_cast<std::uint64_t>(result.pestle_attack) << 32)
                                    ^ (static_cast<std::uint64_t>(result.pestle_target) << 40)
                                    ^ (static_cast<std::uint64_t>(result.pestle_mortar) << 48)
                                    ^ (static_cast<std::uint64_t>(result.pestle_damage) << 49));
                                pestle_digest = mix(pestle_digest ^ static_cast<std::uint64_t>(result.pestle_residual)
                                    ^ (static_cast<std::uint64_t>(result.pestle_resolved) << 32)
                                    ^ (static_cast<std::uint64_t>(result.pestle_resolved_source) << 40)
                                    ^ (static_cast<std::uint64_t>(result.pestle_resolved_target) << 48)
                                    ^ (static_cast<std::uint64_t>(result.pestle_target_lost) << 56));
                                pestle_digest = mix(pestle_digest ^ result.mortar_discards_ignored);
                            }
                            if (result.sale_used != sale_action)
                                throw std::runtime_error("inconsistent selling use event");
                            output.sales += result.sale_used;
                            output.sale_reflections += incoming_sale && result.any_reflected;
                            if (result.sale_used && (!config.selling || result.sale_card != 3
                                || result.sale_item != sold_item || result.sale_price != sold_price))
                                throw std::runtime_error("selling product use accounting mismatch");
                            if (result.sale_resolved) {
                                if (!config.selling || (!sale_action && !incoming_sale) || incoming_counter
                                    || result.sale_card != 3 || result.sale_item != sold_item
                                    || sold_price < 0 || sold_price > godfield::legacy::max_sale_price
                                    || result.sale_price != sold_price || result.sale_source != sale_source || result.sale_target != sale_target
                                    || result.any_reflected || result.magic_reflected || result.bounced || result.weapon_blocked || result.magic_blocked)
                                    throw std::runtime_error("inconsistent sale resolution");
                                int debt = sold_price;
                                std::array<int, 3> expected_paid{};
                                for (int i = 0; i < 3; ++i) {
                                    expected_paid[i] = std::min(sale_target_before[i], debt);
                                    debt -= expected_paid[i];
                                }
                                const int source_money_after_payment = sale_source_money_before
                                    - (sale_source == sale_target ? expected_paid[0] : 0);
                                const int expected_gain = std::min(99 - source_money_after_payment, sold_price);
                                const int target_hand_at_delivery = sale_target_hand_before
                                    - (sale_action && sale_source == sale_target ? 2 : 0);
                                if (result.sale_money_lost != expected_paid[0] || result.sale_mp_lost != expected_paid[1]
                                    || result.sale_hp_lost != expected_paid[2] || result.sale_money_gained != expected_gain
                                    || result.sale_delivered != (target_hand_at_delivery < godfield::legacy::hand_capacity))
                                    throw std::runtime_error("sale charge, credit or delivery accounting mismatch");
                                ++output.sale_resolutions;
                                output.sold_buy_cards += godfield::legacy::find_buying_card(sold_item) != nullptr;
                                output.sold_hp_gambles += godfield::legacy::find_hp_gamble_card(sold_item) != nullptr;
                                output.sold_magic_bounce_cards += godfield::legacy::find_magic_bounce_card(sold_item) != nullptr;
                                output.sundry_boost_sold_cards += godfield::legacy::find_sundry_boost_card(sold_item) != nullptr;
                                output.sold_status_miracles += godfield::legacy::find_status_miracle(sold_item) != nullptr;
                                output.sale_delivered += result.sale_delivered;
                                output.sale_delivery_failures += !result.sale_delivered;
                                output.sale_self_resolutions += sale_source == sale_target;
                                output.sale_opponent_resolutions += sale_source != sale_target;
                                output.sale_zero_price += sold_price == 0;
                                output.sale_price_total += sold_price;
                                output.sale_money_lost += result.sale_money_lost;
                                output.sale_mp_lost += result.sale_mp_lost;
                                output.sale_hp_lost += result.sale_hp_lost;
                                output.sale_money_gained += result.sale_money_gained;
                                output.sale_revival_steps += result.revivals[0] || result.revivals[1];
                                if (const auto* product = artifact(sold_item)) {
                                    if (product->kind == godfield::legacy::Kind::weapon) ++output.sale_weapons;
                                    else if (product->kind == godfield::legacy::Kind::armor) ++output.sale_armor;
                                    else ++output.sale_other;
                                } else ++output.sale_miracles;
                                const auto& event = after.last_sale;
                                if (event.id != 3 || event.item != sold_item || event.source != sale_source || event.target != sale_target
                                    || event.price != sold_price || event.delivered != result.sale_delivered
                                    || event.money_lost != result.sale_money_lost || event.mp_lost != result.sale_mp_lost
                                    || event.hp_lost != result.sale_hp_lost || event.money_gained != result.sale_money_gained)
                                    throw std::runtime_error("sale history accounting mismatch");
                            } else if (result.sale_delivered || result.sale_money_lost || result.sale_mp_lost
                                || result.sale_hp_lost || result.sale_money_gained) {
                                throw std::runtime_error("sale resource event without resolution");
                            }
                            if (config.selling && (sale_action || incoming_sale)) {
                                sale_digest = mix(sale_digest ^ after.decisions ^ sold_item
                                    ^ (static_cast<std::uint64_t>(action.base) << 16)
                                    ^ (static_cast<std::uint64_t>(action.slots) << 24)
                                    ^ (static_cast<std::uint64_t>(sale_source) << 40)
                                    ^ (static_cast<std::uint64_t>(sale_target) << 48));
                                sale_digest = mix(sale_digest ^ static_cast<std::uint64_t>(sold_price)
                                    ^ (static_cast<std::uint64_t>(result.sale_used) << 32)
                                    ^ (static_cast<std::uint64_t>(result.sale_resolved) << 33)
                                    ^ (static_cast<std::uint64_t>(result.sale_delivered) << 34)
                                    ^ (static_cast<std::uint64_t>(result.any_reflected) << 35));
                                sale_digest = mix(sale_digest ^ result.sale_money_lost
                                    ^ (static_cast<std::uint64_t>(result.sale_mp_lost) << 8)
                                    ^ (static_cast<std::uint64_t>(result.sale_hp_lost) << 16)
                                    ^ (static_cast<std::uint64_t>(result.sale_money_gained) << 24));
                            }
                            if (result.buy_used != buy_action || result.buy_decided != purchase_action
                                || result.buy_accepted != (action.kind == godfield::legacy::ActionKind::buy_accept))
                                throw std::runtime_error("inconsistent buying use or decision event");
                            output.buy_uses += result.buy_used;
                            output.buy_attempts += result.buy_attempt;
                            output.buy_offers += result.buy_offered;
                            output.buy_decisions += result.buy_decided;
                            output.buy_reflections += incoming_buy && result.any_reflected;
                            if (result.buy_attempt) {
                                if (!config.buying || (!buy_action && !incoming_buy) || purchase_action
                                    || result.any_reflected || result.buy_source != buy_source || result.buy_target != buy_target
                                    || result.buy_population != buy_population_before)
                                    throw std::runtime_error("buy selection source or population mismatch");
                                if (!buy_population_before) {
                                    if (result.buy_item || result.buy_price || result.buy_slot != godfield::legacy::no_slot
                                        || result.buy_outcome != godfield::legacy::BuyOutcome::empty)
                                        throw std::runtime_error("inconsistent empty buy selection");
                                    ++output.buy_empty;
                                } else {
                                    if (result.buy_slot >= buy_population_before || result.buy_item != buy_hand_before[result.buy_slot]
                                        || trade_price(result.buy_item) != result.buy_price)
                                        throw std::runtime_error("buy selected item, slot or price mismatch");
                                    const auto expected = buy_source == buy_target ? godfield::legacy::BuyOutcome::self
                                        : buyer_money_before < result.buy_price ? godfield::legacy::BuyOutcome::insufficient
                                        : godfield::legacy::BuyOutcome::offered;
                                    if (result.buy_outcome != expected || result.buy_offered != (expected == godfield::legacy::BuyOutcome::offered))
                                        throw std::runtime_error("buy affordability or self outcome mismatch");
                                    output.buy_self_selections += expected == godfield::legacy::BuyOutcome::self;
                                    output.buy_insufficient += expected == godfield::legacy::BuyOutcome::insufficient;
                                }
                            }
                            if (result.buy_offered && (after.phase != godfield::legacy::Phase::purchase
                                || after.actor != result.buy_source || after.last_buy.outcome != godfield::legacy::BuyOutcome::offered))
                                throw std::runtime_error("buy offer did not enter the buyer decision phase");
                            if (result.buy_decided) {
                                if (!config.buying || !purchase_before
                                    || actor != offer_before.source || result.buy_source != offer_before.source
                                    || result.buy_target != offer_before.target || result.buy_item != offer_before.item
                                    || result.buy_price != offer_before.price)
                                    throw std::runtime_error("buy decision changed the revealed offer");
                                if (result.buy_accepted) {
                                    if (result.buy_outcome != godfield::legacy::BuyOutcome::purchased
                                        || buyer_money_before < result.buy_price || buy_source == buy_target
                                        || result.buy_money_lost != result.buy_price
                                        || result.buy_money_gained != std::min(99 - seller_money_before, result.buy_price)
                                        || result.buy_delivered != (buyer_hand_before < godfield::legacy::hand_capacity))
                                        throw std::runtime_error("purchase transfer, price or delivery mismatch");
                                    ++output.buy_purchases;
                                    output.bought_hp_gambles += godfield::legacy::find_hp_gamble_card(result.buy_item) != nullptr;
                                    output.bought_magic_bounce_cards += godfield::legacy::find_magic_bounce_card(result.buy_item) != nullptr;
                                    output.sundry_boost_bought_cards += godfield::legacy::find_sundry_boost_card(result.buy_item) != nullptr;
                                    output.bought_status_miracles += godfield::legacy::find_status_miracle(result.buy_item) != nullptr;
                                    output.buy_delivered += result.buy_delivered;
                                    output.buy_delivery_failures += !result.buy_delivered;
                                    output.buy_money_spent += result.buy_money_lost;
                                    output.buy_money_gained += result.buy_money_gained;
                                    output.buy_zero_price += result.buy_price == 0;
                                    output.buy_revival_steps += result.revivals[0] || result.revivals[1];
                                    if (const auto* product = artifact(result.buy_item)) {
                                        if (product->kind == godfield::legacy::Kind::weapon) ++output.bought_weapons;
                                        else if (product->kind == godfield::legacy::Kind::armor) ++output.bought_armor;
                                        else ++output.bought_other;
                                    } else ++output.bought_miracles;
                                } else {
                                    if (result.buy_outcome != godfield::legacy::BuyOutcome::declined)
                                        throw std::runtime_error("inconsistent declined purchase outcome");
                                    ++output.buy_declines;
                                }
                            }
                            if (!result.buy_accepted && (result.buy_delivered || result.buy_money_lost || result.buy_money_gained))
                                throw std::runtime_error("buy payment without acceptance");
                            if (config.buying && (buy_action || incoming_buy || purchase_action)) {
                                const auto& event = after.last_buy;
                                buy_digest = mix(buy_digest ^ after.decisions ^ static_cast<std::uint64_t>(action.kind)
                                    ^ (static_cast<std::uint64_t>(action.base) << 8) ^ (static_cast<std::uint64_t>(action.target) << 16)
                                    ^ (static_cast<std::uint64_t>(result.buy_used) << 24) ^ (static_cast<std::uint64_t>(result.buy_attempt) << 25)
                                    ^ (static_cast<std::uint64_t>(result.buy_decided) << 26) ^ (static_cast<std::uint64_t>(result.any_reflected) << 27));
                                buy_digest = mix(buy_digest ^ event.item ^ (static_cast<std::uint64_t>(event.source) << 16)
                                    ^ (static_cast<std::uint64_t>(event.target) << 24) ^ (static_cast<std::uint64_t>(event.outcome) << 32)
                                    ^ (static_cast<std::uint64_t>(event.population) << 40) ^ (static_cast<std::uint64_t>(event.slot) << 48));
                                buy_digest = mix(buy_digest ^ event.price ^ (static_cast<std::uint64_t>(event.delivered) << 8)
                                    ^ (static_cast<std::uint64_t>(event.money_lost) << 16) ^ (static_cast<std::uint64_t>(event.money_gained) << 24));
                            }
                            if (result.hp_gamble_used != hp_gamble_action)
                                throw std::runtime_error("inconsistent HP gamble use event");
                            if (result.hp_gamble_used) {
                                const auto& event = after.last_hp_gamble;
                                if (!config.hp_gamble || result.hp_gamble_card != 237
                                    || (result.hp_gamble_delta != 10 && result.hp_gamble_delta != -10)
                                    || result.hp_gamble_source != actor || result.hp_gamble_target != action.target
                                    || event.id != 237 || event.source != actor || event.target != action.target
                                    || event.delta != result.hp_gamble_delta)
                                    throw std::runtime_error("HP gamble launch or signed draw mismatch");
                                ++output.hp_gamble_uses;
                                output.hp_gamble_heal_rolls += result.hp_gamble_delta > 0;
                                output.hp_gamble_damage_rolls += result.hp_gamble_delta < 0;
                                output.hp_gamble_initial_self_targets += action.target == actor;
                                output.hp_gamble_initial_opponent_targets += action.target != actor;
                            }
                            output.hp_gamble_reflections += incoming_hp_gamble && result.any_reflected;
                            if (result.hp_gamble_resolved) {
                                const int expected_delta = incoming_hp_gamble ? gamble_delta_before : result.hp_gamble_delta;
                                const int expected_change = std::clamp(gamble_target_hp_before + expected_delta, 0, 99) - gamble_target_hp_before;
                                const auto& event = after.last_hp_gamble;
                                if (!config.hp_gamble || (!hp_gamble_action && !incoming_hp_gamble)
                                    || result.hp_gamble_card != 237 || incoming_counter
                                    || result.hp_gamble_source != gamble_source || result.hp_gamble_target != gamble_target
                                    || result.hp_gamble_delta != expected_delta || result.hp_gamble_change != expected_change
                                    || (expected_delta != 10 && expected_delta != -10)
                                    || result.any_reflected || result.reflected || result.bounced || result.weapon_blocked || result.magic_blocked
                                    || !event.resolved || event.resolved_source != gamble_source || event.resolved_target != gamble_target
                                    || event.change != expected_change || event.delta != expected_delta)
                                    throw std::runtime_error("HP gamble resolution, clipping or history mismatch");
                                ++output.hp_gamble_resolutions;
                                output.hp_gamble_healing_resolutions += expected_delta > 0;
                                output.hp_gamble_damage_resolutions += expected_delta < 0;
                                output.hp_gamble_self_resolutions += gamble_source == gamble_target;
                                output.hp_gamble_healed += std::max(0, expected_change);
                                output.hp_gamble_damaged += std::max(0, -expected_change);
                                output.hp_gamble_zero_change += expected_change == 0;
                                output.hp_gamble_capped_heals += expected_delta > 0 && expected_change < expected_delta;
                                output.hp_gamble_capped_damage += expected_delta < 0 && expected_change > expected_delta;
                                output.hp_gamble_revival_steps += result.revivals[0] || result.revivals[1];
                            } else if (result.hp_gamble_change) {
                                throw std::runtime_error("HP gamble actual change without resolution");
                            }
                            if (incoming_hp_gamble) {
                                const auto& event = after.last_hp_gamble;
                                if (event.id != gamble_history_before.id || event.source != gamble_history_before.source
                                    || event.target != gamble_history_before.target || event.delta != gamble_history_before.delta)
                                    throw std::runtime_error("HP gamble reflection changed immutable launch history");
                                if (result.any_reflected && (after.pending.hp_gamble_card != incoming_hp_gamble
                                    || after.pending.amount != gamble_delta_before))
                                    throw std::runtime_error("HP gamble reflection rerolled its signed payload");
                            }
                            if (config.hp_gamble && (hp_gamble_action || incoming_hp_gamble)) {
                                hp_gamble_digest = mix(hp_gamble_digest ^ after.decisions ^ result.hp_gamble_card
                                    ^ (static_cast<std::uint64_t>(result.hp_gamble_used) << 16)
                                    ^ (static_cast<std::uint64_t>(result.hp_gamble_resolved) << 17)
                                    ^ (static_cast<std::uint64_t>(result.hp_gamble_source) << 24)
                                    ^ (static_cast<std::uint64_t>(result.hp_gamble_target) << 32)
                                    ^ (static_cast<std::uint64_t>(result.any_reflected) << 40));
                                hp_gamble_digest = mix(hp_gamble_digest ^ static_cast<std::uint64_t>(result.hp_gamble_delta));
                                hp_gamble_digest = mix(hp_gamble_digest ^ static_cast<std::uint64_t>(result.hp_gamble_change));
                            }
                            if (result.sundry_boost_cards_used != selected_sundry_boosts)
                                throw std::runtime_error("SUNDRY boost physical-copy consumption mismatch");
                            output.sundry_boost_discarded_cards += discarded_sundry_boosts;
                            if (selected_sundry_boosts) {
                                ++output.sundry_boost_attacks;
                                output.sundry_boost_cards_used += result.sundry_boost_cards_used;
                                output.sundry_boost_plain_attacks += !ordered_attack;
                                output.sundry_boost_ordered_attacks += ordered_attack;
                                output.sundry_boost_mana_attacks += mana_weapon_base;
                                output.sundry_boost_mana_resets += boost_mana_resets;
                                output.sundry_boost_resets_after_boost += resets_after_boost;
                                output.sundry_boost_double_attacks += boost_has_double;
                                output.sundry_boost_wide_attacks += boost_has_wide;
                                output.sundry_boost_self_attacks += (boost_has_wide ? 1 - actor : action.target) == actor;
                                const auto element_index = static_cast<std::size_t>(boost_element);
                                if (element_index >= output.sundry_boost_elements.size())
                                    throw std::runtime_error("invalid SUNDRY boost final element");
                                ++output.sundry_boost_elements[element_index];
                                output.sundry_boost_raw_max = std::max(output.sundry_boost_raw_max,
                                    static_cast<std::uint64_t>(replayed_boost_raw));
                                bool checked = false;
                                if (after.phase == godfield::legacy::Phase::response && !after.pending.counter_card) {
                                    if (after.pending.amount != replayed_boost_raw || after.pending.element != boost_element)
                                        throw std::runtime_error("SUNDRY boost ordered power or element replay mismatch");
                                    checked = true;
                                }
                                if (result.mana_weapon_attack) {
                                    if (result.mana_weapon_power != replayed_boost_raw || result.mana_weapon_mp_spent != replayed_mana_left)
                                        throw std::runtime_error("SUNDRY boost mana reset replay mismatch");
                                    checked = true;
                                }
                                if (result.hp_absorption_resolved || result.self_damage_resolved) {
                                    const auto amount = result.hp_absorption_resolved ? result.hp_absorption_amount : result.self_damage_amount;
                                    if (amount != replayed_boost_raw) throw std::runtime_error("SUNDRY boost immediate power replay mismatch");
                                    checked = true;
                                }
                                output.sundry_boost_raw_checked += checked;
                            }
                            if (incoming_sundry_boost) {
                                output.sundry_boost_reaction_steps += result.reflected || result.bounced || result.weapon_blocked;
                                output.sundry_boost_counters_queued += result.counter_queued;
                            }
                            if (config.sundry_boost && (selected_sundry_boosts || incoming_sundry_boost || discarded_sundry_boosts)) {
                                sundry_boost_digest = mix(sundry_boost_digest ^ after.decisions ^ static_cast<std::uint64_t>(action.kind)
                                    ^ (static_cast<std::uint64_t>(action.base) << 8) ^ (static_cast<std::uint64_t>(action.slots) << 16)
                                    ^ (static_cast<std::uint64_t>(action.target) << 32) ^ (static_cast<std::uint64_t>(selected_sundry_boosts) << 40)
                                    ^ (static_cast<std::uint64_t>(discarded_sundry_boosts) << 48));
                                sundry_boost_digest = mix(sundry_boost_digest ^ static_cast<std::uint64_t>(replayed_boost_raw)
                                    ^ (static_cast<std::uint64_t>(replayed_mana_left) << 32)
                                    ^ (static_cast<std::uint64_t>(boost_element) << 40));
                                sundry_boost_digest = mix(sundry_boost_digest ^ result.counter_queued
                                    ^ (static_cast<std::uint64_t>(result.reflected) << 8)
                                    ^ (static_cast<std::uint64_t>(result.bounced) << 9)
                                    ^ (static_cast<std::uint64_t>(result.weapon_blocked) << 10));
                                for (int i = 0; i < action.extra_count; ++i)
                                    sundry_boost_digest = mix(sundry_boost_digest ^ action.extras[i] ^ (static_cast<std::uint64_t>(i) << 8));
                            }
                            if (weapon_attack || incoming_sundry_boost)
                                sundry_boost_pending = (selected_sundry_boosts || incoming_sundry_boost)
                                    && after.phase == godfield::legacy::Phase::response && !after.pending.counter_card;
                            std::uint64_t status_known_after = 0;
                            if (config.status_miracles) for (const auto count : after.players[actor].status_abilities) status_known_after += count;
                            const auto status_lost = actor_forgot && godfield::legacy::find_status_miracle(result.forgotten_miracle) ? 1u : 0u;
                            if (result.status_first_use != (used_miracle.status && !recast)
                                || result.status_recast != (used_miracle.status && recast)
                                || status_known_after + status_lost < status_known_before
                                || status_known_after + status_lost - status_known_before != result.status_first_use)
                                throw std::runtime_error("status miracle learning or cast index mismatch");
                            output.status_first_uses += result.status_first_use;
                            output.status_recasts += result.status_recast;
                            output.status_casts += result.status_first_use || result.status_recast;
                            output.status_free_uses += used_miracle.status && free_cast_action;
                            output.status_reflections += incoming_status && result.reflected;
                            output.status_bounces += incoming_status && result.magic_bounced;
                            output.status_blocks += incoming_status && result.magic_blocked;
                            const bool expected_status_resolution = (used_miracle.status && action.target == actor)
                                || (incoming_status && !result.reflected && !result.magic_blocked && (!result.magic_bounced || result.bounce_self));
                            if (result.status_resolved != expected_status_resolution)
                                throw std::runtime_error("status miracle resolution classification mismatch");
                            if (result.status_resolved) {
                                const auto* status = godfield::legacy::find_status_miracle(result.status_miracle_card);
                                const auto target = incoming_status ? incoming_money_target : action.target;
                                const auto source = result.bounce_self ? actor : incoming_status ? incoming_money_source : actor;
                                if (!config.status_miracles || !status || result.status_target != target || result.status_source != source
                                    || status->id != (incoming_status ? incoming_status->id : used_miracle.id) || target > 1 || source > 1)
                                    throw std::runtime_error("status miracle current endpoints mismatch");
                                auto disease = status_before[target].disease;
                                int worse = status_before[target].worse_chance, hp = status_before[target].hp;
                                bool glory = status_before[target].glory, cloud = status_before[target].dark_cloud;
                                // Ordinary prefix disease effects happen before a
                                // reflected/self-bounced MAGIC resolves. Replay them
                                // separately from the new pure status effect.
                                if (incoming_status) for (int slot = 0; slot < status_before[actor].hand_count; ++slot) {
                                    if (!(response_slots & (1u << slot))) continue;
                                    const auto* prefix = godfield::legacy::find_disease_card(status_before[actor].hand[slot]);
                                    if (!prefix || !prefix->on_defense) continue;
                                    if (disease == godfield::legacy::Disease::heaven) hp = 0;
                                    else if (disease == godfield::legacy::Disease::none) disease = prefix->disease;
                                    else disease = static_cast<godfield::legacy::Disease>(std::max(static_cast<int>(prefix->disease), static_cast<int>(disease) + 1));
                                    worse = 0;
                                }
                                const auto previous_disease = disease;
                                const bool previous_glory = glory, previous_cloud = cloud;
                                const bool heaven_fall = disease == godfield::legacy::Disease::heaven
                                    && (status->disease != godfield::legacy::Disease::none || status->harm != godfield::legacy::Harm::none);
                                if (status->cure != godfield::legacy::Cure::none) {
                                    glory = false;
                                    if (status->cure == godfield::legacy::Cure::all) cloud = false;
                                    if (status->cure == godfield::legacy::Cure::all
                                        || disease == godfield::legacy::Disease::cold || disease == godfield::legacy::Disease::fever) {
                                        disease = godfield::legacy::Disease::none; worse = 0;
                                    }
                                } else if (heaven_fall) {
                                    output.status_hp_lost += hp;
                                    hp = 0; worse = 0;
                                } else if (status->disease != godfield::legacy::Disease::none) {
                                    disease = disease == godfield::legacy::Disease::none ? status->disease
                                        : static_cast<godfield::legacy::Disease>(std::max(static_cast<int>(status->disease), static_cast<int>(disease) + 1));
                                    worse = 0;
                                } else cloud = true;
                                ++output.status_resolutions;
                                output.status_self_resolutions += source == target;
                                output.status_lower_cures += status->cure == godfield::legacy::Cure::lower;
                                output.status_all_cures += status->cure == godfield::legacy::Cure::all;
                                output.status_dark_cloud_applications += status->harm == godfield::legacy::Harm::dark_cloud;
                                output.status_cold_applications += status->disease == godfield::legacy::Disease::cold;
                                output.status_heaven_applications += status->disease == godfield::legacy::Disease::heaven;
                                output.status_heaven_falls += heaven_fall;
                                output.status_disease_changes += disease != previous_disease;
                                output.status_diseases_removed += previous_disease != godfield::legacy::Disease::none && disease == godfield::legacy::Disease::none;
                                output.status_glory_cleared += previous_glory && !glory;
                                output.status_dark_cloud_cleared += previous_cloud && !cloud;
                                output.status_dark_cloud_new += !previous_cloud && cloud;
                                output.status_held_resolutions += incoming_status ? incoming_from_ability : recast;
                                auto final_disease = disease;
                                if (result.disease_tick && after.last_tick.seat == target) {
                                    if (after.last_tick.before != disease)
                                        throw std::runtime_error("status effect was not applied before owner disease tick");
                                    final_disease = after.last_tick.after;
                                    worse = result.disease_worsened ? 0 : worse + 1;
                                    hp += result.disease_healing - result.disease_damage;
                                }
                                hp += 10 * result.revivals[target];
                                if (after.players[target].disease != final_disease || after.players[target].worse_chance != worse
                                    || after.players[target].glory != glory || after.players[target].dark_cloud != cloud || after.players[target].hp != hp)
                                    throw std::runtime_error("status effect, owner tick or revival accounting mismatch");
                            }
                            if (config.status_miracles && (used_miracle.status || incoming_status)) {
                                status_digest = mix(status_digest ^ after.decisions ^ result.status_miracle_card
                                    ^ (static_cast<std::uint64_t>(result.status_first_use) << 16)
                                    ^ (static_cast<std::uint64_t>(result.status_recast) << 17)
                                    ^ (static_cast<std::uint64_t>(result.status_resolved) << 18)
                                    ^ (static_cast<std::uint64_t>(result.status_source) << 24)
                                    ^ (static_cast<std::uint64_t>(result.status_target) << 32)
                                    ^ (static_cast<std::uint64_t>(result.spell_mp_spent) << 40));
                                status_digest = mix(status_digest ^ static_cast<std::uint64_t>(action.kind)
                                    ^ (static_cast<std::uint64_t>(action.base) << 8) ^ (static_cast<std::uint64_t>(action.target) << 16)
                                    ^ (static_cast<std::uint64_t>(result.magic_free_cards) << 24)
                                    ^ (static_cast<std::uint64_t>(result.reflected) << 32)
                                    ^ (static_cast<std::uint64_t>(result.magic_bounced) << 33)
                                    ^ (static_cast<std::uint64_t>(result.magic_blocked) << 34));
                            }
                            const auto revival_count = result.revivals[0] + result.revivals[1];
                            auto revival_budget = revival_cards_before;
                            revival_budget[actor] -= discarded_revival_cards;
                            // Physical trade/removal can move244 before the death scan.
                            // Charge a SELL product only on launch; a later response
                            // starts after that copy was already consumed. Self-sale
                            // consumption and its successful receipt cancel here.
                            if (result.sale_used && result.sale_item == 244) --revival_budget[actor];
                            if (result.item_removed && result.removed_item == 244) --revival_budget[result.item_removed_from];
                            if (result.buy_accepted && result.buy_item == 244) --revival_budget[result.buy_target];
                            if (result.sale_delivered && result.sale_item == 244) ++revival_budget[result.sale_target];
                            if (result.buy_delivered && result.buy_item == 244) ++revival_budget[result.buy_source];
                            if ((!config.revival && revival_count) || revival_budget[0] < 0 || revival_budget[1] < 0
                                || result.revivals[0] > revival_budget[0] || result.revivals[1] > revival_budget[1]
                                || (revival_count && (after.last_revival.id != 244 || after.last_revival.seat > 1
                                    || !result.revivals[after.last_revival.seat])))
                                throw std::runtime_error("inconsistent automatic revival event");
                            output.revivals += revival_count;
                            output.revival_steps += revival_count != 0;
                            output.revival_multiple_steps += revival_count > 1;
                            output.revival_both_seats += result.revivals[0] && result.revivals[1];
                            for (int seat = 0; seat < 2; ++seat) output.revivals_by_seat[seat] += result.revivals[seat];
                            output.revival_hp_restored += revival_count * 10;
                            if (config.revival && revival_count) {
                                revival_digest = mix(revival_digest ^ after.decisions
                                    ^ (static_cast<std::uint64_t>(action.kind) << 24)
                                    ^ (static_cast<std::uint64_t>(result.revivals[0]) << 32)
                                    ^ (static_cast<std::uint64_t>(result.revivals[1]) << 40)
                                    ^ (static_cast<std::uint64_t>(after.last_revival.seat) << 48));
                            }
                            if (result.exchanged != exchange_action)
                                throw std::runtime_error("inconsistent exchange event");
                            if (result.exchanged) {
                                const std::array<int, 3> allocation{action.extras[0], action.extras[1], action.extras[2]};
                                const auto sum = [](const auto& values) { return values[0] + values[1] + values[2]; };
                                if (!config.exchanges || incoming_response || turn_owner_before != actor
                                    || action.extra_count != 3 || result.exchange_before != resources_before
                                    || result.exchange_after != allocation || sum(resources_before) != sum(allocation)
                                    || after.decisions != decisions_before + 1
                                    || std::any_of(allocation.begin(), allocation.end(), [](int value) { return value < 0 || value > 99; }))
                                    throw std::runtime_error("exchange allocation conservation mismatch");
                                const auto& current = after.players[actor];
                                if ((result.disease_tick && after.last_tick.seat != actor)
                                    || current.hp != allocation[0] + 10 * result.revivals[actor] - result.disease_damage + result.disease_healing
                                    || current.mp != allocation[1] || current.money != allocation[2]
                                    || current.hp < 0 || current.hp > 99
                                    || (allocation[0] == 0 && !result.revivals[actor] && (!result.terminated || result.truncated
                                        || after.winner != 1 - actor || result.rewards[actor] != -1.f)))
                                    throw std::runtime_error("exchange owner tick or suicide mismatch");
                                ++output.exchanges;
                                output.exchange_noops += allocation == resources_before;
                                // v26 can consume244 before HP0 is finalized.
                                // Earlier profiles still count every zero-HP allocation.
                                output.exchange_suicides += allocation[0] == 0 && !result.revivals[actor];
                                output.exchange_revival_steps += allocation[0] == 0 && result.revivals[actor] != 0;
                                output.exchange_tick_damage += result.disease_damage;
                                output.exchange_tick_healing += result.disease_healing;
                                for (int i = 0; i < 3; ++i) {
                                    const int delta = allocation[i] - resources_before[i];
                                    output.exchange_up[i] += std::max(0, delta);
                                    output.exchange_down[i] += std::max(0, -delta);
                                }
                                exchange_digest = mix(exchange_digest ^ after.decisions
                                    ^ (static_cast<std::uint64_t>(actor) << 32)
                                    ^ (static_cast<std::uint64_t>(action.base) << 40)
                                    ^ (static_cast<std::uint64_t>(action.kind) << 48));
                                for (int i = 0; i < 3; ++i) {
                                    exchange_digest = mix(exchange_digest ^ static_cast<std::uint64_t>(resources_before[i]));
                                    exchange_digest = mix(exchange_digest ^ static_cast<std::uint64_t>(allocation[i]));
                                }
                                exchange_digest = mix(exchange_digest ^ static_cast<std::uint64_t>(result.disease_damage)
                                    ^ (static_cast<std::uint64_t>(result.disease_healing) << 8));
                            }
                            if (result.terminated) terminal_rewards = result.rewards;
                            if ((result.disease_worsened && !result.disease_tick)
                                || result.disease_damage < 0 || result.disease_healing < 0
                                || (result.disease_damage && result.disease_healing)
                                || (!result.disease_tick && (result.disease_damage || result.disease_healing)))
                                throw std::runtime_error("inconsistent disease tick event");
                            if ((result.counter_missed || result.counter_auto_resolved || result.counter_chance_attempts
                                    || result.counter_chance_hits || result.counter_chance_misses || result.counter_chance_forced
                                    || result.counter_hidden_checks || result.counter_mp_gained || result.counter_harm_applications)
                                && !config.counter_extensions)
                                throw std::runtime_error("extended counter event outside extension profile");
                            if (result.counter_chance_hits + result.counter_chance_misses != result.counter_chance_attempts
                                || result.counter_chance_forced > result.counter_chance_hits
                                || result.counter_missed != result.counter_chance_misses
                                || result.counter_auto_resolved > result.counter_launched
                                || result.counter_hidden_checks > result.counter_skipped)
                                throw std::runtime_error("inconsistent extended counter event");
                            if ((result.counter_queued || result.counter_launched || result.counter_skipped || result.counter_resolved)
                                && !config.counters)
                                throw std::runtime_error("counter event outside counter profile");
                            if (result.counter_queued > selected_counters
                                || (result.counter_resolved && !incoming_counter))
                                throw std::runtime_error("inconsistent counter event");
                            // ID194 is always an externally resolved counter, never
                            // an auto-resolved queued item or a simultaneous miracle.
                            // Its resource changes therefore cannot enter the miracle
                            // HP/money deltas. Held-spell MP uses catalog cost, so a
                            // later automatic ID193 refund cannot offset that cost.
                            if (result.counter_money_absorptions != (incoming_money_counter && result.counter_resolved)
                                || (result.counter_money_absorptions && (!config.money_counter || resolution_candidate
                                    || incoming_magic || incoming_counter_id != 194 || used_miracle.id != 0
                                    || incoming_money_source > 1 || incoming_money_target > 1
                                    || incoming_money_source == incoming_money_target)))
                                throw std::runtime_error("inconsistent money counter event");
                            if (!result.counter_money_absorptions && (result.counter_money_gained || result.counter_money_lost
                                || result.counter_mp_lost || result.counter_hp_lost))
                                throw std::runtime_error("money counter resource delta without resolution");
                            if (result.counter_money_absorptions
                                && (after.players[incoming_money_source].money - counter_source_money_before
                                        != static_cast<int>(result.counter_money_gained)
                                    || counter_target_money_before - after.players[incoming_money_target].money
                                        != static_cast<int>(result.counter_money_lost)))
                                throw std::runtime_error("money counter seat accounting mismatch");
                            output.counter_money_absorptions += result.counter_money_absorptions;
                            output.counter_money_gained += result.counter_money_gained;
                            output.counter_money_lost += result.counter_money_lost;
                            output.counter_mp_lost += result.counter_mp_lost;
                            output.counter_hp_lost += result.counter_hp_lost;
                            output.counters_queued += result.counter_queued;
                            output.counters_launched += result.counter_launched;
                            output.counters_skipped += result.counter_skipped;
                            if (result.counter_cancelled && (!config.any_reflection || !incoming_counter
                                || !result.any_reflected || result.counter_resolved))
                                throw std::runtime_error("inconsistent cancelled counter event");
                            output.counters_cancelled += result.counter_cancelled;
                            output.counters_resolved += result.counter_resolved;
                            output.counter_defense_decisions += incoming_response && incoming_counter;
                            output.counter_cards_used += selected_counters;
                            output.counter_missed += result.counter_missed;
                            output.counter_auto_resolved += result.counter_auto_resolved;
                            output.counter_chance_attempts += result.counter_chance_attempts;
                            output.counter_chance_hits += result.counter_chance_hits;
                            output.counter_chance_misses += result.counter_chance_misses;
                            output.counter_chance_forced += result.counter_chance_forced;
                            output.counter_hidden_checks += result.counter_hidden_checks;
                            output.counter_mp_gained += result.counter_mp_gained;
                            output.counter_harm_applications += result.counter_harm_applications;
                            // Public counter checks join the aggregate chance totals;
                            // hidden dead-target checks remain a separate diagnostic.
                            output.chance_attempts += result.counter_chance_attempts;
                            output.chance_hits += result.counter_chance_hits;
                            output.chance_misses += result.counter_chance_misses;
                            output.forced_chance_hits += result.counter_chance_forced;
                            if (result.disease_tick) {
                                ++output.disease_ticks;
                                output.disease_worsenings += result.disease_worsened;
                                output.tick_damage += result.disease_damage;
                                output.tick_healing += result.disease_healing;
                            }
                            if (result.chance_hit && !result.chance_attempt)
                                throw std::runtime_error("chance hit reported without an attempt");
                            if (result.chance_forced && (!result.chance_attempt || !result.chance_hit))
                                throw std::runtime_error("forced chance event must be a hit attempt");
                            if (result.chance_attempt) {
                                ++output.chance_attempts;
                                if (result.chance_hit) ++output.chance_hits;
                                else ++output.chance_misses;
                                if (offensive_cast) ++output.offensive_miracle_chance_attempts;
                            }
                            output.forced_chance_hits += result.chance_forced;
                            if (result.harm_new_flags > result.harm_applications
                                || result.harm_heaven_falls > result.harm_applications)
                                throw std::runtime_error("inconsistent harm application event");
                            output.harm_applications += result.harm_applications;
                            output.harm_new_flags += result.harm_new_flags;
                            output.harm_heaven_falls += result.harm_heaven_falls;
                            output.glory_defense_decisions += result.glory_defense;
                            if (result.dark_execution && !config.celestial)
                                throw std::runtime_error("dark execution outside celestial profile");
                            output.dark_executions += result.dark_execution;
                            if ((result.reflected && result.bounced) || (result.bounce_self && !result.bounced))
                                throw std::runtime_error("inconsistent weapon reaction event");
                            if (result.magic_bounced && (!result.bounced || (!used_miracle.defensive && !result.magic_card_bounced)))
                                throw std::runtime_error("inconsistent magic bounce event");
                            if (result.weapon_blocked && (!used_miracle.defensive || result.bounced))
                                throw std::runtime_error("inconsistent weapon block event");
                            if (result.magic_reflected && (!config.magic_reflection || !incoming_magic
                                || incoming_counter || !result.reflected || result.bounced || used_miracle.id
                                || result.chance_attempt || result.counter_chance_attempts || result.counter_queued
                                || result.counter_launched || result.counter_skipped || result.counter_resolved
                                || result.counter_auto_resolved || mana_before != after.players[actor].mp))
                                throw std::runtime_error("inconsistent magic reflection event");
                            if (result.any_reflected != any_reflection_action
                                || (result.any_reflected && (!config.any_reflection || !incoming_response
                                    || !result.reflected || result.magic_reflected || result.bounced || used_miracle.id
                                    || result.counter_resolved || result.counter_money_absorptions || result.chance_attempt)))
                                throw std::runtime_error("inconsistent universal reflection event");
                            if (result.any_reflected) {
                                ++output.any_reflections;
                                ++game_any_reflections;
                                if (incoming_counter) ++output.any_counter_reflections;
                                else if (incoming_magic) ++output.any_magic_reflections;
                                else if (incoming_kind == godfield::legacy::Effect::damage) ++output.any_weapon_reflections;
                                else ++output.any_utility_reflections;
                                const auto& event = after.last_reaction;
                                any_reflection_digest = mix(any_reflection_digest
                                    ^ static_cast<std::uint64_t>(after.decisions)
                                    ^ (static_cast<std::uint64_t>(event.id) << 32)
                                    ^ (static_cast<std::uint64_t>(incoming_counter_id) << 16));
                                any_reflection_digest = mix(any_reflection_digest ^ static_cast<std::uint64_t>(incoming_damage));
                                any_reflection_digest = mix(any_reflection_digest
                                    ^ static_cast<std::uint64_t>(event.source)
                                    ^ (static_cast<std::uint64_t>(event.target) << 8)
                                    ^ (static_cast<std::uint64_t>(incoming_origin) << 16)
                                    ^ (static_cast<std::uint64_t>(incoming_kind) << 24)
                                    ^ (static_cast<std::uint64_t>(incoming_element) << 32)
                                    ^ (static_cast<std::uint64_t>(incoming_harm) << 40)
                                    ^ (static_cast<std::uint64_t>(incoming_hit_rate) << 48)
                                    ^ (static_cast<std::uint64_t>(incoming_from_ability) << 56));
                                any_reflection_digest = mix(any_reflection_digest
                                    ^ static_cast<std::uint64_t>(incoming_miracle)
                                    ^ (static_cast<std::uint64_t>(action.slots) << 16)
                                    ^ (static_cast<std::uint64_t>(result.attribute_removed) << 32)
                                    ^ (static_cast<std::uint64_t>(result.counter_cancelled) << 40));
                            }
                            if (result.magic_blocked != magic_block_action
                                || (result.magic_blocked && (!config.magic_block || !incoming_magic || incoming_counter
                                    || !selected_magic_block || used_miracle.id || result.reflected || result.bounced
                                    || result.weapon_blocked || result.chance_attempt || result.counter_chance_attempts
                                    || result.counter_queued || result.counter_launched || result.counter_skipped
                                    || result.counter_resolved || result.counter_cancelled || result.counter_missed
                                    || result.counter_auto_resolved || result.counter_hidden_checks
                                    || mana_before != after.players[actor].mp)))
                                throw std::runtime_error("inconsistent magic block event");
                            if (result.magic_blocked) {
                                ++output.magic_blocks;
                                ++game_magic_blocks;
                                if (incoming_kind == godfield::legacy::Effect::damage) ++output.magic_damage_blocks;
                                else ++output.magic_utility_blocks;
                                magic_block_digest = mix(magic_block_digest
                                    ^ static_cast<std::uint64_t>(after.decisions)
                                    ^ (static_cast<std::uint64_t>(selected_magic_block) << 32)
                                    ^ (static_cast<std::uint64_t>(incoming_miracle) << 16));
                                magic_block_digest = mix(magic_block_digest ^ static_cast<std::uint64_t>(incoming_damage));
                                magic_block_digest = mix(magic_block_digest
                                    ^ static_cast<std::uint64_t>(incoming_money_source)
                                    ^ (static_cast<std::uint64_t>(incoming_money_target) << 8)
                                    ^ (static_cast<std::uint64_t>(incoming_origin) << 16)
                                    ^ (static_cast<std::uint64_t>(incoming_kind) << 24)
                                    ^ (static_cast<std::uint64_t>(incoming_element) << 32)
                                    ^ (static_cast<std::uint64_t>(incoming_harm) << 40)
                                    ^ (static_cast<std::uint64_t>(incoming_hit_rate) << 48)
                                    ^ (static_cast<std::uint64_t>(incoming_from_ability) << 56));
                                magic_block_digest = mix(magic_block_digest
                                    ^ static_cast<std::uint64_t>(action.slots)
                                    ^ (static_cast<std::uint64_t>(actor) << 16)
                                    ^ (static_cast<std::uint64_t>(result.attribute_removed) << 24));
                            }
                            if (result.magic_reflected) {
                                ++output.magic_reflections;
                                ++game_magic_reflections;
                                const auto& event = after.last_reaction;
                                magic_reflection_digest = mix(magic_reflection_digest
                                    ^ static_cast<std::uint64_t>(after.decisions)
                                    ^ (static_cast<std::uint64_t>(event.id) << 32)
                                    ^ (static_cast<std::uint64_t>(incoming_miracle) << 16));
                                magic_reflection_digest = mix(magic_reflection_digest ^ static_cast<std::uint64_t>(incoming_damage));
                                magic_reflection_digest = mix(magic_reflection_digest
                                    ^ static_cast<std::uint64_t>(event.source)
                                    ^ (static_cast<std::uint64_t>(event.target) << 8)
                                    ^ (static_cast<std::uint64_t>(incoming_origin) << 16)
                                    ^ (static_cast<std::uint64_t>(incoming_kind) << 24)
                                    ^ (static_cast<std::uint64_t>(incoming_element) << 32)
                                    ^ (static_cast<std::uint64_t>(incoming_harm) << 40)
                                    ^ (static_cast<std::uint64_t>(incoming_hit_rate) << 48)
                                    ^ (static_cast<std::uint64_t>(incoming_from_ability) << 56));
                            }
                            if (result.magic_card_bounced != (selected_magic_bounce != 0)
                                || (result.magic_card_bounced && (!config.magic_bounce || !incoming_magic || !incoming_response
                                    || incoming_counter || !result.magic_bounced || !result.bounced
                                    || result.magic_bounce_card != selected_magic_bounce || used_miracle.id
                                    || result.reflected || result.magic_reflected || result.any_reflected || result.weapon_blocked || result.magic_blocked
                                    || result.chance_attempt || result.counter_chance_attempts || result.counter_queued
                                    || result.counter_launched || result.counter_skipped || result.counter_resolved
                                    || result.counter_auto_resolved || result.counter_cancelled
                                    || ((!result.bounce_self || incoming_kind != godfield::legacy::Effect::mana)
                                        && mana_before != after.players[actor].mp))))
                                throw std::runtime_error("inconsistent physical MAGIC bounce event");
                            if (!result.magic_card_bounced && result.magic_bounce_card)
                                throw std::runtime_error("physical MAGIC bounce ID without event");
                            output.magic_bounce_weapon_attacks += weapon_attack && attack_id == 21;
                            output.magic_bounce_boost_uses += magic_bounce_extra_uses;
                            output.magic_bounce_ordinary_cards_used += ordinary_magic_bounce_cards;
                            if (result.magic_card_bounced) {
                                const auto& event = after.last_reaction;
                                const int local = godfield::legacy::magic_bounce_index(result.magic_bounce_card);
                                if (local < 0 || event.id != result.magic_bounce_card || event.from_ability
                                    || event.source != actor || event.target > 1
                                    || event.reaction != godfield::legacy::WeaponReaction::bounce
                                    || result.bounce_self != (event.source == event.target))
                                    throw std::runtime_error("physical MAGIC bounce history mismatch");
                                ++output.magic_card_bounces;
                                ++output.magic_card_bounces_by_id[local];
                                output.magic_card_self_bounces += result.bounce_self;
                                output.magic_card_damage_bounces += incoming_kind == godfield::legacy::Effect::damage;
                                output.magic_card_utility_bounces += incoming_kind != godfield::legacy::Effect::damage;
                                output.magic_card_held_spell_bounces += incoming_from_ability;
                                output.magic_card_free_spell_bounces += incoming_magic_free != 0;
                                output.magic_card_removal_bounces += result.attribute_removed;
                                magic_card_bounce_digest = mix(magic_card_bounce_digest ^ after.decisions
                                    ^ (static_cast<std::uint64_t>(event.id) << 32)
                                    ^ (static_cast<std::uint64_t>(incoming_miracle) << 16));
                                magic_card_bounce_digest = mix(magic_card_bounce_digest ^ static_cast<std::uint64_t>(incoming_damage));
                                magic_card_bounce_digest = mix(magic_card_bounce_digest ^ event.source
                                    ^ (static_cast<std::uint64_t>(event.target) << 8)
                                    ^ (static_cast<std::uint64_t>(incoming_kind) << 16)
                                    ^ (static_cast<std::uint64_t>(incoming_element) << 24)
                                    ^ (static_cast<std::uint64_t>(incoming_harm) << 32)
                                    ^ (static_cast<std::uint64_t>(incoming_from_ability) << 40)
                                    ^ (static_cast<std::uint64_t>(incoming_magic_free) << 41)
                                    ^ (static_cast<std::uint64_t>(result.attribute_removed) << 56));
                            }
                            output.magic_bounces += result.magic_bounced;
                            output.magic_self_bounces += result.magic_bounced && result.bounce_self;
                            output.weapon_blocks += result.weapon_blocked;
                            if ((result.reflected && !result.magic_reflected && !result.any_reflected)
                                || (result.bounced && !result.magic_bounced)) {
                                ++output.weapon_reactions;
                                if (result.reflected) ++output.reflections;
                                if (result.bounced) ++output.bounces;
                                if (result.bounce_self) ++output.self_bounces;
                            }
                            if (result.attribute_removed != (result.attribute_removals != 0)
                                || (result.attribute_removed && (!config.attribute_removal || !incoming_response
                                    || result.removed_from != incoming_element)))
                                throw std::runtime_error("inconsistent attribute removal event");
                            const bool physical_reaction = !incoming_magic && !result.any_reflected
                                && (result.reflected || (result.bounced && !result.magic_bounced));
                            output.wide_weapon_reactions += incoming_wide && physical_reaction;
                            output.wide_removal_responses += incoming_wide && result.attribute_removed;
                            if (result.attribute_removed) {
                                ++output.attribute_removal_uses;
                                output.attribute_removal_cards_used += result.attribute_removals;
                                const auto element_index = static_cast<std::size_t>(incoming_element);
                                if (element_index >= output.removed_from_elements.size())
                                    throw std::runtime_error("invalid removed attribute");
                                ++output.removed_from_elements[element_index];
                                // v14 separates COUNTER origins; v17 also separates
                                // ordinary non-damage utilities from weapon responses.
                                // Earlier profiles keep their published classifications.
                                if (config.counter_extensions && incoming_counter) ++output.counter_removal_responses;
                                else if (incoming_magic) ++output.removal_magic_responses;
                                else if (config.any_reflection && incoming_kind != godfield::legacy::Effect::damage)
                                    ++output.removal_utility_responses;
                                else ++output.removal_weapon_responses;
                                output.removal_weapon_reactions += physical_reaction;
                                output.removal_magic_bounces += result.magic_bounced;
                                output.removal_magic_reflections += result.magic_reflected;
                                output.removal_magic_blocks += result.magic_blocked;
                                output.removal_any_reflections += result.any_reflected;
                                output.removal_weapon_blocks += result.weapon_blocked;
                            }
                            // Includes later responses in an already-neutralized chain,
                            // while removal_* counts only the step consuming a remover.
                            if (physical_reaction && (incoming_neutralized || result.attribute_removed)) {
                                ++output.neutralized_weapon_reactions;
                                output.neutralized_percentage_weapon_reactions += incoming_percentage;
                            }
                            if (result.attribute_attack != (setter_attack || (ordered_attack && ordered_setters))
                                || (result.attribute_attack && (!config.attribute_change || !result.attribute_cards_used))
                                || (!result.attribute_attack && result.attribute_cards_used))
                                throw std::runtime_error("inconsistent attribute attack event");
                            if (result.attribute_attack) {
                                ++output.attribute_attacks;
                                output.attribute_cards_used += result.attribute_cards_used;
                                if (result.attribute_element == godfield::legacy::Element::fire) ++output.attribute_fire_attacks;
                                else if (result.attribute_element == godfield::legacy::Element::water) ++output.attribute_water_attacks;
                                else if (result.attribute_element == godfield::legacy::Element::neutral) ++output.attribute_neutral_attacks;
                                else throw std::runtime_error("invalid attribute attack outcome");
                            }
                            output.attribute_weapon_base_uses += setter_base;
                            if (weapon_attack || damage_cast) {
                                // Misses may clear pending immediately. Classify
                                // those launched attacks by the chance base card.
                                const auto element = ordered_attack ? ordered_element
                                    : result.attribute_attack ? result.attribute_element
                                    : damage_cast ? used_miracle.element
                                    : chance_attack ? probabilistic_element : after.pending.element;
                                if (element == godfield::legacy::Element::neutral) ++output.neutral_attacks;
                                else ++output.elemental_attacks;
                                if (element == godfield::legacy::Element::light) ++output.light_attacks;
                                if (element == godfield::legacy::Element::dark) ++output.dark_attacks;
                            }
                            // Counts positive compatible defense on an elemental
                            // incoming attack, and raw damage removed before HP clipping.
                            if (elemental_response && selected_defense > 0 && !result.magic_bounced
                                && !result.magic_reflected && !result.any_reflected && !result.weapon_blocked && !result.magic_blocked) {
                                ++output.effective_elemental_defenses;
                                output.elemental_damage_reduced += std::min<std::int64_t>(incoming_damage, selected_defense);
                            }
                            if (used_miracle.id && !used_miracle.additive) {
                                if (recast) ++output.miracle_recasts;
                                else ++output.miracle_first_uses;
                                const auto mana_spent = config.magic_free ? result.spell_mp_spent
                                    : config.counter_extensions ? used_miracle.cost
                                    : std::max(0, mana_before - after.players[actor].mp);
                                output.miracle_mp_spent += mana_spent;
                                const auto learned = ability_slot < 0 ? 0 : used_miracle.status
                                    ? after.players[actor].status_abilities[ability_slot]
                                        + (actor_forgot && result.forgotten_miracle == used_miracle.id ? 1u : 0u) - known_before
                                    : used_miracle.defensive
                                    ? after.players[actor].defensive_abilities[ability_slot] - known_before
                                    : after.players[actor].abilities[ability_slot] - known_before;
                                if (ability_slot >= 0)
                                    output.learned_miracles += learned;
                                if (used_miracle.status) {
                                    output.status_mp_spent += mana_spent;
                                    output.status_learned += learned;
                                }
                                if (used_miracle.defensive) {
                                    if (recast) ++output.defensive_miracle_recasts;
                                    else ++output.defensive_miracle_first_uses;
                                    output.defensive_miracle_mp_spent += mana_spent;
                                    output.defensive_learned_miracles += learned;
                                }
                                if (used_miracle.hp_absorption) {
                                    if (recast) ++output.hp_absorption_miracle_recasts;
                                    else ++output.hp_absorption_miracle_first_uses;
                                    output.hp_absorption_miracle_mp_spent += mana_spent;
                                    output.hp_absorption_learned_miracles += learned;
                                    ++output.hp_absorption_miracle_attacks;
                                }
                                if (offensive_cast) {
                                    ++output.offensive_miracle_attacks;
                                    if (recast) ++output.offensive_miracle_recasts;
                                    else ++output.offensive_miracle_first_uses;
                                    output.offensive_miracle_mp_spent += mana_spent;
                                    output.offensive_learned_miracles += learned;
                                }
                            }
                            if (resolving_miracle && effect_target < 2) {
                                int miracle_healing = after.players[effect_target].hp - hp_before;
                                // Natural disease effects run during the same step.
                                // Remove their actual, HP-capped deltas from this
                                // miracle-only statistic; persistent ticks do not count.
                                if (result.disease_tick && after.last_tick.seat == effect_target)
                                    miracle_healing += result.disease_damage - result.disease_healing;
                                // Automatic244 healing belongs to the death boundary,
                                // separately from the original spell and owner tick.
                                miracle_healing -= 10 * result.revivals[effect_target];
                                if (result.hp_absorption_resolved && result.hp_absorption_source == effect_target)
                                    miracle_healing -= result.hp_absorption_healing;
                                output.miracle_hp_gained += std::max(0, miracle_healing);
                                output.miracle_money_gained += std::max(0, after.players[effect_target].money - money_before);
                                if (resolving_spell.hp_absorption)
                                    output.hp_absorption_miracle_hp_damage += std::max(0, -miracle_healing);
                                if (resolving_spell.additive) {
                                    ++output.additive_single_resolutions;
                                    output.additive_single_hp_damage += std::max(0, -miracle_healing);
                                }
                                if (resolving_offensive_miracle) {
                                    ++output.offensive_miracle_resolutions;
                                    // Actual target HP loss, including Dark and
                                    // status-triggered falls, excluding natural ticks.
                                    output.offensive_miracle_hp_damage += std::max(0, -miracle_healing);
                                }
                            }
                        }
                        const auto& state = env.state();
                        output.decisions += state.decisions;
                        output.buy_active_offers += config.buying && state.phase == godfield::legacy::Phase::purchase;
                        if (config.counters) {
                            output.counter_queue_remaining += state.counter_count;
                            output.counter_active_remaining += state.pending.counter_card != 0;
                        }
                        if (state.terminated) {
                            if (state.truncated)
                                throw std::runtime_error("invalid terminal outcome");
                            if (state.winner == -1) {
                                if (state.players[0].hp != 0 || state.players[1].hp != 0
                                    || terminal_rewards != std::array<float, 2>{0.f, 0.f})
                                    throw std::runtime_error("draw requires both HP zero and zero rewards");
                                ++output.draws;
                            } else {
                                if (state.winner < 0 || state.winner > 1
                                    || state.players[state.winner].hp <= 0
                                    || state.players[1 - state.winner].hp != 0
                                    || terminal_rewards[state.winner] != 1.f
                                    || terminal_rewards[1 - state.winner] != -1.f)
                                    throw std::runtime_error("invalid winner or terminal rewards");
                                ++output.wins[state.winner];
                            }
                        } else if (state.truncated) {
                            ++output.truncations;
                        } else {
                            throw std::runtime_error("done environment has no outcome");
                        }
                        if (config.forgetting) {
                            const auto inventory = retained_total(state.players[0], retained_order) + retained_total(state.players[1], retained_order);
                            if (inventory + output.forgetting_removed - forgotten_at_start
                                != output.miracle_first_uses - learned_at_start)
                                throw std::runtime_error("retained inventory balance mismatch");
                            output.retained_inventory += inventory;
                        }
                        if (config.status_miracles) for (const auto& player : state.players)
                            for (const auto count : player.status_abilities) output.status_retained_inventory += count;
                        // Addition intentionally wraps modulo 2^64, so the digest
                        // is independent of partitioning and worker completion order.
                        auto digest = mix(game_seed
                            ^ (static_cast<std::uint64_t>(state.decisions) << 32)
                            ^ (static_cast<std::uint64_t>(state.players[0].hp) << 16)
                            ^ (static_cast<std::uint64_t>(state.players[1].hp) << 8)
                            ^ static_cast<std::uint64_t>(state.winner + 1));
                        for (const auto& player : state.players) {
                            digest = mix(digest ^ static_cast<std::uint64_t>(player.mp)
                                ^ (static_cast<std::uint64_t>(player.money) << 8));
                            const int ability_types = config.offensive_miracles
                                ? godfield::legacy::total_miracle_count : godfield::legacy::miracle_count;
                            for (int index = 0; index < ability_types; ++index)
                                digest = mix(digest ^ static_cast<std::uint64_t>(player.abilities[index]));
                            // Old main-slot loops remain2/15. Slot15 is appended only in v28.
                            if (config.hp_absorption)
                                digest = mix(digest ^ player.abilities[godfield::legacy::hp_absorption_ability_index]);
                            if (config.disease)
                                digest = mix(digest ^ static_cast<std::uint64_t>(player.disease)
                                    ^ (static_cast<std::uint64_t>(player.worse_chance) << 8));
                            if (config.harms)
                                digest = mix(digest ^ static_cast<std::uint64_t>(player.glory)
                                    ^ (static_cast<std::uint64_t>(player.dark_cloud) << 8));
                        }
                        if (config.elements)
                            digest = mix(digest ^ static_cast<std::uint64_t>(state.pending.element)
                                ^ (static_cast<std::uint64_t>(state.pending.kind) << 8));
                        if (config.chance) {
                            const auto& event = state.last_chance;
                            digest = mix(digest ^ static_cast<std::uint64_t>(event.id)
                                ^ (static_cast<std::uint64_t>(event.source) << 16)
                                ^ (static_cast<std::uint64_t>(event.target) << 24)
                                ^ (static_cast<std::uint64_t>(event.hit_rate) << 32)
                                ^ (static_cast<std::uint64_t>(event.hit) << 40)
                                ^ (static_cast<std::uint64_t>(state.pending.hit_rate) << 48));
                        }
                        if (config.reflection) {
                            const auto& event = state.last_reaction;
                            digest = mix(digest ^ static_cast<std::uint64_t>(event.id)
                                ^ (static_cast<std::uint64_t>(event.source) << 16)
                                ^ (static_cast<std::uint64_t>(event.target) << 24)
                                ^ (static_cast<std::uint64_t>(state.reaction_count) << 32)
                                ^ (static_cast<std::uint64_t>(event.reaction) << 56));
                        }
                        if (config.disease) {
                            const auto& tick = state.last_tick;
                            digest = mix(digest ^ static_cast<std::uint64_t>(tick.seat)
                                ^ (static_cast<std::uint64_t>(tick.before) << 8)
                                ^ (static_cast<std::uint64_t>(tick.after) << 16)
                                ^ (static_cast<std::uint64_t>(tick.worsened) << 24)
                                ^ (static_cast<std::uint64_t>(tick.damage) << 32)
                                ^ (static_cast<std::uint64_t>(tick.healing) << 48));
                            digest = mix(digest ^ static_cast<std::uint64_t>(state.pending.status_card)
                                ^ (static_cast<std::uint64_t>(state.pending.disease) << 16)
                                ^ (static_cast<std::uint64_t>(state.pending.cure) << 24));
                        }
                        if (config.harms)
                            digest = mix(digest ^ static_cast<std::uint64_t>(state.pending.harm_card)
                                ^ (static_cast<std::uint64_t>(state.pending.harm) << 16)
                                ^ (static_cast<std::uint64_t>(state.last_chance.forced) << 24));
                        if (config.celestial) {
                            // Version8 also covers the semantic state that can
                            // distinguish celestial attacks and ongoing games.
                            // Earlier profile digest formulas remain untouched.
                            digest = mix(digest ^ static_cast<std::uint64_t>(state.actor)
                                ^ (static_cast<std::uint64_t>(state.turn_owner) << 8)
                                ^ (static_cast<std::uint64_t>(state.order_index) << 16)
                                ^ (static_cast<std::uint64_t>(state.phase) << 24)
                                ^ (static_cast<std::uint64_t>(state.order[0]) << 32)
                                ^ (static_cast<std::uint64_t>(state.order[1]) << 40)
                                ^ (static_cast<std::uint64_t>(state.turns) << 48));
                            for (const auto& player : state.players) {
                                digest = mix(digest ^ static_cast<std::uint64_t>(player.hand_count)
                                    ^ (static_cast<std::uint64_t>(player.pending_deals) << 16));
                                for (const auto id : player.hand)
                                    digest = mix(digest ^ static_cast<std::uint64_t>(id));
                            }
                            digest = mix(digest ^ static_cast<std::uint64_t>(state.pending.source)
                                ^ (static_cast<std::uint64_t>(state.pending.target) << 8)
                                ^ (static_cast<std::uint64_t>(state.pending.miracle) << 16)
                                ^ (static_cast<std::uint64_t>(state.pending.from_ability) << 32));
                            digest = mix(digest ^ static_cast<std::uint64_t>(state.pending.amount)
                                ^ (static_cast<std::uint64_t>(state.terminated) << 32)
                                ^ (static_cast<std::uint64_t>(state.truncated) << 33));
                        }
                        if (config.offensive_miracles)
                            digest = mix(digest ^ 0x6f66666d69726163ULL
                                ^ static_cast<std::uint64_t>(godfield::legacy::total_miracle_count));
                        if (config.defensive_miracles) {
                            digest = mix(digest ^ 0x6465666d69726163ULL
                                ^ static_cast<std::uint64_t>(godfield::legacy::defensive_miracle_count));
                            for (const auto& player : state.players) {
                                for (const auto count : player.defensive_abilities)
                                    digest = mix(digest ^ static_cast<std::uint64_t>(count));
                                digest = mix(digest ^ static_cast<std::uint64_t>(player.retained_deals));
                            }
                            digest = mix(digest ^ static_cast<std::uint64_t>(state.last_reaction.from_ability));
                        }
                        if (config.attribute_change)
                            digest = mix(digest ^ 0x6174747263686e67ULL
                                ^ static_cast<std::uint64_t>(state.pending.attribute_card));
                        if (config.attribute_removal)
                            digest = mix(digest ^ 0x6174747272656d6fULL
                                ^ static_cast<std::uint64_t>(state.pending.attribute_removed));
                        if (config.counters) {
                            digest = mix(digest ^ 0x636f756e74657273ULL
                                ^ static_cast<std::uint64_t>(state.counter_count)
                                ^ (static_cast<std::uint64_t>(state.counter_generated) << 8)
                                ^ (static_cast<std::uint64_t>(state.counter_started) << 16));
                            for (const auto& player : state.players)
                                digest = mix(digest ^ static_cast<std::uint64_t>(player.dead));
                            digest = mix(digest ^ static_cast<std::uint64_t>(state.pending.origin_source)
                                ^ (static_cast<std::uint64_t>(state.pending.counter_card) << 8));
                            // Queue payloads determine later damage, targeting and
                            // eligibility even when the live pending item is identical.
                            for (const auto& queued : state.counter_queue) {
                                digest = mix(digest ^ static_cast<std::uint64_t>(queued.amount));
                                digest = mix(digest ^ static_cast<std::uint64_t>(queued.kind)
                                    ^ (static_cast<std::uint64_t>(queued.source) << 8)
                                    ^ (static_cast<std::uint64_t>(queued.target) << 16)
                                    ^ (static_cast<std::uint64_t>(queued.origin_source) << 24)
                                    ^ (static_cast<std::uint64_t>(queued.counter_card) << 32)
                                    ^ (static_cast<std::uint64_t>(queued.element) << 48)
                                    ^ (static_cast<std::uint64_t>(queued.hit_rate) << 56));
                                digest = mix(digest ^ static_cast<std::uint64_t>(queued.miracle)
                                    ^ (static_cast<std::uint64_t>(queued.from_ability) << 16)
                                    ^ (static_cast<std::uint64_t>(queued.status_card) << 24)
                                    ^ (static_cast<std::uint64_t>(queued.disease) << 40)
                                    ^ (static_cast<std::uint64_t>(queued.cure) << 48));
                                digest = mix(digest ^ static_cast<std::uint64_t>(queued.harm_card)
                                    ^ (static_cast<std::uint64_t>(queued.harm) << 16)
                                    ^ (static_cast<std::uint64_t>(queued.attribute_card) << 24)
                                    ^ (static_cast<std::uint64_t>(queued.attribute_removed) << 40));
                            }
                        }
                        if (config.counter_extensions) {
                            // The ordered queue already hashes effect kind, harm,
                            // rate and card ID in v13. Also cover the live variant
                            // and explicit action order after queues have drained.
                            digest = mix(digest ^ 0x636e747265787473ULL ^ counter_order_digest);
                            digest = mix(digest ^ static_cast<std::uint64_t>(state.pending.kind)
                                ^ (static_cast<std::uint64_t>(state.pending.harm) << 8)
                                ^ (static_cast<std::uint64_t>(state.pending.hit_rate) << 16)
                                ^ (static_cast<std::uint64_t>(state.pending.counter_card) << 24));
                        }
                        if (config.money_counter)
                            digest = mix(digest ^ 0x6d6f6e6579636e74ULL
                                ^ static_cast<std::uint64_t>(godfield::legacy::Effect::absorb_money));
                        if (config.magic_reflection)
                            digest = mix(digest ^ 0x6d61676963726566ULL ^ magic_reflection_digest
                                ^ (game_magic_reflections << 32));
                        if (config.any_reflection)
                            digest = mix(digest ^ 0x616e797265666c74ULL ^ any_reflection_digest
                                ^ (game_any_reflections << 32));
                        if (config.mana_weapon) {
                            digest = mix(digest ^ 0x6d616e6177656170ULL ^ mana_weapon_digest
                                ^ (game_mana_weapon_attacks << 32));
                            digest = mix(digest ^ static_cast<std::uint64_t>(state.pending.mana_weapon)
                                ^ (static_cast<std::uint64_t>(state.pending.mana_spent) << 16));
                        }
                        if (config.magic_block)
                            digest = mix(digest ^ 0x6d61676963626c6bULL ^ magic_block_digest
                                ^ (game_magic_blocks << 32));
                        if (config.additive_miracles) {
                            digest = mix(digest ^ 0x6164646d69726163ULL ^ additive_action_digest);
                            for (const auto& player : state.players)
                                for (const auto count : player.additive_abilities) digest = mix(digest ^ count);
                            const auto& composition = state.attack_composition;
                            digest = mix(digest ^ composition.base
                                ^ (static_cast<std::uint64_t>(composition.initial_mp) << 16)
                                ^ (static_cast<std::uint64_t>(composition.count) << 24));
                            digest = mix(digest ^ composition.held_mask);
                            // Appended item65 is absent from every earlier digest.
                            const auto digest_capacity = config.magic_free ? godfield::legacy::attack_extra_capacity
                                : godfield::legacy::legacy_attack_extra_capacity;
                            for (int i = 0; i < digest_capacity; ++i) digest = mix(digest ^ composition.items[i]);
                            for (const auto count : composition.initial_additive) digest = mix(digest ^ count);
                        }
                        if (config.doubling_miracles) {
                            digest = mix(digest ^ 0x64626c6d69726163ULL ^ doubling_action_digest);
                            for (const auto& player : state.players) digest = mix(digest ^ player.doubling_ability);
                            digest = mix(digest ^ state.attack_composition.initial_doubling);
                        }
                        if (config.wide_miracles) {
                            digest = mix(digest ^ 0x776964656d697261ULL ^ wide_action_digest);
                            for (const auto& player : state.players) digest = mix(digest ^ player.wide_ability);
                            digest = mix(digest ^ state.attack_composition.initial_wide);
                            digest = mix(digest ^ state.pending.wide_miracle
                                ^ (static_cast<std::uint64_t>(state.last_chance.wide_miracle) << 16));
                        }
                        if (config.magic_free) {
                            digest = mix(digest ^ 0x6d61676963667265ULL ^ magic_free_digest);
                            digest = mix(digest ^ state.pending.free_card
                                ^ (static_cast<std::uint64_t>(state.last_reaction.free_card) << 16)
                                ^ (static_cast<std::uint64_t>(state.attack_composition.held_tail) << 32));
                            const auto& cast = state.free_cast;
                            digest = mix(digest ^ cast.magic
                                ^ (static_cast<std::uint64_t>(cast.initial_mp) << 16)
                                ^ (static_cast<std::uint64_t>(cast.from_ability) << 24)
                                ^ (static_cast<std::uint64_t>(cast.count) << 32));
                            for (const auto id : cast.items) digest = mix(digest ^ id);
                        }
                        if (config.forgetting) {
                            const auto& event = state.last_forget;
                            digest = mix(digest ^ 0x666f726765747469ULL ^ forgetting_digest);
                            digest = mix(digest ^ state.pending.forget_card ^ event.id
                                ^ (static_cast<std::uint64_t>(event.source) << 16)
                                ^ (static_cast<std::uint64_t>(event.target) << 24)
                                ^ (static_cast<std::uint64_t>(event.removed) << 32));
                            digest = mix(digest ^ event.population);
                            for (const auto& player : state.players)
                                for (const auto id : retained_order) digest = mix(digest ^ retained_count(player, id));
                        }
                        if (config.exchanges) {
                            const auto& event = state.last_exchange;
                            digest = mix(digest ^ 0x65786368616e6765ULL ^ exchange_digest);
                            digest = mix(digest ^ event.id ^ (static_cast<std::uint64_t>(event.seat) << 16));
                            for (int i = 0; i < 3; ++i) {
                                digest = mix(digest ^ event.before[i]);
                                digest = mix(digest ^ event.after[i]);
                            }
                        }
                        if (config.revival) {
                            digest = mix(digest ^ 0x7265766976616c32ULL ^ revival_digest);
                            digest = mix(digest ^ state.last_revival.id
                                ^ (static_cast<std::uint64_t>(state.last_revival.seat) << 16));
                            for (const auto& player : state.players) digest = mix(digest ^ player.dead);
                        }
                        if (config.item_removal) {
                            const auto& event = state.last_item_removal;
                            digest = mix(digest ^ 0x6974656d72656d6fULL ^ item_removal_digest);
                            digest = mix(digest ^ state.pending.item_removal_card ^ event.id
                                ^ (static_cast<std::uint64_t>(event.source) << 16)
                                ^ (static_cast<std::uint64_t>(event.target) << 24)
                                ^ (static_cast<std::uint64_t>(event.removed) << 32));
                            digest = mix(digest ^ event.population ^ (static_cast<std::uint64_t>(event.slot) << 8));
                        }
                        if (config.hp_absorption) {
                            const auto& event = state.last_hp_absorption;
                            digest = mix(digest ^ 0x68706162736f7262ULL ^ hp_absorption_digest);
                            digest = mix(digest ^ state.pending.hp_absorption_card ^ event.id
                                ^ (static_cast<std::uint64_t>(event.source) << 16)
                                ^ (static_cast<std::uint64_t>(event.target) << 24)
                                ^ (static_cast<std::uint64_t>(event.healing) << 32));
                            digest = mix(digest ^ static_cast<std::uint64_t>(event.amount));
                        }
                        if (config.self_damage) {
                            const auto& event = state.last_self_damage;
                            digest = mix(digest ^ 0x73656c6664616d67ULL ^ self_damage_digest);
                            digest = mix(digest ^ state.pending.self_damage_card ^ event.id
                                ^ (static_cast<std::uint64_t>(event.source) << 16)
                                ^ (static_cast<std::uint64_t>(event.target) << 24)
                                ^ (static_cast<std::uint64_t>(event.source_damage) << 32)
                                ^ (static_cast<std::uint64_t>(event.target_damage) << 40));
                            digest = mix(digest ^ static_cast<std::uint64_t>(event.amount));
                        }
                        if (config.pestle_mortar) {
                            const auto& event = state.last_pestle;
                            digest = mix(digest ^ 0x706573746c653330ULL ^ pestle_digest);
                            digest = mix(digest ^ state.pending.pestle_card
                                ^ (static_cast<std::uint64_t>(state.pending.mortar_card) << 16)
                                ^ (static_cast<std::uint64_t>(event.id) << 32));
                            digest = mix(digest ^ event.source ^ (static_cast<std::uint64_t>(event.target) << 8)
                                ^ (static_cast<std::uint64_t>(event.mortar) << 16)
                                ^ (static_cast<std::uint64_t>(event.amount) << 32));
                        }
                        if (config.selling) {
                            const auto& event = state.last_sale;
                            digest = mix(digest ^ 0x73656c6c696e6731ULL ^ sale_digest);
                            digest = mix(digest ^ state.pending.sale_card ^ (static_cast<std::uint64_t>(state.pending.sale_item) << 16));
                            digest = mix(digest ^ event.id ^ (static_cast<std::uint64_t>(event.item) << 16)
                                ^ (static_cast<std::uint64_t>(event.source) << 32) ^ (static_cast<std::uint64_t>(event.target) << 40)
                                ^ (static_cast<std::uint64_t>(event.price) << 48));
                            digest = mix(digest ^ event.turn ^ (static_cast<std::uint64_t>(event.delivered) << 32)
                                ^ (static_cast<std::uint64_t>(event.money_lost) << 33) ^ (static_cast<std::uint64_t>(event.mp_lost) << 40)
                                ^ (static_cast<std::uint64_t>(event.hp_lost) << 47) ^ (static_cast<std::uint64_t>(event.money_gained) << 54));
                            digest = mix(digest ^ state.players[0].incoming_sale
                                ^ (static_cast<std::uint64_t>(state.players[1].incoming_sale) << 8));
                        }
                        if (config.buying) {
                            const auto& event = state.last_buy;
                            digest = mix(digest ^ 0x627579696e673332ULL ^ buy_digest ^ state.pending.buy_card);
                            digest = mix(digest ^ event.id ^ (static_cast<std::uint64_t>(event.item) << 16)
                                ^ (static_cast<std::uint64_t>(event.source) << 32) ^ (static_cast<std::uint64_t>(event.target) << 40)
                                ^ (static_cast<std::uint64_t>(event.price) << 48));
                            digest = mix(digest ^ event.turn ^ (static_cast<std::uint64_t>(event.population) << 32)
                                ^ (static_cast<std::uint64_t>(event.slot) << 40) ^ (static_cast<std::uint64_t>(event.outcome) << 48)
                                ^ (static_cast<std::uint64_t>(event.delivered) << 56));
                            digest = mix(digest ^ event.money_lost ^ (static_cast<std::uint64_t>(event.money_gained) << 8)
                                ^ (static_cast<std::uint64_t>(state.players[0].incoming_buy) << 16)
                                ^ (static_cast<std::uint64_t>(state.players[1].incoming_buy) << 24)
                                ^ (static_cast<std::uint64_t>(state.phase) << 32));
                        }
                        if (config.hp_gamble) {
                            const auto& event = state.last_hp_gamble;
                            digest = mix(digest ^ 0x687067616d626c33ULL ^ hp_gamble_digest ^ state.pending.hp_gamble_card);
                            digest = mix(digest ^ event.id ^ (static_cast<std::uint64_t>(event.source) << 16)
                                ^ (static_cast<std::uint64_t>(event.target) << 24) ^ (static_cast<std::uint64_t>(event.resolved) << 32)
                                ^ (static_cast<std::uint64_t>(event.resolved_source) << 40)
                                ^ (static_cast<std::uint64_t>(event.resolved_target) << 48));
                            digest = mix(digest ^ static_cast<std::uint64_t>(event.delta));
                            digest = mix(digest ^ static_cast<std::uint64_t>(event.change) ^ event.turn);
                            const auto& snapshot = env.snapshot();
                            digest = mix(digest ^ snapshot.hp_gamble_count ^ (static_cast<std::uint64_t>(snapshot.hp_gamble_index) << 8));
                            for (int i = 0; i < snapshot.hp_gamble_count; ++i)
                                digest = mix(digest ^ snapshot.scripted_hp_gambles[i] ^ (static_cast<std::uint64_t>(i) << 8));
                        }
                        if (config.magic_bounce) {
                            const auto& event = state.last_reaction;
                            digest = mix(digest ^ 0x6d6167626f756e34ULL ^ magic_card_bounce_digest);
                            digest = mix(digest ^ event.id ^ (static_cast<std::uint64_t>(event.source) << 16)
                                ^ (static_cast<std::uint64_t>(event.target) << 24)
                                ^ (static_cast<std::uint64_t>(event.reaction) << 32)
                                ^ (static_cast<std::uint64_t>(event.from_ability) << 40));
                            for (const auto& player : state.players) digest = mix(digest ^ player.retained_deals);
                            const auto& snapshot = env.snapshot();
                            digest = mix(digest ^ snapshot.target_count ^ (static_cast<std::uint64_t>(snapshot.target_index) << 8));
                            for (int i = 0; i < snapshot.target_count; ++i)
                                digest = mix(digest ^ snapshot.scripted_targets[i] ^ (static_cast<std::uint64_t>(i) << 8));
                        }
                        if (config.sundry_boost) {
                            digest = mix(digest ^ 0x73756e6472796235ULL ^ sundry_boost_digest ^ sundry_boost_pending);
                            for (const auto& player : state.players) {
                                std::uint16_t slots = 0;
                                for (int i = 0; i < player.hand_count; ++i) if (player.hand[i] == 238) slots |= static_cast<std::uint16_t>(1u << i);
                                digest = mix(digest ^ slots);
                            }
                            digest = mix(digest ^ state.attack_composition.base ^ state.attack_composition.count);
                            for (int i = 0; i < state.attack_composition.count; ++i)
                                if (state.attack_composition.items[i] == 238)
                                    digest = mix(digest ^ 238 ^ (static_cast<std::uint64_t>(i) << 16));
                        }
                        if (config.status_miracles) {
                            digest = mix(digest ^ 0x7374617475736d36ULL ^ status_digest);
                            for (const auto& player : state.players) for (const auto count : player.status_abilities) digest = mix(digest ^ count);
                            digest = mix(digest ^ state.pending.miracle ^ (static_cast<std::uint64_t>(state.pending.kind) << 16)
                                ^ (static_cast<std::uint64_t>(state.pending.disease) << 24) ^ (static_cast<std::uint64_t>(state.pending.cure) << 32)
                                ^ (static_cast<std::uint64_t>(state.pending.harm) << 40) ^ (static_cast<std::uint64_t>(state.pending.from_ability) << 48));
                        }
                        output.checksum += digest;
                    }
                } catch (...) {
                    output.error = std::current_exception();
                }
            });
        }
        for (auto& worker : workers) worker.join();
        const double seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();
        Stats total;
        for (const auto& output : stats) {
            if (output.error) std::rethrow_exception(output.error);
            total.decisions += output.decisions;
            total.wins[0] += output.wins[0];
            total.wins[1] += output.wins[1];
            total.draws += output.draws;
            total.truncations += output.truncations;
            total.miracle_first_uses += output.miracle_first_uses;
            total.miracle_recasts += output.miracle_recasts;
            total.miracle_mp_spent += output.miracle_mp_spent;
            total.learned_miracles += output.learned_miracles;
            total.miracle_hp_gained += output.miracle_hp_gained;
            total.miracle_money_gained += output.miracle_money_gained;
            total.elemental_attacks += output.elemental_attacks;
            total.neutral_attacks += output.neutral_attacks;
            total.effective_elemental_defenses += output.effective_elemental_defenses;
            total.elemental_damage_reduced += output.elemental_damage_reduced;
            total.chance_attempts += output.chance_attempts;
            total.chance_hits += output.chance_hits;
            total.chance_misses += output.chance_misses;
            total.weapon_reactions += output.weapon_reactions;
            total.reflections += output.reflections;
            total.bounces += output.bounces;
            total.self_bounces += output.self_bounces;
            total.disease_ticks += output.disease_ticks;
            total.disease_worsenings += output.disease_worsenings;
            total.tick_damage += output.tick_damage;
            total.tick_healing += output.tick_healing;
            total.forced_chance_hits += output.forced_chance_hits;
            total.harm_applications += output.harm_applications;
            total.harm_new_flags += output.harm_new_flags;
            total.harm_heaven_falls += output.harm_heaven_falls;
            total.glory_defense_decisions += output.glory_defense_decisions;
            total.light_attacks += output.light_attacks;
            total.dark_attacks += output.dark_attacks;
            total.dark_executions += output.dark_executions;
            total.offensive_miracle_first_uses += output.offensive_miracle_first_uses;
            total.offensive_miracle_recasts += output.offensive_miracle_recasts;
            total.offensive_miracle_attacks += output.offensive_miracle_attacks;
            total.offensive_miracle_resolutions += output.offensive_miracle_resolutions;
            total.offensive_miracle_mp_spent += output.offensive_miracle_mp_spent;
            total.offensive_learned_miracles += output.offensive_learned_miracles;
            total.offensive_miracle_hp_damage += output.offensive_miracle_hp_damage;
            total.offensive_miracle_chance_attempts += output.offensive_miracle_chance_attempts;
            total.defensive_miracle_first_uses += output.defensive_miracle_first_uses;
            total.defensive_miracle_recasts += output.defensive_miracle_recasts;
            total.defensive_miracle_mp_spent += output.defensive_miracle_mp_spent;
            total.defensive_learned_miracles += output.defensive_learned_miracles;
            total.magic_bounces += output.magic_bounces;
            total.magic_self_bounces += output.magic_self_bounces;
            total.weapon_blocks += output.weapon_blocks;
            total.attribute_attacks += output.attribute_attacks;
            total.attribute_cards_used += output.attribute_cards_used;
            total.attribute_fire_attacks += output.attribute_fire_attacks;
            total.attribute_water_attacks += output.attribute_water_attacks;
            total.attribute_neutral_attacks += output.attribute_neutral_attacks;
            total.attribute_weapon_base_uses += output.attribute_weapon_base_uses;
            total.attribute_removal_uses += output.attribute_removal_uses;
            total.attribute_removal_cards_used += output.attribute_removal_cards_used;
            for (std::size_t i = 0; i < total.removed_from_elements.size(); ++i)
                total.removed_from_elements[i] += output.removed_from_elements[i];
            total.removal_weapon_responses += output.removal_weapon_responses;
            total.removal_magic_responses += output.removal_magic_responses;
            total.counter_removal_responses += output.counter_removal_responses;
            total.removal_utility_responses += output.removal_utility_responses;
            total.removal_weapon_reactions += output.removal_weapon_reactions;
            total.removal_magic_bounces += output.removal_magic_bounces;
            total.removal_weapon_blocks += output.removal_weapon_blocks;
            total.neutralized_weapon_reactions += output.neutralized_weapon_reactions;
            total.neutralized_percentage_weapon_reactions += output.neutralized_percentage_weapon_reactions;
            total.counters_queued += output.counters_queued;
            total.counters_launched += output.counters_launched;
            total.counters_skipped += output.counters_skipped;
            total.counters_resolved += output.counters_resolved;
            total.counter_defense_decisions += output.counter_defense_decisions;
            total.counter_cards_used += output.counter_cards_used;
            total.counter_queue_remaining += output.counter_queue_remaining;
            total.counter_active_remaining += output.counter_active_remaining;
            total.counter_missed += output.counter_missed;
            total.counter_auto_resolved += output.counter_auto_resolved;
            total.counter_chance_attempts += output.counter_chance_attempts;
            total.counter_chance_hits += output.counter_chance_hits;
            total.counter_chance_misses += output.counter_chance_misses;
            total.counter_chance_forced += output.counter_chance_forced;
            total.counter_hidden_checks += output.counter_hidden_checks;
            total.counter_mp_gained += output.counter_mp_gained;
            total.counter_harm_applications += output.counter_harm_applications;
            total.counter_money_absorptions += output.counter_money_absorptions;
            total.counter_money_gained += output.counter_money_gained;
            total.counter_money_lost += output.counter_money_lost;
            total.counter_mp_lost += output.counter_mp_lost;
            total.counter_hp_lost += output.counter_hp_lost;
            total.magic_reflections += output.magic_reflections;
            total.removal_magic_reflections += output.removal_magic_reflections;
            total.any_reflections += output.any_reflections;
            total.any_weapon_reflections += output.any_weapon_reflections;
            total.any_magic_reflections += output.any_magic_reflections;
            total.any_counter_reflections += output.any_counter_reflections;
            total.any_utility_reflections += output.any_utility_reflections;
            total.removal_any_reflections += output.removal_any_reflections;
            total.counters_cancelled += output.counters_cancelled;
            total.mana_weapon_attacks += output.mana_weapon_attacks;
            total.mana_weapon_mp_spent += output.mana_weapon_mp_spent;
            total.mana_weapon_power_total += output.mana_weapon_power_total;
            total.mana_weapon_zero_mp_attacks += output.mana_weapon_zero_mp_attacks;
            total.mana_weapon_zero_power_attacks += output.mana_weapon_zero_power_attacks;
            total.mana_weapon_setter_attacks += output.mana_weapon_setter_attacks;
            total.magic_blocks += output.magic_blocks;
            total.magic_damage_blocks += output.magic_damage_blocks;
            total.magic_utility_blocks += output.magic_utility_blocks;
            total.removal_magic_blocks += output.removal_magic_blocks;
            total.hp_absorption_resolutions += output.hp_absorption_resolutions;
            total.hp_absorption_weapon_resolutions += output.hp_absorption_weapon_resolutions;
            total.hp_absorption_miracle_resolutions += output.hp_absorption_miracle_resolutions;
            total.hp_absorption_self_resolutions += output.hp_absorption_self_resolutions;
            total.hp_absorption_zero_residual += output.hp_absorption_zero_residual;
            total.hp_absorption_zero_healing += output.hp_absorption_zero_healing;
            total.hp_absorption_healing += output.hp_absorption_healing;
            total.hp_absorption_capped += output.hp_absorption_capped;
            total.hp_absorption_dark_resolutions += output.hp_absorption_dark_resolutions;
            total.hp_absorption_miracle_first_uses += output.hp_absorption_miracle_first_uses;
            total.hp_absorption_miracle_recasts += output.hp_absorption_miracle_recasts;
            total.hp_absorption_miracle_mp_spent += output.hp_absorption_miracle_mp_spent;
            total.hp_absorption_learned_miracles += output.hp_absorption_learned_miracles;
            total.hp_absorption_miracle_attacks += output.hp_absorption_miracle_attacks;
            total.hp_absorption_miracle_hp_damage += output.hp_absorption_miracle_hp_damage;
            total.forgetting_hp_absorption += output.forgetting_hp_absorption;
            total.hp_absorption_raw_max = std::max(total.hp_absorption_raw_max, output.hp_absorption_raw_max);
            total.self_damage_attacks += output.self_damage_attacks;
            total.self_damage_resolutions += output.self_damage_resolutions;
            total.self_damage_self_resolutions += output.self_damage_self_resolutions;
            total.self_damage_zero_residual += output.self_damage_zero_residual;
            total.self_damage_zero_source_loss += output.self_damage_zero_source_loss;
            total.self_damage_source_lost += output.self_damage_source_lost;
            total.self_damage_target_lost += output.self_damage_target_lost;
            total.self_damage_capped_source += output.self_damage_capped_source;
            total.self_damage_capped_target += output.self_damage_capped_target;
            total.self_damage_reflections += output.self_damage_reflections;
            total.self_damage_revival_steps += output.self_damage_revival_steps;
            total.self_damage_raw_max = std::max(total.self_damage_raw_max, output.self_damage_raw_max);
            total.pestle_attacks += output.pestle_attacks;
            total.pestle_mortar_locked_attacks += output.pestle_mortar_locked_attacks;
            total.pestle_random_target_attacks += output.pestle_random_target_attacks;
            total.pestle_initial_self_targets += output.pestle_initial_self_targets;
            total.pestle_initial_opponent_targets += output.pestle_initial_opponent_targets;
            total.pestle_resolutions += output.pestle_resolutions;
            total.pestle_self_resolutions += output.pestle_self_resolutions;
            total.pestle_zero_residual += output.pestle_zero_residual;
            total.pestle_target_lost += output.pestle_target_lost;
            total.pestle_capped += output.pestle_capped;
            total.pestle_reflections += output.pestle_reflections;
            total.pestle_blocks += output.pestle_blocks;
            total.pestle_revival_steps += output.pestle_revival_steps;
            total.mortar_discards_ignored += output.mortar_discards_ignored;
            total.item_removed_mortars += output.item_removed_mortars;
            total.pestle_raw_max = std::max(total.pestle_raw_max, output.pestle_raw_max);
            total.sales += output.sales;
            total.sale_resolutions += output.sale_resolutions;
            total.sale_delivered += output.sale_delivered;
            total.sale_delivery_failures += output.sale_delivery_failures;
            total.sale_self_resolutions += output.sale_self_resolutions;
            total.sale_opponent_resolutions += output.sale_opponent_resolutions;
            total.sale_zero_price += output.sale_zero_price;
            total.sale_price_total += output.sale_price_total;
            total.sale_money_lost += output.sale_money_lost;
            total.sale_mp_lost += output.sale_mp_lost;
            total.sale_hp_lost += output.sale_hp_lost;
            total.sale_money_gained += output.sale_money_gained;
            total.sale_reflections += output.sale_reflections;
            total.sale_revival_steps += output.sale_revival_steps;
            total.sale_weapons += output.sale_weapons;
            total.sale_armor += output.sale_armor;
            total.sale_miracles += output.sale_miracles;
            total.sale_other += output.sale_other;
            total.buy_uses += output.buy_uses;
            total.buy_attempts += output.buy_attempts;
            total.buy_empty += output.buy_empty;
            total.buy_self_selections += output.buy_self_selections;
            total.buy_insufficient += output.buy_insufficient;
            total.buy_offers += output.buy_offers;
            total.buy_decisions += output.buy_decisions;
            total.buy_declines += output.buy_declines;
            total.buy_purchases += output.buy_purchases;
            total.buy_delivered += output.buy_delivered;
            total.buy_delivery_failures += output.buy_delivery_failures;
            total.buy_money_spent += output.buy_money_spent;
            total.buy_money_gained += output.buy_money_gained;
            total.buy_zero_price += output.buy_zero_price;
            total.buy_reflections += output.buy_reflections;
            total.buy_active_offers += output.buy_active_offers;
            total.buy_revival_steps += output.buy_revival_steps;
            total.bought_weapons += output.bought_weapons;
            total.bought_armor += output.bought_armor;
            total.bought_miracles += output.bought_miracles;
            total.bought_other += output.bought_other;
            total.sold_buy_cards += output.sold_buy_cards;
            total.hp_gamble_uses += output.hp_gamble_uses;
            total.hp_gamble_heal_rolls += output.hp_gamble_heal_rolls;
            total.hp_gamble_damage_rolls += output.hp_gamble_damage_rolls;
            total.hp_gamble_initial_self_targets += output.hp_gamble_initial_self_targets;
            total.hp_gamble_initial_opponent_targets += output.hp_gamble_initial_opponent_targets;
            total.hp_gamble_resolutions += output.hp_gamble_resolutions;
            total.hp_gamble_healing_resolutions += output.hp_gamble_healing_resolutions;
            total.hp_gamble_damage_resolutions += output.hp_gamble_damage_resolutions;
            total.hp_gamble_self_resolutions += output.hp_gamble_self_resolutions;
            total.hp_gamble_healed += output.hp_gamble_healed;
            total.hp_gamble_damaged += output.hp_gamble_damaged;
            total.hp_gamble_zero_change += output.hp_gamble_zero_change;
            total.hp_gamble_capped_heals += output.hp_gamble_capped_heals;
            total.hp_gamble_capped_damage += output.hp_gamble_capped_damage;
            total.hp_gamble_reflections += output.hp_gamble_reflections;
            total.hp_gamble_revival_steps += output.hp_gamble_revival_steps;
            total.sold_hp_gambles += output.sold_hp_gambles;
            total.bought_hp_gambles += output.bought_hp_gambles;
            total.magic_card_bounces += output.magic_card_bounces;
            total.magic_card_self_bounces += output.magic_card_self_bounces;
            total.magic_card_damage_bounces += output.magic_card_damage_bounces;
            total.magic_card_utility_bounces += output.magic_card_utility_bounces;
            total.magic_card_held_spell_bounces += output.magic_card_held_spell_bounces;
            total.magic_card_free_spell_bounces += output.magic_card_free_spell_bounces;
            total.magic_card_removal_bounces += output.magic_card_removal_bounces;
            total.magic_bounce_weapon_attacks += output.magic_bounce_weapon_attacks;
            total.magic_bounce_boost_uses += output.magic_bounce_boost_uses;
            total.magic_bounce_ordinary_cards_used += output.magic_bounce_ordinary_cards_used;
            total.sold_magic_bounce_cards += output.sold_magic_bounce_cards;
            total.bought_magic_bounce_cards += output.bought_magic_bounce_cards;
            total.sundry_boost_attacks += output.sundry_boost_attacks;
            total.sundry_boost_cards_used += output.sundry_boost_cards_used;
            total.sundry_boost_plain_attacks += output.sundry_boost_plain_attacks;
            total.sundry_boost_ordered_attacks += output.sundry_boost_ordered_attacks;
            total.sundry_boost_mana_attacks += output.sundry_boost_mana_attacks;
            total.sundry_boost_mana_resets += output.sundry_boost_mana_resets;
            total.sundry_boost_resets_after_boost += output.sundry_boost_resets_after_boost;
            total.sundry_boost_double_attacks += output.sundry_boost_double_attacks;
            total.sundry_boost_wide_attacks += output.sundry_boost_wide_attacks;
            total.sundry_boost_reaction_steps += output.sundry_boost_reaction_steps;
            total.sundry_boost_counters_queued += output.sundry_boost_counters_queued;
            total.sundry_boost_discarded_cards += output.sundry_boost_discarded_cards;
            total.sundry_boost_sold_cards += output.sundry_boost_sold_cards;
            total.sundry_boost_bought_cards += output.sundry_boost_bought_cards;
            total.sundry_boost_removed_cards += output.sundry_boost_removed_cards;
            total.sundry_boost_raw_checked += output.sundry_boost_raw_checked;
            total.sundry_boost_self_attacks += output.sundry_boost_self_attacks;
            total.sundry_boost_raw_max = std::max(total.sundry_boost_raw_max, output.sundry_boost_raw_max);
            total.status_first_uses += output.status_first_uses;
            total.status_recasts += output.status_recasts;
            total.status_mp_spent += output.status_mp_spent;
            total.status_learned += output.status_learned;
            total.status_casts += output.status_casts;
            total.status_resolutions += output.status_resolutions;
            total.status_self_resolutions += output.status_self_resolutions;
            total.status_lower_cures += output.status_lower_cures;
            total.status_all_cures += output.status_all_cures;
            total.status_dark_cloud_applications += output.status_dark_cloud_applications;
            total.status_cold_applications += output.status_cold_applications;
            total.status_heaven_applications += output.status_heaven_applications;
            total.status_heaven_falls += output.status_heaven_falls;
            total.status_disease_changes += output.status_disease_changes;
            total.status_diseases_removed += output.status_diseases_removed;
            total.status_glory_cleared += output.status_glory_cleared;
            total.status_dark_cloud_cleared += output.status_dark_cloud_cleared;
            total.status_dark_cloud_new += output.status_dark_cloud_new;
            total.status_hp_lost += output.status_hp_lost;
            total.status_free_uses += output.status_free_uses;
            total.status_reflections += output.status_reflections;
            total.status_bounces += output.status_bounces;
            total.status_blocks += output.status_blocks;
            total.status_held_resolutions += output.status_held_resolutions;
            total.forgetting_status_miracles += output.forgetting_status_miracles;
            total.status_retained_inventory += output.status_retained_inventory;
            total.sold_status_miracles += output.sold_status_miracles;
            total.bought_status_miracles += output.bought_status_miracles;
            total.removed_status_miracles += output.removed_status_miracles;
            for (std::size_t i = 0; i < total.sundry_boost_elements.size(); ++i)
                total.sundry_boost_elements[i] += output.sundry_boost_elements[i];
            for (std::size_t i = 0; i < total.magic_card_bounces_by_id.size(); ++i)
                total.magic_card_bounces_by_id[i] += output.magic_card_bounces_by_id[i];
            total.item_removal_uses += output.item_removal_uses;
            total.item_removal_attempts += output.item_removal_attempts;
            total.items_removed += output.items_removed;
            total.item_removal_empty += output.item_removal_empty;
            total.item_removal_self_targets += output.item_removal_self_targets;
            total.item_removal_opponent_targets += output.item_removal_opponent_targets;
            total.item_removal_reflections += output.item_removal_reflections;
            total.item_removed_weapons += output.item_removed_weapons;
            total.item_removed_armor += output.item_removed_armor;
            total.item_removed_miracles += output.item_removed_miracles;
            total.item_removed_other += output.item_removed_other;
            total.item_removed_revivals += output.item_removed_revivals;
            total.revivals += output.revivals;
            total.revival_steps += output.revival_steps;
            total.revival_multiple_steps += output.revival_multiple_steps;
            total.revival_both_seats += output.revival_both_seats;
            for (int seat = 0; seat < 2; ++seat) total.revivals_by_seat[seat] += output.revivals_by_seat[seat];
            total.revival_hp_restored += output.revival_hp_restored;
            total.exchange_revival_steps += output.exchange_revival_steps;
            total.exchanges += output.exchanges;
            total.exchange_noops += output.exchange_noops;
            total.exchange_suicides += output.exchange_suicides;
            total.exchange_tick_damage += output.exchange_tick_damage;
            total.exchange_tick_healing += output.exchange_tick_healing;
            for (int i = 0; i < 3; ++i) {
                total.exchange_up[i] += output.exchange_up[i];
                total.exchange_down[i] += output.exchange_down[i];
            }
            total.forgetting_uses += output.forgetting_uses;
            total.forgetting_attempts += output.forgetting_attempts;
            total.forgetting_removed += output.forgetting_removed;
            total.forgetting_empty += output.forgetting_empty;
            total.forgetting_self_targets += output.forgetting_self_targets;
            total.forgetting_opponent_targets += output.forgetting_opponent_targets;
            total.forgetting_reflections += output.forgetting_reflections;
            total.forgetting_utility += output.forgetting_utility;
            total.forgetting_offensive += output.forgetting_offensive;
            total.forgetting_defensive += output.forgetting_defensive;
            total.forgetting_additive += output.forgetting_additive;
            total.forgetting_doubling += output.forgetting_doubling;
            total.forgetting_wide += output.forgetting_wide;
            total.retained_inventory += output.retained_inventory;
            total.magic_free_cards += output.magic_free_cards;
            total.magic_free_attack_cards += output.magic_free_attack_cards;
            total.magic_free_cast_cards += output.magic_free_cast_cards;
            total.magic_free_defense_cards += output.magic_free_defense_cards;
            total.magic_free_mp_saved += output.magic_free_mp_saved;
            total.magic_free_attack_mp_saved += output.magic_free_attack_mp_saved;
            total.magic_free_cast_mp_saved += output.magic_free_cast_mp_saved;
            total.magic_free_defense_mp_saved += output.magic_free_defense_mp_saved;
            total.magic_free_suffixes += output.magic_free_suffixes;
            total.free_casts += output.free_casts;
            total.ordered_defenses += output.ordered_defenses;
            total.wide_first_uses += output.wide_first_uses;
            total.wide_recasts += output.wide_recasts;
            total.wide_mp_spent += output.wide_mp_spent;
            total.wide_learned += output.wide_learned;
            total.wide_attacks += output.wide_attacks;
            total.wide_uses += output.wide_uses;
            total.wide_chance_attempts += output.wide_chance_attempts;
            total.wide_chance_hits += output.wide_chance_hits;
            total.wide_forced_hits += output.wide_forced_hits;
            total.wide_weapon_reactions += output.wide_weapon_reactions;
            total.wide_removal_responses += output.wide_removal_responses;
            total.doubling_first_uses += output.doubling_first_uses;
            total.doubling_recasts += output.doubling_recasts;
            total.doubling_mp_spent += output.doubling_mp_spent;
            total.doubling_learned += output.doubling_learned;
            total.doubling_attacks += output.doubling_attacks;
            total.doubling_uses += output.doubling_uses;
            total.additive_first_uses += output.additive_first_uses;
            total.additive_recasts += output.additive_recasts;
            total.additive_mp_spent += output.additive_mp_spent;
            total.additive_learned += output.additive_learned;
            total.additive_single_attacks += output.additive_single_attacks;
            total.additive_single_resolutions += output.additive_single_resolutions;
            total.additive_single_hp_damage += output.additive_single_hp_damage;
            total.additive_extra_uses += output.additive_extra_uses;
            total.ordered_attacks += output.ordered_attacks;
            total.ordered_components += output.ordered_components;
            total.mana_weapon_zero_burn_attacks += output.mana_weapon_zero_burn_attacks;
            total.checksum += output.checksum;
        }
        if (total.wins[0] + total.wins[1] + total.draws + total.truncations != games)
            throw std::runtime_error("outcome accounting mismatch");
        if (total.chance_hits + total.chance_misses != total.chance_attempts)
            throw std::runtime_error("chance accounting mismatch");
        if (total.reflections + total.bounces != total.weapon_reactions || total.self_bounces > total.bounces)
            throw std::runtime_error("weapon reaction accounting mismatch");
        if (total.disease_worsenings > total.disease_ticks)
            throw std::runtime_error("disease tick accounting mismatch");
        if (total.forced_chance_hits > total.chance_hits
            || total.harm_new_flags > total.harm_applications
            || total.harm_heaven_falls > total.harm_applications)
            throw std::runtime_error("harm accounting mismatch");
        if (total.offensive_miracle_first_uses + total.offensive_miracle_recasts != total.offensive_miracle_attacks
            || total.offensive_miracle_chance_attempts > total.offensive_miracle_attacks
            || total.offensive_miracle_resolutions > total.offensive_miracle_attacks)
            throw std::runtime_error("offensive miracle accounting mismatch");
        if (total.magic_self_bounces > total.magic_bounces
            || total.magic_bounces + total.weapon_blocks
                != total.defensive_miracle_first_uses + total.defensive_miracle_recasts + total.magic_card_bounces
            || total.defensive_learned_miracles != total.defensive_miracle_first_uses)
            throw std::runtime_error("defensive miracle accounting mismatch");
        if (total.attribute_fire_attacks + total.attribute_water_attacks + total.attribute_neutral_attacks
                != total.attribute_attacks || total.attribute_cards_used < total.attribute_attacks)
            throw std::runtime_error("attribute attack accounting mismatch");
        std::uint64_t removed_element_uses = 0;
        for (const auto count : total.removed_from_elements) removed_element_uses += count;
        if (removed_element_uses != total.attribute_removal_uses
            || total.removal_weapon_responses + total.removal_magic_responses
                + total.counter_removal_responses + total.removal_utility_responses != total.attribute_removal_uses
            || total.attribute_removal_cards_used < total.attribute_removal_uses
            || total.removal_weapon_reactions + total.removal_weapon_blocks > total.removal_weapon_responses
            || total.removal_magic_bounces + total.removal_magic_reflections + total.removal_magic_blocks > total.removal_magic_responses
            || total.neutralized_percentage_weapon_reactions > total.neutralized_weapon_reactions)
            throw std::runtime_error("attribute removal accounting mismatch");
        if (total.counters_queued != total.counters_launched + total.counters_skipped + total.counter_missed + total.counter_queue_remaining
            || total.counters_launched != total.counters_resolved + total.counter_auto_resolved + total.counters_cancelled + total.counter_active_remaining
            || total.counters_queued > total.counter_cards_used
            || total.counters_resolved + total.counters_cancelled > total.counter_defense_decisions)
            throw std::runtime_error("counter accounting mismatch");
        if (total.counter_chance_hits + total.counter_chance_misses != total.counter_chance_attempts
            || total.counter_chance_forced > total.counter_chance_hits
            || total.counter_missed != total.counter_chance_misses
            || total.counter_hidden_checks > total.counters_skipped)
            throw std::runtime_error("extended counter accounting mismatch");
        if (total.counter_money_absorptions > total.counters_resolved
            || total.counter_money_gained > total.counter_money_absorptions * 99
            || total.counter_money_lost > total.counter_money_absorptions * 99
            || total.counter_mp_lost > total.counter_money_absorptions * 99
            || total.counter_hp_lost > total.counter_money_absorptions * 99)
            throw std::runtime_error("money counter accounting mismatch");
        if (total.any_weapon_reflections + total.any_magic_reflections + total.any_counter_reflections
                + total.any_utility_reflections != total.any_reflections
            || total.removal_any_reflections > total.any_reflections
            || total.removal_any_reflections > total.attribute_removal_uses)
            throw std::runtime_error("universal reflection accounting mismatch");
        // Added miracles can spend all initial MP before MAGICAL's final burn.
        // A zero-power57 therefore requires zero final burn, not zero initial MP.
        if (total.mana_weapon_zero_power_attacks > total.mana_weapon_zero_burn_attacks
            || total.mana_weapon_zero_mp_attacks > total.mana_weapon_attacks
            || total.mana_weapon_setter_attacks > total.mana_weapon_attacks
            || total.mana_weapon_mp_spent > total.mana_weapon_attacks * 99
            || total.mana_weapon_power_total > total.mana_weapon_attacks * (config.magic_free ? 1194 : config.doubling_miracles ? 597 : 423))
            throw std::runtime_error("mana weapon accounting mismatch");
        if (total.magic_damage_blocks + total.magic_utility_blocks != total.magic_blocks
            || total.removal_magic_blocks > total.magic_blocks)
            throw std::runtime_error("magic block accounting mismatch");
        if (total.additive_first_uses + total.additive_recasts != total.additive_single_attacks + total.additive_extra_uses
            || total.additive_learned != total.additive_first_uses
            || total.additive_single_resolutions > total.additive_single_attacks
            || total.ordered_components < total.ordered_attacks
            || total.mana_weapon_zero_burn_attacks > total.mana_weapon_attacks)
            throw std::runtime_error("additive miracle accounting mismatch");
        if (total.doubling_first_uses + total.doubling_recasts != total.doubling_uses
            || total.doubling_learned != total.doubling_first_uses
            || total.doubling_mp_spent > total.doubling_uses * 6
            || (!config.magic_free && total.doubling_mp_spent != total.doubling_uses * 6)
            || total.doubling_attacks > total.ordered_attacks
            || total.doubling_uses < total.doubling_attacks
            || total.doubling_uses > total.doubling_attacks * (config.magic_free ? 17 : 16))
            throw std::runtime_error("doubling miracle accounting mismatch");
        if (total.wide_first_uses + total.wide_recasts != total.wide_uses
            || total.wide_learned != total.wide_first_uses
            || total.wide_mp_spent > total.wide_uses * 5
            || (!config.magic_free && total.wide_mp_spent != total.wide_uses * 5)
            || total.wide_attacks > total.ordered_attacks
            || total.wide_uses < total.wide_attacks || total.wide_uses > total.wide_attacks * (config.magic_free ? 20 : 19)
            || total.wide_chance_attempts != total.wide_attacks || total.wide_chance_hits != total.wide_attacks
            || total.wide_forced_hits > total.wide_chance_hits
            || total.wide_weapon_reactions > total.weapon_reactions
            || total.wide_removal_responses > total.attribute_removal_uses)
            throw std::runtime_error("wide miracle accounting mismatch");
        if (total.magic_free_attack_cards + total.magic_free_cast_cards + total.magic_free_defense_cards != total.magic_free_cards
            || total.magic_free_attack_mp_saved + total.magic_free_cast_mp_saved + total.magic_free_defense_mp_saved != total.magic_free_mp_saved
            || total.magic_free_suffixes > total.ordered_defenses)
            throw std::runtime_error("MAGIC_FREE accounting mismatch");
        if (total.forgetting_removed + total.forgetting_empty != total.forgetting_attempts
            || total.forgetting_self_targets + total.forgetting_opponent_targets != total.forgetting_attempts
            || total.forgetting_utility + total.forgetting_offensive + total.forgetting_defensive
                + total.forgetting_additive + total.forgetting_doubling + total.forgetting_wide + total.forgetting_hp_absorption + total.forgetting_status_miracles != total.forgetting_removed
            || (config.forgetting && total.retained_inventory + total.forgetting_removed != total.learned_miracles))
            throw std::runtime_error("forgetting inventory accounting mismatch");
        if (total.exchange_noops > total.exchanges || total.exchange_suicides > total.exchanges
            || total.exchange_suicides > total.wins[0] + total.wins[1]
            || total.exchange_up[0] + total.exchange_up[1] + total.exchange_up[2]
                != total.exchange_down[0] + total.exchange_down[1] + total.exchange_down[2])
            throw std::runtime_error("exchange resource accounting mismatch");
        if (total.revivals_by_seat[0] + total.revivals_by_seat[1] != total.revivals
            || total.revival_hp_restored != total.revivals * 10
            || total.revival_steps > total.revivals || total.revival_multiple_steps > total.revival_steps
            || total.revival_both_seats > total.revival_multiple_steps
            || total.exchange_revival_steps > total.exchanges || total.exchange_revival_steps > total.revival_steps)
            throw std::runtime_error("revival accounting mismatch");
        if (total.items_removed + total.item_removal_empty != total.item_removal_attempts
            || total.item_removal_attempts > total.item_removal_uses
            || total.item_removal_self_targets + total.item_removal_opponent_targets != total.item_removal_attempts
            || total.item_removed_weapons + total.item_removed_armor + total.item_removed_miracles + total.item_removed_other != total.items_removed
            || total.item_removed_revivals > total.item_removed_other)
            throw std::runtime_error("item removal accounting mismatch");
        if (total.status_first_uses + total.status_recasts != total.status_casts || total.status_learned != total.status_first_uses
            || total.status_lower_cures + total.status_all_cures + total.status_dark_cloud_applications
                + total.status_cold_applications + total.status_heaven_applications != total.status_resolutions
            || total.status_resolutions > total.status_casts || total.status_self_resolutions > total.status_resolutions
            || total.status_held_resolutions > total.status_resolutions || total.status_free_uses > total.status_casts
            || total.status_retained_inventory + total.forgetting_status_miracles != total.status_learned
            || total.status_heaven_falls > total.status_resolutions || total.status_hp_lost > 99 * total.status_heaven_falls
            || total.sold_status_miracles > total.sale_resolutions || total.bought_status_miracles > total.buy_purchases
            || total.removed_status_miracles > total.items_removed)
            throw std::runtime_error("status miracle accounting mismatch");
        std::uint64_t sundry_element_total = 0;
        for (const auto count : total.sundry_boost_elements) sundry_element_total += count;
        if (total.sundry_boost_plain_attacks + total.sundry_boost_ordered_attacks != total.sundry_boost_attacks
            || total.sundry_boost_cards_used < total.sundry_boost_attacks
            || total.sundry_boost_cards_used > 15 * total.sundry_boost_attacks
            || sundry_element_total != total.sundry_boost_attacks
            || total.sundry_boost_mana_attacks > total.sundry_boost_attacks
            || total.sundry_boost_resets_after_boost > total.sundry_boost_mana_resets
            || total.sundry_boost_double_attacks > total.sundry_boost_ordered_attacks
            || total.sundry_boost_wide_attacks > total.sundry_boost_ordered_attacks
            || total.sundry_boost_raw_checked > total.sundry_boost_attacks
            || total.sundry_boost_self_attacks > total.sundry_boost_attacks
            || total.sundry_boost_sold_cards > total.sale_resolutions || total.sundry_boost_bought_cards > total.buy_purchases
            || total.sundry_boost_removed_cards > total.items_removed)
            throw std::runtime_error("SUNDRY boost accounting mismatch");
        std::uint64_t magic_card_by_id_total = 0;
        for (const auto count : total.magic_card_bounces_by_id) magic_card_by_id_total += count;
        if (magic_card_by_id_total != total.magic_card_bounces
            || total.magic_card_damage_bounces + total.magic_card_utility_bounces != total.magic_card_bounces
            || total.magic_card_self_bounces > total.magic_card_bounces
            || total.magic_card_held_spell_bounces > total.magic_card_bounces
            || total.magic_card_free_spell_bounces > total.magic_card_bounces
            || total.magic_card_removal_bounces > total.magic_card_bounces
            || total.magic_card_bounces > total.magic_bounces
            || total.magic_card_self_bounces > total.magic_self_bounces
            || total.sold_magic_bounce_cards > total.sale_resolutions || total.bought_magic_bounce_cards > total.buy_purchases)
            throw std::runtime_error("physical MAGIC bounce accounting mismatch");
        if (total.hp_gamble_heal_rolls + total.hp_gamble_damage_rolls != total.hp_gamble_uses
            || total.hp_gamble_initial_self_targets + total.hp_gamble_initial_opponent_targets != total.hp_gamble_uses
            || total.hp_gamble_healing_resolutions + total.hp_gamble_damage_resolutions != total.hp_gamble_resolutions
            || total.hp_gamble_resolutions > total.hp_gamble_uses
            || total.hp_gamble_healing_resolutions > total.hp_gamble_heal_rolls
            || total.hp_gamble_damage_resolutions > total.hp_gamble_damage_rolls
            || total.hp_gamble_self_resolutions > total.hp_gamble_resolutions
            || total.hp_gamble_zero_change > total.hp_gamble_resolutions
            || total.hp_gamble_healed > total.hp_gamble_healing_resolutions * 10
            || total.hp_gamble_damaged > total.hp_gamble_damage_resolutions * 10
            || total.hp_gamble_capped_heals > total.hp_gamble_healing_resolutions
            || total.hp_gamble_capped_damage > total.hp_gamble_damage_resolutions
            || total.hp_gamble_revival_steps > total.hp_gamble_resolutions
            || total.sold_hp_gambles > total.sale_resolutions || total.bought_hp_gambles > total.buy_purchases)
            throw std::runtime_error("HP gamble accounting mismatch");
        if (total.buy_empty + total.buy_self_selections + total.buy_insufficient + total.buy_offers != total.buy_attempts
            || total.buy_decisions != total.buy_declines + total.buy_purchases
            || total.buy_decisions + total.buy_active_offers != total.buy_offers
            || total.buy_delivered + total.buy_delivery_failures != total.buy_purchases
            || total.bought_weapons + total.bought_armor + total.bought_miracles + total.bought_other != total.buy_purchases
            || total.buy_attempts > total.buy_uses || total.buy_zero_price > total.buy_purchases
            || total.buy_money_spent > total.buy_purchases * godfield::legacy::max_sale_price
            || total.buy_money_gained > total.buy_money_spent || total.buy_revival_steps > total.buy_purchases
            || total.sold_buy_cards > total.sale_resolutions)
            throw std::runtime_error("buying accounting mismatch");
        if (total.sale_delivered + total.sale_delivery_failures != total.sale_resolutions
            || total.sale_self_resolutions + total.sale_opponent_resolutions != total.sale_resolutions
            || total.sale_weapons + total.sale_armor + total.sale_miracles + total.sale_other != total.sale_resolutions
            || total.sale_resolutions > total.sales || total.sale_zero_price > total.sale_resolutions
            || total.sale_revival_steps > total.sale_resolutions
            || total.sale_price_total > total.sale_resolutions * godfield::legacy::max_sale_price
            || total.sale_money_lost + total.sale_mp_lost + total.sale_hp_lost > total.sale_price_total
            || total.sale_money_gained > total.sale_price_total)
            throw std::runtime_error("selling accounting mismatch");
        if (total.pestle_mortar_locked_attacks + total.pestle_random_target_attacks != total.pestle_attacks
            || total.pestle_initial_self_targets + total.pestle_initial_opponent_targets != total.pestle_attacks
            || total.pestle_resolutions > total.pestle_attacks
            || total.pestle_self_resolutions > total.pestle_resolutions
            || total.pestle_zero_residual > total.pestle_resolutions
            || total.pestle_capped > total.pestle_resolutions
            || total.pestle_target_lost > total.pestle_resolutions * 99
            || total.pestle_revival_steps > total.pestle_resolutions
            || total.pestle_raw_max > 999 || total.item_removed_mortars > total.item_removed_other)
            throw std::runtime_error("pestle/mortar accounting mismatch");
        if (total.self_damage_self_resolutions > total.self_damage_resolutions
            || total.self_damage_zero_residual > total.self_damage_resolutions
            || total.self_damage_zero_source_loss > total.self_damage_resolutions
            || total.self_damage_capped_source > total.self_damage_resolutions
            || total.self_damage_capped_target > total.self_damage_resolutions
            || total.self_damage_source_lost > total.self_damage_resolutions * 99
            || total.self_damage_target_lost > total.self_damage_resolutions * 99
            || total.self_damage_revival_steps > total.self_damage_resolutions)
            throw std::runtime_error("self damage accounting mismatch");
        if (total.hp_absorption_weapon_resolutions + total.hp_absorption_miracle_resolutions != total.hp_absorption_resolutions
            || total.hp_absorption_self_resolutions > total.hp_absorption_resolutions
            || total.hp_absorption_zero_residual > total.hp_absorption_resolutions
            || total.hp_absorption_zero_healing > total.hp_absorption_resolutions
            || total.hp_absorption_capped > total.hp_absorption_resolutions
            || total.hp_absorption_dark_resolutions > total.hp_absorption_resolutions
            || total.hp_absorption_healing > total.hp_absorption_resolutions * 99
            || total.hp_absorption_miracle_first_uses + total.hp_absorption_miracle_recasts != total.hp_absorption_miracle_attacks
            || total.hp_absorption_learned_miracles != total.hp_absorption_miracle_first_uses
            || total.hp_absorption_miracle_mp_spent > total.hp_absorption_miracle_attacks * 10
            || total.hp_absorption_miracle_resolutions > total.hp_absorption_miracle_attacks)
            throw std::runtime_error("HP absorption accounting mismatch");
        const double elapsed = std::max(seconds, std::numeric_limits<double>::min());
        std::cout << "ruleset=" << ruleset_name
            << "\nsource=Igoorx/godfield-flash"
            << "\nprofile=not-full-official"
            << "\nselected_profile=" << selected_profile
            << "\npolicy=hierarchical-random"
            << "\ncatalog_cards=" << godfield::legacy::catalog().size()
                + (config.miracles ? godfield::legacy::miracles().size() : 0)
                + (config.elements ? godfield::legacy::element_catalog().size() : 0)
                + (config.chance ? godfield::legacy::chance_catalog().size() : 0)
                + (config.reflection ? godfield::legacy::reaction_catalog().size() : 0)
                + (config.disease ? godfield::legacy::disease_catalog().size() : 0)
                + (config.harms ? godfield::legacy::harm_catalog().size() : 0)
                + (config.celestial ? godfield::legacy::celestial_catalog().size() : 0)
                + (config.offensive_miracles ? godfield::legacy::offensive_miracles().size() : 0)
                + (config.defensive_miracles ? godfield::legacy::defensive_miracles().size() : 0)
                + (config.attribute_change ? godfield::legacy::attribute_change_catalog().size() : 0)
                + (config.attribute_removal ? godfield::legacy::attribute_removal_catalog().size() : 0)
                + (config.counters ? godfield::legacy::counter_catalog().size() : 0)
                + (config.counter_extensions ? godfield::legacy::counter_extensions_catalog().size() : 0)
                + (config.money_counter ? godfield::legacy::money_counter_catalog().size() : 0)
                + (config.magic_reflection ? godfield::legacy::magic_reflection_catalog().size() : 0)
                + (config.any_reflection ? godfield::legacy::any_reflection_catalog().size() : 0)
                + (config.mana_weapon ? godfield::legacy::mana_weapon_catalog().size() : 0)
                + (config.magic_block ? godfield::legacy::magic_block_catalog().size() : 0)
                + (config.additive_miracles ? godfield::legacy::additive_miracles().size() : 0)
                + (config.doubling_miracles ? godfield::legacy::doubling_miracles().size() : 0)
                + (config.wide_miracles ? godfield::legacy::wide_miracles().size() : 0)
                + (config.magic_free ? godfield::legacy::magic_free_catalog().size() : 0)
                + (config.forgetting ? godfield::legacy::forgetting_catalog().size() : 0)
                + (config.exchanges ? godfield::legacy::exchange_catalog().size() : 0)
                + (config.revival ? godfield::legacy::revival_catalog().size() : 0)
                + (config.item_removal ? godfield::legacy::item_removal_catalog().size() : 0)
                + (config.hp_absorption ? godfield::legacy::hp_absorption_catalog().size() + godfield::legacy::hp_absorption_miracles().size() : 0)
                + (config.self_damage ? godfield::legacy::self_damage_catalog().size() : 0)
                + (config.pestle_mortar ? godfield::legacy::pestle_catalog().size() + godfield::legacy::mortar_catalog().size() : 0)
                + (config.selling ? godfield::legacy::selling_catalog().size() : 0)
                + (config.buying ? godfield::legacy::buying_catalog().size() : 0)
                + (config.hp_gamble ? godfield::legacy::hp_gamble_catalog().size() : 0)
                + (config.magic_bounce ? godfield::legacy::magic_bounce_catalog().size() : 0)
                + (config.sundry_boost ? godfield::legacy::sundry_boost_catalog().size() : 0)
                + (config.status_miracles ? godfield::legacy::status_miracles().size() : 0)
            << "\ncatalog_weight=" << catalog_weight
            << "\ninitial_hand=" << static_cast<int>(config.initial_hand)
            << " max_decisions=" << config.max_decisions
            << "\ngames=" << games << " threads=" << threads << " seed=" << seed
            << "\nseconds=" << seconds
            << "\ndecisions=" << total.decisions
            << "\ndecisions_per_second=" << total.decisions / elapsed
            << "\ngames_per_second=" << games / elapsed
            << "\nwins=" << total.wins[0] << ',' << total.wins[1]
            << " draws=" << total.draws
            << " truncations=" << total.truncations
            << "\nmiracle_first_uses=" << total.miracle_first_uses
            << " miracle_recasts=" << total.miracle_recasts
            << " learned_miracles=" << total.learned_miracles
            << "\nmiracle_mp_spent=" << total.miracle_mp_spent
            << " miracle_hp_gained=" << total.miracle_hp_gained
            << " miracle_money_gained=" << total.miracle_money_gained
            << "\nelemental_attacks=" << total.elemental_attacks
            << " neutral_attacks=" << total.neutral_attacks
            << " light_attacks=" << total.light_attacks
            << " dark_attacks=" << total.dark_attacks
            << " dark_executions=" << total.dark_executions
            << "\neffective_elemental_defenses=" << total.effective_elemental_defenses
            << " elemental_damage_reduced=" << total.elemental_damage_reduced
            << "\nchance_attempts=" << total.chance_attempts
            << " chance_hits=" << total.chance_hits
            << " chance_misses=" << total.chance_misses
            << "\nweapon_reactions=" << total.weapon_reactions
            << " reflections=" << total.reflections
            << " bounces=" << total.bounces
            << " self_bounces=" << total.self_bounces
            << "\ndisease_ticks=" << total.disease_ticks
            << " disease_worsenings=" << total.disease_worsenings
            << " tick_damage=" << total.tick_damage
            << " tick_healing=" << total.tick_healing
            << "\nforced_chance_hits=" << total.forced_chance_hits
            << " harm_applications=" << total.harm_applications
            << " harm_new_flags=" << total.harm_new_flags
            << " harm_heaven_falls=" << total.harm_heaven_falls
            << "\nglory_defense_decisions=" << total.glory_defense_decisions
            << " glory_source_cards=" << glory_sources
            << " glory_note=" << (glory_sources ? "normal-pool-source-enabled" : "no-normal-pool-source;validate-with-fixtures")
            << "\noffensive_miracle_first_uses=" << total.offensive_miracle_first_uses
            << " offensive_miracle_recasts=" << total.offensive_miracle_recasts
            << " offensive_miracle_attacks=" << total.offensive_miracle_attacks
            << "\noffensive_miracle_mp_spent=" << total.offensive_miracle_mp_spent
            << " offensive_learned_miracles=" << total.offensive_learned_miracles
            << " offensive_miracle_resolutions=" << total.offensive_miracle_resolutions
            << " offensive_miracle_hp_damage=" << total.offensive_miracle_hp_damage
            << " offensive_miracle_chance_attempts=" << total.offensive_miracle_chance_attempts
            << "\ndefensive_miracle_first_uses=" << total.defensive_miracle_first_uses
            << " defensive_miracle_recasts=" << total.defensive_miracle_recasts
            << " defensive_learned_miracles=" << total.defensive_learned_miracles
            << "\ndefensive_miracle_mp_spent=" << total.defensive_miracle_mp_spent
            << " magic_bounces=" << total.magic_bounces
            << " magic_self_bounces=" << total.magic_self_bounces
            << " weapon_blocks=" << total.weapon_blocks
            << "\nattribute_attacks=" << total.attribute_attacks
            << " attribute_cards_used=" << total.attribute_cards_used
            << " attribute_fire_attacks=" << total.attribute_fire_attacks
            << " attribute_water_attacks=" << total.attribute_water_attacks
            << " attribute_neutral_attacks=" << total.attribute_neutral_attacks
            << " attribute_weapon_base_uses=" << total.attribute_weapon_base_uses
            << "\nattribute_removal_uses=" << total.attribute_removal_uses
            << " attribute_removal_cards_used=" << total.attribute_removal_cards_used
            << " removal_already_neutral=" << total.removed_from_elements[0]
            << " removal_from_fire=" << total.removed_from_elements[1]
            << " removal_from_water=" << total.removed_from_elements[2]
            << " removal_from_wood=" << total.removed_from_elements[3]
            << " removal_from_stone=" << total.removed_from_elements[4]
            << " removal_from_light=" << total.removed_from_elements[5]
            << " removal_from_dark=" << total.removed_from_elements[6]
            << "\nremoval_weapon_responses=" << total.removal_weapon_responses
            << " removal_magic_responses=" << total.removal_magic_responses
            << " counter_removal_responses=" << total.counter_removal_responses
            << " removal_utility_responses=" << total.removal_utility_responses
            << " removal_weapon_reactions=" << total.removal_weapon_reactions
            << " removal_magic_bounces=" << total.removal_magic_bounces
            << " removal_weapon_blocks=" << total.removal_weapon_blocks
            << " neutralized_weapon_reactions=" << total.neutralized_weapon_reactions
            << " neutralized_percentage_weapon_reactions=" << total.neutralized_percentage_weapon_reactions
            << "\ncounters_queued=" << total.counters_queued
            << " counters_launched=" << total.counters_launched
            << " counters_skipped=" << total.counters_skipped
            << " counters_resolved=" << total.counters_resolved
            << " counter_defense_decisions=" << total.counter_defense_decisions
            << " counter_cards_used=" << total.counter_cards_used
            << " counter_queue_remaining=" << total.counter_queue_remaining
            << " counter_active_remaining=" << total.counter_active_remaining
            << "\ncounter_missed=" << total.counter_missed
            << " counter_auto_resolved=" << total.counter_auto_resolved
            << " counter_chance_attempts=" << total.counter_chance_attempts
            << " counter_chance_hits=" << total.counter_chance_hits
            << " counter_chance_misses=" << total.counter_chance_misses
            << " counter_chance_forced=" << total.counter_chance_forced
            << " counter_hidden_checks=" << total.counter_hidden_checks
            << " counter_mp_gained=" << total.counter_mp_gained
            << " counter_harm_applications=" << total.counter_harm_applications
            << "\ncounter_money_absorptions=" << total.counter_money_absorptions
            << " counter_money_gained=" << total.counter_money_gained
            << " counter_money_lost=" << total.counter_money_lost
            << " counter_mp_lost=" << total.counter_mp_lost
            << " counter_hp_lost=" << total.counter_hp_lost
            << "\nmagic_reflections=" << total.magic_reflections
            << " removal_magic_reflections=" << total.removal_magic_reflections
            << "\nany_reflections=" << total.any_reflections
            << " any_weapon_reflections=" << total.any_weapon_reflections
            << " any_magic_reflections=" << total.any_magic_reflections
            << " any_counter_reflections=" << total.any_counter_reflections
            << " any_utility_reflections=" << total.any_utility_reflections
            << " removal_any_reflections=" << total.removal_any_reflections
            << " counters_cancelled=" << total.counters_cancelled
            << "\nmana_weapon_attacks=" << total.mana_weapon_attacks
            << " mana_weapon_mp_spent=" << total.mana_weapon_mp_spent
            << " mana_weapon_power_total=" << total.mana_weapon_power_total
            << " mana_weapon_zero_mp_attacks=" << total.mana_weapon_zero_mp_attacks
            << " mana_weapon_zero_power_attacks=" << total.mana_weapon_zero_power_attacks
            << " mana_weapon_setter_attacks=" << total.mana_weapon_setter_attacks
            << "\nmagic_blocks=" << total.magic_blocks
            << " magic_damage_blocks=" << total.magic_damage_blocks
            << " magic_utility_blocks=" << total.magic_utility_blocks
            << " removal_magic_blocks=" << total.removal_magic_blocks
            << "\nadditive_first_uses=" << total.additive_first_uses
            << " additive_recasts=" << total.additive_recasts
            << " additive_mp_spent=" << total.additive_mp_spent
            << " additive_learned=" << total.additive_learned
            << " additive_single_attacks=" << total.additive_single_attacks
            << " additive_single_resolutions=" << total.additive_single_resolutions
            << " additive_single_hp_damage=" << total.additive_single_hp_damage
            << " additive_extra_uses=" << total.additive_extra_uses
            << " ordered_attacks=" << total.ordered_attacks
            << " ordered_components=" << total.ordered_components
            << " mana_weapon_zero_burn_attacks=" << total.mana_weapon_zero_burn_attacks
            << "\ndoubling_first_uses=" << total.doubling_first_uses
            << " doubling_recasts=" << total.doubling_recasts
            << " doubling_mp_spent=" << total.doubling_mp_spent
            << " doubling_learned=" << total.doubling_learned
            << " doubling_attacks=" << total.doubling_attacks
            << " doubling_uses=" << total.doubling_uses
            << "\nwide_first_uses=" << total.wide_first_uses
            << " wide_recasts=" << total.wide_recasts
            << " wide_mp_spent=" << total.wide_mp_spent
            << " wide_learned=" << total.wide_learned
            << " wide_attacks=" << total.wide_attacks
            << " wide_uses=" << total.wide_uses
            << " wide_chance_attempts=" << total.wide_chance_attempts
            << " wide_chance_hits=" << total.wide_chance_hits
            << " wide_forced_hits=" << total.wide_forced_hits
            << " wide_weapon_reactions=" << total.wide_weapon_reactions
            << " wide_removal_responses=" << total.wide_removal_responses
            << "\nmagic_free_cards=" << total.magic_free_cards
            << " magic_free_attack_cards=" << total.magic_free_attack_cards
            << " magic_free_cast_cards=" << total.magic_free_cast_cards
            << " magic_free_defense_cards=" << total.magic_free_defense_cards
            << " magic_free_mp_saved=" << total.magic_free_mp_saved
            << " magic_free_attack_mp_saved=" << total.magic_free_attack_mp_saved
            << " magic_free_cast_mp_saved=" << total.magic_free_cast_mp_saved
            << " magic_free_defense_mp_saved=" << total.magic_free_defense_mp_saved
            << " magic_free_suffixes=" << total.magic_free_suffixes
            << " free_casts=" << total.free_casts
            << " ordered_defenses=" << total.ordered_defenses
            << "\nforgetting_uses=" << total.forgetting_uses
            << " forgetting_attempts=" << total.forgetting_attempts
            << " forgetting_removed=" << total.forgetting_removed
            << " forgetting_empty=" << total.forgetting_empty
            << " forgetting_self_targets=" << total.forgetting_self_targets
            << " forgetting_opponent_targets=" << total.forgetting_opponent_targets
            << " forgetting_reflections=" << total.forgetting_reflections
            << " forgetting_utility=" << total.forgetting_utility
            << " forgetting_offensive=" << total.forgetting_offensive
            << " forgetting_defensive=" << total.forgetting_defensive
            << " forgetting_additive=" << total.forgetting_additive
            << " forgetting_doubling=" << total.forgetting_doubling
            << " forgetting_wide=" << total.forgetting_wide
            << " retained_inventory=" << total.retained_inventory
            << "\nexchanges=" << total.exchanges
            << " exchange_noops=" << total.exchange_noops
            << " exchange_suicides=" << total.exchange_suicides
            << " exchange_hp_up=" << total.exchange_up[0]
            << " exchange_hp_down=" << total.exchange_down[0]
            << " exchange_mp_up=" << total.exchange_up[1]
            << " exchange_mp_down=" << total.exchange_down[1]
            << " exchange_money_up=" << total.exchange_up[2]
            << " exchange_money_down=" << total.exchange_down[2]
            << " exchange_tick_damage=" << total.exchange_tick_damage
            << " exchange_tick_healing=" << total.exchange_tick_healing
            << "\nrevivals=" << total.revivals
            << " revivals_seat0=" << total.revivals_by_seat[0]
            << " revivals_seat1=" << total.revivals_by_seat[1]
            << " revival_steps=" << total.revival_steps
            << " revival_multiple_steps=" << total.revival_multiple_steps
            << " revival_both_seats=" << total.revival_both_seats
            << " revival_hp_restored=" << total.revival_hp_restored
            << " exchange_revival_steps=" << total.exchange_revival_steps
            << "\nitem_removal_uses=" << total.item_removal_uses
            << " item_removal_attempts=" << total.item_removal_attempts
            << " items_removed=" << total.items_removed
            << " item_removal_empty=" << total.item_removal_empty
            << " item_removal_self_targets=" << total.item_removal_self_targets
            << " item_removal_opponent_targets=" << total.item_removal_opponent_targets
            << " item_removal_reflections=" << total.item_removal_reflections
            << " item_removed_weapons=" << total.item_removed_weapons
            << " item_removed_armor=" << total.item_removed_armor
            << " item_removed_miracles=" << total.item_removed_miracles
            << " item_removed_other=" << total.item_removed_other
            << " item_removed_revivals=" << total.item_removed_revivals
            << "\nhp_absorption_resolutions=" << total.hp_absorption_resolutions
            << " hp_absorption_weapon_resolutions=" << total.hp_absorption_weapon_resolutions
            << " hp_absorption_miracle_resolutions=" << total.hp_absorption_miracle_resolutions
            << " hp_absorption_self_resolutions=" << total.hp_absorption_self_resolutions
            << " hp_absorption_zero_residual=" << total.hp_absorption_zero_residual
            << " hp_absorption_zero_healing=" << total.hp_absorption_zero_healing
            << " hp_absorption_healing=" << total.hp_absorption_healing
            << " hp_absorption_capped=" << total.hp_absorption_capped
            << " hp_absorption_dark_resolutions=" << total.hp_absorption_dark_resolutions
            << " hp_absorption_raw_max=" << total.hp_absorption_raw_max
            << " hp_absorption_miracle_first_uses=" << total.hp_absorption_miracle_first_uses
            << " hp_absorption_miracle_recasts=" << total.hp_absorption_miracle_recasts
            << " hp_absorption_miracle_mp_spent=" << total.hp_absorption_miracle_mp_spent
            << " hp_absorption_learned_miracles=" << total.hp_absorption_learned_miracles
            << " hp_absorption_miracle_attacks=" << total.hp_absorption_miracle_attacks
            << " hp_absorption_miracle_hp_damage=" << total.hp_absorption_miracle_hp_damage
            << " forgetting_hp_absorption=" << total.forgetting_hp_absorption
            << "\nself_damage_attacks=" << total.self_damage_attacks
<< " self_damage_resolutions=" << total.self_damage_resolutions
<< " self_damage_self_resolutions=" << total.self_damage_self_resolutions
<< " self_damage_zero_residual=" << total.self_damage_zero_residual
<< " self_damage_zero_source_loss=" << total.self_damage_zero_source_loss
<< " self_damage_source_lost=" << total.self_damage_source_lost
<< " self_damage_target_lost=" << total.self_damage_target_lost
<< " self_damage_capped_source=" << total.self_damage_capped_source
<< " self_damage_capped_target=" << total.self_damage_capped_target
<< " self_damage_raw_max=" << total.self_damage_raw_max
<< " self_damage_reflections=" << total.self_damage_reflections
<< " self_damage_revival_steps=" << total.self_damage_revival_steps
            << "\npestle_attacks=" << total.pestle_attacks
<< " pestle_mortar_locked_attacks=" << total.pestle_mortar_locked_attacks
<< " pestle_random_target_attacks=" << total.pestle_random_target_attacks
<< " pestle_initial_self_targets=" << total.pestle_initial_self_targets
<< " pestle_initial_opponent_targets=" << total.pestle_initial_opponent_targets
<< " pestle_resolutions=" << total.pestle_resolutions
<< " pestle_self_resolutions=" << total.pestle_self_resolutions
<< " pestle_zero_residual=" << total.pestle_zero_residual
<< " pestle_target_lost=" << total.pestle_target_lost
<< " pestle_capped=" << total.pestle_capped
<< " pestle_raw_max=" << total.pestle_raw_max
<< " pestle_reflections=" << total.pestle_reflections
<< " pestle_blocks=" << total.pestle_blocks
<< " pestle_revival_steps=" << total.pestle_revival_steps
<< " mortar_discards_ignored=" << total.mortar_discards_ignored
<< " item_removed_mortars=" << total.item_removed_mortars
            << "\nsales=" << total.sales
<< " sale_resolutions=" << total.sale_resolutions
<< " sale_delivered=" << total.sale_delivered
<< " sale_delivery_failures=" << total.sale_delivery_failures
<< " sale_self_resolutions=" << total.sale_self_resolutions
<< " sale_opponent_resolutions=" << total.sale_opponent_resolutions
<< " sale_zero_price=" << total.sale_zero_price
<< " sale_price_total=" << total.sale_price_total
<< " sale_money_lost=" << total.sale_money_lost
<< " sale_mp_lost=" << total.sale_mp_lost
<< " sale_hp_lost=" << total.sale_hp_lost
<< " sale_money_gained=" << total.sale_money_gained
<< " sale_reflections=" << total.sale_reflections
<< " sale_revival_steps=" << total.sale_revival_steps
<< " sale_weapons=" << total.sale_weapons
<< " sale_armor=" << total.sale_armor
<< " sale_miracles=" << total.sale_miracles
<< " sale_other=" << total.sale_other
            << "\nbuy_uses=" << total.buy_uses
<< " buy_attempts=" << total.buy_attempts
<< " buy_empty=" << total.buy_empty
<< " buy_self_selections=" << total.buy_self_selections
<< " buy_insufficient=" << total.buy_insufficient
<< " buy_offers=" << total.buy_offers
<< " buy_decisions=" << total.buy_decisions
<< " buy_declines=" << total.buy_declines
<< " buy_purchases=" << total.buy_purchases
<< " buy_delivered=" << total.buy_delivered
<< " buy_delivery_failures=" << total.buy_delivery_failures
<< " buy_money_spent=" << total.buy_money_spent
<< " buy_money_gained=" << total.buy_money_gained
<< " buy_zero_price=" << total.buy_zero_price
<< " buy_reflections=" << total.buy_reflections
<< " buy_active_offers=" << total.buy_active_offers
<< " buy_revival_steps=" << total.buy_revival_steps
<< " bought_weapons=" << total.bought_weapons
<< " bought_armor=" << total.bought_armor
<< " bought_miracles=" << total.bought_miracles
<< " bought_other=" << total.bought_other
<< " sold_buy_cards=" << total.sold_buy_cards
            << "\nhp_gamble_uses=" << total.hp_gamble_uses
<< " hp_gamble_heal_rolls=" << total.hp_gamble_heal_rolls
<< " hp_gamble_damage_rolls=" << total.hp_gamble_damage_rolls
<< " hp_gamble_initial_self_targets=" << total.hp_gamble_initial_self_targets
<< " hp_gamble_initial_opponent_targets=" << total.hp_gamble_initial_opponent_targets
<< " hp_gamble_resolutions=" << total.hp_gamble_resolutions
<< " hp_gamble_healing_resolutions=" << total.hp_gamble_healing_resolutions
<< " hp_gamble_damage_resolutions=" << total.hp_gamble_damage_resolutions
<< " hp_gamble_self_resolutions=" << total.hp_gamble_self_resolutions
<< " hp_gamble_healed=" << total.hp_gamble_healed
<< " hp_gamble_damaged=" << total.hp_gamble_damaged
<< " hp_gamble_zero_change=" << total.hp_gamble_zero_change
<< " hp_gamble_capped_heals=" << total.hp_gamble_capped_heals
<< " hp_gamble_capped_damage=" << total.hp_gamble_capped_damage
<< " hp_gamble_reflections=" << total.hp_gamble_reflections
<< " hp_gamble_revival_steps=" << total.hp_gamble_revival_steps
<< " sold_hp_gambles=" << total.sold_hp_gambles
<< " bought_hp_gambles=" << total.bought_hp_gambles
            << "\nmagic_card_bounces=" << total.magic_card_bounces
<< " magic_card_self_bounces=" << total.magic_card_self_bounces
<< " magic_card_damage_bounces=" << total.magic_card_damage_bounces
<< " magic_card_utility_bounces=" << total.magic_card_utility_bounces
<< " magic_card_held_spell_bounces=" << total.magic_card_held_spell_bounces
<< " magic_card_free_spell_bounces=" << total.magic_card_free_spell_bounces
<< " magic_card_removal_bounces=" << total.magic_card_removal_bounces
<< " magic_bounce_weapon_attacks=" << total.magic_bounce_weapon_attacks
<< " magic_bounce_boost_uses=" << total.magic_bounce_boost_uses
<< " magic_bounce_ordinary_cards_used=" << total.magic_bounce_ordinary_cards_used
<< " sold_magic_bounce_cards=" << total.sold_magic_bounce_cards
<< " bought_magic_bounce_cards=" << total.bought_magic_bounce_cards
            << " magic_card_bounces_id21=" << total.magic_card_bounces_by_id[0]
            << " magic_card_bounces_id119=" << total.magic_card_bounces_by_id[1]
            << " magic_card_bounces_id121=" << total.magic_card_bounces_by_id[2]
            << " magic_card_bounces_id127=" << total.magic_card_bounces_by_id[3]
            << " magic_card_bounces_id134=" << total.magic_card_bounces_by_id[4]
            << " magic_card_bounces_id140=" << total.magic_card_bounces_by_id[5]
            << "\nsundry_boost_attacks=" << total.sundry_boost_attacks
<< " sundry_boost_cards_used=" << total.sundry_boost_cards_used
<< " sundry_boost_plain_attacks=" << total.sundry_boost_plain_attacks
<< " sundry_boost_ordered_attacks=" << total.sundry_boost_ordered_attacks
<< " sundry_boost_mana_attacks=" << total.sundry_boost_mana_attacks
<< " sundry_boost_mana_resets=" << total.sundry_boost_mana_resets
<< " sundry_boost_resets_after_boost=" << total.sundry_boost_resets_after_boost
<< " sundry_boost_double_attacks=" << total.sundry_boost_double_attacks
<< " sundry_boost_wide_attacks=" << total.sundry_boost_wide_attacks
<< " sundry_boost_reaction_steps=" << total.sundry_boost_reaction_steps
<< " sundry_boost_counters_queued=" << total.sundry_boost_counters_queued
<< " sundry_boost_discarded_cards=" << total.sundry_boost_discarded_cards
<< " sundry_boost_sold_cards=" << total.sundry_boost_sold_cards
<< " sundry_boost_bought_cards=" << total.sundry_boost_bought_cards
<< " sundry_boost_removed_cards=" << total.sundry_boost_removed_cards
<< " sundry_boost_raw_max=" << total.sundry_boost_raw_max
<< " sundry_boost_raw_checked=" << total.sundry_boost_raw_checked
<< " sundry_boost_self_attacks=" << total.sundry_boost_self_attacks
            << " sundry_boost_element_neutral=" << total.sundry_boost_elements[0]
            << " sundry_boost_element_fire=" << total.sundry_boost_elements[1]
            << " sundry_boost_element_water=" << total.sundry_boost_elements[2]
            << " sundry_boost_element_wood=" << total.sundry_boost_elements[3]
            << " sundry_boost_element_stone=" << total.sundry_boost_elements[4]
            << " sundry_boost_element_light=" << total.sundry_boost_elements[5]
            << " sundry_boost_element_dark=" << total.sundry_boost_elements[6]
            << "\nstatus_first_uses=" << total.status_first_uses
<< " status_recasts=" << total.status_recasts
<< " status_mp_spent=" << total.status_mp_spent
<< " status_learned=" << total.status_learned
<< " status_casts=" << total.status_casts
<< " status_resolutions=" << total.status_resolutions
<< " status_self_resolutions=" << total.status_self_resolutions
<< " status_lower_cures=" << total.status_lower_cures
<< " status_all_cures=" << total.status_all_cures
<< " status_dark_cloud_applications=" << total.status_dark_cloud_applications
<< " status_cold_applications=" << total.status_cold_applications
<< " status_heaven_applications=" << total.status_heaven_applications
<< " status_heaven_falls=" << total.status_heaven_falls
<< " status_disease_changes=" << total.status_disease_changes
<< " status_diseases_removed=" << total.status_diseases_removed
<< " status_glory_cleared=" << total.status_glory_cleared
<< " status_dark_cloud_cleared=" << total.status_dark_cloud_cleared
<< " status_dark_cloud_new=" << total.status_dark_cloud_new
<< " status_hp_lost=" << total.status_hp_lost
<< " status_free_uses=" << total.status_free_uses
<< " status_reflections=" << total.status_reflections
<< " status_bounces=" << total.status_bounces
<< " status_blocks=" << total.status_blocks
<< " status_held_resolutions=" << total.status_held_resolutions
<< " forgetting_status_miracles=" << total.forgetting_status_miracles
<< " status_retained_inventory=" << total.status_retained_inventory
<< " sold_status_miracles=" << total.sold_status_miracles
<< " bought_status_miracles=" << total.bought_status_miracles
<< " removed_status_miracles=" << total.removed_status_miracles
            << "\nchecksum_version=" << (config.status_miracles ? 36 : config.sundry_boost ? 35 : config.magic_bounce ? 34 : config.hp_gamble ? 33 : config.buying ? 32 : config.selling ? 31 : config.pestle_mortar ? 30 : config.self_damage ? 29 : config.hp_absorption ? 28 : config.item_removal ? 27 : config.revival ? 26 : config.exchanges ? 25 : config.forgetting ? 24 : config.magic_free ? 23 : config.wide_miracles ? 22 : config.doubling_miracles ? 21 : config.additive_miracles ? 20 : config.magic_block ? 19 : config.mana_weapon ? 18 : config.any_reflection ? 17 : config.magic_reflection ? 16 : config.money_counter ? 15 : config.counter_extensions ? 14 : config.counters ? 13 : config.attribute_removal ? 12 : config.attribute_change ? 11 : config.defensive_miracles ? 10 : config.offensive_miracles ? 9 : config.celestial ? 8 : config.harms ? 7 : config.disease ? 6 : config.reflection ? 5 : config.chance ? 4 : config.elements ? 3 : 2)
            << "\nchecksum=" << total.checksum << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what()
            << "\nUsage: godfield_legacy_bench [games=100000] [threads=1] [seed=42] [profile=neutral|miracles|elements|chance|reflection|disease|harms|celestial|offensive-miracles|defensive-miracles|attribute-change|attribute-removal|counter|counter-extensions|money-counter|magic-reflection|any-reflection|mana-weapon|magic-block|additive-miracles|doubling-miracles|wide-miracles|magic-free|forgetting|exchange|revival|item-removal|hp-absorption|self-damage|pestle-mortar|selling|buying|hp-gamble|magic-bounce|sundry-boost|status-miracles]\n";
        return 1;
    }
}
