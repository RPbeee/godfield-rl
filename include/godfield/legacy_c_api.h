#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// Independent ABI v1 for legacy-neutral-subset-v1; existing gf_* is unaffected.
// Handles may run on separate threads, but one handle must not be used concurrently.
typedef struct gl_batch gl_batch;
typedef struct gl_action {
    uint8_t kind;     // 0 pray, 1 attack, 2 defend, 3 use, 4 discard.
    uint8_t base;     // Hand slot, or 255 when unused.
    uint16_t slots;   // Additional attack slots, or defense/discard slots.
    uint8_t target;   // Fixed seat 0/1, or 255 when unused.
    uint8_t reserved; // Must be zero. Explicit tail byte; sizeof(gl_action) == 6.
} gl_action;

int gl_abi_version(void);
const char* gl_ruleset(void); // Static storage; caller must not free.
int gl_observation_size(void);
int gl_options_width(void); // 5: weapon bases, boosts, defenses, utilities, discards.
int gl_hand_capacity(void);
size_t gl_action_size(void);
gl_batch* gl_create(size_t count, uint32_t max_decisions); // NULL on failure.
void gl_destroy(gl_batch* batch);
size_t gl_size(const gl_batch* batch);

// All arrays are caller-owned and contiguous/row-major. count must equal gl_size.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 illegal action.
// No C++ exception crosses the ABI. No automatic reset.
int gl_reset_at(gl_batch* batch, size_t index, uint64_t seed);
int gl_observe(const gl_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
// Options are component masks, NOT a complete legality specification.
// Use gl_legal to validate a composed semantic action. Done rows report false.
int gl_legal(const gl_batch* batch, size_t count, const gl_action* actions,
             uint8_t* legal);
// Reproducible hierarchical random policy, not uniform over semantic actions.
// policy_seeds[count] are read/written; environment RNG and state do not change.
// Done rows return default pray actions without advancing policy_seeds.
int gl_sample_actions(const gl_batch* batch, size_t count, uint64_t* policy_seeds,
                      gl_action* actions);
// Validate all active rows before updating any row. Rejection is atomic.
// rewards[count,2] are in fixed-seat coordinates, not next-actor relative.
// Done rows ignore their action, remain frozen, and emit rewards=0.
int gl_step(gl_batch* batch, size_t count, const gl_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
