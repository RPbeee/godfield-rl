#pragma once
#include "godfield/forgetting_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI25: legacy-exchange-subset-v25. Earlier APIs keep their dimensions.
// Same88-byte action and65th-ref offset17 as gi/gj; kinds0..13 unchanged.
// EXCHANGE(kind14): base=hand2slot,target255,slots/counterfields0,
// extra_count3,extras[0..2]=newHP/MP/YEN, each0..99, conserving current sum.
// HP0 is legal. All unused references, extra_tail and reserved bytes are0.
// This canonical bounded input domain excludes the old server's unchecked
// negative/over99 values. It costs no MP, creates no holding, and has no response.
// Options are conditional components; call gg_legal for full legality.
typedef gj_action gg_action;
typedef struct gg_batch gg_batch;
int gg_abi_version(void);
const char* gg_ruleset(void);
int gg_observation_size(void); //1043; no hidden RNG prefixes or populations.
int gg_options_width(void); //25, word24=exchange2 hand slots.
int gg_hand_capacity(void);
size_t gg_action_size(void);
gg_batch* gg_create(size_t count, uint32_t max_decisions);
void gg_destroy(gg_batch* batch);
size_t gg_size(const gg_batch* batch);
// Status0 success/-1 NULL/-2 count or index/-3 invalid action or value.
int gg_reset_at(gg_batch* batch, size_t index, uint64_t seed);
// At most64 hidden fixture outcomes; rejected replacement changes nothing.
// NULL only for an empty prefix, which clears the configured prefix.
int gg_script_hit_rolls(gg_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gg_script_bounce_targets(gg_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gg_script_disease_rolls(gg_batch* batch, size_t index, const uint8_t* rolls, size_t count);
// Any uint64 ticket is valid: ticket % current total chooses a retained copy.
// Empty population consumes no ticket; one-copy population still consumes one.
int gg_script_forget_tickets(gg_batch* batch, size_t index, const uint64_t* tickets, size_t count);
// Caller-owned row-major buffers; do not concurrently access one handle.
int gg_observe(const gg_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gg_legal(const gg_batch* batch, size_t count, const gg_action* actions, uint8_t* legal);
int gg_sample_actions(const gg_batch* batch, size_t count, uint64_t* policy_seeds, gg_action* actions);
// Validate every active row before mutation. Fixed-seat rewards[count,2].
// Done rows freeze, reward0 and ignore malformed actions. No auto reset.
int gg_step(gg_batch* batch, size_t count, const gg_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);
#ifdef __cplusplus
}
#endif
