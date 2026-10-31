#include "godfield/official_rules.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace godfield::official {
namespace {
const Card& card(CardId id) {
    const auto* value = find_card(id);
    if (!value) throw std::invalid_argument("unknown official model ID");
    return *value;
}
void validate(std::span<const CardId> ids) { for (auto id : ids) (void)card(id); }
void validate(std::span<const VisibleSelection> selected) { for (auto item : selected) (void)card(item.model); }
const Card& first(std::span<const CardId> ids) {
    if (ids.empty()) throw std::invalid_argument("empty official composition");
    validate(ids);
    return card(ids.front());
}
std::int64_t add(std::int64_t a, std::int64_t b) {
    if (b > std::numeric_limits<std::int64_t>::max() - a)
        throw std::overflow_error("official attack/cost overflow");
    return a + b;
}
bool trade_without_cost(Ability ability) noexcept {
    return ability == Ability::discard || ability == Ability::sacrifice || ability == Ability::sell || ability == Ability::add_item;
}
}
Attribute merge_elements(Attribute a, Attribute b) noexcept {
    if (a == b) return a;
    if (!a || !b || a == Element::darkness || b == Element::darkness) return std::nullopt;
    if (a == Element::light) return b;
    if (b == Element::light) return a;
    return std::nullopt;
}
std::int64_t attack_power(std::span<const CardId> ordered, std::int64_t base_override) {
    if (base_override < 0) throw std::invalid_argument("negative official attack override");
    const auto& base = first(ordered);
    std::int64_t power = base_override > 0 ? base_override : base.atk;
    for (std::size_t i = 1; i < ordered.size(); ++i) {
        const auto& extra = card(ordered[i]);
        if (extra.ability == Ability::double_atk) power = add(power, power);
        else if (extra.ability != Ability::cut_cost) power = add(power, extra.atk);
    }
    return power;
}
Attribute attack_element(std::span<const CardId> ordered) {
    auto element = first(ordered).element;
    for (std::size_t i = 1; i < ordered.size(); ++i) {
        const auto& extra = card(ordered[i]);
        if (extra.ability == Ability::filter_atk_element) element = std::nullopt;
        else if (extra.ability == Ability::set_element) element = extra.element;
        else if (extra.ability != Ability::cut_cost) element = merge_elements(element, extra.element);
    }
    return element;
}
Attribute defense_element(std::span<const CardId> ordered) {
    auto element = first(ordered).element;
    for (std::size_t i = 1; i < ordered.size(); ++i) element = merge_elements(element, card(ordered[i]).element);
    return element;
}
std::int64_t magic_cost(std::span<const CardId> ordered) {
    validate(ordered);
    if (ordered.empty() || trade_without_cost(card(ordered.front()).ability)) return 0;
    std::int64_t cost = 0;
    for (std::size_t i = 0; i < ordered.size(); ++i) {
        const auto& current = card(ordered[i]);
        if (current.category != Category::miracles) continue;
        if (i + 1 < ordered.size() && card(ordered[i + 1]).ability == Ability::cut_cost) continue;
        cost = add(cost, current.cost);
    }
    return cost;
}
Reaction reaction_for(CardId id, Category category, Attribute incoming) {
    auto ability = card(id).ability;
    if (ability == Ability::reflect_anything) return Reaction::reflect;
    if (category == Category::weapons && !incoming) {
        if (ability == Ability::bounce_weapon) return Reaction::bounce;
        if (ability == Ability::reflect_weapon) return Reaction::reflect;
        if (ability == Ability::block_weapon) return Reaction::block;
    }
    if (category == Category::miracles) {
        if (ability == Ability::bounce_miracle) return Reaction::bounce;
        if (ability == Ability::reflect_miracle) return Reaction::reflect;
        if (ability == Ability::block_miracle) return Reaction::block;
    }
    return Reaction::none;
}
bool attribute_defense_allowed(CardId id, Attribute incoming) {
    const auto element = card(id).element;
    if (incoming == Element::fire) return element == Element::water || element == Element::light;
    if (incoming == Element::water) return element == Element::fire || element == Element::light;
    if (incoming == Element::wood) return element == Element::stone || element == Element::light;
    if (incoming == Element::stone) return element == Element::wood || element == Element::light;
    if (incoming == Element::light) return false;
    return true;
}
bool can_start_main(CardId id) {
    const auto& c = card(id);
    if (c.category == Category::trade || c.category == Category::weapons) return true;
    if (c.category == Category::sundries) {
        return !c.is_plus_atk && c.ability != Ability::cut_cost && c.ability != Ability::revive && c.ability != Ability::attract_danger;
    }
    if (c.category == Category::miracles) {
        if (c.is_plus_atk && !c.atk) return false;
        return c.ability != Ability::bounce_weapon && c.ability != Ability::reflect_weapon && c.ability != Ability::block_weapon &&
            c.ability != Ability::bounce_miracle && c.ability != Ability::reflect_miracle && c.ability != Ability::block_miracle && c.ability != Ability::reflect_anything;
    }
    return false;
}
bool targets_enemy_by_default(CardId id) {
    const auto& c = card(id);
    if (c.hit_rate > 0) return false;
    if (c.atk > 0 || c.category == Category::weapons) return true;
    return c.ability == Ability::sell || c.ability == Ability::buy || c.ability == Ability::boost_cp_to_enemy || c.ability == Ability::take_cp ||
        c.ability == Ability::add_curse || c.ability == Ability::remove_items || c.ability == Ability::remove_used_miracles;
}
bool can_append_main(std::span<const VisibleSelection> selected, VisibleSelection next) {
    validate(selected);
    const auto& extra = card(next.model);
    if (selected.empty()) return can_start_main(next.model);
    const auto& base = card(selected.front().model);
    if (base.ability == Ability::discard || base.ability == Ability::sacrifice)
        return extra.category != Category::weapons && extra.ability != Ability::revive && extra.ability != Ability::attract_danger;
    if (base.ability == Ability::sell)
        return selected.size() == 1 && !next.used_miracle && extra.ability != Ability::discard && extra.ability != Ability::sacrifice;
    if (card(selected.back().model).category == Category::miracles && extra.ability == Ability::cut_cost) return true;
    return extra.is_plus_atk && base.category == Category::weapons && base.hit_rate == 0;
}
bool can_append_response(std::span<const VisibleSelection> selected, VisibleSelection next, ResponseContext context) {
    validate(selected);
    const auto& extra = card(next.model);
    if (context.flash && !selected.empty()) return false;
    if (!selected.empty() && card(selected.back().model).category == Category::miracles && extra.ability == Ability::cut_cost) return true;
    const auto incoming = !selected.empty() && card(selected.front().model).ability == Ability::filter_atk_element ? Attribute{} : context.incoming_element;
    if (reaction_for(next.model, context.incoming_category, incoming) != Reaction::none)
        return selected.empty() || card(selected.back().model).ability == Ability::filter_atk_element;
    if (std::any_of(selected.begin(), selected.end(), [&](auto item) { return reaction_for(item.model, context.incoming_category, incoming) != Reaction::none; })) return false;
    if (!context.incoming_is_attack || !(extra.defense > 0 || extra.category == Category::armor)) return false;
    if (extra.ability == Ability::filter_atk_element)
        return selected.empty() || card(selected.back().model).ability == Ability::filter_atk_element;
    return attribute_defense_allowed(next.model, incoming);
}
}
