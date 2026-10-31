#include "godfield/hp_gamble_c_api.h"
#include "godfield/legacy_env.hpp"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace legacy = godfield::legacy;
static_assert(std::is_standard_layout_v<g33_action>);
static_assert(sizeof(g33_action) == 88);
static_assert(offsetof(g33_action, kind) == 0 && offsetof(g33_action, base) == 1);
static_assert(offsetof(g33_action, slots) == 2 && offsetof(g33_action, target) == 4);
static_assert(offsetof(g33_action, reserved) == 5);
static_assert(offsetof(g33_action, counter_count) == 6 && offsetof(g33_action, reserved2) == 7);
static_assert(offsetof(g33_action, counter_order) == 8);
static_assert(offsetof(g33_action, extra_count) == 16 && offsetof(g33_action, reserved_extra) == 18);
static_assert(offsetof(g33_action, extra_tail) == 17 && offsetof(g33_action, extras) == 24);
static_assert(static_cast<int>(legacy::ActionKind::pray) == 0);
static_assert(static_cast<int>(legacy::ActionKind::attack) == 1);
static_assert(static_cast<int>(legacy::ActionKind::defend) == 2);
static_assert(static_cast<int>(legacy::ActionKind::use) == 3);
static_assert(static_cast<int>(legacy::ActionKind::discard) == 4);
static_assert(static_cast<int>(legacy::ActionKind::cast) == 5);
static_assert(static_cast<int>(legacy::ActionKind::defend_cast) == 6);
static_assert(static_cast<int>(legacy::ActionKind::attack_set_fire) == 7);
static_assert(static_cast<int>(legacy::ActionKind::attack_set_water) == 8);
static_assert(static_cast<int>(legacy::ActionKind::attack_set_neutral) == 9);
static_assert(static_cast<int>(legacy::ActionKind::cast_additive) == 10);
static_assert(static_cast<int>(legacy::ActionKind::attack_ordered) == 11);
static_assert(static_cast<int>(legacy::ActionKind::cast_ordered) == 12);
static_assert(static_cast<int>(legacy::ActionKind::defend_ordered) == 13);
static_assert(static_cast<int>(legacy::ActionKind::exchange) == 14);
static_assert(static_cast<int>(legacy::ActionKind::sell) == 15);
static_assert(static_cast<int>(legacy::ActionKind::buy) == 16);
static_assert(static_cast<int>(legacy::ActionKind::buy_decline) == 17);
static_assert(static_cast<int>(legacy::ActionKind::buy_accept) == 18);

struct g33_batch { std::vector<legacy::Env> envs; };
namespace {
legacy::Action decode(g33_action action) noexcept {
    legacy::Action out{static_cast<legacy::ActionKind>(action.kind), action.base, action.slots,
                       action.target, action.counter_count, action.counter_order};
    out.extra_count = action.extra_count;
    std::copy_n(action.extras, 64, out.extras.begin());
    out.extras[64] = action.extra_tail;
    return out;
}
g33_action encode(const legacy::Action& action) noexcept {
    g33_action out{};
    out.kind = static_cast<std::uint8_t>(action.kind); out.base = action.base;
    out.slots = action.slots; out.target = action.target;
    out.counter_count = action.counter_count; out.counter_order = action.counter_order;
    out.extra_count = action.extra_count;
    std::copy_n(action.extras.begin(), 64, out.extras);
    out.extra_tail = action.extras[64];
    return out;
}
bool valid(const legacy::Env& env, const g33_action& action) noexcept {
    return action.reserved == 0 && action.reserved2 == 0
        && std::all_of(std::begin(action.reserved_extra), std::end(action.reserved_extra),
                       [](auto byte) { return byte == 0; })
        && action.kind <= 18 && env.legal(decode(action));
}
}

