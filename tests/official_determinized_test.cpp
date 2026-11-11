#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <chrono>
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
bool launch_draws(const MatchSnapshot& s) {
    if(!s.batch)return false;
    const auto& b=*s.batch;const auto& base=*find_card(b.models.front());
    const auto cursed=[&](PlayerId id,Curse c) {
        const auto& curses=find_player(b.initial_state,id)->curses;
        return std::find(curses.begin(),curses.end(),c)!=curses.end();
    };
    return base.ability==Ability::boost_hp_or_deal_damage
        || (base.hit_rate && !cursed(*s.actor,Curse::darkcloud))
        || (cursed(b.owner,Curse::fog) && !base.hit_rate
            && std::none_of(b.models.begin(),b.models.end(),[](auto id){return find_card(id)->ability==Ability::attack_every_enemy;}));
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
    if (!launch_draws(before)) check(world.snapshot().rng==seed,"exact deterministic RNG seed convention");
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
    check(response.determinize_support(7)==Support::ok,"ordinary response supported");
    rejected(response,42,Support::not_actor);
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
CardId image(std::string_view name) {
    for (const auto& c:catalog()) if(name==c.image_name) return c.id;
    check(false,"fixture image exists");return 0;
}
std::vector<CardId> unused(const Match& m, PlayerId viewer) {
    std::vector<CardId> models;
    for(const auto& p:m.snapshot().state.players) if(p.id!=viewer)
        for(const auto& i:p.items) if(!i.used) models.push_back(i.model);
    return models;
}
// Canonicalize only the permitted representation differences. Journal-selected
// tokens are public and must remain identical, including vanished weapons.
MatchSnapshot normalized(MatchSnapshot s, PlayerId viewer) {
    std::set<ItemToken> replaced;
    for(const auto& p:s.state.players) if(p.id!=viewer)
        for(const auto& i:p.items) if(!i.used) replaced.insert(i.token);
    const auto normalize_state=[&](SnapshotState& state) {
        for(auto& p:state.players) if(p.id!=viewer) {
            std::vector<ItemInstance> tail;
            std::erase_if(p.items,[&](auto i) {
                if(!replaced.contains(i.token)) return false;
                i.token=0;tail.push_back(i);return true;
            });
            std::sort(tail.begin(),tail.end(),[](auto a,auto b){return a.model<b.model;});
            p.items.insert(p.items.end(),tail.begin(),tail.end());
        }
    };
    normalize_state(s.state);s.rng=0;s.next_token=0;
    if(s.batch) { normalize_state(s.batch->initial_state);s.batch->rng=0;s.batch->next_token=0; }
    for(auto& f:s.queue) f.redirect_rng=0;
    return s;
}
void truth(const Match& m) {
    const auto viewer=*m.snapshot().actor;
    auto w=m.determinized(viewer,unused(m,viewer),9876);
    check(normalized(m.snapshot(),viewer)==normalized(w.snapshot(),viewer),"true assignment reconstructs entire response snapshot");
}
void response_verify(const Match& m, const std::vector<CardId>& models, std::uint64_t seed) {
    verify(m,models,seed);
    const auto viewer=*m.snapshot().actor;
    const auto w=m.determinized(viewer,models,seed);
    check(w.snapshot().queue.front().redirect_rng==w.snapshot().rng,"redirect RNG follows rebuilt launch");
    if(!launch_draws(m.snapshot())) check(w.snapshot().batch->rng==seed,"deterministic anchor uses exact seed");
    // Exhaust all response prefixes in these small fixtures, with both modes.
    if(m.snapshot().state.players[viewer==42?0:1].items.size()<=4) {
        for(bool canonical:{false,true}) {
            const auto visit=[&](auto&& self,std::vector<ItemToken> prefix)->void {
                auto a=m.legal_extensions(CommandKind::defend,prefix,{canonical});
                check(a==w.legal_extensions(CommandKind::defend,prefix,{canonical}),"complete response prefix tree invariant");
                for(auto t:a.next) { auto next=prefix;next.push_back(t);self(self,std::move(next)); }
            };
            visit(visit,{});
        }
    }
}
void normalized_tokens() {
    auto m=fixture(scene({6,7,235},{8,8}));
    for(bool response:{false,true}) {
        if(response)m.step({42,CommandKind::play,{0},7});
        const auto viewer=*m.snapshot().actor;
        const std::vector<CardId> models{6,7};
        auto altered=m.determinized(viewer,std::vector<CardId>{8,8,8,8},123);
        auto a=m.determinized(viewer,models,42,true);
        auto b=altered.determinized(viewer,models,42,true);
        check(a.snapshot()==b.snapshot(),"normalization drops source hidden tokens and allocator");
        check(a.view(viewer)==m.view(viewer),"normalized world preserves viewer");
        auto restored=a;restored.restore(a.snapshot());
        check(restored.snapshot()==a.snapshot(),"normalized world restores exactly");
        if(response) {
            check(a.snapshot().rng==42 && a.snapshot().batch->rng==42,"normalized anchor RNG reset");
            check(a.snapshot().next_token==a.snapshot().batch->next_token,"normalized anchor allocator matches current");
        }
    }
}
void response_fixtures() {
    auto m=fixture(scene({6,7,235},{8}));m.step({42,CommandKind::play,{0},7});
    truth(m);response_verify(m,{},0);response_verify(m,{6,209,235},42);
    auto source=m.snapshot();auto w=m.determinized(7,std::vector<CardId>{6,209,235},42);
    check(w.snapshot().rng!=source.rng && w.snapshot().batch->rng!=source.batch->rng,"real current/anchor RNG removed");
    bool bad=false;try{(void)m.determinized(7,std::vector<CardId>(18,6),0);}catch(const MatchError& e){bad=e.code==MatchErrorCode::invalid;}
    check(bad,"launch capacity includes consumed weapons");
    for(auto models:{std::vector<CardId>{0},std::vector<CardId>{207},std::vector<CardId>{65535}}) {
        bad=false;try{(void)m.determinized(7,models,0);}catch(const MatchError& e){bad=e.code==MatchErrorCode::invalid;}
        check(bad && m.snapshot()==source,"invalid response assignment is pure");
    }
    const auto sword=image("reflection-sword"),curtain=image("rainbow-curtain");
    auto chain=fixture(scene({6,sword,235},{sword,8}));
    chain.step({42,CommandKind::play,{0},7});chain.step({7,CommandKind::defend,{3},{}});
    rejected(chain,42,Support::not_actor);
    chain.step({42,CommandKind::defend,{1},{}});
    check(chain.snapshot().batch->responses.size()==2,"multi-response journal fixture");
    truth(chain);response_verify(chain,{6,209},87);finish(chain,123);
    auto filtered=fixture(scene({6,curtain,sword},{sword,8}));
    filtered.step({42,CommandKind::play,{0},7});filtered.step({7,CommandKind::defend,{3},{}});
    filtered.step({42,CommandKind::defend,{1,2},{}});truth(filtered);response_verify(filtered,{7},88);
    auto spell=fixture(scene({image("spring"),6},{8}));spell.step({42,CommandKind::play,{0},7});truth(spell);response_verify(spell,{7},9);
    auto special=fixture(scene({5},{6}));special.step({42,CommandKind::buy,{0},7});rejected(special,7,Support::response_special);
    auto gs=scene();gs.players[0].guardian=find_card(255)->guardian;auto guardian=fixture(gs);guardian.guardian_step({42,255,7});rejected(guardian,7,Support::response_special);
    auto fog=scene({6});fog.players[1].curses={Curse::fog};auto f=fixture(fog);f.step({42,CommandKind::play,{0},7});rejected(f,7,Support::hidden_status);
    auto confused=scene({6});confused.players[1].confusion_count=1;f=fixture(confused);f.step({42,CommandKind::play,{0},7});rejected(f,7,Support::hidden_confusion);
    auto dream=scene({6},{19});dream.players[1].curses={Curse::dream};dream.players[1].items[0].fake_model=27;
    f=fixture(dream);f.step({42,CommandKind::play,{0},7});rejected(f,7,Support::hidden_own_model);
    auto launcher_fog=scene({6});launcher_fog.players[0].curses={Curse::fog};f=fixture(launcher_fog);f.step({42,CommandKind::play,{0},7});truth(f);response_verify(f,{7},81);
    for(const auto& c:catalog()) if(match_supported(c.id) && c.hit_rate && c.gift_rate) {
        f=fixture(scene({c.id}));
        for(std::uint64_t seed=0;seed<100;++seed) {
            f.load_scene(scene({c.id}),42,seed);f.step({42,CommandKind::play,{0},{}});
            if(f.snapshot().phase==MatchPhase::response) {truth(f);response_verify(f,{7},81);break;}
        }
        break;
    }
    const auto bounce=image("bouncing-sword");
    // A public bounce draw is rejected even when it happens to return to viewer.
    bool bounced=false;
    for(std::uint64_t seed=0;seed<100 && !bounced;++seed) {
        auto bs=scene({6},{bounce});f.load_scene(bs,42,seed);f.step({42,CommandKind::play,{0},7});
        if(f.legal({7,CommandKind::defend,{1},{}})) {
            f.step({7,CommandKind::defend,{1},{}});
            if(f.snapshot().actor==7) {rejected(f,7,Support::response_random);bounced=true;}
        }
    }
    check(bounced,"public bounce RNG rejection exercised");
    auto twice=fixture(scene({image("saw-boom-boom")},{8}));
    twice.step({42,CommandKind::play,{0},7});twice.step({7,CommandKind::defend,{},{}});
    rejected(twice,7,Support::response_history);
    // Forced chance hits do not draw, unlike ordinary percentage attacks.
    bool forced=false;
    for(const auto& card:catalog()) if(match_supported(card.id) && card.hit_rate && card.category==Category::weapons) {
        auto fs=scene({card.id});fs.players[1].curses={Curse::darkcloud};f=fixture(fs);
        f.step({42,CommandKind::play,{0},{}});
        check(f.determinize_support(7)==Support::ok,"public forced hit needs no game RNG");
        truth(f);response_verify(f,{6},0);forced=true;break;
    }
    check(forced,"forced hit fixture exists");
    auto exhausted=scene({6});exhausted.players[0].items[0].token=max_wire_integer-1;
    f=fixture(exhausted);f.step({42,CommandKind::play,{max_wire_integer-1},7});
    response_verify(f,{6},42);
    bad=false;try{(void)f.determinized(7,std::vector<CardId>{6,7},0);}catch(const MatchError& e){bad=e.code==MatchErrorCode::invalid;}
    check(bad,"response token budget enforced");
    unsigned changed=0;
    for(std::uint64_t seed=0;seed<128;++seed) {
        auto a=m.determinized(7,{},seed),b=m.determinized(7,{},seed+1),same=a;
        for(auto* world:{&a,&b,&same}) world->step({7,CommandKind::defend,{},{}});
        check(a.snapshot()==same.snapshot(),"response seed reproduces future gifts");
        changed+=a.snapshot().state!=b.snapshot().state;
    }
    check(changed>110,"response future gifts resampled");
    std::cout<<"response_future_gifts_differ="<<changed<<"/128\n";
}

void chance_response_fixtures() {
    // Every percentage weapon, both HP gamble signs, and Fog + gamble.
    std::vector<CardId> launches{202};
    for (const auto& c:catalog()) if(match_supported(c.id) && c.hit_rate && c.gift_rate && can_start_main(c.id)) launches.push_back(c.id);
    for (auto model:launches) for (bool fog:{false,true}) {
        unsigned signs=0, tested=0;
        for (std::uint64_t source_seed=0;source_seed<100 && tested<8;++source_seed) {
            auto state=scene({model,19},{8});
            state.players[0].mp=99;
            if(fog) state.players[0].curses={Curse::fog};
            auto m=fixture(state);m.load_scene(state,42,source_seed);
            m.step({42,CommandKind::play,{0},find_card(model)->hit_rate?std::nullopt:std::optional<PlayerId>{7}});
            if(m.snapshot().phase!=MatchPhase::response) continue;
            check(m.determinize_support(7)==Support::ok,"chance response supported");
            const auto public_view=m.view(7);
            signs|=public_view.pending->amount>0?1:2;
            response_verify(m,{6,7},91);truth(m);
            auto w=m.determinized(7,std::vector<CardId>{6,7},91,true);
            // Same public facts, different true hand, original RNG and its
            // launch/redirect copies must generate exactly the same new world.
            auto other=m.determinized(7,std::vector<CardId>{235},source_seed+111,true);
            check(other.view(7)==public_view,"alternate source has identical public chance result");
            check(w.snapshot()==other.determinized(7,std::vector<CardId>{6,7},91,true).snapshot(),"chance world leaks no source RNG/hand/roll");
            unsigned different=0;
            for(std::uint64_t seed=0;seed<32;++seed) {
                auto fresh=m.determinized(7,std::vector<CardId>{6,7},seed,true);
                check(fresh.view(7)==public_view,"public hit/target/sign preserved for every world seed");
                different+=fresh.snapshot().rng!=w.snapshot().rng;
            }
            check(different>20,"chance future RNG varies independently");
            ++tested;
        }
        check(tested>0,"chance launch exercised");
        if(model==202) check(signs==3,"both public gamble signs exercised");
    }
    // Chance-conditioned anchors must also validate after a public reflection
    // chain; redirect RNG is rebuilt rather than copied from either source.
    for(auto model:launches) {
        auto state=scene({model,190,19},{190,8});state.players[0].mp=99;
        auto m=fixture(state);
        for(std::uint64_t seed=0;;++seed) {
            m.load_scene(state,42,seed);
            m.step({42,CommandKind::play,{0},find_card(model)->hit_rate?std::nullopt:std::optional<PlayerId>{7}});
            if(m.snapshot().phase==MatchPhase::response)break;
        }
        m.step({7,CommandKind::defend,{3},{}});
        m.step({42,CommandKind::defend,{1},{}});
        check(m.snapshot().batch->responses.size()==2,"chance reflection chain exercised");
        truth(m);response_verify(m,{6},81);
        auto other=m.determinized(7,std::vector<CardId>{235},711,true);
        check(m.determinized(7,std::vector<CardId>{6},81,true).snapshot()
            ==other.determinized(7,std::vector<CardId>{6},81,true).snapshot(),"reflected chance source RNG/hand leaked");
    }
    for(bool wide:{false,true}) {
        auto state=wide?scene({94,232}):scene({94});state.players[0].mp=99;
        auto m=fixture(state);
        m.step({42,CommandKind::play,wide?std::vector<ItemToken>{0,1}:std::vector<ItemToken>{0},{}});
        if(m.snapshot().phase==MatchPhase::response) rejected(m,7,Support::response_random);
    }
}

void response_randomized() {
    std::mt19937_64 random(991);unsigned accepted=0,invalid=0,continued=0,truths=0;
    for(unsigned seed=0;seed<200;++seed) {
        MatchConfig cfg;cfg.max_decisions=128;cfg.external_artifacts=seed%3==0;cfg.local_guardian_payloads=seed%2;cfg.include_fate=seed%5==0;
        Match m(cfg);std::array<PlayerSetup,2> roster{{{42,Team::solo,"a"},{7,Team::solo,"b"}}};m.reset(roster,seed);
        std::uint64_t p=seed+71,c=seed+93;bool played=false;
        while(!m.done()) {
            if(m.snapshot().phase==MatchPhase::response && m.determinize_support(*m.snapshot().actor)==Support::ok) {
                auto viewer=*m.snapshot().actor;auto before=m.snapshot();
                bool disguised=false;for(const auto& player:before.state.players) if(player.id!=viewer)
                    for(const auto& i:player.items) if(!i.used && i.fake_model) disguised=true;
                if(!disguised) {truth(m);++truths;}
                std::vector<CardId> models;auto catalog=m.gift_catalog();
                const auto& other=before.state.players[viewer==42?1:0];
                auto used=std::count_if(other.items.begin(),other.items.end(),[](auto i){return i.used;});
                for(std::size_t n=random()%(cfg.hand_limit-used+1);n;--n) models.push_back(catalog[random()%catalog.size()]);
                try {
                    auto world=m.determinized(viewer,models,random());++accepted;
                    response_verify(m,models,random());
                    if(!played) {finish(world,seed);played=true;++continued;}
                } catch(const MatchError& e) {check(e.code==MatchErrorCode::invalid,"only incompatible assignments rejected");++invalid;}
                check(m.snapshot()==before,"random response source unchanged");
            }
            advance(m,p,c);
        }
    }
    check(accepted>500 && truths>500 && continued>150,"many random response worlds and complete continuations");
    std::cout<<"response_random_accepted="<<accepted<<" invalid="<<invalid<<" true_reconstructions="<<truths<<" continued="<<continued<<'\n';
}
void measure(unsigned games) {
    constexpr std::array names{"ok","invalid_viewer","unsupported_players","not_main","not_actor","pending_work","reserved_gifts","replay_proof","hidden_status","hidden_confusion","hidden_own_model","response_special","response_random","response_history"};
    std::array<std::array<std::array<std::uint64_t,names.size()>,2>,2> counts{};
    std::array<std::array<std::uint64_t,5>,2> phases{};
    std::array<std::uint64_t,2> attempts{},invalid{},ns{},success_ns{},capacity_invalid{};
    std::array<std::uint64_t,3> newly_supported{};
    const auto cursed=[](const SnapshotState& state,PlayerId id,Curse c) {
        const auto& curses=find_player(state,id)->curses;
        return std::find(curses.begin(),curses.end(),c)!=curses.end();
    };
    std::mt19937_64 random(778);Match catalog_match;auto catalog=catalog_match.gift_catalog();
    for(unsigned seed=0;seed<games;++seed) {
        Match m;std::uint64_t p=seed+101,c=seed+203;
        std::array<PlayerSetup,2> roster{{{42,Team::solo,"a"},{7,Team::solo,"b"}}};m.reset(roster,seed);
        while(!m.done()) {
            const auto& s=m.snapshot();const auto viewer=s.actor.value_or(s.pending_grant.value_or(s.owner));
            const auto seat=viewer==42?0:1; ++phases[seat][static_cast<unsigned>(s.phase)];
            if(s.phase==MatchPhase::main || s.phase==MatchPhase::response) {
                const auto boundary=s.phase==MatchPhase::main?0:1;const auto support=m.determinize_support(viewer);
                ++counts[seat][boundary][static_cast<unsigned>(support)];
                if(boundary && support==Support::ok) {
                    const auto& b=*s.batch;const auto& base=*find_card(b.models.front());
                    if(base.ability==Ability::boost_hp_or_deal_damage) ++newly_supported[1];
                    else if(base.hit_rate && !cursed(b.initial_state,viewer,Curse::darkcloud)) ++newly_supported[0];
                    else if(!base.hit_rate && cursed(b.initial_state,b.owner,Curse::fog)
                            && std::none_of(b.models.begin(),b.models.end(),[](auto id){return find_card(id)->ability==Ability::attack_every_enemy;})) ++newly_supported[2];
                }
                if(support==Support::ok && attempts[boundary]<10000) {
                    const auto& other=s.state.players[1-seat];
                    auto used=std::count_if(other.items.begin(),other.items.end(),[](auto i){return i.used;});
                    std::vector<CardId> models;
                    for(std::size_t n=random()%(s.config.hand_limit-used+1);n;--n) models.push_back(catalog[random()%catalog.size()]);
                    auto sample_seed=random();++attempts[boundary];auto start=std::chrono::steady_clock::now();
                    bool success=true;
                    try {(void)m.determinized(viewer,models,sample_seed);}
                    catch(const MatchError& e) {
                        check(e.code==MatchErrorCode::invalid,"benchmark invalid only");++invalid[boundary];success=false;
                        capacity_invalid[boundary]+=std::string_view(e.what())=="determinized launch inventory exceeds hand limit";
                    }
                    const auto elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count();
                    ns[boundary]+=elapsed;if(success)success_ns[boundary]+=elapsed;
                }
            }
            advance(m,p,c);
        }
    }
    std::cout<<"games="<<games<<" default_config sample_action policy seeds=game+101 chance=game+203 assignment_rng=778\n";
    for(unsigned seat=0;seat<2;++seat) {
        std::cout<<"seat="<<seat<<" phases(main,response,finished,purchase,grant)=";
        for(auto n:phases[seat]) std::cout<<n<<',';std::cout<<'\n';
        for(unsigned boundary=0;boundary<2;++boundary) {
            std::uint64_t total=0;for(auto n:counts[seat][boundary]) total+=n;
            std::cout<<(boundary?"response":"main")<<" total="<<total<<'\n';
            for(unsigned i=0;i<names.size();++i) std::cout<<names[i]<<'='<<counts[seat][boundary][i]<<" ("<<std::fixed<<std::setprecision(4)<<100.*counts[seat][boundary][i]/total<<"%)\n";
        }
    }
    std::cout<<"newly_supported percentage="<<newly_supported[0]<<" gamble="<<newly_supported[1]<<" fog="<<newly_supported[2]<<'\n';
    for(unsigned b=0;b<2;++b) std::cout<<(b?"response":"main")<<" samples="<<attempts[b]<<" invalid="<<invalid[b]<<" capacity_invalid="<<capacity_invalid[b]<<" mean_us_success="<<1e-3*success_ns[b]/(attempts[b]-invalid[b])<<" mean_us_including_invalid="<<1e-3*ns[b]/attempts[b]<<'\n';
}
}
int main(int argc,char** argv) {
    if(argc==3 && std::string(argv[1])=="--measure") { measure(static_cast<unsigned>(std::stoul(argv[2])));return 0; }
    boundaries_and_assignment();normalized_tokens();future_rng();randomized();response_fixtures();chance_response_fixtures();response_randomized();
    std::cout<<"Determinized tests passed.\n";
}
