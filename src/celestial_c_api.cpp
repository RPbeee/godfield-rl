#include "godfield/celestial_c_api.h"
#include "godfield/legacy_env.hpp"
#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace legacy = godfield::legacy;
static_assert(std::is_standard_layout_v<gl_action>);
static_assert(sizeof(gl_action) == 6);
static_assert(offsetof(gl_action, kind) == 0 && offsetof(gl_action, base) == 1);
static_assert(offsetof(gl_action, slots) == 2 && offsetof(gl_action, target) == 4);
static_assert(offsetof(gl_action, reserved) == 5);
static_assert(static_cast<int>(legacy::ActionKind::pray) == 0);
static_assert(static_cast<int>(legacy::ActionKind::attack) == 1);
static_assert(static_cast<int>(legacy::ActionKind::defend) == 2);
static_assert(static_cast<int>(legacy::ActionKind::use) == 3);
static_assert(static_cast<int>(legacy::ActionKind::discard) == 4);
static_assert(static_cast<int>(legacy::ActionKind::cast) == 5);

struct gs_batch { std::vector<legacy::Env> envs; };
namespace {
legacy::Action decode(gl_action action) noexcept {
    return {static_cast<legacy::ActionKind>(action.kind), action.base, action.slots, action.target};
}
gl_action encode(legacy::Action action) noexcept {
    return {static_cast<std::uint8_t>(action.kind), action.base, action.slots, action.target, 0};
}
bool valid(const legacy::Env& env, gl_action action) noexcept {
    return action.reserved == 0 && action.kind <= 5 && env.legal(decode(action));
}
}

extern "C" {
int gs_abi_version(void) { return 8; }
const char* gs_ruleset(void) { return legacy::celestial_ruleset; }
int gs_observation_size(void) { return legacy::celestial_observation_size; }
int gs_options_width(void) { return 7; }
int gs_hand_capacity(void) { return legacy::hand_capacity; }
size_t gs_action_size(void) { return sizeof(gl_action); }
gs_batch* gs_create(size_t count, uint32_t max_decisions) {
    if (!count || !max_decisions) return nullptr;
    try {
        auto batch = gs_batch{};
        batch.envs.reserve(count);
        for (size_t i = 0; i < count; ++i)
            batch.envs.emplace_back(legacy::Config{max_decisions, 10, true, true, true, true, true, true, true});
        return new gs_batch(std::move(batch));
    } catch (...) { return nullptr; }
}
void gs_destroy(gs_batch* batch) { delete batch; }
size_t gs_size(const gs_batch* batch) { return batch ? batch->envs.size() : 0; }
int gs_reset_at(gs_batch* batch, size_t index, uint64_t seed) {
    if (!batch) return -1;
    if (index >= batch->envs.size()) return -2;
    batch->envs[index].reset(seed);
    return 0;
}
int gs_script_hit_rolls(gs_batch* batch, size_t index, const uint8_t* rolls, size_t roll_count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || roll_count > 64) return -2;
    if (!rolls && roll_count) return -1;
    for (size_t i = 0; i < roll_count; ++i)
        if (rolls[i] < 1 || rolls[i] > 100) return -3;
    return batch->envs[index].script_hit_rolls({rolls, roll_count}) ? 0 : -3;
}
int gs_script_bounce_targets(gs_batch* batch, size_t index, const uint8_t* targets, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!targets && count) return -1;
    for (size_t i = 0; i < count; ++i)
        if (targets[i] > 1) return -3;
    return batch->envs[index].script_bounce_targets({targets, count}) ? 0 : -3;
}
int gs_script_disease_rolls(gs_batch* batch, size_t index, const uint8_t* rolls, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!rolls && count) return -1;
    for (size_t i = 0; i < count; ++i)
        if (rolls[i] > 99) return -3;
    return batch->envs[index].script_disease_rolls({rolls, count}) ? 0 : -3;
}
int gs_observe(const gs_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray) {
    if (!batch || !observations || !options || !actors || !phases || !pray) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) {
        const auto& env = batch->envs[i];
        const auto obs = env.observe();
        const auto choices = env.options();
        std::copy_n(obs.begin(), legacy::celestial_observation_size, observations + i * legacy::celestial_observation_size);
        auto* row = options + 7 * i;
        row[0] = choices.weapon_bases; row[1] = choices.boost_slots;
        row[2] = choices.defense_slots; row[3] = choices.utility_slots;
        row[4] = choices.discard_slots; row[5] = choices.ability_slots;
        row[6] = choices.reaction_slots;
        actors[i] = env.state().actor;
        phases[i] = static_cast<std::uint8_t>(env.state().phase);
        pray[i] = choices.pray;
    }
    return 0;
}
int gs_legal(const gs_batch* batch, size_t count, const gl_action* actions, uint8_t* legal) {
    if (!batch || !actions || !legal) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) legal[i] = valid(batch->envs[i], actions[i]);
    return 0;
}
int gs_sample_actions(const gs_batch* batch, size_t count, uint64_t* policy_seeds,
                      gl_action* actions) {
    if (!batch || !policy_seeds || !actions) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) {
        if (batch->envs[i].done()) actions[i] = encode(legacy::Action{});
        else actions[i] = encode(batch->envs[i].sample_action(policy_seeds[i]));
    }
    return 0;
}
int gs_step(gs_batch* batch, size_t count, const gl_action* actions,
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
