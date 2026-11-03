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
// hash: Ordinary pending journals are checked by full restore.
std::uint64_t world_hash(const MatchSnapshot& s) {
    std::uint64_t hash=14695981039346656037ULL;
    mix(hash,s.version); mix(hash,s.state.gf); mix(hash,s.state.update_count); mix(hash,s.state.is_over);
    mix(hash,s.state.tiebreak_gf.has_value()); if(s.state.tiebreak_gf) mix(hash,*s.state.tiebreak_gf);
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
    mix(hash,s.config.include_fate); mix(hash,s.config.external_grants); mix(hash,s.grant_resolutions); mix(hash,s.grant_artifacts); mix(hash,s.grant_devils);
    mix(hash,s.terminated); mix(hash,s.truncated); mix(hash,s.phenomenon_actions); mix(hash,s.fate_actions); mix(hash,s.devil_actions);
    mix(hash,s.chance_rolls); mix(hash,s.chance_hits); mix(hash,s.chance_forced_hits);
    mix(hash,s.counter_created); mix(hash,s.counter_started); mix(hash,s.counter_rolls); mix(hash,s.counter_hits); mix(hash,s.counter_forced_hits);
    mix(hash,s.death_bows); mix(hash,s.death_created); mix(hash,s.death_started); mix(hash,s.death_rolls); mix(hash,s.death_hits); mix(hash,s.death_forced_hits);
    mix(hash,s.revivals); mix(hash,s.redraws); mix(hash,s.redrawn_items); mix(hash,s.dream_rolls); mix(hash,s.dream_masks);
    mix(hash,s.guardian_summons); mix(hash,s.guardian_opportunities); mix(hash,s.auto_guardian_actions);
    mix(hash,s.guardian_departure_rolls); mix(hash,s.guardian_departures);
    return hash;
}




