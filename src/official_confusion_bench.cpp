#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
using namespace godfield::official;
void check(bool ok, const char* message) { if (!ok) throw std::logic_error(message); }
std::uint64_t number(const char* text, bool positive=true) {
    std::uint64_t value=0; const std::string input(text);
    const auto r=std::from_chars(input.data(),input.data()+input.size(),value);
    if (r.ec!=std::errc{} || r.ptr!=input.data()+input.size() || (positive && !value))
        throw std::invalid_argument("invalid numeric argument");
    return value;
}
void mix(std::uint64_t& hash, std::uint64_t value) {
    for (unsigned byte=0;byte!=8;++byte) { hash^=(value>>(byte*8))&255; hash*=1099511628211ULL; }
}
void text_hash(std::uint64_t& hash, const std::string& text) {
    mix(hash,text.size()); for (unsigned char c:text) { hash^=c; hash*=1099511628211ULL; }
}
void item_hash(std::uint64_t& hash, const ItemInstance& item) {
    mix(hash,item.token); mix(hash,item.model); mix(hash,item.used);
    mix(hash,item.fake_model.has_value()); if (item.fake_model) mix(hash,*item.fake_model);
}
// Deliberately a world/slot and selected-counter hash, not a whole-snapshot
// hash: Confusion markers and pending journals are checked by full restore.
std::uint64_t world_hash(const MatchSnapshot& s) {
    std::uint64_t hash=14695981039346656037ULL;
    mix(hash,s.version); mix(hash,s.state.gf); mix(hash,s.state.update_count); mix(hash,s.state.is_over);
    mix(hash,s.state.players.size());
    for (const auto& player:s.state.players) {
        mix(hash,player.id); text_hash(hash,player.name);
        mix(hash,player.user_id.has_value()); if (player.user_id) text_hash(hash,*player.user_id);
        mix(hash,player.alias.has_value()); if (player.alias) text_hash(hash,*player.alias);
        mix(hash,static_cast<unsigned>(player.team)); mix(hash,player.hp); mix(hash,player.mp); mix(hash,player.cp);
        mix(hash,player.dead); mix(hash,static_cast<unsigned>(player.guardian)); mix(hash,player.confusion_count);
        mix(hash,player.curses.size()); for (auto curse:player.curses) mix(hash,static_cast<unsigned>(curse));
        mix(hash,player.items.size()); for (const auto& item:player.items) item_hash(hash,item);
    }
    mix(hash,s.owner); mix(hash,s.actor.has_value()); if (s.actor) mix(hash,*s.actor);
    mix(hash,static_cast<unsigned>(s.phase)); mix(hash,s.rng); mix(hash,s.next_token); mix(hash,s.decisions);
    mix(hash,s.terminated); mix(hash,s.truncated); mix(hash,s.phenomenon_actions);
    mix(hash,s.chance_rolls); mix(hash,s.chance_hits); mix(hash,s.chance_forced_hits);
    mix(hash,s.counter_created); mix(hash,s.counter_started); mix(hash,s.counter_rolls); mix(hash,s.counter_hits); mix(hash,s.counter_forced_hits);
    mix(hash,s.death_bows); mix(hash,s.death_created); mix(hash,s.death_started); mix(hash,s.death_rolls); mix(hash,s.death_hits); mix(hash,s.death_forced_hits);
    mix(hash,s.revivals); mix(hash,s.redraws); mix(hash,s.redrawn_items); mix(hash,s.dream_rolls); mix(hash,s.dream_masks);
    mix(hash,s.guardian_summons); mix(hash,s.guardian_opportunities); mix(hash,s.auto_guardian_actions);
    mix(hash,s.guardian_departure_rolls); mix(hash,s.guardian_departures);
    return hash;
}

