#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct MagicReflectionCard {
    Card card;
};

inline constexpr int magic_reflection_card_count = 4;
inline constexpr const char* magic_reflection_version = "legacy-magic-reflection-v1";
inline constexpr const char* magic_reflection_sha256 = "530f23f74e45db12307dbd606dea9dd01aeabf732a3c410f9a81cd8b401d1d4f";
[[nodiscard]] std::span<const MagicReflectionCard> magic_reflection_catalog() noexcept;
[[nodiscard]] const MagicReflectionCard* find_magic_reflection_card(CardId id) noexcept;
}
