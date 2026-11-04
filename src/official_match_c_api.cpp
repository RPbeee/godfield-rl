#include "godfield/official_match_c_api.h"
#include "godfield/official_match.hpp"
#include "official_state_bridge.hpp"
#include <algorithm>
#include <array>
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
static_assert(sizeof(gfo_match_config) == 24 && offsetof(gfo_match_config, initial_hp) == 8
    && offsetof(gfo_match_config,external_grants)==6 && offsetof(gfo_match_config,include_fate)==7
    && offsetof(gfo_match_config,local_guardian_payloads)==20 && offsetof(gfo_match_config,external_artifacts)==21
    && offsetof(gfo_match_config,reserved_tail)==22);
static_assert(sizeof(gfo_match_command) == 184 && offsetof(gfo_match_command, items) == 24
    && offsetof(gfo_match_command, hp) == 168 && offsetof(gfo_match_command, has_allocation) == 180);
static_assert(std::is_standard_layout_v<gfo_match_choice> && sizeof(gfo_match_choice)==208
    && offsetof(gfo_match_choice,command)==0 && offsetof(gfo_match_choice,fate_actor)==184
    && offsetof(gfo_match_choice,fate_token)==192 && offsetof(gfo_match_choice,kind)==200
    && offsetof(gfo_match_choice,reserved)==201);
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
static_assert(std::is_standard_layout_v<gfo_match_phenomenon_action>
    && sizeof(gfo_match_phenomenon_action) == 24
    && offsetof(gfo_match_phenomenon_action, model) == 8
    && offsetof(gfo_match_phenomenon_action, has_recipient) == 10
    && offsetof(gfo_match_phenomenon_action, guardian_count) == 11
    && offsetof(gfo_match_phenomenon_action, guardians) == 12
    && offsetof(gfo_match_phenomenon_action, reserved) == 21);
static_assert(std::is_standard_layout_v<gfo_match_item_source> && sizeof(gfo_match_item_source) == 16
    && offsetof(gfo_match_item_source, token) == 8);
static_assert(std::is_standard_layout_v<gfo_match_item_shuffle_action> && sizeof(gfo_match_item_shuffle_action) == 16
    && offsetof(gfo_match_item_shuffle_action, count) == 8);
static_assert(std::is_standard_layout_v<gfo_match_fate_action> && sizeof(gfo_match_fate_action) == 64
    && offsetof(gfo_match_fate_action, token) == 8 && offsetof(gfo_match_fate_action, outcome) == 16
    && offsetof(gfo_match_fate_action, shuffle) == 40 && offsetof(gfo_match_fate_action, has_shuffle) == 56
    && offsetof(gfo_match_fate_action, reserved) == 57);
static_assert(sizeof(gfo_match_status) == 248 && offsetof(gfo_match_status, rewards) == 96
    && offsetof(gfo_match_status, player_count) == 240);
static_assert(std::is_standard_layout_v<gfo_match_devil_action> && sizeof(gfo_match_devil_action)==32
    && offsetof(gfo_match_devil_action,removed)==8 && offsetof(gfo_match_devil_action,model)==24
    && offsetof(gfo_match_devil_action,resource)==26 && offsetof(gfo_match_devil_action,has_resource)==27
    && offsetof(gfo_match_devil_action,removed_count)==28 && offsetof(gfo_match_devil_action,reserved)==29);
