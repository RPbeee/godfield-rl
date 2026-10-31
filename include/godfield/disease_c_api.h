#pragma once
#include "godfield/legacy_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v6: legacy-disease-subset-v6. Existing APIs keep their earlier dimensions.
// Shared 6-byte gl_action. Defend's base optionally selects a special reaction.
typedef struct gd_batch gd_batch;
int gd_abi_version(void);
const char* gd_ruleset(void);
int gd_observation_size(void); // 258 floats/row; no hidden worse_chance counter.
int gd_options_width(void); // 7 component masks, not complete action constraints.
int gd_hand_capacity(void);
size_t gd_action_size(void);
gd_batch* gd_create(size_t count, uint32_t max_decisions);
void gd_destroy(gd_batch* batch);
size_t gd_size(const gd_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gd_reset_at(gd_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gd_script_hit_rolls(gd_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gd_script_bounce_targets(gd_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gd_script_disease_rolls(gd_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gd_observe(const gd_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gd_legal(const gd_batch* batch, size_t count, const gl_action* actions, uint8_t* legal);
int gd_sample_actions(const gd_batch* batch, size_t count, uint64_t* policy_seeds, gl_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gd_step(gd_batch* batch, size_t count, const gl_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
