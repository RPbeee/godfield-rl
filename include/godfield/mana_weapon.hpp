#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct ManaWeaponCard {
    Card card;
    int multiplier;
};

inline constexpr int mana_weapon_card_count = 1;
inline constexpr const char* mana_weapon_version = "legacy-mana-weapon-v1";
inline constexpr const char* mana_weapon_sha256 = "e42dc3ffd6538b64d60771ba039cfb7d0dd3955e3d60cb9e711758d0e319c83d";
[[nodiscard]] std::span<const ManaWeaponCard> mana_weapon_catalog() noexcept;
[[nodiscard]] const ManaWeaponCard* find_mana_weapon_card(CardId id) noexcept;
}
