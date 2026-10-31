#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct MagicBlockCard {
    Card card;
};

inline constexpr int magic_block_card_count = 8;
inline constexpr const char* magic_block_version = "legacy-magic-block-v1";
inline constexpr const char* magic_block_sha256 = "32781bbb5acc2860ed47df15fc20e54b8efd3f8c7da5b8f7a9d84136c4808551";
[[nodiscard]] std::span<const MagicBlockCard> magic_block_catalog() noexcept;
[[nodiscard]] const MagicBlockCard* find_magic_block_card(CardId id) noexcept;
}
