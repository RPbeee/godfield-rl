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
    mix(hash,s.config.include_fate); mix(hash,s.config.external_grants); mix(hash,s.grant_resolutions); mix(hash,s.grant_artifacts); mix(hash,s.grant_devils);
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
    restored.restore(game.snapshot());check(restored.snapshot()==game.snapshot(),"sampled guardian payload checkpoint failed full restore");
}
bool domain(Guardian guardian,const Card& card) {
    return guardian==Guardian::moon?card.category==Category::miracles:
        card.category==Category::trade || card.category==Category::weapons || card.category==Category::armor || card.category==Category::sundries;
}
void action_hash(std::uint64_t& hash,const GuardianPayloadAction& action) {
    mix(hash,action.source);mix(hash,static_cast<unsigned>(action.guardian));mix(hash,action.models.size());
    for(auto model:action.models)mix(hash,model);mix(hash,action.target.has_value());if(action.target)mix(hash,*action.target);
}
struct Stats {
    std::uint64_t scenes=0,decisions=0,responses=0,purchase_commands=0,grant_commands=0,terminated=0,truncated=0;
    std::uint64_t sampler_checks=0,independence_checks=0,invalid_sampler_checks=0,earth=0,moon=0,nonowner_sources=0;
    std::uint64_t target_self=0,target_ally=0,target_enemy=0,earth_compounds=0,extras=0,duplicates=0,setter_columns=0,max_models=0;
    std::uint64_t high_cost_moon=0,zero_mp_sources=0,one_mp_sources=0,mana_plays=0,repeat_plays=0,random_hp_plays=0;
    std::uint64_t pending_inventory_checks=0,include_fate_scenes=0,external_grant_scenes=0,dead_input_scenes=0;
    std::uint64_t chance_rolls=0,chance_hits=0,chance_forced=0,counter_created=0,counter_started=0,death_bows=0,death_created=0;
    std::uint64_t revivals=0,redraws=0,redrawn_items=0,departure_rolls=0,departures=0,guardian_summons=0;
    std::uint64_t chance_checksum=0,sampled_action_checksum=0,primitive_checksum=0,checksum=0;
    std::array<std::uint64_t,catalog_count+1> bases{};
    void append(const Stats& other) {
#define ADD(n) n+=other.n
        ADD(scenes);ADD(decisions);ADD(responses);ADD(purchase_commands);ADD(grant_commands);ADD(terminated);ADD(truncated);
        ADD(sampler_checks);ADD(independence_checks);ADD(invalid_sampler_checks);ADD(earth);ADD(moon);ADD(nonowner_sources);
        ADD(target_self);ADD(target_ally);ADD(target_enemy);ADD(earth_compounds);ADD(extras);ADD(duplicates);ADD(setter_columns);
        ADD(high_cost_moon);ADD(zero_mp_sources);ADD(one_mp_sources);ADD(mana_plays);ADD(repeat_plays);ADD(random_hp_plays);
        ADD(pending_inventory_checks);ADD(include_fate_scenes);ADD(external_grant_scenes);ADD(dead_input_scenes);
        ADD(chance_rolls);ADD(chance_hits);ADD(chance_forced);ADD(counter_created);ADD(counter_started);ADD(death_bows);ADD(death_created);
        ADD(revivals);ADD(redraws);ADD(redrawn_items);ADD(departure_rolls);ADD(departures);ADD(guardian_summons);
        ADD(chance_checksum);ADD(sampled_action_checksum);ADD(primitive_checksum);ADD(checksum);
#undef ADD
        max_models=std::max(max_models,other.max_models);for(std::size_t i=0;i<bases.size();++i)bases[i]+=other.bases[i];
    }
};
void refuse_sample(Match& game,PlayerId source,std::uint64_t& chance,Stats& stats) {
    const auto before=game.snapshot();const auto old=chance;bool failed=false;
    try{(void)game.sample_guardian_payload(source,chance);}catch(const MatchError&){failed=true;}
    check(failed && chance==old && game.snapshot()==before,"invalid guardian payload sampling changed Match or chance RNG");++stats.invalid_sampler_checks;
}
void counts_unchanged(const MatchSnapshot& before,const MatchSnapshot& after) {
    for(const auto& p:before.state.players){const auto* now=find_player(after.state,p.id);auto expected=p.confusion_count;
        if(now->dead || after.terminated)expected=0;
        check(now->confusion_count==expected,"external guardian payload debited confusion or changed another live count");}
}
void proof(const MatchSnapshot& initial,const MatchSnapshot& now,const GuardianPayloadAction& action) {
    check(now.guardian_payload_actions==1 && !now.guardian_actions && !now.phenomenon_actions && !now.fate_actions && !now.devil_actions
          && !now.auto_guardian_actions && !now.guardian_opportunities && !now.last_tick && now.state.gf==17,
          "local payload invoked the ordinary owner schedule or an unrelated external selector");
    check(now.last_guardian_payload==action && !now.last_guardian,"launched general payload identity was replaced by a canonical action");
    if(now.batch)check(now.batch->guardian_payload_action==action && !now.batch->confusion_command
        && now.batch->guardian_payload_mp==find_player(initial.state,action.source)->mp
        && now.batch->initial_state==initial.state,"pending payload lost its source/guardian/MP launch anchor");
    for(const auto& frame:now.queue)check(!frame.environmental && find_player(now.state,frame.source) && find_player(now.state,frame.origin),
                                         "guardian/counter/death frame lost its real source, including player0");
    for(const auto& p:initial.state.players)if(p.dead)check(*find_player(now.state,p.id)==p,"payload changed an initially dead player's state");
}
}
int main(int argc,char** argv){try{
    if(argc>5)throw std::invalid_argument("usage: godfield_guardian_payload_sampler_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000,players=argc>2?number(argv[2]):2,seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if(count>1000000000 || players<2 || players>9 || workers>64 || seed>UINT64_MAX-(count-1))throw std::invalid_argument("guardian payload sampler benchmark bounds exceeded");workers=std::min(workers,count);
    std::size_t earth_bases=0,moon_bases=0,earth_extras=0;
    for(const auto& card:catalog()){
        earth_bases+=match_guardian_payload_supported(Guardian::earth,card.id);moon_bases+=match_guardian_payload_supported(Guardian::moon,card.id);
        earth_extras+=match_supported(card.id) && domain(Guardian::earth,card) && card.is_plus_atk;
    }
    check(earth_bases==120 && moon_bases==26 && earth_extras==26,"scoped Earth/Moon base or append descriptor changed");
    std::vector<Stats> results(workers);std::vector<std::exception_ptr> errors(workers);std::vector<std::jthread> threads;
    const auto start=std::chrono::steady_clock::now();
    for(std::uint64_t worker=0;worker<workers;++worker)threads.emplace_back([&,worker]{try{
        auto& stats=results[worker];
        constexpr std::array<CardId,18> cards{190,189,178,184,187,188,208,112,235,199,200,114,239,3,4,5,207,203};
        constexpr std::array<std::int64_t,3> mps{0,1,99};
        for(std::uint64_t i=worker;i<count;i+=workers){
            MatchConfig config;config.max_decisions=96;config.include_fate=(i/2)%2==0;config.external_grants=(i/6)%2==0;
            Match game(config),restored(config);SnapshotState state;state.gf=17;if(config.external_grants)state.tiebreak_gf=17;
            const bool dead_last=players==9 && i%17==0;const auto alive_count=players-static_cast<std::uint64_t>(dead_last);
            const auto source_seat=(i/6)%alive_count;const auto source=source_seat?100+7*source_seat:0;
            const auto guardian=i%2?Guardian::moon:Guardian::earth;
            for(std::uint64_t seat=0;seat<players;++seat){
                PlayerState p;p.id=seat?100+7*seat:0;p.name="G"+std::to_string(seat);p.alias="alias"+std::to_string(seat);p.user_id="user"+std::to_string(seat);
                p.team=i%3==0?(seat%2?Team::two:Team::one):Team::solo;
                p.hp=15+static_cast<std::int64_t>((seed+i+17*seat)%85);p.mp=99;p.cp=(seed+i+seat)%100;
                p.guardian=seat==source_seat?guardian:static_cast<Guardian>(1+(seed+i+seat)%10);
                if((i+seat)%4==0)p.curses.push_back(Curse::cold);
                if((i+seat)%3==0)p.curses.push_back(Curse::fog);
                if((i+seat)%2==0)p.curses.push_back(Curse::dream);
                if((i+seat)%5==0)p.curses.push_back(Curse::darkcloud);
                p.dead=dead_last && seat+1==players;if(p.dead)p.hp=0;p.confusion_count=p.dead?0:static_cast<std::int64_t>((i+seat)%3);
                auto size=6+(seed+i+seat)%13;if(i%19==0 && seat==source_seat)size=0;
                for(std::uint64_t slot=0;slot<size;++slot){ItemInstance item{slot,cards[slot],{},slot==8};
                    if(item.model==114 && (i+seat)%2==0)item.fake_model=125;p.items.push_back(item);}
                if(seat==source_seat)p.mp=mps[(i/2)%mps.size()];state.players.push_back(std::move(p));
            }
            game.load_scene(std::move(state),0,seed+i);audit(game,restored);const auto initial=game.snapshot();
            std::uint64_t chance=(seed+i)^0x9b05688c2b3e6c1fULL,trial=chance,repeat=chance;
            Match first(config),second(config);first.restore(initial);second.restore(initial);
            const auto action=first.sample_guardian_payload(source,trial),same=second.sample_guardian_payload(source,repeat);
            check(action==same && trial==repeat && first.snapshot()==initial && second.snapshot()==initial && game.snapshot()==initial,
                  "same-clone payload sampling changed a Match or differed in action/next chance RNG");++stats.sampler_checks;
            // Separate raw world: no old proof is rewritten, and the helper may not
            // consult source resources, physical copies, or a one-slot hand limit.
            auto world=initial.state;for(auto& p:world.players){p.items.clear();p.mp=p.mp?0:99;}
            auto minimal=config;minimal.hand_limit=1;minimal.initial_hand=0;Match independent(minimal);
            independent.load_scene(std::move(world),0,initial.rng);auto independent_rng=chance;
            check(independent.sample_guardian_payload(source,independent_rng)==action && independent_rng==trial,
                  "payload sampler depends on MP, physical inventory or configured hand limit");++stats.independence_checks;
            refuse_sample(game,max_wire_integer,chance,stats);
            check(action.source==source && action.guardian==guardian && action.target && !action.models.empty() && action.models.size()<=18
                  && match_guardian_payload_supported(guardian,action.models.front()) && game.guardian_payload_legal(action),"sampled payload shape or base is unsupported");
            const auto* from=find_player(initial.state,source);const auto* target=find_player(initial.state,*action.target);
            const bool hostile=attack_power(action.models)>0 || targets_enemy_by_default(action.models.front());
            check(target && !target->dead && is_enemy(*from,*target)==hostile,"payload target was classified relative to the ordinary owner instead of source");
            stats.target_self+=target->id==source;stats.target_ally+=target->id!=source && !is_enemy(*from,*target);stats.target_enemy+=is_enemy(*from,*target);
            std::vector<VisibleSelection> prefix;bool setter=false,duplicate=false;
            for(auto model:action.models){const auto* card=find_card(model);check(card && domain(guardian,*card) && match_supported(model)
                    && can_append_main(prefix,{model,false}),"sampled payload violates its strict domain or actual append predicate");
                duplicate|=std::any_of(prefix.begin(),prefix.end(),[&](const auto& item){return item.model==model;});
                if(!prefix.empty())setter|=card->ability==Ability::set_element;prefix.push_back({model,false});}
            if(guardian==Guardian::moon)check(action.models.size()==1,"Moon sampler silently added mixed-domain FREE/Aura/Wide");
            const auto base=action.models.front();const auto* base_card=find_card(base);++stats.bases[base];
            stats.earth+=guardian==Guardian::earth;stats.moon+=guardian==Guardian::moon;stats.nonowner_sources+=source!=0;
            stats.earth_compounds+=guardian==Guardian::earth && action.models.size()>1;stats.extras+=action.models.size()-1;
            stats.duplicates+=duplicate;stats.setter_columns+=setter;stats.max_models=std::max<std::uint64_t>(stats.max_models,action.models.size());
            stats.high_cost_moon+=guardian==Guardian::moon && base_card->cost>from->mp;stats.zero_mp_sources+=from->mp==0;stats.one_mp_sources+=from->mp==1;
            stats.mana_plays+=base_card->ability==Ability::atk_by2x_mp;stats.repeat_plays+=base_card->ability==Ability::attack_twice;stats.random_hp_plays+=base_card->ability==Ability::boost_hp_or_deal_damage;
            auto chance_hash=std::uint64_t{14695981039346656037ULL};mix(chance_hash,i);mix(chance_hash,chance);mix(chance_hash,trial);stats.chance_checksum+=chance_hash;
            auto sampled_hash=std::uint64_t{14695981039346656037ULL};mix(sampled_hash,i);action_hash(sampled_hash,action);stats.sampled_action_checksum+=sampled_hash;
            game.guardian_payload_step(action);chance=trial;audit(game,restored);proof(initial,game.snapshot(),action);counts_unchanged(initial,game.snapshot());
            if(!game.snapshot().queue.empty() && !game.snapshot().queue.front().is_counter && !game.snapshot().queue.front().is_death
                && game.snapshot().queue.front().models==action.models){
                const auto* launched=find_player(game.snapshot().state,source);
                check(launched->items==from->items && launched->mp==(base_card->ability==Ability::atk_by2x_mp?0:from->mp),
                      "pending ordinary payload consumed source inventory or paid a generic miracle fee");++stats.pending_inventory_checks;
                const auto expected=base_card->ability==Ability::atk_by2x_mp?attack_power(action.models,2*from->mp):attack_power(action.models);
                check(game.snapshot().queue.front().power==expected && game.snapshot().queue.front().guardian==guardian,
                      "payload lost its immutable launch MP/power/guardian identity");
            }
            auto primitive=world_hash(game.snapshot());mix(primitive,i);stats.primitive_checksum+=primitive;
            std::uint64_t policy=(seed+i)^0x510e527fade682d1ULL;std::uint64_t responses=0,purchases=0,grants=0;
            while(!game.done() && game.snapshot().phase!=MatchPhase::main){
                const auto before=game.snapshot();refuse_sample(game,source,chance,stats);
                if(before.phase==MatchPhase::grant){auto copy=policy;bool rejected=false;try{(void)game.sample_action(copy);}catch(const MatchError&){rejected=true;}
                    check(rejected && copy==policy && game.snapshot()==before,"grant pause mutated ordinary policy state");game.grant_step({*before.pending_grant,0,{},{}});++grants;
                }else{const auto command=game.sample_action(policy);check(game.legal(command),"sampled payload response is illegal");
                    if(before.phase==MatchPhase::response){check(command.kind==CommandKind::defend,"response policy selected a main operation");++responses;}
                    else{check(before.phase==MatchPhase::purchase,"unexpected payload pending phase");++purchases;}game.step(command);}
                audit(game,restored);proof(initial,game.snapshot(),action);counts_unchanged(before,game.snapshot());
            }
            const auto& final=game.snapshot();check(final.decisions==1+responses+purchases+grants && final.grant_resolutions==grants,"sampled payload decision/grant accounting failed");
            if(!game.done()){auto expected_owner=PlayerId{0};if(final.state.players.front().dead){
                    const auto next=std::find_if(final.state.players.begin()+1,final.state.players.end(),[](const auto& p){return !p.dead;});check(next!=final.state.players.end(),"nonterminal payload has no next live owner");expected_owner=next->id;}
                check(final.owner==expected_owner && final.actor==final.owner,"external payload failed to resume the old owner or next live seat");}
            ++stats.scenes;stats.decisions+=final.decisions;stats.responses+=responses;stats.purchase_commands+=purchases;stats.grant_commands+=grants;
            stats.terminated+=final.terminated;stats.truncated+=final.truncated;stats.include_fate_scenes+=config.include_fate;stats.external_grant_scenes+=config.external_grants;stats.dead_input_scenes+=dead_last;
            stats.chance_rolls+=final.chance_rolls;stats.chance_hits+=final.chance_hits;stats.chance_forced+=final.chance_forced_hits;
            stats.counter_created+=final.counter_created;stats.counter_started+=final.counter_started;stats.death_bows+=final.death_bows;stats.death_created+=final.death_created;
            stats.revivals+=final.revivals;stats.redraws+=final.redraws;stats.redrawn_items+=final.redrawn_items;
            stats.departure_rolls+=final.guardian_departure_rolls;stats.departures+=final.guardian_departures;stats.guardian_summons+=final.guardian_summons;
            auto hash=world_hash(final);mix(hash,i);stats.checksum+=hash;
        }
    }catch(...){errors[worker]=std::current_exception();}});
    for(auto& thread:threads)thread.join();for(const auto& error:errors)if(error)std::rethrow_exception(error);Stats total;for(const auto& stats:results)total.append(stats);
    std::uint64_t base_scenes=0,covered=0;for(auto n:total.bases){base_scenes+=n;covered+=n!=0;}
    check(total.scenes==count && total.earth+total.moon==count && base_scenes==count && total.sampler_checks==count && total.independence_checks==count
          && total.decisions==count+total.responses+total.purchase_commands+total.grant_commands && total.max_models<=18
          && total.chance_hits<=total.chance_rolls && total.terminated+total.truncated<=count,"guardian payload sampler aggregate accounting failed");
    if(count>=600){check(total.earth_compounds>0 && total.target_self>0 && total.target_enemy>0 && total.high_cost_moon>0 && total.nonowner_sources>0,
                        "payload sampler fixture omitted a meaningful branch");if(players>=3)check(total.target_ally>0,"payload sampler fixture omitted distinct allies");}
    if(count>=10000)check(covered==146 && total.mana_plays>0 && total.repeat_plays>0 && total.random_hp_plays>0,"large payload sampler fixture omitted a base or special weapon/body");
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset<<"\nscope=experimental:local-earth120-moon26-base-and-earth26-append-sampler;external-only-not-canonical-auto-or-official-selector;independent-chance-action-and-world-selected-counter-hashes-plus-full-restore-audit"
             <<"\nguardian_payload_sampler_rule="<<guardian_payload_sampler_rule<<"\nguardian_payload_rule="<<guardian_payload_rule
             <<"\nscenes="<<total.scenes<<"\nplayers="<<players<<"\nseed="<<seed<<"\nworkers="<<workers<<"\nmax_decisions=96\nmodels_covered="<<covered;
#define OUTPUT(n) std::cout<<"\n" #n "="<<total.n
    OUTPUT(decisions);OUTPUT(responses);OUTPUT(purchase_commands);OUTPUT(grant_commands);OUTPUT(terminated);OUTPUT(truncated);OUTPUT(sampler_checks);OUTPUT(independence_checks);OUTPUT(invalid_sampler_checks);
    OUTPUT(earth);OUTPUT(moon);OUTPUT(nonowner_sources);OUTPUT(target_self);OUTPUT(target_ally);OUTPUT(target_enemy);OUTPUT(earth_compounds);OUTPUT(extras);OUTPUT(duplicates);OUTPUT(setter_columns);OUTPUT(max_models);
    OUTPUT(high_cost_moon);OUTPUT(zero_mp_sources);OUTPUT(one_mp_sources);OUTPUT(mana_plays);OUTPUT(repeat_plays);OUTPUT(random_hp_plays);OUTPUT(pending_inventory_checks);OUTPUT(include_fate_scenes);OUTPUT(external_grant_scenes);OUTPUT(dead_input_scenes);
    OUTPUT(chance_rolls);OUTPUT(chance_hits);OUTPUT(chance_forced);OUTPUT(counter_created);OUTPUT(counter_started);OUTPUT(death_bows);OUTPUT(death_created);OUTPUT(revivals);OUTPUT(redraws);OUTPUT(redrawn_items);OUTPUT(departure_rolls);OUTPUT(departures);OUTPUT(guardian_summons);
    OUTPUT(chance_checksum);OUTPUT(sampled_action_checksum);OUTPUT(primitive_checksum);OUTPUT(checksum);
#undef OUTPUT
    std::cout<<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<"guardian payload sampler benchmark: "<<error.what()<<'\n';return 1;}}
