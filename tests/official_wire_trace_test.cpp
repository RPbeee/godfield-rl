#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <span>
#include <string_view>
#include <utility>

namespace {
using namespace godfield::official;
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL official wire trace: " << message << '\n'; std::exit(1); }
}
// Numeric transcription of data/official-v2/wire-game-updates-v59.json.
// This is a privileged training fixture, including received CPU inventory. It
// must never become the input of a public-player policy. No JSON/runtime/network
// dependency is added. Slot IDs are received UI slots, not physical identities.
struct ReceivedSlot { std::uint16_t slot; CardId model; CardId fake = 0; bool used = false; };
constexpr std::array<ReceivedSlot,9> initial_viewer{{{1,199},{2,61},{3,4},{4,103},{5,123},{6,23},{7,79},{8,33},{9,3}}};
constexpr std::array<ReceivedSlot,9> initial_cpu{{{1,212},{2,135},{3,32},{4,161},{5,62},{6,199},{7,156},{8,68},{9,149}}};
constexpr std::array<ReceivedSlot,9> after_self_viewer{{{1,199},{2,61},{3,4},{4,103},{5,123},{7,79},{8,33},{9,3},{6,161}}};
constexpr std::array<ReceivedSlot,9> after_claw_cpu{{{1,212},{2,135},{3,32},{4,161},{6,199},{7,156},{8,68},{9,149},{0,0}}};
// Updates3/4: remaining rows are unchanged; the reused slots receive new models.
constexpr std::array<ReceivedSlot,9> after_defense_cpu{{{1,212},{2,135},{3,32},{4,161},{6,199},{7,156},{8,68},{9,149},{5,184}}};
constexpr std::array<ReceivedSlot,9> after_cure_viewer{{{2,61},{3,4},{4,103},{5,123},{7,79},{8,33},{9,3},{6,161},{1,123}}};
constexpr std::array<ReceivedSlot,9> after_halberd_cpu{{{1,212},{2,135},{4,161},{6,199},{7,156},{8,68},{9,149},{5,184},{0,0}}};
struct ReceivedPlayer {
    PlayerId id; int hp, mp, cp; bool cold; std::span<const ReceivedSlot> slots;
    bool darkcloud = false, dead = false, fog = false, flash = false;
};
struct ReceivedCheckpoint {
    unsigned update; std::int64_t gf; PlayerId owner;
    std::array<ReceivedPlayer,2> players; CardId pending = 0;
    std::span<const CardId> pending_models = {};
    bool ended = false;
    std::optional<PlayerId> pending_origin = {};
    std::optional<std::int64_t> pending_power = {};
    bool counter = false;
};
constexpr std::array checkpoints{
    ReceivedCheckpoint{1,1,1,{{{1,40,10,20,false,initial_viewer},{2,40,10,20,false,initial_cpu}}},0},
    ReceivedCheckpoint{2,2,2,{{{1,34,10,20,false,after_self_viewer},{2,40,10,20,false,after_claw_cpu}}},62},
    ReceivedCheckpoint{3,3,1,{{{1,33,10,20,true,after_self_viewer},{2,40,10,20,false,after_defense_cpu}}},0},
    ReceivedCheckpoint{4,4,2,{{{1,33,10,20,false,after_cure_viewer},{2,40,10,20,false,after_halberd_cpu}}},32}
};
using SlotKey = std::pair<PlayerId,std::uint16_t>;
using SlotMap = std::map<SlotKey,ItemToken>;
SnapshotState initial_state(SlotMap& slots) {
    SnapshotState state;state.gf=1;state.tiebreak_gf=1;
    // Received updateCount is intentionally not synchronized with native commands.
    for (const auto& received : checkpoints.front().players) {
        PlayerState player;player.id=received.id;player.hp=received.hp;player.mp=received.mp;player.cp=received.cp;
        for (const auto& item : received.slots) {
            const auto token=1000*player.id+item.slot;
            slots[{player.id,item.slot}]=token;
            player.items.push_back({token,item.model,{},false});
        }
        state.players.push_back(std::move(player));
    }
    return state;
}
void roundtrip(const Match& game) {
    Match restored;restored.restore(game.snapshot());
    check(restored.snapshot()==game.snapshot(),"every native boundary preserves its complete private replay proof");
}
ItemToken consumed_slot(SlotMap& slots,PlayerId owner,std::uint16_t slot) {
    const auto it=slots.find({owner,slot});check(it!=slots.end(),"selected received slot has a native token");
    const auto token=it->second;slots.erase(it);return token;
}
void compare_checkpoint(const Match& game,const SlotMap& slots,const ReceivedCheckpoint& expected,
                        std::optional<PlayerId> dream_player = {}) {
    const auto& actual=game.snapshot();
    check(actual.state.gf==expected.gf&&actual.owner==expected.owner,"observed GF and turn owner match checkpoint");
    check(actual.state.is_over==expected.ended&&actual.state.players.size()==2,"received terminal flag and roster match");
    std::size_t total_positive=0;
    for(std::size_t seat=0;seat<expected.players.size();++seat) {
        const auto& want=expected.players[seat];const auto& got=actual.state.players[seat];
        check(got.id==want.id&&got.hp==want.hp&&got.mp==want.mp&&got.cp==want.cp,"received gameplay resources match");
        check(got.team==Team::solo&&got.dead==want.dead&&got.guardian==Guardian::none&&got.confusion_count==0,"received default team, life and guardian state match");
        auto curses=want.cold?std::vector<Curse>{Curse::cold}:std::vector<Curse>{};
        if(dream_player==want.id)curses.push_back(Curse::dream);
        if(want.darkcloud)curses.push_back(Curse::darkcloud);
        if(want.fog)curses.push_back(Curse::fog);
        if(want.flash)curses.push_back(Curse::flash);
        check(got.curses==curses,"received curse state matches without adding inferred ticks");
        std::size_t positive=0,placeholders=0;
        for(const auto& row:want.slots) {
            if(!row.model){check(row.slot==0,"known wire placeholder is id0/model0");++placeholders;continue;}
            ++positive;
            const auto mapped=slots.find({want.id,row.slot});check(mapped!=slots.end(),"positive received slot has a current native mapping");
            const auto found=std::find_if(got.items.begin(),got.items.end(),[&](const auto& item){return item.token==mapped->second;});
            check(found!=got.items.end()&&found->model==row.model&&found->fake_model.value_or(0)==row.fake&&found->used==row.used,
                  "positive inventory model/fake/used match independently of server slot identity");
        }
        check(got.items.size()==positive&&actual.gifts[seat]==(expected.ended?0:placeholders),"physical inventory matches; terminal UI placeholders retain no live local debts");
        total_positive+=positive;
    }
    check(slots.size()==total_positive,"slot map contains no consumed stale slot or placeholder");
    if(expected.pending) {
        const auto defender=expected.players[0].id==expected.owner?expected.players[1].id:expected.players[0].id;
        check(actual.phase==MatchPhase::response&&actual.actor==defender&&actual.queue.size()==1,"pending CPU action has viewer as response actor");
        const auto& effect=actual.queue.front();const auto* card=find_card(expected.pending);
        const auto models=expected.pending_models.empty()?std::vector<CardId>{expected.pending}
            :std::vector<CardId>(expected.pending_models.begin(),expected.pending_models.end());
        const auto amount=card&&card->ability==Ability::sell&&models.size()==2
            ?find_card(models[1])->price:card?card->ability_value:0;
        check(card&&effect.models==models&&effect.origin==expected.pending_origin.value_or(expected.owner)&&effect.source==expected.owner&&effect.target==defender
              &&effect.category==card->category&&effect.power==expected.pending_power.value_or(attack_power(models))&&effect.element==attack_element(models)
              &&effect.ability==card->ability&&effect.amount==amount
              &&effect.hit_rate==0&&effect.is_counter==expected.counter&&!effect.is_death&&!effect.environmental,"pending source, target and derived attack match wire head");
        // Curse is derived from the retained base model, not a separate frame field.
        // The later received Cold state is also checked directly at update3.
        if(expected.pending==62)check(effect.ability==Ability::add_curse_on_damage&&card->curse==Curse::cold,
                                     "pending62 retains the on-damage Cold payload through its model");
    } else if(expected.ended) check(actual.phase==MatchPhase::finished&&actual.terminated&&!actual.truncated&&actual.queue.empty()
        &&!actual.batch&&!actual.pending_grant,"received end has no remaining response,batch or live grant");
    else check(actual.phase==MatchPhase::main&&actual.actor==expected.owner&&actual.queue.empty(),"quiescent received boundary is main with no pending attack");
    // Deliberately excluded: native update_count/decisions/RNG/cumulative counters,
    // server transaction grouping, raw slot identity and raw inventory order.
    roundtrip(game);
}
void supplied_receipt(Match& game,SlotMap& slots,PlayerId owner,std::uint16_t reused_slot,CardId model) {
    check(game.snapshot().phase==MatchPhase::grant&&game.snapshot().pending_grant==owner,"one ordinary debt is waiting at the recorded recipient");
    const auto old_rng=game.snapshot().rng;const auto fresh=game.snapshot().next_token;
    check(slots.find({owner,reused_slot})==slots.end(),"consumed server slot is free before its reuse");
    const GrantAction grant{owner,model,{},{}};check(game.grant_legal(grant),"received ordinary model is legal without a selector");
    (void)game.grant_step(grant);
    check(game.snapshot().rng==old_rng,"explicit ordinary receipt consumes no environment RNG");
    slots[{owner,reused_slot}]=fresh;
    check(fresh!=1000*owner+reused_slot,"same server slot does not imply retained physical identity");
    roundtrip(game);
}
void replay(std::uint64_t seed) {
    MatchConfig config;config.initial_hand=0;config.external_grants=true;config.max_decisions=64;
    Match game(config);SlotMap slots;game.load_scene(initial_state(slots),1,seed);
    compare_checkpoint(game,slots,checkpoints[0]);
    const Command self{1,CommandKind::play,{consumed_slot(slots,1,6)},1};
    check(game.legal(self),"hard hammer explicit self play is legal");(void)game.step(self);roundtrip(game);
    supplied_receipt(game,slots,1,6,161);
    const Command claw{2,CommandKind::play,{consumed_slot(slots,2,5)},1};
    check(game.legal(claw),"received CPU62 launch is legal");(void)game.step(claw);roundtrip(game);
    compare_checkpoint(game,slots,checkpoints[1]);
    const Command empty{1,CommandKind::defend,{},{}};
    check(game.legal(empty),"received empty response is legal");(void)game.step(empty);roundtrip(game);
    // CPU184 is inferred from the received post3 inventory delta, not a captured
    // CPU gift event or evidence that the hidden grant event list was complete.
    supplied_receipt(game,slots,2,5,184);
    compare_checkpoint(game,slots,checkpoints[2]);
    const Command cure{1,CommandKind::play,{consumed_slot(slots,1,1)},1};
    check(game.legal(cure),"received self mild cure is legal");(void)game.step(cure);roundtrip(game);
    supplied_receipt(game,slots,1,1,123);
    const Command halberd{2,CommandKind::play,{consumed_slot(slots,2,3)},1};
    check(game.legal(halberd),"received CPU32 launch is legal");(void)game.step(halberd);roundtrip(game);
    compare_checkpoint(game,slots,checkpoints[3]);
}

