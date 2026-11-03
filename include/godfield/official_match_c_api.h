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
    uint8_t initial_hand, hand_limit, reserved[2];
    int32_t initial_hp, initial_mp, initial_cp;
    uint8_t reserved_tail[4];
} gfo_match_config;
typedef struct gfo_match_player {
    uint64_t id;
    const char* name; size_t name_size; // Strict UTF-8, embedded NUL preserved.
    uint8_t team, reserved[7];
} gfo_match_player;
typedef struct gfo_match_command {
    uint64_t actor, target;
    uint8_t kind, has_target, item_count, reserved[5]; // play0/defend1/discard2/pray3/exchange4/sell5/buy6/accept7/decline8
    uint64_t items[18]; // Own physical tokens, unused tail must be0. Token0 valid.
    int32_t hp, mp, cp; // Allocation for exchange only; otherwise all0.
    uint8_t has_allocation, reserved_tail[3]; // Required for exchange, forbidden otherwise.
} gfo_match_command;
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
typedef struct gfo_match_status {
    uint64_t decisions, owner, actor;
    uint64_t player_ids[9]; // Stable player-vector order for rewards.
    double rewards[9];
    uint64_t winners[9];
    uint8_t player_count, winner_count, phase, has_actor, terminated, truncated, reserved[2]; // main0/response1/finished2/purchase3
} gfo_match_status;

int gfo_match_abi_version(void);
const char* gfo_match_ruleset(void);
const char* gfo_match_scope(void);
int gfo_match_catalog_count(void);
int gfo_match_gift_weight(void);
int gfo_match_supported(uint16_t model);
size_t gfo_match_config_size(void);
size_t gfo_match_player_size(void);
size_t gfo_match_command_size(void);
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
int gfo_match_load_scene(gfo_match* match, const gfo_state* state,
                         uint64_t owner, uint64_t seed, gfo_match_status* out);
int gfo_match_get_status(const gfo_match* match, gfo_match_status* out);
int gfo_match_legal(const gfo_match* match, const gfo_match_command* command, uint8_t* out);
int gfo_match_step(gfo_match* match, const gfo_match_command* command, gfo_match_status* out);
// Privileged externally resolved ordinary main command for a confused owner.
// No automatic selector is provided. Uses the unchanged184-byte command.
int gfo_match_confusion_legal(const gfo_match* match, const gfo_match_command* command, uint8_t* out);
int gfo_match_confusion_step(gfo_match* match, const gfo_match_command* command, gfo_match_status* out);
// Externally selected guardian action; no autonomous scheduling or summoning.
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
// Policy RNG is a distinct caller-owned uint64 stream. This never advances the
// game's RNG/state. On failure neither RNG nor command is changed.
int gfo_match_sample_action(const gfo_match* match, uint64_t* policy_rng, gfo_match_command* out);
// Explicit spectator:has_viewer0/viewer0. Otherwise has_viewer1 and an actual ID.
// required includes NUL. NULL0 queries length. Short-4 changes no output bytes or
// required length, and observation does not mutate any match state.
int gfo_match_observe(const gfo_match* match, uint8_t has_viewer, uint64_t viewer,
                      char* buffer, size_t capacity, size_t* required);
#ifdef __cplusplus
}
#endif
