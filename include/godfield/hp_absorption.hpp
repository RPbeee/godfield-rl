#pragma once

#include "godfield/elements.hpp"
#include <array>
#include <span>

namespace godfield::legacy {
struct HPAbsorptionCard {
    Card card;
    Element element;
    int hit_rate;
};
struct HPAbsorptionMiracle {
    CardId id;
    const char* name;
    Element element;
    int amount;
    int cost;
    int weight;
};

inline constexpr int hp_absorption_card_count = 3;
inline constexpr int hp_absorption_miracle_count = 1;
inline constexpr const char* hp_absorption_version = "legacy-hp-absorption-v1";
inline constexpr const char* hp_absorption_sha256 = "f48782ab0d9654445f02540e043d22d9b8d07cdf4d13ff438ea94dc915490de1";
inline constexpr std::array<CardId, 22> hp_absorption_retained_type_ids{
    197, 198, 199, 200, 203, 204, 205, 206, 207, 209, 210,
    214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 226};
[[nodiscard]] std::span<const HPAbsorptionCard> hp_absorption_catalog() noexcept;
[[nodiscard]] const HPAbsorptionCard* find_hp_absorption_card(CardId id) noexcept;
[[nodiscard]] int hp_absorption_index(CardId id) noexcept;
[[nodiscard]] std::span<const HPAbsorptionMiracle> hp_absorption_miracles() noexcept;
[[nodiscard]] const HPAbsorptionMiracle* find_hp_absorption_miracle(CardId id) noexcept;
[[nodiscard]] int hp_absorption_miracle_index(CardId id) noexcept;
}
