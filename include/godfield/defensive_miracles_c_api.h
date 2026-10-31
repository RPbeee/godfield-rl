#pragma once
#include "godfield/legacy_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v10: legacy-defensive-miracles-subset-v10. Existing APIs keep their earlier dimensions.
// Shared 6-byte gl_action. DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
typedef struct gb_batch gb_batch;
int gb_abi_version(void);
const char* gb_ruleset(void);
int gb_observation_size(void); // 356 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gb_options_width(void); // 8 component masks, not complete action constraints.
int gb_hand_capacity(void);
size_t gb_action_size(void);
gb_batch* gb_create(size_t count, uint32_t max_decisions);
void gb_destroy(gb_batch* batch);
size_t gb_size(const gb_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gb_reset_at(gb_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gb_script_hit_rolls(gb_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gb_script_bounce_targets(gb_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gb_script_disease_rolls(gb_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gb_observe(const gb_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gb_legal(const gb_batch* batch, size_t count, const gl_action* actions, uint8_t* legal);
int gb_sample_actions(const gb_batch* batch, size_t count, uint64_t* policy_seeds, gl_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gb_step(gb_batch* batch, size_t count, const gl_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
