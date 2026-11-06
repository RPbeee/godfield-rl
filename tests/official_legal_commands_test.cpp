#include "godfield/official_match.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <tuple>
using namespace godfield::official;
namespace {
void check(bool ok, const char* text) { if (!ok) { std::cerr << text << '\n'; std::exit(1); } }
auto key(const Command& c) {
    return std::tuple{c.kind,c.items,c.target,c.allocation ? c.allocation->hp : -1,
        c.allocation ? c.allocation->mp : -1,c.allocation ? c.allocation->cp : -1};
}
void audit(const Match& game, std::size_t limit) {
    auto before=game.snapshot();
    const auto view=game.observe(before.actor);
    auto actual=game.legal_commands(limit);
    check(actual==game.legal_commands(limit),"deterministic enumeration");
    check(std::is_sorted(actual.begin(),actual.end(),[](const auto& a,const auto& b){return key(a)<key(b);}),"public order");
    std::vector<Command> expected;
    const auto actor=*before.actor;
    const auto& own=*std::find_if(before.state.players.begin(),before.state.players.end(),[&](const auto& p){return p.id==actor;});
    // Deliberately no composition pruning: every kind x ordered token sequence x target.
    Command c{actor,CommandKind::play,{},{}};
    const auto visit=[&](auto&& self)->void {
        for(unsigned kind=0;kind<=9;++kind) if(kind!=4) {
            c.kind=static_cast<CommandKind>(kind); c.target.reset();
            if(game.legal(c)) expected.push_back(c);
            for(const auto& target:before.state.players) {
                c.target=target.id; if(game.legal(c)) expected.push_back(c);
            }
        }
        if(c.items.size()==limit) return;
        for(const auto& item:own.items) if(std::find(c.items.begin(),c.items.end(),item.token)==c.items.end()) {
            c.items.push_back(item.token); self(self); c.items.pop_back();
        }
    };
    visit(visit);
    std::sort(expected.begin(),expected.end(),[](const auto& a,const auto& b){return key(a)<key(b);});
    check(actual==expected,"exhaustive native oracle equality");
    for(const auto& command:actual) check(game.legal(command),"enumerated command legal");
    auto rng=std::uint64_t{42};
    for(int i=0;i<8;++i) {
        const auto sample=game.sample_action(rng);
        if(sample.items.size()<=limit && sample.kind!=CommandKind::exchange)
            check(std::find(actual.begin(),actual.end(),sample)!=actual.end(),"sample membership");
    }
    Match restored(before.config); restored.restore(before);
    check(restored.legal_commands(limit)==actual && game.snapshot()==before && game.observe(before.actor)==view,"snapshot/restore and purity");
}
void rejects(const Match& game, std::size_t count, int step, MatchErrorCode code) {
    const auto before=game.snapshot();bool threw=false;
    try{(void)game.legal_commands(count,step);}catch(const MatchError& e){threw=e.code==code;}
    check(threw && before==game.snapshot(),"rejection code and purity");
}
}
int main(int argc,char**) {
    if(argc>1) {
        for(std::size_t limit=1;limit<=3;++limit) {
            double elapsed=0;std::size_t sum=0,maximum=0;
            for(std::uint64_t seed=0;seed<200;++seed) {
                Match game;game.reset(std::vector<PlayerSetup>{{0,Team::solo,"a"},{77,Team::solo,"b"}},seed);
                auto start=std::chrono::steady_clock::now();
                const auto commands=game.legal_commands(limit);
                elapsed+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
                sum+=commands.size();maximum=std::max(maximum,commands.size());
            }
            std::cout<<"max_items="<<limit<<" scenes=200 mean="<<sum/200.0<<" max="<<maximum<<" us/scene="<<elapsed*1e6/200<<'\n';
        }
        return 0;
    }
    std::size_t scenes=0;
    for(bool grants:{false,true}) for(bool fate:{false,true}) for(std::uint64_t seed=0;seed<4;++seed) {
        MatchConfig config;config.external_grants=grants;config.include_fate=fate;
        Match game(config);game.reset(std::vector<PlayerSetup>{{0,Team::solo,"a"},{77,Team::solo,"b"}},seed);
        std::uint64_t policy=seed+1,chance=seed+2;
        for(int i=0;i<12 && !game.done();++i) {
            auto s=game.snapshot();
            if(s.phase!=MatchPhase::grant && !(s.phase==MatchPhase::main && s.state.players[s.owner==0?0:1].confusion_count)) {
                audit(game,2);if(i==0 && seed==0)audit(game,3);++scenes;
            }
            (void)game.rollout_step(policy,chance);
        }
    }
    // Explicit compositions, used miracles, and apocalypse discard.
    for(bool apocalypse:{false,true}) {
        SnapshotState state;PlayerState own;own.id=0;own.mp=0;
        own.items={{0,235,{},true},{1,204,{},false},{2,199,{},false},{3,200,{},false}};
        PlayerState enemy;enemy.id=77;state.players={own,enemy};
        if(apocalypse)state.tiebreak_gf=0;
        Match fixture;fixture.load_scene(state,0,42);audit(fixture,3);audit(fixture,4);
    }
    for(int step:{1,7,99,2147483647}) {
        SnapshotState state;PlayerState own;own.id=0;own.items={{0,3,{},false}};
        own.hp=99;own.mp=98;own.cp=97;PlayerState enemy;enemy.id=77;state.players={own,enemy};
        Match fixture;fixture.load_scene(state,0,42);const auto before=fixture.snapshot();
        auto commands=fixture.legal_commands(1,step);std::vector<ResourceAllocation> actual,expected;
        for(const auto& c:commands) {check(fixture.legal(c),"exchange legal");if(c.allocation)actual.push_back(*c.allocation);}
        for(int hp=0;hp<=99;hp+=step)for(int mp=0;mp<=99;mp+=step)for(int cp=0;cp<=99;++cp)
            if(hp+mp+cp==294)expected.push_back({hp,mp,cp});
        check(actual==expected && fixture.snapshot()==before,"exchange grid conserves sum and state");
    }
    MatchConfig grant_config;grant_config.external_artifacts=true;grant_config.initial_hand=0;
    Match grant(grant_config);(void)grant.step({0,CommandKind::pray,{},{}});
    rejects(grant,2,0,MatchErrorCode::unsupported);
    Match game;
    rejects(game,0,0,MatchErrorCode::invalid);rejects(game,5,0,MatchErrorCode::invalid);
    rejects(game,2,-1,MatchErrorCode::invalid);rejects(game,2,-2147483647,MatchErrorCode::invalid);
    auto state=game.snapshot().state;state.players[0].confusion_count=3;game.load_scene(state,0,1);
    rejects(game,2,0,MatchErrorCode::unsupported);
    MatchConfig config;config.max_decisions=1;Match finished(config);std::uint64_t rng=1;
    (void)finished.step(finished.sample_action(rng));rejects(finished,2,0,MatchErrorCode::invalid);
    std::cout<<"native oracle scenes="<<scenes<<'\n';
}
