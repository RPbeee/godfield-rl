#pragma once
#include "godfield/counter_extensions_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v19: legacy-magic-block-subset-v19. Existing APIs keep their earlier dimensions.
// Extended 16-byte gw_action; gl..gk remain 6 bytes and gx/gy/gz/gu/gv remain 16 bytes.
// BLOCK_MAGIC cards27/54/55/152/165/182/183/185 use DEFEND.base on MAGIC input.
// Matching blockers cannot opt out via ordinary slots; prefix armor is allowed
// regardless of attribute. The original spell's MP/retention costs remain paid.
// Non-MAGIC armor uses printed DEF; weapon55 retains its normal attack boost.
// DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
// ATTACK_SET_FIRE/WATER/NEUTRAL(kind=7/8/9) selects a reachable final attribute;
// base=weapon slot, slots=all extra ordinary boosts and setters, target=seat.
// Options word8 marks setter hand slots; word9 marks removal slots.
// Removal195 belongs only in DEFEND/DEFEND_CAST slots, before any special base.
// Options word10 marks all six counter hand slots, selected in DEFEND slots.
// Prefix counters queue only when no special fires and ordinary residual is positive.
// All component masks are conditional: composed constraints need gw_legal().
// Counter order contains exactly the selected counter slots in FIFO order,
// even when a special response suppresses counter activation. First slot is the
// low nibble. Count <=16; all unused high nibbles and both reserved bytes are 0.
// Main actions have count=0/order=0. Prefix offsets match gl_action.
typedef gx_action gw_action;
typedef struct gw_batch gw_batch;
int gw_abi_version(void);
const char* gw_ruleset(void);
int gw_observation_size(void); // 750 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gw_options_width(void); // 11 component masks, not complete action constraints.
int gw_hand_capacity(void);
size_t gw_action_size(void);
gw_batch* gw_create(size_t count, uint32_t max_decisions);
void gw_destroy(gw_batch* batch);
size_t gw_size(const gw_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gw_reset_at(gw_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gw_script_hit_rolls(gw_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gw_script_bounce_targets(gw_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gw_script_disease_rolls(gw_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gw_observe(const gw_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gw_legal(const gw_batch* batch, size_t count, const gw_action* actions, uint8_t* legal);
int gw_sample_actions(const gw_batch* batch, size_t count, uint64_t* policy_seeds, gw_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gw_step(gw_batch* batch, size_t count, const gw_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
