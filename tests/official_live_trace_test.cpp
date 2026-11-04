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
constexpr ItemToken retained_miracle_token=4000;
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
    bool terminal=false;
    std::int64_t displayed_mp_cost=0;
    bool initial_used213=false;
    bool cpu_used_count_unknown=false;
};
constexpr std::array<CardId,0> no_models{};
constexpr std::array<CardId,1> star_staff{{75}},power_halberd{{32}},ogre_helm{{150}},goodbye_sword{{81}},great_tree{{213}},kusarigama{{19}};
constexpr std::array<CardId,1> sword_ware{{69}},spiritual_staff{{46}};
constexpr std::array<CardId,3> absorption_column{{45,124,34}};
// A partial public witness: no server damage/RNG/hidden inventory is supplied to
// the executor. The observations are checkpoints, not a whole-game replay.
constexpr std::array cases{
    LiveTransitionCase{"training2-gf1-star-staff-empty-defense",1,2,
        {40,10,20},{40,10,20},{40,10,20},{37,10,20},cpu_id,viewer_id,viewer_id,star_staff,no_models,3,0,Element::light,9,9},
    LiveTransitionCase{"training2-gf4-self-power-halberd-no-response",4,5,
        {40,25,20},{37,10,20},{40,25,20},{28,10,20},viewer_id,viewer_id,cpu_id,power_halberd,no_models,9,0,{},9,{},true},
    LiveTransitionCase{"training2-gf5-absorption26-defense7",5,6,
        {40,25,20},{28,10,20},{59,25,20},{9,10,20},cpu_id,viewer_id,viewer_id,absorption_column,ogre_helm,26,7,{},9,9},
    LiveTransitionCase{"training2-gf6-self-dark-goodbye-sword-terminal",6,6,
        {59,25,20},{9,10,20},{59,25,20},{0,10,20},viewer_id,viewer_id,viewer_id,goodbye_sword,no_models,4,0,Element::darkness,9,8,false,true},
    LiveTransitionCase{"training-gf1-self-great-tree-no-response",1,2,
        {40,10,20},{40,10,20},{40,10,20},{34,7,20},viewer_id,viewer_id,cpu_id,great_tree,no_models,6,0,Element::wood,9,10,false,false,3},
    LiveTransitionCase{"training-gf2-kusarigama-empty-defense",2,3,
        {40,10,20},{34,7,20},{40,10,20},{29,7,20},cpu_id,viewer_id,viewer_id,kusarigama,no_models,5,0,{},10,10,false,false,0,true},
    LiveTransitionCase{"training-gf5-reused-self-great-tree-no-response",5,6,
        {30,30,10},{23,4,20},{30,30,10},{17,1,20},viewer_id,viewer_id,cpu_id,great_tree,no_models,6,0,Element::wood,11,12,false,false,3,true,true},
    LiveTransitionCase{"training-gf2-stone-sword-ware-empty-defense",2,3,
        {40,10,20},{40,10,20},{40,10,20},{38,10,20},cpu_id,viewer_id,viewer_id,sword_ware,no_models,2,0,Element::stone,9,9},
    LiveTransitionCase{"training-gf3-self-spiritual-staff-no-response",3,4,
        {40,10,20},{38,10,20},{40,10,20},{26,10,20},viewer_id,viewer_id,cpu_id,spiritual_staff,no_models,12,0,{},9,9}
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
    if(witness.initial_used213){
        const auto retained=std::find_if(viewer.items.begin(),viewer.items.end(),[](const auto& item){return item.model==213;});
        if(retained!=viewer.items.end())retained->used=true;
        else viewer.items.push_back({retained_miracle_token,213,{},true});
    }
    // Only DEF7 and its nonuse were established for case2: this plain DEF7
    // surrogate does not assert the unselected inventory's exact model identity.
    if(witness.unselected_defense7)viewer.items.push_back({unused_defense_token,149,{},false});
    if(witness.terminal){
        // Eight model identities were read, including selected81. Only the ninth
        // card's DEF5 was transcribed, so vary compatible DEF5 surrogates rather
        // than invent an actor-side208/112 contradicting the observed hand.
        constexpr std::array<CardId,7> known_remaining{{3,12,13,25,195,203,221}};
        for(auto model:known_remaining)viewer.items.push_back({viewer.items.size(),model,{},false});
        viewer.items.push_back({unused_defense_token,static_cast<CardId>(135+profile),{},false});
    }
    for(std::size_t i=0;cpu.items.size()<cpu_count;++i)cpu.items.push_back({i,unused_filler(profile,i),{},false});
    for(std::size_t i=0;viewer.items.size()<witness.viewer_hand_count;++i)viewer.items.push_back({i,unused_filler(profile,i),{},false});
    // No diseases/guardians were observed. All except the Dark attack are nonlethal;
    // the fourth actor's known remaining hand has no208/112. Hidden CPU copies
    // cannot intercept or revive the other player's self-directed damage.
    scene.players={std::move(cpu),std::move(viewer)};return scene;
}
void verify(const LiveTransitionCase& witness,std::uint64_t seed,std::size_t cpu_count,unsigned profile){
    check(attack_power(witness.attack_models)==witness.displayed_power
        &&attack_element(witness.attack_models)==witness.displayed_element &&magic_cost(witness.attack_models)==witness.displayed_mp_cost,
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
    if(witness.initial_used213)check(std::count_if(by_role(match,viewer_id).items.begin(),by_role(match,viewer_id).items.end(),
        [](const auto& item){return item.used&&item.model==213;})==1,witness.id,"observed initial used213 is represented before the command");
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
        if(witness.unselected_defense7){
            const auto& items=by_role(match,viewer_id).items;
            check(std::any_of(items.begin(),items.end(),[](const auto& item){return item.token==unused_defense_token&&item.model==149&&!item.used;})
                &&!match.legal({viewer_id,CommandKind::defend,{unused_defense_token},{}}),
                witness.id,"available ordinary DEF7 remains unused and no self-response input is exposed");
        }
        if(witness.attack_models.front()==46)check(std::none_of(by_role(match,viewer_id).items.begin(),by_role(match,viewer_id).items.end(),
            [](const auto& item){return item.token==attack_token;}),witness.id,"the selected46 copy is consumed without constraining its replacement model");
        // After the first213, the next CPU's zero-MP weapon was observed before
        // its body and cannot change the compared projection. After the later
        // reused213, CPU main itself was read. Neither case invents CPU defense
        // or compares the unidentified new gift model.
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
    if(witness.attack_models.front()==213||witness.initial_used213){
        const auto& items=by_role(match,viewer_id).items;
        const auto retained_token=witness.attack_models.front()==213?attack_token:retained_miracle_token;
        check(std::any_of(items.begin(),items.end(),[&](const auto& item){return item.token==retained_token&&item.model==213&&item.used;})
            &&std::count_if(items.begin(),items.end(),[](const auto& item){return item.used;})==1,
            witness.id,"the same physical213 stays retained with exactly one used miracle");
        if(!witness.cpu_used_count_unknown)check(std::none_of(by_role(match,cpu_id).items.begin(),by_role(match,cpu_id).items.end(),[](const auto& item){return item.used;}),
            witness.id,"the observed CPU used-miracle pane remains empty");
    }
    if(witness.terminal){
        check(match.snapshot().state.gf==witness.gf_after&&match.snapshot().phase==MatchPhase::finished
            &&match.snapshot().terminated&&!match.snapshot().truncated&&!match.snapshot().actor
            &&match.snapshot().winners==std::vector<PlayerId>{cpu_id}
            &&match.snapshot().rewards==std::vector<double>{1.,-1.}
            &&by_role(match,viewer_id).dead&&!by_role(match,cpu_id).dead,
            witness.id,"one self Dark command predicts terminal CPU victory with GF unchanged");
        check(match.snapshot().rng==seed&&std::all_of(match.snapshot().gifts.begin(),match.snapshot().gifts.end(),[](auto debt){return debt==0;})
            &&std::none_of(by_role(match,viewer_id).items.begin(),by_role(match,viewer_id).items.end(),[](const auto& item){return item.token==attack_token;}),
            witness.id,"selected81 is erased and natural termination cancels replacement without drawing");
    }else check(match.snapshot().state.gf==witness.gf_after && match.snapshot().phase==MatchPhase::main
            && match.snapshot().owner==witness.next_owner && match.snapshot().actor==witness.next_owner && !match.done(),
            witness.id,"observed next main owner and GF are predicted");
    if(witness.viewer_after_hand_count)check(by_role(match,viewer_id).items.size()==*witness.viewer_after_hand_count,
        witness.id,"known viewer inventory count matches without comparing new gift models");
    // Excluded from assertions: CPU inventory/count, replacement models/tokens,
    // final RNG, real server IDs, and unobserved intermediate event ordering.
}
std::size_t verify_self_sale_projection(std::span<const std::uint64_t> seeds){
    constexpr std::string_view label="training-gf1-self-sale-naginata";
    constexpr ItemToken operation=attack_token,spare_operation=attack_token+1,commodity=attack_token+2;
    constexpr std::array<CardId,2> models{{4,26}};
    const std::array<VisibleSelection,1> prefix{{{4,false}}};
    check(can_append_main(prefix,{26,false})&&magic_cost(models)==0&&find_card(26)->price==3,
        label,"public catalog predicts a zero-MP sale of the price3 commodity");
    std::size_t probes=0;
    for(auto count:{std::size_t{1},std::size_t{9},std::size_t{18}})for(unsigned profile=0;profile<3;++profile)for(auto seed:seeds){
        SnapshotState scene;scene.gf=1;
        PlayerState cpu;cpu.id=cpu_id;PlayerState viewer;viewer.id=viewer_id;
        viewer.items={{operation,4,{},false},{spare_operation,4,{},false},{commodity,26,{},false}};
        for(std::size_t i=0;viewer.items.size()<9;++i){const auto model=unused_filler(profile,i);if(model!=4&&model!=26)viewer.items.push_back({i,model,{},false});}
        for(std::size_t i=0;i<count;++i)cpu.items.push_back({i,unused_filler(profile,i),{},false});
        scene.players={std::move(cpu),std::move(viewer)};
        MatchConfig config;config.initial_hand=0;config.include_fate=true;config.max_decisions=16;Match match(config);match.load_scene(std::move(scene),viewer_id,seed);
        const auto receipt_token=match.snapshot().next_token;
        const Command sale{viewer_id,CommandKind::sell,{operation,commodity},viewer_id};
        check(match.legal(sale),label,"one owned SELL and the unused commodity accept the observed self target");
        (void)match.step(sale);const auto& snapshot=match.snapshot();const auto& remaining=by_role(match,viewer_id).items;
        check(snapshot.decisions==1&&snapshot.phase==MatchPhase::main&&snapshot.owner==cpu_id&&snapshot.actor==cpu_id
            &&snapshot.state.gf==2&&snapshot.queue.empty()&&!match.done()
            &&resources(by_role(match,viewer_id))==Resources{40,10,20}&&resources(by_role(match,cpu_id))==Resources{40,10,20}
            &&remaining.size()==9,label,"one self sale predicts unchanged public resources and nine copies at GF2 CPU main");
        check(snapshot.last_sale&&snapshot.last_sale->origin==viewer_id&&snapshot.last_sale->source==viewer_id
            &&snapshot.last_sale->target==viewer_id&&snapshot.last_sale->model==26&&snapshot.last_sale->price==3,
            label,"the sale resolves the price3 commodity with self debit and credit rather than an attack");
        check(std::none_of(remaining.begin(),remaining.end(),[](const auto& item){return item.token==operation||item.token==commodity;})
            &&std::any_of(remaining.begin(),remaining.end(),[](const auto& item){return item.token==spare_operation&&item.model==4&&!item.used;})
            &&std::any_of(remaining.begin(),remaining.end(),[&](const auto& item){return item.token==receipt_token&&item.model==26&&!item.used;}),
            label,"selected operation and original commodity are consumed while spare SELL and delivered26 remain");
        // Actual UI totals were SELL2->1 and26 one->one, with a known gift46.
        // Do not fit that gift: compare the known old/delivered copies separately
        // from the new random gift, which may itself be4 or26 in this local run.
        check(std::count_if(remaining.begin(),remaining.end(),[&](const auto& item){return item.token<=receipt_token&&item.model==4;})==1
            &&std::count_if(remaining.begin(),remaining.end(),[&](const auto& item){return item.token<=receipt_token&&item.model==26;})==1,
            label,"known-copy model counts match independently of the unconstrained replacement gift");
        ++probes;
    }
    return probes;
}
std::size_t verify_self_reflection_routing(){
    constexpr std::string_view label="training-apocalypse-gf2-self-reflection-sword-no-response";
    constexpr std::array<CardId,1> models{{38}};
    constexpr std::array<ItemToken,2> unused_defenses{{unused_defense_token,unused_defense_token+1}};
    check(attack_power(models)==10&&magic_cost(models)==0&&find_card(38)->ability==Ability::reflect_weapon,
        label,"the selected reflection sword independently predicts a zero-cost10 self payload");
    std::size_t probes=0;
    for(auto count:{std::size_t{1},std::size_t{9},std::size_t{18}})for(unsigned profile=0;profile<3;++profile)
        for(auto seed:{std::uint64_t{0},std::uint64_t{1},std::uint64_t{2},std::uint64_t{42},std::uint64_t{123},std::uint64_t{20261104},
            std::uint64_t{0x9e3779b97f4a7c15ULL},std::numeric_limits<std::uint64_t>::max()}){
            SnapshotState scene;scene.gf=2;scene.tiebreak_gf=1;
            PlayerState cpu;cpu.id=cpu_id;cpu.hp=50;cpu.mp=10;cpu.cp=20;
            PlayerState viewer;viewer.id=viewer_id;viewer.hp=40;viewer.mp=10;viewer.cp=20;
            viewer.items={{attack_token,38,{},false},{unused_defense_token,135,{},false},{unused_defense_token+1,20,{},false}};
            for(std::size_t i=0;viewer.items.size()<9;++i)viewer.items.push_back({i,unused_filler(profile,i),{},false});
            for(std::size_t i=0;i<count;++i)cpu.items.push_back({i,unused_filler(profile,i),{},false});
            scene.players={std::move(cpu),std::move(viewer)};
            MatchConfig config;config.initial_hand=0;config.external_grants=true;config.max_decisions=16;
            Match match(config);match.load_scene(std::move(scene),viewer_id,seed);
            const Command play{viewer_id,CommandKind::play,{attack_token},viewer_id};
            check(match.legal(play),label,"observed single reflection sword self selection is legal");
            (void)match.step(play);
            // The live UI advanced without a defense choice, but its grant and
            // subsequent CPU action were not captured. This grant pause is a
            // local test boundary, NOT a claim that the actual UI stopped here.
            const auto& snapshot=match.snapshot();const auto& remaining=by_role(match,viewer_id).items;
            check(snapshot.phase==MatchPhase::grant&&!snapshot.actor&&snapshot.pending_grant==viewer_id
                &&snapshot.decisions==1&&snapshot.queue.empty()&&!match.done()&&snapshot.state.gf==2
                &&resources(by_role(match,viewer_id))==Resources{30,10,20}
                &&resources(by_role(match,cpu_id))==Resources{50,10,20},
                label,"one self play reaches deferred gift processing without requiring a defense command");
            check(remaining.size()==8&&std::none_of(remaining.begin(),remaining.end(),[](const auto& item){return item.token==attack_token;})
                &&std::all_of(unused_defenses.begin(),unused_defenses.end(),
                    [&](auto token){return std::any_of(remaining.begin(),remaining.end(),[&](const auto& item){return item.token==token&&!item.used;});})
                &&snapshot.rng==seed&&!match.legal({viewer_id,CommandKind::defend,{unused_defense_token},{}}),
                label,"reflection sword is consumed, both available defenses remain unused and no grant draw is invented");
            Match restored(config);restored.restore(snapshot);check(restored.snapshot()==snapshot,label,"self reflection grant journal roundtrips");
            ++probes;
        }
    return probes;
}
}
int main(){
    constexpr std::array<std::uint64_t,8> seeds{{0,1,2,42,123,20261104,0x9e3779b97f4a7c15ULL,std::numeric_limits<std::uint64_t>::max()}};
    std::size_t projections=0;
    for(const auto& witness:cases)for(auto count:{witness.attacker==cpu_id?witness.attack_models.size():std::size_t{1},std::size_t{9},std::size_t{18}})
        for(unsigned profile=0;profile<3;++profile)for(auto seed:seeds){verify(witness,seed,count,profile);++projections;}
    projections+=verify_self_sale_projection(seeds);
    const auto routing_probes=verify_self_reflection_routing();
    std::cout<<"Official live transition projection passed ("<<cases.size()+1<<" public witness, "<<projections
        <<" synthetic completions; "<<routing_probes<<" separate routing-only probes; hidden hands and server RNG not reconstructed).\n";
}
