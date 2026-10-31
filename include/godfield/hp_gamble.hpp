#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct HPGambleCard { Card card; int amount; };
inline constexpr int hp_gamble_card_count = 1;
inline constexpr const char* hp_gamble_version = "legacy-hp-gamble-v1";
inline constexpr const char* hp_gamble_sha256 = "002dcc3327a34a87652603afd1c1871f6d6840802252eded31090883a5062468";
[[nodiscard]] std::span<const HPGambleCard> hp_gamble_catalog() noexcept;
[[nodiscard]] const HPGambleCard* find_hp_gamble_card(CardId id) noexcept;
[[nodiscard]] int hp_gamble_index(CardId id) noexcept;
}
