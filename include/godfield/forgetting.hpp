#pragma once

#include "godfield/catalog.hpp"
#include <array>
#include <span>

namespace godfield::legacy {
struct ForgetCard {
    Card card;
    int amount;
};

inline constexpr int forgetting_card_count = 1;
inline constexpr const char* forgetting_version = "legacy-forgetting-v1";
inline constexpr const char* forgetting_sha256 = "776d7bef4e738576a33418f3e2890b2a539fbe7582179a973508cfdb84370bcb";
// Canonical weighted-rank order; existing family-local ability indices are unchanged.
inline constexpr std::array<CardId, 21> retained_type_ids{
    197, 198, 199, 200, 203, 204, 205, 206, 207, 209, 210,
    214, 215, 216, 217, 218, 219, 220, 222, 223, 226};
[[nodiscard]] std::span<const ForgetCard> forgetting_catalog() noexcept;
[[nodiscard]] const ForgetCard* find_forgetting_card(CardId id) noexcept;
[[nodiscard]] int forgetting_index(CardId id) noexcept;
}
