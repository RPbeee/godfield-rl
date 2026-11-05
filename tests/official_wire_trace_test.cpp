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
};
struct ReceivedCheckpoint {
    unsigned update; std::int64_t gf; PlayerId owner;
    std::array<ReceivedPlayer,2> players; CardId pending = 0;
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
void compare_checkpoint(const Match& game,const SlotMap& slots,const ReceivedCheckpoint& expected) {
    const auto& actual=game.snapshot();
    check(actual.state.gf==expected.gf&&actual.owner==expected.owner,"observed GF and turn owner match checkpoint");
    check(!actual.state.is_over&&actual.state.players.size()==2,"both received participants remain in a nonterminal match");
    std::size_t total_positive=0;
    for(std::size_t seat=0;seat<expected.players.size();++seat) {
        const auto& want=expected.players[seat];const auto& got=actual.state.players[seat];
        check(got.id==want.id&&got.hp==want.hp&&got.mp==want.mp&&got.cp==want.cp,"received gameplay resources match");
        check(got.team==Team::solo&&!got.dead&&got.guardian==Guardian::none&&got.confusion_count==0,"received default team, alive and guardian state match");
        const std::vector<Curse> curses=want.cold?std::vector<Curse>{Curse::cold}:std::vector<Curse>{};
        check(got.curses==curses,"Cold addition and mild cure match received snapshots");
        std::size_t positive=0,placeholders=0;
        for(const auto& row:want.slots) {
            if(!row.model){check(row.slot==0,"known wire placeholder is id0/model0");++placeholders;continue;}
            ++positive;
            const auto mapped=slots.find({want.id,row.slot});check(mapped!=slots.end(),"positive received slot has a current native mapping");
            const auto found=std::find_if(got.items.begin(),got.items.end(),[&](const auto& item){return item.token==mapped->second;});
            check(found!=got.items.end()&&found->model==row.model&&found->fake_model.value_or(0)==row.fake&&found->used==row.used,
                  "positive inventory model/fake/used match independently of server slot identity");
        }
        check(got.items.size()==positive&&actual.gifts[seat]==placeholders,"received model0 placeholders match local reserved debts, not physical copies");
        total_positive+=positive;
    }
    check(slots.size()==total_positive,"slot map contains no consumed stale slot or placeholder");
    if(expected.pending) {
        check(actual.phase==MatchPhase::response&&actual.actor==1&&actual.queue.size()==1,"pending CPU attack has viewer as response actor");
        const auto& effect=actual.queue.front();const auto* card=find_card(expected.pending);
        check(card&&effect.models==std::vector<CardId>{expected.pending}&&effect.origin==2&&effect.source==2&&effect.target==1
              &&effect.category==Category::weapons&&effect.power==card->atk&&effect.element==card->element
              &&effect.ability==card->ability&&effect.amount==card->ability_value
              &&effect.hit_rate==0&&!effect.is_counter&&!effect.is_death&&!effect.environmental,"pending source, target and derived attack match wire head");
        // Curse is derived from the retained base model, not a separate frame field.
        // The later received Cold state is also checked directly at update3.
        if(expected.pending==62)check(effect.ability==Ability::add_curse_on_damage&&card->curse==Curse::cold,
                                     "pending62 retains the on-damage Cold payload through its model");
    } else check(actual.phase==MatchPhase::main&&actual.actor==expected.owner&&actual.queue.empty(),"quiescent received boundary is main with no pending attack");
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

}
int main() {
    constexpr std::array<std::uint64_t,8> seeds{0,1,2,7,42,1337,1000000000,0xfffffffffffffffeULL};
    for(const auto seed:seeds) {
        replay(seed);
        replay_v60(seed,false);replay_v60(seed,true);
        replay_v62(seed);replay_v63(seed);v64::replay(seed);v65::replay_fate(seed);v65::neptune_effect_component(seed);
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
}
