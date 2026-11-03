#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <chrono>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
using namespace godfield::official;
constexpr std::array<CardId, 5> models{287, 288, 290, 294, 296};
void check(bool ok, const char* message) { if (!ok) throw std::logic_error(message); }
std::uint64_t number(const char* text, bool positive = true) {
    std::uint64_t value = 0; const std::string input(text);
    const auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != input.data() + input.size() || (positive && !value))
        throw std::invalid_argument("invalid numeric argument");
    return value;
}
void mix(std::uint64_t& hash, std::uint64_t value) {
    for (unsigned byte = 0; byte != 8; ++byte) { hash ^= (value >> (byte * 8)) & 255; hash *= 1099511628211ULL; }
}
void text_hash(std::uint64_t& hash, const std::string& value) {
    mix(hash, value.size()); for (unsigned char byte : value) { hash ^= byte; hash *= 1099511628211ULL; }
}
template<class T, class F> void optional_hash(std::uint64_t& hash, const std::optional<T>& value, F write) {
    mix(hash, value.has_value()); if (value) write(*value);
}
void item_hash(std::uint64_t& hash, const ItemInstance& item) {
    mix(hash, item.token); mix(hash, item.model); mix(hash, item.used);
    optional_hash(hash, item.fake_model, [&](auto value) { mix(hash, value); });
}
void guardian_hash(std::uint64_t& hash, const GuardianAction& action) {
    mix(hash, action.source); mix(hash, action.model);
    optional_hash(hash, action.target, [&](auto value) { mix(hash, value); });
}
void phenomenon_hash(std::uint64_t& hash, const PhenomenonAction& action) {
    mix(hash, action.model); optional_hash(hash, action.recipient, [&](auto value) { mix(hash, value); });
    mix(hash, action.guardians.size()); for (auto guardian : action.guardians) mix(hash, static_cast<unsigned>(guardian));
}
// Called only at the two quiescent boundaries, so every pending-work field is
// checked empty. All remaining snapshot fields, including privileged history,
// participate in this hash; it is not a player observation.
std::uint64_t state_hash(const MatchSnapshot& s) {
    check(s.queue.empty() && !s.batch && s.counters.empty() && s.deaths.empty() && !s.purchase && !s.last_item_shuffle && !s.last_fate && !s.last_fate_event && !s.fate_actions,
          "hash boundary has pending work");
    std::uint64_t hash = 14695981039346656037ULL;
#define FIELD(name) mix(hash, static_cast<std::uint64_t>(s.name))
    FIELD(version); FIELD(config.max_decisions); FIELD(config.initial_hand); FIELD(config.hand_limit);
    FIELD(config.initial_hp); FIELD(config.initial_mp); FIELD(config.initial_cp);
    FIELD(state.gf); FIELD(state.is_over); FIELD(state.update_count);
    optional_hash(hash, s.state.tiebreak_gf, [&](auto value) { mix(hash, static_cast<std::uint64_t>(value)); });
    mix(hash, s.state.players.size());
    for (const auto& player : s.state.players) {
        mix(hash, player.id); text_hash(hash, player.name);
        optional_hash(hash, player.user_id, [&](const auto& value) { text_hash(hash, value); });
        optional_hash(hash, player.alias, [&](const auto& value) { text_hash(hash, value); });
        mix(hash, static_cast<unsigned>(player.team)); mix(hash, player.hp); mix(hash, player.mp); mix(hash, player.cp);
        mix(hash, static_cast<unsigned>(player.guardian)); mix(hash, player.confusion_count); mix(hash, player.dead);
        mix(hash, player.curses.size()); for (auto curse : player.curses) mix(hash, static_cast<unsigned>(curse));
        mix(hash, player.items.size()); for (const auto& item : player.items) item_hash(hash, item);
    }
    FIELD(owner); optional_hash(hash, s.actor, [&](auto value) { mix(hash, value); }); FIELD(phase);
    FIELD(rng); FIELD(decisions); FIELD(next_token); FIELD(chance_rolls); FIELD(chance_hits);
    FIELD(counter_created); FIELD(counter_started); FIELD(counter_rolls); FIELD(counter_hits);
    FIELD(chance_forced_hits); FIELD(counter_forced_hits); FIELD(revivals);
    FIELD(death_bows); FIELD(death_created); FIELD(death_started); FIELD(death_rolls); FIELD(death_hits); FIELD(death_forced_hits);
    FIELD(dream_rolls); FIELD(dream_masks); FIELD(redraws); FIELD(redrawn_items);
    FIELD(guardian_actions); FIELD(guardian_payload_actions); FIELD(phenomenon_actions);
    FIELD(guardian_summons); FIELD(guardian_opportunities); FIELD(auto_guardian_actions);
    FIELD(guardian_departure_rolls); FIELD(guardian_departures); FIELD(terminated); FIELD(truncated);
#undef FIELD
    mix(hash, s.gifts.size()); for (auto count : s.gifts) mix(hash, count);
    mix(hash, s.rewards.size()); for (auto reward : s.rewards) mix(hash, std::bit_cast<std::uint64_t>(reward));
    mix(hash, s.winners.size()); for (auto id : s.winners) mix(hash, id);
    optional_hash(hash, s.last_phenomenon, [&](const auto& action) { phenomenon_hash(hash, action); });
    optional_hash(hash, s.last_guardian, [&](const auto& action) { guardian_hash(hash, action); });
    optional_hash(hash, s.last_guardian_payload, [&](const auto& action) {
        mix(hash, action.source); mix(hash, static_cast<unsigned>(action.guardian));
        mix(hash, action.models.size()); for (auto model : action.models) mix(hash, model);
        optional_hash(hash, action.target, [&](auto value) { mix(hash, value); });
    });
    mix(hash, s.last_auto_guardians.size()); for (const auto& action : s.last_auto_guardians) guardian_hash(hash, action);
    mix(hash, s.last_guardian_changes.size());
    for (const auto& event : s.last_guardian_changes) {
        mix(hash, event.player); mix(hash, static_cast<unsigned>(event.before)); mix(hash, static_cast<unsigned>(event.after));
        mix(hash, static_cast<unsigned>(event.reason)); mix(hash, event.hidden_viewers);
    }
    mix(hash, s.last_redraws.size()); for (auto owner : s.last_redraws) mix(hash, owner);
    mix(hash, s.last_reveals.size()); for (const auto& item : s.last_reveals) {
        mix(hash, item.owner); mix(hash, item.face); mix(hash, item.model); mix(hash, item.owner_only);
    }
    optional_hash(hash, s.last_tick, [&](const auto& tick) {
        mix(hash, tick.owner); mix(hash, static_cast<unsigned>(tick.before)); mix(hash, static_cast<unsigned>(tick.after));
        mix(hash, tick.hp_before); mix(hash, tick.hp_after); mix(hash, tick.worsened); mix(hash, tick.fallen);
        optional_hash(hash, tick.revival_token, [&](auto token) { mix(hash, token); });
    });
    optional_hash(hash, s.last_fog, [&](const auto& event) {
        mix(hash, event.owner); mix(hash, event.intended); mix(hash, event.target); mix(hash, event.models.size());
        for (auto model : event.models) mix(hash, model);
    });
    optional_hash(hash, s.last_danger, [&](const auto& event) {
        mix(hash, event.origin); mix(hash, event.target); mix(hash, event.mortar); mix(hash, event.power);
        optional_hash(hash, event.element, [&](auto value) { mix(hash, static_cast<unsigned>(value)); });
        mix(hash, event.models.size()); for (auto model : event.models) mix(hash, model);
    });
    optional_hash(hash, s.last_removal, [&](const auto& event) {
        mix(hash, event.origin); mix(hash, event.source); mix(hash, event.target); mix(hash, event.model);
        mix(hash, event.removed_models.size()); for (auto model : event.removed_models) mix(hash, model);
    });
    optional_hash(hash, s.last_sale, [&](const auto& event) {
        mix(hash, event.origin); mix(hash, event.source); mix(hash, event.target); mix(hash, event.model); mix(hash, event.price);
        optional_hash(hash, event.overflow_model, [&](auto model) { mix(hash, model); });
    });
    optional_hash(hash, s.purchase, [&](const auto& offer) {
        mix(hash, offer.origin); mix(hash, offer.buyer); mix(hash, offer.seller); item_hash(hash, offer.item); mix(hash, offer.slot);
    });
    optional_hash(hash, s.last_purchase, [&](const auto& event) {
        mix(hash, event.origin); mix(hash, event.buyer); mix(hash, event.seller); mix(hash, event.model);
        mix(hash, event.price); mix(hash, static_cast<unsigned>(event.reason));
        optional_hash(hash, event.overflow_model, [&](auto model) { mix(hash, model); });
    });
    return hash;
}
void audit(const Match& game, Match& restored) {
    restored.restore(game.snapshot()); check(restored.snapshot() == game.snapshot(), "phenomenon checkpoint failed roundtrip");
}
struct Stats {
    std::uint64_t scenes = 0, decisions = 0, responses = 0, terminated = 0, truncated = 0, checksum = 0, primitive_checksum = 0;
    std::uint64_t chance_rolls = 0, chance_hits = 0, chance_forced = 0, counter_created = 0, counter_started = 0;
    std::uint64_t revivals = 0, redraws = 0, guardian_opportunities = 0, auto_guardian_actions = 0;
    std::uint64_t guardian_summons = 0, departure_rolls = 0, departures = 0, disease_ticks = 0, gf_total = 0;
    std::uint64_t dead_input_scenes = 0, cp_collected = 0, cp_credited = 0, hp_direct_reduced = 0;
    std::uint64_t fever_assignments = 0, fog_added = 0, guardian_assignments = 0, same_guardian_assignments = 0;
    std::array<std::uint64_t, models.size()> coverage{};
    void append(const Stats& other) {
#define ADD(name) name += other.name
        ADD(scenes); ADD(decisions); ADD(responses); ADD(terminated); ADD(truncated); ADD(checksum); ADD(primitive_checksum);
        ADD(chance_rolls); ADD(chance_hits); ADD(chance_forced); ADD(counter_created); ADD(counter_started);
        ADD(revivals); ADD(redraws); ADD(guardian_opportunities); ADD(auto_guardian_actions); ADD(guardian_summons);
        ADD(departure_rolls); ADD(departures); ADD(disease_ticks); ADD(gf_total); ADD(dead_input_scenes);
        ADD(cp_collected); ADD(cp_credited); ADD(hp_direct_reduced); ADD(fever_assignments); ADD(fog_added);
        ADD(guardian_assignments); ADD(same_guardian_assignments);
#undef ADD
        for (std::size_t i = 0; i < coverage.size(); ++i) coverage[i] += other.coverage[i];
    }
};
void verify_primitive(const MatchSnapshot& before, const MatchSnapshot& after, const PhenomenonAction& action, Stats& stats) {
    check(after.decisions == 1 && after.owner == 100 && after.actor == 100 && after.phase == MatchPhase::main,
          "primitive changed owner, phase or decision boundary");
    check(after.rng == before.rng && after.state.gf == 0 && after.next_token == before.next_token
          && after.phenomenon_actions == 1 && !after.terminated && !after.truncated, "primitive changed RNG or token schedule");
    check(after.guardian_actions == 0 && after.guardian_payload_actions == 0 && after.guardian_summons == 0
          && after.guardian_opportunities == 0 && after.auto_guardian_actions == 0 && after.guardian_departure_rolls == 0
          && after.guardian_departures == 0 && !after.last_tick && after.revivals == 0 && after.death_bows == 0
          && after.counter_created == 0 && after.chance_rolls == 0, "primitive changed non-global accounting");
    check(after.last_phenomenon == action && std::all_of(after.gifts.begin(), after.gifts.end(), [](auto n) { return n == 0; }),
          "primitive lost event or requested gifts");
    auto expected = before.state; expected.update_count += 1;
    std::int64_t total_cp = 0; for (const auto& player : expected.players) if (!player.dead) total_cp += player.cp;
    std::size_t assigned = 0;
    for (auto& player : expected.players) if (!player.dead) {
        switch (action.model) {
        case 287:
            std::erase_if(player.curses, [](auto curse) { return curse >= Curse::cold && curse <= Curse::heaven; });
            player.curses.insert(std::lower_bound(player.curses.begin(), player.curses.end(), Curse::fever), Curse::fever);
            ++stats.fever_assignments;
            break;
        case 288:
            if (std::find(player.curses.begin(), player.curses.end(), Curse::fog) == player.curses.end()) {
                player.curses.insert(std::lower_bound(player.curses.begin(), player.curses.end(), Curse::fog), Curse::fog);
                ++stats.fog_added;
            }
            break;
        case 290: stats.hp_direct_reduced += static_cast<std::uint64_t>(player.hp - 1); player.hp = 1; break;
        case 294: player.cp = 0; break;
        case 296:
            stats.same_guardian_assignments += player.guardian == action.guardians[assigned];
            player.guardian = action.guardians[assigned++]; ++stats.guardian_assignments;
            break;
        default: throw std::logic_error("unexpected primitive model");
        }
    }
    if (action.model == 294) {
        for (auto& player : expected.players) if (player.id == *action.recipient) player.cp = std::min<std::int64_t>(99, total_cp);
        stats.cp_collected += static_cast<std::uint64_t>(total_cp); stats.cp_credited += static_cast<std::uint64_t>(std::min<std::int64_t>(99, total_cp));
    }
    check(after.state == expected, "global primitive changed unrelated state or missed an effect");
    check(after.last_guardian_changes.size() == assigned, "unexpected guardian change count");
    for (std::size_t seat = 0, event_index = 0; seat < expected.players.size(); ++seat) if (!expected.players[seat].dead && action.model == 296) {
        const auto& event = after.last_guardian_changes[event_index++];
        check(event.player == expected.players[seat].id && event.before == before.state.players[seat].guardian
              && event.after == expected.players[seat].guardian && event.reason == GuardianChangeReason::phenomenon,
              "guardian assignment event differs from declared input order");
        std::uint16_t hidden = 0;
        for (std::size_t viewer = 0; viewer < before.state.players.size(); ++viewer) {
            const auto view = project_view(before.state, before.state.players[viewer].id);
            if (view.players[seat].fogged) hidden |= static_cast<std::uint16_t>(1u << viewer);
        }
        check(event.hidden_viewers == hidden, "guardian change violated event-time Fog mask");
    }
}
}

