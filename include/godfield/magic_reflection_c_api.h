#pragma once
#include "godfield/counter_extensions_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v16: legacy-magic-reflection-subset-v16. Existing APIs keep their earlier dimensions.
// Extended 16-byte gz_action; gl..gk remain 6 bytes and gx/gy remain 16 bytes.
// Magic-reflect cards40/171/174/178 use DEFEND.base against all MAGIC types.
// Armor171/174/178 must not opt out via ordinary slots on MAGIC input;
// against other categories they remain ordinary DEF9/10/11 armor.
// Reflection preserves the raw pending payload and does not reroll chance.
// DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
// ATTACK_SET_FIRE/WATER/NEUTRAL(kind=7/8/9) selects a reachable final attribute;
// base=weapon slot, slots=all extra ordinary boosts and setters, target=seat.
// Options word8 marks setter hand slots; word9 marks removal slots.
// Removal195 belongs only in DEFEND/DEFEND_CAST slots, before any special base.
// Options word10 marks all six counter hand slots, selected in DEFEND slots.
// Prefix counters queue only when no special fires and ordinary residual is positive.
// All component masks are conditional: composed constraints need gz_legal().
// Counter order contains exactly the selected counter slots in FIFO order,
// even when a special response suppresses counter activation. First slot is the
// low nibble. Count <=16; all unused high nibbles and both reserved bytes are 0.
// Main actions have count=0/order=0. Prefix offsets match gl_action.
typedef gx_action gz_action;
typedef struct gz_batch gz_batch;
int gz_abi_version(void);
const char* gz_ruleset(void);
int gz_observation_size(void); // 696 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gz_options_width(void); // 11 component masks, not complete action constraints.
int gz_hand_capacity(void);
size_t gz_action_size(void);
gz_batch* gz_create(size_t count, uint32_t max_decisions);
void gz_destroy(gz_batch* batch);
size_t gz_size(const gz_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gz_reset_at(gz_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gz_script_hit_rolls(gz_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gz_script_bounce_targets(gz_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gz_script_disease_rolls(gz_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gz_observe(const gz_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gz_legal(const gz_batch* batch, size_t count, const gz_action* actions, uint8_t* legal);
int gz_sample_actions(const gz_batch* batch, size_t count, uint64_t* policy_seeds, gz_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gz_step(gz_batch* batch, size_t count, const gz_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
