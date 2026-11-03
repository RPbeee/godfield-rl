#pragma once
#include <stddef.h>
#include <stdint.h>
#include "godfield/official_state_c_api.h"
#ifdef __cplusplus
extern "C" {
#endif

// Additive received-display API1, separate from State ABI1 and Match ABI2.
// Immutable UI projection only: no server transitions, RNG, Match resume or IO.
typedef struct gfo_display gfo_display;
typedef struct gfo_display_event {
    gfo_state_active_attack attack;
    uint64_t player_id;
    uint8_t kind, reserved[7]; //0 nextAttack,1 setTargetPlayer,2 advanceGF.
} gfo_display_event;

int gfo_display_api_version(void);
size_t gfo_display_event_size(void);
// Status0 success/-1 NULL/-2 invalid/-3 exception/-4 short output buffer.
// All storage must be valid/nonoverlapping and padding/inactive fields zero.
// Copies board and optional active head from a live same-library State handle.
// The resulting display owns its data; the input handle can be destroyed.
int gfo_display_create(const gfo_state* state, uint64_t turn_player, gfo_display** out);
// Whole-batch copy/validate/commit; original display and out are unchanged on
// every error. nextAttack requires player_id0; other branches require all72
// attack bytes zero. Model arrays are borrowed for this call and copied.
// NULL events/count0 is an immutable clone; NULL/nonzero is invalid storage.
int gfo_display_apply(const gfo_display* display, const gfo_display_event* events,
                       size_t count, gfo_display** out);
void gfo_display_destroy(gfo_display* display); // NULL allowed; live handles only.
// Separate official-active-display-view-v1 JSON. required includes terminatingNUL.
// NULL buffer/capacity0 queries length. Short(-4) changes no output bytes/length.
int gfo_display_view_json(const gfo_display* display, char* buffer,
                           size_t capacity, size_t* required);

#ifdef __cplusplus
}
#endif
