#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct SellCard { Card card; };
struct SalePrice { CardId id; int price; };
inline constexpr int selling_card_count = 1;
inline constexpr int sale_price_count = 216;
inline constexpr int max_sale_price = 30;
inline constexpr const char* selling_version = "legacy-selling-v1";
inline constexpr const char* selling_sha256 = "de8e57c5963c1c8c6c39ea60b6ac8100e35dbd806f02be674a40feaac83b6219";
inline constexpr const char* sale_prices_sha256 = "0ca966940cfe75ce75ca0d172cc041b103bc991383e7bd790f671ecb750724e6";
[[nodiscard]] std::span<const SellCard> selling_catalog() noexcept;
[[nodiscard]] const SellCard* find_selling_card(CardId id) noexcept;
[[nodiscard]] int selling_index(CardId id) noexcept;
[[nodiscard]] std::span<const SalePrice> sale_prices() noexcept;
// Unknown or unsupported IDs are distinct from legitimate zero-price MORTAR245.
[[nodiscard]] int sale_price(CardId id) noexcept;
}
