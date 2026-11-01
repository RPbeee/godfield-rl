#include "godfield/official_c_api.h"
#include "godfield/official_rules.hpp"
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace official = godfield::official;
static_assert(std::is_standard_layout_v<gfo_card> && sizeof(gfo_card) == 44);
static_assert(offsetof(gfo_card, id) == 0 && offsetof(gfo_card, category) == 2);
static_assert(offsetof(gfo_card, element) == 3 && offsetof(gfo_card, ability) == 4);
static_assert(offsetof(gfo_card, curse) == 5 && offsetof(gfo_card, guardian) == 6);
static_assert(offsetof(gfo_card, is_plus_atk) == 7 && offsetof(gfo_card, atk) == 8);
static_assert(offsetof(gfo_card, guardian_attack_rate) == 40);
static_assert(std::is_standard_layout_v<gfo_composition> && sizeof(gfo_composition) == 24);
static_assert(offsetof(gfo_composition, attack_power) == 0 && offsetof(gfo_composition, magic_cost) == 8);
static_assert(offsetof(gfo_composition, attack_element) == 16 && offsetof(gfo_composition, defense_element) == 17);
static_assert(offsetof(gfo_composition, reserved) == 18);

namespace {
std::int8_t element(official::Attribute value) noexcept {
    return value ? static_cast<std::int8_t>(*value) : -1;
}
template<class Enum, class Name>
const char* enum_name(std::uint8_t value, Name name) noexcept {
    try {
        auto spelling = name(static_cast<Enum>(value));
        return spelling.empty() ? nullptr : spelling.data();
    } catch (...) { return nullptr; }
}
}

extern "C" {
int gfo_abi_version(void) { return 1; }
const char* gfo_ruleset(void) { return "official-catalog-client-rules-v2"; }
const char* gfo_scope(void) { return "partial:catalog+client-rules;not-playable"; }
const char* gfo_catalog_sha256(void) { return official::catalog_sha256; }
const char* gfo_client_sha256(void) { return official::client_sha256; }
int gfo_catalog_count(void) { return official::catalog_count; }
int gfo_gift_rate(void) {
    int sum = 0;
    for (const auto& card : official::catalog()) sum += card.gift_rate;
    return sum;
}
size_t gfo_card_size(void) { return sizeof(gfo_card); }
size_t gfo_composition_size(void) { return sizeof(gfo_composition); }
const char* gfo_card_name(uint16_t id) {
    const auto* card = official::find_card(id);
    return card ? card->name : nullptr;
}
const char* gfo_card_image_name(uint16_t id) {
    const auto* card = official::find_card(id);
    return card ? card->image_name : nullptr;
}
const char* gfo_category_name(uint8_t value) {
    return enum_name<official::Category>(value, official::category_name);
}
const char* gfo_element_name(int8_t value) {
    if (value == -1) return "";
    if (value < 0) return nullptr;
    return enum_name<official::Element>(static_cast<uint8_t>(value), official::element_name);
}
const char* gfo_ability_name(uint8_t value) {
    // The source spelling for Ability::none is intentionally empty.
    if (value == 0) return "";
    return enum_name<official::Ability>(value, official::ability_name);
}
const char* gfo_curse_name(uint8_t value) {
    if (value == 0) return "";
    return enum_name<official::Curse>(value, official::curse_name);
}
const char* gfo_guardian_name(uint8_t value) {
    if (value == 0) return "";
    return enum_name<official::Guardian>(value, official::guardian_name);
}
int gfo_get_card(uint16_t id, gfo_card* out) {
    if (!out) return -1;
    try {
        const auto* card = official::find_card(id);
        if (!card) return -2;
        const gfo_card result{card->id, static_cast<uint8_t>(card->category), element(card->element),
            static_cast<uint8_t>(card->ability), static_cast<uint8_t>(card->curse),
            static_cast<uint8_t>(card->guardian), static_cast<uint8_t>(card->is_plus_atk),
            card->atk, card->hit_rate, card->defense, card->ability_value, card->price, card->cost,
            card->gift_rate, card->appearance_rate, card->guardian_attack_rate};
        *out = result;
        return 0;
    } catch (...) { return -3; }
}
int gfo_compose(const uint16_t* ids, size_t count, int64_t base_override, gfo_composition* out) {
    if (!out || (!ids && count)) return -1;
    if (!count || base_override < 0) return -2;
    try {
        std::vector<official::CardId> ordered;
        if (count > ordered.max_size()) return -2;
        ordered.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            if (!official::find_card(ids[i])) return -2;
            ordered.push_back(ids[i]);
        }
        gfo_composition result{};
        result.attack_power = official::attack_power(ordered, base_override);
        result.magic_cost = official::magic_cost(ordered);
        result.attack_element = element(official::attack_element(ordered));
        result.defense_element = element(official::defense_element(ordered));
        *out = result;
        return 0;
    } catch (const std::invalid_argument&) { return -2; }
      catch (...) { return -3; }
}
}
