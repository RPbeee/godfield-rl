#pragma once

#include "godfield/elements.hpp"
#include <span>

namespace godfield::legacy {
struct ChanceCard {
    Card card;
    Element element;
    int hit_rate;
};

inline constexpr int chance_card_count = 12;
inline constexpr const char* chance_version = "legacy-chance-v1";
inline constexpr const char* chance_sha256 = "d80827e64916479a06ae33e34bad2d4407fd8a06c525deb7148b4f8f417e84f8";
[[nodiscard]] std::span<const ChanceCard> chance_catalog() noexcept;
// Only the simple positive-hit-rate allowlist is searched. No fallback card.
[[nodiscard]] const ChanceCard* find_chance_card(CardId id) noexcept;
}
