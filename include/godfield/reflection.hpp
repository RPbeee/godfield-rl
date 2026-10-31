#pragma once

#include "godfield/catalog.hpp"
#include <cstdint>
#include <span>

namespace godfield::legacy {
enum class WeaponReaction : std::uint8_t { none, reflect, bounce, block };
struct ReactionCard {
    Card card;
    WeaponReaction reaction;
};

inline constexpr int reaction_card_count = 2;
inline constexpr const char* reflection_version = "legacy-reflection-v1";
inline constexpr const char* reflection_sha256 = "652187da9080a3b835162e002d8d9d2c627f2ad1e2f595c9a4b672563c886d4d";
[[nodiscard]] std::span<const ReactionCard> reaction_catalog() noexcept;
// Search only the simple neutral non-miracle weapon-reaction allowlist.
[[nodiscard]] const ReactionCard* find_reaction_card(CardId id) noexcept;
}
