#pragma once
#include <stddef.h>
#include <stdint.h>
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
    uint8_t kind, has_target, item_count, reserved[5]; // play0/defend1/discard2/pray3/exchange4/sell5
    uint64_t items[18]; // Own physical tokens, unused tail must be0. Token0 valid.
    int32_t hp, mp, cp; // Allocation for exchange only; otherwise all0.
    uint8_t has_allocation, reserved_tail[3]; // Required for exchange, forbidden otherwise.
} gfo_match_command;
typedef struct gfo_match_status {
    uint64_t decisions, owner, actor;
    uint64_t player_ids[9]; // Stable player-vector order for rewards.
    double rewards[9];
    uint64_t winners[9];
    uint8_t player_count, winner_count, phase, has_actor, terminated, truncated, reserved[2];
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
// Status0 success/-1 NULL/-2 invalid/-3 exception/-4 short buffer/-5 unsupported.
// Every error leaves the complete mutable match, policy RNG and all outputs
// unchanged. Storage must be valid, nonoverlapping caller objects. Zero padding.
// NULL config uses defaults. Player setup is copied; IDs need not equal seats.
int gfo_match_create(const gfo_match_config* config, const gfo_match_player* players,
                     size_t count, uint64_t seed, gfo_match** out);
void gfo_match_destroy(gfo_match* match);
int gfo_match_reset(gfo_match* match, const gfo_match_player* players,
                    size_t count, uint64_t seed, gfo_match_status* out);
int gfo_match_get_status(const gfo_match* match, gfo_match_status* out);
int gfo_match_legal(const gfo_match* match, const gfo_match_command* command, uint8_t* out);
int gfo_match_step(gfo_match* match, const gfo_match_command* command, gfo_match_status* out);
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
