#include "godfield/c_api.h"
#include "godfield/env.hpp"
#include <algorithm>
#include <vector>

struct gf_batch { std::vector<godfield::Env> envs; };
extern "C" {
int gf_abi_version(void) { return 1; }
int gf_action_count(void) { return godfield::action_count; }
int gf_observation_size(void) { return godfield::observation_size; }
gf_batch* gf_create(size_t count, uint32_t max_decisions) {
    if (!count || !max_decisions) return nullptr;
    try {
        auto batch = gf_batch{};
        batch.envs.reserve(count);
        for (size_t i = 0; i < count; ++i) batch.envs.emplace_back(godfield::Config{max_decisions});
        return new gf_batch(std::move(batch));
    } catch (...) { return nullptr; }
}
void gf_destroy(gf_batch* batch) { delete batch; }
size_t gf_size(const gf_batch* batch) { return batch ? batch->envs.size() : 0; }
int gf_reset_at(gf_batch* batch, size_t index, uint64_t seed) {
    if (!batch) return -1;
    if (index >= batch->envs.size()) return -2;
    batch->envs[index].reset(seed);
    return 0;
}
int gf_observe(const gf_batch* batch, size_t count, float* observations,
               uint8_t* masks, uint8_t* actors) {
    if (!batch || !observations || !masks || !actors) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i) {
        const auto obs = batch->envs[i].observe();
        const auto mask = batch->envs[i].action_mask();
        std::copy(obs.begin(), obs.end(), observations + i * godfield::observation_size);
        std::copy(mask.begin(), mask.end(), masks + i * godfield::action_count);
        actors[i] = batch->envs[i].state().actor;
    }
    return 0;
}
int gf_step(gf_batch* batch, size_t count, const int32_t* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated) {
    if (!batch || !actions || !rewards || !terminated || !truncated) return -1;
    if (count != batch->envs.size()) return -2;
    for (size_t i = 0; i < count; ++i)
        if (!batch->envs[i].done() && !batch->envs[i].legal(actions[i])) return -3;
    for (size_t i = 0; i < count; ++i) {
        const auto result = batch->envs[i].step(actions[i]);
        rewards[2 * i] = result.rewards[0];
        rewards[2 * i + 1] = result.rewards[1];
        terminated[i] = result.terminated;
        truncated[i] = result.truncated;
    }
    return 0;
}
}
