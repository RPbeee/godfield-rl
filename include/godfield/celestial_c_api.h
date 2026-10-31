#pragma once
#include "godfield/legacy_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v8: legacy-celestial-subset-v8. Existing APIs keep their earlier dimensions.
// Shared 6-byte gl_action. Defend's base optionally selects a special reaction.
typedef struct gs_batch gs_batch;
int gs_abi_version(void);
const char* gs_ruleset(void);
int gs_observation_size(void); // 301 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gs_options_width(void); // 7 component masks, not complete action constraints.
int gs_hand_capacity(void);
size_t gs_action_size(void);
gs_batch* gs_create(size_t count, uint32_t max_decisions);
void gs_destroy(gs_batch* batch);
size_t gs_size(const gs_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gs_reset_at(gs_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gs_script_hit_rolls(gs_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gs_script_bounce_targets(gs_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gs_script_disease_rolls(gs_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gs_observe(const gs_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gs_legal(const gs_batch* batch, size_t count, const gl_action* actions, uint8_t* legal);
int gs_sample_actions(const gs_batch* batch, size_t count, uint64_t* policy_seeds, gl_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gs_step(gs_batch* batch, size_t count, const gl_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