void audit(const Match& game,Match& restored) {
    restored.restore(game.snapshot());check(restored.snapshot()==game.snapshot(),"fate pool checkpoint failed full restore");
}
std::uint64_t reference_random(std::uint64_t& state) {
    auto word=(state+=0x9e3779b97f4a7c15ULL);
    word=(word^(word>>30))*0xbf58476d1ce4e5b9ULL;word=(word^(word>>27))*0x94d049bb133111ebULL;
    return word^(word>>31);
}
std::uint64_t reference_bounded(std::uint64_t& state,std::uint64_t count) {
    check(count!=0,"empty reference selection");const auto threshold=(std::uint64_t{0}-count)%count;
    std::uint64_t word;do {word=reference_random(state);}while(word<threshold);return word%count;
}
const std::vector<CardId>& reference_tickets() {
    static const auto result=[] {
        std::vector<CardId> values;
        for(const auto& card:catalog())if(card.gift_rate>0)for(int i=0;i<card.gift_rate;++i)values.push_back(card.id);
        check(values.size()==500,"full positive-gift reference weight is not500");return values;
    }();return result;
}
const std::vector<CardId>& reference_faces(CardId model) {
    static const auto choices=[] {
        std::array<std::vector<CardId>,catalog_count+1> out;
        for(const auto& actual:catalog())for(const auto& face:catalog())if(match_dream_compatible(actual.id,face.id))out[actual.id].push_back(face.id);
        return out;
    }();return choices[model];
}
struct Receipt {PlayerId owner=0;ItemInstance item;};
struct Plan {std::vector<Receipt> receipts;std::uint64_t rng=0,next_token=0;};
void reference_gift(Plan& plan,PlayerId owner,bool dream) {
    const auto& pool=reference_tickets();const auto model=pool[reference_bounded(plan.rng,pool.size())];
    std::optional<CardId> fake;
    if(dream) {const auto& faces=reference_faces(model);if(!faces.empty() && reference_bounded(plan.rng,2)==0)fake=faces[reference_bounded(plan.rng,faces.size())];}
    plan.receipts.push_back({owner,{plan.next_token++,model,fake,false}});
}
Plan reference_plan(std::size_t route,std::uint64_t seed,std::uint64_t players) {
    Plan out;out.rng=seed;out.next_token=route==2 || route==3?3:(route==4?1:0);
    if(route==0)for(std::uint64_t seat=0;seat<players;++seat)for(unsigned slot=0;slot<9;++slot)reference_gift(out,seat?100+7*seat:0,false);
    else if(route==2)for(unsigned slot=0;slot<3;++slot)reference_gift(out,0,false);
    else if(route==3) {
        reference_gift(out,107,true);reference_gift(out,107,true); // Two surviving copies are redrawn before the body.
        reference_gift(out,0,false);reference_gift(out,107,true); // Reserved gifts follow input seat order.
    } else reference_gift(out,0,false);
    return out;
}
std::uint64_t probe_seed(std::size_t route,std::uint64_t players) {
    for(std::uint64_t seed=0;seed<65536;++seed) {
        const auto plan=reference_plan(route,seed,players);
        if(std::any_of(plan.receipts.begin(),plan.receipts.end(),[](const auto& receipt){return receipt.item.model==207;}))return seed;
    }
    throw std::logic_error("fixed independent reference seed search did not reach207");
}
struct Stats {
    std::uint64_t scenes=0,decisions=0,player_commands=0,responses=0,grant_commands=0,devil_calls=0,fate_calls=0;
    std::uint64_t terminated=0,truncated=0,drawn_copies=0,observed_copies=0,fate207_copies=0,fate207_owner_scenes=0;
    std::uint64_t used_input_copies=0,redraws=0,redrawn_items=0,revivals=0,guardian_opportunities=0,auto_guardian_actions=0;
    std::uint64_t chance_rolls=0,chance_hits=0,chance_forced=0,counters=0,death_bows=0,death_created=0;
    std::uint64_t reference_probes=0,reference_copies=0,reference207=0,reference_checksum=0;
    std::uint64_t pool_checksum=0,primitive_checksum=0,checksum=0,sampler_exclusion_checks=0;
    std::array<std::uint64_t,6> origins{},route207{};
    std::array<std::uint64_t,catalog_count+1> seen_models{};
    void append(const Stats& other) {
#define ADD(n) n+=other.n
        ADD(scenes);ADD(decisions);ADD(player_commands);ADD(responses);ADD(grant_commands);ADD(devil_calls);ADD(fate_calls);
        ADD(terminated);ADD(truncated);ADD(drawn_copies);ADD(observed_copies);ADD(fate207_copies);ADD(fate207_owner_scenes);
        ADD(used_input_copies);ADD(redraws);ADD(redrawn_items);ADD(revivals);ADD(guardian_opportunities);ADD(auto_guardian_actions);
        ADD(chance_rolls);ADD(chance_hits);ADD(chance_forced);ADD(counters);ADD(death_bows);ADD(death_created);
        ADD(reference_probes);ADD(reference_copies);ADD(reference207);ADD(reference_checksum);ADD(pool_checksum);ADD(primitive_checksum);ADD(checksum);ADD(sampler_exclusion_checks);
#undef ADD
        for(std::size_t i=0;i<origins.size();++i){origins[i]+=other.origins[i];route207[i]+=other.route207[i];}
        for(std::size_t i=0;i<seen_models.size();++i)seen_models[i]+=other.seen_models[i];
    }
};
void collect(const MatchSnapshot& snapshot,ItemToken first_new,std::vector<ItemToken>& seen,Stats& stats,std::size_t route) {
    for(const auto& player:snapshot.state.players)for(const auto& item:player.items)if(item.token>=first_new
        && std::find(seen.begin(),seen.end(),item.token)==seen.end()) {
        check(item.model && find_card(item.model)->gift_rate>0,"fresh receipt outside the full ordinary gift pool");
        seen.push_back(item.token);++stats.observed_copies;++stats.seen_models[item.model];
        if(item.model==207) {
            check(!item.used && !item.fake_model,"ordinary207 was retained or Dream masked");++stats.fate207_copies;++stats.route207[route];
        }
    }
}
void follow(Match& game,Match& restored,std::uint64_t& policy,Stats& stats,ItemToken first_new,std::vector<ItemToken>& seen,std::size_t route) {
    while(!game.done() && game.snapshot().phase!=MatchPhase::main) {
        if(game.snapshot().phase==MatchPhase::grant) {
            const GrantAction action{*game.snapshot().pending_grant,0,{},{}};check(game.grant_legal(action),"normal full-pool grant choice is illegal");
            game.grant_step(action);++stats.grant_commands;
        } else {
            check(game.snapshot().phase==MatchPhase::response || game.snapshot().phase==MatchPhase::purchase,"unexpected full-pool follow-up phase");
            const bool response=game.snapshot().phase==MatchPhase::response;
            const auto command=game.sample_action(policy);check(game.legal(command),"full-pool response proposal is illegal");
            game.step(command);++stats.player_commands;stats.responses+=response;
        }
        audit(game,restored);collect(game.snapshot(),first_new,seen,stats,route);
    }
}
SnapshotState fixture(std::size_t route,std::uint64_t players,std::uint64_t index,bool clean) {
    SnapshotState state;state.gf=17;if(route==2 || route==5)state.tiebreak_gf=17;
    for(std::uint64_t seat=0;seat<players;++seat) {
        PlayerState p;p.id=seat?100+7*seat:0;p.name="F"+std::to_string(seat);p.hp=99;p.mp=99;p.cp=20;
        p.user_id="user"+std::to_string(seat);p.alias="alias"+std::to_string(seat);
        p.team=!clean && index%3==0?(seat%2?Team::two:Team::one):Team::solo;
        if(!clean) {
            p.guardian=static_cast<Guardian>(1+(index+seat)%10);
            if((index+seat)%4==0)p.curses.push_back(Curse::cold);
            if((index+seat)%3==0 && !(route==3 && seat==0))p.curses.push_back(Curse::fog);
            if((index+seat)%2==0)p.curses.push_back(Curse::dream);
            if((index+seat)%7==0)p.curses.push_back(Curse::darkcloud);
            p.confusion_count=(index+seat)%3;
            if(players==9 && index%17==0 && seat+1==players){p.dead=true;p.hp=0;p.confusion_count=0;}
            if(seat>1)p.items={{0,190,{},false},{1,235,{},true},{2,208,{},false},{3,112,{},false}};
        }
        if(seat==0) {
            if(route==1)p.items={};
            if(route==2)p.items={{0,199,{},false},{1,235,{},true},{2,203,{},false}};
            if(route==3)p.items={{0,6,{},false}};
            if(route==4){p.hp=1;p.items={{0,208,{},false}};}
            if(route==5)p.items={};
        }
        if(seat==1 && route==3)p.items={{0,178,{},false},{1,235,{},true},{2,199,{},false}};
        state.players.push_back(std::move(p));
    }return state;
}
void execute_origin(Match& game,Match& restored,std::size_t route,Stats& stats,ItemToken first_new,std::vector<ItemToken>& seen,bool single_sacrifice=false) {
    const auto submit=[&](const Command& command) {
        const auto* owner=find_player(game.snapshot().state,command.actor);
        const bool confused=game.snapshot().phase==MatchPhase::main && owner->confusion_count>0;
        check(confused?game.confusion_legal(command):game.legal(command),"full-pool origin command is illegal");
        if(confused)game.confusion_step(command);else game.step(command);++stats.player_commands;
        audit(game,restored);collect(game.snapshot(),first_new,seen,stats,route);
    };
    if(route==1 || route==5)submit({0,CommandKind::pray,{},{}});
    else if(route==2)submit({0,CommandKind::sacrifice,single_sacrifice?std::vector<ItemToken>{0}:std::vector<ItemToken>{0,1,2},{}});
    else if(route==3) {
        submit({0,CommandKind::play,{0},107});
        check(game.snapshot().phase==MatchPhase::response && game.snapshot().actor==107,"redraw origin did not reach the supplied hat defender");
        submit({107,CommandKind::defend,{0},{}});++stats.responses;
    } else if(route==4) {
        check(game.devil_legal({0,240,{},{}}),"revival origin is illegal");game.devil_step({0,240,{},{}});++stats.devil_calls;
        audit(game,restored);collect(game.snapshot(),first_new,seen,stats,route);
    }
}
void use_owned_fate(Match& game,Match& restored,std::uint64_t& policy,Stats& stats,ItemToken first_new,std::vector<ItemToken>& seen,std::size_t route) {
    if(game.done() || game.snapshot().phase!=MatchPhase::main)return;
    const auto before=game.snapshot();const auto* own=find_player(before.state,before.owner);
    const auto item=std::find_if(own->items.begin(),own->items.end(),[](const auto& copy){return copy.model==207;});
    if(item==own->items.end())return;
    ++stats.fate207_owner_scenes;
    const Command ordinary{own->id,CommandKind::play,{item->token},{}};
    check(!game.legal(ordinary),"normal play incorrectly resolved a random phenomenon");
    if(!own->confusion_count) {
        auto proposal_rng=policy;const auto proposed=game.sample_action(proposal_rng);
        check(!(proposed.kind==CommandKind::play && !proposed.items.empty() && proposed.items.front()==item->token),"ordinary policy started207 without an outcome");
        ++stats.sampler_exclusion_checks;
    }
    const auto token=item->token;const FateAction action{own->id,token,{288,{},{}},{}};
    check(game.fate_legal(action),"gifted207 cannot be used through the explicit Fate API");game.fate_step(action);++stats.fate_calls;
    audit(game,restored);collect(game.snapshot(),first_new,seen,stats,route);
    check(game.snapshot().fate_actions==1 && game.snapshot().last_fate && game.snapshot().last_fate_event
          && game.snapshot().last_fate_event->phenomenon==288
          && std::none_of(find_player(game.snapshot().state,own->id)->items.begin(),find_player(game.snapshot().state,own->id)->items.end(),[&](const auto& copy){return copy.token==token;}),
          "explicit Fate did not consume its gifted original token once");
    follow(game,restored,policy,stats,first_new,seen,route);
    if(!game.done())check(game.snapshot().state.gf==before.state.gf+1,"explicit gifted Fate did not finish one ordinary turn");
}
void verify_plan(const MatchSnapshot& actual,const Plan& plan,Stats& stats,std::size_t route,std::uint64_t seed) {
    check(actual.rng==plan.rng && actual.next_token==plan.next_token,"independent full-pool reference RNG/token progression differs");
    std::size_t copies=0;for(const auto& player:actual.state.players)copies+=player.items.size();
    check(copies==plan.receipts.size(),"clean gift route kept an unexpected old copy or missed a receipt");
    bool found=false;std::uint64_t hash=14695981039346656037ULL;mix(hash,route);mix(hash,seed);
    for(const auto& receipt:plan.receipts) {
        const auto* owner=find_player(actual.state,receipt.owner);
        const auto item=std::find_if(owner->items.begin(),owner->items.end(),[&](const auto& copy){return copy.token==receipt.item.token;});
        check(item!=owner->items.end() && *item==receipt.item,"clean route differs from the independent ordinary weighted/dream reference");
        found|=item->model==207;mix(hash,receipt.owner);item_hash(hash,*item);
    }
    check(found,"fixed route probe failed to deliver an actual207");
    ++stats.reference_probes;stats.reference_copies+=copies;++stats.reference207;stats.reference_checksum+=hash;
}
}
int main(int argc,char** argv) {try {
    if(argc>5)throw std::invalid_argument("usage: godfield_fate_pool_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000,players=argc>2?number(argv[2]):2,seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if(count>1000000000 || players<2 || players>9 || workers>64 || seed>UINT64_MAX-(count-1))throw std::invalid_argument("fate pool benchmark bounds exceeded");workers=std::min(workers,count);
    MatchConfig config;config.max_decisions=96;config.include_fate=true;config.initial_hp=99;config.initial_mp=99;config.initial_cp=20;
    Match descriptor(config),old;
    check(descriptor.gift_weight()==500 && descriptor.gift_catalog().size()==237 && descriptor.gift_supported(207)
          && old.gift_weight()==499 && old.gift_catalog()==match_catalog() && !old.gift_supported(207)
          && !match_supported(207) && !match_guardian_payload_supported(Guardian::earth,207),"opt-in pool changed default/ordinary/guardian command capability");
    std::vector<CardId> expected;for(const auto& card:catalog())if(card.gift_rate>0)expected.push_back(card.id);
    check(descriptor.gift_catalog()==expected && std::count(reference_tickets().begin(),reference_tickets().end(),207)==1,"opt-in gift descriptor is not the exact full positive-weight catalog");
    std::uint64_t pool_hash=14695981039346656037ULL;for(auto id:expected){mix(pool_hash,id);mix(pool_hash,find_card(id)->gift_rate);}
    Stats probes;
    for(std::size_t route=0;route<6;++route) {
        const auto probe=probe_seed(route,players);auto cfg=config;cfg.external_grants=route==5;
        Match game(cfg),restored(cfg);std::vector<PlayerSetup> setups;for(std::uint64_t seat=0;seat<players;++seat)setups.push_back({seat?100+7*seat:0,Team::solo,"F"+std::to_string(seat)});
        if(route==0)game.reset(setups,probe);else game.load_scene(fixture(route,players,0,true),0,probe);
        audit(game,restored);const auto start_token=route==0?0:game.snapshot().next_token;std::vector<ItemToken> seen;
        collect(game.snapshot(),start_token,seen,probes,route);execute_origin(game,restored,route,probes,start_token,seen);
        std::uint64_t policy=probe^0x1f83d9abfb41bd6bULL;follow(game,restored,policy,probes,start_token,seen,route);
        verify_plan(game.snapshot(),reference_plan(route,probe,players),probes,route,probe);
    }
    std::vector<Stats> results(workers);std::vector<std::exception_ptr> errors(workers);std::vector<std::jthread> threads;
    const auto start=std::chrono::steady_clock::now();
    for(std::uint64_t worker=0;worker<workers;++worker)threads.emplace_back([&,worker]{try {
        auto& stats=results[worker];
        for(std::uint64_t i=worker;i<count;i+=workers) {
            const auto route=static_cast<std::size_t>(i%6);auto cfg=config;cfg.external_grants=route==5;
            Match game(cfg),restored(cfg);
            if(route==0) {
                std::vector<PlayerSetup> setups;for(std::uint64_t seat=0;seat<players;++seat)setups.push_back({seat?100+7*seat:0,Team::solo,"F"+std::to_string(seat)});
                game.reset(setups,seed+i);
            }else game.load_scene(fixture(route,players,i,false),0,seed+i);
            audit(game,restored);const auto initial=game.snapshot();const auto first_new=route==0?0:initial.next_token;
            for(const auto& p:initial.state.players)for(const auto& item:p.items)stats.used_input_copies+=item.used;
            std::vector<ItemToken> seen;collect(initial,first_new,seen,stats,route);
            execute_origin(game,restored,route,stats,first_new,seen,(i/6)%2==0);
            std::uint64_t policy=(seed+i)^0x5be0cd19137e2179ULL;follow(game,restored,policy,stats,first_new,seen,route);
            if(!game.done())check(game.snapshot().state.gf==(route==0?1:(route==4?17:18)),"gift route advanced the wrong original ordinary/external schedule");
            auto primitive=world_hash(game.snapshot());mix(primitive,i);mix(primitive,route);stats.primitive_checksum+=primitive;
            use_owned_fate(game,restored,policy,stats,first_new,seen,route);
            const auto& final=game.snapshot();audit(game,restored);
            check(final.phenomenon_actions==0 && final.guardian_actions==0 && final.guardian_payload_actions==0
                  && final.devil_actions==(route==4) && final.fate_actions<=1,"pool opt-in silently introduced an automatic phenomenon or external selector");
            if(route==0)check(final.next_token>=players*9,"reset did not draw the configured nine-copy hands");
            ++stats.scenes;++stats.origins[route];stats.decisions+=final.decisions;stats.terminated+=final.terminated;stats.truncated+=final.truncated;
            stats.drawn_copies+=final.next_token-(route==0?0:initial.next_token);stats.pool_checksum+=pool_hash;
            stats.redraws+=final.redraws;stats.redrawn_items+=final.redrawn_items;stats.revivals+=final.revivals;
            stats.guardian_opportunities+=final.guardian_opportunities;stats.auto_guardian_actions+=final.auto_guardian_actions;
            stats.chance_rolls+=final.chance_rolls;stats.chance_hits+=final.chance_hits;stats.chance_forced+=final.chance_forced_hits;
            stats.counters+=final.counter_created;stats.death_bows+=final.death_bows;stats.death_created+=final.death_created;
            auto hash=world_hash(final);mix(hash,i);mix(hash,route);stats.checksum+=hash;
        }
    }catch(...){errors[worker]=std::current_exception();}});
    for(auto& thread:threads)thread.join();for(const auto& error:errors)if(error)std::rethrow_exception(error);
    Stats total;for(const auto& stats:results)total.append(stats);
    std::uint64_t routes=0,distinct=0;for(auto n:total.origins)routes+=n;for(auto n:total.seen_models)distinct+=n!=0;
    check(total.scenes==count && routes==count && total.decisions==total.player_commands+total.grant_commands+total.devil_calls+total.fate_calls
          && total.observed_copies<=total.drawn_copies && total.fate_calls==total.fate207_owner_scenes
          && total.terminated+total.truncated<=count && probes.reference_probes==6,"fate pool aggregate accounting failed");
    if(count>=10000)check(distinct==237 && total.fate_calls>0,"large fate pool fixture did not cover all237 gifted models and explicit gifted Fate use");
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset<<"\nscope=experimental:opt-in-full237-weight500-gift-paths;explicit-owned207-outcome288-use;not-automatic-phenomenon-selection-or-official-server-rng;world-selected-counter-hashes-plus-full-snapshot-restore-audit"
             <<"\nfate_pool_rule="<<fate_pool_rule<<"\nscenes="<<total.scenes<<"\nplayers="<<players<<"\nseed="<<seed<<"\nworkers="<<workers<<"\nmax_decisions=96\ngift_models=237\ngift_weight=500\ndistinct_gift_models="<<distinct;
#define OUTPUT(n) std::cout<<"\n" #n "="<<total.n
    OUTPUT(decisions);OUTPUT(player_commands);OUTPUT(responses);OUTPUT(grant_commands);OUTPUT(devil_calls);OUTPUT(fate_calls);OUTPUT(terminated);OUTPUT(truncated);
    OUTPUT(drawn_copies);OUTPUT(observed_copies);OUTPUT(fate207_copies);OUTPUT(fate207_owner_scenes);OUTPUT(used_input_copies);OUTPUT(redraws);OUTPUT(redrawn_items);OUTPUT(revivals);
    OUTPUT(guardian_opportunities);OUTPUT(auto_guardian_actions);OUTPUT(chance_rolls);OUTPUT(chance_hits);OUTPUT(chance_forced);OUTPUT(counters);OUTPUT(death_bows);OUTPUT(death_created);OUTPUT(sampler_exclusion_checks);
    OUTPUT(pool_checksum);OUTPUT(primitive_checksum);OUTPUT(checksum);
#undef OUTPUT
    std::cout<<"\nreference_probes="<<probes.reference_probes<<"\nreference_copies="<<probes.reference_copies<<"\nreference207_routes="<<probes.reference207<<"\nreference_checksum="<<probes.reference_checksum;
    for(std::size_t route=0;route<6;++route)std::cout<<"\norigin"<<route<<"_scenes="<<total.origins[route]<<"\norigin"<<route<<"_207_copies="<<total.route207[route];
    std::cout<<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<"fate pool benchmark: "<<error.what()<<'\n';return 1;}}
