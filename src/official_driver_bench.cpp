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
#include <variant>
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
// hash: Fate, grant and confusion journals are checked by full restore.
std::uint64_t world_hash(const MatchSnapshot& s) {
    std::uint64_t hash=14695981039346656037ULL;
    mix(hash,s.version);mix(hash,s.config.include_fate);mix(hash,s.config.external_grants);mix(hash,s.config.local_guardian_payloads); mix(hash,s.state.gf); mix(hash,s.state.update_count); mix(hash,s.state.is_over);
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
    mix(hash,s.terminated); mix(hash,s.truncated); mix(hash,s.phenomenon_actions);mix(hash,s.fate_actions);
    mix(hash,s.chance_rolls); mix(hash,s.chance_hits); mix(hash,s.chance_forced_hits);
    mix(hash,s.counter_created); mix(hash,s.counter_started); mix(hash,s.counter_rolls); mix(hash,s.counter_hits); mix(hash,s.counter_forced_hits);
    mix(hash,s.death_bows); mix(hash,s.death_created); mix(hash,s.death_started); mix(hash,s.death_rolls); mix(hash,s.death_hits); mix(hash,s.death_forced_hits);
    mix(hash,s.revivals); mix(hash,s.redraws); mix(hash,s.redrawn_items); mix(hash,s.dream_rolls); mix(hash,s.dream_masks);
    mix(hash,s.guardian_summons); mix(hash,s.guardian_opportunities); mix(hash,s.auto_guardian_actions);
    mix(hash,s.guardian_departure_rolls); mix(hash,s.guardian_departures);
    return hash;
}

