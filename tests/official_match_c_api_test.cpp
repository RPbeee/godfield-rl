#include "godfield/official_match_c_api.h"
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
    check(gfo_match_catalog_count() == 132 && gfo_match_gift_weight() == 308 && gfo_match_supported(6)
        && !gfo_match_supported(112) && !gfo_match_supported(14), "supported subset descriptor");
    check(std::string(gfo_match_scope()).find("experimental:") == 0, "explicit scope");
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
    gfo_match_destroy(short_match); gfo_match_destroy(a); gfo_match_destroy(b); gfo_match_destroy(nullptr);
    std::cout << "official match C API passed\n";
}
