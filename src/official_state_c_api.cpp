#include "godfield/official_state_c_api.h"
#include "godfield/official_state.hpp"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

namespace official = godfield::official;
struct gfo_state { official::SnapshotState value; };
static_assert(std::is_standard_layout_v<gfo_state_item> && sizeof(gfo_state_item) == 16);
static_assert(offsetof(gfo_state_item, model) == 8 && offsetof(gfo_state_item, used) == 12);
static_assert(std::is_standard_layout_v<gfo_state_meta> && sizeof(gfo_state_meta) == 32);
static_assert(offsetof(gfo_state_meta, has_tiebreak) == 24);
static_assert(std::is_standard_layout_v<gfo_state_response> && sizeof(gfo_state_response) == 8);
static_assert(offsetof(gfo_state_response, incoming_element) == 2);

namespace {
template<std::size_t N> bool zero(const uint8_t (&bytes)[N]) { return std::all_of(std::begin(bytes), std::end(bytes), [](auto v) { return v == 0; }); }
void valid(bool condition) { if (!condition) throw std::invalid_argument("invalid state bridge input"); }
std::string text(const char* data, std::size_t size) {
    valid(size <= static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max())
        && size <= std::string{}.max_size());
    valid(data || !size);
    if (!size) return {};
    // JSON output is UTF-8. Reject overlong, surrogate and out-of-range encodings.
    for (std::size_t i = 0; i < size;) {
        const auto c = static_cast<unsigned char>(data[i++]);
        if (c < 0x80) continue;
        unsigned remaining = 0, code = 0, minimum = 0;
        if (c >= 0xc2 && c <= 0xdf) { remaining = 1; code = c & 31; minimum = 0x80; }
        else if (c >= 0xe0 && c <= 0xef) { remaining = 2; code = c & 15; minimum = 0x800; }
        else if (c >= 0xf0 && c <= 0xf4) { remaining = 3; code = c & 7; minimum = 0x10000; }
        else throw std::invalid_argument("invalid UTF-8");
        valid(remaining <= size - i);
        while (remaining--) { auto v = static_cast<unsigned char>(data[i++]); valid((v & 0xc0) == 0x80); code = (code << 6) | (v & 63); }
        valid(code >= minimum && code <= 0x10ffff && !(code >= 0xd800 && code <= 0xdfff));
    }
    return {data, size};
}
std::optional<std::string> optional_text(const char* data, std::size_t size) { valid(data || !size); return data ? std::optional{text(data, size)} : std::nullopt; }
std::optional<official::ResponseContext> context(const gfo_state_response* r) {
    if (!r) return std::nullopt;
    valid(zero(r->reserved) && r->flash <= 1 && r->incoming_is_attack <= 1 && r->incoming_category <= 7
        && r->incoming_element >= -1 && r->incoming_element <= 5);
    official::Attribute element;
    if (r->incoming_element >= 0) element = static_cast<official::Element>(r->incoming_element);
    return official::ResponseContext{r->flash != 0, static_cast<official::Category>(r->incoming_category), element, r->incoming_is_attack != 0};
}
std::span<const official::ItemToken> selected(const uint64_t* tokens, std::size_t count) {
    valid(count <= official::visible_hand_capacity);
    for (std::size_t i = 0; i < count; ++i) valid(tokens[i] <= official::max_wire_integer);
    return {tokens, count};
}
}
extern "C" {
int gfo_state_abi_version(void) { return 1; }
size_t gfo_state_item_size(void) { return sizeof(gfo_state_item); }
size_t gfo_state_player_size(void) { return sizeof(gfo_state_player); }
size_t gfo_state_meta_size(void) { return sizeof(gfo_state_meta); }
size_t gfo_state_response_size(void) { return sizeof(gfo_state_response); }
int gfo_state_create(const gfo_state_meta* meta, const gfo_state_player* players, size_t player_count,
                     const gfo_state_item* items, size_t item_count, const uint8_t* curses, size_t curse_count, gfo_state** out) {
    if (!meta || !out || (!players && player_count) || (!items && item_count) || (!curses && curse_count)) return -1;
    try {
        valid(player_count <= official::player_capacity && zero(meta->reserved) && meta->has_tiebreak <= 1
            && meta->is_over <= 1 && (meta->has_tiebreak || !meta->tiebreak_gf));
        official::SnapshotState result;
        result.gf = meta->gf; if (meta->has_tiebreak) result.tiebreak_gf = meta->tiebreak_gf;
        result.update_count = meta->update_count; result.is_over = meta->is_over != 0;
        result.players.reserve(player_count);
        std::size_t next_item = 0, next_curse = 0;
        for (std::size_t n = 0; n < player_count; ++n) {
            const auto& p = players[n];
            valid(zero(p.reserved) && p.team <= 4 && p.guardian <= 10 && p.dead <= 1
                && p.item_offset == next_item && p.item_count <= item_count - next_item
                && p.curse_offset == next_curse && p.curse_count <= curse_count - next_curse);
            official::PlayerState value;
            value.id = p.id; value.user_id = optional_text(p.user_id, p.user_id_size);
            value.name = text(p.name, p.name_size); value.alias = optional_text(p.alias, p.alias_size);
            value.team = static_cast<official::Team>(p.team); value.hp = p.hp; value.mp = p.mp; value.cp = p.cp;
            value.confusion_count = p.confusion_count; value.guardian = static_cast<official::Guardian>(p.guardian); value.dead = p.dead != 0;
            valid(p.item_count <= value.items.max_size() && p.curse_count <= value.curses.max_size());
            value.items.reserve(p.item_count); value.curses.reserve(p.curse_count);
            for (std::size_t i = 0; i < p.item_count; ++i) {
                const auto& item = items[next_item++]; valid(zero(item.reserved) && item.used <= 1);
                value.items.push_back({item.token, item.model, item.fake_model ? std::optional<official::CardId>{item.fake_model} : std::nullopt, item.used != 0});
            }
            for (std::size_t i = 0; i < p.curse_count; ++i) {
                const auto curse = curses[next_curse++]; valid(curse >= 1 && curse <= 8);
                value.curses.push_back(static_cast<official::Curse>(curse));
            }
            result.players.push_back(std::move(value));
        }
        valid(next_item == item_count && next_curse == curse_count);
        official::validate_state(result);
        auto owned = std::make_unique<gfo_state>(); owned->value = std::move(result);
        *out = owned.release(); return 0;
    } catch (const std::invalid_argument&) { return -2; } catch (...) { return -3; }
}
void gfo_state_destroy(gfo_state* state) { delete state; }
int gfo_state_resolve_viewer(const gfo_state* state, const char* user_id, size_t size, uint8_t* found, uint64_t* player_id) {
    if (!state || !found || !player_id || (!user_id && size)) return -1;
    try {
        const auto result = official::resolve_viewer(state->value, text(user_id, size));
        const uint8_t present = result.has_value(); const uint64_t id = result.value_or(0);
        *found = present; *player_id = id; return 0;
    } catch (const std::invalid_argument&) { return -2; } catch (...) { return -3; }
}
int gfo_state_view_json(const gfo_state* state, uint8_t has_viewer, uint64_t viewer, char* buffer, size_t capacity, size_t* required) {
    if (!state || !required || (!buffer && capacity)) return -1;
    try {
        valid(has_viewer <= 1 && (has_viewer || !viewer) && viewer <= official::max_wire_integer);
        const auto result = official::view_json(official::project_view(state->value, has_viewer ? std::optional<official::PlayerId>{viewer} : std::nullopt));
        const auto size = result.size() + 1;
        if (!buffer && !capacity) { *required = size; return 0; }
        if (capacity < size) return -4;
        std::memcpy(buffer, result.c_str(), size); *required = size; return 0;
    } catch (const std::invalid_argument&) { return -2; } catch (...) { return -3; }
}
int gfo_state_can_append(const gfo_state* state, uint64_t viewer, const uint64_t* tokens, size_t count, uint64_t next,
                         const gfo_state_response* response, uint8_t* out) {
    if (!state || !out || (!tokens && count)) return -1;
    try {
        valid(viewer <= official::max_wire_integer && next <= official::max_wire_integer);
        const auto ctx = context(response); const auto ordered = selected(tokens, count);
        const uint8_t result = official::can_append_hand(state->value, viewer, ordered, next, ctx);
        *out = result; return 0;
    } catch (const std::invalid_argument&) { return -2; } catch (...) { return -3; }
}
int gfo_state_selection_ready(const gfo_state* state, uint64_t viewer, const uint64_t* tokens, size_t count,
                              const gfo_state_response* response, uint8_t* out) {
    if (!state || !out || (!tokens && count)) return -1;
    try {
        valid(viewer <= official::max_wire_integer);
        const auto ctx = context(response); const auto ordered = selected(tokens, count);
        const uint8_t result = official::selection_ready(state->value, viewer, ordered, ctx);
        *out = result; return 0;
    } catch (const std::invalid_argument&) { return -2; } catch (...) { return -3; }
}
}
