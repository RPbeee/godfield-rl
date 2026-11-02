#include "godfield/official_match_c_api.h"
#include "godfield/official_match.hpp"
#include "official_state_bridge.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace official = godfield::official;
struct gfo_match { official::Match value; explicit gfo_match(official::MatchConfig c) : value(c) {} };
static_assert(sizeof(gfo_match_config) == 24 && offsetof(gfo_match_config, initial_hp) == 8);
static_assert(sizeof(gfo_match_command) == 184 && offsetof(gfo_match_command, items) == 24
    && offsetof(gfo_match_command, hp) == 168 && offsetof(gfo_match_command, has_allocation) == 180);
static_assert(std::is_standard_layout_v<gfo_match_guardian_action>
    && sizeof(gfo_match_guardian_action) == 24
    && offsetof(gfo_match_guardian_action, model) == 16
    && offsetof(gfo_match_guardian_action, has_target) == 18);
static_assert(std::is_standard_layout_v<gfo_match_guardian_payload_action>
    && sizeof(gfo_match_guardian_payload_action) == 64
    && offsetof(gfo_match_guardian_payload_action, models) == 16
    && offsetof(gfo_match_guardian_payload_action, guardian) == 52
    && offsetof(gfo_match_guardian_payload_action, model_count) == 53
    && offsetof(gfo_match_guardian_payload_action, has_target) == 54
    && offsetof(gfo_match_guardian_payload_action, reserved) == 55);
static_assert(sizeof(gfo_match_status) == 248 && offsetof(gfo_match_status, rewards) == 96
    && offsetof(gfo_match_status, player_count) == 240);
