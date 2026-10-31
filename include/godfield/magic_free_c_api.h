#pragma once
#include "godfield/wide_miracles_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v23: legacy-magic-free-subset-v23. Existing APIs keep their earlier dimensions.
// New 88-byte gi_action keeps the old16-byte prefix and extras64 at offset24.
// Offset17 is extra_tail (the65th ordered ref); reserved_extra has6 bytes.
// ATTACK_ORDERED(kind11) supports up to65 refs; old APIs remain max64/tail0.
// CAST_ORDERED(kind12): base=hand0..15/held19716/21717/old-main-held32+i,
// extras are unique Free hand slots (hand base:max15, held base:max16).
// DEFEND_ORDERED(kind13): base/target255,slots/counterfields0; extras is the
// full sequence hand0..15/held21020/held21521, including one relevant Free suffix.
// First noninitial Free waives only its predecessor; special Free suffix is not
// consumed. Glory counts all selected refs. Other attacks' magic defense uses
// remain outside this profile. All reserved and unused reference bytes are0.
// DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
// ATTACK_SET_FIRE/WATER/NEUTRAL(kind=7/8/9) selects a reachable final attribute;
// base=weapon slot, slots=all extra ordinary boosts and setters, target=seat.
// Options word8 marks setter hand slots; word9 marks removal slots.
// Removal195 belongs only in DEFEND/DEFEND_CAST slots, before any special base.
// Options word10 marks all six counter hand slots, selected in DEFEND slots.
// Prefix counters queue only when no special fires and ordinary residual is positive.
// All component masks are conditional: composed constraints need gi_legal().
// Counter order contains exactly the selected counter slots in FIFO order,
// even when a special response suppresses counter activation. First slot is the
// low nibble. Count <=16; all unused high nibbles and both reserved bytes are 0.
// Main actions have count=0/order=0. Prefix offsets match gl_action.
typedef struct gi_action {
    uint8_t kind;
    uint8_t base;
    uint16_t slots;
    uint8_t target;
    uint8_t reserved;
    uint8_t counter_count;
    uint8_t reserved2;
    uint64_t counter_order;
    uint8_t extra_count;
    uint8_t extra_tail;
    uint8_t reserved_extra[6];
    uint8_t extras[64];
} gi_action;
typedef struct gi_batch gi_batch;
int gi_abi_version(void);
const char* gi_ruleset(void);
int gi_observation_size(void); // 993 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gi_options_width(void); // 23 component masks, not complete action constraints.
int gi_hand_capacity(void);
size_t gi_action_size(void);
gi_batch* gi_create(size_t count, uint32_t max_decisions);
void gi_destroy(gi_batch* batch);
size_t gi_size(const gi_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gi_reset_at(gi_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gi_script_hit_rolls(gi_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gi_script_bounce_targets(gi_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gi_script_disease_rolls(gi_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gi_observe(const gi_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gi_legal(const gi_batch* batch, size_t count, const gi_action* actions, uint8_t* legal);
int gi_sample_actions(const gi_batch* batch, size_t count, uint64_t* policy_seeds, gi_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gi_step(gi_batch* batch, size_t count, const gi_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
