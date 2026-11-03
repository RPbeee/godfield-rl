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
// Self-target202 draws its body sign first. Isolate that body from later tick,
// revival and automatic guardian effects in the same step.
int self_random_hp_amount(std::uint64_t rng) {
    auto z=rng+0x9e3779b97f4a7c15ULL;
    z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;
    z=(z^(z>>27))*0x94d049bb133111ebULL;
    return ((z^(z>>31))&1) ? -10 : 10;
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
    std::uint64_t absorb_positive_resolutions = 0, absorb_nominal_hp = 0;
    std::uint64_t self_damage_positive_resolutions = 0, self_damage_nominal_hp = 0;
    std::uint64_t flash_added = 0, flash_removed = 0, flash_response_commands = 0;
    std::uint64_t darkcloud_added = 0, darkcloud_removed = 0, chance_forced_hits = 0, counter_forced_hits = 0;
    std::uint64_t hand_removal_resolved = 0, hand_items_removed = 0, used_removal_resolved = 0, used_miracles_removed = 0;
    std::uint64_t removal_events = 0, empty_removal_events = 0, removal_event_checksum = 0;
    std::uint64_t mana_weapon_plays = 0, mana_mp_spent = 0, mana_base_mp = 0;
    std::uint64_t repeat_weapon_plays = 0, repeat_frames_completed = 0;
    std::uint64_t wide_plays = 0, wide_frames_completed = 0;
    std::uint64_t random_hp_plays = 0, random_hp_resolved = 0, random_hp_heals = 0, random_hp_damages = 0;
    std::uint64_t random_hp_gained = 0, random_hp_lost = 0;
    std::uint64_t revivals = 0;
    std::uint64_t danger_plays = 0, danger_mortar_plays = 0, danger_self_targets = 0, danger_ally_targets = 0;
    std::uint64_t exchange_plays = 0, zero_hp_exchanges = 0, exchanged_resource_total = 0;
    std::uint64_t sell_plays = 0, sell_resolved = 0, sell_nominal_cp = 0, sold_miracles = 0;
    std::uint64_t buy_plays = 0, buy_quotes = 0, purchase_commands = 0, bought = 0, declined = 0;
    std::uint64_t buy_empty = 0, buy_insufficient_cp = 0, buy_cutoff = 0, buy_nominal_cp = 0;
    std::uint64_t disease_ticks = 0, disease_worsened = 0, disease_falls = 0, disease_tick_revivals = 0;
    std::uint64_t disease_hp_gained = 0, disease_hp_lost = 0;
    std::uint64_t fever_mask_uses = 0, heaven_herb_plays = 0, heaven_herb_resolved = 0, heaven_herb_mp_gained = 0;
    std::uint64_t fog_main_commands = 0, fog_launches = 0, fog_target_changed = 0;
    std::uint64_t death_bows = 0, death_created = 0, death_started = 0, death_rolls = 0, death_hits = 0, death_forced_hits = 0, death_responses = 0;
    std::uint64_t sell_overflows = 0, buy_overflows = 0, overflow_events = 0, overflow_event_checksum = 0;
    std::uint64_t dream_rolls = 0, dream_masks = 0, dream_main_commands = 0, dream_masked_commands = 0, dream_public_reveals = 0, dream_private_reveals = 0;
    std::uint64_t redraws = 0, redrawn_items = 0, redraw_notices = 0;
    std::uint64_t buy_self_plays = 0, buy_self_inspections = 0, buy_self_empty = 0;
    std::uint64_t guardian_summons = 0, guardian_opportunities = 0, auto_guardian_actions = 0, guardian_departure_rolls = 0, guardian_departures = 0;
    std::uint64_t auto_guardian_records = 0, guardian_change_records = 0;
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
        absorb_positive_resolutions += s.absorb_positive_resolutions; absorb_nominal_hp += s.absorb_nominal_hp;
        self_damage_positive_resolutions += s.self_damage_positive_resolutions; self_damage_nominal_hp += s.self_damage_nominal_hp;
        flash_added += s.flash_added; flash_removed += s.flash_removed; flash_response_commands += s.flash_response_commands;
        darkcloud_added += s.darkcloud_added; darkcloud_removed += s.darkcloud_removed;
        chance_forced_hits += s.chance_forced_hits; counter_forced_hits += s.counter_forced_hits;
        hand_removal_resolved += s.hand_removal_resolved; hand_items_removed += s.hand_items_removed;
        used_removal_resolved += s.used_removal_resolved; used_miracles_removed += s.used_miracles_removed;
        removal_events += s.removal_events; empty_removal_events += s.empty_removal_events; removal_event_checksum += s.removal_event_checksum;
        mana_weapon_plays += s.mana_weapon_plays; mana_mp_spent += s.mana_mp_spent; mana_base_mp += s.mana_base_mp;
        repeat_weapon_plays += s.repeat_weapon_plays; repeat_frames_completed += s.repeat_frames_completed;
        wide_plays += s.wide_plays; wide_frames_completed += s.wide_frames_completed;
        random_hp_plays += s.random_hp_plays; random_hp_resolved += s.random_hp_resolved;
        random_hp_heals += s.random_hp_heals; random_hp_damages += s.random_hp_damages;
        random_hp_gained += s.random_hp_gained; random_hp_lost += s.random_hp_lost;
        revivals += s.revivals;
        danger_plays += s.danger_plays; danger_mortar_plays += s.danger_mortar_plays;
        danger_self_targets += s.danger_self_targets; danger_ally_targets += s.danger_ally_targets;
        exchange_plays += s.exchange_plays; zero_hp_exchanges += s.zero_hp_exchanges; exchanged_resource_total += s.exchanged_resource_total;
        sell_plays += s.sell_plays; sell_resolved += s.sell_resolved; sell_nominal_cp += s.sell_nominal_cp; sold_miracles += s.sold_miracles;
        buy_plays += s.buy_plays; buy_quotes += s.buy_quotes; purchase_commands += s.purchase_commands;
        bought += s.bought; declined += s.declined; buy_empty += s.buy_empty; buy_insufficient_cp += s.buy_insufficient_cp;
        buy_cutoff += s.buy_cutoff; buy_nominal_cp += s.buy_nominal_cp;
        disease_ticks += s.disease_ticks; disease_worsened += s.disease_worsened; disease_falls += s.disease_falls;
        disease_tick_revivals += s.disease_tick_revivals; disease_hp_gained += s.disease_hp_gained; disease_hp_lost += s.disease_hp_lost;
        fever_mask_uses += s.fever_mask_uses; heaven_herb_plays += s.heaven_herb_plays;
        heaven_herb_resolved += s.heaven_herb_resolved; heaven_herb_mp_gained += s.heaven_herb_mp_gained;
        fog_main_commands += s.fog_main_commands; fog_launches += s.fog_launches; fog_target_changed += s.fog_target_changed;
        death_bows += s.death_bows; death_created += s.death_created; death_started += s.death_started;
        death_rolls += s.death_rolls; death_hits += s.death_hits; death_forced_hits += s.death_forced_hits; death_responses += s.death_responses;
        sell_overflows += s.sell_overflows; buy_overflows += s.buy_overflows; overflow_events += s.overflow_events; overflow_event_checksum += s.overflow_event_checksum;
        dream_rolls += s.dream_rolls; dream_masks += s.dream_masks; dream_main_commands += s.dream_main_commands; dream_masked_commands += s.dream_masked_commands; dream_public_reveals += s.dream_public_reveals; dream_private_reveals += s.dream_private_reveals;
        redraws += s.redraws; redrawn_items += s.redrawn_items; redraw_notices += s.redraw_notices;
        buy_self_plays += s.buy_self_plays; buy_self_inspections += s.buy_self_inspections; buy_self_empty += s.buy_self_empty;
        guardian_summons += s.guardian_summons; guardian_opportunities += s.guardian_opportunities; auto_guardian_actions += s.auto_guardian_actions; guardian_departure_rolls += s.guardian_departure_rolls; guardian_departures += s.guardian_departures;
        auto_guardian_records += s.auto_guardian_records; guardian_change_records += s.guardian_change_records;
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
                        const auto summons_before=before.guardian_summons, opportunities_before=before.guardian_opportunities, auto_guardians_before=before.auto_guardian_actions;
                        const auto departure_rolls_before=before.guardian_departure_rolls, departures_before=before.guardian_departures;
                        const auto auto_records_before=before.phase==MatchPhase::main?0:before.last_auto_guardians.size();
                        const auto change_records_before=before.phase==MatchPhase::main?0:before.last_guardian_changes.size();
                        const auto redraws_before = before.redraws, redrawn_items_before = before.redrawn_items;
                        const auto redraw_notices_before = before.phase == MatchPhase::main ? 0 : before.last_redraws.size();
                        const auto dream_rolls_before = before.dream_rolls, dream_masks_before = before.dream_masks;
                        const auto reveals_before = before.phase == MatchPhase::main ? 0 : before.last_reveals.size();
                        const auto rolls_before = before.chance_rolls, hits_before = before.chance_hits;
                        const auto created_before = before.counter_created, started_before = before.counter_started;
                        const auto counter_rolls_before = before.counter_rolls, counter_hits_before = before.counter_hits;
                        const auto forced_before = before.chance_forced_hits, counter_forced_before = before.counter_forced_hits;
                        const auto revivals_before = before.revivals;
                        const auto tick_already_processed = before.batch && before.batch->tick_processed;
                        const auto death_bows_before = before.death_bows, death_created_before = before.death_created, death_started_before = before.death_started;
                        const auto death_rolls_before = before.death_rolls, death_hits_before = before.death_hits, death_forced_before = before.death_forced_hits;
                        if (command.kind == CommandKind::defend && !before.queue.empty() && before.queue.front().is_death) ++stats.death_responses;
                        std::array<bool, player_capacity> flashed_before{};
                        std::array<bool, player_capacity> clouded_before{};
                        for (std::size_t seat = 0; seat < count; ++seat) {
                            const auto& curses = before.state.players[seat].curses;
                            flashed_before[seat] = std::find(curses.begin(), curses.end(), Curse::flash) != curses.end();
                            clouded_before[seat] = std::find(curses.begin(), curses.end(), Curse::darkcloud) != curses.end();
                            if (before.phase == MatchPhase::response && before.state.players[seat].id == command.actor && flashed_before[seat])
                                ++stats.flash_response_commands;
                        }
                        std::optional<PlayerId> mp_counter_target;
                        std::int64_t mp_counter_gain = 0;
                        if (command.kind == CommandKind::defend && command.items.empty() && !before.queue.empty()) {
                            const auto& effect = before.queue.front();
                            if (effect.ability == Ability::counter_boost2x_mp) {
                                ++stats.mp_counter_resolved; mp_counter_target = effect.target;
                                mp_counter_gain = std::min<std::int64_t>(99-find_player(before.state, effect.target)->mp,effect.amount);
                            } else if (effect.ability == Ability::counter_take_cp) {
                                ++stats.cp_counter_resolved; stats.cp_counter_nominal += effect.amount;
                            }
                        }
                        const auto redirect_depth = before.queue.empty() ? 0 : before.queue.front().redirects.size();
                        bool bounced = false;
                        bool response_stops_payload = false;
                        bool response_redirected = false;
                        std::int64_t response_defense = 0;
                        auto incoming_element = before.queue.empty() ? Attribute{} : before.queue.front().element;
                        if (before.phase == MatchPhase::main) ++stats.main;
                        else if (before.phase == MatchPhase::purchase) ++stats.purchase_commands;
                        else ++stats.responses;
                        if (!before.queue.empty() && before.queue.front().element == Element::darkness) ++stats.dark_responses;
                        const auto& own = *find_player(before.state, command.actor);
                        const auto fog_main = before.phase == MatchPhase::main && std::find(own.curses.begin(), own.curses.end(), Curse::fog) != own.curses.end();
                        if (fog_main) ++stats.fog_main_commands;
                        if (before.phase == MatchPhase::main && std::find(own.curses.begin(), own.curses.end(), Curse::dream) != own.curses.end()) ++stats.dream_main_commands;
                        if (std::any_of(command.items.begin(), command.items.end(), [&](auto token) {
                            return std::any_of(own.items.begin(), own.items.end(), [&](const auto& item) { return item.token == token && item.fake_model.has_value(); });
                        })) ++stats.dream_masked_commands;
                        if (command.kind == CommandKind::sell) { ++stats.sell_plays; ++stats.physical; }
                        if (command.kind == CommandKind::buy) { ++stats.buy_plays; ++stats.physical; if (command.target == command.actor) ++stats.buy_self_plays; }
                        const auto had_purchase = before.purchase.has_value();
                        const auto buy_result_possible = (command.kind == CommandKind::buy && command.target == command.actor) || before.phase == MatchPhase::purchase
                            || (command.kind == CommandKind::defend && command.items.empty() && !before.queue.empty() && before.queue.front().ability == Ability::buy);
                        const auto sale_resolves = (command.kind == CommandKind::sell && command.target == command.actor)
                            || (command.kind == CommandKind::defend && command.items.empty() && !before.queue.empty() && before.queue.front().ability == Ability::sell);
                        if (command.kind == CommandKind::exchange) {
                            ++stats.exchange_plays; stats.exchanged_resource_total += own.hp + own.mp + own.cp;
                            if (command.allocation->hp == 0) ++stats.zero_hp_exchanges;
                        }
                        std::optional<PlayerId> removal_target;
                        Ability removal_ability = Ability::none;
                        if (command.kind == CommandKind::defend && command.items.empty() && !before.queue.empty()) {
                            removal_ability = before.queue.front().ability;
                            removal_target = before.queue.front().target;
                        } else if (command.kind == CommandKind::play && command.target == command.actor && !command.items.empty()) {
                            const auto item = std::find_if(own.items.begin(), own.items.end(), [&](const auto& v) { return v.token == command.items.front(); });
                            removal_ability = find_card(item->model)->ability;
                            removal_target = command.actor;
                        }
                        std::array<ItemToken, visible_hand_capacity> removal_tokens;
                        std::size_t removal_token_count = 0;
                        if (removal_ability == Ability::remove_items || removal_ability == Ability::remove_used_miracles) {
                            const auto& target = *find_player(before.state, *removal_target);
                            for (const auto& item : target.items) {
                                const auto eligible = removal_ability == Ability::remove_items ? !item.used
                                    : item.used && find_card(item.model)->category == Category::miracles;
                                // A self-used physical artifact is consumed before removal, not by removal.
                                const auto consumed = command.kind == CommandKind::play && command.actor == target.id
                                    && std::find(command.items.begin(), command.items.end(), item.token) != command.items.end();
                                if (eligible && !consumed) removal_tokens[removal_token_count++] = item.token;
                            }
                        } else removal_target.reset();
                        if (own.items.size() == before.config.hand_limit) ++stats.full_hands;
                        if (command.kind == CommandKind::pray) ++stats.prayers;
                        if (command.kind == CommandKind::discard) stats.discarded += command.items.size();
                        if (command.kind == CommandKind::defend) stats.defense_cards += command.items.size();
                        if (command.kind == CommandKind::play && command.target == command.actor && !command.items.empty()) {
                            const auto item = std::find_if(own.items.begin(), own.items.end(), [&](const auto& v) { return v.token == command.items[0]; });
                            const auto& card = *find_card(item->model);
                            if (card.atk > 0 || card.category == Category::weapons) ++stats.self_attacks;
                        }
                        int preceding_miracle_cost = 0;
                        bool wide_play = false;
                        bool danger_play = false;
                        if (command.kind == CommandKind::play || command.kind == CommandKind::defend || command.kind == CommandKind::exchange) for (auto token : command.items) {
                            const auto item = std::find_if(own.items.begin(), own.items.end(), [&](const auto& v) { return v.token == token; });
                            const auto& card = *find_card(item->model);
                            if (command.kind == CommandKind::defend && card.ability == Ability::self_curse) ++stats.fever_mask_uses;
                            if (command.kind == CommandKind::play && card.ability == Ability::attack_every_enemy) wide_play = true;
                            if (command.kind == CommandKind::play && token == command.items.front()) {
                                danger_play = card.ability == Ability::danger;
                                if (card.ability == Ability::boost_hp_or_deal_damage) ++stats.random_hp_plays;
                                if (card.ability == Ability::boost_mp_and_add_curse) ++stats.heaven_herb_plays;
                                if (card.ability == Ability::attack_twice) ++stats.repeat_weapon_plays;
                                if (card.ability == Ability::atk_by2x_mp) {
                                    ++stats.mana_weapon_plays; stats.mana_mp_spent += own.mp;
                                    std::array<CardId, visible_hand_capacity> models;
                                    for (std::size_t i = 0; i < command.items.size(); ++i) {
                                        const auto selected = std::find_if(own.items.begin(), own.items.end(), [&](const auto& value) { return value.token == command.items[i]; });
                                        models[i] = selected->model;
                                    }
                                    stats.mana_base_mp += own.mp - magic_cost(std::span<const CardId>{models.data(), command.items.size()});
                                }
                            }
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
                                if (reaction != Reaction::none) response_stops_payload = true;
                                else response_defense += card.defense;
                                if (reaction == Reaction::reflect || reaction == Reaction::bounce) {
                                    response_redirected = true;
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
                        if (wide_play) ++stats.wide_plays;
                        if (command.kind == CommandKind::defend && !response_redirected
                            && before.queue.front().ability == Ability::attack_twice) ++stats.repeat_frames_completed;
                        if (command.kind == CommandKind::defend && !response_redirected && !before.queue.front().is_counter
                            && std::any_of(before.queue.front().models.begin(), before.queue.front().models.end(), [](auto model) {
                                return find_card(model)->ability == Ability::attack_every_enemy;
                            })) ++stats.wide_frames_completed;
                        if (command.kind == CommandKind::defend && !response_stops_payload && incoming_element != Element::darkness) {
                            const auto& effect = before.queue.front();
                            const auto remaining = std::max<std::int64_t>(0, effect.power - response_defense);
                            if (remaining && effect.ability == Ability::absorb_hp) {
                                ++stats.absorb_positive_resolutions; stats.absorb_nominal_hp += remaining;
                            } else if (remaining && effect.ability == Ability::deal_same_damage) {
                                ++stats.self_damage_positive_resolutions; stats.self_damage_nominal_hp += remaining;
                            }
                        }
                        std::optional<PlayerId> random_hp_target;
                        if (command.kind == CommandKind::defend && !response_stops_payload
                            && before.queue.front().ability == Ability::boost_hp_or_deal_damage)
                            random_hp_target = before.queue.front().target;
                        else if (command.kind == CommandKind::play && command.target == command.actor && !command.items.empty()) {
                            const auto item = std::find_if(own.items.begin(), own.items.end(), [&](const auto& v) { return v.token == command.items.front(); });
                            if (find_card(item->model)->ability == Ability::boost_hp_or_deal_damage) random_hp_target = command.actor;
                        }
                        const auto random_hp_before = random_hp_target ? find_player(before.state, *random_hp_target)->hp : 0;
                        const auto random_hp_amount = !random_hp_target ? 0 : before.phase==MatchPhase::response
                            ? before.queue.front().amount : self_random_hp_amount(before.rng);
                        const auto random_hp_body_delta = std::clamp<std::int64_t>(random_hp_before+random_hp_amount,0,99)-random_hp_before;
                        std::optional<PlayerId> herb_target;
                        if (command.kind == CommandKind::defend && command.items.empty() && !before.queue.empty()
                            && before.queue.front().ability == Ability::boost_mp_and_add_curse) herb_target = before.queue.front().target;
                        else if (command.kind == CommandKind::play && command.target == command.actor && !command.items.empty()) {
                            const auto item = std::find_if(own.items.begin(), own.items.end(), [&](const auto& v) { return v.token == command.items.front(); });
                            if (find_card(item->model)->ability == Ability::boost_mp_and_add_curse) herb_target = command.actor;
                        }
                        const auto herb_mp_before = herb_target ? find_player(before.state, *herb_target)->mp : 0;
                        mix(hash, command.actor); mix(hash, static_cast<unsigned>(command.kind)); mix(hash, command.items.size());
                        for (auto token : command.items) mix(hash, token);
                        mix(hash, command.target.has_value()); if (command.target) mix(hash, *command.target);
                        mix(hash, command.allocation.has_value());
                        if (command.allocation) { mix(hash, command.allocation->hp); mix(hash, command.allocation->mp); mix(hash, command.allocation->cp); }
                        const auto step = match.step(command); ++stats.decisions;
                        stats.guardian_summons += match.snapshot().guardian_summons-summons_before;
                        stats.guardian_opportunities += match.snapshot().guardian_opportunities-opportunities_before;
                        stats.auto_guardian_actions += match.snapshot().auto_guardian_actions-auto_guardians_before;
                        stats.guardian_departure_rolls += match.snapshot().guardian_departure_rolls-departure_rolls_before;
                        stats.guardian_departures += match.snapshot().guardian_departures-departures_before;
                        stats.auto_guardian_records += match.snapshot().last_auto_guardians.size()-auto_records_before;
                        stats.guardian_change_records += match.snapshot().last_guardian_changes.size()-change_records_before;
                        stats.redraws += match.snapshot().redraws - redraws_before;
                        stats.redrawn_items += match.snapshot().redrawn_items - redrawn_items_before;
                        stats.redraw_notices += match.snapshot().last_redraws.size() - redraw_notices_before;
                        stats.dream_rolls += match.snapshot().dream_rolls - dream_rolls_before;
                        stats.dream_masks += match.snapshot().dream_masks - dream_masks_before;
                        for (std::size_t event = reveals_before; event < match.snapshot().last_reveals.size(); ++event) {
                            if (match.snapshot().last_reveals[event].owner_only) ++stats.dream_private_reveals;
                            else ++stats.dream_public_reveals;
                        }
                        stats.death_bows += match.snapshot().death_bows - death_bows_before;
                        stats.death_created += match.snapshot().death_created - death_created_before;
                        stats.death_started += match.snapshot().death_started - death_started_before;
                        stats.death_rolls += match.snapshot().death_rolls - death_rolls_before;
                        stats.death_hits += match.snapshot().death_hits - death_hits_before;
                        stats.death_forced_hits += match.snapshot().death_forced_hits - death_forced_before;
                        if (fog_main && match.snapshot().last_fog) {
                            ++stats.fog_launches;
                            if (match.snapshot().last_fog->intended != match.snapshot().last_fog->target) ++stats.fog_target_changed;
                        }
                        if (herb_target) { ++stats.heaven_herb_resolved; stats.heaven_herb_mp_gained += std::min<std::int64_t>(20,99-herb_mp_before); }
                        if (!had_purchase && match.snapshot().purchase) ++stats.buy_quotes;
                        if (buy_result_possible && match.snapshot().last_purchase) {
                            const auto& event = *match.snapshot().last_purchase;
                            if (event.overflow_model) {
                                ++stats.buy_overflows; ++stats.overflow_events; std::uint64_t event_hash = 14695981039346656037ULL;
                                mix(event_hash, 6); mix(event_hash, event.origin); mix(event_hash, event.buyer); mix(event_hash, event.seller);
                                mix(event_hash, event.model); mix(event_hash, *event.overflow_model); stats.overflow_event_checksum += event_hash;
                            }
                            switch (event.reason) {
                            case PurchaseReason::bought: ++stats.bought; stats.buy_nominal_cp += event.price; break;
                            case PurchaseReason::declined: ++stats.declined; break;
                            case PurchaseReason::empty: ++stats.buy_empty; if (event.buyer == event.seller) ++stats.buy_self_empty; break;
                            case PurchaseReason::insufficient_cp: ++stats.buy_insufficient_cp; break;
                            case PurchaseReason::cutoff: ++stats.buy_cutoff; break;
                            case PurchaseReason::self_inspection: ++stats.buy_self_inspections; break;
                            }
                        }
                        if (sale_resolves) {
                            if (!match.snapshot().last_sale) throw std::logic_error("resolved sale has no public result");
                            const auto& event = *match.snapshot().last_sale;
                            ++stats.sell_resolved; stats.sell_nominal_cp += event.price;
                            if (event.overflow_model) {
                                ++stats.sell_overflows; ++stats.overflow_events; std::uint64_t event_hash = 14695981039346656037ULL;
                                mix(event_hash, 5); mix(event_hash, event.origin); mix(event_hash, event.source); mix(event_hash, event.target);
                                mix(event_hash, event.model); mix(event_hash, *event.overflow_model); stats.overflow_event_checksum += event_hash;
                            }
                            if (find_card(event.model)->category == Category::miracles) ++stats.sold_miracles;
                        }
                        if (danger_play) {
                            ++stats.danger_plays;
                            const auto& event = *match.snapshot().last_danger;
                            if (event.mortar) ++stats.danger_mortar_plays;
                            if (event.origin == event.target) ++stats.danger_self_targets;
                            else {
                                const auto& target = *find_player(match.snapshot().state, event.target);
                                const auto& source = *find_player(match.snapshot().state, event.origin);
                                if (source.team != Team::solo && source.team == target.team) ++stats.danger_ally_targets;
                            }
                        }
                        const auto revived = match.snapshot().revivals - revivals_before;
                        stats.revivals += revived;
                        const auto& tick_record = match.snapshot().last_tick;
                        const auto* tick = tick_record && !tick_already_processed ? &*tick_record : nullptr;
                        const auto tick_revived = tick && tick->revival_token ? 1u : 0u;
                        if (tick) {
                            ++stats.disease_ticks;
                            if (tick->worsened) ++stats.disease_worsened;
                            if (tick->fallen) ++stats.disease_falls;
                            stats.disease_tick_revivals += tick_revived;
                            const auto delta = (tick_revived ? 0 : tick->hp_after) - tick->hp_before;
                            if (delta < 0) stats.disease_hp_lost += -delta; else stats.disease_hp_gained += delta;
                        }
                        if (random_hp_target) {
                            ++stats.random_hp_resolved;
                            const auto delta=random_hp_body_delta;
                            if (delta < 0) { ++stats.random_hp_damages; stats.random_hp_lost += -delta; }
                            else { ++stats.random_hp_heals; stats.random_hp_gained += delta; }
                        }
                        if (removal_target) {
                            const auto& after = *find_player(match.snapshot().state, *removal_target);
                            std::uint64_t removed = 0;
                            for (std::size_t i = 0; i < removal_token_count; ++i)
                                if (!(tick && tick->owner == *removal_target && tick->revival_token == removal_tokens[i])
                                    && std::none_of(after.items.begin(), after.items.end(), [&](const auto& item) { return item.token == removal_tokens[i]; })) ++removed;
                            // A same-action owner tick may consume112 after
                            // removal, before its posthumous responses. Those
                            // physical copies are not part of the public broom event.
                            const auto tick_bows = tick && tick->owner == *removal_target && after.dead && removal_ability == Ability::remove_items
                                ? match.snapshot().death_bows - death_bows_before : 0;
                            if (removed < tick_bows) throw std::logic_error("tick bow consumption exceeds missing removal copies");
                            removed -= tick_bows;
                            if (match.snapshot().auto_guardian_actions>auto_guardians_before)
                                removed=std::min<std::size_t>(removal_token_count,removal_ability==Ability::remove_items?3:2);
                            if (removal_ability == Ability::remove_items) { ++stats.hand_removal_resolved; stats.hand_items_removed += removed; }
                            else { ++stats.used_removal_resolved; stats.used_miracles_removed += removed; }
                            const auto& event = match.snapshot().last_removal;
                            if (!event || event->target != *removal_target || event->removed_models.size() != removed
                                || event->model != (removal_ability == Ability::remove_items ? 205 : 206))
                                throw std::logic_error("public removal event disagrees with resolved physical copies");
                            ++stats.removal_events;
                            if (event->removed_models.empty()) ++stats.empty_removal_events;
                            std::uint64_t event_hash = 14695981039346656037ULL;
                            mix(event_hash, event->origin); mix(event_hash, event->source); mix(event_hash, event->target);
                            mix(event_hash, event->model); mix(event_hash, event->removed_models.size());
                            for (auto model : event->removed_models) mix(event_hash, model);
                            stats.removal_event_checksum += event_hash;
                        }
                        for (std::size_t seat = 0; seat < count; ++seat) {
                            const auto& curses = match.snapshot().state.players[seat].curses;
                            const auto flashed = std::find(curses.begin(), curses.end(), Curse::flash) != curses.end();
                            if (flashed && !flashed_before[seat]) ++stats.flash_added;
                            if (!flashed && flashed_before[seat]) ++stats.flash_removed;
                            const auto clouded = std::find(curses.begin(), curses.end(), Curse::darkcloud) != curses.end();
                            if (clouded && !clouded_before[seat]) ++stats.darkcloud_added;
                            if (!clouded && clouded_before[seat]) ++stats.darkcloud_removed;
                        }
                        stats.chance_forced_hits += match.snapshot().chance_forced_hits - forced_before;
                        stats.counter_forced_hits += match.snapshot().counter_forced_hits - counter_forced_before;
                        stats.chance_rolls += match.snapshot().chance_rolls - rolls_before;
                        stats.chance_hits += match.snapshot().chance_hits - hits_before;
                        stats.counter_created += match.snapshot().counter_created - created_before;
                        stats.counter_started += match.snapshot().counter_started - started_before;
                        stats.counter_rolls += match.snapshot().counter_rolls - counter_rolls_before;
                        stats.counter_hits += match.snapshot().counter_hits - counter_hits_before;
                        if (mp_counter_target) stats.mp_counter_gained += mp_counter_gain;
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
        if (stats.games != games || stats.main + stats.responses + stats.purchase_commands != stats.decisions || wins + stats.draws + stats.truncations != games)
            throw std::logic_error("inconsistent scoped benchmark accounting");
        if (stats.random_hp_heals + stats.random_hp_damages != stats.random_hp_resolved || stats.random_hp_resolved > stats.random_hp_plays
            || stats.random_hp_gained > 10 * stats.random_hp_heals || stats.random_hp_lost > 10 * stats.random_hp_damages)
            throw std::logic_error("inconsistent random HP accounting");
        if (stats.removal_events != stats.hand_removal_resolved + stats.used_removal_resolved
            || stats.empty_removal_events > stats.removal_events)
            throw std::logic_error("inconsistent removal event accounting");
        if (stats.sell_resolved > stats.sell_plays || stats.sold_miracles > stats.sell_resolved)
            throw std::logic_error("inconsistent sale accounting");
        if (stats.bought + stats.declined != stats.purchase_commands || stats.purchase_commands > stats.buy_quotes
            || stats.buy_quotes > stats.buy_plays || stats.bought + stats.declined + stats.buy_empty + stats.buy_insufficient_cp + stats.buy_cutoff + stats.buy_self_inspections > stats.buy_plays)
            throw std::logic_error("inconsistent purchase accounting");
        if (stats.disease_ticks > stats.main || stats.disease_worsened > stats.disease_ticks || stats.disease_falls > stats.disease_worsened
            || stats.disease_tick_revivals > stats.revivals || stats.disease_hp_gained > 5 * stats.disease_ticks || stats.disease_hp_lost > 99 * stats.disease_ticks)
            throw std::logic_error("inconsistent disease accounting");
        if (stats.heaven_herb_resolved > stats.heaven_herb_plays || stats.heaven_herb_mp_gained > 20 * stats.heaven_herb_resolved)
            throw std::logic_error("inconsistent Heaven herb accounting");
        if (stats.fog_target_changed > stats.fog_launches || stats.fog_launches > stats.fog_main_commands || stats.fog_main_commands > stats.main)
            throw std::logic_error("inconsistent Fog selection accounting");
        if (stats.death_bows > stats.death_created || stats.death_created > (player_capacity - 1) * stats.death_bows
            || stats.death_hits > stats.death_rolls || stats.death_rolls > stats.death_created
            || stats.death_started != stats.death_hits + stats.death_forced_hits
            || stats.death_forced_hits > stats.death_created - stats.death_rolls || stats.death_responses < stats.death_started)
            throw std::logic_error("inconsistent posthumous accounting");
        if (stats.sell_overflows > stats.sell_resolved || stats.buy_overflows > stats.bought || stats.overflow_events != stats.sell_overflows + stats.buy_overflows)
            throw std::logic_error("inconsistent trade overflow accounting");
        if (stats.dream_masks > stats.dream_rolls || stats.dream_main_commands > stats.main
            || stats.dream_masked_commands > stats.decisions || stats.dream_public_reveals + stats.dream_private_reveals > stats.dream_masks)
            throw std::logic_error("inconsistent Dream mask and reveal accounting");
        if (stats.redraw_notices != stats.redraws || stats.redraws > visible_hand_capacity * stats.decisions
            || stats.redrawn_items > visible_hand_capacity * stats.redraws)
            throw std::logic_error("inconsistent immediate redraw accounting");
        if (stats.buy_self_inspections + stats.buy_self_empty != stats.buy_self_plays || stats.buy_self_plays > stats.buy_plays
            || stats.buy_self_empty > stats.buy_empty || stats.buy_quotes > stats.buy_plays - stats.buy_self_plays)
            throw std::logic_error("inconsistent self purchase inspection accounting");
        if (stats.guardian_summons>stats.main || stats.guardian_opportunities>player_capacity*stats.main
            || stats.auto_guardian_actions>stats.guardian_opportunities || stats.guardian_departures>stats.guardian_departure_rolls
            || stats.guardian_departure_rolls>2*stats.decisions || stats.auto_guardian_records!=stats.auto_guardian_actions
            || stats.guardian_change_records!=stats.guardian_summons+stats.guardian_departures)
            throw std::logic_error("inconsistent guardian lifecycle accounting");
        const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        std::cout << "ruleset=" << match_ruleset << "\nscope=" << match_scope << "\ncards=" << match_catalog().size()
            << "\ncatalog_version=" << catalog_version << "\ncatalog_sha256=" << catalog_sha256 << "\nclient_sha256=" << client_sha256
            << "\ngift_weight=" << match_gift_weight() << "\ngames=" << stats.games << "\nplayers=" << count << "\nseed=" << seed << "\nworkers=" << workers
            << "\nbounce_target_rule=" << bounce_target_rule
            << "\nchance_rule=" << chance_rule
            << "\ncounter_rule=" << counter_rule
            << "\nresource_counter_rule=" << resource_counter_rule
            << "\ndamage_effect_rule=" << damage_effect_rule
            << "\nflash_rule=" << flash_rule
            << "\ndarkcloud_rule=" << darkcloud_rule
            << "\nremoval_rule=" << removal_rule
            << "\nmana_weapon_rule=" << mana_weapon_rule
            << "\nrepeat_attack_rule=" << repeat_attack_rule
            << "\nwide_attack_rule=" << wide_attack_rule
            << "\nrandom_hp_rule=" << random_hp_rule
            << "\nrevival_rule=" << revival_rule
            << "\ndanger_rule=" << danger_rule
            << "\nexchange_rule=" << exchange_rule
            << "\nremoval_event_rule=" << removal_event_rule
            << "\nsell_rule=" << sell_rule
            << "\nbuy_rule=" << buy_rule
            << "\ndisease_rule=" << disease_rule
            << "\ndisease_item_rule=" << disease_item_rule
            << "\nfog_rule=" << fog_rule
            << "\ndeath_rule=" << death_rule
            << "\noverflow_rule=" << overflow_rule
            << "\ndream_rule=" << dream_rule
            << "\nredraw_rule=" << redraw_rule
            << "\nguardian_rule=" << guardian_rule
            << "\nguardian_summon_rule=" << guardian_summon_rule << "\nguardian_departure_rule=" << guardian_departure_rule << "\nguardian_payload_rule=" << guardian_payload_rule << "\nphenomenon_rule=" << phenomenon_rule << "\nitem_shuffle_rule=" << item_shuffle_rule << "\nconfusion_rule=" << confusion_rule << "\nfate_rule=" << fate_rule << "\ndevil_rule=" << devil_rule << "\napocalypse_rule=" << apocalypse_rule << "\ngrant_rule=" << grant_rule << "\ngrant_sampler_rule=" << grant_sampler_rule << "\nfate_pool_rule=" << fate_pool_rule << "\nfate_sampler_rule=" << fate_sampler_rule
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
            << "\nabsorb_positive_resolutions=" << stats.absorb_positive_resolutions << "\nabsorb_nominal_hp=" << stats.absorb_nominal_hp
            << "\nself_damage_positive_resolutions=" << stats.self_damage_positive_resolutions << "\nself_damage_nominal_hp=" << stats.self_damage_nominal_hp
            << "\nflash_added=" << stats.flash_added << "\nflash_removed=" << stats.flash_removed << "\nflash_response_commands=" << stats.flash_response_commands
            << "\ndarkcloud_added=" << stats.darkcloud_added << "\ndarkcloud_removed=" << stats.darkcloud_removed
            << "\nchance_forced_hits=" << stats.chance_forced_hits << "\ncounter_forced_hits=" << stats.counter_forced_hits
            << "\nhand_removal_resolved=" << stats.hand_removal_resolved << "\nhand_items_removed=" << stats.hand_items_removed
            << "\nused_removal_resolved=" << stats.used_removal_resolved << "\nused_miracles_removed=" << stats.used_miracles_removed
            << "\nremoval_events=" << stats.removal_events << "\nempty_removal_events=" << stats.empty_removal_events
            << "\nremoval_event_checksum=" << stats.removal_event_checksum
            << "\nmana_weapon_plays=" << stats.mana_weapon_plays << "\nmana_mp_spent=" << stats.mana_mp_spent << "\nmana_base_mp=" << stats.mana_base_mp
            << "\nrepeat_weapon_plays=" << stats.repeat_weapon_plays << "\nrepeat_frames_completed=" << stats.repeat_frames_completed
            << "\nwide_plays=" << stats.wide_plays << "\nwide_frames_completed=" << stats.wide_frames_completed
            << "\nrandom_hp_plays=" << stats.random_hp_plays << "\nrandom_hp_resolved=" << stats.random_hp_resolved
            << "\nrandom_hp_heals=" << stats.random_hp_heals << "\nrandom_hp_damages=" << stats.random_hp_damages
            << "\nrandom_hp_gained=" << stats.random_hp_gained << "\nrandom_hp_lost=" << stats.random_hp_lost
            << "\nrevivals=" << stats.revivals
            << "\ndanger_plays=" << stats.danger_plays << "\ndanger_mortar_plays=" << stats.danger_mortar_plays
            << "\ndanger_self_targets=" << stats.danger_self_targets << "\ndanger_ally_targets=" << stats.danger_ally_targets
            << "\nexchange_plays=" << stats.exchange_plays << "\nzero_hp_exchanges=" << stats.zero_hp_exchanges
            << "\nexchanged_resource_total=" << stats.exchanged_resource_total
            << "\nsell_plays=" << stats.sell_plays << "\nsell_resolved=" << stats.sell_resolved
            << "\nsell_nominal_cp=" << stats.sell_nominal_cp << "\nsold_miracles=" << stats.sold_miracles
            << "\nbuy_self_plays=" << stats.buy_self_plays << "\nbuy_self_inspections=" << stats.buy_self_inspections << "\nbuy_self_empty=" << stats.buy_self_empty
            << "\nbuy_plays=" << stats.buy_plays << "\nbuy_quotes=" << stats.buy_quotes << "\npurchase_commands=" << stats.purchase_commands
            << "\nbought=" << stats.bought << "\ndeclined=" << stats.declined << "\nbuy_empty=" << stats.buy_empty
            << "\nbuy_insufficient_cp=" << stats.buy_insufficient_cp << "\nbuy_cutoff=" << stats.buy_cutoff << "\nbuy_nominal_cp=" << stats.buy_nominal_cp
            << "\ndisease_ticks=" << stats.disease_ticks << "\ndisease_worsened=" << stats.disease_worsened
            << "\ndisease_falls=" << stats.disease_falls << "\ndisease_tick_revivals=" << stats.disease_tick_revivals
            << "\ndisease_hp_gained=" << stats.disease_hp_gained << "\ndisease_hp_lost=" << stats.disease_hp_lost
            << "\nfever_mask_uses=" << stats.fever_mask_uses << "\nheaven_herb_plays=" << stats.heaven_herb_plays
            << "\nheaven_herb_resolved=" << stats.heaven_herb_resolved << "\nheaven_herb_mp_gained=" << stats.heaven_herb_mp_gained
            << "\nfog_main_commands=" << stats.fog_main_commands << "\nfog_launches=" << stats.fog_launches << "\nfog_target_changed=" << stats.fog_target_changed
            << "\ndeath_bows=" << stats.death_bows << "\ndeath_created=" << stats.death_created << "\ndeath_started=" << stats.death_started
            << "\ndeath_rolls=" << stats.death_rolls << "\ndeath_hits=" << stats.death_hits << "\ndeath_misses=" << stats.death_rolls - stats.death_hits
            << "\ndeath_forced_hits=" << stats.death_forced_hits << "\ndeath_responses=" << stats.death_responses
            << "\nguardian_summons=" << stats.guardian_summons << "\nguardian_opportunities=" << stats.guardian_opportunities
            << "\nauto_guardian_actions=" << stats.auto_guardian_actions << "\nguardian_departure_rolls=" << stats.guardian_departure_rolls
            << "\nguardian_departures=" << stats.guardian_departures << "\nauto_guardian_records=" << stats.auto_guardian_records
            << "\nguardian_change_records=" << stats.guardian_change_records
            << "\nredraws=" << stats.redraws << "\nredrawn_items=" << stats.redrawn_items << "\nredraw_notices=" << stats.redraw_notices
            << "\ndream_rolls=" << stats.dream_rolls << "\ndream_masks=" << stats.dream_masks
            << "\ndream_main_commands=" << stats.dream_main_commands << "\ndream_masked_commands=" << stats.dream_masked_commands
            << "\ndream_public_reveals=" << stats.dream_public_reveals << "\ndream_private_reveals=" << stats.dream_private_reveals
            << "\nsell_overflows=" << stats.sell_overflows << "\nbuy_overflows=" << stats.buy_overflows
            << "\noverflow_events=" << stats.overflow_events << "\noverflow_event_checksum=" << stats.overflow_event_checksum
            << "\nseconds=" << seconds << "\ndecisions_per_second=" << (seconds ? stats.decisions / seconds : 0) << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
