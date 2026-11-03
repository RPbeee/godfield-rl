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
// hash: Ordinary pending journals are checked by full restore.
std::uint64_t world_hash(const MatchSnapshot& s) {
    std::uint64_t hash=14695981039346656037ULL;
    mix(hash,s.version); mix(hash,s.state.gf); mix(hash,s.state.update_count); mix(hash,s.state.is_over);
    mix(hash,s.state.tiebreak_gf.has_value()); if(s.state.tiebreak_gf) mix(hash,*s.state.tiebreak_gf);
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
    mix(hash,s.config.local_guardian_payloads);mix(hash,s.config.include_fate); mix(hash,s.config.external_grants); mix(hash,s.grant_resolutions); mix(hash,s.grant_artifacts); mix(hash,s.grant_devils);
    mix(hash,s.terminated); mix(hash,s.truncated); mix(hash,s.phenomenon_actions); mix(hash,s.fate_actions); mix(hash,s.devil_actions);
    mix(hash,s.chance_rolls); mix(hash,s.chance_hits); mix(hash,s.chance_forced_hits);
    mix(hash,s.counter_created); mix(hash,s.counter_started); mix(hash,s.counter_rolls); mix(hash,s.counter_hits); mix(hash,s.counter_forced_hits);
    mix(hash,s.death_bows); mix(hash,s.death_created); mix(hash,s.death_started); mix(hash,s.death_rolls); mix(hash,s.death_hits); mix(hash,s.death_forced_hits);
    mix(hash,s.revivals); mix(hash,s.redraws); mix(hash,s.redrawn_items); mix(hash,s.dream_rolls); mix(hash,s.dream_masks);
    mix(hash,s.guardian_summons); mix(hash,s.guardian_opportunities); mix(hash,s.auto_guardian_actions);
    mix(hash,s.guardian_departure_rolls); mix(hash,s.guardian_departures);
    return hash;
}




