#include "godfield/legacy_env.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace godfield::legacy {
namespace {
std::uint64_t random(std::uint64_t& state) noexcept {
    auto z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}
std::uint32_t bounded(std::uint64_t& state, std::uint32_t bound) noexcept {
    const auto threshold = (std::uint64_t{0} - bound) % bound;
    for (;;) { const auto x = random(state); if (x >= threshold) return x % bound; }
}
std::uint8_t pick(std::uint16_t mask, std::uint64_t& rng) noexcept {
    auto index = bounded(rng, std::popcount(mask));
    while (index--) mask &= mask - 1;
    return static_cast<std::uint8_t>(std::countr_zero(mask));
}
bool can_defend(Element incoming, Element defense) noexcept {
    switch (incoming) {
    case Element::none: return false;
    case Element::neutral: case Element::dark: return true;
    case Element::fire: return defense == Element::water || defense == Element::light;
    case Element::water: return defense == Element::fire || defense == Element::light;
    case Element::wood: return defense == Element::stone || defense == Element::light;
    case Element::stone: return defense == Element::wood || defense == Element::light;
    case Element::light: return defense == Element::dark;
    }
    return false;
}
Element combine(Element first, Element next) noexcept {
    // Ordinary Light is a mixing identity. ADD_ATTRIBUTE remains unsupported.
    if (first == Element::light) return next;
    if (next == Element::light) return first;
    return first == next ? first : Element::neutral;
}
struct Spell {
    CardId id = 0;
    Effect effect = Effect::none;
    Element element = Element::neutral;
    int amount = 0, cost = 0, hit_rate = 0;
    Harm harm = Harm::none;
    Disease disease = Disease::none;
    Cure cure = Cure::none;
};
Spell spell(CardId id) noexcept {
    if (const auto* m = find_status_miracle(id))
        return {m->id, Effect::status, m->element, 0, m->cost, 0, m->harm, m->disease, m->cure};
    if (const auto* m = find_miracle(id))
        return {m->id, m->effect == MiracleEffect::heal ? Effect::heal : Effect::money,
                Element::neutral, m->amount, m->cost, 0, Harm::none};
    if (const auto* m = find_offensive_miracle(id))
        return {m->id, Effect::damage, m->element, m->amount, m->cost, m->hit_rate, m->harm};
    if (const auto* m = find_hp_absorption_miracle(id))
        return {m->id, Effect::damage, m->element, m->amount, m->cost, 0, Harm::none};
    if (const auto* m = find_defensive_miracle(id))
        return {m->id, Effect::none, Element::neutral, 0, m->cost, 0, Harm::none};
    if (const auto* m = find_additive_miracle(id))
        return {m->id, Effect::damage, m->element, m->amount, m->cost, 0, Harm::none};
    if (const auto* m = find_doubling_miracle(id))
        return {m->id, Effect::none, m->element, 0, m->cost, 0, Harm::none};
    if (const auto* m = find_wide_miracle(id))
        return {m->id, Effect::none, m->element, 0, m->cost, m->hit_rate, Harm::none};
    return {};
}
int spell_index(CardId id) noexcept {
    if (find_hp_absorption_miracle(id)) return hp_absorption_ability_index;
    if (const auto index = miracle_index(id); index >= 0) return index;
    const auto index = offensive_miracle_index(id);
    return index < 0 ? -1 : miracle_count + index;
}
CardId spell_id(int index) noexcept {
    if (index == hp_absorption_ability_index) return hp_absorption_miracles()[0].id;
    return index < miracle_count ? miracles()[index].id : offensive_miracles()[index - miracle_count].id;
}
std::uint64_t bounded64(std::uint64_t& state, std::uint64_t bound) noexcept {
    const auto threshold = (std::uint64_t{0} - bound) % bound;
    for (;;) { const auto x = random(state); if (x >= threshold) return x % bound; }
}
std::uint64_t retained_count(const Player& player, CardId id) noexcept {
    if (const int i = status_miracle_index(id); i >= 0) return player.status_abilities[i];
    if (const int i = spell_index(id); i >= 0) return player.abilities[i];
    if (const int i = defensive_miracle_index(id); i >= 0) return player.defensive_abilities[i];
    if (const int i = additive_miracle_index(id); i >= 0) return player.additive_abilities[i];
    if (find_doubling_miracle(id)) return player.doubling_ability;
    if (find_wide_miracle(id)) return player.wide_ability;
    return 0;
}
std::uint64_t retained_total(const Player& player) noexcept {
    std::uint64_t total = 0;
    for (const auto id : status_retained_type_ids) total += retained_count(player, id);
    return total;
}
void remove_retained(Player& player, CardId id) noexcept {
    if (const int i = status_miracle_index(id); i >= 0) --player.status_abilities[i];
    else if (const int i = spell_index(id); i >= 0) --player.abilities[i];
    else if (const int i = defensive_miracle_index(id); i >= 0) --player.defensive_abilities[i];
    else if (const int i = additive_miracle_index(id); i >= 0) --player.additive_abilities[i];
    else if (find_doubling_miracle(id)) --player.doubling_ability;
    else if (find_wide_miracle(id)) --player.wide_ability;
}
bool held_at(const AttackComposition& record, int i) noexcept {
    return i < 64 ? ((record.held_mask >> i) & 1ULL) != 0 : record.held_tail;
}
void set_held(AttackComposition& record, int i) noexcept {
    if (i < 64) record.held_mask |= 1ULL << i;
    else record.held_tail = true;
}
bool can_special(const Pending& pending, DefensiveMiracleKind kind) noexcept {
    if (pending.counter_card) return false;
    if (kind == DefensiveMiracleKind::bounce_magic) return pending.miracle != 0;
    return pending.kind == Effect::damage && !pending.miracle && pending.element == Element::neutral;
}
bool tracks_retained_deals(const Config& config) noexcept {
    return config.defensive_miracles || config.magic_reflection || config.any_reflection || config.magic_block || config.additive_miracles || config.magic_bounce || config.status_miracles;
}
bool is_magic_item(const Pending& pending) noexcept {
    return pending.miracle != 0 && !pending.counter_card;
}
bool attribute_attack(ActionKind kind) noexcept {
    return kind == ActionKind::attack_set_fire || kind == ActionKind::attack_set_water
        || kind == ActionKind::attack_set_neutral;
}
bool weapon_attack(ActionKind kind) noexcept { return kind == ActionKind::attack || attribute_attack(kind); }
bool can_weapon_react(const Pending& pending) noexcept {
    return pending.kind == Effect::damage && !pending.miracle && !pending.counter_card && pending.element == Element::neutral
        && (!pending.hit_rate || pending.attribute_removed || pending.wide_miracle);
}
Element attribute_result(ActionKind kind) noexcept {
    if (kind == ActionKind::attack_set_fire) return Element::fire;
    if (kind == ActionKind::attack_set_water) return Element::water;
    return Element::neutral;
}
struct CounterInfo {
    CardId id = 0;
    Element element = Element::neutral;
    Effect effect = Effect::none;
    int multiplier = 0, hit_rate = 0;
    Harm harm = Harm::none;
    CounterExtensionTarget target = CounterExtensionTarget::incoming_source;
};
CounterInfo counter_info(CardId id) noexcept {
    if (const auto* c = find_counter_card(id))
        return {id, c->element, Effect::damage, c->multiplier, 0, Harm::none, CounterExtensionTarget::incoming_source};
    if (const auto* c = find_counter_extension_card(id))
        return {id, c->element, c->effect == CounterExtensionEffect::damage ? Effect::damage
                : (c->effect == CounterExtensionEffect::harm ? Effect::harm : Effect::mana),
                c->multiplier, c->hit_rate, c->harm, c->target};
    if (const auto* c = find_money_counter_card(id))
        return {id, c->element, Effect::absorb_money, c->multiplier, 0, Harm::none, CounterExtensionTarget::incoming_source};
    return {};
}
}
Env::Env(Config config) {
    if (!config.max_decisions || config.initial_hand > hand_capacity
        || ((config.chance || config.disease) && !config.elements)
        || (config.celestial && (!config.elements || !config.chance || !config.harms))
        || (config.offensive_miracles && (!config.miracles || !config.celestial))
        || (config.defensive_miracles && (!config.offensive_miracles || !config.reflection))
        || ((config.attribute_change || config.attribute_removal || config.counters) && !config.elements)
        || (config.counter_extensions && (!config.counters || !config.celestial))
        || (config.money_counter && !config.counter_extensions)
        || (config.magic_reflection && (!config.reflection || !config.miracles))
        || (config.magic_bounce && (!config.reflection || !config.miracles))
        || (config.status_miracles && (!config.miracles || !config.disease || !config.celestial))
        || (config.dying_weapon && !config.celestial)
        || (config.any_reflection && !config.reflection)
        || (config.additive_miracles && (!config.miracles || !config.celestial))
        || (config.doubling_miracles && !config.additive_miracles)
        || (config.wide_miracles && !config.additive_miracles)
        || (config.magic_free && !config.additive_miracles)
        || (config.forgetting && !config.miracles)
        || (config.hp_absorption && (!config.miracles || !config.celestial))
        || (config.pestle_mortar && !config.celestial))
        throw std::invalid_argument("invalid legacy environment config");
    snapshot_.config = config;
    reset(0);
}
bool Env::supported(CardId id) const noexcept {
    return find_card(id) || (snapshot_.config.miracles && find_miracle(id))
        || (snapshot_.config.elements && find_element_card(id))
        || (snapshot_.config.chance && find_chance_card(id))
        || (snapshot_.config.reflection && find_reaction_card(id))
        || (snapshot_.config.disease && find_disease_card(id))
        || (snapshot_.config.harms && find_harm_card(id))
        || (snapshot_.config.celestial && find_celestial_card(id))
        || (snapshot_.config.offensive_miracles && find_offensive_miracle(id))
        || (snapshot_.config.defensive_miracles && find_defensive_miracle(id))
        || (snapshot_.config.attribute_change && find_attribute_change_card(id))
        || (snapshot_.config.attribute_removal && find_attribute_removal_card(id))
        || (snapshot_.config.counters && find_counter_card(id))
        || (snapshot_.config.counter_extensions && find_counter_extension_card(id))
        || (snapshot_.config.money_counter && find_money_counter_card(id))
        || (snapshot_.config.magic_reflection && find_magic_reflection_card(id))
        || (snapshot_.config.any_reflection && find_any_reflection_card(id))
        || (snapshot_.config.mana_weapon && find_mana_weapon_card(id))
        || (snapshot_.config.magic_block && find_magic_block_card(id))
        || (snapshot_.config.additive_miracles && find_additive_miracle(id))
        || (snapshot_.config.doubling_miracles && find_doubling_miracle(id))
        || (snapshot_.config.wide_miracles && find_wide_miracle(id))
        || (snapshot_.config.magic_free && find_magic_free_card(id))
        || (snapshot_.config.forgetting && find_forgetting_card(id))
        || (snapshot_.config.exchanges && find_exchange_card(id))
        || (snapshot_.config.revival && find_revival_card(id))
        || (snapshot_.config.item_removal && find_item_removal_card(id))
        || (snapshot_.config.hp_absorption && (find_hp_absorption_card(id) || find_hp_absorption_miracle(id)))
        || (snapshot_.config.self_damage && find_self_damage_card(id))
        || (snapshot_.config.pestle_mortar && (find_pestle_card(id) || find_mortar_card(id)))
        || (snapshot_.config.selling && find_selling_card(id))
        || (snapshot_.config.buying && find_buying_card(id))
        || (snapshot_.config.hp_gamble && find_hp_gamble_card(id))
        || (snapshot_.config.magic_bounce && find_magic_bounce_card(id))
        || (snapshot_.config.sundry_boost && find_sundry_boost_card(id))
        || (snapshot_.config.status_miracles && find_status_miracle(id))
        || (snapshot_.config.dying_weapon && find_dying_weapon_card(id));
}
const Card* Env::artifact(CardId id) const noexcept {
    if (const auto* card = find_card(id)) return card;
    if (const auto* card = find_element_card(id)) return &card->card;
    if (const auto* card = find_chance_card(id)) return &card->card;
    if (const auto* card = find_reaction_card(id)) return &card->card;
    if (const auto* card = find_disease_card(id)) return &card->card;
    if (const auto* card = find_harm_card(id)) return &card->card;
    if (const auto* card = find_celestial_card(id)) return &card->card;
    if (const auto* card = find_attribute_change_card(id)) return &card->card;
    if (const auto* card = find_attribute_removal_card(id)) return &card->card;
    if (const auto* card = find_counter_card(id)) return &card->card;
    if (const auto* card = find_counter_extension_card(id)) return &card->card;
    if (const auto* card = find_money_counter_card(id)) return &card->card;
    if (const auto* card = find_magic_reflection_card(id)) return &card->card;
    if (const auto* card = find_any_reflection_card(id)) return &card->card;
    if (const auto* card = find_mana_weapon_card(id)) return &card->card;
    if (const auto* card = find_magic_block_card(id)) return &card->card;
    if (const auto* card = find_magic_free_card(id)) return &card->card;
    if (const auto* card = find_forgetting_card(id)) return &card->card;
    if (const auto* card = find_exchange_card(id)) return &card->card;
    if (const auto* card = find_revival_card(id)) return &card->card;
    if (const auto* card = find_item_removal_card(id)) return &card->card;
    if (const auto* card = find_hp_absorption_card(id)) return &card->card;
    if (const auto* card = find_self_damage_card(id)) return &card->card;
    if (const auto* card = find_pestle_card(id)) return &card->card;
    if (const auto* card = find_mortar_card(id)) return &card->card;
    if (const auto* card = find_selling_card(id)) return &card->card;
    if (const auto* card = find_buying_card(id)) return &card->card;
    if (const auto* card = find_hp_gamble_card(id)) return &card->card;
    if (const auto* card = find_magic_bounce_card(id)) return &card->card;
    if (const auto* card = find_sundry_boost_card(id)) return &card->card;
    if (const auto* card = find_dying_weapon_card(id)) return &card->card;
    return nullptr;
}
int Env::trade_price(CardId id) const noexcept {
    if (snapshot_.config.dying_weapon) if (const auto* card = find_dying_weapon_card(id)) return card->card.price;
    if (snapshot_.config.status_miracles) if (const auto* m = find_status_miracle(id)) return m->price;
    if (snapshot_.config.sundry_boost) if (const auto* card = find_sundry_boost_card(id)) return card->card.price;
    if (snapshot_.config.magic_bounce) if (const auto* card = find_magic_bounce_card(id)) return card->card.price;
    if (snapshot_.config.hp_gamble) if (const auto* card = find_hp_gamble_card(id)) return card->card.price;
    if (snapshot_.config.buying) if (const auto* card = find_buying_card(id)) return card->card.price;
    return sale_price(id);
}
Element Env::element_of(CardId id) const noexcept {
    if (const auto* m = find_status_miracle(id)) return m->element;
    if (const auto* card = find_dying_weapon_card(id)) return card->element;
    if (const auto* card = find_pestle_card(id)) return card->element;
    if (const auto* card = find_hp_absorption_card(id)) return card->element;
    if (const auto* m = find_hp_absorption_miracle(id)) return m->element;
    if (const auto* card = find_element_card(id)) return card->element;
    if (const auto* card = find_chance_card(id)) return card->element;
    if (const auto* card = find_disease_card(id)) return card->element;
    if (const auto* card = find_harm_card(id)) return card->element;
    if (const auto* card = find_celestial_card(id)) return card->element;
    if (const auto* m = find_offensive_miracle(id)) return m->element;
    if (const auto* m = find_additive_miracle(id)) return m->element;
    if (const auto* card = find_attribute_change_card(id)) return card->element;
    if (const auto* card = find_counter_card(id)) return card->element;
    if (const auto* card = find_counter_extension_card(id)) return card->element;
    if (const auto* card = find_money_counter_card(id)) return card->element;
    return Element::neutral;
}
int Env::chance_rate(CardId id) const noexcept {
    if (const auto* card = find_dying_weapon_card(id)) return card->hit_rate;
    if (const auto* card = find_hp_absorption_card(id)) return card->hit_rate;
    if (const auto* card = find_chance_card(id)) return card->hit_rate;
    if (const auto* card = find_celestial_card(id)) return card->hit_rate;
    if (const auto* m = find_offensive_miracle(id)) return m->hit_rate;
    if (const auto* c = find_counter_extension_card(id)) return c->hit_rate;
    if (const auto* m = find_wide_miracle(id)) return m->hit_rate;
    return 0;
}
CardId Env::attribute_component(Action action) const noexcept {
    if (!snapshot_.config.attribute_change || !attribute_attack(action.kind)) return 0;
    const auto& p = state().players[state().actor];
    for (int i = 0; i < p.hand_count; ++i) if (action.slots & (1u << i)) {
        const auto* setter = find_attribute_change_card(p.hand[i]);
        if (!setter) continue;
        if (action.kind != ActionKind::attack_set_neutral) {
            if (setter->element == attribute_result(action.kind)) return setter->card.id;
        } else {
            // Other extras, then this setter, then an incompatible ordinary
            // boost are a witness for NE. The base is never a suffix witness.
            for (int j = 0; j < p.hand_count; ++j) if (action.slots & (1u << j)) {
                const auto* boost = artifact(p.hand[j]);
                const auto element = element_of(p.hand[j]);
                if (boost && boost->boost > 0 && element != Element::light && element != setter->element)
                    return setter->card.id;
            }
        }
    }
    return 0;
}
CardId Env::draw() noexcept {
    if (snapshot_.draw_index < snapshot_.draw_count)
        return snapshot_.scripted_draws[snapshot_.draw_index++];
    static const int weight = [] { int sum = 0; for (const auto& c : catalog()) sum += c.weight; return sum; }();
    int total_weight = weight;
    if (snapshot_.config.miracles) for (const auto& m : miracles()) total_weight += m.weight;
    if (snapshot_.config.elements) {
        static const int element_weight = [] {
            int sum = 0; for (const auto& c : element_catalog()) sum += c.card.weight; return sum;
        }();
        total_weight += element_weight;
    }
    if (snapshot_.config.chance) {
        static const int chance_weight = [] {
            int sum = 0; for (const auto& c : chance_catalog()) sum += c.card.weight; return sum;
        }();
        total_weight += chance_weight;
    }
    if (snapshot_.config.reflection) {
        static const int reaction_weight = [] {
            int sum = 0; for (const auto& c : reaction_catalog()) sum += c.card.weight; return sum;
        }();
        total_weight += reaction_weight;
    }
    if (snapshot_.config.disease) {
        static const int disease_weight = [] {
            int sum = 0; for (const auto& c : disease_catalog()) sum += c.card.weight; return sum;
        }();
        total_weight += disease_weight;
    }
    if (snapshot_.config.harms) {
        static const int harm_weight = [] {
            int sum = 0; for (const auto& c : harm_catalog()) sum += c.card.weight; return sum;
        }();
        total_weight += harm_weight;
    }
    if (snapshot_.config.celestial) {
        static const int celestial_weight = [] {
            int sum = 0; for (const auto& c : celestial_catalog()) sum += c.card.weight; return sum;
        }();
        total_weight += celestial_weight;
    }
    if (snapshot_.config.offensive_miracles)
        for (const auto& m : offensive_miracles()) total_weight += m.weight;
    if (snapshot_.config.defensive_miracles)
        for (const auto& m : defensive_miracles()) total_weight += m.weight;
    if (snapshot_.config.attribute_change)
        for (const auto& c : attribute_change_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.attribute_removal)
        for (const auto& c : attribute_removal_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.counters)
        for (const auto& c : counter_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.counter_extensions)
        for (const auto& c : counter_extensions_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.money_counter)
        for (const auto& c : money_counter_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.magic_reflection)
        for (const auto& c : magic_reflection_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.any_reflection)
        for (const auto& c : any_reflection_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.mana_weapon)
        for (const auto& c : mana_weapon_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.magic_block)
        for (const auto& c : magic_block_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.additive_miracles)
        for (const auto& m : additive_miracles()) total_weight += m.weight;
    if (snapshot_.config.doubling_miracles)
        for (const auto& m : doubling_miracles()) total_weight += m.weight;
    if (snapshot_.config.wide_miracles)
        for (const auto& m : wide_miracles()) total_weight += m.weight;
    if (snapshot_.config.magic_free)
        for (const auto& m : magic_free_catalog()) total_weight += m.card.weight;
    if (snapshot_.config.forgetting)
        for (const auto& c : forgetting_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.exchanges)
        for (const auto& c : exchange_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.revival)
        for (const auto& c : revival_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.item_removal)
        for (const auto& c : item_removal_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.hp_absorption) {
        for (const auto& c : hp_absorption_catalog()) total_weight += c.card.weight;
        for (const auto& m : hp_absorption_miracles()) total_weight += m.weight;
    }
    if (snapshot_.config.self_damage)
        for (const auto& c : self_damage_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.pestle_mortar) {
        for (const auto& c : pestle_catalog()) total_weight += c.card.weight;
        for (const auto& c : mortar_catalog()) total_weight += c.card.weight;
    }
    if (snapshot_.config.selling)
        for (const auto& c : selling_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.buying)
        for (const auto& c : buying_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.hp_gamble)
        for (const auto& c : hp_gamble_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.magic_bounce)
        for (const auto& c : magic_bounce_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.sundry_boost)
        for (const auto& c : sundry_boost_catalog()) total_weight += c.card.weight;
    if (snapshot_.config.status_miracles)
        for (const auto& m : status_miracles()) total_weight += m.weight;
    if (snapshot_.config.dying_weapon)
        for (const auto& c : dying_weapon_catalog()) total_weight += c.card.weight;
    auto ticket = bounded(snapshot_.rng_state, total_weight);
    for (const auto& c : catalog()) {
        if (ticket < static_cast<unsigned>(c.weight)) return c.id;
        ticket -= c.weight;
    }
    if (snapshot_.config.miracles) for (const auto& m : miracles()) {
        if (ticket < static_cast<unsigned>(m.weight)) return m.id;
        ticket -= m.weight;
    }
    if (snapshot_.config.elements) for (const auto& c : element_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.chance) for (const auto& c : chance_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.reflection) for (const auto& c : reaction_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.disease) for (const auto& c : disease_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.harms) for (const auto& c : harm_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.celestial) for (const auto& c : celestial_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.offensive_miracles) for (const auto& m : offensive_miracles()) {
        if (ticket < static_cast<unsigned>(m.weight)) return m.id;
        ticket -= m.weight;
    }
    if (snapshot_.config.defensive_miracles) for (const auto& m : defensive_miracles()) {
        if (ticket < static_cast<unsigned>(m.weight)) return m.id;
        ticket -= m.weight;
    }
    if (snapshot_.config.attribute_change) for (const auto& c : attribute_change_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.attribute_removal) for (const auto& c : attribute_removal_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.counters) for (const auto& c : counter_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.counter_extensions) for (const auto& c : counter_extensions_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.money_counter) for (const auto& c : money_counter_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.magic_reflection) for (const auto& c : magic_reflection_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.any_reflection) for (const auto& c : any_reflection_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.mana_weapon) for (const auto& c : mana_weapon_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.magic_block) for (const auto& c : magic_block_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.additive_miracles) for (const auto& m : additive_miracles()) {
        if (ticket < static_cast<unsigned>(m.weight)) return m.id;
        ticket -= m.weight;
    }
    if (snapshot_.config.doubling_miracles) for (const auto& m : doubling_miracles()) {
        if (ticket < static_cast<unsigned>(m.weight)) return m.id;
        ticket -= m.weight;
    }
    if (snapshot_.config.wide_miracles) for (const auto& m : wide_miracles()) {
        if (ticket < static_cast<unsigned>(m.weight)) return m.id;
        ticket -= m.weight;
    }
    if (snapshot_.config.magic_free) for (const auto& m : magic_free_catalog()) {
        if (ticket < static_cast<unsigned>(m.card.weight)) return m.card.id;
        ticket -= m.card.weight;
    }
    if (snapshot_.config.forgetting) for (const auto& c : forgetting_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.exchanges) for (const auto& c : exchange_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.revival) for (const auto& c : revival_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.item_removal) for (const auto& c : item_removal_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.hp_absorption) {
        for (const auto& c : hp_absorption_catalog()) {
            if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
            ticket -= c.card.weight;
        }
        for (const auto& m : hp_absorption_miracles()) {
            if (ticket < static_cast<unsigned>(m.weight)) return m.id;
            ticket -= m.weight;
        }
    }
    if (snapshot_.config.self_damage) for (const auto& c : self_damage_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.pestle_mortar) {
        for (const auto& c : pestle_catalog()) {
            if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
            ticket -= c.card.weight;
        }
        for (const auto& c : mortar_catalog()) {
            if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
            ticket -= c.card.weight;
        }
    }
    if (snapshot_.config.selling) for (const auto& c : selling_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.buying) for (const auto& c : buying_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.hp_gamble) for (const auto& c : hp_gamble_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.magic_bounce) for (const auto& c : magic_bounce_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.sundry_boost) for (const auto& c : sundry_boost_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    if (snapshot_.config.status_miracles) for (const auto& m : status_miracles()) {
        if (ticket < static_cast<unsigned>(m.weight)) return m.id;
        ticket -= m.weight;
    }
    if (snapshot_.config.dying_weapon) for (const auto& c : dying_weapon_catalog()) {
        if (ticket < static_cast<unsigned>(c.card.weight)) return c.card.id;
        ticket -= c.card.weight;
    }
    return catalog().back().id;
}
void Env::deal(int seat, std::uint64_t count) noexcept {
    auto& p = snapshot_.state.players[seat];
    for (std::uint64_t i = 0; i < count; ++i) {
        const auto id = draw(); // Consume outcomes even when the hand is full.
        if (p.hand_count < hand_capacity) p.hand[p.hand_count++] = id;
    }
}
void Env::reset(std::uint64_t seed) noexcept {
    const auto config = snapshot_.config;
    snapshot_ = Snapshot{};
    snapshot_.config = config;
    snapshot_.rng_state = seed;
    deal(0, config.initial_hand);
    deal(1, config.initial_hand);
    auto& s = snapshot_.state;
    if (bounded(snapshot_.rng_state, 2)) std::swap(s.order[0], s.order[1]);
    s.actor = s.turn_owner = s.order[0];
}
bool Env::replay_composition(const AttackComposition& record, CompositionPlan& plan) const noexcept {
    plan = {};
    if (!snapshot_.config.additive_miracles || !supported(record.base) || !record.count
        || record.count > (snapshot_.config.magic_free ? attack_extra_capacity : legacy_attack_extra_capacity) || record.initial_mp > 99) return false;
    const auto* base = artifact(record.base);
    if (!base || base->kind != Kind::weapon || find_pestle_card(record.base) || chance_rate(record.base)
        || (!base->attack && !find_mana_weapon_card(record.base))) return false;
    if ((record.count < 64 && (record.held_mask >> record.count))
        || (record.count <= 64 && record.held_tail)) return false;
    const auto cap = 15ULL * snapshot_.config.max_decisions;
    if (record.initial_additive[0] > cap || record.initial_additive[1] > cap - record.initial_additive[0]) return false;
    const auto initial_additive = record.initial_additive[0] + record.initial_additive[1];
    if ((!snapshot_.config.doubling_miracles && record.initial_doubling)
        || record.initial_doubling > cap - initial_additive) return false;
    if ((!snapshot_.config.wide_miracles && record.initial_wide)
        || record.initial_wide > cap - initial_additive - record.initial_doubling) return false;
    plan.acquired = record.initial_additive;
    plan.acquired_doubling = record.initial_doubling;
    plan.acquired_wide = record.initial_wide;
    int mp = record.initial_mp;
    const bool mana = find_mana_weapon_card(record.base) != nullptr;
    plan.amount = mana ? 2 * mp : base->attack;
    plan.element = element_of(record.base);
    if (snapshot_.config.magic_free) {
        for (int i = 0; i < record.count; ++i) if (find_magic_free_card(record.items[i])) {
            // Index0 in items is position1 in the full command. The first FREE
            // after the base marks exactly its immediate predecessor.
            plan.free_index = i;
            plan.free_card = record.items[i];
            break;
        }
    }
    plan.free_cards = snapshot_.config.magic_free && find_magic_free_card(record.base) ? 1 : 0;
    bool used_magic = false;
    for (int i = 0; i < attack_extra_capacity; ++i) {
        const auto id = record.items[i];
        if (i >= record.count) { if (id) return false; continue; }
        if (!supported(id)) return false;
        const bool held = held_at(record, i);
        const bool free = plan.free_index == i + 1;
        if (find_magic_free_card(id)) {
            if (!snapshot_.config.magic_free || held || !used_magic) return false;
            ++plan.physical;
            ++plan.free_cards;
            continue; // After a MAGIC, a carrier contributes no ATK/DFS/color.
        }
        if (const auto* m = find_additive_miracle(id)) {
            const auto index = additive_miracle_index(id);
            if (held) {
                if (!plan.acquired[index]) return false;
                ++plan.held;
                ++plan.additive_recasts;
            } else {
                ++plan.acquired[index];
                ++plan.physical;
                ++plan.first_uses;
            }
            const int paid = free ? 0 : m->cost;
            if (mp < paid) return false;
            mp -= paid;
            plan.cost += paid;
            plan.additive_cost += paid;
            if (free) plan.mp_saved = static_cast<std::uint16_t>(m->cost);
            if (mana && !free) plan.amount = 2 * mp;
            used_magic = true;
            plan.amount += m->amount;
            plan.element = combine(plan.element, m->element);
        } else if (const auto* m = find_doubling_miracle(id)) {
            const int paid = free ? 0 : m->cost;
            if (!snapshot_.config.doubling_miracles || mp < paid) return false;
            if (held) {
                if (!plan.acquired_doubling) return false;
                ++plan.held;
                ++plan.doubling_recasts;
            } else {
                ++plan.physical;
                ++plan.acquired_doubling;
                ++plan.doubling_first_uses;
            }
            mp -= paid;
            plan.cost += paid;
            plan.doubling_cost += paid;
            if (free) plan.mp_saved = static_cast<std::uint16_t>(m->cost);
            if (mana && !free) plan.amount = 2 * mp;
            used_magic = true;
            plan.amount *= 2;
            plan.element = combine(plan.element, m->element);
        } else if (const auto* m = find_wide_miracle(id)) {
            const int paid = free ? 0 : m->cost;
            if (!snapshot_.config.wide_miracles || mp < paid) return false;
            if (held) {
                if (!plan.acquired_wide) return false;
                ++plan.held;
                ++plan.wide_recasts;
            } else {
                ++plan.physical;
                ++plan.acquired_wide;
                ++plan.wide_first_uses;
            }
            mp -= paid;
            plan.cost += paid;
            plan.wide_cost += paid;
            if (free) plan.mp_saved = static_cast<std::uint16_t>(m->cost);
            if (mana && !free) plan.amount = 2 * mp;
            used_magic = true;
            plan.wide = true;
            plan.element = Element::neutral;
            plan.element = combine(plan.element, m->element);
        } else {
            if (held) return false;
            const auto* extra = artifact(id);
            const auto* setter = snapshot_.config.attribute_change ? find_attribute_change_card(id) : nullptr;
            if (!extra || (!setter && extra->boost <= 0)) return false;
            ++plan.physical;
            plan.amount += extra->boost;
            if (setter) {
                plan.element = setter->element;
                plan.setter = id;
                ++plan.setters;
            } else plan.element = combine(plan.element, element_of(id));
        }
    }
    plan.mana_spent = static_cast<std::uint8_t>(mana ? mp : 0);
    return plan.physical <= hand_capacity && plan.held <= (snapshot_.config.magic_free ? 50 : 49)
        && plan.acquired[0] <= cap && plan.acquired[1] <= cap - plan.acquired[0]
        && plan.acquired_doubling <= cap - plan.acquired[0] - plan.acquired[1]
        && plan.acquired_wide <= cap - plan.acquired[0] - plan.acquired[1] - plan.acquired_doubling;
}
bool Env::prepare_ordered(Action action, AttackComposition& record, CompositionPlan& plan, std::uint16_t& slots) const noexcept {
    const auto& p = state().players[state().actor];
    if (action.kind != ActionKind::attack_ordered || action.base >= p.hand_count || action.target > 1
        || action.slots || action.counter_count || action.counter_order
        || !action.extra_count || action.extra_count > attack_extra_capacity) return false;
    record = {};
    record.base = p.hand[action.base];
    record.initial_mp = static_cast<std::uint8_t>(p.mp);
    record.initial_additive = p.additive_abilities;
    record.initial_doubling = p.doubling_ability;
    record.initial_wide = p.wide_ability;
    record.count = action.extra_count;
    slots = static_cast<std::uint16_t>(1u << action.base);
    for (int i = 0; i < attack_extra_capacity; ++i) {
        const auto ref = action.extras[i];
        if (i >= action.extra_count) { if (ref) return false; continue; }
        if (ref < hand_capacity) {
            if (ref >= p.hand_count || (slots & (1u << ref))) return false;
            slots |= static_cast<std::uint16_t>(1u << ref);
            record.items[i] = p.hand[ref];
        } else if (ref < hand_capacity + additive_miracle_count) {
            record.items[i] = additive_miracles()[ref - hand_capacity].id;
            set_held(record, i);
        } else if (ref == hand_capacity + additive_miracle_count && snapshot_.config.doubling_miracles) {
            record.items[i] = doubling_miracles()[0].id;
            set_held(record, i);
        } else if (ref == hand_capacity + additive_miracle_count + 1 && snapshot_.config.wide_miracles) {
            record.items[i] = wide_miracles()[0].id;
            set_held(record, i);
        } else return false;
    }
    return replay_composition(record, plan);
}
bool Env::prepare_sale(Action a, CardId& item, int& price) const noexcept {
    if (!snapshot_.config.selling || state().phase != Phase::main || a.kind != ActionKind::sell
        || a.base >= state().players[state().actor].hand_count || a.target > 1
        || !std::has_single_bit(a.slots) || a.counter_count || a.counter_order || a.extra_count
        || std::any_of(a.extras.begin(), a.extras.end(), [](auto ref) { return ref != 0; })) return false;
    const auto& p = state().players[state().actor];
    const auto slot = std::countr_zero(a.slots);
    if (!find_selling_card(p.hand[a.base]) || slot >= p.hand_count || slot == a.base) return false;
    item = p.hand[slot];
    price = trade_price(item);
    return price >= 0;
}
bool Env::prepare_exchange(Action a, LastExchange& record) const noexcept {
    if (!snapshot_.config.exchanges || state().phase != Phase::main || a.kind != ActionKind::exchange
        || a.base >= state().players[state().actor].hand_count || a.target != no_slot
        || a.slots || a.counter_count || a.counter_order || a.extra_count != 3) return false;
    const auto& p = state().players[state().actor];
    if (!find_exchange_card(p.hand[a.base])) return false;
    for (int i = 0; i < attack_extra_capacity; ++i)
        if (i < 3 ? a.extras[i] > 99 : a.extras[i] != 0) return false;
    if (a.extras[0] + a.extras[1] + a.extras[2] != p.hp + p.mp + p.money) return false;
    record.id = p.hand[a.base];
    record.seat = state().actor;
    record.before = {static_cast<std::uint8_t>(p.hp), static_cast<std::uint8_t>(p.mp), static_cast<std::uint8_t>(p.money)};
    std::copy_n(a.extras.begin(), 3, record.after.begin());
    return true;
}
bool Env::prepare_free_cast(Action a, FreeCastPlan& plan) const noexcept {
    if (!snapshot_.config.magic_free || state().phase != Phase::main || a.kind != ActionKind::cast_ordered
        || a.target > 1 || a.slots || a.counter_count || a.counter_order || !a.extra_count || a.extra_count > hand_capacity) return false;
    const auto& p = state().players[state().actor];
    CardId id = 0;
    bool held = false;
    if (a.base < p.hand_count) { id = p.hand[a.base]; plan.hand_base = a.base; plan.slots = static_cast<std::uint16_t>(1u << a.base); }
    else if (a.base == 16 || a.base == 17) {
        const auto index = a.base - 16;
        if (!p.additive_abilities[index]) return false;
        id = additive_miracles()[index].id; held = true;
    } else if (a.base >= 32 && a.base < 32 + main_ability_capacity) {
        if (!p.abilities[a.base - 32]) return false;
        id = spell_id(a.base - 32); held = true;
    } else if (snapshot_.config.status_miracles && a.base >= 48 && a.base < 48 + status_miracle_count) {
        if (!p.status_abilities[a.base - 48]) return false;
        id = status_miracles()[a.base - 48].id; held = true;
    } else return false;
    const auto m = spell(id);
    if (!supported(id) || !m.id || (m.effect != Effect::damage && m.effect != Effect::heal && m.effect != Effect::money && m.effect != Effect::status)) return false;
    if (!held) {
        if (find_additive_miracle(id)) {
            if (p.additive_abilities[0] + p.additive_abilities[1] + p.doubling_ability + p.wide_ability >= 15ULL * snapshot_.config.max_decisions) return false;
        } else {
            std::uint64_t acquired = 0;
            for (const auto n : p.abilities) acquired += n;
            for (const auto n : p.defensive_abilities) acquired += n;
            for (const auto n : p.status_abilities) acquired += n;
            if (acquired >= snapshot_.config.max_decisions) return false;
        }
    }
    plan.record.magic = id;
    plan.record.initial_mp = static_cast<std::uint8_t>(p.mp);
    plan.record.from_ability = held;
    plan.record.count = a.extra_count;
    for (int i = 0; i < attack_extra_capacity; ++i) {
        const auto ref = a.extras[i];
        if (i >= a.extra_count) { if (ref) return false; continue; }
        if (ref >= p.hand_count || (plan.slots & (1u << ref)) || !find_magic_free_card(p.hand[ref])) return false;
        plan.slots |= static_cast<std::uint16_t>(1u << ref);
        plan.record.items[i] = p.hand[ref];
    }
    return true;
}
bool Env::prepare_ordered_defense(Action a, DefensePlan& plan) const noexcept {
    if (!snapshot_.config.magic_free || state().phase != Phase::response || a.kind != ActionKind::defend_ordered
        || a.base != no_slot || a.target != no_slot || a.slots || a.counter_count || a.counter_order
        || !a.extra_count || a.extra_count > hand_capacity + 1) return false;
    const auto& p = state().players[state().actor];
    if (p.glory && a.extra_count > 1) return false; // Preserve the selected-card Glory contract.
    std::array<CardId, hand_capacity + 1> ids{};
    std::uint16_t seen = 0;
    int free_target = -1;
    for (int i = 0; i < attack_extra_capacity; ++i) {
        const auto ref = a.extras[i];
        if (i >= a.extra_count) { if (ref) return false; continue; }
        if (ref < p.hand_count) {
            if (seen & (1u << ref)) return false;
            seen |= static_cast<std::uint16_t>(1u << ref);
            ids[i] = p.hand[ref];
        } else if (ref == 20 || ref == 21) ids[i] = defensive_miracles()[ref - 20].id;
        else return false;
        if (i > 0 && free_target < 0 && find_magic_free_card(ids[i])) free_target = i - 1;
    }
    plan.action = {ActionKind::defend, no_slot, 0, no_slot};
    bool special = false;
    for (int i = 0; i < a.extra_count; ++i) {
        const auto id = ids[i];
        const auto ref = a.extras[i];
        const auto* m = find_defensive_miracle(id);
        const bool reaction = m || find_reaction_card(id) || find_any_reflection_card(id)
            || (is_magic_item(state().pending) && (find_magic_reflection_card(id) || find_magic_block_card(id) || find_magic_bounce_card(id)));
        if (reaction) {
            special = true;
            plan.action.kind = ref >= 20 ? ActionKind::defend_cast : ActionKind::defend;
            plan.action.base = ref >= 20 ? static_cast<std::uint8_t>(ref - 20) : ref;
            if (i + 1 < a.extra_count) {
                if (i + 2 != a.extra_count || a.extras[i + 1] >= hand_capacity || !find_magic_free_card(ids[i + 1])) return false;
                plan.suffix = true;
            }
            if (m && free_target == i) {
                plan.free_card = ids[i + 1];
                plan.mp_saved = static_cast<std::uint16_t>(m->cost);
            }
            break;
        }
        if (ref >= hand_capacity) return false;
        const auto* c = artifact(id);
        if (!c || (c->kind != Kind::armor && !c->defense && !find_magic_free_card(id))) return false;
        plan.action.slots |= static_cast<std::uint16_t>(1u << ref);
        plan.prefix[plan.prefix_count++] = ref;
        if (snapshot_.config.counter_extensions && counter_info(id).id)
            plan.action.counter_order |= static_cast<std::uint64_t>(ref) << (4 * plan.action.counter_count++);
    }
    if (!special && plan.prefix_count != a.extra_count) return false;
    if (plan.free_card) {
        auto funded = *this;
        funded.snapshot_.state.players[state().actor].mp = plan.mp_saved;
        return funded.legal(plan.action);
    }
    return legal(plan.action);
}
bool Env::restore(const Snapshot& next) noexcept {
    if (next.state.phase == Phase::purchase) {
        if (next.format_tag != snapshot_.format_tag || next.version != snapshot_.version || next.config != snapshot_.config
            || !next.config.buying || next.state.terminated
            || std::uint64_t{next.state.decisions} < std::uint64_t{next.state.reaction_count} + 2) return false;
        const auto& state = next.state;
        const auto& buy = state.last_buy;
        const auto& pending = state.pending;
        if (!find_buying_card(buy.id) || buy.id != pending.buy_card || buy.outcome != BuyOutcome::offered
            || buy.source > 1 || buy.target > 1 || buy.source == buy.target || state.actor != buy.source
            || pending.source != buy.source || pending.target != buy.target || buy.population == 0 || buy.population > hand_capacity
            || buy.slot >= buy.population || buy.population != state.players[buy.target].hand_count
            || state.players[buy.target].hand[buy.slot] != buy.item || !supported(buy.item)
            || buy.price < 0 || buy.price != trade_price(buy.item) || state.players[buy.source].money < buy.price
            || state.players[buy.source].hp <= 0 || state.players[buy.target].hp <= 0
            || state.players[buy.source].dead || state.players[buy.target].dead
            || buy.delivered || buy.money_lost || buy.money_gained
            || std::uint64_t{buy.turn} != std::uint64_t{state.turns} + 1) return false;
        // Reuse the original-response proof with its defender as actor. Only
        // the canonical live offer above changes which seat may act next.
        Snapshot response = next;
        response.state.phase = Phase::response;
        response.state.actor = pending.target;
        response.state.last_buy = {};
        if (!restore(response)) return false;
        snapshot_ = next;
        return true;
    }
    if (next.format_tag != snapshot_.format_tag || next.version != snapshot_.version
        || next.config != snapshot_.config || !next.config.max_decisions || next.config.initial_hand > hand_capacity
        || next.draw_count > next.scripted_draws.size() || next.draw_index > next.draw_count
        || next.hit_count > next.scripted_hits.size() || next.hit_index > next.hit_count
        || next.target_count > next.scripted_targets.size() || next.target_index > next.target_count
        || next.disease_count > next.scripted_diseases.size() || next.disease_index > next.disease_count
        || next.forget_count > next.scripted_forgets.size() || next.forget_index > next.forget_count
        || next.item_removal_count > next.scripted_item_removals.size() || next.item_removal_index > next.item_removal_count
        || next.pestle_target_count > next.scripted_pestle_targets.size() || next.pestle_target_index > next.pestle_target_count
        || next.buy_count > next.scripted_buys.size() || next.buy_index > next.buy_count
        || next.hp_gamble_count > next.scripted_hp_gambles.size() || next.hp_gamble_index > next.hp_gamble_count) return false;
    for (std::size_t i = 0; i < next.scripted_draws.size(); ++i) {
        if (i < next.draw_count ? !supported(next.scripted_draws[i]) : next.scripted_draws[i] != 0) return false;
    }
    if (!next.config.chance && (next.hit_count || next.hit_index)) return false;
    for (std::size_t i = 0; i < next.scripted_hits.size(); ++i) {
        const auto roll = next.scripted_hits[i];
        if (i < next.hit_count ? (roll < 1 || roll > 100) : roll != 0) return false;
    }
    if (!next.config.reflection && (next.target_count || next.target_index)) return false;
    for (std::size_t i = 0; i < next.scripted_targets.size(); ++i)
        if (i < next.target_count ? next.scripted_targets[i] > 1 : next.scripted_targets[i] != 0) return false;
    if (!next.config.disease && (next.disease_count || next.disease_index)) return false;
    for (std::size_t i = 0; i < next.scripted_diseases.size(); ++i)
        if (i < next.disease_count ? next.scripted_diseases[i] > 99 : next.scripted_diseases[i] != 0) return false;
    if (!next.config.forgetting && (next.forget_count || next.forget_index)) return false;
    for (std::size_t i = next.forget_count; i < next.scripted_forgets.size(); ++i)
        if (next.scripted_forgets[i]) return false;
    if (!next.config.item_removal && (next.item_removal_count || next.item_removal_index)) return false;
    for (std::size_t i = next.item_removal_count; i < next.scripted_item_removals.size(); ++i)
        if (next.scripted_item_removals[i]) return false;
    if (!next.config.pestle_mortar && (next.pestle_target_count || next.pestle_target_index)) return false;
    for (std::size_t i = 0; i < next.scripted_pestle_targets.size(); ++i)
        if (i < next.pestle_target_count ? next.scripted_pestle_targets[i] > 1 : next.scripted_pestle_targets[i] != 0) return false;
    if (!next.config.buying && (next.buy_count || next.buy_index)) return false;
    for (std::size_t i = next.buy_count; i < next.scripted_buys.size(); ++i) if (next.scripted_buys[i]) return false;
    if (!next.config.hp_gamble && (next.hp_gamble_count || next.hp_gamble_index)) return false;
    for (std::size_t i = 0; i < next.scripted_hp_gambles.size(); ++i)
        if (i < next.hp_gamble_count ? next.scripted_hp_gambles[i] > 1 : next.scripted_hp_gambles[i] != 0) return false;
    const auto& s = next.state;
    const auto* status_magic = next.config.status_miracles ? find_status_miracle(s.pending.miracle) : nullptr;
    const bool status_payload = s.pending.kind == Effect::status;
    if (status_payload ? !status_magic : status_magic != nullptr) return false;
    if (status_payload && (s.phase != Phase::response || s.pending.amount || s.pending.status_card || s.pending.harm_card
        || s.pending.counter_card || s.pending.mana_weapon || s.pending.mana_spent || s.pending.wide_miracle || s.pending.hit_rate
        || s.pending.hp_absorption_card || s.pending.self_damage_card || s.pending.pestle_card || s.pending.mortar_card
        || s.pending.disease != status_magic->disease || s.pending.cure != status_magic->cure || s.pending.harm != status_magic->harm)) return false;
    if (!s.last_hp_gamble.id) {
        if (s.last_hp_gamble != LastHPGamble{}) return false;
    } else {
        const auto& event = s.last_hp_gamble;
        const auto* card = find_hp_gamble_card(event.id);
        if (!next.config.hp_gamble || !card || event.source > 1 || event.target > 1
            || (event.delta != card->amount && event.delta != -card->amount) || !event.turn) return false;
        if (event.resolved) {
            if (event.turn > s.turns || event.resolved_source > 1 || event.resolved_target > 1
                || (event.source == event.target ? (event.resolved_source != event.source || event.resolved_target != event.target)
                    : event.resolved_source == event.resolved_target)
                || (event.delta > 0 ? (event.change < 0 || event.change > event.delta)
                    : (event.change >= 0 || event.change < event.delta))) return false;
        } else if (s.pending.kind != Effect::hp_gamble || event.id != s.pending.hp_gamble_card
            || std::uint64_t{event.turn} != std::uint64_t{s.turns} + 1
            || event.resolved_source != no_slot || event.resolved_target != no_slot || event.change) return false;
        if ((s.last_sale.id && s.last_sale.turn == event.turn) || (s.last_buy.id && s.last_buy.turn == event.turn)) return false;
    }
    if (!s.last_buy.id) {
        if (s.last_buy != LastBuy{}) return false;
    } else {
        const auto& buy = s.last_buy;
        if (!next.config.buying || !find_buying_card(buy.id) || buy.source > 1 || buy.target > 1
            || !buy.turn || buy.turn > s.turns || buy.population > hand_capacity
            || buy.outcome == BuyOutcome::none || buy.outcome == BuyOutcome::offered
            || static_cast<unsigned>(buy.outcome) > static_cast<unsigned>(BuyOutcome::purchased)) return false;
        if (!buy.population) {
            if (buy.outcome != BuyOutcome::empty || buy.item || buy.price || buy.slot != no_slot) return false;
        } else if (buy.outcome == BuyOutcome::empty || buy.slot >= buy.population || !supported(buy.item)
            || buy.price < 0 || buy.price != trade_price(buy.item)
            || ((buy.source == buy.target) != (buy.outcome == BuyOutcome::self))
            || (buy.outcome == BuyOutcome::insufficient && !buy.price)) return false;
        if (buy.outcome == BuyOutcome::purchased) {
            if (!buy.delivered || buy.money_lost != buy.price || buy.money_gained > buy.price) return false;
        } else if (buy.delivered || buy.money_lost || buy.money_gained) return false;
        if (s.last_sale.id && s.last_sale.turn == buy.turn) return false;
    }
    if (!s.last_sale.id) {
        if (s.last_sale != LastSale{}) return false;
    } else {
        const auto& sale = s.last_sale;
        if (!next.config.selling || !find_selling_card(sale.id) || !supported(sale.item) || sale.source > 1 || sale.target > 1
            || sale.price != trade_price(sale.item) || sale.price < 0 || sale.price > max_sale_price
            || sale.money_lost + sale.mp_lost + sale.hp_lost > sale.price || sale.money_gained > sale.price
            || (sale.source == sale.target && sale.money_gained != sale.price)
            || !sale.turn || sale.turn > s.turns) return false;
    }
    if (!s.last_pestle.id) {
        if (s.last_pestle != LastPestle{}) return false;
    } else {
        const auto& event = s.last_pestle;
        const auto* card = find_pestle_card(event.id);
        if (!next.config.pestle_mortar || !card || event.source > 1 || event.target > 1
            || (event.mortar ? (!find_mortar_card(event.mortar) || event.amount != card->mortar_amount)
                : event.amount != card->card.attack)) return false;
    }
    if (!s.last_self_damage.id) {
        if (s.last_self_damage != LastSelfDamage{}) return false;
    } else {
        const auto& event = s.last_self_damage;
        const auto limit = next.config.magic_free ? magic_free_damage_limit
            : (next.config.doubling_miracles ? doubling_damage_limit : std::int64_t{480});
        if (!next.config.self_damage || !find_self_damage_card(event.id) || event.source > 1 || event.target > 1
            || event.amount < 0 || event.amount > limit || event.source_damage > 99 || event.target_damage > 99
            || event.source_damage > event.amount || event.target_damage > event.amount
            || (event.source == event.target && (event.source_damage + event.target_damage > 99
                || (event.source_damage < event.amount && event.target_damage != 0)))) return false;
    }
    if (!s.last_hp_absorption.id) {
        if (s.last_hp_absorption != LastHPAbsorption{}) return false;
    } else {
        const auto& event = s.last_hp_absorption;
        const auto* weapon = find_hp_absorption_card(event.id);
        const auto* magic = find_hp_absorption_miracle(event.id);
        const auto limit = magic ? std::int64_t{magic->amount} : (weapon && weapon->hit_rate ? std::int64_t{weapon->card.attack}
            : (next.config.magic_free ? magic_free_damage_limit : (next.config.doubling_miracles ? doubling_damage_limit : std::int64_t{480})));
        if (!next.config.hp_absorption || (!weapon && !magic) || event.source > 1 || event.target > 1
            || event.amount < 0 || event.amount > limit || event.healing > 99 || event.healing > event.amount) return false;
    }
    if (!s.last_item_removal.id) {
        if (s.last_item_removal != LastItemRemoval{}) return false;
    } else {
        const auto& event = s.last_item_removal;
        if (!next.config.item_removal || !find_item_removal_card(event.id) || event.source > 1 || event.target > 1
            || event.population > hand_capacity || ((event.population == 0) != (event.removed == 0))
            || (event.population ? (event.slot >= event.population || !supported(event.removed)) : event.slot != no_slot)) return false;
    }
    const bool death_tracking = next.config.counters || next.config.revival;
    if (!s.last_revival.id) {
        if (s.last_revival != LastRevival{}) return false;
    } else if (!next.config.revival || !find_revival_card(s.last_revival.id) || s.last_revival.seat > 1) return false;
    if (!s.last_exchange.id) {
        if (s.last_exchange != LastExchange{}) return false;
    } else {
        if (!next.config.exchanges || !find_exchange_card(s.last_exchange.id) || s.last_exchange.seat > 1 || !s.last_exchange.before[0]) return false;
        int before = 0, after = 0;
        for (int i = 0; i < 3; ++i) {
            if (s.last_exchange.before[i] > 99 || s.last_exchange.after[i] > 99) return false;
            before += s.last_exchange.before[i];
            after += s.last_exchange.after[i];
        }
        if (before != after) return false;
    }
    if (!s.last_forget.id) {
        if (s.last_forget != LastForget{}) return false;
    } else {
        if (!next.config.forgetting || !find_forgetting_card(s.last_forget.id)
            || s.last_forget.source > 1 || s.last_forget.target > 1
            || s.last_forget.population > 16ULL * next.config.max_decisions
            || ((s.last_forget.population == 0) != (s.last_forget.removed == 0))) return false;
        if (s.last_forget.removed && (!supported(s.last_forget.removed)
            || std::find(status_retained_type_ids.begin(), status_retained_type_ids.end(), s.last_forget.removed) == status_retained_type_ids.end())) return false;
    }
    if (s.last_tick.seat == no_slot) {
        if (s.last_tick != State::LastDiseaseTick{}) return false;
    } else if (!next.config.disease || s.last_tick.seat > 1
               || s.last_tick.before < Disease::cold || s.last_tick.before > Disease::heaven
               || s.last_tick.after < Disease::cold || s.last_tick.after > Disease::heaven
               || s.last_tick.damage < 0 || s.last_tick.damage > 99
               || s.last_tick.healing < 0 || s.last_tick.healing > 99
               || (s.last_tick.damage && s.last_tick.healing)) return false;
    if (s.last_tick.seat != no_slot) {
        const auto& tick = s.last_tick;
        const auto expected = tick.worsened
            ? static_cast<Disease>(std::min(static_cast<int>(tick.before) + 1, static_cast<int>(Disease::heaven)))
            : tick.before;
        if (tick.after != expected) return false;
        if (tick.before == Disease::heaven && tick.worsened) {
            if (tick.damage < 1 || tick.healing != 0) return false;
        } else if (tick.after == Disease::heaven) {
            if (tick.damage != 0 || tick.healing > 5) return false;
        } else {
            const int maximum = tick.after == Disease::cold ? 1 : (tick.after == Disease::fever ? 2 : 5);
            if (tick.damage < 1 || tick.damage > maximum || tick.healing != 0) return false;
        }
    }
    if (static_cast<unsigned>(s.pending.disease) > static_cast<unsigned>(Disease::heaven)
        || static_cast<unsigned>(s.pending.cure) > static_cast<unsigned>(Cure::all)
        || static_cast<unsigned>(s.pending.harm) > static_cast<unsigned>(Harm::dark_cloud)) return false;
    const bool forget_payload = s.pending.kind == Effect::forget;
    const bool removal_payload = s.pending.kind == Effect::remove_item;
    const bool sale_payload = s.pending.kind == Effect::sell;
    const bool buy_payload = s.pending.kind == Effect::buy;
    const bool gamble_payload = s.pending.kind == Effect::hp_gamble;
    const bool untyped_payload = forget_payload || removal_payload || sale_payload || buy_payload || gamble_payload;
    if ((forget_payload ? (!next.config.forgetting || !find_forgetting_card(s.pending.forget_card)) : s.pending.forget_card != 0)
        || (removal_payload ? (!next.config.item_removal || !find_item_removal_card(s.pending.item_removal_card)) : s.pending.item_removal_card != 0)
        || (sale_payload ? (!next.config.selling || !find_selling_card(s.pending.sale_card) || !supported(s.pending.sale_item)
            || trade_price(s.pending.sale_item) < 0 || s.pending.amount != trade_price(s.pending.sale_item))
            : (s.pending.sale_card != 0 || s.pending.sale_item != 0))
        || (buy_payload ? (!next.config.buying || !find_buying_card(s.pending.buy_card) || s.pending.amount) : s.pending.buy_card != 0)
        || (gamble_payload ? (!next.config.hp_gamble || !find_hp_gamble_card(s.pending.hp_gamble_card)
            || (s.pending.amount != find_hp_gamble_card(s.pending.hp_gamble_card)->amount
                && s.pending.amount != -find_hp_gamble_card(s.pending.hp_gamble_card)->amount)) : s.pending.hp_gamble_card != 0)) return false;
    if (untyped_payload) {
        if (s.phase != Phase::response || s.pending.element != Element::none || (!sale_payload && !gamble_payload && s.pending.amount) || s.pending.miracle || s.pending.from_ability
            || s.pending.hit_rate || s.pending.status_card || s.pending.disease != Disease::none || s.pending.cure != Cure::none
            || s.pending.harm_card || s.pending.harm != Harm::none || s.pending.attribute_card || s.pending.attribute_removed
            || s.pending.counter_card || s.pending.mana_weapon || s.pending.mana_spent || s.pending.wide_miracle || s.pending.free_card
            || s.attack_composition.base || s.free_cast.magic || s.pending.hp_absorption_card || s.pending.self_damage_card || s.pending.pestle_card || s.pending.mortar_card) return false;
    }
    if (sale_payload) {
        if (s.turn_owner > 1) return false;
        const auto& owner = s.players[s.turn_owner];
        const auto& other = s.players[1 - s.turn_owner];
        const std::uint64_t reactions = s.reaction_count;
        if (owner.pending_deals != 1 + reactions / 2 || other.pending_deals != (reactions + 1) / 2
            || owner.retained_deals || other.retained_deals || owner.hand_count + owner.pending_deals > hand_capacity - 1) return false;
    }
    if (buy_payload || gamble_payload) {
        if (s.turn_owner > 1 || s.players[0].hp <= 0 || s.players[1].hp <= 0) return false;
        const auto& owner = s.players[s.turn_owner];
        const auto& other = s.players[1 - s.turn_owner];
        const std::uint64_t reactions = s.reaction_count;
        if (owner.pending_deals != 1 + reactions / 2 || other.pending_deals != (reactions + 1) / 2
            || owner.retained_deals || other.retained_deals || owner.hand_count + owner.pending_deals > hand_capacity
            || other.hand_count + other.pending_deals > hand_capacity) return false;
    }
    if (gamble_payload) {
        const auto& event = s.last_hp_gamble;
        if (event.id != s.pending.hp_gamble_card || event.resolved || event.source != s.turn_owner
            || event.target != 1 - s.turn_owner || event.delta != s.pending.amount
            || std::uint64_t{s.decisions} < std::uint64_t{s.reaction_count} + 1) return false;
    }
    if (s.pending.hp_absorption_card) {
        const auto id = s.pending.hp_absorption_card;
        const auto* weapon = find_hp_absorption_card(id);
        const auto* magic = find_hp_absorption_miracle(id);
        if (!next.config.hp_absorption || (!weapon && !magic) || s.phase != Phase::response
            || s.pending.kind != Effect::damage || s.pending.counter_card || s.pending.mana_weapon || s.pending.mana_spent || s.pending.self_damage_card
            || s.pending.status_card || s.pending.harm_card || s.pending.harm != Harm::none || s.pending.disease != Disease::none
            || s.pending.cure != Cure::none || s.pending.element == Element::dark) return false;
        if (magic) {
            if (s.pending.miracle != id) return false;
        } else {
            if (s.pending.miracle || s.pending.from_ability || (s.attack_composition.base && s.attack_composition.base != id)) return false;
            if (!s.attack_composition.base && (s.pending.amount < weapon->card.attack
                || s.pending.amount > weapon->card.attack + (weapon->hit_rate ? 0 : 225)
                || s.pending.hit_rate != weapon->hit_rate
                || (s.pending.element != (s.pending.attribute_removed ? Element::neutral : weapon->element)
                    && (!s.pending.attribute_card || s.pending.element != element_of(s.pending.attribute_card))))) return false;
        }
    } else if ((s.pending.miracle && find_hp_absorption_miracle(s.pending.miracle))
        || (s.attack_composition.base && find_hp_absorption_card(s.attack_composition.base))
        || (s.pending.hit_rate && find_hp_absorption_card(s.last_chance.id))) return false;
    if (s.pending.self_damage_card) {
        const auto* card = find_self_damage_card(s.pending.self_damage_card);
        if (!next.config.self_damage || !card || s.phase != Phase::response || s.pending.kind != Effect::damage
            || s.pending.counter_card || s.pending.miracle || s.pending.from_ability || s.pending.mana_weapon || s.pending.mana_spent
            || s.pending.hp_absorption_card || s.pending.status_card || s.pending.disease != Disease::none || s.pending.cure != Cure::none
            || s.pending.harm_card || s.pending.harm != Harm::none || s.pending.element == Element::dark
            || (s.attack_composition.base && s.attack_composition.base != card->card.id)) return false;
        if (!s.attack_composition.base && (s.pending.amount < card->card.attack || s.pending.amount > card->card.attack + 225
            || s.pending.hit_rate || (s.pending.element != Element::neutral
                && (!s.pending.attribute_card || s.pending.element != element_of(s.pending.attribute_card))))) return false;
    } else if (s.attack_composition.base && find_self_damage_card(s.attack_composition.base)) return false;
    if (s.pending.pestle_card) {
        const auto& event = s.last_pestle;
        if (!next.config.pestle_mortar || !find_pestle_card(s.pending.pestle_card) || s.phase != Phase::response
            || s.pending.kind != Effect::damage || s.pending.miracle || s.pending.from_ability || s.pending.counter_card
            || s.pending.hp_absorption_card || s.pending.self_damage_card || s.pending.mana_weapon || s.pending.mana_spent
            || s.pending.status_card || s.pending.disease != Disease::none || s.pending.cure != Cure::none
            || s.pending.harm_card || s.pending.harm != Harm::none || s.pending.hit_rate || s.pending.attribute_card
            || s.pending.wide_miracle || s.pending.free_card || s.attack_composition.base || s.free_cast.magic
            || event.id != s.pending.pestle_card || event.source != s.turn_owner || event.target == event.source
            || s.pending.amount != event.amount || s.pending.mortar_card != event.mortar
            || s.pending.element != (s.pending.attribute_removed ? Element::neutral : Element::light)) return false;
        std::uint8_t first_mortar = no_slot;
        for (int seat = 0; seat < 2; ++seat) if (!s.players[seat].dead) {
            const auto& player = s.players[seat];
            if (std::any_of(player.hand.begin(), player.hand.begin() + player.hand_count,
                [](CardId id) { return find_mortar_card(id) != nullptr; })) { first_mortar = static_cast<std::uint8_t>(seat); break; }
        }
        if (event.mortar ? first_mortar != event.target : first_mortar != no_slot) return false;
    } else if (s.pending.mortar_card) return false;
    if (s.pending.free_card && (!next.config.magic_free || !find_magic_free_card(s.pending.free_card)
        || !s.pending.miracle || !s.free_cast.magic || s.phase != Phase::response)) return false;
    if (s.pending.wide_miracle && (!next.config.wide_miracles
        || s.pending.wide_miracle != wide_miracles()[0].id || s.phase != Phase::response
        || s.pending.kind != Effect::damage || s.pending.miracle || s.pending.counter_card
        || !s.attack_composition.base || s.pending.hit_rate != 100)) return false;
    if (s.pending.mana_weapon) {
        // Ordered attacks below replay every paid addition and the final burn.
        // Older attacks spend all MP once, then add at most15 ordinary boosts.
        // MP counter193 starts only after this original item's marker clears.
        if (!next.config.mana_weapon || !find_mana_weapon_card(s.pending.mana_weapon)
            || s.phase != Phase::response || s.turn_owner > 1 || s.players[s.turn_owner].mp != 0
            || s.pending.kind != Effect::damage || s.pending.mana_spent > 99
            || (!s.attack_composition.base && (s.pending.amount < 2 * s.pending.mana_spent || s.pending.amount > 2 * s.pending.mana_spent + 225))
            || (!s.attack_composition.base && !s.pending.attribute_card && s.pending.element != Element::neutral)
            || s.pending.miracle || s.pending.from_ability || s.pending.counter_card || (s.pending.hit_rate && !s.pending.wide_miracle)
            || s.pending.status_card || s.pending.disease != Disease::none || s.pending.cure != Cure::none
            || s.pending.harm_card || s.pending.harm != Harm::none) return false;
    } else if (s.pending.mana_spent) return false;
    if (s.pending.attribute_removed && (!next.config.attribute_removal || s.pending.element != Element::neutral
        || s.phase != Phase::response || !s.reaction_count)) return false;
    if (s.pending.attribute_card) {
        const auto* setter = find_attribute_change_card(s.pending.attribute_card);
        if (!next.config.attribute_change || !setter || s.pending.kind != Effect::damage
            || s.pending.miracle || s.pending.from_ability || (s.pending.hit_rate && !s.pending.wide_miracle)
            || (!s.attack_composition.base && s.pending.element != Element::neutral && s.pending.element != setter->element)) return false;
    }
    if (s.pending.harm_card) {
        const auto* c = find_harm_card(s.pending.harm_card);
        const auto* celestial = next.config.celestial ? find_celestial_card(s.pending.harm_card) : nullptr;
        const auto* source = c ? &c->card : (celestial ? &celestial->card : nullptr);
        const auto harm = c ? c->harm : (celestial ? celestial->harm : Harm::none);
        const auto element = c ? c->element : (celestial ? celestial->element : Element::neutral);
        if (!next.config.harms || !source || harm == Harm::none || (c && c->on_defense) || s.pending.harm != harm
            || s.pending.status_card || s.pending.miracle || s.pending.from_ability || (s.pending.hit_rate && !s.pending.wide_miracle)
            || s.pending.kind != Effect::damage || s.pending.amount < source->attack
            || (!s.attack_composition.base && !s.pending.attribute_card && element != Element::light
                && s.pending.element != Element::neutral && s.pending.element != element)) return false;
    } else if (s.pending.harm != Harm::none) {
        const auto* m = next.config.offensive_miracles ? find_offensive_miracle(s.pending.miracle) : nullptr;
        const auto c = counter_info(s.pending.counter_card);
        if ((!m || s.pending.harm != m->harm)
            && !(status_payload && s.pending.harm == status_magic->harm)
            && !(next.config.counter_extensions && c.id && c.effect == Effect::harm && s.pending.harm == c.harm)) return false;
    }
    if (s.pending.status_card) {
        const auto* c = find_disease_card(s.pending.status_card);
        if (!next.config.disease || !c || c->on_defense || s.pending.miracle || s.pending.from_ability
            || (s.pending.hit_rate && !s.pending.wide_miracle) || s.pending.disease != c->disease || s.pending.cure != c->cure) return false;
        if (c->cure != Cure::none) {
            if (s.pending.kind != Effect::cure || s.pending.amount != 0) return false;
        } else if (c->card.kind == Kind::mana) {
            if (s.pending.kind != Effect::mana || s.pending.amount != c->card.mp) return false;
        } else if (c->card.kind == Kind::weapon) {
            if (s.pending.kind != Effect::damage || s.pending.amount < c->card.attack
                || (!s.attack_composition.base && !s.pending.attribute_card && s.pending.element != Element::neutral && s.pending.element != c->element)) return false;
        } else return false;
    } else if (!status_payload && (s.pending.disease != Disease::none || s.pending.cure != Cure::none || s.pending.kind == Effect::cure)) return false;
    if (s.last_reaction.id == 0) {
        if (s.last_reaction != State::LastReaction{}) return false;
    } else {
        const auto* c = find_reaction_card(s.last_reaction.id);
        const auto* m = find_defensive_miracle(s.last_reaction.id);
        if (s.last_reaction.source > 1 || s.last_reaction.target > 1) return false;
        if (c) {
            if (!next.config.reflection || c->reaction != s.last_reaction.reaction || s.last_reaction.from_ability) return false;
        } else if (find_any_reflection_card(s.last_reaction.id)) {
            if (!next.config.any_reflection || s.last_reaction.reaction != WeaponReaction::reflect
                || s.last_reaction.from_ability) return false;
        } else if (find_magic_reflection_card(s.last_reaction.id)) {
            if (!next.config.magic_reflection || s.last_reaction.reaction != WeaponReaction::reflect
                || s.last_reaction.from_ability) return false;
        } else if (find_magic_bounce_card(s.last_reaction.id)) {
            if (!next.config.magic_bounce || s.last_reaction.reaction != WeaponReaction::bounce
                || s.last_reaction.from_ability) return false;
        } else if (find_magic_block_card(s.last_reaction.id)) {
            if (!next.config.magic_block || s.last_reaction.reaction != WeaponReaction::block
                || s.last_reaction.from_ability || s.last_reaction.source != s.last_reaction.target) return false;
        } else if (m) {
            const auto expected = m->kind == DefensiveMiracleKind::bounce_magic ? WeaponReaction::bounce : WeaponReaction::block;
            if (!next.config.defensive_miracles || s.last_reaction.reaction != expected
                || ((!next.config.forgetting || (s.phase == Phase::response && s.reaction_count))
                    && !s.players[s.last_reaction.source].defensive_abilities[defensive_miracle_index(m->id)])
                || (expected == WeaponReaction::block && s.last_reaction.source != s.last_reaction.target)) return false;
        } else return false;
        if (s.last_reaction.reaction == WeaponReaction::reflect && s.last_reaction.source == s.last_reaction.target) return false;
    }
    if (s.last_reaction.free_card && (!next.config.magic_free || !find_magic_free_card(s.last_reaction.free_card)
        || !find_defensive_miracle(s.last_reaction.id))) return false;
    if (!next.config.reflection && s.reaction_count != 0) return false;
    if (s.last_chance.id == 0) {
        if (s.last_chance != LastChance{}) return false;
    } else {
        const auto rate = chance_rate(s.last_chance.id);
        if (!next.config.chance || !supported(s.last_chance.id) || s.last_chance.source > 1 || s.last_chance.target > 1
            || s.last_chance.target == s.last_chance.source
            || (s.last_chance.forced && (!next.config.harms || !s.last_chance.hit))) return false;
        if (s.last_chance.wide_miracle) {
            const auto* base = artifact(s.last_chance.id);
            if (!next.config.wide_miracles || s.last_chance.wide_miracle != wide_miracles()[0].id
                || !base || base->kind != Kind::weapon || rate || s.last_chance.hit_rate != 100
                || !s.last_chance.hit) return false;
        } else if (!rate || find_wide_miracle(s.last_chance.id) || s.last_chance.hit_rate != rate) return false;
    }
    if ((!untyped_payload && static_cast<unsigned>(s.pending.element) > static_cast<unsigned>(next.config.celestial ? Element::dark : Element::stone))
        || (!untyped_payload && !next.config.elements && s.pending.element != Element::neutral)
        || (!untyped_payload && s.pending.kind != Effect::damage && s.pending.element != Element::neutral
            && !(next.config.counter_extensions && s.pending.counter_card && s.pending.kind == Effect::harm)
            && !status_payload)) return false;
    if ((!next.config.chance && s.pending.hit_rate != 0)
        || (s.pending.kind != Effect::damage && s.pending.hit_rate != 0)) return false;
    if (s.actor > 1 || s.turn_owner > 1 || s.order_index > 1 || s.order[0] > 1 || s.order[1] > 1
        || s.order[0] == s.order[1] || s.turn_owner != s.order[s.order_index]
        || s.decisions > next.config.max_decisions || s.turns > s.decisions
        || (s.terminated && s.truncated)) return false;
    for (int seat = 0; seat < 2; ++seat) {
        const auto& p = s.players[seat];
        const bool received_before_death = next.config.selling && s.terminated && p.hp == 0
            && s.phase == Phase::main && s.last_sale.id && s.last_sale.target == seat && s.last_sale.delivered
            && s.last_sale.turn == s.turns;
        if (p.incoming_sale != static_cast<std::uint8_t>(received_before_death)) return false;
        const bool bought_before_death = next.config.buying && s.terminated && p.hp == 0 && s.phase == Phase::main
            && s.last_buy.id && s.last_buy.outcome == BuyOutcome::purchased && s.last_buy.source == seat
            && s.last_buy.delivered && s.last_buy.turn == s.turns;
        if (p.incoming_buy != static_cast<std::uint8_t>(bought_before_death) || (p.incoming_buy && p.incoming_sale)) return false;
        if (p.hp < 0 || p.hp > 99 || p.mp < 0 || p.mp > 99 || p.money < 0 || p.money > 99
            || p.hand_count > hand_capacity) return false;
        if ((p.dead && (!death_tracking || p.hp != 0))
            || (death_tracking && s.terminated && p.hp == 0 && !p.dead)) return false;
        if (tracks_retained_deals(next.config)) {
            // Additive casts cost at least2MP: at most49 held uses at99MP.
            // Older response spells cost at least5MP, plus one main-turn cast.
            if (p.retained_deals > (next.config.magic_free ? std::uint64_t{s.decisions} + 49 : (next.config.additive_miracles ? 49 : (next.config.defensive_miracles ? 20 : 1)))
                || p.retained_deals > p.pending_deals || (!next.config.additive_miracles && p.retained_deals > s.decisions)
                || p.pending_deals > hand_capacity + p.retained_deals
                || p.hand_count + p.pending_deals - p.retained_deals > hand_capacity + p.incoming_sale + p.incoming_buy) return false;
        } else if (p.pending_deals > hand_capacity || p.retained_deals) return false;
        if (next.config.counters && p.hand_count + p.pending_deals - p.retained_deals > hand_capacity + p.incoming_sale + p.incoming_buy) return false;
        if (p.disease > Disease::heaven || p.worse_chance > 100
            || (p.disease == Disease::none && p.worse_chance != 0)
            || (!next.config.disease && (p.disease != Disease::none || p.worse_chance != 0))) return false;
        if (!next.config.harms && (p.glory || p.dark_cloud)) return false;
        for (int i = 0; i < hand_capacity; ++i)
            if (i < p.hand_count ? !supported(p.hand[i]) : p.hand[i] != 0) return false;
        std::uint64_t acquired = 0;
        for (auto count : p.abilities) acquired += count;
        for (auto count : p.status_abilities) {
            if (!next.config.status_miracles && count) return false;
            acquired += count;
        }
        for (auto count : p.defensive_abilities) {
            if (!next.config.defensive_miracles && count) return false;
            acquired += count;
        }
        const auto additive_cap = next.config.additive_miracles ? 15ULL * next.config.max_decisions : 0;
        if (p.additive_abilities[0] > additive_cap || p.additive_abilities[1] > additive_cap - p.additive_abilities[0]
            || (!next.config.doubling_miracles && p.doubling_ability)
            || p.doubling_ability > additive_cap - p.additive_abilities[0] - p.additive_abilities[1]
            || (!next.config.wide_miracles && p.wide_ability)
            || p.wide_ability > additive_cap - p.additive_abilities[0] - p.additive_abilities[1] - p.doubling_ability) return false;
        if ((!next.config.miracles && acquired != 0) || acquired > next.config.max_decisions) return false;
        if (!next.config.offensive_miracles)
            for (int i = miracle_count; i < total_miracle_count; ++i) if (p.abilities[i]) return false;
        if (!next.config.hp_absorption && p.abilities[hp_absorption_ability_index]) return false;
    }
    if (next.config.magic_free && s.players[0].retained_deals + s.players[1].retained_deals > std::uint64_t{s.decisions} + 49) return false;
    if (s.terminated) {
        if (s.phase != Phase::main) return false;
        if (s.players[0].hp == 0 && s.players[1].hp == 0) {
            if ((!next.config.disease && !next.config.counters && !next.config.self_damage) || s.winner != -1) return false;
        } else if (s.winner < 0 || s.winner > 1 || s.players[s.winner].hp == 0
                   || s.players[1 - s.winner].hp != 0) return false;
    } else {
        if (s.winner != -1) return false;
        for (const auto& p : s.players) if (p.hp == 0) {
            if (death_tracking && p.dead) continue;
            if (!next.config.disease || s.phase != Phase::response || !s.reaction_count
                || p.disease != Disease::heaven || p.worse_chance != 0) return false;
        }
    }
    if (s.counter_count > counter_queue_capacity || s.counter_generated > counter_queue_capacity
        || s.counter_started > s.counter_generated
        || s.counter_generated != s.counter_started + s.counter_count) return false;
    const auto valid_counter = [&](const Pending& item, bool queued) noexcept {
        const auto c = counter_info(item.counter_card);
        if (!next.config.counters || !c.id
            || (find_counter_extension_card(c.id) && !next.config.counter_extensions)
            || (find_money_counter_card(c.id) && !next.config.money_counter)
            || item.kind != c.effect || item.source > 1 || item.target > 1 || item.origin_source > 1
            || (c.target == CounterExtensionTarget::self ? item.source != item.target : item.source == item.target)
            || item.element != c.element || item.hit_rate != c.hit_rate || item.harm != c.harm
            || item.miracle || item.from_ability
            || item.status_card || item.disease != Disease::none || item.cure != Cure::none
            || item.harm_card || item.attribute_card || item.attribute_removed || item.mana_weapon || item.mana_spent || item.wide_miracle || item.free_card || item.forget_card || item.item_removal_card || item.hp_absorption_card || item.self_damage_card || item.pestle_card || item.mortar_card || item.sale_card || item.sale_item || item.buy_card || item.hp_gamble_card) return false;
        if (c.effect == Effect::harm) {
            if (item.amount != 0) return false;
        } else if (item.amount <= 0 || !c.multiplier || item.amount % c.multiplier != 0
                   || item.amount > ((next.config.magic_free ? magic_free_damage_limit : (next.config.doubling_miracles ? doubling_damage_limit : std::int64_t{next.config.pestle_mortar ? 999 : 480})) << s.counter_generated)) return false;
        if (queued) {
            if (item.source != item.origin_source) return false;
        } else {
            if (c.target == CounterExtensionTarget::self) return false;
            if (item.source != (item.origin_source ^ (s.reaction_count & 1u))) return false;
            if (s.reaction_count && (!next.config.any_reflection || s.reaction_count >= s.decisions
                || !find_any_reflection_card(s.last_reaction.id)
                || s.last_reaction.reaction != WeaponReaction::reflect || s.last_reaction.from_ability
                || s.last_reaction.source != item.source || s.last_reaction.target != item.target)) return false;
        }
        if (!queued && c.hit_rate && (s.last_chance.id != c.id || !s.last_chance.hit
            || s.last_chance.source != item.origin_source || s.last_chance.target != 1 - item.origin_source
            || s.last_chance.hit_rate != c.hit_rate)) return false;
        return true;
    };
    std::array<unsigned, 2> pending_counters{};
    for (int i = 0; i < counter_queue_capacity; ++i) {
        if (i < s.counter_count) {
            if (!valid_counter(s.counter_queue[i], true)) return false;
            ++pending_counters[s.counter_queue[i].origin_source];
        } else if (s.counter_queue[i] != Pending{}) return false;
    }
    if (death_tracking) {
        if (!s.pending.counter_card && s.phase == Phase::response
            && (s.players[0].dead || s.players[1].dead)) return false;
        if (s.phase == Phase::main && !s.terminated && (s.players[0].dead || s.players[1].dead)) return false;
    }
    if (!next.config.counters) {
        if (s.counter_count || s.counter_generated || s.counter_started || s.pending.counter_card
            || s.pending.origin_source != no_slot) return false;
    } else {
        if (s.pending.counter_card) {
            if (s.phase != Phase::response || !s.counter_started || !valid_counter(s.pending, false)) return false;
            ++pending_counters[s.pending.origin_source];
        } else if (s.counter_generated || (s.pending.kind != Effect::none && s.pending.origin_source != s.turn_owner)) return false;
        unsigned physical = 0;
        for (int seat = 0; seat < 2; ++seat) {
            const auto spent = static_cast<unsigned>(s.players[seat].pending_deals - s.players[seat].retained_deals);
            physical += spent;
            unsigned reflected = 0;
            if (s.pending.counter_card && s.reaction_count)
                reflected = s.reaction_count / 2 + ((s.reaction_count & 1u) && seat != s.pending.origin_source);
            if (pending_counters[seat] + reflected > spent) return false;
        }
        if (std::uint64_t{s.counter_generated} + (s.pending.counter_card ? s.reaction_count : 0) > physical) return false;
    }
    if (s.truncated ? s.decisions != next.config.max_decisions
                    : (!s.terminated && s.decisions >= next.config.max_decisions)) return false;
    if (s.phase == Phase::main) {
        if (s.actor != s.turn_owner || s.pending != Pending{} || s.reaction_count != 0) return false;
        for (const auto& p : s.players)
            if (p.hp > 0 && (p.pending_deals != 0 || p.retained_deals != 0)) return false;
    } else if (s.phase == Phase::response) {
        if (s.pending.source > 1 || s.pending.target != s.actor || s.pending.source == s.actor
            || (!gamble_payload && ((s.pending.kind == Effect::cure || s.pending.kind == Effect::harm || status_payload || (untyped_payload && !sale_payload))
                ? s.pending.amount != 0 : (s.pending.amount < 0 || (!s.pending.amount && !s.pending.mana_weapon && !sale_payload))))) return false;
        if (death_tracking && s.players[s.actor].dead) return false;
        if (s.pending.counter_card) {
            if (!valid_counter(s.pending, false)) return false;
        } else {
        const auto& source = s.players[s.pending.source];
        const auto& original = s.players[s.turn_owner];
        if (next.config.magic_free && s.pending.miracle
            && (original.retained_deals > (s.pending.from_ability ? 1ULL : 0ULL) + s.reaction_count / 2
                || s.players[1 - s.turn_owner].retained_deals > (std::uint64_t{s.reaction_count} + 1) / 2)) return false;
        if (s.pending.attribute_card && original.pending_deals < 2) return false;
        if (s.reaction_count) {
            if (!next.config.reflection || s.reaction_count >= s.decisions || !s.last_reaction.id
                || s.pending.source != s.last_reaction.source || s.pending.target != s.last_reaction.target
                || s.pending.source != (s.turn_owner ^ (s.reaction_count & 1u))) return false;
            if (next.config.defensive_miracles && find_defensive_miracle(s.last_reaction.id)) {
                if (s.last_reaction.reaction != WeaponReaction::bounce || !s.pending.miracle
                    || (s.last_reaction.from_ability && source.retained_deals == 0)) return false;
                if (s.last_reaction.free_card && std::find(source.hand.begin(), source.hand.begin() + source.hand_count,
                    s.last_reaction.free_card) == source.hand.begin() + source.hand_count) return false;
            } else if (find_magic_bounce_card(s.last_reaction.id)) {
                if (!next.config.magic_bounce || !is_magic_item(s.pending)
                    || s.last_reaction.reaction != WeaponReaction::bounce || s.last_reaction.from_ability) return false;
            } else if (find_any_reflection_card(s.last_reaction.id)) {
                if (!next.config.any_reflection || s.last_reaction.reaction != WeaponReaction::reflect
                    || s.last_reaction.from_ability) return false;
            } else if (find_magic_reflection_card(s.last_reaction.id)) {
                if (!next.config.magic_reflection || !is_magic_item(s.pending)
                    || s.last_reaction.reaction != WeaponReaction::reflect || s.last_reaction.from_ability) return false;
            } else if (s.pending.kind != Effect::damage || s.pending.element != Element::neutral
                       || s.pending.miracle || s.pending.from_ability
                       || (s.pending.hit_rate && !s.pending.attribute_removed && !s.pending.wide_miracle)) return false;
            std::uint64_t spent = 0;
            for (const auto& p : s.players) {
                if (p.pending_deals == 0
                    || (!tracks_retained_deals(next.config) && p.hand_count + p.pending_deals > hand_capacity)) return false;
                spent += p.pending_deals;
            }
            if (s.reaction_count >= spent) return false;
            if (s.pending.attribute_removed && spent < std::uint64_t{s.reaction_count} + 2) return false;
        } else if (s.pending.source != s.turn_owner || source.pending_deals == 0
                   || s.players[s.pending.target].pending_deals != 0) return false;
        if (tracks_retained_deals(next.config) && !s.reaction_count && !s.attack_composition.base
            && original.retained_deals != (s.pending.from_ability ? 1 : 0)) return false;
        if (s.pending.from_ability) {
            if (!s.pending.miracle || (tracks_retained_deals(next.config)
                ? original.retained_deals == 0 : source.pending_deals != 1)) return false;
        } else if (!tracks_retained_deals(next.config) && source.hand_count + source.pending_deals > hand_capacity) return false;
        if (s.pending.miracle) {
            const auto m = spell(s.pending.miracle);
            const auto index = spell_index(m.id);
            const auto ai = additive_miracle_index(m.id);
            const auto si = status_miracle_index(m.id);
            const bool owned = si >= 0 ? original.status_abilities[si] != 0
                : (ai >= 0 ? original.additive_abilities[ai] != 0 : (index >= 0 && original.abilities[index] != 0));
            if (!next.config.miracles || !m.id || !supported(m.id) || !owned
                || ((!tracks_retained_deals(next.config) || !s.reaction_count) && !s.free_cast.magic && original.pending_deals != 1)
                || s.pending.amount != m.amount || s.pending.kind != m.effect
                || s.pending.element != (s.pending.attribute_removed ? Element::neutral : m.element)
                || s.pending.hit_rate != m.hit_rate || s.pending.harm != m.harm
                || s.pending.disease != m.disease || s.pending.cure != m.cure
                || s.pending.harm_card || s.pending.status_card) return false;
        } else if (s.pending.from_ability || s.pending.kind == Effect::money) return false;
        if (s.pending.wide_miracle) {
            if (!s.last_chance.hit || s.last_chance.id != s.attack_composition.base
                || s.last_chance.wide_miracle != s.pending.wide_miracle || s.last_chance.hit_rate != 100
                || s.last_chance.source != s.turn_owner || s.last_chance.target != 1 - s.turn_owner) return false;
        } else if (s.pending.hit_rate) {
            if (s.last_chance.wide_miracle || !s.last_chance.hit || s.pending.hit_rate != chance_rate(s.last_chance.id)
                || s.last_chance.source != s.turn_owner || s.last_chance.target != 1 - s.turn_owner
                || (!s.reaction_count && !s.free_cast.magic && original.pending_deals != 1)) return false;
            if (s.pending.miracle) {
                if (s.pending.miracle != s.last_chance.id) return false;
            } else {
                const auto* c = artifact(s.last_chance.id);
                if (!c || s.pending.from_ability || s.pending.amount != c->attack
                    || s.pending.element != (s.pending.attribute_removed ? Element::neutral : element_of(c->id))) return false;
            }
        }
        if (s.pending.kind == Effect::damage) { if (s.pending.amount > (s.attack_composition.base ? (next.config.magic_free ? magic_free_damage_limit : (next.config.doubling_miracles ? doubling_damage_limit : std::int64_t{480})) : std::int64_t{s.pending.pestle_card ? 999 : 480})) return false; }
        else if (s.pending.kind == Effect::heal || s.pending.kind == Effect::mana || s.pending.kind == Effect::money) {
            if (s.pending.amount > 20) return false;
        } else if (s.pending.kind != Effect::cure && !untyped_payload && !status_payload) return false;
        }
    } else return false;
    if (s.attack_composition.base) {
        CompositionPlan plan;
        const auto& record = s.attack_composition;
        if (s.phase != Phase::response || s.turn_owner > 1 || s.pending.counter_card || s.pending.miracle
            || !replay_composition(record, plan)) return false;
        const auto& owner = s.players[s.turn_owner];
        if (owner.additive_abilities != plan.acquired || owner.doubling_ability != plan.acquired_doubling
            || owner.wide_ability != plan.acquired_wide
            || owner.mp != (find_mana_weapon_card(record.base) ? 0 : record.initial_mp - plan.cost)
            || owner.retained_deals != plan.held || s.players[1 - s.turn_owner].retained_deals
            || owner.pending_deals - owner.retained_deals < plan.physical + s.reaction_count / 2
            || s.players[1 - s.turn_owner].pending_deals < (s.reaction_count + 1) / 2
            || (!s.reaction_count && (owner.pending_deals != plan.physical + plan.held || owner.retained_deals != plan.held))
            || s.pending.kind != Effect::damage || s.pending.amount != plan.amount
            || s.pending.hit_rate != (plan.wide ? 100 : 0)
            || s.pending.wide_miracle != (plan.wide ? wide_miracles()[0].id : 0)
            || s.pending.from_ability || s.pending.attribute_card != plan.setter
            || s.pending.element != (s.pending.attribute_removed ? Element::neutral : plan.element)
            || s.pending.mana_weapon != (find_mana_weapon_card(record.base) ? record.base : 0)
            || s.pending.mana_spent != plan.mana_spent) return false;
        const auto physical_spent = s.players[0].pending_deals - s.players[0].retained_deals
            + s.players[1].pending_deals - s.players[1].retained_deals;
        if (physical_spent < static_cast<std::uint64_t>(plan.physical) + s.reaction_count + (s.pending.attribute_removed ? 1u : 0u)) return false;
        const auto* disease = find_disease_card(record.base);
        const auto* harm = find_harm_card(record.base);
        const auto* celestial = find_celestial_card(record.base);
        const auto harm_id = harm ? harm->card.id : (celestial && celestial->harm != Harm::none ? celestial->card.id : 0);
        const auto harm_kind = harm ? harm->harm : (celestial ? celestial->harm : Harm::none);
        if (s.pending.status_card != (disease ? record.base : 0)
            || s.pending.disease != (disease ? disease->disease : Disease::none)
            || s.pending.cure != (disease ? disease->cure : Cure::none)
            || s.pending.harm_card != harm_id || s.pending.harm != harm_kind) return false;
    } else if (s.attack_composition != AttackComposition{}) return false;
    if (s.free_cast.magic) {
        const auto& record = s.free_cast;
        const auto m = spell(record.magic);
        if (!next.config.magic_free || s.phase != Phase::response || s.turn_owner > 1 || !supported(record.magic)
            || !record.count || record.count > hand_capacity || (!record.from_ability && record.count == hand_capacity)
            || record.initial_mp > 99 || record.magic != s.pending.miracle || record.from_ability != s.pending.from_ability
            || record.items[0] != s.pending.free_card || s.attack_composition.base
            || (m.effect != Effect::heal && m.effect != Effect::damage && m.effect != Effect::money && m.effect != Effect::status)) return false;
        const auto& owner = s.players[s.turn_owner];
        if (owner.pending_deals < record.count + 1u || owner.mp > record.initial_mp
            || (!s.reaction_count && (owner.mp != record.initial_mp || owner.pending_deals != record.count + 1u))) return false;
        if (s.pending.attribute_removed && s.players[0].pending_deals + s.players[1].pending_deals
            < std::uint64_t{s.reaction_count} + record.count + 2) return false;
        const auto response_cost = record.initial_mp - owner.mp;
        // In this typed response subset, only210 can spend owner MP while the
        // original MAGIC remains active;215 ends it, and MP counters start later.
        if (response_cost % 5 || static_cast<std::uint64_t>(response_cost) > 5ULL * (s.reaction_count / 2)) return false;
        for (int i = 0; i < hand_capacity; ++i)
            if (i < record.count ? !find_magic_free_card(record.items[i]) : record.items[i] != 0) return false;
    } else if (s.free_cast != FreeCastComposition{} || s.pending.free_card) return false;
    snapshot_ = next;
    return true;
}
bool Env::script_item_removal_tickets(std::span<const std::uint64_t> tickets) noexcept {
    if (tickets.size() > snapshot_.scripted_item_removals.size() || (!snapshot_.config.item_removal && !tickets.empty())) return false;
    snapshot_.scripted_item_removals.fill(0);
    std::copy(tickets.begin(), tickets.end(), snapshot_.scripted_item_removals.begin());
    snapshot_.item_removal_count = static_cast<std::uint8_t>(tickets.size());
    snapshot_.item_removal_index = 0;
    return true;
}
bool Env::script_hp_gamble_rolls(std::span<const std::uint8_t> rolls) noexcept {
    if (rolls.size() > snapshot_.scripted_hp_gambles.size() || (!snapshot_.config.hp_gamble && !rolls.empty())) return false;
    for (auto roll : rolls) if (roll > 1) return false;
    snapshot_.scripted_hp_gambles.fill(0);
    std::copy(rolls.begin(), rolls.end(), snapshot_.scripted_hp_gambles.begin());
    snapshot_.hp_gamble_count = static_cast<std::uint8_t>(rolls.size());
    snapshot_.hp_gamble_index = 0;
    return true;
}
bool Env::script_buy_tickets(std::span<const std::uint64_t> tickets) noexcept {
    if (tickets.size() > snapshot_.scripted_buys.size() || (!snapshot_.config.buying && !tickets.empty())) return false;
    snapshot_.scripted_buys.fill(0);
    std::copy(tickets.begin(), tickets.end(), snapshot_.scripted_buys.begin());
    snapshot_.buy_count = static_cast<std::uint8_t>(tickets.size());
    snapshot_.buy_index = 0;
    return true;
}
bool Env::script_forget_tickets(std::span<const std::uint64_t> tickets) noexcept {
    if (tickets.size() > snapshot_.scripted_forgets.size() || (!snapshot_.config.forgetting && !tickets.empty())) return false;
    snapshot_.scripted_forgets.fill(0);
    std::copy(tickets.begin(), tickets.end(), snapshot_.scripted_forgets.begin());
    snapshot_.forget_count = static_cast<std::uint8_t>(tickets.size());
    snapshot_.forget_index = 0;
    return true;
}
bool Env::script_draws(std::span<const CardId> draws) noexcept {
    if (draws.size() > snapshot_.scripted_draws.size()) return false;
    for (auto id : draws) if (!supported(id)) return false;
    snapshot_.scripted_draws.fill(0);
    std::copy(draws.begin(), draws.end(), snapshot_.scripted_draws.begin());
    snapshot_.draw_count = static_cast<std::uint8_t>(draws.size());
    snapshot_.draw_index = 0;
    return true;
}
bool Env::script_hit_rolls(std::span<const std::uint8_t> rolls) noexcept {
    if (rolls.size() > snapshot_.scripted_hits.size() || (!snapshot_.config.chance && !rolls.empty())) return false;
    for (auto roll : rolls) if (roll < 1 || roll > 100) return false;
    snapshot_.scripted_hits.fill(0);
    std::copy(rolls.begin(), rolls.end(), snapshot_.scripted_hits.begin());
    snapshot_.hit_count = static_cast<std::uint8_t>(rolls.size());
    snapshot_.hit_index = 0;
    return true;
}
bool Env::script_pestle_targets(std::span<const std::uint8_t> targets) noexcept {
    if (targets.size() > snapshot_.scripted_pestle_targets.size() || (!snapshot_.config.pestle_mortar && !targets.empty())) return false;
    for (auto target : targets) if (target > 1) return false;
    snapshot_.scripted_pestle_targets.fill(0);
    std::copy(targets.begin(), targets.end(), snapshot_.scripted_pestle_targets.begin());
    snapshot_.pestle_target_count = static_cast<std::uint8_t>(targets.size());
    snapshot_.pestle_target_index = 0;
    return true;
}
bool Env::script_bounce_targets(std::span<const std::uint8_t> targets) noexcept {
    if (targets.size() > snapshot_.scripted_targets.size() || (!snapshot_.config.reflection && !targets.empty())) return false;
    for (auto target : targets) if (target > 1) return false;
    snapshot_.scripted_targets.fill(0);
    std::copy(targets.begin(), targets.end(), snapshot_.scripted_targets.begin());
    snapshot_.target_count = static_cast<std::uint8_t>(targets.size());
    snapshot_.target_index = 0;
    return true;
}
bool Env::script_disease_rolls(std::span<const std::uint8_t> rolls) noexcept {
    if (rolls.size() > snapshot_.scripted_diseases.size() || (!snapshot_.config.disease && !rolls.empty())) return false;
    for (auto roll : rolls) if (roll > 99) return false;
    snapshot_.scripted_diseases.fill(0);
    std::copy(rolls.begin(), rolls.end(), snapshot_.scripted_diseases.begin());
    snapshot_.disease_count = static_cast<std::uint8_t>(rolls.size());
    snapshot_.disease_index = 0;
    return true;
}
std::uint8_t Env::disease_roll() noexcept {
    if (snapshot_.disease_index < snapshot_.disease_count) return snapshot_.scripted_diseases[snapshot_.disease_index++];
    return static_cast<std::uint8_t>(bounded(snapshot_.rng_state, 100));
}
void Env::apply_disease(int seat, Disease incoming) noexcept {
    if (incoming == Disease::none) return;
    auto& p = snapshot_.state.players[seat];
    if (p.disease == Disease::heaven) {
        p.hp = 0;
    } else if (p.disease == Disease::none) {
        p.disease = incoming;
    } else {
        p.disease = static_cast<Disease>(std::max(static_cast<int>(incoming), static_cast<int>(p.disease) + 1));
    }
    p.worse_chance = 0;
}
void Env::apply_harm(int seat, Harm harm, StepResult& result) noexcept {
    if (harm == Harm::none) return;
    ++result.harm_applications;
    auto& p = snapshot_.state.players[seat];
    // Heaven's fall precedes even duplicate-harm detection in the legacy game.
    if (p.disease == Disease::heaven) {
        p.hp = 0;
        p.worse_chance = 0;
        ++result.harm_heaven_falls;
        return;
    }
    auto& flag = harm == Harm::glory ? p.glory : p.dark_cloud;
    if (!flag) ++result.harm_new_flags;
    flag = true;
}
void Env::cure_disease(int seat, Cure cure) noexcept {
    auto& p = snapshot_.state.players[seat];
    if (cure == Cure::lower || cure == Cure::all) p.glory = false;
    if (cure == Cure::all) p.dark_cloud = false;
    if (cure == Cure::all || (cure == Cure::lower && (p.disease == Disease::cold || p.disease == Disease::fever))) {
        p.disease = Disease::none;
        p.worse_chance = 0;
    }
}
std::uint8_t Env::bounce_target() noexcept {
    if (snapshot_.target_index < snapshot_.target_count) return snapshot_.scripted_targets[snapshot_.target_index++];
    // Death is finalized at the turn boundary. Even a provisional HP0 seat
    // remains a legacy bounce candidate while the response chain is unresolved.
    return static_cast<std::uint8_t>(bounded(snapshot_.rng_state, 2));
}
std::uint8_t Env::hit_roll() noexcept {
    if (snapshot_.hit_index < snapshot_.hit_count) return snapshot_.scripted_hits[snapshot_.hit_index++];
    return static_cast<std::uint8_t>(bounded(snapshot_.rng_state, 100) + 1);
}
Options Env::options() const noexcept {
    Options out;
    if (done()) return out;
    const auto& s = state();
    if (s.phase == Phase::purchase) { out.purchase_choices = 3; return out; }
    const auto& p = s.players[s.actor];
    if ((snapshot_.config.counters || snapshot_.config.revival) && p.dead) return out;
    if (s.phase == Phase::response && snapshot_.config.attribute_removal)
        for (int i = 0; i < p.hand_count; ++i)
            if (find_attribute_removal_card(p.hand[i])) out.removal_slots |= 1u << i;
    auto candidate = s.pending;
    if (out.removal_slots) {
        candidate.element = Element::neutral;
        candidate.attribute_removed = true;
    }
    if (s.phase == Phase::response) out.defense_limit = p.glory ? 1 : hand_capacity;
    const bool carrier_available = snapshot_.config.magic_free && std::any_of(p.hand.begin(), p.hand.begin() + p.hand_count,
        [](CardId id) { return find_magic_free_card(id) != nullptr; });
    const auto active_miracles = snapshot_.config.hp_absorption ? main_ability_capacity
        : (snapshot_.config.offensive_miracles ? total_miracle_count : miracle_count);
    std::uint16_t prefix_slots = 0;
    std::uint64_t acquired = 0;
    for (int i = 0; i < active_miracles; ++i) acquired += p.abilities[i];
    if (snapshot_.config.defensive_miracles)
        for (auto count : p.defensive_abilities) acquired += count;
    for (auto count : p.status_abilities) acquired += count;
    for (int i = 0; i < p.hand_count; ++i) {
        const auto bit = static_cast<std::uint16_t>(1u << i);
        if (s.phase == Phase::main && snapshot_.config.dying_weapon && find_dying_weapon_card(p.hand[i])) out.dying_weapon_slots |= bit;
        if (snapshot_.config.magic_bounce && find_magic_bounce_card(p.hand[i])) out.magic_bounce_slots |= bit;
        if (s.phase == Phase::main && snapshot_.config.sundry_boost && find_sundry_boost_card(p.hand[i])) out.sundry_boost_slots |= bit;
        if (s.phase == Phase::main && snapshot_.config.hp_gamble && find_hp_gamble_card(p.hand[i])) {
            out.hp_gamble_slots |= bit;
            out.utility_slots |= bit;
        }
        if (s.phase == Phase::main && snapshot_.config.buying && find_buying_card(p.hand[i])) out.buy_slots |= bit;
        if (s.phase == Phase::main && snapshot_.config.selling && p.hand_count >= 2 && find_selling_card(p.hand[i])) out.sell_slots |= bit;
        if (s.phase == Phase::main && snapshot_.config.pestle_mortar) {
            if (find_pestle_card(p.hand[i])) out.pestle_slots |= bit;
            if (find_mortar_card(p.hand[i])) out.mortar_slots |= bit;
        }
        if (s.phase == Phase::main && snapshot_.config.self_damage && find_self_damage_card(p.hand[i])) out.self_damage_slots |= bit;
        if (s.phase == Phase::main && snapshot_.config.hp_absorption
            && (find_hp_absorption_card(p.hand[i]) || find_hp_absorption_miracle(p.hand[i]))) out.hp_absorption_slots |= bit;
        if (s.phase == Phase::main && snapshot_.config.status_miracles && find_status_miracle(p.hand[i])) out.status_slots |= bit;
        const auto* card = artifact(p.hand[i]);
        if (!card) {
            const auto m = spell(p.hand[i]);
            const auto* defense = find_defensive_miracle(m.id);
            if (carrier_available) {
                if (s.phase == Phase::main && m.effect != Effect::none) {
                    const bool capacity = find_additive_miracle(m.id)
                        ? p.additive_abilities[0] + p.additive_abilities[1] + p.doubling_ability + p.wide_ability < 15ULL * snapshot_.config.max_decisions
                        : acquired < snapshot_.config.max_decisions;
                    if (capacity) out.free_cast_slots |= bit;
                }
                if (s.phase == Phase::response && defense && acquired < snapshot_.config.max_decisions
                    && can_special(candidate, defense->kind)) out.free_defensive_slots |= bit;
            }
            if (const auto* wide = find_wide_miracle(m.id)) {
                if (s.phase == Phase::main) {
                    out.discard_slots |= bit;
                    const auto cap = 15ULL * snapshot_.config.max_decisions;
                    if (p.mp >= wide->cost && p.additive_abilities[0] + p.additive_abilities[1] + p.doubling_ability + p.wide_ability < cap)
                        out.wide_slots |= bit;
                }
                continue;
            }
            if (const auto* doubling = find_doubling_miracle(m.id)) {
                if (s.phase == Phase::main) {
                    out.discard_slots |= bit;
                    const auto cap = 15ULL * snapshot_.config.max_decisions;
                    if (p.mp >= doubling->cost && p.additive_abilities[0] + p.additive_abilities[1] + p.doubling_ability + p.wide_ability < cap)
                        out.doubling_slots |= bit;
                }
                continue;
            }
            if (s.phase == Phase::main) {
                out.discard_slots |= bit;
                if (const auto ai = additive_miracle_index(m.id); ai >= 0) {
                    const auto cap = 15ULL * snapshot_.config.max_decisions;
                    if (p.mp >= m.cost && p.additive_abilities[0] + p.additive_abilities[1] + p.doubling_ability + p.wide_ability < cap) {
                        out.utility_slots |= bit;
                        out.additive_slots |= bit;
                    }
                } else if (!defense && p.mp >= m.cost && acquired < snapshot_.config.max_decisions) out.utility_slots |= bit;
            } else if (defense && p.mp >= defense->cost && acquired < snapshot_.config.max_decisions
                       && can_special(candidate, defense->kind)) {
                out.reaction_slots |= bit;
            }
            continue;
        }
        const auto& c = *card;
        if (s.phase == Phase::main && snapshot_.config.exchanges && find_exchange_card(c.id)) out.exchange_slots |= bit;
        if (s.phase == Phase::main && snapshot_.config.forgetting && find_forgetting_card(c.id)) {
            out.utility_slots |= bit;
            out.forget_slots |= bit;
        }
        if (s.phase == Phase::main && snapshot_.config.item_removal && find_item_removal_card(c.id)) {
            out.utility_slots |= bit;
            out.item_removal_slots |= bit;
        }
        const bool free_carrier = snapshot_.config.magic_free && find_magic_free_card(c.id);
        if (free_carrier) out.magic_free_slots |= bit;
        const bool magic_special = s.phase == Phase::response && is_magic_item(s.pending)
            && ((snapshot_.config.magic_reflection && find_magic_reflection_card(c.id))
                || (snapshot_.config.magic_bounce && find_magic_bounce_card(c.id)));
        const bool any_special = s.phase == Phase::response && snapshot_.config.any_reflection
            && find_any_reflection_card(c.id);
        const bool block_special = s.phase == Phase::response && snapshot_.config.magic_block
            && is_magic_item(s.pending) && find_magic_block_card(c.id);
        // A matching special always fires. Reserve it for the special base,
        // never as optional ordinary DFS in a prefix.
        if (s.phase == Phase::response && (c.kind == Kind::armor || c.defense > 0 || free_carrier) && !magic_special && !any_special && !block_special) prefix_slots |= bit;
        if (magic_special || any_special || block_special) out.reaction_slots |= bit;
        if (s.phase == Phase::response && snapshot_.config.counters && counter_info(c.id).id) out.counter_slots |= bit;
        if (s.phase == Phase::main) {
            if (c.kind == Kind::weapon && (c.attack > 0 || (snapshot_.config.mana_weapon && find_mana_weapon_card(c.id)))) out.weapon_bases |= bit;
            if (c.boost > 0) out.boost_slots |= bit;
            if (snapshot_.config.attribute_change && find_attribute_change_card(c.id)) out.attribute_slots |= bit;
            if (c.kind == Kind::heal || c.kind == Kind::mana) out.utility_slots |= bit;
            if (c.discardable || (snapshot_.config.pestle_mortar && find_mortar_card(c.id))) out.discard_slots |= bit;
        } else if ((s.pending.kind == Effect::damage || snapshot_.config.defensive_miracles
                    || snapshot_.config.attribute_removal || snapshot_.config.counters || snapshot_.config.magic_reflection || snapshot_.config.any_reflection || snapshot_.config.magic_block || snapshot_.config.magic_bounce || snapshot_.config.status_miracles)
                   && !magic_special && !any_special && !block_special && (c.kind == Kind::armor || c.defense > 0 || free_carrier)
                   && can_defend(s.pending.element, element_of(c.id))) {
            out.defense_slots |= bit;
        }
        if (s.phase == Phase::response && snapshot_.config.reflection && can_weapon_react(candidate)
            && find_reaction_card(c.id)) out.reaction_slots |= bit;
    }
    if (s.phase == Phase::main && snapshot_.config.miracles)
        for (int i = 0; i < active_miracles; ++i)
            if (p.abilities[i] && p.mp >= spell(spell_id(i)).cost) out.ability_slots |= 1u << i;
    if (s.phase == Phase::main && snapshot_.config.status_miracles)
        for (int i = 0; i < status_miracle_count; ++i)
            if (p.status_abilities[i] && p.mp >= status_miracles()[i].cost) out.status_ability_slots |= 1u << i;
    if (s.phase == Phase::main && snapshot_.config.additive_miracles)
        for (int i = 0; i < additive_miracle_count; ++i)
            if (p.additive_abilities[i] && p.mp >= additive_miracles()[i].cost) out.additive_ability_slots |= 1u << i;
    if (s.phase == Phase::main && snapshot_.config.doubling_miracles
        && p.doubling_ability && p.mp >= doubling_miracles()[0].cost) out.doubling_ability_slots = 1;
    if (s.phase == Phase::main && snapshot_.config.wide_miracles
        && p.wide_ability && p.mp >= wide_miracles()[0].cost) out.wide_ability_slots = 1;
    if (s.phase == Phase::response && snapshot_.config.defensive_miracles)
        for (int i = 0; i < defensive_miracle_count; ++i) {
            const auto& m = defensive_miracles()[i];
            if (p.defensive_abilities[i] && p.mp >= m.cost && can_special(candidate, m.kind))
                out.defensive_ability_slots |= 1u << i;
        }
    if (carrier_available) {
        if (s.phase == Phase::main) {
            for (int i = 0; i < active_miracles; ++i) if (p.abilities[i]) out.free_ability_slots |= 1u << i;
            if (snapshot_.config.status_miracles) for (int i = 0; i < status_miracle_count; ++i)
                if (p.status_abilities[i]) out.free_status_ability_slots |= 1u << i;
            for (int i = 0; i < additive_miracle_count; ++i) if (p.additive_abilities[i]) out.free_additive_ability_slots |= 1u << i;
        } else if (snapshot_.config.defensive_miracles) {
            for (int i = 0; i < defensive_miracle_count; ++i)
                if (p.defensive_abilities[i] && can_special(candidate, defensive_miracles()[i].kind)) out.free_defensive_ability_slots |= 1u << i;
        }
    }
    if (s.phase == Phase::response && s.pending.miracle) {
        bool magic_bounce = (out.defensive_ability_slots & 1u) != 0;
        for (int i = 0; !magic_bounce && i < p.hand_count; ++i)
            if ((out.reaction_slots & (1u << i)) && (p.hand[i] == defensive_miracles()[0].id || find_magic_reflection_card(p.hand[i]) || find_any_reflection_card(p.hand[i]) || find_magic_block_card(p.hand[i]) || find_magic_bounce_card(p.hand[i]))) magic_bounce = true;
        // Magic reactions skip the final attribute check. Incompatible armor can
        // still be spent before it; ordinary defense must pass legal() below.
        if (magic_bounce) out.defense_slots = prefix_slots;
    }
    if (out.removal_slots) out.defense_slots = prefix_slots;
    if (snapshot_.config.counter_extensions && s.phase == Phase::response) out.defense_slots = prefix_slots;
    if (s.phase == Phase::response && (s.pending.kind == Effect::forget || s.pending.kind == Effect::remove_item || s.pending.kind == Effect::sell || s.pending.kind == Effect::buy || s.pending.kind == Effect::hp_gamble)) {
        out.defense_slots = out.removal_slots = out.counter_slots = 0;
        out.defensive_ability_slots = out.free_defensive_slots = out.free_defensive_ability_slots = 0;
        std::uint16_t only_any = 0;
        for (int i = 0; i < p.hand_count; ++i)
            if ((out.reaction_slots & (1u << i)) && find_any_reflection_card(p.hand[i])) only_any |= 1u << i;
        out.reaction_slots = only_any;
    }
    if (out.sell_slots) out.sale_item_slots = static_cast<std::uint16_t>((1u << p.hand_count) - 1u);
    out.pray = s.phase == Phase::main && out.weapon_bases == 0;
    return out;
}
bool Env::legal(Action a) const noexcept {
    if (done()) return false;
    if (state().phase == Phase::purchase) {
        return snapshot_.config.buying && (a.kind == ActionKind::buy_decline || a.kind == ActionKind::buy_accept)
            && a.base == no_slot && a.target == no_slot && !a.slots && !a.counter_count && !a.counter_order && !a.extra_count
            && std::all_of(a.extras.begin(), a.extras.end(), [](auto ref) { return ref == 0; });
    }
    if (a.kind == ActionKind::sell) { CardId item = 0; int price = 0; return prepare_sale(a, item, price); }
    if (a.kind == ActionKind::exchange) { LastExchange record; return prepare_exchange(a, record); }
    if (a.kind == ActionKind::cast_ordered) { FreeCastPlan plan; return prepare_free_cast(a, plan); }
    if (a.kind == ActionKind::defend_ordered) { DefensePlan plan; return prepare_ordered_defense(a, plan); }
    if (a.kind == ActionKind::attack_ordered) {
        if (state().phase != Phase::main) return false;
        AttackComposition record;
        CompositionPlan plan;
        std::uint16_t slots = 0;
        return prepare_ordered(a, record, plan, slots);
    }
    if (a.extra_count || std::any_of(a.extras.begin(), a.extras.end(), [](auto x) { return x != 0; })) return false;
    const auto& s = state();
    const auto& p = s.players[s.actor];
    if ((snapshot_.config.counters || snapshot_.config.revival) && p.dead) return false;
    const auto opts = options();
    if (!snapshot_.config.counter_extensions) {
        if (a.counter_count || a.counter_order) return false;
    } else {
        const auto selected = s.phase == Phase::response ? static_cast<std::uint16_t>(a.slots & opts.counter_slots) : 0;
        if (a.counter_count > hand_capacity || a.counter_count != std::popcount(static_cast<unsigned>(selected))) return false;
        if (a.counter_count < hand_capacity && (a.counter_order >> (4 * a.counter_count)) != 0) return false;
        std::uint16_t seen = 0;
        for (int n = 0; n < a.counter_count; ++n) {
            const auto slot = static_cast<unsigned>((a.counter_order >> (4 * n)) & 15u);
            const auto bit = static_cast<std::uint16_t>(1u << slot);
            if (!(selected & bit) || (seen & bit)) return false;
            seen |= bit;
        }
        if (seen != selected) return false;
    }
    if (s.phase == Phase::response) {
        const bool retained = a.kind == ActionKind::defend_cast;
        if ((a.kind != ActionKind::defend && !retained) || a.target != no_slot
            || (a.slots & opts.defense_slots) != a.slots) return false;
        if (std::popcount(a.slots) + (retained || a.base != no_slot) > opts.defense_limit) return false;
        auto incoming = s.pending;
        if (a.slots & opts.removal_slots) {
            incoming.element = Element::neutral;
            incoming.attribute_removed = true;
        }
        if (retained)
            return a.base < defensive_miracle_count && (opts.defensive_ability_slots & (1u << a.base))
                && can_special(incoming, defensive_miracles()[a.base].kind);
        if (a.base != no_slot) {
            if (a.base >= p.hand_count || !(opts.reaction_slots & (1u << a.base)) || (a.slots & (1u << a.base))) return false;
            if (const auto* m = find_defensive_miracle(p.hand[a.base])) return can_special(incoming, m->kind);
            if (snapshot_.config.any_reflection && find_any_reflection_card(p.hand[a.base]))
                return !a.slots || incoming.miracle || (!incoming.counter_card && incoming.element == Element::neutral);
            if (snapshot_.config.magic_bounce && find_magic_bounce_card(p.hand[a.base]))
                return is_magic_item(incoming);
            if (snapshot_.config.magic_block && find_magic_block_card(p.hand[a.base]))
                return is_magic_item(incoming);
            if (snapshot_.config.magic_reflection && find_magic_reflection_card(p.hand[a.base]))
                return is_magic_item(incoming);
            return can_weapon_react(incoming);
        }
        if (!a.slots) return true;
        Element combined = Element::neutral;
        bool first = true;
        for (int i = 0; i < p.hand_count; ++i) if (a.slots & (1u << i)) {
            const auto next = element_of(p.hand[i]);
            combined = first ? next : combine(combined, next);
            first = false;
        }
        return can_defend(incoming.element, combined);
    }
    if (weapon_attack(a.kind) && a.base < p.hand_count && find_pestle_card(p.hand[a.base]))
        return a.kind == ActionKind::attack && a.slots == 0 && a.target < 2;
    switch (a.kind) {
    case ActionKind::pray:
        return opts.pray && a.base == no_slot && a.slots == 0 && a.target == no_slot;
    case ActionKind::attack:
        return a.base < p.hand_count && a.target < 2 && (opts.weapon_bases & (1u << a.base))
            && !(a.slots & (1u << a.base)) && (a.slots & opts.boost_slots) == a.slots
            && (!chance_rate(p.hand[a.base]) || a.slots == 0);
    case ActionKind::attack_set_fire:
    case ActionKind::attack_set_water:
    case ActionKind::attack_set_neutral:
        return a.base < p.hand_count && a.target < 2 && (opts.weapon_bases & (1u << a.base))
            && !(a.slots & (1u << a.base)) && !chance_rate(p.hand[a.base])
            && (a.slots & (opts.boost_slots | opts.attribute_slots)) == a.slots && attribute_component(a) != 0;
    case ActionKind::buy:
        return a.base < p.hand_count && a.target < 2 && !a.slots && (opts.buy_slots & (1u << a.base));
    case ActionKind::use:
        return a.base < p.hand_count && a.target < 2 && a.slots == 0 && (opts.utility_slots & (1u << a.base));
    case ActionKind::cast_status:
        return snapshot_.config.status_miracles && a.base < status_miracle_count && a.target < 2 && !a.slots
            && (opts.status_ability_slots & (1u << a.base));
    case ActionKind::cast:
        return a.base < main_ability_capacity && a.target < 2 && a.slots == 0 && (opts.ability_slots & (1u << a.base));
    case ActionKind::cast_additive:
        return a.base < additive_miracle_count && a.target < 2 && a.slots == 0
            && (opts.additive_ability_slots & (1u << a.base));
    case ActionKind::discard:
        return a.base == no_slot && a.target == no_slot && a.slots != 0
            && (a.slots & opts.discard_slots) == a.slots;
    default: return false;
    }
}
void Env::consume(int seat, std::uint16_t slots) noexcept {
    auto& p = snapshot_.state.players[seat];
    std::uint8_t write = 0;
    for (int read = 0; read < p.hand_count; ++read)
        if (!(slots & (1u << read))) p.hand[write++] = p.hand[read];
    std::fill(p.hand.begin() + write, p.hand.end(), 0);
    p.hand_count = write;
}
void Env::finish_turn(StepResult& result) noexcept {
    auto& s = snapshot_.state;
    s.pending = Pending{};
    s.attack_composition = {};
    s.free_cast = {};
    s.reaction_count = 0;
    s.phase = Phase::main;
    if (snapshot_.config.counters) {
        s.counter_queue.fill(Pending{});
        s.counter_count = s.counter_generated = s.counter_started = 0;
    }
    ++s.turns;
    auto& owner = s.players[s.turn_owner];
    if (snapshot_.config.disease && owner.hp > 0 && owner.disease != Disease::none) {
        const auto before = owner.disease;
        const int before_hp = owner.hp;
        const bool worsened = disease_roll() < owner.worse_chance;
        if (worsened) apply_disease(s.turn_owner, owner.disease);
        else ++owner.worse_chance;
        if (owner.hp > 0) {
            switch (owner.disease) {
            case Disease::cold: owner.hp = std::max(0, owner.hp - 1); break;
            case Disease::fever: owner.hp = std::max(0, owner.hp - 2); break;
            case Disease::hell: owner.hp = std::max(0, owner.hp - 5); break;
            case Disease::heaven: owner.hp = std::min(99, owner.hp + 5); break;
            case Disease::none: break;
            }
        }
        result.disease_tick = true;
        result.disease_worsened = worsened;
        result.disease_damage = std::max(0, before_hp - owner.hp);
        result.disease_healing = std::max(0, owner.hp - before_hp);
        s.last_tick = State::LastDiseaseTick{s.turn_owner, before, owner.disease, worsened,
                                           result.disease_damage, result.disease_healing};
    }
    finalize_deaths(result);
    // Legacy endInning distributes accumulated gifts before checking game end.
    for (int seat = 0; seat < 2; ++seat) if (s.players[seat].hp > 0) {
        deal(seat, s.players[seat].pending_deals);
        s.players[seat].pending_deals = 0;
        s.players[seat].retained_deals = 0;
        s.players[seat].incoming_sale = 0;
        s.players[seat].incoming_buy = 0;
    }
    if (s.players[0].hp == 0 || s.players[1].hp == 0) {
        s.terminated = true;
        if (s.players[0].hp == 0 && s.players[1].hp == 0) {
            s.winner = -1; // Learning convention for the old server's zero-survivor end.
        } else {
            s.winner = s.players[0].hp == 0 ? 1 : 0;
            result.rewards[s.winner] = 1;
            result.rewards[1 - s.winner] = -1;
        }
        s.actor = s.turn_owner;
        return;
    }
    if (++s.order_index == 2) {
        s.order_index = 0;
        s.order = {0, 1};
        if (bounded(snapshot_.rng_state, 2)) std::swap(s.order[0], s.order[1]);
    }
    s.actor = s.turn_owner = s.order[s.order_index];
}
void Env::finalize_deaths(StepResult& result) noexcept {
    if (!snapshot_.config.counters && !snapshot_.config.revival) return;
    auto& players = snapshot_.state.players;
    // The legacy server scans seats, counting not.dead before each death.
    // HP0 can still be provisional until this item or owner tick has finished.
    for (int seat = 0; seat < 2; ++seat) {
        auto& p = players[seat];
        if (p.hp != 0 || p.dead) continue;
        if (snapshot_.config.revival && !players[0].dead && !players[1].dead) {
            bool revived = false;
            for (int slot = 0; slot < p.hand_count; ++slot) {
                const auto* card = find_revival_card(p.hand[slot]);
                if (!card) continue;
                const auto id = card->card.id;
                p.hp = std::min(99, p.hp + card->amount);
                consume(seat, static_cast<std::uint16_t>(1u << slot));
                snapshot_.state.last_revival = {id, static_cast<std::uint8_t>(seat)};
                ++result.revivals[seat];
                revived = true;
                break;
            }
            if (revived) continue;
        }
        p.dead = true;
    }
}
void Env::finish_item(StepResult& result) noexcept {
    snapshot_.state.attack_composition = {};
    snapshot_.state.free_cast = {};
    finalize_deaths(result);
    if (!snapshot_.config.counters) { finish_turn(result); return; }
    auto& s = snapshot_.state;
    while (s.counter_count) {
        const auto next = s.counter_queue[0];
        for (int i = 1; i < s.counter_count; ++i) s.counter_queue[i - 1] = s.counter_queue[i];
        s.counter_queue[--s.counter_count] = Pending{};
        ++s.counter_started;
        bool hit = true, forced = false;
        if (next.hit_rate) {
            // Legacy hit checks precede the dead-target guard. A result that
            // never reaches a public command must not overwrite LastChance.
            forced = s.players[next.target].dark_cloud;
            hit = forced || hit_roll() <= next.hit_rate;
        }
        if (s.players[next.target].dead) {
            if (next.hit_rate) ++result.counter_hidden_checks;
            ++result.counter_skipped;
            continue;
        }
        if (next.hit_rate) {
            s.last_chance = LastChance{next.counter_card, next.source, next.target, next.hit_rate, hit, forced};
            ++result.counter_chance_attempts;
            result.counter_chance_forced += forced;
            if (hit) ++result.counter_chance_hits;
            else {
                ++result.counter_chance_misses;
                ++result.counter_missed;
                continue;
            }
        }
        ++result.counter_launched;
        if (next.source == next.target) {
            auto& owner = s.players[next.source];
            const int before = owner.mp;
            owner.mp = static_cast<int>(std::min<std::int64_t>(99, owner.mp + next.amount));
            result.counter_mp_gained += owner.mp - before;
            ++result.counter_auto_resolved;
            continue;
        }
        s.pending = next;
        s.reaction_count = 0;
        s.actor = next.target;
        s.phase = Phase::response;
        return;
    }
    finish_turn(result);
}
void Env::remove_item(StepResult& result) noexcept {
    auto& s = snapshot_.state;
    const auto seat = s.pending.target;
    const auto total = s.players[seat].hand_count;
    result.item_removal_attempt = true;
    result.item_removed_from = seat;
    result.item_removal_population = total;
    s.last_item_removal = {s.pending.item_removal_card, s.pending.source, seat, 0, total, no_slot};
    if (!total) return;
    const auto slot = static_cast<std::uint8_t>(snapshot_.item_removal_index < snapshot_.item_removal_count
        ? snapshot_.scripted_item_removals[snapshot_.item_removal_index++] % total : bounded(snapshot_.rng_state, total));
    const auto id = s.players[seat].hand[slot];
    result.item_removed = true;
    result.removed_item = id;
    result.removed_item_slot = slot;
    s.last_item_removal.removed = id;
    s.last_item_removal.slot = slot;
    consume(seat, static_cast<std::uint16_t>(1u << slot)); // Victim earns no replacement gift.
}
void Env::offer_buy(StepResult& result) noexcept {
    auto& s = snapshot_.state;
    LastBuy buy;
    buy.id = s.pending.buy_card;
    buy.source = s.pending.source;
    buy.target = s.pending.target;
    buy.turn = s.turns + 1;
    const auto& seller = s.players[buy.target];
    buy.population = seller.hand_count;
    if (buy.population) {
        buy.slot = static_cast<std::uint8_t>(snapshot_.buy_index < snapshot_.buy_count
            ? snapshot_.scripted_buys[snapshot_.buy_index++] % buy.population : bounded(snapshot_.rng_state, buy.population));
        buy.item = seller.hand[buy.slot];
        buy.price = trade_price(buy.item);
        buy.outcome = buy.source == buy.target ? BuyOutcome::self
            : (s.players[buy.source].money < buy.price ? BuyOutcome::insufficient : BuyOutcome::offered);
    } else buy.outcome = BuyOutcome::empty;
    s.last_buy = buy;
    result.buy_attempt = true;
    result.buy_item = buy.item;
    result.buy_price = buy.price;
    result.buy_source = buy.source;
    result.buy_target = buy.target;
    result.buy_population = buy.population;
    result.buy_slot = buy.slot;
    result.buy_outcome = buy.outcome;
    if (buy.outcome == BuyOutcome::offered) {
        result.buy_offered = true;
        s.phase = Phase::purchase;
        s.actor = buy.source;
    } else finish_item(result);
}
void Env::decide_buy(bool accepted, StepResult& result) noexcept {
    auto& s = snapshot_.state;
    auto& buy = s.last_buy;
    if (accepted) {
        auto& seller = s.players[buy.target];
        auto& buyer = s.players[buy.source];
        consume(buy.target, static_cast<std::uint16_t>(1u << buy.slot));
        const int before = seller.money;
        seller.money = std::min(99, seller.money + buy.price);
        buy.money_gained = static_cast<std::uint8_t>(seller.money - before);
        if (buyer.hand_count < hand_capacity) {
            buyer.hand[buyer.hand_count++] = buy.item;
            ++buyer.incoming_buy;
            buy.delivered = true;
        }
        buyer.money -= buy.price;
        buy.money_lost = static_cast<std::uint8_t>(buy.price);
        buy.outcome = BuyOutcome::purchased;
    } else buy.outcome = BuyOutcome::declined;
    result.buy_decided = true;
    result.buy_accepted = accepted;
    result.buy_delivered = buy.delivered;
    result.buy_item = buy.item;
    result.buy_price = buy.price;
    result.buy_source = buy.source;
    result.buy_target = buy.target;
    result.buy_population = buy.population;
    result.buy_slot = buy.slot;
    result.buy_outcome = buy.outcome;
    result.buy_money_lost = buy.money_lost;
    result.buy_money_gained = buy.money_gained;
    finish_item(result);
}
void Env::forget_one(StepResult& result) noexcept {
    auto& s = snapshot_.state;
    auto& player = s.players[s.pending.target];
    const auto total = retained_total(player);
    result.forget_attempt = true;
    result.forgotten_from = s.pending.target;
    result.forget_population = total;
    s.last_forget = LastForget{s.pending.forget_card, s.pending.source, s.pending.target, 0, total};
    if (!total) return;
    auto ticket = snapshot_.forget_index < snapshot_.forget_count
        ? snapshot_.scripted_forgets[snapshot_.forget_index++] % total : bounded64(snapshot_.rng_state, total);
    for (const auto id : status_retained_type_ids) {
        const auto weight = retained_count(player, id);
        if (ticket < weight) {
            remove_retained(player, id);
            result.forget_removed = true;
            result.forgotten_miracle = id;
            s.last_forget.removed = id;
            return;
        }
        ticket -= weight;
    }
}
void Env::resolve(int defense, StepResult& result) noexcept {
    auto& s = snapshot_.state;
    auto& p = s.players[s.pending.target];
    switch (s.pending.kind) {
    case Effect::damage: {
        const auto damage = std::max<std::int64_t>(0, s.pending.amount - defense);
        if (damage > 0) {
            apply_disease(s.pending.target, s.pending.disease);
            apply_harm(s.pending.target, s.pending.harm, result);
        }
        if (s.pending.hp_absorption_card) {
            auto& source = s.players[s.pending.source];
            const int before = source.hp;
            if (s.pending.element != Element::dark)
                source.hp = static_cast<int>(std::min<std::int64_t>(99, source.hp + damage));
            result.hp_absorption_resolved = true;
            result.hp_absorption_amount = damage;
            result.hp_absorption_healing = static_cast<std::uint8_t>(source.hp - before);
            result.hp_absorption_source = s.pending.source;
            result.hp_absorption_target = s.pending.target;
            result.hp_absorption_card = s.pending.hp_absorption_card;
            s.last_hp_absorption = {s.pending.hp_absorption_card, s.pending.source, s.pending.target,
                damage, result.hp_absorption_healing};
        }
        int self_source_loss = 0;
        if (s.pending.self_damage_card && s.pending.element != Element::dark) {
            auto& source = s.players[s.pending.source];
            const int before = source.hp;
            source.hp = static_cast<int>(std::max<std::int64_t>(0, source.hp - damage));
            self_source_loss = before - source.hp;
        }
        const int target_before = p.hp; // After source loss: self-alias must be charged twice.
        if (damage > 0 && s.pending.element == Element::dark) {
            p.hp = 0;
            result.dark_execution = true;
        } else p.hp = static_cast<int>(std::max<std::int64_t>(0, p.hp - damage));
        if (s.pending.pestle_card) {
            result.pestle_resolved = true;
            result.pestle_target = s.last_pestle.target;
            result.pestle_mortar = s.pending.mortar_card != 0;
            result.pestle_damage = static_cast<std::uint16_t>(s.pending.amount);
            result.pestle_residual = damage;
            result.pestle_target_lost = static_cast<std::uint8_t>(target_before - p.hp);
            result.pestle_resolved_source = s.pending.source;
            result.pestle_resolved_target = s.pending.target;
        }
        if (s.pending.self_damage_card) {
            result.self_damage_resolved = true;
            result.self_damage_amount = damage;
            result.self_damage_card = s.pending.self_damage_card;
            result.self_damage_source = s.pending.source;
            result.self_damage_target = s.pending.target;
            result.self_damage_source_lost = static_cast<std::uint8_t>(self_source_loss);
            result.self_damage_target_lost = static_cast<std::uint8_t>(target_before - p.hp);
            s.last_self_damage = {s.pending.self_damage_card, s.pending.source, s.pending.target, damage,
                result.self_damage_source_lost, result.self_damage_target_lost};
        }
        break;
    }
    case Effect::heal: p.hp = static_cast<int>(std::min<std::int64_t>(99, p.hp + s.pending.amount)); break;
    case Effect::mana:
        p.mp = static_cast<int>(std::min<std::int64_t>(99, p.mp + s.pending.amount));
        apply_disease(s.pending.target, s.pending.disease);
        break;
    case Effect::money: p.money = static_cast<int>(std::min<std::int64_t>(99, p.money + s.pending.amount)); break;
    case Effect::status:
        if (s.pending.cure != Cure::none) cure_disease(s.pending.target, s.pending.cure);
        else if (s.pending.disease != Disease::none) apply_disease(s.pending.target, s.pending.disease);
        else apply_harm(s.pending.target, s.pending.harm, result);
        result.status_resolved = true;
        result.status_miracle_card = s.pending.miracle;
        result.status_source = s.pending.source;
        result.status_target = s.pending.target;
        break;
    case Effect::cure: cure_disease(s.pending.target, s.pending.cure); break;
    case Effect::absorb_money: {
        // ABSORB_YEN credits the full requested amount before charging the
        // target. Insufficient YEN is paid from MP and then HP; actual payment
        // does not limit the credit. A dead source still owns its queued item.
        auto& source = s.players[s.pending.source];
        const int source_before = source.money;
        source.money = static_cast<int>(std::min<std::int64_t>(99, source.money + s.pending.amount));
        auto debt = s.pending.amount;
        const auto pay = [&debt](int& resource) noexcept {
            const int paid = static_cast<int>(std::min<std::int64_t>(resource, debt));
            resource -= paid;
            debt -= paid;
            return paid;
        };
        ++result.counter_money_absorptions;
        result.counter_money_gained += source.money - source_before;
        result.counter_money_lost += pay(p.money);
        result.counter_mp_lost += pay(p.mp);
        result.counter_hp_lost += pay(p.hp);
        break;
    }
    case Effect::hp_gamble: {
        auto& event = s.last_hp_gamble;
        const int before = p.hp;
        p.hp = static_cast<int>(std::clamp<std::int64_t>(p.hp + s.pending.amount, 0, 99));
        event.resolved = true;
        event.resolved_source = s.pending.source;
        event.resolved_target = s.pending.target;
        event.change = p.hp - before;
        result.hp_gamble_resolved = true;
        result.hp_gamble_card = s.pending.hp_gamble_card;
        result.hp_gamble_delta = static_cast<int>(s.pending.amount);
        result.hp_gamble_change = event.change;
        result.hp_gamble_source = s.pending.source;
        result.hp_gamble_target = s.pending.target;
        break;
    }
    case Effect::buy: offer_buy(result); return;
    case Effect::sell: {
        LastSale sale;
        sale.id = s.pending.sale_card;
        sale.item = s.pending.sale_item;
        sale.source = s.pending.source;
        sale.target = s.pending.target;
        sale.price = static_cast<int>(s.pending.amount);
        sale.turn = s.turns + 1;
        int debt = sale.price;
        const auto pay = [&debt](int& resource) noexcept {
            const int paid = std::min(resource, debt);
            resource -= paid;
            debt -= paid;
            return static_cast<std::uint8_t>(paid);
        };
        // SELL charges first. With self-alias, credit cannot undo MP/HP losses.
        sale.money_lost = pay(p.money);
        sale.mp_lost = pay(p.mp);
        sale.hp_lost = pay(p.hp);
        if (p.hand_count < hand_capacity) {
            p.hand[p.hand_count++] = sale.item;
            ++p.incoming_sale;
            sale.delivered = true;
        }
        auto& seller = s.players[s.pending.source];
        const int before = seller.money;
        seller.money = std::min(99, seller.money + sale.price);
        sale.money_gained = static_cast<std::uint8_t>(seller.money - before);
        s.last_sale = sale;
        result.sale_resolved = true;
        result.sale_card = sale.id;
        result.sale_item = sale.item;
        result.sale_source = sale.source;
        result.sale_target = sale.target;
        result.sale_price = sale.price;
        result.sale_delivered = sale.delivered;
        result.sale_money_lost = sale.money_lost;
        result.sale_mp_lost = sale.mp_lost;
        result.sale_hp_lost = sale.hp_lost;
        result.sale_money_gained = sale.money_gained;
        break;
    }
    case Effect::forget: forget_one(result); break;
    case Effect::remove_item: remove_item(result); break;
    case Effect::harm:
        apply_harm(s.pending.target, s.pending.harm, result);
        if (s.pending.counter_card) ++result.counter_harm_applications;
        break;
    default: break;
    }
    result.counter_resolved = s.pending.counter_card != 0;
    finish_item(result);
}
void Env::launch_pending(CardId card, StepResult& result) noexcept {
    auto& s = snapshot_.state;
    if (snapshot_.config.counters) s.pending.origin_source = s.pending.source;
    if (const auto rate = s.pending.wide_miracle ? wide_miracles()[0].hit_rate : chance_rate(card)) {
        // Percent weapons and miracles each attack every enemy once. This
        // duel has one enemy, regardless of the originally selected target.
        const auto enemy = static_cast<std::uint8_t>(1 - s.pending.source);
        const bool forced = s.players[enemy].dark_cloud;
        const bool hit = forced || hit_roll() <= rate;
        s.last_chance = LastChance{card, s.pending.source, enemy, static_cast<std::uint8_t>(rate), hit, forced};
        s.last_chance.wide_miracle = s.pending.wide_miracle;
        result.chance_attempt = true;
        result.chance_hit = hit;
        result.chance_forced = forced;
        if (hit) {
            s.pending.target = enemy;
            s.pending.hit_rate = static_cast<std::uint8_t>(rate);
            s.phase = Phase::response;
            s.actor = enemy;
        } else finish_item(result);
    } else if (s.pending.target == s.pending.source) resolve(0, result);
    else { s.phase = Phase::response; s.actor = s.pending.target; }
}
StepResult Env::step(Action a) noexcept {
    StepResult result;
    result.actor = state().actor;
    result.terminated = state().terminated;
    result.truncated = state().truncated;
    if (!legal(a)) return result;
    DefensePlan defense_plan;
    const bool ordered_defense = a.kind == ActionKind::defend_ordered;
    if (ordered_defense) { prepare_ordered_defense(a, defense_plan); a = defense_plan.action; }
    result.valid = true;
    result.ordered_defense = ordered_defense;
    result.magic_free_suffix = defense_plan.suffix;
    result.magic_free_mp_saved = defense_plan.mp_saved;
    auto& s = snapshot_.state;
    auto& p = s.players[s.actor];
    ++s.decisions;
    if (s.phase == Phase::purchase) {
        decide_buy(a.kind == ActionKind::buy_accept, result);
    } else if (a.kind == ActionKind::defend || a.kind == ActionKind::defend_cast) {
        const bool retained = a.kind == ActionKind::defend_cast;
        result.glory_defense = p.glory;
        int defense = 0;
        if (snapshot_.config.magic_free) {
            const auto physical = static_cast<std::uint16_t>(a.slots | (retained || a.base == no_slot ? 0u : 1u << a.base));
            for (int i = 0; i < p.hand_count; ++i)
                if ((physical & (1u << i)) && find_magic_free_card(p.hand[i])) ++result.magic_free_cards;
        }
        for (int n = 0; n < (ordered_defense ? defense_plan.prefix_count : p.hand_count); ++n) {
            const int i = ordered_defense ? defense_plan.prefix[n] : n;
            if (!(a.slots & (1u << i))) continue;
            if (find_attribute_removal_card(p.hand[i])) {
                if (!result.attribute_removals) result.removed_from = s.pending.element;
                ++result.attribute_removals;
                result.attribute_removed = true;
                s.pending.element = Element::neutral;
                s.pending.attribute_removed = true;
            }
            defense += artifact(p.hand[i])->defense;
            if (const auto* status = find_disease_card(p.hand[i]); status && status->on_defense)
                apply_disease(s.actor, status->disease);
        }
        if (!retained && a.base == no_slot) {
            if (snapshot_.config.counters && s.pending.kind == Effect::damage) {
                const auto residual = std::max<std::int64_t>(0, s.pending.amount - defense);
                const int count = snapshot_.config.counter_extensions ? a.counter_count : p.hand_count;
                if (residual > 0) for (int n = 0; n < count; ++n) {
                    const int i = snapshot_.config.counter_extensions
                        ? static_cast<int>((a.counter_order >> (4 * n)) & 15u) : n;
                    if (!(a.slots & (1u << i))) continue;
                    if (const auto counter = counter_info(p.hand[i]); counter.id) {
                        const auto target = counter.target == CounterExtensionTarget::self ? s.actor
                            : (counter.target == CounterExtensionTarget::all_enemies
                                ? static_cast<std::uint8_t>(1 - s.actor) : s.pending.source);
                        if (counter.target == CounterExtensionTarget::all_enemies && s.players[target].dead) continue;
                        Pending queued{counter.effect, s.actor, target, residual * counter.multiplier};
                        queued.element = counter.element;
                        queued.hit_rate = static_cast<std::uint8_t>(counter.hit_rate);
                        queued.harm = counter.harm;
                        queued.origin_source = s.actor;
                        queued.counter_card = counter.id;
                        s.counter_queue[s.counter_count++] = queued;
                        ++s.counter_generated;
                        ++result.counter_queued;
                    }
                }
            }
            p.pending_deals += std::popcount(a.slots);
            consume(s.actor, a.slots);
            resolve(defense, result);
        } else if (!retained && find_magic_block_card(p.hand[a.base])) {
            const auto card_id = p.hand[a.base];
            const auto defender = s.actor;
            const auto slots = static_cast<std::uint16_t>(a.slots | (1u << a.base));
            p.pending_deals += std::popcount(slots);
            consume(defender, slots);
            // BLOCK_MAGIC ends the original item without applying its payload.
            // Prefix status/consumption persists; MP/retention/hit is not undone.
            s.last_reaction = State::LastReaction{card_id, defender, defender, WeaponReaction::block, false};
            result.magic_blocked = true;
            finish_item(result);
        } else if (const auto* m = retained ? &defensive_miracles()[a.base] : find_defensive_miracle(p.hand[a.base])) {
            const auto paid = defense_plan.free_card ? 0 : m->cost;
            p.mp -= paid;
            result.spell_mp_spent = static_cast<std::uint16_t>(paid);
            if (retained) ++p.retained_deals;
            else ++p.defensive_abilities[defensive_miracle_index(m->id)];
            const auto slots = static_cast<std::uint16_t>(a.slots | (retained ? 0u : (1u << a.base)));
            p.pending_deals += std::popcount(a.slots) + 1;
            consume(s.actor, slots);
            result.spell_first_use = result.defensive_first_use = !retained;
            result.spell_recast = result.defensive_recast = retained;
            const auto defender = s.actor;
            if (m->kind == DefensiveMiracleKind::block_weapon) {
                s.last_reaction = State::LastReaction{m->id, defender, defender, WeaponReaction::block, retained, defense_plan.free_card};
                result.weapon_blocked = true;
                finish_item(result);
            } else {
                const auto target = bounce_target();
                s.last_reaction = State::LastReaction{m->id, defender, target, WeaponReaction::bounce, retained, defense_plan.free_card};
                ++s.reaction_count;
                s.pending.source = defender;
                s.pending.target = target;
                result.bounced = result.magic_bounced = true;
                result.bounce_self = target == defender;
                if (target == defender) resolve(0, result);
                else s.actor = target;
            }
        } else {
            const auto card_id = p.hand[a.base];
            const bool magic = find_magic_reflection_card(card_id) != nullptr;
            const bool any = find_any_reflection_card(card_id) != nullptr;
            const bool magic_bounce = snapshot_.config.magic_bounce && find_magic_bounce_card(card_id);
            const auto reaction = magic_bounce ? WeaponReaction::bounce
                : ((magic || any) ? WeaponReaction::reflect : find_reaction_card(card_id)->reaction);
            const auto slots = static_cast<std::uint16_t>(a.slots | (1u << a.base));
            p.pending_deals += std::popcount(slots);
            consume(s.actor, slots);
            const auto defender = s.actor;
            const auto target = reaction == WeaponReaction::reflect ? s.pending.source : bounce_target();
            s.last_reaction = State::LastReaction{card_id, defender, target, reaction};
            ++s.reaction_count;
            // Earlier ordinary armor was consumed, but the historical reaction
            // returns the original attack object, not the reduced local clone.
            s.pending.source = defender;
            s.pending.target = target;
            result.reflected = reaction == WeaponReaction::reflect;
            result.magic_reflected = magic;
            result.magic_bounced = result.magic_card_bounced = magic_bounce;
            if (magic_bounce) result.magic_bounce_card = card_id;
            result.any_reflected = any;
            result.bounced = reaction == WeaponReaction::bounce;
            result.bounce_self = result.bounced && target == defender;
            if (any && s.players[target].dead) {
                // The item was already popped/launched. Ending it here must
                // not count as another queue skip or as an applied effect.
                result.counter_cancelled = s.pending.counter_card != 0;
                finish_item(result);
            } else if (target == defender) resolve(0, result);
            else s.actor = target;
        }
    } else if (a.kind == ActionKind::buy) {
        const auto id = p.hand[a.base];
        ++p.pending_deals;
        consume(s.actor, static_cast<std::uint16_t>(1u << a.base));
        result.buy_used = true;
        result.buy_source = s.actor;
        result.buy_target = a.target;
        s.pending = Pending{Effect::buy, s.actor, a.target, 0};
        s.pending.element = Element::none;
        s.pending.buy_card = id;
        launch_pending(id, result);
    } else if (a.kind == ActionKind::sell) {
        CardId item = 0;
        int price = 0;
        prepare_sale(a, item, price);
        const auto id = p.hand[a.base];
        ++p.pending_deals;
        consume(s.actor, static_cast<std::uint16_t>(a.slots | (1u << a.base)));
        result.sale_used = true;
        result.sale_card = id;
        result.sale_item = item;
        result.sale_price = price;
        result.sale_source = s.actor;
        result.sale_target = a.target;
        s.pending = Pending{Effect::sell, s.actor, a.target, price};
        s.pending.element = Element::none;
        s.pending.sale_card = id;
        s.pending.sale_item = item;
        launch_pending(id, result);
    } else if (a.kind == ActionKind::exchange) {
        LastExchange record;
        prepare_exchange(a, record);
        result.exchanged = true;
        for (int i = 0; i < 3; ++i) {
            result.exchange_before[i] = record.before[i];
            result.exchange_after[i] = record.after[i];
        }
        s.last_exchange = record;
        p.hp = record.after[0];
        p.mp = record.after[1];
        p.money = record.after[2];
        ++p.pending_deals;
        consume(s.actor, static_cast<std::uint16_t>(1u << a.base));
        finish_item(result);
    } else if (a.kind == ActionKind::pray) {
        ++p.pending_deals;
        finish_item(result);
    } else if (a.kind == ActionKind::discard) {
        auto discarded = a.slots;
        if (snapshot_.config.pestle_mortar) for (int slot = 0; slot < p.hand_count; ++slot)
            if ((discarded & (1u << slot)) && find_mortar_card(p.hand[slot])) {
                discarded &= static_cast<std::uint16_t>(~(1u << slot));
                ++result.mortar_discards_ignored;
            }
        consume(s.actor, discarded); // Discard does not earn replacement gifts.
        finish_item(result);
    } else if (a.kind == ActionKind::attack && find_pestle_card(p.hand[a.base])) {
        const auto card = *find_pestle_card(p.hand[a.base]);
        const auto source = s.actor;
        ++p.pending_deals;
        consume(source, static_cast<std::uint16_t>(1u << a.base));
        std::uint8_t target = no_slot;
        for (int seat = 0; seat < 2; ++seat) if (!s.players[seat].dead) {
            const auto& candidate = s.players[seat];
            if (std::any_of(candidate.hand.begin(), candidate.hand.begin() + candidate.hand_count,
                [](CardId id) { return find_mortar_card(id) != nullptr; })) { target = static_cast<std::uint8_t>(seat); break; }
        }
        const bool locked = target != no_slot;
        if (!locked) target = snapshot_.pestle_target_index < snapshot_.pestle_target_count
            ? snapshot_.scripted_pestle_targets[snapshot_.pestle_target_index++]
            : static_cast<std::uint8_t>(bounded(snapshot_.rng_state, 2));
        const CardId mortar = locked ? mortar_catalog()[0].card.id : 0;
        const int amount = locked ? card.mortar_amount : card.card.attack;
        s.last_pestle = {card.card.id, source, target, mortar, amount};
        result.pestle_attack = true;
        result.pestle_target = target;
        result.pestle_mortar = locked;
        result.pestle_damage = static_cast<std::uint16_t>(amount);
        s.pending = Pending{Effect::damage, source, target, amount};
        s.pending.element = card.element;
        s.pending.pestle_card = card.card.id;
        s.pending.mortar_card = mortar;
        launch_pending(card.card.id, result);
    } else if (a.kind == ActionKind::cast_ordered) {
        FreeCastPlan plan;
        prepare_free_cast(a, plan);
        const auto& record = plan.record;
        const auto m = spell(record.magic);
        const bool held = record.from_ability;
        if (!held) {
            if (const auto si = status_miracle_index(m.id); si >= 0) ++p.status_abilities[si];
            else if (const auto ai = additive_miracle_index(m.id); ai >= 0) ++p.additive_abilities[ai];
            else ++p.abilities[spell_index(m.id)];
        }
        if (find_additive_miracle(m.id)) {
            result.additive_first_uses = !held;
            result.additive_recasts = held;
        } else {
            result.spell_first_use = !held;
            result.spell_recast = held;
        }
        if (find_status_miracle(m.id)) {
            result.status_first_use = !held; result.status_recast = held;
            result.status_miracle_card = m.id; result.status_source = s.actor; result.status_target = a.target;
        }
        result.free_cast = true;
        result.magic_free_cards = record.count;
        result.magic_free_mp_saved = static_cast<std::uint16_t>(m.cost);
        p.pending_deals += std::popcount(plan.slots) + (held ? 1u : 0u);
        p.retained_deals += held;
        consume(s.actor, plan.slots);
        s.free_cast = record;
        s.pending = Pending{m.effect, s.actor, a.target, m.amount, m.id, held};
        s.pending.element = m.element;
        s.pending.harm = m.harm;
        s.pending.disease = m.disease;
        s.pending.cure = m.cure;
        s.pending.hp_absorption_card = find_hp_absorption_miracle(m.id) ? m.id : 0;
        s.pending.free_card = record.items[0];
        launch_pending(m.id, result);
    } else if (a.kind == ActionKind::attack_ordered) {
        AttackComposition record;
        CompositionPlan plan;
        std::uint16_t slots = 0;
        prepare_ordered(a, record, plan, slots); // legal() validated before mutation.
        const auto id = record.base;
        s.attack_composition = record;
        p.additive_abilities = plan.acquired;
        p.doubling_ability = plan.acquired_doubling;
        p.wide_ability = plan.acquired_wide;
        p.mp -= plan.cost;
        s.pending = Pending{Effect::damage, s.actor, a.target, plan.amount};
        s.pending.element = plan.element;
        s.pending.hp_absorption_card = find_hp_absorption_card(record.base) ? record.base : 0;
        s.pending.self_damage_card = find_self_damage_card(record.base) ? record.base : 0;
        s.pending.wide_miracle = plan.wide ? wide_miracles()[0].id : 0;
        s.pending.attribute_card = plan.setter;
        result.magic_free_cards = plan.free_cards;
        result.magic_free_mp_saved = plan.mp_saved;
        result.ordered_attack = true;
        result.ordered_components = record.count;
        if (snapshot_.config.sundry_boost)
            for (int i = 0; i < record.count; ++i)
                if (find_sundry_boost_card(record.items[i])) ++result.sundry_boost_cards_used;
        result.additive_first_uses = plan.first_uses;
        result.additive_recasts = plan.additive_recasts;
        result.additive_mp_spent = static_cast<std::uint16_t>(plan.additive_cost);
        result.doubling_first_uses = plan.doubling_first_uses;
        result.doubling_recasts = plan.doubling_recasts;
        result.doubling_mp_spent = static_cast<std::uint16_t>(plan.doubling_cost);
        result.wide_first_uses = plan.wide_first_uses;
        result.wide_recasts = plan.wide_recasts;
        result.wide_mp_spent = static_cast<std::uint16_t>(plan.wide_cost);
        result.wide_attack = plan.wide;
        if (plan.setter) {
            result.attribute_attack = true;
            result.attribute_cards_used = plan.setters;
            result.attribute_element = plan.element;
        }
        if (find_mana_weapon_card(id)) {
            p.mp = 0;
            s.pending.mana_weapon = id;
            s.pending.mana_spent = plan.mana_spent;
            result.mana_weapon_attack = true;
            result.mana_weapon_mp_spent = plan.mana_spent;
            result.mana_weapon_power = static_cast<std::uint16_t>(plan.amount);
        }
        if (const auto* status = find_disease_card(id)) {
            s.pending.status_card = id;
            s.pending.disease = status->disease;
            s.pending.cure = status->cure;
        }
        if (const auto* harm = find_harm_card(id)) {
            s.pending.harm_card = id;
            s.pending.harm = harm->harm;
        }
        if (const auto* celestial = find_celestial_card(id); celestial && celestial->harm != Harm::none) {
            s.pending.harm_card = id;
            s.pending.harm = celestial->harm;
        }
        p.pending_deals += plan.physical + plan.held;
        p.retained_deals += plan.held;
        consume(s.actor, slots);
        launch_pending(id, result);
    } else if (a.kind == ActionKind::use && snapshot_.config.hp_gamble && find_hp_gamble_card(p.hand[a.base])) {
        const auto& card = *find_hp_gamble_card(p.hand[a.base]);
        const auto id = card.card.id;
        const auto source = s.actor;
        ++p.pending_deals;
        consume(source, static_cast<std::uint16_t>(1u << a.base));
        const auto roll = snapshot_.hp_gamble_index < snapshot_.hp_gamble_count
            ? snapshot_.scripted_hp_gambles[snapshot_.hp_gamble_index++] : bounded(snapshot_.rng_state, 2);
        const int delta = roll ? card.amount : -card.amount;
        s.last_hp_gamble = {id, source, a.target, delta, false, no_slot, no_slot, 0, s.turns + 1};
        result.hp_gamble_used = true;
        result.hp_gamble_card = id;
        result.hp_gamble_delta = delta;
        result.hp_gamble_source = source;
        result.hp_gamble_target = a.target;
        s.pending = Pending{Effect::hp_gamble, source, a.target, delta};
        s.pending.element = Element::none;
        s.pending.hp_gamble_card = id;
        launch_pending(id, result);
    } else if (a.kind == ActionKind::use && find_item_removal_card(p.hand[a.base])) {
        const auto id = p.hand[a.base];
        ++p.pending_deals;
        consume(s.actor, static_cast<std::uint16_t>(1u << a.base));
        s.pending = Pending{Effect::remove_item, s.actor, a.target, 0};
        s.pending.element = Element::none;
        s.pending.item_removal_card = id;
        launch_pending(id, result);
    } else if (a.kind == ActionKind::use && find_forgetting_card(p.hand[a.base])) {
        const auto id = p.hand[a.base];
        ++p.pending_deals;
        consume(s.actor, static_cast<std::uint16_t>(1u << a.base));
        s.pending = Pending{Effect::forget, s.actor, a.target, 0};
        s.pending.element = Element::none;
        s.pending.forget_card = id;
        launch_pending(id, result);
    } else if (a.kind == ActionKind::cast || a.kind == ActionKind::cast_additive || a.kind == ActionKind::cast_status || !artifact(p.hand[a.base])) {
        const bool from_ability = a.kind == ActionKind::cast || a.kind == ActionKind::cast_additive || a.kind == ActionKind::cast_status;
        const auto m = spell(a.kind == ActionKind::cast_status ? status_miracles()[a.base].id
            : (a.kind == ActionKind::cast_additive ? additive_miracles()[a.base].id
                : (from_ability ? spell_id(a.base) : p.hand[a.base])));
        p.mp -= m.cost;
        if (!from_ability) {
            if (const auto si = status_miracle_index(m.id); si >= 0) ++p.status_abilities[si];
            else if (const auto ai = additive_miracle_index(m.id); ai >= 0) ++p.additive_abilities[ai];
            else ++p.abilities[spell_index(m.id)];
            consume(s.actor, static_cast<std::uint16_t>(1u << a.base));
        }
        if (find_additive_miracle(m.id)) {
            result.additive_first_uses = !from_ability;
            result.additive_recasts = from_ability;
            result.additive_mp_spent = static_cast<std::uint16_t>(m.cost);
        } else {
            result.spell_first_use = !from_ability;
            result.spell_recast = from_ability;
            result.spell_mp_spent = static_cast<std::uint16_t>(m.cost);
        }
        if (find_status_miracle(m.id)) {
            result.status_first_use = !from_ability; result.status_recast = from_ability;
            result.status_miracle_card = m.id; result.status_source = s.actor; result.status_target = a.target;
        }
        ++p.pending_deals;
        if (from_ability && tracks_retained_deals(snapshot_.config)) ++p.retained_deals;
        s.pending = Pending{m.effect, s.actor, a.target, m.amount, m.id, from_ability};
        s.pending.element = m.element;
        s.pending.harm = m.harm;
        s.pending.disease = m.disease;
        s.pending.cure = m.cure;
        s.pending.hp_absorption_card = find_hp_absorption_miracle(m.id) ? m.id : 0;
        launch_pending(m.id, result);
    } else {
        const auto& base = *artifact(p.hand[a.base]);
        if (snapshot_.config.dying_weapon && weapon_attack(a.kind) && find_dying_weapon_card(base.id)) result.dying_weapon_attack = true;
        if (snapshot_.config.magic_free && find_magic_free_card(base.id)) result.magic_free_cards = 1;
        const auto* mana = snapshot_.config.mana_weapon && weapon_attack(a.kind) ? find_mana_weapon_card(base.id) : nullptr;
        const auto spent = static_cast<std::uint8_t>(mana ? p.mp : 0);
        int amount = mana ? mana->multiplier * spent : (weapon_attack(a.kind) ? base.attack : (base.hp ? base.hp : base.mp));
        const auto slots = static_cast<std::uint16_t>(a.slots | (1u << a.base));
        Element element = element_of(base.id);
        if (weapon_attack(a.kind))
            for (int i = 0; i < p.hand_count; ++i)
                if (a.slots & (1u << i)) {
                    amount += artifact(p.hand[i])->boost;
                    if (snapshot_.config.sundry_boost && find_sundry_boost_card(p.hand[i])) ++result.sundry_boost_cards_used;
                    element = combine(element, element_of(p.hand[i]));
                }
        const auto setter = attribute_component(a);
        if (setter) {
            element = attribute_result(a.kind);
            result.attribute_attack = true;
            result.attribute_element = element;
            for (int i = 0; i < p.hand_count; ++i)
                if ((a.slots & (1u << i)) && find_attribute_change_card(p.hand[i])) ++result.attribute_cards_used;
        }
        s.pending = Pending{weapon_attack(a.kind) ? Effect::damage : (base.hp ? Effect::heal : Effect::mana),
                            s.actor, a.target, amount};
        s.pending.element = element;
        s.pending.attribute_card = setter;
        s.pending.hp_absorption_card = find_hp_absorption_card(base.id) ? base.id : 0;
        s.pending.self_damage_card = find_self_damage_card(base.id) ? base.id : 0;
        if (mana) {
            p.mp = 0;
            s.pending.mana_weapon = base.id;
            s.pending.mana_spent = spent;
            result.mana_weapon_attack = true;
            result.mana_weapon_mp_spent = spent;
            result.mana_weapon_power = static_cast<std::uint16_t>(amount);
        }
        if (const auto* status = find_disease_card(base.id)) {
            s.pending.status_card = status->card.id;
            s.pending.disease = status->disease;
            s.pending.cure = status->cure;
            if (status->cure != Cure::none) { s.pending.kind = Effect::cure; s.pending.amount = 0; }
        }
        if (const auto* harm = find_harm_card(base.id)) {
            s.pending.harm_card = harm->card.id;
            s.pending.harm = harm->harm;
        }
        if (const auto* celestial = find_celestial_card(base.id); celestial && celestial->harm != Harm::none) {
            s.pending.harm_card = celestial->card.id;
            s.pending.harm = celestial->harm;
        }
        p.pending_deals += std::popcount(slots);
        consume(s.actor, slots);
        launch_pending(base.id, result);
    }
    if (!s.terminated && s.decisions >= snapshot_.config.max_decisions) s.truncated = true;
    result.terminated = s.terminated;
    result.truncated = s.truncated;
    return result;
}
Observation Env::observe() const noexcept {
    Observation obs{};
    const auto& s = state();
    const auto& me = s.players[s.actor];
    const auto& other = s.players[1 - s.actor];
    obs[0] = me.hp / 99.f; obs[1] = me.mp / 99.f; obs[2] = me.money / 99.f;
    obs[3] = other.hp / 99.f; obs[4] = other.mp / 99.f; obs[5] = other.money / 99.f;
    obs[6] = s.phase == Phase::main; obs[7] = s.phase == Phase::response;
    obs[8] = s.pending.amount / 480.f;
    obs[9] = static_cast<float>(s.decisions) / snapshot_.config.max_decisions;
    obs[10] = s.actor == s.turn_owner;
    obs[11] = other.hand_count / 16.f;
    if (s.pending.kind <= Effect::mana) obs[12 + static_cast<int>(s.pending.kind)] = 1.f;
    obs[128] = s.pending.kind == Effect::money;
    for (int i = 0; i < miracle_count; ++i)
        obs[129 + i] = static_cast<float>(me.abilities[i]) / snapshot_.config.max_decisions;
    obs[131] = snapshot_.config.miracles;
    if (snapshot_.config.elements && s.pending.element <= Element::stone)
        obs[164 + static_cast<int>(s.pending.element)] = 1.f;
    obs[185] = snapshot_.config.elements;
    obs[202] = s.pending.hit_rate / 100.f;
    obs[203] = snapshot_.config.chance;
    obs[226] = snapshot_.config.reflection;
    obs[227] = s.reaction_count / 32.f;
    if (snapshot_.config.reflection && s.pending.kind != Effect::none) obs[228] = s.pending.source == s.turn_owner;
    if (s.last_reaction.id) {
        obs[229] = 1.f;
        obs[230] = s.last_reaction.id / 363.f;
        obs[231] = s.last_reaction.reaction == WeaponReaction::reflect;
        obs[232] = s.last_reaction.reaction == WeaponReaction::bounce;
        obs[233] = s.last_reaction.source == s.actor;
        obs[234] = s.last_reaction.target == s.actor;
        obs[235] = s.last_reaction.source == s.last_reaction.target;
    }
    if (snapshot_.config.disease) {
        obs[236 + static_cast<int>(me.disease)] = 1.f;
        obs[241 + static_cast<int>(other.disease)] = 1.f;
    }
    obs[246] = static_cast<int>(s.pending.disease) / 4.f;
    obs[247] = static_cast<int>(s.pending.cure) / 2.f;
    obs[248] = snapshot_.config.disease;
    if (s.last_tick.seat != no_slot) {
        obs[249] = 1.f;
        obs[250] = s.last_tick.worsened;
        obs[251] = s.last_tick.seat == s.actor;
        obs[252] = static_cast<int>(s.last_tick.before) / 4.f;
        obs[253] = static_cast<int>(s.last_tick.after) / 4.f;
        obs[254] = s.last_tick.damage / 99.f;
        obs[255] = s.last_tick.healing / 99.f;
    }
    obs[256] = s.pending.status_card / 363.f;
    obs[257] = s.pending.kind == Effect::cure;
    if (snapshot_.config.harms) {
        obs[258] = me.glory; obs[259] = me.dark_cloud;
        obs[260] = other.glory; obs[261] = other.dark_cloud;
        obs[262] = 1.f;
        if (s.phase == Phase::response && !done()) obs[263] = (me.glory ? 1.f : 16.f) / 16.f;
    }
    obs[264] = s.last_chance.forced;
    obs[265] = static_cast<int>(s.pending.harm) / 2.f;
    obs[266] = s.pending.harm_card / 363.f;
    if (snapshot_.config.celestial) {
        obs[283] = 1.f;
        obs[284] = static_cast<int>(s.pending.element) / 6.f;
    }
    if (snapshot_.config.offensive_miracles) {
        obs[301] = 1.f;
        obs[302] = s.pending.miracle != 0;
        obs[303] = s.pending.miracle / 363.f;
        obs[304] = s.pending.from_ability;
        for (int i = 0; i < offensive_miracle_count; ++i)
            obs[305 + i] = static_cast<float>(me.abilities[miracle_count + i]) / snapshot_.config.max_decisions;
    }
    if (snapshot_.config.defensive_miracles) {
        obs[334] = 1.f;
        for (int i = 0; i < defensive_miracle_count; ++i)
            obs[335 + i] = static_cast<float>(me.defensive_abilities[i]) / snapshot_.config.max_decisions;
        obs[353] = s.last_reaction.from_ability;
        obs[354] = s.last_reaction.reaction == WeaponReaction::block;
        obs[355] = static_cast<float>(s.reaction_count) / snapshot_.config.max_decisions;
    }
    if (snapshot_.config.attribute_change) {
        obs[356] = 1.f;
        obs[373] = s.pending.attribute_card / 363.f;
        obs[374] = s.pending.attribute_card != 0;
    }
    if (snapshot_.config.attribute_removal) {
        obs[375] = 1.f;
        obs[392] = s.pending.attribute_removed;
    }
    if (snapshot_.config.counters) {
        obs[393] = 1.f;
        obs[410] = s.pending.counter_card / 363.f;
        if (s.pending.kind != Effect::none) obs[411] = s.pending.origin_source == s.actor;
        obs[412] = me.dead; obs[413] = other.dead;
        obs[414] = s.counter_count / static_cast<float>(counter_queue_capacity);
        obs[415] = s.counter_generated / static_cast<float>(counter_queue_capacity);
        obs[416] = s.counter_started / static_cast<float>(counter_queue_capacity);
        obs[417] = s.pending.counter_card != 0;
        if (s.pending.counter_card) obs[418] = static_cast<float>(s.pending.amount) / counter_damage_limit;
        for (int i = 0; i < s.counter_count; ++i) {
            const auto& item = s.counter_queue[i];
            obs[419 + i * 3] = item.source == s.actor;
            obs[420 + i * 3] = item.target == s.actor;
            obs[421 + i * 3] = item.amount / 480.f;
        }
    }
    if (snapshot_.config.counter_extensions) {
        obs[515] = 1.f;
        obs[532] = s.pending.kind == Effect::harm;
        for (int i = 0; i < s.counter_count; ++i) {
            const auto& item = s.counter_queue[i];
            obs[533 + i * 4] = item.counter_card / 363.f;
            obs[534 + i * 4] = static_cast<int>(item.kind) / 6.f;
            obs[535 + i * 4] = item.hit_rate / 100.f;
            obs[536 + i * 4] = static_cast<int>(item.harm) / 2.f;
        }
    }
    if (snapshot_.config.money_counter) {
        obs[661] = 1.f;
        obs[678] = s.pending.kind == Effect::absorb_money;
    }
    if (snapshot_.config.magic_reflection) obs[679] = 1.f;
    if (snapshot_.config.any_reflection) obs[696] = 1.f;
    if (snapshot_.config.mana_weapon) {
        obs[713] = 1.f;
        obs[730] = s.pending.mana_weapon != 0;
        obs[731] = s.pending.mana_spent / 99.f;
    }
    if (snapshot_.config.magic_block) {
        obs[732] = 1.f;
        obs[749] = s.last_reaction.reaction == WeaponReaction::block && find_magic_block_card(s.last_reaction.id);
    }
    if (snapshot_.config.additive_miracles) {
        obs[750] = 1.f;
        for (int i = 0; i < additive_miracle_count; ++i)
            obs[767 + i] = static_cast<float>(me.additive_abilities[i]) / (15.0f * snapshot_.config.max_decisions);
        const auto& record = s.attack_composition;
        if (record.base) {
            CompositionPlan plan;
            replay_composition(record, plan);
            obs[769] = 1.f;
            obs[770] = record.base / 363.f;
            obs[771] = record.count / 64.f;
            obs[772] = record.initial_mp / 99.f;
            obs[773] = plan.cost / 99.f;
            for (int i = 0; i < std::min<int>(record.count, legacy_attack_extra_capacity); ++i) {
                obs[775 + 2 * i] = record.items[i] / 363.f;
                obs[776 + 2 * i] = held_at(record, i);
            }
        }
        if (const auto ai = additive_miracle_index(s.pending.miracle); ai >= 0) obs[774] = (ai + 1) / 2.f;
    }
    if (snapshot_.config.doubling_miracles) {
        obs[903] = 1.f;
        obs[920] = static_cast<float>(me.doubling_ability) / (15.0f * snapshot_.config.max_decisions);
        if (s.attack_composition.base) {
            int count = 0;
            for (int i = 0; i < s.attack_composition.count; ++i)
                count += find_doubling_miracle(s.attack_composition.items[i]) != nullptr;
            obs[921] = count / 16.f;
        }
        obs[922] = static_cast<float>(s.pending.amount) / doubling_damage_limit;
        if (s.pending.counter_card) obs[923] = static_cast<float>(s.pending.amount) / doubling_counter_damage_limit;
    }
    if (snapshot_.config.wide_miracles) {
        obs[924] = 1.f;
        obs[941] = static_cast<float>(me.wide_ability) / (15.0f * snapshot_.config.max_decisions);
        obs[942] = s.pending.wide_miracle != 0;
        obs[943] = s.last_chance.wide_miracle != 0;
        int count = 0;
        if (s.attack_composition.base)
            for (int i = 0; i < s.attack_composition.count; ++i)
                count += find_wide_miracle(s.attack_composition.items[i]) != nullptr;
        obs[944] = count / 19.f;
    }
    if (snapshot_.config.magic_free) {
        obs[945] = 1.f;
        obs[962] = s.pending.free_card / 363.f;
        obs[963] = s.last_reaction.free_card / 363.f;
        if (s.attack_composition.base) {
            CompositionPlan plan;
            replay_composition(s.attack_composition, plan);
            obs[964] = plan.free_index < 0 ? 0.f : (plan.free_index + 1) / 66.f;
            obs[965] = plan.free_card / 363.f;
            obs[966] = s.attack_composition.items[64] / 363.f;
            obs[967] = s.attack_composition.held_tail;
            obs[972] = s.attack_composition.count / 65.f;
            obs[973] = plan.mp_saved / 99.f;
        }
        obs[968] = static_cast<float>(s.pending.amount) / magic_free_damage_limit;
        if (s.pending.counter_card) obs[969] = static_cast<float>(s.pending.amount) / magic_free_counter_damage_limit;
        obs[970] = static_cast<float>(me.pending_deals) / (static_cast<double>(snapshot_.config.max_decisions) + 65);
        obs[971] = static_cast<float>(me.retained_deals) / (static_cast<double>(snapshot_.config.max_decisions) + 50);
        obs[974] = s.free_cast.magic != 0;
        obs[975] = s.free_cast.count / 16.f;
        obs[976] = s.free_cast.magic / 363.f;
        for (int i = 0; i < hand_capacity; ++i) obs[977 + i] = s.free_cast.items[i] / 363.f;
    }
    if (snapshot_.config.forgetting) {
        obs[993] = 1.f;
        obs[1010] = s.pending.kind == Effect::forget;
        if (s.last_forget.id) {
            obs[1011] = 1.f;
            obs[1012] = s.last_forget.source == s.actor;
            obs[1013] = s.last_forget.target == s.actor;
            obs[1014] = s.last_forget.removed / 363.f;
            obs[1015] = s.last_forget.removed != 0;
        }
        obs[1016] = static_cast<float>(retained_total(me)) / (16.0 * snapshot_.config.max_decisions);
    }
    if (snapshot_.config.exchanges) {
        obs[1017] = 1.f;
        if (s.last_exchange.id) {
            obs[1034] = 1.f;
            obs[1035] = s.last_exchange.seat == s.actor;
            int total = 0;
            for (int i = 0; i < 3; ++i) {
                obs[1036 + i] = s.last_exchange.before[i] / 99.f;
                obs[1039 + i] = s.last_exchange.after[i] / 99.f;
                total += s.last_exchange.after[i];
            }
            obs[1042] = total / 297.f;
        }
    }
    if (snapshot_.config.dying_weapon) {
        obs[1345] = 1.f;
        for (int i = 0; i < me.hand_count; ++i) obs[1346 + i] = find_dying_weapon_card(me.hand[i]) != nullptr;
        obs[1362] = s.phase == Phase::response && s.pending.hit_rate && s.last_chance.id == 117
            && !s.pending.miracle && !s.pending.counter_card;
        if (s.last_chance.id == 117) {
            obs[1363] = 1.f; obs[1364] = s.last_chance.hit; obs[1365] = s.last_chance.forced;
            obs[1366] = s.last_chance.source == s.actor; obs[1367] = s.last_chance.target == s.actor;
        }
    }
    if (snapshot_.config.status_miracles) {
        obs[1317] = 1.f;
        for (int i = 0; i < me.hand_count; ++i) {
            const auto index = status_miracle_index(me.hand[i]);
            if (index >= 0) obs[1318 + i] = (index + 1) / 5.f;
        }
        for (int i = 0; i < status_miracle_count; ++i) obs[1334 + i] = static_cast<float>(me.status_abilities[i]) / snapshot_.config.max_decisions;
        if (s.pending.kind == Effect::status) {
            obs[1339] = 1.f; obs[1340] = (status_miracle_index(s.pending.miracle) + 1) / 5.f;
            obs[1341] = static_cast<int>(s.pending.disease) / 4.f;
            obs[1342] = static_cast<int>(s.pending.cure) / 2.f;
            obs[1343] = static_cast<int>(s.pending.harm) / 2.f;
            obs[1344] = s.pending.from_ability;
        }
    }
    if (snapshot_.config.sundry_boost) {
        obs[1300] = 1.f;
        for (int i = 0; i < me.hand_count; ++i) obs[1301 + i] = find_sundry_boost_card(me.hand[i]) != nullptr;
    }
    if (snapshot_.config.magic_bounce) {
        obs[1278] = 1.f;
        for (int i = 0; i < me.hand_count; ++i) obs[1279 + i] = find_magic_bounce_card(me.hand[i]) != nullptr;
        if (find_magic_bounce_card(s.last_reaction.id)) {
            obs[1295] = 1.f;
            obs[1296] = s.last_reaction.id / 363.f;
            obs[1297] = s.last_reaction.source == s.actor;
            obs[1298] = s.last_reaction.target == s.actor;
            obs[1299] = s.last_reaction.source == s.last_reaction.target;
        }
    }
    if (snapshot_.config.hp_gamble) {
        obs[1251] = 1.f;
        for (int i = 0; i < me.hand_count; ++i) obs[1252 + i] = find_hp_gamble_card(me.hand[i]) != nullptr;
        obs[1268] = s.pending.kind == Effect::hp_gamble;
        if (s.pending.kind == Effect::hp_gamble) obs[1269] = s.pending.amount / 10.f;
        if (s.last_hp_gamble.id) {
            const auto& event = s.last_hp_gamble;
            obs[1270] = 1.f;
            obs[1271] = event.source == s.actor;
            obs[1272] = event.target == s.actor;
            obs[1273] = event.delta / 10.f;
            obs[1274] = event.resolved;
            obs[1275] = event.resolved_source == s.actor;
            obs[1276] = event.resolved_target == s.actor;
            obs[1277] = event.change / 10.f;
        }
    }
    if (snapshot_.config.buying) {
        obs[1221] = 1.f;
        obs[1238] = s.pending.kind == Effect::buy;
        obs[1239] = s.phase == Phase::purchase;
        if (s.phase == Phase::purchase) {
            obs[1240] = s.last_buy.item / 363.f;
            obs[1241] = static_cast<float>(s.last_buy.price) / max_sale_price;
        }
        if (s.last_buy.id) {
            const auto& buy = s.last_buy;
            obs[1242] = 1.f;
            obs[1243] = buy.source == s.actor;
            obs[1244] = buy.target == s.actor;
            obs[1245] = buy.item / 363.f;
            obs[1246] = static_cast<float>(buy.price) / max_sale_price;
            obs[1247] = static_cast<unsigned>(buy.outcome) / 6.f;
            obs[1248] = buy.delivered;
            obs[1249] = buy.money_lost / 99.f;
            obs[1250] = buy.money_gained / 99.f;
        }
    }
    if (snapshot_.config.selling) {
        obs[1175] = 1.f;
        obs[1208] = s.pending.kind == Effect::sell;
        obs[1209] = s.pending.sale_item / 363.f;
        if (s.pending.kind == Effect::sell) obs[1210] = static_cast<float>(s.pending.amount) / max_sale_price;
        if (s.last_sale.id) {
            const auto& sale = s.last_sale;
            obs[1211] = 1.f;
            obs[1212] = sale.source == s.actor;
            obs[1213] = sale.target == s.actor;
            obs[1214] = sale.item / 363.f;
            obs[1215] = static_cast<float>(sale.price) / max_sale_price;
            obs[1216] = sale.delivered;
            obs[1217] = sale.money_lost / 99.f;
            obs[1218] = sale.mp_lost / 99.f;
            obs[1219] = sale.hp_lost / 99.f;
            obs[1220] = sale.money_gained / 99.f;
        }
    }
    if (snapshot_.config.pestle_mortar) {
        obs[1135] = 1.f;
        obs[1168] = s.pending.pestle_card != 0;
        obs[1169] = s.pending.mortar_card != 0;
        if (s.last_pestle.id) {
            obs[1170] = 1.f;
            obs[1171] = s.last_pestle.source == s.actor;
            obs[1172] = s.last_pestle.target == s.actor;
            obs[1173] = s.last_pestle.mortar != 0;
            obs[1174] = s.last_pestle.amount / 999.f;
        }
    }
    if (snapshot_.config.self_damage) {
        obs[1110] = 1.f;
        obs[1127] = s.pending.self_damage_card / 363.f;
        if (s.last_self_damage.id) {
            obs[1128] = 1.f;
            obs[1129] = s.last_self_damage.source == s.actor;
            obs[1130] = s.last_self_damage.target == s.actor;
            obs[1131] = s.last_self_damage.id / 363.f;
            obs[1132] = static_cast<float>(s.last_self_damage.amount) / magic_free_damage_limit;
            obs[1133] = s.last_self_damage.source_damage / 99.f;
            obs[1134] = s.last_self_damage.target_damage / 99.f;
        }
    }
    if (snapshot_.config.hp_absorption) {
        obs[1085] = 1.f;
        obs[1102] = static_cast<float>(me.abilities[hp_absorption_ability_index]) / snapshot_.config.max_decisions;
        obs[1103] = s.pending.hp_absorption_card / 363.f;
        if (s.last_hp_absorption.id) {
            obs[1104] = 1.f;
            obs[1105] = s.last_hp_absorption.source == s.actor;
            obs[1106] = s.last_hp_absorption.target == s.actor;
            obs[1107] = s.last_hp_absorption.id / 363.f;
            obs[1108] = static_cast<float>(s.last_hp_absorption.amount) / magic_free_damage_limit;
            obs[1109] = s.last_hp_absorption.healing / 99.f;
        }
    }
    if (snapshot_.config.item_removal) {
        obs[1062] = 1.f;
        obs[1079] = s.pending.kind == Effect::remove_item;
        if (s.last_item_removal.id) {
            obs[1080] = 1.f;
            obs[1081] = s.last_item_removal.source == s.actor;
            obs[1082] = s.last_item_removal.target == s.actor;
            obs[1083] = s.last_item_removal.removed / 363.f;
            obs[1084] = s.last_item_removal.removed != 0;
        }
    }
    if (snapshot_.config.revival) {
        obs[1043] = 1.f;
        if (s.last_revival.id) {
            obs[1060] = 1.f;
            obs[1061] = s.last_revival.seat == s.actor;
        }
    }
    if (s.last_chance.id) {
        obs[204] = 1.f;
        obs[205] = s.last_chance.hit;
        obs[206] = s.last_chance.id / 363.f;
        obs[207] = s.last_chance.hit_rate / 100.f;
        obs[208] = s.last_chance.source == s.actor;
        obs[209] = s.last_chance.target == s.actor;
    }
    for (int i = 0; i < me.hand_count; ++i) {
        const int offset = 16 + i * 7;
        const auto* card = artifact(me.hand[i]);
        if (const auto* harm = find_harm_card(me.hand[i])) obs[267 + i] = static_cast<int>(harm->harm) / 2.f;
        if (const auto* celestial = find_celestial_card(me.hand[i]))
            obs[267 + i] = static_cast<int>(celestial->harm) / 2.f;
        if (const auto* m = find_offensive_miracle(me.hand[i])) {
            obs[267 + i] = static_cast<int>(m->harm) / 2.f;
            obs[318 + i] = m->amount / 30.f;
        }
        if (find_doubling_miracle(me.hand[i])) obs[904 + i] = 1.f;
        if (find_wide_miracle(me.hand[i])) obs[925 + i] = 1.f;
        if (find_forgetting_card(me.hand[i])) obs[994 + i] = 1.f;
        if (find_exchange_card(me.hand[i])) obs[1018 + i] = 1.f;
        if (find_revival_card(me.hand[i])) obs[1044 + i] = 1.f;
        if (find_item_removal_card(me.hand[i])) obs[1063 + i] = 1.f;
        if (find_self_damage_card(me.hand[i])) obs[1111 + i] = 1.f;
        if (find_buying_card(me.hand[i])) obs[1222 + i] = 1.f;
        if (snapshot_.config.selling) {
            if (find_selling_card(me.hand[i])) obs[1176 + i] = 1.f;
            obs[1192 + i] = static_cast<float>(trade_price(me.hand[i])) / max_sale_price;
        }
        if (find_pestle_card(me.hand[i])) obs[1136 + i] = 1.f;
        if (find_mortar_card(me.hand[i])) obs[1152 + i] = 1.f;
        if (const auto hi = hp_absorption_index(me.hand[i]); hi >= 0) obs[1086 + i] = (hi + 1) / 4.f;
        else if (find_hp_absorption_miracle(me.hand[i])) obs[1086 + i] = 1.f;
        if (const auto i_free = magic_free_index(me.hand[i]); i_free >= 0) obs[946 + i] = (i_free + 1) / 5.f;
        if (const auto ai = additive_miracle_index(me.hand[i]); ai >= 0) {
            obs[751 + i] = (ai + 1) / 2.f;
            obs[318 + i] = additive_miracles()[ai].amount / 30.f;
        }
        if (const auto* m = find_defensive_miracle(me.hand[i]))
            obs[337 + i] = (static_cast<int>(m->kind) + 1) / 2.f;
        if (const auto* setter = find_attribute_change_card(me.hand[i]))
            obs[357 + i] = static_cast<int>(setter->element) / 6.f;
        if (find_attribute_removal_card(me.hand[i])) obs[376 + i] = 1.f;
        if (find_counter_card(me.hand[i])) obs[394 + i] = 1.f;
        if (find_money_counter_card(me.hand[i])) {
            obs[394 + i] = 1.f;
            obs[662 + i] = 1.f;
        }
        if (const auto* c = find_counter_extension_card(me.hand[i])) {
            obs[394 + i] = 1.f;
            obs[516 + i] = (c - counter_extensions_catalog().data() + 1) / 4.f;
        }
        if (find_any_reflection_card(me.hand[i])) obs[697 + i] = 1.f;
        if (find_mana_weapon_card(me.hand[i])) obs[714 + i] = 1.f;
        if (const auto* c = find_magic_block_card(me.hand[i]))
            obs[733 + i] = (c - magic_block_catalog().data() + 1) / 8.f;
        if (const auto* c = find_magic_reflection_card(me.hand[i]))
            obs[680 + i] = (c - magic_reflection_catalog().data() + 1) / 4.f;
        if (const auto* reaction = find_reaction_card(me.hand[i]))
            obs[210 + i] = static_cast<int>(reaction->reaction) / 3.f;
        const auto element = element_of(me.hand[i]);
        if (element <= Element::stone) obs[169 + i] = static_cast<int>(element) / 4.f;
        if (snapshot_.config.celestial) obs[285 + i] = static_cast<int>(element) / 6.f;
        obs[186 + i] = chance_rate(me.hand[i]) / 100.f;
        if (!card) {
            const auto m = spell(me.hand[i]);
            obs[offset] = m.id / 363.f;
            if (m.effect == Effect::heal) obs[offset + 4] = m.amount / 20.f;
            obs[132 + i] = 1.f;
            obs[148 + i] = m.cost / 99.f;
            continue;
        }
        const auto& c = *card;
        obs[offset] = c.id / 363.f;
        const auto* mana = find_mana_weapon_card(c.id);
        obs[offset + 1] = (mana ? me.mp * mana->multiplier : c.attack) / 30.f;
        obs[offset + 2] = c.boost / 20.f;
        obs[offset + 3] = c.defense / 30.f;
        obs[offset + 4] = c.hp / 20.f;
        obs[offset + 5] = c.mp / 20.f;
        obs[offset + 6] = static_cast<int>(c.kind) / 3.f;
    }
    return obs;
}
Action Env::sample_action(std::uint64_t& policy_rng) const noexcept {
    if (done()) return {};
    const auto opts = options();
    if (state().phase == Phase::purchase)
        return {bounded(policy_rng, 2) ? ActionKind::buy_accept : ActionKind::buy_decline};
    if (state().phase == Phase::response) {
        if (snapshot_.config.magic_free && opts.magic_free_slots
            && (opts.free_defensive_slots || opts.free_defensive_ability_slots)) {
            std::array<Action, 288> choices{};
            unsigned count = 0;
            for (int base = 0; base < 22; ++base) {
                const bool candidate = base < 16 ? (opts.free_defensive_slots & (1u << base))
                    : (base >= 20 && (opts.free_defensive_ability_slots & (1u << (base - 20))));
                if (!candidate) continue;
                for (int free = 0; free < 16; ++free) if (opts.magic_free_slots & (1u << free)) {
                    Action trial{ActionKind::defend_ordered, no_slot, 0, no_slot};
                    trial.extra_count = 2; trial.extras[0] = static_cast<std::uint8_t>(base); trial.extras[1] = static_cast<std::uint8_t>(free);
                    if (legal(trial)) choices[count++] = trial;
                }
            }
            if (count && bounded(policy_rng, 2)) return choices[bounded(policy_rng, count)];
        }
        const auto complete_response = [&](Action action) noexcept {
            if (snapshot_.config.counter_extensions) {
                auto remaining = static_cast<std::uint16_t>(action.slots & opts.counter_slots);
                while (remaining) {
                    const auto slot = pick(remaining, policy_rng);
                    action.counter_order |= static_cast<std::uint64_t>(slot) << (4 * action.counter_count++);
                    remaining &= static_cast<std::uint16_t>(~(1u << slot));
                }
            }
            if ((!snapshot_.config.attribute_removal && !snapshot_.config.counter_extensions
                    && !snapshot_.config.magic_reflection && !snapshot_.config.any_reflection && !snapshot_.config.magic_block
                    && !snapshot_.config.magic_bounce) || legal(action)) return action;
            if (opts.removal_slots) {
                const auto removal = static_cast<std::uint16_t>(opts.removal_slots & -opts.removal_slots);
                auto with_removal = action;
                with_removal.slots |= removal;
                if (legal(with_removal)) return with_removal;
                if (snapshot_.config.any_reflection && action.kind == ActionKind::defend
                    && action.base < state().players[state().actor].hand_count
                    && find_any_reflection_card(state().players[state().actor].hand[action.base]))
                    return Action{ActionKind::defend, action.base, 0, no_slot};
                // Glory may permit the remover alone but not a 2-card response.
                const Action removal_only{ActionKind::defend, no_slot, removal, no_slot};
                if (legal(removal_only)) return removal_only;
            }
            if (snapshot_.config.any_reflection && action.kind == ActionKind::defend
                && action.base < state().players[state().actor].hand_count
                && find_any_reflection_card(state().players[state().actor].hand[action.base]))
                return Action{ActionKind::defend, action.base, 0, no_slot};
            return Action{ActionKind::defend, no_slot, 0, no_slot};
        };
        if (opts.defensive_ability_slots && bounded(policy_rng, 2))
            return complete_response({ActionKind::defend_cast, pick(opts.defensive_ability_slots, policy_rng),
                    opts.defense_limit == 1 ? std::uint16_t{0}
                        : static_cast<std::uint16_t>(random(policy_rng) & opts.defense_slots), no_slot});
        if (opts.reaction_slots && bounded(policy_rng, 2))
            return complete_response({ActionKind::defend, pick(opts.reaction_slots, policy_rng),
                    opts.defense_limit == 1 ? std::uint16_t{0}
                        : static_cast<std::uint16_t>(random(policy_rng) & opts.defense_slots), no_slot});
        auto ordinary_slots = opts.defense_slots;
        if ((snapshot_.config.defensive_miracles || snapshot_.config.magic_reflection || snapshot_.config.any_reflection || snapshot_.config.magic_block) && state().pending.miracle) {
            const auto& p = state().players[state().actor];
            for (int i = 0; i < p.hand_count; ++i)
                if (!(opts.removal_slots & (1u << i)) && !can_defend(state().pending.element, element_of(p.hand[i])))
                    ordinary_slots &= static_cast<std::uint16_t>(~(1u << i));
        }
        if (opts.defense_limit == 1) {
            const auto selected = bounded(policy_rng, std::popcount(ordinary_slots) + 1);
            auto mask = ordinary_slots;
            if (selected == 0) return complete_response({ActionKind::defend, no_slot, 0, no_slot});
            for (unsigned i = 1; i < selected; ++i) mask &= mask - 1;
            return complete_response({ActionKind::defend, no_slot, static_cast<std::uint16_t>(1u << std::countr_zero(mask)), no_slot});
        }
        return complete_response({ActionKind::defend, no_slot, static_cast<std::uint16_t>(random(policy_rng) & ordinary_slots), no_slot});
    }
    std::array<ActionKind, 11> kinds{};
    unsigned count = 0;
    if (opts.weapon_bases) kinds[count++] = ActionKind::attack;
    if (opts.utility_slots) kinds[count++] = ActionKind::use;
    if (opts.exchange_slots) kinds[count++] = ActionKind::exchange;
    if (opts.sell_slots) kinds[count++] = ActionKind::sell;
    if (opts.buy_slots) kinds[count++] = ActionKind::buy;
    if (opts.ability_slots) kinds[count++] = ActionKind::cast;
    if (opts.additive_ability_slots) kinds[count++] = ActionKind::cast_additive;
    if (opts.status_ability_slots) kinds[count++] = ActionKind::cast_status;
    if (opts.magic_free_slots && (opts.free_cast_slots || opts.free_ability_slots || opts.free_additive_ability_slots || opts.free_status_ability_slots)) kinds[count++] = ActionKind::cast_ordered;
    if (opts.discard_slots) kinds[count++] = ActionKind::discard;
    if (opts.pray) kinds[count++] = ActionKind::pray;
    const auto kind = kinds[bounded(policy_rng, count)];
    if (kind == ActionKind::buy) return {kind, pick(opts.buy_slots, policy_rng), 0, static_cast<std::uint8_t>(1 - state().actor)};
    if (kind == ActionKind::sell) {
        const auto base = pick(opts.sell_slots, policy_rng);
        const auto goods = static_cast<std::uint16_t>(opts.sale_item_slots & ~(1u << base));
        return {kind, base, static_cast<std::uint16_t>(1u << pick(goods, policy_rng)), static_cast<std::uint8_t>(1 - state().actor)};
    }
    if (kind == ActionKind::attack) {
        const auto base = pick(opts.weapon_bases, policy_rng);
        if (find_pestle_card(state().players[state().actor].hand[base]))
            return {ActionKind::attack, base, 0, static_cast<std::uint8_t>(1 - state().actor)};
        if (snapshot_.config.additive_miracles && !chance_rate(state().players[state().actor].hand[base])) {
            Action action{ActionKind::attack_ordered, base, 0, static_cast<std::uint8_t>(1 - state().actor)};
            auto physical = static_cast<std::uint16_t>((opts.boost_slots | opts.attribute_slots | opts.additive_slots | opts.doubling_slots | opts.wide_slots | opts.magic_free_slots) & ~(1u << base));
            if (snapshot_.config.magic_free) {
                const auto& p = state().players[state().actor];
                // Include unaffordable hand modifiers: an adjacent FREE can fund them.
                for (int i = 0; i < p.hand_count; ++i)
                    if (find_additive_miracle(p.hand[i]) || find_doubling_miracle(p.hand[i]) || find_wide_miracle(p.hand[i])) physical |= 1u << i;
                struct Choice { std::uint8_t ref, carrier; };
                for (int n = 0; n < attack_extra_capacity; ++n) {
                    std::array<Choice, 340> candidates{};
                    unsigned available = 0;
                    for (int ref = 0; ref < 20; ++ref) {
                        if (ref < 16 && !(physical & (1u << ref))) continue;
                        auto trial = action;
                        if (trial.extra_count >= attack_extra_capacity) continue;
                        trial.extras[trial.extra_count++] = static_cast<std::uint8_t>(ref);
                        if (legal(trial)) candidates[available++] = {static_cast<std::uint8_t>(ref), no_slot};
                        else if (trial.extra_count < attack_extra_capacity) {
                            for (int carrier = 0; carrier < 16; ++carrier) if ((physical & opts.magic_free_slots) & (1u << carrier)) {
                                auto paired = trial;
                                paired.extras[paired.extra_count++] = static_cast<std::uint8_t>(carrier);
                                if (legal(paired)) candidates[available++] = {static_cast<std::uint8_t>(ref), static_cast<std::uint8_t>(carrier)};
                            }
                        }
                    }
                    if (!available || (action.extra_count && bounded(policy_rng, 2))) break;
                    const auto choice = candidates[bounded(policy_rng, available)];
                    action.extras[action.extra_count++] = choice.ref;
                    if (choice.ref < 16) physical &= static_cast<std::uint16_t>(~(1u << choice.ref));
                    if (choice.carrier != no_slot) {
                        action.extras[action.extra_count++] = choice.carrier;
                        physical &= static_cast<std::uint16_t>(~(1u << choice.carrier));
                    }
                }
                if (action.extra_count) return action;
            }
            for (int n = 0; n < legacy_attack_extra_capacity; ++n) {
                std::array<std::uint8_t, 20> candidates{};
                unsigned available = 0;
                for (int ref = 0; ref < (snapshot_.config.wide_miracles ? 20 : 18 + snapshot_.config.doubling_miracles); ++ref) {
                    if (ref < 16 && !(physical & (1u << ref))) continue;
                    auto trial = action;
                    trial.extras[trial.extra_count++] = static_cast<std::uint8_t>(ref);
                    if (legal(trial)) candidates[available++] = static_cast<std::uint8_t>(ref);
                }
                if (!available || (action.extra_count && bounded(policy_rng, 2))) break;
                const auto ref = candidates[bounded(policy_rng, available)];
                action.extras[action.extra_count++] = ref;
                if (ref < 16) physical &= static_cast<std::uint16_t>(~(1u << ref));
            }
            if (action.extra_count) return action;
        }
        const auto attributes = static_cast<std::uint16_t>(opts.attribute_slots & ~(1u << base));
        if (snapshot_.config.attribute_change && attributes
            && !chance_rate(state().players[state().actor].hand[base]) && bounded(policy_rng, 2)) {
            auto extras = static_cast<std::uint16_t>(random(policy_rng)
                & (opts.boost_slots | opts.attribute_slots) & ~(1u << base));
            if (!(extras & attributes)) extras |= 1u << pick(attributes, policy_rng);
            std::uint16_t outcomes = 0;
            for (int i = 0; i < 3; ++i) {
                const Action action{static_cast<ActionKind>(static_cast<int>(ActionKind::attack_set_fire) + i),
                                    base, extras, static_cast<std::uint8_t>(1 - state().actor)};
                if (attribute_component(action)) outcomes |= 1u << i;
            }
            return {static_cast<ActionKind>(static_cast<int>(ActionKind::attack_set_fire) + pick(outcomes, policy_rng)),
                    base, extras, static_cast<std::uint8_t>(1 - state().actor)};
        }
        const auto extras = chance_rate(state().players[state().actor].hand[base]) ? std::uint16_t{0}
            : static_cast<std::uint16_t>(random(policy_rng) & opts.boost_slots & ~(1u << base));
        return {kind, base, extras,
                static_cast<std::uint8_t>(1 - state().actor)};
    }
    if (kind == ActionKind::exchange) {
        const auto& p = state().players[state().actor];
        const int total = p.hp + p.mp + p.money;
        unsigned ways = 0;
        for (int hp = 0; hp <= 99; ++hp) {
            const int low = std::max(0, total - hp - 99), high = std::min(99, total - hp);
            if (low <= high) ways += static_cast<unsigned>(high - low + 1);
        }
        auto ticket = bounded(policy_rng, ways);
        Action action{kind, pick(opts.exchange_slots, policy_rng), 0, no_slot};
        action.extra_count = 3;
        for (int hp = 0; hp <= 99; ++hp) {
            const int low = std::max(0, total - hp - 99), high = std::min(99, total - hp);
            if (low > high) continue;
            const auto count = static_cast<unsigned>(high - low + 1);
            if (ticket < count) {
                const int mp = low + static_cast<int>(ticket);
                action.extras[0] = static_cast<std::uint8_t>(hp);
                action.extras[1] = static_cast<std::uint8_t>(mp);
                action.extras[2] = static_cast<std::uint8_t>(total - hp - mp);
                return action;
            }
            ticket -= count;
        }
        return action;
    }
    if (kind == ActionKind::cast_ordered) {
        std::array<std::uint8_t, hand_capacity + main_ability_capacity + additive_miracle_count + status_miracle_count> bases{};
        unsigned count = 0;
        for (int i = 0; i < 16; ++i) if (opts.free_cast_slots & (1u << i)) bases[count++] = static_cast<std::uint8_t>(i);
        for (int i = 0; i < additive_miracle_count; ++i) if (opts.free_additive_ability_slots & (1u << i)) bases[count++] = static_cast<std::uint8_t>(16 + i);
        for (int i = 0; i < main_ability_capacity; ++i) if (opts.free_ability_slots & (1u << i)) bases[count++] = static_cast<std::uint8_t>(32 + i);
        for (int i = 0; i < status_miracle_count; ++i) if (opts.free_status_ability_slots & (1u << i)) bases[count++] = static_cast<std::uint8_t>(48 + i);
        Action action{kind, bases[bounded(policy_rng, count)], 0, state().actor};
        const auto id = action.base < 16 ? state().players[state().actor].hand[action.base]
            : action.base < 18 ? additive_miracles()[action.base - 16].id
            : action.base >= 48 ? status_miracles()[action.base - 48].id : spell_id(action.base - 32);
        if (spell(id).effect == Effect::damage || (find_status_miracle(id) && spell(id).cure == Cure::none))
            action.target = static_cast<std::uint8_t>(1 - state().actor);
        action.extra_count = 1;
        action.extras[0] = pick(opts.magic_free_slots, policy_rng);
        return action;
    }
    if (kind == ActionKind::use) {
        const auto slot = pick(opts.utility_slots, policy_rng);
        if (snapshot_.config.hp_gamble && find_hp_gamble_card(state().players[state().actor].hand[slot]))
            return {kind, slot, 0, static_cast<std::uint8_t>(bounded(policy_rng, 2))};
        const auto target = (spell(state().players[state().actor].hand[slot]).effect == Effect::damage
            || (find_status_miracle(state().players[state().actor].hand[slot]) && spell(state().players[state().actor].hand[slot]).cure == Cure::none)
            || find_forgetting_card(state().players[state().actor].hand[slot])
            || find_item_removal_card(state().players[state().actor].hand[slot])) ? 1 - state().actor : state().actor;
        return {kind, slot, 0, static_cast<std::uint8_t>(target)};
    }
    if (kind == ActionKind::cast) {
        const auto slot = pick(opts.ability_slots, policy_rng);
        const auto target = spell(spell_id(slot)).effect == Effect::damage ? 1 - state().actor : state().actor;
        return {kind, slot, 0, static_cast<std::uint8_t>(target)};
    }
    if (kind == ActionKind::cast_status) {
        const auto slot = pick(opts.status_ability_slots, policy_rng);
        return {kind, slot, 0, static_cast<std::uint8_t>(status_miracles()[slot].cure == Cure::none ? 1 - state().actor : state().actor)};
    }
    if (kind == ActionKind::cast_additive)
        return {kind, pick(opts.additive_ability_slots, policy_rng), 0, static_cast<std::uint8_t>(1 - state().actor)};
    if (kind == ActionKind::discard) {
        auto slots = static_cast<std::uint16_t>(random(policy_rng) & opts.discard_slots);
        if (!slots) slots = static_cast<std::uint16_t>(1u << pick(opts.discard_slots, policy_rng));
        return {kind, no_slot, slots, no_slot};
    }
    return {};
}
}
