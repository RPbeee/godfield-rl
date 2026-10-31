#pragma once
#include "godfield/legacy_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v3: legacy-elements-miracles-subset-v3. Existing gl_* and gm_* are unchanged.
// Shared 6-byte gl_action layout. 0 pray, 1 attack, 2 defend, 3 use, 4 discard, 5 cast.
// cast: base=retained miracle type index, slots=0, target=fixed seat, reserved=0.
typedef struct ge_batch ge_batch;
int ge_abi_version(void);
const char* ge_ruleset(void);
int ge_observation_size(void); // 186 floats per row.
int ge_options_width(void); // 6 component masks; not complete legality constraints.
int ge_hand_capacity(void);
size_t ge_action_size(void);
ge_batch* ge_create(size_t count, uint32_t max_decisions);
void ge_destroy(ge_batch* batch);
size_t ge_size(const ge_batch* batch);

// Arrays are caller-owned, contiguous row-major. No concurrency on one handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 illegal action.
int ge_reset_at(ge_batch* batch, size_t index, uint64_t seed);
int ge_observe(const ge_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int ge_legal(const ge_batch* batch, size_t count, const gl_action* actions,
             uint8_t* legal);
// Hierarchical random sampling; policy_seeds are read/written, game RNG is unchanged.
int ge_sample_actions(const ge_batch* batch, size_t count, uint64_t* policy_seeds,
                      gl_action* actions);
// All active rows validated before any mutation; rejection is atomic.
// Fixed-seat rewards[count,2]. Done rows freeze, reward=0, ignore malformed actions.
int ge_step(ge_batch* batch, size_t count, const gl_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