static_assert(std::is_standard_layout_v<gfo_match_artifact_grant_action>
    && sizeof(gfo_match_artifact_grant_action)==16
    && offsetof(gfo_match_artifact_grant_action,model)==8
    && offsetof(gfo_match_artifact_grant_action,fake_model)==10
    && offsetof(gfo_match_artifact_grant_action,reserved)==12);
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
    valid(c->external_grants<=1 && c->include_fate<=1 && c->local_guardian_payloads<=1 && c->external_artifacts<=1 && zero(c->reserved_tail));
    return {c->max_decisions, c->initial_hand, c->hand_limit, c->initial_hp, c->initial_mp, c->initial_cp, c->external_grants!=0,c->include_fate!=0,c->local_guardian_payloads!=0,c->external_artifacts!=0};
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
    valid(zero(c.reserved) && zero(c.reserved_tail) && c.kind <= 9 && c.has_target <= 1 && c.has_allocation <= 1
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
official::PlayerChoice choice(const gfo_match_choice& input) {
    valid(input.kind<=1 && zero(input.reserved));
    if (!input.kind) { valid(!input.fate_actor && !input.fate_token); return command(input.command); }
    const gfo_match_command inactive{};
    valid(std::memcmp(&input.command,&inactive,sizeof inactive)==0
        && input.fate_actor<=official::max_wire_integer && input.fate_token<=official::max_wire_integer);
    return official::FateRequest{input.fate_actor,input.fate_token};
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
official::PhenomenonAction phenomenon_action(const gfo_match_phenomenon_action& input) {
    valid(zero(input.reserved) && input.has_recipient <= 1 && input.guardian_count <= official::player_capacity
        && input.recipient <= official::max_wire_integer && (input.has_recipient || !input.recipient));
    official::PhenomenonAction result; result.model = input.model;
    if (input.has_recipient) result.recipient = input.recipient;
    for (std::size_t i = 0; i < official::player_capacity; ++i) {
        if (i < input.guardian_count) {
            valid(input.guardians[i] >= 1 && input.guardians[i] <= 10);
            result.guardians.push_back(static_cast<official::Guardian>(input.guardians[i]));
        } else valid(input.guardians[i] == 0);
    }
    return result;
}
official::ItemShuffleAction item_shuffle_action(const gfo_match_item_shuffle_action& input) {
    official::ItemShuffleAction result; result.order.reserve(input.count);
    for (std::size_t i = 0; i < input.count; ++i) {
        const auto& source = input.order[i];
        valid(source.owner <= official::max_wire_integer && source.token <= official::max_wire_integer);
        result.order.push_back({source.owner,source.token});
    }
    return result;
}
official::FateAction fate_action(const gfo_match_fate_action& input) {
    valid(zero(input.reserved) && input.has_shuffle <= 1 && input.actor <= official::max_wire_integer && input.token <= official::max_wire_integer);
    valid((input.has_shuffle == 1) == (input.outcome.model == 295));
    valid(input.has_shuffle || (!input.shuffle.order && !input.shuffle.count));
    official::FateAction result; result.actor=input.actor; result.token=input.token; result.outcome=phenomenon_action(input.outcome);
    if(input.has_shuffle) result.shuffle=item_shuffle_action(input.shuffle);
    return result;
}
official::DevilAction devil_action(const gfo_match_devil_action& input) {
    valid(zero(input.reserved) && input.target<=official::max_wire_integer && input.has_resource<=1 && input.removed_count<=2);
    valid((input.has_resource==1)==(input.model==244) && (input.has_resource ? input.resource<=2 : input.resource==0));
    valid(input.model==243 || input.removed_count==0);
    official::DevilAction result; result.target=input.target; result.model=input.model;
    if(input.has_resource) result.resource=static_cast<official::Resource>(input.resource);
    for(std::size_t i=0;i<2;++i) {
        valid(input.removed[i]<=official::max_wire_integer && (i<input.removed_count || input.removed[i]==0));
        if(i<input.removed_count) result.removed.push_back(input.removed[i]);
    }
    return result;
}
official::ArtifactGrantAction artifact_grant_action(const gfo_match_artifact_grant_action& input) {
    valid(zero(input.reserved) && input.target<=official::max_wire_integer);
    official::ArtifactGrantAction result;result.target=input.target;result.model=input.model;
    if(input.fake_model)result.fake_model=input.fake_model;
    return result;
}
gfo_match_command encode(const official::Command& c) {
    valid(c.items.size() <= official::visible_hand_capacity && static_cast<unsigned>(c.kind) <= 9
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
gfo_match_grant_action encode_grant(const official::GrantAction& action) {
    valid(action.target<=official::max_wire_integer && (action.model==0 || official::match_devil_supported(action.model))
        && action.removed.size()<=2 && (action.model==244)==action.resource.has_value()
        && (action.model==243 || action.removed.empty()));
    gfo_match_grant_action out{};out.target=action.target;out.model=action.model;
    out.has_resource=action.resource.has_value();
    if(action.resource){valid(static_cast<unsigned>(*action.resource)<=2);out.resource=static_cast<uint8_t>(*action.resource);}
    out.removed_count=static_cast<uint8_t>(action.removed.size());
    for(std::size_t i=0;i<action.removed.size();++i){
        valid(action.removed[i]<=official::max_wire_integer && (!i || action.removed[i]!=action.removed[0]));out.removed[i]=action.removed[i];
    }
    return out;
}
gfo_match_fate_action encode_fate(const official::FateAction& action,std::array<gfo_match_item_source,162>& order) {
    valid(action.actor<=official::max_wire_integer && action.token<=official::max_wire_integer
        && action.outcome.model>=287 && action.outcome.model<=296
        && action.shuffle.has_value()==(action.outcome.model==295));
    const auto& outcome=action.outcome;const bool targeted=outcome.model>=291&&outcome.model<=294;
    valid(outcome.recipient.has_value()==targeted && (outcome.model==296 || outcome.guardians.empty())
        && outcome.guardians.size()<=official::player_capacity);
    gfo_match_fate_action out{};out.actor=action.actor;out.token=action.token;out.outcome.model=outcome.model;
    out.outcome.has_recipient=outcome.recipient.has_value();
    if(outcome.recipient){valid(*outcome.recipient<=official::max_wire_integer);out.outcome.recipient=*outcome.recipient;}
    out.outcome.guardian_count=static_cast<uint8_t>(outcome.guardians.size());
    for(std::size_t i=0;i<outcome.guardians.size();++i){
        const auto guardian=static_cast<unsigned>(outcome.guardians[i]);valid(guardian>=1&&guardian<=10);out.outcome.guardians[i]=static_cast<uint8_t>(guardian);
    }
    out.has_shuffle=action.shuffle.has_value();
    if(action.shuffle){
        valid(action.shuffle->order.size()<=order.size());out.shuffle.count=action.shuffle->order.size();
        for(std::size_t i=0;i<out.shuffle.count;++i){const auto& source=action.shuffle->order[i];
            valid(source.owner<=official::max_wire_integer&&source.token<=official::max_wire_integer);
            for(std::size_t j=0;j<i;++j)valid(order[j].owner!=source.owner||order[j].token!=source.token);
            order[i]={source.owner,source.token};
        }
    }
    return out;
}
gfo_match_guardian_payload_action encode_guardian_payload(const official::GuardianPayloadAction& action) {
    valid(action.source<=official::max_wire_integer && action.target && *action.target<=official::max_wire_integer
        && (action.guardian==official::Guardian::earth || action.guardian==official::Guardian::moon)
        && !action.models.empty() && action.models.size()<=official::visible_hand_capacity
        && official::match_guardian_payload_supported(action.guardian,action.models.front()));
    gfo_match_guardian_payload_action out{};out.source=action.source;out.target=*action.target;out.has_target=1;
    out.guardian=static_cast<uint8_t>(action.guardian);out.model_count=static_cast<uint8_t>(action.models.size());
    for(std::size_t i=0;i<action.models.size();++i){valid(action.models[i]>=1 && action.models[i]<=official::catalog_count);out.models[i]=action.models[i];}
    return out;
}
gfo_match_choice encode_choice(const official::PlayerChoice& selected) {
    gfo_match_choice out{};
    if (const auto* c=std::get_if<official::Command>(&selected)) out.command=encode(*c);
    else {
        const auto& request=std::get<official::FateRequest>(selected);
        valid(request.actor<=official::max_wire_integer && request.token<=official::max_wire_integer);
        out.kind=1;out.fate_actor=request.actor;out.fate_token=request.token;
    }
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
int gfo_match_driver_api_version(void) { return 1; }
const char* gfo_match_choice_rule(void) { return official::choice_rule; }
const char* gfo_match_driver_rule(void) { return official::driver_rule; }
const char* gfo_match_ruleset(void) { return official::match_ruleset; }
const char* gfo_match_scope(void) { return official::match_scope; }
const char* gfo_match_self_attack_rule(void) { return official::self_attack_rule; }
int gfo_match_catalog_count(void) { int n = 0; for (const auto& c : official::catalog()) n += official::match_supported(c.id); return n; }
int gfo_match_gift_weight(void) { return official::match_gift_weight(); }
int gfo_match_supported(uint16_t model) { return official::match_supported(model); }
int gfo_match_gift_supported(const gfo_match* match,uint16_t model,uint8_t* out) {
    if(!match || !out)return -1;
    try {const uint8_t result=match->value.gift_supported(model);*out=result;return 0;}
    catch(...){return error();}
}
int gfo_match_get_gift_weight(const gfo_match* match,uint16_t* out) {
    if(!match || !out)return -1;
    try {const auto result=match->value.gift_weight();valid(result>=0 && result<=std::numeric_limits<uint16_t>::max());*out=static_cast<uint16_t>(result);return 0;}
    catch(...){return error();}
}
size_t gfo_match_config_size(void) { return sizeof(gfo_match_config); }
size_t gfo_match_player_size(void) { return sizeof(gfo_match_player); }
size_t gfo_match_command_size(void) { return sizeof(gfo_match_command); }
size_t gfo_match_choice_size(void) { return sizeof(gfo_match_choice); }
size_t gfo_match_status_size(void) { return sizeof(gfo_match_status); }
size_t gfo_match_guardian_action_size(void) { return sizeof(gfo_match_guardian_action); }
int gfo_match_guardian_supported(uint16_t model) { return official::match_guardian_supported(model); }
size_t gfo_match_guardian_payload_action_size(void) { return sizeof(gfo_match_guardian_payload_action); }
int gfo_match_guardian_payload_supported(uint8_t guardian, uint16_t model) {
    if (guardian != static_cast<uint8_t>(official::Guardian::earth) && guardian != static_cast<uint8_t>(official::Guardian::moon)) return 0;
    return official::match_guardian_payload_supported(static_cast<official::Guardian>(guardian), model);
}
size_t gfo_match_phenomenon_action_size(void) { return sizeof(gfo_match_phenomenon_action); }
int gfo_match_phenomenon_supported(uint16_t model) { return official::match_phenomenon_supported(model); }
size_t gfo_match_item_source_size(void) { return sizeof(gfo_match_item_source); }
size_t gfo_match_item_shuffle_action_size(void) { return sizeof(gfo_match_item_shuffle_action); }
int gfo_match_item_shuffle_supported(uint16_t model) { return official::match_item_shuffle_supported(model); }
size_t gfo_match_fate_action_size(void) { return sizeof(gfo_match_fate_action); }
int gfo_match_fate_supported(uint16_t model) { return official::match_fate_supported(model); }
size_t gfo_match_devil_action_size(void) { return sizeof(gfo_match_devil_action); }
int gfo_match_devil_supported(uint16_t model) { return official::match_devil_supported(model); }
size_t gfo_match_grant_action_size(void) { return sizeof(gfo_match_grant_action); }
int gfo_match_grant_supported(uint16_t model) { return official::match_grant_supported(model); }
size_t gfo_match_artifact_grant_action_size(void) { return sizeof(gfo_match_artifact_grant_action); }
int gfo_match_artifact_grant_supported(uint16_t model) { return official::match_artifact_grant_supported(model); }
const char* gfo_match_artifact_grant_rule(void) { return official::artifact_grant_rule; }
const char* gfo_match_normal_grant_rule(void) { return official::normal_grant_rule; }

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
        if (official::bridge::has_active_attack(*state)) throw official::MatchError{official::MatchErrorCode::unsupported,"pending server attacks cannot initialize a quiescent scene"};
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
int gfo_match_choice_legal(const gfo_match* match,const gfo_match_choice* input,uint8_t* out) {
    if (!match || !input || !out) return -1;
    try { const auto parsed=choice(*input);const uint8_t result=match->value.choice_legal(parsed);*out=result;return 0; }
    catch(...) { return error(); }
}
int gfo_match_sample_choice(const gfo_match* match,uint64_t* policy_rng,gfo_match_choice* out) {
    if (!match || !policy_rng || !out) return -1;
    try { auto trial=*policy_rng;const auto result=encode_choice(match->value.sample_choice(trial));
        *out=result;*policy_rng=trial;return 0; }
    catch(...) { return error(); }
}
int gfo_match_choice_step(gfo_match* match,const gfo_match_choice* input,uint64_t* chance_rng,gfo_match_status* out) {
    if (!match || !input || !chance_rng || !out) return -1;
    try { const auto parsed=choice(*input);auto next=match->value;auto trial=*chance_rng;
        next.choice_step(parsed,trial);const auto result=status(next);
        match->value=std::move(next);*out=result;*chance_rng=trial;return 0; }
    catch(...) { return error(); }
}
int gfo_match_rollout_step(gfo_match* match,uint64_t* policy_rng,uint64_t* chance_rng,gfo_match_status* out) {
    if (!match || !policy_rng || !chance_rng || !out) return -1;
    try { valid(policy_rng!=chance_rng);auto next=match->value;auto policy=*policy_rng;auto chance=*chance_rng;
        next.rollout_step(policy,chance);const auto result=status(next);
        match->value=std::move(next);*out=result;*policy_rng=policy;*chance_rng=chance;return 0; }
    catch(...) { return error(); }
}
int gfo_match_confusion_legal(const gfo_match* match, const gfo_match_command* input, uint8_t* out) {
    if (!match || !input || !out) return -1;
    try {
        const auto parsed = command(*input);
        const uint8_t result = match->value.confusion_legal(parsed);
        *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_confusion_step(gfo_match* match, const gfo_match_command* input, gfo_match_status* out) {
    if (!match || !input || !out) return -1;
    try {
        const auto parsed = command(*input); auto next = match->value;
        next.confusion_step(parsed); const auto result = status(next);
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
int gfo_match_phenomenon_legal(const gfo_match* match, const gfo_match_phenomenon_action* input, uint8_t* out) {
    if (!match || !input || !out) return -1;
    try {
        const auto parsed = phenomenon_action(*input);
        const uint8_t result = match->value.phenomenon_legal(parsed);
        *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_phenomenon_step(gfo_match* match, const gfo_match_phenomenon_action* input, gfo_match_status* out) {
    if (!match || !input || !out) return -1;
    try {
        const auto parsed = phenomenon_action(*input); auto next = match->value;
        next.phenomenon_step(parsed); const auto result = status(next);
        match->value = std::move(next); *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_item_shuffle_legal(const gfo_match* match, const gfo_match_item_shuffle_action* input, uint8_t* out) {
    if (!match || !input || !out) return -1;
    if (input->count > official::player_capacity * official::visible_hand_capacity) return -2;
    if (!input->order && input->count) return -1;
    try {
        const auto parsed = item_shuffle_action(*input);
        const uint8_t result = match->value.item_shuffle_legal(parsed);
        *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_item_shuffle_step(gfo_match* match, const gfo_match_item_shuffle_action* input, gfo_match_status* out) {
    if (!match || !input || !out) return -1;
    if (input->count > official::player_capacity * official::visible_hand_capacity) return -2;
    if (!input->order && input->count) return -1;
    try {
        const auto parsed = item_shuffle_action(*input); auto next = match->value;
        next.item_shuffle_step(parsed); const auto result = status(next);
        match->value = std::move(next); *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_fate_legal(const gfo_match* match, const gfo_match_fate_action* input, uint8_t* out) {
    if (!match || !input || !out) return -1;
    if(input->shuffle.count>official::player_capacity*official::visible_hand_capacity) return -2;
    if(input->has_shuffle>1 || (input->has_shuffle==1)!=(input->outcome.model==295)) return -2;
    if(input->has_shuffle==1 && !input->shuffle.order && input->shuffle.count) return -1;
    try { const auto parsed=fate_action(*input); const uint8_t result=match->value.fate_legal(parsed); *out=result; return 0; }
    catch (...) { return error(); }
}
int gfo_match_fate_step(gfo_match* match, const gfo_match_fate_action* input, gfo_match_status* out) {
    if (!match || !input || !out) return -1;
    if(input->shuffle.count>official::player_capacity*official::visible_hand_capacity) return -2;
    if(input->has_shuffle>1 || (input->has_shuffle==1)!=(input->outcome.model==295)) return -2;
    if(input->has_shuffle==1 && !input->shuffle.order && input->shuffle.count) return -1;
    try {
        const auto parsed=fate_action(*input); auto next=match->value; next.fate_step(parsed); const auto result=status(next);
        match->value=std::move(next); *out=result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_devil_legal(const gfo_match* match, const gfo_match_devil_action* input, uint8_t* out) {
    if(!match || !input || !out) return -1;
    try { const auto parsed=devil_action(*input); const uint8_t result=match->value.devil_legal(parsed); *out=result; return 0; }
    catch(...) { return error(); }
}
int gfo_match_devil_step(gfo_match* match, const gfo_match_devil_action* input, gfo_match_status* out) {
    if(!match || !input || !out) return -1;
    try { const auto parsed=devil_action(*input); auto next=match->value; next.devil_step(parsed); const auto result=status(next);
        match->value=std::move(next); *out=result; return 0; }
    catch(...) { return error(); }
}
int gfo_match_grant_legal(const gfo_match* match, const gfo_match_grant_action* input, uint8_t* out) {
    if(!match || !input || !out) return -1;
    try { const auto parsed=devil_action(*input); const uint8_t result=match->value.grant_legal(parsed); *out=result; return 0; }
    catch(...) { return error(); }
}
int gfo_match_grant_step(gfo_match* match, const gfo_match_grant_action* input, gfo_match_status* out) {
    if(!match || !input || !out) return -1;
    try { const auto parsed=devil_action(*input); auto next=match->value; next.grant_step(parsed); const auto result=status(next);
        match->value=std::move(next); *out=result; return 0; }
    catch(...) { return error(); }
}
int gfo_match_artifact_grant_legal(const gfo_match* match, const gfo_match_artifact_grant_action* input, uint8_t* out) {
    if(!match || !input || !out)return -1;
    try { const auto parsed=artifact_grant_action(*input);const uint8_t result=match->value.artifact_grant_legal(parsed);*out=result;return 0; }
    catch(...) { return error(); }
}
int gfo_match_artifact_grant_step(gfo_match* match, const gfo_match_artifact_grant_action* input, gfo_match_status* out) {
    if(!match || !input || !out)return -1;
    try { const auto parsed=artifact_grant_action(*input);auto next=match->value;next.artifact_grant_step(parsed);const auto result=status(next);
        match->value=std::move(next);*out=result;return 0; }
    catch(...) { return error(); }
}
int gfo_match_sample_action(const gfo_match* match, uint64_t* policy_rng, gfo_match_command* out) {
    if (!match || !policy_rng || !out) return -1;
    try {
        valid(!match->value.done()); auto rng = *policy_rng; const auto result = encode(match->value.sample_action(rng));
        *policy_rng = rng; *out = result; return 0;
    } catch (...) { return error(); }
}
int gfo_match_sample_confusion(const gfo_match* match,uint64_t* chance_rng,gfo_match_command* out) {
    if(!match || !chance_rng || !out)return -1;
    try {
        auto rng=*chance_rng;const auto result=encode(match->value.sample_confusion(rng));
        *out=result;*chance_rng=rng;return 0;
    }catch(...){return error();}
}
int gfo_match_sample_guardian_payload(const gfo_match* match,uint64_t source,uint64_t* chance_rng,gfo_match_guardian_payload_action* out) {
    if(!match || !chance_rng || !out)return -1;
    try {
        valid(source<=official::max_wire_integer);auto rng=*chance_rng;
        const auto result=encode_guardian_payload(match->value.sample_guardian_payload(source,rng));
        *out=result;*chance_rng=rng;return 0;
    }catch(...){return error();}
}
int gfo_match_sample_grant(const gfo_match* match, uint64_t* chance_rng, gfo_match_grant_action* out) {
    if(!match || !chance_rng || !out) return -1;
    try {
        auto rng=*chance_rng;const auto result=encode_grant(match->value.sample_grant(rng));
        *chance_rng=rng;*out=result;return 0;
    } catch(...) {return error();}
}
int gfo_match_sample_fate(const gfo_match* match,uint64_t token,uint64_t* chance_rng,
                        gfo_match_item_source* order,size_t capacity,gfo_match_fate_action* out) {
    if(!match || !chance_rng || !out)return -1;
    if(capacity>official::player_capacity*official::visible_hand_capacity)return -2;
    if(capacity&&!order)return -1;
    try {
        valid(token<=official::max_wire_integer);auto rng=*chance_rng;
        const auto action=match->value.sample_fate(token,rng);std::array<gfo_match_item_source,162> staged;
        auto encoded=encode_fate(action,staged);
        if(encoded.shuffle.count>capacity)return -4;
        if(encoded.shuffle.count){std::copy_n(staged.begin(),encoded.shuffle.count,order);encoded.shuffle.order=order;}
        *out=encoded;*chance_rng=rng;return 0;
    }catch(...){return error();}
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
