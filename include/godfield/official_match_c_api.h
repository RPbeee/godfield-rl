#pragma once
#include <stddef.h>
#include <stdint.h>
#include "godfield/official_state_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// Match ABI2: query gfo_match_abi_version before using any command structure.
// ABI1 match clients must migrate; catalog/state ABI1 remains unchanged.
// Experimental local scheduling, NOT authoritative official-server equivalence.
typedef struct gfo_match gfo_match;
typedef struct gfo_match_config {
    uint32_t max_decisions;
    uint8_t initial_hand, hand_limit, external_grants, include_fate; // Both flags strictly0/1, default0.
    int32_t initial_hp, initial_mp, initial_cp;
    uint8_t local_guardian_payloads; // Strictly0/1, default0: local automatic Earth/Moon payloads.
    uint8_t reserved_tail[3];
} gfo_match_config;
typedef struct gfo_match_player {
    uint64_t id;
    const char* name; size_t name_size; // Strict UTF-8, embedded NUL preserved.
    uint8_t team, reserved[7];
} gfo_match_player;
typedef struct gfo_match_command {
    uint64_t actor, target;
    uint8_t kind, has_target, item_count, reserved[5]; // play0/defend1/discard2/pray3/exchange4/sell5/buy6/accept7/decline8/sacrifice9
    uint64_t items[18]; // Own physical tokens, unused tail must be0. Token0 valid.
    int32_t hp, mp, cp; // Allocation for exchange only; otherwise all0.
    uint8_t has_allocation, reserved_tail[3]; // Required for exchange, forbidden otherwise.
} gfo_match_command;
// Additive driver API1; existing Match ABI2/Command184 remain unchanged.
typedef struct gfo_match_choice {
    gfo_match_command command;
    uint64_t fate_actor, fate_token;
    uint8_t kind, reserved[7]; //0 Command,1 FateRequest; the inactive variant is entirely zero.
} gfo_match_choice;
typedef struct gfo_match_guardian_action {
    uint64_t source, target;
    uint16_t model;
    uint8_t has_target, reserved[5];
} gfo_match_guardian_action;
typedef struct gfo_match_guardian_payload_action {
    uint64_t source, target;
    uint16_t models[18];
    uint8_t guardian, model_count, has_target, reserved[9];
} gfo_match_guardian_payload_action;
typedef struct gfo_match_phenomenon_action {
    uint64_t recipient;
    uint16_t model;
    uint8_t has_recipient, guardian_count, guardians[9], reserved[3];
} gfo_match_phenomenon_action;
typedef struct gfo_match_item_source { uint64_t owner, token; } gfo_match_item_source;
typedef struct gfo_match_item_shuffle_action {
    const gfo_match_item_source* order;
    size_t count;
} gfo_match_item_shuffle_action;
typedef struct gfo_match_fate_action {
    uint64_t actor, token;
    gfo_match_phenomenon_action outcome;
    gfo_match_item_shuffle_action shuffle;
    uint8_t has_shuffle, reserved[7];
} gfo_match_fate_action;
typedef struct gfo_match_devil_action {
    uint64_t target, removed[2];
    uint16_t model;
    uint8_t resource, has_resource, removed_count, reserved[3];
} gfo_match_devil_action;
typedef gfo_match_devil_action gfo_match_grant_action;
typedef struct gfo_match_status {
    uint64_t decisions, owner, actor;
    uint64_t player_ids[9]; // Stable player-vector order for rewards.
    double rewards[9];
    uint64_t winners[9];
    uint8_t player_count, winner_count, phase, has_actor, terminated, truncated, reserved[2]; // main0/response1/finished2/purchase3/grant4(no actor)
} gfo_match_status;