static_assert(std::is_nothrow_move_assignable_v<official::Match>);
namespace {
template<std::size_t N> bool zero(const uint8_t (&bytes)[N]) { return std::all_of(std::begin(bytes), std::end(bytes), [](auto v) { return v == 0; }); }
void valid(bool condition) { if (!condition) throw std::invalid_argument("invalid match ABI input"); }
int error() noexcept {
    try { throw; }
    catch (const official::MatchError& e) { return e.code == official::MatchErrorCode::unsupported ? -5 : -2; }
    catch (const std::invalid_argument&) { return -2; }
    catch (...) { return -3; }
}
std::string text(const char* data, std::size_t size) {
    valid(size <= static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max()) && size <= std::string{}.max_size());
    valid(data || !size);
    if (!size) return {};
    for (std::size_t i = 0; i < size;) {
        const auto c = static_cast<unsigned char>(data[i++]);
        if (c < 0x80) continue;
        unsigned remaining = 0, code = 0, minimum = 0;
        if (c >= 0xc2 && c <= 0xdf) { remaining = 1; code = c & 31; minimum = 0x80; }
        else if (c >= 0xe0 && c <= 0xef) { remaining = 2; code = c & 15; minimum = 0x800; }
        else if (c >= 0xf0 && c <= 0xf4) { remaining = 3; code = c & 7; minimum = 0x10000; }
        else throw std::invalid_argument("invalid UTF8 match name");
        valid(remaining <= size - i);
        while (remaining--) { const auto v = static_cast<unsigned char>(data[i++]); valid((v & 0xc0) == 0x80); code = (code << 6) | (v & 63); }
        valid(code >= minimum && code <= 0x10ffff && !(code >= 0xd800 && code <= 0xdfff));
    }
    return {data, size};
}
official::MatchConfig config(const gfo_match_config* c) {
    if (!c) return {};
    valid(zero(c->reserved) && zero(c->reserved_tail));
    return {c->max_decisions, c->initial_hand, c->hand_limit, c->initial_hp, c->initial_mp, c->initial_cp};
}
std::vector<official::PlayerSetup> setup(const gfo_match_player* players, std::size_t count) {
    valid(count <= official::player_capacity);
    std::vector<official::PlayerSetup> result; result.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto& p = players[i]; valid(zero(p.reserved) && p.id <= official::max_wire_integer && p.team <= 4);
        result.push_back({p.id, static_cast<official::Team>(p.team), text(p.name, p.name_size)});
    }
    return result;
}
official::Command command(const gfo_match_command& c) {
    valid(zero(c.reserved) && zero(c.reserved_tail) && c.kind <= 8 && c.has_target <= 1 && c.has_allocation <= 1
        && (c.has_allocation || (!c.hp && !c.mp && !c.cp)) && c.item_count <= official::visible_hand_capacity
        && c.actor <= official::max_wire_integer && c.target <= official::max_wire_integer && (c.has_target || !c.target));
    official::Command result; result.actor = c.actor; result.kind = static_cast<official::CommandKind>(c.kind);
    if (c.has_target) result.target = c.target;
    if (c.has_allocation) result.allocation = official::ResourceAllocation{c.hp,c.mp,c.cp};
    for (std::size_t i = 0; i < 18; ++i) {
        if (i < c.item_count) { valid(c.items[i] <= official::max_wire_integer); result.items.push_back(c.items[i]); }
        else valid(c.items[i] == 0);
    }
    return result;
}
official::GuardianAction guardian_action(const gfo_match_guardian_action& input) {
    valid(zero(input.reserved) && input.has_target <= 1
        && input.source <= official::max_wire_integer && input.target <= official::max_wire_integer
        && (input.has_target || !input.target));
    return {input.source, input.model,
        input.has_target ? std::optional<official::PlayerId>{input.target} : std::nullopt};
}
official::GuardianPayloadAction guardian_payload_action(const gfo_match_guardian_payload_action& input) {
    valid(zero(input.reserved) && input.has_target <= 1 && input.model_count >= 1 && input.model_count <= 18
        && (input.guardian == static_cast<uint8_t>(official::Guardian::earth)
            || input.guardian == static_cast<uint8_t>(official::Guardian::moon))
        && input.source <= official::max_wire_integer && input.target <= official::max_wire_integer
        && (input.has_target || !input.target));
    official::GuardianPayloadAction result;
    result.source = input.source; result.guardian = static_cast<official::Guardian>(input.guardian);
    if (input.has_target) result.target = input.target;
    for (std::size_t i = 0; i < 18; ++i) {
        if (i < input.model_count) result.models.push_back(input.models[i]);
        else valid(input.models[i] == 0);
    }
    return result;
}
gfo_match_command encode(const official::Command& c) {
    valid(c.items.size() <= official::visible_hand_capacity && static_cast<unsigned>(c.kind) <= 8
        && (c.kind == official::CommandKind::exchange) == c.allocation.has_value()
        && c.actor <= official::max_wire_integer && (!c.target || *c.target <= official::max_wire_integer));
    if (c.allocation) valid(c.allocation->hp >= 0 && c.allocation->hp <= 99 && c.allocation->mp >= 0 && c.allocation->mp <= 99
        && c.allocation->cp >= 0 && c.allocation->cp <= 99);
    gfo_match_command out{}; out.actor = c.actor; out.kind = static_cast<uint8_t>(c.kind);
    out.has_target = c.target.has_value(); out.target = c.target.value_or(0); out.item_count = static_cast<uint8_t>(c.items.size());
    out.has_allocation = c.allocation.has_value();
    if (c.allocation) { out.hp = c.allocation->hp; out.mp = c.allocation->mp; out.cp = c.allocation->cp; }
    for (std::size_t i = 0; i < c.items.size(); ++i) { valid(c.items[i] <= official::max_wire_integer); out.items[i] = c.items[i]; }
    return out;
}
gfo_match_status status(const official::Match& match) {
    const auto& s = match.snapshot();
    if (s.state.players.size() > 9 || s.winners.size() > 9 || s.rewards.size() != s.state.players.size()) throw std::logic_error("invalid native match output sizes");
    gfo_match_status out{}; out.decisions = s.decisions; out.owner = s.owner;
    out.actor = s.actor.value_or(0); out.has_actor = s.actor.has_value(); out.player_count = static_cast<uint8_t>(s.state.players.size());
    out.winner_count = static_cast<uint8_t>(s.winners.size()); out.phase = static_cast<uint8_t>(s.phase);
    out.terminated = s.terminated; out.truncated = s.truncated;
    for (std::size_t i = 0; i < s.state.players.size(); ++i) {
        if (!std::isfinite(s.rewards[i])) throw std::logic_error("nonfinite match reward");
        out.player_ids[i] = s.state.players[i].id; out.rewards[i] = s.rewards[i];
    }
    for (std::size_t i = 0; i < s.winners.size(); ++i) out.winners[i] = s.winners[i];
    return out;
}
}
extern "C" {
int gfo_match_abi_version(void) { return 2; }
const char* gfo_match_ruleset(void) { return official::match_ruleset; }
const char* gfo_match_scope(void) { return official::match_scope; }
int gfo_match_catalog_count(void) { int n = 0; for (const auto& c : official::catalog()) n += official::match_supported(c.id); return n; }
int gfo_match_gift_weight(void) { return official::match_gift_weight(); }
int gfo_match_supported(uint16_t model) { return official::match_supported(model); }
size_t gfo_match_config_size(void) { return sizeof(gfo_match_config); }
size_t gfo_match_player_size(void) { return sizeof(gfo_match_player); }
size_t gfo_match_command_size(void) { return sizeof(gfo_match_command); }
size_t gfo_match_status_size(void) { return sizeof(gfo_match_status); }
size_t gfo_match_guardian_action_size(void) { return sizeof(gfo_match_guardian_action); }
int gfo_match_guardian_supported(uint16_t model) { return official::match_guardian_supported(model); }
size_t gfo_match_guardian_payload_action_size(void) { return sizeof(gfo_match_guardian_payload_action); }
int gfo_match_guardian_payload_supported(uint8_t guardian, uint16_t model) {
    if (guardian != static_cast<uint8_t>(official::Guardian::earth) && guardian != static_cast<uint8_t>(official::Guardian::moon)) return 0;
    return official::match_guardian_payload_supported(static_cast<official::Guardian>(guardian), model);
}
int gfo_match_create(const gfo_match_config* c, const gfo_match_player* players, size_t count, uint64_t seed, gfo_match** out) {
    if (!out || (!players && count)) return -1;
    try {
        const auto parsed = setup(players, count); auto owned = std::make_unique<gfo_match>(config(c));
        owned->value.reset(parsed, seed); *out = owned.release(); return 0;
    } catch (...) { return error(); }
}
void gfo_match_destroy(gfo_match* match) { delete match; }
int gfo_match_reset(gfo_match* match, const gfo_match_player* players, size_t count, uint64_t seed, gfo_match_status* out) {
    if (!match || !out || (!players && count)) return -1;
    try {
        const auto parsed = setup(players, count); auto next = match->value; next.reset(parsed, seed); const auto result = status(next);
        match->value = std::move(next); *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_load_scene(gfo_match* match, const gfo_state* state, uint64_t owner, uint64_t seed, gfo_match_status* out) {
    if (!match || !state || !out) return -1;
    try {
        valid(owner <= official::max_wire_integer);
        const auto& input = official::bridge::snapshot(*state);
        // The read-only state API intentionally permits larger foreign hands.
        // Reject them before copying into a bounded execution scene.
        valid(input.players.size() >= 2 && input.players.size() <= official::player_capacity);
        for (const auto& player : input.players)
            valid(player.items.size() <= match->value.snapshot().config.hand_limit);
        auto next = match->value;
        next.load_scene(input, owner, seed);
        const auto result = status(next);
        match->value = std::move(next); *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_get_status(const gfo_match* match, gfo_match_status* out) {
    if (!match || !out) return -1;
    try { const auto result = status(match->value); *out = result; return 0; } catch (...) { return error(); }
}
int gfo_match_legal(const gfo_match* match, const gfo_match_command* input, uint8_t* out) {
    if (!match || !input || !out) return -1;
    try { const auto parsed = command(*input); const uint8_t result = match->value.legal(parsed); *out = result; return 0; } catch (...) { return error(); }
}
int gfo_match_step(gfo_match* match, const gfo_match_command* input, gfo_match_status* out) {
    if (!match || !input || !out) return -1;
    try {
        const auto parsed = command(*input); auto next = match->value; next.step(parsed); const auto result = status(next);
        match->value = std::move(next); *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_guardian_legal(const gfo_match* match, const gfo_match_guardian_action* input, uint8_t* out) {
    if (!match || !input || !out) return -1;
    try {
        const auto parsed = guardian_action(*input);
        const uint8_t result = match->value.guardian_legal(parsed);
        *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_guardian_step(gfo_match* match, const gfo_match_guardian_action* input, gfo_match_status* out) {
    if (!match || !input || !out) return -1;
    try {
        const auto parsed = guardian_action(*input);
        auto next = match->value;
        next.guardian_step(parsed);
        const auto result = status(next);
        match->value = std::move(next); *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_guardian_payload_legal(const gfo_match* match, const gfo_match_guardian_payload_action* input, uint8_t* out) {
    if (!match || !input || !out) return -1;
    try {
        const auto parsed = guardian_payload_action(*input);
        const uint8_t result = match->value.guardian_payload_legal(parsed);
        *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_guardian_payload_step(gfo_match* match, const gfo_match_guardian_payload_action* input, gfo_match_status* out) {
    if (!match || !input || !out) return -1;
    try {
        const auto parsed = guardian_payload_action(*input); auto next = match->value;
        next.guardian_payload_step(parsed); const auto result = status(next);
        match->value = std::move(next); *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_sample_action(const gfo_match* match, uint64_t* policy_rng, gfo_match_command* out) {
    if (!match || !policy_rng || !out) return -1;
    try {
        valid(!match->value.done()); auto rng = *policy_rng; const auto result = encode(match->value.sample_action(rng));
        *policy_rng = rng; *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_observe(const gfo_match* match, uint8_t has_viewer, uint64_t viewer, char* buffer, size_t capacity, size_t* required) {
    if (!match || !required || (!buffer && capacity)) return -1;
    try {
        valid(has_viewer <= 1 && (has_viewer || !viewer) && viewer <= official::max_wire_integer);
        const auto result = match->value.observe(has_viewer ? std::optional<official::PlayerId>{viewer} : std::nullopt);
        const auto size = result.size() + 1;
        if (!buffer && !capacity) { *required = size; return 0; }
        if (capacity < size) return -4;
        std::memcpy(buffer, result.c_str(), size); *required = size; return 0;
    } catch (...) { return error(); }
}
}
