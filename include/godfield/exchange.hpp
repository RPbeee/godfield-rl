#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct ExchangeCard {
    Card card;
};

inline constexpr int exchange_card_count = 1;
inline constexpr const char* exchange_version = "legacy-exchange-v1";
inline constexpr const char* exchange_sha256 = "d4f95f4f3853e789db2a66460e49857437b5884723a28df35774cf575287cdc5";
[[nodiscard]] std::span<const ExchangeCard> exchange_catalog() noexcept;
[[nodiscard]] const ExchangeCard* find_exchange_card(CardId id) noexcept;
[[nodiscard]] int exchange_index(CardId id) noexcept;
}
