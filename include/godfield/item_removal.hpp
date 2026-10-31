#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct ItemRemovalCard {
    Card card;
    int amount;
};

inline constexpr int item_removal_card_count = 1;
inline constexpr const char* item_removal_version = "legacy-item-removal-v1";
inline constexpr const char* item_removal_sha256 = "e5ec7deb813c3c6a9414b9bc05d48dfc31062b88283658a0d28f1a13623adaa2";
[[nodiscard]] std::span<const ItemRemovalCard> item_removal_catalog() noexcept;
[[nodiscard]] const ItemRemovalCard* find_item_removal_card(CardId id) noexcept;
[[nodiscard]] int item_removal_index(CardId id) noexcept;
}