void audit(const Match& game, Match& restored) {
    restored.restore(game.snapshot());
    check(restored.snapshot()==game.snapshot(),"confusion snapshot failed full roundtrip");
}
// Only this privileged test controller removes a count to propose a command.
// It is deliberately not an implementation of the official automatic selector.
Command proposal(const Match& game, std::uint64_t& policy) {
    auto state=game.snapshot(); const auto actor=*state.actor;
    for (auto& player:state.state.players) if (player.id==actor) player.confusion_count=0;
    state.last_phenomenon.reset(); state.last_item_shuffle.reset();
    Match controller(state.config); controller.restore(state);
    return controller.sample_action(policy);
}
void count_transition(const MatchSnapshot& before, const MatchSnapshot& after,
                      std::optional<PlayerId> debited) {
    for (const auto& old:before.state.players) {
        const auto* now=find_player(after.state,old.id); check(now!=nullptr,"count transition changed roster");
        auto expected=old.confusion_count;
        if (debited && *debited==old.id) { check(expected>0,"confusion launch debited zero count"); --expected; }
        if (now->dead || after.terminated) expected=0;
        check(now->confusion_count==expected,"count changed outside launch/death/natural-end boundary");
    }
}
struct Stats {
    std::uint64_t scenes=0,phenomenon_actions=0,main_launches=0,confusion_commands=0,normal_main_commands=0;
    std::uint64_t responses=0,purchase_commands=0,decisions=0,terminated=0,truncated=0;
    std::uint64_t confusion_points_added=0,max_count=0,confusion_launch_debits=0,pending_confusion_anchors=0;
    std::uint64_t sampler_rejections=0,normal_step_rejections=0,dead_input_scenes=0;
    std::uint64_t primitive_checksum=0,checksum=0,chance_rolls=0,chance_hits=0,chance_forced=0;
    std::uint64_t counter_created=0,counter_started=0,counter_rolls=0,counter_hits=0,counter_forced=0;
    std::uint64_t death_bows=0,death_created=0,death_started=0,death_rolls=0,death_hits=0,death_forced=0;
    std::uint64_t revivals=0,redraws=0,redrawn_items=0,disease_ticks=0;
    std::uint64_t guardian_opportunities=0,auto_guardian_actions=0,departure_rolls=0,departures=0;
    std::array<std::uint64_t,2> pulses{};
    void append(const Stats& other) {
#define ADD(name) name+=other.name
        ADD(scenes); ADD(phenomenon_actions); ADD(main_launches); ADD(confusion_commands); ADD(normal_main_commands);
        ADD(responses); ADD(purchase_commands); ADD(decisions); ADD(terminated); ADD(truncated);
        ADD(confusion_points_added); ADD(confusion_launch_debits); ADD(pending_confusion_anchors);
        ADD(sampler_rejections); ADD(normal_step_rejections); ADD(dead_input_scenes);
        ADD(primitive_checksum); ADD(checksum); ADD(chance_rolls); ADD(chance_hits); ADD(chance_forced);
        ADD(counter_created); ADD(counter_started); ADD(counter_rolls); ADD(counter_hits); ADD(counter_forced);
        ADD(death_bows); ADD(death_created); ADD(death_started); ADD(death_rolls); ADD(death_hits); ADD(death_forced);
        ADD(revivals); ADD(redraws); ADD(redrawn_items); ADD(disease_ticks);
        ADD(guardian_opportunities); ADD(auto_guardian_actions); ADD(departure_rolls); ADD(departures);
#undef ADD
        max_count=std::max(max_count,other.max_count);
        for (std::size_t i=0;i<pulses.size();++i) pulses[i]+=other.pulses[i];
    }
};
void verify_addition(const MatchSnapshot& before, const MatchSnapshot& after, Stats& stats) {
    auto expected=before.state; ++expected.update_count;
    for (auto& player:expected.players) if (!player.dead) {
        player.confusion_count+=3; stats.confusion_points_added+=3;
        stats.max_count=std::max(stats.max_count,static_cast<std::uint64_t>(player.confusion_count));
    }
    check(after.state==expected && after.decisions==before.decisions+1 && after.rng==before.rng
          && after.next_token==before.next_token && after.owner==before.owner && after.actor==before.actor
          && after.phase==MatchPhase::main && after.phenomenon_actions==before.phenomenon_actions+1,
          "289 changed more than alive count/update/decision accounting");
    check(after.gifts==before.gifts && after.last_phenomenon==PhenomenonAction{289,{},{}}
          && !after.batch && after.queue.empty() && after.counters.empty() && after.deaths.empty()
          && after.guardian_actions==0 && after.guardian_payload_actions==0 && after.guardian_opportunities==0
          && after.auto_guardian_actions==0 && !after.last_tick && after.revivals==0 && after.redraws==0,
          "289 ran an ordinary body/schedule or requested gifts");
}
}
int main(int argc,char** argv) { try {
    if (argc>5) throw std::invalid_argument("usage: godfield_confusion_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000,players=argc>2?number(argv[2]):2,seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if (count>1000000000 || players<2 || players>9 || workers>64 || seed>UINT64_MAX-(count-1))
        throw std::invalid_argument("confusion benchmark bounds exceeded");
    workers=std::min(workers,count);
    check(match_dream_compatible(6,7),"fixture fake is not compatible");
    std::vector<Stats> results(workers); std::vector<std::exception_ptr> errors(workers); std::vector<std::jthread> threads;
    const auto start=std::chrono::steady_clock::now();
    for (std::uint64_t worker=0;worker<workers;++worker) threads.emplace_back([&,worker] { try {
        MatchConfig config; config.max_decisions=64; Match game(config),restored(config); auto& stats=results[worker];
        constexpr std::array<CardId,18> cards{6,235,235,208,112,190,178,184,187,188,3,4,5,203,236,239,66,58};
        for (std::uint64_t i=worker;i<count;i+=workers) {
            SnapshotState state; state.gf=17;
            const bool dead_last=players==9 && i%17==0;
            for (std::uint64_t seat=0;seat<players;++seat) {
                PlayerState player; player.id=seat==0?0:100+7*seat; player.name="P"+std::to_string(seat);
                player.user_id="user"+std::to_string(seat); player.alias="A"+std::to_string(seat);
                player.team=i%3==0?(seat%2?Team::two:Team::one):Team::solo;
                player.hp=player.mp=99; player.cp=(seed+i+seat)%100;
                player.guardian=static_cast<Guardian>(1+(seed+i+seat)%10);
                if ((i+seat)%4==0) player.curses.push_back(Curse::cold);
                if ((i+seat)%3==0) player.curses.push_back(Curse::fog);
                if ((i+seat)%2==0) player.curses.push_back(Curse::dream);
                player.dead=dead_last && seat+1==players; if (player.dead) player.hp=0;
                player.confusion_count=player.dead?0:static_cast<std::int64_t>((i+seat)%3);
                const auto size=4+(seed+i+seat)%15;
                for (std::uint64_t slot=0;slot<size;++slot) {
                    ItemInstance item{slot,cards[slot],{},slot==2};
                    if (slot==0 && (i+seat)%2==0) item.fake_model=7;
                    player.items.push_back(item);
                }
                state.players.push_back(std::move(player));
            }
            game.load_scene(std::move(state),0,seed+i); audit(game,restored);
            const auto initial_alive=alive_players(game.snapshot().state).size();
            const auto pulses=1+i%2;
            for (std::uint64_t pulse=0;pulse<pulses;++pulse) {
                const auto before=game.snapshot(); const PhenomenonAction action{289,{},{}};
                check(game.phenomenon_legal(action),"289 fixture not legal");
                game.phenomenon_step(action); audit(game,restored); verify_addition(before,game.snapshot(),stats);
            }
            ++stats.pulses[pulses-1]; stats.phenomenon_actions+=pulses; stats.dead_input_scenes+=dead_last;
            auto primitive=world_hash(game.snapshot()); mix(primitive,i); stats.primitive_checksum+=primitive;
            std::uint64_t policy=(seed+i)^0xa54ff53a5f1d36f1ULL;
            for (std::size_t launch=0;launch<initial_alive*3 && !game.done();++launch) {
                const auto before=game.snapshot();
                check(before.phase==MatchPhase::main && before.actor==before.owner,"controller did not reach a main boundary");
                const auto actor=*before.actor; const auto old_count=find_player(before.state,actor)->confusion_count;
                const bool confused=old_count>0;
                const auto command=confused?proposal(game,policy):game.sample_action(policy);
                check(game.snapshot()==before,"external test proposal mutated the live match");
                check(command.actor==actor && command.kind!=CommandKind::defend
                      && command.kind!=CommandKind::accept && command.kind!=CommandKind::decline,
                      "controller proposed a non-main command");
                if (confused) {
                    check(!game.legal(command) && game.confusion_legal(command),"confusion did not gate normal main API");
                    auto rejected_policy=policy; bool rejected=false;
                    try { (void)game.sample_action(rejected_policy); } catch (const MatchError&) { rejected=true; }
                    check(rejected && rejected_policy==policy && game.snapshot()==before,"sampler rejection changed live state or policy RNG");
                    ++stats.sampler_rejections; rejected=false;
                    try { game.step(command); } catch (const MatchError&) { rejected=true; }
                    check(rejected && game.snapshot()==before,"ordinary step bypassed confusion or rejected nonatomically");
                    ++stats.normal_step_rejections;
                    game.confusion_step(command); ++stats.confusion_commands; ++stats.confusion_launch_debits;
                } else {
                    check(game.legal(command) && !game.confusion_legal(command),"unconfused main did not use normal API exclusively");
                    game.step(command); ++stats.normal_main_commands;
                }
                audit(game,restored); count_transition(before,game.snapshot(),confused?std::optional<PlayerId>{actor}:std::nullopt);
                ++stats.main_launches;
                if (game.snapshot().batch) {
                    check(game.snapshot().batch->confusion_command==confused
                          && find_player(game.snapshot().batch->initial_state,actor)->confusion_count==old_count,
                          "pending batch lost its pre-debit count or confusion marker");
                    stats.pending_confusion_anchors+=confused;
                }
                while (!game.done() && game.snapshot().phase!=MatchPhase::main) {
                    const auto pending=game.snapshot(); const auto response=game.sample_action(policy);
                    check(game.legal(response) && !game.confusion_legal(response),"pending command used confusion main API");
                    if (pending.phase==MatchPhase::response) { check(response.kind==CommandKind::defend,"response proposed non-defense"); ++stats.responses; }
                    else { check(pending.phase==MatchPhase::purchase && (response.kind==CommandKind::accept || response.kind==CommandKind::decline),"unexpected pending phase"); ++stats.purchase_commands; }
                    game.step(response); audit(game,restored); count_transition(pending,game.snapshot(),{});
                    if (game.snapshot().batch) check(game.snapshot().batch->confusion_command==confused,
                                                    "response changed originating confusion marker");
                }
                stats.disease_ticks+=game.snapshot().last_tick.has_value();
            }
            const auto& final=game.snapshot();
            check(final.phenomenon_actions==pulses && !final.guardian_actions && !final.guardian_payload_actions,
                  "ordinary command invented an external effect activation");
            for (const auto& player:final.state.players) {
                check(player.confusion_count>=0 && (!player.dead || player.confusion_count==0)
                      && (!final.terminated || player.confusion_count==0),"dead/end count did not clear");
                stats.max_count=std::max(stats.max_count,static_cast<std::uint64_t>(player.confusion_count));
            }
            ++stats.scenes; stats.decisions+=final.decisions; stats.terminated+=final.terminated; stats.truncated+=final.truncated;
#define COUNT(name) stats.name+=final.name
            COUNT(chance_rolls); COUNT(chance_hits); COUNT(counter_created); COUNT(counter_started); COUNT(counter_rolls); COUNT(counter_hits);
            COUNT(death_bows); COUNT(death_created); COUNT(death_started); COUNT(death_rolls); COUNT(death_hits);
            COUNT(revivals); COUNT(redraws); COUNT(redrawn_items); COUNT(guardian_opportunities); COUNT(auto_guardian_actions);
#undef COUNT
            stats.chance_forced+=final.chance_forced_hits; stats.counter_forced+=final.counter_forced_hits; stats.death_forced+=final.death_forced_hits;
            stats.departure_rolls+=final.guardian_departure_rolls; stats.departures+=final.guardian_departures;
            auto hash=world_hash(final); mix(hash,i); stats.checksum+=hash;
        }
    } catch (...) { errors[worker]=std::current_exception(); } });
    for (auto& thread:threads) thread.join(); for (const auto& error:errors) if (error) std::rethrow_exception(error);
    Stats total; for (const auto& stats:results) total.append(stats);
    check(total.scenes==count && total.pulses[0]+total.pulses[1]==count
          && total.phenomenon_actions==total.pulses[0]+2*total.pulses[1]
          && total.main_launches==total.confusion_commands+total.normal_main_commands
          && total.confusion_commands==total.sampler_rejections && total.confusion_commands==total.normal_step_rejections
          && total.confusion_launch_debits==total.confusion_commands
          && total.decisions==total.phenomenon_actions+total.main_launches+total.responses+total.purchase_commands
          && total.chance_hits<=total.chance_rolls && total.counter_hits<=total.counter_rolls
          && total.death_hits<=total.death_rolls && total.terminated+total.truncated<=count,
          "confusion benchmark accounting failed");
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset<<"\nscope=experimental:external289-and-bounded-command-resolution;privileged-unconfused-clone-proposals-not-official-selector;world-count-and-selected-counter-hashes-plus-full-restore-audit"
             <<"\nconfusion_rule="<<confusion_rule<<"\nfate_rule="<<fate_rule<<"\ndevil_rule="<<devil_rule<<"\napocalypse_rule="<<apocalypse_rule<<"\ngrant_rule="<<grant_rule<<"\ngrant_sampler_rule="<<grant_sampler_rule<<"\nfate_pool_rule="<<fate_pool_rule<<"\nphenomenon_rule="<<phenomenon_rule
             <<"\nscenes="<<total.scenes<<"\nplayers="<<players<<"\nseed="<<seed<<"\nworkers="<<workers;
#define OUTPUT(name) std::cout<<"\n" #name "="<<total.name
    OUTPUT(phenomenon_actions); OUTPUT(main_launches); OUTPUT(confusion_commands); OUTPUT(normal_main_commands);
    OUTPUT(responses); OUTPUT(purchase_commands); OUTPUT(decisions); OUTPUT(terminated); OUTPUT(truncated);
    OUTPUT(confusion_points_added); OUTPUT(max_count); OUTPUT(confusion_launch_debits); OUTPUT(pending_confusion_anchors);
    OUTPUT(sampler_rejections); OUTPUT(normal_step_rejections); OUTPUT(dead_input_scenes);
    OUTPUT(chance_rolls); OUTPUT(chance_hits); OUTPUT(chance_forced);
    OUTPUT(counter_created); OUTPUT(counter_started); OUTPUT(counter_rolls); OUTPUT(counter_hits); OUTPUT(counter_forced);
    OUTPUT(death_bows); OUTPUT(death_created); OUTPUT(death_started); OUTPUT(death_rolls); OUTPUT(death_hits); OUTPUT(death_forced);
    OUTPUT(revivals); OUTPUT(redraws); OUTPUT(redrawn_items); OUTPUT(disease_ticks);
    OUTPUT(guardian_opportunities); OUTPUT(auto_guardian_actions); OUTPUT(departure_rolls); OUTPUT(departures);
    OUTPUT(primitive_checksum); OUTPUT(checksum);
#undef OUTPUT
    std::cout<<"\none_pulse_scenes="<<total.pulses[0]<<"\ntwo_pulse_scenes="<<total.pulses[1]
             <<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';
    return 0;
} catch (const std::exception& error) { std::cerr<<"confusion benchmark: "<<error.what()<<'\n'; return 1; } }
