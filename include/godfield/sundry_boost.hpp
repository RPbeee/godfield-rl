#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct SundryBoostCard { Card card; };
inline constexpr int sundry_boost_card_count = 1;
inline constexpr const char* sundry_boost_version = "legacy-sundry-boost-v1";
inline constexpr const char* sundry_boost_sha256 = "f5dbeda651e84d56b375c63f855ea0348509e132e0df8c4a6010ac1687043158";
[[nodiscard]] std::span<const SundryBoostCard> sundry_boost_catalog() noexcept;
[[nodiscard]] const SundryBoostCard* find_sundry_boost_card(CardId id) noexcept;
[[nodiscard]] int sundry_boost_index(CardId id) noexcept;
}