// Numeric transcription of data/official-v2/wire-game-updates-v60.json.
// CPU32/id9 +51/id1 are restored ONLY from the received useAttackItems event.
// Appending these consumed copies reconstructs a deterministic pre-launch scene;
// it does not recover the original initial draw or its unobserved array order.
constexpr std::array<ReceivedSlot,9> v60_pending_cpu{{{2,26},{3,5},{4,191},{5,57},{6,206},{7,200},{8,26},{0,0},{0,0}}};
constexpr std::array<ReceivedSlot,9> v60_pending_viewer{{{1,5},{2,19},{3,118},{4,232},{5,142},{6,26},{7,87},{8,38},{9,13}}};
constexpr std::array<ReceivedSlot,7> v60_terminal_cpu{{{2,26},{3,5},{5,57},{7,200},{8,26},{1,135},{4,172}}};
constexpr std::array<ReceivedSlot,9> v60_terminal_viewer{{{1,5},{2,19},{4,232},{6,26},{7,87},{8,38},{9,13},{3,68},{0,0}}};
constexpr std::array<ReceivedSlot,2> v60_received_launch{{{9,32},{1,51}}};
void compare_v60(const Match& game,const SlotMap& slots,bool terminal) {
    const auto& snap=game.snapshot();
    check(snap.state.gf==1&&snap.state.tiebreak_gf==1&&snap.owner==1&&snap.state.is_over==terminal,"v60 received GF/threshold/owner/ending match");
    check(snap.state.players.size()==2,"v60 received roster size");
    std::size_t positive_total=0;
    for(std::size_t seat=0;seat<2;++seat) {
        const auto& player=snap.state.players[seat];
        const auto rows=seat==0?(terminal?std::span<const ReceivedSlot>(v60_terminal_cpu):std::span<const ReceivedSlot>(v60_pending_cpu))
                               :(terminal?std::span<const ReceivedSlot>(v60_terminal_viewer):std::span<const ReceivedSlot>(v60_pending_viewer));
        check(player.id==seat+1&&player.hp==(seat==0?40:terminal?0:40)&&player.mp==10&&player.cp==20,
              "v60 both received resource triples match");
        check(player.team==Team::solo&&player.guardian==Guardian::none&&player.curses.empty()&&player.confusion_count==0
              &&player.dead==(terminal&&seat==1),"v60 received alive/team/curse/guardian defaults match");
        std::size_t positive=0,empty=0;
        for(const auto& row:rows) {
            if(!row.model){check(row.slot==0,"v60 known empty display row");++empty;continue;}
            ++positive;const auto mapped=slots.find({player.id,row.slot});check(mapped!=slots.end(),"v60 qualified slot mapping exists");
            const auto item=std::find_if(player.items.begin(),player.items.end(),[&](const auto& x){return x.token==mapped->second;});
            check(item!=player.items.end()&&item->model==row.model&&item->fake_model.value_or(0)==row.fake&&item->used==row.used,
                  "v60 positive model/fake/used match, without equating slot and physical token");
        }
        check(player.items.size()==positive,"v60 positive copies exclude all renderer placeholders");
        if(!terminal)check(snap.gifts[seat]==empty,"v60 live placeholder count matches local reserved debts");
        else check(snap.gifts[seat]==0,"terminal cancellation leaves no live debt despite dead-viewer empty sprite");
        positive_total+=positive;
    }
    check(slots.size()==positive_total,"v60 no stale mappings for removed/consumed copies");
    if(!terminal) {
        check(snap.phase==MatchPhase::response&&snap.actor==2&&snap.queue.size()==1,"v60 checkpoint1 waits for viewer defense");
        const auto& effect=snap.queue.front();
        check(effect.origin==1&&effect.source==1&&effect.target==2&&effect.models==std::vector<CardId>{32,51}
              &&effect.category==Category::weapons&&effect.power==22&&!effect.element&&effect.ability==Ability::none
              &&effect.hit_rate==0&&!effect.is_counter&&!effect.is_death&&!effect.environmental,
              "v60 reconstructed received32+51 is one neutral22 initial payload");
    } else {
        check(game.done()&&snap.terminated&&!snap.truncated&&snap.phase==MatchPhase::finished&&snap.queue.empty()
              &&snap.winners==std::vector<PlayerId>{1},"v60 viewer death ends the match with CPU winner before GF advance");
        check(snap.grant_resolutions==6&&snap.grant_artifacts==3&&snap.grant_devils==3,
              "v60 selected completion has three explicit ordinary receipts and three recorded devils");
    }
    // Native inventory order, fresh IDs, RNG and command counters are NOT
    // compared with the server. The terminal dead-player empty row is not debt.
    roundtrip(game);
}
void replay_v60(std::uint64_t seed,bool reverse_cpu_receipts) {
    MatchConfig config;config.initial_hand=0;config.external_grants=true;config.max_decisions=64;
    Match game(config);SlotMap slots;SnapshotState state;state.gf=1;state.tiebreak_gf=1;
    for(std::size_t seat=0;seat<2;++seat) {
        PlayerState player;player.id=seat+1;
        const auto rows=seat==0?std::span<const ReceivedSlot>(v60_pending_cpu):std::span<const ReceivedSlot>(v60_pending_viewer);
        const auto add=[&](const ReceivedSlot& row){
            if(!row.model)return;
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,{},false});
        };
        for(const auto& row:rows)add(row);
        if(seat==0)for(const auto& row:v60_received_launch)add(row);
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);roundtrip(game);
    const Command launch{1,CommandKind::play,{consumed_slot(slots,1,9),consumed_slot(slots,1,1)},2};
    check(game.legal(launch),"v60 event-restored CPU composition is accepted");(void)game.step(launch);roundtrip(game);
    compare_v60(game,slots,false);
    check(find_card(118)->defense+find_card(142)->defense==8,"v60 catalog defense is eight rather than received damage injection");
    const Command defense{2,CommandKind::defend,{consumed_slot(slots,2,3),consumed_slot(slots,2,5)},{}};
    check(game.legal(defense),"v60 received two-card defense is accepted");(void)game.step(defense);roundtrip(game);
    check(game.snapshot().state.players[1].hp==26&&game.snapshot().pending_grant==1,"v60 computed22-minus8 is14 before any devil");
    const GrantAction thief{1,243,{}, {consumed_slot(slots,1,4),consumed_slot(slots,1,6)}};
    check(game.grant_legal(thief),"v60 recorded thief choices are valid");(void)game.grant_step(thief);roundtrip(game);
    check(game.snapshot().gifts==std::vector<std::uint32_t>{2,2},"v60 thief removal does not create compensation debt");
    // These two CPU normal outcomes are established only by the terminal
    // inventory delta. Their missing event order is NOT fitted: exercise both
    // admissible orders as separate local completions and qualify each slot map.
    if(reverse_cpu_receipts){supplied_receipt(game,slots,1,4,172);supplied_receipt(game,slots,1,1,135);}
    else {supplied_receipt(game,slots,1,1,135);supplied_receipt(game,slots,1,4,172);}
    supplied_receipt(game,slots,2,3,68);
    check(game.snapshot().pending_grant==2&&game.snapshot().state.players[1].hp==26,"v60 viewer first recorded normal result leaves one original gift debt");
    const GrantAction medium{2,241,{},{}};
    check(game.grant_legal(medium),"v60 recorded medium devil is legal");(void)game.grant_step(medium);roundtrip(game);
    check(game.snapshot().state.players[1].hp==6&&game.snapshot().pending_grant==2,"v60 medium20 leaves HP6 and retries the same local debt");
    const GrantAction large{2,242,{},{}};
    check(game.grant_legal(large),"v60 recorded large devil is legal");(void)game.grant_step(large);roundtrip(game);
    compare_v60(game,slots,true);
}

// Numeric transcription of data/official-v2/wire-game-updates-v62.json.
// These are privileged received inventories, not policy observations. Update2
// contains both main operations. CPU207 is a snapshot delta, not a captured CPU
// gift event. The existing eight fixed seeds all take the observed non-worsening
// Heaven branch; no seed search, exclusion, skip or supplied disease outcome is
// used. This is conditional trace agreement, not server RNG/distribution proof.
constexpr std::array<ReceivedSlot,9> v62_initial_viewer{{{1,35},{2,135},{3,199},{4,155},{5,3},{6,4},{7,31},{8,156},{9,198}}};
constexpr std::array<ReceivedSlot,9> v62_initial_cpu{{{1,118},{2,157},{3,44},{4,135},{5,219},{6,5},{7,3},{8,191},{9,212}}};
constexpr std::array<ReceivedSlot,9> v62_final_viewer{{{1,35},{2,135},{3,199},{4,155},{5,3},{6,4},{7,31},{8,156},{9,191}}};
constexpr std::array<ReceivedSlot,9> v62_final_cpu{{{1,118},{2,157},{3,44},{4,135},{5,219},{6,5},{7,3},{9,212},{8,207}}};
void compare_v62(const Match& game,const SlotMap& slots,bool final) {
    const auto& snap=game.snapshot();
    check(snap.config.external_artifacts&&snap.config.include_fate&&!snap.config.external_grants,
          "v62 uses the actual full ordinary gift pool and normal-only reserved input mode");
    check(snap.state.gf==(final?3:1)&&!snap.state.tiebreak_gf&&snap.owner==1&&snap.actor==1&&snap.phase==MatchPhase::main
          &&!snap.state.is_over&&!snap.terminated&&!snap.truncated&&snap.queue.empty()&&!snap.pending_grant,
          "v62 received initial/final GF, owner, no-apocalypse and quiescent boundaries match");
    check(snap.state.players.size()==2&&slots.size()==18&&snap.gifts==std::vector<std::uint32_t>{0,0},"v62 exactly eighteen positive mapped copies and no residual debt");
    for(std::size_t seat=0;seat<2;++seat) {
        const auto& player=snap.state.players[seat];
        const auto& rows=seat==0?(final?v62_final_viewer:v62_initial_viewer):(final?v62_final_cpu:v62_initial_cpu);
        check(player.id==seat+1&&player.hp==(final?45:40)&&player.mp==(final&&seat==0?30:10)&&player.cp==20
              &&player.items.size()==rows.size()&&!player.dead&&player.guardian==Guardian::none&&player.confusion_count==0&&player.team==Team::solo,
              "v62 received resources, alive/default state and physical counts match");
        const auto curses=final&&seat==0?std::vector<Curse>{Curse::heaven}:std::vector<Curse>{};
        check(player.curses==curses,"v62 Heaven is applied only to its observed recipient");
        for(const auto& row:rows) {
            const auto mapped=slots.find({player.id,row.slot});check(mapped!=slots.end(),"v62 positive row has a qualified local mapping");
            check(std::find(player.items.begin(),player.items.end(),ItemInstance{mapped->second,row.model,{},false})!=player.items.end(),
                  "v62 actual model, absent fake and unused state match without received-slot/native-token identity");
        }
    }
    if(final)check(snap.grant_resolutions==2&&snap.grant_artifacts==2&&snap.grant_devils==0&&snap.decisions==4,
                  "v62 local completion contains two player plays and two explicit normal receipts");
    // Received transaction counters, RNG, inventory order and native fresh IDs
    // are excluded. The intermediate grant boundaries are local, not extra
    // received checkpoints. All old nonselected mapped copies remain intact.
    roundtrip(game);
}
void replay_v62(std::uint64_t seed) {
    MatchConfig config;config.initial_hand=0;config.max_decisions=64;config.external_artifacts=true;config.include_fate=true;
    Match game(config);SlotMap slots;SnapshotState state;state.gf=1;
    for(std::size_t seat=0;seat<2;++seat) {
        PlayerState player;player.id=seat+1;
        const auto& rows=seat==0?v62_initial_viewer:v62_initial_cpu;
        for(const auto& row:rows){const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;player.items.push_back({token,row.model,{},false});}
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);compare_v62(game,slots,false);
    check(find_card(198)->ability==Ability::boost_mp_and_add_curse&&find_card(198)->ability_value==20&&find_card(198)->curse==Curse::heaven
          &&find_card(191)->ability==Ability::boost_hp&&find_card(191)->ability_value==5,"v62 resource changes are derived from catalog effects, not supplied damage");
    const Command herb{1,CommandKind::play,{consumed_slot(slots,1,9)},1};
    check(game.legal(herb),"v62 observed self198 is legal");(void)game.step(herb);roundtrip(game);
    check(game.snapshot().phase==MatchPhase::grant&&game.snapshot().pending_grant==1&&game.snapshot().state.gf==1
          &&game.snapshot().state.players[0].hp==45&&game.snapshot().state.players[0].mp==30
          &&game.snapshot().last_tick&&game.snapshot().last_tick->before==Curse::heaven&&game.snapshot().last_tick->after==Curse::heaven
          &&!game.snapshot().last_tick->worsened&&!game.snapshot().last_tick->fallen,
          "v62 existing fixed seed takes the recorded Heaven non-worsening branch before its pending ordinary receipt");
    supplied_receipt(game,slots,1,9,191);
    const Command dew{2,CommandKind::play,{consumed_slot(slots,2,8)},2};
    check(game.legal(dew),"v62 observed CPU self191 is legal");(void)game.step(dew);roundtrip(game);
    check(game.snapshot().pending_grant==2&&game.snapshot().state.gf==2&&game.snapshot().state.players[1].hp==45,
          "v62 CPU recovery completes before the inferred ordinary inventory delta");
    supplied_receipt(game,slots,2,8,207);compare_v62(game,slots,true);
}

// v63 has one received post-action update, not an authoritative initial frame.
// Reinsert only the consumed copies named by useAttackItems into the remaining
// received inventories. This is a deterministic reconstructed pre-play scene;
// neither original draw/order nor original physical token identities are known.
constexpr std::array<ReceivedSlot,9> v63_before_viewer{{{1,80},{2,20},{3,195},{4,135},{5,170},{6,144},{7,19},{8,193},{9,162}}};
constexpr std::array<ReceivedSlot,9> v63_before_cpu{{{1,195},{2,60},{3,94},{4,14},{5,3},{6,23},{7,3},{8,3},{9,166}}};
constexpr std::array<ReceivedSlot,9> v63_after_viewer{{{2,20},{3,195},{4,135},{5,170},{6,144},{7,19},{8,193},{9,162},{1,135,135}}};
constexpr std::array<ReceivedSlot,9> v63_after_cpu{{{1,195},{3,94},{4,14},{5,3},{6,23},{7,3},{8,3},{9,166},{0,0}}};
void replay_v63(std::uint64_t seed) {
    MatchConfig config;config.initial_hand=0;config.max_decisions=64;config.external_artifacts=true;
    Match game(config);SlotMap slots;SnapshotState state;state.gf=1;
    for(std::size_t seat=0;seat<2;++seat) {
        PlayerState player;player.id=seat+1;
        const auto& rows=seat==0?v63_before_viewer:v63_before_cpu;
        for(const auto& row:rows) {
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,{},false});
        }
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);roundtrip(game);
    const Command mallet{1,CommandKind::play,{consumed_slot(slots,1,1)},1};
    check(game.legal(mallet),"v63 received self80 selection is legal");(void)game.step(mallet);roundtrip(game);
    const auto& pending=game.snapshot();
    check(pending.phase==MatchPhase::grant&&pending.pending_grant==1&&pending.queue.empty()
          &&pending.state.players[0].hp==36&&pending.state.players[0].curses==std::vector<Curse>{Curse::dream},
          "v63 one play resolves self80 damage4/Dream without a separate defense command");
    const auto rng=pending.rng;const auto fresh=pending.next_token;
    const ArtifactGrantAction received{1,135,135};
    check(game.artifact_grant_legal(received),"v63 explicit received identity disguise is legal");
    (void)game.artifact_grant_step(received);slots[{1,1}]=fresh;roundtrip(game);
    check(game.snapshot().rng==rng&&fresh!=1001,"v63 identity grant consumes no RNG and reused slot gets a fresh local token");
    const Command icicle{2,CommandKind::play,{consumed_slot(slots,2,2)},1};
    check(game.legal(icicle),"v63 received CPU60 launch is legal");(void)game.step(icicle);roundtrip(game);
    const auto& snap=game.snapshot();
    check(snap.state.gf==2&&snap.owner==2&&!snap.state.is_over&&snap.state.players.size()==2,
          "v63 received post-only GF/owner/live roster match");
    check(snap.phase==MatchPhase::response&&snap.actor==1&&snap.queue.size()==1,
          "v63 received CPU launch waits for viewer response");
    const auto& effect=snap.queue.front();
    check(effect.models==std::vector<CardId>{60}&&effect.origin==2&&effect.source==2&&effect.target==1
          &&effect.category==Category::weapons&&effect.power==1&&effect.element==Element::water
          &&effect.ability==find_card(60)->ability&&effect.amount==find_card(60)->ability_value
          &&effect.hit_rate==0&&!effect.is_counter&&!effect.is_death&&!effect.environmental,
          "v63 received pending icicle has derived Water1 payload");
    std::size_t total=0;
    for(std::size_t seat=0;seat<2;++seat) {
        const auto& player=snap.state.players[seat];const auto& rows=seat==0?v63_after_viewer:v63_after_cpu;
        check(player.id==seat+1&&player.hp==(seat==0?36:40)&&player.mp==10&&player.cp==20
              &&!player.dead&&player.team==Team::solo&&player.guardian==Guardian::none&&player.confusion_count==0,
              "v63 received resource/alive/team/guardian defaults match");
        check(player.curses==(seat==0?std::vector<Curse>{Curse::dream}:std::vector<Curse>{}),
              "v63 received Dream only on self80 user");
        std::size_t positive=0,empty=0;
        for(const auto& row:rows) {
            if(!row.model){++empty;continue;}
            ++positive;const auto mapped=slots.find({player.id,row.slot});check(mapped!=slots.end(),"v63 qualified slot mapping exists");
            const auto item=std::find_if(player.items.begin(),player.items.end(),[&](const auto& x){return x.token==mapped->second;});
            check(item!=player.items.end()&&item->model==row.model&&item->fake_model.value_or(0)==row.fake
                  &&item->fake_model.has_value()==(row.fake!=0)&&item->used==row.used,
                  "v63 received model, explicit fake presence and unused status match all positive copies");
        }
        check(player.items.size()==positive&&snap.gifts[seat]==empty,"v63 pending CPU debt matches its received placeholder");total+=positive;
    }
    check(slots.size()==total,"v63 no stale consumed slot mappings");
    check(game.observe(1).find("\"disguised\":true")!=std::string::npos,"v63 identity copy retains owner-visible disguise marker");
    // updateCount2 is one received grouping, not the native command counter.
    // CPU true inventory is privileged fixture evidence, never policy input.
}


