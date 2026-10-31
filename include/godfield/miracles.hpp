#pragma once

#include "godfield/catalog.hpp"
#include <cstdint>
#include <span>

namespace godfield::legacy {
enum class MiracleEffect : std::uint8_t { heal, money };
struct Miracle {
    CardId id;
    const char* name;
    MiracleEffect effect;
    int amount;
    int cost;
    int weight;
};

inline constexpr int miracle_count = 2;
inline constexpr const char* miracles_version = "legacy-miracles-v1";
inline constexpr const char* miracles_sha256 = "07da28e8a3c809fe634f90f94a24eb9cfb53dfbdf8d2cb7658a145a2e250eb1c";
[[nodiscard]] std::span<const Miracle> miracles() noexcept;
// Stable kind slots: ID216 -> 0, ID220 -> 1. Unsupported IDs -> -1.
[[nodiscard]] int miracle_index(CardId id) noexcept;
[[nodiscard]] const Miracle* find_miracle(CardId id) noexcept;
}
