#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct SelfDamageCard { Card card; };
inline constexpr int self_damage_card_count = 1;
inline constexpr const char* self_damage_version = "legacy-self-damage-v1";
inline constexpr const char* self_damage_sha256 = "4bb6e47733986c7985c5a01f8f832a6ae4f0861fdf2f8afded478631384728ec";
[[nodiscard]] std::span<const SelfDamageCard> self_damage_catalog() noexcept;
[[nodiscard]] const SelfDamageCard* find_self_damage_card(CardId id) noexcept;
[[nodiscard]] int self_damage_index(CardId id) noexcept;
}