// v64: four received updates, after a reconstructed GF3 pre-play scene.
// Numeric copies below retain the received model/fake/used values. The local
// initializer uses slot order only as a deterministic fixture convention.
namespace v64 {
constexpr std::array<ReceivedSlot,9> before_viewer{{{1,135,135},{2,20},{3,195},{4,135},{5,170},{6,144},{7,19},{8,193},{9,162}}};
constexpr std::array<std::array<ReceivedSlot,9>,4> after_viewer{{
    {{{2,20},{4,135},{5,170},{6,144},{7,19},{8,193},{9,162},{1,135,135},{3,195,195}}},
    {{{2,20},{4,135},{5,170},{6,144},{7,19},{8,193},{9,162},{3,195,195},{1,213}}},
    {{{2,20},{4,135},{5,170},{6,144},{7,19},{8,193},{9,162},{1,213},{3,197,195}}},
    {{{2,20},{4,135},{5,170},{6,144},{7,19},{8,193},{9,162},{1,213},{3,13,13}}}
}};
constexpr std::array<ReceivedSlot,9> before_cpu{{{1,195},{2,64},{3,94},{4,14},{5,3},{6,23},{7,3},{8,3},{9,166}}};
constexpr std::array<std::array<ReceivedSlot,9>,4> after_cpu{{
    {{{1,195},{3,94},{4,14},{5,3},{7,3},{8,3},{9,166},{2,64},{0,0}}},
    {{{1,195},{3,94},{4,14},{5,3},{7,3},{8,3},{9,166},{2,64},{6,188}}},
    {{{1,195},{3,94},{4,14},{5,3},{7,3},{9,166},{2,64},{6,188},{8,196}}},
    {{{3,94},{4,14},{5,3},{7,3},{9,166},{2,64},{6,188},{8,196},{1,202}}}
}};

void receipt(Match& game,SlotMap& slots,PlayerId owner,std::uint16_t slot,CardId model,CardId face=0) {
    check(game.snapshot().phase==MatchPhase::grant&&game.snapshot().pending_grant==owner,
          "v64 recorded recipient has an ordinary debt at the local boundary");
    check(slots.find({owner,slot})==slots.end(),"v64 reused slot is free after its previous copy was consumed");
    const auto rng=game.snapshot().rng;const auto fresh=game.snapshot().next_token;
    const ArtifactGrantAction action{owner,model,face?std::optional<CardId>{face}:std::nullopt};
    check(game.artifact_grant_legal(action),"v64 received actual/fake result is legal");
    (void)game.artifact_grant_step(action);slots[{owner,slot}]=fresh;
    check(game.snapshot().rng==rng&&game.snapshot().dream_rolls==0&&game.snapshot().dream_masks==0,
          "v64 explicit receipts leave game RNG and local generation statistics unchanged");roundtrip(game);
}
void compare(const Match& game,const SlotMap& slots,std::size_t checkpoint,std::uint64_t seed) {
    const auto& snap=game.snapshot();constexpr std::array gf{4,5,7,9};
    constexpr std::array hp{35,34,34,34},mp{15,15,20,35};
    constexpr std::array cpu_hp{40,40,30,30},cpu_mp{10,10,30,35},cpu_cp{20,20,10,10};
    check(snap.state.gf==gf[checkpoint]&&snap.owner==(checkpoint?1:2)&&!snap.state.is_over
          &&!snap.terminated&&!snap.truncated&&snap.state.players.size()==2,"v64 received GF/owner/nonterminal roster match");
    check(snap.rng==seed&&snap.dream_rolls==0&&snap.dream_masks==0,
          "v64 no seed fitting or local body/mask draws are needed for these finite results");
    std::size_t total=0;
    for(std::size_t seat=0;seat<2;++seat) {
        const auto& player=snap.state.players[seat];const auto& rows=seat?after_cpu[checkpoint]:after_viewer[checkpoint];
        check(player.id==seat+1&&player.hp==(seat?cpu_hp[checkpoint]:hp[checkpoint])
              &&player.mp==(seat?cpu_mp[checkpoint]:mp[checkpoint])&&player.cp==(seat?cpu_cp[checkpoint]:20),
              "v64 both received resource triples match");
        check(!player.dead&&player.team==Team::solo&&player.guardian==Guardian::none&&player.confusion_count==0
              &&player.curses==(seat?std::vector<Curse>{}:std::vector<Curse>{Curse::dream}),
              "v64 received live/team/guardian/curse defaults match");
        std::size_t positive=0,empty=0;
        for(const auto& row:rows) {
            if(!row.model){check(row.slot==0,"v64 canonical received empty slot");++empty;continue;}
            ++positive;const auto mapping=slots.find({player.id,row.slot});check(mapping!=slots.end(),"v64 received slot has its current native token");
            const auto item=std::find_if(player.items.begin(),player.items.end(),[&](const auto& x){return x.token==mapping->second;});
            check(item!=player.items.end()&&item->model==row.model&&item->fake_model.value_or(0)==row.fake
                  &&item->fake_model.has_value()==(row.fake!=0)&&item->used==row.used,
                  "v64 all positive models, explicit fake presence and used status match");
        }
        check(player.items.size()==positive&&snap.gifts[seat]==empty,"v64 positive copies and live placeholder debts match");total+=positive;
    }
    check(slots.size()==total,"v64 no stale consumed slot mappings remain");
    if(checkpoint==0) {
        check(snap.phase==MatchPhase::response&&snap.actor==1&&snap.queue.size()==1,"v64 update4 waits for viewer defense");
        const auto& attack=snap.queue.front();
        check(attack.models==std::vector<CardId>{23}&&attack.origin==2&&attack.source==2&&attack.target==1
              &&attack.category==Category::weapons&&attack.power==6&&!attack.element&&attack.ability==Ability::none
              &&attack.hit_rate==0&&!attack.is_death&&!attack.is_counter&&!attack.environmental,
              "v64 pending neutral CPU23 is derived from the received launch model");
    } else check(snap.phase==MatchPhase::main&&snap.actor==1&&snap.queue.empty(),"v64 later checkpoints are viewer main");
    // Excluded: server updateCount vs native decisions, initial draws/order,
    // permanent slot identity and the omitted CPU gift/retry event ordering.
    roundtrip(game);
}
void replay(std::uint64_t seed) {
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SlotMap slots;SnapshotState state;state.gf=3;
    for(std::size_t seat=0;seat<2;++seat) {
        PlayerState player;player.id=seat+1;
        if(!seat){player.hp=35;player.curses={Curse::dream};}
        const auto& rows=seat?before_cpu:before_viewer;
        for(const auto& row:rows) {
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,row.fake?std::optional<CardId>{row.fake}:std::nullopt,row.used});
        }
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);roundtrip(game);
    const auto play_slot=[&](PlayerId owner,std::uint16_t slot,PlayerId target) {
        const Command action{owner,CommandKind::play,{consumed_slot(slots,owner,slot)},target};
        check(game.legal(action),"v64 received player choice is legal using its visible face");(void)game.step(action);roundtrip(game);
    };
    play_slot(1,3,1);receipt(game,slots,1,3,195,195);
    play_slot(2,6,1);compare(game,slots,0,seed);
    const Command defense{1,CommandKind::defend,{consumed_slot(slots,1,1)},{}};
    check(game.legal(defense),"v64 received identity135 DEF5 selection is legal");(void)game.step(defense);roundtrip(game);
    check(game.snapshot().state.players[0].hp==34&&game.snapshot().last_reveals==std::vector<RevealedItem>{{1,135,135,false}},
          "v64 identity defense consumes actual DEF5 and publishes the same-model reveal");
    receipt(game,slots,1,1,213);receipt(game,slots,2,6,188);compare(game,slots,1,seed);
    play_slot(1,3,1);
    check(game.snapshot().state.players[0].mp==20&&game.snapshot().last_reveals==std::vector<RevealedItem>{{1,195,195,false}},
          "v64 identity195 self use applies MP5 and retains explicit presence in its use reveal");
    receipt(game,slots,1,3,197,195);
    const auto public_hand=project_view(game.snapshot().state,1).hand;
    const auto visible=std::find_if(public_hand.begin(),public_hand.end(),[&](const auto& x){return x.token==slots.at({1,3});});
    check(visible!=public_hand.end()&&visible->face==195&&visible->reference==195&&visible->disguised
          &&match_dream_compatible(197,195),"v64 actual197 remains hidden behind compatible face195 before use");
    const Command exchange{2,CommandKind::exchange,{consumed_slot(slots,2,8)},{},ResourceAllocation{30,30,10}};
    check(game.legal(exchange),"v64 received CPU exchange conserves the resource sum");(void)game.step(exchange);roundtrip(game);
    receipt(game,slots,2,8,196);compare(game,slots,2,seed);
    play_slot(1,3,1);
    check(game.snapshot().state.players[0].mp==35&&game.snapshot().last_reveals==std::vector<RevealedItem>{{1,195,197,false}},
          "v64 visible MP5 flower resolves actual197 MP15 and publishes the different-model reveal");
    receipt(game,slots,1,3,13,13);play_slot(2,1,2);receipt(game,slots,2,1,202);compare(game,slots,3,seed);
    // CPU188/196/202 are inventory-delta completions, not captured gift events.
    // The next CPU main clears native per-action notices, unlike a server batch
    // which can aggregate both commands. Reveal assertions are made before it.
}
}