int gfo_match_abi_version(void);
int gfo_match_driver_api_version(void);
const char* gfo_match_choice_rule(void);
const char* gfo_match_driver_rule(void);
const char* gfo_match_ruleset(void);
const char* gfo_match_scope(void);
const char* gfo_match_self_attack_rule(void);
int gfo_match_catalog_count(void);
int gfo_match_gift_weight(void);
int gfo_match_supported(uint16_t model);
// Actual configured artifact pool; ordinary Command capability/global499 above
// remain unchanged. include_fate adds207 only to this pool (237models/weight500).
int gfo_match_gift_supported(const gfo_match* match, uint16_t model, uint8_t* out);
int gfo_match_get_gift_weight(const gfo_match* match, uint16_t* out);
size_t gfo_match_config_size(void);
size_t gfo_match_player_size(void);
size_t gfo_match_command_size(void);
size_t gfo_match_choice_size(void);
size_t gfo_match_status_size(void);
size_t gfo_match_guardian_action_size(void);
int gfo_match_guardian_supported(uint16_t model);
size_t gfo_match_guardian_payload_action_size(void);
int gfo_match_guardian_payload_supported(uint8_t guardian, uint16_t model);
size_t gfo_match_phenomenon_action_size(void);
int gfo_match_phenomenon_supported(uint16_t model);
size_t gfo_match_item_source_size(void);
size_t gfo_match_item_shuffle_action_size(void);
int gfo_match_item_shuffle_supported(uint16_t model);
size_t gfo_match_fate_action_size(void);
int gfo_match_fate_supported(uint16_t model);
size_t gfo_match_devil_action_size(void);
int gfo_match_devil_supported(uint16_t model);
size_t gfo_match_grant_action_size(void);
int gfo_match_grant_supported(uint16_t model);
// Status0 success/-1 NULL/-2 invalid/-3 exception/-4 short buffer/-5 unsupported.
// Every error leaves the complete mutable match, policy RNG and all outputs
// unchanged. Storage must be valid, nonoverlapping caller objects. Zero padding.
// NULL config uses defaults. Player setup is copied; IDs need not equal seats.
int gfo_match_create(const gfo_match_config* config, const gfo_match_player* players,
                     size_t count, uint64_t seed, gfo_match** out);
void gfo_match_destroy(gfo_match* match);
int gfo_match_reset(gfo_match* match, const gfo_match_player* players,
                    size_t count, uint64_t seed, gfo_match_status* out);
// Privileged quiescent scene initialization, not a checkpoint/server replay.
// Input must be a live immutable state from this same library. An independent
// deep copy is validated against the Match scope before committing. Existing
// config is retained; RNG/counters/pending work are initialized, not imported.
// A state with an attached active attack is rejected as unsupported (-5).
int gfo_match_load_scene(gfo_match* match, const gfo_state* state,
                         uint64_t owner, uint64_t seed, gfo_match_status* out);
int gfo_match_get_status(const gfo_match* match, gfo_match_status* out);
int gfo_match_legal(const gfo_match* match, const gfo_match_command* command, uint8_t* out);
int gfo_match_step(gfo_match* match, const gfo_match_command* command, gfo_match_status* out);
// Visible-policy choice plus owned207 request at an ordinary main boundary.
// FateRequest validates its source/token only, without selecting future effects.
// Sampling and choice_step are separate transactions: commit a trial policy RNG
// only after step success. No sampling changes the Match.
int gfo_match_choice_legal(const gfo_match* match, const gfo_match_choice* choice, uint8_t* out);
int gfo_match_sample_choice(const gfo_match* match, uint64_t* policy_rng, gfo_match_choice* out);
int gfo_match_choice_step(gfo_match* match, const gfo_match_choice* choice,
                          uint64_t* chance_rng, gfo_match_status* out);
// Exactly one accepted decision: ordinary choice, confusion, or grant dispatch.
// Match/status/both RNGs commit together. All caller output storage must be
// valid and nonoverlapping; policy_rng==chance_rng is explicitly rejected(-2).
// Equal seed values in distinct storage are valid. NULL required pointers are-1.
int gfo_match_rollout_step(gfo_match* match, uint64_t* policy_rng,
                           uint64_t* chance_rng, gfo_match_status* out);
