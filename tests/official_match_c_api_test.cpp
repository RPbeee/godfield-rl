#include "godfield/official_match_c_api.h"
#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <utility>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
namespace {
void check(bool ok, const char* text) { if (!ok) { std::cerr << "FAIL match C API: " << text << '\n'; std::exit(1); } }
template<class T> bool equal(const T& a, const T& b) { return std::memcmp(&a, &b, sizeof(T)) == 0; }
std::string observe(gfo_match* match, uint8_t own = 1, uint64_t id = 0) {
    size_t size = 0;
    check(gfo_match_observe(match, own, id, nullptr, 0, &size) == 0 && size > 1, "observation size query");
    std::vector<char> bytes(size + 2, 'Z');
    check(gfo_match_observe(match, own, id, bytes.data() + 1, size, &size) == 0
        && bytes.front() == 'Z' && bytes.back() == 'Z' && bytes[size] == 0, "observe canaries/NUL");
    return {bytes.data() + 1};
}
gfo_match_status status(gfo_match* match) { gfo_match_status s{}; check(gfo_match_get_status(match, &s) == 0, "status"); return s; }

// A foreign physical attack bounced back onto the defender keeps the explicit
// redirected-self response route. All setup and transitions cross the C ABI.
gfo_match* redirected_alias_fixture(uint16_t model) {
    gfo_match_config config{};config.max_decisions=128;config.hand_limit=18;
    config.initial_hp=40;
    std::array<gfo_match_player,2> setup{};setup[0].id=0;setup[1].id=77;
    for(uint64_t seed=0;seed<64;++seed) {
        gfo_match* game=nullptr;gfo_state* state=nullptr;gfo_match_status result{};
        check(gfo_match_create(&config,setup.data(),setup.size(),0,&game)==0,"redirected alias match create");
        gfo_state_meta meta{};
        std::array<gfo_state_player,2> players{};players[0].id=0;players[1].id=77;
        for(auto& player:players)player.hp=40;
        players[0].item_count=1;players[1].item_offset=1;players[1].item_count=2;
        std::array<gfo_state_item,3> items{};
        items[0].token=0;items[0].model=model;items[1].token=0;items[1].model=21;items[2].token=1;items[2].model=184;
        check(gfo_state_create(&meta,players.data(),players.size(),items.data(),items.size(),nullptr,0,&state)==0
            &&gfo_match_load_scene(game,state,0,seed,&result)==0,"redirected alias typed scene");
        gfo_state_destroy(state);
        gfo_match_command attack{};attack.item_count=1;attack.items[0]=0;attack.has_target=1;attack.target=77;
        gfo_match_command bounce{};bounce.actor=77;bounce.kind=1;bounce.item_count=1;bounce.items[0]=0;
        check(gfo_match_step(game,&attack,&result)==0&&gfo_match_step(game,&bounce,&result)==0,"foreign launch and physical bounce");
        if(result.phase==1&&result.has_actor&&result.actor==77
            &&observe(game,0,0).find("\"origin\":0,\"source\":77,\"target\":77")!=std::string::npos)return game;
        gfo_match_destroy(game);
    }
    check(false,"bounded bounce fixture reaches an aliased defender");return nullptr;
}

// The public C interface has no execution-checkpoint import. Pair each typed
// C transition with native execution and audit the native full replay separately.
void explicit_ordinary_grant_interop() {
    namespace official = godfield::official;
    for(unsigned variant=0;variant<3;++variant) {
        const bool opted=variant!=0, dream=variant==2;
        gfo_match_config config{};config.max_decisions=32;config.hand_limit=18;
        config.initial_hp=40;config.initial_mp=10;config.initial_cp=20;
        config.external_grants=1;config.include_fate=opted;
        std::array<gfo_match_player,2> setup{};setup[1].id=7;
        gfo_match* game=nullptr;gfo_match_status result{};
        check(gfo_match_create(&config,setup.data(),setup.size(),42,&game)==0,"explicit ordinary C match create");
        std::array<gfo_state_player,2> people{};people[1].id=7;
        for(auto& p:people){p.hp=40;p.mp=10;p.cp=20;}
        people[0].item_count=2;people[1].item_offset=2;
        std::array<gfo_state_item,2> items{};
        items[0].model=152;items[1].token=1;items[1].model=235;items[1].used=1;
        const uint8_t curse=static_cast<uint8_t>(official::Curse::dream);
        if(dream){people[0].curse_count=1;people[1].curse_offset=1;}
        gfo_state_meta meta{};meta.has_tiebreak=1;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,people.data(),people.size(),items.data(),items.size(),dream?&curse:nullptr,dream?1:0,&scene)==0
            &&gfo_match_load_scene(game,scene,0,42,&result)==0,"explicit ordinary C typed scene");
        gfo_state_destroy(scene);
        official::MatchConfig native_config;native_config.initial_hand=0;native_config.max_decisions=32;
        native_config.external_grants=true;native_config.include_fate=opted;
        official::Match native(native_config);official::SnapshotState state;state.tiebreak_gf=0;
        state.players.resize(2);state.players[1].id=7;
        state.players[0].items={{0,152,{},false},{1,235,{},true}};
        if(dream)state.players[0].curses={official::Curse::dream};
        native.load_scene(state,0,42);
        const auto audit=[&] {
            official::Match restored;restored.restore(native.snapshot());
            check(restored.snapshot()==native.snapshot(),"explicit receipt and remaining debt full native snapshot roundtrip");
            check(observe(game,0,0)==native.observe({})&&observe(game,1,0)==native.observe(0)
                &&observe(game,1,7)==native.observe(7),"typed C receipt matches native public projections");
        };
        gfo_match_command sacrifice{};sacrifice.kind=9;sacrifice.item_count=2;sacrifice.items[1]=1;
        check(gfo_match_step(game,&sacrifice,&result)==0&&result.phase==4,"two receipts reserved for explicit-model C interop");
        (void)native.step({0,official::CommandKind::sacrifice,{0,1},{}});audit();
        const auto before=observe(game,1,0);const auto before_status=status(game);uint8_t legal=0;
        gfo_match_grant_action supplied{};supplied.model=opted?207:6;
        if(dream) {
            for(uint16_t model:{uint16_t{6},uint16_t{207}}){supplied.model=model;result=before_status;
                check(gfo_match_grant_legal(game,&supplied,&legal)==0&&!legal
                    &&gfo_match_grant_step(game,&supplied,&result)==-5&&equal(result,before_status)
                    &&observe(game,1,0)==before,"Dream explicit-model grants are unsupported and atomic");}
            supplied.model=0;
        } else {
            if(!opted){auto disabled=supplied;disabled.model=207;result=before_status;
                check(gfo_match_grant_legal(game,&disabled,&legal)==0&&!legal
                    &&gfo_match_grant_step(game,&disabled,&result)<0&&equal(result,before_status)
                    &&observe(game,1,0)==before,"static207 capability still requires opted-in gift pool");}
            for(unsigned bad_kind=0;bad_kind<4;++bad_kind){auto bad=supplied;
                if(bad_kind==0){bad.has_resource=1;bad.resource=0;}
                if(bad_kind==1){bad.removed_count=1;}
                if(bad_kind==2){bad.removed[1]=1;}
                if(bad_kind==3){bad.reserved[0]=1;}
                uint8_t marker=0xa5;result=before_status;
                check(gfo_match_grant_legal(game,&bad,&marker)==-2&&marker==0xa5
                    &&gfo_match_grant_step(game,&bad,&result)==-2&&equal(result,before_status)
                    &&observe(game,1,0)==before,"ordinary-model payload cannot carry resource, removed copies or padding");}
        }
        const auto launch_rng=native.snapshot().rng;
        for(unsigned receipt=0;receipt<2;++receipt){
            if(receipt)supplied.model=dream?0:214;
            const auto before_receipt=native.snapshot();
            check(gfo_match_grant_legal(game,&supplied,&legal)==0&&legal,"ordinary-model C legal is pure");
            check(gfo_match_grant_step(game,&supplied,&result)==0,"ordinary-model C step");
            (void)native.grant_step({0,supplied.model,{},{}});audit();
            if(!dream){
                check(native.snapshot().rng==launch_rng,"supplied ordinary receipts consume no environment RNG");
                const auto after_receipt=native.snapshot();
                const auto& item=after_receipt.state.players[0].items.back();
                check(item.model==supplied.model&&!item.used&&!item.fake_model
                    &&item.token==before_receipt.next_token,"ordinary receipt is a fresh unused unmasked local copy");
            }
            const auto spectator=observe(game,0,0);
            check(spectator.find("\"last_grant\":{\"target\":0,\"devil\":null,\"removed_models\":[]}")!=std::string::npos
                &&spectator.find("\"grant_proof\"")==std::string::npos&&spectator.find("\"grants\"")==std::string::npos
                &&spectator.find("\"token\"")==std::string::npos,"ordinary grant public notice does not expose its supplied model or private proof");
        }
        check(result.phase==0&&result.owner==7&&result.decisions==3,"two explicit receipts fill exactly two debts and advance once");
        gfo_match_destroy(game);
    }
}

void artifact_grant_bridge() {
    namespace official = godfield::official;
    check(gfo_match_artifact_grant_action_size()==16&&sizeof(gfo_match_artifact_grant_action)==16
        &&offsetof(gfo_match_artifact_grant_action,model)==8&&offsetof(gfo_match_artifact_grant_action,fake_model)==10
        &&offsetof(gfo_match_artifact_grant_action,reserved)==12&&gfo_match_grant_action_size()==32,
        "new artifact16 layout leaves old grant32 layout unchanged");
    std::size_t count=0;for(uint16_t model=0;model<=296;++model)count+=gfo_match_artifact_grant_supported(model)!=0;
    check(count==237&&gfo_match_artifact_grant_supported(207)&&!gfo_match_artifact_grant_supported(0)
        &&!gfo_match_artifact_grant_supported(240)&&std::string(gfo_match_artifact_grant_rule())==official::artifact_grant_rule,
        "artifact static ordinary union and exact rule getter");
    check(!official::match_dream_compatible(135,135)&&official::match_dream_disguise_supported(135,135)
        &&!official::match_dream_disguise_supported(207,207),"identity import predicate is separate from distinct random-mask compatibility");
    std::array<std::string,2> identity_public;
    for(unsigned variant=0;variant<6;++variant) {
        const bool identity=variant>=4;
        const bool dream=variant!=0, masked=variant==2||variant==4, opted=variant==3;
        const bool ordinary_mode=masked||identity;
        const uint16_t face=masked?(identity?135:16):0;
        gfo_match_config cfg{};cfg.initial_hp=40;cfg.initial_mp=10;cfg.initial_cp=20;cfg.hand_limit=18;
        cfg.max_decisions=32;cfg.external_grants=!ordinary_mode;cfg.external_artifacts=ordinary_mode;cfg.include_fate=opted;
        std::array<gfo_match_player,2> setup{};setup[1].id=7;gfo_match* game=nullptr;
        check(gfo_match_create(&cfg,setup.data(),setup.size(),42,&game)==0,"artifact C match create");
        std::array<gfo_state_player,2> people{};people[1].id=7;for(auto& p:people){p.hp=40;p.mp=10;p.cp=20;}
        const uint8_t curse=static_cast<uint8_t>(official::Curse::dream);
        if(dream){people[0].curse_count=1;people[1].curse_offset=1;}
        gfo_state_meta meta{};meta.has_tiebreak=!ordinary_mode;gfo_state* state=nullptr;gfo_match_status result{};
        check(gfo_state_create(&meta,people.data(),people.size(),nullptr,0,dream?&curse:nullptr,dream?1:0,&state)==0
            &&gfo_match_load_scene(game,state,0,42,&result)==0,"artifact C received board");
        gfo_state_destroy(state);
        official::MatchConfig nc;nc.initial_hand=0;nc.max_decisions=32;nc.external_grants=!ordinary_mode;nc.external_artifacts=ordinary_mode;nc.include_fate=opted;
        official::Match native(nc);official::SnapshotState ns;if(!ordinary_mode)ns.tiebreak_gf=0;ns.players.resize(2);ns.players[1].id=7;
        if(dream)ns.players[0].curses={official::Curse::dream};native.load_scene(ns,0,42);
        gfo_match_command pray{};pray.kind=3;
        check(gfo_match_step(game,&pray,&result)==0&&result.phase==4,"artifact waits at existing grant phase");
        (void)native.step({0,official::CommandKind::pray,{},{}});
        const auto before=observe(game,1,0), spectator_before=observe(game,0,0);const auto old_status=status(game);
        gfo_match_artifact_grant_action supplied{};supplied.model=identity?135:opted?207:23;supplied.fake_model=face;
        const auto old_input=supplied;
        struct Guarded { uint64_t left;gfo_match_status value;uint64_t right; };
        Guarded guarded{0x123456789abcdef0ULL,old_status,0xfedcba9876543210ULL};uint8_t marker=0xa5;
        check(gfo_match_artifact_grant_legal(nullptr,&supplied,&marker)==-1
            &&gfo_match_artifact_grant_legal(game,nullptr,&marker)==-1
            &&gfo_match_artifact_grant_legal(game,&supplied,nullptr)==-1&&marker==0xa5
            &&gfo_match_artifact_grant_step(nullptr,&supplied,&guarded.value)==-1
            &&gfo_match_artifact_grant_step(game,nullptr,&guarded.value)==-1
            &&gfo_match_artifact_grant_step(game,&supplied,nullptr)==-1&&equal(guarded.value,old_status),
            "artifact NULL guards preserve outputs and pending world");
        for(unsigned byte=0;byte<5;++byte){auto bad=supplied;if(byte<4)bad.reserved[byte]=1;else bad.target=official::max_wire_integer+1;
            marker=0xa5;guarded.value=old_status;
            check(gfo_match_artifact_grant_legal(game,&bad,&marker)==-2&&marker==0xa5
                &&gfo_match_artifact_grant_step(game,&bad,&guarded.value)==-2&&equal(guarded.value,old_status)
                &&observe(game,1,0)==before,"artifact strict reserved/ID fields are atomic");}
        for(unsigned error=0;error<7;++error){auto bad=supplied;
            if(error==0)bad.target=7;if(error==1){bad.model=0;bad.fake_model=0;}
            if(error==2){bad.model=240;bad.fake_model=0;}if(error==3){bad.model=207;bad.fake_model=207;}
            if(error==4){bad.model=23;bad.fake_model=231;}if(error==5){bad.model=207;bad.fake_model=16;}
            if(error==6){bad.model=23;bad.fake_model=207;}
            marker=0xa5;guarded.value=old_status;
            check(gfo_match_artifact_grant_legal(game,&bad,&marker)==0&&!marker
                &&gfo_match_artifact_grant_step(game,&bad,&guarded.value)<0&&equal(guarded.value,old_status)
                &&observe(game,1,0)==before,"artifact semantic invalidity does not leak or partially resolve");}
        if(!dream){auto no_dream=supplied;no_dream.fake_model=16;guarded.value=old_status;
            check(gfo_match_artifact_grant_legal(game,&no_dream,&marker)==0&&!marker
                &&gfo_match_artifact_grant_step(game,&no_dream,&guarded.value)<0&&equal(guarded.value,old_status),"supplied face requires Dream");}
        if(!opted){auto no_pool=supplied;no_pool.model=207;no_pool.fake_model=0;guarded.value=old_status;
            check(gfo_match_artifact_grant_legal(game,&no_pool,&marker)==0&&!marker
                &&gfo_match_artifact_grant_step(game,&no_pool,&guarded.value)<0&&equal(guarded.value,old_status),"artifact207 requires configured gift pool");}
        uint64_t chance=42;gfo_match_grant_action sampled{};
        check(gfo_match_sample_grant(game,&chance,&sampled)==0&&(sampled.model==0||(sampled.model>=240&&sampled.model<=244))
            &&observe(game,1,0)==before,"old local chance sampler still emits only0 or devils and is pure");
        const auto before_native=native.snapshot();marker=0xa5;
        check(gfo_match_artifact_grant_legal(game,&supplied,&marker)==0&&marker&&observe(game,0,0)==spectator_before,
              "artifact legal query is pure");
        check(gfo_match_artifact_grant_step(game,&supplied,&guarded.value)==0&&equal(supplied,old_input)
            &&equal(guarded.value,status(game))&&guarded.left==0x123456789abcdef0ULL&&guarded.right==0xfedcba9876543210ULL,"artifact step writes full status while preserving input and canaries");
        official::ArtifactGrantAction action;action.target=0;action.model=supplied.model;
        if(masked)action.fake_model=face;(void)native.artifact_grant_step(action);
        check(native.snapshot().rng==before_native.rng,"artifact input never draws a Dream coin, face or ordinary model");
        const auto& item=native.snapshot().state.players[0].items.front();
        check(item.token==before_native.next_token&&item.model==supplied.model&&!item.used
            &&item.fake_model==(masked?std::optional<official::CardId>{face}:std::nullopt),"typed artifact receipt preserves exact requested presence");
        official::Match restored;restored.restore(native.snapshot());
        check(restored.snapshot()==native.snapshot()&&observe(game,0,0)==native.observe({})
            &&observe(game,1,0)==native.observe(0)&&observe(game,1,7)==native.observe(7),"artifact full proof roundtrip and C/native viewer projections agree");
        const auto public_view=observe(game,0,0);
        check(public_view.find("\"last_grant\":{\"target\":0,\"devil\":null,\"removed_models\":[]}")!=std::string::npos
            &&public_view.find("\"fake_model\"")==std::string::npos&&public_view.find("\"token\"")==std::string::npos,
            "artifact actual/face/presence are absent from the public grant notice");
        if(identity) {
            identity_public[variant-4]=public_view;
            const auto own=observe(game,1,0);
            check(own.find(masked?"\"disguised\":true":"\"disguised\":false")!=std::string::npos,
                  "identical face and absent face retain distinct owner-visible presence");
            const auto pair_step=[&](gfo_match_command c,official::Command n){
                check(gfo_match_step(game,&c,&result)==0,"identity receipt follow-up C command");(void)native.step(n);
                official::Match replay;replay.restore(native.snapshot());check(replay.snapshot()==native.snapshot()
                    &&observe(game,1,0)==native.observe(0),"identity mask through a later response fully replays");};
            const auto pair_artifact=[&](uint64_t recipient,uint16_t model){
                gfo_match_artifact_grant_action c{};c.target=recipient;c.model=model;
                check(gfo_match_artifact_grant_step(game,&c,&result)==0,"identity receipt follow-up C ordinary input");
                (void)native.artifact_grant_step({recipient,model,{}});};
            gfo_match_command follow{};follow.actor=7;follow.kind=3;
            pair_step(follow,{7,official::CommandKind::pray,{},{}});pair_artifact(7,23);
            follow.actor=0;pair_step(follow,{0,official::CommandKind::pray,{},{}});pair_artifact(0,199);
            const auto weapon=native.snapshot().state.players[1].items.front().token;
            follow={};follow.actor=7;follow.item_count=1;follow.items[0]=weapon;follow.has_target=1;
            pair_step(follow,{7,official::CommandKind::play,{weapon},0});
            follow={};follow.kind=1;follow.item_count=1;follow.items[0]=0;
            pair_step(follow,{0,official::CommandKind::defend,{0},{}});
            const auto& after=native.snapshot();
            check(after.state.players[0].hp==39&&after.last_reveals.size()==std::size_t(masked),
                  "identity defense uses actual DEF5 and emits a reveal only when presence existed");
            if(masked)check(after.last_reveals[0].owner==0&&after.last_reveals[0].face==135&&after.last_reveals[0].model==135
                &&observe(game,0,0).find("\"last_reveals\":[{\"owner\":0,\"face\":135,\"model\":135}]")!=std::string::npos,
                "same-model reveal keeps its explicit face and actual values");
        }
        gfo_match_destroy(game);
    }
    check(identity_public[0]==identity_public[1],"identical-vs-absent face is not leaked to other viewers before use");
}

void normal_grant_modes_bridge() {
    namespace official=godfield::official;
    check(sizeof(gfo_match_config)==24&&offsetof(gfo_match_config,external_artifacts)==21
        &&offsetof(gfo_match_config,reserved_tail)==22
        &&std::string(gfo_match_normal_grant_rule())==official::normal_grant_rule,"normal grant flag fits old24-byte ABI and declares its own rule");
    for(bool artifacts:{false,true})for(bool devils:{false,true})for(int threshold:{-1,0,1}) {
        gfo_match_config cfg{};cfg.initial_hp=40;cfg.initial_mp=10;cfg.initial_cp=20;cfg.hand_limit=18;cfg.max_decisions=32;
        cfg.external_artifacts=artifacts;cfg.external_grants=devils;
        std::array<gfo_match_player,2> setup{};setup[1].id=7;gfo_match* game=nullptr;gfo_match_status result{};
        check(gfo_match_create(&cfg,setup.data(),setup.size(),42,&game)==0,"four-mode normal grant C create");
        std::array<gfo_state_player,2> people{};people[1].id=7;for(auto& p:people){p.hp=40;p.mp=10;p.cp=20;}
        gfo_state_meta meta{};meta.has_tiebreak=threshold>=0;meta.tiebreak_gf=threshold>=0?threshold:0;
        const auto load=[&](uint64_t update){meta.update_count=update;gfo_state* state=nullptr;
            check(gfo_state_create(&meta,people.data(),people.size(),nullptr,0,nullptr,0,&state)==0
                &&gfo_match_load_scene(game,state,0,42,&result)==0,"normal grant typed initial board");gfo_state_destroy(state);};
        load(0);official::MatchConfig nc;nc.initial_hand=0;nc.max_decisions=32;nc.external_artifacts=artifacts;nc.external_grants=devils;
        official::Match native(nc);official::SnapshotState ns;ns.players.resize(2);ns.players[1].id=7;
        if(threshold>=0)ns.tiebreak_gf=threshold;native.load_scene(ns,0,42);
        gfo_match_command pray{};pray.kind=3;check(gfo_match_step(game,&pray,&result)==0,"normal grant C prayer");
        (void)native.step({0,official::CommandKind::pray,{},{}});
        const bool authorized=devils&&threshold==0;
        check((result.phase==4)==(artifacts||authorized),"pause gate is independent of devil authorization");
        if(artifacts||authorized) {
            const auto before=observe(game,1,0);const auto old_status=status(game);
            gfo_match_grant_action devil{};devil.model=240;uint8_t legal=0;
            check(gfo_match_grant_legal(game,&devil,&legal)==0&&bool(legal)==authorized,"devils still require old flag plus event-time apocalypse");
            if(!authorized){result=old_status;
                check(gfo_match_grant_step(game,&devil,&result)==-5&&equal(result,old_status)&&observe(game,1,0)==before,
                      "normal-only devil refusal preserves pending continuation and outputs");}
            uint64_t policy=17,chance=123;gfo_match_grant_action sampled{};sampled.model=65535;
            check(gfo_match_sample_grant(game,&chance,&sampled)==0&&observe(game,1,0)==before,"normal-mode local chance query is pure");
            if(!authorized) {
                check(sampled.model==0&&chance==123,"normal-only chooser consumes no chance entropy");
                gfo_match_command command{};command.kind=77;const auto old_command=command;
                check(gfo_match_sample_action(game,&policy,&command)==-5&&policy==17&&equal(command,old_command),"normal player sampler cannot select a grant result");
                chance=123;check(gfo_match_rollout_step(game,&policy,&chance,&result)==0&&policy==17&&chance==123,
                                "normal-only driver leaves both external streams unchanged");
                (void)native.grant_step({0,0,{},{}});
            } else {
                check(chance!=123,"authorized apocalypse keeps its existing chance draw");
                gfo_match_artifact_grant_action artifact{};artifact.model=23;
                check(gfo_match_artifact_grant_step(game,&artifact,&result)==0,"apocalypse supports explicit ordinary receipt too");
                (void)native.artifact_grant_step({0,23,{}});
            }
        }
        official::Match restored;restored.restore(native.snapshot());
        check(restored.snapshot()==native.snapshot()&&observe(game,0,0)==native.observe({})
            &&observe(game,1,0)==native.observe(0)&&observe(game,1,7)==native.observe(7),"mode/threshold crossing full replay and C-native public views agree");
        check(result.phase==0&&result.owner==7,"original main advances once after optional external normal result");
        if(artifacts&&!devils&&threshold<0) {
            load(official::max_wire_integer-1);check(gfo_match_step(game,&pray,&result)==0&&result.phase==4,"normal-only action can reach last update before result");
            const auto before=observe(game,1,0);const auto old_status=status(game);uint64_t policy=17,chance=123;
            check(gfo_match_rollout_step(game,&policy,&chance,&result)==-2&&policy==17&&chance==123&&equal(result,old_status)
                &&observe(game,1,0)==before,"late normal-only driver failure preserves world, both streams and caller status");
            gfo_match_artifact_grant_action artifact{};artifact.model=23;result=old_status;
            check(gfo_match_artifact_grant_step(game,&artifact,&result)==-2&&equal(result,old_status)&&observe(game,1,0)==before,
                  "late explicit normal result failure preserves C output");
        }
        gfo_match_destroy(game);
    }
}
void self_absorption_alias_bridge() {
    namespace official=godfield::official;
    for(uint16_t model:{uint16_t{27},uint16_t{45},uint16_t{216}}) for(int hp:{1,3,95,99}) {
        gfo_match_config config{};config.max_decisions=64;config.hand_limit=18;config.external_artifacts=1;
        config.initial_hp=40;config.initial_mp=20;config.initial_cp=20;
        std::array<gfo_match_player,2> setup{};setup[1].id=7;
        gfo_match* game=nullptr;gfo_state* received=nullptr;gfo_match_status outcome{};
        check(gfo_match_create(&config,setup.data(),setup.size(),42,&game)==0,"self absorption C create");
        gfo_state_meta meta{};std::array<gfo_state_player,2> people{};people[1].id=7;
        for(auto& person:people){person.hp=40;person.mp=20;person.cp=20;}
        people[0].hp=hp;people[0].item_count=3;people[1].item_offset=3;
        std::array<gfo_state_item,3> items{};items[0].model=model;
        items[1].token=1;items[1].model=208;items[2].token=2;items[2].model=112;
        check(gfo_state_create(&meta,people.data(),people.size(),items.data(),items.size(),nullptr,0,&received)==0
            &&gfo_match_load_scene(game,received,0,42,&outcome)==0,"self absorption C typed low/cap/passive scene");
        gfo_state_destroy(received);
        official::MatchConfig nc;nc.initial_hand=0;nc.initial_mp=20;nc.max_decisions=64;nc.external_artifacts=true;
        official::Match native(nc);official::SnapshotState state;state.players.resize(2);state.players[1].id=7;
        for(auto& person:state.players)person.mp=20;
        state.players[0].hp=hp;state.players[0].items={{0,model,{},false},{1,208,{},false},{2,112,{},false}};
        native.load_scene(state,0,42);
        gfo_match_command use{};use.has_target=1;use.item_count=1;
        check(gfo_match_step(game,&use,&outcome)==0&&outcome.phase==4&&!outcome.has_actor&&outcome.decisions==1,
              "self absorption C finishes both stages in one command then only awaits a gift");
        (void)native.step({0,official::CommandKind::play,{0},0});
        const auto raw=official::find_card(model)->atk;const auto expected=std::min(99,std::max(0,hp-raw)+raw);
        check(native.snapshot().state.players[0].hp==expected&&!native.snapshot().state.players[0].dead
            &&native.snapshot().revivals==0&&native.snapshot().death_bows==0&&native.snapshot().rng==42
            &&observe(game,0,0)==native.observe({})&&observe(game,1,0)==native.observe(0),
              "C/native self absorption agree on low/cap HP,nominal heal,no208/112 and public metadata");
        official::Match copy;copy.restore(native.snapshot());check(copy.snapshot()==native.snapshot(),"self absorption C-corresponding full native replay");
        const auto before=observe(game);const auto previous=status(game);outcome=previous;
        check(gfo_match_step(game,&use,&outcome)==-5&&equal(outcome,previous)&&observe(game)==before,
              "ordinary retry during absorption grant phase preserves C output and world");
        gfo_match_artifact_grant_action artifact{};artifact.model=199;
        check(gfo_match_artifact_grant_step(game,&artifact,&outcome)==0&&outcome.phase==0&&outcome.owner==7,
              "self absorption C explicit receipt resumes owner advancement once");
        (void)native.artifact_grant_step({0,199,{}});
        check(observe(game,0,0)==native.observe({})&&native.snapshot().state.players[0].hp==expected,
              "grant continuation does not reapply either alias stage");
        gfo_match_destroy(game);
    }
}
void directed_dream_receipt_bridge() {
    namespace official=godfield::official;
    std::array<std::string,3> masked_views;
    for(uint16_t actual:{uint16_t{19},uint16_t{27}}) {
        gfo_match_config config{};config.max_decisions=64;config.hand_limit=18;config.external_artifacts=1;
        config.initial_hp=40;config.initial_mp=10;config.initial_cp=20;
        std::array<gfo_match_player,2> setup{};setup[1].id=7;
        gfo_match* game=nullptr;gfo_state* state=nullptr;gfo_match_status outcome{};
        check(gfo_match_create(&config,setup.data(),setup.size(),42,&game)==0,"directed Dream C create");
        std::array<gfo_state_player,2> people{};people[1].id=7;
        for(auto& person:people){person.hp=40;person.mp=10;person.cp=20;}
        people[0].curse_count=1;people[1].curse_offset=1;gfo_state_meta meta{};
        const uint8_t dream=static_cast<uint8_t>(official::Curse::dream);
        check(gfo_state_create(&meta,people.data(),people.size(),nullptr,0,&dream,1,&state)==0
            &&gfo_match_load_scene(game,state,0,42,&outcome)==0,"directed Dream typed empty recipient scene");
        gfo_state_destroy(state);
        official::MatchConfig nc;nc.initial_hand=0;nc.max_decisions=64;nc.external_artifacts=true;
        official::Match native(nc);official::SnapshotState board;board.players.resize(2);board.players[1].id=7;
        board.players[0].curses={official::Curse::dream};native.load_scene(board,0,42);
        const auto audit=[&] {
            official::Match restored;restored.restore(native.snapshot());
            check(restored.snapshot()==native.snapshot()&&observe(game,1,0)==native.observe(0)
                &&observe(game,1,7)==native.observe(7)&&observe(game,0,0)==native.observe({}),
                "directed Dream typed receipt/use agrees with native public views and full replay");
        };
        const auto command=[&](const gfo_match_command& c,const official::Command& n) {
            check(gfo_match_step(game,&c,&outcome)==0,"directed Dream C follow-up command");(void)native.step(n);audit();
        };
        const auto artifact=[&](uint64_t target,uint16_t model,uint16_t face) {
            gfo_match_artifact_grant_action c{};c.target=target;c.model=model;c.fake_model=face;
            check(gfo_match_artifact_grant_step(game,&c,&outcome)==0,"directed Dream explicit result C step");
            (void)native.artifact_grant_step({target,model,face?std::optional<official::CardId>{face}:std::nullopt});audit();
        };
        gfo_match_command pray{};pray.kind=3;command(pray,{0,official::CommandKind::pray,{},{}});
        const auto before=observe(game);const auto previous=status(game);
        for(const auto pair:{std::pair<uint16_t,uint16_t>{27,19},std::pair<uint16_t,uint16_t>{19,45}}) {
            gfo_match_artifact_grant_action bad{};bad.model=pair.first;bad.fake_model=pair.second;
            uint8_t legal=1;outcome=previous;
            check(gfo_match_artifact_grant_legal(game,&bad,&legal)==0&&!legal
                &&gfo_match_artifact_grant_step(game,&bad,&outcome)<0&&equal(outcome,previous)&&observe(game)==before,
                "reverse and unobserved pairs reject without C output or debt mutation");
        }
        artifact(0,actual,27);check(native.snapshot().rng==42&&native.snapshot().dream_rolls==0&&native.snapshot().dream_masks==0,
            "observed pair receipt leaves all local Dream generation RNG/statistics unchanged");
        pray.actor=7;command(pray,{7,official::CommandKind::pray,{},{}});artifact(7,199,0);
        const std::array views{observe(game,1,0),observe(game,1,7),observe(game,0,0)};
        if(actual==19)masked_views=views;else check(masked_views==views,"same face27 hides actual19/27 from every observer before use");
        gfo_match_command self{};self.has_target=1;self.item_count=1;
        const auto token=native.snapshot().state.players[0].items.front().token;self.items[0]=token;
        uint8_t legal=0;check(gfo_match_legal(game,&self,&legal)==0&&legal,"face27 selection has no hidden actual-trait reject oracle");
        command(self,{0,official::CommandKind::play,{token},0});
        check(outcome.phase==4&&native.snapshot().state.players[0].hp==(actual==19?35:40)
            &&native.snapshot().state.players[0].mp==10&&native.snapshot().state.players[0].cp==20
            &&native.snapshot().last_reveals==std::vector<official::RevealedItem>{{0,27,actual,false}},
            "actual19 uses plain5 damage without the face27 heal and reveals its actual model only on use");
        gfo_match_destroy(game);
    }
}
void redraw_response_bridge() {
    namespace official=godfield::official;
    check(gfo_match_redraw_item_size()==8&&gfo_match_redraw_result_size()==16
        &&offsetof(gfo_match_redraw_item,has_fake)==4&&offsetof(gfo_match_redraw_result,item_count)==8
        &&std::string(gfo_match_redraw_response_rule())==official::redraw_response_rule,"redraw additive8/16B layout and static rule");
    for(unsigned scenario=0;scenario<4;++scenario){
        const unsigned hats=scenario==3?2:1,remaining=scenario==2?8:scenario==0?0:1;
        gfo_match_config config{};config.max_decisions=64;config.hand_limit=18;config.external_artifacts=1;
        config.initial_hp=40;config.initial_mp=10;config.initial_cp=20;
        std::array<gfo_match_player,2> setup{};setup[1].id=7;gfo_match* game=nullptr;gfo_match_status outcome{};
        check(gfo_match_create(&config,setup.data(),2,42,&game)==0,"supplied redraw C create");
        std::array<gfo_state_player,2> people{};people[1].id=7;
        for(auto& player:people){player.hp=40;player.mp=10;player.cp=20;}
        people[0].item_count=1;people[1].item_offset=1;people[1].item_count=hats+remaining;
        std::vector<gfo_state_item> items(1+hats+remaining);items[0].model=6;
        for(unsigned i=0;i<hats+remaining;++i){items[i+1].token=i;items[i+1].model=i<hats?178:i==hats?235:6;items[i+1].used=i==hats;}
        gfo_state_meta meta{};meta.gf=1;gfo_state* received=nullptr;
        check(gfo_state_create(&meta,people.data(),2,items.data(),items.size(),nullptr,0,&received)==0
            &&gfo_match_load_scene(game,received,0,42,&outcome)==0,"supplied redraw typed initial scene");gfo_state_destroy(received);
        official::MatchConfig nc;nc.initial_hand=0;nc.external_artifacts=true;nc.max_decisions=64;official::Match native(nc);
        official::SnapshotState state;state.gf=1;state.players.resize(2);state.players[1].id=7;state.players[0].items={{0,6,{},false}};
        for(unsigned i=0;i<hats+remaining;++i)state.players[1].items.push_back({i,static_cast<official::CardId>(i<hats?178:i==hats?235:6),{},i==hats});
        native.load_scene(state,0,42);
        const auto audit=[&]{official::Match restored;restored.restore(native.snapshot());check(restored.snapshot()==native.snapshot(),"redraw private proof restores at every boundary");
            check(observe(game,0,0)==native.observe({})&&observe(game,1,7)==native.observe(7),"redraw C/native spectator and own views match");};
        gfo_match_command launch{};launch.has_target=1;launch.target=7;launch.item_count=1;
        check(gfo_match_step(game,&launch,&outcome)==0,"redraw incoming weapon launch");native.step({0,official::CommandKind::play,{0},7});audit();
        gfo_match_command defense{};defense.actor=7;defense.kind=1;defense.item_count=hats;
        official::RedrawResponseAction native_action;native_action.response={7,official::CommandKind::defend,{}, {}};
        for(unsigned i=0;i<hats;++i){defense.items[i]=i;native_action.response.items.push_back(i);}
        std::vector<std::vector<gfo_match_redraw_item>> rows(hats);std::vector<gfo_match_redraw_result> results(hats);
        const std::array<std::pair<uint16_t,uint16_t>,8> observed{{{3,0},{19,27},{5,0},{202,202},{29,23},{113,135},{196,195},{5,0}}};
        for(unsigned i=0;i<hats;++i){official::RedrawResult nr;
            for(unsigned j=0;j<remaining;++j){const auto pair=remaining==8?observed[j]:std::pair<uint16_t,uint16_t>{19,27};
                rows[i].push_back({pair.first,pair.second,static_cast<uint8_t>(pair.second!=0),{}});
                nr.items.push_back({pair.first,pair.second?std::optional<official::CardId>{pair.second}:std::nullopt});}
            results[i]={rows[i].empty()?nullptr:rows[i].data(),rows[i].size()};native_action.results.push_back(std::move(nr));}
        const auto before=observe(game,1,7);const auto old=status(game);uint8_t legal=77;
        if(scenario==1){
            const auto unchanged=[&]{check(equal(status(game),old)&&observe(game,1,7)==before,"redraw invalid C input leaves full state unchanged");};
            check(gfo_match_redraw_response_legal(nullptr,&defense,results.data(),1,&legal)==-1
                &&gfo_match_redraw_response_legal(game,nullptr,results.data(),1,&legal)==-1
                &&gfo_match_redraw_response_legal(game,&defense,results.data(),1,nullptr)==-1&&legal==77,"redraw NULL outputs unchanged");
            check(gfo_match_redraw_response_legal(game,&defense,reinterpret_cast<const gfo_match_redraw_result*>(1),std::numeric_limits<size_t>::max(),&legal)==-2&&legal==77,"redraw outer extent checked before pointer");
            gfo_match_redraw_result huge{reinterpret_cast<const gfo_match_redraw_item*>(1),19};
            check(gfo_match_redraw_response_legal(game,&defense,&huge,1,&legal)==-2&&legal==77,"redraw nested extent checked before pointer");
            check(gfo_match_redraw_response_legal(game,&defense,nullptr,1,&legal)==-1&&legal==77,"redraw missing result storage");
            gfo_match_redraw_result null_items{nullptr,1};check(gfo_match_redraw_response_legal(game,&defense,&null_items,1,&legal)==-1&&legal==77,"redraw missing item storage");
            for(unsigned error=0;error<6;++error){auto bad=rows[0][0];
                if(error<3)bad.reserved[error]=1;if(error==3)bad.has_fake=2;if(error==4)bad.has_fake=0;if(error==5)bad.fake_model=0;
                const gfo_match_redraw_result invalid{&bad,1};outcome=old;legal=77;
                check(gfo_match_redraw_response_legal(game,&defense,&invalid,1,&legal)==-2&&legal==77
                    &&gfo_match_redraw_response_step(game,&defense,&invalid,1,&outcome)==-2&&equal(outcome,old),"redraw padding/presence errors preserve canaries");unchanged();}
            outcome=old;check(gfo_match_redraw_response_step(game,&defense,nullptr,0,&outcome)==-2&&equal(outcome,old),"partial supplied result list is rejected atomically");unchanged();
        }
        legal=0;check(gfo_match_redraw_response_legal(game,&defense,results.data(),results.size(),&legal)==0&&legal&&observe(game,1,7)==before,"redraw legal is a pure query");
        check(gfo_match_redraw_response_step(game,&defense,results.data(),results.size(),&outcome)==0,"complete supplied replacement response succeeds");native.redraw_response_step(native_action);
        for(auto& row:rows)for(auto& item:row)item.model=1; // Borrowed input no longer backs state/proof.
        audit();check(native.snapshot().rng==42&&native.snapshot().dream_rolls==0&&native.snapshot().dream_masks==0
            &&native.snapshot().redraws==hats&&native.snapshot().redrawn_items==hats*remaining
            &&native.snapshot().state.players[1].items.size()==remaining,"supplied zero/eight/multiple replacements do not draw or retain used copies");
        while(native.snapshot().phase==official::MatchPhase::grant){const auto target=*native.snapshot().pending_grant;gfo_match_artifact_grant_action artifact{};artifact.target=target;artifact.model=6;
            check(gfo_match_artifact_grant_step(game,&artifact,&outcome)==0,"reserved gifts remain independent after supplied redraw");native.artifact_grant_step({target,6,{}});audit();}
        gfo_match_destroy(game);
    }
}
void purchase_response_bridge() {
    namespace official=godfield::official;
    check(gfo_match_purchase_selection_size()==16&&offsetof(gfo_match_purchase_selection,has_commodity)==8
        &&offsetof(gfo_match_purchase_selection,reserved)==9
        &&std::string(gfo_match_purchase_response_rule())==official::purchase_response_rule,"purchase selection additive16B layout and rule");
    for(unsigned scenario=0;scenario<3;++scenario){
        const bool empty=scenario==2;const int cp=scenario==0?0:99;
        gfo_match_config config{};config.max_decisions=64;config.hand_limit=18;config.external_artifacts=1;config.initial_hp=40;config.initial_mp=99;config.initial_cp=cp;
        std::array<gfo_match_player,2> setup{};setup[1].id=7;gfo_match* game=nullptr;gfo_match_status outcome{};
        check(gfo_match_create(&config,setup.data(),2,42,&game)==0,"supplied purchase C create");
        std::array<gfo_state_player,2> people{};people[1].id=7;for(auto& p:people){p.hp=40;p.mp=10;p.cp=20;}
        people[0].mp=99;people[0].cp=cp;people[0].item_count=1;people[1].item_offset=1;people[1].item_count=empty?1:2;people[1].curse_count=1;
        std::vector<gfo_state_item> items(empty?2:3);items[0].token=9;items[0].model=5;
        if(!empty){items[1].model=6;items[1].fake_model=57;}
        items.back().token=1;items.back().model=235;items.back().used=1;
        const uint8_t curse=static_cast<uint8_t>(official::Curse::dream);gfo_state_meta meta{};meta.gf=1;gfo_state* received=nullptr;
        check(gfo_state_create(&meta,people.data(),2,items.data(),items.size(),&curse,1,&received)==0
            &&gfo_match_load_scene(game,received,0,42,&outcome)==0,"supplied purchase typed scene");gfo_state_destroy(received);
        official::MatchConfig nc;nc.initial_hand=0;nc.initial_mp=99;nc.initial_cp=cp;nc.external_artifacts=true;nc.max_decisions=64;official::Match native(nc);official::SnapshotState state;state.gf=1;state.players.resize(2);
        state.players[0].mp=99;state.players[0].cp=cp;state.players[0].items={{9,5,{},false}};state.players[1].id=7;state.players[1].curses={official::Curse::dream};
        if(!empty)state.players[1].items.push_back({0,6,57,false});state.players[1].items.push_back({1,235,{},true});native.load_scene(state,0,42);
        const auto audit=[&]{official::Match restored;restored.restore(native.snapshot());check(restored.snapshot()==native.snapshot(),"supplied purchase pending/completed full proof restores");
            check(observe(game,0,0)==native.observe({})&&observe(game,1,7)==native.observe(7),"supplied purchase C/native projections match");};
        gfo_match_command buy{};buy.kind=6;buy.has_target=1;buy.target=7;buy.item_count=1;buy.items[0]=9;
        check(gfo_match_step(game,&buy,&outcome)==0,"supplied purchase incoming BUY launch");native.step({0,official::CommandKind::buy,{9},7});audit();
        gfo_match_command response{};response.actor=7;response.kind=1;gfo_match_purchase_selection selection{};selection.has_commodity=!empty;
        const auto before=observe(game,1,7);const auto old=status(game);uint8_t legal=77;
        if(scenario==1){
            check(gfo_match_purchase_response_legal(nullptr,&response,&selection,&legal)==-1
                &&gfo_match_purchase_response_legal(game,nullptr,&selection,&legal)==-1
                &&gfo_match_purchase_response_legal(game,&response,nullptr,&legal)==-1
                &&gfo_match_purchase_response_legal(game,&response,&selection,nullptr)==-1&&legal==77,"purchase NULL checks preserve outputs");
            for(unsigned error=0;error<10;++error){auto bad=selection;
                if(error<7)bad.reserved[error]=1;if(error==7)bad.has_commodity=2;if(error==8){bad.has_commodity=0;bad.commodity=1;}if(error==9)bad.commodity=official::max_wire_integer+1;
                legal=77;outcome=old;
                check(gfo_match_purchase_response_legal(game,&response,&bad,&legal)==-2&&legal==77
                    &&gfo_match_purchase_response_step(game,&response,&bad,&outcome)==-2&&equal(outcome,old)
                    &&observe(game,1,7)==before,"purchase padding/inactive/unsafe selection is atomic");}
            for(const auto token:{uint64_t{1},uint64_t{9},uint64_t{123}}){auto bad=selection;bad.commodity=token;legal=77;outcome=old;
                check(gfo_match_purchase_response_legal(game,&response,&bad,&legal)==0&&!legal
                    &&gfo_match_purchase_response_step(game,&response,&bad,&outcome)==-2&&equal(outcome,old)
                    &&observe(game,1,7)==before,"purchase used/foreign/unknown seller token rejected without mutation");}
            auto absent=selection;absent.has_commodity=0;legal=77;check(gfo_match_purchase_response_legal(game,&response,&absent,&legal)==0&&!legal,"absence cannot replace a present unused seller copy");
        }
        check(gfo_match_purchase_response_legal(game,&response,&selection,&legal)==0&&legal&&observe(game,1,7)==before,"exact token0/empty supplied selection legal is pure");
        check(gfo_match_purchase_response_step(game,&response,&selection,&outcome)==0,"supplied BUY response succeeds");
        native.purchase_response_step({{7,official::CommandKind::defend,{},{}},empty?std::optional<official::ItemToken>{}:std::optional<official::ItemToken>{0}});audit();
        check(native.snapshot().rng==42,"supplied commodity consumes no selector RNG");
        if(scenario==1){check(outcome.phase==3&&outcome.actor==0&&native.snapshot().purchase&&native.snapshot().purchase->item.token==0,"affordable current token0 creates buyer quote");
            selection.commodity=123; // Input storage is not retained by the proof.
            gfo_match_command no{};no.kind=8;check(gfo_match_step(game,&no,&outcome)==0,"supplied quote NO remains an ordinary command");native.step({0,official::CommandKind::decline,{},{}});audit();
            check(native.snapshot().state.players[1].items.front()==official::ItemInstance{0,6,{},false},"NO restores the exact chosen unmasked copy");
        }else check(native.snapshot().last_purchase&&native.snapshot().last_purchase->reason==(empty?official::PurchaseReason::empty:official::PurchaseReason::insufficient_cp),"empty and CP-only refusal remain distinct results");
        check(native.snapshot().phase==official::MatchPhase::grant&&native.snapshot().state.players[0].hp==40&&native.snapshot().state.players[0].mp==99,"no CP borrowing from HP/MP and original operation gift remains separate");
        gfo_match_artifact_grant_action artifact{};artifact.model=6;check(gfo_match_artifact_grant_step(game,&artifact,&outcome)==0,"supplied purchase finishes through the existing reserved pump");native.artifact_grant_step({0,6,{}});audit();
        gfo_match_destroy(game);
    }
}
void observed_self41_bridge() {
    namespace official=godfield::official;
    gfo_match_config config{};config.max_decisions=64;config.hand_limit=18;config.external_artifacts=1;
    config.initial_hp=40;config.initial_mp=10;config.initial_cp=20;
    std::array<gfo_match_player,2> setup{};setup[1].id=7;gfo_match* game=nullptr;gfo_match_status outcome{};
    check(gfo_match_create(&config,setup.data(),2,42,&game)==0,"self41 typed match create");
    std::array<gfo_state_player,2> people{};people[1].id=7;for(auto& person:people){person.hp=40;person.mp=10;person.cp=20;}
    people[0].item_count=2;people[1].item_offset=2;std::array<gfo_state_item,2> items{};items[0].model=41;items[1].token=1;items[1].model=150;
    gfo_state_meta meta{};meta.gf=1;gfo_state* received=nullptr;
    check(gfo_state_create(&meta,people.data(),2,items.data(),items.size(),nullptr,0,&received)==0
        &&gfo_match_load_scene(game,received,0,42,&outcome)==0,"self41 typed received scene");gfo_state_destroy(received);
    official::MatchConfig nc;nc.initial_hand=0;nc.external_artifacts=true;nc.max_decisions=64;official::Match native(nc);
    official::SnapshotState state;state.gf=1;state.players.resize(2);state.players[1].id=7;state.players[0].items={{0,41,{},false},{1,150,{},false}};native.load_scene(state,0,42);
    const auto audit=[&]{official::Match restored;restored.restore(native.snapshot());check(restored.snapshot()==native.snapshot(),"self41 pending and completed proof resumes");
        check(observe(game,1,0)==native.observe(0)&&observe(game,0,0)==native.observe({}),"self41 C/native public projection parity");};
    const auto before=observe(game,1,0);const auto old=status(game);gfo_match_command use{};use.has_target=1;use.item_count=1;
    auto bad=use;bad.has_allocation=1;bad.hp=40;bad.mp=10;bad.cp=20;uint8_t legal=77;outcome=old;
    check(gfo_match_legal(game,&bad,&legal)==0&&!legal&&gfo_match_step(game,&bad,&outcome)==-2&&equal(outcome,old)
        &&observe(game,1,0)==before,"self41 semantic allocation rejection preserves state/status");
    legal=0;check(gfo_match_legal(game,&use,&legal)==0&&legal&&gfo_match_step(game,&use,&outcome)==0,"observed single41 self command is accepted");native.step({0,official::CommandKind::play,{0},0});audit();
    check(outcome.phase==4&&!outcome.has_actor&&outcome.decisions==1&&native.snapshot().queue.empty()
        &&native.snapshot().state.players[0].hp==29&&native.snapshot().state.players[0].mp==10&&native.snapshot().state.players[0].cp==20
        &&native.snapshot().state.players[0].curses==std::vector<official::Curse>{official::Curse::darkcloud}
        &&native.snapshot().state.players[0].items==std::vector<official::ItemInstance>{{1,150,{},false}}
        &&native.snapshot().gifts[0]==1&&native.snapshot().rng==42,"self41 damage11 and DarkCloud complete without consuming spare defense or drawing");
    gfo_match_artifact_grant_action artifact{};artifact.model=49;
    check(gfo_match_artifact_grant_step(game,&artifact,&outcome)==0,"self41 observed ordinary receipt is a separate chance input");native.artifact_grant_step({0,49,{}});audit();
    check(outcome.phase==0&&outcome.owner==7&&outcome.decisions==2&&native.snapshot().state.gf==2&&native.snapshot().state.players[0].items.size()==2,"self41 one operation gift advances to the next owner exactly once");
    gfo_match_destroy(game);
}
void observed_self55_bridge() {
    namespace official=godfield::official;
    gfo_match_config config{};config.max_decisions=64;config.hand_limit=18;config.external_artifacts=1;config.initial_hp=40;config.initial_mp=10;config.initial_cp=20;
    std::array<gfo_match_player,2> setup{};setup[1].id=7;gfo_match* game=nullptr;gfo_match_status outcome{};
    check(gfo_match_create(&config,setup.data(),2,42,&game)==0,"self55 C create");
    std::array<gfo_state_player,2> people{};people[1].id=7;for(auto& p:people){p.hp=40;p.mp=10;p.cp=20;}people[0].item_count=2;people[1].item_offset=2;
    std::array<gfo_state_item,2> items{};items[0].token=3;items[0].model=55;items[1].token=9;items[1].model=176;
    gfo_state_meta meta{};meta.gf=1;gfo_state* received=nullptr;
    check(gfo_state_create(&meta,people.data(),2,items.data(),2,nullptr,0,&received)==0&&gfo_match_load_scene(game,received,0,42,&outcome)==0,"self55 typed scene");gfo_state_destroy(received);
    official::MatchConfig nc;nc.initial_hand=0;nc.max_decisions=64;nc.external_artifacts=true;official::Match native(nc);official::SnapshotState state;state.gf=1;state.players.resize(2);state.players[1].id=7;
    state.players[0].items={{3,55,{},false},{9,176,{},false}};native.load_scene(state,0,42);
    const auto audit=[&]{official::Match restored;restored.restore(native.snapshot());check(restored.snapshot()==native.snapshot(),"self55 full pending/completed proof resumes");
        check(observe(game,1,0)==native.observe(0)&&observe(game,0,0)==native.observe({}),"self55 C/native projection parity");};
    gfo_match_command use{};use.has_target=1;use.item_count=1;use.items[0]=3;auto bad=use;bad.has_allocation=1;bad.hp=40;bad.mp=10;bad.cp=20;
    const auto before=observe(game,1,0);const auto old=status(game);uint8_t legal=77;outcome=old;
    check(gfo_match_legal(game,&bad,&legal)==0&&!legal&&gfo_match_step(game,&bad,&outcome)==-2&&equal(outcome,old)&&observe(game,1,0)==before,"self55 semantic illegal allocation is transactional");
    check(gfo_match_legal(game,&use,&legal)==0&&legal&&gfo_match_step(game,&use,&outcome)==0,"observed self55 play succeeds");native.step({0,official::CommandKind::play,{3},0});audit();
    check(outcome.phase==4&&!outcome.has_actor&&outcome.decisions==1&&native.snapshot().queue.empty()&&native.snapshot().state.players[0].hp==25
        &&native.snapshot().state.players[0].mp==10&&native.snapshot().state.players[0].cp==20&&native.snapshot().state.players[0].curses.empty()
        &&native.snapshot().state.players[0].items==std::vector<official::ItemInstance>{{9,176,{},false}}
        &&native.snapshot().gifts[0]==1&&native.snapshot().rng==42,"self55 deals15 in one command without self-block or using176");
    gfo_match_artifact_grant_action artifact{};artifact.model=184;check(gfo_match_artifact_grant_step(game,&artifact,&outcome)==0,"self55 ordinary184 receipt");native.artifact_grant_step({0,184,{}});audit();
    check(outcome.phase==0&&outcome.owner==7&&outcome.decisions==2&&native.snapshot().state.gf==2&&native.snapshot().state.players[0].items.size()==2,"self55 one physical gift advances once");gfo_match_destroy(game);
}
}
int main() {
    observed_self55_bridge();
    observed_self41_bridge();
    purchase_response_bridge();
    redraw_response_bridge();
    std::size_t fixed_query_probes = 0;
    check(gfo_match_abi_version() == 2 && gfo_match_config_size() == sizeof(gfo_match_config)
        && gfo_match_player_size() == sizeof(gfo_match_player) && gfo_match_command_size() == sizeof(gfo_match_command)
        && gfo_match_status_size() == sizeof(gfo_match_status)
        && gfo_match_guardian_action_size() == sizeof(gfo_match_guardian_action)
        && sizeof(gfo_match_guardian_action) == 24 && gfo_state_abi_version() == 1
        && gfo_match_guardian_payload_action_size() == 64 && sizeof(gfo_match_guardian_payload_action) == 64
        && offsetof(gfo_match_guardian_payload_action, models) == 16
        && offsetof(gfo_match_guardian_payload_action, guardian) == 52
        && offsetof(gfo_match_guardian_payload_action, model_count) == 53
        && offsetof(gfo_match_guardian_payload_action, has_target) == 54
        && offsetof(gfo_match_guardian_payload_action, reserved) == 55
        && gfo_match_phenomenon_action_size() == 24 && sizeof(gfo_match_phenomenon_action) == 24
        && offsetof(gfo_match_phenomenon_action, model) == 8
        && offsetof(gfo_match_phenomenon_action, has_recipient) == 10
        && offsetof(gfo_match_phenomenon_action, guardian_count) == 11
        && offsetof(gfo_match_phenomenon_action, guardians) == 12
        && offsetof(gfo_match_phenomenon_action, reserved) == 21
        && gfo_match_item_source_size() == 16 && sizeof(gfo_match_item_source) == 16
        && offsetof(gfo_match_item_source, token) == 8
        && gfo_match_item_shuffle_action_size() == 16 && sizeof(gfo_match_item_shuffle_action) == 16
        && offsetof(gfo_match_item_shuffle_action, count) == 8
        && gfo_match_fate_action_size()==64&&sizeof(gfo_match_fate_action)==64
        &&offsetof(gfo_match_fate_action,outcome)==16&&offsetof(gfo_match_fate_action,shuffle)==40
        &&offsetof(gfo_match_fate_action,has_shuffle)==56&&offsetof(gfo_match_fate_action,reserved)==57
        &&gfo_match_devil_action_size()==32&&sizeof(gfo_match_devil_action)==32
        &&offsetof(gfo_match_devil_action,removed)==8&&offsetof(gfo_match_devil_action,model)==24
        &&offsetof(gfo_match_devil_action,resource)==26&&offsetof(gfo_match_devil_action,has_resource)==27
        &&offsetof(gfo_match_devil_action,removed_count)==28&&offsetof(gfo_match_devil_action,reserved)==29
        &&offsetof(gfo_match_config,external_grants)==6&&offsetof(gfo_match_config,include_fate)==7
        &&offsetof(gfo_match_config,local_guardian_payloads)==20&&offsetof(gfo_match_config,external_artifacts)==21
        &&offsetof(gfo_match_config,reserved_tail)==22
        &&gfo_match_grant_action_size()==32&&sizeof(gfo_match_grant_action)==32, "ABI sizes");
    check(gfo_match_catalog_count() == 236 && gfo_match_gift_weight() == 499 && gfo_match_supported(6)
        && gfo_match_supported(183) && gfo_match_supported(222), "supported subset descriptor");
    check(std::string(gfo_match_ruleset()) == "official-execution-subset-v62"
        && std::string(gfo_match_scope()) == "experimental:236-default-or237-opt-in;external207;external-devils;threshold-sacrifice;external-reserved-grants;not-official-server-equivalence", "explicit v62 scope");
    check(std::string(gfo_match_self_attack_rule()) == "experimental:ordinary-main-explicit-self-target;weapon-base-ability-none-reflect-weapon-reflect-miracle-block-miracle-cut-cost-absorb-hp-attack-twice-atk-by2x-mp-deal-same-damage-or-dream-darkcloud-on-damage-or-miracle-base-ability-none-absorb-hp;hit-rate0-no-wide;immediate-zero-defense-one-decision;dream80-observed36-same-trait-inference-darkcloud41-observed-block-miracle55-observed42-50-56-same-trait-inference-other-curse-weapons-excluded;twice-fresh-launch-payload-two-bodies-stop-on-natural-or-owner-death;other-self-specials-and-redirects-keep-existing-response-route"
        && std::string(gfo_match_self_attack_rule()) == godfield::official::self_attack_rule, "self-attack rule getter");
    for (auto id : {38, 39, 156, 167, 174, 189, 190}) check(gfo_match_supported(id), "added reflection/filter membership");
    for (auto id : {36, 80, 183, 222}) check(gfo_match_supported(id), "Dream scoped models are supported");
    for (auto id : {42, 50, 55, 56, 165, 171, 177, 179, 233}) check(gfo_match_supported(id), "added block membership");
    for (auto id : {46, 66, 83, 143, 163, 173, 204, 231}) check(gfo_match_supported(id), "added composition membership");
    for (auto id : {21, 34, 114, 125, 137, 151, 164, 234}) check(gfo_match_supported(id), "added bounce membership");
    for (auto id : {95, 96, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 225, 226, 227, 228, 229, 230}) check(gfo_match_supported(id), "added pure chance membership");
    for (auto id : {181, 184}) check(gfo_match_supported(id), "damage counter membership");
    for (auto id : {187, 188}) check(gfo_match_supported(id), "resource counter membership");
    for (auto id : {27, 45, 98, 216, 53}) check(gfo_match_supported(id), "damage-effect membership");
    for (auto id : {70, 185, 224, 199, 200, 237, 238}) check(gfo_match_supported(id), "Flash and cure membership");
    const char name[] = {'A', '\0', 'B'};
    std::array<gfo_match_player, 2> players{}; players[0].id = 0; players[0].name = name; players[0].name_size = 3;
    players[1].id = 77; players[1].name = "C"; players[1].name_size = 1;
    // Explicit self-target38/39 with an optional plain ATK addon completes
    // without a synthetic defend or activation of the weapon's response trait.
    // This synthetic routing check does not reconstruct the live game's later CPU/grant actions.
    for(uint16_t model:{uint16_t{38},uint16_t{39}}) for(bool addon:{false,true}) {
        gfo_match* self = nullptr; gfo_state* scene = nullptr; gfo_match_status outcome{};
        check(gfo_match_create(nullptr, players.data(), 2, 0, &self) == 0, "self reflection-weapon create");
        gfo_state_meta meta{}; meta.gf = 2;
        std::array<gfo_state_player, 2> people{}; people[0].id = 0; people[1].id = 77;
        for (auto& person : people) { person.hp = 40; person.mp = 10; person.cp = 20; }
        const std::size_t count=addon?3:2;
        people[1].hp = 50; people[0].item_count = count; people[1].item_offset = count;
        std::array<gfo_state_item,3> items{};items[0].model=model;
        if(addon){items[1].token=1;items[1].model=162;}
        items[count-1].token=2;items[count-1].model=142;
        check(gfo_state_create(&meta, people.data(), people.size(), items.data(), count, nullptr, 0, &scene) == 0
            && gfo_match_load_scene(self, scene, 0, 0, &outcome) == 0, "self reflection-weapon typed scene");
        gfo_state_destroy(scene);
        gfo_match_command use{}; use.item_count = 1; use.items[0] = 0; use.has_target = 1; use.target = 0;
        if(addon){use.item_count=2;use.items[1]=1;}
        uint8_t allowed = 0;
        check(gfo_match_legal(self, &use, &allowed) == 0 && allowed
            && gfo_match_step(self, &use, &outcome) == 0 && outcome.phase == 0 && outcome.actor == 77
            && outcome.decisions == 1 && observe(self).find("\"pending\":null") != std::string::npos
            && observe(self).find(addon?"\"hp\":15,\"mp\":10,\"cp\":20":"\"hp\":30,\"mp\":10,\"cp\":20") != std::string::npos
            && observe(self).find("\"hp\":50,\"mp\":10,\"cp\":20") != std::string::npos,
            "self38/39 applies raw10 or25 once with no response and preserves opposite resources");
        const auto view = observe(self); const auto saved = outcome;
        check(view.find("\"token\":0,")==std::string::npos&&(!addon||view.find("\"token\":1,")==std::string::npos)
            &&view.find("\"token\":2,\"face\":142")!=std::string::npos,
            "self39 consumes selected weapon/addon once and leaves spare DEF unused");
        gfo_match_command extra{}; extra.kind = 1;
        check(gfo_match_legal(self, &extra, &allowed) == 0 && !allowed
            && gfo_match_step(self, &extra, &outcome) == -2 && equal(outcome, saved) && observe(self) == view,
            "extra self-defense is rejected without changing completed state");
        gfo_match_destroy(self);
    }
    // Plain miracle213 self-target: charge once, retain the token, then finish the main command.
    {
        gfo_match* self = nullptr; gfo_state* scene = nullptr; gfo_match_status outcome{};
        check(gfo_match_create(nullptr, players.data(), 2, 0, &self) == 0, "self plain miracle create");
        gfo_state_meta meta{}; meta.gf = 1;
        std::array<gfo_state_player, 2> people{}; people[0].id = 0; people[1].id = 77;
        for (auto& person : people) { person.hp = 40; person.mp = 10; person.cp = 20; }
        people[0].item_count = 1; people[1].item_offset = 1;
        gfo_state_item miracle{}; miracle.token = 0; miracle.model = 213;
        check(gfo_state_create(&meta, people.data(), people.size(), &miracle, 1, nullptr, 0, &scene) == 0
            && gfo_match_load_scene(self, scene, 0, 0, &outcome) == 0, "self plain miracle typed scene");
        gfo_state_destroy(scene);
        gfo_match_command use{}; use.item_count = 1; use.items[0] = 0; use.has_target = 1; use.target = 0;
        uint8_t allowed = 0;
        check(gfo_match_legal(self, &use, &allowed) == 0 && allowed
            && gfo_match_step(self, &use, &outcome) == 0 && outcome.phase == 0 && outcome.actor == 77 && outcome.owner == 77
            && outcome.decisions == 1 && observe(self).find("\"pending\":null") != std::string::npos
            && observe(self).find("\"hp\":34,\"mp\":7,\"cp\":20") != std::string::npos
            && observe(self).find("\"hp\":40,\"mp\":10,\"cp\":20") != std::string::npos
            && observe(self).find("\"token\":0,\"face\":213,\"reference\":213,\"disguised\":false,\"used\":true") != std::string::npos,
            "plain self miracle pays3MP once, takes6HP and retains used token without response");
        const auto view = observe(self); const auto saved = outcome;
        gfo_match_command extra{}; extra.kind = 1;
        check(gfo_match_legal(self, &extra, &allowed) == 0 && !allowed
            && gfo_match_step(self, &extra, &outcome) == -2 && equal(outcome, saved) && observe(self) == view,
            "extra plain-miracle self-defense rejected atomically after completion");
        gfo_match_destroy(self);
    }
    // Source-checked routing, with special-body arithmetic kept as a local contract.
    for (const auto example : std::array<std::array<int, 3>, 7>{{
            {{14, 34, 10}}, {{27, 40, 10}}, {{45, 40, 10}},
            {{53, 12, 10}}, {{58, 20, 0}}, {{216, 40, 0}}, {{46, 28, 10}}}}) {
        gfo_match* self = nullptr; gfo_state* scene = nullptr; gfo_match_status result{};
        check(gfo_match_create(nullptr, players.data(), 2, 0, &self) == 0, "special self create");
        gfo_state_meta meta{}; meta.gf = 1;
        std::array<gfo_state_player, 2> people{}; people[0].id = 0; people[1].id = 77;
        for (auto& person : people) { person.hp = 40; person.mp = 10; person.cp = 20; }
        people[0].item_count = 1; people[1].item_offset = 1;
        gfo_state_item item{}; item.model = static_cast<uint16_t>(example[0]);
        check(gfo_state_create(&meta, people.data(), 2, &item, 1, nullptr, 0, &scene) == 0
            && gfo_match_load_scene(self, scene, 0, 0, &result) == 0, "special self scene");
        gfo_state_destroy(scene);
        gfo_match_command command{}; command.item_count = 1; command.items[0] = 0;
        command.has_target = 1; command.target = 0;
        check(gfo_match_step(self, &command, &result) == 0 && result.phase == 0
            && result.owner == 77 && result.actor == 77 && result.decisions == 1,
            "special self completes all bodies within one accepted decision");
        const auto view = observe(self); const auto saved = result;
        check(view.find("\"pending\":null") != std::string::npos
            && view.find("\"hp\":" + std::to_string(example[1]) + ",\"mp\":" + std::to_string(example[2]) + ",\"cp\":20") != std::string::npos
            && view.find("\"hp\":40,\"mp\":10,\"cp\":20") != std::string::npos,
            "special self preserves existing special arithmetic and launch MP");
        if (example[0] == 216) check(view.find("\"token\":0,\"face\":216,\"reference\":216,\"disguised\":false,\"used\":true") != std::string::npos,
            "self absorption miracle retains its used physical token");
        gfo_match_command extra{}; extra.kind = 1; uint8_t allowed = 1;
        check(gfo_match_legal(self, &extra, &allowed) == 0 && !allowed
            && gfo_match_step(self, &extra, &result) == -2 && equal(result, saved) && observe(self) == view,
            "extra special self defense rejected atomically");
        gfo_match_destroy(self);
    }
    gfo_match* a = nullptr; gfo_match* b = nullptr;
    check(gfo_match_create(nullptr, players.data(), 2, 42, &a) == 0 && a, "create");
    check(gfo_match_create(nullptr, players.data(), 2, 42, &b) == 0 && b, "create replay");
    const auto before = observe(a); const auto initial = status(a);
    check(before.find("\"catalog_version\":\"official-v2\"")!=std::string::npos
        &&before.find(godfield::official::catalog_sha256)!=std::string::npos
        &&before.find(godfield::official::client_sha256)!=std::string::npos,"C bridge projects the current pinned dataset provenance");
    check(before.find("A\\u0000B") != std::string::npos && initial.player_count == 2 && initial.player_ids[1] == 77, "NUL name/fixed ID order");
    std::array<char, 8> short_buffer{}; short_buffer.fill('Q'); const auto old = short_buffer; size_t needed = 555;
    check(gfo_match_observe(a, 1, 0, short_buffer.data(), short_buffer.size(), &needed) == -4
        && short_buffer == old && needed == 555 && observe(a) == before, "short observe is entirely atomic");
    check(gfo_match_observe(a, 1, 88, short_buffer.data(), 8, &needed) == -2 && needed == 555 && short_buffer == old, "unknown viewer output guard");
    check(gfo_match_observe(a, 0, 9, nullptr, 0, &needed) == -2 && needed == 555, "canonical spectator selector");
    uint64_t rng = 12345; gfo_match_command sample{};
    check(gfo_match_sample_action(a, &rng, &sample) == 0 && observe(a) == before, "sample changes only separate policy RNG");
    const auto sample_rng = rng;
    check(gfo_match_sample_action(a, &rng, nullptr) == -1 && rng == sample_rng && observe(a) == before, "NULL sampler output preserves RNG/state");
    check(gfo_match_step(a, &sample, nullptr) == -1 && observe(a) == before, "NULL step result preserves game");
    gfo_match_status result; std::memset(&result, 0x51, sizeof(result)); const auto saved_result = result;
    auto illegal = sample; illegal.actor = 999;
    check(gfo_match_step(a, &illegal, &result) == -2 && equal(result, saved_result) && observe(a) == before, "invalid step leaves output and game unchanged");
    uint8_t legal = 77;
    illegal = sample; illegal.item_count = 19;
    check(gfo_match_legal(a, &illegal, &legal) == -2 && legal == 77, "count19 rejected before items access");
    illegal = sample; illegal.reserved[4] = 1;
    check(gfo_match_step(a, &illegal, &result) == -2 && equal(result, saved_result), "reserved command bytes rejected");
    illegal = sample;
    if (illegal.item_count < 18) { illegal.items[17] = 1; check(gfo_match_step(a, &illegal, &result) == -2 && equal(result, saved_result), "unused token tail rejected"); }
    auto bad_players = players; bad_players[1].id = 0;
    check(gfo_match_reset(a, bad_players.data(), 2, 55, &result) == -2 && equal(result, saved_result) && observe(a) == before, "invalid reset atomicity");
    bad_players = players; bad_players[0].name = "X"; bad_players[0].name_size = std::numeric_limits<size_t>::max();
    check(gfo_match_reset(a, bad_players.data(), 2, 55, &result) == -2 && equal(result, saved_result) && observe(a) == before, "SIZE_MAX string rejected before dereference");
    check(gfo_match_reset(a, players.data(), std::numeric_limits<size_t>::max(), 90, &result) == -2 && equal(result, saved_result), "SIZE_MAX setup count rejected before dereference");
    bad_players = players; bad_players[0].name = "\xed\xa0\x80"; bad_players[0].name_size = 3;
    check(gfo_match_reset(a, bad_players.data(), 2, 55, &result) == -2 && equal(result, saved_result), "invalid UTF8 name rejected");
    auto* sentinel = reinterpret_cast<gfo_match*>(uintptr_t{1});
    check(gfo_match_create(nullptr, bad_players.data(), 2, 42, &sentinel) == -2
        && sentinel == reinterpret_cast<gfo_match*>(uintptr_t{1}), "create output unchanged");
    // Replay after every failed operation proves the private game RNG remains unchanged.
    uint64_t ra = 9, rb = 9;
    for (int i = 0; i < 128 && status(a).has_actor; ++i) {
        gfo_match_command ca{}, cb{};
        check(gfo_match_sample_action(a, &ra, &ca) == 0 && gfo_match_sample_action(b, &rb, &cb) == 0 && ra == rb && equal(ca, cb), "separate-stream deterministic replay");
        gfo_match_status sa{}, sb{};
        check(gfo_match_step(a, &ca, &sa) == 0 && gfo_match_step(b, &cb, &sb) == 0 && equal(sa, sb)
            && observe(a, 0, 0) == observe(b, 0, 0), "replay status/board including future gifts");
    }
    gfo_match_config cfg{}; cfg.max_decisions = 1; cfg.initial_hand = 0; cfg.hand_limit = 18;
    cfg.initial_hp = 40; cfg.initial_mp = 10; cfg.initial_cp = 20;
    gfo_match* short_match = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, 118, &short_match) == 0, "zero-hand canonical match");
    gfo_match_command pray{}; pray.actor = 0; pray.kind = 3;
    check(gfo_match_step(short_match, &pray, &result) == 0 && result.truncated && !result.terminated
        && !result.has_actor && result.rewards[0] == 0 && result.rewards[1] == 0, "fixed-seat truncation");
    rng = 98; sample.kind = 77; const auto stale = sample;
    check(gfo_match_sample_action(short_match, &rng, &sample) == -2 && rng == 98 && equal(sample, stale), "finished sampler atomic failure");
    // Discover tokens with the same public seeded reset contract; all actions
    // and their continuation below cross the opaque C boundary, not load_scene.
    namespace official = godfield::official;
    official::MatchConfig reflection_config; reflection_config.initial_hand = 18; reflection_config.initial_hp = 99;
    official::Match fixture(reflection_config);
    const std::array<official::PlayerSetup, 2> setup{{{0, official::Team::solo, "A"}, {77, official::Team::solo, "B"}}};
    bool found = false; uint64_t fixture_seed = 0, weapon_token = 0, mirror_token = 0;
    for (; fixture_seed < 4096 && !found; ++fixture_seed) {
        fixture.reset(setup, fixture_seed);
        bool weapon = false, mirror = false;
        for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 6) { weapon = true; weapon_token = item.token; }
        for (const auto& item : fixture.snapshot().state.players[1].items) if (item.model == 38) { mirror = true; mirror_token = item.token; }
        found = weapon && mirror;
    }
    check(found, "seeded reflection fixture is reachable from supported gift pool"); --fixture_seed;
    cfg.max_decisions = 512; cfg.initial_hand = 18; cfg.initial_hp = 99;
    gfo_match* reflected_match = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, fixture_seed, &reflected_match) == 0, "reflection create");
    gfo_match_command attack{}; attack.actor = 0; attack.has_target = 1; attack.target = 77; attack.item_count = 1; attack.items[0] = weapon_token;
    check(gfo_match_step(reflected_match, &attack, &result) == 0 && result.phase == 1 && result.actor == 77 && result.owner == 0, "attack opens response via C ABI");
    gfo_match_command reflect{}; reflect.actor = 77; reflect.kind = 1; reflect.item_count = 1; reflect.items[0] = mirror_token;
    check(gfo_match_step(reflected_match, &reflect, &result) == 0 && result.phase == 1 && result.actor == 0 && result.owner == 0, "reflection returns response to attacker");
    const auto returned = observe(reflected_match, 0, 0);
    check(returned.find("\"origin\":0,\"source\":77,\"target\":0") != std::string::npos, "reflection public origin/current-source separation");
    const auto reflected_status = result;
    check(gfo_match_step(reflected_match, &reflect, &result) == -2 && equal(result, reflected_status)
        && observe(reflected_match, 0, 0) == returned, "wrong-actor continuation is atomic");
    gfo_match_command accept{}; accept.actor = 0; accept.kind = 1;
    check(gfo_match_step(reflected_match, &accept, &result) == 0 && result.phase == 0 && result.actor == 77 && result.owner == 77,
        "reflected item resolves once then normal next turn");
    found = false; fixture_seed = 0; uint64_t wall_token = 0;
    for (; fixture_seed < 4096 && !found; ++fixture_seed) {
        fixture.reset(setup, fixture_seed);
        bool weapon = false, wall = false;
        for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 6) { weapon = true; weapon_token = item.token; }
        for (const auto& item : fixture.snapshot().state.players[1].items) if (item.model == 233) { wall = true; wall_token = item.token; }
        found = weapon && wall;
    }
    check(found, "retained Wall reachable from public reset pool"); --fixture_seed;
    gfo_match* blocked_match = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, fixture_seed, &blocked_match) == 0, "Wall fixture create");
    attack.items[0] = weapon_token;
    check(gfo_match_step(blocked_match, &attack, &result) == 0 && result.phase == 1 && result.actor == 77, "Wall incoming weapon");
    gfo_match_command wall{}; wall.actor = 77; wall.kind = 1; wall.item_count = 1; wall.items[0] = wall_token;
    check(gfo_match_step(blocked_match, &wall, &result) == 0 && result.phase == 0 && result.actor == 77 && result.owner == 77, "matching block finishes instead of opening another response");
    const auto blocked = observe(blocked_match, 1, 77);
    check(blocked.find("\"pending\":null") != std::string::npos && blocked.find("\"hp\":99,\"mp\":4") != std::string::npos,
        "block cancels damage and pays six MP");
    const auto wall_json = "\"token\":" + std::to_string(wall_token) + ",\"face\":233,\"reference\":233,\"disguised\":false,\"used\":true";
    check(blocked.find(wall_json) != std::string::npos, "blocking miracle stays retained at same token");
    wall.kind = 0; wall.has_target = 1; wall.target = 0;
    const auto blocked_status = result; legal = 55;
    check(gfo_match_legal(blocked_match, &wall, &legal) == 0 && !legal, "retained Wall is not a main action");
    check(gfo_match_step(blocked_match, &wall, &result) == -2 && equal(result, blocked_status)
        && observe(blocked_match, 1, 77) == blocked, "main Wall rejection preserves outputs and frame");
    found = false; fixture_seed = 0; uint64_t spring_token = 0, free_token = 0;
    for (; fixture_seed < 8192 && !found; ++fixture_seed) {
        fixture.reset(setup, fixture_seed); bool spring = false, free = false;
        for (const auto& item : fixture.snapshot().state.players[0].items) {
            if (item.model == 235) { spring = true; spring_token = item.token; }
            if (item.model == 204) { free = true; free_token = item.token; }
        }
        found = spring && free;
    }
    check(found, "zero-MP Spring/FREE fixture reachable"); --fixture_seed;
    cfg.initial_hp = 40; cfg.initial_mp = 0;
    gfo_match* free_match = nullptr; gfo_match* free_replay = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, fixture_seed, &free_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, fixture_seed, &free_replay) == 0, "zero-MP FREE create");
    const auto unsampled = observe(free_match);
    bool pair_sampled = false;
    for (uint64_t policy_seed = 0; policy_seed < 4096 && !pair_sampled; ++policy_seed) {
        uint64_t policy = policy_seed, same_policy = policy_seed; gfo_match_command candidate{}, replay{};
        check(gfo_match_sample_action(free_match, &policy, &candidate) == 0
            && gfo_match_sample_action(free_match, &same_policy, &replay) == 0
            && policy == same_policy && equal(candidate, replay), "zero-MP sampler deterministic policy stream");
        check(gfo_match_legal(free_match, &candidate, &legal) == 0 && legal, "every sampled zero-MP action legal");
        pair_sampled = candidate.kind == 0 && candidate.item_count == 2
            && candidate.items[0] == spring_token && candidate.items[1] == free_token;
    }
    check(pair_sampled && observe(free_match) == unsampled, "zero-MP sampler reaches miracle+FREE without changing observation");
    gfo_match_command free_cast{}; free_cast.actor = 0; free_cast.has_target = 1; free_cast.target = 0;
    free_cast.item_count = 2; free_cast.items[0] = spring_token; free_cast.items[1] = free_token;
    gfo_match_status free_result{}, replay_result{};
    check(gfo_match_step(free_match, &free_cast, &free_result) == 0 && gfo_match_step(free_replay, &free_cast, &replay_result) == 0
        && equal(free_result, replay_result) && observe(free_match) == observe(free_replay), "sampling preserves private RNG including next replacement gift");
    const auto free_observation = observe(free_match);
    check(free_observation.find("\"hp\":50,\"mp\":0") != std::string::npos
        && free_observation.find("\"token\":" + std::to_string(free_token) + ",") == std::string::npos
        && free_observation.find("\"token\":" + std::to_string(spring_token) + ",\"face\":235,\"reference\":235,\"disguised\":false,\"used\":true") != std::string::npos,
        "zero-MP FREE heal consumes carrier and retains miracle");
    std::size_t hand_count = 0, offset = 0;
    while ((offset = free_observation.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 18, "FREE replacement gifts respect retained full-hand cap");
    gfo_match_destroy(free_match); gfo_match_destroy(free_replay);
    found = false; fixture_seed = 0;
    for (; fixture_seed < 16384 && !found; ++fixture_seed) {
        fixture.reset(setup, fixture_seed); bool weapon = false, wall_found = false, free = false;
        for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 6) { weapon = true; weapon_token = item.token; }
        for (const auto& item : fixture.snapshot().state.players[1].items) {
            if (item.model == 233) { wall_found = true; wall_token = item.token; }
            if (item.model == 204) { free = true; free_token = item.token; }
        }
        found = weapon && wall_found && free;
    }
    check(found, "zero-MP Wall/FREE fixture reachable"); --fixture_seed;
    gfo_match* free_wall = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, fixture_seed, &free_wall) == 0, "zero-MP Wall create");
    attack.items[0] = weapon_token;
    check(gfo_match_step(free_wall, &attack, &result) == 0 && result.phase == 1, "zero-MP Wall incoming attack");
    wall.kind = 1; wall.has_target = 0; wall.target = 0; wall.item_count = 2; wall.items[0] = wall_token; wall.items[1] = free_token;
    check(gfo_match_legal(free_wall, &wall, &legal) == 0 && legal, "zero-MP Wall+FREE response legal");
    check(gfo_match_step(free_wall, &wall, &result) == 0 && result.phase == 0 && result.actor == 77, "free Wall completes payload");
    const auto free_wall_observation = observe(free_wall, 1, 77);
    check(free_wall_observation.find("\"hp\":40,\"mp\":0") != std::string::npos
        && free_wall_observation.find("\"pending\":null") != std::string::npos
        && free_wall_observation.find("\"token\":" + std::to_string(free_token) + ",") == std::string::npos
        && free_wall_observation.find("\"token\":" + std::to_string(wall_token) + ",\"face\":233,\"reference\":233,\"disguised\":false,\"used\":true") != std::string::npos,
        "free response consumes physical carrier and retains same Wall token");
    hand_count = 0; offset = 0;
    while ((offset = free_wall_observation.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 18, "free response refill clipped at hand cap");
    gfo_match_destroy(free_wall);
    // Public seeded reset contains Spring+FREE and Turbulence+FREE at MP0.
    fixture.reset(setup, 511832);
    bool got_spring = false, got_cast_free = false, got_bounce = false, got_bounce_free = false;
    uint64_t cast_free_token = 0, bounce_token = 0, bounce_free_token = 0;
    for (const auto& item : fixture.snapshot().state.players[0].items) {
        if (item.model == 235) { got_spring = true; spring_token = item.token; }
        if (item.model == 204) { got_cast_free = true; cast_free_token = item.token; }
    }
    for (const auto& item : fixture.snapshot().state.players[1].items) {
        if (item.model == 234) { got_bounce = true; bounce_token = item.token; }
        if (item.model == 173) { got_bounce_free = true; bounce_free_token = item.token; }
    }
    check(got_spring && got_cast_free && got_bounce && got_bounce_free, "public reset zero-MP bounce fixture");
    gfo_match* bounce_match = nullptr; gfo_match* bounce_replay = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, 511832, &bounce_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 511832, &bounce_replay) == 0, "bounce fixture create");
    free_cast.target = 77; free_cast.items[0] = spring_token; free_cast.items[1] = cast_free_token;
    check(gfo_match_step(bounce_match, &free_cast, &free_result) == 0
        && gfo_match_step(bounce_replay, &free_cast, &replay_result) == 0 && equal(free_result, replay_result), "incoming recovery miracle opens bounce response");
    gfo_match_command turbulence{}; turbulence.actor = 77; turbulence.kind = 1; turbulence.item_count = 1; turbulence.items[0] = bounce_token;
    const auto before_bounce = observe(bounce_match, 1, 77); result = free_result;
    check(gfo_match_step(bounce_match, &turbulence, &result) == -2 && equal(result, free_result)
        && observe(bounce_match, 1, 77) == before_bounce, "paid Turbulence cannot spill MP debt at MP0");
    turbulence.item_count = 2; turbulence.items[1] = bounce_free_token;
    check(gfo_match_legal(bounce_match, &turbulence, &legal) == 0 && legal, "zero-MP Turbulence/FREE is legal");
    pair_sampled = false;
    for (uint64_t policy_seed = 0; policy_seed < 4096 && !pair_sampled; ++policy_seed) {
        uint64_t policy = policy_seed, same_policy = policy_seed; gfo_match_command candidate{}, replay{};
        check(gfo_match_sample_action(bounce_match, &policy, &candidate) == 0
            && gfo_match_sample_action(bounce_match, &same_policy, &replay) == 0
            && policy == same_policy && equal(candidate, replay), "bounce sampler reproducible policy stream");
        check(gfo_match_legal(bounce_match, &candidate, &legal) == 0 && legal, "all sampled bounce-context actions legal");
        pair_sampled = equal(candidate, turbulence);
    }
    check(pair_sampled && observe(bounce_match, 1, 77) == before_bounce, "sampler reaches free bounce without changing public state");
    check(gfo_match_step(bounce_match, &turbulence, &free_result) == 0
        && gfo_match_step(bounce_replay, &turbulence, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 1 && free_result.owner == 0 && (free_result.actor == 0 || free_result.actor == 77),
        "bounce transfers to an alive responder while retaining original owner");
    const auto bounced_view = observe(bounce_match, 1, 77);
    check(bounced_view.find("\"schema\":\"official-execution-view-v60\"") != std::string::npos
        && bounced_view.find("\"bounce_target_rule\":\"experimental:uniform-all-alive-in-input-order;includes-self-and-teammates\"") != std::string::npos
        && bounced_view.find("\"origin\":0,\"source\":77,\"target\":" + std::to_string(free_result.actor)) != std::string::npos
        && bounced_view.find("\"bounces\":1") != std::string::npos && bounced_view.find("\"reflections\":0") != std::string::npos,
        "public bounce contract and separate redirect counters");
    check(bounced_view.find("\"redirect_rng\":") == std::string::npos && bounced_view.find("\"redirects\":") == std::string::npos
        && bounced_view.find("\"tokens\":") == std::string::npos, "private redirect replay evidence is not observed");
    check(bounced_view.find("\"token\":" + std::to_string(bounce_free_token) + ",") == std::string::npos
        && bounced_view.find("\"token\":" + std::to_string(bounce_token) + ",\"face\":234,\"reference\":234,\"disguised\":false,\"used\":true") != std::string::npos,
        "bounce FREE physical suffix consumed while miracle is retained");
    hand_count = 0; offset = 0;
    while ((offset = bounced_view.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 17, "bounce replacement remains deferred");
    accept.actor = free_result.actor;
    check(gfo_match_step(bounce_match, &accept, &free_result) == 0
        && gfo_match_step(bounce_replay, &accept, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 0 && free_result.actor == 77 && free_result.owner == 77,
        "self or foreign bounce always has explicit response then finishes once");
    check(observe(bounce_match, 1, 0) == observe(bounce_replay, 1, 0)
        && observe(bounce_match, 1, 77) == observe(bounce_replay, 1, 77), "query/policy sampling preserves bounce RNG and future gift stream");
    const auto complete_bounce = observe(bounce_match, 1, 77);
    hand_count = 0; offset = 0;
    while ((offset = complete_bounce.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 18 && complete_bounce.find("\"pending\":null") != std::string::npos, "bounce final gift cap");
    gfo_match_destroy(bounce_match); gfo_match_destroy(bounce_replay);
    // Chance commands use the existing ABI1 has_target=0 representation.
    std::array<gfo_match_player, 4> chance_players{};
    const std::array<official::PlayerSetup, 4> chance_setup{{{0, official::Team::one, "A"}, {77, official::Team::one, "B"},
        {88, official::Team::solo, "C"}, {99, official::Team::two, "D"}}};
    for (std::size_t i = 0; i < chance_players.size(); ++i) {
        chance_players[i].id = chance_setup[i].id; chance_players[i].team = static_cast<uint8_t>(chance_setup[i].team);
    }
    fixture.reset(chance_setup, 716); uint64_t chance_token = 0; bool found_chance = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 95) { chance_token = item.token; found_chance = true; }
    check(found_chance, "public reset multi-target chance fixture");
    gfo_match* chance_match = nullptr; gfo_match* chance_replay = nullptr;
    check(gfo_match_create(&cfg, chance_players.data(), 4, 716, &chance_match) == 0
        && gfo_match_create(&cfg, chance_players.data(), 4, 716, &chance_replay) == 0, "chance create");
    gfo_match_command chance_command{}; chance_command.item_count = 1; chance_command.items[0] = chance_token;
    auto explicit_target = chance_command; explicit_target.has_target = 1; explicit_target.target = 99;
    const auto chance_before = observe(chance_match); result = status(chance_match); const auto chance_status = result;
    check(gfo_match_legal(chance_match, &explicit_target, &legal) == 0 && !legal
        && gfo_match_step(chance_match, &explicit_target, &result) == -2 && equal(result, chance_status)
        && observe(chance_match) == chance_before, "explicit chance target is rejected atomically");
    bool chance_sampled = false;
    for (uint64_t policy_seed = 0; policy_seed < 4096 && !chance_sampled; ++policy_seed) {
        uint64_t policy = policy_seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(chance_match, &policy, &candidate) == 0
            && gfo_match_legal(chance_match, &candidate, &legal) == 0 && legal, "multi-target samples all legal");
        chance_sampled = equal(candidate, chance_command);
    }
    check(chance_sampled, "C ABI sampler emits targetless chance command");
    check(gfo_match_step(chance_match, &chance_command, &free_result) == 0
        && gfo_match_step(chance_replay, &chance_command, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 1 && free_result.actor == 99 && free_result.owner == 0, "miss first enemy then response only for second enemy");
    const auto chance_view = observe(chance_match, 0, 0); // Spectator: only public pending/privacy checks below.
    check(chance_view.find("\"hit_rate\":75") != std::string::npos
        && chance_view.find("\"chance_rule\":\"experimental:targetless-all-alive-enemies-in-input-order;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll;skip-dead;cancel-on-origin-death\"") != std::string::npos,
        "public chance metadata and hit display");
    for (const auto* key : {"\"chance\":", "\"chance_rolls\":", "\"chance_hits\":", "\"targets\":", "\"cursor\":", "\"initial_state\":", "\"responses\":"})
        check(chance_view.find(key) == std::string::npos, "private batch replay state stays hidden");
    accept.actor = 99;
    check(gfo_match_step(chance_match, &accept, &free_result) == 0
        && gfo_match_step(chance_replay, &accept, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 0 && free_result.actor == 77 && free_result.owner == 77
        && observe(chance_match) == observe(chance_replay), "batch completion defers gifts until all targets finish");
    const auto complete_chance = observe(chance_match, 1, 0); // Explicit owner view includes replacement hand.
    check(complete_chance.find("\"id\":99,\"name\":\"\",\"team\":2,\"dead\":false,\"fogged\":false,\"status\":{\"hp\":39") != std::string::npos,
        "only hit enemy loses one HP");
    hand_count = 0; offset = 0;
    while ((offset = complete_chance.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 18, "chance launch replacement counted once");
    gfo_match_destroy(chance_match); gfo_match_destroy(chance_replay);
    fixture.reset(setup, 703); found_chance = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 95) { chance_token = item.token; found_chance = true; }
    check(found_chance && gfo_match_create(&cfg, players.data(), 2, 703, &chance_match) == 0, "all-miss fixture create");
    chance_command.items[0] = chance_token;
    check(gfo_match_step(chance_match, &chance_command, &result) == 0 && result.decisions == 1 && result.phase == 0
        && result.actor == 77 && result.owner == 77 && observe(chance_match).find("\"pending\":null") != std::string::npos,
        "all misses complete without any defense decision");
    gfo_match_destroy(chance_match);
    fixture.reset(setup, 4302);
    bool got_chance_mc = false; got_cast_free = false; bool got_reflector = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) {
        if (item.model == 225) { got_chance_mc = true; spring_token = item.token; }
        if (item.model == 204) { got_cast_free = true; cast_free_token = item.token; }
    }
    for (const auto& item : fixture.snapshot().state.players[1].items) if (item.model == 39) { got_reflector = true; mirror_token = item.token; }
    check(got_chance_mc && got_cast_free && got_reflector
        && gfo_match_create(&cfg, players.data(), 2, 4302, &chance_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 4302, &chance_replay) == 0, "zero-MP chance miracle/FREE fixture create");
    chance_command.item_count = 2; chance_command.items[0] = spring_token; chance_command.items[1] = cast_free_token;
    chance_sampled = false;
    for (uint64_t policy_seed = 0; policy_seed < 4096 && !chance_sampled; ++policy_seed) {
        uint64_t policy = policy_seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(chance_match, &policy, &candidate) == 0
            && gfo_match_legal(chance_match, &candidate, &legal) == 0 && legal, "zero-MP chance-pair samples all legal");
        chance_sampled = equal(candidate, chance_command);
    }
    check(chance_sampled && gfo_match_step(chance_match, &chance_command, &free_result) == 0
        && gfo_match_step(chance_replay, &chance_command, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 1 && free_result.actor == 77, "targetless chance miracle FREE sampled and hits");
    reflect.items[0] = mirror_token;
    check(gfo_match_step(chance_match, &reflect, &free_result) == 0 && gfo_match_step(chance_replay, &reflect, &replay_result) == 0
        && equal(free_result, replay_result) && free_result.phase == 1 && free_result.actor == 0, "hit chance payload can be reflected without reroll");
    const auto redirected_chance = observe(chance_match, 0, 0); // Spectator: origin/source/target and rate only.
    check(redirected_chance.find("\"origin\":0,\"source\":77,\"target\":0") != std::string::npos
        && redirected_chance.find("\"hit_rate\":75") != std::string::npos && redirected_chance.find("\"reflections\":1") != std::string::npos,
        "redirect keeps original chance rate and owner evidence");
    accept.actor = 0;
    check(gfo_match_step(chance_match, &accept, &free_result) == 0 && gfo_match_step(chance_replay, &accept, &replay_result) == 0
        && equal(free_result, replay_result) && observe(chance_match) == observe(chance_replay)
        && observe(chance_match).find("\"hp\":35,\"mp\":0") != std::string::npos, "free chance reflection resolves once and keeps query/policy RNG separate");
    const auto retained_chance = observe(chance_match);
    check(retained_chance.find("\"token\":" + std::to_string(spring_token) + ",\"face\":225,\"reference\":225,\"disguised\":false,\"used\":true") != std::string::npos
        && retained_chance.find("\"token\":" + std::to_string(cast_free_token) + ",") == std::string::npos,
        "chance miracle learned once and physical FREE consumed once");
    gfo_match_destroy(chance_match); gfo_match_destroy(chance_replay);
    // Ordinary DEF reduces HP loss before the selected counters enter FIFO.
    fixture.reset(setup, 54105);
    uint64_t armor_token = 0, mars_token = 0, saturn_token = 0;
    bool got_attack = false, got_armor = false, got_mars = false, got_saturn = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 32) { weapon_token = item.token; got_attack = true; }
    for (const auto& item : fixture.snapshot().state.players[1].items) {
        if (item.model == 118) { armor_token = item.token; got_armor = true; }
        if (item.model == 181) { mars_token = item.token; got_mars = true; }
        if (item.model == 184) { saturn_token = item.token; got_saturn = true; }
    }
    check(got_attack && got_armor && got_mars && got_saturn, "public reset mixed-counter fixture");
    gfo_match* counter_match = nullptr; gfo_match* counter_replay = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, 54105, &counter_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 54105, &counter_replay) == 0, "counter fixture create");
    attack.items[0] = weapon_token;
    check(gfo_match_step(counter_match, &attack, &free_result) == 0 && gfo_match_step(counter_replay, &attack, &replay_result) == 0
        && equal(free_result, replay_result), "ordinary incoming damage for counters");
    const auto before_counters = observe(counter_match, 1, 77);
    for (uint64_t policy_seed = 0; policy_seed < 64; ++policy_seed) {
        uint64_t policy = policy_seed, same_policy = policy_seed; gfo_match_command candidate{}, duplicate{};
        check(gfo_match_sample_action(counter_match, &policy, &candidate) == 0
            && gfo_match_sample_action(counter_match, &same_policy, &duplicate) == 0 && policy == same_policy && equal(candidate, duplicate)
            && gfo_match_legal(counter_match, &candidate, &legal) == 0 && legal, "counter-context sampler legal and reproducible");
    }
    check(observe(counter_match, 1, 77) == before_counters, "counter-context policy sampling leaves match unchanged");
    gfo_match_command defend_counters{}; defend_counters.actor = 77; defend_counters.kind = 1; defend_counters.item_count = 3;
    defend_counters.items[0] = armor_token; defend_counters.items[1] = mars_token; defend_counters.items[2] = saturn_token;
    check(gfo_match_step(counter_match, &defend_counters, &free_result) == 0
        && gfo_match_step(counter_replay, &defend_counters, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 1 && free_result.actor == 0 && free_result.owner == 0, "counter FIFO begins before turn completion");
    const auto mars_view = observe(counter_match, 1, 77);
    check(mars_view.find("\"counter_rule\":\"experimental:actual-hp-loss;survivors-only;selection-order-fifo-before-next-target;skip-dead-source-or-target;single-target;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll\"") != std::string::npos
        && mars_view.find("\"origin\":77,\"source\":77,\"target\":0") != std::string::npos
        && mars_view.find("\"counter\":true") != std::string::npos && mars_view.find("\"models\":[181]") != std::string::npos
        && mars_view.find("\"power\":7") != std::string::npos && mars_view.find("\"hit_rate\":75") != std::string::npos,
        "Mars uses actual seven HP loss after DEF2 instead of original ATK9");
    check(mars_view.find("\"hp\":33,\"mp\":0") != std::string::npos, "surviving defender paid real HP loss");
    hand_count = 0; offset = 0;
    while ((offset = mars_view.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 15, "three selected physical defense cards consumed once before deferred refill");
    for (const auto* key : {"\"batch\":", "\"counters\":", "\"counter_created\":", "\"counter_started\":", "\"counter_rolls\":", "\"counter_hits\":", "\"responses\":", "\"initial_state\":"})
        check(mars_view.find(key) == std::string::npos, "future reserved counters and replay journal stay private");
    accept.actor = 0;
    check(gfo_match_step(counter_match, &accept, &free_result) == 0 && gfo_match_step(counter_replay, &accept, &replay_result) == 0
        && equal(free_result, replay_result) && free_result.phase == 1 && free_result.actor == 0 && free_result.owner == 0,
        "first counter resolves then second FIFO counter opens response");
    const auto saturn_view = observe(counter_match, 0, 0);
    check(saturn_view.find("\"counter\":true") != std::string::npos && saturn_view.find("\"models\":[184]") != std::string::npos
        && saturn_view.find("\"power\":14") != std::string::npos && saturn_view.find("\"hit_rate\":0") != std::string::npos,
        "Saturn doubles actual HP loss and follows Mars selection order");
    check(gfo_match_step(counter_match, &accept, &free_result) == 0 && gfo_match_step(counter_replay, &accept, &replay_result) == 0
        && equal(free_result, replay_result) && free_result.phase == 0 && free_result.actor == 77 && free_result.owner == 77,
        "all counters complete before ordinary next owner");
    check(observe(counter_match, 1, 0) == observe(counter_replay, 1, 0)
        && observe(counter_match, 1, 77) == observe(counter_replay, 1, 77), "policy queries preserve counter roll and future gift RNG");
    const auto complete_counter = observe(counter_match, 1, 77);
    check(complete_counter.find("\"hp\":19,\"mp\":0") != std::string::npos && complete_counter.find("\"hp\":33,\"mp\":0") != std::string::npos
        && complete_counter.find("\"pending\":null") != std::string::npos, "counter FIFO final resource deltas");
    hand_count = 0; offset = 0;
    while ((offset = complete_counter.find("\"token\":", offset)) != std::string::npos) { ++hand_count; ++offset; }
    check(hand_count == 18, "counter defenses refill only after full action");
    gfo_match_destroy(counter_match); gfo_match_destroy(counter_replay);
    fixture.reset(setup, 58017); got_attack = false; got_mars = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 6) { weapon_token = item.token; got_attack = true; }
    for (const auto& item : fixture.snapshot().state.players[1].items) if (item.model == 181) { mars_token = item.token; got_mars = true; }
    check(got_attack && got_mars && gfo_match_create(&cfg, players.data(), 2, 58017, &counter_match) == 0, "Mars-miss fixture create");
    attack.items[0] = weapon_token; defend_counters.item_count = 1; defend_counters.items[0] = mars_token;
    defend_counters.items[1] = 0; defend_counters.items[2] = 0;
    check(gfo_match_step(counter_match, &attack, &result) == 0
        && gfo_match_step(counter_match, &defend_counters, &result) == 0 && result.decisions == 2 && result.phase == 0
        && result.actor == 77 && result.owner == 77 && observe(counter_match).find("\"pending\":null") != std::string::npos,
        "missed Mars counter consumes no extra response decision");
    gfo_match_destroy(counter_match);
    // Resource counters stay armor payloads, never summon metadata guardians.
    cfg.initial_cp = 0;
    fixture.reset(setup, 1004); bool got_mp_counter = false; got_attack = false; got_armor = false;
    uint64_t resource_token = 0;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 6) { weapon_token = item.token; got_attack = true; }
    for (const auto& item : fixture.snapshot().state.players[1].items) {
        if (item.model == 187) { resource_token = item.token; got_mp_counter = true; }
        if (item.model == 142) { armor_token = item.token; got_armor = true; }
    }
    check(got_attack && got_mp_counter && got_armor, "public reset Neptune fixture");
    gfo_match* resource_match = nullptr; gfo_match* resource_replay = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, 1004, &resource_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 1004, &resource_replay) == 0, "resource counter create");
    attack.items[0] = weapon_token;
    check(gfo_match_step(resource_match, &attack, &free_result) == 0
        && gfo_match_step(resource_replay, &attack, &replay_result) == 0 && equal(free_result, replay_result), "resource counter incoming attack");
    gfo_match_command resource_defense{}; resource_defense.actor = 77; resource_defense.kind = 1; resource_defense.item_count = 1; resource_defense.items[0] = resource_token;
    check(gfo_match_step(resource_match, &resource_defense, &free_result) == 0
        && gfo_match_step(resource_replay, &resource_defense, &replay_result) == 0 && equal(free_result, replay_result)
        && free_result.phase == 1 && free_result.actor == 77 && free_result.owner == 0, "Neptune has explicit self response");
    const auto resource_view = observe(resource_match, 1, 77);
    check(resource_view.find("\"resource_counter_rule\":\"experimental:mp-self-target-with-response;cp-target-incoming-source;debit-current-target-then-full-credit-current-source;resource-debt-no-new-counter\"") != std::string::npos
        && resource_view.find("\"origin\":77,\"source\":77,\"target\":77") != std::string::npos
        && resource_view.find("\"power\":0") != std::string::npos && resource_view.find("\"amount\":2") != std::string::npos
        && resource_view.find("\"guardian\":\"neptune\"") == std::string::npos, "resource payload carries amount without summoning metadata guardian");
    auto invalid_resource_defense = resource_defense; invalid_resource_defense.items[0] = armor_token; result = free_result;
    check(gfo_match_step(resource_match, &invalid_resource_defense, &result) == -2 && equal(result, free_result)
        && observe(resource_match, 1, 77) == resource_view, "ordinary DEF cannot answer non-damage resource counter");
    for (uint64_t policy_seed = 0; policy_seed < 64; ++policy_seed) {
        uint64_t policy = policy_seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(resource_match, &policy, &candidate) == 0
            && gfo_match_legal(resource_match, &candidate, &legal) == 0 && legal, "resource-response samples legal");
    }
    accept.actor = 77;
    check(gfo_match_step(resource_match, &accept, &free_result) == 0 && gfo_match_step(resource_replay, &accept, &replay_result) == 0
        && equal(free_result, replay_result) && free_result.phase == 0 && free_result.owner == 77
        && observe(resource_match, 1, 77) == observe(resource_replay, 1, 77)
        && observe(resource_match).find("\"hp\":39,\"mp\":2,\"cp\":0") != std::string::npos, "Neptune applies once; policy queries leave future gifts unchanged");
    gfo_match_destroy(resource_match); gfo_match_destroy(resource_replay);
    fixture.reset(setup, 14656); got_attack = false; bool got_cp_counter = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 32) { weapon_token = item.token; got_attack = true; }
    for (const auto& item : fixture.snapshot().state.players[1].items) if (item.model == 188) { resource_token = item.token; got_cp_counter = true; }
    check(got_attack && got_cp_counter && gfo_match_create(&cfg, players.data(), 2, 14656, &resource_match) == 0, "public reset Venus fixture");
    attack.items[0] = weapon_token; resource_defense.items[0] = resource_token;
    check(gfo_match_step(resource_match, &attack, &result) == 0 && gfo_match_step(resource_match, &resource_defense, &result) == 0
        && result.phase == 1 && result.actor == 0 && result.owner == 0, "Venus targets incoming source");
    const auto cp_view = observe(resource_match, 0, 0);
    check(cp_view.find("\"origin\":77,\"source\":77,\"target\":0") != std::string::npos
        && cp_view.find("\"amount\":9") != std::string::npos && cp_view.find("\"power\":0") != std::string::npos,
        "Venus uses actual nine HP loss as nominal CP amount");
    accept.actor = 0;
    check(gfo_match_step(resource_match, &accept, &result) == 0 && result.phase == 0
        && observe(resource_match).find("\"hp\":31,\"mp\":0,\"cp\":9") != std::string::npos
        && observe(resource_match).find("\"hp\":31,\"mp\":0,\"cp\":0") != std::string::npos,
        "CP debt falls through MP and HP while creditor receives nominal CP");
    gfo_match_destroy(resource_match);
    fixture.reset(setup, 1794); got_attack = false; got_cp_counter = false; bool got_any_reflector = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) {
        if (item.model == 6) { weapon_token = item.token; got_attack = true; }
        if (item.model == 190) { mirror_token = item.token; got_any_reflector = true; }
    }
    for (const auto& item : fixture.snapshot().state.players[1].items) if (item.model == 188) { resource_token = item.token; got_cp_counter = true; }
    check(got_attack && got_cp_counter && got_any_reflector && gfo_match_create(&cfg, players.data(), 2, 1794, &resource_match) == 0, "public reset Venus reflection fixture");
    attack.items[0] = weapon_token; resource_defense.items[0] = resource_token;
    check(gfo_match_step(resource_match, &attack, &result) == 0 && gfo_match_step(resource_match, &resource_defense, &result) == 0, "CP reflection incoming");
    gfo_match_command resource_mirror{}; resource_mirror.actor = 0; resource_mirror.kind = 1; resource_mirror.item_count = 1; resource_mirror.items[0] = mirror_token;
    check(gfo_match_step(resource_match, &resource_mirror, &result) == 0 && result.phase == 1 && result.actor == 77
        && observe(resource_match, 0, 0).find("\"origin\":77,\"source\":0,\"target\":77") != std::string::npos,
        "universal reflection exchanges current creditor and debtor");
    accept.actor = 77;
    check(gfo_match_step(resource_match, &accept, &result) == 0
        && observe(resource_match).find("\"hp\":40,\"mp\":0,\"cp\":1") != std::string::npos
        && observe(resource_match).find("\"hp\":38,\"mp\":0,\"cp\":0") != std::string::npos,
        "reflected CP payment follows current endpoints");
    gfo_match_destroy(resource_match);
    // Explicit ordinary self-attack resolves immediately, so held counters cannot be selected.
    fixture.reset(setup, 14656); got_attack = false; got_cp_counter = false;
    for (const auto& item : fixture.snapshot().state.players[0].items) {
        if (item.model == 6) { weapon_token = item.token; got_attack = true; }
        if (item.model == 188) { resource_token = item.token; got_cp_counter = true; }
    }
    cfg.initial_hp = 2;
    check(got_attack && got_cp_counter && gfo_match_create(&cfg, players.data(), 2, 14656, &resource_match) == 0, "public reset immediate self-attack fixture");
    attack.items[0] = weapon_token; attack.target = 0; resource_defense.actor = 0; resource_defense.items[0] = resource_token;
    check(gfo_match_step(resource_match, &attack, &result) == 0 && result.phase == 0 && result.actor == 77
        && result.decisions == 1 && !result.terminated
        && observe(resource_match).find("\"hp\":1,\"mp\":0,\"cp\":0") != std::string::npos
        && observe(resource_match).find("\"pending\":null") != std::string::npos
        && observe(resource_match).find("\"self_attack_rule\":\"" + std::string(gfo_match_self_attack_rule()) + "\"") != std::string::npos,
        "ordinary self attack applies raw1 without a defense or CP-counter decision");
    const auto immediate_view = observe(resource_match); const auto immediate_status = result;
    check(gfo_match_legal(resource_match, &resource_defense, &legal) == 0 && !legal
        && gfo_match_step(resource_match, &resource_defense, &result) == -2 && equal(result, immediate_status)
        && observe(resource_match) == immediate_view, "obsolete self-defense rejected atomically after immediate completion");
    gfo_match_destroy(resource_match);
    cfg.initial_hp = 1;
    check(gfo_match_create(&cfg, players.data(), 2, 14656, &resource_match) == 0
        && gfo_match_step(resource_match, &attack, &result) == 0 && result.terminated && result.decisions == 1
        && result.winner_count == 1 && result.winners[0] == 77 && result.rewards[0] == -1 && result.rewards[1] == 1
        && observe(resource_match).find("\"hp\":0,\"mp\":0,\"cp\":0") != std::string::npos,
        "lethal ordinary self attack ends naturally without a response or CP credit");
    gfo_match_destroy(resource_match);
    // Damage effects use current source, full raw remainder and ordered clamps.
    const auto receipt_token = [&](std::size_t seat, official::CardId model) {
        for (const auto& item : fixture.snapshot().state.players[seat].items) if (item.model == model) return item.token;
        std::cerr << "missing seeded model=" << model << " seat=" << seat << " inventory=";
        for (const auto& item : fixture.snapshot().state.players[seat].items) std::cerr << item.model << ',';
        std::cerr << '\n';
        check(false, "public reset fixture model missing"); return uint64_t{0};
    };
    cfg.initial_hp = 3; cfg.initial_mp = 0; cfg.initial_cp = 0;
    fixture.reset(setup, 1121); weapon_token = receipt_token(0, 27);
    gfo_match* effect_match = nullptr; gfo_match* effect_replay = nullptr;
    check(gfo_match_create(&cfg, players.data(), 2, 1121, &effect_match) == 0, "raw absorption fixture create");
    gfo_match_command effect_attack{}; effect_attack.has_target = 1; effect_attack.target = 77; effect_attack.item_count = 1; effect_attack.items[0] = weapon_token;
    gfo_match_command effect_pass{}; effect_pass.actor = 77; effect_pass.kind = 1;
    check(gfo_match_step(effect_match, &effect_attack, &result) == 0 && gfo_match_step(effect_match, &effect_pass, &result) == 0
        && result.terminated && result.winners[0] == 0 && result.rewards[0] == 1 && result.rewards[1] == -1,
        "raw absorption completes natural terminal");
    const auto raw_absorb = observe(effect_match);
    check(raw_absorb.find("\"damage_effect_rule\":\"experimental:positive-nondark-remainder;self-absorption-target-damage-before-full-nominal-heal;nonself-absorption-and-same-damage-source-side-effect-before-target-damage;counter-basis-target-damage-only;death-and-revival-after-whole-body;self27-low-hp-observed-other-aliases-same-trait-inference\"") != std::string::npos
        && raw_absorb.find("\"hp\":10,\"mp\":0") != std::string::npos && raw_absorb.find("\"hp\":0,\"mp\":0") != std::string::npos,
        "source gains raw7 even when target can lose only3");
    gfo_match_destroy(effect_match);
    cfg.initial_hp = 98; effect_attack.target = 0; effect_pass.actor = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 1121, &effect_match) == 0
        && gfo_match_step(effect_match, &effect_attack, &result) == 0 && result.phase == 0 && result.decisions == 1
        && observe(effect_match).find("\"hp\":98,\"mp\":0") != std::string::npos,
        "immediate source-target alias takes raw7 then heals nominal7 without defense");
    gfo_match_destroy(effect_match);
    cfg.initial_hp = 40; effect_attack.target = 77; effect_pass.actor = 77;
    check(gfo_match_create(&cfg, players.data(), 2, 1121, &effect_match) == 0, "zero-remainder absorption create");
    gfo_match_command effect_defense{}; effect_defense.actor = 77; effect_defense.kind = 1; effect_defense.item_count = 1;
    effect_defense.items[0] = receipt_token(1, 176);
    check(gfo_match_step(effect_match, &effect_attack, &result) == 0 && gfo_match_step(effect_match, &effect_defense, &result) == 0
        && observe(effect_match).find("\"hp\":47") == std::string::npos && result.phase == 0, "zero remainder does not heal source");
    gfo_match_destroy(effect_match);
    effect_match = redirected_alias_fixture(27);
    effect_defense.actor = 77; effect_defense.items[0] = 1; effect_pass.actor = 77;
    check(gfo_match_step(effect_match, &effect_defense, &result) == 0
        && result.phase == 1 && result.actor == 77, "redirected aliased absorption creates target-stage counter despite net HP0 change");
    const auto absorption_counter = observe(effect_match, 0, 0);
    check(absorption_counter.find("\"power\":14") != std::string::npos
        && absorption_counter.find("\"id\":77,\"name\":\"\",\"team\":0,\"dead\":false,\"fogged\":false,\"status\":{\"hp\":40,\"mp\":0") != std::string::npos,
        "redirected counter basis is target-stage7 rather than net0");
    check(gfo_match_step(effect_match, &effect_pass, &result) == 0
        && observe(effect_match).find("\"id\":77,\"name\":\"\",\"team\":0,\"dead\":false,\"fogged\":false,\"status\":{\"hp\":26,\"mp\":0") != std::string::npos,
        "counter does not inherit absorption ability");
    gfo_match_destroy(effect_match); effect_pass.actor = 0;
    fixture.reset(setup, 9249); effect_attack.target = 77; effect_attack.item_count = 2;
    effect_attack.items[0] = receipt_token(0, 216); effect_attack.items[1] = receipt_token(0, 204);
    effect_defense.actor = 77; effect_defense.items[0] = receipt_token(1, 39);
    check(gfo_match_create(&cfg, players.data(), 2, 9249, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 9249, &effect_replay) == 0, "free absorption miracle create");
    const auto before_effect_samples = observe(effect_match);
    for (uint64_t policy_seed = 0; policy_seed < 64; ++policy_seed) {
        uint64_t policy = policy_seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "damage-effect sampler stays legal");
    }
    check(observe(effect_match) == before_effect_samples && gfo_match_step(effect_match, &effect_attack, &free_result) == 0
        && gfo_match_step(effect_replay, &effect_attack, &replay_result) == 0 && equal(free_result, replay_result)
        && gfo_match_step(effect_match, &effect_defense, &free_result) == 0
        && gfo_match_step(effect_replay, &effect_defense, &replay_result) == 0 && equal(free_result, replay_result), "FREE miracle and redirect are transactional before body");
    const auto unresolved_absorb = observe(effect_match, 0, 0);
    check(unresolved_absorb.find("\"origin\":0,\"source\":77,\"target\":0") != std::string::npos
        && unresolved_absorb.find("\"hp\":50") == std::string::npos, "redirect does not apply absorption yet");
    check(gfo_match_step(effect_match, &effect_pass, &free_result) == 0 && gfo_match_step(effect_replay, &effect_pass, &replay_result) == 0
        && equal(free_result, replay_result) && observe(effect_match) == observe(effect_replay), "effect queries preserve future gift RNG");
    const auto absorbed_mc = observe(effect_match);
    check(absorbed_mc.find("\"hp\":30,\"mp\":0") != std::string::npos && absorbed_mc.find("\"hp\":50,\"mp\":0") != std::string::npos
        && absorbed_mc.find("\"token\":" + std::to_string(effect_attack.items[0]) + ",\"face\":216,\"reference\":216,\"disguised\":false,\"used\":true") != std::string::npos
        && absorbed_mc.find("\"token\":" + std::to_string(effect_attack.items[1]) + ",") == std::string::npos,
        "miracle heals reflected current source, retains original token and consumes FREE once");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 2); effect_attack.item_count = 1; effect_attack.items[1] = 0; effect_attack.has_target = 0; effect_attack.target = 0;
    effect_attack.items[0] = receipt_token(0, 98); effect_pass.actor = 77;
    check(gfo_match_create(&cfg, players.data(), 2, 2, &effect_match) == 0
        && gfo_match_step(effect_match, &effect_attack, &result) == 0 && result.phase == 1
        && gfo_match_step(effect_match, &effect_pass, &result) == 0 && observe(effect_match).find("\"hp\":43,\"mp\":0") != std::string::npos
        && observe(effect_match).find("\"hp\":37,\"mp\":0") != std::string::npos, "Wood75% absorption uses targetless hit pipeline");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 214); effect_attack.items[0] = receipt_token(0, 98);
    check(gfo_match_create(&cfg, players.data(), 2, 214, &effect_match) == 0
        && gfo_match_step(effect_match, &effect_attack, &result) == 0 && result.phase == 0 && result.decisions == 1
        && observe(effect_match).find("\"hp\":43") == std::string::npos, "missed chance absorption has no source effect");
    gfo_match_destroy(effect_match);
    effect_match = redirected_alias_fixture(53);
    effect_defense.actor = 77; effect_defense.items[0] = 1; effect_pass.actor = 77;
    check(gfo_match_step(effect_match, &effect_defense, &result) == 0
        && result.phase == 1 && result.actor == 77
        && observe(effect_match).find("\"id\":77,\"name\":\"\",\"team\":0,\"dead\":false,\"fogged\":false,\"status\":{\"hp\":12,\"mp\":0") != std::string::npos
        && observe(effect_match).find("\"power\":28") != std::string::npos,
        "redirected self damage subtracts raw14 twice, but counter doubles only target-stage14");
    check(gfo_match_step(effect_match, &effect_pass, &result) == 0 && result.terminated
        && result.winner_count == 1 && result.winners[0] == 0
        && observe(effect_match).find("\"id\":0,\"name\":\"\",\"team\":0,\"dead\":false,\"fogged\":false,\"status\":{\"hp\":40,\"mp\":0") != std::string::npos,
        "derived counter kills the aliased defender while original attacker stays unharmed");
    gfo_match_destroy(effect_match); effect_pass.actor = 0;
    // Flash restrictions and curing cross the opaque ABI, including atomic failure.
    fixture.reset(setup, 1292501); cfg.initial_hp = 40; cfg.initial_mp = cfg.initial_cp = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 1292501, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 1292501, &effect_replay) == 0, "Flash public reset fixture create");
    gfo_match_command flash_attack{}; flash_attack.item_count = 1; flash_attack.items[0] = receipt_token(0, 70); flash_attack.has_target = 1;
    gfo_match_command flash_pass{}; flash_pass.kind = 1;
    const auto both_step = [&](const gfo_match_command& command) {
        check(gfo_match_step(effect_match, &command, &free_result) == 0
            && gfo_match_step(effect_replay, &command, &replay_result) == 0 && equal(free_result, replay_result), "curse replay command/status");
    };
    both_step(flash_attack); both_step(flash_pass);
    check(observe(effect_match).find("\"curses\":[\"flash\"]") != std::string::npos
        && observe(effect_match).find("\"flash_rule\":") != std::string::npos, "Flash is visible through C ABI");
    flash_attack.actor = 77; flash_attack.items[0] = receipt_token(1, 6); both_step(flash_attack);
    gfo_match_command two_defenses{}; two_defenses.kind = 1;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (two_defenses.item_count < 2 && item.model < 181) {
        const auto* card = official::find_card(item.model);
        if (card->category == official::Category::armor && !card->element && card->ability == official::Ability::none && card->defense > 0)
            two_defenses.items[two_defenses.item_count++] = item.token;
    }
    check(two_defenses.item_count == 2, "public Flash fixture has two ordinary defenses");
    const auto flashed_view = observe(effect_match); result = status(effect_match); const auto flashed_status = result;
    check(gfo_match_legal(effect_match, &two_defenses, &legal) == 0 && !legal
        && gfo_match_step(effect_match, &two_defenses, &result) == -2 && equal(result, flashed_status)
        && observe(effect_match) == flashed_view, "Flash rejects two-card response without changing output/state");
    for (uint64_t seed{}; seed < 64; ++seed) {
        ++fixed_query_probes;
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0 && candidate.item_count <= 1
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "Flash sampler respects single-card response");
    }
    both_step(flash_pass);
    gfo_match_command cure{}; cure.item_count = 2; cure.items[0] = receipt_token(0, 237); cure.items[1] = receipt_token(0, 204); cure.has_target = 1;
    both_step(cure);
    check(observe(effect_match) == observe(effect_replay) && observe(effect_match).find("\"curses\":[\"flash\"]") == std::string::npos
        && observe(effect_match).find("\"hp\":37,\"mp\":0") != std::string::npos,
        "main miracle plus FREE cures Flash while preserving gift RNG after failed response and sampling");
    const auto held_cure = "\"token\":" + std::to_string(cure.items[0]) + ",\"face\":237,\"reference\":237,\"disguised\":false,\"used\":true";
    check(observe(effect_match).find(held_cure) != std::string::npos, "cure retains original miracle token");
    flash_attack.items[0] = receipt_token(1, 21); both_step(flash_attack);
    check(gfo_match_legal(effect_match, &two_defenses, &legal) == 0 && legal, "curing Flash restores multi-card response legality");
    both_step(two_defenses); check(observe(effect_match) == observe(effect_replay), "post-cure replay remains deterministic");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 1967907);
    check(gfo_match_create(&cfg, players.data(), 2, 1967907, &effect_match) == 0, "Flash counter create");
    flash_attack.actor = 0; flash_attack.target = 77; flash_attack.items[0] = receipt_token(0, 6);
    gfo_match_command flash_defense{}; flash_defense.actor = 77; flash_defense.kind = 1; flash_defense.item_count = 1; flash_defense.items[0] = receipt_token(1, 185);
    check(gfo_match_step(effect_match, &flash_attack, &result) == 0 && gfo_match_step(effect_match, &flash_defense, &result) == 0
        && observe(effect_match).find("\"models\":[185]") != std::string::npos && observe(effect_match).find("\"power\":0") != std::string::npos,
        "Flash counter is a nonattack response payload");
    flash_pass.item_count = 1; flash_pass.items[0] = receipt_token(0, 190);
    check(gfo_match_step(effect_match, &flash_pass, &result) == 0, "universal reflection accepts Flash counter");
    flash_pass.actor = 77; flash_pass.item_count = 0; flash_pass.items[0] = 0;
    check(gfo_match_step(effect_match, &flash_pass, &result) == 0
        && observe(effect_match, 1, 77).find("\"hp\":39,\"mp\":0,\"cp\":0,\"curses\":[\"flash\"]") != std::string::npos,
        "reflected Flash counter applies to current target without HP damage");
    gfo_match_destroy(effect_match);
    // DarkCloud is a supported curse; forced percentage hits remain public,
    // while all lifetime counters and future action state remain private.
    for (auto id : {41, 186, 223}) check(gfo_match_supported(id), "DarkCloud model membership");
    cfg.initial_hp = 40; cfg.initial_mp = 5; cfg.initial_cp = 0;
    fixture.reset(setup, 3230);
    check(gfo_match_create(&cfg, players.data(), 2, 3230, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 3230, &effect_replay) == 0, "forced chance public reset fixture create");
    gfo_match_command cloud_cast{}; cloud_cast.item_count = 1; cloud_cast.items[0] = receipt_token(0, 223); cloud_cast.has_target = 1; cloud_cast.target = 77;
    gfo_match_command cloud_pass{}; cloud_pass.actor = 77; cloud_pass.kind = 1;
    both_step(cloud_cast); both_step(cloud_pass);
    check(observe(effect_match).find("\"curses\":[\"darkcloud\"]") != std::string::npos, "foreign miracle adds DarkCloud");
    const auto yield_other = [&](uint64_t reserved) {
        for (const auto& item : fixture.snapshot().state.players[1].items) {
            if (item.token == reserved) continue;
            gfo_match_command discard{}; discard.actor = 77; discard.kind = 2; discard.item_count = 1; discard.items[0] = item.token;
            uint8_t allowed = 0;
            check(gfo_match_legal(effect_match, &discard, &allowed) == 0, "public discard legality");
            if (allowed) { both_step(discard); return; }
        }
        check(false, "public fixture needs an unreserved discard");
    };
    mirror_token = receipt_token(1, 190); yield_other(mirror_token);
    const auto pre_forced = observe(effect_match);
    for (uint64_t seed{}; seed < 64; ++seed) {
        ++fixed_query_probes;
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "DarkCloud context sampler stays legal");
    }
    check(observe(effect_match) == pre_forced, "DarkCloud policy queries do not mutate match");
    gfo_match_command forced_attack{}; forced_attack.item_count = 1; forced_attack.items[0] = receipt_token(0, 95);
    both_step(forced_attack);
    const auto forced_view = observe(effect_match, 0, 0);
    check(free_result.actor == 77 && forced_view.find("\"forced_hit\":true") != std::string::npos
        && forced_view.find("\"hit_rate\":75") != std::string::npos
        && forced_view.find("\"darkcloud_rule\":\"experimental:force-percent-hit-without-draw-at-target-activation;redirects-no-recheck;idempotent-until-all-cure;no-tick\"") != std::string::npos,
        "DarkCloud marks forced chance activation without changing original percentage");
    for (const auto* key : {"\"chance_forced_hits\":", "\"counter_forced_hits\":", "\"chance_hits\":", "\"counter_hits\":", "\"chance_rolls\":", "\"counter_rolls\":", "\"batch\":", "\"counters\":"})
        check(forced_view.find(key) == std::string::npos, "forced counters and future reservations remain private");
    gfo_match_command cloud_mirror{}; cloud_mirror.actor = 77; cloud_mirror.kind = 1; cloud_mirror.item_count = 1; cloud_mirror.items[0] = mirror_token;
    both_step(cloud_mirror);
    check(free_result.actor == 0 && observe(effect_match).find("\"forced_hit\":true") != std::string::npos,
        "forced-hit evidence survives redirect onto a non-DarkCloud target");
    cloud_pass.actor = 0; both_step(cloud_pass);
    check(observe(effect_match) == observe(effect_replay) && observe(effect_match).find("\"hp\":39,\"mp\":0") != std::string::npos,
        "forced-hit redirect resolves once and preserves private gift RNG under queries");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 11865);
    check(gfo_match_create(&cfg, players.data(), 2, 11865, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 11865, &effect_replay) == 0, "forced Mars fixture create");
    cloud_cast.items[0] = receipt_token(0, 223); cloud_cast.target = 0; both_step(cloud_cast);
    yield_other(receipt_token(1, 181));
    cloud_cast.items[0] = receipt_token(0, 6); cloud_cast.target = 77; both_step(cloud_cast);
    gfo_match_command cloud_counter{}; cloud_counter.actor = 77; cloud_counter.kind = 1; cloud_counter.item_count = 1; cloud_counter.items[0] = receipt_token(1, 181);
    both_step(cloud_counter);
    const auto forced_counter = observe(effect_match, 0, 0);
    check(forced_counter.find("\"models\":[181]") != std::string::npos && forced_counter.find("\"forced_hit\":true") != std::string::npos
        && forced_counter.find("\"hit_rate\":75") != std::string::npos && forced_counter.find("\"counter\":true") != std::string::npos,
        "Mars activation against DarkCloud is a forced counter hit");
    cloud_pass.actor = 0; both_step(cloud_pass);
    check(observe(effect_match) == observe(effect_replay), "forced counter future gift replay");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    cfg.initial_mp = 0;
    for (const auto reaction_model : {39, 55}) {
        fixture.reset(setup, 6138700);
        check(gfo_match_create(&cfg, players.data(), 2, 6138700, &effect_match) == 0, "FREE DarkCloud miracle fixture create");
        cloud_cast.items[0] = receipt_token(0, 223); cloud_cast.item_count = 2; cloud_cast.items[1] = receipt_token(0, 204); cloud_cast.target = 77;
        cloud_mirror.items[0] = receipt_token(1, static_cast<official::CardId>(reaction_model));
        check(gfo_match_step(effect_match, &cloud_cast, &result) == 0 && gfo_match_step(effect_match, &cloud_mirror, &result) == 0, "FREE DarkCloud accepts matching mirror or block");
        if (reaction_model == 39) check(gfo_match_step(effect_match, &cloud_pass, &result) == 0, "reflected DarkCloud resolves at current target");
        const auto cloud_body = observe(effect_match);
        check((cloud_body.find("\"curses\":[\"darkcloud\"]") != std::string::npos) == (reaction_model == 39),
            "block cancels curse; reflect returns it");
        check(cloud_body.find("\"token\":" + std::to_string(cloud_cast.items[0]) + ",\"face\":223,\"reference\":223,\"disguised\":false,\"used\":true") != std::string::npos
            && cloud_body.find("\"token\":" + std::to_string(cloud_cast.items[1]) + ",") == std::string::npos,
            "curse miracle retention and FREE consumption are not rolled back by reaction");
        gfo_match_destroy(effect_match);
    }
    fixture.reset(setup, 22345);
    check(gfo_match_create(&cfg, players.data(), 2, 22345, &effect_match) == 0, "DarkCloud counter fixture create");
    cloud_cast.item_count = 1; cloud_cast.items[1] = 0; cloud_cast.items[0] = receipt_token(0, 6);
    cloud_counter.items[0] = receipt_token(1, 186);
    check(gfo_match_step(effect_match, &cloud_cast, &result) == 0 && gfo_match_step(effect_match, &cloud_counter, &result) == 0
        && result.actor == 0 && observe(effect_match).find("\"models\":[186]") != std::string::npos
        && observe(effect_match).find("\"power\":0") != std::string::npos, "DarkCloud counter is a non-damage response payload");
    gfo_match_command forbidden_cloud_defense{}; forbidden_cloud_defense.kind = 1; forbidden_cloud_defense.item_count = 1; forbidden_cloud_defense.items[0] = receipt_token(0, 161);
    const auto before_cloud_error = observe(effect_match); const auto cloud_status = result;
    check(gfo_match_step(effect_match, &forbidden_cloud_defense, &result) == -2 && equal(result, cloud_status)
        && observe(effect_match) == before_cloud_error, "ordinary DEF cannot answer DarkCloud counter and failure is atomic");
    cloud_mirror.actor = 0; cloud_mirror.items[0] = receipt_token(0, 190);
    cloud_pass.actor = 77;
    check(gfo_match_step(effect_match, &cloud_mirror, &result) == 0 && gfo_match_step(effect_match, &cloud_pass, &result) == 0
        && observe(effect_match, 1, 77).find("\"hp\":39,\"mp\":0,\"cp\":0,\"curses\":[\"darkcloud\"]") != std::string::npos,
        "universal mirror returns DarkCloud counter to its generator");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 9); cfg.initial_hp = 5;
    check(gfo_match_create(&cfg, players.data(), 2, 9, &effect_match) == 0, "lethal DarkCloud weapon fixture");
    cloud_cast.items[0] = receipt_token(0, 41); cloud_pass.actor = 77;
    check(gfo_match_step(effect_match, &cloud_cast, &result) == 0 && gfo_match_step(effect_match, &cloud_pass, &result) == 0
        && result.terminated && result.winners[0] == 0
        && observe(effect_match, 1, 77).find("\"curses\":[\"darkcloud\"]") != std::string::npos,
        "positive lethal target HP loss also records DarkCloud");
    gfo_match_destroy(effect_match);
    // Resolved removals publish model order to all viewers, without private sampling state.
    for (auto id : {205, 206}) check(gfo_match_supported(id), "removal membership");
    const auto hand_size = [&](gfo_match* match, uint64_t player) {
        const auto view = observe(match, 1, player); std::size_t count = 0, at = 0;
        while ((at = view.find("\"token\":", at)) != std::string::npos) { ++count; ++at; }
        return count;
    };
    const auto has_token = [&](gfo_match* match, uint64_t player, uint64_t token) {
        return observe(match, 1, player).find("\"token\":" + std::to_string(token) + ",") != std::string::npos;
    };
    cfg.initial_hp = 40; cfg.initial_mp = 99; cfg.initial_hand = 18;
    fixture.reset(setup, 33);
    check(gfo_match_create(&cfg, players.data(), 2, 33, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 33, &effect_replay) == 0, "Broom public reset fixture create");
    gfo_match_command remove_command{}; remove_command.item_count = 1; remove_command.items[0] = receipt_token(0, 205); remove_command.has_target = 1; remove_command.target = 77;
    gfo_match_command remove_pass{}; remove_pass.kind = 1; remove_pass.actor = 77;
    both_step(remove_command);
    const auto removal_view = observe(effect_match, 0, 0);
    check(removal_view.find("\"removal_rule\":\"experimental:unused-hand-max3;used-miracles-max2;uniform-without-replacement-in-inventory-order;bounded-draw-per-removal;empty-no-draw;no-removal-gifts\"") != std::string::npos
        && removal_view.find("\"power\":0") != std::string::npos && removal_view.find("\"amount\":3") != std::string::npos, "public removal scope and derived amount");
    check(removal_view.find("\"last_removal\":null") != std::string::npos
        && removal_view.find(official::removal_event_rule) != std::string::npos, "unresolved removal has no result event and publishes the event contract");
    for (uint64_t seed{}; seed < 64; ++seed) {
        ++fixed_query_probes;
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "removal-context sampled actions legal");
    }
    both_step(remove_pass);
    check(hand_size(effect_match, 0) == 18 && hand_size(effect_match, 77) == 15
        && observe(effect_match, 1, 0) == observe(effect_replay, 1, 0)
        && observe(effect_match, 1, 77) == observe(effect_replay, 1, 77), "three unused removals with only user gift; query and policy RNG separate");
    std::size_t removed_count = 0;
    for (const auto& item : fixture.snapshot().state.players[1].items) if (!has_token(effect_match, 77, item.token)) ++removed_count;
    check(removed_count == 3, "without-replacement removes three distinct physical copies");
    for (const auto* key : {"\"removed\":", "\"removed_slots\":", "\"removal_rng\":", "\"batch\":"})
        check(observe(effect_match, 0, 0).find(key) == std::string::npos, "removal exposes no private slot, token selection or sampling state");
    for (const uint64_t viewer : {uint64_t{0}, uint64_t{77}})
        check(observe(effect_match, 1, viewer).find("\"last_removal\":{\"origin\":0,\"source\":0,\"target\":77,\"model\":205,\"removed_models\":[40,113,65]}") != std::string::npos, "Broom publicly preserves actual draw order");
    check(observe(effect_match, 0, 0).find("\"removed_models\":[40,113,65]") != std::string::npos, "spectator receives the same removed models");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 1143935);
    check(gfo_match_create(&cfg, players.data(), 2, 1143935, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 1143935, &effect_replay) == 0, "Soap three-retained fixture create");
    const std::array<uint64_t, 3> learned_tokens{receipt_token(0, 235), receipt_token(0, 237), receipt_token(0, 238)};
    gfo_match_command learn_command{}; learn_command.item_count = 1; learn_command.has_target = 1;
    for (const auto token : learned_tokens) {
        learn_command.items[0] = token; both_step(learn_command); yield_other(std::numeric_limits<uint64_t>::max());
    }
    remove_command.items[0] = receipt_token(0, 206); remove_command.target = 0;
    const auto soap_before = observe(effect_match);
    for (uint64_t seed{}; seed < 64; ++seed) {
        ++fixed_query_probes;
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "retained-removal context samples legal");
    }
    check(observe(effect_match) == soap_before, "Soap query leaves candidate inventory unchanged");
    both_step(remove_command);
    check(hand_size(effect_match, 0) == 16 && observe(effect_match) == observe(effect_replay), "two retained removals grant no automatic replacement");
    removed_count = 0; uint64_t removed_miracle = 0;
    for (const auto token : learned_tokens) if (!has_token(effect_match, 0, token)) { ++removed_count; removed_miracle = token; }
    check(removed_count == 2, "Soap stops at two out of three used miracles");
    yield_other(std::numeric_limits<uint64_t>::max());
    learn_command.items[0] = removed_miracle;
    const auto soap_after = observe(effect_match); result = status(effect_match); const auto soap_status = result;
    check(gfo_match_legal(effect_match, &learn_command, &legal) == 0 && !legal
        && gfo_match_step(effect_match, &learn_command, &result) == -2 && equal(result, soap_status)
        && observe(effect_match) == soap_after, "deleted retained token cannot be cast and failure is atomic");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 23945);
    check(gfo_match_create(&cfg, players.data(), 2, 23945, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 23945, &effect_replay) == 0, "Soap duplicate-model copies fixture create");
    std::vector<uint64_t> duplicate_springs;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 235) duplicate_springs.push_back(item.token);
    check(duplicate_springs.size() == 2 && duplicate_springs[0] != duplicate_springs[1], "two physical copies share a model but not a token");
    for (const auto token : duplicate_springs) {
        learn_command.items[0] = token; both_step(learn_command); yield_other(std::numeric_limits<uint64_t>::max());
    }
    remove_command.items[0] = receipt_token(0, 206); both_step(remove_command);
    check(hand_size(effect_match, 0) == 16 && !has_token(effect_match, 0, duplicate_springs[0]) && !has_token(effect_match, 0, duplicate_springs[1])
        && observe(effect_match).find("\"used\":true") == std::string::npos, "Soap removes copies rather than only one model type");
    check(observe(effect_match, 0, 0).find("\"last_removal\":{\"origin\":0,\"source\":0,\"target\":0,\"model\":206,\"removed_models\":[235,235]}") != std::string::npos, "Soap public result retains duplicate model copies");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 115843);
    check(gfo_match_create(&cfg, players.data(), 2, 115843, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 115843, &effect_replay) == 0, "reflected Soap fixture create");
    learn_command.items[0] = receipt_token(0, 235); both_step(learn_command);
    const auto soap_mirror = receipt_token(1, 190); yield_other(soap_mirror);
    remove_command.items[0] = receipt_token(0, 206); remove_command.target = 77; both_step(remove_command);
    gfo_match_command removal_mirror{}; removal_mirror.actor = 77; removal_mirror.kind = 1; removal_mirror.item_count = 1; removal_mirror.items[0] = soap_mirror;
    both_step(removal_mirror); remove_pass.actor = 0; both_step(remove_pass);
    check(!has_token(effect_match, 0, learn_command.items[0]) && hand_size(effect_match, 0) == 17 && hand_size(effect_match, 77) == 17
        && observe(effect_match) == observe(effect_replay), "universal reflection changes whose retained copy is removed");
    check(observe(effect_match, 0, 0).find("\"last_removal\":{\"origin\":0,\"source\":77,\"target\":0,\"model\":206,\"removed_models\":[235]}") != std::string::npos, "reflected result distinguishes immutable origin and current source");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    cfg.initial_hand = 1; cfg.initial_mp = 0;
    // With no retained copies, Soap ignores the recipient's unused miracle/carrier.
    for (const uint64_t target : {uint64_t{0}, uint64_t{77}}) {
        check(gfo_match_create(&cfg, players.data(), 2, 61757, &effect_match) == 0, "empty Soap fixture create");
        remove_command.items[0] = 0; remove_command.target = target;
        check(gfo_match_step(effect_match, &remove_command, &result) == 0, "empty Soap accepted");
        if (target == 77) { remove_pass.actor = 77; check(gfo_match_step(effect_match, &remove_pass, &result) == 0, "empty foreign Soap passes"); }
        check(hand_size(effect_match, 0) == 1 && hand_size(effect_match, 77) == 1
            && observe(effect_match).find("\"used\":true") == std::string::npos, "empty removal grants only physical use gift and no retained miracle");
        gfo_match_destroy(effect_match);
    }
    check(gfo_match_create(&cfg, players.data(), 2, 101314, &effect_match) == 0, "sole unused miracle Broom fixture create");
    remove_command.target = 77; remove_pass.actor = 77;
    check(observe(effect_match, 1, 77).find("\"face\":224") != std::string::npos
        && gfo_match_step(effect_match, &remove_command, &result) == 0 && gfo_match_step(effect_match, &remove_pass, &result) == 0
        && hand_size(effect_match, 0) == 1 && hand_size(effect_match, 77) == 0, "Broom removes sole unused miracle without learning or recipient compensation");
    gfo_match_destroy(effect_match);
    // MP-derived power is computed once from remaining MP, then redirected intact.
    for (auto id : {14, 58}) check(gfo_match_supported(id), "repeat and mana weapon membership");
    cfg.initial_hand = 18; cfg.initial_hp = 40; cfg.initial_mp = 0;
    fixture.reset(setup, 62);
    check(gfo_match_create(&cfg, players.data(), 2, 62, &effect_match) == 0, "zero-MP mana weapon create");
    gfo_match_command mana_attack{}; mana_attack.item_count = 1; mana_attack.has_target = 1; mana_attack.target = 77; mana_attack.items[0] = receipt_token(0, 58);
    gfo_match_command new_pass{}; new_pass.actor = 77; new_pass.kind = 1;
    check(gfo_match_step(effect_match, &mana_attack, &result) == 0 && result.phase == 1 && result.actor == 77
        && observe(effect_match).find("\"power\":0,") != std::string::npos
        && observe(effect_match).find("\"mana_weapon_rule\":\"experimental:base-2x-mp-after-composed-miracle-cost;ordered-boosts;consume-all-mp-once-at-launch;redirects-no-repayment\"") != std::string::npos,
        "MP0 remains a zero-power weapon response");
    check(gfo_match_step(effect_match, &new_pass, &result) == 0 && result.phase == 0, "zero-power weapon resolves");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 128946); cfg.initial_hp = 99; cfg.initial_mp = 10;
    const std::vector<std::pair<std::vector<official::CardId>, int>> mana_columns{
        {{58, 231}, 16}, {{58, 210}, 18}, {{58, 210, 231}, 12}, {{58, 231, 210}, 10}};
    for (const auto& [models, power] : mana_columns) {
        check(gfo_match_create(&cfg, players.data(), 2, 128946, &effect_match) == 0, "paid mana column create");
        mana_attack.item_count = static_cast<uint8_t>(models.size()); std::fill_n(mana_attack.items, 18, 0);
        for (std::size_t i = 0; i < models.size(); ++i) mana_attack.items[i] = receipt_token(0, models[i]);
        check(gfo_match_step(effect_match, &mana_attack, &result) == 0
            && observe(effect_match).find("\"power\":" + std::to_string(power) + ",") != std::string::npos
            && observe(effect_match).find("\"hp\":99,\"mp\":0") != std::string::npos, "completed miracle cost precedes dynamic base and ordered Aura");
        check(gfo_match_step(effect_match, &new_pass, &result) == 0 && hand_size(effect_match, 0) == 18, "paid mana column refills once");
        gfo_match_destroy(effect_match);
    }
    fixture.reset(setup, 1908520);
    const std::vector<std::pair<std::vector<official::CardId>, int>> free_columns{
        {{58, 12}, 2}, {{58, 12, 231, 204}, 44}, {{58, 231, 204, 12}, 42}};
    for (std::size_t column = 0; column < free_columns.size(); ++column) {
        cfg.initial_mp = column == 0 ? 0 : 10;
        check(gfo_match_create(&cfg, players.data(), 2, 1908520, &effect_match) == 0, "FREE mana column create");
        const auto& [models, power] = free_columns[column];
        mana_attack.item_count = static_cast<uint8_t>(models.size()); std::fill_n(mana_attack.items, 18, 0);
        for (std::size_t i = 0; i < models.size(); ++i) mana_attack.items[i] = receipt_token(0, models[i]);
        check(gfo_match_step(effect_match, &mana_attack, &result) == 0
            && observe(effect_match).find("\"power\":" + std::to_string(power) + ",") != std::string::npos, "zero-MP boost and FREE Aura preserve ordered power");
        check(gfo_match_step(effect_match, &new_pass, &result) == 0 && hand_size(effect_match, 0) == 18
            && !has_token(effect_match, 0, receipt_token(0, 58)), "mana weapon physical consumption and bounded refill");
        gfo_match_destroy(effect_match);
    }
    fixture.reset(setup, 69); cfg.initial_mp = 10; cfg.initial_hp = 40;
    check(gfo_match_create(&cfg, players.data(), 2, 69, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 69, &effect_replay) == 0, "mana redirect create");
    mana_attack.item_count = 1; std::fill_n(mana_attack.items, 18, 0); mana_attack.items[0] = receipt_token(0, 58);
    const auto before_mana_sample = observe(effect_match);
    for (uint64_t seed{}; seed < 64; ++seed) {
        ++fixed_query_probes;
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0 && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "mana context samples legal");
    }
    check(observe(effect_match) == before_mana_sample, "mana policy queries do not burn MP");
    both_step(mana_attack);
    gfo_match_command mana_mirror{}; mana_mirror.actor = 77; mana_mirror.kind = 1; mana_mirror.item_count = 1; mana_mirror.items[0] = receipt_token(1, 38); both_step(mana_mirror);
    check(observe(effect_match).find("\"origin\":0,\"source\":77,\"target\":0") != std::string::npos
        && observe(effect_match).find("\"power\":20,") != std::string::npos
        && observe(effect_match).find("\"hp\":40,\"mp\":10") != std::string::npos, "redirect carries immutable power without burning defender MP");
    new_pass.actor = 0; both_step(new_pass);
    check(observe(effect_match) == observe(effect_replay) && observe(effect_match).find("\"hp\":20,\"mp\":0") != std::string::npos,
        "mana redirects resolve once and policy queries preserve future gift RNG");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    // Two initial frames share a single use/payment, with counters ahead of hit2.
    fixture.reset(setup, 813); cfg.initial_mp = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 813, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 813, &effect_replay) == 0, "repeat/counter create");
    gfo_match_command repeat{}; repeat.item_count = 1; repeat.items[0] = receipt_token(0, 14); repeat.has_target = 1; repeat.target = 77;
    gfo_match_command repeat_counter{}; repeat_counter.actor = 77; repeat_counter.kind = 1; repeat_counter.item_count = 1; repeat_counter.items[0] = receipt_token(1, 184);
    both_step(repeat); both_step(repeat_counter);
    check(free_result.actor == 0 && observe(effect_match).find("\"counter\":true") != std::string::npos
        && observe(effect_match).find("\"power\":6,") != std::string::npos
        && observe(effect_match).find("\"repeat_attack_rule\":\"experimental:two-fresh-passes-over-initial-targets;single-consumption-and-payment;counter-fifo-before-next-hit;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action\"") != std::string::npos,
        "first repeat hit queues counter before second original payload");
    new_pass.actor = 0; both_step(new_pass);
    const auto second_repeat = observe(effect_match);
    check(free_result.actor == 77 && free_result.owner == 0 && second_repeat.find("\"models\":[14]") != std::string::npos
        && second_repeat.find("\"counter\":false") != std::string::npos && hand_size(effect_match, 0) == 17, "second payload opens before any deferred gift");
    new_pass.actor = 77; both_step(new_pass);
    check(free_result.decisions == 4 && free_result.actor == 77 && free_result.owner == 77
        && hand_size(effect_match, 0) == 18 && hand_size(effect_match, 77) == 18
        && observe(effect_match) == observe(effect_replay), "whole repeat action finishes and gifts only once");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 2553); cfg.initial_mp = 10;
    check(gfo_match_create(&cfg, players.data(), 2, 2553, &effect_match) == 0, "paid repeat create");
    repeat.item_count = 2; repeat.items[0] = receipt_token(0, 14); repeat.items[1] = receipt_token(0, 210);
    check(gfo_match_step(effect_match, &repeat, &result) == 0, "paid repeat launch");
    for (int hit = 0; hit < 2; ++hit) {
        check(observe(effect_match).find("\"power\":5,") != std::string::npos
            && observe(effect_match).find("\"hp\":40,\"mp\":8") != std::string::npos
            && gfo_match_step(effect_match, &new_pass, &result) == 0, "repeat charges additional miracle only at first launch");
    }
    check(result.decisions == 3 && hand_size(effect_match, 0) == 18 && has_token(effect_match, 0, repeat.items[1])
        && observe(effect_match).find("\"hp\":30,\"mp\":10") != std::string::npos, "two equal hits retain miracle token once");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 4997); cfg.initial_mp = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 4997, &effect_match) == 0, "repeat fresh-element create");
    repeat.items[0] = receipt_token(0, 14); repeat.items[1] = receipt_token(0, 66);
    repeat_counter.items[0] = receipt_token(1, 189);
    check(gfo_match_step(effect_match, &repeat, &result) == 0 && gfo_match_step(effect_match, &repeat_counter, &result) == 0
        && observe(effect_match).find("\"element\":\"fire\"") != std::string::npos
        && observe(effect_match).find("\"power\":5,") != std::string::npos,
        "second payload restores original element after first filter");
    check(gfo_match_step(effect_match, &new_pass, &result) == 0, "fresh second payload resolves");
    gfo_match_destroy(effect_match);
    for (const bool redirect_first : {false, true}) {
        const uint64_t seed = redirect_first ? 813 : 16; fixture.reset(setup, seed); cfg.initial_hp = 3;
        check(gfo_match_create(&cfg, players.data(), 2, seed, &effect_match) == 0, "repeat natural ending create");
        repeat.item_count = 1; repeat.items[0] = receipt_token(0, 14); repeat.items[1] = 0;
        check(gfo_match_step(effect_match, &repeat, &result) == 0, "repeat ending launch");
        if (redirect_first) {
            mana_mirror.items[0] = receipt_token(1, 38);
            check(gfo_match_step(effect_match, &mana_mirror, &result) == 0, "first repeat reflected toward original owner");
        }
        new_pass.actor = redirect_first ? 0 : 77;
        check(gfo_match_step(effect_match, &new_pass, &result) == 0 && result.terminated && result.winner_count == 1
            && result.winners[0] == (redirect_first ? 77 : 0) && result.decisions == (redirect_first ? 3 : 2),
            "natural death cancels second payload before another response");
        gfo_match_destroy(effect_match);
    }
    // Mirage keeps ABI1's optional target representation; a live hint is ignored.
    check(gfo_match_supported(232), "Mirage membership"); cfg.initial_hand = 18; cfg.initial_hp = 40; cfg.initial_mp = 5;
    fixture.reset(chance_setup, 15216);
    std::vector<std::string> wide_baseline;
    for (int target_hint = -1; target_hint < 4; ++target_hint) {
        check(gfo_match_create(&cfg, chance_players.data(), 4, 15216, &effect_match) == 0, "four-seat wide create");
        gfo_match_command wide{}; wide.item_count = 2; wide.items[0] = receipt_token(0, 6); wide.items[1] = receipt_token(0, 232);
        if (target_hint >= 0) { wide.has_target = 1; wide.target = chance_players[static_cast<std::size_t>(target_hint)].id; }
        auto invalid_wide = wide; invalid_wide.has_target = 1; invalid_wide.target = 9999; result = status(effect_match); const auto pre_wide_status = result;
        const auto pre_wide = observe(effect_match);
        check(gfo_match_legal(effect_match, &invalid_wide, &legal) == 0 && !legal
            && gfo_match_step(effect_match, &invalid_wide, &result) == -2 && equal(result, pre_wide_status)
            && observe(effect_match) == pre_wide, "unknown Wide hint rejected atomically");
        check(gfo_match_step(effect_match, &wide, &result) == 0, "Wide accepts absent/self/ally/enemy live hint");
        std::vector<std::string> trace{observe(effect_match)};
        check(trace.front().find("\"wide_attack_rule\":\"experimental:initial-alive-enemies-in-input-order;optional-alive-target-does-not-alter-roster;repeat-by-full-roster-pass;single-payment;counter-fifo-before-next-target;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action\"") != std::string::npos,
            "Wide public experimental contract");
        gfo_match_command wide_pass{}; wide_pass.kind = 1;
        for (const uint64_t actor : {uint64_t{88}, uint64_t{99}}) {
            check(result.phase == 1 && result.actor == actor && result.owner == 0 && hand_size(effect_match, 0) == 17
                && observe(effect_match).find("\"hit_rate\":0") != std::string::npos
                && observe(effect_match).find("\"forced_hit\":false") != std::string::npos, "guaranteed Wide follows initial enemy roster without chance draw");
            wide_pass.actor = actor; check(gfo_match_step(effect_match, &wide_pass, &result) == 0, "Wide target responds"); trace.push_back(observe(effect_match));
        }
        check(result.decisions == 3 && result.actor == 77 && result.owner == 77 && hand_size(effect_match, 0) == 18,
            "all Wide targets precede deferred user gift and next owner");
        if (wide_baseline.empty()) wide_baseline = trace;
        else check(trace == wide_baseline, "all live target hints produce identical roster, state and gift stream");
        gfo_match_destroy(effect_match);
    }
    fixture.reset(setup, 15216); cfg.initial_mp = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 15216, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 15216, &effect_replay) == 0, "zero-MP Mirage/FREE create");
    const auto wide_token = receipt_token(0, 232); const auto wide_free = receipt_token(0, 204);
    const auto before_wide_sample = observe(effect_match); bool sampled_wide_free = false;
    for (uint64_t seed{}; seed < 4096 && !sampled_wide_free; ++seed) {
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "zero-MP Wide samples legal");
        bool has_wide = false, has_free = false;
        for (std::size_t i = 0; i < candidate.item_count; ++i) { has_wide |= candidate.items[i] == wide_token; has_free |= candidate.items[i] == wide_free; }
        if (has_wide) check(!candidate.has_target && candidate.target == 0, "sampler clears target after appending Mirage");
        sampled_wide_free = has_wide && has_free;
    }
    check(sampled_wide_free && observe(effect_match) == before_wide_sample, "sampler atomically appends Mirage/FREE without changing match");
    gfo_match_command wide_free_cast{}; wide_free_cast.item_count = 3; wide_free_cast.items[0] = receipt_token(0, 6); wide_free_cast.items[1] = wide_token; wide_free_cast.items[2] = wide_free;
    both_step(wide_free_cast); new_pass.actor = 77; both_step(new_pass);
    check(observe(effect_match) == observe(effect_replay) && hand_size(effect_match, 0) == 18
        && has_token(effect_match, 0, wide_token) && !has_token(effect_match, 0, wide_free), "FREE Wide retention, consumption and future gift RNG");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(chance_setup, 3307); cfg.initial_mp = 5;
    check(gfo_match_create(&cfg, chance_players.data(), 4, 3307, &effect_match) == 0, "repeat Wide create");
    gfo_match_command wide{}; wide.item_count = 2; wide.items[0] = receipt_token(0, 14); wide.items[1] = receipt_token(0, 232);
    check(gfo_match_step(effect_match, &wide, &result) == 0, "repeat Wide launch");
    for (const uint64_t actor : {uint64_t{88}, uint64_t{99}, uint64_t{88}, uint64_t{99}}) {
        check(result.actor == actor && result.owner == 0 && hand_size(effect_match, 0) == 17, "repeat traverses two full roster passes before gifting");
        new_pass.actor = actor; check(gfo_match_step(effect_match, &new_pass, &result) == 0, "repeat Wide response");
    }
    check(result.decisions == 5 && result.actor == 77 && hand_size(effect_match, 0) == 18, "repeat Wide finishes with one launch payment/consumption");
    gfo_match_destroy(effect_match);
    fixture.reset(chance_setup, 149293);
    check(gfo_match_create(&cfg, chance_players.data(), 4, 149293, &effect_match) == 0, "Wide counter-order create");
    wide.items[0] = receipt_token(0, 6); wide.items[1] = receipt_token(0, 232);
    gfo_match_command wide_counter{}; wide_counter.actor = 88; wide_counter.kind = 1; wide_counter.item_count = 1; wide_counter.items[0] = receipt_token(2, 184);
    check(gfo_match_step(effect_match, &wide, &result) == 0 && gfo_match_step(effect_match, &wide_counter, &result) == 0
        && result.actor == 0 && observe(effect_match).find("\"counter\":true") != std::string::npos
        && observe(effect_match).find("\"power\":2,") != std::string::npos, "counter FIFO starts before next Wide enemy");
    new_pass.actor = 0;
    check(gfo_match_step(effect_match, &new_pass, &result) == 0 && result.actor == 99, "next enemy follows resolved counter");
    new_pass.actor = 99; check(gfo_match_step(effect_match, &new_pass, &result) == 0 && result.actor == 77, "Wide counter batch finishes");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 69); cfg.initial_mp = 10;
    check(gfo_match_create(&cfg, players.data(), 2, 69, &effect_match) == 0, "mana Wide create");
    wide.items[0] = receipt_token(0, 58); wide.items[1] = receipt_token(0, 232);
    check(gfo_match_step(effect_match, &wide, &result) == 0
        && observe(effect_match).find("\"power\":10,") != std::string::npos
        && observe(effect_match).find("\"hp\":40,\"mp\":0") != std::string::npos, "Wide cost5 precedes MP-derived base10");
    new_pass.actor = 77; check(gfo_match_step(effect_match, &new_pass, &result) == 0, "mana Wide resolves");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 170013);
    check(gfo_match_create(&cfg, players.data(), 2, 170013, &effect_match) == 0, "two Mirage copies create");
    wide.item_count = 1; wide.items[0] = receipt_token(0, 6); std::vector<uint64_t> wide_copies;
    for (const auto& item : fixture.snapshot().state.players[0].items) if (item.model == 232) { wide.items[wide.item_count++] = item.token; wide_copies.push_back(item.token); }
    check(wide_copies.size() == 2 && gfo_match_step(effect_match, &wide, &result) == 0
        && observe(effect_match).find("\"hp\":40,\"mp\":0") != std::string::npos, "both Mirage copies pay5 and become retained");
    check(gfo_match_step(effect_match, &new_pass, &result) == 0 && result.decisions == 2
        && has_token(effect_match, 0, wide_copies[0]) && has_token(effect_match, 0, wide_copies[1]), "multiple Mirage copies expand roster only once");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 3857); cfg.initial_mp = 5;
    check(gfo_match_create(&cfg, players.data(), 2, 3857, &effect_match) == 0, "invalid Mirage compositions create");
    wide.item_count = 1; wide.items[0] = receipt_token(0, 232); wide.items[1] = 0; wide.items[2] = 0;
    check(gfo_match_legal(effect_match, &wide, &legal) == 0 && !legal, "Mirage has no standalone main cast");
    wide.item_count = 2; wide.items[0] = receipt_token(0, 95); wide.items[1] = receipt_token(0, 232);
    check(gfo_match_legal(effect_match, &wide, &legal) == 0 && !legal, "percentage weapon cannot append Mirage");
    gfo_match_destroy(effect_match);
    for (const bool setter_after : {false, true}) {
        fixture.reset(setup, 54753);
        check(gfo_match_create(&cfg, players.data(), 2, 54753, &effect_match) == 0, "ordered Mirage element create");
        wide.item_count = 3; wide.items[0] = receipt_token(0, 6); wide.items[1] = receipt_token(0, setter_after ? 232 : 66); wide.items[2] = receipt_token(0, setter_after ? 66 : 232);
        check(gfo_match_step(effect_match, &wide, &result) == 0
            && observe(effect_match).find(setter_after ? "\"element\":\"fire\"" : "\"element\":null") != std::string::npos,
            "Mirage neutralizes prior element while later setter restores its own");
        check(gfo_match_step(effect_match, &new_pass, &result) == 0, "ordered Wide element resolves"); gfo_match_destroy(effect_match);
    }
    //202 fixes a signed nonattack payload at launch; the boundary never rerolls it.
    check(gfo_match_supported(202), "random-HP sundry membership"); cfg.initial_hand = 18; cfg.initial_hp = 40; cfg.initial_mp = 0; cfg.max_decisions = 512;
    gfo_match_command random_hp{}; random_hp.item_count = 1; random_hp.has_target = 1;
    gfo_match_command random_pass{}; random_pass.actor = 77; random_pass.kind = 1;
    for (const uint64_t seed : {uint64_t{54}, uint64_t{34}}) for (const uint64_t target : {uint64_t{0}, uint64_t{77}}) {
        fixture.reset(setup, seed); random_hp.items[0] = receipt_token(0, 202); random_hp.target = target;
        check(gfo_match_create(&cfg, players.data(), 2, seed, &effect_match) == 0 && gfo_match_step(effect_match, &random_hp, &result) == 0, "random HP self/foreign use");
        const int delta = seed == 54 ? 10 : -10;
        if (target == 77) {
            const auto pending_random = observe(effect_match, 0, 0);
            check(result.phase == 1 && pending_random.find("\"amount\":" + std::to_string(delta) + "}") != std::string::npos
                && pending_random.find("\"power\":0,") != std::string::npos, "launch publishes fixed positive or negative zero-power amount");
            check(gfo_match_step(effect_match, &random_pass, &result) == 0, "random HP response resolves");
        }
        const auto resolved_random = observe(effect_match);
        check(result.phase == 0 && result.actor == 77 && hand_size(effect_match, 0) == 18 && !has_token(effect_match, 0, random_hp.items[0])
            && resolved_random.find("\"hp\":" + std::to_string(40 + delta) + ",\"mp\":0") != std::string::npos
            && resolved_random.find("\"random_hp_rule\":\"experimental:launch-once;uniform-plus10-or-minus10;zero-heals-one-damages;fixed-through-reflections;nonattack-no-defense-or-counters;hp-clips-0-99;gifts-after-action\"") != std::string::npos,
            "signed HP use consumes physical sundry once and grants only normal use gift");
        gfo_match_destroy(effect_match);
    }
    fixture.reset(setup, 54); cfg.initial_hp = 99; random_hp.items[0] = receipt_token(0, 202); random_hp.target = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 54, &effect_match) == 0 && gfo_match_step(effect_match, &random_hp, &result) == 0
        && result.decisions == 1 && hand_size(effect_match, 0) == 18 && !has_token(effect_match, 0, random_hp.items[0])
        && observe(effect_match).find("\"hp\":99,\"mp\":0") != std::string::npos, "HP99 positive no-op still consumes, draws sign and advances action");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 34); cfg.initial_hp = 40; cfg.max_decisions = 1; random_hp.items[0] = receipt_token(0, 202); random_hp.target = 77;
    check(gfo_match_create(&cfg, players.data(), 2, 34, &effect_match) == 0 && gfo_match_step(effect_match, &random_hp, &result) == 0
        && result.truncated && !result.terminated && result.rewards[0] == 0 && result.rewards[1] == 0 && hand_size(effect_match, 0) == 17
        && observe(effect_match).find("\"pending\":null") != std::string::npos, "cutoff drops pending signed body and deferred gift");
    gfo_match_destroy(effect_match); cfg.initial_hp = 5; random_hp.target = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 34, &effect_match) == 0 && gfo_match_step(effect_match, &random_hp, &result) == 0
        && result.terminated && !result.truncated && result.winners[0] == 77, "self damage natural death precedes one-decision cutoff");
    gfo_match_destroy(effect_match);
    fixture.reset(setup, 709465); cfg.initial_hp = 40; cfg.max_decisions = 512;
    check(gfo_match_create(&cfg, players.data(), 2, 709465, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 709465, &effect_replay) == 0, "random HP two-reflection fixture");
    random_hp.items[0] = receipt_token(0, 202); random_hp.target = 77;
    auto invalid_random = random_hp; invalid_random.item_count = 2; invalid_random.items[1] = receipt_token(0, 190);
    const auto before_random = observe(effect_match); result = status(effect_match); const auto before_random_status = result;
    check(gfo_match_legal(effect_match, &invalid_random, &legal) == 0 && !legal
        && gfo_match_step(effect_match, &invalid_random, &result) == -2 && equal(result, before_random_status)
        && observe(effect_match) == before_random, "invalid random-HP composition does not draw or mutate outputs");
    for (uint64_t seed{}; seed < 64; ++seed) {
        ++fixed_query_probes;
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0 && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "random-HP policy samples legal without game draws");
    }
    both_step(random_hp);
    const auto signed_view = observe(effect_match, 0, 0);
    const int fixed_delta = signed_view.find("\"amount\":10}") != std::string::npos ? 10 : -10;
    check(signed_view.find("\"amount\":" + std::to_string(fixed_delta) + "}") != std::string::npos, "random HP sign is publicly fixed");
    gfo_match_command random_defense{}; random_defense.actor = 77; random_defense.kind = 1; random_defense.item_count = 1;
    for (const auto model : {118, 184}) {
        random_defense.items[0] = receipt_token(1, static_cast<official::CardId>(model)); result = free_result;
        check(gfo_match_legal(effect_match, &random_defense, &legal) == 0 && !legal
            && gfo_match_step(effect_match, &random_defense, &result) == -2 && equal(result, free_result)
            && observe(effect_match, 0, 0) == signed_view, "ordinary DEF and counter reject signed nonattack without mutation");
    }
    for (const auto seat : {std::size_t{1}, std::size_t{0}}) {
        random_defense.actor = players[seat].id; random_defense.items[0] = receipt_token(seat, 190); both_step(random_defense);
        check(observe(effect_match).find("\"amount\":" + std::to_string(fixed_delta) + "}") != std::string::npos
            && observe(effect_match).find("\"power\":0,") != std::string::npos, "universal reflection never rerolls random HP");
    }
    check(observe(effect_match).find("\"reflections\":2") != std::string::npos, "both redirects retain one signed payload");
    both_step(random_pass);
    check(observe(effect_match, 1, 0) == observe(effect_replay, 1, 0) && observe(effect_match, 1, 77) == observe(effect_replay, 1, 77)
        && observe(effect_match).find("\"hp\":" + std::to_string(40 + fixed_delta) + ",\"mp\":0") != std::string::npos, "queries and rejected defenses preserve fixed sign and future gift RNG");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    fixture.reset(setup, 937);
    check(gfo_match_create(&cfg, players.data(), 2, 937, &effect_match) == 0
        && gfo_match_create(&cfg, players.data(), 2, 937, &effect_replay) == 0, "discard202 no-draw control create");
    gfo_match_command discard_random{}; discard_random.kind = 2; discard_random.item_count = 1; discard_random.items[0] = receipt_token(0, 202);
    auto discard_control = discard_random; discard_control.items[0] = receipt_token(0, 118);
    check(gfo_match_step(effect_match, &discard_random, &free_result) == 0 && gfo_match_step(effect_replay, &discard_control, &replay_result) == 0
        && equal(free_result, replay_result), "202 and ordinary discard have identical transition status");
    gfo_match_command future_draw{}; future_draw.actor = 77; future_draw.kind = 0; future_draw.has_target = 1; future_draw.target = 77; future_draw.item_count = 1; future_draw.items[0] = receipt_token(1, 191);
    both_step(future_draw);
    check(observe(effect_match, 1, 77) == observe(effect_replay, 1, 77), "discard202 consumes no random sign draw before next player's gift");
    gfo_match_destroy(effect_match); gfo_match_destroy(effect_replay);
    // Passive revival crosses the same opaque boundary as ordinary commands.
    cfg.initial_hand = 18; cfg.hand_limit = 18; cfg.initial_mp = 0; cfg.max_decisions = 512;
    for (const auto trial : {std::array<uint64_t,3>{1288,57,5}, std::array<uint64_t,3>{380,64,40}, std::array<uint64_t,3>{1205,202,5}}) {
        fixture.reset(setup, trial[0]); cfg.initial_hp = static_cast<int>(trial[2]);
        const auto amulet_token = receipt_token(1, 208);
        check(gfo_match_create(&cfg, players.data(), 2, trial[0], &effect_match) == 0, "revival body fixture create");
        gfo_match_command revive_attack{}; revive_attack.item_count = 1; revive_attack.items[0] = receipt_token(0, static_cast<official::CardId>(trial[1]));
        revive_attack.has_target = 1; revive_attack.target = 77;
        check(gfo_match_step(effect_match, &revive_attack, &result) == 0 && result.actor == 77, "revival body waits for response");
        auto passive_defense = random_pass; passive_defense.item_count = 1; passive_defense.items[0] = amulet_token;
        const auto passive_before = observe(effect_match); const auto passive_status = result;
        check(gfo_match_legal(effect_match, &passive_defense, &legal) == 0 && !legal
            && gfo_match_step(effect_match, &passive_defense, &result) == -2 && equal(result, passive_status)
            && observe(effect_match) == passive_before, "passive amulet cannot be selected as defense");
        check(gfo_match_step(effect_match, &random_pass, &result) == 0 && !result.terminated && result.phase == 0 && result.actor == 77
            && observe(effect_match).find("\"hp\":10,\"mp\":0") != std::string::npos
            && hand_size(effect_match,77) == 18 && !has_token(effect_match,77,amulet_token)
            && observe(effect_match).find(official::revival_rule) != std::string::npos
            && observe(effect_match).find("\"revivals\"") == std::string::npos,
            "normal, Dark and negative HP revive at10, defer replacement and hide native counter");
        gfo_match_destroy(effect_match);
    }
    cfg.initial_hand = cfg.hand_limit = 1; cfg.initial_hp = 40;
    check(gfo_match_create(&cfg, players.data(), 2, 29, &effect_match) == 0
        && observe(effect_match).find("\"face\":208") != std::string::npos, "single full passive hand fixture");
    const auto passive_hand = observe(effect_match); const auto passive_status = status(effect_match);
    gfo_match_command passive_use{}; passive_use.item_count = 1; passive_use.items[0] = 0; passive_use.has_target = 1;
    check(gfo_match_legal(effect_match, &passive_use, &legal) == 0 && !legal, "passive amulet cannot be used manually");
    passive_use.kind = 2; passive_use.has_target = 0;
    check(gfo_match_step(effect_match, &passive_use, &result) == -2 && observe(effect_match) == passive_hand, "passive amulet discard is forbidden");
    gfo_match_command full_prayer{};
    for (uint64_t seed{}; seed < 32; ++seed) {
        uint64_t policy = seed;
        check(gfo_match_sample_action(effect_match, &policy, &full_prayer) == 0 && full_prayer.kind == 3
            && !full_prayer.item_count && !full_prayer.has_target, "full passive hand samples prayer rather than a deadlock");
    }
    check(equal(status(effect_match),passive_status) && gfo_match_step(effect_match, &full_prayer, &result) == 0
        && result.actor == 77 && hand_size(effect_match,0) == 1 && has_token(effect_match,0,0), "full prayer keeps passive hand and advances the turn");
    gfo_match_destroy(effect_match);
    cfg.initial_hand = cfg.hand_limit = 18; cfg.initial_hp = 99; cfg.initial_mp = 0; cfg.max_decisions = 512;
    fixture.reset(setup, 10783);
    check(gfo_match_create(&cfg, players.data(), 2, 10783, &effect_match) == 0, "public danger reflection fixture");
    gfo_match_command danger_attack{}; danger_attack.item_count = 1; danger_attack.items[0] = receipt_token(0,94);
    check(gfo_match_step(effect_match,&danger_attack,&result) == 0 && result.actor == 77
        && observe(effect_match).find("\"last_danger\":{\"origin\":0,\"target\":77,\"models\":[94],\"mortar\":false,\"power\":30,\"element\":\"light\"}") != std::string::npos,
        "targetless danger publishes actual target and payload, without hidden holder list");
    gfo_match_command danger_reflect{}; danger_reflect.actor = 77; danger_reflect.kind = 1; danger_reflect.item_count = 2;
    danger_reflect.items[0] = receipt_token(1,189); danger_reflect.items[1] = receipt_token(1,38);
    check(gfo_match_step(effect_match,&danger_reflect,&result) == 0 && result.actor == 0
        && observe(effect_match).find("\"element\":null") != std::string::npos, "filtered danger reflects without reselection");
    gfo_match_command danger_pass{}; danger_pass.kind = 1;
    check(gfo_match_step(effect_match,&danger_pass,&result) == 0 && observe(effect_match).find("\"hp\":69,\"mp\":0") != std::string::npos,
        "reflected danger keeps original30 body");gfo_match_destroy(effect_match);
    for(const auto trial : {std::array<uint64_t,2>{2644,0},std::array<uint64_t,2>{74,77}}) {
        fixture.reset(setup,trial[0]);const auto holder = trial[1] == 0 ? 0u : 1u; const auto mortar_token = receipt_token(holder,209);
        check(gfo_match_create(&cfg,players.data(),2,trial[0],&effect_match) == 0,"attracted danger fixture");
        danger_attack.items[0] = receipt_token(0,94);
        check(gfo_match_step(effect_match,&danger_attack,&result) == 0 && result.terminated && result.decisions == 1
            && observe(effect_match).find("\"mortar\":true,\"power\":99,") != std::string::npos
            && !has_token(effect_match,trial[1],mortar_token) && observe(effect_match).find("\"pending\":null") != std::string::npos,
            "attracted99 body consumes mortar immediately and ends without defense");gfo_match_destroy(effect_match);
    }
    fixture.reset(setup, 275);cfg.initial_mp = 99;
    check(gfo_match_create(&cfg,players.data(),2, 275,&effect_match) == 0,"unsupported danger plus Wide fixture");
    danger_attack.item_count = 2;danger_attack.items[0] = receipt_token(0,94);danger_attack.items[1] = receipt_token(0,232);
    const auto before_gap = observe(effect_match);result = status(effect_match);const auto gap_status = result;
    check(gfo_match_legal(effect_match,&danger_attack,&legal) == 0 && !legal
        && gfo_match_step(effect_match,&danger_attack,&result) == -5 && equal(result,gap_status) && observe(effect_match) == before_gap,
        "client-valid Wide interaction returns explicit unsupported without mutation");gfo_match_destroy(effect_match);
    // ABI2 transports the full canonical resource allocation in its new tail.
    cfg.initial_hand = cfg.hand_limit = 18;cfg.initial_hp = 40;cfg.initial_mp = 0;cfg.initial_cp = 20;cfg.max_decisions = 512;
    fixture.reset(setup, 0);
    check(gfo_match_create(&cfg,players.data(),2, 0,&effect_match) == 0,"exchange ABI2 fixture");
    gfo_match_command exchange{};exchange.kind = 4;exchange.item_count = 1;exchange.items[0] = receipt_token(0,3);
    exchange.has_allocation = 1;exchange.hp = 20;exchange.mp = 30;exchange.cp = 10;
    const auto before_exchange = observe(effect_match);const auto exchange_status = status(effect_match);
    for(int error=0;error<8;++error){auto bad=exchange;
        if(error==0)bad.has_allocation=0;if(error==1)bad.reserved_tail[2]=1;if(error==2)bad.has_allocation=2;
        if(error==3){bad.hp=99;bad.mp=99;bad.cp=99;}if(error==4){bad.hp=-1;bad.mp=41;bad.cp=20;}
        if(error==5)bad.has_target=1;if(error==6)bad.kind=0;if(error==7){bad.item_count=2;bad.items[1]=bad.items[0];}
        result=exchange_status;check(gfo_match_step(effect_match,&bad,&result) == -2 && equal(result,exchange_status)
            &&observe(effect_match)==before_exchange,"invalid ABI2 tail, sum and role preserve output and full state");}
    check(gfo_match_legal(effect_match,&exchange,&legal)==0&&legal&&gfo_match_step(effect_match,&exchange,&result)==0
        &&result.phase==0&&result.actor==77&&result.decisions==1&&hand_size(effect_match,0)==18
        &&!has_token(effect_match,0,exchange.items[0])&&observe(effect_match).find("\"hp\":20,\"mp\":30,\"cp\":10")!=std::string::npos
        &&observe(effect_match).find(official::exchange_rule)!=std::string::npos,"exchange has zero fee, direct resources and one replacement without response");
    gfo_match_destroy(effect_match);
    cfg.initial_hand=cfg.hand_limit=1;
    check(gfo_match_create(&cfg,players.data(),2, 0,&effect_match)==0,"single exchange sampler fixture");
    const auto before_allocation_queries=observe(effect_match);bool saw_exchange=false;
    for(uint64_t policy_seed{};policy_seed<128;++policy_seed){uint64_t policy=policy_seed;gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match,&policy,&candidate)==0&&gfo_match_legal(effect_match,&candidate,&legal)==0&&legal,"ABI2 sampler emits a legal command");
        if(candidate.kind==4){saw_exchange=true;check(candidate.has_allocation==1&&!candidate.has_target&&candidate.item_count==1
            &&candidate.hp>=0&&candidate.hp<=99&&candidate.mp>=0&&candidate.mp<=99&&candidate.cp>=0&&candidate.cp<=99
            &&candidate.hp+candidate.mp+candidate.cp==60,"sampled allocation tail conserves current resources within0..99");}
        else check(!candidate.has_allocation&&!candidate.hp&&!candidate.mp&&!candidate.cp,"non-exchange allocation tail stays canonical zero");}
    check(saw_exchange&&observe(effect_match)==before_allocation_queries,"allocation sampling changes only caller policy RNG");gfo_match_destroy(effect_match);
    cfg.initial_hand=cfg.hand_limit=18;cfg.initial_hp=5;cfg.initial_cp=0;cfg.max_decisions=1;
    for(const uint64_t seed:{uint64_t{0},uint64_t{136}}){fixture.reset(setup,seed);exchange.items[0]=receipt_token(0,3);exchange.hp=0;exchange.mp=5;exchange.cp=0;
        check(gfo_match_create(&cfg,players.data(),2,seed,&effect_match)==0&&gfo_match_step(effect_match,&exchange,&result)==0,"zero HP exchange accepted");
        check((seed==136?result.truncated:result.terminated)&&!(seed==136?result.terminated:result.truncated)
            &&observe(effect_match).find(seed==136?"\"hp\":10,\"mp\":5,\"cp\":0":"\"hp\":0,\"mp\":5,\"cp\":0")!=std::string::npos
            &&hand_size(effect_match,0)==(seed==136?16u:17u),"HP0 exchange resolves revival or natural loss before cutoff without issuing deferred gifts");
        gfo_match_destroy(effect_match);}
    // Empty resolved events survive rejected calls, then clear on the next accepted main.
    cfg.initial_hand = 1; cfg.hand_limit = 18; cfg.initial_hp = 40; cfg.initial_mp = 0; cfg.initial_cp = 20; cfg.max_decisions = 512;
    check(gfo_match_create(&cfg, players.data(), 2, 33, &effect_match) == 0
        && observe(effect_match).find("\"last_removal\":null") != std::string::npos, "removal result starts null");
    gfo_match_command empty_broom{}; empty_broom.item_count = 1; empty_broom.items[0] = 0; empty_broom.has_target = 1;
    check(gfo_match_step(effect_match, &empty_broom, &result) == 0
        && observe(effect_match).find("\"last_removal\":{\"origin\":0,\"source\":0,\"target\":0,\"model\":205,\"removed_models\":[]}") != std::string::npos, "self Broom consumes first and records an empty result");
    const auto empty_event = observe(effect_match); const auto empty_status = status(effect_match);
    gfo_match_command clear_removal{}; clear_removal.actor = 77; clear_removal.kind = 0; clear_removal.item_count = 1; clear_removal.items[0] = 1; clear_removal.has_target = 1;
    auto wrong_actor = clear_removal; wrong_actor.actor = 0; result = empty_status;
    check(gfo_match_step(effect_match, &wrong_actor, &result) == -2 && equal(result, empty_status)
        && observe(effect_match) == empty_event, "invalid main preserves the completed removal event");
    check(gfo_match_step(effect_match, &clear_removal, nullptr) != 0 && observe(effect_match) == empty_event
        && equal(status(effect_match), empty_status), "invalid output pointer preserves the removal event");
    check(gfo_match_step(effect_match, &clear_removal, &result) == 0
        && observe(effect_match).find("\"last_removal\":null") != std::string::npos, "accepted next main clears removal result");
    check(gfo_match_reset(effect_match, players.data(), 2, 60, &result) == 0
        && observe(effect_match).find("\"last_removal\":null") != std::string::npos, "reset clears removal result");
    gfo_match_destroy(effect_match);
    cfg.max_decisions = 2;
    check(gfo_match_create(&cfg, players.data(), 2, 101314, &effect_match) == 0, "cutoff public removal fixture");
    empty_broom.target = 77;
    check(gfo_match_step(effect_match, &empty_broom, &result) == 0
        && observe(effect_match).find("\"last_removal\":null") != std::string::npos, "pending foreign removal still has no result");
    remove_pass.actor = 77;
    check(gfo_match_step(effect_match, &remove_pass, &result) == 0 && result.truncated
        && hand_size(effect_match, 0) == 0 && hand_size(effect_match, 77) == 0
        && observe(effect_match, 0, 0).find("\"last_removal\":{\"origin\":0,\"source\":0,\"target\":77,\"model\":205,\"removed_models\":[224]}") != std::string::npos,
        "cutoff retains resolved public result while cancelling gifts");
    gfo_match_destroy(effect_match);
    // SELL keeps ABI2's fixed command tail and uses a distinct, unused physical product.
    cfg.initial_hand = cfg.hand_limit = 18; cfg.initial_hp = 40; cfg.initial_mp = 4; cfg.initial_cp = 3; cfg.max_decisions = 512;
    check(gfo_match_create(&cfg, players.data(), 2, 395, &effect_match) == 0 && gfo_match_supported(4), "SELL public seeded fixture");
    gfo_match_command sale{}; sale.kind = 5; sale.item_count = 2; sale.items[0] = 0; sale.items[1] = 15; sale.has_target = 1;
    const auto before_sale = observe(effect_match); const auto sale_status = status(effect_match);
    for (int error = 0; error < 7; ++error) {
        auto bad = sale;
        if (error == 0) bad.kind = 0; if (error == 1) bad.items[1] = bad.items[0];
        if (error == 2) std::swap(bad.items[0], bad.items[1]); if (error == 3) { bad.item_count = 1; bad.items[1] = 0; }
        if (error == 4) bad.has_target = 0; if (error == 5) { bad.has_allocation = 1; bad.hp = 40; bad.mp = 4; bad.cp = 3; }
        if (error == 6) bad.reserved_tail[1] = 1;
        result = sale_status; legal = 0xa5;
        const auto query_status = gfo_match_legal(effect_match, &bad, &legal);
        check((error == 6 ? query_status == -2 && legal == 0xa5 : query_status == 0 && !legal)
            && gfo_match_step(effect_match, &bad, &result) == -2 && equal(result, sale_status)
            && observe(effect_match) == before_sale, "invalid SELL command is canonical and atomic");
    }
    auto full_sale = sale; full_sale.target = 77; result = sale_status;
    check(gfo_match_legal(effect_match, &full_sale, &legal) == 0 && legal && equal(result, sale_status)
        && observe(effect_match) == before_sale, "full recipient does not leak through SELL preflight");
    bool sampled_sale = false;
    for (uint64_t seed = 0; seed < 128; ++seed) {
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "SELL policy candidates are legal");
        if (candidate.kind == 5) { sampled_sale = true; check(candidate.item_count == 2 && candidate.items[0] != candidate.items[1]
            && candidate.has_target && (candidate.target == 0 || candidate.target == 77) && !candidate.has_allocation, "SELL sampler tail and recipient are canonical"); }
    }
    check(sampled_sale && observe(effect_match) == before_sale, "SELL sampling leaves environment and public event unchanged");
    check(gfo_match_step(effect_match, &sale, &result) == 0 && result.actor == 77 && result.phase == 0
        && observe(effect_match).find("\"hp\":37,\"mp\":0,\"cp\":10") != std::string::npos
        && hand_size(effect_match, 0) == 18 && !has_token(effect_match, 0, 0) && !has_token(effect_match, 0, 15)
        && has_token(effect_match, 0, 36), "self SELL debits CP then MP then HP, credits afterward and replaces only SELL");
    const auto sale_event = "\"last_sale\":{\"origin\":0,\"source\":0,\"target\":0,\"model\":153,\"price\":10,\"overflow_model\":null}";
    check(observe(effect_match, 0, 0).find(sale_event) != std::string::npos
        && observe(effect_match, 1, 77).find(sale_event) != std::string::npos, "resolved sale publicly identifies product and price");
    const auto resolved_sale = observe(effect_match); result = status(effect_match); const auto resolved_status = result;
    check(gfo_match_step(effect_match, &sale, &result) == -2 && equal(result, resolved_status)
        && observe(effect_match) == resolved_sale, "rejected next main preserves sale event");
    check(gfo_match_reset(effect_match, players.data(), 2, 395, &result) == 0
        && observe(effect_match).find("\"last_sale\":null") != std::string::npos, "reset clears sale event");
    gfo_match_destroy(effect_match);
    // An unused miracle is transferred, never retained or charged as a cast.
    cfg.initial_hand = 4; cfg.initial_mp = 99; cfg.initial_cp = 20;
    check(gfo_match_create(&cfg, players.data(), 2, 190782, &effect_match) == 0, "reflected miracle sale fixture");
    sale.items[0] = 1; sale.items[1] = 0; sale.target = 77;
    check(gfo_match_step(effect_match, &sale, &result) == 0
        && observe(effect_match).find("\"last_sale\":null") != std::string::npos, "foreign sale waits without a completed sale event");
    gfo_match_command sale_mirror{}; sale_mirror.kind = 1; sale_mirror.actor = 77; sale_mirror.item_count = 1; sale_mirror.items[0] = 4;
    check(gfo_match_step(effect_match, &sale_mirror, &result) == 0 && result.actor == 0, "trade accepts universal reflection");
    gfo_match_command sale_pass{}; sale_pass.kind = 1;
    check(gfo_match_step(effect_match, &sale_pass, &result) == 0
        && observe(effect_match, 0, 0).find("\"last_sale\":{\"origin\":0,\"source\":77,\"target\":0,\"model\":215,\"price\":0,\"overflow_model\":null}") != std::string::npos
        && observe(effect_match).find("\"token\":8,\"face\":215") != std::string::npos
        && observe(effect_match).find("\"used\":true") == std::string::npos
        && hand_size(effect_match, 0) == 4 && hand_size(effect_match, 77) == 4,
        "reflection preserves product while transferring a fresh unused miracle without MP or extra gift");
    gfo_match_destroy(effect_match);
    cfg.initial_hp = 40; cfg.initial_mp = 4; cfg.initial_cp = 3; cfg.max_decisions = 2;
    check(gfo_match_create(&cfg, players.data(), 2, 51, &effect_match) == 0, "sale cutoff fixture");
    sale.items[0] = 3; sale.items[1] = 1; sale.target = 77; sale_pass.actor = 77;
    check(gfo_match_step(effect_match, &sale, &result) == 0 && gfo_match_step(effect_match, &sale_pass, &result) == 0
        && result.truncated && !result.terminated && hand_size(effect_match, 0) == 2 && hand_size(effect_match, 77) == 5
        && observe(effect_match, 0, 0).find("\"last_sale\":{\"origin\":0,\"source\":0,\"target\":77,\"model\":43,\"price\":10,\"overflow_model\":null}") != std::string::npos,
        "cutoff preserves resolved sale and direct delivery while cancelling the deferred SELL gift");
    gfo_match_destroy(effect_match);
    // BUY adds a decision phase without changing ABI2's command width or existing phase numbers.
    cfg.initial_hand = 4; cfg.hand_limit = 18; cfg.initial_hp = 40; cfg.initial_mp = 0; cfg.initial_cp = 99; cfg.max_decisions = 512;
    check(gfo_match_create(&cfg, players.data(), 2, 3440, &effect_match) == 0 && gfo_match_supported(5)
        && observe(effect_match).find(official::buy_rule) != std::string::npos, "BUY public seeded fixture and explicit contract");
    gfo_match_command buy{}; buy.kind = 6; buy.item_count = 1; buy.items[0] = 2; buy.has_target = 1; buy.target = 77;
    const auto before_buy = observe(effect_match); const auto buy_status = status(effect_match);
    for (int error = 0; error < 5; ++error) {
        auto bad = buy;
        if (error == 0) bad.kind = 0; if (error == 1) { bad.item_count = 2; bad.items[1] = 0; }
        if (error == 2) { bad.has_target = 0; bad.target = 0; } if (error == 3) bad.target = 999;
        if (error == 4) { bad.has_allocation = 1; bad.hp = 40; bad.cp = 99; }
        result = buy_status; check(gfo_match_legal(effect_match, &bad, &legal) == 0 && !legal
            && gfo_match_step(effect_match, &bad, &result) == -2 && equal(result, buy_status)
            && observe(effect_match) == before_buy, "invalid BUY role and allocation reject atomically");
    }
    auto self_buy = buy; self_buy.target = 0; result = buy_status;
    check(gfo_match_legal(effect_match, &self_buy, &legal) == 0 && legal && equal(result, buy_status)
        && observe(effect_match) == before_buy, "self BUY is a legal pure query before its separate inspection path");
    gfo_match_command buy_pass{}; buy_pass.kind = 1; buy_pass.actor = 77;
    check(gfo_match_step(effect_match, &buy, &result) == 0 && result.phase == 1
        && observe(effect_match).find("\"purchase\":null") != std::string::npos
        && gfo_match_step(effect_match, &buy_pass, &result) == 0 && result.phase == 3 && result.actor == 0 && result.owner == 0,
        "foreign BUY selects escrow only after response and enters purchaser phase3");
    const auto quote_text = "\"purchase\":{\"origin\":0,\"buyer\":0,\"seller\":77,\"model\":233,\"price\":0}";
    check(observe(effect_match, 0, 0).find(quote_text) != std::string::npos
        && observe(effect_match, 1, 77).find(quote_text) != std::string::npos
        && observe(effect_match).find("\"pending\":null") != std::string::npos
        && !has_token(effect_match, 77, 4), "quote publicly exposes only model and price while exact unused copy enters escrow");
    const auto quote_view = observe(effect_match); const auto quote_status = status(effect_match);
    bool saw_accept = false, saw_decline = false;
    for (uint64_t seed = 0; seed < 32; ++seed) {
        uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0
            && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal && !candidate.item_count && !candidate.has_target
            && !candidate.has_allocation && (candidate.kind == 7 || candidate.kind == 8), "purchase sampler emits only canonical YES or NO");
        saw_accept |= candidate.kind == 7; saw_decline |= candidate.kind == 8;
    }
    check(saw_accept && saw_decline && observe(effect_match) == quote_view, "purchase queries do not change escrow or RNG");
    gfo_match_command accept_buy{}; accept_buy.kind = 7;
    auto wrong_buyer = accept_buy; wrong_buyer.actor = 77; result = quote_status;
    check(gfo_match_step(effect_match, &wrong_buyer, &result) == -2 && equal(result, quote_status)
        && observe(effect_match) == quote_view, "only current buyer may decide purchase");
    check(gfo_match_step(effect_match, &accept_buy, &result) == 0 && result.phase == 0 && result.actor == 77
        && observe(effect_match).find("\"last_purchase\":{\"origin\":0,\"buyer\":0,\"seller\":77,\"model\":233,\"price\":0,\"bought\":true,\"reason\":\"bought\",\"overflow_model\":null}") != std::string::npos
        && observe(effect_match).find("\"token\":8,\"face\":233") != std::string::npos
        && observe(effect_match).find("\"used\":true") == std::string::npos
        && hand_size(effect_match, 0) == 5 && hand_size(effect_match, 77) == 3, "accepted BUY transfers fresh unused miracle without casting or product gift");
    gfo_match_destroy(effect_match);
    for (const uint32_t maximum : {uint32_t{512}, uint32_t{2}, uint32_t{1}}) {
        cfg.max_decisions = maximum;
        check(gfo_match_create(&cfg, players.data(), 2, 3440, &effect_match) == 0 && gfo_match_step(effect_match, &buy, &result) == 0, "decline and cutoff escrow fixture");
        if (maximum != 1) {
            check(gfo_match_step(effect_match, &buy_pass, &result) == 0, "BUY response reaches quote or cutoff");
            if (maximum == 512) { auto decline_buy = accept_buy; decline_buy.kind = 8;
                check(gfo_match_step(effect_match, &decline_buy, &result) == 0
                    && observe(effect_match).find("\"reason\":\"declined\"") != std::string::npos, "NO returns the chosen artifact"); }
            else check(result.truncated && observe(effect_match).find("\"reason\":\"cutoff\"") != std::string::npos, "body cutoff returns undecided escrow");
        } else check(result.truncated && observe(effect_match).find("\"last_purchase\":null") != std::string::npos, "launch cutoff invents no quote or selection");
        check(observe(effect_match).find("\"purchase\":null") != std::string::npos && hand_size(effect_match, 77) == 4
            && has_token(effect_match, 77, 4) && hand_size(effect_match, 0) == (maximum == 512 ? 4u : 3u), "escrow returns original token and gift obeys cutoff");
        check(gfo_match_reset(effect_match, players.data(), 2, 3, &result) == 0
            && observe(effect_match).find("\"last_purchase\":null") != std::string::npos, "reset clears completed purchase history");
        gfo_match_destroy(effect_match);
    }
    cfg.max_decisions = 512;
    check(gfo_match_create(&cfg, players.data(), 2, 23962, &effect_match) == 0, "reflected BUY fixture");
    buy.items[0] = 1; gfo_match_command buy_mirror{}; buy_mirror.kind = 1; buy_mirror.actor = 77; buy_mirror.item_count = 1; buy_mirror.items[0] = 5;
    buy_pass.actor = 0;
    check(gfo_match_step(effect_match, &buy, &result) == 0 && gfo_match_step(effect_match, &buy_mirror, &result) == 0
        && gfo_match_step(effect_match, &buy_pass, &result) == 0 && result.phase == 3 && result.actor == 77
        && observe(effect_match, 0, 0).find("\"purchase\":{\"origin\":0,\"buyer\":77,\"seller\":0,\"model\":200,\"price\":15}") != std::string::npos,
        "reflection exchanges buyer and seller while preserving origin");
    accept_buy.actor = 77;
    check(gfo_match_step(effect_match, &accept_buy, &result) == 0 && hand_size(effect_match, 0) == 3 && hand_size(effect_match, 77) == 5
        && observe(effect_match, 1, 77).find("\"token\":8,\"face\":200") != std::string::npos, "current buyer receives fresh product after reflection");
    gfo_match_destroy(effect_match);
    cfg.initial_hand = 1; cfg.initial_cp = 0;
    check(gfo_match_create(&cfg, players.data(), 2, 3049, &effect_match) == 0, "unaffordable BUY fixture");
    buy.items[0] = 0; buy_pass.actor = 77;
    check(gfo_match_step(effect_match, &buy, &result) == 0 && gfo_match_step(effect_match, &buy_pass, &result) == 0 && result.phase == 0
        && observe(effect_match, 0, 0).find("\"last_purchase\":{\"origin\":0,\"buyer\":0,\"seller\":77,\"model\":6,\"price\":1,\"bought\":false,\"reason\":\"insufficient-cp\",\"overflow_model\":null}") != std::string::npos
        && has_token(effect_match, 77, 1) && hand_size(effect_match, 77) == 1, "CP-only affordability returns original product without approval or debt");
    gfo_match_destroy(effect_match);
    // Disease side effects are public status changes; privileged tick records remain private.
    for (auto id : {30, 33, 48, 62, 219, 220}) check(gfo_match_supported(id), "disease model membership");
    cfg.initial_hand = cfg.hand_limit = 18; cfg.initial_hp = 40; cfg.initial_mp = 0; cfg.initial_cp = 20; cfg.max_decisions = 512;
    fixture.reset(setup, 39);
    check(gfo_match_create(&cfg, players.data(), 2, 39, &effect_match) == 0, "cold weapon and mild cure fixture");
    gfo_match_command cold_attack{}; cold_attack.item_count = 1; cold_attack.items[0] = receipt_token(0, 33); cold_attack.has_target = 1; cold_attack.target = 77;
    gfo_match_command cold_pass{}; cold_pass.kind = 1; cold_pass.actor = 77;
    check(gfo_match_step(effect_match, &cold_attack, &result) == 0
        && observe(effect_match, 0, 0).find("\"curses\":[\"cold\"]") == std::string::npos
        && gfo_match_step(effect_match, &cold_pass, &result) == 0
        && observe(effect_match, 0, 0).find("\"hp\":31,\"mp\":0,\"cp\":20,\"curses\":[\"cold\"]") != std::string::npos,
        "positive weapon damage applies Cold without ticking the next owner prematurely");
    gfo_match_command mild_cure{}; mild_cure.actor = 77; mild_cure.item_count = 1; mild_cure.items[0] = receipt_token(1, 199); mild_cure.has_target = 1; mild_cure.target = 77;
    check(gfo_match_step(effect_match, &mild_cure, &result) == 0
        && observe(effect_match, 0, 0).find("\"hp\":31,\"mp\":0,\"cp\":20,\"curses\":[]") != std::string::npos
        && observe(effect_match, 0, 0).find("\"curses\":[\"cold\"]") == std::string::npos,
        "mild cure removes Cold before its owner-end damage");
    gfo_match_destroy(effect_match);
    cfg.initial_mp = 99; fixture.reset(setup, 113);
    check(gfo_match_create(&cfg, players.data(), 2, 113, &effect_match) == 0, "retained Wind progression fixture");
    gfo_match_command wind{}; wind.item_count = 1; wind.items[0] = receipt_token(0, 219); wind.has_target = 1;
    std::vector<uint64_t> cycle_discards;
    for (const auto& item : fixture.snapshot().state.players[1].items) {
        const auto& card = *official::find_card(item.model);
        if (card.category != official::Category::weapons && card.ability != official::Ability::revive && card.ability != official::Ability::attract_danger) cycle_discards.push_back(item.token);
    }
    check(cycle_discards.size() >= 4, "Wind fixture has four public discard transitions");
    const std::array<const char*, 4> diseases{"cold", "fever", "hell", "heaven"}; const std::array<int, 4> disease_hp{39, 37, 32, 37};
    for (std::size_t stage = 0; stage < diseases.size(); ++stage) {
        check(gfo_match_step(effect_match, &wind, &result) == 0 && result.actor == 77
            && observe(effect_match).find("\"hp\":" + std::to_string(disease_hp[stage]) + ",\"mp\":" + std::to_string(99 - 6 * (stage + 1))) != std::string::npos
            && observe(effect_match).find(std::string("\"curses\":[\"") + diseases[stage] + "\"]") != std::string::npos
            && observe(effect_match).find("\"last_tick\"") == std::string::npos,
            "same retained Wind advances one disease stage and ticks only the owner once");
        gfo_match_command yield{}; yield.actor = 77; yield.kind = 2; yield.item_count = 1; yield.items[0] = cycle_discards[stage];
        check(gfo_match_step(effect_match, &yield, &result) == 0, "other player passes by an ordinary discard");
    }
    check(gfo_match_step(effect_match, &wind, &result) == 0 && result.terminated && result.winners[0] == 77
        && observe(effect_match).find("\"hp\":0,\"mp\":69,\"cp\":20,\"curses\":[\"heaven\"]") != std::string::npos,
        "disease reapplication to Heaven falls before healing and never refunds retained cost");
    gfo_match_destroy(effect_match);
    cfg.initial_cp = 99; fixture.reset(setup, 112);
    check(gfo_match_create(&cfg, players.data(), 2, 112, &effect_match) == 0
        && observe(effect_match).find(official::disease_rule) != std::string::npos, "Heaven BUY tick-boundary fixture");
    gfo_match_command heaven{}; heaven.item_count = 1; heaven.items[0] = receipt_token(0, 220); heaven.has_target = 1;
    check(gfo_match_step(effect_match, &heaven, &result) == 0 && observe(effect_match).find("\"hp\":45,\"mp\":84") != std::string::npos, "first Heaven owner tick heals5");
    gfo_match_command other_discard{}; other_discard.actor = 77; other_discard.kind = 2; other_discard.item_count = 1;
    bool discarded_for_quote = false;
    for (const auto& item : fixture.snapshot().state.players[1].items) {
        other_discard.items[0] = item.token;
        if (gfo_match_legal(effect_match, &other_discard, &legal) == 0 && legal) { check(gfo_match_step(effect_match, &other_discard, &result) == 0, "other owner discard before BUY"); discarded_for_quote = true; break; }
    }
    check(discarded_for_quote, "Heaven fixture has a legal other-owner action");
    buy.items[0] = receipt_token(0, 5); buy.target = 77; buy_pass.actor = 77;
    check(gfo_match_step(effect_match, &buy, &result) == 0 && gfo_match_step(effect_match, &buy_pass, &result) == 0
        && result.phase == 3 && observe(effect_match).find("\"hp\":45,\"mp\":84") != std::string::npos, "purchase waiting does not trigger the owner disease tick");
    gfo_match_command decline_heaven{}; decline_heaven.kind = 8;
    check(gfo_match_step(effect_match, &decline_heaven, &result) == 0
        && observe(effect_match).find("\"hp\":50,\"mp\":84") != std::string::npos
        && observe(effect_match).find("\"last_tick\"") == std::string::npos, "final purchase choice triggers exactly one private owner tick");
    gfo_match_destroy(effect_match);
    cfg.max_decisions = 1;
    check(gfo_match_create(&cfg, players.data(), 2, 112, &effect_match) == 0 && gfo_match_step(effect_match, &heaven, &result) == 0
        && result.truncated && !result.terminated
        && observe(effect_match).find("\"hp\":40,\"mp\":84") != std::string::npos
        && observe(effect_match).find("\"curses\":[\"heaven\"]") != std::string::npos, "command cutoff precedes even a completed-action disease tick");
    gfo_match_destroy(effect_match);
    // Self-curse armor runs after physical consumption even when its DEF cancels the entire attack.
    cfg.initial_hand = 4; cfg.hand_limit = 18; cfg.initial_hp = 40; cfg.initial_mp = 0; cfg.initial_cp = 20; cfg.max_decisions = 512;
    check(gfo_match_supported(168) && gfo_match_supported(198)
        && gfo_match_create(&cfg, players.data(), 2, 92506, &effect_match) == 0, "mask public reset fixture");
    gfo_match_command mask_attack{}; mask_attack.item_count = 1; mask_attack.items[0] = 1; mask_attack.has_target = 1; mask_attack.target = 77;
    check(gfo_match_step(effect_match, &mask_attack, &result) == 0, "bronze attack before mask");
    gfo_match_command mask_defense{}; mask_defense.actor = 77; mask_defense.kind = 1; mask_defense.item_count = 2;
    const auto before_mask = observe(effect_match); const auto mask_status = status(effect_match);
    for (const bool reversed : {false, true}) {
        mask_defense.items[0] = reversed ? 4 : 6; mask_defense.items[1] = reversed ? 6 : 4; result = mask_status;
        check(gfo_match_legal(effect_match, &mask_defense, &legal) == 0 && !legal
            && gfo_match_step(effect_match, &mask_defense, &result) == -2 && equal(result, mask_status)
            && observe(effect_match) == before_mask, "mask and matching bounce cannot share a response in either order");
    }
    mask_defense.item_count = 1; mask_defense.items[0] = 6; mask_defense.items[1] = 0;
    check(gfo_match_step(effect_match, &mask_defense, &result) == 0 && !has_token(effect_match, 77, 6)
        && observe(effect_match, 0, 0).find("\"hp\":40,\"mp\":0,\"cp\":20,\"curses\":[\"fever\"]") != std::string::npos,
        "full defense still applies self Fever before passing ownership");
    gfo_match_destroy(effect_match);
    check(gfo_match_create(&cfg, players.data(), 2, 92506, &effect_match) == 0, "discard mask control fixture");
    gfo_match_command discard_mask{}; discard_mask.kind = 2; discard_mask.item_count = 1; discard_mask.items[0] = 0;
    check(gfo_match_step(effect_match, &discard_mask, &result) == 0, "yield before mask discard");
    discard_mask.actor = 77; discard_mask.items[0] = 6;
    check(gfo_match_step(effect_match, &discard_mask, &result) == 0
        && observe(effect_match, 0, 0).find("\"curses\":[\"fever\"]") == std::string::npos, "discard does not execute armor self-curse");
    gfo_match_destroy(effect_match);
    cfg.initial_hand = 18; cfg.initial_mp = 99;
    check(gfo_match_create(&cfg, players.data(), 2, 86483, &effect_match) == 0, "Heaven mask with passive revival fixture");
    gfo_match_command prepare_heaven{}; prepare_heaven.item_count = 1; prepare_heaven.items[0] = 11; prepare_heaven.has_target = 1;
    check(gfo_match_step(effect_match, &prepare_heaven, &result) == 0
        && observe(effect_match).find("\"hp\":45,\"mp\":84") != std::string::npos, "establish Heaven before defender's next response");
    mask_attack.actor = 77; mask_attack.items[0] = 21; mask_attack.target = 0;
    mask_defense.actor = 0; mask_defense.items[0] = 6;
    check(gfo_match_step(effect_match, &mask_attack, &result) == 0 && gfo_match_step(effect_match, &mask_defense, &result) == 0
        && !result.terminated && result.actor == 0 && !has_token(effect_match, 0, 6) && !has_token(effect_match, 0, 5)
        && hand_size(effect_match, 0) == 18 && observe(effect_match).find("\"hp\":10,\"mp\":84,\"cp\":20,\"curses\":[\"heaven\"]") != std::string::npos,
        "Heaven prefix falls at HP0, regular body fully defends, then amulet revives and both physical consumptions get deferred gifts");
    gfo_match_destroy(effect_match);
    cfg.initial_hand = 4; cfg.initial_mp = 90;
    check(gfo_match_create(&cfg, players.data(), 2, 7479, &effect_match) == 0
        && observe(effect_match).find(official::disease_item_rule) != std::string::npos, "herb effect contract fixture");
    gfo_match_command herb{}; herb.item_count = 1; herb.items[0] = 1; herb.has_target = 1; herb.target = 77;
    gfo_match_command herb_pass{}; herb_pass.actor = 77; herb_pass.kind = 1;
    check(gfo_match_step(effect_match, &herb, &result) == 0 && gfo_match_step(effect_match, &herb_pass, &result) == 0
        && observe(effect_match, 0, 0).find("\"hp\":40,\"mp\":99,\"cp\":20,\"curses\":[\"heaven\"]") != std::string::npos,
        "foreign herb caps MP before Heaven and does not tick the recipient early");
    gfo_match_destroy(effect_match); cfg.max_decisions = 3;
    check(gfo_match_create(&cfg, players.data(), 2, 7479, &effect_match) == 0, "reflected herb cutoff fixture");
    gfo_match_command herb_mirror{}; herb_mirror.actor = 77; herb_mirror.kind = 1; herb_mirror.item_count = 1; herb_mirror.items[0] = 6;
    herb_pass.actor = 0;
    check(gfo_match_step(effect_match, &herb, &result) == 0 && gfo_match_step(effect_match, &herb_mirror, &result) == 0
        && gfo_match_step(effect_match, &herb_pass, &result) == 0 && result.truncated
        && observe(effect_match).find("\"hp\":40,\"mp\":99,\"cp\":20,\"curses\":[\"heaven\"]") != std::string::npos
        && hand_size(effect_match, 0) == 3 && hand_size(effect_match, 77) == 3,
        "reflection keeps herb MP and disease payload, with completed-body cutoff before owner tick and gifts");
    gfo_match_destroy(effect_match);
    // Fog projection and policy queries must not expose covered opponent state.
    for (auto id : {72, 97, 182, 221}) check(gfo_match_supported(id), "Fog model membership");
    cfg.initial_hand = cfg.hand_limit = 18; cfg.initial_hp = 40; cfg.initial_mp = 99; cfg.initial_cp = 20; cfg.max_decisions = 512;
    fixture.reset(setup, 487975);
    const auto fog_token = receipt_token(0, 221), fog_cure = receipt_token(0, 199), fog_sell = receipt_token(0, 4), fog_product = receipt_token(0, 6), fog_spring = receipt_token(1, 235);
    check(gfo_match_create(&cfg, players.data(), 2, 487975, &effect_match) == 0, "Fog display and SELL fixture");
    gfo_match_command fog_cast{}; fog_cast.item_count = 1; fog_cast.items[0] = fog_token; fog_cast.has_target = 1;
    check(gfo_match_step(effect_match, &fog_cast, &result) == 0
        && observe(effect_match).find("\"status\":null") != std::string::npos
        && observe(effect_match, 0, 0).find("\"status\":null") == std::string::npos,
        "live Fog viewer loses enemy status while spectator keeps it");
    gfo_match_command public_spring{}; public_spring.actor = 77; public_spring.item_count = 1; public_spring.items[0] = fog_spring; public_spring.has_target = 1; public_spring.target = 77;
    check(gfo_match_step(effect_match, &public_spring, &result) == 0
        && observe(effect_match).find("\"used_models\":[235]") != std::string::npos, "covered opponent used-miracle panel remains public");
    gfo_match_command fog_foreign_sale{}; fog_foreign_sale.kind = 5; fog_foreign_sale.item_count = 2; fog_foreign_sale.items[0] = fog_sell; fog_foreign_sale.items[1] = fog_product; fog_foreign_sale.has_target = 1; fog_foreign_sale.target = 77;
    const auto fog_view = observe(effect_match); result = status(effect_match); const auto fog_status = result;
    check(gfo_match_legal(effect_match, &fog_foreign_sale, &legal) == 0 && legal && equal(result, fog_status) && observe(effect_match) == fog_view,
        "Fog foreign SELL preflight ignores recipient capacity and leaves state unchanged");
    bool fog_self_sale = false;
    for (uint64_t seed = 0; seed < 1024; ++seed) { uint64_t policy = seed; gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match, &policy, &candidate) == 0 && gfo_match_legal(effect_match, &candidate, &legal) == 0 && legal, "Fog policy emits legal visible-information actions");
        if (candidate.kind == 5) { fog_self_sale = true; check((candidate.target == 0 || candidate.target == 77) && candidate.has_target, "SELL policy uses public alive targets without hidden capacity decisions"); }
    }
    check(fog_self_sale && observe(effect_match) == fog_view, "Fog policy queries preserve environment");
    fog_cast.items[0] = fog_cure;
    check(gfo_match_step(effect_match, &fog_cast, &result) == 0 && observe(effect_match).find("\"status\":null") == std::string::npos
        && observe(effect_match).find("\"last_fog\"") == std::string::npos, "mild cure reveals panes without exposing the private launch record");
    gfo_match_destroy(effect_match);
    const std::array<official::PlayerSetup,3> fog_setup{{{0,official::Team::solo,"A"},{77,official::Team::solo,"B"},{88,official::Team::solo,"C"}}};
    std::array<gfo_match_player,3> fog_players{}; fog_players[0].id = 0; fog_players[1].id = 77; fog_players[2].id = 88;
    fixture.reset(fog_setup, 1243); const auto redirect_fog = receipt_token(0,221), redirect_club = receipt_token(0,6);
    check(gfo_match_create(&cfg, fog_players.data(), 3, 1243, &effect_match) == 0, "three-player Fog target fixture");
    fog_cast.items[0] = redirect_fog; check(gfo_match_step(effect_match, &fog_cast, &result) == 0, "Fog self cast before target selection");
    for (std::size_t seat = 1; seat < 3; ++seat) { bool yielded = false; gfo_match_command discard{}; discard.actor = fog_setup[seat].id; discard.kind = 2; discard.item_count = 1;
        for (const auto& item : fixture.snapshot().state.players[seat].items) { discard.items[0] = item.token;
            if (gfo_match_legal(effect_match,&discard,&legal) == 0 && legal) { check(gfo_match_step(effect_match,&discard,&result) == 0,"other owner discard"); yielded = true; break; } }
        check(yielded,"Fog target fixture has a legal other-owner action");
    }
    gfo_match_command fog_attack{}; fog_attack.item_count = 1; fog_attack.items[0] = redirect_club; fog_attack.has_target = 1; fog_attack.target = 77;
    check(gfo_match_step(effect_match,&fog_attack,&result) == 0 && result.actor == 88
        && observe(effect_match).find("\"forced_hit\":null") != std::string::npos
        && observe(effect_match,0,0).find("\"forced_hit\":false") != std::string::npos,
        "Fog chooses another alive enemy and redacts even a false activation flag");
    gfo_match_command fog_pass{}; fog_pass.kind = 1; fog_pass.actor = 88;
    check(gfo_match_step(effect_match,&fog_pass,&result) == 0,"reselected target resolves normally"); gfo_match_destroy(effect_match);
    fixture.reset(setup, 6772081);
    const auto force_fog = receipt_token(0,221), force_cloud = receipt_token(0,223), force_spark = receipt_token(0,95), force_mirror = receipt_token(1,190);
    check(gfo_match_create(&cfg,players.data(),2, 6772081,&effect_match) == 0,"Fog forced-hit reflection fixture");
    const auto discard_other = [&] { gfo_match_command discard{}; discard.actor=77; discard.kind=2; discard.item_count=1;
        for (const auto& item : fixture.snapshot().state.players[1].items) if (item.token != force_mirror) { discard.items[0]=item.token;
            if(gfo_match_legal(effect_match,&discard,&legal)==0&&legal){check(gfo_match_step(effect_match,&discard,&result)==0,"other discard preserving mirror");return;} }
        check(false,"Fog forced fixture needs an unused ordinary discard");
    };
    fog_cast.items[0]=force_cloud;fog_cast.target=77;fog_pass.actor=77;
    check(gfo_match_step(effect_match,&fog_cast,&result)==0&&gfo_match_step(effect_match,&fog_pass,&result)==0,"apply DarkCloud before Fog");discard_other();
    fog_cast.items[0]=force_fog;fog_cast.target=0;check(gfo_match_step(effect_match,&fog_cast,&result)==0,"cover DarkCloud enemy");discard_other();
    fog_attack.items[0]=force_spark;fog_attack.has_target=0;fog_attack.target=0;
    check(gfo_match_step(effect_match,&fog_attack,&result)==0&&observe(effect_match).find("\"forced_hit\":null")!=std::string::npos
        &&observe(effect_match,0,0).find("\"forced_hit\":true")!=std::string::npos,"hidden forced hit is null only to Fog viewer");
    gfo_match_command force_reflect{};force_reflect.actor=77;force_reflect.kind=1;force_reflect.item_count=1;force_reflect.items[0]=force_mirror;
    check(gfo_match_step(effect_match,&force_reflect,&result)==0&&result.actor==0
        &&observe(effect_match).find("\"forced_hit\":null")!=std::string::npos
        &&observe(effect_match,0,0).find("\"forced_hit\":true")!=std::string::npos,
        "reflection to visible self retains redaction based on original activation target");
    fog_pass.actor=0;check(gfo_match_step(effect_match,&fog_pass,&result)==0,"reflected forced body resolves once");gfo_match_destroy(effect_match);
    for(const bool counter:{false,true}){const uint64_t seed=counter?1252:2;fixture.reset(setup,seed);
        const auto tool=receipt_token(0,counter?6:97); const auto ring=counter?receipt_token(1,182):0;
        check(gfo_match_create(&cfg,players.data(),2,seed,&effect_match)==0,"Fog chance and counter fixture");
        fog_attack.items[0]=tool;fog_attack.has_target=counter;fog_attack.target=counter?77:0;fog_pass.actor=77;fog_pass.item_count=counter?1:0;fog_pass.items[0]=ring;
        check(gfo_match_step(effect_match,&fog_attack,&result)==0&&gfo_match_step(effect_match,&fog_pass,&result)==0,"Fog hit or counter response accepted");
        if(counter){check(result.actor==0&&observe(effect_match,0,0).find("\"models\":[182]")!=std::string::npos,"Fog ring queues pure curse FIFO");fog_pass.actor=0;fog_pass.item_count=0;fog_pass.items[0]=0;check(gfo_match_step(effect_match,&fog_pass,&result)==0,"Fog counter passes");}
        check(observe(effect_match,1,counter?0:77).find("\"status\":null")!=std::string::npos,"Fog weapon and counter both hide enemies for final recipient");gfo_match_destroy(effect_match);
    }
    //112's active quarter-chance weapon role is separate from its posthumous payload.
    cfg.initial_hand = cfg.hand_limit = 18; cfg.initial_hp = 40; cfg.initial_mp = 0; cfg.initial_cp = 20; cfg.max_decisions = 512;
    for (const bool hit : {false,true}) {
        const uint64_t seed = hit ? 216 : 71; fixture.reset(setup,seed); const auto bow = receipt_token(0,112);
        check(gfo_match_create(&cfg,players.data(),2,seed,&effect_match)==0&&observe(effect_match).find(official::death_rule)!=std::string::npos,"dying weapon active role fixture");
        gfo_match_command dying_main{};dying_main.item_count=1;dying_main.items[0]=bow;
        auto targeted=dying_main;targeted.has_target=1;targeted.target=77;const auto before=observe(effect_match);result=status(effect_match);const auto saved=result;
        check(gfo_match_step(effect_match,&targeted,&result)==-2&&equal(result,saved)&&observe(effect_match)==before,"percent dying weapon rejects explicit target atomically");
        check(gfo_match_step(effect_match,&dying_main,&result)==0,"dying weapon hit or miss accepted");
        if(hit){check(result.phase==1&&observe(effect_match,0,0).find("\"power\":1")!=std::string::npos
            &&observe(effect_match,0,0).find("\"hit_rate\":25")!=std::string::npos&&observe(effect_match,0,0).find("\"death\":false")!=std::string::npos,"active bow uses Light1/25 and is not posthumous");
            gfo_match_command pass{};pass.kind=1;pass.actor=77;check(gfo_match_step(effect_match,&pass,&result)==0,"active bow response passes");}
        check(result.phase==0&&!has_token(effect_match,0,bow)&&hand_size(effect_match,0)==18,"active bow consumes once and gifts once whether hit or miss");gfo_match_destroy(effect_match);
    }
    cfg.initial_hp=1;
    check(gfo_match_create(&cfg,players.data(),2, 1626,&effect_match)==0,"last-enemy bow death fixture");
    gfo_match_command dying_main{};dying_main.item_count=1;dying_main.items[0]=14;gfo_match_command dying_pass{};dying_pass.kind=1;dying_pass.actor=77;
    check(gfo_match_step(effect_match,&dying_main,&result)==0&&gfo_match_step(effect_match,&dying_pass,&result)==0
        &&result.terminated&&result.winners[0]==0&&has_token(effect_match,77,30),"natural terminal precedes passive112 consumption");gfo_match_destroy(effect_match);
    cfg.initial_hp=5;
    check(gfo_match_create(&cfg,fog_players.data(),3, 5045,&effect_match)==0,"three-player Dark posthumous fixture");
    gfo_match_command fatal_dark{};fatal_dark.item_count=1;fatal_dark.items[0]=1;fatal_dark.has_target=1;fatal_dark.target=77;
    check(gfo_match_step(effect_match,&fatal_dark,&result)==0&&gfo_match_step(effect_match,&dying_pass,&result)==0&&result.actor==0
        &&observe(effect_match,0,0).find("\"source\":77,\"target\":0")!=std::string::npos
        &&observe(effect_match,0,0).find("\"power\":30")!=std::string::npos&&observe(effect_match,0,0).find("\"hit_rate\":75")!=std::string::npos
        &&observe(effect_match,0,0).find("\"death\":true")!=std::string::npos&&!has_token(effect_match,77,30),"actual Dark death reserves consumed112 and permits dead source for Light30/75 response");
    dying_pass.actor=0;
    check(gfo_match_step(effect_match,&dying_pass,&result)==0&&result.terminated&&result.winners[0]==88
        &&result.rewards[0]==-1&&result.rewards[1]==-1&&result.rewards[2]==1,"posthumous body can change the fixed-seat winner before pending work is cancelled");gfo_match_destroy(effect_match);
    check(gfo_match_create(&cfg,fog_players.data(),3, 111506,&effect_match)==0,"posthumous dead-source reflection fixture");
    fatal_dark.items[0]=5;dying_pass.actor=77;
    check(gfo_match_step(effect_match,&fatal_dark,&result)==0&&gfo_match_step(effect_match,&dying_pass,&result)==0,"first posthumous hit waits for living original attacker");
    gfo_match_command dead_return{};dead_return.kind=1;dead_return.item_count=1;dead_return.items[0]=3;
    check(gfo_match_step(effect_match,&dead_return,&result)==0&&result.actor==88&&!has_token(effect_match,0,3)
        &&observe(effect_match,0,0).find("\"source\":77,\"target\":88")!=std::string::npos
        &&observe(effect_match,0,0).find("\"death\":true")!=std::string::npos,"dead-target reflection cancels that frame and continues FIFO without asking a corpse to respond");
    dying_pass.actor=88;
    check(gfo_match_step(effect_match,&dying_pass,&result)==0&&result.terminated&&result.winners[0]==0,"next posthumous enemy resolves independently");gfo_match_destroy(effect_match);
    check(gfo_match_create(&cfg,fog_players.data(),3, 22085,&effect_match)==0,"amulet before posthumous reservation fixture");
    fatal_dark.items[0]=6;dying_pass.actor=77;
    check(gfo_match_step(effect_match,&fatal_dark,&result)==0&&gfo_match_step(effect_match,&dying_pass,&result)==0&&result.phase==0&&result.actor==77
        &&has_token(effect_match,77,19)&&!has_token(effect_match,77,18)
        &&observe(effect_match,0,0).find("\"hp\":10")!=std::string::npos&&observe(effect_match,0,0).find("\"pending\":null")!=std::string::npos,
        "successful passive revival preserves bow and creates no posthumous response");gfo_match_destroy(effect_match);
    cfg.max_decisions=2;
    check(gfo_match_create(&cfg,fog_players.data(),3, 5045,&effect_match)==0,"posthumous cutoff fixture");fatal_dark.items[0]=1;
    check(gfo_match_step(effect_match,&fatal_dark,&result)==0&&gfo_match_step(effect_match,&dying_pass,&result)==0&&result.truncated&&!result.terminated
        &&result.rewards[0]==0&&result.rewards[1]==0&&result.rewards[2]==0&&!has_token(effect_match,77,30)
        &&observe(effect_match,0,0).find("\"pending\":null")!=std::string::npos&&observe(effect_match,0,0).find("\"death_created\"")==std::string::npos,
        "cutoff retains boundary consumption but cancels activation and hides reserved posthumous work");gfo_match_destroy(effect_match);
    // Full trade delivery evicts an actual existing copy, never a replacement draw or passive activation.
    cfg.initial_hand = cfg.hand_limit = 18; cfg.initial_hp = 40; cfg.initial_mp = 0; cfg.initial_cp = 99; cfg.max_decisions = 512;
    check(gfo_match_create(&cfg,players.data(),2, 395,&effect_match)==0,"full foreign sale overflow fixture");
    gfo_match_command overflow_sale{};overflow_sale.kind=5;overflow_sale.item_count=2;overflow_sale.items[0]=0;overflow_sale.items[1]=15;overflow_sale.has_target=1;overflow_sale.target=77;
    const auto before_overflow=observe(effect_match);check(gfo_match_legal(effect_match,&overflow_sale,&legal)==0&&legal&&observe(effect_match)==before_overflow,"full recipient is not a legal-query oracle");
    gfo_match_command overflow_pass{};overflow_pass.kind=1;overflow_pass.actor=77;
    check(gfo_match_step(effect_match,&overflow_sale,&result)==0&&gfo_match_step(effect_match,&overflow_pass,&result)==0
        &&observe(effect_match,0,0).find("\"last_sale\":{\"origin\":0,\"source\":0,\"target\":77,\"model\":153,\"price\":10,\"overflow_model\":181}")!=std::string::npos
        &&!has_token(effect_match,77,19)&&has_token(effect_match,77,36)&&hand_size(effect_match,0)==17&&hand_size(effect_match,77)==18,
        "uniform overflow evicts token33/model4 then delivers fresh153, with only the operation gift");
    gfo_match_destroy(effect_match);cfg.initial_mp=99;cfg.initial_cp=20;
    check(gfo_match_create(&cfg,players.data(),2, 7786,&effect_match)==0,"used miracle overflow fixture");
    gfo_match_command overflow_setup{};overflow_setup.kind=2;overflow_setup.item_count=1;overflow_setup.items[0]=2;
    check(gfo_match_step(effect_match,&overflow_setup,&result)==0,"yield without an extra gift draw");
    overflow_setup.kind=0;overflow_setup.actor=77;overflow_setup.items[0]=24;overflow_setup.has_target=1;overflow_setup.target=77;
    check(gfo_match_step(effect_match,&overflow_setup,&result)==0,"retain235 at full capacity");
    overflow_sale.items[0]=12;overflow_sale.items[1]=5;
    check(gfo_match_step(effect_match,&overflow_sale,&result)==0&&gfo_match_step(effect_match,&overflow_pass,&result)==0
        &&!has_token(effect_match,77,24)&&hand_size(effect_match,0)==16&&hand_size(effect_match,77)==18
        &&observe(effect_match,0,0).find("\"overflow_model\":235")!=std::string::npos
        &&observe(effect_match,1,77).find("\"used_models\":[235]")==std::string::npos,
        "used235 is eligible for overflow without a compensating gift");
    const auto evicted_view=observe(effect_match,1,77);result=status(effect_match);const auto evicted_status=result;
    check(gfo_match_step(effect_match,&overflow_setup,&result)==-2&&equal(result,evicted_status)&&observe(effect_match,1,77)==evicted_view,"evicted retained token cannot be recast");
    gfo_match_destroy(effect_match);cfg.initial_hp=5;cfg.initial_mp=cfg.initial_cp=0;
    check(gfo_match_create(&cfg,players.data(),2, 83881,&effect_match)==0,"amulet eviction before death fixture");
    overflow_sale.items[0]=11;overflow_sale.items[1]=6;
    check(gfo_match_step(effect_match,&overflow_sale,&result)==0&&gfo_match_step(effect_match,&overflow_pass,&result)==0
        &&result.terminated&&result.winners[0]==0&&!has_token(effect_match,77,26)&&has_token(effect_match,77,36)
        &&hand_size(effect_match,0)==16&&observe(effect_match,0,0).find("\"model\":38,\"price\":5,\"overflow_model\":208")!=std::string::npos,
        "CP debt then amulet eviction and nonpassive delivery precede death, without revival or terminal gifts");
    gfo_match_destroy(effect_match);cfg.initial_hp=40;cfg.initial_mp=99;cfg.initial_cp=20;fixture.reset(setup, 487975);
    check(gfo_match_create(&cfg,players.data(),2, 487975,&effect_match)==0,"Fog foreign sale overflow fixture");
    gfo_match_command overflow_fog{};overflow_fog.item_count=1;overflow_fog.items[0]=receipt_token(0,221);overflow_fog.has_target=1;
    check(gfo_match_step(effect_match,&overflow_fog,&result)==0,"establish Fog before foreign sale");
    overflow_setup.kind=0;overflow_setup.actor=77;overflow_setup.items[0]=receipt_token(1,235);overflow_setup.has_target=1;overflow_setup.target=77;
    check(gfo_match_step(effect_match,&overflow_setup,&result)==0,"keep covered recipient full with retained miracle");
    overflow_sale.items[0]=receipt_token(0,4);overflow_sale.items[1]=receipt_token(0,6);
    check(gfo_match_legal(effect_match,&overflow_sale,&legal)==0&&legal&&gfo_match_step(effect_match,&overflow_sale,&result)==0
        &&gfo_match_step(effect_match,&overflow_pass,&result)==0&&hand_size(effect_match,77)==18
        &&observe(effect_match).find("\"status\":null")!=std::string::npos
        &&observe(effect_match).find("\"overflow_model\":null")==std::string::npos
        &&observe(effect_match).find("\"last_fog\"")==std::string::npos,
        "Fog foreign sale resolves overflow publicly while covered resources and launch record remain hidden");
    gfo_match_destroy(effect_match);
    // Dream exposes a compatible face to queries, then reveals the actual body.
    cfg.initial_hand=17;cfg.hand_limit=18;cfg.initial_hp=40;cfg.initial_mp=10;cfg.initial_cp=20;cfg.max_decisions=512;
    check(gfo_match_create(&cfg,players.data(),2, 7614,&effect_match)==0
        &&observe(effect_match).find(official::dream_rule)!=std::string::npos,"Dream explicit experimental contract");
    gfo_match_command dream_cast{};dream_cast.item_count=1;dream_cast.items[0]=15;dream_cast.has_target=1;
    check(gfo_match_step(effect_match,&dream_cast,&result)==0&&result.actor==77
        &&observe(effect_match).find("\"token\":34,\"face\":8,\"reference\":8,\"disguised\":true,\"used\":false")!=std::string::npos
        &&observe(effect_match).find("\"last_reveals\":[]")!=std::string::npos,"Dream masks only a newly gifted eligible instance");
    gfo_match_command dream_yield{};dream_yield.actor=77;dream_yield.kind=2;dream_yield.item_count=1;dream_yield.items[0]=20;
    check(gfo_match_step(effect_match,&dream_yield,&result)==0,"yield without extra gift RNG before fake use");
    const auto masked_view=observe(effect_match);const auto masked_status=status(effect_match);
    gfo_match_command fake_attack{};fake_attack.item_count=1;fake_attack.items[0]=34;fake_attack.has_target=1;fake_attack.target=77;
    check(gfo_match_legal(effect_match,&fake_attack,&legal)==0&&legal,"compatible displayed weapon has a legal command");
    for(uint64_t policy_seed=0;policy_seed<64;++policy_seed){uint64_t policy=policy_seed;gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match,&policy,&candidate)==0&&gfo_match_legal(effect_match,&candidate,&legal)==0&&legal,"Dream sampler emits legal visible-information actions");}
    check(observe(effect_match)==masked_view&&equal(status(effect_match),masked_status),"Dream queries preserve disguise and environment");
    check(gfo_match_step(effect_match,&fake_attack,&result)==0&&result.actor==77
        &&observe(effect_match).find("\"models\":[26]")!=std::string::npos
        &&observe(effect_match).find("\"power\":7")!=std::string::npos,"face13 ATK3 executes actual23 ATK6");
    const std::string public_reveal="\"last_reveals\":[{\"owner\":0,\"face\":8,\"model\":26}]";
    check(observe(effect_match).find(public_reveal)!=std::string::npos
        &&observe(effect_match,0,0).find(public_reveal)!=std::string::npos
        &&observe(effect_match,1,77).find(public_reveal)!=std::string::npos,"used fake public reveal contains models and no instance token");
    const auto fake_pending=observe(effect_match);const auto fake_pending_status=status(effect_match);result=fake_pending_status;
    check(gfo_match_step(effect_match,&fake_attack,&result)==-2&&equal(result,fake_pending_status)&&observe(effect_match)==fake_pending,"failed fake reuse preserves public reveal and pending body");
    gfo_match_command dream_pass{};dream_pass.actor=77;dream_pass.kind=1;
    check(gfo_match_step(effect_match,&dream_pass,&result)==0
        &&observe(effect_match).find("\"hp\":33,\"mp\":10")!=std::string::npos
        &&observe(effect_match).find(public_reveal)!=std::string::npos,"response preserves fixed actual attack and reveal history");
    gfo_match_destroy(effect_match);
    // Clearing a hidden hand reveals only to that hand's owner, not spectators.
    check(gfo_match_create(&cfg,players.data(),2, 7614,&effect_match)==0
        &&gfo_match_step(effect_match,&dream_cast,&result)==0
        &&gfo_match_step(effect_match,&dream_yield,&result)==0,"Dream private cure fixture");
    gfo_match_command dream_cure{};dream_cure.item_count=1;dream_cure.items[0]=4;dream_cure.has_target=1;
    check(gfo_match_step(effect_match,&dream_cure,&result)==0
        &&observe(effect_match).find(public_reveal)!=std::string::npos
        &&observe(effect_match).find("\"token\":34,\"face\":26,\"reference\":26,\"disguised\":false")!=std::string::npos
        &&observe(effect_match).find("\"curses\":[\"dream\"]")==std::string::npos,"all cure clears Dream and unmasks the owner's remaining hand");
    check(observe(effect_match,0,0).find("\"last_reveals\":[]")!=std::string::npos
        &&observe(effect_match,1,77).find("\"last_reveals\":[]")!=std::string::npos,"all-cure hand flips leak neither model nor count to foreign views");
    check(gfo_match_reset(effect_match,players.data(),2, 7614,&result)==0
        &&observe(effect_match).find("\"last_reveals\":[]")!=std::string::npos,"reset clears reveal events");
    gfo_match_destroy(effect_match);
    // Redraw replaces real inventory immediately, including an already-retained miracle.
    cfg.initial_hand=4;cfg.initial_mp=10;cfg.initial_hp=40;cfg.initial_cp=20;cfg.hand_limit=18;cfg.max_decisions=512;
    check(gfo_match_supported(178)&&!gfo_match_supported(207)
        &&gfo_match_create(&cfg,players.data(),2, 703521,&effect_match)==0
        &&observe(effect_match).find(official::redraw_rule)!=std::string::npos,"redraw178 explicit scoped membership and contract");
    gfo_match_command hat_yield{};hat_yield.kind=2;hat_yield.item_count=1;hat_yield.items[0]=3;
    check(gfo_match_step(effect_match,&hat_yield,&result)==0,"redraw setup yields without consuming game RNG");
    gfo_match_command manual_hat{};manual_hat.actor=77;manual_hat.item_count=1;manual_hat.items[0]=6;manual_hat.has_target=1;manual_hat.target=77;
    const auto before_manual_hat=observe(effect_match,1,77);const auto manual_hat_status=status(effect_match);result=manual_hat_status;
    check(gfo_match_legal(effect_match,&manual_hat,&legal)==0&&!legal
        &&gfo_match_step(effect_match,&manual_hat,&result)==-2&&equal(result,manual_hat_status)
        &&observe(effect_match,1,77)==before_manual_hat,"armor cannot trigger redraw as a main command");
    manual_hat.items[0]=7;
    check(gfo_match_step(effect_match,&manual_hat,&result)==0&&hand_size(effect_match,77)==5
        &&observe(effect_match,1,77).find("\"used_models\":[235]")!=std::string::npos,"retained Spring and its deferred gift precede redraw");
    gfo_match_command hat_attack{};hat_attack.item_count=1;hat_attack.items[0]=1;hat_attack.has_target=1;hat_attack.target=77;
    check(gfo_match_step(effect_match,&hat_attack,&result)==0,"redraw receives a normal uncolored attack");
    gfo_match_command hat_response{};hat_response.actor=77;hat_response.kind=1;hat_response.item_count=1;hat_response.items[0]=6;
    check(gfo_match_step(effect_match,&hat_response,&result)==0&&result.phase==0&&result.actor==77
        &&hand_size(effect_match,77)==5&&hand_size(effect_match,0)==3
        &&!has_token(effect_match,77,7)&&!has_token(effect_match,77,8),"used MC and its original gift are replaced without cancelling reserved gifts");
    for(auto token:{9u,10u,11u,12u,14u})check(has_token(effect_match,77,token),"redraw uses fresh monotonically allocated tokens");
    for(auto viewer:{0u,77u})check(observe(effect_match,1,viewer).find("\"last_redraws\":[77]")!=std::string::npos,"redraw publicly exposes only the affected owner ID");
    const auto redraw_public=observe(effect_match,0,0);
    check(redraw_public.find("\"last_redraws\":[77]")!=std::string::npos
        &&redraw_public.find("\"last_reveals\":[]")!=std::string::npos
        &&redraw_public.find("\"redrawn_items\"")==std::string::npos
        &&redraw_public.find("\"redraw_models\"")==std::string::npos
        &&observe(effect_match,1,77).find("\"used\":true")==std::string::npos,"replacement does not publish old hand contents or preserve used status");
    gfo_match_destroy(effect_match);
    // Self BUY is inspection, never an alias of foreign escrow/payment/delivery.
    cfg.initial_hand=4;cfg.initial_mp=0;cfg.initial_cp=0;cfg.initial_hp=40;cfg.hand_limit=18;cfg.max_decisions=512;
    check(gfo_match_create(&cfg,players.data(),2, 3440,&effect_match)==0,"self inspection CP0 fixture");
    gfo_match_command inspect_self{};inspect_self.kind=6;inspect_self.item_count=1;inspect_self.items[0]=2;inspect_self.has_target=1;
    const auto before_inspection=observe(effect_match);const auto inspection_status=status(effect_match);
    check(gfo_match_legal(effect_match,&inspect_self,&legal)==0&&legal&&observe(effect_match)==before_inspection,"self inspection legal query is independent of item price");
    bool sample_self_buy=false;
    for(uint64_t seed=0;seed<128;++seed){uint64_t policy=seed;gfo_match_command candidate{};
        check(gfo_match_sample_action(effect_match,&policy,&candidate)==0&&gfo_match_legal(effect_match,&candidate,&legal)==0&&legal,"self-inclusive BUY sampler stays legal");
        sample_self_buy|=candidate.kind==6&&candidate.has_target&&candidate.target==0;}
    check(sample_self_buy&&observe(effect_match)==before_inspection&&equal(status(effect_match),inspection_status),"self inspection sampler mutates only policy RNG");
    check(gfo_match_step(effect_match,&inspect_self,&result)==0&&result.phase==0&&result.actor==77&&result.decisions==1
        &&hand_size(effect_match,0)==4&&!has_token(effect_match,0,2)&&has_token(effect_match,0,0)&&has_token(effect_match,0,8),"CP0 self BUY keeps its selected physical copy and gifts only the operation");
    const std::string inspection_event="\"last_purchase\":{\"origin\":0,\"buyer\":0,\"seller\":0,\"model\":195,\"price\":1,\"bought\":false,\"reason\":\"self_inspection\",\"overflow_model\":null}";
    for(auto viewer:{0u,77u})check(observe(effect_match,1,viewer).find(inspection_event)!=std::string::npos,"self-selected actual model and price are public");
    const auto after_inspection=observe(effect_match);const auto after_inspection_status=status(effect_match);
    check(observe(effect_match,0,0).find(inspection_event)!=std::string::npos
        &&after_inspection.find("\"purchase\":null")!=std::string::npos
        &&after_inspection.find("\"pending\":null")!=std::string::npos
        &&after_inspection.find("\"token\":3,\"face\":195,\"reference\":195,\"disguised\":false,\"used\":false")!=std::string::npos
        &&after_inspection.find("\"cp\":0")!=std::string::npos,"self inspection creates no quote or transfer and keeps unused state");
    for(uint8_t kind:{uint8_t{7},uint8_t{8}}){gfo_match_command invalid{};invalid.actor=77;invalid.kind=kind;result=after_inspection_status;
        check(gfo_match_legal(effect_match,&invalid,&legal)==0&&!legal&&gfo_match_step(effect_match,&invalid,&result)==-2
            &&equal(result,after_inspection_status)&&observe(effect_match)==after_inspection,"self inspection cannot accept or decline and rejects atomically");}
    gfo_match_destroy(effect_match);
    // Empty inspection does not draw a selection ticket, and cutoff does not undo its result.
    cfg.initial_hand=1;inspect_self.items[0]=0;
    check(gfo_match_create(&cfg,players.data(),2, 3049,&effect_match)==0
        &&gfo_match_create(&cfg,players.data(),2, 3049,&effect_replay)==0,"empty self inspection/control pair");
    gfo_match_command empty_control{};empty_control.kind=3;
    check(gfo_match_step(effect_match,&inspect_self,&result)==0&&gfo_match_step(effect_replay,&empty_control,&result)==0
        &&observe(effect_match).find("\"buyer\":0,\"seller\":0,\"model\":0,\"price\":0,\"bought\":false,\"reason\":\"empty\"")!=std::string::npos,"consuming the sole BUY precedes empty selection");
    const auto gift_entry=[&](gfo_match* match){const auto view=observe(match);const auto start=view.find("\"token\":2,");
        check(start!=std::string::npos,"same next gift token exists");return view.substr(start,view.find('}',start)-start+1);};
    check(gift_entry(effect_match)==gift_entry(effect_replay)&&hand_size(effect_match,0)==1,"empty inspection and prayer use the same single gift draw");
    gfo_match_destroy(effect_match);gfo_match_destroy(effect_replay);
    cfg.initial_hand=4;cfg.max_decisions=1;inspect_self.items[0]=2;
    check(gfo_match_create(&cfg,players.data(),2, 3440,&effect_match)==0&&gfo_match_step(effect_match,&inspect_self,&result)==0
        &&result.truncated&&!result.terminated&&hand_size(effect_match,0)==3&&!has_token(effect_match,0,8)
        &&observe(effect_match).find(inspection_event)!=std::string::npos,"self inspection resolves before cutoff, which cancels only deferred gift");
    gfo_match_destroy(effect_match);
    // Self inspection reveals the actual price/model but does not remove the disguised instance.
    cfg.initial_hand=17;cfg.initial_mp=10;cfg.max_decisions=512;
    check(gfo_match_create(&cfg,players.data(),2, 16297,&effect_match)==0,"masked self-inspection fixture");
    gfo_match_command inspected_dream{};inspected_dream.item_count=1;inspected_dream.items[0]=9;inspected_dream.has_target=1;
    gfo_match_command inspected_yield{};inspected_yield.actor=77;inspected_yield.kind=2;inspected_yield.item_count=1;inspected_yield.items[0]=18;
    check(gfo_match_step(effect_match,&inspected_dream,&result)==0&&gfo_match_step(effect_match,&inspected_yield,&result)==0
        &&observe(effect_match).find("\"token\":34,\"face\":88,\"reference\":88,\"disguised\":true")!=std::string::npos,"Dream prepares a visibly masked own commodity");
    inspect_self.items[0]=14;
    check(gfo_match_step(effect_match,&inspect_self,&result)==0&&result.phase==0&&result.actor==77
        &&hand_size(effect_match,0)==18&&has_token(effect_match,0,34)&&has_token(effect_match,0,35)&&!has_token(effect_match,0,14)
        &&observe(effect_match).find("\"token\":34,\"face\":84,\"reference\":84,\"disguised\":false,\"used\":false")!=std::string::npos,"inspection unmasks the same physical token then gives one normal replacement");
    for(auto viewer:{0u,77u})check(observe(effect_match,1,viewer).find("\"last_reveals\":[{\"owner\":0,\"face\":88,\"model\":84}]")!=std::string::npos
        &&observe(effect_match,1,viewer).find("\"buyer\":0,\"seller\":0,\"model\":84,\"price\":10")!=std::string::npos,"masked self inspection publicly reports actual model and price without a token");
    gfo_match_destroy(effect_match);
    // Immutable typed states initialize independent execution scenes; guardian
    // inputs use an additive24-byte API, leaving ordinary ABI2 commands unchanged.
    cfg.initial_hand=0;cfg.initial_hp=99;cfg.initial_mp=99;cfg.initial_cp=10;cfg.max_decisions=64;
    gfo_match* guardian_match=nullptr;
    check(gfo_match_create(&cfg,players.data(),2, 534,&guardian_match)==0,"external guardian match create");
    check(!gfo_match_guardian_supported(244)&&!gfo_match_guardian_supported(287)
        &&!gfo_match_supported(245)&&gfo_match_catalog_count()==236,"guardian action namespace is separate from weighted gifts");
    for(uint16_t model=245;model<=286;++model){
        const auto& card=*official::find_card(model);
        check(gfo_match_guardian_supported(model),"all42 canonical guardian actions are advertised");
        std::array<gfo_state_player,3> scene_players{};
        char scene_name[]="scene-copy";
        for(std::size_t seat=0;seat<scene_players.size();++seat){auto& player=scene_players[seat];
            player.id=seat==0?0:(seat==1?7:9);player.hp=99;player.mp=99;player.cp=10;}
        scene_players[0].name=scene_name;scene_players[0].name_size=sizeof(scene_name)-1;
        scene_players[1].guardian=static_cast<uint8_t>(card.guardian);
        gfo_state_meta scene_meta{};scene_meta.gf=31;
        gfo_state* scene=nullptr;
        check(gfo_state_create(&scene_meta,scene_players.data(),3,nullptr,0,nullptr,0,&scene)==0,"typed guardian scene input");
        scene_name[0]='X';
        check(gfo_match_load_scene(guardian_match,scene,0,model,&result)==0&&result.phase==0&&result.actor==0
            &&result.owner==0&&result.decisions==0&&result.player_count==3,"quiescent scene initializes counters and preserves arbitrary IDs");
        if(model==245){
            const auto initial_board=observe(guardian_match);gfo_state_active_attack active{};const uint16_t public_model=6;
            active.models=&public_model;active.model_count=1;active.player_id=7;gfo_state* pending=nullptr;
            check(gfo_state_with_active_attack(scene,&active,&pending)==0,"attach a public frame independently of board-only scene");
            gfo_match_status unchanged;std::memset(&unchanged,0x6b,sizeof(unchanged));const auto sentinel=unchanged;
            check(gfo_match_load_scene(guardian_match,pending,0,model,&unchanged)==-5&&equal(unchanged,sentinel)
                &&observe(guardian_match)==initial_board,"C scene import refuses a pending public attack without mutating Match or outputs");
            gfo_state_destroy(pending);
        }
        gfo_state_destroy(scene);
        const auto before_guardian=observe(guardian_match);
        check(before_guardian.find("scene-copy")!=std::string::npos&&before_guardian.find("\"last_guardian\":null")!=std::string::npos,
            "loaded Match owns a deep copy after caller storage and state handle disappear");
        gfo_match_guardian_action action{};action.source=7;action.model=model;action.has_target=model!=280;
        check(gfo_match_guardian_legal(guardian_match,&action,&legal)==0&&legal&&observe(guardian_match)==before_guardian,"external guardian preflight is pure");
        if(model==245){
            uint64_t policy=19;gfo_match_command ordinary{};
            check(gfo_match_sample_action(guardian_match,&policy,&ordinary)==0&&ordinary.kind==3&&observe(guardian_match)==before_guardian,"normal policy never injects an autonomous guardian");
            auto bad=action;bad.reserved[0]=1;uint8_t marker=0xa5;
            gfo_match_status sentinel_status;std::memset(&sentinel_status,0x6b,sizeof(sentinel_status));const auto sentinel_before=sentinel_status;
            check(gfo_match_guardian_legal(guardian_match,&bad,&marker)==-2&&marker==0xa5
                &&gfo_match_guardian_step(guardian_match,&bad,&sentinel_status)==-2&&equal(sentinel_status,sentinel_before)
                &&observe(guardian_match)==before_guardian,"malformed guardian padding changes no state or output");
            bad=action;bad.has_target=0;bad.target=7;
            check(gfo_match_guardian_legal(guardian_match,&bad,&marker)==-2&&marker==0xa5,"absent guardian target must be zero");
            bad=action;bad.model=201;
            check(gfo_match_guardian_legal(guardian_match,&bad,&marker)==0&&!marker
                &&gfo_match_guardian_step(guardian_match,&bad,&sentinel_status)==-5&&equal(sentinel_status,sentinel_before)
                &&observe(guardian_match)==before_guardian,"unsupported summon is not a canonical external action");
            check(gfo_match_load_scene(guardian_match,nullptr,0,1,&sentinel_status)==-1
                &&gfo_match_guardian_step(guardian_match,&action,nullptr)==-1
                &&gfo_match_guardian_legal(guardian_match,nullptr,&marker)==-1
                &&equal(sentinel_status,sentinel_before)&&observe(guardian_match)==before_guardian,"new API NULL guards are atomic");
            // A valid read-only catalog scene can still be outside execution scope.
            gfo_state_item unsupported_item{};unsupported_item.model=1;
            scene_players[0].name="safe";scene_players[0].name_size=4;scene_players[0].item_count=1;
            scene_players[1].item_offset=1;scene_players[2].item_offset=1;
            check(gfo_state_create(&scene_meta,scene_players.data(),3,&unsupported_item,1,nullptr,0,&scene)==0,"read-only state accepts a known summon model");
            check(gfo_match_load_scene(guardian_match,scene,0,1,&sentinel_status)==-5&&equal(sentinel_status,sentinel_before)
                &&observe(guardian_match)==before_guardian,"unsupported execution import preserves previous scene and output");
            gfo_state_destroy(scene);
        }
        check(gfo_match_guardian_step(guardian_match,&action,&result)==0,"canonical external guardian action crosses C ABI");
        unsigned responses=0;
        while(result.phase==1){check(++responses<=3,"canonical guardian batch completes finitely");
            const auto pending=observe(guardian_match,0,0);
            check(pending.find("\"models\":["+std::to_string(model)+"]")!=std::string::npos,"pending guardian body retains the canonical model");
            gfo_match_command pass{};pass.kind=1;pass.actor=result.actor;
            check(gfo_match_step(guardian_match,&pass,&result)==0,"ordinary response resolves external guardian body");}
        const auto completed=observe(guardian_match,0,0);
        const std::string event="\"last_guardian\":{\"source\":7,\"model\":"+std::to_string(model)+",\"guardian\":\""+std::string(official::guardian_name(card.guardian))+"\",\"target\":"+(model==280?std::string("null"):std::string("0"))+"}";
        check(result.phase==0&&completed.find("\"gf\":31")!=std::string::npos&&completed.find(event)!=std::string::npos
            &&completed.find("\"guardian_actions\"")==std::string::npos,"guardian result resumes ordinary play without GF advance or private scheduler counters");
    }
    gfo_match_destroy(guardian_match);
    // Automatic work stays in the ordinary action and exposes only selected
    // models; the opaque API must preserve owner, tick and deferred gift timing.
    cfg.initial_hand=0;cfg.max_decisions=64;cfg.initial_hp=40;cfg.initial_mp=10;cfg.initial_cp=20;
    check(gfo_match_create(&cfg,players.data(),2,0,&guardian_match)==0,"automatic guardian bridge create");
    const auto load_guardian_scene=[&](bool departure){
        std::array<gfo_state_player,2> people{};
        people[0].id=0;people[1].id=7;
        for(auto& person:people){person.hp=40;person.mp=10;person.cp=20;}
        people[1].guardian=static_cast<uint8_t>(departure?official::Guardian::mars:official::Guardian::moon);
        people[0].curse_count=1;people[1].curse_offset=1;
        const uint8_t curse=static_cast<uint8_t>(departure?official::Curse::fog:official::Curse::cold);
        gfo_state_item club{};club.token=10;club.model=6;
        people[0].item_count=departure?1:0;people[1].item_offset=departure?1:0;
        gfo_state_meta meta{};meta.gf=7;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,people.data(),2,departure?&club:nullptr,departure?1:0,&curse,1,&scene)==0,"typed automatic/departure scene create");
        check(gfo_match_load_scene(guardian_match,scene,0,0,&result)==0,"typed automatic/departure scene load");gfo_state_destroy(scene);
    };
    load_guardian_scene(false);
    gfo_match_command owner_pray{};owner_pray.kind=3;
    check(gfo_match_step(guardian_match,&owner_pray,&result)==0&&result.phase==1&&result.actor==0&&result.owner==0&&result.decisions==1,
        "automatic Moon produces response without an activation decision or owner change");
    auto auto_view=observe(guardian_match);
    const std::string selected_moon="\"last_auto_guardians\":[{\"source\":7,\"model\":286,\"guardian\":\"moon\",\"target\":0}]";
    check(auto_view.find(selected_moon)!=std::string::npos&&auto_view.find("\"hp\":39,")!=std::string::npos
        &&auto_view.find("\"hand\":[]")!=std::string::npos&&auto_view.find("\"gf\":7")!=std::string::npos,
        "owner disease tick runs once before auto and main prayer gift remains deferred");
    gfo_match_command auto_pass{};auto_pass.kind=1;
    check(gfo_match_step(guardian_match,&auto_pass,&result)==0&&result.phase==0&&result.actor==7&&result.owner==7&&result.decisions==2
        &&observe(guardian_match).find("\"hp\":29,")!=std::string::npos&&observe(guardian_match).find("\"gf\":8")!=std::string::npos,
        "automatic body completes before a single GF advance and does not repeat owner tick");
    check(observe(guardian_match).find("guardian_opportunities")==std::string::npos&&observe(guardian_match).find("guardian_departure_rolls")==std::string::npos,
        "private scheduling counters never cross JSON");
    load_guardian_scene(true);
    gfo_match_command fog_club{};fog_club.item_count=1;fog_club.items[0]=10;fog_club.has_target=1;fog_club.target=7;
    check(gfo_match_step(guardian_match,&fog_club,&result)==0&&observe(guardian_match).find("\"last_guardian_changes\":[]")!=std::string::npos,
        "unresolved damage publishes no departure");
    auto_pass.actor=7;
    check(gfo_match_step(guardian_match,&auto_pass,&result)==0,"positive direct HP loss resolves departure draw");
    const std::string departed="\"last_guardian_changes\":[{\"player\":7,\"before\":\"mars\",\"after\":null,\"reason\":\"departure\"}]";
    check(observe(guardian_match,0,0).find(departed)!=std::string::npos&&observe(guardian_match,1,7).find(departed)!=std::string::npos
        &&observe(guardian_match,1,0).find("\"last_guardian_changes\":[]")!=std::string::npos,
        "event-time Fog filters the whole change entry for hidden viewers only");
    const auto departed_view=observe(guardian_match);const auto departed_status=status(guardian_match);auto_pass.actor=0;
    result=departed_status;
    check(gfo_match_step(guardian_match,&auto_pass,&result)==-2&&equal(result,departed_status)&&observe(guardian_match)==departed_view,
        "invalid later input preserves departure event and status");
    gfo_match_destroy(guardian_match);
    // Additive64-byte payload input carries catalog models without inventing
    // inventory tokens or changing the established command/status ABI2 widths.
    check(!gfo_match_guardian_payload_supported(0,6)&&!gfo_match_guardian_payload_supported(255,6)
        &&!gfo_match_guardian_payload_supported(9,3)&&!gfo_match_guardian_payload_supported(9,94)
        &&!gfo_match_guardian_payload_supported(10,231)&&!gfo_match_guardian_payload_supported(10,232),"payload membership excludes unsupported roles and families");
    unsigned earth_count=0,moon_count=0;
    for(uint16_t model=0;model<=296;++model){earth_count+=gfo_match_guardian_payload_supported(9,model);moon_count+=gfo_match_guardian_payload_supported(10,model);}
    check(earth_count==120&&moon_count==26,"separate standalone Earth and Moon capabilities");
    cfg.initial_hand=0;cfg.max_decisions=64;
    gfo_match* payload_match=nullptr;
    check(gfo_match_create(&cfg,players.data(),2,0,&payload_match)==0,"payload bridge create");
    const auto load_payload_scene=[&](official::Guardian guardian,uint64_t update){
        std::array<gfo_state_player,2> people{};
        for(auto& person:people){person.hp=99;person.mp=10;person.cp=20;}
        people[0].id=0;people[0].item_count=1;people[1].id=7;people[1].item_offset=1;people[1].guardian=static_cast<uint8_t>(guardian);
        gfo_state_item mirror{};mirror.token=10;mirror.model=190;
        gfo_state_meta meta{};meta.gf=12;meta.update_count=update;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,people.data(),2,&mirror,1,nullptr,0,&scene)==0,"payload typed scene create");
        check(gfo_match_load_scene(payload_match,scene,0,0,&result)==0,"payload scene load");gfo_state_destroy(scene);
    };
    load_payload_scene(official::Guardian::earth,0);
    gfo_match_guardian_payload_action payload{};payload.source=7;payload.guardian=9;payload.model_count=3;
    payload.models[0]=58;payload.models[1]=203;payload.models[2]=83;payload.has_target=1;
    const auto before_payload=observe(payload_match,1,7);const auto payload_status=status(payload_match);
    for(unsigned error=0;error<10;++error){auto bad=payload;
        if(error==0)bad.reserved[0]=1;if(error==1)bad.reserved[8]=1;if(error==2)bad.model_count=0;
        if(error==3)bad.model_count=19;if(error==4)bad.has_target=2;if(error==5){bad.has_target=0;bad.target=1;}
        if(error==6)bad.models[17]=6;if(error==7)bad.guardian=8;
        if(error==8)bad.source=official::max_wire_integer+1;if(error==9)bad.target=official::max_wire_integer+1;
        uint8_t marker=0xa5;result=payload_status;
        check(gfo_match_guardian_payload_legal(payload_match,&bad,&marker)==-2&&marker==0xa5
            &&gfo_match_guardian_payload_step(payload_match,&bad,&result)==-2&&equal(result,payload_status)
            &&observe(payload_match,1,7)==before_payload,"payload malformed input preserves marker, status and entire view");
    }
    result=payload_status;uint8_t marker=0xa5;
    check(gfo_match_guardian_payload_legal(nullptr,&payload,&marker)==-1
        &&gfo_match_guardian_payload_legal(payload_match,nullptr,&marker)==-1
        &&gfo_match_guardian_payload_legal(payload_match,&payload,nullptr)==-1
        &&gfo_match_guardian_payload_step(payload_match,nullptr,&result)==-1
        &&gfo_match_guardian_payload_step(payload_match,&payload,nullptr)==-1
        &&marker==0xa5&&equal(result,payload_status)&&observe(payload_match,1,7)==before_payload,"payload NULL guards are atomic");
    check(gfo_match_guardian_payload_legal(payload_match,&payload,&legal)==0&&legal
        &&observe(payload_match,1,7)==before_payload,"supplied composition legality uses no source inventory and has no side effect");
    check(gfo_match_guardian_payload_step(payload_match,&payload,&result)==0&&result.owner==0&&result.actor==0&&result.decisions==1,
        "payload source remains separate from ordinary turn owner");
    const std::string payload_event="\"last_guardian_payload\":{\"source\":7,\"guardian\":\"earth\",\"models\":[58,203,83],\"target\":0}";
    check(observe(payload_match,0,0).find(payload_event)!=std::string::npos
        &&observe(payload_match,0,0).find("\"power\":35")!=std::string::npos
        &&observe(payload_match,0,0).find("\"element\":\"water\"")!=std::string::npos
        &&observe(payload_match,1,7).find("\"hand\":[]")!=std::string::npos
        &&observe(payload_match,0,0).find("\"mp\":0")!=std::string::npos,"MP weapon burns launch source MP once and publishes actual ordered composition");
    gfo_match_command payload_defense{};payload_defense.kind=1;payload_defense.item_count=1;payload_defense.items[0]=10;
    check(gfo_match_step(payload_match,&payload_defense,&result)==0&&result.actor==7
        &&observe(payload_match,0,0).find("\"power\":35")!=std::string::npos,"redirect retains precomputed payload after source MP becomes zero");
    payload_defense.actor=7;payload_defense.item_count=0;payload_defense.items[0]=0;
    check(gfo_match_step(payload_match,&payload_defense,&result)==0&&result.phase==0&&result.owner==0&&result.actor==0
        &&observe(payload_match,0,0).find("\"hp\":64")!=std::string::npos&&observe(payload_match,0,0).find("\"gf\":12")!=std::string::npos
        &&observe(payload_match,1,7).find("\"hand\":[]")!=std::string::npos,"response completion preserves owner/GF and gives no source operation gift or retention");
    load_payload_scene(official::Guardian::moon,official::max_wire_integer);
    payload={};payload.source=7;payload.guardian=10;payload.models[0]=235;payload.model_count=1;payload.target=7;payload.has_target=1;
    const auto late_view=observe(payload_match,1,7);const auto late_status=status(payload_match);result=late_status;
    check(gfo_match_guardian_payload_legal(payload_match,&payload,&legal)==0&&legal
        &&gfo_match_guardian_payload_step(payload_match,&payload,&result)==-2&&equal(result,late_status)
        &&observe(payload_match,1,7)==late_view,"late update exhaustion rolls back self-heal and payload event across C ABI");
    gfo_match_destroy(payload_match);
    unsigned phenomenon_count=0;
    for(uint16_t model=0;model<=296;++model)phenomenon_count+=gfo_match_phenomenon_supported(model);
    check(phenomenon_count==9&&gfo_match_phenomenon_supported(287)&&gfo_match_phenomenon_supported(288)
        &&gfo_match_phenomenon_supported(289)&&gfo_match_phenomenon_supported(290)&&gfo_match_phenomenon_supported(291)
        &&gfo_match_phenomenon_supported(292)&&gfo_match_phenomenon_supported(293)&&gfo_match_phenomenon_supported(294)&&gfo_match_phenomenon_supported(296)
        &&!gfo_match_supported(207)&&gfo_match_catalog_count()==236,"phenomena are a separate capability and do not alter weighted gifts");
    gfo_match* pulse_match=nullptr;
    check(gfo_match_create(&cfg,players.data(),2,0,&pulse_match)==0,"phenomenon bridge create");
    std::array<gfo_state_player,9> pulse_people{};
    for(std::size_t i=0;i<9;++i){auto& person=pulse_people[i];person.id=i?100+i*7:0;person.hp=40;person.mp=20+i;person.cp=10+i;}
    pulse_people[8].dead=1;pulse_people[8].hp=0;pulse_people[8].cp=99;
    const auto load_pulse_scene=[&](uint64_t update){gfo_state_meta meta{};meta.gf=8;meta.update_count=update;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,pulse_people.data(),pulse_people.size(),nullptr,0,nullptr,0,&scene)==0,"typed source-free pulse scene");
        check(gfo_match_load_scene(pulse_match,scene,0,0,&result)==0,"load pulse scene");gfo_state_destroy(scene);};
    load_pulse_scene(0);
    gfo_match_phenomenon_action pulse{};pulse.model=294;pulse.has_recipient=1;pulse.recipient=pulse_people[3].id;
    const auto pulse_before=observe(pulse_match,0,0);const auto pulse_status=status(pulse_match);
    for(unsigned error=0;error<8;++error){auto bad=pulse;
        if(error==0)bad.reserved[0]=1;if(error==1)bad.reserved[2]=1;if(error==2)bad.has_recipient=2;
        if(error==3)bad.has_recipient=0;if(error==4)bad.guardian_count=10;
        if(error==5)bad.guardians[8]=1;if(error==6){bad.guardian_count=1;bad.guardians[0]=11;}
        if(error==7)bad.recipient=official::max_wire_integer+1;
        uint8_t marker=0xa5;result=pulse_status;
        check(gfo_match_phenomenon_legal(pulse_match,&bad,&marker)==-2&&marker==0xa5
            &&gfo_match_phenomenon_step(pulse_match,&bad,&result)==-2&&equal(result,pulse_status)
            &&observe(pulse_match,0,0)==pulse_before,"phenomenon malformed wire changes no view, output or marker");}
    uint8_t pulse_marker=0xa5;result=pulse_status;
    check(gfo_match_phenomenon_legal(nullptr,&pulse,&pulse_marker)==-1
        &&gfo_match_phenomenon_legal(pulse_match,nullptr,&pulse_marker)==-1
        &&gfo_match_phenomenon_legal(pulse_match,&pulse,nullptr)==-1
        &&gfo_match_phenomenon_step(pulse_match,nullptr,&result)==-1
        &&gfo_match_phenomenon_step(pulse_match,&pulse,nullptr)==-1
        &&pulse_marker==0xa5&&equal(result,pulse_status)&&observe(pulse_match,0,0)==pulse_before,"phenomenon NULL guards are atomic");
    check(gfo_match_phenomenon_legal(pulse_match,&pulse,&legal)==0&&legal&&observe(pulse_match,0,0)==pulse_before
        &&gfo_match_phenomenon_step(pulse_match,&pulse,&result)==0&&result.phase==0&&result.actor==0&&result.owner==0&&result.decisions==1,
        "source-free collection is one external decision without owner advance");
    const auto collected=observe(pulse_match,0,0);
    check(collected.find("\"last_phenomenon\":{\"model\":294,\"recipient\":121}")!=std::string::npos
        &&collected.find("\"hp\":40,\"mp\":23,\"cp\":99")!=std::string::npos
        &&collected.find("\"hp\":0,\"mp\":28,\"cp\":99")!=std::string::npos
        &&collected.find("\"gf\":8")!=std::string::npos&&collected.find("cp_total")==std::string::npos,
        "collection caps the original alive CP sum, excludes dead CP, and publishes no aggregate");
    // Nine explicit assignments exercise the event bound independently of the
    // normal summon counter and the prior three-changes-per-decision bound.
    pulse_people[8].dead=0;pulse_people[8].hp=40;
    for(auto& person:pulse_people)person.guardian=static_cast<uint8_t>(official::Guardian::mars);
    load_pulse_scene(0);pulse={};pulse.model=296;pulse.guardian_count=9;
    for(std::size_t i=0;i<9;++i)pulse.guardians[i]=i==0?1:10;
    check(gfo_match_phenomenon_step(pulse_match,&pulse,&result)==0&&result.phase==0&&result.decisions==1,
        "all nine guardian assignments include same-type values without a redraw or operation gift");
    const auto assigned=observe(pulse_match,0,0);std::size_t changes=0,position=0;
    while((position=assigned.find("\"reason\":\"phenomenon\"",position))!=std::string::npos){++changes;++position;}
    check(changes==9&&assigned.find("\"last_phenomenon\":{\"model\":296,\"recipient\":null}")!=std::string::npos
        &&assigned.find("\"guardians\":")==std::string::npos&&assigned.find("phenomenon_actions")==std::string::npos,
        "assignment transport contains only the filtered per-player changes and source-free public event");
    load_pulse_scene(official::max_wire_integer);
    const auto late_pulse_view=observe(pulse_match,0,0);const auto late_pulse_status=status(pulse_match);result=late_pulse_status;
    check(gfo_match_phenomenon_legal(pulse_match,&pulse,&legal)==0&&legal
        &&gfo_match_phenomenon_step(pulse_match,&pulse,&result)==-2&&equal(result,late_pulse_status)
        &&observe(pulse_match,0,0)==late_pulse_view,"late update exhaustion rolls back all guardian changes and public events");
    gfo_match_destroy(pulse_match);
    // Targeted weather has no player source. PlayerID0 remains a real player
    // for ordinary and derived frames, never a sentinel for that absence.
    cfg.initial_hand=0;cfg.max_decisions=64;
    gfo_match* weather_match=nullptr;
    check(gfo_match_create(&cfg,players.data(),2,0,&weather_match)==0,"targeted phenomenon bridge create");
    const auto load_weather_scene=[&](int target_hp,uint64_t update){
        std::array<gfo_state_player,3> people{};
        people[0].id=0;people[1].id=7;people[2].id=23;
        for(auto& person:people){person.hp=99;person.mp=10;person.cp=20;}
        people[1].hp=target_hp;people[1].mp=0;people[1].item_count=3;people[2].item_offset=3;
        std::array<gfo_state_item,3> items{};
        items[0].token=10;items[0].model=189;items[1].token=11;items[1].model=187;items[2].token=12;items[2].model=190;
        gfo_state_meta meta{};meta.gf=8;meta.update_count=update;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,people.data(),people.size(),items.data(),items.size(),nullptr,0,&scene)==0,"typed source-free targeted scene");
        check(gfo_match_load_scene(weather_match,scene,0,0,&result)==0,"load targeted scene");gfo_state_destroy(scene);
    };
    load_weather_scene(99,0);
    gfo_match_phenomenon_action weather{};weather.model=291;weather.recipient=7;weather.has_recipient=1;
    const auto weather_before=observe(weather_match,0,0);
    check(gfo_match_phenomenon_legal(weather_match,&weather,&legal)==0&&legal&&observe(weather_match,0,0)==weather_before
        &&gfo_match_phenomenon_step(weather_match,&weather,&result)==0&&result.actor==7&&result.owner==0&&result.phase==1,
        "targeted environmental launch is a pure-query then one external decision");
    const auto active_weather=observe(weather_match,0,0);
    check(active_weather.find("\"origin\":null,\"source\":null,\"target\":7")!=std::string::npos
        &&active_weather.find("\"environmental\":true")!=std::string::npos
        &&active_weather.find("\"category\":\"phenomena\"")!=std::string::npos
        &&active_weather.find("\"power\":50")!=std::string::npos,"null source/origin are distinct from the unrelated live player0");
    gfo_match_command weather_defense{};weather_defense.actor=7;weather_defense.kind=1;weather_defense.item_count=2;
    weather_defense.items[0]=10;weather_defense.items[1]=11;
    check(gfo_match_step(weather_match,&weather_defense,&result)==0&&result.actor==7&&result.phase==1
        &&observe(weather_match,0,0).find("\"origin\":7,\"source\":7,\"target\":7")!=std::string::npos
        &&observe(weather_match,0,0).find("\"environmental\":false")!=std::string::npos
        &&observe(weather_match,0,0).find("\"models\":[187]")!=std::string::npos
        &&observe(weather_match,0,0).find("\"amount\":100")!=std::string::npos,
        "MP self-counter is generated with a real source and does not inherit environmental absence");
    weather_defense.item_count=0;weather_defense.items[0]=weather_defense.items[1]=0;
    check(gfo_match_step(weather_match,&weather_defense,&result)==0&&result.owner==0&&result.actor==0&&result.phase==0
        &&observe(weather_match,0,0).find("\"hp\":49,\"mp\":99")!=std::string::npos
        &&observe(weather_match,1,0).find("\"hand\":[]")!=std::string::npos
        &&observe(weather_match,0,0).find("\"gf\":8")!=std::string::npos,
        "ordinary responses get their gifts but environmental action has no source operation gift or GF advance");
    load_weather_scene(40,0);weather.model=293;
    auto wrong_weather_actor=owner_pray;wrong_weather_actor.actor=999;
    check(gfo_match_step(weather_match,&wrong_weather_actor,&result)==-2,"wrong ordinary actor does not mutate the source-free scene");
    check(gfo_match_phenomenon_step(weather_match,&weather,&result)==0&&result.phase==1,"recovery50 waits for the selected recipient response");
    weather_defense.item_count=1;weather_defense.items[0]=12;
    check(gfo_match_step(weather_match,&weather_defense,&result)==0&&result.phase==0&&result.actor==0
        &&observe(weather_match,0,0).find("\"hp\":40,\"mp\":0")!=std::string::npos
        &&!has_token(weather_match,7,12)&&hand_size(weather_match,7)==3,
        "mirror consumes and gets its gift while cancelling source-free recovery without inventing player0 as source");
    load_weather_scene(40,official::max_wire_integer);weather.model=292;
    const auto weather_late_view=observe(weather_match,0,0);const auto weather_late_status=status(weather_match);result=weather_late_status;
    check(gfo_match_phenomenon_legal(weather_match,&weather,&legal)==0&&legal
        &&gfo_match_phenomenon_step(weather_match,&weather,&result)==-2&&equal(result,weather_late_status)
        &&observe(weather_match,0,0)==weather_late_view,"late update failure rolls back the percentage launch draw and pending environmental frame");
    gfo_match_destroy(weather_match);
    check(gfo_match_item_shuffle_supported(295)&&!gfo_match_item_shuffle_supported(294)&&!gfo_match_item_shuffle_supported(296)
        &&!gfo_match_phenomenon_supported(295)&&!gfo_match_supported(295),"295 has a dedicated manifest API without changing simple phenomenon or gift capabilities");
    cfg.initial_hand=0;cfg.max_decisions=64;
    gfo_match* shuffle_match=nullptr;
    check(gfo_match_create(&cfg,players.data(),2,0,&shuffle_match)==0,"item shuffle bridge create");
    const auto load_shuffle_scene=[&](bool empty,bool exhausted,uint64_t update){
        std::array<gfo_state_player,3> people{};people[0].id=0;people[1].id=7;people[2].id=23;
        for(auto& person:people){person.hp=40;person.mp=99;person.cp=20;}
        people[2].dead=1;people[2].hp=0;
        std::vector<gfo_state_item> items(empty?1:5);
        if(!empty){items[0].token=0;items[0].model=6;items[1].token=1;items[1].model=235;items[1].used=1;
            items[2].token=0;items[2].model=152;items[3].token=1;items[3].model=191;
            people[0].item_count=2;people[1].item_offset=2;people[1].item_count=2;people[2].item_offset=4;}
        items.back().token=exhausted?official::max_wire_integer:0;items.back().model=208;people[2].item_count=1;
        gfo_state_meta meta{};meta.gf=9;meta.update_count=update;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,people.data(),3,items.data(),items.size(),nullptr,0,&scene)==0,"typed per-owner duplicate-token shuffle scene");
        check(gfo_match_load_scene(shuffle_match,scene,0,0,&result)==0,"load shuffle scene");gfo_state_destroy(scene);
    };
    load_shuffle_scene(false,false,0);
    std::array<gfo_match_item_source,4> order{{{7,0},{7,1},{0,1},{0,0}}};
    gfo_match_item_shuffle_action shuffle{order.data(),order.size()};
    const auto shuffle_before=observe(shuffle_match,0,0);const auto shuffle_status=status(shuffle_match);uint8_t shuffle_marker=0xa5;
    for(const auto count:{std::numeric_limits<size_t>::max(),size_t{163}}){
        gfo_match_item_shuffle_action bad{reinterpret_cast<const gfo_match_item_source*>(uintptr_t{1}),count};result=shuffle_status;
        check(gfo_match_item_shuffle_legal(shuffle_match,&bad,&shuffle_marker)==-2&&shuffle_marker==0xa5
            &&gfo_match_item_shuffle_step(shuffle_match,&bad,&result)==-2&&equal(result,shuffle_status)
            &&observe(shuffle_match,0,0)==shuffle_before,"oversized manifests reject before dereferencing pointer1");}
    gfo_match_item_shuffle_action null_order{nullptr,1};result=shuffle_status;
    check(gfo_match_item_shuffle_legal(shuffle_match,&null_order,&shuffle_marker)==-1
        &&gfo_match_item_shuffle_step(shuffle_match,&null_order,&result)==-1
        &&gfo_match_item_shuffle_step(shuffle_match,nullptr,&result)==-1
        &&gfo_match_item_shuffle_step(shuffle_match,&shuffle,nullptr)==-1
        &&equal(result,shuffle_status)&&shuffle_marker==0xa5,"NULL manifest guards preserve all outputs");
    auto duplicate=order;duplicate[1]=duplicate[0];gfo_match_item_shuffle_action bad_cover{duplicate.data(),duplicate.size()};
    check(gfo_match_item_shuffle_legal(shuffle_match,&bad_cover,&legal)==0&&!legal
        &&gfo_match_item_shuffle_step(shuffle_match,&bad_cover,&result)==-2
        &&observe(shuffle_match,0,0)==shuffle_before,"duplicate source pair cannot replace a missing copy");
    auto bad_integer=order;bad_integer[0].owner=official::max_wire_integer+1;bad_cover={bad_integer.data(),bad_integer.size()};
    check(gfo_match_item_shuffle_legal(shuffle_match,&bad_cover,&shuffle_marker)==-2&&shuffle_marker==0xa5,"source integer validation precedes native mutation");
    check(gfo_match_item_shuffle_legal(shuffle_match,&shuffle,&legal)==0&&legal&&observe(shuffle_match,0,0)==shuffle_before
        &&gfo_match_item_shuffle_step(shuffle_match,&shuffle,&result)==0&&result.phase==0&&result.owner==0&&result.actor==0&&result.decisions==1,
        "full source-pair permutation uses no inventory token sentinel or player-source action");
    const auto shuffled=observe(shuffle_match,1,7);order.fill({999,999});
    check(observe(shuffle_match,1,7)==shuffled&&shuffled.find("\"token\":4,\"face\":235,\"reference\":235,\"disguised\":false,\"used\":true")!=std::string::npos
        &&has_token(shuffle_match,0,2)&&has_token(shuffle_match,0,3)&&has_token(shuffle_match,7,5)&&has_token(shuffle_match,23,0)
        &&observe(shuffle_match,0,0).find("\"last_phenomenon\":{\"model\":295,\"recipient\":null}")!=std::string::npos
        &&observe(shuffle_match,0,0).find("\"order\":")==std::string::npos,"manifest storage is copied and only fresh alive tokens plus preserved used flags are observable to their owner");
    order={{{7,0},{7,1},{0,1},{0,0}}};
    load_shuffle_scene(false,true,0);const auto exhausted_shuffle=observe(shuffle_match,0,0);const auto exhausted_status=status(shuffle_match);result=exhausted_status;
    check(gfo_match_item_shuffle_legal(shuffle_match,&shuffle,&legal)==0&&legal
        &&gfo_match_item_shuffle_step(shuffle_match,&shuffle,&result)==-2&&equal(result,exhausted_status)
        &&observe(shuffle_match,0,0)==exhausted_shuffle,"fresh-token budget exhaustion rolls back the entire shuffle");
    load_shuffle_scene(true,true,0);gfo_match_item_shuffle_action empty_shuffle{nullptr,0};
    check(gfo_match_item_shuffle_legal(shuffle_match,&empty_shuffle,&legal)==0&&legal
        &&gfo_match_item_shuffle_step(shuffle_match,&empty_shuffle,&result)==0&&result.decisions==1
        &&has_token(shuffle_match,23,official::max_wire_integer),"empty alive permutation accepts NULL0 even when the next-token space is exhausted");
    load_shuffle_scene(false,false,official::max_wire_integer);const auto late_shuffle=observe(shuffle_match,0,0);const auto late_shuffle_status=status(shuffle_match);result=late_shuffle_status;
    check(gfo_match_item_shuffle_step(shuffle_match,&shuffle,&result)==-2&&equal(result,late_shuffle_status)
        &&observe(shuffle_match,0,0)==late_shuffle,"late update exhaustion restores old owner-token pairs and event history");
    gfo_match_destroy(shuffle_match);
    // Confused-owner commands use the unchanged ordinary wire, but remain a
    // privileged externally resolved route rather than a fabricated selector.
    cfg.initial_hand=0;cfg.hand_limit=1;cfg.max_decisions=64;
    gfo_match* confused_match=nullptr;
    check(gfo_match_create(&cfg,players.data(),2,0,&confused_match)==0,"confusion bridge create");
    const auto load_confused_scene=[&](int64_t count,uint64_t update){
        std::array<gfo_state_player,2> people{};people[0].id=0;people[1].id=7;
        for(std::size_t i=0;i<2;++i){auto& person=people[i];person.hp=40;person.mp=10;person.cp=20;
            person.confusion_count=count;person.item_offset=i;person.item_count=1;}
        std::array<gfo_state_item,2> items{};for(auto& item:items)item.model=208;
        gfo_state_meta meta{};meta.gf=9;meta.update_count=update;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,people.data(),2,items.data(),items.size(),nullptr,0,&scene)==0,"typed positive confusion count scene");
        check(gfo_match_load_scene(confused_match,scene,0,0,&result)==0,"load confusion scene");gfo_state_destroy(scene);
    };
    const auto confused_flags=[&](){const auto view=observe(confused_match,0,0);std::size_t count=0,pos=0;
        while((pos=view.find("\"confused\":true",pos))!=std::string::npos){++count;++pos;}return count;};
    load_confused_scene(1,0);
    gfo_match_command supplied_prayer{};supplied_prayer.kind=3;
    const auto confused_before=observe(confused_match,0,0);const auto confused_status=status(confused_match);result=confused_status;
    check(gfo_match_legal(confused_match,&supplied_prayer,&legal)==0&&!legal
        &&gfo_match_confusion_legal(confused_match,&supplied_prayer,&legal)==0&&legal
        &&gfo_match_step(confused_match,&supplied_prayer,&result)==-5&&equal(result,confused_status)
        &&observe(confused_match,0,0)==confused_before,"normal main is blocked while supplied confusion command preflight is pure");
    uint64_t confused_policy=123;gfo_match_command confused_sample{};std::memset(&confused_sample,0x5a,sizeof(confused_sample));const auto confused_sample_before=confused_sample;
    check(gfo_match_sample_action(confused_match,&confused_policy,&confused_sample)==-5&&confused_policy==123
        &&equal(confused_sample,confused_sample_before)&&observe(confused_match,0,0)==confused_before,"unsupported confused policy sampling preserves caller RNG, command and world");
    check(confused_flags()==2&&gfo_match_confusion_step(confused_match,&supplied_prayer,&result)==0&&result.actor==7&&confused_flags()==1,
        "one externally supplied main consumes one count and resumes ordinary input order");
    supplied_prayer.actor=7;
    check(gfo_match_confusion_step(confused_match,&supplied_prayer,&result)==0&&result.actor==0&&confused_flags()==0,
        "other owner counts remain independent and eventually return to ordinary control");
    supplied_prayer.actor=0;
    check(gfo_match_confusion_legal(confused_match,&supplied_prayer,&legal)==0&&!legal
        &&gfo_match_legal(confused_match,&supplied_prayer,&legal)==0&&legal,"confusion route requires a positive count, not just a valid ordinary command");
    gfo_match_phenomenon_action mushrooms{};mushrooms.model=289;
    check(gfo_match_phenomenon_supported(289)&&gfo_match_phenomenon_step(confused_match,&mushrooms,&result)==0&&confused_flags()==2
        &&observe(confused_match,0,0).find("\"last_phenomenon\":{\"model\":289,\"recipient\":null}")!=std::string::npos
        &&observe(confused_match,0,0).find("confusionCount")==std::string::npos,"289 adds hidden counts and publishes only the existing boolean and phenomenon event");
    const auto mushroom_view=observe(confused_match,0,0);const auto mushroom_status=status(confused_match);result=mushroom_status;uint8_t confusion_marker=0xa5;
    auto bad_confusion=supplied_prayer;bad_confusion.reserved[4]=1;
    check(gfo_match_confusion_legal(confused_match,&bad_confusion,&confusion_marker)==-2&&confusion_marker==0xa5
        &&gfo_match_confusion_step(confused_match,&bad_confusion,&result)==-2&&equal(result,mushroom_status)
        &&gfo_match_confusion_legal(nullptr,&supplied_prayer,&confusion_marker)==-1
        &&gfo_match_confusion_step(confused_match,nullptr,&result)==-1
        &&gfo_match_confusion_step(confused_match,&supplied_prayer,nullptr)==-1
        &&observe(confused_match,0,0)==mushroom_view,"confusion functions retain command padding, NULL and output atomicity guards");
    load_confused_scene(official::max_wire_integer-2,0);const auto overflow_confusion=observe(confused_match,0,0);const auto overflow_confusion_status=status(confused_match);result=overflow_confusion_status;
    check(gfo_match_phenomenon_legal(confused_match,&mushrooms,&legal)==0&&!legal
        &&gfo_match_phenomenon_step(confused_match,&mushrooms,&result)==-2&&equal(result,overflow_confusion_status)
        &&observe(confused_match,0,0)==overflow_confusion,"289 preflights every live count before adding3 to any participant");
    load_confused_scene(1,official::max_wire_integer);const auto late_confusion=observe(confused_match,0,0);const auto late_confusion_status=status(confused_match);result=late_confusion_status;
    check(gfo_match_confusion_legal(confused_match,&supplied_prayer,&legal)==0&&legal
        &&gfo_match_confusion_step(confused_match,&supplied_prayer,&result)==-2&&equal(result,late_confusion_status)
        &&observe(confused_match,0,0)==late_confusion&&confused_flags()==2,"late update failure restores the predecrement positive count and full old event state");
    gfo_match_destroy(confused_match);
    check(gfo_match_fate_supported(207)&&!gfo_match_fate_supported(206)&&!gfo_match_supported(207)
        &&gfo_match_catalog_count()==236&&gfo_match_gift_weight()==499,"owned207 uses its distinct composite capability without entering default gifts");
    cfg.initial_hand=0;cfg.hand_limit=18;cfg.max_decisions=64;
    gfo_match* fate_match=nullptr;check(gfo_match_create(&cfg,players.data(),2,0,&fate_match)==0,"fate bridge create");
    const auto load_fate_scene=[&](uint64_t update){
        std::array<gfo_state_player,2> people{};people[0].id=0;people[1].id=7;
        for(auto& person:people){person.hp=40;person.mp=10;person.cp=20;}
        people[0].confusion_count=1;people[0].item_count=2;people[1].item_offset=2;people[1].item_count=1;
        std::array<gfo_state_item,3> items{};items[0].model=207;items[1].token=1;items[1].model=235;items[1].used=1;items[2].model=152;
        gfo_state_meta meta{};meta.gf=9;meta.update_count=update;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,people.data(),2,items.data(),items.size(),nullptr,0,&scene)==0,"typed owned207 scene with source-local token0");
        check(gfo_match_load_scene(fate_match,scene,0,0,&result)==0,"load fate scene");gfo_state_destroy(scene);
    };
    load_fate_scene(0);gfo_match_fate_action fate{};fate.outcome.model=287;
    const auto before_fate=observe(fate_match,0,0);const auto fate_status=status(fate_match);
    for(unsigned error=0;error<10;++error){auto bad=fate;
        if(error==0)bad.reserved[0]=1;if(error==1)bad.reserved[6]=1;if(error==2)bad.has_shuffle=2;
        if(error==3)bad.outcome.reserved[2]=1;if(error==4)bad.shuffle.count=1;
        if(error==5)bad.shuffle.order=reinterpret_cast<const gfo_match_item_source*>(uintptr_t{1});
        if(error==6)bad.actor=official::max_wire_integer+1;if(error==7)bad.token=official::max_wire_integer+1;
        if(error==8){bad.has_shuffle=1;bad.shuffle={reinterpret_cast<const gfo_match_item_source*>(uintptr_t{1}),1};}
        if(error==9)bad.outcome.model=295;
        result=fate_status;uint8_t marker=0xa5;
        check(gfo_match_fate_legal(fate_match,&bad,&marker)==-2&&marker==0xa5
            &&gfo_match_fate_step(fate_match,&bad,&result)==-2&&equal(result,fate_status)
            &&observe(fate_match,0,0)==before_fate,"fate wire failures restore card, hidden count, world, event and caller outputs");}
    auto huge_fate=fate;huge_fate.has_shuffle=1;huge_fate.outcome.model=295;
    huge_fate.shuffle={reinterpret_cast<const gfo_match_item_source*>(uintptr_t{1}),std::numeric_limits<size_t>::max()};result=fate_status;uint8_t fate_marker=0xa5;
    check(gfo_match_fate_legal(fate_match,&huge_fate,&fate_marker)==-2&&fate_marker==0xa5
        &&gfo_match_fate_step(fate_match,&huge_fate,&result)==-2&&equal(result,fate_status),"fate nested count rejects before reading pointer1");
    auto null_fate=huge_fate;null_fate.shuffle={nullptr,1};
    check(gfo_match_fate_legal(fate_match,&null_fate,&fate_marker)==-1
        &&gfo_match_fate_step(fate_match,&null_fate,&result)==-1
        &&gfo_match_fate_step(fate_match,nullptr,&result)==-1&&gfo_match_fate_step(fate_match,&fate,nullptr)==-1
        &&observe(fate_match,0,0)==before_fate,"fate NULL nested and top-level inputs are atomic");
    check(gfo_match_fate_legal(fate_match,&fate,&legal)==0&&legal&&observe(fate_match,0,0)==before_fate
        &&gfo_match_fate_step(fate_match,&fate,&result)==0&&result.phase==0&&result.actor==7&&result.owner==7&&result.decisions==1
        &&!has_token(fate_match,0,0)&&hand_size(fate_match,0)==2
        &&observe(fate_match,0,0).find("\"last_fate\":{\"actor\":0,\"model\":207,\"phenomenon\":287}")!=std::string::npos,
        "atomic fate consumes owned207, debits confusion and completes one ordinary turn with its operation gift");
    load_fate_scene(0);
    std::array<gfo_match_item_source,2> fate_order{{{7,0},{0,1}}};fate.outcome.model=295;fate.has_shuffle=1;fate.shuffle={fate_order.data(),fate_order.size()};
    check(gfo_match_fate_legal(fate_match,&fate,&legal)==0&&legal&&gfo_match_fate_step(fate_match,&fate,&result)==0
        &&result.actor==7&&has_token(fate_match,0,2)&&has_token(fate_match,0,4)&&has_token(fate_match,7,3)
        &&observe(fate_match,1,7).find("\"used_models\":[235]")!=std::string::npos,
        "295 nested manifest covers only the post207 live slots and precedes the reserved operation gift");
    load_fate_scene(0);auto invalid_order=fate_order;invalid_order[1]={0,0};fate.shuffle={invalid_order.data(),invalid_order.size()};
    const auto invalid_fate_view=observe(fate_match,0,0);const auto invalid_fate_status=status(fate_match);result=invalid_fate_status;
    check(gfo_match_fate_legal(fate_match,&fate,&legal)==0&&!legal&&gfo_match_fate_step(fate_match,&fate,&result)==-2
        &&equal(result,invalid_fate_status)&&observe(fate_match,0,0)==invalid_fate_view,"a manifest referring to consumed207 rolls back to before207 use");
    load_fate_scene(official::max_wire_integer);fate={};fate.outcome.model=290;
    const auto late_fate_view=observe(fate_match,0,0);const auto late_fate_status=status(fate_match);result=late_fate_status;
    check(gfo_match_fate_legal(fate_match,&fate,&legal)==0&&legal&&gfo_match_fate_step(fate_match,&fate,&result)==-2
        &&equal(result,late_fate_status)&&observe(fate_match,0,0)==late_fate_view,"late fate failure restores the pre207 card and predecrement count as well as global HP");
    gfo_match_destroy(fate_match);
    for(uint16_t model=240;model<=244;++model)check(gfo_match_devil_supported(model)&&!gfo_match_supported(model),"external devil capability does not alter gifts");
    check(!gfo_match_devil_supported(239)&&!gfo_match_devil_supported(245),"devil capability has exactly five models");
    cfg.initial_hand=0;cfg.hand_limit=18;cfg.max_decisions=64;
    gfo_match* devil_match=nullptr;check(gfo_match_create(&cfg,players.data(),2,0,&devil_match)==0,"devil bridge create");
    const auto load_devil_scene=[&](uint64_t update,int hp,bool mixed){
        std::array<gfo_state_player,2> people{};people[0].id=0;people[1].id=7;
        for(auto& person:people){person.hp=hp;person.mp=95;person.cp=95;}
        people[0].confusion_count=2;
        std::array<gfo_state_item,3> items{};items[0].model=207;items[1].token=1;items[1].model=235;items[1].used=1;items[2].token=2;items[2].model=208;
        people[1].item_count=mixed?items.size():0;
        gfo_state_meta meta{};meta.gf=9;meta.update_count=update;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,people.data(),2,items.data(),mixed?items.size():0,nullptr,0,&scene)==0,"typed devil target with token0 and used miracle");
        check(gfo_match_load_scene(devil_match,scene,0,0,&result)==0,"load devil scene");gfo_state_destroy(scene);
    };
    load_devil_scene(0,99,true);gfo_match_devil_action devil{};devil.target=7;devil.model=240;
    const auto before_devil=observe(devil_match,0,0);const auto devil_status=status(devil_match);
    for(unsigned error=0;error<12;++error){auto bad=devil;
        if(error==0)bad.reserved[0]=1;if(error==1)bad.reserved[2]=1;if(error==2)bad.has_resource=2;
        if(error==3)bad.resource=1;if(error==4)bad.has_resource=1;if(error==5)bad.model=244;
        if(error==6){bad.model=244;bad.has_resource=1;bad.resource=3;}
        if(error==7){bad.model=243;bad.removed_count=3;}
        if(error==8){bad.model=243;bad.removed_count=1;bad.removed[1]=2;}
        if(error==9){bad.model=243;bad.removed_count=1;bad.removed[0]=official::max_wire_integer+1;}
        if(error==10)bad.target=official::max_wire_integer+1;
        if(error==11){bad.model=244;bad.has_resource=1;bad.removed_count=1;}
        result=devil_status;uint8_t marker=0xa5;
        check(gfo_match_devil_legal(devil_match,&bad,&marker)==-2&&marker==0xa5
            &&gfo_match_devil_step(devil_match,&bad,&result)==-2&&equal(result,devil_status)
            &&observe(devil_match,0,0)==before_devil,"devil wire variants and unused fields reject without changing outputs or world");
    }
    result=devil_status;uint8_t devil_marker=0xa5;
    check(gfo_match_devil_legal(nullptr,&devil,&devil_marker)==-1&&gfo_match_devil_legal(devil_match,nullptr,&devil_marker)==-1
        &&gfo_match_devil_legal(devil_match,&devil,nullptr)==-1&&devil_marker==0xa5
        &&gfo_match_devil_step(nullptr,&devil,&result)==-1&&gfo_match_devil_step(devil_match,nullptr,&result)==-1
        &&gfo_match_devil_step(devil_match,&devil,nullptr)==-1&&equal(result,devil_status),"devil NULL input guards preserve outputs");
    for(uint16_t model=240;model<=242;++model){load_devil_scene(0,99,false);devil={};devil.target=7;devil.model=model;
        const auto before=observe(devil_match,0,0);
        check(gfo_match_devil_legal(devil_match,&devil,&legal)==0&&legal&&observe(devil_match,0,0)==before
            &&gfo_match_devil_step(devil_match,&devil,&result)==0&&result.phase==0&&result.owner==0&&result.actor==0&&result.decisions==1,
            "direct devil HP body is one external decision without original response");
        const auto view=observe(devil_match,0,0);const auto hp=99-10*(model-239);
        check(view.find("\"hp\":"+std::to_string(hp))!=std::string::npos
            &&view.find("\"last_devil\":{\"model\":"+std::to_string(model)+",\"target\":7,\"removed_models\":[]}")!=std::string::npos,
            "fixed devil damage and public event are metadata driven");
    }
    load_devil_scene(0,99,true);devil={};devil.target=7;devil.model=243;devil.removed_count=2;devil.removed[0]=1;devil.removed[1]=0;
    check(gfo_match_devil_legal(devil_match,&devil,&legal)==0&&legal&&gfo_match_devil_step(devil_match,&devil,&result)==0
        &&hand_size(devil_match,7)==1&&has_token(devil_match,7,2)
        &&observe(devil_match,0,0).find("\"last_devil\":{\"model\":243,\"target\":7,\"removed_models\":[235,207]}")!=std::string::npos,
        "exact removed token order includes used miracle and special207 without gifts or passive activation");
    load_devil_scene(0,99,true);devil.removed[1]=1;const auto duplicate_view=observe(devil_match,0,0);const auto duplicate_status=status(devil_match);result=duplicate_status;
    check(gfo_match_devil_legal(devil_match,&devil,&legal)==0&&!legal&&gfo_match_devil_step(devil_match,&devil,&result)==-2
        &&equal(result,duplicate_status)&&observe(devil_match,0,0)==duplicate_view,"duplicate physical removal is atomically rejected");
    for(uint8_t resource=0;resource<3;++resource){load_devil_scene(0,95,false);devil={};devil.target=0;devil.model=244;devil.has_resource=1;devil.resource=resource;
        check(gfo_match_devil_legal(devil_match,&devil,&legal)==0&&legal&&gfo_match_devil_step(devil_match,&devil,&result)==0&&result.owner==0&&result.actor==0,
            "fairy accepts each explicit resource and real target ID0");
        check(observe(devil_match,0,0).find("\"last_devil\":{\"model\":244,\"target\":0,\"removed_models\":[]}")!=std::string::npos,
            "resource selector is absent from the public devil event");
    }
    load_devil_scene(official::max_wire_integer,95,false);devil={};devil.model=244;devil.has_resource=1;devil.resource=2;
    const auto late_devil=observe(devil_match,0,0);const auto late_devil_status=status(devil_match);result=late_devil_status;
    check(gfo_match_devil_legal(devil_match,&devil,&legal)==0&&legal&&gfo_match_devil_step(devil_match,&devil,&result)==-2
        &&equal(result,late_devil_status)&&observe(devil_match,0,0)==late_devil,"late devil update exhaustion restores resources, confusion, event and all caller bytes");
    gfo_match_destroy(devil_match);
    cfg.initial_hand=0;cfg.hand_limit=18;cfg.max_decisions=64;
    gfo_match* sacrifice_match=nullptr;gfo_match* discard_match=nullptr;
    check(gfo_match_create(&cfg,players.data(),2,0,&sacrifice_match)==0&&gfo_match_create(&cfg,players.data(),2,0,&discard_match)==0,"sacrifice bridge create without virtual owned2");
    const auto load_sacrifice_scene=[&](gfo_match* match,int64_t gf,bool threshold,int64_t at,int confused,uint64_t update){
        std::array<gfo_state_player,2> people{};people[0].id=0;people[1].id=7;
        for(auto& person:people){person.hp=40;person.mp=0;person.cp=20;}
        people[0].confusion_count=confused;people[0].item_count=5;people[1].item_offset=5;
        std::array<gfo_state_item,5> items{};items[0].model=235;items[0].used=1;
        for(std::size_t i=1;i<items.size();++i)items[i].token=i;
        items[1].model=207;items[2].model=6;items[3].model=208;items[4].model=209;
        gfo_state_meta meta{};meta.gf=gf;meta.has_tiebreak=threshold;meta.tiebreak_gf=at;meta.update_count=update;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,people.data(),2,items.data(),items.size(),nullptr,0,&scene)==0,"typed threshold scene");
        check(gfo_match_load_scene(match,scene,0,0,&result)==0,"load nonnegative apocalypse threshold scene");gfo_state_destroy(scene);
    };
    gfo_match_command sacrifice{};sacrifice.kind=9;sacrifice.item_count=2;sacrifice.items[0]=0;sacrifice.items[1]=1;
    for(int boundary=0;boundary<5;++boundary){const bool has=boundary!=0;const int64_t gf=boundary<2?0:boundary;
        const int64_t threshold=boundary<2?0:3;const bool active=has&&gf>=threshold;
        load_sacrifice_scene(sacrifice_match,gf,has,threshold,0,0);const auto before=observe(sacrifice_match,0,0);const auto saved=status(sacrifice_match);
        auto discard=sacrifice;discard.kind=2;
        check(gfo_match_legal(sacrifice_match,&sacrifice,&legal)==0&&bool(legal)==active
            &&gfo_match_legal(sacrifice_match,&discard,&legal)==0&&bool(legal)==!active
            &&before.find(active?"\"apocalypse_active\":true":"\"apocalypse_active\":false")!=std::string::npos,
            "null, explicit0 and threshold minus/equal/plus1 select exactly one operation");
        const auto wrong=active?discard:sacrifice;result=saved;
        check(gfo_match_step(sacrifice_match,&wrong,&result)==-2&&equal(result,saved)&&observe(sacrifice_match,0,0)==before,
            "wrong operation at the GF boundary is fully atomic");
    }
    load_sacrifice_scene(sacrifice_match,1,true,1,0,0);const auto sacrifice_before=observe(sacrifice_match,0,0);const auto sacrifice_status=status(sacrifice_match);
    for(unsigned error=0;error<5;++error){auto bad=sacrifice;
        if(error==0)bad.kind=10;if(error==1)bad.items[17]=1;if(error==2)bad.reserved[0]=1;
        if(error==3)bad.reserved_tail[0]=1;if(error==4)bad.item_count=19;
        result=sacrifice_status;uint8_t marker=0xa5;
        check(gfo_match_legal(sacrifice_match,&bad,&marker)==-2&&marker==0xa5
            &&gfo_match_step(sacrifice_match,&bad,&result)==-2&&equal(result,sacrifice_status)
            &&observe(sacrifice_match,0,0)==sacrifice_before,"opcode9 retains all command transport guards and rejects opcode10");
    }
    for(uint64_t forbidden:{uint64_t{2},uint64_t{3},uint64_t{4}}){auto bad=sacrifice;bad.item_count=1;bad.items[0]=forbidden;bad.items[1]=0;
        check(gfo_match_legal(sacrifice_match,&bad,&legal)==0&&!legal,"weapons, amulets and mortars remain unsacrificable");
    }
    check(gfo_match_step(sacrifice_match,&sacrifice,&result)==0&&result.owner==7&&result.actor==7&&result.decisions==1
        &&hand_size(sacrifice_match,0)==5&&!has_token(sacrifice_match,0,0)&&!has_token(sacrifice_match,0,1)
        &&has_token(sacrifice_match,0,5)&&has_token(sacrifice_match,0,6),"sacrifice erases retained MC and207 then reserves one ordinary gift per physical copy");
    load_sacrifice_scene(sacrifice_match,0,true,0,0,0);load_sacrifice_scene(discard_match,0,false,0,0,0);bool sampled_sacrifice=false;
    for(uint64_t seed=0;seed<64;++seed){auto ra=seed,rb=seed;gfo_match_command ordinary{},apocalypse{};
        check(gfo_match_sample_action(discard_match,&ra,&ordinary)==0&&gfo_match_sample_action(sacrifice_match,&rb,&apocalypse)==0,"sampler encodes new opcode9");
        auto expected=ordinary;if(expected.kind==2)expected.kind=9;
        check(equal(expected,apocalypse)&&ra==rb&&gfo_match_legal(sacrifice_match,&apocalypse,&legal)==0&&legal,
            "sacrifice is a one-for-one proposal mapping without extra policy RNG");sampled_sacrifice|=apocalypse.kind==9;
    }
    check(sampled_sacrifice,"opcode9 sampler branch was actually exercised");
    load_sacrifice_scene(sacrifice_match,0,true,0,1,0);const auto confused_sacrifice=observe(sacrifice_match,0,0);const auto confused_sacrifice_status=status(sacrifice_match);result=confused_sacrifice_status;
    check(gfo_match_legal(sacrifice_match,&sacrifice,&legal)==0&&!legal&&gfo_match_step(sacrifice_match,&sacrifice,&result)==-5
        &&equal(result,confused_sacrifice_status)&&observe(sacrifice_match,0,0)==confused_sacrifice
        &&gfo_match_confusion_legal(sacrifice_match,&sacrifice,&legal)==0&&legal
        &&gfo_match_confusion_step(sacrifice_match,&sacrifice,&result)==0
        &&observe(sacrifice_match,0,0).find("\"confused\":true")==std::string::npos,"supplied confusion command accepts opcode9 and debits one count");
    load_sacrifice_scene(sacrifice_match,1,true,0,0,official::max_wire_integer);const auto late_sacrifice=observe(sacrifice_match,0,0);const auto late_sacrifice_status=status(sacrifice_match);result=late_sacrifice_status;
    check(gfo_match_legal(sacrifice_match,&sacrifice,&legal)==0&&legal&&gfo_match_step(sacrifice_match,&sacrifice,&result)==-2
        &&equal(result,late_sacrifice_status)&&observe(sacrifice_match,0,0)==late_sacrifice,"late sacrifice failure restores used state, copies, gifts and caller output");
    gfo_match_destroy(sacrifice_match);gfo_match_destroy(discard_match);
    normal_grant_modes_bridge();
    self_absorption_alias_bridge();
    directed_dream_receipt_bridge();
    artifact_grant_bridge();
    explicit_ordinary_grant_interop();
    check(gfo_match_grant_supported(0)&&gfo_match_grant_supported(6)&&gfo_match_grant_supported(207)
        &&!gfo_match_grant_supported(1)&&!gfo_match_grant_supported(245),"static grant capability includes ordinary IDs, with config and recipient checks deferred to legal");
    std::size_t grant_capabilities=0;for(uint16_t model=0;model<=296;++model)grant_capabilities+=gfo_match_grant_supported(model)!=0;
    check(grant_capabilities==243,"grant capability is237 ordinary models, normal0 and five devils");
    for(uint16_t model=240;model<=244;++model)check(gfo_match_grant_supported(model),"all five supplied devils can resolve a grant");
    auto grant_cfg=cfg;grant_cfg.initial_hand=0;grant_cfg.hand_limit=18;grant_cfg.max_decisions=32;grant_cfg.external_grants=1;
    for(unsigned bad_byte=0;bad_byte<6;++bad_byte){auto bad=grant_cfg;
        if(bad_byte==0)bad.external_grants=2;if(bad_byte==1)bad.include_fate=2;if(bad_byte==2)bad.local_guardian_payloads=2;if(bad_byte==3)bad.external_artifacts=2;if(bad_byte>=4)bad.reserved_tail[bad_byte-4]=1;
        auto* sentinel_match=reinterpret_cast<gfo_match*>(uintptr_t{1});
        check(gfo_match_create(&bad,players.data(),2,0,&sentinel_match)==-2&&sentinel_match==reinterpret_cast<gfo_match*>(uintptr_t{1}),
            "four config flags are strict booleans and both remaining tail bytes stay reserved");
    }
    gfo_match* grant_match=nullptr;check(gfo_match_create(&grant_cfg,players.data(),2,0,&grant_match)==0,"external grants opt-in create");
    const auto load_grant_scene=[&](uint64_t update){
        std::array<gfo_state_player,2> people{};people[0].id=0;people[1].id=7;
        for(auto& person:people){person.hp=40;person.mp=10;person.cp=20;}
        people[0].item_count=2;people[1].item_offset=2;
        std::array<gfo_state_item,2> items{};items[0].model=152;items[1].token=1;items[1].model=235;items[1].used=1;
        gfo_state_meta meta{};meta.gf=0;meta.has_tiebreak=1;meta.tiebreak_gf=0;meta.update_count=update;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,people.data(),2,items.data(),items.size(),nullptr,0,&scene)==0,"typed grant recipient ID0 scene");
        check(gfo_match_load_scene(grant_match,scene,0,0,&result)==0,"load grant scene");gfo_state_destroy(scene);
    };
    load_grant_scene(0);gfo_match_command grant_offer{};grant_offer.kind=9;grant_offer.item_count=1;
    check(gfo_match_step(grant_match,&grant_offer,&result)==0&&result.phase==4&&!result.has_actor&&result.owner==0&&result.decisions==1,
        "reserved artifact pauses in phase4 without a policy actor");
    const auto grant_before=observe(grant_match,0,0);const auto grant_status=status(grant_match);gfo_match_grant_action grant{};
    check(grant_before.find("\"pending_grant\":{\"target\":0}")!=std::string::npos&&has_token(grant_match,0,1)&&!has_token(grant_match,0,0),
        "grant exposes only pending recipient after original item consumption");
    uint64_t grant_rng=123;gfo_match_command no_policy{};no_policy.kind=77;const auto old_no_policy=no_policy;
    check(gfo_match_sample_action(grant_match,&grant_rng,&no_policy)==-5&&grant_rng==123&&equal(no_policy,old_no_policy),
        "grant waiting refuses normal sampler without changing RNG or output");
    auto no_command=grant_offer;no_command.kind=3;no_command.item_count=0;
    check(gfo_match_legal(grant_match,&no_command,&legal)==0&&!legal,"normal commands cannot resolve environment phase4");
    for(unsigned error=0;error<10;++error){auto bad=grant;
        if(error==0)bad.reserved[0]=1;if(error==1)bad.reserved[2]=1;if(error==2)bad.has_resource=2;
        if(error==3)bad.has_resource=1;if(error==4)bad.resource=1;if(error==5)bad.removed_count=1;
        if(error==6)bad.removed[1]=1;if(error==7)bad.target=official::max_wire_integer+1;
        if(error==8)bad.model=244;if(error==9){bad.model=243;bad.removed_count=3;}
        result=grant_status;uint8_t marker=0xa5;
        check(gfo_match_grant_legal(grant_match,&bad,&marker)==-2&&marker==0xa5
            &&gfo_match_grant_step(grant_match,&bad,&result)==-2&&equal(result,grant_status)
            &&observe(grant_match,0,0)==grant_before,"grant wire errors preserve the pending continuation and caller outputs");
    }
    result=grant_status;uint8_t grant_marker=0xa5;
    check(gfo_match_grant_legal(nullptr,&grant,&grant_marker)==-1&&gfo_match_grant_legal(grant_match,nullptr,&grant_marker)==-1
        &&gfo_match_grant_legal(grant_match,&grant,nullptr)==-1&&grant_marker==0xa5
        &&gfo_match_grant_step(nullptr,&grant,&result)==-1&&gfo_match_grant_step(grant_match,nullptr,&result)==-1
        &&gfo_match_grant_step(grant_match,&grant,nullptr)==-1&&equal(result,grant_status),"grant NULL guards are atomic");
    auto wrong_recipient=grant;wrong_recipient.target=7;
    check(gfo_match_grant_legal(grant_match,&wrong_recipient,&legal)==0&&!legal
        &&gfo_match_grant_step(grant_match,&wrong_recipient,&result)==-2&&equal(result,grant_status),"only the current live recipient can receive a supplied grant");
    check(gfo_match_grant_legal(grant_match,&grant,&legal)==0&&legal&&gfo_match_grant_step(grant_match,&grant,&result)==0
        &&result.phase==0&&result.owner==7&&result.actor==7&&result.decisions==2&&hand_size(grant_match,0)==2&&has_token(grant_match,0,2)
        &&observe(grant_match,0,0).find("\"last_grant\":{\"target\":0,\"devil\":null,\"removed_models\":[]}")!=std::string::npos,
        "ordinary grant fills one debt, keeps its model private and advances the original turn once");
    load_grant_scene(official::max_wire_integer-1);check(gfo_match_step(grant_match,&grant_offer,&result)==0&&result.phase==4,"main action may reach the last update count while waiting for a grant");
    const auto late_grant=observe(grant_match,0,0);const auto late_grant_status=status(grant_match);result=late_grant_status;
    check(gfo_match_grant_legal(grant_match,&grant,&legal)==0&&legal&&gfo_match_grant_step(grant_match,&grant,&result)==-2
        &&equal(result,late_grant_status)&&observe(grant_match,0,0)==late_grant&&!has_token(grant_match,0,0),
        "late grant failure rolls back to pending state rather than undoing the accepted original command");
    auto late_explicit_grant=grant;late_explicit_grant.model=6;result=late_grant_status;
    check(gfo_match_grant_legal(grant_match,&late_explicit_grant,&legal)==0&&legal
        &&gfo_match_grant_step(grant_match,&late_explicit_grant,&result)==-2&&equal(result,late_grant_status)
        &&observe(grant_match,0,0)==late_grant,"numeric ordinary late failure preserves C output and pending state");
    gfo_match_artifact_grant_action late_artifact{};late_artifact.model=23;result=late_grant_status;
    check(gfo_match_artifact_grant_legal(grant_match,&late_artifact,&legal)==0&&legal
        &&gfo_match_artifact_grant_step(grant_match,&late_artifact,&result)==-2&&equal(result,late_grant_status)
        &&observe(grant_match,0,0)==late_grant,"artifact late update failure preserves C outputs and original pending proof");
    gfo_match_destroy(grant_match);grant_cfg.max_decisions=3;
    check(gfo_match_create(&grant_cfg,players.data(),2,0,&grant_match)==0,"bounded supplied devil retry create");
    load_grant_scene(0);check(gfo_match_step(grant_match,&grant_offer,&result)==0,"start bounded devil retry");
    grant.model=244;grant.has_resource=1;
    check(gfo_match_grant_step(grant_match,&grant,&result)==0&&result.phase==4&&result.decisions==2&&hand_size(grant_match,0)==1
        &&gfo_match_grant_step(grant_match,&grant,&result)==0&&result.phase==2&&result.truncated&&result.decisions==3&&hand_size(grant_match,0)==1
        &&observe(grant_match,0,0).find("\"pending_grant\":null")!=std::string::npos,"each explicit devil retry costs a decision and cutoff cancels the outstanding slot");
    gfo_match_destroy(grant_match);
    grant_cfg.max_decisions=32;check(gfo_match_create(&grant_cfg,players.data(),2,0,&grant_match)==0,"chance helper match create");
    std::array<gfo_state_player,2> chance_people{};chance_people[0].id=0;chance_people[1].id=7;
    for(auto& person:chance_people){person.hp=99;person.mp=99;person.cp=99;}
    chance_people[0].item_count=4;chance_people[0].curse_count=1;chance_people[1].item_offset=4;chance_people[1].curse_offset=1;
    std::array<gfo_state_item,4> chance_items{};
    chance_items[0].model=235;chance_items[0].used=1;chance_items[1].token=901;chance_items[1].model=207;
    chance_items[2].token=77;chance_items[2].model=118;chance_items[2].fake_model=123;chance_items[3].token=1;chance_items[3].model=152;
    const uint8_t chance_dream=static_cast<uint8_t>(official::Curse::dream);gfo_state_meta chance_meta{};chance_meta.has_tiebreak=1;
    gfo_state* chance_scene=nullptr;
    check(gfo_state_create(&chance_meta,chance_people.data(),2,chance_items.data(),chance_items.size(),&chance_dream,1,&chance_scene)==0
        &&gfo_match_load_scene(grant_match,chance_scene,0,42,&result)==0,"chance fixture preserves raw used207fake token order");gfo_state_destroy(chance_scene);
    gfo_match_grant_action chance_out;std::memset(&chance_out,0xa5,sizeof chance_out);const auto old_chance_out=chance_out;uint64_t chance_rng=123;
    const auto before_chance_main=observe(grant_match,0,0);
    check(gfo_match_sample_grant(grant_match,&chance_rng,&chance_out)==-2&&chance_rng==123&&equal(chance_out,old_chance_out)
        &&observe(grant_match,0,0)==before_chance_main,"wrong-phase chance failure is atomic");
    gfo_match_command chance_offer{};chance_offer.kind=9;chance_offer.item_count=1;chance_offer.items[0]=1;
    check(gfo_match_step(grant_match,&chance_offer,&result)==0&&result.phase==4,"chance fixture enters external grant waiting");
    const auto grant_sample_before=observe(grant_match,0,0);const auto grant_sample_status=status(grant_match);
    check(gfo_match_sample_grant(nullptr,&chance_rng,&chance_out)==-1&&gfo_match_sample_grant(grant_match,nullptr,&chance_out)==-1
        &&gfo_match_sample_grant(grant_match,&chance_rng,nullptr)==-1&&chance_rng==123&&equal(chance_out,old_chance_out),"chance NULL guards preserve the entire output and stream");
    struct ChanceGolden{uint64_t seed,next;uint16_t model;uint8_t resource,has_resource,count;uint64_t first,second;};
    const std::array<ChanceGolden,9> chance_goldens{{
        {0,11400714819323198485ULL,0,0,0,0,0,0},
        {2,11400714819323198487ULL,241,0,0,0,0,0},
        {5,15755400384260043844ULL,243,0,0,2,901,77},
        {8,4354685564936845362ULL,244,2,1,0,0,0},
        {11,11400714819323198496ULL,242,0,0,0,0,0},
        {12,4354685564936845366ULL,244,0,1,0,0,0},
        {29,11400714819323198514ULL,240,0,0,0,0,0},
        {106,4354685564936845460ULL,244,1,1,0,0,0},
        {7046029254386353131ULL,11400714819323198485ULL,0,0,0,0,0,0}
    }};
    for(const auto& golden:chance_goldens){chance_rng=golden.seed;chance_out=old_chance_out;gfo_match_grant_action expected{};
        expected.model=golden.model;expected.resource=golden.resource;expected.has_resource=golden.has_resource;
        expected.removed_count=golden.count;expected.removed[0]=golden.first;expected.removed[1]=golden.second;
        check(gfo_match_sample_grant(grant_match,&chance_rng,&chance_out)==0&&chance_rng==golden.next&&equal(chance_out,expected)
            &&gfo_match_grant_legal(grant_match,&chance_out,&legal)==0&&legal&&equal(status(grant_match),grant_sample_status)
            &&observe(grant_match,0,0)==grant_sample_before,"chance exact independent goldens cover six outcomes, raw partial FY, resource caps, rejection and zeroed inactive bytes");
    }
    gfo_match_grant_action ordinary_chance{};
    check(gfo_match_grant_step(grant_match,&ordinary_chance,&result)==0&&result.phase==0,"sampling does not prevent later ordinary resolution");
    chance_rng=123;chance_out=old_chance_out;
    check(gfo_match_sample_grant(grant_match,&chance_rng,&chance_out)==-2&&chance_rng==123&&equal(chance_out,old_chance_out),"completed chance helper refuses new draws atomically");
    gfo_match_destroy(grant_match);
    auto fate_pool_cfg=grant_cfg;fate_pool_cfg.external_grants=0;fate_pool_cfg.include_fate=1;fate_pool_cfg.initial_hand=1;
    gfo_match* fate_pool_match=nullptr;check(gfo_match_create(&fate_pool_cfg,players.data(),2,1464,&fate_pool_match)==0,"opt-in configured500pool create");
    uint8_t pool_allowed=0xa5;uint16_t pool_weight=0xbabe;
    check(gfo_match_gift_supported(nullptr,207,&pool_allowed)==-1&&pool_allowed==0xa5
        &&gfo_match_gift_supported(fate_pool_match,207,nullptr)==-1
        &&gfo_match_get_gift_weight(nullptr,&pool_weight)==-1&&pool_weight==0xbabe
        &&gfo_match_get_gift_weight(fate_pool_match,nullptr)==-1,"configured pool query NULL guards preserve outputs");
    check(gfo_match_get_gift_weight(fate_pool_match,&pool_weight)==0&&pool_weight==500
        &&gfo_match_gift_supported(fate_pool_match,207,&pool_allowed)==0&&pool_allowed==1
        &&!gfo_match_supported(207)&&gfo_match_catalog_count()==236&&gfo_match_gift_weight()==499,
        "current500 gift descriptor differs from frozen236 ordinary Command capability and global499");
    std::size_t configured_models=0,configured_weight=0;
    for(const auto& card:official::catalog()){
        pool_allowed=0xa5;check(gfo_match_gift_supported(fate_pool_match,card.id,&pool_allowed)==0&&bool(pool_allowed)==(card.gift_rate>0),
            "all current237 positive-gift catalog entries match the native configured pool");
        if(pool_allowed){++configured_models;configured_weight+=card.gift_rate;}
    }
    check(configured_models==237&&configured_weight==500,"complete canonical positive-gift mass is500");
    for(uint16_t absent:{uint16_t{0},uint16_t{297},std::numeric_limits<uint16_t>::max()})
        check(gfo_match_gift_supported(fate_pool_match,absent,&pool_allowed)==0&&!pool_allowed,"unknown IDs have no configured gift membership");
    const auto earned_fate_view=observe(fate_pool_match,1,0);const auto earned_fate_status=status(fate_pool_match);
    check(earned_fate_view.find("\"gift_pool\":{\"include_fate\":true,\"models\":237,\"weight\":500}")!=std::string::npos
        &&earned_fate_view.find("\"token\":0,\"face\":207,\"reference\":207,\"disguised\":false,\"used\":false")!=std::string::npos,
        "known seed1464 draws one unused unmasked207 from500-ticket reset");
    gfo_match_command forbidden_fate_play{};forbidden_fate_play.item_count=1;forbidden_fate_play.has_target=1;result=earned_fate_status;
    check(gfo_match_legal(fate_pool_match,&forbidden_fate_play,&legal)==0&&!legal
        &&gfo_match_step(fate_pool_match,&forbidden_fate_play,&result)==-5&&equal(result,earned_fate_status)
        &&observe(fate_pool_match,1,0)==earned_fate_view,"a naturally gifted207 still cannot dispatch as an ordinary Command");
    gfo_match_fate_action gifted_fate{};gifted_fate.outcome.model=290;
    check(gfo_match_fate_legal(fate_pool_match,&gifted_fate,&legal)==0&&legal&&gfo_match_fate_step(fate_pool_match,&gifted_fate,&result)==0
        &&result.owner==77&&!has_token(fate_pool_match,0,0),"a gifted207 uses the existing atomic FateAction API");
    check(gfo_match_reset(fate_pool_match,players.data(),2,1464,&result)==0
        &&gfo_match_get_gift_weight(fate_pool_match,&pool_weight)==0&&pool_weight==500
        &&observe(fate_pool_match,1,0)==earned_fate_view,"reset retains include_fate and reproduces the current pool exactly");
    gfo_match_destroy(fate_pool_match);fate_pool_cfg.include_fate=0;
    check(gfo_match_create(&fate_pool_cfg,players.data(),2,1464,&fate_pool_match)==0
        &&gfo_match_get_gift_weight(fate_pool_match,&pool_weight)==0&&pool_weight==499
        &&gfo_match_gift_supported(fate_pool_match,207,&pool_allowed)==0&&!pool_allowed,
        "explicitfalse retains the original pool descriptors");gfo_match_destroy(fate_pool_match);
    auto fate_sample_cfg=fate_pool_cfg;fate_sample_cfg.initial_hand=0;fate_sample_cfg.max_decisions=64;
    gfo_match* fate_sample_match=nullptr;check(gfo_match_create(&fate_sample_cfg,players.data(),2,42,&fate_sample_match)==0,"fate sampler bridge create");
    std::array<gfo_state_player,3> fate_sample_people{};
    for(auto& person:fate_sample_people){person.hp=99;person.mp=99;person.cp=99;}
    fate_sample_people[0].id=0;fate_sample_people[0].confusion_count=1;fate_sample_people[0].item_count=3;
    fate_sample_people[1].id=7;fate_sample_people[1].item_offset=3;fate_sample_people[1].item_count=1;
    fate_sample_people[2].id=23;fate_sample_people[2].hp=0;fate_sample_people[2].dead=1;fate_sample_people[2].item_offset=4;fate_sample_people[2].item_count=1;
    std::array<gfo_state_item,5> fate_sample_items{};
    fate_sample_items[0].model=207;fate_sample_items[1].model=207;fate_sample_items[1].token=9;
    fate_sample_items[2].model=235;fate_sample_items[2].token=2;fate_sample_items[2].used=1;
    fate_sample_items[3].model=152;fate_sample_items[4].model=208;
    gfo_state_meta fate_sample_meta{};gfo_state* fate_sample_scene=nullptr;
    check(gfo_state_create(&fate_sample_meta,fate_sample_people.data(),3,fate_sample_items.data(),5,nullptr,0,&fate_sample_scene)==0
        &&gfo_match_load_scene(fate_sample_match,fate_sample_scene,0,42,&result)==0,"load confused main source with two207 copies and a dead inventory");gfo_state_destroy(fate_sample_scene);
    std::array<gfo_match_item_source,164> fate_buffer;for(auto& entry:fate_buffer)entry={0xababababababababULL,0xcdcdcdcdcdcdcdcdULL};
    const auto fate_buffer_before=fate_buffer;gfo_match_fate_action fate_sample_out;std::memset(&fate_sample_out,0xa5,sizeof fate_sample_out);
    const auto fate_header_before=fate_sample_out;uint64_t fate_chance=4;const auto fate_query_view=observe(fate_sample_match,0,0);const auto fate_query_status=status(fate_sample_match);
    check(gfo_match_sample_fate(nullptr,0,&fate_chance,fate_buffer.data()+1,162,&fate_sample_out)==-1
        &&gfo_match_sample_fate(fate_sample_match,0,nullptr,fate_buffer.data()+1,162,&fate_sample_out)==-1
        &&gfo_match_sample_fate(fate_sample_match,0,&fate_chance,fate_buffer.data()+1,162,nullptr)==-1
        &&gfo_match_sample_fate(fate_sample_match,0,&fate_chance,nullptr,1,&fate_sample_out)==-1
        &&gfo_match_sample_fate(fate_sample_match,0,&fate_chance,nullptr,163,&fate_sample_out)==-2
        &&gfo_match_sample_fate(fate_sample_match,0,&fate_chance,reinterpret_cast<gfo_match_item_source*>(uintptr_t{1}),std::numeric_limits<size_t>::max(),&fate_sample_out)==-2
        &&fate_chance==4&&equal(fate_sample_out,fate_header_before)&&equal(fate_buffer,fate_buffer_before),"Fate variable manifest NULL and capacity guards never read or write caller storage");
    for(uint64_t bad_token:{uint64_t{2},uint64_t{99},official::max_wire_integer+1})
        check(gfo_match_sample_fate(fate_sample_match,bad_token,&fate_chance,fate_buffer.data()+1,162,&fate_sample_out)==-2
            &&fate_chance==4&&equal(fate_sample_out,fate_header_before)&&equal(fate_buffer,fate_buffer_before),"Fate invalid token rejects before sampling or short-buffer resolution");
    check(gfo_match_sample_fate(fate_sample_match,0,&fate_chance,nullptr,0,&fate_sample_out)==-4
        &&gfo_match_sample_fate(fate_sample_match,0,&fate_chance,fate_buffer.data()+1,2,&fate_sample_out)==-4
        &&fate_chance==4&&equal(fate_sample_out,fate_header_before)&&equal(fate_buffer,fate_buffer_before),"short Fate manifest is an atomic -4 including the entire caller tail");
    struct FateGolden{uint64_t seed,next;uint16_t model;uint8_t targeted;uint64_t target;};
    const std::array<FateGolden,10> fate_goldens{{
        {2,11400714819323198487ULL,287,0,0},{15,11400714819323198500ULL,288,0,0},{6,11400714819323198491ULL,289,0,0},
        {3,11400714819323198488ULL,290,0,0},{20,4354685564936845374ULL,291,1,7},{0,4354685564936845354ULL,292,1,0},
        {10,4354685564936845364ULL,293,1,0},{7,4354685564936845361ULL,294,1,0},
        {4,15755400384260043843ULL,295,0,0},{17,15755400384260043856ULL,296,0,0}
    }};
    for(const auto& golden:fate_goldens){fate_chance=golden.seed;fate_buffer=fate_buffer_before;fate_sample_out=fate_header_before;gfo_match_fate_action expected{};
        expected.outcome.model=golden.model;expected.outcome.has_recipient=golden.targeted;expected.outcome.recipient=golden.target;
        if(golden.model==296){expected.outcome.guardian_count=2;expected.outcome.guardians[0]=4;expected.outcome.guardians[1]=7;}
        if(golden.model==295){expected.has_shuffle=1;expected.shuffle={fate_buffer.data()+1,3};}
        check(gfo_match_sample_fate(fate_sample_match,0,&fate_chance,fate_buffer.data()+1,162,&fate_sample_out)==0
            &&fate_chance==golden.next&&equal(fate_sample_out,expected),"ten independent Fate goldens include targetID0 and full canonical64-byte headers");
        auto expected_buffer=fate_buffer_before;
        if(golden.model==295){expected_buffer[1]={0,9};expected_buffer[2]={7,0};expected_buffer[3]={0,2};}
        check(equal(fate_buffer,expected_buffer)&&equal(status(fate_sample_match),fate_query_status)
            &&observe(fate_sample_match,0,0)==fate_query_view,"Fate output points at caller prefix, preserves guards/tail and never consumes a card or confusion count");
    }
    fate_chance=2;check(gfo_match_sample_fate(fate_sample_match,0,&fate_chance,nullptr,0,&fate_sample_out)==0
        &&!fate_sample_out.has_shuffle&&!fate_sample_out.shuffle.order&&!fate_sample_out.shuffle.count,"non295 Fate succeeds without any manifest storage");
    for(std::size_t count=0;count<=1;++count){
        fate_sample_people[0].item_count=1;fate_sample_people[1].item_offset=1;fate_sample_people[1].item_count=count;
        fate_sample_items[1]={};fate_sample_items[1].model=235;fate_sample_items[1].used=1;
        check(gfo_state_create(&fate_sample_meta,fate_sample_people.data(),2,fate_sample_items.data(),1+count,nullptr,0,&fate_sample_scene)==0
            &&gfo_match_load_scene(fate_sample_match,fate_sample_scene,0,42,&result)==0,"load empty or singleton post207 manifest");gfo_state_destroy(fate_sample_scene);
        fate_chance=4;fate_buffer=fate_buffer_before;
        check(gfo_match_sample_fate(fate_sample_match,0,&fate_chance,count?fate_buffer.data()+1:nullptr,count,&fate_sample_out)==0
            &&fate_chance==11400714819323198489ULL&&fate_sample_out.has_shuffle&&fate_sample_out.shuffle.count==count
            &&fate_sample_out.shuffle.order==(count?fate_buffer.data()+1:nullptr),"zero and singleton Fate shuffles consume no Fisher-Yates draws");
        if(count)check(fate_buffer[1].owner==7&&fate_buffer[1].token==0&&equal(fate_buffer[0],fate_buffer_before[0])
            &&equal(fate_buffer[2],fate_buffer_before[2]),"singleton caller source copy remains guarded");
    }
    fate_sample_people[1].confusion_count=official::max_wire_integer;
    check(gfo_state_create(&fate_sample_meta,fate_sample_people.data(),2,fate_sample_items.data(),2,nullptr,0,&fate_sample_scene)==0
        &&gfo_match_load_scene(fate_sample_match,fate_sample_scene,0,42,&result)==0,"load unrepresentable289 target count");gfo_state_destroy(fate_sample_scene);
    fate_chance=6;fate_buffer=fate_buffer_before;fate_sample_out=fate_header_before;const auto overflow_fate_view=observe(fate_sample_match,0,0);
    check(gfo_match_sample_fate(fate_sample_match,0,&fate_chance,fate_buffer.data()+1,162,&fate_sample_out)==-2
        &&fate_chance==6&&equal(fate_sample_out,fate_header_before)&&equal(fate_buffer,fate_buffer_before)
        &&observe(fate_sample_match,0,0)==overflow_fate_view,"invalid sampled289 is not rerolled and changes no chance stream, output or state");
    gfo_match_destroy(fate_sample_match);
    auto conf_sample_cfg=fate_sample_cfg;conf_sample_cfg.max_decisions=64;conf_sample_cfg.external_grants=0;conf_sample_cfg.include_fate=0;
    gfo_match* conf_sample_match=nullptr;gfo_match* conf_sample_normal=nullptr;
    check(gfo_match_create(&conf_sample_cfg,players.data(),2,42,&conf_sample_match)==0
        &&gfo_match_create(&conf_sample_cfg,players.data(),2,42,&conf_sample_normal)==0,"confusion sampler positive and count0 scene handles");
    std::array<gfo_state_player,2> conf_sample_people{};conf_sample_people[0].id=0;conf_sample_people[0].item_count=6;
    conf_sample_people[1].id=7;conf_sample_people[1].item_offset=6;conf_sample_people[1].item_count=1;
    for(auto& person:conf_sample_people){person.hp=40;person.mp=10;person.cp=20;}
    std::array<gfo_state_item,7> conf_sample_items{};
    const std::array<uint16_t,6> conf_sample_models{{152,118,3,4,5,6}};
    for(std::size_t i=0;i<6;++i){conf_sample_items[i].token=i;conf_sample_items[i].model=conf_sample_models[i];}conf_sample_items[6].model=152;
    const auto load_confusion_sampler=[&](gfo_match* match,bool positive,uint64_t update){
        conf_sample_people[0].confusion_count=positive?1:0;gfo_state_meta meta{};meta.update_count=update;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,conf_sample_people.data(),2,conf_sample_items.data(),7,nullptr,0,&scene)==0
            &&gfo_match_load_scene(match,scene,0,42,&result)==0,"load fresh count0/positive policy counterfactual without editing action history");gfo_state_destroy(scene);
    };
    load_confusion_sampler(conf_sample_match,true,0);load_confusion_sampler(conf_sample_normal,false,0);
    const auto conf_sample_before=observe(conf_sample_match,0,0);const auto conf_sample_status=status(conf_sample_match);
    gfo_match_command conf_sample_out;std::memset(&conf_sample_out,0xa5,sizeof conf_sample_out);const auto conf_sample_old=conf_sample_out;uint64_t conf_sample_rng=123;
    check(gfo_match_sample_confusion(nullptr,&conf_sample_rng,&conf_sample_out)==-1
        &&gfo_match_sample_confusion(conf_sample_match,nullptr,&conf_sample_out)==-1
        &&gfo_match_sample_confusion(conf_sample_match,&conf_sample_rng,nullptr)==-1
        &&gfo_match_sample_confusion(conf_sample_normal,&conf_sample_rng,&conf_sample_out)==-2
        &&gfo_match_sample_action(conf_sample_match,&conf_sample_rng,&conf_sample_out)==-5
        &&conf_sample_rng==123&&equal(conf_sample_out,conf_sample_old)
        &&observe(conf_sample_match,0,0)==conf_sample_before,"confusion sampler NULL/nonconfused gates and old normal refusal preserve184-byte out and chance stream");
    std::array<bool,10> conf_sample_seen{};gfo_match_command conf_sample_discard{};
    for(uint64_t seed=0;seed<512;++seed){auto chance=seed,policy=seed;gfo_match_command reference{};conf_sample_out=conf_sample_old;
        check(gfo_match_sample_confusion(conf_sample_match,&chance,&conf_sample_out)==0
            &&gfo_match_sample_action(conf_sample_normal,&policy,&reference)==0&&chance==policy&&equal(conf_sample_out,reference)
            &&conf_sample_out.kind<conf_sample_seen.size()
            &&gfo_match_confusion_legal(conf_sample_match,&conf_sample_out,&legal)==0&&legal
            &&gfo_match_legal(conf_sample_normal,&conf_sample_out,&legal)==0&&legal,
            "shared visible policy law matches count0 scene including whole184-byte canonical command and next RNG");
        conf_sample_seen[conf_sample_out.kind]=true;if(conf_sample_out.kind==2)conf_sample_discard=conf_sample_out;
    }
    check(conf_sample_seen[0]&&conf_sample_seen[2]&&conf_sample_seen[4]&&conf_sample_seen[5]&&conf_sample_seen[6]
        &&equal(status(conf_sample_match),conf_sample_status)&&observe(conf_sample_match,0,0)==conf_sample_before,
        "play/discard/exchange/sell/buy branches were exercised without debiting confusion or changing any public state");
    gfo_match_status conf_sample_reference_result{};
    check(gfo_match_confusion_step(conf_sample_match,&conf_sample_discard,&result)==0
        &&gfo_match_step(conf_sample_normal,&conf_sample_discard,&conf_sample_reference_result)==0&&equal(result,conf_sample_reference_result)
        &&observe(conf_sample_match,0,0)==observe(conf_sample_normal,0,0),"only applying the selected command debits one count");
    gfo_match_command conf_sample_prayer{};conf_sample_prayer.actor=7;conf_sample_prayer.kind=3;
    check(gfo_match_step(conf_sample_match,&conf_sample_prayer,&result)==0&&gfo_match_step(conf_sample_normal,&conf_sample_prayer,&conf_sample_reference_result)==0
        &&observe(conf_sample_match,1,7)==observe(conf_sample_normal,1,7),"repeated local sampling did not advance environment RNG before the next ordinary artifact");
    load_confusion_sampler(conf_sample_match,true,official::max_wire_integer);const auto conf_sample_late=observe(conf_sample_match,0,0);const auto conf_sample_late_status=status(conf_sample_match);
    conf_sample_rng=0;check(gfo_match_sample_confusion(conf_sample_match,&conf_sample_rng,&conf_sample_out)==0,"sampling ignores future update exhaustion");result=conf_sample_late_status;
    check(gfo_match_confusion_step(conf_sample_match,&conf_sample_out,&result)==-2&&equal(result,conf_sample_late_status)
        &&observe(conf_sample_match,0,0)==conf_sample_late,"failed later application restores the predebit state without undoing the separate chance sample");
    conf_sample_people[0].item_count=2;conf_sample_people[0].confusion_count=1;conf_sample_people[1].item_offset=2;conf_sample_people[1].item_count=0;
    gfo_state_meta conf_reject_meta{};gfo_state* conf_reject_scene=nullptr;
    check(gfo_state_create(&conf_reject_meta,conf_sample_people.data(),2,conf_sample_items.data(),2,nullptr,0,&conf_reject_scene)==0
        &&gfo_match_load_scene(conf_sample_match,conf_reject_scene,0,42,&result)==0,"three-proposal confusion rejection fixture");gfo_state_destroy(conf_reject_scene);
    conf_sample_rng=7046029254386353131ULL;gfo_match_command conf_reject_expected{};conf_reject_expected.kind=2;conf_reject_expected.item_count=1;
    check(gfo_match_sample_confusion(conf_sample_match,&conf_sample_rng,&conf_sample_out)==0
        &&conf_sample_rng==11400714819323198485ULL&&equal(conf_sample_out,conf_reject_expected),"zero first SplitMix word is rejected for three proposals before canonical discard selection");
    gfo_match_destroy(conf_sample_match);gfo_match_destroy(conf_sample_normal);
    auto payload_sample_cfg=conf_sample_cfg;gfo_match* payload_sample_match=nullptr;
    check(gfo_match_create(&payload_sample_cfg,players.data(),2,42,&payload_sample_match)==0,"ordinary guardian payload sampler create");
    std::array<gfo_state_player,5> payload_sample_people{};
    const std::array<uint64_t,5> payload_sample_ids{{0,17,23,42,99}};
    for(std::size_t i=0;i<5;++i){payload_sample_people[i].id=payload_sample_ids[i];payload_sample_people[i].hp=99;payload_sample_people[i].team=(i==1||i==2)?1:2;}
    payload_sample_people[0].confusion_count=1;payload_sample_people[1].confusion_count=3;
    payload_sample_people[4].dead=1;payload_sample_people[4].hp=0;payload_sample_people[4].guardian=10;
    const auto load_payload_sampler=[&](uint8_t guardian,uint64_t update){
        payload_sample_people[1].guardian=guardian;gfo_state_meta meta{};meta.update_count=update;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,payload_sample_people.data(),5,nullptr,0,nullptr,0,&scene)==0
            &&gfo_match_load_scene(payload_sample_match,scene,0,42,&result)==0,"load nonowner empty-hand zero-MP guardian source");gfo_state_destroy(scene);
    };
    load_payload_sampler(9,0);gfo_match_guardian_payload_action payload_sample_out;std::memset(&payload_sample_out,0xa5,sizeof payload_sample_out);
    const auto payload_sample_old=payload_sample_out;uint64_t payload_sample_rng=123;
    const auto payload_sample_before=observe(payload_sample_match,0,0);const auto payload_sample_status=status(payload_sample_match);
    check(gfo_match_sample_guardian_payload(nullptr,17,&payload_sample_rng,&payload_sample_out)==-1
        &&gfo_match_sample_guardian_payload(payload_sample_match,17,nullptr,&payload_sample_out)==-1
        &&gfo_match_sample_guardian_payload(payload_sample_match,17,&payload_sample_rng,nullptr)==-1
        &&payload_sample_rng==123&&equal(payload_sample_out,payload_sample_old),"guardian sampler NULL guards preserve all64 output bytes and chance stream");
    for(uint64_t bad_source:{uint64_t{0},uint64_t{23},uint64_t{99},uint64_t{999},official::max_wire_integer+1})
        check(gfo_match_sample_guardian_payload(payload_sample_match,bad_source,&payload_sample_rng,&payload_sample_out)==-2
            &&payload_sample_rng==123&&equal(payload_sample_out,payload_sample_old)
            &&equal(status(payload_sample_match),payload_sample_status)&&observe(payload_sample_match,0,0)==payload_sample_before,
            "absent, dead, non-Earth/Moon and unsafe sources fail without a selection or mutation");
    const auto payload_golden=[&](uint64_t seed,uint64_t next,std::initializer_list<uint16_t> models,uint64_t target){
        gfo_match_guardian_payload_action expected{};expected.source=17;expected.target=target;expected.has_target=1;
        expected.guardian=payload_sample_people[1].guardian;expected.model_count=static_cast<uint8_t>(models.size());std::copy(models.begin(),models.end(),expected.models);
        payload_sample_rng=seed;payload_sample_out=payload_sample_old;const auto before=observe(payload_sample_match,0,0);const auto saved=status(payload_sample_match);
        check(gfo_match_sample_guardian_payload(payload_sample_match,17,&payload_sample_rng,&payload_sample_out)==0
            &&payload_sample_rng==next&&equal(payload_sample_out,expected)
            &&gfo_match_guardian_payload_legal(payload_sample_match,&payload_sample_out,&legal)==0&&legal
            &&equal(status(payload_sample_match),saved)&&observe(payload_sample_match,0,0)==before,
            "independent guardian goldens match entire64-byte columns, source-inferred domain, explicit targets and next RNG");
    };
    payload_golden(0,15755400384260043839ULL,{61},42);
    payload_golden(1,6018027440424182932ULL,{71,68,25},42);
    payload_golden(42,1663341875487337619ULL,{19,51},0);
    payload_golden(6,1663341875487337583ULL,{38,162},42);
    payload_golden(201,15755400384260044040ULL,{58},0);
    payload_golden(38596,4597363874025048504ULL,{20,68,22,22,25,78,88,78,150,15,63,63,88,56,31,9,25,15},42);
    payload_golden(7046029254386353131ULL,15755400384260043839ULL,{61},42);
    load_payload_sampler(10,0);
    payload_golden(0,4354685564936845354ULL,{219},0);
    payload_golden(1,4354685564936845355ULL,{229},42);
    payload_golden(42,4354685564936845396ULL,{219},42);
    payload_golden(7046029254386353131ULL,4354685564936845354ULL,{219},0);
    load_payload_sampler(9,official::max_wire_integer);payload_sample_rng=201;
    check(gfo_match_sample_guardian_payload(payload_sample_match,17,&payload_sample_rng,&payload_sample_out)==0,"guardian sampling does not inspect future update budget");
    const auto payload_sample_late=observe(payload_sample_match,0,0);const auto payload_sample_late_status=status(payload_sample_match);result=payload_sample_late_status;
    check(gfo_match_guardian_payload_step(payload_sample_match,&payload_sample_out,&result)==-2&&equal(result,payload_sample_late_status)
        &&observe(payload_sample_match,0,0)==payload_sample_late,"separate payload application failure rolls back the entire current main state");
    gfo_match_destroy(payload_sample_match);
    auto payload_auto_cfg=payload_sample_cfg;payload_auto_cfg.local_guardian_payloads=1;
    gfo_match* payload_auto_match=nullptr;
    check(gfo_match_create(&payload_auto_cfg,players.data(),2,2,&payload_auto_match)==0,"local automatic payload config crosses the unchanged24-byte ABI");
    std::array<gfo_state_player,4> payload_auto_people{};
    const std::array<uint64_t,4> payload_auto_ids{{0,17,23,42}};
    for(std::size_t i=0;i<4;++i){payload_auto_people[i].id=payload_auto_ids[i];payload_auto_people[i].hp=99;
        payload_auto_people[i].team=(i==1||i==2)?1:2;payload_auto_people[i].item_offset=i?18:0;}
    std::array<gfo_state_item,18> payload_auto_items{};
    for(std::size_t i=0;i<18;++i){payload_auto_items[i].token=i;payload_auto_items[i].model=235;payload_auto_items[i].used=1;}
    payload_auto_people[0].item_count=18;
    for(bool moon:{false,true}){
        payload_auto_people[1].guardian=moon?10:9;gfo_state_meta meta{};meta.gf=7;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,payload_auto_people.data(),4,payload_auto_items.data(),18,nullptr,0,&scene)==0
            &&gfo_match_load_scene(payload_auto_match,scene,0,2,&result)==0,"full owner prayer has no gift draw before zero-MP nonowner payload");gfo_state_destroy(scene);
        gfo_match_command prayer{};prayer.kind=3;
        check(gfo_match_step(payload_auto_match,&prayer,&result)==0&&result.phase==1&&result.owner==0&&result.actor==(moon?23:42)&&result.decisions==1,
            "automatic payload stays in the original action without its own decision");
        const auto event=std::string("\"last_auto_guardian_payloads\":[{\"source\":17,\"guardian\":\"")+(moon?"moon":"earth")
            +"\",\"models\":"+(moon?"[236]":"[32,9,31]")+",\"target\":"+(moon?"23":"42")+"}]";
        auto view=observe(payload_auto_match,0,0);
        check(view.find(event)!=std::string::npos&&view.find("\"local_guardian_payloads\":true")!=std::string::npos
            &&view.find("\"last_auto_guardians\":[]")!=std::string::npos&&view.find("\"last_guardian_payload\":null")!=std::string::npos,
            "seed2 game RNG selects independent Earth or Moon columns and publishes only launched local payloads");
        const auto pending_status=result;auto bad=gfo_match_command{};bad.kind=1;bad.actor=999;
        check(gfo_match_step(payload_auto_match,&bad,&result)==-2&&equal(result,pending_status)&&observe(payload_auto_match,0,0)==view,
            "invalid response leaves auto payload, original owner and public log untouched");
        gfo_match_command pass{};pass.kind=1;pass.actor=moon?23:42;
        check(gfo_match_step(payload_auto_match,&pass,&result)==0&&result.phase==0&&result.owner==17&&result.actor==17&&result.decisions==2,
            "automatic response resumes ordinary next owner without a second activation charge");
        view=observe(payload_auto_match,0,0);
        check(view.find(event)!=std::string::npos&&view.find("\"gf\":8")!=std::string::npos
            &&(moon||view.find("\"hp\":81,")!=std::string::npos),"completed payload log and single GF advance survive opaque bridge replay");
    }
    check(gfo_match_reset(payload_auto_match,players.data(),2,2,&result)==0
        &&observe(payload_auto_match,0,0).find("\"local_guardian_payloads\":true")!=std::string::npos
        &&observe(payload_auto_match,0,0).find("\"last_auto_guardian_payloads\":[]")!=std::string::npos,"reset retains local payload opt-in and clears its public history");
    gfo_match_destroy(payload_auto_match);
    check(gfo_match_driver_api_version()==1&&gfo_match_choice_size()==208&&sizeof(gfo_match_choice)==208
        &&offsetof(gfo_match_choice,fate_actor)==184&&offsetof(gfo_match_choice,fate_token)==192
        &&offsetof(gfo_match_choice,kind)==200&&offsetof(gfo_match_choice,reserved)==201
        &&std::string(gfo_match_choice_rule()).starts_with("experimental:")&&std::string(gfo_match_driver_rule()).starts_with("experimental:"),
        "additive driver API1 advertises separate policy rules without changing Match ABI2");
    gfo_match_config driver_cfg{};driver_cfg.max_decisions=64;driver_cfg.hand_limit=18;
    driver_cfg.initial_hp=driver_cfg.initial_mp=driver_cfg.initial_cp=99;
    gfo_match* driver_match=nullptr;gfo_match* driver_manual=nullptr;
    check(gfo_match_create(&driver_cfg,players.data(),2,42,&driver_match)==0
        &&gfo_match_create(&driver_cfg,players.data(),2,42,&driver_manual)==0,"create unchanged-default driver handles");
    const auto load_driver=[&](gfo_match* into,bool fate,uint64_t update,bool max_confusion){
        std::array<gfo_state_player,2> people{};people[1].id=7;
        for(auto& person:people)person.hp=person.mp=person.cp=99;
        people[0].item_count=fate?2:0;people[1].item_offset=people[0].item_count;
        if(max_confusion)people[1].confusion_count=official::max_wire_integer;
        std::array<gfo_state_item,2> cards{};cards[0].model=cards[1].model=207;cards[1].token=7;
        gfo_state_meta meta{};meta.update_count=update;gfo_state* scene=nullptr;
        check(gfo_state_create(&meta,people.data(),2,fate?cards.data():nullptr,fate?2:0,nullptr,0,&scene)==0
            &&gfo_match_load_scene(into,scene,0,42,&result)==0,"initialize pure driver world without a future outcome");gfo_state_destroy(scene);
    };
    load_driver(driver_match,false,0,false);
    const auto driver_before=observe(driver_match,0,0);const auto driver_before_status=status(driver_match);
    for(uint64_t seed=0;seed<64;++seed){auto ordinary_rng=seed,choice_rng=seed;gfo_match_command ordinary{};gfo_match_choice chosen{};
        check(gfo_match_sample_action(driver_match,&ordinary_rng,&ordinary)==0&&gfo_match_sample_choice(driver_match,&choice_rng,&chosen)==0
            &&ordinary_rng==choice_rng&&!chosen.kind&&!chosen.fate_actor&&!chosen.fate_token&&equal(chosen.command,ordinary),
            "no207 choice delegates to ordinary policy without an envelope draw");}
    check(observe(driver_match,0,0)==driver_before&&equal(status(driver_match),driver_before_status),"sampling changes no world, decision or public history");
    gfo_match_choice driver_canary;std::memset(&driver_canary,0xa5,sizeof driver_canary);const auto driver_canary_before=driver_canary;
    uint64_t driver_policy=123,driver_chance=456;uint8_t driver_legal=177;
    check(gfo_match_sample_choice(nullptr,&driver_policy,&driver_canary)==-1
        &&gfo_match_sample_choice(driver_match,nullptr,&driver_canary)==-1
        &&gfo_match_sample_choice(driver_match,&driver_policy,nullptr)==-1
        &&driver_policy==123&&equal(driver_canary,driver_canary_before),"choice NULL failures preserve all208 output bytes and policy stream");
    load_driver(driver_match,true,0,false);load_driver(driver_manual,true,0,false);
    gfo_match_choice request{};request.kind=1;
    driver_policy=0;
    check(gfo_match_sample_choice(driver_match,&driver_policy,&driver_canary)==0&&equal(driver_canary,request)
        &&driver_policy==11400714819323198485ULL
        &&gfo_match_choice_legal(driver_match,&request,&driver_legal)==0&&driver_legal,"request actor0/token0 is an owned207 copy, with entirely zero inactive Command");
    for(std::size_t byte=0;byte<sizeof(gfo_match_command);++byte){auto bad=request;
        reinterpret_cast<unsigned char*>(&bad.command)[byte]=1;driver_legal=177;
        check(gfo_match_choice_legal(driver_match,&bad,&driver_legal)==-2&&driver_legal==177,"every inactive Command byte is checked before request processing");}
    for(unsigned error=0;error<11;++error){auto bad=request;
        if(error<7)bad.reserved[error]=1;if(error==7)bad.kind=2;
        if(error==8)bad.fate_actor=official::max_wire_integer+1;if(error==9)bad.fate_token=official::max_wire_integer+1;
        if(error==10){bad={};bad.fate_token=1;}driver_legal=177;
        check(gfo_match_choice_legal(driver_match,&bad,&driver_legal)==-2&&driver_legal==177,"variant tag, padding, safe IDs and inactive Fate fields are canonical");}
    gfo_match_fate_action supplied{};uint64_t manual_chance=2;driver_chance=2;
    check(gfo_match_sample_fate(driver_manual,0,&manual_chance,nullptr,0,&supplied)==0&&supplied.outcome.model==287
        &&gfo_match_fate_step(driver_manual,&supplied,&result)==0,"separate supplied outcome is a reference for Request resolution");
    const auto driver_expected=status(driver_manual);
    check(gfo_match_choice_step(driver_match,&request,&driver_chance,&result)==0&&equal(result,driver_expected)&&driver_chance==manual_chance
        &&observe(driver_match,0,0)==observe(driver_manual,0,0),"choice_step resolves Fate with chance only and matches explicit sample+fate transaction");
    load_driver(driver_match,true,0,true);const auto driver_fail_before=observe(driver_match,0,0);const auto driver_fail_status=status(driver_match);result=driver_fail_status;
    driver_policy=0;driver_chance=6;
    check(gfo_match_choice_legal(driver_match,&request,&driver_legal)==0&&driver_legal
        &&gfo_match_rollout_step(driver_match,&driver_policy,&driver_chance,&result)==-2
        &&driver_policy==0&&driver_chance==6&&equal(result,driver_fail_status)&&observe(driver_match,0,0)==driver_fail_before,
        "invalid sampled289 rolls back the request envelope, chance, Match and caller status without reroll");
    driver_chance=6;
    check(gfo_match_choice_step(driver_match,&request,&driver_chance,&result)==-2&&driver_chance==6
        &&equal(result,driver_fail_status)&&observe(driver_match,0,0)==driver_fail_before,"separate choice_step also preserves the current world and chance on rejection");
    uint64_t driver_alias=55;
    check(gfo_match_rollout_step(driver_match,&driver_alias,&driver_alias,&result)==-2&&driver_alias==55
        &&equal(result,driver_fail_status)&&observe(driver_match,0,0)==driver_fail_before,"identical RNG pointers are rejected before any selection");
    check(gfo_match_rollout_step(nullptr,&driver_policy,&driver_chance,&result)==-1
        &&gfo_match_rollout_step(driver_match,nullptr,&driver_chance,&result)==-1
        &&gfo_match_rollout_step(driver_match,&driver_policy,nullptr,&result)==-1
        &&gfo_match_rollout_step(driver_match,&driver_policy,&driver_chance,nullptr)==-1
        &&equal(result,driver_fail_status)&&driver_policy==0&&driver_chance==6,"driver NULL checks preserve both streams and output");
    load_driver(driver_match,false,official::max_wire_integer,false);const auto driver_late=observe(driver_match,0,0);const auto driver_late_status=status(driver_match);result=driver_late_status;
    driver_policy=1;driver_chance=2;
    check(gfo_match_rollout_step(driver_match,&driver_policy,&driver_chance,&result)==-2&&driver_policy==1&&driver_chance==2
        &&equal(result,driver_late_status)&&observe(driver_match,0,0)==driver_late,"late update failure restores pending policy selection and both original streams");
    load_driver(driver_match,false,0,false);driver_policy=driver_chance=0;
    check(gfo_match_rollout_step(driver_match,&driver_policy,&driver_chance,&result)==0&&result.decisions==1
        &&driver_policy!=0&&driver_chance==0,"equal RNG values in distinct storage are valid, ordinary command uses no external chance");
    gfo_match_destroy(driver_match);gfo_match_destroy(driver_manual);
    check(fixed_query_probes == 384, "all six fixed policy-query loops executed64 probes");
    gfo_match_destroy(blocked_match);
    gfo_match_destroy(reflected_match);
    gfo_match_destroy(short_match); gfo_match_destroy(a); gfo_match_destroy(b); gfo_match_destroy(nullptr);
    std::cout << "official match C API passed\n";
}
