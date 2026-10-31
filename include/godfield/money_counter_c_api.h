#pragma once
#include "godfield/counter_extensions_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v15: legacy-money-counter-subset-v15. Existing APIs keep their earlier dimensions.
// Extended 16-byte gy_action; earlier APIs retain their layouts (gl..gk: 6 bytes; gx: 16 bytes).
// DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
// ATTACK_SET_FIRE/WATER/NEUTRAL(kind=7/8/9) selects a reachable final attribute;
// base=weapon slot, slots=all extra ordinary boosts and setters, target=seat.
// Options word8 marks setter hand slots; word9 marks removal slots.
// Removal195 belongs only in DEFEND/DEFEND_CAST slots, before any special base.
// Options word10 marks all six counter hand slots, selected in DEFEND slots.
// Prefix counters queue only when no special fires and ordinary residual is positive.
// All component masks are conditional: composed constraints need gy_legal().
// Counter order contains exactly the selected counter slots in FIFO order,
// even when a special response suppresses counter activation. First slot is the
// low nibble. Count <=16; all unused high nibbles and both reserved bytes are 0.
// Main actions have count=0/order=0. Prefix offsets match gl_action.
typedef gx_action gy_action;
typedef struct gy_batch gy_batch;
int gy_abi_version(void);
const char* gy_ruleset(void);
int gy_observation_size(void); // 679 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gy_options_width(void); // 11 component masks, not complete action constraints.
int gy_hand_capacity(void);
size_t gy_action_size(void);
gy_batch* gy_create(size_t count, uint32_t max_decisions);
void gy_destroy(gy_batch* batch);
size_t gy_size(const gy_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gy_reset_at(gy_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gy_script_hit_rolls(gy_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gy_script_bounce_targets(gy_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gy_script_disease_rolls(gy_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gy_observe(const gy_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gy_legal(const gy_batch* batch, size_t count, const gy_action* actions, uint8_t* legal);
int gy_sample_actions(const gy_batch* batch, size_t count, uint64_t* policy_seeds, gy_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gy_step(gy_batch* batch, size_t count, const gy_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
