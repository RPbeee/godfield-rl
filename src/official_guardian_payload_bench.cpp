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
#include <vector>

namespace {
using namespace godfield::official;
std::uint64_t number(const char* text,bool positive=true){std::uint64_t n=0;const std::string s(text);const auto r=std::from_chars(s.data(),s.data()+s.size(),n);if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size()||(positive&&!n))throw std::invalid_argument("invalid argument");return n;}
void mix(std::uint64_t& hash,std::uint64_t value){for(unsigned i=0;i<8;++i){hash^=(value>>(i*8))&255;hash*=1099511628211ULL;}}
struct Stats {
    std::uint64_t scenes=0,decisions=0,responses=0,terminal=0,truncated=0,checksum=0,rolls=0,hits=0,forced=0,redraws=0,departure_rolls=0,departures=0;
    std::array<std::uint64_t,152> models{};
    void append(const Stats& s){scenes+=s.scenes;decisions+=s.decisions;responses+=s.responses;terminal+=s.terminal;truncated+=s.truncated;checksum+=s.checksum;rolls+=s.rolls;hits+=s.hits;forced+=s.forced;redraws+=s.redraws;departure_rolls+=s.departure_rolls;departures+=s.departures;for(unsigned i=0;i<152;++i)models[i]+=s.models[i];}
};
void audit(const Match& m){Match copy(m.snapshot().config);copy.restore(m.snapshot());if(copy.snapshot()!=m.snapshot())throw std::logic_error("guardian checkpoint does not roundtrip");}
}
int main(int argc,char** argv){try{
    if(argc>5)throw std::invalid_argument("usage: godfield_guardian_payload_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000,players=argc>2?number(argv[2]):2,seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if(count>1000000000||players<2||players>9||workers>64||seed>UINT64_MAX-(count-1))throw std::invalid_argument("guardian benchmark bounds exceeded");workers=std::min(workers,count);
    std::vector<GuardianPayloadAction> actions;
    for(auto guardian:{Guardian::earth,Guardian::moon})for(const auto& card:catalog())if(match_guardian_payload_supported(guardian,card.id))
        actions.push_back({100,guardian,{card.id},(card.atk>0||targets_enemy_by_default(card.id))?107u:100u});
    if(actions.size()!=146)throw std::logic_error("ordinary payload base set changed");
    for(auto models:std::vector<std::vector<CardId>>{{6,203},{6,66,203},{14,83,203},{58,66,203},{27,83},{53,203}})
        actions.push_back({100,Guardian::earth,std::move(models),107});
    std::vector<Stats> stats(workers);std::vector<std::exception_ptr> errors(workers);std::vector<std::jthread> threads;
    const auto start=std::chrono::steady_clock::now();
    for(std::uint64_t worker=0;worker<workers;++worker)threads.emplace_back([&,worker]{try{
        MatchConfig config;config.max_decisions=64;Match m(config);
        for(std::uint64_t i=worker;i<count;i+=workers){const auto column=i%actions.size();const auto& action=actions[column];const auto model=action.models.front();SnapshotState state;
            for(unsigned seat=0;seat<players;++seat){PlayerState p;p.id=100+seat*7;p.name="P"+std::to_string(seat);p.hp=p.mp=99;p.cp=static_cast<std::int64_t>((seed+i+seat)%100);
                for(auto id:{CardId{190},CardId{189},CardId{178},CardId{184},CardId{235}})p.items.push_back({p.items.size(),id,{},false});state.players.push_back(std::move(p));}
            state.players[0].guardian=action.guardian;
            // Deterministic exogenous scenario generation, not a guardian scheduler.
            if(i%3==0)state.players[1].curses={Curse::darkcloud};
            m.load_scene(std::move(state),107,seed+i);std::uint64_t policy=(seed+i)^0x6a09e667f3bcc909ULL;
            if(!m.guardian_payload_legal(action))throw std::logic_error("ordinary guardian scenario not legal");m.guardian_payload_step(action);audit(m);++stats[worker].models[column];
            while(!m.done()&&m.snapshot().phase!=MatchPhase::main){const auto command=m.sample_action(policy);if(!m.legal(command))throw std::logic_error("guardian response policy not legal");m.step(command);audit(m);++stats[worker].responses;}
            const auto& s=m.snapshot();if(s.guardian_payload_actions!=1||s.guardian_actions||s.auto_guardian_actions||s.guardian_opportunities||s.state.gf!=0||s.last_tick)throw std::logic_error("external action changed ordinary schedule");
            ++stats[worker].scenes;stats[worker].decisions+=s.decisions;stats[worker].terminal+=s.terminated;stats[worker].truncated+=s.truncated;
            stats[worker].rolls+=s.chance_rolls;stats[worker].hits+=s.chance_hits;stats[worker].forced+=s.chance_forced_hits;stats[worker].redraws+=s.redraws;stats[worker].departure_rolls+=s.guardian_departure_rolls;stats[worker].departures+=s.guardian_departures;
            std::uint64_t hash=14695981039346656037ULL;mix(hash,i);mix(hash,model);mix(hash,static_cast<unsigned>(action.guardian));for(auto part:action.models)mix(hash,part);mix(hash,s.rng);mix(hash,s.decisions);mix(hash,s.owner);mix(hash,s.terminated);mix(hash,s.truncated);
            for(const auto& p:s.state.players){mix(hash,p.id);mix(hash,p.hp);mix(hash,p.mp);mix(hash,p.cp);mix(hash,p.dead);mix(hash,static_cast<unsigned>(p.guardian));mix(hash,p.curses.size());for(auto curse:p.curses)mix(hash,static_cast<unsigned>(curse));mix(hash,p.items.size());for(const auto& item:p.items){mix(hash,item.token);mix(hash,item.model);mix(hash,item.fake_model.value_or(0));mix(hash,item.used);}}
            stats[worker].checksum+=hash;
        }
    }catch(...){errors[worker]=std::current_exception();}});
    for(auto& t:threads)t.join();for(const auto& e:errors)if(e)std::rethrow_exception(e);Stats total;for(const auto& s:stats)total.append(s);
    std::uint64_t models=0;unsigned covered=0,bases=0;for(unsigned j=0;j<152;++j){const auto n=total.models[j];models+=n;covered+=n>0;if(j<146)bases+=n>0;}
    if(total.scenes!=count||models!=count||total.decisions!=count+total.responses||total.hits>total.rolls||total.terminal+total.truncated>count||covered!=std::min<std::uint64_t>(count,152)||bases!=std::min<std::uint64_t>(count,146))throw std::logic_error("guardian benchmark accounting mismatch");
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset<<"\nscope=experimental:external146-ordinary-guardian-bases-and6-earth-compounds;not-general-selector\nguardian_payload_rule="<<guardian_payload_rule<<"\nphenomenon_rule="<<phenomenon_rule << "\nitem_shuffle_rule=" << item_shuffle_rule << "\nconfusion_rule=" << confusion_rule << "\nfate_rule=" << fate_rule << "\ndevil_rule=" << devil_rule << "\napocalypse_rule=" << apocalypse_rule << "\ngrant_rule=" << grant_rule << "\ngrant_sampler_rule=" << grant_sampler_rule << "\nfate_pool_rule=" << fate_pool_rule << "\nfate_sampler_rule=" << fate_sampler_rule << "\nconfusion_sampler_rule=" << confusion_sampler_rule << "\nguardian_payload_sampler_rule=" << guardian_payload_sampler_rule
        <<"\nscenes="<<total.scenes<<"\nplayers="<<players<<"\nseed="<<seed<<"\nworkers="<<workers<<"\nmodels_covered="<<bases<<"\ncolumns_covered="<<covered<<"\ndecisions="<<total.decisions<<"\nresponses="<<total.responses
        <<"\nterminated="<<total.terminal<<"\ntruncated="<<total.truncated<<"\nchance_rolls="<<total.rolls<<"\nchance_hits="<<total.hits<<"\nchance_forced="<<total.forced<<"\nredraws="<<total.redraws<<"\nguardian_departure_rolls="<<total.departure_rolls<<"\nguardian_departures="<<total.departures<<"\nchecksum="<<total.checksum<<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';
    return 0;
}catch(const std::exception& e){std::cerr<<"guardian benchmark: "<<e.what()<<'\n';return 1;}}
