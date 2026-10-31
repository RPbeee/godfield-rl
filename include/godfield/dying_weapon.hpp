#pragma once
#include "godfield/elements.hpp"
#include <span>
namespace godfield::legacy {
struct DyingWeaponCard { Card card; Element element; int hit_rate; int dying_amount; };
inline constexpr int dying_weapon_card_count = 1;
inline constexpr const char* dying_weapon_version = "legacy-dying-weapon-v1";
inline constexpr const char* dying_weapon_sha256 = "7e48741eee82ae73aea8577427f5042b44e5b6d96139197db6d954078de139c9";
[[nodiscard]] std::span<const DyingWeaponCard> dying_weapon_catalog() noexcept;
[[nodiscard]] const DyingWeaponCard* find_dying_weapon_card(CardId id) noexcept;
[[nodiscard]] int dying_weapon_index(CardId id) noexcept;
}
