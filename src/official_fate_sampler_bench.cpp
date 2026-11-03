#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
using namespace godfield::official;
void check(bool ok, const char* message) { if (!ok) throw std::logic_error(message); }
std::uint64_t number(const char* text, bool positive=true) {
    std::uint64_t value=0; const std::string input(text);
    const auto r=std::from_chars(input.data(),input.data()+input.size(),value);
    if (r.ec!=std::errc{} || r.ptr!=input.data()+input.size() || (positive && !value))
        throw std::invalid_argument("invalid numeric argument");
    return value;
}
void mix(std::uint64_t& hash, std::uint64_t value) {
    for (unsigned byte=0;byte!=8;++byte) { hash^=(value>>(byte*8))&255; hash*=1099511628211ULL; }
}
void text_hash(std::uint64_t& hash, const std::string& text) {
    mix(hash,text.size()); for (unsigned char c:text) { hash^=c; hash*=1099511628211ULL; }
}
void item_hash(std::uint64_t& hash, const ItemInstance& item) {
    mix(hash,item.token); mix(hash,item.model); mix(hash,item.used);
    mix(hash,item.fake_model.has_value()); if (item.fake_model) mix(hash,*item.fake_model);
}
// Deliberately a world/slot and selected-counter hash, not a whole-snapshot
// hash: Fate records and pending journals are checked by full restore.
std::uint64_t world_hash(const MatchSnapshot& s) {
    std::uint64_t hash=14695981039346656037ULL;
    mix(hash,s.version); mix(hash,s.config.include_fate); mix(hash,s.config.external_grants); mix(hash,s.state.gf); mix(hash,s.state.update_count); mix(hash,s.state.is_over);
    mix(hash,s.state.players.size());
    for (const auto& player:s.state.players) {
        mix(hash,player.id); text_hash(hash,player.name);
        mix(hash,player.user_id.has_value()); if (player.user_id) text_hash(hash,*player.user_id);
        mix(hash,player.alias.has_value()); if (player.alias) text_hash(hash,*player.alias);
        mix(hash,static_cast<unsigned>(player.team)); mix(hash,player.hp); mix(hash,player.mp); mix(hash,player.cp);
        mix(hash,player.dead); mix(hash,static_cast<unsigned>(player.guardian)); mix(hash,player.confusion_count);
        mix(hash,player.curses.size()); for (auto curse:player.curses) mix(hash,static_cast<unsigned>(curse));
        mix(hash,player.items.size()); for (const auto& item:player.items) item_hash(hash,item);
    }
    mix(hash,s.owner); mix(hash,s.actor.has_value()); if (s.actor) mix(hash,*s.actor);
    mix(hash,static_cast<unsigned>(s.phase)); mix(hash,s.rng); mix(hash,s.next_token); mix(hash,s.decisions);
    mix(hash,s.grant_resolutions);mix(hash,s.grant_artifacts);mix(hash,s.grant_devils);
    mix(hash,s.terminated); mix(hash,s.truncated); mix(hash,s.phenomenon_actions); mix(hash,s.fate_actions);
    mix(hash,s.chance_rolls); mix(hash,s.chance_hits); mix(hash,s.chance_forced_hits);
    mix(hash,s.counter_created); mix(hash,s.counter_started); mix(hash,s.counter_rolls); mix(hash,s.counter_hits); mix(hash,s.counter_forced_hits);
    mix(hash,s.death_bows); mix(hash,s.death_created); mix(hash,s.death_started); mix(hash,s.death_rolls); mix(hash,s.death_hits); mix(hash,s.death_forced_hits);
    mix(hash,s.revivals); mix(hash,s.redraws); mix(hash,s.redrawn_items); mix(hash,s.dream_rolls); mix(hash,s.dream_masks);
    mix(hash,s.guardian_summons); mix(hash,s.guardian_opportunities); mix(hash,s.auto_guardian_actions);
    mix(hash,s.guardian_departure_rolls); mix(hash,s.guardian_departures);
    return hash;
}


