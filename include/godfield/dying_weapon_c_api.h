#pragma once
#include "godfield/status_miracles_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI37: legacy-dying-weapon-subset-v37. Earlier APIs keep their dimensions.
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
// BUY(kind16): base=hand4slot,target0/1, other fields0. Consume/gift1.
// First196 may reflect. Sample one final target physical copy, publishing its
// ID and fixed price. Self only samples; empty/insufficient YEN end immediately.
// Nonself affordable (including price0) waits at phase2 with actor=current buyer.
// BUY_DECLINE17/BUY_ACCEPT18: base/target255, all other fields0. Only phase2.
// YES removes target's selected copy, credits target(cap99), deals buyer a fresh
// copy(cap16), then charges buyer's YEN. NO leaves goods/resources unchanged.
// Waiting does not tick disease, distribute gifts or advance turns. Physical
// population, selected slot and future tickets are never observation features.
// HP gamble237 uses existing USE(kind3), base=hand237slot,target0/1, other0.
// On use choose delta -10(bit0)/+10(bit1) once and publish before response.
// Only first196 responds to None. Reflection preserves delta, without reroll.
// Resolve once at current target, clipping HP0/99; revival/death follows it.
// No MP, holding, boost or Free extra; consume/gift1. Future bits stay hidden.
// FLICK_MAGIC21/119/121/127/134/140 are physical, cost0, no holding.
// MAGIC response: mandatory bounce (DEFEND.base=carrier), no optout via slots.
// Prefix armor is consumed but its damage reduction is discarded;195 remains.
// Random target includes self; reuse script_bounce_targets. No hit reroll or MP.
// NonMAGIC: armor DEF0/1/2/3/4 remains ordinary;21 attacks/boosts5, no ordinary DEF.
// No new action/effect/tape. Public history exposes the used card and target;
// future targets remain hidden.
// SUNDRY238: added boost10/NE, cost0/holding0, consume/gift1 per physical copy.
// Only attack extras, no base/USE or response role. ATTACK uses slots;
// ATTACK_ORDERED uses unique hand refs. Not valid on an original percent weapon
// or MAGIC base. Paid magic after57 recalculates damage, erasing earlier boosts.
// FREE immediately after238 saves no MP; it never waives an earlier magic.
// Original server accepts some zero-DEF SUNDRY defense inputs; this canonical
// subset excludes them (UI/official acceptance is unverified).
// Status MAGIC locals0..4=202/208/211/212/224 (lower cure, DarkCloud,
// all cure, Cold, Heaven). USE physical, CAST_STATUS(kind19) retained local,
// target0/1, zero slots/counter/extras. MP3/5/5/6/15; use retains a copy.
// Free CAST_ORDERED bases48..52 are retained status locals; physical hand bases
// are unchanged. Ordinary DFS never reduces these effects;208 DARK is not ATK.
// Typed MAGIC reaction preserves payload and does not repay MP or retain again.
// Nondefensive status MAGIC response consumption accepted by the weak old
// server guard is excluded by this canonical subset; UI fidelity unverified.
// DYING_ATTACK117 is a LIGHT weapon ATK1/rate75/price10, no boost/DFS/MP.
// Use ATTACK with no extras; requested self is normalized to alive enemy.
// Printsub30 is dying attack power, never ordinary attack/DEF/MP. Death hook
// requires at least2 living dead-flags AFTER marking the holder dead, impossible
// with exactly2 seats. Revival244 takes an earlier continue, so does not bypass it.
// No dying queue/event is invented. Hit75 succeeds,76 misses; DarkCloud forces
// success without hit RNG. Only existing typed reactions and195 rules apply.
// Older ability indices and masks remain fixed. Options require g37_legal().
typedef g36_action g37_action;
typedef struct g37_batch g37_batch;
int g37_abi_version(void);
const char* g37_ruleset(void);
int g37_observation_size(void); //1368; no hidden RNG prefixes or populations.
int g37_options_width(void); //41; word40=main hand117 membership, all earlier words stable.
int g37_hand_capacity(void);
size_t g37_action_size(void);
g37_batch* g37_create(size_t count, uint32_t max_decisions);
void g37_destroy(g37_batch* batch);
size_t g37_size(const g37_batch* batch);
// Status0 success/-1 NULL/-2 count or index/-3 invalid action or value.
int g37_reset_at(g37_batch* batch, size_t index, uint64_t seed);
// At most64 hidden fixture outcomes; rejected replacement changes nothing.
// NULL only for an empty prefix, which clears the configured prefix.
int g37_script_hit_rolls(g37_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int g37_script_bounce_targets(g37_batch* batch, size_t index, const uint8_t* targets, size_t count);
int g37_script_disease_rolls(g37_batch* batch, size_t index, const uint8_t* rolls, size_t count);
// Any uint64 ticket is valid: ticket % current total chooses a retained copy.
// Empty population consumes no ticket; one-copy population still consumes one.
int g37_script_forget_tickets(g37_batch* batch, size_t index, const uint64_t* tickets, size_t count);
// Any uint64 ticket is valid: ticket % current hand count chooses a slot.
// Empty hand consumes no ticket; one-copy hand still consumes one.
int g37_script_item_removal_tickets(g37_batch* batch, size_t index, const uint64_t* tickets, size_t count);
// Hidden no-mortar target prefix, at most64 seats0/1. Mortar locks consume none.
int g37_script_pestle_targets(g37_batch* batch, size_t index, const uint8_t* targets, size_t count);
// Ticket modulo current physical hand count; empty consumes none, singleton one.
int g37_script_buy_tickets(g37_batch* batch, size_t index, const uint64_t* tickets, size_t count);
// At most64 hidden bits:0=-10,1=+10; rejected replacement is atomic.
int g37_script_hp_gamble_rolls(g37_batch* batch, size_t index, const uint8_t* rolls, size_t count);
// Caller-owned row-major buffers; do not concurrently access one handle.
int g37_observe(const g37_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int g37_legal(const g37_batch* batch, size_t count, const g37_action* actions, uint8_t* legal);
int g37_sample_actions(const g37_batch* batch, size_t count, uint64_t* policy_seeds, g37_action* actions);
// Validate every active row before mutation. Fixed-seat rewards[count,2].
// Done rows freeze, reward0 and ignore malformed actions. No auto reset.
int g37_step(g37_batch* batch, size_t count, const g37_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);
#ifdef __cplusplus
}
#endif
