#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ABI1: current official catalog and public client calculations only.
// This is NOT a playable environment or a complete server implementation.
// Model IDs1..296 are official IDs, independent from every legacy profile.
// Enum spellings can be queried below; omitted element is -1, others0..5.
typedef struct gfo_card {
    uint16_t id;
    uint8_t category;
    int8_t element;
    uint8_t ability, curse, guardian, is_plus_atk;
    int32_t atk, hit_rate, defense, ability_value, price, cost;
    int32_t gift_rate, appearance_rate, guardian_attack_rate;
} gfo_card;

typedef struct gfo_composition {
    int64_t attack_power, magic_cost;
    int8_t attack_element, defense_element;
    uint8_t reserved[6];
} gfo_composition;

int gfo_abi_version(void);
const char* gfo_ruleset(void);
const char* gfo_scope(void);
const char* gfo_catalog_sha256(void);
const char* gfo_client_sha256(void);
int gfo_catalog_count(void);
int gfo_gift_rate(void);
size_t gfo_card_size(void);
size_t gfo_composition_size(void);
// Static immutable UTF-8 strings, valid for the library lifetime. Invalid value
// returns NULL. For element=-1 the spelling is the empty string.
const char* gfo_card_name(uint16_t id);
const char* gfo_card_image_name(uint16_t id);
const char* gfo_category_name(uint8_t value);
const char* gfo_element_name(int8_t value);
const char* gfo_ability_name(uint8_t value);
const char* gfo_curse_name(uint8_t value);
const char* gfo_guardian_name(uint8_t value);
// Status0 success/-1 NULL/-2 invalid input/-3 exception or integer overflow.
// On any error, caller-owned output bytes are unchanged. Inputs and outputs
// must point to valid caller storage. No exception crosses the C boundary.
int gfo_get_card(uint16_t id, gfo_card* out);
// Nonempty ordered IDs. Positive override replaces the base attack;0 uses its
// catalog ATK. Negative override is invalid. This computes tw/l9/tx/la and does
// not assert that the selection is a legal action or simulate MP/turn changes.
int gfo_compose(const uint16_t* ids, size_t count, int64_t base_override,
                gfo_composition* out);
#ifdef __cplusplus
}
#endif
