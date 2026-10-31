#pragma once

#include "godfield/catalog.hpp"
#include <cstdint>
#include <span>

namespace godfield::legacy {
enum class Element : std::uint8_t { neutral, fire, water, wood, stone, light, dark, none };
struct ElementCard {
    Card card;
    Element element;
};

inline constexpr int element_card_count = 54;
inline constexpr const char* elements_version = "legacy-elements-v1";
inline constexpr const char* elements_sha256 = "72fe1864f518a69e8b27e18018d7e8fe034863273c5515f52aebfd06f4cf0211";
[[nodiscard]] std::span<const ElementCard> element_catalog() noexcept;
// Only this separate four-element allowlist is searched. No neutral fallback.
[[nodiscard]] const ElementCard* find_element_card(CardId id) noexcept;
}
