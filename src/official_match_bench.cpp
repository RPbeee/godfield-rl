#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <exception>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

namespace {
using namespace godfield::official;
std::uint64_t parse(const char* text, bool positive = true) {
    std::uint64_t n = 0; const std::string input(text);
    const auto result = std::from_chars(input.data(), input.data() + input.size(), n);
    if (result.ec != std::errc{} || result.ptr != input.data() + input.size() || (positive && !n)) throw std::invalid_argument("invalid unsigned argument");
    return n;
}
void mix(std::uint64_t& h, std::uint64_t value) {
    for (unsigned i = 0; i < 8; ++i) { h ^= (value >> (i * 8)) & 255; h *= 1099511628211ULL; }
}
struct Stats {
    std::uint64_t games = 0, decisions = 0, draws = 0, truncations = 0, checksum = 0;
    std::uint64_t main = 0, responses = 0, physical = 0, first_uses = 0, recasts = 0, defense_cards = 0;
    std::uint64_t prayers = 0, discarded = 0, self_attacks = 0, dark_responses = 0, full_hands = 0;
    std::uint64_t reflections = 0, bounces = 0, filters = 0, max_redirect_depth = 0;
    std::uint64_t turbulence_first_uses = 0, turbulence_recasts = 0, self_bounce_responses = 0;
    std::uint64_t blocks = 0, wall_first_uses = 0, wall_recasts = 0;
    std::uint64_t free_main = 0, free_responses = 0, waived_mp = 0, doublings = 0, element_setters = 0;
    std::uint64_t chance_attacks = 0, chance_rolls = 0, chance_hits = 0;
    std::uint64_t counter_created = 0, counter_started = 0, counter_rolls = 0, counter_hits = 0;
    std::uint64_t mp_counter_resolved = 0, mp_counter_gained = 0, cp_counter_resolved = 0, cp_counter_nominal = 0;
    std::array<std::uint64_t, player_capacity> wins{};
    void append(const Stats& s) {
        games += s.games; decisions += s.decisions; draws += s.draws; truncations += s.truncations; checksum += s.checksum;
        main += s.main; responses += s.responses; physical += s.physical; first_uses += s.first_uses; recasts += s.recasts;
        defense_cards += s.defense_cards; prayers += s.prayers; discarded += s.discarded; self_attacks += s.self_attacks;
        dark_responses += s.dark_responses; full_hands += s.full_hands;
        reflections += s.reflections; bounces += s.bounces; filters += s.filters; max_redirect_depth = std::max(max_redirect_depth, s.max_redirect_depth);
        turbulence_first_uses += s.turbulence_first_uses; turbulence_recasts += s.turbulence_recasts; self_bounce_responses += s.self_bounce_responses;
        blocks += s.blocks; wall_first_uses += s.wall_first_uses; wall_recasts += s.wall_recasts;
        free_main += s.free_main; free_responses += s.free_responses; waived_mp += s.waived_mp;
        doublings += s.doublings; element_setters += s.element_setters;
        chance_attacks += s.chance_attacks; chance_rolls += s.chance_rolls; chance_hits += s.chance_hits;
        counter_created += s.counter_created; counter_started += s.counter_started;
        counter_rolls += s.counter_rolls; counter_hits += s.counter_hits;
        mp_counter_resolved += s.mp_counter_resolved; mp_counter_gained += s.mp_counter_gained;
        cp_counter_resolved += s.cp_counter_resolved; cp_counter_nominal += s.cp_counter_nominal;
        for (std::size_t i = 0; i < wins.size(); ++i) wins[i] += s.wins[i];
    }
};
}
int main(int argc, char** argv) {
    try {
        if (argc > 5) throw std::invalid_argument("usage: godfield_official_bench [games=10000] [players=2] [seed=42] [workers=1]");
        const auto games = argc > 1 ? parse(argv[1]) : 10000;
        const auto count = argc > 2 ? parse(argv[2]) : 2;
        const auto seed = argc > 3 ? parse(argv[3], false) : 42;
        auto workers = argc > 4 ? parse(argv[4]) : 1;
        if (games > 1000000000 || count < 2 || count > 9 || workers > 64 || seed > UINT64_MAX - (games - 1)) throw std::invalid_argument("benchmark limits/seed range exceeded");
        workers = std::min(workers, games);
        std::vector<PlayerSetup> setup;
        for (std::uint64_t i = 0; i < count; ++i) setup.push_back({100 + i * 7, Team::solo, "P" + std::to_string(i)});
        std::vector<Stats> results(workers); std::vector<std::exception_ptr> errors(workers); std::vector<std::jthread> threads;
        const auto start = std::chrono::steady_clock::now();
        for (std::uint64_t worker = 0; worker < workers; ++worker) threads.emplace_back([&, worker](std::stop_token stop) {
            try {
                Match match; auto& stats = results[worker];
                for (std::uint64_t i = worker; i < games && !stop.stop_requested(); i += workers) {
                    match.reset(setup, seed + i); std::uint64_t policy = (seed + i) ^ 0x6a09e667f3bcc909ULL;
                    std::uint64_t hash = 14695981039346656037ULL; mix(hash, i);
                    while (!match.done()) {
                        const auto command = match.sample_action(policy); const auto& before = match.snapshot();
                        const auto rolls_before = before.chance_rolls, hits_before = before.chance_hits;
                        const auto created_before = before.counter_created, started_before = before.counter_started;
                        const auto counter_rolls_before = before.counter_rolls, counter_hits_before = before.counter_hits;
                        std::optional<PlayerId> mp_counter_target;
                        std::int64_t mp_counter_before = 0;
                        if (command.kind == CommandKind::defend && command.items.empty() && !before.queue.empty()) {
                            const auto& effect = before.queue.front();
                            if (effect.ability == Ability::counter_boost2x_mp) {
                                ++stats.mp_counter_resolved; mp_counter_target = effect.target;
                                mp_counter_before = find_player(before.state, effect.target)->mp;
                            } else if (effect.ability == Ability::counter_take_cp) {
                                ++stats.cp_counter_resolved; stats.cp_counter_nominal += effect.amount;
                            }
                        }
                        const auto redirect_depth = before.queue.empty() ? 0 : before.queue.front().redirects.size();
                        bool bounced = false;
                        auto incoming_element = before.queue.empty() ? Attribute{} : before.queue.front().element;
                        if (before.phase == MatchPhase::main) ++stats.main; else ++stats.responses;
                        if (!before.queue.empty() && before.queue.front().element == Element::darkness) ++stats.dark_responses;
                        const auto& own = *find_player(before.state, command.actor);
                        if (own.items.size() == before.config.hand_limit) ++stats.full_hands;
                        if (command.kind == CommandKind::pray) ++stats.prayers;
                        if (command.kind == CommandKind::discard) stats.discarded += command.items.size();
                        if (command.kind == CommandKind::defend) stats.defense_cards += command.items.size();
                        if (command.kind == CommandKind::play && command.target == command.actor && !command.items.empty()) {
                            const auto item = std::find_if(own.items.begin(), own.items.end(), [&](const auto& v) { return v.token == command.items[0]; });
                            if (find_card(item->model)->atk > 0) ++stats.self_attacks;
                        }
                        int preceding_miracle_cost = 0;
                        if (command.kind == CommandKind::play || command.kind == CommandKind::defend) for (auto token : command.items) {
                            const auto item = std::find_if(own.items.begin(), own.items.end(), [&](const auto& v) { return v.token == token; });
                            const auto& card = *find_card(item->model);
                            if (command.kind == CommandKind::play && token == command.items.front() && card.hit_rate) ++stats.chance_attacks;
                            if (card.category == Category::miracles) { if (item->used) ++stats.recasts; else ++stats.first_uses; }
                            else ++stats.physical;
                            if (card.ability == Ability::cut_cost) {
                                if (command.kind == CommandKind::play) ++stats.free_main; else ++stats.free_responses;
                                stats.waived_mp += preceding_miracle_cost;
                            }
                            preceding_miracle_cost = card.category == Category::miracles ? card.cost : 0;
                            if (card.ability == Ability::double_atk) ++stats.doublings;
                            if (card.ability == Ability::set_element) ++stats.element_setters;
                            if (command.kind == CommandKind::defend) {
                                if (find_card(item->model)->ability == Ability::filter_atk_element) { ++stats.filters; incoming_element.reset(); }
                                const auto reaction = reaction_for(item->model, before.queue.front().category, incoming_element);
                                if (reaction == Reaction::reflect || reaction == Reaction::bounce) {
                                    if (reaction == Reaction::reflect) ++stats.reflections; else { ++stats.bounces; bounced = true; }
                                    stats.max_redirect_depth = std::max(stats.max_redirect_depth, static_cast<std::uint64_t>(redirect_depth + 1));
                                }
                                if (reaction == Reaction::block) ++stats.blocks;
                                if (find_card(item->model)->ability == Ability::block_weapon) {
                                    if (item->used) ++stats.wall_recasts; else ++stats.wall_first_uses;
                                }
                                if (card.category == Category::miracles && card.ability == Ability::bounce_miracle) {
                                    if (item->used) ++stats.turbulence_recasts; else ++stats.turbulence_first_uses;
                                }
                            }
                        }
                        mix(hash, command.actor); mix(hash, static_cast<unsigned>(command.kind)); mix(hash, command.items.size());
                        for (auto token : command.items) mix(hash, token);
                        mix(hash, command.target.has_value()); if (command.target) mix(hash, *command.target);
                        const auto step = match.step(command); ++stats.decisions;
                        stats.chance_rolls += match.snapshot().chance_rolls - rolls_before;
                        stats.chance_hits += match.snapshot().chance_hits - hits_before;
                        stats.counter_created += match.snapshot().counter_created - created_before;
                        stats.counter_started += match.snapshot().counter_started - started_before;
                        stats.counter_rolls += match.snapshot().counter_rolls - counter_rolls_before;
                        stats.counter_hits += match.snapshot().counter_hits - counter_hits_before;
                        if (mp_counter_target) stats.mp_counter_gained += find_player(match.snapshot().state, *mp_counter_target)->mp - mp_counter_before;
                        if (bounced && step.actor == command.actor) ++stats.self_bounce_responses;
                        mix(hash, static_cast<unsigned>(step.phase)); mix(hash, step.terminated); mix(hash, step.truncated);
                    }
                    const auto& terminal = match.snapshot(); ++stats.games;
                    if (terminal.truncated) ++stats.truncations; else if (terminal.winners.empty()) ++stats.draws;
                    for (std::size_t seat = 0; seat < count; ++seat) if (terminal.rewards[seat] > 0) ++stats.wins[seat];
                    mix(hash, terminal.rng); mix(hash, terminal.decisions); for (auto id : terminal.winners) mix(hash, id); stats.checksum += hash;
                    match.restore(match.snapshot()); // Audit every completed episode as well as its rollout.
                }
            } catch (...) { errors[worker] = std::current_exception(); }
        });
        for (auto& thread : threads) thread.join();
        for (const auto& error : errors) if (error) std::rethrow_exception(error);
        Stats stats; for (const auto& local : results) stats.append(local);
        std::uint64_t wins = 0; for (auto value : stats.wins) wins += value;
        if (stats.games != games || stats.main + stats.responses != stats.decisions || wins + stats.draws + stats.truncations != games)
            throw std::logic_error("inconsistent scoped benchmark accounting");
        const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "ruleset=" << match_ruleset << "\nscope=" << match_scope << "\ncards=" << match_catalog().size()
            << "\ngift_weight=" << match_gift_weight() << "\ngames=" << stats.games << "\nplayers=" << count << "\nseed=" << seed << "\nworkers=" << workers
            << "\nbounce_target_rule=" << bounce_target_rule
            << "\nchance_rule=" << chance_rule
            << "\ncounter_rule=" << counter_rule
            << "\nresource_counter_rule=" << resource_counter_rule
            << "\ndecisions=" << stats.decisions << "\ndraws=" << stats.draws << "\ntruncations=" << stats.truncations << "\nwins=";
        for (std::size_t i = 0; i < count; ++i) { if (i) std::cout << ','; std::cout << stats.wins[i]; }
        std::cout << "\nmain_commands=" << stats.main << "\nresponses=" << stats.responses << "\nphysical_used=" << stats.physical
            << "\nmiracle_first_uses=" << stats.first_uses << "\nmiracle_recasts=" << stats.recasts << "\ndefense_cards=" << stats.defense_cards
            << "\nprayers=" << stats.prayers << "\ndiscarded=" << stats.discarded << "\nself_attacks=" << stats.self_attacks
            << "\ndark_responses=" << stats.dark_responses << "\nfull_hand_actions=" << stats.full_hands << "\nchecksum=" << stats.checksum
            << "\nreflections=" << stats.reflections << "\nbounces=" << stats.bounces << "\nfilters=" << stats.filters << "\nmax_redirect_depth=" << stats.max_redirect_depth
            << "\nturbulence_first_uses=" << stats.turbulence_first_uses << "\nturbulence_recasts=" << stats.turbulence_recasts << "\nself_bounce_responses=" << stats.self_bounce_responses
            << "\nblocks=" << stats.blocks << "\nwall_first_uses=" << stats.wall_first_uses << "\nwall_recasts=" << stats.wall_recasts
            << "\nfree_main_used=" << stats.free_main << "\nfree_response_used=" << stats.free_responses << "\nwaived_mp=" << stats.waived_mp
            << "\ndoublings=" << stats.doublings << "\nelement_setters=" << stats.element_setters
            << "\nchance_attacks=" << stats.chance_attacks << "\nchance_rolls=" << stats.chance_rolls << "\nchance_hits=" << stats.chance_hits
            << "\nchance_misses=" << stats.chance_rolls - stats.chance_hits
            << "\ncounter_created=" << stats.counter_created << "\ncounter_started=" << stats.counter_started
            << "\ncounter_rolls=" << stats.counter_rolls << "\ncounter_hits=" << stats.counter_hits
            << "\ncounter_misses=" << stats.counter_rolls - stats.counter_hits
            << "\nmp_counter_resolved=" << stats.mp_counter_resolved << "\nmp_counter_gained=" << stats.mp_counter_gained
            << "\ncp_counter_resolved=" << stats.cp_counter_resolved << "\ncp_counter_nominal=" << stats.cp_counter_nominal
            << "\nseconds=" << seconds << "\ndecisions_per_second=" << (seconds ? stats.decisions / seconds : 0) << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
