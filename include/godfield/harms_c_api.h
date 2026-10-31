#pragma once
#include "godfield/legacy_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v7: legacy-harms-subset-v7. Existing APIs keep their earlier dimensions.
// Shared 6-byte gl_action. Defend's base optionally selects a special reaction.
typedef struct gh_batch gh_batch;
int gh_abi_version(void);
const char* gh_ruleset(void);
int gh_observation_size(void); // 283 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gh_options_width(void); // 7 component masks, not complete action constraints.
int gh_hand_capacity(void);
size_t gh_action_size(void);
gh_batch* gh_create(size_t count, uint32_t max_decisions);
void gh_destroy(gh_batch* batch);
size_t gh_size(const gh_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gh_reset_at(gh_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gh_script_hit_rolls(gh_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gh_script_bounce_targets(gh_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gh_script_disease_rolls(gh_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gh_observe(const gh_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gh_legal(const gh_batch* batch, size_t count, const gl_action* actions, uint8_t* legal);
int gh_sample_actions(const gh_batch* batch, size_t count, uint64_t* policy_seeds, gl_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gh_step(gh_batch* batch, size_t count, const gl_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
