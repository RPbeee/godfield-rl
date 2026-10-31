#pragma once
#include "godfield/revival_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI27: legacy-item-removal-subset-v27. Earlier APIs keep their dimensions.
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
// Options are conditional components; call g27_legal for full legality.
typedef g26_action g27_action;
typedef struct g27_batch g27_batch;
int g27_abi_version(void);
const char* g27_ruleset(void);
int g27_observation_size(void); //1085; no hidden RNG prefixes or populations.
int g27_options_width(void); //26, word25=item-removal240 hand slots.
int g27_hand_capacity(void);
size_t g27_action_size(void);
g27_batch* g27_create(size_t count, uint32_t max_decisions);
void g27_destroy(g27_batch* batch);
size_t g27_size(const g27_batch* batch);
// Status0 success/-1 NULL/-2 count or index/-3 invalid action or value.
int g27_reset_at(g27_batch* batch, size_t index, uint64_t seed);
// At most64 hidden fixture outcomes; rejected replacement changes nothing.
// NULL only for an empty prefix, which clears the configured prefix.
int g27_script_hit_rolls(g27_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int g27_script_bounce_targets(g27_batch* batch, size_t index, const uint8_t* targets, size_t count);
int g27_script_disease_rolls(g27_batch* batch, size_t index, const uint8_t* rolls, size_t count);
// Any uint64 ticket is valid: ticket % current total chooses a retained copy.
// Empty population consumes no ticket; one-copy population still consumes one.
int g27_script_forget_tickets(g27_batch* batch, size_t index, const uint64_t* tickets, size_t count);
// Any uint64 ticket is valid: ticket % current hand count chooses a slot.
// Empty hand consumes no ticket; one-copy hand still consumes one.
int g27_script_item_removal_tickets(g27_batch* batch, size_t index, const uint64_t* tickets, size_t count);
// Caller-owned row-major buffers; do not concurrently access one handle.
int g27_observe(const g27_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int g27_legal(const g27_batch* batch, size_t count, const g27_action* actions, uint8_t* legal);
int g27_sample_actions(const g27_batch* batch, size_t count, uint64_t* policy_seeds, g27_action* actions);
// Validate every active row before mutation. Fixed-seat rewards[count,2].
// Done rows freeze, reward0 and ignore malformed actions. No auto reset.
int g27_step(g27_batch* batch, size_t count, const g27_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);
#ifdef __cplusplus
}
#endif
