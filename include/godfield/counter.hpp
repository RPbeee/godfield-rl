#pragma once

#include "godfield/elements.hpp"
#include <span>

namespace godfield::legacy {
struct CounterCard {
    Card card;
    Element element;
    int multiplier;
};

inline constexpr int counter_card_count = 1;
inline constexpr const char* counter_version = "legacy-counter-v1";
inline constexpr const char* counter_sha256 = "a8215ab41d377211742564c8fb230ea2414a9741c0dbd81cdfbd2d6867206be0";
[[nodiscard]] std::span<const CounterCard> counter_catalog() noexcept;
[[nodiscard]] const CounterCard* find_counter_card(CardId id) noexcept;
}
