#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>

namespace {
using namespace godfield::official;
using Support = DeterminizeSupport;
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL determinized: " << message << '\n'; std::exit(1); }
}
SnapshotState scene(std::initializer_list<CardId> own = {}, std::initializer_list<CardId> other = {}) {
    SnapshotState s; PlayerState a, b; a.id=42; b.id=7; a.name="a"; b.name="b";
    ItemToken token=0;
    for (auto id:own) a.items.push_back({token++,id,{},false});
    for (auto id:other) b.items.push_back({token++,id,{},false});
    s.players={a,b}; return s;
}
Match fixture(SnapshotState s, MatchConfig config = {}) {
    Match m(config); m.load_scene(std::move(s),42,17); return m;
}
void rejected(const Match& m, PlayerId viewer, Support reason) {
    const auto before=m.snapshot();
    check(m.determinize_support(viewer)==reason,"precise support reason");
    bool failed=false;
    try { (void)m.determinized(viewer,{},9); }
    catch (const MatchError& e) { failed=e.code==MatchErrorCode::unsupported; }
    check(failed && m.snapshot()==before,"unsupported determinization is pure");
}
void invalid_assignment(const Match& m, std::vector<CardId> models) {
    const auto before=m.snapshot(); bool failed=false;
    try { (void)m.determinized(42,models,9); }
    catch (const MatchError& e) { failed=e.code==MatchErrorCode::invalid; }
    check(failed && m.snapshot()==before,"invalid assignment rolls back with invalid code");
}
bool danger_wide(const Match& m, const Command& c) {
    if(c.kind!=CommandKind::play || c.items.size()!=2) return false;
    const auto* p=find_player(m.snapshot().state,c.actor);
    std::vector<CardId> models;
    for(auto token:c.items) for(const auto& item:p->items) if(item.token==token) models.push_back(face_model(item));
    return models==std::vector<CardId>{94,232};
}
std::uint64_t known_exceptions=0, comparisons=0;
void compare_legal(const Match& a, const Match& b) {
    check(a.legal_kinds()==b.legal_kinds(),"legal kinds invariant");
    auto ac=a.legal_commands(2,33), bc=b.legal_commands(2,33);
    for(const auto& c:ac) if(std::find(bc.begin(),bc.end(),c)==bc.end()) {
        check(danger_wide(a,c),"only known 94+232 legality may disappear"); ++known_exceptions;
    }
    for(const auto& c:bc) if(std::find(ac.begin(),ac.end(),c)==ac.end()) {
        check(danger_wide(b,c),"only known 94+232 legality may appear"); ++known_exceptions;
    }
    for(auto kind:a.legal_kinds()) {
        check(a.legal_extensions(kind,{})==b.legal_extensions(kind,{}),"empty prefix invariant");
        for(auto token:a.legal_extensions(kind,{}).next) {
            const std::array prefix{token};
            const auto an=a.legal_extensions(kind,prefix), bn=b.legal_extensions(kind,prefix);
            if(an!=bn) {
                auto x=an,y=bn;
                auto erase_wide=[&](auto& node) { std::erase_if(node.next,[&](auto t) {
                    return danger_wide(a,Command{*a.snapshot().actor,CommandKind::play,{token,t},{}});
                }); };
                erase_wide(x);erase_wide(y);
                check(x==y,"only known Wide extension may differ");
            }
        }
    }
}
void verify(const Match& m, std::vector<CardId> models, std::uint64_t seed, bool legality=true) {
    const auto before=m.snapshot(); const auto viewer=*before.actor;
    auto world=m.determinized(viewer,models,seed);
    check(world.view(viewer)==m.view(viewer),"exact typed viewer observation invariant");
    check(world.last_decision(viewer)==m.last_decision(viewer),"last decision invariant");
    if(legality) compare_legal(m,world);
    auto reversed=models;std::reverse(reversed.begin(),reversed.end());
    check(world.snapshot()==m.determinized(viewer,reversed,seed).snapshot(),"same seed and multiset give identical snapshots");
    std::vector<ItemInstance> old_used,new_used;std::vector<CardId> actual;
    const auto& old_other=before.state.players[before.state.players.front().id==viewer?1:0];
    const auto& other=world.snapshot().state.players[before.state.players.front().id==viewer?1:0];
    for(const auto& item:old_other.items) if(item.used) old_used.push_back(item);
    std::set<ItemToken> tokens;
    for(const auto& p:world.snapshot().state.players) for(const auto& item:p.items) {
        check(tokens.insert(item.token).second,"all physical tokens unique in reset/fixture worlds");
        if(p.id!=viewer) {
            if(item.used) new_used.push_back(item);
            else { actual.push_back(item.model); check(!item.fake_model && item.token>=before.next_token,"fresh unmasked unused instance"); }
        }
    }
    std::sort(models.begin(),models.end());
    check(actual==models && new_used==old_used,"assignment multiset and used items preserved");
    check(world.snapshot().rng==seed,"exact game RNG seed convention");
    Match restored; restored.restore(world.snapshot());
    check(restored.snapshot()==world.snapshot(),"full restore validation roundtrip");
    check(m.snapshot()==before,"source snapshot unchanged"); ++comparisons;
}
// sample_action is the ordinary policy; the other routes consume the caller's
// chance stream, just as rollout_step does (which also supports Fate requests).
void advance(Match& m,std::uint64_t& policy,std::uint64_t& chance) {
    if(m.snapshot().phase==MatchPhase::grant) m.grant_step(m.sample_grant(chance));
    else if(m.snapshot().phase==MatchPhase::main && find_player(m.snapshot().state,m.snapshot().owner)->confusion_count>0)
        m.confusion_step(m.sample_confusion(chance));
    else m.step(m.sample_action(policy));
}
void finish(Match& m,std::uint64_t seed) {
    std::uint64_t p=seed,c=seed+133;
    while(!m.done()) { m.rollout_step(p,c); }
    Match restored;restored.restore(m.snapshot());
    check(restored.snapshot()==m.snapshot(),"playable world ends in a valid snapshot");
}
void boundaries_and_assignment() {
    auto m=fixture(scene({6},{235,7,235,8}));
    auto s=m.snapshot();s.state.players[1].items[0].used=true;s.state.players[1].items[2].used=true;m.restore(s);
    verify(m,{},0);verify(m,{8,6,8,209},99);
    invalid_assignment(m,std::vector<CardId>(17,6));
    for(auto id:{CardId{0},CardId{65535},CardId{287},CardId{207}}) invalid_assignment(m,{id});
    MatchConfig cfg;cfg.include_fate=true;auto fate=fixture(scene(),cfg);verify(fate,{207},0);
    cfg.hand_limit=3;cfg.initial_hand=0;auto small=fixture(scene({}, {235}),cfg);
    s=small.snapshot();s.state.players[1].items[0].used=true;small.restore(s);
    verify(small,{6,7},0);invalid_assignment(small,{6,7,8});
    auto exhausted=scene({6});exhausted.players[0].items[0].token=max_wire_integer;
    auto end=fixture(exhausted);verify(end,{},42);invalid_assignment(end,{6});
    exhausted.players[0].items[0].token=max_wire_integer-1;end=fixture(exhausted);verify(end,{6},42);invalid_assignment(end,{6,7});
    rejected(m,999,Support::invalid_viewer);rejected(m,7,Support::not_actor);
    auto many=scene();PlayerState third;third.id=99;many.players.push_back(third);
    rejected(fixture(many),42,Support::unsupported_players);
    auto team=scene();team.players[0].team=Team::one;rejected(fixture(team),42,Support::unsupported_players);
    auto response=fixture(scene({6}));response.step({42,CommandKind::play,{0},7});
    rejected(response,7,Support::not_main);
    auto fog=scene();fog.players[0].curses={Curse::fog};rejected(fixture(fog),42,Support::hidden_status);
    fog=scene();fog.players[1].curses={Curse::fog};verify(fixture(fog),{6},7);
    for(unsigned seat:{0u,1u}) { auto confused=scene();confused.players[seat].confusion_count=1;rejected(fixture(confused),42,Support::hidden_confusion); }
    auto dream=scene({19},{19});dream.players[0].curses={Curse::dream};dream.players[0].items[0].fake_model=27;
    rejected(fixture(dream),42,Support::hidden_own_model);
    dream.players[0].items[0].fake_model=19;verify(fixture(dream),{},8);
    dream.players[0].items[0].fake_model.reset();dream.players[1].curses={Curse::dream};dream.players[1].items[0].fake_model=27;
    verify(fixture(dream),{7,7},8);
    // Whole-action proofs persist even after the effect and gift queues drained.
    auto summon=fixture(scene({201}));summon.summon_result_step({{42,CommandKind::play,{0},42},Guardian::earth});
    rejected(summon,*summon.snapshot().actor,Support::replay_proof);
    auto shuffle=fixture(scene());shuffle.item_shuffle_step({});rejected(shuffle,42,Support::replay_proof);
    auto f=fixture(scene({207}),MatchConfig{.include_fate=true});f.fate_step({42,0,{290,{},{}},{}});
    rejected(f,*f.snapshot().actor,Support::replay_proof);
    auto grant=fixture(scene(),MatchConfig{.external_artifacts=true});grant.step({42,CommandKind::pray,{},{}});
    rejected(grant,42,Support::not_main);grant.artifact_grant_step({42,6,{}});
    rejected(grant,*grant.snapshot().actor,Support::replay_proof);
    auto purchase=fixture(scene({5},{6}));purchase.step({42,CommandKind::buy,{0},7});purchase.step({7,CommandKind::defend,{},{}});
    rejected(purchase,42,Support::not_main);
    MatchConfig cutoff;cutoff.max_decisions=1;auto done=fixture(scene(),cutoff);done.step({42,CommandKind::pray,{},{}});rejected(done,42,Support::not_main);
    // Explicitly exercise and report the documented hidden-mortar mask leak.
    auto wide=fixture(scene({94,232}));verify(wide,{209},42);
    check(known_exceptions>0,"known 94+232 exception exercised");
}
void future_rng() {
    auto source=fixture(scene());unsigned changed=0;
    for(std::uint64_t seed=0;seed<128;++seed) {
        auto a=source.determinized(42,{},seed),b=source.determinized(42,{},seed+1),c=a;
        // Identical commands force future gifts to both players.
        for(auto actor:{PlayerId{42},PlayerId{7}}) {
            Command pray{actor,CommandKind::pray,{},{}};a.step(pray);b.step(pray);c.step(pray);
        }
        check(a.snapshot()==c.snapshot(),"same seed and action sequence reproduce gifts and entire world");
        if(a.snapshot().state!=b.snapshot().state) ++changed;
    }
    check(changed>110,"different seeds usually change future distribution");
    std::cout<<"future_gifts_differ="<<changed<<"/128\n";
}
void randomized() {
    std::mt19937_64 random(555);unsigned continued=0;
    for(std::uint64_t seed=0;seed<120;++seed) {
        MatchConfig cfg;cfg.max_decisions=128;cfg.external_artifacts=seed%3==0;cfg.local_guardian_payloads=seed%2;cfg.include_fate=seed%5==0;
        Match m(cfg);std::array<PlayerSetup,2> roster{{{42,Team::solo,"a"},{7,Team::solo,"b"}}};m.reset(roster,seed);
        std::uint64_t policy=seed+71,chance=seed+93;bool played=false;
        while(!m.done()) {
            if(m.snapshot().phase==MatchPhase::main && m.determinize_support(*m.snapshot().actor)==Support::ok) {
                auto viewer=*m.snapshot().actor;
                const auto& other=m.snapshot().state.players[viewer==42?1:0];
                auto used=std::count_if(other.items.begin(),other.items.end(),[](const auto& i){return i.used;});
                std::vector<CardId> models;auto catalog=m.gift_catalog();
                for(std::size_t n=random()%(cfg.hand_limit-used+1);n;--n) models.push_back(catalog[random()%catalog.size()]);
                verify(m,models,random());
                if(!played) { auto world=m.determinized(viewer,models,random());finish(world,seed);played=true;++continued; }
            }
            advance(m,policy,chance);
        }
    }
    check(comparisons>1500 && continued==120,"many random supported boundaries and complete world continuations");
    std::cout<<"random_worlds_continued="<<continued<<" comparisons="<<comparisons<<" known_94_232_command_differences="<<known_exceptions<<'\n';
}
void measure(unsigned games) {
    constexpr std::array names{"ok","invalid_viewer","unsupported_players","not_main","not_actor","pending_work","reserved_gifts","replay_proof","hidden_status","hidden_confusion","hidden_own_model"};
    std::array<std::array<std::uint64_t,names.size()>,2> counts{};
    for(unsigned seed=0;seed<games;++seed) {
        Match m;std::uint64_t p=seed+101,c=seed+203;
        std::array<PlayerSetup,2> roster{{{42,Team::solo,"a"},{7,Team::solo,"b"}}};m.reset(roster,seed);
        while(!m.done()) {
            if(m.snapshot().phase==MatchPhase::main) {
                const auto viewer=*m.snapshot().actor;
                ++counts[viewer==42?0:1][static_cast<unsigned>(m.determinize_support(viewer))];
            }
            advance(m,p,c);
        }
    }
    std::cout<<"games="<<games<<" default_config sample_action policy seeds=game+101 chance=game+203\n";
    for(unsigned seat=0;seat<2;++seat) {
        std::uint64_t total=0;for(auto n:counts[seat]) total+=n;
        std::cout<<"seat="<<seat<<" main_starts="<<total<<'\n';
        for(unsigned i=0;i<names.size();++i) std::cout<<names[i]<<'='<<counts[seat][i]<<" ("<<std::fixed<<std::setprecision(4)<<100.*counts[seat][i]/total<<"%)\n";
    }
}
}
int main(int argc,char** argv) {
    if(argc==3 && std::string(argv[1])=="--measure") { measure(static_cast<unsigned>(std::stoul(argv[2])));return 0; }
    boundaries_and_assignment();future_rng();randomized();
    std::cout<<"Determinized tests passed.\n";
}