int main(int argc, char** argv) {
    try {
        if (argc > 5) throw std::invalid_argument("usage: godfield_phenomenon_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
        const auto count = argc > 1 ? number(argv[1]) : 10000;
        const auto players = argc > 2 ? number(argv[2]) : 2;
        const auto seed = argc > 3 ? number(argv[3], false) : 42;
        auto workers = argc > 4 ? number(argv[4]) : 1;
        if (count > 1000000000 || players < 2 || players > 9 || workers > 64 || seed > UINT64_MAX - (count - 1))
            throw std::invalid_argument("phenomenon benchmark bounds exceeded");
        workers = std::min(workers, count);
        std::vector<Stats> stats(workers); std::vector<std::exception_ptr> errors(workers); std::vector<std::jthread> threads;
        const auto start = std::chrono::steady_clock::now();
        for (std::uint64_t worker = 0; worker < workers; ++worker) threads.emplace_back([&, worker] {
            try {
                MatchConfig config; config.max_decisions = 64;
                Match game(config), restored(config);
                auto& out = stats[worker];
                for (std::uint64_t i = worker; i < count; i += workers) {
                    const auto scene_seed = seed + i; const auto column = i % models.size();
                    SnapshotState state;
                    for (std::size_t seat = 0; seat < players; ++seat) {
                        PlayerState player; player.id = 100 + seat * 7; player.name = "P" + std::to_string(seat);
                        player.hp = 1 + static_cast<std::int64_t>((scene_seed % 99 + seat) % 99); player.mp = 99;
                        player.cp = static_cast<std::int64_t>((scene_seed % 100 + seat) % 100);
                        player.curses.push_back(static_cast<Curse>(1 + (scene_seed % 4 + seat) % 4));
                        if ((scene_seed % 3 + seat) % 3 == 0) player.curses.push_back(Curse::fog);
                        player.guardian = static_cast<Guardian>(1 + (scene_seed % 10 + seat) % 10);
                        for (auto model : {CardId{6}, CardId{190}, CardId{178}, CardId{184}, CardId{235}}) {
                            // A weapon would forbid the required ordinary prayer.
                            // Other players retain the complete requested inventory.
                            if (seat == 0 && model == 6) continue;
                            player.items.push_back({seat * 100 + player.items.size(), model, {}, false});
                        }
                        if (players == 9 && seat == 8 && i % 17 == 0) { player.hp = 0; player.dead = true; ++out.dead_input_scenes; }
                        state.players.push_back(std::move(player));
                    }
                    const auto alive = alive_players(state);
                    PhenomenonAction action; action.model = models[column];
                    if (action.model == 294) action.recipient = alive[(i / models.size()) % alive.size()];
                    if (action.model == 296) {
                        for (std::size_t seat = 0, live_index = 0; seat < state.players.size(); ++seat) if (!state.players[seat].dead) {
                            const auto mode = (i / models.size()) % 3;
                            action.guardians.push_back(mode == 0 ? state.players[seat].guardian : mode == 1 ? Guardian::mars
                                : static_cast<Guardian>(1 + (scene_seed % 10 + live_index) % 10));
                            ++live_index;
                        }
                    }
                    game.load_scene(std::move(state), 100, scene_seed);
                    const auto before = game.snapshot();
                    check(game.phenomenon_legal(action), "phenomenon scenario is not legal");
                    game.phenomenon_step(action); audit(game, restored);
                    verify_primitive(before, game.snapshot(), action, out);
                    std::uint64_t first_hash = state_hash(game.snapshot()); mix(first_hash, i); out.primitive_checksum += first_hash;
                    ++out.coverage[column];
                    const Command prayer{100, CommandKind::pray, {}, {}, {}};
                    check(game.legal(prayer), "ordinary prayer after primitive is not legal");
                    game.step(prayer); audit(game, restored);
                    std::uint64_t policy = scene_seed ^ 0x6a09e667f3bcc909ULL;
                    while (!game.done() && game.snapshot().phase != MatchPhase::main) {
                        check(game.snapshot().phase == MatchPhase::response, "unexpected purchase phase in phenomenon scene");
                        const auto command = game.sample_action(policy); check(game.legal(command), "response sampler produced an illegal command");
                        game.step(command); audit(game, restored); ++out.responses;
                    }
                    const auto& final = game.snapshot();
                    check(final.phenomenon_actions == 1 && final.guardian_actions == 0 && final.guardian_payload_actions == 0,
                          "ordinary replay lost phenomenon prefix or invented an external action");
                    check(final.decisions >= 2 && final.decisions <= config.max_decisions && !final.last_phenomenon,
                          "ordinary main did not replace the primitive event");
                    std::uint64_t hash = state_hash(final); mix(hash, i); mix(hash, action.model); mix(hash, policy);
                    out.checksum += hash; ++out.scenes; out.decisions += final.decisions; out.terminated += final.terminated; out.truncated += final.truncated;
                    out.chance_rolls += final.chance_rolls; out.chance_hits += final.chance_hits; out.chance_forced += final.chance_forced_hits;
                    out.counter_created += final.counter_created; out.counter_started += final.counter_started; out.revivals += final.revivals; out.redraws += final.redraws;
                    out.guardian_opportunities += final.guardian_opportunities; out.auto_guardian_actions += final.auto_guardian_actions;
                    out.guardian_summons += final.guardian_summons; out.departure_rolls += final.guardian_departure_rolls; out.departures += final.guardian_departures;
                    out.disease_ticks += final.last_tick.has_value(); out.gf_total += static_cast<std::uint64_t>(final.state.gf);
                }
            } catch (...) { errors[worker] = std::current_exception(); }
        });
        for (auto& thread : threads) thread.join();
        for (const auto& error : errors) if (error) std::rethrow_exception(error);
        Stats total; for (const auto& part : stats) total.append(part);
        std::uint64_t covered_scenes = 0; unsigned covered_models = 0;
        for (auto n : total.coverage) { covered_scenes += n; covered_models += n != 0; }
        check(total.scenes == count && covered_scenes == count && covered_models == std::min<std::uint64_t>(5, count)
              && total.decisions == 2 * count + total.responses && total.terminated + total.truncated <= count
              && total.chance_hits <= total.chance_rolls && total.departures <= total.departure_rolls
              && total.auto_guardian_actions <= total.guardian_opportunities, "phenomenon benchmark accounting mismatch");
        const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "ruleset=" << match_ruleset << "\nscope=experimental:external5-global-primitives-then-ordinary-prayer-tick-guardian-replay;not-phenomenon-selector"
                  << "\ncatalog_version=" << catalog_version << "\ncatalog_sha256=" << catalog_sha256 << "\nclient_sha256=" << client_sha256
                  << "\nphenomenon_rule=" << phenomenon_rule << "\nitem_shuffle_rule=" << item_shuffle_rule << "\nconfusion_rule=" << confusion_rule << "\nfate_rule=" << fate_rule << "\nguardian_rule=" << guardian_rule;
#define OUTPUT(key, value) std::cout << "\n" key "=" << (value)
        OUTPUT("scenes", total.scenes); OUTPUT("players", players); OUTPUT("seed", seed); OUTPUT("workers", workers);
        OUTPUT("models_covered", covered_models); OUTPUT("decisions", total.decisions); OUTPUT("responses", total.responses);
        OUTPUT("phenomenon_actions", total.scenes); OUTPUT("guardian_actions", 0); OUTPUT("guardian_payload_actions", 0);
        OUTPUT("terminated", total.terminated); OUTPUT("truncated", total.truncated); OUTPUT("dead_input_scenes", total.dead_input_scenes);
        for (std::size_t i = 0; i < models.size(); ++i) std::cout << "\nmodel" << models[i] << "_scenes=" << total.coverage[i];
        OUTPUT("fever_assignments", total.fever_assignments); OUTPUT("fog_added", total.fog_added); OUTPUT("hp_direct_reduced", total.hp_direct_reduced);
        OUTPUT("cp_collected", total.cp_collected); OUTPUT("cp_credited", total.cp_credited); OUTPUT("guardian_assignments", total.guardian_assignments);
        OUTPUT("same_guardian_assignments", total.same_guardian_assignments); OUTPUT("chance_rolls", total.chance_rolls);
        OUTPUT("chance_hits", total.chance_hits); OUTPUT("chance_forced", total.chance_forced); OUTPUT("counter_created", total.counter_created);
        OUTPUT("counter_started", total.counter_started); OUTPUT("revivals", total.revivals); OUTPUT("redraws", total.redraws);
        OUTPUT("guardian_opportunities", total.guardian_opportunities); OUTPUT("auto_guardian_actions", total.auto_guardian_actions);
        OUTPUT("guardian_summons", total.guardian_summons); OUTPUT("guardian_departure_rolls", total.departure_rolls);
        OUTPUT("guardian_departures", total.departures); OUTPUT("disease_ticks", total.disease_ticks); OUTPUT("gf_total", total.gf_total);
        OUTPUT("primitive_checksum", total.primitive_checksum); OUTPUT("checksum", total.checksum); OUTPUT("seconds", seconds);
        OUTPUT("decisions_per_second", seconds ? total.decisions / seconds : 0);
#undef OUTPUT
        std::cout << '\n'; return 0;
    } catch (const std::exception& error) { std::cerr << "phenomenon benchmark: " << error.what() << '\n'; return 1; }
}
