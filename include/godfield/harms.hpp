#pragma once

#include "godfield/elements.hpp"
#include <cstdint>
#include <span>

namespace godfield::legacy {
enum class Harm : std::uint8_t { none, glory, dark_cloud };
struct HarmCard {
    Card card;
    Element element;
    Harm harm;
    bool on_defense;
};

inline constexpr int harm_card_count = 1;
inline constexpr const char* harms_version = "legacy-harms-v1";
inline constexpr const char* harms_sha256 = "940f9ec88d8b13a1aeafaf5f81c65c43a52cb2f82f23a2287aa8e6355cea005c";
[[nodiscard]] std::span<const HarmCard> harm_catalog() noexcept;
[[nodiscard]] const HarmCard* find_harm_card(CardId id) noexcept;
}
