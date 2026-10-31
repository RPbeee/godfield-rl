#pragma once
#include "godfield/additive_miracles_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v21: legacy-doubling-miracles-subset-v21. Existing APIs keep their earlier dimensions.
// 88-byte gq_action aliases ABI20 gp_action; earlier layouts remain unchanged.
// CAST_ADDITIVE(kind10) still accepts only local0=197/1=217; never214 alone.
// ATTACK_ORDERED(kind11) additionally accepts hand214 and retained ref18.
// Refs0..15 are unique pre-consumption hand slots,16/17 are retained197/217,
// and18 is retained214. Same-action first acquisition may precede a held ref.
// Options words13/14 mark hand214 candidates/retained214 bit0.
// DOUBLE_ATK costs6MP, doubles current power, then mixes NE attribute.
// All reserved bytes and unused references are zero.
// DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
// ATTACK_SET_FIRE/WATER/NEUTRAL(kind=7/8/9) selects a reachable final attribute;
// base=weapon slot, slots=all extra ordinary boosts and setters, target=seat.
// Options word8 marks setter hand slots; word9 marks removal slots.
// Removal195 belongs only in DEFEND/DEFEND_CAST slots, before any special base.
// Options word10 marks all six counter hand slots, selected in DEFEND slots.
// Prefix counters queue only when no special fires and ordinary residual is positive.
// All component masks are conditional: composed constraints need gq_legal().
// Counter order contains exactly the selected counter slots in FIFO order,
// even when a special response suppresses counter activation. First slot is the
// low nibble. Count <=16; all unused high nibbles and both reserved bytes are 0.
// Main actions have count=0/order=0. Prefix offsets match gl_action.
typedef gp_action gq_action;
typedef struct gq_batch gq_batch;
int gq_abi_version(void);
const char* gq_ruleset(void);
int gq_observation_size(void); // 924 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gq_options_width(void); // 15 component masks, not complete action constraints.
int gq_hand_capacity(void);
size_t gq_action_size(void);
gq_batch* gq_create(size_t count, uint32_t max_decisions);
void gq_destroy(gq_batch* batch);
size_t gq_size(const gq_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gq_reset_at(gq_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gq_script_hit_rolls(gq_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gq_script_bounce_targets(gq_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gq_script_disease_rolls(gq_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gq_observe(const gq_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gq_legal(const gq_batch* batch, size_t count, const gq_action* actions, uint8_t* legal);
int gq_sample_actions(const gq_batch* batch, size_t count, uint64_t* policy_seeds, gq_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gq_step(gq_batch* batch, size_t count, const gq_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
