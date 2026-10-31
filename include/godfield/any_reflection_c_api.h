#pragma once
#include "godfield/counter_extensions_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v17: legacy-any-reflection-subset-v17. Existing APIs keep their earlier dimensions.
// Extended 16-byte gu_action; gl..gk remain 6 bytes and gx/gy/gz remain 16 bytes.
// Any-reflect196 uses DEFEND.base. Empty slots means 196 first, which bypasses
// attribute/Dark/counter checks. Nonempty slots precede196: MAGIC is unrestricted;
// non-MAGIC must be non-counter and neutral after any selected removal195.
// No ignored suffix selection is represented by this semantic action.
// Reflection preserves raw payload and does not reroll chance.
// DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
// ATTACK_SET_FIRE/WATER/NEUTRAL(kind=7/8/9) selects a reachable final attribute;
// base=weapon slot, slots=all extra ordinary boosts and setters, target=seat.
// Options word8 marks setter hand slots; word9 marks removal slots.
// Removal195 belongs only in DEFEND/DEFEND_CAST slots, before any special base.
// Options word10 marks all six counter hand slots, selected in DEFEND slots.
// Prefix counters queue only when no special fires and ordinary residual is positive.
// All component masks are conditional: composed constraints need gu_legal().
// Counter order contains exactly the selected counter slots in FIFO order,
// even when a special response suppresses counter activation. First slot is the
// low nibble. Count <=16; all unused high nibbles and both reserved bytes are 0.
// Main actions have count=0/order=0. Prefix offsets match gl_action.
typedef gx_action gu_action;
typedef struct gu_batch gu_batch;
int gu_abi_version(void);
const char* gu_ruleset(void);
int gu_observation_size(void); // 713 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gu_options_width(void); // 11 component masks, not complete action constraints.
int gu_hand_capacity(void);
size_t gu_action_size(void);
gu_batch* gu_create(size_t count, uint32_t max_decisions);
void gu_destroy(gu_batch* batch);
size_t gu_size(const gu_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gu_reset_at(gu_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gu_script_hit_rolls(gu_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gu_script_bounce_targets(gu_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gu_script_disease_rolls(gu_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gu_observe(const gu_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gu_legal(const gu_batch* batch, size_t count, const gu_action* actions, uint8_t* legal);
int gu_sample_actions(const gu_batch* batch, size_t count, uint64_t* policy_seeds, gu_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gu_step(gu_batch* batch, size_t count, const gu_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
