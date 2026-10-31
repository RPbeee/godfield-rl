#pragma once

#include "godfield/elements.hpp"
#include <cstdint>
#include <span>

namespace godfield::legacy {
enum class Disease : std::uint8_t { none, cold, fever, hell, heaven };
enum class Cure : std::uint8_t { none, lower, all };
struct DiseaseCard {
    Card card;
    Element element;
    Disease disease;
    Cure cure;
    bool on_defense;
};

inline constexpr int disease_card_count = 8;
inline constexpr const char* disease_version = "legacy-disease-v1";
inline constexpr const char* disease_sha256 = "581393b8127cb9065d6ccca65f7f31fe6a473cbf5ece02e6dcdb0c9a99263615";
[[nodiscard]] std::span<const DiseaseCard> disease_catalog() noexcept;
[[nodiscard]] const DiseaseCard* find_disease_card(CardId id) noexcept;
}
