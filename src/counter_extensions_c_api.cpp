#include "godfield/counter_extensions_c_api.h"
#include "godfield/legacy_env.hpp"
#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace legacy = godfield::legacy;
static_assert(std::is_standard_layout_v<gx_action>);
static_assert(sizeof(gx_action) == 16);
static_assert(offsetof(gx_action, kind) == 0 && offsetof(gx_action, base) == 1);
static_assert(offsetof(gx_action, slots) == 2 && offsetof(gx_action, target) == 4);
static_assert(offsetof(gx_action, reserved) == 5);
static_assert(offsetof(gx_action, counter_count) == 6 && offsetof(gx_action, reserved2) == 7);
static_assert(offsetof(gx_action, counter_order) == 8);
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

struct gx_batch { std::vector<legacy::Env> envs; };
namespace {
legacy::Action decode(gx_action action) noexcept {
    return {static_cast<legacy::ActionKind>(action.kind), action.base, action.slots, action.target, action.counter_count, action.counter_order};
}
gx_action encode(legacy::Action action) noexcept {
    return {static_cast<std::uint8_t>(action.kind), action.base, action.slots, action.target, 0, action.counter_count, 0, action.counter_order};
}
bool valid(const legacy::Env& env, gx_action action) noexcept {
    return action.reserved == 0 && action.reserved2 == 0 && action.kind <= 9 && env.legal(decode(action));
}
}

extern "C" {
int gx_abi_version(void) { return 14; }
const char* gx_ruleset(void) { return legacy::counter_extensions_ruleset; }
int gx_observation_size(void) { return legacy::counter_extensions_observation_size; }
int gx_options_width(void) { return 11; }
int gx_hand_capacity(void) { return legacy::hand_capacity; }
size_t gx_action_size(void) { return sizeof(gx_action); }
gx_batch* gx_create(size_t count, uint32_t max_decisions) {
    if (!count || !max_decisions) return nullptr;
    try {
        auto batch = gx_batch{};
        batch.envs.reserve(count);
        for (size_t i = 0; i < count; ++i)
            batch.envs.emplace_back(legacy::Config{max_decisions, 10, true, true, true, true, true, true, true, true, true, true, true, true, true});
        return new gx_batch(std::move(batch));
    } catch (...) { return nullptr; }
}
void gx_destroy(gx_batch* batch) { delete batch; }
size_t gx_size(const gx_batch* batch) { return batch ? batch->envs.size() : 0; }
int gx_reset_at(gx_batch* batch, size_t index, uint64_t seed) {
    if (!batch) return -1;
    if (index >= batch->envs.size()) return -2;
    batch->envs[index].reset(seed);
    return 0;
}
int gx_script_hit_rolls(gx_batch* batch, size_t index, const uint8_t* rolls, size_t roll_count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || roll_count > 64) return -2;
    if (!rolls && roll_count) return -1;
    for (size_t i = 0; i < roll_count; ++i)
        if (rolls[i] < 1 || rolls[i] > 100) return -3;
    return batch->envs[index].script_hit_rolls({rolls, roll_count}) ? 0 : -3;
}
int gx_script_bounce_targets(gx_batch* batch, size_t index, const uint8_t* targets, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!targets && count) return -1;
    for (size_t i = 0; i < count; ++i)
        if (targets[i] > 1) return -3;
    return batch->envs[index].script_bounce_targets({targets, count}) ? 0 : -3;
}
int gx_script_disease_rolls(gx_batch* batch, size_t index, const uint8_t* rolls, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!rolls && count) return -1;
    for (size_t i = 0; i < count; ++i)
        if (rolls[i] > 99) return -3;
    return batch->envs[index].script_disease_rolls({rolls, count}) ? 0 : -3;
}
int gx_observe(const gx_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray) {
    if (!batch || !observations || !options || !actors || !phases || !pray) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) {
        const auto& env = batch->envs[i];
        const auto obs = env.observe();
        const auto choices = env.options();
        std::copy_n(obs.begin(), legacy::counter_extensions_observation_size, observations + i * legacy::counter_extensions_observation_size);
        auto* row = options + 11 * i;
        row[0] = choices.weapon_bases; row[1] = choices.boost_slots;
        row[2] = choices.defense_slots; row[3] = choices.utility_slots;
        row[4] = choices.discard_slots; row[5] = choices.ability_slots;
        row[6] = choices.reaction_slots;
        row[7] = choices.defensive_ability_slots;
        row[8] = choices.attribute_slots;
        row[9] = choices.removal_slots;
        row[10] = choices.counter_slots;
        actors[i] = env.state().actor;
        phases[i] = static_cast<std::uint8_t>(env.state().phase);
        pray[i] = choices.pray;
    }
    return 0;
}
int gx_legal(const gx_batch* batch, size_t count, const gx_action* actions, uint8_t* legal) {
    if (!batch || !actions || !legal) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) legal[i] = valid(batch->envs[i], actions[i]);
    return 0;
}
int gx_sample_actions(const gx_batch* batch, size_t count, uint64_t* policy_seeds,
                      gx_action* actions) {
    if (!batch || !policy_seeds || !actions) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) {
        if (batch->envs[i].done()) actions[i] = encode(legacy::Action{});
        else actions[i] = encode(batch->envs[i].sample_action(policy_seeds[i]));
    }
    return 0;
}
int gx_step(gx_batch* batch, size_t count, const gx_action* actions,
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
