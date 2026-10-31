#pragma once

#include "godfield/harms.hpp"
#include <span>

namespace godfield::legacy {
struct CelestialCard {
    Card card;
    Element element;
    Harm harm;
    int hit_rate;
};

inline constexpr int celestial_card_count = 18;
inline constexpr const char* celestial_version = "legacy-celestial-v1";
inline constexpr const char* celestial_sha256 = "7c123d39fa061577d180e94bd6f5372e8eaf6a4f502fc6fbb1f9ce8fffe153b8";
[[nodiscard]] std::span<const CelestialCard> celestial_catalog() noexcept;
[[nodiscard]] const CelestialCard* find_celestial_card(CardId id) noexcept;
}
