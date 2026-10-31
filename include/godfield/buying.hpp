#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct BuyCard { Card card; };
inline constexpr int buying_card_count = 1;
inline constexpr const char* buying_version = "legacy-buying-v1";
inline constexpr const char* buying_sha256 = "54a1d75946acdb90162b71aed9640a9c234a737b2ebf8ece4505b1b9812f1b36";
[[nodiscard]] std::span<const BuyCard> buying_catalog() noexcept;
[[nodiscard]] const BuyCard* find_buying_card(CardId id) noexcept;
[[nodiscard]] int buying_index(CardId id) noexcept;
}
