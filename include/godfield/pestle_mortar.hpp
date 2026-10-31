#pragma once

#include "godfield/elements.hpp"
#include <span>

namespace godfield::legacy {
struct PestleCard { Card card; Element element; int mortar_amount; };
struct MortarCard { Card card; };
inline constexpr int pestle_card_count = 1;
inline constexpr int mortar_card_count = 1;
inline constexpr const char* pestle_mortar_version = "legacy-pestle-mortar-v1";
inline constexpr const char* pestle_mortar_sha256 = "bed61767a6682dc14d3c2a631e47644e5fa6ff66a1bb16c26bd960504c221e8d";
[[nodiscard]] std::span<const PestleCard> pestle_catalog() noexcept;
[[nodiscard]] const PestleCard* find_pestle_card(CardId id) noexcept;
[[nodiscard]] int pestle_index(CardId id) noexcept;
[[nodiscard]] std::span<const MortarCard> mortar_catalog() noexcept;
[[nodiscard]] const MortarCard* find_mortar_card(CardId id) noexcept;
[[nodiscard]] int mortar_index(CardId id) noexcept;
}
