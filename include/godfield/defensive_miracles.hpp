#pragma once

#include "godfield/catalog.hpp"
#include <cstdint>
#include <span>

namespace godfield::legacy {
enum class DefensiveMiracleKind : std::uint8_t { bounce_magic, block_weapon };
struct DefensiveMiracle {
    CardId id;
    const char* name;
    DefensiveMiracleKind kind;
    int cost;
    int weight;
};

inline constexpr int defensive_miracle_count = 2;
inline constexpr const char* defensive_miracles_version = "legacy-defensive-miracles-v1";
inline constexpr const char* defensive_miracles_sha256 = "c6efd5ccd68bb428b71a9626271738cead83bab853962869912492fe741a8497";
[[nodiscard]] std::span<const DefensiveMiracle> defensive_miracles() noexcept;
// Local slots 0/1 are a separate defensive group, not general cast slots.
[[nodiscard]] int defensive_miracle_index(CardId id) noexcept;
[[nodiscard]] const DefensiveMiracle* find_defensive_miracle(CardId id) noexcept;
}
