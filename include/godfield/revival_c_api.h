#pragma once
#include "godfield/exchange_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI26: legacy-revival-subset-v26. Earlier APIs keep their dimensions.
// Same88-byte action and65th-ref offset17 as gi/gj; kinds0..13 unchanged.
// EXCHANGE(kind14): base=hand2slot,target255,slots/counterfields0,
// extra_count3,extras[0..2]=newHP/MP/YEN, each0..99, conserving current sum.
// HP0 is legal. All unused references, extra_tail and reserved bytes are0.
// This canonical bounded input domain excludes the old server's unchecked
// negative/over99 values. It costs no MP, creates no holding, and has no response.
// REVIVE244 is passive: before dead finalization, when at least2 dead flags
// are false, the first244 is consumed and HP increases10. Gift0, no new action.
// Scan order is fixedseat0 then1. Manual DISCARD is legal, normal USE is not.
// Options are conditional components; call g26_legal for full legality.
typedef gg_action g26_action;
typedef struct g26_batch g26_batch;
int g26_abi_version(void);
const char* g26_ruleset(void);
int g26_observation_size(void); //1062; no hidden RNG prefixes or populations.
int g26_options_width(void); //25, word24=exchange2 hand slots.
int g26_hand_capacity(void);
size_t g26_action_size(void);
g26_batch* g26_create(size_t count, uint32_t max_decisions);
void g26_destroy(g26_batch* batch);
size_t g26_size(const g26_batch* batch);
// Status0 success/-1 NULL/-2 count or index/-3 invalid action or value.
int g26_reset_at(g26_batch* batch, size_t index, uint64_t seed);
// At most64 hidden fixture outcomes; rejected replacement changes nothing.
// NULL only for an empty prefix, which clears the configured prefix.
int g26_script_hit_rolls(g26_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int g26_script_bounce_targets(g26_batch* batch, size_t index, const uint8_t* targets, size_t count);
int g26_script_disease_rolls(g26_batch* batch, size_t index, const uint8_t* rolls, size_t count);
// Any uint64 ticket is valid: ticket % current total chooses a retained copy.
// Empty population consumes no ticket; one-copy population still consumes one.
int g26_script_forget_tickets(g26_batch* batch, size_t index, const uint64_t* tickets, size_t count);
// Caller-owned row-major buffers; do not concurrently access one handle.
int g26_observe(const g26_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int g26_legal(const g26_batch* batch, size_t count, const g26_action* actions, uint8_t* legal);
int g26_sample_actions(const g26_batch* batch, size_t count, uint64_t* policy_seeds, g26_action* actions);
// Validate every active row before mutation. Fixed-seat rewards[count,2].
// Done rows freeze, reward0 and ignore malformed actions. No auto reset.
int g26_step(g26_batch* batch, size_t count, const g26_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);
#ifdef __cplusplus
}
#endif
