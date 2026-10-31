#pragma once
#include "godfield/legacy_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v11: legacy-attribute-change-subset-v11. Existing APIs keep their earlier dimensions.
// Shared 6-byte gl_action. DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
// ATTACK_SET_FIRE/WATER/NEUTRAL(kind=7/8/9) selects a reachable final attribute;
// base=weapon slot, slots=all extra ordinary boosts and setters, target=seat.
// Options word8 marks setter hand slots. All composed constraints need ga_legal().
typedef struct ga_batch ga_batch;
int ga_abi_version(void);
const char* ga_ruleset(void);
int ga_observation_size(void); // 375 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int ga_options_width(void); // 9 component masks, not complete action constraints.
int ga_hand_capacity(void);
size_t ga_action_size(void);
ga_batch* ga_create(size_t count, uint32_t max_decisions);
void ga_destroy(ga_batch* batch);
size_t ga_size(const ga_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int ga_reset_at(ga_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int ga_script_hit_rolls(ga_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int ga_script_bounce_targets(ga_batch* batch, size_t index, const uint8_t* targets, size_t count);
int ga_script_disease_rolls(ga_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int ga_observe(const ga_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int ga_legal(const ga_batch* batch, size_t count, const gl_action* actions, uint8_t* legal);
int ga_sample_actions(const ga_batch* batch, size_t count, uint64_t* policy_seeds, gl_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int ga_step(ga_batch* batch, size_t count, const gl_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
