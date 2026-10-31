#pragma once
#include "godfield/official_catalog.hpp"
#include <cstdint>
#include <optional>
#include <span>

namespace godfield::official {
using Attribute = std::optional<Element>;
enum class Reaction : std::uint8_t { none, bounce, reflect, block };

// These functions reproduce public client composition/selection rules. They are
// not a complete server simulator and do not decide hidden outcomes or turn order.
// Numeric conformance fixtures stay within JavaScript's safe integer range.
// Larger values use checked int64 arithmetic, an extension rather than JS rounding.
[[nodiscard]] Attribute merge_elements(Attribute first, Attribute second) noexcept;
[[nodiscard]] std::int64_t attack_power(std::span<const CardId> ordered, std::int64_t base_override = 0);
[[nodiscard]] Attribute attack_element(std::span<const CardId> ordered);
[[nodiscard]] Attribute defense_element(std::span<const CardId> ordered);
[[nodiscard]] std::int64_t magic_cost(std::span<const CardId> ordered);
[[nodiscard]] Reaction reaction_for(CardId card, Category incoming_category, Attribute incoming_element);
[[nodiscard]] bool attribute_defense_allowed(CardId card, Attribute incoming_element);
[[nodiscard]] bool can_start_main(CardId card);
[[nodiscard]] bool targets_enemy_by_default(CardId card);

struct VisibleSelection {
    CardId model = 0; // Displayed model, including a dream disguise; never the hidden true ID.
    bool used_miracle = false;
};
struct ResponseContext {
    bool flash = false;
    Category incoming_category = Category::weapons;
    Attribute incoming_element;
    bool incoming_is_attack = true;
};
// Mirrors the UI's append predicate (jM.ev), not the server's hidden validation.
// All IDs in this API belong to the visible models, not physical instance IDs.
[[nodiscard]] bool can_append_main(std::span<const VisibleSelection> selected, VisibleSelection next);
[[nodiscard]] bool can_append_response(std::span<const VisibleSelection> selected, VisibleSelection next, ResponseContext context);
}
