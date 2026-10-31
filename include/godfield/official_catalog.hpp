#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace godfield::official {
using CardId = std::uint16_t;
enum class Category : std::uint8_t { trade, weapons, armor, sundries, miracles, devils, guardians, phenomena };
// Omitted source element is std::nullopt, not an inferred neutral/untyped effect.
enum class Element : std::uint8_t { fire, water, wood, stone, light, darkness };
enum class Curse : std::uint8_t { none, cold, fever, hell, heaven, fog, dream, flash, darkcloud };
enum class Guardian : std::uint8_t { none, mars, mercury, jupiter, saturn, uranus, pluto, neptune, venus, earth, moon };
enum class Ability : std::uint8_t {
    none,
    absorb_hp,
    add_curse,
    add_curse_on_damage,
    add_item,
    atk_by2x_mp,
    attack_dyingly,
    attack_every_enemy,
    attack_somebody,
    attack_twice,
    attract_danger,
    block_miracle,
    block_weapon,
    boost_cp,
    boost_cp_of_everybody,
    boost_cp_to_enemy,
    boost_hp,
    boost_hp_or_deal_damage,
    boost_mp,
    boost_mp_and_add_curse,
    boost_something,
    bounce_miracle,
    bounce_weapon,
    buy,
    call_phenomenon,
    category_weapons,
    collect_cp_of_everybody,
    confuse_everybody,
    counter2x_atk,
    counter_atk,
    counter_boost2x_mp,
    counter_curse,
    counter_take_cp,
    cut_cost,
    danger,
    deal_damage,
    deal_same_damage,
    discard,
    double_atk,
    exchange,
    filter_atk_element,
    reflect_anything,
    reflect_miracle,
    reflect_weapon,
    remove_all_curses,
    remove_items,
    remove_mild_curses,
    remove_something,
    remove_used_miracles,
    revive,
    sacrifice,
    self_curse,
    self_curse_and_redraw,
    sell,
    set_curse_of_everybody,
    set_element,
    set_guardian,
    set_guardian_of_everybody,
    set_hp_of_everybody,
    shuffle_items_of_everybody,
    take_cp,
};
struct Card {
    CardId id;
    const char* name;
    const char* image_name;
    Category category;
    std::optional<Element> element;
    bool is_plus_atk;
    int atk, hit_rate, defense;
    Ability ability;
    int ability_value;
    Curse curse;
    Guardian guardian;
    int price, cost, gift_rate, appearance_rate, guardian_attack_rate;
};
inline constexpr int catalog_count = 296;
inline constexpr const char* catalog_version = "official-v1";
inline constexpr const char* catalog_sha256 = "9ce45da9dd372da1ab30397068885a20bbd5927b0891946b5fc7bc07226fa1a8";
[[nodiscard]] std::span<const Card> catalog() noexcept;
[[nodiscard]] const Card* find_card(CardId id) noexcept;
[[nodiscard]] int card_index(CardId id) noexcept;
[[nodiscard]] std::string_view category_name(Category value) noexcept;
[[nodiscard]] std::string_view element_name(Element value) noexcept;
[[nodiscard]] std::string_view ability_name(Ability value) noexcept;
[[nodiscard]] std::string_view curse_name(Curse value) noexcept;
[[nodiscard]] std::string_view guardian_name(Guardian value) noexcept;
}
