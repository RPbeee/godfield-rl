#pragma once
#include "godfield/counter_extensions_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI v18: legacy-mana-weapon-subset-v18. Existing APIs keep their earlier dimensions.
// Extended 16-byte gv_action; gl..gk remain 6 bytes and gx/gy/gz/gu remain 16 bytes.
// Mana weapon57 is an ATTACK base only. It spends all current MP and uses
// 2*MP as base power before ordinary boosts/setters. MP0/ATK0 is legal and
// still prevents PRAY. Reflection preserves weapon type and recorded mana.
// Additional MAGIC extras, MAGIC_FREE and their ordering are not supported.
// DEFEND base is a hand reaction slot.
// DEFEND_CAST(kind=6) base is retained defensive kind 0=210/1=215; slots are prefix armor.
// Options word7 is its local two-bit mask; target must be 255.
// ATTACK_SET_FIRE/WATER/NEUTRAL(kind=7/8/9) selects a reachable final attribute;
// base=weapon slot, slots=all extra ordinary boosts and setters, target=seat.
// Options word8 marks setter hand slots; word9 marks removal slots.
// Removal195 belongs only in DEFEND/DEFEND_CAST slots, before any special base.
// Options word10 marks all six counter hand slots, selected in DEFEND slots.
// Prefix counters queue only when no special fires and ordinary residual is positive.
// All component masks are conditional: composed constraints need gv_legal().
// Counter order contains exactly the selected counter slots in FIFO order,
// even when a special response suppresses counter activation. First slot is the
// low nibble. Count <=16; all unused high nibbles and both reserved bytes are 0.
// Main actions have count=0/order=0. Prefix offsets match gl_action.
typedef gx_action gv_action;
typedef struct gv_batch gv_batch;
int gv_abi_version(void);
const char* gv_ruleset(void);
int gv_observation_size(void); // 732 floats/row; no hidden RNG, fixture prefixes or worsening counter.
int gv_options_width(void); // 11 component masks, not complete action constraints.
int gv_hand_capacity(void);
size_t gv_action_size(void);
gv_batch* gv_create(size_t count, uint32_t max_decisions);
void gv_destroy(gv_batch* batch);
size_t gv_size(const gv_batch* batch);

// Caller-owned row-major buffers, no concurrent use of a batch handle.
// Status: 0 success, -1 NULL, -2 count/index mismatch, -3 invalid action/value.
int gv_reset_at(gv_batch* batch, size_t index, uint64_t seed);
// Fixture prefixes, up to 64 outcomes. Invalid replacement changes nothing.
// NULL is allowed only for an empty prefix, which clears the configured prefix.
int gv_script_hit_rolls(gv_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gv_script_bounce_targets(gv_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gv_script_disease_rolls(gv_batch* batch, size_t index, const uint8_t* rolls, size_t count); // 0..99
int gv_observe(const gv_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gv_legal(const gv_batch* batch, size_t count, const gv_action* actions, uint8_t* legal);
int gv_sample_actions(const gv_batch* batch, size_t count, uint64_t* policy_seeds, gv_action* actions);
// All active rows validate before any mutation. Fixed-seat rewards[count,2].
// Done rows freeze, emit rewards=0 and ignore malformed actions. No auto reset.
int gv_step(gv_batch* batch, size_t count, const gv_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);

#ifdef __cplusplus
}
#endif
