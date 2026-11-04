#include "godfield/official_match.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <string>

namespace {
using namespace godfield::official;
void check(bool value,const char* message){if(!value){std::cerr<<"FAIL local driver: "<<message<<'\n';std::exit(1);}}
PlayerState person(PlayerId id,std::initializer_list<CardId> ids={}){PlayerState p;p.id=id;p.name="p"+std::to_string(id);p.hp=p.mp=p.cp=99;for(auto model:ids)p.items.push_back({p.items.size(),model,{},false});return p;}
SnapshotState scene(std::initializer_list<CardId> own={},std::initializer_list<CardId> other={}){SnapshotState s;s.gf=17;s.players={person(0,own),person(7,other)};return s;}
Match match(SnapshotState state,MatchConfig config={},std::uint64_t seed=42){Match m(config);m.load_scene(std::move(state),0,seed);return m;}
Command prayer(PlayerId actor=0){return {actor,CommandKind::pray,{},{}};}
Command defense(PlayerId actor){return {actor,CommandKind::defend,{},{}};}
void checkpoint(const Match& m){Match restored;restored.restore(m.snapshot());check(restored.snapshot()==m.snapshot(),"checkpoint preserves complete Match50 snapshot");}
MatchStep manual(Match& m,std::uint64_t& policy,std::uint64_t& chance){
    if(m.snapshot().phase==MatchPhase::grant)return m.grant_step(m.sample_grant(chance));
    const auto* actor=find_player(m.snapshot().state,*m.snapshot().actor);
    if(m.snapshot().phase==MatchPhase::main&&actor->confusion_count>0)return m.confusion_step(m.sample_confusion(chance));
    const auto choice=m.sample_choice(policy);
    if(const auto* command=std::get_if<Command>(&choice))return m.step(*command);
    const auto request=std::get<FateRequest>(choice);return m.fate_step(m.sample_fate(request.token,chance));
}
void compare_step(Match& a,Match& b,std::uint64_t& ap,std::uint64_t& ac,std::uint64_t& bp,std::uint64_t& bc){
    check(a.rollout_step(ap,ac)==manual(b,bp,bc)&&a.snapshot()==b.snapshot()&&ap==bp&&ac==bc,"driver equals independently orchestrated old primitive calls and both streams");checkpoint(a);
}
void choice_legality_and_no_future_probe(){
    auto a=match(scene({207},{6}));const auto before=a.snapshot();
    check(a.choice_legal(FateRequest{0,0})&&!a.choice_legal(FateRequest{7,0})&&!a.choice_legal(FateRequest{0,1}),"token0 actor0 request owns the actual207 and is source-qualified");
    check(!a.choice_legal(Command{0,CommandKind::play,{0},7})&&a.choice_legal(Command{0,CommandKind::discard,{0},{}}),"new intention does not alter old207 Command play/discard roles");
    check(a.snapshot()==before&&a.observe(0).find(choice_rule)==std::string::npos&&a.observe(0).find(driver_rule)==std::string::npos,"choice preflight and separate rule getters add no old observation fields");
    auto confused=scene({207},{});confused.players[0].confusion_count=1;auto c=match(confused);check(!c.choice_legal(FateRequest{0,0}),"a player does not choose207 during main confusion");
    const auto old=c.snapshot();std::uint64_t rng=42;bool failed=false;try{(void)c.sample_choice(rng);}catch(const MatchError&){failed=true;}
    check(failed&&rng==42&&c.snapshot()==old,"confused choice sampling refuses without drawing");
    auto overflow=scene({207},{});overflow.players[1].confusion_count=static_cast<std::int64_t>(max_wire_integer);auto legal=match(overflow);
    check(legal.choice_legal(FateRequest{0,0}),"legality of an intention does not peek at a future289 overflow");
}
void unchanged_no207_policy_and_visible_choice_noninterference(){
    for(unsigned players:{2u,9u})for(std::uint64_t seed=0;seed<64;++seed){SnapshotState state;for(unsigned i=0;i<players;++i)state.players.push_back(person(i?100+i:0,{6,118,190,235}));auto game=match(state);const auto before=game.snapshot();auto p=seed,q=seed;
        const auto choice=game.sample_choice(p);check(std::holds_alternative<Command>(choice)&&std::get<Command>(choice)==game.sample_action(q)&&p==q&&game.snapshot()==before,"no207 delegates the exact old policy and entropy law");}
    auto first=scene({207,6,235},{6});auto second=first;second.players[1].items={};second.players[1].hp=1;second.players[1].guardian=Guardian::moon;second.players[1].curses={Curse::heaven};
    auto a=match(first),b=match(second);for(std::uint64_t seed=0;seed<128;++seed){auto ap=seed,bp=seed;check(a.sample_choice(ap)==b.sample_choice(bp)&&ap==bp,"player choice does not inspect another player's hidden hand/resource/guardian or game RNG");}
}
void independent_bigint_envelope_vectors_and_copy_order(){
    constexpr std::uint64_t gamma=0x9e3779b97f4a7c15ULL;
    for(unsigned n:{1u,2u,18u}){auto state=scene();for(unsigned i=0;i<n;++i)state.players[0].items.push_back({i?100+i:0,207,{},false});auto game=match(state);const auto before=game.snapshot();
        const std::array<std::uint64_t,4> seeds{0,42,UINT64_MAX,0x61c8864680b583ebULL};
        const std::array<unsigned,4> tickets=n==1?std::array<unsigned,4>{1,1,0,0}:n==2?std::array<unsigned,4>{1,1,2,1}:std::array<unsigned,4>{16,9,8,16};
        for(unsigned j=0;j<seeds.size();++j){auto policy=seeds[j];const auto choice=game.sample_choice(policy);
            auto expected=seeds[j]+gamma;if(j==3&&n!=1)expected+=gamma;
            if(tickets[j])check(choice==PlayerChoice{FateRequest{0,state.players[0].items[tickets[j]-1].token}}&&policy==expected,"independent BigInt envelope ticket and rejection count select inventory copy");
            else {auto delegated=expected;check(choice==PlayerChoice{game.sample_action(delegated)}&&policy==delegated,"ordinary envelope delegates from the exact post-envelope stream");}
            check(game.snapshot()==before,"envelope sampling is const and chance-free");}
    }
}
void independent_request_to_fate_stage_vectors(){
    SnapshotState state;state.players={person(7),person(0),person(42)};state.players[0].items={{19,235,{},true},{0,207,{},false},{5,6,8,false},{11,207,{},false}};state.players[0].curses={Curse::dream};
    state.players[1].items={{0,6,{},false},{4,208,{},false}};state.players[2].dead=true;state.players[2].hp=0;state.players[2].items={{0,207,{},false}};
    Match game;game.load_scene(state,7,42);auto p=UINT64_MAX;check(game.sample_choice(p)==PlayerChoice{FateRequest{7,11}}&&p==0x9e3779b97f4a7c14ULL,"independent N2 actor7/raw-token order request consumes policy only");
    struct Vector{std::uint64_t seed,next;CardId outcome;};
    for(const auto& v:{Vector{0x57b4c1fc7ebefd60ULL,0xf5ec3bb5fe097975ULL,287},Vector{0x445dad76252c0cfdULL,0xe295272fa4768912ULL,289},Vector{0xcd46863b18fc3046ULL,0x09b579ae17912870ULL,291},Vector{0x29a03e5c9696abe0ULL,0x0446ab891476201fULL,296},Vector{0xa0cae6e9a463f84dULL,0xb7e0478920d864b6ULL,295}}){
        auto chance=v.seed;const auto before=game.snapshot();const auto fate=game.sample_fate(11,chance);check(fate.outcome.model==v.outcome&&chance==v.next&&game.snapshot()==before,"independent separate Fate stage consumes only chance after a fixed player request");
        if(v.outcome==291)check(fate.outcome.recipient==7,"targeted result is chance, not part of the request");
        if(v.outcome==296)check(fate.outcome.guardians==std::vector<Guardian>{Guardian::mercury,Guardian::moon},"guardian assignments are chance-owned in alive input order");
        if(v.outcome==295)check(fate.shuffle&&fate.shuffle->order==std::vector<ItemSource>{{7,19},{0,0},{7,0},{7,5},{0,4}},"shuffle excludes only selected7/11, retains other207 and excludes dead copies");
    }
}
void all_fate_results_and_manual_dispatch_equivalence(){
    auto state=scene({207,190,178},{208,187,112});MatchConfig cfg;cfg.external_grants=true;cfg.include_fate=true;cfg.local_guardian_payloads=true;cfg.max_decisions=128;state.tiebreak_gf=0;
    std::set<CardId> covered;
    for(std::uint64_t seed=0;seed<256&&covered.size()<10;++seed){auto probe=match(state,cfg);auto candidate=seed;const auto fate=probe.sample_fate(0,candidate);if(!covered.insert(fate.outcome.model).second)continue;
        auto a=match(state,cfg),b=match(state,cfg);std::uint64_t ap=0,bp=0,ac=seed,bc=seed;compare_step(a,b,ap,ac,bp,bc);
        check(a.snapshot().fate_actions==1&&a.snapshot().decisions==1&&ap==0x9e3779b97f4a7c15ULL,"driver launch uses one ordinary decision and a request-only policy draw");
        while(!a.done()&&a.snapshot().phase!=MatchPhase::main)compare_step(a,b,ap,ac,bp,bc);
    }
    check(covered.size()==10,"driver routes all ten existing Fate outcomes and pending continuations");
}
void chance_vs_policy_routes_and_choice_step(){
    auto ordinary=match(scene());auto old=ordinary;std::uint64_t policy=0,chance=77,op=0,oc=77;compare_step(ordinary,old,policy,chance,op,oc);check(chance==77,"ordinary driver decision does not draw external chance");
    auto confused=scene({207,190},{});confused.players[0].confusion_count=3;auto c=match(confused),d=c;policy=19;op=19;chance=7;oc=7;compare_step(c,d,policy,chance,op,oc);
    check(policy==19&&c.snapshot().fate_actions==0&&c.snapshot().state.players[0].confusion_count==2,"confused routing uses chance and preserves old no207 policy/debit");
    auto grant=scene();grant.tiebreak_gf=0;MatchConfig cfg;cfg.external_grants=true;auto g=match(grant,cfg);g.step(prayer());auto h=g;policy=31;op=31;chance=1;oc=1;compare_step(g,h,policy,chance,op,oc);check(policy==31&&g.snapshot().grant_resolutions==1,"grant routing never asks the player for an environment outcome");
    auto trade=match(scene({5},{6}));trade.step({0,CommandKind::buy,{0},7});auto response=trade;policy=12;op=12;chance=55;oc=55;compare_step(trade,response,policy,chance,op,oc);
    check(trade.snapshot().phase==MatchPhase::purchase,"empty seller response reaches the purchase continuation");
    auto purchase=trade;compare_step(trade,purchase,policy,chance,op,oc);check(chance==55,"response and purchase decisions use policy only");
    auto explicit_game=match(scene({207},{})),manual_game=explicit_game;chance=42;oc=42;const PlayerChoice request=FateRequest{0,0};
    check(explicit_game.choice_step(request,chance)==manual_game.fate_step(manual_game.sample_fate(0,oc))&&explicit_game.snapshot()==manual_game.snapshot()&&chance==oc,"explicit typed choice resolves through the same old chance/body API");
}
void atomic_world_two_streams_and_alias_failures(){
    for(bool fate:{false,true}){auto state=scene(fate?std::initializer_list<CardId>{207}:std::initializer_list<CardId>{});state.update_count=max_wire_integer;auto game=match(state);const auto before=game.snapshot();std::uint64_t policy=0,chance=42;bool failed=false;
        try{game.rollout_step(policy,chance);}catch(const MatchError&){failed=true;}check(failed&&game.snapshot()==before&&policy==0&&chance==42,"late update failure rolls back world and both trial streams");}
    auto overflow=scene({207},{});overflow.players[1].confusion_count=static_cast<std::int64_t>(max_wire_integer);auto game=match(overflow);const auto before=game.snapshot();std::uint64_t policy=0,chance=0x445dad76252c0cfdULL;bool failed=false;
    try{game.rollout_step(policy,chance);}catch(const MatchError&){failed=true;}check(failed&&game.snapshot()==before&&policy==0&&chance==0x445dad76252c0cfdULL,"future289 failure also rolls back the earlier player envelope draw without reroll");
    failed=false;std::uint64_t one=42;try{game.rollout_step(one,one);}catch(const MatchError&){failed=true;}check(failed&&one==42&&game.snapshot()==before,"aliased policy/chance references are rejected before work");
    auto full=scene({}, {6,208});for(unsigned i=0;i<18;++i)full.players[0].items.push_back({i==17?max_wire_integer:i,207,{},false});
    auto exhausted=match(full);const auto full_before=exhausted.snapshot();std::uint64_t full_policy=0x970ae342d6e8c180ULL,full_chance=0xa0cae6e9a463f84dULL;
    auto sampled=full_policy;check(exhausted.sample_choice(sampled)==PlayerChoice{FateRequest{0,max_wire_integer}}&&sampled==0x35425cfc56333d95ULL&&exhausted.choice_legal(FateRequest{0,max_wire_integer}),"full18 maxsafe request is legal without a hidden future token-budget probe");
    failed=false;try{exhausted.rollout_step(full_policy,full_chance);}catch(const MatchError&){failed=true;}
    check(failed&&exhausted.snapshot()==full_before&&full_policy==0x970ae342d6e8c180ULL&&full_chance==0xa0cae6e9a463f84dULL,"later295 fresh-token exhaustion rolls back world, envelope and full chance permutation without reroll");
    auto control=match(scene());std::uint64_t a=42,b=42;control.rollout_step(a,b);check(b==42,"equal initial values are allowed in distinct stream storage");
    MatchConfig cap;cap.max_decisions=1;auto ended=match(scene(),cap);a=7;b=9;ended.rollout_step(a,b);const auto saved=ended.snapshot();const auto olda=a,oldb=b;failed=false;
    try{ended.rollout_step(a,b);}catch(const MatchError&){failed=true;}check(failed&&ended.snapshot()==saved&&a==olda&&b==oldb,"finished driver rejects with both streams and snapshot unchanged");
}
void snapshot_plus_streams_resumes_across_mixed_routes(){
    auto state=scene({207,190,178,187},{208,235});state.tiebreak_gf=0;state.players[1].guardian=Guardian::moon;MatchConfig cfg;cfg.max_decisions=48;cfg.external_grants=true;cfg.include_fate=true;cfg.local_guardian_payloads=true;
    auto a=match(state,cfg);std::uint64_t policy=0,chance=0x445dad76252c0cfdULL;
    while(!a.done()){const auto saved=a.snapshot();auto resumed_policy=policy,resumed_chance=chance;Match resumed;resumed.restore(saved);
        check(a.rollout_step(policy,chance)==resumed.rollout_step(resumed_policy,resumed_chance)&&a.snapshot()==resumed.snapshot()&&policy==resumed_policy&&chance==resumed_chance,"Match snapshot plus two streams reproduces each mixed driver boundary");}
    check(a.snapshot().config==cfg&&a.snapshot().version==match_snapshot_version,"additive driver preserves existing snapshot/config schema");
}
}
int main(){
    choice_legality_and_no_future_probe();unchanged_no207_policy_and_visible_choice_noninterference();independent_bigint_envelope_vectors_and_copy_order();independent_request_to_fate_stage_vectors();all_fate_results_and_manual_dispatch_equivalence();chance_vs_policy_routes_and_choice_step();atomic_world_two_streams_and_alias_failures();snapshot_plus_streams_resumes_across_mixed_routes();
    std::cout<<"Local driver passed (8 groups; choice/chance separation, manual equivalence, two-stream atomicity/resume).\n";
}
