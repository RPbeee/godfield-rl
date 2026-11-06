#include "godfield/official_match.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <set>

namespace {
using namespace godfield::official;
void check(bool ok,const char* message) { if(!ok) { std::cerr<<"FAIL last decision: "<<message<<'\n'; std::exit(1); } }
std::set<DecisionSource> sources;
std::size_t scenes=0, decisions=0, projections=0;
PlayerState player(PlayerId id,std::initializer_list<CardId> models={}) {
    PlayerState p; p.id=id; p.name="p"+std::to_string(id);
    for(auto model:models) p.items.push_back({p.items.size(),model,{},false});
    return p;
}
SnapshotState scene(std::initializer_list<CardId> own={},std::initializer_list<CardId> other={}) {
    SnapshotState s; s.players={player(42,own),player(7,other),player(0)}; return s;
}
Match fixture(SnapshotState state,MatchConfig config={},std::uint64_t seed=42) {
    config.max_decisions=96; Match game(config); game.load_scene(std::move(state),42,seed); ++scenes;
    check(!game.last_decision({}),"load_scene clears record"); return game;
}
void inspect(const Match& game,const MatchSnapshot& before,const Command* command=nullptr) {
    ++decisions;
    const auto saved=game.snapshot(); const auto spectator=game.last_decision({});
    check(spectator && spectator->decision==saved.decisions,"accepted decision number");
    check(spectator->gf==before.state.gf && spectator->phase==before.phase,"acceptance GF and phase");
    sources.insert(spectator->source);
    if(command) {
        check(spectator->actor==command->actor && spectator->kind==command->kind,"command actor and kind");
        std::vector<CardId> faces;
        for(const auto& p:before.state.players) if(p.id==command->actor)
            for(auto token:command->items) for(const auto& item:p.items) if(item.token==token) faces.push_back(face_model(item));
        check(spectator->faces==faces,"pre-consumption faces in selection order");
    }
    std::vector<std::optional<PlayerId>> viewers{std::nullopt};
    for(const auto& p:saved.state.players) viewers.push_back(p.id);
    for(auto viewer:viewers) {
        const auto value=game.last_decision(viewer); ++projections;
        check(value && value->decision==saved.decisions,"all viewers have record");
        (void)game.view(viewer);
        check(game.snapshot()==saved,"last_decision and view preserve full state and RNG");
        Match copy; copy.restore(saved);
        check(copy.snapshot()==saved && copy.last_decision(viewer)==value,"snapshot restore preserves record");
    }
    auto invalid=game;
    bool failed=false;
    try { invalid.step({999,CommandKind::pray,{},{}}); } catch(const MatchError&) { failed=true; }
    check(failed && invalid.snapshot()==saved,"rejected input preserves record and full state");
}
void step(Match& game,Command command) { const auto before=game.snapshot(); game.step(command); inspect(game,before,&command); }
template<class F> void external(Match& game,DecisionSource source,F action,const Command* command=nullptr) {
    const auto before=game.snapshot(); action(); inspect(game,before,command);
    check(game.last_decision({})->source==source,"external source");
}
void random_games() {
    for(std::uint64_t seed=0;seed<32;++seed) {
        MatchConfig cfg; cfg.max_decisions=96; cfg.include_fate=seed&1; cfg.external_grants=seed&2;
        cfg.external_artifacts=seed&4; cfg.local_guardian_payloads=seed&8;
        Match game(cfg); std::vector<PlayerSetup> roster;
        for(unsigned i=0;i<(seed%3==0?9u:seed%3==1?3u:2u);++i) roster.push_back({42+i*7,Team::solo,"seat"});
        game.reset(roster,seed); ++scenes; check(!game.last_decision({}),"reset clears record");
        std::uint64_t policy=seed+91,chance=seed+203;
        while(!game.done()) {
            const auto before=game.snapshot(); auto p=policy,c=chance; std::optional<Command> command;
            if(before.phase!=MatchPhase::grant) {
                const auto own=std::find_if(before.state.players.begin(),before.state.players.end(),[&](const auto& x){return x.id==before.owner;});
                if(before.phase==MatchPhase::main && own->confusion_count>0) command=game.sample_confusion(c);
                else { auto choice=game.sample_choice(p); if(auto* cmd=std::get_if<Command>(&choice)) command=*cmd; }
            }
            game.rollout_step(policy,chance); inspect(game,before,command?&*command:nullptr);
        }
    }
}
void special_scenes() {
    for(bool fog:{false,true}) for(auto kind:{CommandKind::discard,CommandKind::sacrifice}) {
        auto s=scene({178,201}); if(kind==CommandKind::sacrifice) s.tiebreak_gf=0; if(fog) s.players[1].curses={Curse::fog};
        auto game=fixture(s); step(game,{42,kind,{1,0},{}});
        auto value=game.last_decision(7);
        check(value->faces_hidden==fog && value->faces==(fog?std::vector<CardId>{}:std::vector<CardId>{201,178}),"event-time fog discard/sacrifice");
        check(!game.last_decision(42)->faces_hidden,"owner sees discard");
    }
    // Terminal cleanup must not recompute the event-time fog visibility.
    auto terminal_state=scene({178}); terminal_state.players[1].curses={Curse::fog};
    MatchConfig cutoff; cutoff.max_decisions=1; Match terminal(cutoff); terminal.load_scene(terminal_state,42,42); ++scenes;
    step(terminal,{42,CommandKind::discard,{0},{}});
    check(terminal.done() && terminal.last_decision(7)->faces_hidden,"cutoff preserves event-time fog mask");
    // Fog does not hide an ally's pane, matching project_view.
    auto allied=scene({178}); allied.players[0].team=Team::one; allied.players[1].team=Team::one;
    allied.players[1].curses={Curse::fog}; auto ally=fixture(allied); step(ally,{42,CommandKind::discard,{0},{}});
    check(!ally.last_decision(7)->faces_hidden,"fog ally remains visible");
    // Public launch targets ignore optional wide/chance inputs and retain fog's resolved target.
    auto fog_state=scene({6}); fog_state.players[0].curses={Curse::fog}; auto fog_launch=fixture(fog_state);
    step(fog_launch,{42,CommandKind::play,{0},7});
    check(fog_launch.last_decision({})->target==fog_launch.snapshot().last_fog->target,"fog target is resolved target");
    auto wide=fixture(scene({6,232})); step(wide,{42,CommandKind::play,{0,1},7});
    check(!wide.last_decision({})->target,"wide optional target is absent");
    auto exchange=fixture(scene());
    // Locate the supported exchange model from the catalog rather than its display name.
    for(auto model:match_catalog()) if(find_card(model)->ability==Ability::exchange) {
        exchange=fixture(scene({model})); break;
    }
    step(exchange,{42,CommandKind::exchange,{0},{},ResourceAllocation{30,20,20}});
    check(exchange.last_decision({})->allocation==ResourceAllocation{30,20,20},"exchange allocation");
    auto dream=scene({19}); dream.players[0].curses={Curse::dream}; dream.players[0].items[0].fake_model=27;
    auto masked=fixture(dream); step(masked,{42,CommandKind::play,{0},7});
    check(masked.last_decision({})->faces==std::vector<CardId>{27},"Dream exposes face only");
    check(!masked.view({}).last_reveals.empty() && masked.view({}).last_reveals[0].model==19,"actual model stays in existing reveal");
    auto pray=fixture(scene()); step(pray,{42,CommandKind::pray,{},{}}); check(pray.last_decision({})->faces.empty(),"pray hides gift");
    auto confused=scene({6}); confused.players[0].confusion_count=3; auto confusion=fixture(confused);
    Command command{42,CommandKind::play,{0},7};
    external(confusion,DecisionSource::confusion,[&]{confusion.confusion_step(command);},&command);
    auto fate=fixture(scene({207})); external(fate,DecisionSource::fate,[&]{fate.fate_step({42,0,{288,{},{}},{}});});
    check(fate.last_decision({})->faces==std::vector<CardId>{207} && fate.last_decision({})->outcome_model==288 && !fate.last_decision({})->kind,"Fate public fields");
    auto devil=fixture(scene()); external(devil,DecisionSource::devil,[&]{devil.devil_step({7,240,{},{}});});
    auto guardian_state=scene(); guardian_state.players[0].guardian=Guardian::mars;
    auto guardian=fixture(guardian_state); external(guardian,DecisionSource::guardian,[&]{guardian.guardian_step({42,245,7});});
    guardian_state.players[0].guardian=Guardian::earth;
    auto payload=fixture(guardian_state); external(payload,DecisionSource::guardian_payload,[&]{payload.guardian_payload_step({42,Guardian::earth,{6},7});});
    for(CardId model:{CardId{287},CardId{288},CardId{289},CardId{290},CardId{291},CardId{292},CardId{293},CardId{294},CardId{296}}) {
        auto game=fixture(scene()); PhenomenonAction action{model,{},{}};
        if(model>=291 && model<=294) action.recipient=7;
        if(model==296) action.guardians={Guardian::earth,Guardian::moon,Guardian::mars};
        external(game,DecisionSource::phenomenon,[&]{game.phenomenon_step(action);});
        check(!game.last_decision({})->actor && game.last_decision({})->outcome_model==model,"phenomenon fields");
    }
    auto shuffle=fixture(scene({6},{7})); external(shuffle,DecisionSource::item_shuffle,[&]{shuffle.item_shuffle_step({{{7,0},{42,0}}});});
    MatchConfig config; config.external_artifacts=true;
    for(bool artifact:{false,true}) {
        auto game=fixture(scene(),config); step(game,{42,CommandKind::pray,{},{}});
        if(artifact) external(game,DecisionSource::artifact_grant,[&]{game.artifact_grant_step({42,6,{}});});
        else external(game,DecisionSource::grant,[&]{game.grant_step({42,0,{},{}});});
        check(!game.last_decision({})->outcome_model && !game.last_decision({})->actor && game.last_decision({})->faces.empty(),"ordinary grant model hidden");
    }
    auto redraw=fixture(scene({6},{178,7})); step(redraw,{42,CommandKind::play,{0},7});
    command={7,CommandKind::defend,{0},{}};
    external(redraw,DecisionSource::redraw_response,[&]{redraw.redraw_response_step({command,{{{{6,{}}}}}});},&command);
    auto purchase=fixture(scene({5},{6})); step(purchase,{42,CommandKind::buy,{0},7}); command={7,CommandKind::defend,{},{}};
    external(purchase,DecisionSource::purchase_response,[&]{purchase.purchase_response_step({command,ItemToken{0}});},&command);
    step(purchase,{42,CommandKind::accept,{},{}});
    auto summon=fixture(scene({201})); command={42,CommandKind::play,{0},7};
    external(summon,DecisionSource::summon_result,[&]{summon.summon_result_step({command,Guardian::neptune});},&command);
    auto auto_state=scene({6},{233}); auto_state.players[1].guardian=Guardian::neptune;
    auto automatic=fixture(auto_state,config); step(automatic,{42,CommandKind::play,{0},7}); command={7,CommandKind::defend,{0},{}};
    external(automatic,DecisionSource::automatic_guardian_response,[&]{automatic.automatic_guardian_response_step({command,{{7,0,{}}}});},&command);
    // Both PlayerChoice alternatives use their underlying decision source.
    std::uint64_t chance=9;
    auto choice=fixture(scene()); command={42,CommandKind::pray,{},{}};
    external(choice,DecisionSource::player,[&]{choice.choice_step(command,chance);},&command);
    auto choice_fate=fixture(scene({207})); external(choice_fate,DecisionSource::fate,[&]{choice_fate.choice_step(FateRequest{42,0},chance);});
}
void privacy_and_validation() {
    auto a=scene({201},{7}), b=a; b.players[1].items={{100,57,{},false},{200,19,{},false}};
    for(auto kind:{CommandKind::play,CommandKind::discard,CommandKind::pray}) {
        auto first=fixture(a), second=fixture(b), seed=fixture(a,{},999);
        Command cmd{42,kind,kind==CommandKind::pray?std::vector<ItemToken>{}:std::vector<ItemToken>{0},kind==CommandKind::play?std::optional<PlayerId>{7}:std::nullopt};
        step(first,cmd); step(second,cmd); step(seed,cmd);
        for(auto viewer:{std::optional<PlayerId>{42},std::optional<PlayerId>{0},std::optional<PlayerId>{}})
            check(first.last_decision(viewer)==second.last_decision(viewer) && first.last_decision(viewer)==seed.last_decision(viewer),"foreign unused hands and seeds do not leak");
    }
    auto redraw_a=fixture(scene({6},{178,7})); step(redraw_a,{42,CommandKind::play,{0},7}); auto redraw_b=redraw_a;
    Command response{7,CommandKind::defend,{0},{}};
    external(redraw_a,DecisionSource::redraw_response,[&]{redraw_a.redraw_response_step({response,{{{{6,{}}}}}});},&response);
    external(redraw_b,DecisionSource::redraw_response,[&]{redraw_b.redraw_response_step({response,{{{{19,{}}}}}});},&response);
    check(redraw_a.last_decision(42)==redraw_b.last_decision(42),"supplied redraw models do not leak");
    auto summon_a=fixture(scene({201})), summon_b=summon_a; Command launch{42,CommandKind::play,{0},7};
    external(summon_a,DecisionSource::summon_result,[&]{summon_a.summon_result_step({launch,Guardian::earth});},&launch);
    external(summon_b,DecisionSource::summon_result,[&]{summon_b.summon_result_step({launch,Guardian::moon});},&launch);
    check(summon_a.last_decision(42)==summon_b.last_decision(42),"supplied summon result excluded from Command record");
    MatchConfig cfg; cfg.external_artifacts=true;
    auto grant_a=fixture(scene(),cfg); step(grant_a,{42,CommandKind::pray,{},{}}); auto grant_b=grant_a;
    external(grant_a,DecisionSource::artifact_grant,[&]{grant_a.artifact_grant_step({42,6,{}});});
    external(grant_b,DecisionSource::artifact_grant,[&]{grant_b.artifact_grant_step({42,19,{}});});
    check(grant_a.last_decision(7)==grant_b.last_decision(7),"supplied artifact models do not leak");
    auto game=fixture(scene()); step(game,{42,CommandKind::pray,{},{}}); const auto saved=game.snapshot();
    for(unsigned mutation=0;mutation<5;++mutation) {
        auto bad=saved;
        switch(mutation) {
        case 0: bad.version=71; break;
        case 1: ++bad.last_decision->value.decision; break;
        case 2: bad.last_decision->value.source=static_cast<DecisionSource>(255); break;
        case 3: bad.last_decision->hidden_viewers=0x8000; break;
        case 4: bad.last_decision->value.faces_hidden=true; break;
        }
        bool failed=false; try { game.restore(bad); } catch(const MatchError&) { failed=true; }
        check(failed && game.snapshot()==saved,"invalid record/version restore is atomic");
    }
    game.load_scene(scene(),42,99); check(!game.last_decision({}),"load clears previous record");
    game.reset(std::vector<PlayerSetup>{{42,Team::solo,"a"},{7,Team::solo,"b"}},42); check(!game.last_decision({}),"reset clears previous record");
}
}
int main() {
    random_games(); special_scenes(); privacy_and_validation();
    check(sources.size()==14,"all DecisionSource values exercised");
    std::cout<<"Last decision passed: "<<scenes<<" games/scenes, "<<decisions<<" accepted positions, "<<projections<<" viewer projections; "<<sources.size()<<" sources.\n";
}
