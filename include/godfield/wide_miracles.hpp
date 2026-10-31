#pragma once

#include "godfield/elements.hpp"
#include <span>

namespace godfield::legacy {
struct WideMiracle {
    CardId id;
    const char* name;
    Element element;
    int cost;
    int weight;
    int hit_rate;
};

inline constexpr int wide_miracle_count = 1;
inline constexpr const char* wide_miracles_version = "legacy-doubling-miracles-v1";
inline constexpr const char* wide_miracles_sha256 = "f27ec84ab8cbc68c8f018cfb33b705f30e800d342db1d3166b9b85af20c443d5";
[[nodiscard]] std::span<const WideMiracle> wide_miracles() noexcept;
[[nodiscard]] const WideMiracle* find_wide_miracle(CardId id) noexcept;
[[nodiscard]] int wide_miracle_index(CardId id) noexcept;
}