// v65 has two separate training games. Only the owned207/Fog game is a full
// conditional replay. Neptune's observed201 draw and automatic move selector
// are not reproduced by choosing or searching native seeds.
namespace v65 {
constexpr std::array<std::array<ReceivedSlot,9>,3> fate_viewer{{
    {{{1,211},{2,74},{3,32},{4,3},{5,33},{6,142},{7,207},{8,69},{9,16}}},
    {{{1,211},{2,74},{3,32},{4,3},{5,33},{6,142},{8,69},{9,16},{7,192}}},
    {{{1,211},{2,74},{3,32},{4,3},{5,33},{8,69},{9,16},{7,192},{6,166}}}
}};
constexpr std::array<ReceivedSlot,10> component_viewer{{{1,176},{2,67},{3,142},{5,23},{7,23},{8,4},{9,26},{6,196},{4,233,0,true},{10,123}}};
constexpr std::array<std::array<ReceivedSlot,9>,3> fate_cpu{{
    {{{1,144},{2,35},{3,89},{4,156},{5,38},{6,76},{7,109},{8,170},{9,182}}},
    {{{1,144},{2,35},{3,89},{4,156},{5,38},{7,109},{8,170},{9,182},{0,0}}},
    {{{1,144},{2,35},{3,89},{4,156},{5,38},{7,109},{8,170},{9,182},{6,26}}}
}};
constexpr std::array<ReceivedSlot,10> component_cpu{{{1,3},{3,173},{4,4},{6,196},{7,3},{8,3},{9,113},{5,231,0,true},{2,200},{10,19}}};

void compare_fate(const Match& game,const SlotMap& slots,std::size_t checkpoint) {
    const auto& snap=game.snapshot();
    check(snap.state.gf==static_cast<std::int64_t>(checkpoint+1)&&snap.owner==(checkpoint==1?2:1)
          &&!snap.terminated&&!snap.truncated&&!snap.state.is_over&&snap.state.players.size()==2,
          "v65 Fate received GF/owner/nonterminal roster match");
    std::size_t total=0;
    for(std::size_t seat=0;seat<2;++seat) {
        const auto& player=snap.state.players[seat];const auto& rows=seat?fate_cpu[checkpoint]:fate_viewer[checkpoint];
        check(player.id==seat+1&&player.hp==40&&player.mp==10&&player.cp==20&&!player.dead
              &&player.team==Team::solo&&player.guardian==Guardian::none&&player.confusion_count==0,
              "v65 both received resources and live/team/guardian defaults match");
        check(player.curses==(checkpoint?std::vector<Curse>{Curse::fog}:std::vector<Curse>{}),
              "v65 supplied288 sets Fog on everyone and defended Dark does not clear it");
        std::size_t positive=0,empty=0;
        for(const auto& row:rows) {
            if(!row.model){check(row.slot==0,"v65 received canonical placeholder");++empty;continue;}
            ++positive;const auto map=slots.find({player.id,row.slot});check(map!=slots.end(),"v65 qualified received slot has a native token");
            const auto item=std::find_if(player.items.begin(),player.items.end(),[&](const auto& x){return x.token==map->second;});
            check(item!=player.items.end()&&item->model==row.model&&!item->fake_model&&item->used==row.used,
                  "v65 every positive model,absent fake and used status matches");
        }
        check(player.items.size()==positive&&snap.gifts[seat]==empty,"v65 physical counts and live placeholder debt match");total+=positive;
    }
    check(slots.size()==total,"v65 no stale consumed slot mappings");
    if(checkpoint==1) {
        check(snap.phase==MatchPhase::response&&snap.actor==1&&snap.queue.size()==1,"v65 Fog CPU launch waits for viewer defense");
        const auto& attack=snap.queue.front();
        check(attack.models==std::vector<CardId>{76}&&attack.origin==2&&attack.source==2&&attack.target==1
              &&attack.category==Category::weapons&&attack.power==3&&attack.element==Element::darkness
              &&attack.ability==Ability::none&&attack.hit_rate==0&&!attack.is_counter&&!attack.is_death,
              "v65 received Cobra retains its derived Dark3 payload");
    } else check(snap.phase==MatchPhase::main&&snap.actor==1&&snap.queue.empty(),"v65 initial and final checkpoints are viewer main");
    if(checkpoint) {
        const auto own=game.observe(1);
        check(own.find("\"fogged\":true")!=std::string::npos&&own.find("\"status\":null")!=std::string::npos,
              "v65 opponent resources remain hidden in the policy view despite the privileged fixture");
    }
    // No server updateCount/native decision equality or backend gift order claim.
    // Fog's one-enemy target still uses its local draw; no seed is selected/skipped.
    roundtrip(game);
}
void replay_fate(std::uint64_t seed) {
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.include_fate=true;config.max_decisions=64;
    Match game(config);SlotMap slots;SnapshotState state;state.gf=1;
    for(std::size_t seat=0;seat<2;++seat) {
        PlayerState player;player.id=seat+1;const auto& rows=seat?fate_cpu[0]:fate_viewer[0];
        for(const auto& row:rows){const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;player.items.push_back({token,row.model,{},false});}
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);compare_fate(game,slots,0);
    const FateAction fate{1,consumed_slot(slots,1,7),PhenomenonAction{288,{},{}},{}};
    check(game.fate_legal(fate),"v65 owned207 accepts the received288 result through FateAction");(void)game.fate_step(fate);roundtrip(game);
    check(game.snapshot().fate_actions==1&&game.snapshot().phenomenon_actions==0&&game.snapshot().rng==seed
          &&game.snapshot().last_fate_event==std::optional<FateEvent>{{1,288}},
          "v65 supplied outcome uses the owned action proof without a local phenomenon selector");
    v64::receipt(game,slots,1,7,192);
    const Command cobra{2,CommandKind::play,{consumed_slot(slots,2,6)},1};
    check(game.legal(cobra),"v65 received Fog launch has its sole alive enemy");(void)game.step(cobra);compare_fate(game,slots,1);
    const Command defend142{1,CommandKind::defend,{consumed_slot(slots,1,6)},{}};
    check(game.legal(defend142),"v65 received DEF6 can fully prevent Dark3");(void)game.step(defend142);roundtrip(game);
    check(game.snapshot().state.players[0].hp==40&&!game.snapshot().state.players[0].dead,"v65 no positive Dark damage means no death");
    v64::receipt(game,slots,1,6,166);v64::receipt(game,slots,2,6,26);compare_fate(game,slots,2);
}
void neptune_effect_component(std::uint64_t seed) {
    // Synthetic effect-only context: keep all post3 positive inventories fixed
    // and reverse just the received HP5. Gift/summon/scheduler order is excluded.
    SnapshotState state;state.gf=3;
    for(std::size_t seat=0;seat<2;++seat) {
        PlayerState player;player.id=seat+1;player.mp=4;
        if(!seat)player.guardian=Guardian::neptune;
        const auto& rows=seat?component_cpu:component_viewer;
        for(const auto& row:rows)player.items.push_back({1000*player.id+row.slot,row.model,{},row.used});
        state.players.push_back(std::move(player));
    }
    Match game;game.load_scene(state,1,seed);const GuardianAction action{1,276,1};
    check(game.guardian_legal(action),"v65 observed276 is a supported canonical Neptune body");(void)game.guardian_step(action);
    auto expected=state;expected.players[0].hp=45;
    check(game.snapshot().state.players==expected.players&&game.snapshot().state.gf==3&&game.snapshot().owner==1
          &&game.snapshot().rng==seed&&game.snapshot().guardian_actions==1,
          "v65 explicit276 component applies HP5 only,without changing items,MP,CP,GF or RNG");roundtrip(game);
}
}


// v66: self45 and self14 are separate received before/after pairs. This tests
// state projections,not the unresolved intermediate absorb clip/death order.
namespace v66 {
constexpr std::array<std::array<ReceivedSlot,9>,2> absorb_p1{{
    {{{1,191},{2,54},{3,161},{4,154},{5,41},{6,45},{7,12},{8,47},{9,230}}},
    {{{1,191},{2,54},{3,161},{4,154},{5,41},{7,12},{8,47},{9,230},{6,161}}}
}};
constexpr std::array<std::array<ReceivedSlot,9>,2> absorb_p2{{
    {{{1,189},{2,59},{3,149},{4,78},{5,226},{6,117},{7,5},{8,118},{9,5}}},
    {{{1,189},{2,59},{3,149},{4,78},{5,226},{6,117},{8,118},{9,5},{0,0}}}
}};
constexpr std::array<std::array<ReceivedSlot,9>,2> twice_p1{{
    {{{2,13},{3,167},{4,5},{5,177},{6,43},{7,191},{8,151},{9,175},{1,149}}},
    {{{2,13},{3,167},{4,5},{5,177},{6,43},{8,151},{9,175},{1,149},{7,47}}}
}};
constexpr std::array<std::array<ReceivedSlot,9>,2> twice_p2{{
    {{{1,146},{2,41},{3,13},{4,185},{5,4},{6,14},{7,3},{8,113},{9,21}}},
    {{{1,146},{2,41},{3,13},{4,185},{5,4},{7,3},{8,113},{9,21},{6,19}}}
}};

void compare(const Match& game,const SlotMap& slots,bool twice,std::size_t checkpoint,std::uint64_t seed) {
    const auto& snap=game.snapshot();const auto expected_owner=twice?2:checkpoint?2:1;
    check(snap.state.gf==(twice?checkpoint?4:2:checkpoint?2:1)&&snap.owner==expected_owner
          &&snap.rng==seed&&!snap.state.is_over&&!snap.terminated&&!snap.truncated&&snap.state.players.size()==2,
          "v66 received GF/owner/nonterminal state and deterministic local stream match");
    std::size_t total=0;
    for(std::size_t seat=0;seat<2;++seat) {
        const auto& player=snap.state.players[seat];
        const auto& rows=twice?(seat?twice_p2[checkpoint]:twice_p1[checkpoint]):(seat?absorb_p2[checkpoint]:absorb_p1[checkpoint]);
        check(player.id==seat+1&&player.hp==(twice&&checkpoint?(seat?34:45):40)
              &&player.mp==(twice&&!seat?25:10)&&player.cp==20&&!player.dead&&player.team==Team::solo
              &&player.guardian==Guardian::none&&player.curses.empty()&&player.confusion_count==0,
              "v66 both received resource triples and live/team/curse/guardian defaults match");
        std::size_t positive=0,empty=0;
        for(const auto& row:rows) {
            if(!row.model){check(row.slot==0,"v66 canonical received placeholder");++empty;continue;}
            ++positive;const auto mapped=slots.find({player.id,row.slot});check(mapped!=slots.end(),"v66 qualified slot maps to a current copy");
            const auto item=std::find_if(player.items.begin(),player.items.end(),[&](const auto& x){return x.token==mapped->second;});
            check(item!=player.items.end()&&item->model==row.model&&!item->fake_model&&!item->used,
                  "v66 every positive received model,absent fake and unused flag match");
        }
        check(player.items.size()==positive&&snap.gifts[seat]==empty,"v66 physical copies and reserved placeholder debts match");total+=positive;
    }
    check(slots.size()==total,"v66 no stale mappings survive consumption/reuse");
    if(!twice&&checkpoint) {
        check(snap.phase==MatchPhase::response&&snap.actor==1&&snap.queue.size()==1&&!snap.purchase,
              "v66 CPU BUY is only a response launch,no random offer has been chosen");
        const auto& buy=snap.queue.front();
        check(buy.models==std::vector<CardId>{5}&&buy.origin==2&&buy.source==2&&buy.target==1
              &&buy.category==Category::trade&&buy.ability==Ability::buy&&buy.power==0,
              "v66 received BUY operation keeps its actor,target and category");
    } else check(snap.phase==MatchPhase::main&&snap.actor==expected_owner&&snap.queue.empty(),"v66 quiescent checkpoint is main");
    // Exclude raw slot identity/order,server updateCount/native decisions and
    // the native intermediate heal/damage order not distinguished at HP40.
    roundtrip(game);
}
void replay(std::uint64_t seed,bool twice) {
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=twice?2:1;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat) {
        PlayerState player;player.id=seat+1;if(twice&&!seat)player.mp=25;
        const auto& rows=twice?(seat?twice_p2[0]:twice_p1[0]):(seat?absorb_p2[0]:absorb_p1[0]);
        for(const auto& row:rows){const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;player.items.push_back({token,row.model,{},false});}
        state.players.push_back(std::move(player));
    }
    const PlayerId owner=twice?2:1;
    game.load_scene(std::move(state),owner,seed);compare(game,slots,twice,0,seed);
    const Command self{owner,CommandKind::play,{consumed_slot(slots,owner,6)},owner};
    check(game.legal(self),"v66 observed single-item self choice is legal");(void)game.step(self);roundtrip(game);
    check(game.snapshot().phase==MatchPhase::grant&&!game.snapshot().actor&&game.snapshot().queue.empty()
          &&game.snapshot().pending_grant==owner&&game.snapshot().decisions==1
          &&game.snapshot().state.players[twice?1:0].hp==(twice?34:40)
          &&game.snapshot().gifts[twice?1:0]==1,
          "v66 one self command resolves body without defense and reserves exactly one ordinary receipt");
    v64::receipt(game,slots,owner,6,twice?19:161);
    if(twice) {
        const Command dew{1,CommandKind::play,{consumed_slot(slots,1,7)},1};
        check(game.legal(dew),"v66 recorded CPU191 self recovery is legal");(void)game.step(dew);roundtrip(game);
        v64::receipt(game,slots,1,7,47); // Positive inventory delta,not a captured CPU gift event.
    } else {
        const Command buy{2,CommandKind::buy,{consumed_slot(slots,2,7)},1};
        check(game.legal(buy),"v66 recorded CPU BUY launch is legal");(void)game.step(buy);roundtrip(game);
    }
    compare(game,slots,twice,1,seed);
}
}

// v68: six received checkpoints from one CPU training game. Full numeric
// inventories are privileged validation fixtures, never policy observations.
namespace v68 {
constexpr std::array<std::array<ReceivedSlot,9>,6> p1{{
    {{{1,19},{2,122},{3,39},{4,130},{5,162},{6,191},{7,27},{8,199},{9,186}}},
    {{{1,19},{2,122},{4,130},{6,191},{7,27},{8,199},{9,186},{3,113},{5,166}}},
    {{{1,19},{2,122},{4,130},{6,191},{7,27},{8,199},{9,186},{3,113},{5,166}}},
    {{{2,122},{4,130},{6,191},{7,27},{8,199},{9,186},{3,113},{5,166},{1,163}}},
    {{{2,122},{4,130},{6,191},{7,27},{8,199},{9,186},{3,113},{5,166},{1,163}}},
    {{{2,122},{4,130},{6,191},{8,199},{9,186},{3,113},{5,166},{1,163},{7,38}}},
}};
constexpr std::array<std::array<ReceivedSlot,9>,6> p2{{
    {{{1,26},{2,67},{3,197},{4,201},{5,3},{6,172},{7,21},{8,19},{9,21}}},
    {{{1,26},{2,67},{3,197},{4,201},{5,3},{6,172},{7,21},{9,21},{0,0}}},
    {{{1,26},{2,67},{3,197},{4,201},{5,3},{6,172},{7,21},{9,21},{8,115}}},
    {{{1,26},{3,197},{4,201},{5,3},{6,172},{7,21},{9,21},{8,115},{0,0}}},
    {{{1,26},{3,197},{4,201},{5,3},{6,172},{7,21},{9,21},{8,115},{2,26}}},
    {{{1,26},{4,201},{5,3},{6,172},{7,21},{9,21},{8,115},{2,26},{3,123}}},
}};
constexpr std::array checkpoints{
    ReceivedCheckpoint{1,1,1,{{{1,40,10,20,false,p1[0]},{2,40,10,20,false,p2[0]}}},0},
    ReceivedCheckpoint{2,2,2,{{{1,15,10,20,false,p1[1]},{2,40,10,20,false,p2[1]}}},19},
    ReceivedCheckpoint{3,3,1,{{{1,10,10,20,false,p1[2]},{2,40,10,20,false,p2[2]}}},0},
    ReceivedCheckpoint{4,4,2,{{{1,5,10,20,false,p1[3]},{2,40,10,20,false,p2[3]}}},67},
    ReceivedCheckpoint{5,5,1,{{{1,3,10,20,false,p1[4]},{2,40,10,20,false,p2[4]}}},0},
    ReceivedCheckpoint{6,7,1,{{{1,7,10,20,false,p1[5]},{2,40,25,20,false,p2[5]}}},0},
};

void compare(const Match& game,const SlotMap& slots,std::size_t checkpoint,std::uint64_t seed) {
    compare_checkpoint(game,slots,checkpoints[checkpoint]);
    const auto& snap=game.snapshot();
    check(snap.rng==seed&&!snap.terminated&&!snap.truncated,
          "v68 no selector/body RNG,seed search or terminal result is substituted");
}
void replay(std::uint64_t seed) {
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=1;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat) {
        PlayerState player;player.id=seat+1;
        const auto& rows=seat?p2[0]:p1[0];
        for(const auto& row:rows) {
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,{},false});
        }
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);compare(game,slots,0,seed);
    const Command prepare{1,CommandKind::play,
        {consumed_slot(slots,1,3),consumed_slot(slots,1,5)},1};
    check(game.legal(prepare),"v68 observed self39+162 composition is legal");
    (void)game.step(prepare);roundtrip(game);
    check(game.snapshot().state.players[0].hp==15&&game.snapshot().gifts[0]==2,
          "v68 one combined self25 command reserves two physical replacements");
    v64::receipt(game,slots,1,3,113);v64::receipt(game,slots,1,5,166);
    const Command first_cpu{2,CommandKind::play,{consumed_slot(slots,2,8)},1};
    check(game.legal(first_cpu),"v68 received CPU19 launch is legal");
    (void)game.step(first_cpu);roundtrip(game);compare(game,slots,1,seed);
    const Command empty{1,CommandKind::defend,{},{}};
    check(game.legal(empty),"v68 first received empty response is legal");
    (void)game.step(empty);roundtrip(game);
    v64::receipt(game,slots,2,8,115); // CPU positive inventory delta,not a gift event.
    compare(game,slots,2,seed);
    const Command lower{1,CommandKind::play,{consumed_slot(slots,1,1)},1};
    check(game.legal(lower),"v68 observed self19 is legal");
    (void)game.step(lower);roundtrip(game);v64::receipt(game,slots,1,1,163);
    const Command second_cpu{2,CommandKind::play,{consumed_slot(slots,2,2)},1};
    check(game.legal(second_cpu),"v68 received CPU67 launch is legal");
    (void)game.step(second_cpu);roundtrip(game);compare(game,slots,3,seed);
    check(game.legal(empty),"v68 second received empty response is legal");
    (void)game.step(empty);roundtrip(game);v64::receipt(game,slots,2,2,26);
    compare(game,slots,4,seed);
    const Command absorb{1,CommandKind::play,{consumed_slot(slots,1,7)},1};
    check(game.legal(absorb),"v68 observed low-HP self27 choice is legal");
    (void)game.step(absorb);roundtrip(game);
    check(game.snapshot().state.players[0].hp==7&&!game.snapshot().state.players[0].dead
          &&game.snapshot().revivals==0&&game.snapshot().phase==MatchPhase::grant
          &&game.snapshot().gifts[0]==1,
          "v68 HP3 self27 finishes atHP7 alive without revival or an extra defense command");
    v64::receipt(game,slots,1,7,38);
    const Command recovery{2,CommandKind::play,{consumed_slot(slots,2,3)},2};
    check(game.legal(recovery),"v68 observed CPU197 implicit-self MP15 is legal");
    (void)game.step(recovery);roundtrip(game);v64::receipt(game,slots,2,3,123);
    compare(game,slots,5,seed);
}
}

