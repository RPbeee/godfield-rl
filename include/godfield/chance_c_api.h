#pragma once
#include "godfield/legacy_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v4: legacy-elements-miracles-chance-subset-v4; earlier APIs unchanged.
// Same 6-byte gl_action layout and kind codes 0..5 as gm/ge.
typedef struct gc_batch gc_batch;
int gc_abi_version(void);
const char* gc_ruleset(void);
int gc_observation_size(void); // 210 floats per row.
int gc_options_width(void); // 6 component masks, not full legality constraints.
int gc_hand_capacity(void);
size_t gc_action_size(void);
gc_batch* gc_create(size_t count, uint32_t max_decisions);
void gc_destroy(gc_batch* batch);
size_t gc_size(const gc_batch* batch);

// Caller-owned contiguous row-major arrays; no concurrency on one batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gc_reset_at(gc_batch* batch, size_t index, uint64_t seed);
// Fixture-only outcome prefix: up to 64 rolls, each in [1,100].
// Replaces this row's unconsumed scripted prefix without advancing game RNG.
// Invalid input leaves the previous prefix and environment unchanged.
// NULL rolls with roll_count==0 is valid and clears the prefix.
int gc_script_hit_rolls(gc_batch* batch, size_t index,
                       const uint8_t* rolls, size_t roll_count);
int gc_observe(const gc_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gc_legal(const gc_batch* batch, size_t count, const gl_action* actions,
             uint8_t* legal);
int gc_sample_actions(const gc_batch* batch, size_t count, uint64_t* policy_seeds,
                      gl_action* actions);
// All active rows validate before mutation. Fixed-seat rewards[count,2].
// No auto reset. Done rows freeze, emit zero rewards and ignore malformed actions.
int gc_step(gc_batch* batch, size_t count, const gl_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
