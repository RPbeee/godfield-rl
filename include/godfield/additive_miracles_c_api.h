#pragma once
#include "godfield/counter_extensions_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v20: legacy-additive-miracles-subset-v20. Existing APIs keep their earlier dimensions.
// New 88-byte gp_action; earlier APIs retain their 6-byte/16-byte layouts.
// CAST_ADDITIVE(kind10): base=local retained kind0=197/1=217, slots=0, target=seat.
// ATTACK_ORDERED(kind11): base=WEAPON hand slot, slots=0, target=seat, extras1..64.
// Ordered refs0..15 are pre-consumption hand slots (unique and not base);
// refs16/17 are retained197/217 (repeatable, including after same-row first-use).
// New options words11/12 are hand additive candidates/local retained two-bit mask.
// Other kinds have extra_count=0; all reserved and unused extra bytes are zero.
// DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
// ATTACK_SET_FIRE/WATER/NEUTRAL(kind=7/8/9) selects a reachable final attribute;
// base=weapon slot, slots=all extra ordinary boosts and setters, target=seat.
// Options word8 marks setter hand slots; word9 marks removal slots.
// Removal195 belongs only in DEFEND/DEFEND_CAST slots, before any special base.
// Options word10 marks all six counter hand slots, selected in DEFEND slots.
// Prefix counters queue only when no special fires and ordinary residual is positive.
// All component masks are conditional: composed constraints need gp_legal().
// Counter order contains exactly the selected counter slots in FIFO order,
// even when a special response suppresses counter activation. First slot is the
// low nibble. Count <=16; all unused high nibbles and both reserved bytes are 0.
// Main actions have count=0/order=0. Prefix offsets match gl_action.
typedef struct gp_action {
    uint8_t kind;
    uint8_t base;
    uint16_t slots;
    uint8_t target;
    uint8_t reserved;
    uint8_t counter_count;
    uint8_t reserved2;
    uint64_t counter_order;
    uint8_t extra_count;
    uint8_t reserved_extra[7];
    uint8_t extras[64];
} gp_action;
typedef struct gp_batch gp_batch;
int gp_abi_version(void);
const char* gp_ruleset(void);
int gp_observation_size(void); // 903 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gp_options_width(void); // 13 component masks, not complete action constraints.
int gp_hand_capacity(void);
size_t gp_action_size(void);
gp_batch* gp_create(size_t count, uint32_t max_decisions);
void gp_destroy(gp_batch* batch);
size_t gp_size(const gp_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gp_reset_at(gp_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gp_script_hit_rolls(gp_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gp_script_bounce_targets(gp_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gp_script_disease_rolls(gp_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gp_observe(const gp_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gp_legal(const gp_batch* batch, size_t count, const gp_action* actions, uint8_t* legal);
int gp_sample_actions(const gp_batch* batch, size_t count, uint64_t* policy_seeds, gp_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gp_step(gp_batch* batch, size_t count, const gp_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
