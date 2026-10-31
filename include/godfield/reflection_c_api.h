#pragma once
#include "godfield/legacy_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v5: legacy-weapon-reactions-subset-v5. Older profiles remain independent.
// Same 6-byte gl_action and codes 0..5. For defend, base is a reaction slot
// (255 for ordinary defense), slots is preceding ordinary armor, target=255.
typedef struct gr_batch gr_batch;
int gr_abi_version(void);
const char* gr_ruleset(void);
int gr_observation_size(void); // 236 floats/row.
int gr_options_width(void); // 7 masks; seventh is reaction slots.
int gr_hand_capacity(void);
size_t gr_action_size(void);
gr_batch* gr_create(size_t count, uint32_t max_decisions);
void gr_destroy(gr_batch* batch);
size_t gr_size(const gr_batch* batch);

// Caller-owned row-major buffers. No concurrent calls on one handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gr_reset_at(gr_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes only: up to 64 outcomes. Empty input clears a prefix.
// Invalid replacement preserves the previous prefix and game state.
int gr_script_hit_rolls(gr_batch* batch, size_t index, const uint8_t* rolls, size_t count);
// Seat targets must be 0 or 1. NULL is permitted only with count==0.
int gr_script_bounce_targets(gr_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gr_observe(const gr_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gr_legal(const gr_batch* batch, size_t count, const gl_action* actions, uint8_t* legal);
int gr_sample_actions(const gr_batch* batch, size_t count, uint64_t* policy_seeds, gl_action* actions);
// Atomic active-row validation. Fixed-seat rewards[count,2]. No auto reset.
// Done rows remain frozen, reward=0, and ignore malformed actions.
int gr_step(gr_batch* batch, size_t count, const gl_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
