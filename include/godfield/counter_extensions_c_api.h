#pragma once
#include "godfield/legacy_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v14: legacy-counter-extensions-subset-v14. Existing APIs keep their earlier dimensions.
// Extended 16-byte gx_action; existing APIs retain their 6-byte actions. DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
// ATTACK_SET_FIRE/WATER/NEUTRAL(kind=7/8/9) selects a reachable final attribute;
// base=weapon slot, slots=all extra ordinary boosts and setters, target=seat.
// Options word8 marks setter hand slots; word9 marks removal slots.
// Removal195 belongs only in DEFEND/DEFEND_CAST slots, before any special base.
// Options word10 marks all five counter hand slots, selected in DEFEND slots.
// Prefix counters queue only when no special fires and ordinary residual is positive.
// All component masks are conditional: composed constraints need gx_legal().
// Counter order contains exactly the selected counter slots in FIFO order,
// even when a special response suppresses counter activation. First slot is the
// low nibble. Count <=16; all unused high nibbles and both reserved bytes are 0.
// Main actions have count=0/order=0. Prefix offsets match gl_action.
typedef struct gx_action {
    uint8_t kind;
    uint8_t base;
    uint16_t slots;
    uint8_t target;
    uint8_t reserved;
    uint8_t counter_count;
    uint8_t reserved2;
    uint64_t counter_order;
} gx_action;
typedef struct gx_batch gx_batch;
int gx_abi_version(void);
const char* gx_ruleset(void);
int gx_observation_size(void); // 661 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gx_options_width(void); // 11 component masks, not complete action constraints.
int gx_hand_capacity(void);
size_t gx_action_size(void);
gx_batch* gx_create(size_t count, uint32_t max_decisions);
void gx_destroy(gx_batch* batch);
size_t gx_size(const gx_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gx_reset_at(gx_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gx_script_hit_rolls(gx_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gx_script_bounce_targets(gx_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gx_script_disease_rolls(gx_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gx_observe(const gx_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gx_legal(const gx_batch* batch, size_t count, const gx_action* actions, uint8_t* legal);
int gx_sample_actions(const gx_batch* batch, size_t count, uint64_t* policy_seeds, gx_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gx_step(gx_batch* batch, size_t count, const gx_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
