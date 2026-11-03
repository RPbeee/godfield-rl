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
// hash: Devil records and pending journals are checked by full restore.
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
    mix(hash,s.terminated); mix(hash,s.truncated); mix(hash,s.phenomenon_actions); mix(hash,s.fate_actions); mix(hash,s.devil_actions);
    mix(hash,s.chance_rolls); mix(hash,s.chance_hits); mix(hash,s.chance_forced_hits);
    mix(hash,s.counter_created); mix(hash,s.counter_started); mix(hash,s.counter_rolls); mix(hash,s.counter_hits); mix(hash,s.counter_forced_hits);
    mix(hash,s.death_bows); mix(hash,s.death_created); mix(hash,s.death_started); mix(hash,s.death_rolls); mix(hash,s.death_hits); mix(hash,s.death_forced_hits);
    mix(hash,s.revivals); mix(hash,s.redraws); mix(hash,s.redrawn_items); mix(hash,s.dream_rolls); mix(hash,s.dream_masks);
    mix(hash,s.guardian_summons); mix(hash,s.guardian_opportunities); mix(hash,s.auto_guardian_actions);
    mix(hash,s.guardian_departure_rolls); mix(hash,s.guardian_departures);
    return hash;
}



void audit(const Match& game, Match& restored) {
    restored.restore(game.snapshot()); check(restored.snapshot()==game.snapshot(),"devil snapshot failed full roundtrip");
}
std::uint64_t bounded100(std::uint64_t& state) {
    constexpr auto bound=std::uint64_t{100},threshold=(std::uint64_t{0}-bound)%bound;
    std::uint64_t value;
    do {
        value=(state+=0x9e3779b97f4a7c15ULL); value=(value^(value>>30))*0xbf58476d1ce4e5b9ULL;
        value=(value^(value>>27))*0x94d049bb133111ebULL; value^=value>>31;
    } while (value<threshold);
    return value%bound;
}
void count_transition(const MatchSnapshot& before, const MatchSnapshot& after) {
    for (const auto& player:before.state.players) {
        const auto* now=find_player(after.state,player.id); check(now!=nullptr,"devil changed roster");
        const auto expected=(now->dead || after.terminated)?0:player.confusion_count;
        check(now->confusion_count==expected,"external devil debited or changed confusion outside death/end");
    }
}
struct Stats {
    std::uint64_t scenes=0,decisions=0,responses=0,purchase_commands=0,terminated=0,truncated=0,devil_actions=0;
    std::uint64_t damage_actions=0,damage_nominal=0,damage_actual=0,removal_actions=0,removed_copies=0;
    std::uint64_t removed_used=0,removed_fake=0,removed_passives=0,removed207=0,resource_actions=0,resource_gained=0;
    std::uint64_t no_selector_rng_checks=0,revival_gift_checks=0,dead_input_scenes=0,owner_is_target=0,target0_scenes=0;
    std::uint64_t real_frame_observations=0,source0_frame_observations=0,primitive_checksum=0,checksum=0;
    std::uint64_t chance_rolls=0,chance_hits=0,chance_forced=0,counter_created=0,counter_started=0,counter_rolls=0,counter_hits=0,counter_forced=0;
    std::uint64_t death_bows=0,death_created=0,death_started=0,death_rolls=0,death_hits=0,death_forced=0;
    std::uint64_t revivals=0,redraws=0,redrawn_items=0,departure_rolls=0,departures=0;
    std::array<std::uint64_t,5> coverage{};
    std::array<std::uint64_t,3> resource_coverage{},removal_sizes{};
    void append(const Stats& other) {
#define ADD(name) name+=other.name
        ADD(scenes); ADD(decisions); ADD(responses); ADD(purchase_commands); ADD(terminated); ADD(truncated); ADD(devil_actions);
        ADD(damage_actions); ADD(damage_nominal); ADD(damage_actual); ADD(removal_actions); ADD(removed_copies);
        ADD(removed_used); ADD(removed_fake); ADD(removed_passives); ADD(removed207); ADD(resource_actions); ADD(resource_gained);
        ADD(no_selector_rng_checks); ADD(revival_gift_checks); ADD(dead_input_scenes); ADD(owner_is_target); ADD(target0_scenes);
        ADD(real_frame_observations); ADD(source0_frame_observations); ADD(primitive_checksum); ADD(checksum);
        ADD(chance_rolls); ADD(chance_hits); ADD(chance_forced); ADD(counter_created); ADD(counter_started); ADD(counter_rolls); ADD(counter_hits); ADD(counter_forced);
        ADD(death_bows); ADD(death_created); ADD(death_started); ADD(death_rolls); ADD(death_hits); ADD(death_forced);
        ADD(revivals); ADD(redraws); ADD(redrawn_items); ADD(departure_rolls); ADD(departures);
#undef ADD
        for (std::size_t i=0;i<coverage.size();++i) coverage[i]+=other.coverage[i];
        for (std::size_t i=0;i<resource_coverage.size();++i) { resource_coverage[i]+=other.resource_coverage[i]; removal_sizes[i]+=other.removal_sizes[i]; }
    }
};
void inspect_frames(const MatchSnapshot& s, Stats& stats) {
    const auto inspect=[&](const EffectFrame& effect,bool active) {
        check(!effect.environmental && (effect.is_death || effect.is_counter)
              && find_player(s.state,effect.origin) && find_player(s.state,effect.source) && find_player(s.state,effect.target),
              "devil created an original defense/190 window or fabricated a derivative source");
        if (active) { ++stats.real_frame_observations; stats.source0_frame_observations+=effect.source==0; }
    };
    for (const auto& effect:s.queue) inspect(effect,true);
    for (const auto& effect:s.deaths) inspect(effect,false);
    for (const auto& effect:s.counters) inspect(effect,false);
}
void verify_proof(const MatchSnapshot& before,const MatchSnapshot& now,const DevilAction& action,
                  const std::vector<CardId>& removed) {
    check(now.devil_actions==1 && !now.phenomenon_actions && !now.fate_actions
          && !now.guardian_actions && !now.guardian_payload_actions && !now.guardian_opportunities
          && !now.auto_guardian_actions && !now.guardian_summons && !now.last_tick && now.state.gf==17,
          "devil ran an ordinary/external operation schedule");
    check(now.last_devil && now.last_devil->action==action && now.last_devil->anchor.initial_state==before.state
          && now.last_devil->anchor.rng==before.rng && now.last_devil->anchor.next_token==before.next_token
          && now.last_devil->anchor.decisions==0 && now.last_devil->anchor.devil_actions==0
          && now.last_devil->responses.size()==now.decisions-1,"devil lost pre-action proof or response accounting");
    check(now.last_devil_event && now.last_devil_event->model==action.model && now.last_devil_event->target==action.target
          && now.last_devil_event->removed_models==removed,"devil notice differs from supplied copy order");
    if (now.batch) check(now.batch->devil_action==action && now.batch->tick_processed
                         && !now.batch->confusion_command,"devil pending lost external/no-tick/no-debit marker");
    for (const auto& player:before.state.players) if (player.dead)
        check(*find_player(now.state,player.id)==player,"devil changed an initially dead player");
}
std::int64_t resource_value(const PlayerState& player,Resource resource) {
    return resource==Resource::hp?player.hp:resource==Resource::mp?player.mp:player.cp;
}
void verify_direct(const MatchSnapshot& before,const MatchSnapshot& after,const DevilAction& action,
                   const std::vector<CardId>& removed,Stats& stats) {
    const auto* old=find_player(before.state,action.target); const auto* now=find_player(after.state,action.target);
    check(after.decisions==1 && after.counter_created==0,"direct devil created initial response counters");
    if (!after.queue.empty()) check(after.queue.front().is_death && !after.queue.front().environmental,
                                   "direct devil made a body defense window");
    if (action.model>=243) {
        auto expected=before.state; ++expected.update_count;
        auto& target=*std::find_if(expected.players.begin(),expected.players.end(),[&](const auto& p) { return p.id==action.target; });
        if (action.model==243) {
            ++stats.removal_actions; ++stats.removal_sizes[action.removed.size()]; stats.removed_copies+=action.removed.size();
            for (auto token:action.removed) {
                const auto item=std::find_if(target.items.begin(),target.items.end(),[&](const auto& v) { return v.token==token; });
                check(item!=target.items.end(),"supplied removal copy vanished before erase");
                stats.removed_used+=item->used; stats.removed_fake+=item->fake_model.has_value(); stats.removed207+=item->model==207;
                const auto ability=find_card(item->model)->ability;
                stats.removed_passives+=ability==Ability::revive || ability==Ability::attract_danger || ability==Ability::attack_dyingly;
                target.items.erase(item);
            }
            check(after.last_devil_event->removed_models==removed,"243 changed supplied actual-model order");
        } else {
            const auto resource=*action.resource; ++stats.resource_actions; ++stats.resource_coverage[static_cast<unsigned>(resource)];
            auto& value=resource==Resource::hp?target.hp:resource==Resource::mp?target.mp:target.cp;
            value=std::min<std::int64_t>(99,value+10);
            stats.resource_gained+=resource_value(*now,resource)-resource_value(*old,resource);
        }
        check(after.state==expected && after.rng==before.rng && after.next_token==before.next_token
              && after.gifts==before.gifts && after.queue.empty() && !after.batch && after.phase==MatchPhase::main
              && after.owner==before.owner && !after.revivals && !after.death_bows && !after.redraws
              && !after.guardian_departure_rolls,"243/244 changed another copy/resource/schedule or consumed RNG");
        ++stats.no_selector_rng_checks;
    } else {
        const auto nominal=find_card(action.model)->ability_value; const auto loss=std::min<std::int64_t>(old->hp,nominal);
        ++stats.damage_actions; stats.damage_nominal+=nominal; stats.damage_actual+=loss;
        const auto hp=after.revivals?10:old->hp-loss;
        check(now->hp==hp && after.revivals<=1,"direct HP loss or passive revival amount was wrong");
        auto expected_rng=before.rng; bool departed=false;
        if (loss>0 && old->guardian!=Guardian::none) departed=bounded100(expected_rng)<10;
        check(after.guardian_departure_rolls==static_cast<std::uint64_t>(loss>0 && old->guardian!=Guardian::none)
              && after.guardian_departures==static_cast<std::uint64_t>(departed)
              && now->guardian==(departed?Guardian::none:old->guardian),"direct HP departure differs from independent local10percent check");
        if (!after.death_bows && !after.revivals) { check(after.rng==expected_rng,"direct devil consumed selector RNG"); ++stats.no_selector_rng_checks; }
        check(after.next_token==before.next_token+after.revivals
              && std::all_of(after.gifts.begin(),after.gifts.end(),[](auto n) { return n==0; }),
              "devil granted an operation gift or lost its revival-only gift");
        ++stats.revival_gift_checks;
    }
}
}
int main(int argc,char** argv) { try {
    if (argc>5) throw std::invalid_argument("usage: godfield_devil_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000,players=argc>2?number(argv[2]):2,seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if (count>1000000000 || players<2 || players>9 || workers>64 || seed>UINT64_MAX-(count-1)) throw std::invalid_argument("devil benchmark bounds exceeded");
    workers=std::min(workers,count);
    check(match_gift_weight()==499 && match_dream_compatible(6,7),"devil fixture or gift pool changed");
    std::vector<Stats> results(workers); std::vector<std::exception_ptr> errors(workers); std::vector<std::jthread> threads;
    const auto start=std::chrono::steady_clock::now();
    for (std::uint64_t worker=0;worker<workers;++worker) threads.emplace_back([&,worker] { try {
        MatchConfig config; config.max_decisions=64; Match game(config),restored(config); auto& stats=results[worker];
        constexpr std::array<CardId,18> cards{208,112,190,187,235,6,178,184,188,207,209,189,235,203,236,239,66,58};
        constexpr std::array<std::int64_t,5> hps{1,5,15,40,99};
        constexpr std::array<std::int64_t,4> mps{0,95,99,10};
        for (std::uint64_t i=worker;i<count;i+=workers) {
            const auto column=static_cast<std::size_t>(i%5); const auto model=static_cast<CardId>(240+column);
            SnapshotState state; state.gf=17; const bool dead_last=players==9 && i%17==0;
            for (std::uint64_t seat=0;seat<players;++seat) {
                PlayerState p; p.id=seat==0?0:100+7*seat; p.name="P"+std::to_string(seat);
                p.alias="A"+std::to_string(seat); p.user_id="user"+std::to_string(seat);
                p.team=i%3==0?(seat%2?Team::two:Team::one):Team::solo;
                p.hp=hps[(i/5+seat)%hps.size()]; p.mp=mps[(i/5+seat)%mps.size()]; p.cp=(seed+i+seat)%100;
                p.guardian=(i+seat)%3==0?Guardian::none:static_cast<Guardian>(1+(seed+i+seat)%10);
                if ((i+seat)%4==0) p.curses.push_back(Curse::cold);
                if ((i+seat)%3==0) p.curses.push_back(Curse::fog);
                if ((i+seat)%2==0) p.curses.push_back(Curse::dream);
                if ((i+seat)%5==0) p.curses.push_back(Curse::darkcloud);
                p.dead=dead_last && seat+1==players; if (p.dead) p.hp=0;
                p.confusion_count=p.dead?0:static_cast<std::int64_t>((i+seat)%3);
                const auto variant=(i/5+seat)%7;
                const auto size=variant==0?0:variant==1?1:4+(seed+i+seat)%15;
                for (std::uint64_t slot=0;slot<size;++slot) {
                    auto id=cards[slot]; bool used=slot==4;
                    if (slot==0 && (i/5+seat)%2) id=112;
                    if (size==1) { constexpr std::array<CardId,5> one{235,207,208,112,6}; id=one[(i/5+seat)%5]; used=id==235; }
                    ItemInstance item{slot,id,{},used};
                    if (id==6 && (i+seat)%2==0) item.fake_model=7;
                    p.items.push_back(item);
                }
                state.players.push_back(std::move(p));
            }
            const auto alive=alive_players(state);
            const auto owner=alive[(i/5/players)%alive.size()]; const auto target=alive[(i/5+column)%alive.size()];
            game.load_scene(std::move(state),owner,seed+i); audit(game,restored); const auto before=game.snapshot();
            DevilAction action; action.target=target; action.model=model;
            if (model==244) action.resource=static_cast<Resource>((i/5)%3);
            std::vector<CardId> removed;
            if (model==243) {
                const auto& hand=find_player(before.state,target)->items;
                if (hand.size()==1) action.removed={hand.front().token};
                else if (!hand.empty()) { const auto first=(i/5)%hand.size(); action.removed={hand[(first+1)%hand.size()].token,hand[first].token}; }
                for (auto token:action.removed) { const auto it=std::find_if(hand.begin(),hand.end(),[&](const auto& item) { return item.token==token; }); removed.push_back(it->model); }
            }
            check(game.devil_legal(action),"explicit devil fixture is not legal");
            game.devil_step(action); audit(game,restored); verify_proof(before,game.snapshot(),action,removed);
            count_transition(before,game.snapshot()); verify_direct(before,game.snapshot(),action,removed,stats);
            inspect_frames(game.snapshot(),stats);
            std::string notice="\"last_devil\":{\"model\":"+std::to_string(model)+",\"target\":"+std::to_string(target)+",\"removed_models\":[";
            for (std::size_t n=0;n<removed.size();++n) { if(n) notice+=','; notice+=std::to_string(removed[n]); }
            notice+="]}"; check(game.observe({}).find(notice)!=std::string::npos,"public devil notice changed fields/order");
            auto primitive=world_hash(game.snapshot()); mix(primitive,i); mix(primitive,model); mix(primitive,target); stats.primitive_checksum+=primitive;
            std::uint64_t policy=(seed+i)^0x1f83d9abfb41bd6bULL;
            while (!game.done() && game.snapshot().phase!=MatchPhase::main) {
                const auto pending=game.snapshot(); const auto command=game.sample_action(policy); check(game.legal(command),"derived devil response is illegal");
                if (pending.phase==MatchPhase::response) { check(command.kind==CommandKind::defend,"derived response proposed non-defense"); ++stats.responses; }
                else { check(pending.phase==MatchPhase::purchase && (command.kind==CommandKind::accept || command.kind==CommandKind::decline),"unexpected devil pending phase"); ++stats.purchase_commands; }
                game.step(command); audit(game,restored); verify_proof(before,game.snapshot(),action,removed); count_transition(pending,game.snapshot()); inspect_frames(game.snapshot(),stats);
            }
            const auto& final=game.snapshot();
            if (!game.done()) {
                auto expected=owner; const auto* old_owner=find_player(final.state,owner);
                if (old_owner->dead) {
                    const auto old=std::find_if(final.state.players.begin(),final.state.players.end(),[&](const auto& p) { return p.id==owner; });
                    const auto index=static_cast<std::size_t>(old-final.state.players.begin());
                    for (std::size_t n=1;n<=final.state.players.size();++n) { const auto& next=final.state.players[(index+n)%final.state.players.size()]; if(!next.dead) { expected=next.id; break; } }
                }
                check(final.owner==expected && final.actor==expected && final.phase==MatchPhase::main,"devil changed live owner or chose wrong next survivor");
            }
            ++stats.scenes; ++stats.coverage[column]; stats.decisions+=final.decisions; stats.devil_actions+=final.devil_actions;
            stats.terminated+=final.terminated; stats.truncated+=final.truncated; stats.dead_input_scenes+=dead_last;
            stats.owner_is_target+=owner==target; stats.target0_scenes+=target==0;
#define COUNT(name) stats.name+=final.name
            COUNT(chance_rolls); COUNT(chance_hits); COUNT(counter_created); COUNT(counter_started); COUNT(counter_rolls); COUNT(counter_hits);
            COUNT(death_bows); COUNT(death_created); COUNT(death_started); COUNT(death_rolls); COUNT(death_hits); COUNT(revivals); COUNT(redraws); COUNT(redrawn_items);
#undef COUNT
            stats.chance_forced+=final.chance_forced_hits; stats.counter_forced+=final.counter_forced_hits; stats.death_forced+=final.death_forced_hits;
            stats.departure_rolls+=final.guardian_departure_rolls; stats.departures+=final.guardian_departures;
            auto hash=world_hash(final); mix(hash,i); mix(hash,model); mix(hash,target); stats.checksum+=hash;
        }
    } catch (...) { errors[worker]=std::current_exception(); } });
    for (auto& thread:threads) thread.join(); for (const auto& error:errors) if (error) std::rethrow_exception(error);
    Stats total; for (const auto& stats:results) total.append(stats);
    unsigned models_covered=0,resources_covered=0,removal_sizes_covered=0; std::uint64_t covered_scenes=0;
    for (auto n:total.coverage) { models_covered+=n>0; covered_scenes+=n; }
    for (auto n:total.resource_coverage) resources_covered+=n>0;
    for (auto n:total.removal_sizes) removal_sizes_covered+=n>0;
    check(total.scenes==count && total.devil_actions==count && covered_scenes==count
          && models_covered==std::min<std::uint64_t>(count,5) && resources_covered==std::min<std::uint64_t>(count/5,3)
          && total.decisions==count+total.responses+total.purchase_commands && total.removed_copies<=2*total.removal_actions
          && total.resource_gained<=10*total.resource_actions && total.damage_actual<=total.damage_nominal
          && total.chance_hits<=total.chance_rolls && total.counter_hits<=total.counter_rolls
          && total.death_hits<=total.death_rolls && total.terminated+total.truncated<=count,"devil benchmark accounting failed");
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset<<"\nscope=experimental:external-five-devils-with-explicit-target-resource-and-copies;not-apocalypse-gift-selector-or-cascade;world-selected-counter-hashes-plus-full-restore-audit"
             <<"\ndevil_rule="<<devil_rule<<"\napocalypse_rule="<<apocalypse_rule<<"\ngrant_rule="<<grant_rule<<"\ngrant_sampler_rule="<<grant_sampler_rule<<"\nfate_pool_rule="<<fate_pool_rule<<"\nfate_sampler_rule="<<fate_sampler_rule<<"\nconfusion_sampler_rule="<<confusion_sampler_rule<<"\nguardian_payload_sampler_rule="<<guardian_payload_sampler_rule<<"\nscenes="<<total.scenes<<"\nplayers="<<players<<"\nseed="<<seed<<"\nworkers="<<workers
             <<"\nmodels_covered="<<models_covered<<"\nresources_covered="<<resources_covered<<"\nremoval_sizes_covered="<<removal_sizes_covered;
#define OUTPUT(name) std::cout<<"\n" #name "="<<total.name
    OUTPUT(decisions); OUTPUT(responses); OUTPUT(purchase_commands); OUTPUT(terminated); OUTPUT(truncated); OUTPUT(devil_actions);
    OUTPUT(damage_actions); OUTPUT(damage_nominal); OUTPUT(damage_actual); OUTPUT(removal_actions); OUTPUT(removed_copies);
    OUTPUT(removed_used); OUTPUT(removed_fake); OUTPUT(removed_passives); OUTPUT(removed207); OUTPUT(resource_actions); OUTPUT(resource_gained);
    OUTPUT(no_selector_rng_checks); OUTPUT(revival_gift_checks); OUTPUT(dead_input_scenes); OUTPUT(owner_is_target); OUTPUT(target0_scenes);
    OUTPUT(real_frame_observations); OUTPUT(source0_frame_observations); OUTPUT(chance_rolls); OUTPUT(chance_hits); OUTPUT(chance_forced);
    OUTPUT(counter_created); OUTPUT(counter_started); OUTPUT(counter_rolls); OUTPUT(counter_hits); OUTPUT(counter_forced);
    OUTPUT(death_bows); OUTPUT(death_created); OUTPUT(death_started); OUTPUT(death_rolls); OUTPUT(death_hits); OUTPUT(death_forced);
    OUTPUT(revivals); OUTPUT(redraws); OUTPUT(redrawn_items); OUTPUT(departure_rolls); OUTPUT(departures); OUTPUT(primitive_checksum); OUTPUT(checksum);
#undef OUTPUT
    for (std::size_t n=0;n<total.coverage.size();++n) std::cout<<"\nmodel"<<(240+n)<<"_scenes="<<total.coverage[n];
    for (std::size_t n=0;n<3;++n) std::cout<<"\nresource"<<n<<"_scenes="<<total.resource_coverage[n]<<"\nremoved"<<n<<"_scenes="<<total.removal_sizes[n];
    std::cout<<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';
    return 0;
} catch (const std::exception& error) { std::cerr<<"devil benchmark: "<<error.what()<<'\n'; return 1; } }
