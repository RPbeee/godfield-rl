#include "godfield/additive_miracles_c_api.h"
#include "godfield/legacy_env.hpp"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace legacy = godfield::legacy;
static_assert(std::is_standard_layout_v<gp_action>);
static_assert(sizeof(gp_action) == 88);
static_assert(offsetof(gp_action, kind) == 0 && offsetof(gp_action, base) == 1);
static_assert(offsetof(gp_action, slots) == 2 && offsetof(gp_action, target) == 4);
static_assert(offsetof(gp_action, reserved) == 5);
static_assert(offsetof(gp_action, counter_count) == 6 && offsetof(gp_action, reserved2) == 7);
static_assert(offsetof(gp_action, counter_order) == 8);
static_assert(offsetof(gp_action, extra_count) == 16 && offsetof(gp_action, reserved_extra) == 17);
static_assert(offsetof(gp_action, extras) == 24);
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

struct gp_batch { std::vector<legacy::Env> envs; };
namespace {
legacy::Action decode(gp_action action) noexcept {
    legacy::Action out{static_cast<legacy::ActionKind>(action.kind), action.base, action.slots,
                       action.target, action.counter_count, action.counter_order};
    out.extra_count = action.extra_count;
    std::copy_n(action.extras, 64, out.extras.begin());
    return out;
}
gp_action encode(const legacy::Action& action) noexcept {
    gp_action out{};
    out.kind = static_cast<std::uint8_t>(action.kind); out.base = action.base;
    out.slots = action.slots; out.target = action.target;
    out.counter_count = action.counter_count; out.counter_order = action.counter_order;
    out.extra_count = action.extra_count;
    std::copy_n(action.extras.begin(), 64, out.extras);
    return out;
}
bool valid(const legacy::Env& env, const gp_action& action) noexcept {
    return action.reserved == 0 && action.reserved2 == 0
        && std::all_of(std::begin(action.reserved_extra), std::end(action.reserved_extra),
                       [](auto byte) { return byte == 0; })
        && action.kind <= 11 && (action.kind != 10 || action.base < 2)
        && action.extra_count <= 64
        && std::all_of(action.extras, action.extras + action.extra_count, [](auto ref) { return ref <= 17; })
        && env.legal(decode(action));
}
}

extern "C" {
int gp_abi_version(void) { return 20; }
const char* gp_ruleset(void) { return legacy::additive_miracles_ruleset; }
int gp_observation_size(void) { return legacy::additive_miracles_observation_size; }
int gp_options_width(void) { return 13; }
int gp_hand_capacity(void) { return legacy::hand_capacity; }
size_t gp_action_size(void) { return sizeof(gp_action); }
gp_batch* gp_create(size_t count, uint32_t max_decisions) {
    if (!count || !max_decisions) return nullptr;
    try {
        auto batch = gp_batch{};
        batch.envs.reserve(count);
        for (size_t i = 0; i < count; ++i)
            batch.envs.emplace_back(legacy::Config{max_decisions, 10, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true});
        return new gp_batch(std::move(batch));
    } catch (...) { return nullptr; }
}
void gp_destroy(gp_batch* batch) { delete batch; }
size_t gp_size(const gp_batch* batch) { return batch ? batch->envs.size() : 0; }
int gp_reset_at(gp_batch* batch, size_t index, uint64_t seed) {
    if (!batch) return -1;
    if (index >= batch->envs.size()) return -2;
    batch->envs[index].reset(seed);
    return 0;
}
int gp_script_hit_rolls(gp_batch* batch, size_t index, const uint8_t* rolls, size_t roll_count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || roll_count > 64) return -2;
    if (!rolls && roll_count) return -1;
    for (size_t i = 0; i < roll_count; ++i)
        if (rolls[i] < 1 || rolls[i] > 100) return -3;
    return batch->envs[index].script_hit_rolls({rolls, roll_count}) ? 0 : -3;
}
int gp_script_bounce_targets(gp_batch* batch, size_t index, const uint8_t* targets, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!targets && count) return -1;
    for (size_t i = 0; i < count; ++i)
        if (targets[i] > 1) return -3;
    return batch->envs[index].script_bounce_targets({targets, count}) ? 0 : -3;
}
int gp_script_disease_rolls(gp_batch* batch, size_t index, const uint8_t* rolls, size_t count) {
    if (!batch) return -1;
    if (index >= batch->envs.size() || count > 64) return -2;
    if (!rolls && count) return -1;
    for (size_t i = 0; i < count; ++i)
        if (rolls[i] > 99) return -3;
    return batch->envs[index].script_disease_rolls({rolls, count}) ? 0 : -3;
}
int gp_observe(const gp_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray) {
    if (!batch || !observations || !options || !actors || !phases || !pray) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) {
        const auto& env = batch->envs[i];
        const auto obs = env.observe();
        const auto choices = env.options();
        std::copy_n(obs.begin(), legacy::additive_miracles_observation_size, observations + i * legacy::additive_miracles_observation_size);
        auto* row = options + 13 * i;
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
        actors[i] = env.state().actor;
        phases[i] = static_cast<std::uint8_t>(env.state().phase);
        pray[i] = choices.pray;
    }
    return 0;
}
int gp_legal(const gp_batch* batch, size_t count, const gp_action* actions, uint8_t* legal) {
    if (!batch || !actions || !legal) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) legal[i] = valid(batch->envs[i], actions[i]);
    return 0;
}
int gp_sample_actions(const gp_batch* batch, size_t count, uint64_t* policy_seeds,
                      gp_action* actions) {
    if (!batch || !policy_seeds || !actions) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) {
        if (batch->envs[i].done()) actions[i] = encode(legacy::Action{});
        else actions[i] = encode(batch->envs[i].sample_action(policy_seeds[i]));
    }
    return 0;
}
int gp_step(gp_batch* batch, size_t count, const gp_action* actions,
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
