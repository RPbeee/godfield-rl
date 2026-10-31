#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct MagicBounceCard { Card card; };
inline constexpr int magic_bounce_card_count = 6;
inline constexpr const char* magic_bounce_version = "legacy-magic-bounce-v1";
inline constexpr const char* magic_bounce_sha256 = "1f1472af2ee9fd6de4613f9af226211b909e6b0f8eb938f92b772e0e9c2a8ddf";
[[nodiscard]] std::span<const MagicBounceCard> magic_bounce_catalog() noexcept;
[[nodiscard]] const MagicBounceCard* find_magic_bounce_card(CardId id) noexcept;
[[nodiscard]] int magic_bounce_index(CardId id) noexcept;
}
