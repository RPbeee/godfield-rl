#pragma once
#include "godfield/legacy_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v13: legacy-counter-subset-v13. Existing APIs keep their earlier dimensions.
// Shared 6-byte gl_action. DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
// ATTACK_SET_FIRE/WATER/NEUTRAL(kind=7/8/9) selects a reachable final attribute;
// base=weapon slot, slots=all extra ordinary boosts and setters, target=seat.
// Options word8 marks setter hand slots; word9 marks removal slots.
// Removal195 belongs only in DEFEND/DEFEND_CAST slots, before any special base.
// Options word10 marks counter190 hand slots, selected in DEFEND slots.
// Prefix counters queue only when no special fires and ordinary residual is positive.
// All component masks are conditional: composed constraints need gk_legal().
typedef struct gk_batch gk_batch;
int gk_abi_version(void);
const char* gk_ruleset(void);
int gk_observation_size(void); // 515 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gk_options_width(void); // 11 component masks, not complete action constraints.
int gk_hand_capacity(void);
size_t gk_action_size(void);
gk_batch* gk_create(size_t count, uint32_t max_decisions);
void gk_destroy(gk_batch* batch);
size_t gk_size(const gk_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gk_reset_at(gk_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gk_script_hit_rolls(gk_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gk_script_bounce_targets(gk_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gk_script_disease_rolls(gk_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gk_observe(const gk_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gk_legal(const gk_batch* batch, size_t count, const gl_action* actions, uint8_t* legal);
int gk_sample_actions(const gk_batch* batch, size_t count, uint64_t* policy_seeds, gl_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gk_step(gk_batch* batch, size_t count, const gl_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
