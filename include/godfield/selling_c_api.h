#pragma once
#include "godfield/pestle_mortar_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI31: legacy-selling-subset-v31. Earlier APIs keep their dimensions.
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
// after both damage applications.
// PESTLE99 uses ATTACK base=hand99slot, target0/1 (ignored on selection), no extras.
// The first not.dead mortar245 holder in fixed seat order receives LIGHT999;
// absent mortar, a random alive seat including self receives LIGHT30.
// 245 is never consumed by selection. DISCARD may select it, but consume skips
// it; a245-only discard ends the turn with hand unchanged and no gift.
// Server response semantics are retained even for mortar999. Native public
// provenance records initial selection; reflection does not choose again.
// SELL(kind15): base=hand3slot, slots=one distinct physical goods bit,
// target0/1, all extras/counterfields0. Both cards are consumed upfront; gift1.
// Resolve target charge(YEN->MP->HP), fresh goods delivery(cap16, failure not
// rolled back), then current-source credit(cap99). Only first196 responds.
// Goods may be weapons/MAGIC/245/239/TRADE, never a retained ability or base.
// Self sale obeys the same sequential charge/delivery/credit; it is not a no-op.
// Options components require g31_legal().
typedef g30_action g31_action;
typedef struct g31_batch g31_batch;
int g31_abi_version(void);
const char* g31_ruleset(void);
int g31_observation_size(void); //1221; no hidden RNG prefixes or populations.
int g31_options_width(void); //32, word30=sell3 bases, word31=physical sale goods candidates.
int g31_hand_capacity(void);
size_t g31_action_size(void);
g31_batch* g31_create(size_t count, uint32_t max_decisions);
void g31_destroy(g31_batch* batch);
size_t g31_size(const g31_batch* batch);
// Status0 success/-1 NULL/-2 count or index/-3 invalid action or value.
int g31_reset_at(g31_batch* batch, size_t index, uint64_t seed);
// At most64 hidden fixture outcomes; rejected replacement changes nothing.
// NULL only for an empty prefix, which clears the configured prefix.
int g31_script_hit_rolls(g31_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int g31_script_bounce_targets(g31_batch* batch, size_t index, const uint8_t* targets, size_t count);
int g31_script_disease_rolls(g31_batch* batch, size_t index, const uint8_t* rolls, size_t count);
// Any uint64 ticket is valid: ticket % current total chooses a retained copy.
// Empty population consumes no ticket; one-copy population still consumes one.
int g31_script_forget_tickets(g31_batch* batch, size_t index, const uint64_t* tickets, size_t count);
// Any uint64 ticket is valid: ticket % current hand count chooses a slot.
// Empty hand consumes no ticket; one-copy hand still consumes one.
int g31_script_item_removal_tickets(g31_batch* batch, size_t index, const uint64_t* tickets, size_t count);
// Hidden no-mortar target prefix, at most64 seats0/1. Mortar locks consume none.
int g31_script_pestle_targets(g31_batch* batch, size_t index, const uint8_t* targets, size_t count);
// Caller-owned row-major buffers; do not concurrently access one handle.
int g31_observe(const g31_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int g31_legal(const g31_batch* batch, size_t count, const g31_action* actions, uint8_t* legal);
int g31_sample_actions(const g31_batch* batch, size_t count, uint64_t* policy_seeds, g31_action* actions);
// Validate every active row before mutation. Fixed-seat rewards[count,2].
// Done rows freeze, reward0 and ignore malformed actions. No auto reset.
int g31_step(g31_batch* batch, size_t count, const g31_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);
#ifdef __cplusplus
}
#endif