// v69: begin at the received post-redraw GF2,not a generated178 result.
// CPU inventories remain privileged validation fixtures,never policy input.
namespace v69 {
constexpr std::array<std::array<ReceivedSlot,9>,5> p1{{
    {{{1,81,0},{3,47,0},{4,129,0},{5,35,0},{6,230,0},{7,15,0},{8,118,0},{9,16,0},{2,5,0}}},
    {{{1,81,0},{4,129,0},{5,35,0},{6,230,0},{7,15,0},{8,118,0},{9,16,0},{2,5,0},{0,0,0}}},
    {{{1,81,0},{4,129,0},{5,35,0},{6,230,0},{7,15,0},{8,118,0},{9,16,0},{2,5,0},{3,105,0}}},
    {{{1,81,0},{4,129,0},{6,230,0},{7,15,0},{8,118,0},{9,16,0},{2,5,0},{3,105,0},{0,0,0}}},
    {{{1,81,0},{4,129,0},{6,230,0},{7,15,0},{8,118,0},{9,16,0},{2,5,0},{3,105,0},{5,142,0}}},
}};
constexpr std::array<std::array<ReceivedSlot,9>,5> p2{{
    {{{1,3,0},{2,19,27},{3,5,0},{4,202,202},{5,29,23},{6,113,135},{7,196,195},{8,5,0},{9,203,0}}},
    {{{1,3,0},{3,5,0},{4,202,202},{5,29,23},{6,113,135},{7,196,195},{8,5,0},{9,203,0},{2,21,21}}},
    {{{1,3,0},{3,5,0},{4,202,202},{5,29,23},{7,196,195},{8,5,0},{9,203,0},{2,21,21},{6,195,196}}},
    {{{3,5,0},{4,202,202},{5,29,23},{7,196,195},{8,5,0},{9,203,0},{2,21,21},{6,195,196},{1,213,0}}},
    {{{3,5,0},{4,202,202},{5,29,23},{7,196,195},{8,5,0},{9,203,0},{2,21,21},{6,195,196},{1,213,0}}},
}};
constexpr std::array checkpoints{
    ReceivedCheckpoint{2,2,2,{{{1,40,10,20,false,p1[0]},{2,40,10,20,false,p2[0]}}},0},
    ReceivedCheckpoint{3,3,1,{{{1,40,10,20,false,p1[1]},{2,35,10,20,false,p2[1]}}},47},
    ReceivedCheckpoint{4,4,2,{{{1,40,10,20,false,p1[2]},{2,23,10,20,false,p2[2]}}},0},
    ReceivedCheckpoint{5,5,1,{{{1,40,10,20,false,p1[3]},{2,23,30,0,false,p2[3]}}},35},
    ReceivedCheckpoint{6,6,2,{{{1,40,10,20,false,p1[4]},{2,13,30,0,false,p2[4]}}},0},
};

void compare(const Match& game,const SlotMap& slots,std::size_t checkpoint,std::uint64_t seed,
             std::uint64_t redraw_count=0) {
    compare_checkpoint(game,slots,checkpoints[checkpoint],PlayerId{2});
    const auto& snap=game.snapshot();
    check(snap.rng==seed&&!snap.terminated&&!snap.truncated&&snap.redraws==redraw_count&&snap.redrawn_items==8*redraw_count
          &&snap.dream_rolls==0&&snap.dream_masks==0,
          "v69 post-redraw projection uses no generated replacement,mask draw or seed fitting");
}
void continue_replay(Match& game,SlotMap& slots,std::uint64_t seed,std::uint64_t redraw_count=0) {
    const auto initial_decisions=game.snapshot().decisions;
    const Command self{2,CommandKind::play,{consumed_slot(slots,2,2)},2};
    check(game.legal(self),"v69 face27 self choice is legal with the received actual19");
    (void)game.step(self);roundtrip(game);
    check(game.snapshot().state.players[1].hp==35&&game.snapshot().decisions==initial_decisions+1
          &&game.snapshot().phase==MatchPhase::grant&&game.snapshot().gifts[1]==1
          &&game.snapshot().queue.empty(),
          "v69 actual19 deals5 without fake27 absorption or an extra self-defense");
    v64::receipt(game,slots,2,2,21,21);
    const Command attack{1,CommandKind::play,{consumed_slot(slots,1,3)},2};
    check(game.legal(attack),"v69 received CPU47 launch is legal");
    (void)game.step(attack);roundtrip(game);compare(game,slots,1,seed,redraw_count);
    const Command defense{2,CommandKind::defend,{consumed_slot(slots,2,6)},{}};
    check(game.legal(defense),"v69 visibleDEF5 permits the recorded physical response");
    (void)game.step(defense);roundtrip(game);
    check(game.snapshot().state.players[1].hp==23,
          "v69 actual113 DEF1,not face135 DEF5,leaves12 target damage");
    // The CPU receipt is a positive inventory delta. Its ordering relative to
    // the explicit viewer gift is not an observed server schedule.
    v64::receipt(game,slots,1,3,105);v64::receipt(game,slots,2,6,195,196);
    compare(game,slots,2,seed,redraw_count);
    const Command exchange{2,CommandKind::exchange,{consumed_slot(slots,2,1)},{},ResourceAllocation{23,30,0}};
    check(game.legal(exchange),"v69 observed no-fee exchange allocation is legal");
    (void)game.step(exchange);roundtrip(game);v64::receipt(game,slots,2,1,213);
    const Command next{1,CommandKind::play,{consumed_slot(slots,1,5)},2};
    check(game.legal(next),"v69 received CPU35 launch is legal");
    (void)game.step(next);roundtrip(game);compare(game,slots,3,seed,redraw_count);
    const Command empty{2,CommandKind::defend,{},{}};
    check(game.legal(empty),"v69 recorded empty response preserves CP0 and MP30");
    (void)game.step(empty);roundtrip(game);v64::receipt(game,slots,1,5,142);
    compare(game,slots,4,seed,redraw_count);
    // Stop before the multi-candidate BUY selector. The observed positive-price
    // rejection remains record-only; no seed is selected to mimic its commodity.
}
void replay(std::uint64_t seed) {
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=2;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat) {
        PlayerState player;player.id=seat+1;if(seat==1)player.curses={Curse::dream};
        const auto& rows=seat?p2[0]:p1[0];
        for(const auto& row:rows) {
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,row.fake?std::optional<CardId>{row.fake}:std::nullopt,row.used});
        }
        state.players.push_back(std::move(player));
    }
    check(!match_dream_compatible(19,27)&&match_dream_disguise_supported(19,27)
          &&!match_dream_disguise_supported(27,19),
          "v69 accepted received19-to27 does not change the old generation class or reverse direction");
    game.load_scene(std::move(state),2,seed);compare(game,slots,0,seed);
    continue_replay(game,slots,seed);
}
}

// v71 extends the same received sequence with a privileged commodity outcome.
// It supplies the exact seller copy rather than fitting a local selection seed.
namespace v71 {
constexpr std::array<ReceivedSlot,9> after_cpu{{{1,81},{4,129},{6,230},{7,15},{8,118},{9,16},{3,105},{5,142},{0,0}}};
constexpr std::array<ReceivedSlot,9> after_viewer{{{4,202,202},{5,29,23},{7,196,195},{8,5},{9,203},{2,21,21},{6,195,196},{1,213},{3,237}}};
constexpr ReceivedCheckpoint after{7,7,1,{{{1,40,10,20,false,after_cpu},{2,13,30,0,false,after_viewer}}},5};

void continue_replay(Match& game,SlotMap& slots,std::uint64_t seed) {
    const auto commodity=slots.at({1,2});
    const Command buy{2,CommandKind::buy,{consumed_slot(slots,2,3)},1};
    check(game.legal(buy),"v71 received CP0 BUY launch is legal before commodity selection");
    (void)game.step(buy);roundtrip(game);
    const auto pending=game.snapshot();
    const PurchaseResponseAction response{{1,CommandKind::defend,{},{}},commodity};
    check(game.purchase_response_legal(response)&&game.snapshot()==pending,
          "v71 supplied seller copy preflight leaves the pending BUY unchanged");
    (void)game.purchase_response_step(response);roundtrip(game);
    const auto& rejected=game.snapshot();
    check(rejected.rng==seed&&rejected.purchase_proof.has_value()
          &&!rejected.purchase&&rejected.last_purchase
          &&rejected.last_purchase->reason==PurchaseReason::insufficient_cp
          &&rejected.last_purchase->model==5&&rejected.last_purchase->price==5
          &&rejected.state.players[0].hp==40&&rejected.state.players[0].mp==10&&rejected.state.players[0].cp==20
          &&rejected.state.players[1].hp==13&&rejected.state.players[1].mp==30&&rejected.state.players[1].cp==0
          &&rejected.phase==MatchPhase::grant&&rejected.pending_grant==2,
          "v71 price5 selection immediately rejects CP0 without quote,resource debt or selection RNG");
    const auto returned=std::find_if(rejected.state.players[0].items.begin(),rejected.state.players[0].items.end(),
                                    [=](const auto& item){return item.token==commodity;});
    check(returned!=rejected.state.players[0].items.end()&&returned->model==5&&!returned->fake_model&&!returned->used,
          "v71 rejected commodity returns as the same unused unmasked physical copy");
    v64::receipt(game,slots,2,3,237);
    check(game.snapshot().purchase_proof.has_value(),"v71 purchase proof retains the explicit ordinary receipt");
    const Command next{1,CommandKind::buy,{consumed_slot(slots,1,2)},2};
    check(game.legal(next),"v71 CPU can immediately use the returned same BUY copy");
    (void)game.step(next);roundtrip(game);compare_checkpoint(game,slots,after,PlayerId{2});
    check(game.snapshot().rng==seed&&!game.snapshot().purchase_proof
          &&game.snapshot().redraws==1&&game.snapshot().redrawn_items==8
          &&game.snapshot().dream_rolls==0&&game.snapshot().dream_masks==0,
          "v71 next accepted action clears the old proof; final CPU BUY stays unanswered without selector RNG");
}
}

// v70 reuses v69 received states; no new official observations.
namespace v70 {
constexpr std::array<ReceivedSlot,9> before_cpu{{{1,81},{3,47},{4,129},{5,35},{6,230},{7,15},{8,118},{9,16},{0,0}}};
constexpr std::array<ReceivedSlot,9> before_viewer{{{1,199},{2,203},{3,180},{4,123},{5,178},{6,148},{7,5},{8,4},{9,5}}};
constexpr ReceivedCheckpoint before{1,1,1,{{{1,40,10,20,false,before_cpu},{2,40,10,20,false,before_viewer}}},35};

void replay(std::uint64_t seed,bool supplied_purchase=false) {
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=1;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat) {
        PlayerState player;player.id=seat+1;
        for(const auto& row:seat?before_viewer:before_cpu) {
            if(!row.model)continue;
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,{},false});
        }
        // Reconstruct only the consumed CPU35 copy from the received launch.
        // Its original initial-draw/order is not known.
        if(!seat){player.items.push_back({1002,35,{},false});slots[{1,2}]=1002;}
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);
    const Command launch{1,CommandKind::play,{consumed_slot(slots,1,2)},2};
    check(game.legal(launch),"v70 reconstructed received CPU35 launch is legal");
    (void)game.step(launch);roundtrip(game);compare_checkpoint(game,slots,before);
    const auto fresh=game.snapshot().next_token;
    RedrawResult replacement;
    for(std::size_t i=0;i<8;++i) {
        const auto& row=v69::p2[0][i];
        replacement.items.push_back({row.model,row.fake?std::optional<CardId>{row.fake}:std::nullopt});
    }
    const RedrawResponseAction response{{2,CommandKind::defend,{consumed_slot(slots,2,5)},{}},{replacement}};
    const auto pending=game.snapshot();
    check(game.redraw_response_legal(response)&&game.snapshot()==pending,
          "v70 supplied8 redraw preflight is immutable and accepts the received19/27 and202/202");
    (void)game.redraw_response_step(response);roundtrip(game);
    check(game.snapshot().state.players[1].hp==40
          &&game.snapshot().state.players[1].curses==std::vector<Curse>{Curse::dream}
          &&game.snapshot().redraws==1&&game.snapshot().redrawn_items==8
          &&game.snapshot().state.players[1].items.size()==8&&game.snapshot().next_token==fresh+8
          &&game.snapshot().gifts==std::vector<std::uint32_t>{1,1}
          &&game.snapshot().phase==MatchPhase::grant&&game.snapshot().pending_grant==1
          &&game.snapshot().rng==seed&&game.snapshot().dream_rolls==0&&game.snapshot().dream_masks==0
          &&game.snapshot().redraw_proof.has_value(),
          "v70 supplied178 replaces8 physical copies before body and preserves both ordinary debts without selector RNG");
    for(auto it=slots.begin();it!=slots.end();) {
        if(it->first.first==2)it=slots.erase(it);else++it;
    }
    for(std::size_t i=0;i<8;++i)slots[{2,v69::p2[0][i].slot}]=fresh+i;
    // CPU5 is a received positive inventory delta. The explicit viewer gift203
    // is separate from replaceItems8; CPU-vs-viewer receipt order is unobserved.
    v64::receipt(game,slots,1,2,5);v64::receipt(game,slots,2,9,203);
    v69::compare(game,slots,0,seed,1);v69::continue_replay(game,slots,seed,1);
    if(supplied_purchase)v71::continue_replay(game,slots,seed);
}
}