// Privileged externally resolved ordinary main command for a confused owner.
// This step consumes a supplied command; sample_confusion is a separate local helper.
// Uses the unchanged184-byte command.
int gfo_match_confusion_legal(const gfo_match* match, const gfo_match_command* command, uint8_t* out);
int gfo_match_confusion_step(gfo_match* match, const gfo_match_command* command, gfo_match_status* out);
// Explicit canonical action entry, separate from automatic scheduling/summoning.
// Source must own the matching guardian. Absent target requires canonical0.
int gfo_match_guardian_legal(const gfo_match* match, const gfo_match_guardian_action* action, uint8_t* out);
int gfo_match_guardian_step(gfo_match* match, const gfo_match_guardian_action* action, gfo_match_status* out);
// Externally supplied Earth/Moon ordinary-model composition, not a selector.
// Models are catalog IDs, not owned item tokens; unused model tail must be0.
// guardian is Earth9 or Moon10. No source items, ordinary MP fee or retention.
int gfo_match_guardian_payload_legal(const gfo_match* match, const gfo_match_guardian_payload_action* action, uint8_t* out);
int gfo_match_guardian_payload_step(gfo_match* match, const gfo_match_guardian_payload_action* action, gfo_match_status* out);
// Externally supplied phenomenon, not a random selector.291..293 await responses.
// 291..294 require an alive recipient; only296 accepts guardian assignments (1..10),
// exactly one per alive player in input order. Unused fields/tails must be0.
int gfo_match_phenomenon_legal(const gfo_match* match, const gfo_match_phenomenon_action* action, uint8_t* out);
int gfo_match_phenomenon_step(gfo_match* match, const gfo_match_phenomenon_action* action, gfo_match_status* out);
// Privileged295 permutation manifest, not a normal policy action/query.
// Each (owner,token) alive copy occurs exactly once; destinations are existing
// alive slots in player/inventory order. count<=162 is checked before reading.
// NULL order is allowed only for count0. The source array is copied, not retained.
int gfo_match_item_shuffle_legal(const gfo_match* match, const gfo_match_item_shuffle_action* action, uint8_t* out);
int gfo_match_item_shuffle_step(gfo_match* match, const gfo_match_item_shuffle_action* action, gfo_match_status* out);
// Atomic207 use plus an externally supplied outcome; no random selector.
//295 requires shuffle; other outcomes forbid it. Inactive shuffle must be NULL0.
// The manifest covers alive inventories AFTER207 is consumed, excluding gifts.
int gfo_match_fate_legal(const gfo_match* match, const gfo_match_fate_action* action, uint8_t* out);
int gfo_match_fate_step(gfo_match* match, const gfo_match_fate_action* action, gfo_match_status* out);
// Externally supplied devil outcome, not an appearance/target selector or gift.
//240..242 accept no payload;243 accepts exactly min(2,target inventory) distinct
// owned tokens;244 requires resource HP0/MP1/CP2. All inactive fields are zero.
int gfo_match_devil_legal(const gfo_match* match, const gfo_match_devil_action* action, uint8_t* out);
int gfo_match_devil_step(gfo_match* match, const gfo_match_devil_action* action, gfo_match_status* out);
// Opt-in apocalypse reserved-gift continuation. Only the pending recipient may
// be selected. Model0 draws one normal artifact;240..244 apply a supplied devil
// and retain the original debt. Each resolution is one decision, not a selector.
int gfo_match_grant_legal(const gfo_match* match, const gfo_match_grant_action* action, uint8_t* out);
int gfo_match_grant_step(gfo_match* match, const gfo_match_grant_action* action, gfo_match_status* out);
// Policy RNG is a distinct caller-owned uint64 stream. This never advances the
// game's RNG/state. On failure neither RNG nor command is changed.
int gfo_match_sample_action(const gfo_match* match, uint64_t* policy_rng, gfo_match_command* out);
// Local visible-policy driver for a positive-confusion main owner only.
// Same proposal law as sample_action on a count0 scene; no automatic server
// selector is claimed. Sampling changes only chance_rng. Apply separately via
// confusion_step, which performs the single count debit. Errors preserve out/RNG.
int gfo_match_sample_confusion(const gfo_match* match, uint64_t* chance_rng, gfo_match_command* out);
// Privileged local Earth/Moon payload driver at a main boundary. The alive
// source need not be the ordinary owner; current guardian determines the domain.
// Selection ignores source MP/hand. Only chance_rng changes, no play is committed.
// Output uses the existing64-byte payload, explicit target and zeroed fixed tails.
int gfo_match_sample_guardian_payload(const gfo_match* match, uint64_t source, uint64_t* chance_rng,
                                      gfo_match_guardian_payload_action* out);
// Privileged local chance helper, only in phase4. Uses the current recipient's
// raw inventory for243, not a policy-visible hand. One bounded100 ticket:
//240=0..6/241=7..11/242=12..14/243=15..19/244=20..24/normal=25..99.
//243 uses partial Fisher-Yates (including singleton draws),244 uniform HP/MP/CP.
// No server selector is claimed. Only chance_rng changes on success; the match,
// normal policy RNG and game RNG do not change. Errors preserve RNG/output.
int gfo_match_sample_grant(const gfo_match* match, uint64_t* chance_rng, gfo_match_grant_action* out);
// Privileged local Fate outcome/target/guardian/shuffle driver for one owned207.
// Main owner only; confusion may remain. No game/policy RNG or state is changed.
// order is caller-owned, capacity<=162; NULL is allowed only at capacity0.
//295 uses the post207-consumption alive manifest. Success copies only count
// records and points out.shuffle.order at caller storage (NULL for count0).
// Non295/empty295 need no buffer. Short-4, errors and all failures preserve
// chance_rng, the entire out header and every byte of order, including its tail.
int gfo_match_sample_fate(const gfo_match* match, uint64_t token, uint64_t* chance_rng,
                          gfo_match_item_source* order, size_t capacity, gfo_match_fate_action* out);
// Explicit spectator:has_viewer0/viewer0. Otherwise has_viewer1 and an actual ID.
// required includes NUL. NULL0 queries length. Short-4 changes no output bytes or
// required length, and observation does not mutate any match state.
int gfo_match_observe(const gfo_match* match, uint8_t has_viewer, uint64_t viewer,
                      char* buffer, size_t capacity, size_t* required);
#ifdef __cplusplus
}
#endif
