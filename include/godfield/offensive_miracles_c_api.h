#pragma once
#include "godfield/legacy_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v9: legacy-offensive-miracles-subset-v9. Existing APIs keep their earlier dimensions.
// Shared 6-byte gl_action. Defend's base optionally selects a special reaction.
typedef struct go_batch go_batch;
int go_abi_version(void);
const char* go_ruleset(void);
int go_observation_size(void); // 334 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int go_options_width(void); // 7 component masks, not complete action constraints.
int go_hand_capacity(void);
size_t go_action_size(void);
go_batch* go_create(size_t count, uint32_t max_decisions);
void go_destroy(go_batch* batch);
size_t go_size(const go_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int go_reset_at(go_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int go_script_hit_rolls(go_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int go_script_bounce_targets(go_batch* batch, size_t index, const uint8_t* targets, size_t count);
int go_script_disease_rolls(go_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int go_observe(const go_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int go_legal(const go_batch* batch, size_t count, const gl_action* actions, uint8_t* legal);
int go_sample_actions(const go_batch* batch, size_t count, uint64_t* policy_seeds, gl_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int go_step(go_batch* batch, size_t count, const gl_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
