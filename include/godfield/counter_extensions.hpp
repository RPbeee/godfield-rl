#pragma once

#include "godfield/harms.hpp"
#include <span>

namespace godfield::legacy {
enum class CounterExtensionEffect : std::uint8_t { damage, harm, mana };
enum class CounterExtensionTarget : std::uint8_t { all_enemies, incoming_source, self };
struct CounterExtensionCard {
    Card card;
    Element element;
    CounterExtensionEffect effect;
    int multiplier;
    int hit_rate;
    Harm harm;
    CounterExtensionTarget target;
};

inline constexpr int counter_extension_card_count = 4;
inline constexpr const char* counter_extensions_version = "legacy-counter-extensions-v1";
inline constexpr const char* counter_extensions_sha256 = "f1cf29e7839df0d5664b5851a414cf67e53cf1a34209cf50683b7a0e0380e97f";
[[nodiscard]] std::span<const CounterExtensionCard> counter_extensions_catalog() noexcept;
[[nodiscard]] const CounterExtensionCard* find_counter_extension_card(CardId id) noexcept;
}
