#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct AnyReflectionCard {
    Card card;
};

inline constexpr int any_reflection_card_count = 1;
inline constexpr const char* any_reflection_version = "legacy-any-reflection-v1";
inline constexpr const char* any_reflection_sha256 = "682523fa55a0b4a554d190d92057e08d2b14b60c4cb5d152e622a3aade6d14bd";
[[nodiscard]] std::span<const AnyReflectionCard> any_reflection_catalog() noexcept;
[[nodiscard]] const AnyReflectionCard* find_any_reflection_card(CardId id) noexcept;
}
