#include "godfield/legacy_c_api.h"
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

struct gl_batch { std::vector<legacy::Env> envs; };
namespace {
legacy::Action decode(gl_action action) noexcept {
    return {static_cast<legacy::ActionKind>(action.kind), action.base, action.slots, action.target};
}
gl_action encode(legacy::Action action) noexcept {
    return {static_cast<std::uint8_t>(action.kind), action.base, action.slots, action.target, 0};
}
bool valid(const legacy::Env& env, gl_action action) noexcept {
    return action.reserved == 0 && action.kind <= 4 && env.legal(decode(action));
}
}

extern "C" {
int gl_abi_version(void) { return 1; }
const char* gl_ruleset(void) { return legacy::ruleset; }
int gl_observation_size(void) { return legacy::base_observation_size; }
int gl_options_width(void) { return 5; }
int gl_hand_capacity(void) { return legacy::hand_capacity; }
size_t gl_action_size(void) { return sizeof(gl_action); }
gl_batch* gl_create(size_t count, uint32_t max_decisions) {
    if (!count || !max_decisions) return nullptr;
    try {
        auto batch = gl_batch{};
        batch.envs.reserve(count);
        for (size_t i = 0; i < count; ++i)
            batch.envs.emplace_back(legacy::Config{max_decisions});
        return new gl_batch(std::move(batch));
    } catch (...) { return nullptr; }
}
void gl_destroy(gl_batch* batch) { delete batch; }
size_t gl_size(const gl_batch* batch) { return batch ? batch->envs.size() : 0; }
int gl_reset_at(gl_batch* batch, size_t index, uint64_t seed) {
    if (!batch) return -1;
    if (index >= batch->envs.size()) return -2;
    batch->envs[index].reset(seed);
    return 0;
}
int gl_observe(const gl_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray) {
    if (!batch || !observations || !options || !actors || !phases || !pray) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) {
        const auto& env = batch->envs[i];
        const auto obs = env.observe();
        const auto choices = env.options();
        std::copy_n(obs.begin(), legacy::base_observation_size, observations + i * legacy::base_observation_size);
        auto* row = options + 5 * i;
        row[0] = choices.weapon_bases; row[1] = choices.boost_slots;
        row[2] = choices.defense_slots; row[3] = choices.utility_slots;
        row[4] = choices.discard_slots;
        actors[i] = env.state().actor;
        phases[i] = static_cast<std::uint8_t>(env.state().phase);
        pray[i] = choices.pray;
    }
    return 0;
}
int gl_legal(const gl_batch* batch, size_t count, const gl_action* actions, uint8_t* legal) {
    if (!batch || !actions || !legal) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) legal[i] = valid(batch->envs[i], actions[i]);
    return 0;
}
int gl_sample_actions(const gl_batch* batch, size_t count, uint64_t* policy_seeds,
                      gl_action* actions) {
    if (!batch || !policy_seeds || !actions) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) {
        if (batch->envs[i].done()) actions[i] = encode(legacy::Action{});
        else actions[i] = encode(batch->envs[i].sample_action(policy_seeds[i]));
    }
    return 0;
}
int gl_step(gl_batch* batch, size_t count, const gl_action* actions,
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
