#pragma once
#include "godfield/magic_free_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// ABI24: legacy-forgetting-subset-v24. Earlier APIs keep their dimensions.
// Same88-byte action and65th-ref offset17 as gi_action; kinds0..13 unchanged.
// Forget241 is a single USE hand base with target0/1, slots/extras/counter0.
// No ordinary defense/195/prefix196 is permitted against this untyped effect;
// first196 can reflect it, including an unconsumed Free suffix in kind13.
// Holdings are sampled with copy-count weights in ascending CardId order.
// Acquisition order and original Flash XML abilityIndex are outside this API.
// Options are conditional components, not complete legal-action constraints.
typedef gi_action gj_action;
typedef struct gj_batch gj_batch;
int gj_abi_version(void);
const char* gj_ruleset(void);
int gj_observation_size(void); //1017; no hidden population or fixture outcomes.
int gj_options_width(void); //24, word23=forget241 hand slots.
int gj_hand_capacity(void);
size_t gj_action_size(void);
gj_batch* gj_create(size_t count, uint32_t max_decisions);
void gj_destroy(gj_batch* batch);
size_t gj_size(const gj_batch* batch);
// Status0 success/-1 NULL/-2 count or index/-3 invalid action or value.
int gj_reset_at(gj_batch* batch, size_t index, uint64_t seed);
// At most64 hidden fixture outcomes. NULL only for an empty prefix.
// Rejected replacements change nothing. Empty clears the configured prefix.
int gj_script_hit_rolls(gj_batch* batch, size_t index, const uint8_t* rolls, size_t count);
int gj_script_bounce_targets(gj_batch* batch, size_t index, const uint8_t* targets, size_t count);
int gj_script_disease_rolls(gj_batch* batch, size_t index, const uint8_t* rolls, size_t count);
// Any uint64 ticket is valid: ticket % current total chooses a retained copy.
// Empty population consumes no ticket; one-copy population still consumes one.
int gj_script_forget_tickets(gj_batch* batch, size_t index, const uint64_t* tickets, size_t count);
// Caller-owned row-major buffers; do not concurrently access one handle.
int gj_observe(const gj_batch* batch, size_t count, float* observations,
               uint16_t* options, uint8_t* actors, uint8_t* phases, uint8_t* pray);
int gj_legal(const gj_batch* batch, size_t count, const gj_action* actions, uint8_t* legal);
int gj_sample_actions(const gj_batch* batch, size_t count, uint64_t* policy_seeds, gj_action* actions);
// Validate every active row before mutation. Fixed-seat rewards[count,2].
// Done rows freeze, reward0 and ignore malformed actions. No auto reset.
int gj_step(gj_batch* batch, size_t count, const gj_action* actions,
            float* rewards, uint8_t* terminated, uint8_t* truncated);
#ifdef __cplusplus
}
#endif