// v72 starts from a reconstructed pre41 scene,not a received initial game.
// It compares the four saved updates2..5,including the final empty defense.
namespace v72 {
constexpr std::array<std::array<ReceivedSlot,9>,3> viewer{{
    {{{1,45},{2,191},{3,6},{4,16},{5,116},{6,5},{7,179},{9,206},{8,49}}},
    {{{1,45},{2,191},{3,6},{4,16},{5,116},{6,5},{9,206},{8,49},{7,44}}},
    {{{1,45},{2,191},{3,6},{4,16},{5,116},{6,5},{9,206},{7,44},{8,199}}},
}};
constexpr std::array<std::array<ReceivedSlot,9>,3> cpu{{
    {{{1,90},{2,39},{3,183},{4,21},{5,19},{6,156},{7,43},{8,25},{0,0}}},
    {{{1,90},{2,39},{3,183},{4,21},{5,19},{6,156},{7,43},{8,25},{9,35}}},
    {{{1,90},{2,39},{3,183},{4,21},{6,156},{9,35},{0,0},{0,0},{0,0}}},
}};
constexpr std::array<CardId,3> compound{19,43,25};
constexpr std::array checkpoints{
    ReceivedCheckpoint{2,2,2,{{{1,29,10,20,false,viewer[0],true},{2,40,10,20,false,cpu[0]}}},92},
    ReceivedCheckpoint{3,3,1,{{{1,29,10,20,false,viewer[1],true},{2,40,10,20,false,cpu[1]}}},0},
    ReceivedCheckpoint{4,4,2,{{{1,16,10,20,false,viewer[2],true},{2,40,10,20,false,cpu[2]}}},19,compound},
    ReceivedCheckpoint{5,4,2,{{{1,0,10,20,false,viewer[2],true,true},{2,40,10,20,false,cpu[2]}}},0,{},true},
};
void compare(const Match& game,const SlotMap& slots,std::size_t checkpoint,std::uint64_t seed){
    compare_checkpoint(game,slots,checkpoints[checkpoint]);
    const auto& s=game.snapshot();
    check(s.rng==seed&&s.redraws==0&&s.dream_rolls==0&&s.dream_masks==0&&s.guardian_actions==0
          &&s.auto_guardian_actions==0&&s.revivals==0,"v72 deterministic received projection has no fitted selection,chance,guardian or revival draws");
    const auto saved45=std::find_if(s.state.players[0].items.begin(),s.state.players[0].items.end(),
                                  [](const auto& item){return item.model==45;});
    check(saved45!=s.state.players[0].items.end()&&!saved45->used,"v72 retained45 is never used;low-HP absorption remains unobserved");
}
void replay(std::uint64_t seed){
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=1;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat){
        PlayerState player;player.id=seat+1;
        for(const auto& row:seat?cpu[0]:viewer[0]){
            if(!row.model)continue;
            const auto model=!seat&&row.slot==8?CardId{41}:row.model;
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,model,{},false});
        }
        if(seat){player.items.push_back({2009,92,{},false});slots[{2,9}]=2009;}
        // Initial physical order and draw history were not captured.
        std::sort(player.items.begin(),player.items.end(),[](const auto& a,const auto& b){return a.token<b.token;});
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);
    const Command self41{1,CommandKind::play,{consumed_slot(slots,1,8)},1};
    check(game.legal(self41),"v72 received41 explicit self command is legal");
    (void)game.step(self41);roundtrip(game);
    check(game.snapshot().phase==MatchPhase::grant&&game.snapshot().pending_grant==1
          &&game.snapshot().state.players[0].hp==29&&game.snapshot().state.players[0].curses==std::vector<Curse>{Curse::darkcloud}
          &&game.snapshot().decisions==1&&game.snapshot().queue.empty(),
          "v72 actual41 resolves11 damage and DarkCloud immediately without an extra self-defense");
    v64::receipt(game,slots,1,8,49);
    (void)game.step({2,CommandKind::play,{consumed_slot(slots,2,9)},1});roundtrip(game);compare(game,slots,0,seed);
    (void)game.step({1,CommandKind::defend,{consumed_slot(slots,1,7)},{}});roundtrip(game);
    check(game.snapshot().state.players[0].hp==29,"v72 actual179 DEF15 safely cancels Dark10");
    // CPU35 is a positive inventory delta,not a received viewer gift event.
    // The local seat-order pump is not evidence for backend receipt order.
    v64::receipt(game,slots,1,7,44);v64::receipt(game,slots,2,9,35);compare(game,slots,1,seed);
    (void)game.step({1,CommandKind::play,{consumed_slot(slots,1,8)},1});roundtrip(game);
    check(game.snapshot().state.players[0].hp==16&&game.snapshot().queue.empty(),"v72 self49 immediately applies13 without a defensive use of its DEF1");
    v64::receipt(game,slots,1,8,199);
    std::vector<ItemToken> additions;
    for(const auto slot:{5,7,8})additions.push_back(consumed_slot(slots,2,slot));
    (void)game.step({2,CommandKind::play,additions,1});roundtrip(game);compare(game,slots,2,seed);
    (void)game.step({1,CommandKind::defend,{},{}});roundtrip(game);compare(game,slots,3,seed);
    check(game.snapshot().winners==std::vector<PlayerId>{2}&&game.snapshot().rewards==std::vector<double>{-1,1},
          "v72 empty defense against actual22 ends the reconstructed sequence with CPU2 winning");
}
}

// v73 has a received initial state and a received end. Initial distribution and
// CPU decisions are supplied evidence;no local sampling is fitted to the game.
namespace v73 {
constexpr std::array<std::array<ReceivedSlot,9>,3> viewer{{
    {{{1,3},{2,19},{3,55},{4,176},{5,149},{6,138},{7,32},{8,3},{9,165}}},
    {{{1,3},{2,19},{4,176},{5,149},{6,138},{7,32},{8,3},{9,165},{3,184}}},
    {{{1,3},{2,19},{4,176},{5,149},{6,138},{7,32},{8,3},{9,165},{0,0}}},
}};
constexpr std::array<std::array<ReceivedSlot,9>,3> cpu{{
    {{{1,35},{2,190},{3,107},{4,207},{5,40},{6,203},{7,21},{8,43},{9,206}}},
    {{{1,35},{2,190},{3,107},{4,207},{6,203},{7,21},{8,43},{9,206},{0,0}}},
    {{{1,35},{3,107},{4,207},{6,203},{7,21},{8,43},{9,206},{0,0},{0,0}}},
}};
constexpr std::array checkpoints{
    ReceivedCheckpoint{1,1,1,{{{1,40,10,20,false,viewer[0]},{2,40,10,20,false,cpu[0]}}},0},
    ReceivedCheckpoint{2,2,2,{{{1,25,10,20,false,viewer[1]},{2,40,10,20,false,cpu[1]}}},40},
    ReceivedCheckpoint{3,2,2,{{{1,14,10,20,false,viewer[2]},{2,40,10,20,false,cpu[2]}}},184,{},false,PlayerId{1},22,true},
    ReceivedCheckpoint{4,2,2,{{{1,0,10,20,false,viewer[2],false,true},{2,40,10,20,false,cpu[2]}}},0,{},true},
};
void compare(const Match& game,const SlotMap& slots,std::size_t checkpoint,std::uint64_t seed){
    compare_checkpoint(game,slots,checkpoints[checkpoint]);
    const auto& s=game.snapshot();
    check(s.rng==seed&&s.redraws==0&&s.dream_rolls==0&&s.dream_masks==0&&s.auto_guardian_actions==0&&s.revivals==0
          &&s.chance_rolls==0&&s.counter_rolls==0,"v73 uses the same fixed seeds without chance,RNG or recipient fitting");
    const auto& other=s.state.players[1].items;
    check(std::any_of(other.begin(),other.end(),[](const auto& item){return item.model==207&&!item.used;}),
          "v73 received owned207 remains unused and does not imply a Fate choice or outcome");
}
void replay(std::uint64_t seed){
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.include_fate=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=1;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat){
        PlayerState player;player.id=seat+1;
        for(const auto& row:seat?cpu[0]:viewer[0]){
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,{},false});
        }
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);compare(game,slots,0,seed);
    const Command self55{1,CommandKind::play,{consumed_slot(slots,1,3)},1};
    check(game.legal(self55),"v73 actual55 explicit self is a legal single-card main command");
    (void)game.step(self55);roundtrip(game);
    check(game.snapshot().phase==MatchPhase::grant&&game.snapshot().pending_grant==1&&game.snapshot().decisions==1
          &&game.snapshot().state.players[0].hp==25&&game.snapshot().queue.empty(),
          "v73 actual55 deals15 directly without a self-defense or a block reaction");
    v64::receipt(game,slots,1,3,184);
    (void)game.step({2,CommandKind::play,{consumed_slot(slots,2,5)},1});roundtrip(game);compare(game,slots,1,seed);
    (void)game.step({1,CommandKind::defend,{consumed_slot(slots,1,3)},{}});roundtrip(game);
    const auto& generated=game.snapshot();
    check(generated.state.players[0].hp==14&&generated.phase==MatchPhase::response&&generated.actor==2
          &&generated.queue.size()==1&&generated.queue.front().is_counter
          &&generated.queue.front().origin==1&&generated.queue.front().source==1&&generated.queue.front().target==2
          &&generated.queue.front().models==std::vector<CardId>{184}
          &&generated.queue.front().power==2*(25-generated.state.players[0].hp),
          "v73 normal184 resolver derives22 from the11 HP loss;received atk22 is never injected");
    (void)game.step({2,CommandKind::defend,{consumed_slot(slots,2,2)},{}});roundtrip(game);compare(game,slots,2,seed);
    const auto& reflected=game.snapshot();
    check(reflected.queue.front().initial_target==2&&reflected.queue.front().redirects.size()==1
          &&reflected.queue.front().redirects.front().models==std::vector<CardId>{190}
          &&reflected.queue.front().redirects.front().defender==2&&reflected.queue.front().redirects.front().target==1
          &&reflected.counter_created==1&&reflected.counter_started==1,
          "v73 regular190 changes the counter source/target while retaining original generator1");
    check(!game.legal({1,CommandKind::defend,{slots.at({1,9})},{}}),
          "v73 unused165 is neither a miracle reaction nor ordinary Wood/Light defense against the Stone counter");
    (void)game.step({1,CommandKind::defend,{},{}});roundtrip(game);compare(game,slots,3,seed);
    check(game.snapshot().winners==std::vector<PlayerId>{2}&&game.snapshot().rewards==std::vector<double>{-1,1}
          &&game.snapshot().decisions==6,
          "v73 complete received lifecycle ends on the reflected counter before any reserved replacement");
}
}

