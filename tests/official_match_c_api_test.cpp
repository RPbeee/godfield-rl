#include "godfield/official_match_c_api.h"
#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <utility>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
namespace {
void check(bool ok, const char* text) { if (!ok) { std::cerr << "FAIL match C API: " << text << '\n'; std::exit(1); } }
template<class T> bool equal(const T& a, const T& b) { return std::memcmp(&a, &b, sizeof(T)) == 0; }
std::string observe(gfo_match* match, uint8_t own = 1, uint64_t id = 0) {
    size_t size = 0;
    check(gfo_match_observe(match, own, id, nullptr, 0, &size) == 0 && size > 1, "observation size query");
    std::vector<char> bytes(size + 2, 'Z');
    check(gfo_match_observe(match, own, id, bytes.data() + 1, size, &size) == 0
        && bytes.front() == 'Z' && bytes.back() == 'Z' && bytes[size] == 0, "observe canaries/NUL");
    return {bytes.data() + 1};
}
gfo_match_status status(gfo_match* match) { gfo_match_status s{}; check(gfo_match_get_status(match, &s) == 0, "status"); return s; }
}
int main() {
    check(gfo_match_abi_version() == 1 && gfo_match_config_size() == sizeof(gfo_match_config)
        && gfo_match_player_size() == sizeof(gfo_match_player) && gfo_match_command_size() == sizeof(gfo_match_command)
        && gfo_match_status_size() == sizeof(gfo_match_status), "ABI sizes");
    check(gfo_match_catalog_count() == 209 && gfo_match_gift_weight() == 412 && gfo_match_supported(6)
        && !gfo_match_supported(112) && !gfo_match_supported(30), "supported subset descriptor");
    check(std::string(gfo_match_ruleset()) == "official-execution-subset-v14"
        && std::string(gfo_match_scope()) == "experimental:209-models;not-official-server-equivalence", "explicit v14 scope");
    for (auto id : {38, 39, 156, 167, 174, 189, 190}) check(gfo_match_supported(id), "added reflection/filter membership");
    for (auto id : {112, 182, 183}) check(!gfo_match_supported(id), "death-passive chance and curse counters unsupported");
    for (auto id : {42, 50, 55, 56, 165, 171, 177, 179, 233}) check(gfo_match_supported(id), "added block membership");
    for (auto id : {46, 66, 83, 143, 163, 173, 204, 231}) check(gfo_match_supported(id), "added composition membership");
    for (auto id : {21, 34, 114, 125, 137, 151, 164, 234}) check(gfo_match_supported(id), "added bounce membership");
    for (auto id : {95, 96, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 225, 226, 227, 228, 229, 230}) check(gfo_match_supported(id), "added pure chance membership");
    for (auto id : {181, 184}) check(gfo_match_supported(id), "damage counter membership");
    for (auto id : {187, 188}) check(gfo_match_supported(id), "resource counter membership");
    for (auto id : {27, 45, 98, 216, 53}) check(gfo_match_supported(id), "damage-effect membership");
    for (auto id : {70, 185, 224, 199, 200, 237, 238}) check(gfo_match_supported(id), "Flash and cure membership");
    const char name[] = {'A', '\0', 'B'};
    std::array<gfo_match_player, 2> players{}; players[0].id = 0; players[0].name = name; players[0].name_size = 3;
    players[1].id = 77; players[1].name = "C"; players[1].name_size = 1;
    gfo_match* a = nullptr; gfo_match* b = nullptr;
    check(gfo_match_create(nullptr, players.data(), 2, 42, &a) == 0 && a, "create");
    check(gfo_match_create(nullptr, players.data(), 2, 42, &b) == 0 && b, "create replay");
    const auto before = observe(a); const auto initial = status(a);
    check(before.find("A\\u0000B") != std::string::npos && initial.player_count == 2 && initial.player_ids[1] == 77, "NUL name/fixed ID order");
    std::array<char, 8> short_buffer{}; short_buffer.fill('Q'); const auto old = short_buffer; size_t needed = 555;
    check(gfo_match_observe(a, 1, 0, short_buffer.data(), short_buffer.size(), &needed) == -4
        && short_buffer == old && needed == 555 && observe(a) == before, "short observe is entirely atomic");
    check(gfo_match_observe(a, 1, 88, short_buffer.data(), 8, &needed) == -2 && needed == 555 && short_buffer == old, "unknown viewer output guard");
    check(gfo_match_observe(a, 0, 9, nullptr, 0, &needed) == -2 && needed == 555, "canonical spectator selector");
    uint64_t rng = 12345; gfo_match_command sample{};
    check(gfo_match_sample_action(a, &rng, &sample) == 0 && observe(a) == before, "sample changes only separate policy RNG");
    const auto sample_rng = rng;
    check(gfo_match_sample_action(a, &rng, nullptr) == -1 && rng == sample_rng && observe(a) == before, "NULL sampler output preserves RNG/state");
    check(gfo_match_step(a, &sample, nullptr) == -1 && observe(a) == before, "NULL step result preserves game");
    gfo_match_status result; std::memset(&result, 0x51, sizeof(result)); const auto saved_result = result;
    auto illegal = sample; illegal.actor = 999;
    check(gfo_match_step(a, &illegal, &result) == -2 && equal(result, saved_result) && observe(a) == before, "invalid step leaves output and game unchanged");
    uint8_t legal = 77;
    illegal = sample; illegal.item_count = 19;
    check(gfo_match_legal(a, &illegal, &legal) == -2 && legal == 77, "count19 rejected before items access");
    illegal = sample; illegal.reserved[4] = 1;
    check(gfo_match_step(a, &illegal, &result) == -2 && equal(result, saved_result), "reserved command bytes rejected");
    illegal = sample;
    if (illegal.item_count < 18) { illegal.items[17] = 1; check(gfo_match_step(a, &illegal, &result) == -2 && equal(result, saved_result), "unused token tail rejected"); }
    auto bad_players = players; bad_players[1].id = 0;
    check(gfo_match_reset(a, bad_players.data(), 2, 7, &result) == -2 && equal(result, saved_result) && observe(a) == before, "invalid reset atomicity");
    bad_players = players; bad_players[0].name = "X"; bad_players[0].name_size = std::numeric_limits<size_t>::max();
    check(gfo_match_reset(a, bad_players.data(), 2, 7, &result) == -2 && equal(result, saved_result) && observe(a) == before, "SIZE_MAX string rejected before dereference");
    check(gfo_match_reset(a, players.data(), std::numeric_limits<size_t>::max(), 7, &result) == -2 && equal(result, saved_result), "SIZE_MAX setup count rejected before dereference");
    bad_players = players; bad_players[0].name = "\xed\xa0\x80"; bad_players[0].name_size = 3;
    check(gfo_match_reset(a, bad_players.data(), 2, 7, &result) == -2 && equal(result, saved_result), "invalid UTF8 name rejected");
    auto* sentinel = reinterpret_cast<gfo_match*>(uintptr_t{1});
    check(gfo_match_create(nullptr, bad_players.data(), 2, 42, &sentinel) == -2
        && sentinel == reinterpret_cast<gfo_match*>(uintptr_t{1}), "create output unchanged");
    // Replay after every failed operation proves the private game RNG remains unchanged.
    uint64_t ra = 9, rb = 9;
    for (int i = 0; i < 128 && status(a).has_actor; ++i) {
        gfo_match_command ca{}, cb{};
        check(gfo_match_sample_action(a, &ra, &ca) == 0 && gfo_match_sample_action(b, &rb, &cb) == 0 && ra == rb && equal(ca, cb), "separate-stream deterministic replay");
        gfo_match_status sa{}, sb{};
        check(gfo_match_step(a, &ca, &sa) == 0 && gfo_match_step(b, &cb, &sb) == 0 && equal(sa, sb)
            && observe(a, 0, 0) == observe(b, 0, 0), "replay status/board including future gifts");
    }
    gfo_match_config cfg{}; cfg.max_decisions = 1; cfg.initial_hand = 0; cfg.hand_limit = 18;
    cfg.initial_hp = 40; cfg.initial_mp = 10; cfg.initial_cp = 20;
    gfo_match* short_match = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, 1, &short_match) == 0, "zero-hand canonical match");
    gfo_match_command pray{}; pray.actor = 0; pray.kind = 3;
    check(gfo_match_step(short_match, &pray, &result) == 0 && result.truncated && !result.terminated
        && !result.has_actor && result.rewards[0] == 0 && result.rewards[1] == 0, "fixed-seat truncation");
    rng = 98; sample.kind = 77; const auto stale = sample;
    check(gfo_match_sample_action(short_match, &rng, &sample) == -2 && rng == 98 && equal(sample, stale), "finished sampler atomic failure");
    // Discover tokens with the same public seeded reset contract; all actions
    // and their continuation below cross the opaque C boundary, not load_scene.
    namespace official = godfield::official;
    official::MatchConfig reflection_config; reflection_config.initial_hand = 18; reflection_config.initial_hp = 99;
    official::Match fixture(reflection_config);
    const std::array<official::PlayerSetup, 2> setup{{{0, official::Team::solo, "A"}, {77, official::Team::solo, "B"}}};
    bool found = false; uint64_t fixture_seed = 0, weapon_token = 0, mirror_token = 0;
    for (; fixture_seed < 4096 && !found; ++fixture_seed) {
        fixture.reset(setup, fixture_seed);
        bool weapon = false, mirror = false;
        for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 6) { weapon = true; weapon_token = item.token; }
        for (const auto& item : fixture.snapshot().state.players[1].items) if (item.model == 38) { mirror = true; mirror_token = item.token; }
        found = weapon && mirror;
    }
    check(found, "seeded reflection fixture is reachable from supported gift pool"); --fixture_seed;
    cfg.max_decisions = 512; cfg.initial_hand = 18; cfg.initial_hp = 99;
    gfo_match* reflected_match = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, fixture_seed, &reflected_match) == 0, "reflection create");
    gfo_match_command attack{}; attack.actor = 0; attack.has_target = 1; attack.target = 77; attack.item_count = 1; attack.items[0] = weapon_token;
    check(gfo_match_step(reflected_match, &attack, &result) == 0 && result.phase == 1 && result.actor == 77 && result.owner == 0, "attack opens response via C ABI");
    gfo_match_command reflect{}; reflect.actor = 77; reflect.kind = 1; reflect.item_count = 1; reflect.items[0] = mirror_token;
    check(gfo_match_step(reflected_match, &reflect, &result) == 0 && result.phase == 1 && result.actor == 0 && result.owner == 0, "reflection returns response to attacker");
    const auto returned = observe(reflected_match, 0, 0);
    check(returned.find("\"origin\":0,\"source\":77,\"target\":0") != std::string::npos, "reflection public origin/current-source separation");
    const auto reflected_status = result;
    check(gfo_match_step(reflected_match, &reflect, &result) == -2 && equal(result, reflected_status)
        && observe(reflected_match, 0, 0) == returned, "wrong-actor continuation is atomic");
    gfo_match_command accept{}; accept.actor = 0; accept.kind = 1;
    check(gfo_match_step(reflected_match, &accept, &result) == 0 && result.phase == 0 && result.actor == 77 && result.owner == 77,
        "reflected item resolves once then normal next turn");
    found = false; fixture_seed = 0; uint64_t wall_token = 0;
    for (; fixture_seed < 4096 && !found; ++fixture_seed) {
        fixture.reset(setup, fixture_seed);
        bool weapon = false, wall = false;
        for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 6) { weapon = true; weapon_token = item.token; }
        for (const auto& item : fixture.snapshot().state.players[1].items) if (item.model == 233) { wall = true; wall_token = item.token; }
        found = weapon && wall;
    }
    check(found, "retained Wall reachable from public reset pool"); --fixture_seed;
    gfo_match* blocked_match = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, fixture_seed, &blocked_match) == 0, "Wall fixture create");
    attack.items[0] = weapon_token;
    check(gfo_match_step(blocked_match, &attack, &result) == 0 && result.phase == 1 && result.actor == 77, "Wall incoming weapon");
    gfo_match_command wall{}; wall.actor = 77; wall.kind = 1; wall.item_count = 1; wall.items[0] = wall_token;
    check(gfo_match_step(blocked_match, &wall, &result) == 0 && result.phase == 0 && result.actor == 77 && result.owner == 77, "matching block finishes instead of opening another response");
    const auto blocked = observe(blocked_match, 1, 77);
    check(blocked.find("\"pending\":null") != std::string::npos && blocked.find("\"hp\":99,\"mp\":4") != std::string::npos,
        "block cancels damage and pays six MP");
    const auto wall_json = "\"token\":" + std::to_string(wall_token) + ",\"face\":233,\"reference\":233,\"disguised\":false,\"used\":true";
    check(blocked.find(wall_json) != std::string::npos, "blocking miracle stays retained at same token");
    wall.kind = 0; wall.has_target = 1; wall.target = 0;
    const auto blocked_status = result; legal = 55;
    check(gfo_match_legal(blocked_match, &wall, &legal) == 0 && !legal, "retained Wall is not a main action");
    check(gfo_match_step(blocked_match, &wall, &result) == -2 && equal(result, blocked_status)
        && observe(blocked_match, 1, 77) == blocked, "main Wall rejection preserves outputs and frame");
    found = false; fixture_seed = 0; uint64_t spring_token = 0, free_token = 0;
    for (; fixture_seed < 8192 && !found; ++fixture_seed) {
        fixture.reset(setup, fixture_seed); bool spring = false, free = false;
        for (const auto& item : fixture.snapshot().state.players[0].items) {
            if (item.model == 235) { spring = true; spring_token = item.token; }
            if (item.model == 204) { free = true; free_token = item.token; }
        }
        found = spring && free;
    }
    check(found, "zero-MP Spring/FREE fixture reachable"); --fixture_seed;
    cfg.initial_hp = 40; cfg.initial_mp = 0;
    gfo_match* free_match = nullptr; gfo_match* free_replay = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, fixture_seed, &free_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, fixture_seed, &free_replay) == 0, "zero-MP FREE create");
    const auto unsampled = observe(free_match);
    bool pair_sampled = false;
    for (uint64_t policy_seed = 0; policy_seed < 4096 && !pair_sampled; ++policy_seed) {
        uint64_t policy = policy_seed, same_policy = policy_seed; gfo_match_command candidate{}, replay{};
        check(gfo_match_sample_action(free_match, &policy, &candidate) == 0
            && gfo_match_sample_action(free_match, &same_policy, &replay) == 0
            && policy == same_policy && equal(candidate, replay), "zero-MP sampler deterministic policy stream");
        check(gfo_match_legal(free_match, &candidate, &legal) == 0 && legal, "every sampled zero-MP action legal");
        pair_sampled = candidate.kind == 0 && candidate.item_count == 2
            && candidate.items[0] == spring_token && candidate.items[1] == free_token;
    }
    check(pair_sampled && observe(free_match) == unsampled, "zero-MP sampler reaches miracle+FREE without changing observation");
    gfo_match_command free_cast{}; free_cast.actor = 0; free_cast.has_target = 1; free_cast.target = 0;
    free_cast.item_count = 2; free_cast.items[0] = spring_token; free_cast.items[1] = free_token;
    gfo_match_status free_result{}, replay_result{};
    check(gfo_match_step(free_match, &free_cast, &free_result) == 0 && gfo_match_step(free_replay, &free_cast, &replay_result) == 0
        && equal(free_result, replay_result) && observe(free_match) == observe(free_replay), "sampling preserves private RNG including next replacement gift");
    const auto free_observation = observe(free_match);
    check(free_observation.find("\"hp\":50,\"mp\":0") != std::string::npos
        && free_observation.find("\"token\":" + std::to_string(free_token) + ",") == std::string::npos
        && free_observation.find("\"token\":" + std::to_string(spring_token) + ",\"face\":235,\"reference\":235,\"disguised\":false,\"used\":true") != std::string::npos,
        "zero-MP FREE heal consumes carrier and retains miracle");
    std::size_t hand_count = 0, offset = 0;
    while ((offset = free_observation.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 18, "FREE replacement gifts respect retained full-hand cap");
    gfo_match_destroy(free_match); gfo_match_destroy(free_replay);
    found = false; fixture_seed = 0;
    for (; fixture_seed < 8192 && !found; ++fixture_seed) {
        fixture.reset(setup, fixture_seed); bool weapon = false, wall_found = false, free = false;
        for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 6) { weapon = true; weapon_token = item.token; }
        for (const auto& item : fixture.snapshot().state.players[1].items) {
            if (item.model == 233) { wall_found = true; wall_token = item.token; }
            if (item.model == 204) { free = true; free_token = item.token; }
        }
        found = weapon && wall_found && free;
    }
    check(found, "zero-MP Wall/FREE fixture reachable"); --fixture_seed;
    gfo_match* free_wall = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, fixture_seed, &free_wall) == 0, "zero-MP Wall create");
    attack.items[0] = weapon_token;
    check(gfo_match_step(free_wall, &attack, &result) == 0 && result.phase == 1, "zero-MP Wall incoming attack");
    wall.kind = 1; wall.has_target = 0; wall.target = 0; wall.item_count = 2; wall.items[0] = wall_token; wall.items[1] = free_token;
    check(gfo_match_legal(free_wall, &wall, &legal) == 0 && legal, "zero-MP Wall+FREE response legal");
    check(gfo_match_step(free_wall, &wall, &result) == 0 && result.phase == 0 && result.actor == 77, "free Wall completes payload");
    const auto free_wall_observation = observe(free_wall, 1, 77);
    check(free_wall_observation.find("\"hp\":40,\"mp\":0") != std::string::npos
        && free_wall_observation.find("\"pending\":null") != std::string::npos
        && free_wall_observation.find("\"token\":" + std::to_string(free_token) + ",") == std::string::npos
        && free_wall_observation.find("\"token\":" + std::to_string(wall_token) + ",\"face\":233,\"reference\":233,\"disguised\":false,\"used\":true") != std::string::npos,
        "free response consumes physical carrier and retains same Wall token");
    hand_count = 0; offset = 0;
    while ((offset = free_wall_observation.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 18, "free response refill clipped at hand cap");
    gfo_match_destroy(free_wall);
    // Public seeded reset contains Spring+FREE and Turbulence+FREE at MP0.
    fixture.reset(setup, 208429);
    bool got_spring = false, got_cast_free = false, got_bounce = false, got_bounce_free = false;
    uint64_t cast_free_token = 0, bounce_token = 0, bounce_free_token = 0;
    for (const auto& item : fixture.snapshot().state.players[0].items) {
        if (item.model == 235) { got_spring = true; spring_token = item.token; }
        if (item.model == 204) { got_cast_free = true; cast_free_token = item.token; }
    }
    for (const auto& item : fixture.snapshot().state.players[1].items) {
        if (item.model == 234) { got_bounce = true; bounce_token = item.token; }
        if (item.model == 173) { got_bounce_free = true; bounce_free_token = item.token; }
    }
    check(got_spring && got_cast_free && got_bounce && got_bounce_free, "public reset zero-MP bounce fixture");
    gfo_match* bounce_match = nullptr; gfo_match* bounce_replay = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, 208429, &bounce_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 208429, &bounce_replay) == 0, "bounce fixture create");
    free_cast.target = 77; free_cast.items[0] = spring_token; free_cast.items[1] = cast_free_token;
    check(gfo_match_step(bounce_match, &free_cast, &free_result) == 0
        && gfo_match_step(bounce_replay, &free_cast, &replay_result) == 0 && equal(free_result, replay_result), "incoming recovery miracle opens bounce response");
    gfo_match_command turbulence{}; turbulence.actor = 77; turbulence.kind = 1; turbulence.item_count = 1; turbulence.items[0] = bounce_token;
    const auto before_bounce = observe(bounce_match, 1, 77); result = free_result;
    check(gfo_match_step(bounce_match, &turbulence, &result) == -2 && equal(result, free_result)
        && observe(bounce_match, 1, 77) == before_bounce, "paid Turbulence cannot spill MP debt at MP0");
    turbulence.item_count = 2; turbulence.items[1] = bounce_free_token;
    check(gfo_match_legal(bounce_match, &turbulence, &legal) == 0 && legal, "zero-MP Turbulence/FREE is legal");
    pair_sampled = false;
    for (uint64_t policy_seed = 0; policy_seed < 4096 && !pair_sampled; ++policy_seed) {
        uint64_t policy = policy_seed, same_policy = policy_seed; gfo_match_command candidate{}, replay{};
        check(gfo_match_sample_action(bounce_match, &policy, &candidate) == 0
            && gfo_match_sample_action(bounce_match, &same_policy, &replay) == 0
            && policy == same_policy && equal(candidate, replay), "bounce sampler reproducible policy stream");
        check(gfo_match_legal(bounce_match, &candidate, &legal) == 0 && legal, "all sampled bounce-context actions legal");
        pair_sampled = equal(candidate, turbulence);
    }
    check(pair_sampled && observe(bounce_match, 1, 77) == before_bounce, "sampler reaches free bounce without changing public state");
    check(gfo_match_step(bounce_match, &turbulence, &free_result) == 0
        && gfo_match_step(bounce_replay, &turbulence, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 1 && free_result.owner == 0 && (free_result.actor == 0 || free_result.actor == 77),
        "bounce transfers to an alive responder while retaining original owner");
    const auto bounced_view = observe(bounce_match, 1, 77);
    check(bounced_view.find("\"schema\":\"official-execution-view-v12\"") != std::string::npos
        && bounced_view.find("\"bounce_target_rule\":\"experimental:uniform-all-alive-in-input-order;includes-self-and-teammates\"") != std::string::npos
        && bounced_view.find("\"origin\":0,\"source\":77,\"target\":" + std::to_string(free_result.actor)) != std::string::npos
        && bounced_view.find("\"bounces\":1") != std::string::npos && bounced_view.find("\"reflections\":0") != std::string::npos,
        "public bounce contract and separate redirect counters");
    check(bounced_view.find("\"redirect_rng\":") == std::string::npos && bounced_view.find("\"redirects\":") == std::string::npos
        && bounced_view.find("\"tokens\":") == std::string::npos, "private redirect replay evidence is not observed");
    check(bounced_view.find("\"token\":" + std::to_string(bounce_free_token) + ",") == std::string::npos
        && bounced_view.find("\"token\":" + std::to_string(bounce_token) + ",\"face\":234,\"reference\":234,\"disguised\":false,\"used\":true") != std::string::npos,
        "bounce FREE physical suffix consumed while miracle is retained");
    hand_count = 0; offset = 0;
    while ((offset = bounced_view.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 17, "bounce replacement remains deferred");
    accept.actor = free_result.actor;
    check(gfo_match_step(bounce_match, &accept, &free_result) == 0
        && gfo_match_step(bounce_replay, &accept, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 0 && free_result.actor == 77 && free_result.owner == 77,
        "self or foreign bounce always has explicit response then finishes once");
    check(observe(bounce_match, 1, 0) == observe(bounce_replay, 1, 0)
        && observe(bounce_match, 1, 77) == observe(bounce_replay, 1, 77), "query/policy sampling preserves bounce RNG and future gift stream");
    const auto complete_bounce = observe(bounce_match, 1, 77);
    hand_count = 0; offset = 0;
    while ((offset = complete_bounce.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 18 && complete_bounce.find("\"pending\":null") != std::string::npos, "bounce final gift cap");
    gfo_match_destroy(bounce_match); gfo_match_destroy(bounce_replay);
    // Chance commands use the existing ABI1 has_target=0 representation.
    std::array<gfo_match_player, 4> chance_players{};
    const std::array<official::PlayerSetup, 4> chance_setup{{{0, official::Team::one, "A"}, {77, official::Team::one, "B"},
        {88, official::Team::solo, "C"}, {99, official::Team::two, "D"}}};
    for (std::size_t i = 0; i < chance_players.size(); ++i) {
        chance_players[i].id = chance_setup[i].id; chance_players[i].team = static_cast<uint8_t>(chance_setup[i].team);
    }
    fixture.reset(chance_setup, 21); uint64_t chance_token = 0; bool found_chance = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 95) { chance_token = item.token; found_chance = true; }
    check(found_chance, "public reset multi-target chance fixture");
    gfo_match* chance_match = nullptr; gfo_match* chance_replay = nullptr;
    check(gfo_match_create(&cfg, chance_players.data(), 4, 21, &chance_match) == 0
        && gfo_match_create(&cfg, chance_players.data(), 4, 21, &chance_replay) == 0, "chance create");
    gfo_match_command chance_command{}; chance_command.item_count = 1; chance_command.items[0] = chance_token;
    auto explicit_target = chance_command; explicit_target.has_target = 1; explicit_target.target = 99;
    const auto chance_before = observe(chance_match); result = status(chance_match); const auto chance_status = result;
    check(gfo_match_legal(chance_match, &explicit_target, &legal) == 0 && !legal
        && gfo_match_step(chance_match, &explicit_target, &result) == -2 && equal(result, chance_status)
        && observe(chance_match) == chance_before, "explicit chance target is rejected atomically");
    bool chance_sampled = false;
    for (uint64_t policy_seed = 0; policy_seed < 4096 && !chance_sampled; ++policy_seed) {
        uint64_t policy = policy_seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(chance_match, &policy, &candidate) == 0
            && gfo_match_legal(chance_match, &candidate, &legal) == 0 && legal, "multi-target samples all legal");
        chance_sampled = equal(candidate, chance_command);
    }
    check(chance_sampled, "C ABI sampler emits targetless chance command");
    check(gfo_match_step(chance_match, &chance_command, &free_result) == 0
        && gfo_match_step(chance_replay, &chance_command, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 1 && free_result.actor == 99 && free_result.owner == 0, "miss first enemy then response only for second enemy");
    const auto chance_view = observe(chance_match, 0, 0); // Spectator: only public pending/privacy checks below.
    check(chance_view.find("\"hit_rate\":75") != std::string::npos
        && chance_view.find("\"chance_rule\":\"experimental:targetless-all-alive-enemies-in-input-order;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll;skip-dead;cancel-on-origin-death\"") != std::string::npos,
        "public chance metadata and hit display");
    for (const auto* key : {"\"chance\":", "\"chance_rolls\":", "\"chance_hits\":", "\"targets\":", "\"cursor\":", "\"initial_state\":", "\"responses\":"})
        check(chance_view.find(key) == std::string::npos, "private batch replay state stays hidden");
    accept.actor = 99;
    check(gfo_match_step(chance_match, &accept, &free_result) == 0
        && gfo_match_step(chance_replay, &accept, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 0 && free_result.actor == 77 && free_result.owner == 77
        && observe(chance_match) == observe(chance_replay), "batch completion defers gifts until all targets finish");
    const auto complete_chance = observe(chance_match, 1, 0); // Explicit owner view includes replacement hand.
    check(complete_chance.find("\"id\":99,\"name\":\"\",\"team\":2,\"dead\":false,\"fogged\":false,\"status\":{\"hp\":39") != std::string::npos,
        "only hit enemy loses one HP");
    hand_count = 0; offset = 0;
    while ((offset = complete_chance.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 18, "chance launch replacement counted once");
    gfo_match_destroy(chance_match); gfo_match_destroy(chance_replay);
    fixture.reset(setup, 5); found_chance = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 95) { chance_token = item.token; found_chance = true; }
    check(found_chance && gfo_match_create(&cfg, players.data(), 2, 5, &chance_match) == 0, "all-miss fixture create");
    chance_command.items[0] = chance_token;
    check(gfo_match_step(chance_match, &chance_command, &result) == 0 && result.decisions == 1 && result.phase == 0
        && result.actor == 77 && result.owner == 77 && observe(chance_match).find("\"pending\":null") != std::string::npos,
        "all misses complete without any defense decision");
    gfo_match_destroy(chance_match);
    fixture.reset(setup, 9971);
    bool got_chance_mc = false; got_cast_free = false; bool got_reflector = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) {
        if (item.model == 225) { got_chance_mc = true; spring_token = item.token; }
        if (item.model == 204) { got_cast_free = true; cast_free_token = item.token; }
    }
    for (const auto& item : fixture.snapshot().state.players[1].items) if (item.model == 39) { got_reflector = true; mirror_token = item.token; }
    check(got_chance_mc && got_cast_free && got_reflector
        && gfo_match_create(&cfg, players.data(), 2, 9971, &chance_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 9971, &chance_replay) == 0, "zero-MP chance miracle/FREE fixture create");
    chance_command.item_count = 2; chance_command.items[0] = spring_token; chance_command.items[1] = cast_free_token;
    chance_sampled = false;
    for (uint64_t policy_seed = 0; policy_seed < 4096 && !chance_sampled; ++policy_seed) {
        uint64_t policy = policy_seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(chance_match, &policy, &candidate) == 0
            && gfo_match_legal(chance_match, &candidate, &legal) == 0 && legal, "zero-MP chance-pair samples all legal");
        chance_sampled = equal(candidate, chance_command);
    }
    check(chance_sampled && gfo_match_step(chance_match, &chance_command, &free_result) == 0
        && gfo_match_step(chance_replay, &chance_command, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 1 && free_result.actor == 77, "targetless chance miracle FREE sampled and hits");
    reflect.items[0] = mirror_token;
    check(gfo_match_step(chance_match, &reflect, &free_result) == 0 && gfo_match_step(chance_replay, &reflect, &replay_result) == 0
        && equal(free_result, replay_result) && free_result.phase == 1 && free_result.actor == 0, "hit chance payload can be reflected without reroll");
    const auto redirected_chance = observe(chance_match, 0, 0); // Spectator: origin/source/target and rate only.
    check(redirected_chance.find("\"origin\":0,\"source\":77,\"target\":0") != std::string::npos
        && redirected_chance.find("\"hit_rate\":75") != std::string::npos && redirected_chance.find("\"reflections\":1") != std::string::npos,
        "redirect keeps original chance rate and owner evidence");
    accept.actor = 0;
    check(gfo_match_step(chance_match, &accept, &free_result) == 0 && gfo_match_step(chance_replay, &accept, &replay_result) == 0
        && equal(free_result, replay_result) && observe(chance_match) == observe(chance_replay)
        && observe(chance_match).find("\"hp\":35,\"mp\":0") != std::string::npos, "free chance reflection resolves once and keeps query/policy RNG separate");
    const auto retained_chance = observe(chance_match);
    check(retained_chance.find("\"token\":" + std::to_string(spring_token) + ",\"face\":225,\"reference\":225,\"disguised\":false,\"used\":true") != std::string::npos
        && retained_chance.find("\"token\":" + std::to_string(cast_free_token) + ",") == std::string::npos,
        "chance miracle learned once and physical FREE consumed once");
    gfo_match_destroy(chance_match); gfo_match_destroy(chance_replay);
    // Ordinary DEF reduces HP loss before the selected counters enter FIFO.
    fixture.reset(setup, 10264);
    uint64_t armor_token = 0, mars_token = 0, saturn_token = 0;
    bool got_attack = false, got_armor = false, got_mars = false, got_saturn = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 32) { weapon_token = item.token; got_attack = true; }
    for (const auto& item : fixture.snapshot().state.players[1].items) {
        if (item.model == 118) { armor_token = item.token; got_armor = true; }
        if (item.model == 181) { mars_token = item.token; got_mars = true; }
        if (item.model == 184) { saturn_token = item.token; got_saturn = true; }
    }
    check(got_attack && got_armor && got_mars && got_saturn, "public reset mixed-counter fixture");
    gfo_match* counter_match = nullptr; gfo_match* counter_replay = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, 10264, &counter_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 10264, &counter_replay) == 0, "counter fixture create");
    attack.items[0] = weapon_token;
    check(gfo_match_step(counter_match, &attack, &free_result) == 0 && gfo_match_step(counter_replay, &attack, &replay_result) == 0
        && equal(free_result, replay_result), "ordinary incoming damage for counters");
    const auto before_counters = observe(counter_match, 1, 77);
    for (uint64_t policy_seed = 0; policy_seed < 64; ++policy_seed) {
        uint64_t policy = policy_seed, same_policy = policy_seed; gfo_match_command candidate{}, duplicate{};
        check(gfo_match_sample_action(counter_match, &policy, &candidate) == 0
            && gfo_match_sample_action(counter_match, &same_policy, &duplicate) == 0 && policy == same_policy && equal(candidate, duplicate)
            && gfo_match_legal(counter_match, &candidate, &legal) == 0 && legal, "counter-context sampler legal and reproducible");
    }
    check(observe(counter_match, 1, 77) == before_counters, "counter-context policy sampling leaves match unchanged");
    gfo_match_command defend_counters{}; defend_counters.actor = 77; defend_counters.kind = 1; defend_counters.item_count = 3;
    defend_counters.items[0] = armor_token; defend_counters.items[1] = mars_token; defend_counters.items[2] = saturn_token;
    check(gfo_match_step(counter_match, &defend_counters, &free_result) == 0
        && gfo_match_step(counter_replay, &defend_counters, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 1 && free_result.actor == 0 && free_result.owner == 0, "counter FIFO begins before turn completion");
    const auto mars_view = observe(counter_match, 1, 77);
    check(mars_view.find("\"counter_rule\":\"experimental:actual-hp-loss;survivors-only;selection-order-fifo-before-next-target;skip-dead-source-or-target;single-target;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll\"") != std::string::npos
        && mars_view.find("\"origin\":77,\"source\":77,\"target\":0") != std::string::npos
        && mars_view.find("\"counter\":true") != std::string::npos && mars_view.find("\"models\":[181]") != std::string::npos
        && mars_view.find("\"power\":7") != std::string::npos && mars_view.find("\"hit_rate\":75") != std::string::npos,
        "Mars uses actual seven HP loss after DEF2 instead of original ATK9");
    check(mars_view.find("\"hp\":33,\"mp\":0") != std::string::npos, "surviving defender paid real HP loss");
    hand_count = 0; offset = 0;
    while ((offset = mars_view.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 15, "three selected physical defense cards consumed once before deferred refill");
    for (const auto* key : {"\"batch\":", "\"counters\":", "\"counter_created\":", "\"counter_started\":", "\"counter_rolls\":", "\"counter_hits\":", "\"responses\":", "\"initial_state\":"})
        check(mars_view.find(key) == std::string::npos, "future reserved counters and replay journal stay private");
    accept.actor = 0;
    check(gfo_match_step(counter_match, &accept, &free_result) == 0 && gfo_match_step(counter_replay, &accept, &replay_result) == 0
        && equal(free_result, replay_result) && free_result.phase == 1 && free_result.actor == 0 && free_result.owner == 0,
        "first counter resolves then second FIFO counter opens response");
    const auto saturn_view = observe(counter_match, 0, 0);
    check(saturn_view.find("\"counter\":true") != std::string::npos && saturn_view.find("\"models\":[184]") != std::string::npos
        && saturn_view.find("\"power\":14") != std::string::npos && saturn_view.find("\"hit_rate\":0") != std::string::npos,
        "Saturn doubles actual HP loss and follows Mars selection order");
    check(gfo_match_step(counter_match, &accept, &free_result) == 0 && gfo_match_step(counter_replay, &accept, &replay_result) == 0
        && equal(free_result, replay_result) && free_result.phase == 0 && free_result.actor == 77 && free_result.owner == 77,
        "all counters complete before ordinary next owner");
    check(observe(counter_match, 1, 0) == observe(counter_replay, 1, 0)
        && observe(counter_match, 1, 77) == observe(counter_replay, 1, 77), "policy queries preserve counter roll and future gift RNG");
    const auto complete_counter = observe(counter_match, 1, 77);
    check(complete_counter.find("\"hp\":19,\"mp\":0") != std::string::npos && complete_counter.find("\"hp\":33,\"mp\":0") != std::string::npos
        && complete_counter.find("\"pending\":null") != std::string::npos, "counter FIFO final resource deltas");
    hand_count = 0; offset = 0;
    while ((offset = complete_counter.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 18, "counter defenses refill only after full action");
    gfo_match_destroy(counter_match); gfo_match_destroy(counter_replay);
    fixture.reset(setup, 43242); got_attack = false; got_mars = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 6) { weapon_token = item.token; got_attack = true; }
    for (const auto& item : fixture.snapshot().state.players[1].items) if (item.model == 181) { mars_token = item.token; got_mars = true; }
    check(got_attack && got_mars && gfo_match_create(&cfg, players.data(), 2, 43242, &counter_match) == 0, "Mars-miss fixture create");
    attack.items[0] = weapon_token; defend_counters.item_count = 1; defend_counters.items[0] = mars_token;
    defend_counters.items[1] = 0; defend_counters.items[2] = 0;
    check(gfo_match_step(counter_match, &attack, &result) == 0
        && gfo_match_step(counter_match, &defend_counters, &result) == 0 && result.decisions == 2 && result.phase == 0
        && result.actor == 77 && result.owner == 77 && observe(counter_match).find("\"pending\":null") != std::string::npos,
        "missed Mars counter consumes no extra response decision");
    gfo_match_destroy(counter_match);
    // Resource counters stay armor payloads, never summon metadata guardians.
    cfg.initial_cp = 0;
    fixture.reset(setup, 152); bool got_mp_counter = false; got_attack = false; got_armor = false;
    uint64_t resource_token = 0;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 6) { weapon_token = item.token; got_attack = true; }
    for (const auto& item : fixture.snapshot().state.players[1].items) {
        if (item.model == 187) { resource_token = item.token; got_mp_counter = true; }
        if (item.model == 142) { armor_token = item.token; got_armor = true; }
    }
    check(got_attack && got_mp_counter && got_armor, "public reset Neptune fixture");
    gfo_match* resource_match = nullptr; gfo_match* resource_replay = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, 152, &resource_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 152, &resource_replay) == 0, "resource counter create");
    attack.items[0] = weapon_token;
    check(gfo_match_step(resource_match, &attack, &free_result) == 0
        && gfo_match_step(resource_replay, &attack, &replay_result) == 0 && equal(free_result, replay_result), "resource counter incoming attack");
    gfo_match_command resource_defense{}; resource_defense.actor = 77; resource_defense.kind = 1; resource_defense.item_count = 1; resource_defense.items[0] = resource_token;
    check(gfo_match_step(resource_match, &resource_defense, &free_result) == 0
        && gfo_match_step(resource_replay, &resource_defense, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 1 && free_result.actor == 77 && free_result.owner == 0, "Neptune has explicit self response");
    const auto resource_view = observe(resource_match, 1, 77);
    check(resource_view.find("\"resource_counter_rule\":\"experimental:mp-self-target-with-response;cp-target-incoming-source;debit-current-target-then-full-credit-current-source;resource-debt-no-new-counter\"") != std::string::npos
        && resource_view.find("\"origin\":77,\"source\":77,\"target\":77") != std::string::npos
        && resource_view.find("\"power\":0") != std::string::npos && resource_view.find("\"amount\":2") != std::string::npos
        && resource_view.find("\"guardian\":\"neptune\"") == std::string::npos, "resource payload carries amount without summoning metadata guardian");
    auto invalid_resource_defense = resource_defense; invalid_resource_defense.items[0] = armor_token; result = free_result;
    check(gfo_match_step(resource_match, &invalid_resource_defense, &result) == -2 && equal(result, free_result)
        && observe(resource_match, 1, 77) == resource_view, "ordinary DEF cannot answer non-damage resource counter");
    for (uint64_t policy_seed = 0; policy_seed < 64; ++policy_seed) {
        uint64_t policy = policy_seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(resource_match, &policy, &candidate) == 0
            && gfo_match_legal(resource_match, &candidate, &legal) == 0 && legal, "resource-response samples legal");
    }
    accept.actor = 77;
    check(gfo_match_step(resource_match, &accept, &free_result) == 0 && gfo_match_step(resource_replay, &accept, &replay_result) == 0
        && equal(free_result, replay_result) && free_result.phase == 0 && free_result.owner == 77
        && observe(resource_match, 1, 77) == observe(resource_replay, 1, 77)
        && observe(resource_match).find("\"hp\":39,\"mp\":2,\"cp\":0") != std::string::npos, "Neptune applies once; policy queries leave future gifts unchanged");
    gfo_match_destroy(resource_match); gfo_match_destroy(resource_replay);
    fixture.reset(setup, 14243); got_attack = false; bool got_cp_counter = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 32) { weapon_token = item.token; got_attack = true; }
    for (const auto& item : fixture.snapshot().state.players[1].items) if (item.model == 188) { resource_token = item.token; got_cp_counter = true; }
    check(got_attack && got_cp_counter && gfo_match_create(&cfg, players.data(), 2, 14243, &resource_match) == 0, "public reset Venus fixture");
    attack.items[0] = weapon_token; resource_defense.items[0] = resource_token;
    check(gfo_match_step(resource_match, &attack, &result) == 0 && gfo_match_step(resource_match, &resource_defense, &result) == 0
        && result.phase == 1 && result.actor == 0 && result.owner == 0, "Venus targets incoming source");
    const auto cp_view = observe(resource_match, 0, 0);
    check(cp_view.find("\"origin\":77,\"source\":77,\"target\":0") != std::string::npos
        && cp_view.find("\"amount\":9") != std::string::npos && cp_view.find("\"power\":0") != std::string::npos,
        "Venus uses actual nine HP loss as nominal CP amount");
    accept.actor = 0;
    check(gfo_match_step(resource_match, &accept, &result) == 0 && result.phase == 0
        && observe(resource_match).find("\"hp\":31,\"mp\":0,\"cp\":9") != std::string::npos
        && observe(resource_match).find("\"hp\":31,\"mp\":0,\"cp\":0") != std::string::npos,
        "CP debt falls through MP and HP while creditor receives nominal CP");
    gfo_match_destroy(resource_match);
    fixture.reset(setup, 3409); got_attack = false; got_cp_counter = false; bool got_any_reflector = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) {
        if (item.model == 6) { weapon_token = item.token; got_attack = true; }
        if (item.model == 190) { mirror_token = item.token; got_any_reflector = true; }
    }
    for (const auto& item : fixture.snapshot().state.players[1].items) if (item.model == 188) { resource_token = item.token; got_cp_counter = true; }
    check(got_attack && got_cp_counter && got_any_reflector && gfo_match_create(&cfg, players.data(), 2, 3409, &resource_match) == 0, "public reset Venus reflection fixture");
    attack.items[0] = weapon_token; resource_defense.items[0] = resource_token;
    check(gfo_match_step(resource_match, &attack, &result) == 0 && gfo_match_step(resource_match, &resource_defense, &result) == 0, "CP reflection incoming");
    gfo_match_command resource_mirror{}; resource_mirror.actor = 0; resource_mirror.kind = 1; resource_mirror.item_count = 1; resource_mirror.items[0] = mirror_token;
    check(gfo_match_step(resource_match, &resource_mirror, &result) == 0 && result.phase == 1 && result.actor == 77
        && observe(resource_match, 0, 0).find("\"origin\":77,\"source\":0,\"target\":77") != std::string::npos,
        "universal reflection exchanges current creditor and debtor");
    accept.actor = 77;
    check(gfo_match_step(resource_match, &accept, &result) == 0
        && observe(resource_match).find("\"hp\":40,\"mp\":0,\"cp\":1") != std::string::npos
        && observe(resource_match).find("\"hp\":38,\"mp\":0,\"cp\":0") != std::string::npos,
        "reflected CP payment follows current endpoints");
    gfo_match_destroy(resource_match);
    // Source/target aliasing does not cancel debt or resurrect an HP0 creditor.
    fixture.reset(setup, 14243); got_attack = false; got_cp_counter = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) {
        if (item.model == 6) { weapon_token = item.token; got_attack = true; }
        if (item.model == 188) { resource_token = item.token; got_cp_counter = true; }
    }
    cfg.initial_hp = 2;
    check(got_attack && got_cp_counter && gfo_match_create(&cfg, players.data(), 2, 14243, &resource_match) == 0, "public reset alias/death fixture");
    attack.items[0] = weapon_token; attack.target = 0; resource_defense.actor = 0; resource_defense.items[0] = resource_token;
    check(gfo_match_step(resource_match, &attack, &result) == 0 && gfo_match_step(resource_match, &resource_defense, &result) == 0
        && result.phase == 1 && result.actor == 0, "self attack emits aliased CP response while source alive");
    accept.actor = 0;
    check(gfo_match_step(resource_match, &accept, &result) == 0 && result.terminated && result.winner_count == 1 && result.winners[0] == 77
        && result.rewards[0] == -1 && result.rewards[1] == 1
        && observe(resource_match).find("\"hp\":0,\"mp\":0,\"cp\":1") != std::string::npos,
        "debit then credit preserves aliased HP0 death and fixed-seat rewards");
    gfo_match_destroy(resource_match);
    // Damage effects use current source, full raw remainder and ordered clamps.
    const auto receipt_token = [&](std::size_t seat, official::CardId model) {
        for (const auto& item : fixture.snapshot().state.players[seat].items) if (item.model == model) return item.token;
        check(false, "damage-effect public reset fixture model missing"); return uint64_t{0};
    };
    cfg.initial_hp = 3; cfg.initial_mp = 0; cfg.initial_cp = 0;
    fixture.reset(setup, 352); weapon_token = receipt_token(0, 27);
    gfo_match* effect_match = nullptr; gfo_match* effect_replay = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, 352, &effect_match) == 0, "raw absorption fixture create");
    gfo_match_command effect_attack{}; effect_attack.has_target = 1; effect_attack.target = 77; effect_attack.item_count = 1; effect_attack.items[0] = weapon_token;
    gfo_match_command effect_pass{}; effect_pass.actor = 77; effect_pass.kind = 1;
    check(gfo_match_step(effect_match, &effect_attack, &result) == 0 && gfo_match_step(effect_match, &effect_pass, &result) == 0
        && result.terminated && result.winners[0] == 0 && result.rewards[0] == 1 && result.rewards[1] == -1,
        "raw absorption completes natural terminal");
    const auto raw_absorb = observe(effect_match);
    check(raw_absorb.find("\"damage_effect_rule\":\"experimental:positive-nondark-remainder;current-source-side-effect-before-target-damage;counter-basis-target-damage-only\"") != std::string::npos
        && raw_absorb.find("\"hp\":10,\"mp\":0") != std::string::npos && raw_absorb.find("\"hp\":0,\"mp\":0") != std::string::npos,
        "source gains raw7 even when target can lose only3");
    gfo_match_destroy(effect_match);
    cfg.initial_hp = 98; effect_attack.target = 0; effect_pass.actor = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 352, &effect_match) == 0
        && gfo_match_step(effect_match, &effect_attack, &result) == 0 && gfo_match_step(effect_match, &effect_pass, &result) == 0
        && observe(effect_match).find("\"hp\":92,\"mp\":0") != std::string::npos,
        "source-target alias heals to99 then takes raw7");
    gfo_match_destroy(effect_match);
    cfg.initial_hp = 40; effect_attack.target = 77; effect_pass.actor = 77;
    check(gfo_match_create(&cfg, players.data(), 2, 352, &effect_match) == 0, "zero-remainder absorption create");
    gfo_match_command effect_defense{}; effect_defense.actor = 77; effect_defense.kind = 1; effect_defense.item_count = 1;
    effect_defense.items[0] = receipt_token(1, 176);
    check(gfo_match_step(effect_match, &effect_attack, &result) == 0 && gfo_match_step(effect_match, &effect_defense, &result) == 0
        && observe(effect_match).find("\"hp\":47") == std::string::npos && result.phase == 0, "zero remainder does not heal source");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 335); effect_attack.items[0] = receipt_token(0, 27); effect_attack.target = 0;
    effect_defense.actor = 0; effect_defense.items[0] = receipt_token(0, 184); effect_pass.actor = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 335, &effect_match) == 0
        && gfo_match_step(effect_match, &effect_attack, &result) == 0 && gfo_match_step(effect_match, &effect_defense, &result) == 0
        && result.phase == 1 && result.actor == 0, "aliased absorption creates target-stage counter despite net HP0 change");
    const auto absorption_counter = observe(effect_match, 0, 0);
    check(absorption_counter.find("\"power\":14") != std::string::npos && absorption_counter.find("\"hp\":40,\"mp\":0") != std::string::npos,
        "counter basis is target-stage7 rather than net0");
    check(gfo_match_step(effect_match, &effect_pass, &result) == 0 && observe(effect_match).find("\"hp\":26,\"mp\":0") != std::string::npos,
        "counter does not inherit absorption ability");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 2091); effect_attack.target = 77; effect_attack.item_count = 2;
    effect_attack.items[0] = receipt_token(0, 216); effect_attack.items[1] = receipt_token(0, 204);
    effect_defense.actor = 77; effect_defense.items[0] = receipt_token(1, 39);
    check(gfo_match_create(&cfg, players.data(), 2, 2091, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 2091, &effect_replay) == 0, "free absorption miracle create");
    const auto before_effect_samples = observe(effect_match);
    for (uint64_t policy_seed = 0; policy_seed < 64; ++policy_seed) {
        uint64_t policy = policy_seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "damage-effect sampler stays legal");
    }
    check(observe(effect_match) == before_effect_samples && gfo_match_step(effect_match, &effect_attack, &free_result) == 0
        && gfo_match_step(effect_replay, &effect_attack, &replay_result) == 0 && equal(free_result, replay_result)
        && gfo_match_step(effect_match, &effect_defense, &free_result) == 0
        && gfo_match_step(effect_replay, &effect_defense, &replay_result) == 0 && equal(free_result, replay_result), "FREE miracle and redirect are transactional before body");
    const auto unresolved_absorb = observe(effect_match, 0, 0);
    check(unresolved_absorb.find("\"origin\":0,\"source\":77,\"target\":0") != std::string::npos
        && unresolved_absorb.find("\"hp\":50") == std::string::npos, "redirect does not apply absorption yet");
    check(gfo_match_step(effect_match, &effect_pass, &free_result) == 0 && gfo_match_step(effect_replay, &effect_pass, &replay_result) == 0
        && equal(free_result, replay_result) && observe(effect_match) == observe(effect_replay), "effect queries preserve future gift RNG");
    const auto absorbed_mc = observe(effect_match);
    check(absorbed_mc.find("\"hp\":30,\"mp\":0") != std::string::npos && absorbed_mc.find("\"hp\":50,\"mp\":0") != std::string::npos
        && absorbed_mc.find("\"token\":" + std::to_string(effect_attack.items[0]) + ",\"face\":216,\"reference\":216,\"disguised\":false,\"used\":true") != std::string::npos
        && absorbed_mc.find("\"token\":" + std::to_string(effect_attack.items[1]) + ",") == std::string::npos,
        "miracle heals reflected current source, retains original token and consumes FREE once");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 7); effect_attack.item_count = 1; effect_attack.items[1] = 0; effect_attack.has_target = 0; effect_attack.target = 0;
    effect_attack.items[0] = receipt_token(0, 98); effect_pass.actor = 77;
    check(gfo_match_create(&cfg, players.data(), 2, 7, &effect_match) == 0
        && gfo_match_step(effect_match, &effect_attack, &result) == 0 && result.phase == 1
        && gfo_match_step(effect_match, &effect_pass, &result) == 0 && observe(effect_match).find("\"hp\":43,\"mp\":0") != std::string::npos
        && observe(effect_match).find("\"hp\":37,\"mp\":0") != std::string::npos, "Wood75% absorption uses targetless hit pipeline");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 32); effect_attack.items[0] = receipt_token(0, 98);
    check(gfo_match_create(&cfg, players.data(), 2, 32, &effect_match) == 0
        && gfo_match_step(effect_match, &effect_attack, &result) == 0 && result.phase == 0 && result.decisions == 1
        && observe(effect_match).find("\"hp\":43") == std::string::npos, "missed chance absorption has no source effect");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 1776); effect_attack.has_target = 1; effect_attack.target = 0; effect_attack.items[0] = receipt_token(0, 53);
    effect_defense.actor = 0; effect_defense.items[0] = receipt_token(0, 184); effect_pass.actor = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 1776, &effect_match) == 0
        && gfo_match_step(effect_match, &effect_attack, &result) == 0 && gfo_match_step(effect_match, &effect_defense, &result) == 0
        && result.phase == 1 && observe(effect_match).find("\"hp\":12,\"mp\":0") != std::string::npos
        && observe(effect_match).find("\"power\":28") != std::string::npos,
        "self damage subtracts raw14 twice, but counter doubles only target-stage14");
    check(gfo_match_step(effect_match, &effect_pass, &result) == 0 && result.terminated && result.winner_count == 1 && result.winners[0] == 77,
        "derived counter has no inherited self-damage side effect");
    gfo_match_destroy(effect_match);
    // Flash restrictions and curing cross the opaque ABI, including atomic failure.
    fixture.reset(setup, 146990); cfg.initial_hp = 40; cfg.initial_mp = cfg.initial_cp = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 146990, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 146990, &effect_replay) == 0, "Flash public reset fixture create");
    gfo_match_command flash_attack{}; flash_attack.item_count = 1; flash_attack.items[0] = receipt_token(0, 70); flash_attack.has_target = 1;
    gfo_match_command flash_pass{}; flash_pass.kind = 1;
    const auto both_step = [&](const gfo_match_command& command) {
        check(gfo_match_step(effect_match, &command, &free_result) == 0
            && gfo_match_step(effect_replay, &command, &replay_result) == 0 && equal(free_result, replay_result), "curse replay command/status");
    };
    both_step(flash_attack); both_step(flash_pass);
    check(observe(effect_match).find("\"curses\":[\"flash\"]") != std::string::npos
        && observe(effect_match).find("\"flash_rule\":") != std::string::npos, "Flash is visible through C ABI");
    flash_attack.actor = 77; flash_attack.items[0] = receipt_token(1, 6); both_step(flash_attack);
    gfo_match_command two_defenses{}; two_defenses.kind = 1;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (two_defenses.item_count < 2 && item.model < 181) {
        const auto* card = official::find_card(item.model);
        if (card->category == official::Category::armor && !card->element && card->ability == official::Ability::none && card->defense > 0)
            two_defenses.items[two_defenses.item_count++] = item.token;
    }
    check(two_defenses.item_count == 2, "public Flash fixture has two ordinary defenses");
    const auto flashed_view = observe(effect_match); result = status(effect_match); const auto flashed_status = result;
    check(gfo_match_legal(effect_match, &two_defenses, &legal) == 0 && !legal
        && gfo_match_step(effect_match, &two_defenses, &result) == -2 && equal(result, flashed_status)
        && observe(effect_match) == flashed_view, "Flash rejects two-card response without changing output/state");
    for (uint64_t seed = 0; seed < 64; ++seed) {
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0 && candidate.item_count <= 1
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "Flash sampler respects single-card response");
    }
    both_step(flash_pass);
    gfo_match_command cure{}; cure.item_count = 2; cure.items[0] = receipt_token(0, 237); cure.items[1] = receipt_token(0, 204); cure.has_target = 1;
    both_step(cure);
    check(observe(effect_match) == observe(effect_replay) && observe(effect_match).find("\"curses\":[\"flash\"]") == std::string::npos
        && observe(effect_match).find("\"hp\":37,\"mp\":0") != std::string::npos,
        "main miracle plus FREE cures Flash while preserving gift RNG after failed response and sampling");
    const auto held_cure = "\"token\":" + std::to_string(cure.items[0]) + ",\"face\":237,\"reference\":237,\"disguised\":false,\"used\":true";
    check(observe(effect_match).find(held_cure) != std::string::npos, "cure retains original miracle token");
    flash_attack.items[0] = receipt_token(1, 21); both_step(flash_attack);
    check(gfo_match_legal(effect_match, &two_defenses, &legal) == 0 && legal, "curing Flash restores multi-card response legality");
    both_step(two_defenses); check(observe(effect_match) == observe(effect_replay), "post-cure replay remains deterministic");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 349070);
    check(gfo_match_create(&cfg, players.data(), 2, 349070, &effect_match) == 0, "Flash counter create");
    flash_attack.actor = 0; flash_attack.target = 77; flash_attack.items[0] = receipt_token(0, 6);
    gfo_match_command flash_defense{}; flash_defense.actor = 77; flash_defense.kind = 1; flash_defense.item_count = 1; flash_defense.items[0] = receipt_token(1, 185);
    check(gfo_match_step(effect_match, &flash_attack, &result) == 0 && gfo_match_step(effect_match, &flash_defense, &result) == 0
        && observe(effect_match).find("\"models\":[185]") != std::string::npos && observe(effect_match).find("\"power\":0") != std::string::npos,
        "Flash counter is a nonattack response payload");
    flash_pass.item_count = 1; flash_pass.items[0] = receipt_token(0, 190);
    check(gfo_match_step(effect_match, &flash_pass, &result) == 0, "universal reflection accepts Flash counter");
    flash_pass.actor = 77; flash_pass.item_count = 0; flash_pass.items[0] = 0;
    check(gfo_match_step(effect_match, &flash_pass, &result) == 0
        && observe(effect_match, 1, 77).find("\"hp\":39,\"mp\":0,\"cp\":0,\"curses\":[\"flash\"]") != std::string::npos,
        "reflected Flash counter applies to current target without HP damage");
    gfo_match_destroy(effect_match);
    // DarkCloud is a supported curse; forced percentage hits remain public,
    // while all lifetime counters and future action state remain private.
    for (auto id : {41, 186, 223}) check(gfo_match_supported(id), "DarkCloud model membership");
    cfg.initial_hp = 40; cfg.initial_mp = 5; cfg.initial_cp = 0;
    fixture.reset(setup, 4630);
    check(gfo_match_create(&cfg, players.data(), 2, 4630, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 4630, &effect_replay) == 0, "forced chance public reset fixture create");
    gfo_match_command cloud_cast{}; cloud_cast.item_count = 1; cloud_cast.items[0] = receipt_token(0, 223); cloud_cast.has_target = 1; cloud_cast.target = 77;
    gfo_match_command cloud_pass{}; cloud_pass.actor = 77; cloud_pass.kind = 1;
    both_step(cloud_cast); both_step(cloud_pass);
    check(observe(effect_match).find("\"curses\":[\"darkcloud\"]") != std::string::npos, "foreign miracle adds DarkCloud");
    const auto yield_other = [&](uint64_t reserved) {
        for (const auto& item : fixture.snapshot().state.players[1].items) {
            if (item.token == reserved) continue;
            gfo_match_command discard{}; discard.actor = 77; discard.kind = 2; discard.item_count = 1; discard.items[0] = item.token;
            uint8_t allowed = 0;
            check(gfo_match_legal(effect_match, &discard, &allowed) == 0, "public discard legality");
            if (allowed) { both_step(discard); return; }
        }
        check(false, "public fixture needs an unreserved discard");
    };
    mirror_token = receipt_token(1, 190); yield_other(mirror_token);
    const auto pre_forced = observe(effect_match);
    for (uint64_t seed = 0; seed < 64; ++seed) {
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "DarkCloud context sampler stays legal");
    }
    check(observe(effect_match) == pre_forced, "DarkCloud policy queries do not mutate match");
    gfo_match_command forced_attack{}; forced_attack.item_count = 1; forced_attack.items[0] = receipt_token(0, 95);
    both_step(forced_attack);
    const auto forced_view = observe(effect_match, 0, 0);
    check(free_result.actor == 77 && forced_view.find("\"forced_hit\":true") != std::string::npos
        && forced_view.find("\"hit_rate\":75") != std::string::npos
        && forced_view.find("\"darkcloud_rule\":\"experimental:force-percent-hit-without-draw-at-target-activation;redirects-no-recheck;idempotent-until-all-cure;no-tick\"") != std::string::npos,
        "DarkCloud marks forced chance activation without changing original percentage");
    for (const auto* key : {"\"chance_forced_hits\":", "\"counter_forced_hits\":", "\"chance_hits\":", "\"counter_hits\":", "\"chance_rolls\":", "\"counter_rolls\":", "\"batch\":", "\"counters\":"})
        check(forced_view.find(key) == std::string::npos, "forced counters and future reservations remain private");
    gfo_match_command cloud_mirror{}; cloud_mirror.actor = 77; cloud_mirror.kind = 1; cloud_mirror.item_count = 1; cloud_mirror.items[0] = mirror_token;
    both_step(cloud_mirror);
    check(free_result.actor == 0 && observe(effect_match).find("\"forced_hit\":true") != std::string::npos,
        "forced-hit evidence survives redirect onto a non-DarkCloud target");
    cloud_pass.actor = 0; both_step(cloud_pass);
    check(observe(effect_match) == observe(effect_replay) && observe(effect_match).find("\"hp\":39,\"mp\":0") != std::string::npos,
        "forced-hit redirect resolves once and preserves private gift RNG under queries");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 307);
    check(gfo_match_create(&cfg, players.data(), 2, 307, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 307, &effect_replay) == 0, "forced Mars fixture create");
    cloud_cast.items[0] = receipt_token(0, 223); cloud_cast.target = 0; both_step(cloud_cast);
    yield_other(receipt_token(1, 181));
    cloud_cast.items[0] = receipt_token(0, 6); cloud_cast.target = 77; both_step(cloud_cast);
    gfo_match_command cloud_counter{}; cloud_counter.actor = 77; cloud_counter.kind = 1; cloud_counter.item_count = 1; cloud_counter.items[0] = receipt_token(1, 181);
    both_step(cloud_counter);
    const auto forced_counter = observe(effect_match, 0, 0);
    check(forced_counter.find("\"models\":[181]") != std::string::npos && forced_counter.find("\"forced_hit\":true") != std::string::npos
        && forced_counter.find("\"hit_rate\":75") != std::string::npos && forced_counter.find("\"counter\":true") != std::string::npos,
        "Mars activation against DarkCloud is a forced counter hit");
    cloud_pass.actor = 0; both_step(cloud_pass);
    check(observe(effect_match) == observe(effect_replay), "forced counter future gift replay");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    cfg.initial_mp = 0;
    for (const auto reaction_model : {39, 55}) {
        fixture.reset(setup, 351815);
        check(gfo_match_create(&cfg, players.data(), 2, 351815, &effect_match) == 0, "FREE DarkCloud miracle fixture create");
        cloud_cast.items[0] = receipt_token(0, 223); cloud_cast.item_count = 2; cloud_cast.items[1] = receipt_token(0, 204); cloud_cast.target = 77;
        cloud_mirror.items[0] = receipt_token(1, static_cast<official::CardId>(reaction_model));
        check(gfo_match_step(effect_match, &cloud_cast, &result) == 0 && gfo_match_step(effect_match, &cloud_mirror, &result) == 0, "FREE DarkCloud accepts matching mirror or block");
        if (reaction_model == 39) check(gfo_match_step(effect_match, &cloud_pass, &result) == 0, "reflected DarkCloud resolves at current target");
        const auto cloud_body = observe(effect_match);
        check((cloud_body.find("\"curses\":[\"darkcloud\"]") != std::string::npos) == (reaction_model == 39),
            "block cancels curse; reflect returns it");
        check(cloud_body.find("\"token\":" + std::to_string(cloud_cast.items[0]) + ",\"face\":223,\"reference\":223,\"disguised\":false,\"used\":true") != std::string::npos
            && cloud_body.find("\"token\":" + std::to_string(cloud_cast.items[1]) + ",") == std::string::npos,
            "curse miracle retention and FREE consumption are not rolled back by reaction");
        gfo_match_destroy(effect_match);
    }
    fixture.reset(setup, 2475);
    check(gfo_match_create(&cfg, players.data(), 2, 2475, &effect_match) == 0, "DarkCloud counter fixture create");
    cloud_cast.item_count = 1; cloud_cast.items[1] = 0; cloud_cast.items[0] = receipt_token(0, 6);
    cloud_counter.items[0] = receipt_token(1, 186);
    check(gfo_match_step(effect_match, &cloud_cast, &result) == 0 && gfo_match_step(effect_match, &cloud_counter, &result) == 0
        && result.actor == 0 && observe(effect_match).find("\"models\":[186]") != std::string::npos
        && observe(effect_match).find("\"power\":0") != std::string::npos, "DarkCloud counter is a non-damage response payload");
    gfo_match_command forbidden_cloud_defense{}; forbidden_cloud_defense.kind = 1; forbidden_cloud_defense.item_count = 1; forbidden_cloud_defense.items[0] = receipt_token(0, 161);
    const auto before_cloud_error = observe(effect_match); const auto cloud_status = result;
    check(gfo_match_step(effect_match, &forbidden_cloud_defense, &result) == -2 && equal(result, cloud_status)
        && observe(effect_match) == before_cloud_error, "ordinary DEF cannot answer DarkCloud counter and failure is atomic");
    cloud_mirror.actor = 0; cloud_mirror.items[0] = receipt_token(0, 190);
    cloud_pass.actor = 77;
    check(gfo_match_step(effect_match, &cloud_mirror, &result) == 0 && gfo_match_step(effect_match, &cloud_pass, &result) == 0
        && observe(effect_match, 1, 77).find("\"hp\":39,\"mp\":0,\"cp\":0,\"curses\":[\"darkcloud\"]") != std::string::npos,
        "universal mirror returns DarkCloud counter to its generator");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 10); cfg.initial_hp = 5;
    check(gfo_match_create(&cfg, players.data(), 2, 10, &effect_match) == 0, "lethal DarkCloud weapon fixture");
    cloud_cast.items[0] = receipt_token(0, 41); cloud_pass.actor = 77;
    check(gfo_match_step(effect_match, &cloud_cast, &result) == 0 && gfo_match_step(effect_match, &cloud_pass, &result) == 0
        && result.terminated && result.winners[0] == 0
        && observe(effect_match, 1, 77).find("\"curses\":[\"darkcloud\"]") != std::string::npos,
        "positive lethal target HP loss also records DarkCloud");
    gfo_match_destroy(effect_match);
    // Removal observes only completed own-hand changes, never foreign victims.
    for (auto id : {205, 206}) check(gfo_match_supported(id), "removal membership");
    const auto hand_size = [&](gfo_match* match, uint64_t player) {
        const auto view = observe(match, 1, player); std::size_t count = 0, at = 0;
        while ((at = view.find("\"token\":", at)) != std::string::npos) { ++count; ++at; }
        return count;
    };
    const auto has_token = [&](gfo_match* match, uint64_t player, uint64_t token) {
        return observe(match, 1, player).find("\"token\":" + std::to_string(token) + ",") != std::string::npos;
    };
    cfg.initial_hp = 40; cfg.initial_mp = 99; cfg.initial_hand = 18;
    fixture.reset(setup, 56);
    check(gfo_match_create(&cfg, players.data(), 2, 56, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 56, &effect_replay) == 0, "Broom public reset fixture create");
    gfo_match_command remove_command{}; remove_command.item_count = 1; remove_command.items[0] = receipt_token(0, 205); remove_command.has_target = 1; remove_command.target = 77;
    gfo_match_command remove_pass{}; remove_pass.kind = 1; remove_pass.actor = 77;
    both_step(remove_command);
    const auto removal_view = observe(effect_match, 0, 0);
    check(removal_view.find("\"removal_rule\":\"experimental:unused-hand-max3;used-miracles-max2;uniform-without-replacement-in-inventory-order;bounded-draw-per-removal;empty-no-draw;no-removal-gifts\"") != std::string::npos
        && removal_view.find("\"power\":0") != std::string::npos && removal_view.find("\"amount\":3") != std::string::npos, "public removal scope and derived amount");
    for (uint64_t seed = 0; seed < 64; ++seed) {
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "removal-context sampled actions legal");
    }
    both_step(remove_pass);
    check(hand_size(effect_match, 0) == 18 && hand_size(effect_match, 77) == 15
        && observe(effect_match, 1, 0) == observe(effect_replay, 1, 0)
        && observe(effect_match, 1, 77) == observe(effect_replay, 1, 77), "three unused removals with only user gift; query and policy RNG separate");
    std::size_t removed_count = 0;
    for (const auto& item : fixture.snapshot().state.players[1].items) if (!has_token(effect_match, 77, item.token)) ++removed_count;
    check(removed_count == 3, "without-replacement removes three distinct physical copies");
    for (const auto* key : {"\"removed\":", "\"removed_models\":", "\"removed_slots\":", "\"removal_rng\":", "\"batch\":"})
        check(observe(effect_match, 0, 0).find(key) == std::string::npos, "removal adds no foreign victim or sampling-state observation");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 1600377);
    check(gfo_match_create(&cfg, players.data(), 2, 1600377, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 1600377, &effect_replay) == 0, "Soap three-retained fixture create");
    const std::array<uint64_t, 3> learned_tokens{receipt_token(0, 235), receipt_token(0, 237), receipt_token(0, 238)};
    gfo_match_command learn_command{}; learn_command.item_count = 1; learn_command.has_target = 1;
    for (const auto token : learned_tokens) {
        learn_command.items[0] = token; both_step(learn_command); yield_other(std::numeric_limits<uint64_t>::max());
    }
    remove_command.items[0] = receipt_token(0, 206); remove_command.target = 0;
    const auto soap_before = observe(effect_match);
    for (uint64_t seed = 0; seed < 64; ++seed) {
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "retained-removal context samples legal");
    }
    check(observe(effect_match) == soap_before, "Soap query leaves candidate inventory unchanged");
    both_step(remove_command);
    check(hand_size(effect_match, 0) == 16 && observe(effect_match) == observe(effect_replay), "two retained removals grant no automatic replacement");
    removed_count = 0; uint64_t removed_miracle = 0;
    for (const auto token : learned_tokens) if (!has_token(effect_match, 0, token)) { ++removed_count; removed_miracle = token; }
    check(removed_count == 2, "Soap stops at two out of three used miracles");
    yield_other(std::numeric_limits<uint64_t>::max());
    learn_command.items[0] = removed_miracle;
    const auto soap_after = observe(effect_match); result = status(effect_match); const auto soap_status = result;
    check(gfo_match_legal(effect_match, &learn_command, &legal) == 0 && !legal
        && gfo_match_step(effect_match, &learn_command, &result) == -2 && equal(result, soap_status)
        && observe(effect_match) == soap_after, "deleted retained token cannot be cast and failure is atomic");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 29519);
    check(gfo_match_create(&cfg, players.data(), 2, 29519, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 29519, &effect_replay) == 0, "Soap duplicate-model copies fixture create");
    std::vector<uint64_t> duplicate_springs;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 235) duplicate_springs.push_back(item.token);
    check(duplicate_springs.size() == 2 && duplicate_springs[0] != duplicate_springs[1], "two physical copies share a model but not a token");
    for (const auto token : duplicate_springs) {
        learn_command.items[0] = token; both_step(learn_command); yield_other(std::numeric_limits<uint64_t>::max());
    }
    remove_command.items[0] = receipt_token(0, 206); both_step(remove_command);
    check(hand_size(effect_match, 0) == 16 && !has_token(effect_match, 0, duplicate_springs[0]) && !has_token(effect_match, 0, duplicate_springs[1])
        && observe(effect_match).find("\"used\":true") == std::string::npos, "Soap removes copies rather than only one model type");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 13862);
    check(gfo_match_create(&cfg, players.data(), 2, 13862, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 13862, &effect_replay) == 0, "reflected Soap fixture create");
    learn_command.items[0] = receipt_token(0, 235); both_step(learn_command);
    const auto soap_mirror = receipt_token(1, 190); yield_other(soap_mirror);
    remove_command.items[0] = receipt_token(0, 206); remove_command.target = 77; both_step(remove_command);
    gfo_match_command removal_mirror{}; removal_mirror.actor = 77; removal_mirror.kind = 1; removal_mirror.item_count = 1; removal_mirror.items[0] = soap_mirror;
    both_step(removal_mirror); remove_pass.actor = 0; both_step(remove_pass);
    check(!has_token(effect_match, 0, learn_command.items[0]) && hand_size(effect_match, 0) == 17 && hand_size(effect_match, 77) == 17
        && observe(effect_match) == observe(effect_replay), "universal reflection changes whose retained copy is removed");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    cfg.initial_hand = 1; cfg.initial_mp = 0;
    // With no retained copies, Soap ignores the recipient's unused miracle/carrier.
    for (const uint64_t target : {uint64_t{0}, uint64_t{77}}) {
        check(gfo_match_create(&cfg, players.data(), 2, 72, &effect_match) == 0, "empty Soap fixture create");
        remove_command.items[0] = 0; remove_command.target = target;
        check(gfo_match_step(effect_match, &remove_command, &result) == 0, "empty Soap accepted");
        if (target == 77) { remove_pass.actor = 77; check(gfo_match_step(effect_match, &remove_pass, &result) == 0, "empty foreign Soap passes"); }
        check(hand_size(effect_match, 0) == 1 && hand_size(effect_match, 77) == 1
            && observe(effect_match).find("\"used\":true") == std::string::npos, "empty removal grants only physical use gift and no retained miracle");
        gfo_match_destroy(effect_match);
    }
    check(gfo_match_create(&cfg, players.data(), 2, 196830, &effect_match) == 0, "sole unused miracle Broom fixture create");
    remove_command.target = 77; remove_pass.actor = 77;
    check(observe(effect_match, 1, 77).find("\"face\":224") != std::string::npos
        && gfo_match_step(effect_match, &remove_command, &result) == 0 && gfo_match_step(effect_match, &remove_pass, &result) == 0
        && hand_size(effect_match, 0) == 1 && hand_size(effect_match, 77) == 0, "Broom removes sole unused miracle without learning or recipient compensation");
    gfo_match_destroy(effect_match);
    // MP-derived power is computed once from remaining MP, then redirected intact.
    for (auto id : {14, 58}) check(gfo_match_supported(id), "repeat and mana weapon membership");
    cfg.initial_hand = 18; cfg.initial_hp = 40; cfg.initial_mp = 0;
    fixture.reset(setup, 7);
    check(gfo_match_create(&cfg, players.data(), 2, 7, &effect_match) == 0, "zero-MP mana weapon create");
    gfo_match_command mana_attack{}; mana_attack.item_count = 1; mana_attack.has_target = 1; mana_attack.target = 77; mana_attack.items[0] = receipt_token(0, 58);
    gfo_match_command new_pass{}; new_pass.actor = 77; new_pass.kind = 1;
    check(gfo_match_step(effect_match, &mana_attack, &result) == 0 && result.phase == 1 && result.actor == 77
        && observe(effect_match).find("\"power\":0,") != std::string::npos
        && observe(effect_match).find("\"mana_weapon_rule\":\"experimental:base-2x-mp-after-composed-miracle-cost;ordered-boosts;consume-all-mp-once-at-launch;redirects-no-repayment\"") != std::string::npos,
        "MP0 remains a zero-power weapon response");
    check(gfo_match_step(effect_match, &new_pass, &result) == 0 && result.phase == 0, "zero-power weapon resolves");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 13057); cfg.initial_hp = 99; cfg.initial_mp = 10;
    const std::vector<std::pair<std::vector<official::CardId>, int>> mana_columns{
        {{58, 231}, 16}, {{58, 210}, 18}, {{58, 210, 231}, 12}, {{58, 231, 210}, 10}};
    for (const auto& [models, power] : mana_columns) {
        check(gfo_match_create(&cfg, players.data(), 2, 13057, &effect_match) == 0, "paid mana column create");
        mana_attack.item_count = static_cast<uint8_t>(models.size()); std::fill_n(mana_attack.items, 18, 0);
        for (std::size_t i = 0; i < models.size(); ++i) mana_attack.items[i] = receipt_token(0, models[i]);
        check(gfo_match_step(effect_match, &mana_attack, &result) == 0
            && observe(effect_match).find("\"power\":" + std::to_string(power) + ",") != std::string::npos
            && observe(effect_match).find("\"hp\":99,\"mp\":0") != std::string::npos, "completed miracle cost precedes dynamic base and ordered Aura");
        check(gfo_match_step(effect_match, &new_pass, &result) == 0 && hand_size(effect_match, 0) == 18, "paid mana column refills once");
        gfo_match_destroy(effect_match);
    }
    fixture.reset(setup, 76570);
    const std::vector<std::pair<std::vector<official::CardId>, int>> free_columns{
        {{58, 12}, 2}, {{58, 12, 231, 204}, 44}, {{58, 231, 204, 12}, 42}};
    for (std::size_t column = 0; column < free_columns.size(); ++column) {
        cfg.initial_mp = column == 0 ? 0 : 10;
        check(gfo_match_create(&cfg, players.data(), 2, 76570, &effect_match) == 0, "FREE mana column create");
        const auto& [models, power] = free_columns[column];
        mana_attack.item_count = static_cast<uint8_t>(models.size()); std::fill_n(mana_attack.items, 18, 0);
        for (std::size_t i = 0; i < models.size(); ++i) mana_attack.items[i] = receipt_token(0, models[i]);
        check(gfo_match_step(effect_match, &mana_attack, &result) == 0
            && observe(effect_match).find("\"power\":" + std::to_string(power) + ",") != std::string::npos, "zero-MP boost and FREE Aura preserve ordered power");
        check(gfo_match_step(effect_match, &new_pass, &result) == 0 && hand_size(effect_match, 0) == 18
            && !has_token(effect_match, 0, receipt_token(0, 58)), "mana weapon physical consumption and bounded refill");
        gfo_match_destroy(effect_match);
    }
    fixture.reset(setup, 7); cfg.initial_mp = 10; cfg.initial_hp = 40;
    check(gfo_match_create(&cfg, players.data(), 2, 7, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 7, &effect_replay) == 0, "mana redirect create");
    mana_attack.item_count = 1; std::fill_n(mana_attack.items, 18, 0); mana_attack.items[0] = receipt_token(0, 58);
    const auto before_mana_sample = observe(effect_match);
    for (uint64_t seed = 0; seed < 64; ++seed) {
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0 && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "mana context samples legal");
    }
    check(observe(effect_match) == before_mana_sample, "mana policy queries do not burn MP");
    both_step(mana_attack);
    gfo_match_command mana_mirror{}; mana_mirror.actor = 77; mana_mirror.kind = 1; mana_mirror.item_count = 1; mana_mirror.items[0] = receipt_token(1, 38); both_step(mana_mirror);
    check(observe(effect_match).find("\"origin\":0,\"source\":77,\"target\":0") != std::string::npos
        && observe(effect_match).find("\"power\":20,") != std::string::npos
        && observe(effect_match).find("\"hp\":40,\"mp\":10") != std::string::npos, "redirect carries immutable power without burning defender MP");
    new_pass.actor = 0; both_step(new_pass);
    check(observe(effect_match) == observe(effect_replay) && observe(effect_match).find("\"hp\":20,\"mp\":0") != std::string::npos,
        "mana redirects resolve once and policy queries preserve future gift RNG");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    // Two initial frames share a single use/payment, with counters ahead of hit2.
    fixture.reset(setup, 58); cfg.initial_mp = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 58, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 58, &effect_replay) == 0, "repeat/counter create");
    gfo_match_command repeat{}; repeat.item_count = 1; repeat.items[0] = receipt_token(0, 14); repeat.has_target = 1; repeat.target = 77;
    gfo_match_command repeat_counter{}; repeat_counter.actor = 77; repeat_counter.kind = 1; repeat_counter.item_count = 1; repeat_counter.items[0] = receipt_token(1, 184);
    both_step(repeat); both_step(repeat_counter);
    check(free_result.actor == 0 && observe(effect_match).find("\"counter\":true") != std::string::npos
        && observe(effect_match).find("\"power\":6,") != std::string::npos
        && observe(effect_match).find("\"repeat_attack_rule\":\"experimental:two-fresh-passes-over-initial-targets;single-consumption-and-payment;counter-fifo-before-next-hit;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action\"") != std::string::npos,
        "first repeat hit queues counter before second original payload");
    new_pass.actor = 0; both_step(new_pass);
    const auto second_repeat = observe(effect_match);
    check(free_result.actor == 77 && free_result.owner == 0 && second_repeat.find("\"models\":[14]") != std::string::npos
        && second_repeat.find("\"counter\":false") != std::string::npos && hand_size(effect_match, 0) == 17, "second payload opens before any deferred gift");
    new_pass.actor = 77; both_step(new_pass);
    check(free_result.decisions == 4 && free_result.actor == 77 && free_result.owner == 77
        && hand_size(effect_match, 0) == 18 && hand_size(effect_match, 77) == 18
        && observe(effect_match) == observe(effect_replay), "whole repeat action finishes and gifts only once");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 1862); cfg.initial_mp = 10;
    check(gfo_match_create(&cfg, players.data(), 2, 1862, &effect_match) == 0, "paid repeat create");
    repeat.item_count = 2; repeat.items[0] = receipt_token(0, 14); repeat.items[1] = receipt_token(0, 210);
    check(gfo_match_step(effect_match, &repeat, &result) == 0, "paid repeat launch");
    for (int hit = 0; hit < 2; ++hit) {
        check(observe(effect_match).find("\"power\":5,") != std::string::npos
            && observe(effect_match).find("\"hp\":40,\"mp\":8") != std::string::npos
            && gfo_match_step(effect_match, &new_pass, &result) == 0, "repeat charges additional miracle only at first launch");
    }
    check(result.decisions == 3 && hand_size(effect_match, 0) == 18 && has_token(effect_match, 0, repeat.items[1])
        && observe(effect_match).find("\"hp\":30,\"mp\":10") != std::string::npos, "two equal hits retain miracle token once");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 8665); cfg.initial_mp = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 8665, &effect_match) == 0, "repeat fresh-element create");
    repeat.items[0] = receipt_token(0, 14); repeat.items[1] = receipt_token(0, 66);
    repeat_counter.items[0] = receipt_token(1, 189);
    check(gfo_match_step(effect_match, &repeat, &result) == 0 && gfo_match_step(effect_match, &repeat_counter, &result) == 0
        && observe(effect_match).find("\"element\":\"fire\"") != std::string::npos
        && observe(effect_match).find("\"power\":5,") != std::string::npos,
        "second payload restores original element after first filter");
    check(gfo_match_step(effect_match, &new_pass, &result) == 0, "fresh second payload resolves");
    gfo_match_destroy(effect_match);
    for (const bool redirect_first : {false, true}) {
        const uint64_t seed = redirect_first ? 148 : 39; fixture.reset(setup, seed); cfg.initial_hp = 3;
        check(gfo_match_create(&cfg, players.data(), 2, seed, &effect_match) == 0, "repeat natural ending create");
        repeat.item_count = 1; repeat.items[0] = receipt_token(0, 14); repeat.items[1] = 0;
        check(gfo_match_step(effect_match, &repeat, &result) == 0, "repeat ending launch");
        if (redirect_first) {
            mana_mirror.items[0] = receipt_token(1, 38);
            check(gfo_match_step(effect_match, &mana_mirror, &result) == 0, "first repeat reflected toward original owner");
        }
        new_pass.actor = redirect_first ? 0 : 77;
        check(gfo_match_step(effect_match, &new_pass, &result) == 0 && result.terminated && result.winner_count == 1
            && result.winners[0] == (redirect_first ? 77 : 0) && result.decisions == (redirect_first ? 3 : 2),
            "natural death cancels second payload before another response");
        gfo_match_destroy(effect_match);
    }
    // Mirage keeps ABI1's optional target representation; a live hint is ignored.
    check(gfo_match_supported(232), "Mirage membership"); cfg.initial_hand = 18; cfg.initial_hp = 40; cfg.initial_mp = 5;
    fixture.reset(chance_setup, 104);
    std::vector<std::string> wide_baseline;
    for (int target_hint = -1; target_hint < 4; ++target_hint) {
        check(gfo_match_create(&cfg, chance_players.data(), 4, 104, &effect_match) == 0, "four-seat wide create");
        gfo_match_command wide{}; wide.item_count = 2; wide.items[0] = receipt_token(0, 6); wide.items[1] = receipt_token(0, 232);
        if (target_hint >= 0) { wide.has_target = 1; wide.target = chance_players[static_cast<std::size_t>(target_hint)].id; }
        auto invalid_wide = wide; invalid_wide.has_target = 1; invalid_wide.target = 9999; result = status(effect_match); const auto pre_wide_status = result;
        const auto pre_wide = observe(effect_match);
        check(gfo_match_legal(effect_match, &invalid_wide, &legal) == 0 && !legal
            && gfo_match_step(effect_match, &invalid_wide, &result) == -2 && equal(result, pre_wide_status)
            && observe(effect_match) == pre_wide, "unknown Wide hint rejected atomically");
        check(gfo_match_step(effect_match, &wide, &result) == 0, "Wide accepts absent/self/ally/enemy live hint");
        std::vector<std::string> trace{observe(effect_match)};
        check(trace.front().find("\"wide_attack_rule\":\"experimental:initial-alive-enemies-in-input-order;optional-alive-target-does-not-alter-roster;repeat-by-full-roster-pass;single-payment;counter-fifo-before-next-target;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action\"") != std::string::npos,
            "Wide public experimental contract");
        gfo_match_command wide_pass{}; wide_pass.kind = 1;
        for (const uint64_t actor : {uint64_t{88}, uint64_t{99}}) {
            check(result.phase == 1 && result.actor == actor && result.owner == 0 && hand_size(effect_match, 0) == 17
                && observe(effect_match).find("\"hit_rate\":0") != std::string::npos
                && observe(effect_match).find("\"forced_hit\":false") != std::string::npos, "guaranteed Wide follows initial enemy roster without chance draw");
            wide_pass.actor = actor; check(gfo_match_step(effect_match, &wide_pass, &result) == 0, "Wide target responds"); trace.push_back(observe(effect_match));
        }
        check(result.decisions == 3 && result.actor == 77 && result.owner == 77 && hand_size(effect_match, 0) == 18,
            "all Wide targets precede deferred user gift and next owner");
        if (wide_baseline.empty()) wide_baseline = trace;
        else check(trace == wide_baseline, "all live target hints produce identical roster, state and gift stream");
        gfo_match_destroy(effect_match);
    }
    fixture.reset(setup, 3162); cfg.initial_mp = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 3162, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 3162, &effect_replay) == 0, "zero-MP Mirage/FREE create");
    const auto wide_token = receipt_token(0, 232); const auto wide_free = receipt_token(0, 204);
    const auto before_wide_sample = observe(effect_match); bool sampled_wide_free = false;
    for (uint64_t seed = 0; seed < 4096 && !sampled_wide_free; ++seed) {
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "zero-MP Wide samples legal");
        bool has_wide = false, has_free = false;
        for (std::size_t i = 0; i < candidate.item_count; ++i) { has_wide |= candidate.items[i] == wide_token; has_free |= candidate.items[i] == wide_free; }
        if (has_wide) check(!candidate.has_target && candidate.target == 0, "sampler clears target after appending Mirage");
        sampled_wide_free = has_wide && has_free;
    }
    check(sampled_wide_free && observe(effect_match) == before_wide_sample, "sampler atomically appends Mirage/FREE without changing match");
    gfo_match_command wide_free_cast{}; wide_free_cast.item_count = 3; wide_free_cast.items[0] = receipt_token(0, 6); wide_free_cast.items[1] = wide_token; wide_free_cast.items[2] = wide_free;
    both_step(wide_free_cast); new_pass.actor = 77; both_step(new_pass);
    check(observe(effect_match) == observe(effect_replay) && hand_size(effect_match, 0) == 18
        && has_token(effect_match, 0, wide_token) && !has_token(effect_match, 0, wide_free), "FREE Wide retention, consumption and future gift RNG");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(chance_setup, 721); cfg.initial_mp = 5;
    check(gfo_match_create(&cfg, chance_players.data(), 4, 721, &effect_match) == 0, "repeat Wide create");
    gfo_match_command wide{}; wide.item_count = 2; wide.items[0] = receipt_token(0, 14); wide.items[1] = receipt_token(0, 232);
    check(gfo_match_step(effect_match, &wide, &result) == 0, "repeat Wide launch");
    for (const uint64_t actor : {uint64_t{88}, uint64_t{99}, uint64_t{88}, uint64_t{99}}) {
        check(result.actor == actor && result.owner == 0 && hand_size(effect_match, 0) == 17, "repeat traverses two full roster passes before gifting");
        new_pass.actor = actor; check(gfo_match_step(effect_match, &new_pass, &result) == 0, "repeat Wide response");
    }
    check(result.decisions == 5 && result.actor == 77 && hand_size(effect_match, 0) == 18, "repeat Wide finishes with one launch payment/consumption");
    gfo_match_destroy(effect_match);
    fixture.reset(chance_setup, 3717);
    check(gfo_match_create(&cfg, chance_players.data(), 4, 3717, &effect_match) == 0, "Wide counter-order create");
    wide.items[0] = receipt_token(0, 6); wide.items[1] = receipt_token(0, 232);
    gfo_match_command wide_counter{}; wide_counter.actor = 88; wide_counter.kind = 1; wide_counter.item_count = 1; wide_counter.items[0] = receipt_token(2, 184);
    check(gfo_match_step(effect_match, &wide, &result) == 0 && gfo_match_step(effect_match, &wide_counter, &result) == 0
        && result.actor == 0 && observe(effect_match).find("\"counter\":true") != std::string::npos
        && observe(effect_match).find("\"power\":2,") != std::string::npos, "counter FIFO starts before next Wide enemy");
    new_pass.actor = 0;
    check(gfo_match_step(effect_match, &new_pass, &result) == 0 && result.actor == 99, "next enemy follows resolved counter");
    new_pass.actor = 99; check(gfo_match_step(effect_match, &new_pass, &result) == 0 && result.actor == 77, "Wide counter batch finishes");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 337); cfg.initial_mp = 10;
    check(gfo_match_create(&cfg, players.data(), 2, 337, &effect_match) == 0, "mana Wide create");
    wide.items[0] = receipt_token(0, 58); wide.items[1] = receipt_token(0, 232);
    check(gfo_match_step(effect_match, &wide, &result) == 0
        && observe(effect_match).find("\"power\":10,") != std::string::npos
        && observe(effect_match).find("\"hp\":40,\"mp\":0") != std::string::npos, "Wide cost5 precedes MP-derived base10");
    new_pass.actor = 77; check(gfo_match_step(effect_match, &new_pass, &result) == 0, "mana Wide resolves");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 2656);
    check(gfo_match_create(&cfg, players.data(), 2, 2656, &effect_match) == 0, "two Mirage copies create");
    wide.item_count = 1; wide.items[0] = receipt_token(0, 6); std::vector<uint64_t> wide_copies;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 232) { wide.items[wide.item_count++] = item.token; wide_copies.push_back(item.token); }
    check(wide_copies.size() == 2 && gfo_match_step(effect_match, &wide, &result) == 0
        && observe(effect_match).find("\"hp\":40,\"mp\":0") != std::string::npos, "both Mirage copies pay5 and become retained");
    check(gfo_match_step(effect_match, &new_pass, &result) == 0 && result.decisions == 2
        && has_token(effect_match, 0, wide_copies[0]) && has_token(effect_match, 0, wide_copies[1]), "multiple Mirage copies expand roster only once");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 4630); cfg.initial_mp = 5;
    check(gfo_match_create(&cfg, players.data(), 2, 4630, &effect_match) == 0, "invalid Mirage compositions create");
    wide.item_count = 1; wide.items[0] = receipt_token(0, 232); wide.items[1] = 0; wide.items[2] = 0;
    check(gfo_match_legal(effect_match, &wide, &legal) == 0 && !legal, "Mirage has no standalone main cast");
    wide.item_count = 2; wide.items[0] = receipt_token(0, 95); wide.items[1] = receipt_token(0, 232);
    check(gfo_match_legal(effect_match, &wide, &legal) == 0 && !legal, "percentage weapon cannot append Mirage");
    gfo_match_destroy(effect_match);
    for (const bool setter_after : {false, true}) {
        fixture.reset(setup, 9332);
        check(gfo_match_create(&cfg, players.data(), 2, 9332, &effect_match) == 0, "ordered Mirage element create");
        wide.item_count = 3; wide.items[0] = receipt_token(0, 6); wide.items[1] = receipt_token(0, setter_after ? 232 : 66); wide.items[2] = receipt_token(0, setter_after ? 66 : 232);
        check(gfo_match_step(effect_match, &wide, &result) == 0
            && observe(effect_match).find(setter_after ? "\"element\":\"fire\"" : "\"element\":null") != std::string::npos,
            "Mirage neutralizes prior element while later setter restores its own");
        check(gfo_match_step(effect_match, &new_pass, &result) == 0, "ordered Wide element resolves"); gfo_match_destroy(effect_match);
    }
    gfo_match_destroy(blocked_match);
    gfo_match_destroy(reflected_match);
    gfo_match_destroy(short_match); gfo_match_destroy(a); gfo_match_destroy(b); gfo_match_destroy(nullptr);
    std::cout << "official match C API passed\n";
}
