#pragma once

#include "godfield/elements.hpp"
#include <span>

namespace godfield::legacy {
struct MagicFreeCard {
    Card card;
    Element element;
};

inline constexpr int magic_free_card_count = 5;
inline constexpr const char* magic_free_version = "legacy-magic-free-v1";
inline constexpr const char* magic_free_sha256 = "44ac7bfe88ff0c6b77163aae9024fd50ea8fbe0f4713136f8b6c019e8fe52572";
[[nodiscard]] std::span<const MagicFreeCard> magic_free_catalog() noexcept;
[[nodiscard]] const MagicFreeCard* find_magic_free_card(CardId id) noexcept;
[[nodiscard]] int magic_free_index(CardId id) noexcept;
}