std::uint64_t bounded(std::uint64_t& state,std::uint64_t bound) {
    const auto threshold=(std::uint64_t{0}-bound)%bound;
    std::uint64_t value;
    do {
        value=(state+=0x9e3779b97f4a7c15ULL);
        value=(value^(value>>30))*0xbf58476d1ce4e5b9ULL;
        value=(value^(value>>27))*0x94d049bb133111ebULL;
        value^=value>>31;
    } while(value<threshold);
    return value%bound;
}
void hash_command(std::uint64_t& hash,const Command& c) {
    mix(hash,c.actor);mix(hash,static_cast<unsigned>(c.kind));mix(hash,c.items.size());
    for(auto token:c.items)mix(hash,token);
    mix(hash,c.target.has_value());if(c.target)mix(hash,*c.target);
    mix(hash,c.allocation.has_value());if(c.allocation){mix(hash,c.allocation->hp);mix(hash,c.allocation->mp);mix(hash,c.allocation->cp);}
}
void hash_choice(std::uint64_t& hash,const PlayerChoice& choice) {
    mix(hash,choice.index());
    if(const auto* c=std::get_if<Command>(&choice))hash_command(hash,*c);
    else {const auto& request=std::get<FateRequest>(choice);mix(hash,request.actor);mix(hash,request.token);}
}
void audit(const Match& game,Match& resumed) {
    resumed.restore(game.snapshot());
    check(resumed.snapshot()==game.snapshot(),"driver checkpoint failed complete snapshot roundtrip");
}
struct Stats {
    std::uint64_t scenes=0,decisions=0,main_commands=0,responses=0,purchases=0,grants=0,confusion_commands=0,fate_requests=0;
    std::uint64_t choice_checks=0,old_entropy_checks=0,copy_law_checks=0,alias_rejections=0,resume_checks=0,manual_route_checks=0;
    std::uint64_t prepared_decisions=0,include_fate_scenes=0,external_grant_scenes=0,local_payload_scenes=0,dead_input_scenes=0;
    std::uint64_t terminated=0,truncated=0,grant_artifacts=0,grant_devils=0,revivals=0,redraws=0,redrawn_items=0;
    std::uint64_t counter_created=0,death_bows=0,death_created=0,guardian_opportunities=0,auto_guardian_actions=0;
    std::uint64_t policy_checksum=0,chance_checksum=0,choice_checksum=0,primitive_checksum=0,checksum=0,gf_total=0;
    std::array<std::uint64_t,8> fixtures{};
    std::array<std::uint64_t,10> outcomes{},commands{};
    void append(const Stats& b) {
#define ADD(n) n+=b.n
        ADD(scenes);ADD(decisions);ADD(main_commands);ADD(responses);ADD(purchases);ADD(grants);ADD(confusion_commands);ADD(fate_requests);
        ADD(choice_checks);ADD(old_entropy_checks);ADD(copy_law_checks);ADD(alias_rejections);ADD(resume_checks);ADD(manual_route_checks);
        ADD(prepared_decisions);ADD(include_fate_scenes);ADD(external_grant_scenes);ADD(local_payload_scenes);ADD(dead_input_scenes);
        ADD(terminated);ADD(truncated);ADD(grant_artifacts);ADD(grant_devils);ADD(revivals);ADD(redraws);ADD(redrawn_items);
        ADD(counter_created);ADD(death_bows);ADD(death_created);ADD(guardian_opportunities);ADD(auto_guardian_actions);
        ADD(policy_checksum);ADD(chance_checksum);ADD(choice_checksum);ADD(primitive_checksum);ADD(checksum);ADD(gf_total);
#undef ADD
        for(std::size_t k=0;k<fixtures.size();++k)fixtures[k]+=b.fixtures[k];
        for(std::size_t k=0;k<outcomes.size();++k){outcomes[k]+=b.outcomes[k];commands[k]+=b.commands[k];}
    }
};
PlayerChoice checked_choice(const Match& game,std::uint64_t& policy,Stats& stats) {
    const auto before=game.snapshot();auto first=policy,second=policy;
    const auto selected=game.sample_choice(first),repeat=game.sample_choice(second);
    check(selected==repeat && first==second && game.snapshot()==before,"choice sampling is impure or clone nondeterministic");
    check(game.choice_legal(selected),"sampled PlayerChoice is illegal");
    ++stats.choice_checks;
    std::vector<ItemToken> tokens;
    if(before.phase==MatchPhase::main) {
        const auto* owner=find_player(before.state,before.owner);
        check(owner && owner->confusion_count==0,"normal choice accepted positive confusion");
        for(const auto& item:owner->items)if(item.model==207 && !item.used && !item.fake_model)tokens.push_back(item.token);
    }
    auto expected_rng=policy;
    if(tokens.empty()) {
        const auto old=game.sample_action(expected_rng);
        check(std::holds_alternative<Command>(selected) && std::get<Command>(selected)==old && first==expected_rng,
              "no207/response/purchase choice changed the old sampler entropy or command");
        ++stats.old_entropy_checks;
    } else {
        const auto ticket=bounded(expected_rng,tokens.size()+1);
        const PlayerChoice expected=ticket==0?PlayerChoice{game.sample_action(expected_rng)}:
            PlayerChoice{FateRequest{before.owner,tokens[ticket-1]}};
        check(selected==expected && first==expected_rng,"choice did not use inventory-copy tickets plus one old-policy branch");
        ++stats.copy_law_checks;
    }
    policy=first;return selected;
}
// Independent explicit routing is compared with the transactional convenience
// driver. This is a local controller, not the official server's random policy.
void manual_step(Match& game,std::uint64_t& policy,std::uint64_t& chance,Stats& stats,std::uint64_t& action_hash) {
    const auto before=game.snapshot();
    if(before.phase==MatchPhase::grant) {
        const auto grant=game.sample_grant(chance);
        check(game.grant_legal(grant),"manual grant sampler returned illegal payload");
        mix(action_hash,4);mix(action_hash,grant.target);mix(action_hash,grant.model);
        mix(action_hash,grant.resource.has_value());if(grant.resource)mix(action_hash,static_cast<unsigned>(*grant.resource));
        mix(action_hash,grant.removed.size());for(auto token:grant.removed)mix(action_hash,token);
        (void)game.grant_step(grant);++stats.grants;return;
    }
    const auto* actor=find_player(before.state,*before.actor);
    if(before.phase==MatchPhase::main && actor->confusion_count>0) {
        const auto command=game.sample_confusion(chance);
        check(game.confusion_legal(command),"manual confusion sample is illegal");
        mix(action_hash,3);hash_command(action_hash,command);
        (void)game.confusion_step(command);++stats.confusion_commands;++stats.main_commands;
        ++stats.commands[static_cast<unsigned>(command.kind)];return;
    }
    const auto choice=checked_choice(game,policy,stats);hash_choice(action_hash,choice);
    if(const auto* request=std::get_if<FateRequest>(&choice)) {
        check(before.phase==MatchPhase::main && request->actor==before.owner,"Fate intent escaped normal main");
        const auto fate=game.sample_fate(request->token,chance);
        check(game.fate_legal(fate),"choice request could not be completed by local chance helper");
        ++stats.outcomes[fate.outcome.model-287];
        (void)game.fate_step(fate);++stats.fate_requests;++stats.main_commands;
    } else {
        const auto& command=std::get<Command>(choice);
        if(before.phase==MatchPhase::response)++stats.responses;
        else if(before.phase==MatchPhase::purchase)++stats.purchases;
        else ++stats.main_commands;
        ++stats.commands[static_cast<unsigned>(command.kind)];
        (void)game.step(command);
    }
}
SnapshotState fixture(std::uint64_t i,std::uint64_t players,std::uint64_t seed,MatchConfig& config) {
    const auto type=i%8;
    config.max_decisions=i%101==0?2:96;
    config.include_fate=i%2==0;
    config.external_grants=type==4 || type==7 || i%3==0;
    config.local_guardian_payloads=i%4==0;
    SnapshotState state;state.gf=17;
    if(config.external_grants || type==7)state.tiebreak_gf=17;
    constexpr std::array<CardId,18> cards{6,235,208,112,190,178,184,187,188,3,4,5,203,236,239,66,204,231};
    for(std::uint64_t seat=0;seat<players;++seat) {
        PlayerState p;p.id=seat==0?0:100+7*seat;p.name="P"+std::to_string(seat);
        p.user_id="user"+std::to_string(seat);p.alias="A"+std::to_string(seat);
        p.team=i%3==0?(seat%2?Team::two:Team::one):Team::solo;
        p.hp=seat==0?99:40+static_cast<std::int64_t>((seed+i+seat)%60);p.mp=i%7==0?0:99;p.cp=99;
        p.dead=players==9 && seat==8 && i%19==0;if(p.dead)p.hp=0;
        if(type!=4 && type!=5 && type!=6 && (i+seat)%3==0)p.guardian=static_cast<Guardian>(1+(i+seat)%10);
        if(type!=4 && type!=5 && type!=6 && (i+seat)%5==0)p.curses.push_back(Curse::cold);
        if(type!=5 && type!=6 && (i+seat)%4==0)p.curses.push_back(Curse::fog);
        if((i+seat)%2==0)p.curses.push_back(Curse::dream);
        if(type!=4 && type!=5 && type!=6 && (i+seat)%7==0)p.curses.push_back(Curse::darkcloud);
        const auto size=4+(seed+i+seat)%15;
        for(std::uint64_t slot=0;slot<size;++slot) {
            ItemInstance item{slot,cards[slot],{},slot==1};
            if(slot==0 && (i+seat)%2==0)item.fake_model=7;
            p.items.push_back(item);
        }
        if(seat==0) {
            if(type==1)p.items={{0,207,{},false},{1,235,{},true}};
            if(type==2){p.items[0]={0,207,{},false};p.items.back()={static_cast<ItemToken>(size-1),207,{},false};}
            if(type==3){p.items[0]={0,207,{},false};p.confusion_count=1+(i%2);}
            if(type==4)p.items.clear();
            if(type==5)p.items={{0,5,{},false}};
            if(type==6)p.items={{0,6,{},false}};
            if(type==7)p.items={{0,235,{},true},{1,199,{},false},{2,3,{},false},{3,4,{},false},{4,5,{},false}};
        }
        if(type==5 && seat==1)p.items={{0,235,{},false}};
        state.players.push_back(std::move(p));
    }
    return state;
}
}
int main(int argc,char** argv) {try {
    if(argc>5)throw std::invalid_argument("usage: godfield_driver_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000,players=argc>2?number(argv[2]):2,seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if(count>1000000000 || players<2 || players>9 || workers>64 || seed>UINT64_MAX-(count-1))throw std::invalid_argument("driver benchmark bounds exceeded");
    workers=std::min(workers,count);
    const auto start=std::chrono::steady_clock::now();
    std::vector<Stats> results(workers);std::vector<std::exception_ptr> errors(workers);std::vector<std::jthread> threads;
    for(std::uint64_t worker=0;worker<workers;++worker)threads.emplace_back([&,worker]{try {
        auto& stats=results[worker];
        for(std::uint64_t i=worker;i<count;i+=workers) {
            MatchConfig config;auto state=fixture(i,players,seed,config);
            // Preloaded purchase/response starts are explicit fixtures; their setup
            // commands are counted separately from convenience-driver decisions.
            if(i%8==5 || i%8==6)config.max_decisions=96;
            Match first(config),second(config),manual(config),resumed(config);
            first.load_scene(std::move(state),0,seed+i);
            if(i%8==5) {
                (void)first.step(Command{0,CommandKind::buy,{0},PlayerId{107}});
                check(first.snapshot().phase==MatchPhase::response,"BUY fixture did not start response");
                (void)first.step(Command{107,CommandKind::defend,{},{}});
                check(first.snapshot().phase==MatchPhase::purchase,"BUY fixture did not produce a quote");
            } else if(i%8==6) {
                (void)first.step(Command{0,CommandKind::play,{0},PlayerId{107}});
                check(first.snapshot().phase==MatchPhase::response,"attack fixture did not start response");
            }
            const auto initial=first.snapshot();second.restore(initial);manual.restore(initial);audit(first,resumed);
            ++stats.fixtures[i%8];stats.prepared_decisions+=initial.decisions;
            stats.include_fate_scenes+=config.include_fate;stats.external_grant_scenes+=config.external_grants;
            stats.local_payload_scenes+=config.local_guardian_payloads;
            stats.dead_input_scenes+=std::any_of(initial.state.players.begin(),initial.state.players.end(),[](const auto& p){return p.dead;});
            auto primitive=world_hash(initial);mix(primitive,i);stats.primitive_checksum+=primitive;
            std::uint64_t policy=(seed+i)^0x510e527fade682d1ULL,chance=(seed+i)^0x9b05688c2b3e6c1fULL;
            // Equal stream values are valid; only aliased storage is forbidden.
            if(i%29==0)chance=policy;
            auto other_policy=policy,other_chance=chance;
            // An alias must fail even if the current route only needs one stream.
            auto alias=policy;bool rejected=false;
            try{(void)first.rollout_step(alias,alias);}catch(const MatchError&){rejected=true;}
            check(rejected && alias==policy && first.snapshot()==initial,"aliased driver RNG references were not atomically rejected");
            ++stats.alias_rejections;
            auto action_hash=std::uint64_t{14695981039346656037ULL};mix(action_hash,i);
            do {
                const auto before=first.snapshot();const auto previous_policy=policy;auto manual_policy=policy,manual_chance=chance;
                manual.restore(before);
                manual_step(manual,manual_policy,manual_chance,stats,action_hash);
                (void)first.rollout_step(policy,chance);
                (void)second.rollout_step(other_policy,other_chance);
                check(first.snapshot()==second.snapshot() && policy==other_policy && chance==other_chance,
                      "driver clones diverged in full snapshot or either external RNG");
                check(first.snapshot()==manual.snapshot() && policy==manual_policy && chance==manual_chance,
                      "driver differs from explicit command/Fate/confusion/grant routing");
                check(first.snapshot().decisions==before.decisions+1,"driver did not accept exactly one decision");
                ++stats.manual_route_checks;++stats.resume_checks;
                audit(first,resumed);
                // Restart the second clone from each new checkpoint plus the two
                // saved caller RNGs; the next iteration checks resumed execution.
                second.restore(resumed.snapshot());other_policy=policy;other_chance=chance;
                if(before.phase==MatchPhase::grant || (before.phase==MatchPhase::main && find_player(before.state,before.owner)->confusion_count>0))
                    check(policy==previous_policy,"chance-only driver route changed policy unexpectedly");
            } while(!first.done() && first.snapshot().phase!=MatchPhase::main);
            const auto& final=first.snapshot();
            stats.decisions+=final.decisions-initial.decisions;++stats.scenes;
            stats.terminated+=final.terminated;stats.truncated+=final.truncated;
            stats.grant_artifacts+=final.grant_artifacts-initial.grant_artifacts;stats.grant_devils+=final.grant_devils-initial.grant_devils;
            stats.revivals+=final.revivals-initial.revivals;stats.redraws+=final.redraws-initial.redraws;stats.redrawn_items+=final.redrawn_items-initial.redrawn_items;
            stats.counter_created+=final.counter_created-initial.counter_created;stats.death_bows+=final.death_bows-initial.death_bows;stats.death_created+=final.death_created-initial.death_created;
            stats.guardian_opportunities+=final.guardian_opportunities-initial.guardian_opportunities;stats.auto_guardian_actions+=final.auto_guardian_actions-initial.auto_guardian_actions;
            stats.gf_total+=final.state.gf;
            auto world=world_hash(final);mix(world,i);stats.checksum+=world;
            auto ph=std::uint64_t{14695981039346656037ULL};mix(ph,i);mix(ph,policy);stats.policy_checksum+=ph;
            auto ch=std::uint64_t{14695981039346656037ULL};mix(ch,i);mix(ch,chance);stats.chance_checksum+=ch;
            stats.choice_checksum+=action_hash;
        }
    }catch(...){errors[worker]=std::current_exception();}});
    for(auto& thread:threads)thread.join();for(const auto& error:errors)if(error)std::rethrow_exception(error);
    Stats total;for(const auto& stats:results)total.append(stats);
    check(total.scenes==count && total.decisions==total.main_commands+total.responses+total.purchases+total.grants,
          "driver aggregate decision accounting failed");
    if(count>=600)check(total.fate_requests && total.confusion_commands && total.grants && total.purchases && total.responses
                       && total.old_entropy_checks && total.copy_law_checks,"driver fixture omitted a stable routing branch");
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset<<"\nscope=experimental:typed-player-intent-and-local-chance-driver;one-original-action-or-prepared-purchase-response;no-official-selector;world-selected-counter-hashes-plus-full-snapshot-restore-audit"
             <<"\nchoice_rule="<<choice_rule<<"\ndriver_rule="<<driver_rule
             <<"\nscenes="<<count<<"\nplayers="<<players<<"\nseed="<<seed<<"\nworkers="<<workers<<"\nmax_decisions=96\ncutoff_max_decisions=2";
#define OUTPUT(n) std::cout<<"\n" #n "="<<total.n
    OUTPUT(decisions);OUTPUT(main_commands);OUTPUT(responses);OUTPUT(purchases);OUTPUT(grants);OUTPUT(confusion_commands);OUTPUT(fate_requests);
    OUTPUT(choice_checks);OUTPUT(old_entropy_checks);OUTPUT(copy_law_checks);OUTPUT(alias_rejections);OUTPUT(resume_checks);OUTPUT(manual_route_checks);
    OUTPUT(prepared_decisions);OUTPUT(include_fate_scenes);OUTPUT(external_grant_scenes);OUTPUT(local_payload_scenes);OUTPUT(dead_input_scenes);
    OUTPUT(terminated);OUTPUT(truncated);OUTPUT(grant_artifacts);OUTPUT(grant_devils);OUTPUT(revivals);OUTPUT(redraws);OUTPUT(redrawn_items);
    OUTPUT(counter_created);OUTPUT(death_bows);OUTPUT(death_created);OUTPUT(guardian_opportunities);OUTPUT(auto_guardian_actions);OUTPUT(gf_total);
    OUTPUT(policy_checksum);OUTPUT(chance_checksum);OUTPUT(choice_checksum);OUTPUT(primitive_checksum);OUTPUT(checksum);
#undef OUTPUT
    for(std::size_t k=0;k<total.fixtures.size();++k)std::cout<<"\nfixture_"<<k<<"="<<total.fixtures[k];
    for(std::size_t k=0;k<total.outcomes.size();++k)std::cout<<"\nfate_"<<(287+k)<<"="<<total.outcomes[k]<<"\ncommand_"<<k<<"="<<total.commands[k];
    std::cout<<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<"driver benchmark: "<<e.what()<<'\n';return 1;}}
