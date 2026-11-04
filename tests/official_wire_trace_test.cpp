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
}
int main() {
    constexpr std::array<std::uint64_t,8> seeds{0,1,2,7,42,1337,1000000000,0xfffffffffffffffeULL};
    for(const auto seed:seeds)replay(seed);
    std::cout<<"official wire trace: 8 replays x3 curated viewer operations, 4 checkpoints each; "
             <<"explicit ordinary receipts, qualified slot mapping and full native boundary restores; "
             <<"not full matches, server RNG equivalence or public-player policy inputs\n";
}
