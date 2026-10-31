#pragma once

#include <cstdint>
#include <span>

namespace godfield::legacy {
using CardId = std::uint16_t;
enum class Kind : std::uint8_t { weapon, armor, heal, mana, sundry, trade };

// IDs are zero-based rows in the historical emulator's ItemData.CSV.
// name is a technical label; no official-name mapping is implied.
// A weapon can be both a base (attack>0) and an add-on (boost>0).
// Armor can also be an add-on. Roles are independent of Kind.
struct Card {
    CardId id;
    const char* name;
    Kind kind;
    int attack;
    int boost;
    int defense;
    int hp;
    int mp;
    int price;
    int weight;
    bool discardable;
};

inline constexpr const char* catalog_version = "legacy-neutral-v1";
inline constexpr const char* catalog_sha256 = "9d78cfdc44b89c53468b31e1f8e1b00f49ab68fbb19aeaf13d4d699535945ca3";
[[nodiscard]] std::span<const Card> catalog() noexcept;
// Unknown, excluded and sentinel IDs return nullptr. No fallback card.
[[nodiscard]] const Card* find_card(CardId id) noexcept;
}
