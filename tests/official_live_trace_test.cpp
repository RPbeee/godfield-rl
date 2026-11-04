#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using namespace godfield::official;
constexpr PlayerId cpu_id=0,viewer_id=7; // Local role mapping, not asserted server IDs.
constexpr ItemToken attack_token=1000; // Arbitrary local physical token, not a model ID.
constexpr ItemToken defense_token=2000,unused_defense_token=3000;
struct Resources {
    std::int64_t hp,mp,cp;
    bool operator==(const Resources&) const = default;
};
struct LiveTransitionCase {
    std::string_view id;
    std::int64_t gf_before,gf_after;
    Resources cpu_before,viewer_before,cpu_after,viewer_after;
    PlayerId attacker,target,next_owner;
    std::span<const CardId> attack_models,defense_models;
    std::int64_t displayed_power,displayed_defense;
    Attribute displayed_element;
    std::size_t viewer_hand_count;
    std::optional<std::size_t> viewer_after_hand_count;
    bool unselected_defense7=false;
};
constexpr std::array<CardId,0> no_models{};
constexpr std::array<CardId,1> star_staff{{75}},power_halberd{{32}},ogre_helm{{150}};
constexpr std::array<CardId,3> absorption_column{{45,124,34}};
// A partial public witness: no server damage/RNG/hidden inventory is supplied to
// the executor. The observations are checkpoints, not a whole-game replay.
constexpr std::array cases{
    LiveTransitionCase{"training2-gf1-star-staff-empty-defense",1,2,
        {40,10,20},{40,10,20},{40,10,20},{37,10,20},cpu_id,viewer_id,viewer_id,star_staff,no_models,3,0,Element::light,9,9},
    LiveTransitionCase{"training2-gf4-self-power-halberd-no-response",4,5,
        {40,25,20},{37,10,20},{40,25,20},{28,10,20},viewer_id,viewer_id,cpu_id,power_halberd,no_models,9,0,{},9,{},true},
    LiveTransitionCase{"training2-gf5-absorption26-defense7",5,6,
        {40,25,20},{28,10,20},{59,25,20},{9,10,20},cpu_id,viewer_id,viewer_id,absorption_column,ogre_helm,26,7,{},9,9}
};
void check(bool ok,std::string_view case_id,const char* message){
    if(!ok){std::cerr<<"FAIL live transition "<<case_id<<": "<<message<<'\n';std::exit(1);}
}
Resources resources(const PlayerState& player){return {player.hp,player.mp,player.cp};}
const PlayerState& by_role(const Match& match,PlayerId id){return *find_player(match.snapshot().state,id);}
CardId unused_filler(unsigned profile,std::size_t offset){
    // Unknown *unused* copies only. Invented used miracles would contradict a
    // known public used-pane, and are never needed for this witness.
    constexpr std::array<CardId,12> passive_rich{{208,112,209,207,178,184,188,223,235,3,4,5}};
    constexpr std::array<CardId,12> varied{{6,58,14,94,203,189,190,233,234,198,201,239}};
    if(profile==0)return 6;
    return profile==1?passive_rich[offset%passive_rich.size()]:varied[offset%varied.size()];
}
SnapshotState completion(const LiveTransitionCase& witness,std::size_t cpu_count,unsigned profile){
    SnapshotState scene;scene.gf=witness.gf_before; // Observed no-apocalypse mode.
    PlayerState cpu;cpu.id=cpu_id;cpu.hp=witness.cpu_before.hp;cpu.mp=witness.cpu_before.mp;cpu.cp=witness.cpu_before.cp;
    PlayerState viewer;viewer.id=viewer_id;viewer.hp=witness.viewer_before.hp;viewer.mp=witness.viewer_before.mp;viewer.cp=witness.viewer_before.cp;
    auto& attacker=witness.attacker==cpu_id?cpu:viewer;
    for(std::size_t i=0;i<witness.attack_models.size();++i)attacker.items.push_back({attack_token+i,witness.attack_models[i],{},false});
    for(std::size_t i=0;i<witness.defense_models.size();++i)viewer.items.push_back({defense_token+i,witness.defense_models[i],{},false});
    // Only DEF7 and its nonuse were established for case2: this plain DEF7
    // surrogate does not assert the unselected inventory's exact model identity.
    if(witness.unselected_defense7)viewer.items.push_back({unused_defense_token,149,{},false});
    for(std::size_t i=0;cpu.items.size()<cpu_count;++i)cpu.items.push_back({i,unused_filler(profile,i),{},false});
    for(std::size_t i=0;viewer.items.size()<witness.viewer_hand_count;++i)viewer.items.push_back({i,unused_filler(profile,i),{},false});
    // No diseases/guardians were observed. Neither unknown copies nor the gift
    // model can trigger passive208/112/209 on this nonlethal plain attack.
    scene.players={std::move(cpu),std::move(viewer)};return scene;
}
void verify(const LiveTransitionCase& witness,std::uint64_t seed,std::size_t cpu_count,unsigned profile){
    check(attack_power(witness.attack_models)==witness.displayed_power
        &&attack_element(witness.attack_models)==witness.displayed_element &&magic_cost(witness.attack_models)==0,
        witness.id,"public catalog independently predicts the observed ordered attack summary");
    std::vector<VisibleSelection> prefix;
    for(auto model:witness.attack_models){check(can_append_main(prefix,{model,false}),witness.id,"observed source column satisfies the client append predicate");prefix.push_back({model,false});}
    std::int64_t defense=0;prefix.clear();
    for(auto model:witness.defense_models){check(can_append_response(prefix,{model,false},{false,Category::weapons,witness.displayed_element,true})
            &&reaction_for(model,Category::weapons,witness.displayed_element)==Reaction::none,witness.id,"observed armor is ordinary defense, not a counter or reflection");
        defense+=find_card(model)->defense;prefix.push_back({model,false});}
    check(defense==witness.displayed_defense,witness.id,"catalog DEF predicts the public response summary");
    if(witness.attack_models.front()==45)check(find_card(124)->category==Category::armor&&find_card(124)->atk==5&&find_card(124)->is_plus_atk
        &&find_card(34)->atk==9&&find_card(34)->is_plus_atk&&find_card(34)->ability==Ability::bounce_miracle,
        witness.id,"armor124 and weapon34 are source ATK extras, not independently cast response abilities");
    MatchConfig config;config.initial_hand=0;config.include_fate=true;config.max_decisions=16;
    Match match(config);match.load_scene(completion(witness,cpu_count,profile),witness.attacker,seed);
    std::vector<ItemToken> attack_tokens,defense_tokens;
    for(std::size_t i=0;i<witness.attack_models.size();++i)attack_tokens.push_back(attack_token+i);
    for(std::size_t i=0;i<witness.defense_models.size();++i)defense_tokens.push_back(defense_token+i);
    // Use only observed choices, never a fitted seed or a post-result damage.
    const Command attack{witness.attacker,CommandKind::play,attack_tokens,witness.target};
    check(match.legal(attack),witness.id,"observed selected weapon and target are accepted");
    (void)match.step(attack);
    if(witness.attacker==witness.target){
        check(match.snapshot().phase!=MatchPhase::response&&match.snapshot().queue.empty()&&match.snapshot().decisions==1,
            witness.id,"observed self attack resolves from one play without inventing a defense choice");
        const auto& items=by_role(match,viewer_id).items;
        check(std::any_of(items.begin(),items.end(),[](const auto& item){return item.token==unused_defense_token&&item.model==149&&!item.used;})
            &&!match.legal({viewer_id,CommandKind::defend,{unused_defense_token},{}}),
            witness.id,"available ordinary DEF7 remains unused and no self-response input is exposed");
    }else{
        const auto& pending=match.snapshot();
        check(pending.phase==MatchPhase::response && pending.actor==viewer_id && pending.owner==witness.attacker
            && pending.state.gf==witness.gf_before && pending.queue.size()==1
            && pending.queue.front().power==witness.displayed_power && pending.queue.front().element==witness.displayed_element
            &&pending.queue.front().models==std::vector<CardId>(witness.attack_models.begin(),witness.attack_models.end()),
            witness.id,"engine predicts the observed attack before a response");
        if(witness.attack_models.front()==45)check(pending.queue.front().ability==Ability::absorb_hp
            &&pending.queue.front().redirects.empty(),witness.id,"only the base absorption ability is active; extra34 does not bounce the source column");
        check(resources(by_role(match,cpu_id))==witness.cpu_before && resources(by_role(match,viewer_id))==witness.viewer_before,
            witness.id,"launch has not prematurely applied observed body HP changes");
        const Command response{viewer_id,CommandKind::defend,defense_tokens,{}};
        check(match.legal(response),witness.id,"observed response is accepted");
        (void)match.step(response);
    }
    check(resources(by_role(match,cpu_id))==witness.cpu_after && resources(by_role(match,viewer_id))==witness.viewer_after,
        witness.id,"computed resource result equals the actual public checkpoint");
    check(match.snapshot().state.gf==witness.gf_after && match.snapshot().phase==MatchPhase::main
        && match.snapshot().owner==witness.next_owner && match.snapshot().actor==witness.next_owner && !match.done(),
        witness.id,"observed next main owner and GF are predicted");
    if(witness.viewer_after_hand_count)check(by_role(match,viewer_id).items.size()==*witness.viewer_after_hand_count,
        witness.id,"known viewer inventory count matches without comparing new gift models");
    // Excluded from assertions: CPU inventory/count, replacement models/tokens,
    // final RNG, real server IDs, and unobserved intermediate event ordering.
}
}
int main(){
    constexpr std::array<std::uint64_t,8> seeds{{0,1,2,42,123,20261104,0x9e3779b97f4a7c15ULL,std::numeric_limits<std::uint64_t>::max()}};
    std::size_t projections=0;
    for(const auto& witness:cases)for(auto count:{witness.attacker==cpu_id?witness.attack_models.size():std::size_t{1},std::size_t{9},std::size_t{18}})
        for(unsigned profile=0;profile<3;++profile)for(auto seed:seeds){verify(witness,seed,count,profile);++projections;}
    std::cout<<"Official live transition projection passed ("<<cases.size()<<" public witness, "<<projections
        <<" synthetic completions; hidden hands and server RNG not reconstructed).\n";
}