void audit(const Match& game, Match& restored) {
    restored.restore(game.snapshot()); check(restored.snapshot()==game.snapshot(),"fate snapshot failed full roundtrip");
}
std::uint64_t bounded100(std::uint64_t& state) {
    constexpr auto bound=std::uint64_t{100}; constexpr auto threshold=(std::uint64_t{0}-bound)%bound;
    std::uint64_t value;
    do {
        value=(state+=0x9e3779b97f4a7c15ULL);
        value=(value^(value>>30))*0xbf58476d1ce4e5b9ULL;
        value=(value^(value>>27))*0x94d049bb133111ebULL;
        value^=value>>31;
    } while (value<threshold);
    return value%bound;
}
std::vector<CardId> used_models(const SnapshotState& state) {
    std::vector<CardId> result;
    for (const auto& player:state.players) for (const auto& item:player.items) if (item.used) result.push_back(item.model);
    std::sort(result.begin(),result.end()); return result;
}
void count_transition(const MatchSnapshot& before, const MatchSnapshot& after,
                      bool launch, CardId outcome) {
    for (const auto& old:before.state.players) {
        const auto* now=find_player(after.state,old.id); check(now!=nullptr,"fate changed the roster");
        auto expected=old.confusion_count;
        if (launch && old.id==0 && expected>0) --expected;
        if (launch && outcome==289 && !old.dead) expected+=3;
        if (now->dead || after.terminated) expected=0;
        check(now->confusion_count==expected,"fate count changed outside debit/289/death/end");
    }
}
struct Stats {
    std::uint64_t scenes=0,decisions=0,responses=0,purchase_commands=0,grant_commands=0,terminated=0,truncated=0;
    std::uint64_t sampler_checks=0,invalid_sampler_checks=0,confused_policy_refusals=0,target_self=0,target_ally=0,target_enemy=0,guardian_assignments=0;
    std::uint64_t include_fate_scenes=0,external_grant_scenes=0,cutoff_probes=0,shuffle_empty=0,shuffle_single=0,shuffle_other_token0=0;
    std::uint64_t chance_checksum=0,sampled_action_checksum=0;
    std::uint64_t fate_actions=0,confusion_debits=0,shuffle_scenes=0,shuffle_copies=0,shuffle_used_copies=0;
    std::uint64_t used_input_copies=0,used_preserved_at_launch=0,dead_input_scenes=0,sampler_exclusion_checks=0;
    std::uint64_t initial_world_responses=0,no_selector_world_checks=0,environmental_cancels=0,environmental_counter_cards=0;
    std::uint64_t real_frame_observations=0,source0_frame_observations=0,primitive_checksum=0,manifest_checksum=0,checksum=0;
    std::uint64_t chance_rolls=0,chance_hits=0,chance_forced=0,counter_created=0,counter_started=0,counter_rolls=0,counter_hits=0,counter_forced=0;
    std::uint64_t death_bows=0,death_created=0,death_started=0,death_rolls=0,death_hits=0,death_forced=0;
    std::uint64_t revivals=0,redraws=0,redrawn_items=0,disease_ticks=0,guardian_summons=0,guardian_opportunities=0,auto_guardian_actions=0;
    std::uint64_t departure_rolls=0,departures=0,gf_total=0;
    std::array<std::uint64_t,10> coverage{};
    void append(const Stats& other) {
#define ADD(name) name+=other.name
        ADD(scenes); ADD(decisions); ADD(responses); ADD(purchase_commands); ADD(grant_commands); ADD(terminated); ADD(truncated);
        ADD(sampler_checks);ADD(invalid_sampler_checks);ADD(confused_policy_refusals);ADD(target_self);ADD(target_ally);ADD(target_enemy);ADD(guardian_assignments);
        ADD(include_fate_scenes);ADD(external_grant_scenes);ADD(cutoff_probes);ADD(shuffle_empty);ADD(shuffle_single);ADD(shuffle_other_token0);
        ADD(chance_checksum);ADD(sampled_action_checksum);
        ADD(fate_actions); ADD(confusion_debits); ADD(shuffle_scenes); ADD(shuffle_copies); ADD(shuffle_used_copies);
        ADD(used_input_copies); ADD(used_preserved_at_launch); ADD(dead_input_scenes); ADD(sampler_exclusion_checks);
        ADD(initial_world_responses); ADD(no_selector_world_checks); ADD(environmental_cancels); ADD(environmental_counter_cards);
        ADD(real_frame_observations); ADD(source0_frame_observations); ADD(primitive_checksum); ADD(manifest_checksum); ADD(checksum);
        ADD(chance_rolls); ADD(chance_hits); ADD(chance_forced); ADD(counter_created); ADD(counter_started); ADD(counter_rolls); ADD(counter_hits); ADD(counter_forced);
        ADD(death_bows); ADD(death_created); ADD(death_started); ADD(death_rolls); ADD(death_hits); ADD(death_forced);
        ADD(revivals); ADD(redraws); ADD(redrawn_items); ADD(disease_ticks); ADD(guardian_summons); ADD(guardian_opportunities); ADD(auto_guardian_actions);
        ADD(departure_rolls); ADD(departures); ADD(gf_total);
#undef ADD
        for (std::size_t i=0;i<coverage.size();++i) coverage[i]+=other.coverage[i];
    }
};
void hash_action(std::uint64_t& hash,const FateAction& action) {
    mix(hash,action.actor);mix(hash,action.token);mix(hash,action.outcome.model);mix(hash,action.outcome.recipient.has_value());
    if(action.outcome.recipient)mix(hash,*action.outcome.recipient);
    mix(hash,action.outcome.guardians.size());for(auto guardian:action.outcome.guardians)mix(hash,static_cast<unsigned>(guardian));
    mix(hash,action.shuffle.has_value());if(action.shuffle){mix(hash,action.shuffle->order.size());for(const auto& ref:action.shuffle->order){mix(hash,ref.owner);mix(hash,ref.token);}}
}
void refuse_sample(Match& game,ItemToken token,std::uint64_t& chance,Stats& stats) {
    const auto before=game.snapshot();const auto prior=chance;bool failed=false;
    try{(void)game.sample_fate(token,chance);}catch(const MatchError&){failed=true;}
    check(failed && game.snapshot()==before && chance==prior,"invalid Fate sampling changed Match or caller-owned chance RNG");++stats.invalid_sampler_checks;
}
void inspect_frames(const Match& game, const FateAction& action, Stats& stats) {
    const auto& s=game.snapshot();
    const auto inspect=[&](const EffectFrame& effect, bool active) {
        check(find_player(s.state,effect.target)!=nullptr,"fate frame target missing");
        if (effect.environmental) {
            check(effect.origin==0 && effect.source==0 && !effect.is_counter && !effect.is_death
                  && effect.category==Category::phenomena && effect.models==std::vector<CardId>{action.outcome.model}
                  && effect.tokens.empty() && effect.target==*action.outcome.recipient,
                  "fate world frame fabricated a source or changed result/recipient");
            if (active) {
                const auto view=game.observe(effect.target);
                check(view.find("\"origin\":null,\"source\":null,")!=std::string::npos
                      && view.find("\"environmental\":true")!=std::string::npos,"world view exposes player0 as fate source");
            }
        } else {
            check(find_player(s.state,effect.origin)!=nullptr && find_player(s.state,effect.source)!=nullptr,
                  "fate derivative or guardian frame has no real source");
            if (active) { ++stats.real_frame_observations; stats.source0_frame_observations+=effect.source==0; }
        }
    };
    for (const auto& effect:s.queue) inspect(effect,true);
    for (const auto& effect:s.counters) inspect(effect,false);
    for (const auto& effect:s.deaths) inspect(effect,false);
}
void verify_proof(const MatchSnapshot& before, const MatchSnapshot& now, const FateAction& action) {
    check(now.fate_actions==1 && now.phenomenon_actions==0 && now.guardian_actions==0 && now.guardian_payload_actions==0,
          "owned fate was counted as another external activation");
    check(now.last_fate && now.last_fate->action==action && now.last_fate->anchor.initial_state==before.state
          && now.last_fate->anchor.rng==before.rng && now.last_fate->anchor.next_token==before.next_token
          && now.last_fate->anchor.decisions==0 && now.last_fate->anchor.fate_actions==0
          && now.last_fate->responses.size()==now.decisions-1-now.grant_resolutions
          && now.last_fate_event==FateEvent{0,action.outcome.model},"fate lost pre207 proof/public notice or journal accounting");
    if (now.batch) check(now.batch->fate_action==action && !now.batch->phenomenon_action
                         && now.batch->confusion_command==(before.state.players.front().confusion_count>0),
                         "pending fate lost its ordinary-schedule/confusion marker");
    const auto* source=find_player(now.state,0);
    check(std::none_of(source->items.begin(),source->items.end(),[](const auto& item) { return item.token==0; }),
          "original207 ownership token survived its consumption");
    for (const auto& player:before.state.players) if (player.dead)
        check(*find_player(now.state,player.id)==player,"fate changed an initially dead inventory/resource/count");
}
}
int main(int argc,char** argv) { try {
    if (argc>5) throw std::invalid_argument("usage: godfield_fate_sampler_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000,players=argc>2?number(argv[2]):2,seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if (count>1000000000 || players<2 || players>9 || workers>64 || seed>UINT64_MAX-(count-1))
        throw std::invalid_argument("fate sampler benchmark bounds exceeded");
    workers=std::min(workers,count);
    check(match_fate_supported(207) && !match_supported(207) && match_gift_weight()==499,"scene207 entered default gift pool");
    check(match_dream_compatible(6,7),"fate fixture fake is not compatible");
    std::vector<Stats> results(workers); std::vector<std::exception_ptr> errors(workers); std::vector<std::jthread> threads;
    const auto start=std::chrono::steady_clock::now();
    for (std::uint64_t worker=0;worker<workers;++worker) threads.emplace_back([&,worker] { try {
        auto& stats=results[worker];
        constexpr std::array<CardId,18> cards{207,6,235,208,112,190,178,184,187,188,3,4,5,203,236,239,66,207};
        for (std::uint64_t i=worker;i<count;i+=workers) {
            const bool cutoff=i%97==0;
            MatchConfig config;config.max_decisions=cutoff?2:96;config.include_fate=i%2==0;config.external_grants=cutoff || i%3==0;
            Match game(config),restored(config);
            SnapshotState state; state.gf=17;if(config.external_grants)state.tiebreak_gf=17; const bool dead_last=players==9 && i%17==0;
            for (std::uint64_t seat=0;seat<players;++seat) {
                PlayerState player; player.id=seat==0?0:100+7*seat; player.name="P"+std::to_string(seat);
                player.user_id="user"+std::to_string(seat); player.alias="A"+std::to_string(seat);
                player.team=i%3==0?(seat%2?Team::two:Team::one):Team::solo;
                player.hp=seat==0?99:15+static_cast<std::int64_t>((seed+i+17*seat)%85); player.mp=99; player.cp=(seed+i+seat)%100;
                player.guardian=cutoff?Guardian::none:static_cast<Guardian>(1+(seed+i+seat)%10);
                if (!cutoff && (i+seat)%4==0) player.curses.push_back(Curse::cold);
                if (!cutoff && (i+seat)%3==0) player.curses.push_back(Curse::fog);
                if (!cutoff && (i+seat)%2==0) player.curses.push_back(Curse::dream);
                if (!cutoff && (i+seat)%5==0) player.curses.push_back(Curse::darkcloud);
                player.dead=dead_last && seat+1==players; if (player.dead) player.hp=0;
                player.confusion_count=player.dead?0:static_cast<std::int64_t>((i+seat)%3);
                const auto small=i%53;const auto size=small<2?(seat==0?1+small:0):4+(seed+i+seat)%15;
                for (std::uint64_t slot=0;slot<size;++slot) {
                    ItemInstance item{slot,cards[slot],{},slot==2};
                    if (slot==1 && !cutoff && (i+seat)%2==0) item.fake_model=7;
                    player.items.push_back(item);
                }
                state.players.push_back(std::move(player));
            }
            game.load_scene(std::move(state),0,seed+i); audit(game,restored);
            const auto before=game.snapshot(); const auto alive=alive_players(before.state);
            std::uint64_t chance=(seed+i)^0x9b05688c2b3e6c1fULL;
            const auto chance_before=chance;auto trial=chance,repeat_chance=chance;
            Match first(config),second(config);first.restore(before);second.restore(before);
            const auto action=first.sample_fate(0,trial),repeat=second.sample_fate(0,repeat_chance);
            check(action==repeat && trial==repeat_chance && first.snapshot()==before && second.snapshot()==before
                  && game.snapshot()==before,"same-clone Fate sampling changed Match or disagreed on action/next chance RNG");
            ++stats.sampler_checks;
            check(action.actor==0 && action.token==0 && action.outcome.model>=287 && action.outcome.model<=296,"Fate sampler returned a noncanonical owner/token/outcome");
            const auto model=action.outcome.model;const auto column=static_cast<std::size_t>(model-287);
            if(model>=291 && model<=294) {
                check(action.outcome.recipient && find_player(before.state,*action.outcome.recipient)
                      && !find_player(before.state,*action.outcome.recipient)->dead,"targeted sample omitted a live recipient");
                const auto* target=find_player(before.state,*action.outcome.recipient);
                stats.target_self+=target->id==0;
                stats.target_ally+=target->id!=0 && !is_enemy(before.state.players.front(),*target);
                stats.target_enemy+=is_enemy(before.state.players.front(),*target);
            } else check(!action.outcome.recipient,"global sample has an inactive recipient");
            if(model==296) {
                check(action.outcome.guardians.size()==alive.size(),"guardian assignment count does not match alive input order");
                for(auto guardian:action.outcome.guardians)check(static_cast<unsigned>(guardian)>=1 && static_cast<unsigned>(guardian)<=10,"sampled guardian is none or unknown");
                stats.guardian_assignments+=action.outcome.guardians.size();
            } else check(action.outcome.guardians.empty(),"nonguardian result has inactive guardian assignments");
            if(model!=295)check(!action.shuffle,"nonshuffle result has an inactive manifest");
            auto chance_hash=std::uint64_t{14695981039346656037ULL};mix(chance_hash,i);mix(chance_hash,chance_before);mix(chance_hash,trial);stats.chance_checksum+=chance_hash;
            auto action_hash=std::uint64_t{14695981039346656037ULL};hash_action(action_hash,action);mix(action_hash,i);stats.sampled_action_checksum+=action_hash;
            // Invalid-token sampling must not consume the independently owned chance stream.
            refuse_sample(game,max_wire_integer,chance,stats);
            if (model==295) {
                check(action.shuffle.has_value(),"shuffle sample omitted its explicit post-consumption manifest");
                std::vector<ItemSource> expected;
                for(const auto& player:before.state.players)if(!player.dead)for(const auto& item:player.items)
                    if(player.id!=0 || item.token!=0)expected.push_back({player.id,item.token});
                auto actual=action.shuffle->order;
                const auto order=[](const ItemSource& a,const ItemSource& b){return a.owner<b.owner || (a.owner==b.owner && a.token<b.token);};
                std::sort(expected.begin(),expected.end(),order);std::sort(actual.begin(),actual.end(),order);
                check(actual==expected,"shuffle sample is not an exact source-qualified cover after consuming only owner0/token0");
                stats.shuffle_empty+=actual.empty();stats.shuffle_single+=actual.size()==1;
                for(const auto& ref:actual)stats.shuffle_other_token0+=ref.owner!=0 && ref.token==0;
                ++stats.shuffle_scenes; stats.shuffle_copies+=action.shuffle->order.size();
                std::uint64_t manifest=14695981039346656037ULL; mix(manifest,i);
                for (const auto& entry:action.shuffle->order) {
                    mix(manifest,entry.owner); mix(manifest,entry.token);
                    const auto* owner=find_player(before.state,entry.owner);
                    const auto item=std::find_if(owner->items.begin(),owner->items.end(),[&](const auto& copy) { return copy.token==entry.token; });
                    check(item!=owner->items.end(),"post207 manifest source is absent"); stats.shuffle_used_copies+=item->used;
                }
                stats.manifest_checksum+=manifest;
            }
            check(!game.legal(Command{0,CommandKind::play,{0},{}}) && !game.confusion_legal(Command{0,CommandKind::play,{0},{}}) && game.fate_legal(action),"normal207 play or explicit fate legality wrong");
            if (!before.state.players.front().confusion_count) {
                auto probe=seed+i; const auto candidate=game.sample_action(probe);
                check(!(candidate.kind==CommandKind::play && !candidate.items.empty() && candidate.items.front()==0),
                      "ordinary sampler proposed unresolved207 play");
                check(game.snapshot()==before,"normal policy changed the pre-Fate state");++stats.sampler_exclusion_checks;
            } else {
                std::uint64_t probe=seed+i;const auto prior=probe;bool rejected=false;
                try{(void)game.sample_action(probe);}catch(const MatchError&){rejected=true;}
                check(rejected && probe==prior && game.snapshot()==before,"confused main policy refusal changed state or policy RNG");++stats.confused_policy_refusals;
            }
            stats.used_input_copies+=used_models(before.state).size();
            game.fate_step(action);chance=trial; audit(game,restored); verify_proof(before,game.snapshot(),action);
            count_transition(before,game.snapshot(),true,model);
            check(used_models(before.state)==used_models(game.snapshot().state),"fate launch discarded or unlearned used miracles");
            stats.used_preserved_at_launch+=used_models(game.snapshot().state).size();
            stats.confusion_debits+=before.state.players.front().confusion_count>0;
            if (!game.snapshot().queue.empty() && game.snapshot().queue.front().environmental) {
                const auto& effect=game.snapshot().queue.front(); ++stats.initial_world_responses;
                auto expected_rng=seed+i;
                if (model==292 && !effect.forced_hit) check(bounded100(expected_rng)<75,"world pending exists after an independent miss");
                check(game.snapshot().rng==expected_rng,"world launch consumed selector RNG beyond existing percentage activation");
                ++stats.no_selector_world_checks;
            }
            inspect_frames(game,action,stats);
            const auto public_notice="\"last_fate\":{\"actor\":0,\"model\":207,\"phenomenon\":"+std::to_string(model)+"}";
            check(game.observe({}).find(public_notice)!=std::string::npos,"public fate notice includes the wrong actor/result");
            auto primitive=world_hash(game.snapshot()); mix(primitive,i); mix(primitive,model); stats.primitive_checksum+=primitive;
            std::uint64_t policy=(seed+i)^0x510e527fade682d1ULL;
            while (!game.done() && game.snapshot().phase!=MatchPhase::main) {
                const auto pending=game.snapshot();
                refuse_sample(game,0,chance,stats);
                if(pending.phase==MatchPhase::grant) {
                    const auto old_policy=policy;bool rejected=false;
                    try {(void)game.sample_action(policy);}catch(const MatchError&){rejected=true;}
                    check(rejected && policy==old_policy && game.snapshot()==pending,"grant pause did not atomically refuse normal policy");
                    game.grant_step({*pending.pending_grant,0,{},{}});++stats.grant_commands;
                    audit(game,restored);verify_proof(before,game.snapshot(),action);count_transition(pending,game.snapshot(),false,model);inspect_frames(game,action,stats);
                    continue;
                }
                const auto command=game.sample_action(policy);
                check(game.legal(command),"ordinary fate response/purchase sampler is illegal");
                const bool world=!pending.queue.empty() && pending.queue.front().environmental;
                std::uint64_t mp_cards=0; bool cancel=false;
                if (world) for (auto token:command.items) {
                    const auto& hand=find_player(pending.state,command.actor)->items;
                    const auto item=std::find_if(hand.begin(),hand.end(),[&](const auto& value) { return value.token==token; });
                    check(item!=hand.end(),"response token is not owned"); const auto ability=find_card(item->model)->ability;
                    mp_cards+=ability==Ability::counter_boost2x_mp; cancel|=item->model==190;
                    stats.environmental_counter_cards+=ability==Ability::counter_atk || ability==Ability::counter2x_atk
                        || ability==Ability::counter_curse || ability==Ability::counter_take_cp;
                }
                if (pending.phase==MatchPhase::response) { check(command.kind==CommandKind::defend,"response proposed non-defense"); ++stats.responses; }
                else { check(pending.phase==MatchPhase::purchase && (command.kind==CommandKind::accept || command.kind==CommandKind::decline),"unexpected fate pending phase"); ++stats.purchase_commands; }
                game.step(command); audit(game,restored); verify_proof(before,game.snapshot(),action);
                count_transition(pending,game.snapshot(),false,model);
                if (world) check(game.snapshot().counter_created-pending.counter_created<=mp_cards,"world fate generated a source-directed counter");
                stats.environmental_cancels+=cancel; inspect_frames(game,action,stats);
            }
            const auto& final=game.snapshot();
            check(final.fate_actions==1 && final.phenomenon_actions==0 && !final.guardian_actions && !final.guardian_payload_actions,
                  "owned fate entered an external activation counter");
            if (!game.done()) check(final.phase==MatchPhase::main && final.owner!=0 && final.state.gf==18,
                                    "resolved fate did not finish the ordinary GF/owner schedule");
            else check(final.state.gf==17,"terminal/cutoff fate advanced GF");
            ++stats.scenes; ++stats.coverage[column]; stats.decisions+=final.decisions; stats.fate_actions+=final.fate_actions;
            stats.terminated+=final.terminated; stats.truncated+=final.truncated; stats.dead_input_scenes+=dead_last;stats.include_fate_scenes+=config.include_fate;stats.external_grant_scenes+=config.external_grants;stats.cutoff_probes+=cutoff;
#define COUNT(name) stats.name+=final.name
            COUNT(chance_rolls); COUNT(chance_hits); COUNT(counter_created); COUNT(counter_started); COUNT(counter_rolls); COUNT(counter_hits);
            COUNT(death_bows); COUNT(death_created); COUNT(death_started); COUNT(death_rolls); COUNT(death_hits);
            COUNT(revivals); COUNT(redraws); COUNT(redrawn_items); COUNT(guardian_summons); COUNT(guardian_opportunities); COUNT(auto_guardian_actions);
#undef COUNT
            stats.chance_forced+=final.chance_forced_hits; stats.counter_forced+=final.counter_forced_hits; stats.death_forced+=final.death_forced_hits;
            stats.departure_rolls+=final.guardian_departure_rolls; stats.departures+=final.guardian_departures;
            stats.disease_ticks+=final.last_tick.has_value(); stats.gf_total+=final.state.gf;
            auto hash=world_hash(final); mix(hash,i); mix(hash,model); stats.checksum+=hash;
        }
    } catch (...) { errors[worker]=std::current_exception(); } });
    for (auto& thread:threads) thread.join(); for (const auto& error:errors) if (error) std::rethrow_exception(error);
    Stats total; for (const auto& stats:results) total.append(stats);
    unsigned covered=0; std::uint64_t model_scenes=0; for (auto n:total.coverage) { covered+=n>0; model_scenes+=n; }
    check(total.scenes==count && total.fate_actions==count && model_scenes==count
          && total.sampler_checks==count && total.decisions==count+total.responses+total.purchase_commands+total.grant_commands
          && total.used_input_copies==total.used_preserved_at_launch && total.no_selector_world_checks==total.initial_world_responses
          && total.chance_hits<=total.chance_rolls && total.counter_hits<=total.counter_rolls
          && total.death_hits<=total.death_rolls && total.environmental_cancels<=total.initial_world_responses
          && total.terminated+total.truncated<=count,"fate sampler benchmark accounting failed");
    if(count>=600) {
        check(covered==10 && total.target_self>0 && total.target_enemy>0,"local Fate fixture omitted an outcome/self/enemy recipient");
        if(players>=3)check(total.target_ally>0,"local Fate fixture omitted a distinct ally recipient");
    }
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset<<"\nscope=experimental:local-uniform-ten-phenomena-fate-sampler;independent-external-chance-rng;ordinary-owner-schedule;not-official-selector-or-distribution;world-selected-counter-hashes-plus-full-restore-audit"
             <<"\nfate_rule="<<fate_rule<<"\ndevil_rule="<<devil_rule<<"\napocalypse_rule="<<apocalypse_rule<<"\ngrant_rule="<<grant_rule<<"\ngrant_sampler_rule="<<grant_sampler_rule<<"\nfate_pool_rule="<<fate_pool_rule<<"\nfate_sampler_rule="<<fate_sampler_rule<<"\nconfusion_sampler_rule="<<confusion_sampler_rule<<"\nguardian_payload_sampler_rule="<<guardian_payload_sampler_rule << "\nguardian_payload_auto_rule=" << guardian_payload_auto_rule<<"\nscenes="<<total.scenes<<"\nplayers="<<players<<"\nseed="<<seed<<"\nworkers="<<workers<<"\nmodels_covered="<<covered;
#define OUTPUT(name) std::cout<<"\n" #name "="<<total.name
    OUTPUT(decisions);OUTPUT(grant_commands);OUTPUT(sampler_checks);OUTPUT(invalid_sampler_checks);OUTPUT(confused_policy_refusals);OUTPUT(target_self);OUTPUT(target_ally);OUTPUT(target_enemy);OUTPUT(guardian_assignments);
    OUTPUT(include_fate_scenes);OUTPUT(external_grant_scenes);OUTPUT(cutoff_probes);OUTPUT(shuffle_empty);OUTPUT(shuffle_single);OUTPUT(shuffle_other_token0);
    OUTPUT(chance_checksum);OUTPUT(sampled_action_checksum); OUTPUT(responses); OUTPUT(purchase_commands); OUTPUT(terminated); OUTPUT(truncated); OUTPUT(fate_actions); OUTPUT(confusion_debits);
    OUTPUT(shuffle_scenes); OUTPUT(shuffle_copies); OUTPUT(shuffle_used_copies); OUTPUT(used_input_copies); OUTPUT(used_preserved_at_launch);
    OUTPUT(dead_input_scenes); OUTPUT(sampler_exclusion_checks); OUTPUT(initial_world_responses); OUTPUT(no_selector_world_checks);
    OUTPUT(environmental_cancels); OUTPUT(environmental_counter_cards); OUTPUT(real_frame_observations); OUTPUT(source0_frame_observations);
    OUTPUT(chance_rolls); OUTPUT(chance_hits); OUTPUT(chance_forced); OUTPUT(counter_created); OUTPUT(counter_started); OUTPUT(counter_rolls); OUTPUT(counter_hits); OUTPUT(counter_forced);
    OUTPUT(death_bows); OUTPUT(death_created); OUTPUT(death_started); OUTPUT(death_rolls); OUTPUT(death_hits); OUTPUT(death_forced);
    OUTPUT(revivals); OUTPUT(redraws); OUTPUT(redrawn_items); OUTPUT(disease_ticks); OUTPUT(guardian_summons); OUTPUT(guardian_opportunities); OUTPUT(auto_guardian_actions);
    OUTPUT(departure_rolls); OUTPUT(departures); OUTPUT(gf_total); OUTPUT(primitive_checksum); OUTPUT(manifest_checksum); OUTPUT(checksum);
#undef OUTPUT
    for (std::size_t i=0;i<total.coverage.size();++i) std::cout<<"\nmodel"<<(287+i)<<"_scenes="<<total.coverage[i];
    std::cout<<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';
    return 0;
} catch (const std::exception& error) { std::cerr<<"fate sampler benchmark: "<<error.what()<<'\n'; return 1; } }
