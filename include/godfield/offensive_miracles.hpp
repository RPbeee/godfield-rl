#pragma once

#include "godfield/harms.hpp"
#include <span>

namespace godfield::legacy {
struct OffensiveMiracle {
    CardId id;
    const char* name;
    Element element;
    int amount;
    int cost;
    int weight;
    int hit_rate;
    Harm harm;
};

inline constexpr int offensive_miracle_count = 13;
inline constexpr const char* offensive_miracles_version = "legacy-offensive-miracles-v1";
inline constexpr const char* offensive_miracles_sha256 = "965afd092eca1d2dc9f07c4d5ce1ba46347b23dd0bac09f1acb500ce7535e658";
[[nodiscard]] std::span<const OffensiveMiracle> offensive_miracles() noexcept;
// Returns local kind index 0..12. Shared cast slots are this index plus 2.
// The earlier two utility miracles are not members of this new catalog.
[[nodiscard]] int offensive_miracle_index(CardId id) noexcept;
[[nodiscard]] const OffensiveMiracle* find_offensive_miracle(CardId id) noexcept;
}
