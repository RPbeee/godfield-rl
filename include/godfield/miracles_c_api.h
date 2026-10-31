#pragma once
#include "godfield/legacy_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v2, legacy-neutral-miracles-subset-v2. Independent from unchanged gl_*.
// gl_action layout is shared (6 bytes); kind 5 = cast a retained miracle.
// For cast: base is the miracle-type index (0=old ID216, 1=old ID220),
// slots=0, target is fixed seat 0/1, reserved=0.
typedef struct gm_batch gm_batch;
int gm_abi_version(void);
const char* gm_ruleset(void);
int gm_observation_size(void); // 164 floats per row.
int gm_options_width(void); // 6 uint16 masks; last is affordable retained types.
int gm_hand_capacity(void);
size_t gm_action_size(void);
gm_batch* gm_create(size_t count, uint32_t max_decisions);
void gm_destroy(gm_batch* batch);
size_t gm_size(const gm_batch* batch);

// Caller-owned, contiguous row-major buffers. No concurrent calls on one handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 illegal action.
int gm_reset_at(gm_batch* batch, size_t index, uint64_t seed);
int gm_observe(const gm_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
// Component masks are not a complete legality specification.
int gm_legal(const gm_batch* batch, size_t count, const gl_action* actions,
             uint8_t* legal);
// Hierarchical random policy; seeds are read/written. Environment state/RNG unchanged.
// Done rows produce default pray actions without advancing policy seeds.
int gm_sample_actions(const gm_batch* batch, size_t count, uint64_t* policy_seeds,
                      gl_action* actions);
// Validate all active rows before mutation. Rewards are fixed-seat [count,2].
// No auto reset. Done rows freeze, emit zero reward, and ignore malformed actions.
int gm_step(gm_batch* batch, size_t count, const gl_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
