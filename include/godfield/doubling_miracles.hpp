#pragma once

#include "godfield/elements.hpp"
#include <span>

namespace godfield::legacy {
struct DoublingMiracle {
    CardId id;
    const char* name;
    Element element;
    int cost;
    int weight;
};

inline constexpr int doubling_miracle_count = 1;
inline constexpr const char* doubling_miracles_version = "legacy-doubling-miracles-v1";
inline constexpr const char* doubling_miracles_sha256 = "fa51237e871fd606bf3f81bb063ac71b250bfe5a49f2f28ed3c254f6514de739";
[[nodiscard]] std::span<const DoublingMiracle> doubling_miracles() noexcept;
[[nodiscard]] const DoublingMiracle* find_doubling_miracle(CardId id) noexcept;
[[nodiscard]] int doubling_miracle_index(CardId id) noexcept;
}
