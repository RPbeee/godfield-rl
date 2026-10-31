#pragma once

#include "godfield/catalog.hpp"
#include <span>

namespace godfield::legacy {
struct ReviveCard {
    Card card;
    int amount;
};

inline constexpr int revival_card_count = 1;
inline constexpr const char* revival_version = "legacy-revival-v1";
inline constexpr const char* revival_sha256 = "518a71ca143c5f8b94f3d26b8beb2b26110f740e8866f58eb21eb9003681c561";
[[nodiscard]] std::span<const ReviveCard> revival_catalog() noexcept;
[[nodiscard]] const ReviveCard* find_revival_card(CardId id) noexcept;
[[nodiscard]] int revival_index(CardId id) noexcept;
}
