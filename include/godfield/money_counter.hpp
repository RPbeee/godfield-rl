#pragma once

#include "godfield/elements.hpp"
#include <span>

namespace godfield::legacy {
struct MoneyCounterCard {
    Card card;
    Element element;
    int multiplier;
};

inline constexpr int money_counter_card_count = 1;
inline constexpr const char* money_counter_version = "legacy-money-counter-v1";
inline constexpr const char* money_counter_sha256 = "d1444924adad2045923a6f2974832bedb8efb2d2a41d7c7b31b9c5196f53a0f1";
[[nodiscard]] std::span<const MoneyCounterCard> money_counter_catalog() noexcept;
[[nodiscard]] const MoneyCounterCard* find_money_counter_card(CardId id) noexcept;
}
