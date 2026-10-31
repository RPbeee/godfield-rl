#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct AttributeRemovalCard {
    Card card;
};

inline constexpr int attribute_removal_card_count = 1;
inline constexpr const char* attribute_removal_version = "legacy-attribute-removal-v1";
inline constexpr const char* attribute_removal_sha256 = "26d17ef9b787115d0a18de93c06c3c3c2fb994dff8b970d0602f3f3bb6cb3ecc";
[[nodiscard]] std::span<const AttributeRemovalCard> attribute_removal_catalog() noexcept;
[[nodiscard]] const AttributeRemovalCard* find_attribute_removal_card(CardId id) noexcept;
}
