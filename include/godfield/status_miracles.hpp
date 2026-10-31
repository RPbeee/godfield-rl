#pragma once

#include "godfield/disease.hpp"
#include "godfield/harms.hpp"
#include <array>
#include <span>

namespace godfield::legacy {
struct StatusMiracle {
    CardId id;
    const char* name;
    Element element;
    Cure cure;
    Disease disease;
    Harm harm;
    int cost, price, weight;
};
inline constexpr int status_miracle_count = 5;
inline constexpr const char* status_miracles_version = "legacy-status-miracles-v1";
inline constexpr const char* status_miracles_sha256 = "ee9ff379ac79e9f7303cdc77334bf2c6951005fc7930cd5d931b707377e1c0cd";
inline constexpr std::array<CardId, 27> status_retained_type_ids{
    197, 198, 199, 200, 202, 203, 204, 205, 206, 207, 208,
    209, 210, 211, 212, 214, 215, 216, 217, 218, 219, 220,
    221, 222, 223, 224, 226};
[[nodiscard]] std::span<const StatusMiracle> status_miracles() noexcept;
[[nodiscard]] const StatusMiracle* find_status_miracle(CardId id) noexcept;
[[nodiscard]] int status_miracle_index(CardId id) noexcept;
}
