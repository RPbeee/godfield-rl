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
constexpr std::array<CardId, 3> models{291, 292, 293};
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
    check(s.queue.empty() && !s.batch && s.counters.empty() && s.deaths.empty() && !s.purchase && !s.last_item_shuffle && !s.last_fate && !s.last_fate_event && !s.fate_actions && !s.last_devil && !s.last_devil_event && !s.devil_actions && !s.config.external_grants && !s.config.include_fate && !s.pending_grant && !s.grant_proof && !s.last_grant && !s.grant_resolutions && !s.grant_artifacts && !s.grant_devils,
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
    restored.restore(game.snapshot());
    check(restored.snapshot() == game.snapshot(), "targeted phenomenon checkpoint failed roundtrip");
}
struct Stats {
    std::uint64_t scenes=0, decisions=0, responses=0, terminated=0, truncated=0, checksum=0;
    std::uint64_t initial_world_responses=0, environmental_cancels=0, environmental_counter_cards=0;
    std::uint64_t real_frame_observations=0, source0_frame_observations=0;
    std::uint64_t chance_rolls=0, chance_hits=0, chance_forced=0;
    std::uint64_t counter_created=0, counter_started=0, counter_rolls=0, counter_hits=0, counter_forced=0;
    std::uint64_t death_bows=0, death_created=0, death_started=0, death_rolls=0, death_hits=0, death_forced=0;
    std::uint64_t revivals=0, redraws=0, redrawn_items=0, departure_rolls=0, departures=0;
    std::uint64_t owner0_scenes=0, recipient0_scenes=0, owner_recipient_same=0;
    std::array<std::uint64_t, models.size()> coverage{};
    void append(const Stats& other) {
#define ADD(name) name += other.name
        ADD(scenes); ADD(decisions); ADD(responses); ADD(terminated); ADD(truncated); ADD(checksum);
        ADD(initial_world_responses); ADD(environmental_cancels); ADD(environmental_counter_cards);
        ADD(real_frame_observations); ADD(source0_frame_observations);
        ADD(chance_rolls); ADD(chance_hits); ADD(chance_forced);
        ADD(counter_created); ADD(counter_started); ADD(counter_rolls); ADD(counter_hits); ADD(counter_forced);
        ADD(death_bows); ADD(death_created); ADD(death_started); ADD(death_rolls); ADD(death_hits); ADD(death_forced);
        ADD(revivals); ADD(redraws); ADD(redrawn_items); ADD(departure_rolls); ADD(departures);
        ADD(owner0_scenes); ADD(recipient0_scenes); ADD(owner_recipient_same);
#undef ADD
        for (std::size_t i=0;i<coverage.size();++i) coverage[i]+=other.coverage[i];
    }
};
bool known_player(const MatchSnapshot& snapshot, PlayerId id) {
    return std::any_of(snapshot.state.players.begin(), snapshot.state.players.end(),
                       [&](const auto& player) { return player.id==id; });
}
void inspect_frames(const Match& game, Stats& stats, const PhenomenonAction& action) {
    const auto& s=game.snapshot();
    const auto inspect=[&](const EffectFrame& effect, bool active) {
        check(known_player(s,effect.target), "frame target is outside the actual roster");
        if (effect.environmental) {
            check(effect.source==0 && effect.origin==0 && !effect.is_counter && !effect.is_death,
                  "world frame source slots were interpreted as player0");
            check(effect.category==Category::phenomena && effect.models==std::vector<CardId>{action.model}
                  && effect.tokens.empty() && effect.target==*action.recipient,
                  "world frame has a fabricated identity or changed explicit recipient");
            if (active) {
                const auto public_view=game.observe(effect.target);
                check(public_view.find("\"origin\":null,\"source\":null,")!=std::string::npos
                      && public_view.find("\"environmental\":true")!=std::string::npos,
                      "world pending view exposed player0 as a source");
            }
        } else {
            check(known_player(s,effect.source) && known_player(s,effect.origin),
                  "derived frame has no actual player source");
            check(effect.is_counter || effect.is_death, "external scene generated a non-derived player frame");
            if (active) {
                ++stats.real_frame_observations;
                stats.source0_frame_observations+=effect.source==0;
            }
        }
    };
    for (const auto& effect:s.queue) inspect(effect,true);
    for (const auto& effect:s.counters) inspect(effect,false);
    for (const auto& effect:s.deaths) inspect(effect,false);
}
}
int main(int argc, char** argv) { try {
    if (argc>5) throw std::invalid_argument("usage: godfield_targeted_phenomenon_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000;
    const auto players=argc>2?number(argv[2]):2;
    const auto seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if (count>1000000000 || players<2 || players>9 || workers>64 || seed>UINT64_MAX-(count-1))
        throw std::invalid_argument("targeted phenomenon benchmark bounds exceeded");
    workers=std::min(workers,count);
    std::vector<Stats> results(workers);
    std::vector<std::exception_ptr> errors(workers);
    std::vector<std::jthread> threads;
    const auto start=std::chrono::steady_clock::now();
    for (std::uint64_t worker=0;worker<workers;++worker) threads.emplace_back([&,worker] { try {
        MatchConfig config; config.max_decisions=64;
        Match game(config), restored(config);
        auto& stats=results[worker];
        for (std::uint64_t i=worker;i<count;i+=workers) {
            const auto column=static_cast<std::size_t>(i%models.size());
            SnapshotState scene; scene.gf=17;
            for (std::uint64_t seat=0;seat<players;++seat) {
                PlayerState player; player.id=seat==0?0:100+7*seat;
                player.name="P"+std::to_string(seat);
                player.hp=15+static_cast<std::int64_t>((seed+i+17*seat)%85);
                player.mp=99; player.cp=static_cast<std::int64_t>((seed+i+seat)%100);
                player.guardian=static_cast<Guardian>(1+(seed+i+seat)%10);
                // Canonical enum order. These are exogenous scene parameters,
                // not an implementation of a server phenomenon selector.
                if ((i+seat)%4==0) player.curses.push_back(Curse::cold);
                if ((i+seat)%3==0) player.curses.push_back(Curse::fog);
                if ((i+seat)%2==0) player.curses.push_back(Curse::darkcloud);
                for (auto model:{CardId{190},CardId{189},CardId{178},CardId{184},CardId{187},
                                 CardId{188},CardId{208},CardId{112},CardId{235}})
                    player.items.push_back({player.items.size(),model,{},false});
                scene.players.push_back(std::move(player));
            }
            // Independent cycles cover ID0 both as owner and as recipient.
            const auto owner_seat=(i/3)%players;
            const auto recipient_seat=(i/3/players+i%3)%players;
            const auto owner=scene.players[owner_seat].id;
            const auto recipient=scene.players[recipient_seat].id;
            const PhenomenonAction action{models[column],recipient,{}};
            game.load_scene(std::move(scene),owner,seed+i);
            check(game.snapshot().state.gf==17 && game.snapshot().rng==seed+i,
                  "scene load changed requested GF or RNG");
            check(game.phenomenon_legal(action), "targeted phenomenon scenario is not legal");
            game.phenomenon_step(action); audit(game,restored);
            const auto& launched=game.snapshot();
            check(launched.decisions==1 && launched.phenomenon_actions==1,
                  "external injection did not count exactly one decision");
            check(launched.last_phenomenon==action, "targeted launch lost its public event");
            if (!launched.queue.empty()) {
                check(launched.phase==MatchPhase::response && launched.queue.front().environmental,
                      "initial targeted pending is not a world frame");
                ++stats.initial_world_responses;
            } else check(models[column]==292 && launched.phase==MatchPhase::main,
                         "only a missed percentage launch may have no initial response");
            inspect_frames(game,stats,action);
            std::uint64_t policy=(seed+i)^0xbb67ae8584caa73bULL;
            while (!game.done() && game.snapshot().phase!=MatchPhase::main) {
                check(game.snapshot().phase==MatchPhase::response && game.snapshot().queue.size()==1,
                      "targeted scene entered a non-response pending phase");
                const auto command=game.sample_action(policy);
                check(game.legal(command) && command.kind==CommandKind::defend,
                      "visible response sampler produced an illegal command");
                const auto& before=game.snapshot();
                const bool world=before.queue.front().environmental;
                bool cancel=false;
                std::uint64_t source_directed=0, self_mp_cards=0;
                const auto counters_before=before.counter_created;
                for (auto token:command.items) {
                    const auto& hand=find_player(before.state,command.actor)->items;
                    const auto item=std::find_if(hand.begin(),hand.end(),[&](const auto& candidate) { return candidate.token==token; });
                    check(item!=hand.end(), "sampled response token is not owned");
                    if (world) {
                        cancel|=item->model==190;
                        const auto ability=find_card(item->model)->ability;
                        self_mp_cards+=ability==Ability::counter_boost2x_mp;
                        source_directed+=ability==Ability::counter_atk || ability==Ability::counter2x_atk
                            || ability==Ability::counter_curse || ability==Ability::counter_take_cp;
                    }
                }
                game.step(command); audit(game,restored); ++stats.responses;
                if (world) check(game.snapshot().counter_created-counters_before<=self_mp_cards,
                                 "source-free body created a source-directed counter");
                stats.environmental_cancels+=cancel;
                stats.environmental_counter_cards+=source_directed;
                inspect_frames(game,stats,action);
            }
            const auto& final=game.snapshot();
            check(final.phenomenon_actions==1 && final.guardian_actions==0 && final.guardian_payload_actions==0
                  && final.guardian_opportunities==0 && final.auto_guardian_actions==0 && final.guardian_summons==0,
                  "external scene ran an ordinary guardian scheduler or summon");
            check(final.state.gf==17 && !final.last_tick,
                  "external scene advanced GF or ticked its ordinary owner");
            check(final.last_phenomenon==action, "responses cleared the external phenomenon notice");
            ++stats.scenes; ++stats.coverage[column];
            stats.owner0_scenes+=owner==0; stats.recipient0_scenes+=recipient==0;
            stats.owner_recipient_same+=owner==recipient;
#define COUNT(name) stats.name+=final.name
            COUNT(decisions); COUNT(chance_rolls); COUNT(chance_hits); COUNT(counter_created); COUNT(counter_started);
            COUNT(counter_rolls); COUNT(counter_hits); COUNT(death_bows); COUNT(death_created); COUNT(death_started);
            COUNT(death_rolls); COUNT(death_hits); COUNT(revivals); COUNT(redraws); COUNT(redrawn_items);
#undef COUNT
            stats.chance_forced+=final.chance_forced_hits;
            stats.counter_forced+=final.counter_forced_hits; stats.death_forced+=final.death_forced_hits;
            stats.departure_rolls+=final.guardian_departure_rolls; stats.departures+=final.guardian_departures;
            stats.terminated+=final.terminated; stats.truncated+=final.truncated;
            auto hash=state_hash(final); mix(hash,i); mix(hash,models[column]); mix(hash,owner); mix(hash,recipient);
            stats.checksum+=hash;
        }
    } catch (...) { errors[worker]=std::current_exception(); } });
    for (auto& thread:threads) thread.join();
    for (const auto& error:errors) if (error) std::rethrow_exception(error);
    Stats total; for (const auto& stats:results) total.append(stats);
    unsigned covered=0; std::uint64_t model_scenes=0;
    for (auto n:total.coverage) { covered+=n>0; model_scenes+=n; }
    check(total.scenes==count && model_scenes==count && covered==std::min<std::uint64_t>(count,3)
          && total.decisions==count+total.responses && total.terminated+total.truncated<=count,
          "targeted phenomenon decision/model accounting failed");
    check(total.chance_hits<=total.chance_rolls && total.counter_hits<=total.counter_rolls
          && total.counter_started<=total.counter_created && total.counter_rolls<=total.counter_created
          && total.counter_forced<=total.counter_created-total.counter_rolls
          && total.counter_hits+total.counter_forced<=total.counter_started,
          "percentage accounting failed");
    check(total.death_hits<=total.death_rolls && total.death_started==total.death_hits+total.death_forced
          && total.environmental_cancels<=total.initial_world_responses,
          "death or world cancellation accounting failed");
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset
        <<"\nscope=experimental:external-targeted-source-free-phenomena291-292-293;explicit-scene-recipients;190-cancels;source-directed-counters-skipped;not-server-selector"
        <<"\nphenomenon_rule="<<phenomenon_rule << "\nitem_shuffle_rule=" << item_shuffle_rule << "\nconfusion_rule=" << confusion_rule << "\nfate_rule=" << fate_rule << "\ndevil_rule=" << devil_rule << "\napocalypse_rule=" << apocalypse_rule << "\ngrant_rule=" << grant_rule << "\ngrant_sampler_rule=" << grant_sampler_rule << "\nfate_pool_rule=" << fate_pool_rule << "\nfate_sampler_rule=" << fate_sampler_rule << "\nconfusion_sampler_rule=" << confusion_sampler_rule << "\nguardian_payload_sampler_rule=" << guardian_payload_sampler_rule << "\nguardian_payload_auto_rule=" << guardian_payload_auto_rule<<"\nscenes="<<total.scenes<<"\nplayers="<<players
        <<"\nseed="<<seed<<"\nworkers="<<workers<<"\nmodels_covered="<<covered;
#define OUTPUT(name) std::cout<<"\n" #name "="<<total.name
    OUTPUT(decisions); OUTPUT(responses); OUTPUT(terminated); OUTPUT(truncated); OUTPUT(initial_world_responses);
    OUTPUT(environmental_cancels); OUTPUT(environmental_counter_cards); OUTPUT(real_frame_observations); OUTPUT(source0_frame_observations);
    OUTPUT(owner0_scenes); OUTPUT(recipient0_scenes); OUTPUT(owner_recipient_same);
    OUTPUT(chance_rolls); OUTPUT(chance_hits); OUTPUT(chance_forced);
    OUTPUT(counter_created); OUTPUT(counter_started); OUTPUT(counter_rolls); OUTPUT(counter_hits); OUTPUT(counter_forced);
    OUTPUT(death_bows); OUTPUT(death_created); OUTPUT(death_started); OUTPUT(death_rolls); OUTPUT(death_hits); OUTPUT(death_forced);
    OUTPUT(revivals); OUTPUT(redraws); OUTPUT(redrawn_items); OUTPUT(departure_rolls); OUTPUT(departures); OUTPUT(checksum);
#undef OUTPUT
    std::cout<<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';
    return 0;
} catch (const std::exception& error) {
    std::cerr<<"targeted phenomenon benchmark: "<<error.what()<<'\n'; return 1;
} }
