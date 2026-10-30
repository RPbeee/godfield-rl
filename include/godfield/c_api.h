#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// ABI v1. All buffers are caller-owned, contiguous and row-major.
// Handles may run on separate threads; one handle must not be used concurrently.
typedef struct gf_batch gf_batch;
int gf_abi_version(void);
int gf_action_count(void);
int gf_observation_size(void);
gf_batch* gf_create(size_t count, uint32_t max_decisions); // NULL on failure.
void gf_destroy(gf_batch* batch);
size_t gf_size(const gf_batch* batch);
// Return 0 on success, -1 for NULL, -2 for size/index mismatch,
// -3 for illegal actions (no row stepped). Never throw across the ABI.
int gf_reset_at(gf_batch* batch, size_t index, uint64_t seed);
int gf_observe(const gf_batch* batch, size_t count, float* observations,
               uint8_t* masks, uint8_t* actors);
// rewards[count,2] in FIXED player coordinates. No automatic reset.
// Already-done rows are frozen, emit reward=0, and ignore their action.
int gf_step(gf_batch* batch, size_t count, const int32_t* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);
#ifdef __cplusplus
}
#endif
