#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

// Additive ABI1 extension: immutable decoded snapshots and fresh UI views only.
// No transition engine, actor/timing validation, hidden outcome or game connection.
typedef struct gfo_state gfo_state;
typedef struct gfo_state_item {
    uint64_t token;              // Per-player unique;0 is a valid physical token.
    uint16_t model, fake_model;  // model1..296; fake_model0 means absent.
    uint8_t used, reserved[3];
} gfo_state_item;
typedef struct gfo_state_player {
    uint64_t id;
    const char* user_id; size_t user_id_size; // NULL/0 absent; nonNULL/0 present empty string.
    const char* name; size_t name_size;
    const char* alias; size_t alias_size;     // Same optional convention as user_id.
    int64_t hp, mp, cp, confusion_count;
    size_t item_offset, item_count, curse_offset, curse_count;
    uint8_t team, guardian, dead, reserved[5];
} gfo_state_player;
typedef struct gfo_state_meta {
    int64_t gf, tiebreak_gf;
    uint64_t update_count;
    uint8_t has_tiebreak, is_over, reserved[6];
} gfo_state_meta;
typedef struct gfo_state_response {
    uint8_t flash, incoming_category;
    int8_t incoming_element; // -1 absent;0..5 official Element.
    uint8_t incoming_is_attack, reserved[4];
} gfo_state_response;

int gfo_state_abi_version(void);
size_t gfo_state_item_size(void);
size_t gfo_state_player_size(void);
size_t gfo_state_meta_size(void);
size_t gfo_state_response_size(void);
// Status0 success/-1 NULL/-2 invalid/-3 exception/-4 short output buffer.
// Inputs must be valid caller storage, UTF-8 strings and canonical zero padding.
// Every error leaves ALL outputs unchanged. Item/curse slices must partition
// their flat arrays contiguously in player order; no ignored trailing records.
// Creates an owned deep copy only after complete validation. At most9 players.
int gfo_state_create(const gfo_state_meta* meta,
                     const gfo_state_player* players, size_t player_count,
                     const gfo_state_item* items, size_t item_count,
                     const uint8_t* curses, size_t curse_count, gfo_state** out);
void gfo_state_destroy(gfo_state* state); // NULL allowed; live handles only.
// found0/player_id0 means no match. Duplicate user IDs follow the client first match.
int gfo_state_resolve_viewer(const gfo_state* state, const char* user_id, size_t size,
                            uint8_t* found, uint64_t* player_id);
// has_viewer0/viewer0 is explicit spectator; has_viewer1 accepts arbitrary IDs.
// required includes the terminatingNUL. NULL buffer/capacity0 queries size.
// A short buffer returns-4 and changes neither buffer nor required.
int gfo_state_view_json(const gfo_state* state, uint8_t has_viewer, uint64_t viewer,
                       char* buffer, size_t capacity, size_t* required);
// Selection queries use only the viewer's visible own models. Optional response
// is caller-supplied public context, NOT inferred actor/target/turn legality.
// response.flash is ignored: the snapshot viewer's actual flash curse controls
// the predicate. Selection counts over18 are invalid, before token dereference.
int gfo_state_can_append(const gfo_state* state, uint64_t viewer,
                         const uint64_t* selected, size_t count, uint64_t next,
                         const gfo_state_response* response, uint8_t* out);
int gfo_state_selection_ready(const gfo_state* state, uint64_t viewer,
                              const uint64_t* selected, size_t count,
                              const gfo_state_response* response, uint8_t* out);
#ifdef __cplusplus
}
#endif
