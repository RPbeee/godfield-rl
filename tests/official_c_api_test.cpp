#include "godfield/official_c_api.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>

namespace {
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL official C ABI: " << message << '\n'; std::exit(1); }
}
template<class T> bool same(const T& a, const T& b) { return std::memcmp(&a, &b, sizeof(T)) == 0; }
}
int main() {
    static_assert(sizeof(gfo_card) == 44 && offsetof(gfo_card, atk) == 8);
    static_assert(sizeof(gfo_composition) == 24 && offsetof(gfo_composition, reserved) == 18);
    check(gfo_abi_version() == 1 && gfo_card_size() == 44 && gfo_composition_size() == 24, "ABI dimensions");
    check(gfo_catalog_count() == 296 && gfo_gift_rate() == 500, "official catalog totals");
    check(std::strstr(gfo_scope(), "not-playable") != nullptr, "partial scope explicit");
    check(std::strlen(gfo_catalog_sha256()) == 64 && std::strlen(gfo_client_sha256()) == 64, "source digests");
    gfo_card card;
    std::memset(&card, 0xA5, sizeof(card));
    const auto before_card = card;
    check(gfo_get_card(65, nullptr) == -1, "NULL card output");
    for (auto bad : {std::uint16_t{0}, std::uint16_t{297}, std::uint16_t{65535}}) {
        check(gfo_get_card(bad, &card) == -2 && same(card, before_card), "bad ID preserves every card byte");
        check(!gfo_card_name(bad) && !gfo_card_image_name(bad), "bad ID string NULL");
    }
    check(gfo_get_card(65, &card) == 0 && card.id == 65 && card.atk == 2, "official model65 metadata");
    check(std::strcmp(gfo_card_image_name(65), "ouch-knife") == 0 && std::strcmp(gfo_ability_name(card.ability), "") == 0,
          "official65 is not legacy65 setter");
    check(gfo_get_card(66, &card) == 0 && card.is_plus_atk && std::strcmp(gfo_ability_name(card.ability), "setElement") == 0,
          "current setter is official66");
    check(!gfo_category_name(255) && !gfo_ability_name(255) && !gfo_curse_name(255) && !gfo_guardian_name(255), "bad enum string NULL");
    check(!gfo_element_name(-2) && !gfo_element_name(6) && std::strcmp(gfo_element_name(-1), "") == 0, "element missing versus invalid");

    gfo_composition out;
    std::memset(&out, 0xA5, sizeof(out));
    const auto before = out;
    const std::array<std::uint16_t, 2> ids{13, 66}, bad{13, 297}, overflow{13, 231};
    check(gfo_compose(ids.data(), ids.size(), 0, nullptr) == -1, "NULL compose output");
    check(gfo_compose(nullptr, 1, 0, &out) == -1 && same(out, before), "NULL input leaves output");
    check(gfo_compose(nullptr, 0, 0, &out) == -2 && same(out, before), "empty leaves output");
    check(gfo_compose(ids.data(), ids.size(), -1, &out) == -2 && same(out, before), "negative leaves output");
    check(gfo_compose(bad.data(), bad.size(), 0, &out) == -2 && same(out, before), "invalid later ID leaves output");
    check(gfo_compose(overflow.data(), overflow.size(), std::numeric_limits<std::int64_t>::max(), &out) == -3 && same(out, before),
          "overflow caught and output unchanged");
    check(gfo_compose(ids.data(), std::numeric_limits<std::size_t>::max(), 0, &out) == -2 && same(out, before),
          "impossible input length rejected before reading");
    check(gfo_compose(ids.data(), ids.size(), 0, &out) == 0, "valid compose");
    check(out.attack_power == 5 && out.magic_cost == 0 && out.attack_element == 0 && out.defense_element == -1,
          "setter adds power and sets fire while neutral defense stays neutral");
    for (auto byte : out.reserved) check(byte == 0, "reserved bytes canonical zero");
    const std::array<std::uint16_t, 2> free_magic{235, 204};
    check(gfo_compose(free_magic.data(), free_magic.size(), 0, &out) == 0 && out.magic_cost == 0, "current Spring FREE cost0");
    std::cout << "Official C ABI checks passed\n";
}