extern "C" {
int g33_abi_version(void) { return 33; }
const char* g33_ruleset(void) { return legacy::hp_gamble_ruleset; }
int g33_observation_size(void) { return legacy::hp_gamble_observation_size; }
int g33_options_width(void) { return 35; }
int g33_hand_capacity(void) { return legacy::hand_capacity; }
size_t g33_action_size(void) { return sizeof(g33_action); }
g33_batch* g33_create(size_t count, uint32_t max_decisions) {
    if (!count || !max_decisions) return nullptr;
    try {
        auto batch = g33_batch{};
        batch.envs.reserve(count);
        for (size_t i = 0; i < count; ++i)
            batch.envs.emplace_back(legacy::Config{max_decisions, 10, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true});
        return new g33_batch(std::move(batch));
    } catch (...) { return nullptr; }
}
void g33_destroy(g33_batch* batch) { delete batch; }
size_t g33_size(const g33_batch* batch) { return batch ? batch->envs.size() : 0; }
int g33_reset_at(g33_batch* batch, size_t index, uint64_t seed) {
    if (!batch) return -1;
    if (index >= batch->envs.size()) return -2;
    batch->envs[index].reset(seed);
    return 0;
}
int g33_script_hit_rolls(g33_batch* batch, size_t index, const uint8_t* rolls, size_t roll_count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || roll_count > 64) return -2;
    if (!rolls && roll_count) return -1;
    for (size_t i = 0; i < roll_count; ++i)
        if (rolls[i] < 1 || rolls[i] > 100) return -3;
    return batch->envs[index].script_hit_rolls({rolls, roll_count}) ? 0 : -3;
}
int g33_script_bounce_targets(g33_batch* batch, size_t index, const uint8_t* targets, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!targets && count) return -1;
    for (size_t i = 0; i < count; ++i)
        if (targets[i] > 1) return -3;
    return batch->envs[index].script_bounce_targets({targets, count}) ? 0 : -3;
}
int g33_script_disease_rolls(g33_batch* batch, size_t index, const uint8_t* rolls, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!rolls && count) return -1;
    for (size_t i = 0; i < count; ++i)
        if (rolls[i] > 99) return -3;
    return batch->envs[index].script_disease_rolls({rolls, count}) ? 0 : -3;
}
int g33_script_forget_tickets(g33_batch* batch, size_t index, const uint64_t* tickets, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!tickets && count) return -1;
    return batch->envs[index].script_forget_tickets({tickets, count}) ? 0 : -3;
}
int g33_script_item_removal_tickets(g33_batch* batch, size_t index, const uint64_t* tickets, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!tickets && count) return -1;
    return batch->envs[index].script_item_removal_tickets({tickets, count}) ? 0 : -3;
}
int g33_script_pestle_targets(g33_batch* batch, size_t index, const uint8_t* targets, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!targets && count) return -1;
    for (size_t i = 0; i < count; ++i)
        if (targets[i] > 1) return -3;
    return batch->envs[index].script_pestle_targets({targets, count}) ? 0 : -3;
}
int g33_script_buy_tickets(g33_batch* batch, size_t index, const uint64_t* tickets, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!tickets && count) return -1;
    return batch->envs[index].script_buy_tickets({tickets, count}) ? 0 : -3;
}
int g33_script_hp_gamble_rolls(g33_batch* batch, size_t index, const uint8_t* rolls, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!rolls && count) return -1;
    for (size_t i = 0; i < count; ++i)
        if (rolls[i] > 1) return -3;
    return batch->envs[index].script_hp_gamble_rolls({rolls, count}) ? 0 : -3;
}
int g33_observe(const g33_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray) {
    if (!batch || !observations || !options || !actors || !phases || !pray) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) {
        const auto& env = batch->envs[i];
        const auto obs = env.observe();
        const auto choices = env.options();
        std::copy_n(obs.begin(), legacy::hp_gamble_observation_size, observations + i * legacy::hp_gamble_observation_size);
        auto* row = options + 35 * i;
        row[0] = choices.weapon_bases; row[1] = choices.boost_slots;
        row[2] = choices.defense_slots; row[3] = choices.utility_slots;
        row[4] = choices.discard_slots; row[5] = choices.ability_slots;
        row[6] = choices.reaction_slots;
        row[7] = choices.defensive_ability_slots;
        row[8] = choices.attribute_slots;
        row[9] = choices.removal_slots;
        row[10] = choices.counter_slots;
        row[11] = choices.additive_slots;
        row[12] = choices.additive_ability_slots;
        row[13] = choices.doubling_slots;
        row[14] = choices.doubling_ability_slots;
        row[15] = choices.wide_slots;
        row[16] = choices.wide_ability_slots;
        row[17] = choices.magic_free_slots;
        row[18] = choices.free_cast_slots;
        row[19] = choices.free_ability_slots;
        row[20] = choices.free_additive_ability_slots;
        row[21] = choices.free_defensive_slots;
        row[22] = choices.free_defensive_ability_slots;
        row[23] = choices.forget_slots;
        row[24] = choices.exchange_slots;
        row[25] = choices.item_removal_slots;
        row[26] = choices.hp_absorption_slots;
        row[27] = choices.self_damage_slots;
        row[28] = choices.pestle_slots;
        row[29] = choices.mortar_slots;
        row[30] = choices.sell_slots;
        row[31] = choices.sale_item_slots;
        row[32] = choices.buy_slots;
        row[33] = choices.purchase_choices;
        row[34] = choices.hp_gamble_slots;
        actors[i] = env.state().actor;
        phases[i] = static_cast<std::uint8_t>(env.state().phase);
        pray[i] = choices.pray;
    }
    return 0;
}
int g33_legal(const g33_batch* batch, size_t count, const g33_action* actions, uint8_t* legal) {
    if (!batch || !actions || !legal) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) legal[i] = valid(batch->envs[i], actions[i]);
    return 0;
}
int g33_sample_actions(const g33_batch* batch, size_t count, uint64_t* policy_seeds,
                      g33_action* actions) {
    if (!batch || !policy_seeds || !actions) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) {
        if (batch->envs[i].done()) actions[i] = encode(legacy::Action{});
        else actions[i] = encode(batch->envs[i].sample_action(policy_seeds[i]));
    }
    return 0;
}
int g33_step(g33_batch* batch, size_t count, const g33_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated) {
    if (!batch || !actions || !rewards || !terminated || !truncated) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i)
        if (!batch->envs[i].done() && !valid(batch->envs[i], actions[i])) return -3;
    for (size_t i = 0; i < count; ++i) {
        const auto& state = batch->envs[i].state();
        if (batch->envs[i].done()) {
            rewards[2 * i] = rewards[2 * i + 1] = 0.f;
            terminated[i] = state.terminated;
            truncated[i] = state.truncated;
        } else {
            const auto result = batch->envs[i].step(decode(actions[i]));
            rewards[2 * i] = result.rewards[0]; rewards[2 * i + 1] = result.rewards[1];
            terminated[i] = result.terminated; truncated[i] = result.truncated;
        }
    }
    return 0;
}
}