// v74 has received updates1 and3. The intervening self21 operation is observed
// in the UI;its raw use event and update2 were not captured or reconstructed.
namespace v74 {
constexpr std::array<ReceivedSlot,9> initial_viewer{{{1,142},{2,93},{3,21},{4,4},{5,124},{6,23},{7,210},{8,154},{9,4}}};
constexpr std::array<ReceivedSlot,9> initial_cpu{{{1,97},{2,75},{3,132},{4,118},{5,187},{6,118},{7,3},{8,118},{9,5}}};
constexpr std::array<ReceivedSlot,8> final_viewer{{{1,142},{2,93},{4,4},{5,124},{7,210},{8,154},{9,4},{3,44}}};
constexpr std::array<ReceivedSlot,10> final_cpu{{{1,97},{2,75},{3,132},{4,118},{5,187},{6,118},{7,3},{8,118},{9,23},{10,13}}};
constexpr std::array checkpoints{
    ReceivedCheckpoint{1,1,1,{{{1,40,10,20,false,initial_viewer},{2,40,10,20,false,initial_cpu}}},0},
    ReceivedCheckpoint{3,3,1,{{{1,35,10,22,false,final_viewer},{2,40,10,18,false,final_cpu}}},0},
};
void compare(const Match& game,const SlotMap& slots,std::size_t checkpoint,std::uint64_t seed){
    compare_checkpoint(game,slots,checkpoints[checkpoint]);
    const auto& s=game.snapshot();
    check(s.rng==seed&&s.chance_rolls==0&&s.counter_rolls==0&&s.dream_rolls==0&&s.dream_masks==0
          &&s.redraws==0&&s.auto_guardian_actions==0&&s.revivals==0,
          "v74 supplied commodity and ordinary deltas use the fixed seeds without fitting or unobserved chance branches");
}
void replay(std::uint64_t seed){
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=1;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat){
        PlayerState player;player.id=seat+1;
        for(const auto& row:seat?initial_cpu:initial_viewer){
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,{},false});
        }
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);compare(game,slots,0,seed);
    const Command self21{1,CommandKind::play,{consumed_slot(slots,1,3)},1};
    check(game.legal(self21),"v74 UI-observed single21 self command is legal");
    (void)game.step(self21);roundtrip(game);
    check(game.snapshot().decisions==1&&game.snapshot().state.players[0].hp==35
          &&game.snapshot().phase==MatchPhase::grant&&game.snapshot().pending_grant==1&&game.snapshot().queue.empty()
          &&game.snapshot().rng==seed,
          "v74 UI-observed self21 applies5 without an extra self-defense or a local bounce draw");
    // Neither the44 gift event nor the CPU5 launch was received as raw update2.
    // The UI shows the launch;44 and13 are separate inventory-delta inputs.
    v64::receipt(game,slots,1,3,44);
    (void)game.step({2,CommandKind::buy,{consumed_slot(slots,2,9)},1});roundtrip(game);
    const auto commodity=slots.at({1,6});
    const PurchaseResponseAction selected{{1,CommandKind::defend,{},{}},commodity};
    check(game.purchase_response_legal(selected),"v74 received seller23 copy is an eligible supplied commodity");
    (void)game.purchase_response_step(selected);roundtrip(game);
    const auto& quoted=game.snapshot();
    check(quoted.phase==MatchPhase::purchase&&quoted.actor==2&&quoted.purchase
          &&quoted.purchase->item.model==23&&quoted.purchase->item.token==commodity&&find_card(23)->price==2,
          "v74 supplied23 result uses the existing affordable quote;no raw quote checkpoint is claimed");
    (void)consumed_slot(slots,1,6);
    const auto delivered=game.snapshot().next_token;
    (void)game.step({2,CommandKind::accept,{},{}});roundtrip(game);slots[{2,9}]=delivered;
    check(game.snapshot().state.players[1].items.size()==9&&game.snapshot().gifts[1]==1
          &&game.snapshot().last_purchase&&game.snapshot().last_purchase->reason==PurchaseReason::bought
          &&game.snapshot().last_purchase->model==23&&game.snapshot().last_purchase->price==2
          &&game.snapshot().state.players[0].cp==22&&game.snapshot().state.players[1].cp==18,
          "v74 ordinary accept transfers2 CP and a fresh product without a seller replacement or extra operation fee");
    v64::receipt(game,slots,2,10,13);compare(game,slots,1,seed);
    check(game.snapshot().state.players[0].items.size()==8&&game.snapshot().state.players[1].items.size()==10
          &&game.snapshot().purchase_proof&&game.snapshot().grant_proof
          &&*game.snapshot().purchase_proof==*game.snapshot().grant_proof&&game.snapshot().decisions==6,
          "v74 purchase plus normal13 receipt preserves the whole private proof and the received ten-copy buyer hand");
}
}

namespace v75 {
// Received initial1 and post-self2, not a reconstructed initial draw.
constexpr std::array<ReceivedSlot,9> initial_viewer{{{1,191},{2,58},{3,200},{4,145},{5,5},{6,130},{7,130},{8,195},{9,166}}};
constexpr std::array<ReceivedSlot,9> initial_cpu{{{1,178},{2,4},{3,172},{4,113},{5,5},{6,221},{7,193},{8,154},{9,105}}};
constexpr std::array<ReceivedSlot,9> after_viewer{{{1,191},{3,200},{4,145},{5,5},{6,130},{7,130},{8,195},{9,166},{2,23}}};
constexpr std::array<ReceivedSlot,10> after_cpu{{{1,178},{2,4},{3,172},{4,113},{5,5},{7,193},{8,154},{9,105},{6,221,0,true},{0,0}}};
constexpr std::array checkpoints{
    ReceivedCheckpoint{1,1,1,{{{1,40,10,20,false,initial_viewer},{2,40,10,20,false,initial_cpu}}}},
    ReceivedCheckpoint{2,2,2,{{{1,20,0,20,false,after_viewer},{2,40,7,20,false,after_cpu}}},221}
};
void replay(std::uint64_t seed) {
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=1;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat){
        PlayerState player;player.id=seat+1;
        for(const auto& row:seat?initial_cpu:initial_viewer){
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,{},false});
        }
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);compare_checkpoint(game,slots,checkpoints[0]);
    const Command self{1,CommandKind::play,{consumed_slot(slots,1,2)},1};
    check(game.legal(self),"v75 received58 is legal for explicit self use");
    (void)game.step(self);roundtrip(game);
    check(game.snapshot().decisions==1&&game.snapshot().state.players[0].hp==20
          &&game.snapshot().state.players[0].mp==0&&game.snapshot().phase==MatchPhase::grant
          &&game.snapshot().pending_grant==1&&game.snapshot().gifts[0]==1&&game.snapshot().queue.empty(),
          "v75 self58 fixes20 power before consuming all10 MP, with one command and no self-defense");
    v64::receipt(game,slots,1,2,23);
    // Miracle221 retains its physical copy as used; its gift is still reserved.
    const Command fog{2,CommandKind::play,{slots.at({2,6})},1};
    check(game.legal(fog),"v75 received CPU221 launch is legal");
    (void)game.step(fog);roundtrip(game);compare_checkpoint(game,slots,checkpoints[1]);
    const auto& s=game.snapshot();
    check(s.decisions==3&&s.rng==seed&&s.chance_rolls==0&&s.counter_rolls==0
          &&s.dream_rolls==0&&s.dream_masks==0&&s.redraws==0&&s.auto_guardian_actions==0
          &&s.revivals==0&&s.queue.front().ability==Ability::add_curse
          &&find_card(221)->curse==Curse::fog,
          "v75 fixed seeds use no selector or hit draw; Fog remains unanswered and CPU105 is unused");
}
}

namespace v76 {
constexpr std::array<ReceivedSlot,9> initial_viewer{{{1,5},{2,170},{3,135},{4,29},{5,198},{6,78},{7,48},{8,5},{9,163}}};
constexpr std::array<ReceivedSlot,9> initial_cpu{{{1,79},{2,13},{3,199},{4,60},{5,4},{6,187},{7,3},{8,82},{9,168}}};
constexpr std::array<ReceivedSlot,9> after_viewer{{{1,5},{2,170},{3,135},{4,29},{5,198},{6,78},{8,5},{9,163},{7,58}}};
constexpr std::array<ReceivedSlot,8> after_cpu{{{1,79},{2,13},{3,199},{4,60},{6,187},{7,3},{8,82},{0,0}}};
constexpr std::array<CardId,2> sale_models{4,168};
constexpr std::array checkpoints{
    ReceivedCheckpoint{1,1,1,{{{1,40,10,20,false,initial_viewer},{2,40,10,20,false,initial_cpu}}}},
    ReceivedCheckpoint{2,2,2,{{{1,26,10,20,true,after_viewer},{2,40,10,20,false,after_cpu}}},4,sale_models}
};
void replay(std::uint64_t seed){
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=1;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat){
        PlayerState player;player.id=seat+1;
        for(const auto& row:seat?initial_cpu:initial_viewer){
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,{},false});
        }
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);compare_checkpoint(game,slots,checkpoints[0]);
    const Command self{1,CommandKind::play,{consumed_slot(slots,1,7)},1};
    check(game.legal(self),"v76 observed48 single self command is legal");
    (void)game.step(self);roundtrip(game);
    const auto& body=game.snapshot();
    check(body.decisions==1&&body.phase==MatchPhase::grant&&body.pending_grant==1&&body.gifts[0]==1
          &&body.queue.empty()&&body.state.players[0].hp==26&&body.state.players[0].mp==10
          &&body.state.players[0].curses==std::vector<Curse>{Curse::cold}
          &&body.last_tick&&body.last_tick->owner==1&&body.last_tick->before==Curse::cold
          &&body.last_tick->after==Curse::cold&&!body.last_tick->worsened&&!body.last_tick->fallen
          &&body.last_tick->hp_before==27&&body.last_tick->hp_after==26,
          "v76 self48 applies13,Cold and one non-worsening1-damage owner tick before the ordinary gift");
    const auto tick_rng=body.rng;check(tick_rng!=seed,"v76 disease tick uses the existing environment RNG rather than a supplied outcome");
    v64::receipt(game,slots,1,7,58);
    const Command sale{2,CommandKind::sell,{consumed_slot(slots,2,5),consumed_slot(slots,2,9)},1};
    check(game.legal(sale),"v76 observed CPU4 plus168 SELL launch is legal");
    (void)game.step(sale);roundtrip(game);compare_checkpoint(game,slots,checkpoints[1]);
    const auto& s=game.snapshot();
    check(s.decisions==3&&s.rng==tick_rng&&s.chance_rolls==0&&s.counter_rolls==0&&s.dream_rolls==0
          &&s.dream_masks==0&&s.redraws==0&&s.auto_guardian_actions==0&&s.revivals==0
          &&s.state.players[0].cp==20&&s.state.players[1].cp==20&&!s.last_sale,
          "v76 fixed seeds keep the observed non-worsening branch;pending SELL has no delivery,CP transfer or Fever yet");
}
}

namespace v78 {
constexpr std::array<ReceivedSlot,9> initial_viewer{{{1,5},{2,154},{3,53},{4,161},{5,199},{6,149},{7,201},{8,74},{9,192}}};
constexpr std::array<ReceivedSlot,9> initial_cpu{{{1,123},{2,128},{3,4},{4,181},{5,146},{6,5},{7,19},{8,61},{9,195}}};
constexpr std::array<ReceivedSlot,9> after_viewer{{{1,5},{2,154},{4,161},{5,199},{6,149},{7,201},{8,74},{9,192},{3,160}}};
constexpr std::array<ReceivedSlot,9> after_cpu{{{1,123},{2,128},{3,4},{4,181},{5,146},{6,5},{8,61},{9,195},{0,0}}};
constexpr std::array checkpoints{
    ReceivedCheckpoint{1,1,1,{{{1,40,10,20,false,initial_viewer},{2,40,10,20,false,initial_cpu}}}},
    ReceivedCheckpoint{2,2,2,{{{1,12,10,20,false,after_viewer},{2,40,10,20,false,after_cpu}}},19}
};
void replay(std::uint64_t seed){
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=1;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat){
        PlayerState player;player.id=seat+1;
        for(const auto& row:seat?initial_cpu:initial_viewer){
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,{},false});
        }
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);compare_checkpoint(game,slots,checkpoints[0]);
    constexpr std::array<CardId,1> models{53};
    check(find_card(53)->ability==Ability::deal_same_damage&&attack_power(models)==14,
          "v78 derives the same-damage body from actual53 rather than injecting a received damage total");
    const Command self{1,CommandKind::play,{consumed_slot(slots,1,3)},1};
    check(game.legal(self),"v78 received53 single self play is legal");
    (void)game.step(self);roundtrip(game);
    const auto& body=game.snapshot();
    check(body.decisions==1&&body.phase==MatchPhase::grant&&body.pending_grant==1&&body.gifts[0]==1
          &&body.queue.empty()&&body.state.players[0].hp==40-2*attack_power(models)
          &&body.state.players[0].mp==10&&body.state.players[0].cp==20&&body.state.players[0].items.size()==8
          &&body.state.players[0].curses.empty()&&!body.last_tick&&body.rng==seed,
          "v78 one self53 command applies both nominal deductions and one consumption without a defense or disease tick");
    v64::receipt(game,slots,1,3,160);
    const Command cpu{2,CommandKind::play,{consumed_slot(slots,2,7)},1};
    check(game.legal(cpu),"v78 received CPU19 launch is legal");
    (void)game.step(cpu);roundtrip(game);compare_checkpoint(game,slots,checkpoints[1]);
    const auto& s=game.snapshot();
    check(s.decisions==3&&s.rng==seed&&s.chance_rolls==0&&s.counter_rolls==0&&s.dream_rolls==0
          &&s.dream_masks==0&&s.redraws==0&&s.auto_guardian_actions==0&&s.revivals==0,
          "v78 all fixed seeds match initial and post without fitting;CPU19 remains unanswered");
    // Source and target are the same received player. Two14 events support the
    // total,not an independently identified source-versus-target effect order.
}
}

namespace v79 {
constexpr std::array<ReceivedSlot,9> initial_viewer{{{1,161},{2,113},{3,118},{4,140},{5,72},{6,26},{7,151},{8,140},{9,32}}};
constexpr std::array<ReceivedSlot,9> initial_cpu{{{1,236},{2,193},{3,139},{4,40},{5,23},{6,130},{7,40},{8,199},{9,30}}};
constexpr std::array<ReceivedSlot,9> after_viewer{{{1,161},{2,113},{3,118},{4,140},{6,26},{7,151},{8,140},{9,32},{5,10}}};
constexpr std::array<ReceivedSlot,9> after_cpu{{{1,236},{2,193},{3,139},{5,23},{6,130},{7,40},{8,199},{9,30},{0,0}}};
constexpr std::array checkpoints{
    ReceivedCheckpoint{1,1,1,{{{1,40,10,20,false,initial_viewer},{2,40,10,20,false,initial_cpu}}}},
    ReceivedCheckpoint{2,2,2,{{{1,37,10,20,false,after_viewer,false,false,true},{2,40,10,20,false,after_cpu}}},40}
};
void replay(std::uint64_t seed){
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=1;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat){
        PlayerState player;player.id=seat+1;
        for(const auto& row:seat?initial_cpu:initial_viewer){
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,{},false});
        }
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);compare_checkpoint(game,slots,checkpoints[0]);
    const Command self{1,CommandKind::play,{consumed_slot(slots,1,5)},1};
    check(game.legal(self),"v79 received72 single self play is legal");
    (void)game.step(self);roundtrip(game);
    const auto& body=game.snapshot();
    check(body.decisions==1&&body.phase==MatchPhase::grant&&body.pending_grant==1&&body.gifts[0]==1
          &&body.queue.empty()&&body.state.players[0].hp==37&&body.state.players[0].mp==10
          &&body.state.players[0].cp==20&&body.state.players[0].curses==std::vector<Curse>{Curse::fog}
          &&!body.last_tick&&!body.last_fog&&body.rng==seed,
          "v79 raw self72 deals3 and applies Fog before its ordinary gift without self-defense or a target draw");
    v64::receipt(game,slots,1,5,10);
    const Command cpu{2,CommandKind::play,{consumed_slot(slots,2,4)},1};
    check(game.legal(cpu),"v79 received CPU40 copy id4 is legal");
    (void)game.step(cpu);roundtrip(game);compare_checkpoint(game,slots,checkpoints[1]);
    const auto& s=game.snapshot();
    const auto own=project_view(s.state,PlayerId{1}),spectator=project_view(s.state,{});
    check(own.players[0].status&&own.players[0].status->hp==37
          &&own.players[1].fogged&&!own.players[1].status&&spectator.players[1].status
          &&spectator.players[1].status->hp==40&&spectator.players[1].status->mp==10
          &&spectator.players[1].status->cp==20,
          "v79 UI-hidden CPU status is separate from the privileged received resources;Fog does not erase the world");
    check(s.decisions==3&&s.rng==seed&&s.chance_rolls==0&&s.counter_rolls==0&&s.dream_rolls==0
          &&s.dream_masks==0&&s.redraws==0&&s.auto_guardian_actions==0&&s.revivals==0,
          "v79 all fixed seeds compare both received states with supplied10 and no fitted outcomes;CPU40 is unanswered");
    // v77 is a different UI-only game;this raw fixture does not retroactively
    // identify that game's replacement,CPU inventory or unseen event sequence.
}
}

