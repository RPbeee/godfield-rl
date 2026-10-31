#pragma once

#include "godfield/elements.hpp"
#include <span>

namespace godfield::legacy {
struct AttributeChangeCard {
    Card card;
    Element element;
};

inline constexpr int attribute_change_card_count = 2;
inline constexpr const char* attribute_change_version = "legacy-attribute-change-v1";
inline constexpr const char* attribute_change_sha256 = "3c9546aec0c0d022b4e3bf4e5f3c5c4bb1b708bf15b97ffd44bda035e7662b7c";
[[nodiscard]] std::span<const AttributeChangeCard> attribute_change_catalog() noexcept;
[[nodiscard]] const AttributeChangeCard* find_attribute_change_card(CardId id) noexcept;
}
