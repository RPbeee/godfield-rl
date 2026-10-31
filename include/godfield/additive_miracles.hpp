#pragma once

#include "godfield/elements.hpp"
#include <span>

namespace godfield::legacy {
struct AdditiveMiracle {
    CardId id;
    const char* name;
    Element element;
    int amount;
    int cost;
    int weight;
};

inline constexpr int additive_miracle_count = 2;
inline constexpr const char* additive_miracles_version = "legacy-additive-miracles-v1";
inline constexpr const char* additive_miracles_sha256 = "a78dafd8ad33105b0d7cae48c281dacc2c482a2ade44d595438903a1d21cd997";
[[nodiscard]] std::span<const AdditiveMiracle> additive_miracles() noexcept;
[[nodiscard]] const AdditiveMiracle* find_additive_miracle(CardId id) noexcept;
[[nodiscard]] int additive_miracle_index(CardId id) noexcept;
}
