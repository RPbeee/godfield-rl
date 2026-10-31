#include "godfield/official_match_c_api.h"
#include "godfield/official_match.hpp"
#include <array>
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
    check(gfo_match_catalog_count() == 164 && gfo_match_gift_weight() == 351 && gfo_match_supported(6)
        && !gfo_match_supported(112) && !gfo_match_supported(14), "supported subset descriptor");
    check(std::string(gfo_match_ruleset()) == "official-execution-subset-v5"
        && std::string(gfo_match_scope()) == "experimental:164-models;not-official-server-equivalence", "explicit v5 scope");
    for (auto id : {38, 39, 156, 167, 174, 189, 190}) check(gfo_match_supported(id), "added reflection/filter membership");
    for (auto id : {95, 181, 184}) check(!gfo_match_supported(id), "remaining chance/counter unsupported");
    for (auto id : {42, 50, 55, 56, 165, 171, 177, 179, 233}) check(gfo_match_supported(id), "added block membership");
    for (auto id : {46, 66, 83, 143, 163, 173, 204, 231}) check(gfo_match_supported(id), "added composition membership");
    for (auto id : {21, 34, 114, 125, 137, 151, 164, 234}) check(gfo_match_supported(id), "added bounce membership");
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
    fixture.reset(setup, 16564);
    bool got_spring = false, got_cast_free = false, got_bounce = false, got_bounce_free = false;
    uint64_t cast_free_token = 0, bounce_token = 0, bounce_free_token = 0;
    for (const auto& item : fixture.snapshot().state.players[0].items) {
        if (item.model == 235) { got_spring = true; spring_token = item.token; }
        if (item.model == 163) { got_cast_free = true; cast_free_token = item.token; }
    }
    for (const auto& item : fixture.snapshot().state.players[1].items) {
        if (item.model == 234) { got_bounce = true; bounce_token = item.token; }
        if (item.model == 46) { got_bounce_free = true; bounce_free_token = item.token; }
    }
    check(got_spring && got_cast_free && got_bounce && got_bounce_free, "public reset zero-MP bounce fixture");
    gfo_match* bounce_match = nullptr; gfo_match* bounce_replay = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, 16564, &bounce_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 16564, &bounce_replay) == 0, "bounce fixture create");
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
    check(bounced_view.find("\"schema\":\"official-execution-view-v3\"") != std::string::npos
        && bounced_view.find("\"bounce_target_rule\":\"experimental:uniform-all-alive-in-input-order;includes-self-and-teammates\"") != std::string::npos
        && bounced_view.find("\"origin\":0,\"source\":77,\"target\":" + std::to_string(free_result.actor)) != std::string::npos
        && bounced_view.find("\"bounces\":1") != std::string::npos && bounced_view.find("\"reflections\":0") != std::string::npos,
        "public bounce contract and separate redirect counters");
    check(bounced_view.find("redirect_rng") == std::string::npos && bounced_view.find("redirects") == std::string::npos
        && bounced_view.find("\"tokens\"") == std::string::npos, "private redirect replay evidence is not observed");
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
    gfo_match_destroy(blocked_match);
    gfo_match_destroy(reflected_match);
    gfo_match_destroy(short_match); gfo_match_destroy(a); gfo_match_destroy(b); gfo_match_destroy(nullptr);
    std::cout << "official match C API passed\n";
}
