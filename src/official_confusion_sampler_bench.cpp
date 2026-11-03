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
    mix(hash,s.version);mix(hash,s.config.include_fate);mix(hash,s.config.external_grants); mix(hash,s.state.gf); mix(hash,s.state.update_count); mix(hash,s.state.is_over);
    mix(hash,s.state.tiebreak_gf.has_value());if(s.state.tiebreak_gf)mix(hash,*s.state.tiebreak_gf);
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
    mix(hash,s.grant_resolutions);mix(hash,s.grant_artifacts);mix(hash,s.grant_devils);
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
// The live helper is compared with two unchanged clones and a separately loaded
// count-zero counterfactual world. That comparison does not rewrite proof records.
Command proposal(const Match& game,std::uint64_t& chance) {
    const auto before=game.snapshot();const auto actor=*before.actor;
    Match a(before.config),b(before.config);a.restore(before);b.restore(before);
    auto first=chance,second=chance;
    const auto selected=a.sample_confusion(first),repeated=b.sample_confusion(second);
    check(selected==repeated && first==second && a.snapshot()==before && b.snapshot()==before && game.snapshot()==before,
          "same-state confusion helper clones changed Match or differed in action/next chance RNG");
    auto world=before.state;for(auto& player:world.players)if(player.id==actor)player.confusion_count=0;
    Match normal(before.config);normal.load_scene(std::move(world),before.owner,before.rng);
    auto counterfactual=chance;const auto expected=normal.sample_action(counterfactual);
    check(selected==expected && first==counterfactual,"confusion helper differs from the existing count-zero normal proposal law");
    chance=first;return selected;
}
void hash_command(std::uint64_t& hash,const Command& command) {
    mix(hash,command.actor);mix(hash,static_cast<unsigned>(command.kind));mix(hash,command.items.size());
    for(auto token:command.items)mix(hash,token);mix(hash,command.target.has_value());if(command.target)mix(hash,*command.target);
    mix(hash,command.allocation.has_value());if(command.allocation){mix(hash,command.allocation->hp);mix(hash,command.allocation->mp);mix(hash,command.allocation->cp);}
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
    std::uint64_t responses=0,purchase_commands=0,grant_commands=0,decisions=0,terminated=0,truncated=0;
    std::uint64_t sampler_checks=0,invalid_sampler_checks=0,empty_hand_scenes=0,full_used_scenes=0,free_commands=0,used_commands=0,cure_commands=0;
    std::uint64_t include_fate_scenes=0,external_grant_scenes=0,chance_checksum=0,sampled_action_checksum=0;
    std::array<std::uint64_t,10> commands{};
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
        ADD(responses); ADD(purchase_commands);ADD(grant_commands);ADD(sampler_checks);ADD(invalid_sampler_checks);
        ADD(empty_hand_scenes);ADD(full_used_scenes);ADD(free_commands);ADD(used_commands);ADD(cure_commands);
        ADD(include_fate_scenes);ADD(external_grant_scenes);ADD(chance_checksum);ADD(sampled_action_checksum);ADD(decisions); ADD(terminated); ADD(truncated);
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
        for(std::size_t i=0;i<commands.size();++i)commands[i]+=other.commands[i];
    }
};
void refuse_helper(Match& game,std::uint64_t& chance,Stats& stats) {
    const auto before=game.snapshot();const auto prior=chance;bool failed=false;
    try{(void)game.sample_confusion(chance);}catch(const MatchError&){failed=true;}
    check(failed && chance==prior && game.snapshot()==before,"invalid confusion sampling changed state or caller chance RNG");++stats.invalid_sampler_checks;
}
void command_shape(const MatchSnapshot& before,const Command& command,Stats& stats) {
    ++stats.commands[static_cast<unsigned>(command.kind)];
    const auto* own=find_player(before.state,command.actor);bool free=false,used=false;
    for(auto token:command.items){const auto item=std::find_if(own->items.begin(),own->items.end(),[&](const auto& copy){return copy.token==token;});
        check(item!=own->items.end(),"confusion sampler returned another player's token");free|=find_card(item->model)->ability==Ability::cut_cost;used|=item->used;}
    stats.free_commands+=free && command.kind==CommandKind::play;stats.used_commands+=used;
    if(command.kind==CommandKind::exchange){
        check(command.items.size()==1 && !command.target && command.allocation,"exchange sample lost its typed allocation");
        const auto& a=*command.allocation;check(a.hp>=0 && a.hp<=99 && a.mp>=0 && a.mp<=99 && a.cp>=0 && a.cp<=99
            && a.hp+a.mp+a.cp==own->hp+own->mp+own->cp,"exchange sample changed its resource sum or domain");
    }else check(!command.allocation,"nonexchange sample contains an allocation");
    if(command.kind==CommandKind::sell)check(command.items.size()==2 && command.items[0]!=command.items[1] && command.target,"SELL sample lost its distinct commodity or target");
    if(command.kind==CommandKind::buy)check(command.items.size()==1 && command.target,"BUY sample lost its operation or target");
    if(command.kind==CommandKind::pray)check(command.items.empty() && !command.target,"pray sample has an inactive field");
    if(command.kind==CommandKind::sacrifice)check(before.state.tiebreak_gf && before.state.gf>=*before.state.tiebreak_gf,"sampler sacrificed before its threshold");
    if(command.kind==CommandKind::play && !command.items.empty()){
        const auto item=std::find_if(own->items.begin(),own->items.end(),[&](const auto& copy){return copy.token==command.items.front();});
        check(item->model!=207,"local normal proposal law secretly started an unresolved Fate action");
        const auto ability=find_card(item->model)->ability;stats.cure_commands+=ability==Ability::remove_mild_curses || ability==Ability::remove_all_curses;
    }
}
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
    if (argc>5) throw std::invalid_argument("usage: godfield_confusion_sampler_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000,players=argc>2?number(argv[2]):2,seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if (count>1000000000 || players<2 || players>9 || workers>64 || seed>UINT64_MAX-(count-1))
        throw std::invalid_argument("confusion sampler benchmark bounds exceeded");
    workers=std::min(workers,count);
    check(match_dream_compatible(6,7),"fixture fake is not compatible");
    std::vector<Stats> results(workers); std::vector<std::exception_ptr> errors(workers); std::vector<std::jthread> threads;
    const auto start=std::chrono::steady_clock::now();
    for (std::uint64_t worker=0;worker<workers;++worker) threads.emplace_back([&,worker] { try {
        auto& stats=results[worker];
        constexpr std::array<CardId,18> cards{6,235,235,208,112,190,178,184,187,188,3,4,5,203,236,239,66,58};
        for (std::uint64_t i=worker;i<count;i+=workers) {
            const auto column=static_cast<unsigned>(i%10);
            MatchConfig config;config.max_decisions=128;config.include_fate=i%2==0;config.external_grants=i%3==0;
            Match game(config),restored(config);
            SnapshotState state; state.gf=17;if(column==6 || config.external_grants)state.tiebreak_gf=17;else if(i%4==0)state.tiebreak_gf=18;
            const bool dead_last=players==9 && i%17==0;
            for (std::uint64_t seat=0;seat<players;++seat) {
                PlayerState player; player.id=seat==0?0:100+7*seat; player.name="P"+std::to_string(seat);
                player.user_id="user"+std::to_string(seat); player.alias="A"+std::to_string(seat);
                player.team=i%3==0?(seat%2?Team::two:Team::one):Team::solo;
                player.hp=player.mp=99; player.cp=(seed+i+seat)%100;
                player.guardian=column==0?Guardian::none:static_cast<Guardian>(1+(seed+i+seat)%10);
                if (column!=0 && (i+seat)%4==0) player.curses.push_back(Curse::cold);
                if (column!=0 && (i+seat)%3==0) player.curses.push_back(Curse::fog);
                if (column!=0 && (i+seat)%2==0) player.curses.push_back(Curse::dream);
                player.dead=dead_last && seat+1==players; if (player.dead) player.hp=0;
                player.confusion_count=player.dead?0:static_cast<std::int64_t>((i+seat)%3);
                const auto size=4+(seed+i+seat)%15;
                for (std::uint64_t slot=0;slot<size;++slot) {
                    ItemInstance item{slot,cards[slot],{},slot==2};
                    if (slot==0 && column!=0 && (i+seat)%2==0) item.fake_model=7;
                    player.items.push_back(item);
                }
                if(seat==0){
                    if(column==0)player.items.clear();
                    else if(column==1){player.items.clear();for(ItemToken slot=0;slot<18;++slot)player.items.push_back({slot,235,{},true});}
                    else if(column==2)player.items={{0,3,{},false}};
                    else if(column==3){player.mp=0;player.items={{0,235,{},false},{1,204,{},false}};}
                    else if(column==4)player.items={{0,4,{},false},{1,6,{},false},{2,199,{},false}};
                    else if(column==5){player.cp=99;player.items={{0,5,{},false}};}
                    else if(column==6)player.items={{0,199,{},false},{1,235,{},true},{2,203,{},false}};
                    else if(column==7)player.items={{0,207,{},false}};
                    else if(column==8){player.hp=1;player.items={{0,199,{},false},{1,200,{},false},{2,208,{},false},{3,112,{},false},{4,190,{},false},{5,178,{},false},{6,235,{},true}};}
                    else {player.hp=5;player.items={{0,14,{},false},{1,207,{},false},{2,208,{},false},{3,112,{},false},{4,190,{},false},{5,178,{},false},{6,184,{},false},{7,187,{},false},{8,188,{},false},{9,235,{},true},{10,239,{},false},{11,199,{},false},{12,200,{},false},{13,3,{},false},{14,4,{},false},{15,5,{},false},{16,203,{},false},{17,204,{},false}};}
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
            std::uint64_t policy=(seed+i)^0xa54ff53a5f1d36f1ULL,chance=(seed+i)^0x9b05688c2b3e6c1fULL;
            auto chance_trace=std::uint64_t{14695981039346656037ULL},action_trace=chance_trace;mix(chance_trace,chance);
            for (std::size_t launch=0;launch<initial_alive*(pulses==1?4:3) && !game.done();++launch) {
                const auto before=game.snapshot();
                check(before.phase==MatchPhase::main && before.actor==before.owner,"controller did not reach a main boundary");
                const auto actor=*before.actor; const auto old_count=find_player(before.state,actor)->confusion_count;
                const bool confused=old_count>0;
                auto trial=chance;const auto command=confused?proposal(game,trial):game.sample_action(policy);
                if(confused){++stats.sampler_checks;mix(chance_trace,chance);mix(chance_trace,trial);hash_command(action_trace,command);}
                else refuse_helper(game,chance,stats);
                command_shape(before,command,stats);
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
                    game.confusion_step(command);chance=trial; ++stats.confusion_commands; ++stats.confusion_launch_debits;
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
                    const auto pending=game.snapshot();refuse_helper(game,chance,stats);
                    if(pending.phase==MatchPhase::grant){
                        auto rejected_policy=policy;bool rejected=false;try{(void)game.sample_action(rejected_policy);}catch(const MatchError&){rejected=true;}
                        check(rejected && rejected_policy==policy && game.snapshot()==pending,"grant phase changed normal policy RNG or state");
                        game.grant_step({*pending.pending_grant,0,{},{}});++stats.grant_commands;
                        audit(game,restored);count_transition(pending,game.snapshot(),{});
                        if(game.snapshot().batch)check(game.snapshot().batch->confusion_command==confused,"grant changed the originating confusion marker");continue;
                    }
                    const auto response=game.sample_action(policy);
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
            check(final.phenomenon_actions==pulses && !final.guardian_actions && !final.guardian_payload_actions && !final.fate_actions && !final.devil_actions,
                  "ordinary command invented an external effect activation");
            for (const auto& player:final.state.players) {
                check(player.confusion_count>=0 && (!player.dead || player.confusion_count==0)
                      && (!final.terminated || player.confusion_count==0),"dead/end count did not clear");
                stats.max_count=std::max(stats.max_count,static_cast<std::uint64_t>(player.confusion_count));
            }
            ++stats.scenes;stats.include_fate_scenes+=config.include_fate;stats.external_grant_scenes+=config.external_grants;
            stats.empty_hand_scenes+=column==0;stats.full_used_scenes+=column==1;
            mix(chance_trace,chance);mix(chance_trace,i);stats.chance_checksum+=chance_trace;mix(action_trace,i);stats.sampled_action_checksum+=action_trace;
            stats.decisions+=final.decisions; stats.terminated+=final.terminated; stats.truncated+=final.truncated;
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
          && total.confusion_launch_debits==total.confusion_commands && total.sampler_checks==total.confusion_commands
          && total.decisions==total.phenomenon_actions+total.main_launches+total.responses+total.purchase_commands+total.grant_commands
          && total.chance_hits<=total.chance_rolls && total.counter_hits<=total.counter_rolls
          && total.death_hits<=total.death_rolls && total.terminated+total.truncated<=count,
          "confusion sampler benchmark accounting failed");
    if(count>=600){
        check(total.normal_main_commands>0 && total.responses>0 && total.purchase_commands>0 && total.grant_commands>0
              && total.free_commands>0 && total.used_commands>0 && total.cure_commands>0,"confusion sampler fixture omitted a meaningful branch");
        for(auto kind:{CommandKind::play,CommandKind::pray,CommandKind::exchange,CommandKind::sell,CommandKind::buy,CommandKind::sacrifice})
            check(total.commands[static_cast<unsigned>(kind)]>0,"confusion sampler fixture omitted a main command shape");
    }
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset<<"\nscope=experimental:local-normal-proposal-law-for-external-confusion;two-unchanged-clones-and-countzero-loaded-world-counterfactual;not-official-uniform-legal-confusion;world-count-selected-counter-and-independent-chance-action-hashes-plus-full-restore-audit"
             <<"\nconfusion_rule="<<confusion_rule<<"\nconfusion_sampler_rule="<<confusion_sampler_rule<<"\nfate_rule="<<fate_rule<<"\ndevil_rule="<<devil_rule<<"\napocalypse_rule="<<apocalypse_rule<<"\ngrant_rule="<<grant_rule<<"\ngrant_sampler_rule="<<grant_sampler_rule<<"\nfate_pool_rule="<<fate_pool_rule<<"\nfate_sampler_rule="<<fate_sampler_rule<<"\nphenomenon_rule="<<phenomenon_rule
             <<"\nscenes="<<total.scenes<<"\nplayers="<<players<<"\nseed="<<seed<<"\nworkers="<<workers;
#define OUTPUT(name) std::cout<<"\n" #name "="<<total.name
    OUTPUT(phenomenon_actions); OUTPUT(main_launches); OUTPUT(confusion_commands); OUTPUT(normal_main_commands);
    OUTPUT(responses);OUTPUT(grant_commands);OUTPUT(sampler_checks);OUTPUT(invalid_sampler_checks);OUTPUT(empty_hand_scenes);OUTPUT(full_used_scenes);OUTPUT(free_commands);OUTPUT(used_commands);OUTPUT(cure_commands);
    OUTPUT(include_fate_scenes);OUTPUT(external_grant_scenes);OUTPUT(chance_checksum);OUTPUT(sampled_action_checksum); OUTPUT(purchase_commands); OUTPUT(decisions); OUTPUT(terminated); OUTPUT(truncated);
    OUTPUT(confusion_points_added); OUTPUT(max_count); OUTPUT(confusion_launch_debits); OUTPUT(pending_confusion_anchors);
    OUTPUT(sampler_rejections); OUTPUT(normal_step_rejections); OUTPUT(dead_input_scenes);
    OUTPUT(chance_rolls); OUTPUT(chance_hits); OUTPUT(chance_forced);
    OUTPUT(counter_created); OUTPUT(counter_started); OUTPUT(counter_rolls); OUTPUT(counter_hits); OUTPUT(counter_forced);
    OUTPUT(death_bows); OUTPUT(death_created); OUTPUT(death_started); OUTPUT(death_rolls); OUTPUT(death_hits); OUTPUT(death_forced);
    OUTPUT(revivals); OUTPUT(redraws); OUTPUT(redrawn_items); OUTPUT(disease_ticks);
    OUTPUT(guardian_opportunities); OUTPUT(auto_guardian_actions); OUTPUT(departure_rolls); OUTPUT(departures);
    OUTPUT(primitive_checksum); OUTPUT(checksum);
#undef OUTPUT
    for(std::size_t kind=0;kind<total.commands.size();++kind)std::cout<<"\nmain_kind"<<kind<<"="<<total.commands[kind];
    std::cout<<"\nmax_decisions=128\none_pulse_scenes="<<total.pulses[0]<<"\ntwo_pulse_scenes="<<total.pulses[1]
             <<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';
    return 0;
} catch (const std::exception& error) { std::cerr<<"confusion sampler benchmark: "<<error.what()<<'\n'; return 1; } }