void audit(const Match& game,Match& restored) {
    restored.restore(game.snapshot());check(restored.snapshot()==game.snapshot(),"automatic payload checkpoint failed full restore");
}
void action_hash(std::uint64_t& hash,const GuardianPayloadAction& action) {
    mix(hash,action.source);mix(hash,static_cast<unsigned>(action.guardian));mix(hash,action.models.size());
    for(auto model:action.models)mix(hash,model);mix(hash,action.target.has_value());if(action.target)mix(hash,*action.target);
}
void all_actions_hash(std::uint64_t& hash,const MatchSnapshot& s) {
    mix(hash,s.last_auto_guardians.size());for(const auto& action:s.last_auto_guardians){mix(hash,action.source);mix(hash,action.model);mix(hash,action.target.has_value());if(action.target)mix(hash,*action.target);}
    mix(hash,s.last_auto_guardian_payloads.size());for(const auto& action:s.last_auto_guardian_payloads)action_hash(hash,action);
}
void validate_logs(const MatchSnapshot& s) {
    check(s.auto_guardian_actions==s.last_auto_guardians.size()+s.last_auto_guardian_payloads.size()
          && !s.guardian_actions && !s.guardian_payload_actions && !s.phenomenon_actions && !s.fate_actions && !s.devil_actions,
          "automatic payload was counted as an external action or omitted from the total launch log");
    for(const auto& action:s.last_auto_guardian_payloads){
        check(action.target && !action.models.empty() && action.models.size()<=18
              && match_guardian_payload_supported(action.guardian,action.models.front()),"automatic payload log has an unsupported launch shape");
        std::vector<VisibleSelection> prefix;
        for(auto id:action.models){const auto& card=*find_card(id);
            const bool domain=action.guardian==Guardian::moon?card.category==Category::miracles:
                card.category==Category::weapons || card.category==Category::armor || card.category==Category::sundries || card.category==Category::trade;
            check(domain && match_supported(id) && can_append_main(prefix,{id,false}),"automatic payload broke its local strict-domain composition");prefix.push_back({id,false});}
        if(action.guardian==Guardian::moon)check(action.models.size()==1,"automatic Moon silently gained mixed-domain addons");
    }
}
void counts_unchanged(const MatchSnapshot& before,const MatchSnapshot& after){
    for(const auto& p:before.state.players){const auto* now=find_player(after.state,p.id);
        const auto expected=now->dead || after.terminated?0:p.confusion_count;
        check(now->confusion_count==expected,"internal guardian opportunity consumed a source's confusion count");}
}
struct Stats {
    std::uint64_t scenes=0,decisions=0,responses=0,purchase_commands=0,grant_commands=0,terminated=0,truncated=0;
    std::uint64_t payloads=0,canonical=0,earth=0,moon=0,compounds=0,extras=0,duplicate_columns=0,setter_columns=0;
    std::uint64_t external_purity_checks=0,pending_launches=0,zero_mp_moon_pending=0,high_cost_moon_pending=0,no_fee_checks=0,source_inventory_checks=0;
    std::uint64_t mana_plays=0,repeat_plays=0,random_hp_plays=0,second_hit_contexts=0,private_context_checks=0;
    std::uint64_t tick_records=0,tick_stability_checks=0,owner_tick_deaths=0,gift_accounting_checks=0;
    std::uint64_t false_scenes=0,false_decisions=0,false_responses=0,false_grants=0,false_auto_actions=0,false_earth_moon_fallbacks=0,no_earth_moon_controls=0;
    std::uint64_t chance_rolls=0,chance_hits=0,chance_forced=0,counter_created=0,counter_started=0,death_bows=0,death_created=0;
    std::uint64_t revivals=0,redraws=0,redrawn_items=0,guardian_opportunities=0,guardian_summons=0,departure_rolls=0,departures=0;
    std::uint64_t primitive_checksum=0,launched_checksum=0,checksum=0,default_checksum=0;
    std::array<std::uint64_t,catalog_count+1> bases{};
    void append(const Stats& o){
#define ADD(n) n+=o.n
        ADD(scenes);ADD(decisions);ADD(responses);ADD(purchase_commands);ADD(grant_commands);ADD(terminated);ADD(truncated);
        ADD(payloads);ADD(canonical);ADD(earth);ADD(moon);ADD(compounds);ADD(extras);ADD(duplicate_columns);ADD(setter_columns);
        ADD(external_purity_checks);ADD(pending_launches);ADD(zero_mp_moon_pending);ADD(high_cost_moon_pending);ADD(no_fee_checks);ADD(source_inventory_checks);
        ADD(mana_plays);ADD(repeat_plays);ADD(random_hp_plays);ADD(second_hit_contexts);ADD(private_context_checks);
        ADD(tick_records);ADD(tick_stability_checks);ADD(owner_tick_deaths);ADD(gift_accounting_checks);
        ADD(false_scenes);ADD(false_decisions);ADD(false_responses);ADD(false_grants);ADD(false_auto_actions);ADD(false_earth_moon_fallbacks);ADD(no_earth_moon_controls);
        ADD(chance_rolls);ADD(chance_hits);ADD(chance_forced);ADD(counter_created);ADD(counter_started);ADD(death_bows);ADD(death_created);
        ADD(revivals);ADD(redraws);ADD(redrawn_items);ADD(guardian_opportunities);ADD(guardian_summons);ADD(departure_rolls);ADD(departures);
        ADD(primitive_checksum);ADD(launched_checksum);ADD(checksum);ADD(default_checksum);
#undef ADD
        for(std::size_t i=0;i<bases.size();++i)bases[i]+=o.bases[i];
    }
};
void inspect_context(const MatchSnapshot& initial,const MatchSnapshot& s,std::vector<bool>& seen,Stats& stats,bool before_first_response){
    if(!s.batch || !s.batch->automatic_payload)return;
    const auto& launch=*s.batch->automatic_payload;const auto& action=launch.action;const auto& base=*find_card(action.models.front());
    check(!s.last_auto_guardian_payloads.empty() && s.last_auto_guardian_payloads.back()==action
          && launch.launch_mp>=0 && launch.launch_mp<=99 && launch.cursor>=1
          && launch.cursor<=(base.ability==Ability::attack_twice?2:1)
          && (base.ability==Ability::boost_hp_or_deal_damage?(launch.random_hp_delta==10 || launch.random_hp_delta==-10):launch.random_hp_delta==0),
          "automatic payload context lost its launched action, MP, sign or remaining-hit cursor");
    ++stats.private_context_checks;stats.second_hit_contexts+=launch.cursor==2;
    const auto slot=s.last_auto_guardian_payloads.size()-1;if(seen.size()<=slot)seen.resize(slot+1);
    if(!seen[slot]){
        seen[slot]=true;++stats.pending_launches;stats.zero_mp_moon_pending+=action.guardian==Guardian::moon && launch.launch_mp==0;
        stats.high_cost_moon_pending+=action.guardian==Guardian::moon && base.cost>launch.launch_mp;
        if(!s.queue.empty()){
            const auto& frame=s.queue.front();
            if(!frame.is_counter && !frame.is_death && frame.models==action.models && frame.origin==action.source && frame.redirects.empty()){
                const auto* source=find_player(s.state,action.source);
                check(source->mp==(base.ability==Ability::atk_by2x_mp?0:launch.launch_mp),"automatic payload paid an ordinary miracle fee");++stats.no_fee_checks;
                if(before_first_response){check(source->items==find_player(initial.state,action.source)->items,"automatic payload consumed source copies at launch");++stats.source_inventory_checks;}
            }
        }
    }
    if(!s.queue.empty()){
        const auto& frame=s.queue.front();
        if(!frame.is_counter && !frame.is_death && frame.origin==action.source && frame.models==action.models){
            const auto power=base.ability==Ability::atk_by2x_mp?attack_power(action.models,2*launch.launch_mp):attack_power(action.models);
            check(frame.power==power && frame.guardian==action.guardian,"automatic original/repeated payload recalculated launch MP, power or guardian identity");
        }
    }
}
struct Completed {MatchSnapshot state;std::uint64_t responses=0,purchases=0,grants=0;};
Completed legacy_run(MatchConfig config,const SnapshotState& state,std::uint64_t seed,std::uint64_t policy){
    config.local_guardian_payloads=false;Match game(config),restored(config);game.load_scene(state,0,seed);audit(game,restored);
    check(game.legal({0,CommandKind::pray,{},{}}),"legacy control prayer is illegal");game.step({0,CommandKind::pray,{},{}});audit(game,restored);
    Completed result;
    while(!game.done() && game.snapshot().phase!=MatchPhase::main){
        if(game.snapshot().phase==MatchPhase::grant){game.grant_step({*game.snapshot().pending_grant,0,{},{}});++result.grants;}
        else{const auto before=game.snapshot().phase;const auto action=game.sample_action(policy);check(game.legal(action),"legacy control follow-up is illegal");game.step(action);if(before==MatchPhase::response)++result.responses;else{check(before==MatchPhase::purchase,"unexpected legacy control phase");++result.purchases;}}
        audit(game,restored);
    }
    result.state=game.snapshot();check(result.state.last_auto_guardian_payloads.empty() && result.state.auto_guardian_actions==result.state.last_auto_guardians.size()
        && result.state.decisions==1+result.responses+result.purchases+result.grants,"defaultfalse control changed canonical-only accounting");
    for(const auto& action:result.state.last_auto_guardians){if(find_card(action.model)->guardian==Guardian::earth)check(action.model==285,"default Earth lost canonical285");
        if(find_card(action.model)->guardian==Guardian::moon)check(action.model==286,"default Moon lost canonical286");}
    return result;
}
}
int main(int argc,char** argv){try{
    if(argc>5)throw std::invalid_argument("usage: godfield_guardian_payload_auto_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000,players=argc>2?number(argv[2]):2,seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if(count>1000000000 || players<2 || players>9 || workers>64 || seed>UINT64_MAX-(count-1))throw std::invalid_argument("guardian payload auto benchmark bounds exceeded");workers=std::min(workers,count);
    std::vector<Stats> results(workers);std::vector<std::exception_ptr> errors(workers);std::vector<std::jthread> threads;
    const auto start=std::chrono::steady_clock::now();
    for(std::uint64_t worker=0;worker<workers;++worker)threads.emplace_back([&,worker]{try{
        auto& stats=results[worker];
        constexpr std::array<CardId,18> own_cards{190,189,178,184,187,188,208,235,199,200,114,239,3,4,5,207,203,143};
        constexpr std::array<std::int64_t,3> mps{0,1,99};
        for(std::uint64_t i=worker;i<count;i+=workers){
            MatchConfig config;config.max_decisions=128;config.local_guardian_payloads=true;config.include_fate=(i/2)%2==0;config.external_grants=(i/6)%2==0;
            SnapshotState state;state.gf=17;if(config.external_grants)state.tiebreak_gf=17;
            const bool control=i%5==0,dead_last=players==9 && i%17==0;
            for(std::uint64_t seat=0;seat<players;++seat){
                PlayerState p;p.id=seat?100+7*seat:0;p.name="A"+std::to_string(seat);p.alias="alias"+std::to_string(seat);p.user_id="user"+std::to_string(seat);
                p.team=i%3==0?(seat%2?Team::two:Team::one):Team::solo;
                p.hp=seat==0?(i%19==0?1:99):15+static_cast<std::int64_t>((seed+i+seat*17)%85);
                p.mp=seat?mps[(i/2+seat)%mps.size()]:99;p.cp=(seed+i+seat)%100;
                p.guardian=control || seat==0 || (i+seat)%4==0?static_cast<Guardian>(1+(i+seat)%8):((i+seat)%2?Guardian::earth:Guardian::moon);
                // Seat1 is always an enemy of owner0, with both local payload types across scenes.
                if(seat==1 && !control)p.guardian=(i/5)%2?Guardian::moon:Guardian::earth;
                if((i+seat)%4==0)p.curses.push_back(Curse::cold);
                if((i+seat)%3==0)p.curses.push_back(Curse::fog);
                if((i+seat)%2==0)p.curses.push_back(Curse::dream);
                if((i+seat)%7==0)p.curses.push_back(Curse::darkcloud);
                p.dead=dead_last && seat+1==players;if(p.dead)p.hp=0;p.confusion_count=seat && !p.dead?static_cast<std::int64_t>((i+seat)%3):0;
                const auto size=8+(seed+i+seat)%11;
                for(std::uint64_t slot=0;slot<size;++slot){auto id=own_cards[slot];if(seat && slot==9)id=112;
                    ItemInstance item{slot,id,{},id==235};if(id==114 && (i+seat)%2==0)item.fake_model=125;p.items.push_back(item);}
                state.players.push_back(std::move(p));
            }
            const auto initial_state=state;const auto policy_seed=(seed+i)^0x510e527fade682d1ULL;
            const auto old=legacy_run(config,initial_state,seed+i,policy_seed);
            ++stats.false_scenes;stats.false_decisions+=old.state.decisions;stats.false_responses+=old.responses;stats.false_grants+=old.grants;stats.false_auto_actions+=old.state.auto_guardian_actions;
            for(const auto& action:old.state.last_auto_guardians)stats.false_earth_moon_fallbacks+=find_card(action.model)->guardian==Guardian::earth || find_card(action.model)->guardian==Guardian::moon;
            auto old_hash=world_hash(old.state);all_actions_hash(old_hash,old.state);mix(old_hash,i);stats.default_checksum+=old_hash;
            Match game(config),twin(config),restored(config);game.load_scene(state,0,seed+i);twin.load_scene(std::move(state),0,seed+i);
            const auto initial=game.snapshot();check(twin.snapshot()==initial,"true clone initial worlds differ");audit(game,restored);
            if(!control){std::uint64_t outside=(seed+i)^0xa54ff53a5f1d36f1ULL;
                const auto sampled=game.sample_guardian_payload(initial.state.players[1].id,outside);
                check(game.guardian_payload_legal(sampled) && game.snapshot()==initial,"external sampler mutated game RNG or started the opt-in automatic stage");++stats.external_purity_checks;}
            check(game.legal({0,CommandKind::pray,{},{}}),"ordinary auto-stage prayer is illegal");game.step({0,CommandKind::pray,{},{}});twin.step({0,CommandKind::pray,{},{}});
            check(game.snapshot()==twin.snapshot(),"true clone prayer stages differ");audit(game,restored);validate_logs(game.snapshot());counts_unchanged(initial,game.snapshot());
            const auto tick=game.snapshot().last_tick;stats.tick_records+=tick.has_value();
            if(game.snapshot().state.players.front().dead){check(game.snapshot().auto_guardian_actions==0 && game.snapshot().guardian_opportunities==0,"auto stage ran after fatal owner tick");++stats.owner_tick_deaths;}
            std::vector<bool> pending_seen;inspect_context(initial,game.snapshot(),pending_seen,stats,true);
            auto primitive=world_hash(game.snapshot());all_actions_hash(primitive,game.snapshot());mix(primitive,i);stats.primitive_checksum+=primitive;
            std::uint64_t policy=policy_seed,twin_policy=policy_seed;std::uint64_t responses=0,purchases=0,grants=0;
            while(!game.done() && game.snapshot().phase!=MatchPhase::main){
                const auto before=game.snapshot();check(before==twin.snapshot(),"true clone pending worlds differ");
                if(before.phase==MatchPhase::grant){
                    auto rejected_rng=policy;bool rejected=false;try{(void)game.sample_action(rejected_rng);}catch(const MatchError&){rejected=true;}
                    check(rejected && rejected_rng==policy && game.snapshot()==before,"grant pause changed normal policy or state");
                    const GrantAction action{*before.pending_grant,0,{},{}};game.grant_step(action);twin.grant_step(action);++grants;
                }else{
                    const auto phase=before.phase;const auto action=game.sample_action(policy),same=twin.sample_action(twin_policy);
                    check(action==same && policy==twin_policy && game.legal(action),"true clone policies differ");game.step(action);twin.step(same);
                    if(phase==MatchPhase::response)++responses;else{check(phase==MatchPhase::purchase,"unexpected automatic payload phase");++purchases;}
                }
                check(game.snapshot()==twin.snapshot(),"true clone accepted-step worlds differ");audit(game,restored);validate_logs(game.snapshot());counts_unchanged(before,game.snapshot());
                check(game.snapshot().last_tick==tick,"auto payload/response/grant repeated or rewrote the owner's historical tick");++stats.tick_stability_checks;
                if(before.state.players.front().dead)check(game.snapshot().last_auto_guardian_payloads.size()==before.last_auto_guardian_payloads.size(),"new auto payload launched after owner death");
                inspect_context(initial,game.snapshot(),pending_seen,stats,false);
            }
            const auto& final=game.snapshot();check(final.decisions==1+responses+purchases+grants && final.grant_resolutions==grants,
                                                   "automatic payload consumed an external decision or lost a follow-up command");
            if(!game.done()){
                const auto next=std::find_if(final.state.players.begin()+1,final.state.players.end(),[](const auto& p){return !p.dead;});
                check(next!=final.state.players.end() && final.owner==next->id && final.actor==final.owner && final.state.gf==18,"ordinary action did not advance exactly once after all auto/gift work");
            }else check(final.state.gf==17,"terminal/cutoff auto action advanced GF");
            if(control){auto normalized=final;normalized.config.local_guardian_payloads=false;check(normalized==old.state,"other8-only true mode differs beyond its config flag");++stats.no_earth_moon_controls;}
            if(!game.done() && responses==0 && purchases==0 && final.redraws==0
                && std::all_of(final.state.players.begin(),final.state.players.end(),[](const auto& p){return !p.dead;})
                && std::none_of(final.last_auto_guardians.begin(),final.last_auto_guardians.end(),[](const auto& action){return find_card(action.model)->ability==Ability::add_item;})){
                const auto ordinary=initial.state.players.front().items.size()<config.hand_limit?1u:0u;
                check(final.next_token-initial.next_token==ordinary+final.revivals,"virtual auto payload added an operation gift/retention copy");++stats.gift_accounting_checks;
            }
            std::uint64_t launched=14695981039346656037ULL;mix(launched,i);
            for(const auto& action:final.last_auto_guardian_payloads){const auto base=action.models.front();const auto ability=find_card(base)->ability;
                ++stats.bases[base];++stats.payloads;stats.earth+=action.guardian==Guardian::earth;stats.moon+=action.guardian==Guardian::moon;
                stats.compounds+=action.models.size()>1;stats.extras+=action.models.size()-1;bool duplicate=false,setter=false;std::vector<CardId> seen;
                for(auto id:action.models){duplicate|=std::find(seen.begin(),seen.end(),id)!=seen.end();seen.push_back(id);setter|=find_card(id)->ability==Ability::set_element;}
                stats.duplicate_columns+=duplicate;stats.setter_columns+=setter;stats.mana_plays+=ability==Ability::atk_by2x_mp;stats.repeat_plays+=ability==Ability::attack_twice;stats.random_hp_plays+=ability==Ability::boost_hp_or_deal_damage;action_hash(launched,action);
            }
            stats.launched_checksum+=launched;stats.canonical+=final.last_auto_guardians.size();
            ++stats.scenes;stats.decisions+=final.decisions;stats.responses+=responses;stats.purchase_commands+=purchases;stats.grant_commands+=grants;stats.terminated+=final.terminated;stats.truncated+=final.truncated;
            stats.chance_rolls+=final.chance_rolls;stats.chance_hits+=final.chance_hits;stats.chance_forced+=final.chance_forced_hits;stats.counter_created+=final.counter_created;stats.counter_started+=final.counter_started;
            stats.death_bows+=final.death_bows;stats.death_created+=final.death_created;stats.revivals+=final.revivals;stats.redraws+=final.redraws;stats.redrawn_items+=final.redrawn_items;
            stats.guardian_opportunities+=final.guardian_opportunities;stats.guardian_summons+=final.guardian_summons;stats.departure_rolls+=final.guardian_departure_rolls;stats.departures+=final.guardian_departures;
            auto hash=world_hash(final);all_actions_hash(hash,final);mix(hash,i);stats.checksum+=hash;
        }
    }catch(...){errors[worker]=std::current_exception();}});
    for(auto& thread:threads)thread.join();for(const auto& error:errors)if(error)std::rethrow_exception(error);Stats total;for(const auto& stats:results)total.append(stats);
    std::uint64_t covered=0,bases=0;for(auto n:total.bases){covered+=n!=0;bases+=n;}
    check(total.scenes==count && total.false_scenes==count && total.payloads==total.earth+total.moon && bases==total.payloads
          && total.decisions==count+total.responses+total.purchase_commands+total.grant_commands && total.false_decisions>=count
          && total.chance_hits<=total.chance_rolls && total.terminated+total.truncated<=count,"automatic payload aggregate accounting failed");
    if(count>=600)check(total.payloads>0 && total.earth>0 && total.moon>0 && total.compounds>0 && total.zero_mp_moon_pending>0
                       && total.no_earth_moon_controls>0,"auto payload fixture omitted a stable meaningful branch");
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset<<"\nscope=experimental:ordinary-prayer-through-opt-in-local-earth-moon-auto-stage;other8-only-true-false-parity-and-default-canonical-controls;not-official-general-selector;world-selected-counter-and-launch-log-hashes-plus-full-restore-audit"
             <<"\nguardian_payload_auto_rule="<<guardian_payload_auto_rule<<"\nguardian_payload_sampler_rule="<<guardian_payload_sampler_rule<<"\nguardian_rule="<<guardian_rule
             <<"\nscenes="<<total.scenes<<"\nplayers="<<players<<"\nseed="<<seed<<"\nworkers="<<workers<<"\nmax_decisions=128\nmodels_covered="<<covered;
#define OUTPUT(n) std::cout<<"\n" #n "="<<total.n
    OUTPUT(decisions);OUTPUT(responses);OUTPUT(purchase_commands);OUTPUT(grant_commands);OUTPUT(terminated);OUTPUT(truncated);OUTPUT(payloads);OUTPUT(canonical);OUTPUT(earth);OUTPUT(moon);OUTPUT(compounds);OUTPUT(extras);OUTPUT(duplicate_columns);OUTPUT(setter_columns);
    OUTPUT(external_purity_checks);OUTPUT(pending_launches);OUTPUT(zero_mp_moon_pending);OUTPUT(high_cost_moon_pending);OUTPUT(no_fee_checks);OUTPUT(source_inventory_checks);OUTPUT(mana_plays);OUTPUT(repeat_plays);OUTPUT(random_hp_plays);OUTPUT(second_hit_contexts);OUTPUT(private_context_checks);
    OUTPUT(tick_records);OUTPUT(tick_stability_checks);OUTPUT(owner_tick_deaths);OUTPUT(gift_accounting_checks);OUTPUT(false_scenes);OUTPUT(false_decisions);OUTPUT(false_responses);OUTPUT(false_grants);OUTPUT(false_auto_actions);OUTPUT(false_earth_moon_fallbacks);OUTPUT(no_earth_moon_controls);
    OUTPUT(chance_rolls);OUTPUT(chance_hits);OUTPUT(chance_forced);OUTPUT(counter_created);OUTPUT(counter_started);OUTPUT(death_bows);OUTPUT(death_created);OUTPUT(revivals);OUTPUT(redraws);OUTPUT(redrawn_items);OUTPUT(guardian_opportunities);OUTPUT(guardian_summons);OUTPUT(departure_rolls);OUTPUT(departures);
    OUTPUT(primitive_checksum);OUTPUT(launched_checksum);OUTPUT(default_checksum);OUTPUT(checksum);
#undef OUTPUT
    std::cout<<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<"guardian payload auto benchmark: "<<error.what()<<'\n';return 1;}}
