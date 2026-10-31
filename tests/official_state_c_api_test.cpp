#include "godfield/official_state_c_api.h"
#include <array>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
void check(bool ok, const char* text) { if (!ok) { std::cerr << "FAIL state C API: " << text << '\n'; std::exit(1); } }
std::string view(gfo_state* state, uint8_t present = 1, uint64_t viewer = 0) {
    size_t required = 999;
    check(gfo_state_view_json(state, present, viewer, nullptr, 0, &required) == 0 && required > 1, "query view length");
    std::vector<char> bytes(required + 2, 'Z');
    check(gfo_state_view_json(state, present, viewer, bytes.data() + 1, required, &required) == 0, "write UTF8 view");
    check(bytes.front() == 'Z' && bytes.back() == 'Z' && bytes[required] == '\0', "exact output boundary and NUL");
    return {bytes.data() + 1};
}
}
int main() {
    check(gfo_state_abi_version() == 1 && gfo_state_item_size() == sizeof(gfo_state_item)
        && gfo_state_player_size() == sizeof(gfo_state_player) && gfo_state_meta_size() == sizeof(gfo_state_meta)
        && gfo_state_response_size() == sizeof(gfo_state_response), "ABI sizes");
    const char name[] = {'A', '\0', 'B'};
    const char uid[] = {'u', '\0', 'i'};
    gfo_state_meta meta{};
    gfo_state_player p{}; p.name = name; p.name_size = 3; p.user_id = uid; p.user_id_size = 3;
    p.hp = 40; p.mp = 10; p.cp = 20; p.item_count = 2;
    std::array<gfo_state_item, 2> items{{{0, 6, 0, 0, {}}, {3, 9, 0, 0, {}}}};
    gfo_state* handle = nullptr;
    check(gfo_state_create(&meta, &p, 1, items.data(), items.size(), nullptr, 0, &handle) == 0 && handle, "copy immutable frame");
    const auto initial = view(handle);
    check(initial.find("A\\u0000B") != std::string::npos && initial.find("\"token\":0") != std::string::npos, "NUL-safe UTF8 string and token0");
    p.hp = 0; items[0].model = 53;
    check(view(handle) == initial, "create owns all caller records/strings");
    uint8_t found = 9; uint64_t id = 99;
    check(gfo_state_resolve_viewer(handle, uid, 3, &found, &id) == 0 && found == 1 && id == 0, "NUL user ID and player0");
    check(gfo_state_resolve_viewer(handle, "absent", 6, &found, &id) == 0 && found == 0, "explicit no user match");
    std::array<char, 8> short_buffer{}; short_buffer.fill('Q'); const auto old_buffer = short_buffer; size_t needed = 787;
    check(gfo_state_view_json(handle, 1, 0, short_buffer.data(), short_buffer.size(), &needed) == -4
        && short_buffer == old_buffer && needed == 787, "short leaves every output unchanged");
    check(gfo_state_view_json(handle, 1, 123, short_buffer.data(), 8, &needed) == -2 && short_buffer == old_buffer && needed == 787, "bad viewer transactional output");
    check(gfo_state_view_json(handle, 0, 1, nullptr, 0, &needed) == -2 && needed == 787, "spectator must be explicit canonical selector");
    check(gfo_state_view_json(handle, 0, 0, nullptr, 1, &needed) == -1, "NULL output storage");
    check(view(handle, 0, 0).find("\"viewer\":null") != std::string::npos, "spectator");
    const uint64_t first = 0;
    uint8_t result = 177;
    check(gfo_state_can_append(handle, 0, &first, 1, 3, nullptr, &result) == 0 && result == 1, "physical token lookup into visible main kernel");
    check(gfo_state_selection_ready(handle, 0, &first, 1, nullptr, &result) == 0 && result == 1, "selection ready");
    result = 177;
    for (size_t count : {size_t{19}, std::numeric_limits<size_t>::max()}) {
        check(gfo_state_can_append(handle, 0, &first, count, 3, nullptr, &result) == -2 && result == 177, "count rejected before dereference");
        check(gfo_state_selection_ready(handle, 0, &first, count, nullptr, &result) == -2 && result == 177, "ready count rejected before dereference");
    }
    gfo_state_response bad{}; bad.incoming_element = -1; bad.reserved[3] = 1;
    check(gfo_state_selection_ready(handle, 0, nullptr, 0, &bad, &result) == -2 && result == 177, "response zero padding");
    gfo_state* sentinel = reinterpret_cast<gfo_state*>(uintptr_t{1});
    p.hp = 40; items[0].model = 6;
    auto invalid = p; invalid.team = 5;
    check(gfo_state_create(&meta, &invalid, 1, items.data(), 2, nullptr, 0, &sentinel) == -2 && sentinel == reinterpret_cast<gfo_state*>(uintptr_t{1}), "create cannot replace output on bad input");
    const char broken_utf[] = {char(0xc0), char(0x80)};
    invalid = p; invalid.name = broken_utf; invalid.name_size = 2;
    check(gfo_state_create(&meta, &invalid, 1, items.data(), 2, nullptr, 0, &sentinel) == -2, "overlong UTF8 rejected");
    invalid.name = "\xed\xa0\x80"; invalid.name_size = 3;
    check(gfo_state_create(&meta, &invalid, 1, items.data(), 2, nullptr, 0, &sentinel) == -2, "UTF8 surrogate rejected");
    invalid = p; invalid.name = "X"; invalid.name_size = std::numeric_limits<size_t>::max();
    check(gfo_state_create(&meta, &invalid, 1, items.data(), 2, nullptr, 0, &sentinel) == -2
        && sentinel == reinterpret_cast<gfo_state*>(uintptr_t{1}), "impossible string extent rejected before dereference");
    found = 177; id = 888;
    check(gfo_state_resolve_viewer(handle, "X", std::numeric_limits<size_t>::max(), &found, &id) == -2
        && found == 177 && id == 888, "impossible viewer string extent preserves outputs");
    auto duplicate_items = items; duplicate_items[1].token = duplicate_items[0].token;
    check(gfo_state_create(&meta, &p, 1, duplicate_items.data(), 2, nullptr, 0, &sentinel) == -2, "duplicate token rejected");
    invalid = p; invalid.item_count = 1;
    check(gfo_state_create(&meta, &invalid, 1, items.data(), 2, nullptr, 0, &sentinel) == -2, "no ignored flat records");
    items[0].model = 0;
    check(gfo_state_create(&meta, &p, 1, items.data(), 2, nullptr, 0, &sentinel) == -2, "UI model0 placeholder rejected");
    check(view(handle) == initial, "all failures preserve prior live frame");
    gfo_state_destroy(handle); gfo_state_destroy(nullptr);
    std::cout << "official state C API passed\n";
}