namespace v80 {
constexpr std::array<ReceivedSlot,9> initial_viewer{{{1,35},{2,130},{3,5},{4,149},{5,16},{6,4},{7,113},{8,70},{9,53}}};
constexpr std::array<ReceivedSlot,9> initial_cpu{{{1,131},{2,19},{3,10},{4,11},{5,123},{6,45},{7,31},{8,224},{9,135}}};
constexpr std::array<ReceivedSlot,9> after_viewer{{{1,35},{2,130},{3,5},{4,149},{5,16},{6,4},{7,113},{9,53},{8,4}}};
constexpr std::array<ReceivedSlot,9> after_cpu{{{1,131},{2,19},{4,11},{5,123},{6,45},{7,31},{8,224},{9,135},{0,0}}};
constexpr std::array checkpoints{
    ReceivedCheckpoint{1,1,1,{{{1,40,10,20,false,initial_viewer},{2,40,10,20,false,initial_cpu}}}},
    ReceivedCheckpoint{2,2,2,{{{1,38,10,20,false,after_viewer,false,false,false,true},{2,40,10,20,false,after_cpu}}},10}
};
void replay(std::uint64_t seed){
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=1;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat){
        PlayerState player;player.id=seat+1;
        for(const auto& row:seat?initial_cpu:initial_viewer){
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,{},false});
        }
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);compare_checkpoint(game,slots,checkpoints[0]);
    const Command self{1,CommandKind::play,{consumed_slot(slots,1,8)},1};
    check(game.legal(self),"v80 received70 single self play is legal");
    (void)game.step(self);roundtrip(game);
    const auto& body=game.snapshot();
    check(body.decisions==1&&body.phase==MatchPhase::grant&&body.pending_grant==1&&body.gifts[0]==1
          &&body.queue.empty()&&body.state.players[0].hp==38&&body.state.players[0].mp==10
          &&body.state.players[0].cp==20&&body.state.players[0].curses==std::vector<Curse>{Curse::flash}
          &&!body.last_tick&&body.rng==seed,
          "v80 self70 deals2 and applies Flash before one ordinary gift without self-defense or a disease tick");
    v64::receipt(game,slots,1,8,4);
    const Command cpu{2,CommandKind::play,{consumed_slot(slots,2,3)},1};
    check(game.legal(cpu),"v80 received CPU10 launch is legal");
    (void)game.step(cpu);roundtrip(game);compare_checkpoint(game,slots,checkpoints[1]);
    const auto before=game.snapshot();
    check(game.legal({1,CommandKind::defend,{slots.at({1,2})},{}})
          &&!game.legal({1,CommandKind::defend,{slots.at({1,2}),slots.at({1,4})},{}})
          &&game.snapshot()==before,
          "v80 local preflight connects self-applied Flash to the existing one-card limit without an observed UI defense probe");
    check(before.decisions==3&&before.rng==seed&&before.chance_rolls==0&&before.counter_rolls==0
          &&before.dream_rolls==0&&before.dream_masks==0&&before.redraws==0
          &&before.auto_guardian_actions==0&&before.revivals==0,
          "v80 all fixed seeds compare both states without fitting;held224 is unused and CPU10 remains unanswered");
}
}

namespace v81 {
// First authoritative state already contains CPU29 pending. Only its consumed
// copy is restored from the received use event to construct the native launch.
constexpr std::array<ReceivedSlot,9> prelaunch_cpu{{{1,3},{2,135},{3,170},{4,29},{5,123},{6,161},{7,57},{8,32},{9,54}}};
constexpr std::array<ReceivedSlot,9> initial_cpu{{{1,3},{2,135},{3,170},{5,123},{6,161},{7,57},{8,32},{9,54},{0,0}}};
constexpr std::array<ReceivedSlot,9> initial_viewer{{{1,191},{2,73},{3,4},{4,182},{5,66},{6,145},{7,118},{8,198},{9,123}}};
constexpr std::array<ReceivedSlot,9> after_defense_cpu{{{1,3},{2,135},{3,170},{5,123},{6,161},{7,57},{8,32},{9,54},{4,196}}};
constexpr std::array<ReceivedSlot,9> final_cpu{{{1,3},{2,135},{3,170},{5,123},{6,161},{7,57},{8,32},{9,54},{4,3}}};
constexpr std::array<ReceivedSlot,9> final_viewer{{{1,191},{2,73},{3,4},{4,182},{6,145},{7,118},{8,198},{9,123},{5,21}}};
constexpr std::array checkpoints{
    ReceivedCheckpoint{1,1,1,{{{1,40,10,20,false,initial_cpu},{2,40,10,20,false,initial_viewer}}},29},
    ReceivedCheckpoint{2,2,2,{{{1,40,10,20,false,after_defense_cpu},{2,32,10,20,false,initial_viewer}}}},
    ReceivedCheckpoint{3,4,2,{{{1,40,20,20,false,final_cpu},{2,30,10,20,false,final_viewer}}}}
};
void replay(std::uint64_t seed){
    MatchConfig config;config.initial_hand=0;config.external_artifacts=true;config.max_decisions=64;
    Match game(config);SnapshotState state;state.gf=1;SlotMap slots;
    for(std::size_t seat=0;seat<2;++seat){
        PlayerState player;player.id=seat+1;
        for(const auto& row:seat?initial_viewer:prelaunch_cpu){
            const auto token=1000*player.id+row.slot;slots[{player.id,row.slot}]=token;
            player.items.push_back({token,row.model,{},false});
        }
        state.players.push_back(std::move(player));
    }
    game.load_scene(std::move(state),1,seed);
    (void)game.step({1,CommandKind::play,{consumed_slot(slots,1,4)},2});roundtrip(game);
    compare_checkpoint(game,slots,checkpoints[0]);
    (void)game.step({2,CommandKind::defend,{},{}});roundtrip(game);
    check(game.snapshot().state.players[1].hp==32&&game.snapshot().pending_grant==1,
          "v81 received empty response takes8 before the CPU replacement pause");
    // CPU196 and final CPU3 are inventory deltas,not captured gift events.
    v64::receipt(game,slots,1,4,196);compare_checkpoint(game,slots,checkpoints[1]);
    const Command self{2,CommandKind::play,{consumed_slot(slots,2,5)},2};
    check(game.legal(self),"v81 actual66 is legal as the lone self base");
    (void)game.step(self);roundtrip(game);
    const auto& body=game.snapshot();
    check(body.decisions==4&&body.phase==MatchPhase::grant&&body.pending_grant==2&&body.gifts[1]==1
          &&body.queue.empty()&&body.state.players[1].hp==30&&body.state.players[1].mp==10
          &&body.state.players[1].cp==20&&body.state.players[1].curses.empty()&&!body.last_tick&&body.rng==seed,
          "v81 self66 applies2 with no extra defense,resource fee,disease tick or draw before its gift");
    v64::receipt(game,slots,2,5,21);
    (void)game.step({1,CommandKind::play,{consumed_slot(slots,1,4)},1});roundtrip(game);
    check(game.snapshot().phase==MatchPhase::grant&&game.snapshot().pending_grant==1
          &&game.snapshot().state.players[0].mp==20&&find_card(196)->ability_value==10,
          "v81 received CPU196 recovery derives its ten MP from the existing catalog body");
    v64::receipt(game,slots,1,4,3);compare_checkpoint(game,slots,checkpoints[2]);
    const auto& s=game.snapshot();
    check(s.decisions==7&&s.rng==seed&&s.chance_rolls==0&&s.counter_rolls==0&&s.dream_rolls==0
          &&s.dream_masks==0&&s.redraws==0&&s.auto_guardian_actions==0&&s.revivals==0,
          "v81 seven native decisions compare three received updates without fitting or a prelaunch wire claim");
}
}

}
int main() {
    constexpr std::array<std::uint64_t,8> seeds{0,1,2,7,42,1337,1000000000,0xfffffffffffffffeULL};
    for(const auto seed:seeds) {
        replay(seed);
        replay_v60(seed,false);replay_v60(seed,true);
        replay_v62(seed);replay_v63(seed);v64::replay(seed);v65::replay_fate(seed);v65::neptune_effect_component(seed);v66::replay(seed,false);v66::replay(seed,true);v68::replay(seed);v69::replay(seed);v70::replay(seed);v70::replay(seed,true);v72::replay(seed);v73::replay(seed);v74::replay(seed);v75::replay(seed);v76::replay(seed);v78::replay(seed);v79::replay(seed);v80::replay(seed);v81::replay(seed);
    }
    std::cout<<"official wire trace: 8 replays x3 curated viewer operations, 4 checkpoints each; "
             <<"explicit ordinary receipts, qualified slot mapping and full native boundary restores; "
             <<"not full matches, server RNG equivalence or public-player policy inputs\n";
    std::cout<<"v60: 8 seeds x2 unknown-CPU-receipt-order completions x2 received checkpoints; "
             <<"pre-launch CPU hand restored from received use events; dead UI placeholder is not live debt\n";
    std::cout<<"v62: existing 8 fixed seeds x2 received checkpoints, no-apocalypse198/191 plays and explicit191/207 receipts; "
             <<"all seeds match the observed non-worsening Heaven branch without search or skip; not server RNG equivalence\n";
    std::cout<<"v63: existing 8 seeds x1 received post-only checkpoint; reconstructed pre-play copies, self80 and explicit135/135; no original initial draw/order or server RNG claim\n";
    std::cout<<"v64: existing 8 seeds x4 received checkpoints; reconstructed GF3, identity135/195 use and actual197/fake195 MP15; explicit artifacts and CPU inventory-delta completions, not backend order or RNG equivalence\n";
    std::cout<<"v65: 8 seeds x3 owned207/Fog received checkpoints; separate8 Neptune276 body-only components, no summon/automatic-guardian full replay or seed fitting\n";
    std::cout<<"v66: 2 scenarios x8 seeds x2 state checkpoints; self45 endpoint and self14 two hits/single receipt; absorb intermediate order and unobserved BUY offer excluded\n";
    std::cout<<"v68: 8 seeds x6 received states from initial setup through self27 atHP3 toHP7; five viewer operations and CPU MP recovery; explicit receipts,not a completed game or server RNG proof\n";
    std::cout<<"v69: 8 seeds x5 received post-redraw states; masked19/27 self,113/135 defense,CP0 exchange and empty response;178 generation and multi-candidate BUY selector excluded\n";
    std::cout<<"v70: 8 seeds x6 reused v69 received states including178 with supplied8 replacements; reserved gifts separate,full private replay; no new observations or selector/RNG equivalence\n";
    std::cout<<"v71: 8 seeds x7 reused v69 received states,extending supplied178 with supplied seller5 CP0 BUY rejection and237 receipt; final CPU BUY unanswered,no selection seed fitting or probability proof\n";
    std::cout<<"v72: 8 seeds x4 received post-only updates,self41 DarkCloud without self-defense,179 versusDark92,self49 and lethal compound22; pre41 scene reconstructed,no initial-draw/order or low-HP45 claim\n";
    std::cout<<"v73: 8 seeds x4 received initial-to-end states,self55,11damage-to22counter,190reflection and empty fatal response;initial hand/ordinary184 supplied,no initial distribution,CPU policy or server RNG claim\n";
    std::cout<<"v74: 8 seeds x2 received endpoints1 and3 with missing rawupdate2;UI-observed self21,supplied23 BUY and normal44/13 deltas;no fabricated update2,complete lifecycle or selection distribution claim\n";
    std::cout<<"v75: 8 seeds x2 received states,self58 atMP10 for20 damage/allMP and supplied23 receipt;CPU221 launch remains unanswered,no runtime change,initial distribution or server RNG claim\n";
    std::cout<<"v76: 8 fixed seeds x2 received states,self48 for13 damage/Cold/one1-damage tick before supplied58;CPU4+168 SELL unanswered,no seed fitting,distribution or server RNG equivalence claim\n";
    std::cout<<"v78: 8 fixed seeds x2 received states,self53 for14+14 damage and supplied160;CPU19 unanswered,no source-versus-target order,low-HP boundary,initial distribution or server RNG equivalence claim\n";
    std::cout<<"v79: 8 fixed seeds x2 received states,self72/Fog and supplied10;UI hides CPU status while world retains40/10/20,CPU40 unanswered;v77 remains a separate UI-only record,no initial distribution or server RNG equivalence claim\n";
    std::cout<<"v80: 8 fixed seeds x2 selected received states,self70/Flash and supplied4;one-card preflight is local only,CPU10 unanswered,unused224,no fitted outcomes or server RNG equivalence claim\n";
    std::cout<<"v81: 8 fixed seeds x3 received states,viewer2 empty defense and self66;CPU29 prelaunch copy reconstructed from event,CPU196/3 deltas and actual21 supplied,CPU196 recovery;no prelaunch wire,element-priority or server RNG equivalence claim\n";
}
