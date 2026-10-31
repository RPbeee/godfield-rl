#pragma once
#include "godfield/hp_absorption_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI29: legacy-self-damage-subset-v29. Earlier APIs keep their dimensions.
// Same88-byte action and65th-ref offset17 as gi/gj; kinds0..13 unchanged.
// EXCHANGE(kind14): base=hand2slot,target255,slots/counterfields0,
// extra_count3,extras[0..2]=newHP/MP/YEN, each0..99, conserving current sum.
// HP0 is legal. All unused references, extra_tail and reserved bytes are0.
// This canonical bounded input domain excludes the old server's unchecked
// negative/over99 values. It costs no MP, creates no holding, and has no response.
// REVIVE244 is passive: before dead finalization, when at least2 dead flags
// are false, the first244 is consumed and HP increases10. Gift0, no new action.
// Scan order is fixedseat0 then1. Manual DISCARD is legal, normal USE is not.
// REMOVE_ITEMS240 is a single USE hand base with target0/1. It removes one
// final target hand slot uniformly, with no target gift. Only first196 responds.
// Reflection consumption precedes sampling. The removed ID is public; its slot
// and population remain hidden. No ordinary armor/195/prefix196 is permitted.
// ABSORB_HP26/42/103 weapons and221MAGIC heal the current source by positive
// residual before damaging the target. Heal caps99; raw is not target-HP-clipped.
// CAST base15 and FREE heldbase47 are221. Older main indices remain unchanged.
// Magic221 only permits Free extras; it is not an attack modifier. Normal
// weapons26/42 allow existing ordered extras; chance103 does not. Mana57 cannot
// carry absorption.
// DAMAGE_TO_SELF53 applies positive residual to current source, then target.
// Self-target applies twice. Record each actual clipped loss independently.
// It is a guaranteed NE weapon base; existing boosts/ordered additions apply.
// No new action or spell holding is introduced. Source death is finalized only
// after both damage applications. Options components need g29_legal().
typedef g28_action g29_action;
typedef struct g29_batch g29_batch;
int g29_abi_version(void);
const char* g29_ruleset(void);
int g29_observation_size(void); //1135; no hidden RNG prefixes or populations.
int g29_options_width(void); //28, word27=self-damage53 hand membership slots.
int g29_hand_capacity(void);
size_t g29_action_size(void);
g29_batch* g29_create(size_t count, uint32_t max_decisions);
void g29_destroy(g29_batch* batch);
size_t g29_size(const g29_batch* batch);
// Status0 success/-1 NULL/-2 count or index/-3 invalid action or value.
int g29_reset_at(g29_batch* batch, size_t index, uint64_t seed);
// At most64 hidden fixture outcomes; rejected replacement changes nothing.
// NULL only for an empty prefix, which clears the configured prefix.
int g29_script_hit_rolls(g29_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int g29_script_bounce_targets(g29_batch* batch, size_t index, const uint8_t* targets, size_t count);
int g29_script_disease_rolls(g29_batch* batch, size_t index, const uint8_t* rolls, size_t count);
// Any uint64 ticket is valid: ticket % current total chooses a retained copy.
// Empty population consumes no ticket; one-copy population still consumes one.
int g29_script_forget_tickets(g29_batch* batch, size_t index, const uint64_t* tickets, size_t count);
// Any uint64 ticket is valid: ticket % current hand count chooses a slot.
// Empty hand consumes no ticket; one-copy hand still consumes one.
int g29_script_item_removal_tickets(g29_batch* batch, size_t index, const uint64_t* tickets, size_t count);
// Caller-owned row-major buffers; do not concurrently access one handle.
int g29_observe(const g29_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int g29_legal(const g29_batch* batch, size_t count, const g29_action* actions, uint8_t* legal);
int g29_sample_actions(const g29_batch* batch, size_t count, uint64_t* policy_seeds, g29_action* actions);
// Validate every active row before mutation. Fixed-seat rewards[count,2].
// Done rows freeze, reward0 and ignore malformed actions. No auto reset.
int g29_step(g29_batch* batch, size_t count, const g29_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);
#ifdef __cplusplus
}
#endif
