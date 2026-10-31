#pragma once
#include "godfield/doubling_miracles_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v22: legacy-wide-miracles-subset-v22. Existing APIs keep their earlier dimensions.
// 88-byte gt_action aliases ABI21 gq_action; earlier layouts remain unchanged.
// WIDE_ATK209 is ordered-only: a hand ref or reusable retained ref19.
// Refs0..18 retain ABI21 meaning. Options words15/16 are hand209/held209 bit0.
// A completed wide attack targets the enemy even when target=self was specified.
// It has rate100 but consumes one hit roll unless the enemy has DarkCloud.
// NE wide weapons can use20/39/215 without removal195; MAGIC reactions reject.
// Reserved bytes and unused references must be zero.
// DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
// ATTACK_SET_FIRE/WATER/NEUTRAL(kind=7/8/9) selects a reachable final attribute;
// base=weapon slot, slots=all extra ordinary boosts and setters, target=seat.
// Options word8 marks setter hand slots; word9 marks removal slots.
// Removal195 belongs only in DEFEND/DEFEND_CAST slots, before any special base.
// Options word10 marks all six counter hand slots, selected in DEFEND slots.
// Prefix counters queue only when no special fires and ordinary residual is positive.
// All component masks are conditional: composed constraints need gt_legal().
// Counter order contains exactly the selected counter slots in FIFO order,
// even when a special response suppresses counter activation. First slot is the
// low nibble. Count <=16; all unused high nibbles and both reserved bytes are 0.
// Main actions have count=0/order=0. Prefix offsets match gl_action.
typedef gq_action gt_action;
typedef struct gt_batch gt_batch;
int gt_abi_version(void);
const char* gt_ruleset(void);
int gt_observation_size(void); // 945 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gt_options_width(void); // 17 component masks, not complete action constraints.
int gt_hand_capacity(void);
size_t gt_action_size(void);
gt_batch* gt_create(size_t count, uint32_t max_decisions);
void gt_destroy(gt_batch* batch);
size_t gt_size(const gt_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gt_reset_at(gt_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gt_script_hit_rolls(gt_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gt_script_bounce_targets(gt_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gt_script_disease_rolls(gt_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gt_observe(const gt_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gt_legal(const gt_batch* batch, size_t count, const gt_action* actions, uint8_t* legal);
int gt_sample_actions(const gt_batch* batch, size_t count, uint64_t* policy_seeds, gt_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gt_step(gt_batch* batch, size_t count, const gt_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
