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
    mix(hash,s.config.external_grants); mix(hash,s.grant_resolutions); mix(hash,s.grant_artifacts); mix(hash,s.grant_devils);
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
    restored.restore(game.snapshot());
    check(restored.snapshot()==game.snapshot(),"grant checkpoint failed full snapshot roundtrip");
}
bool has(const PlayerState& player,Curse curse) {
    return std::find(player.curses.begin(),player.curses.end(),curse)!=player.curses.end();
}
std::string event_json(const DevilEvent& event) {
    std::string out="{\"target\":"+std::to_string(event.target)+",\"devil\":";
    out+=event.model?std::to_string(event.model):"null"; out+=",\"removed_models\":[";
    for(std::size_t j=0;j<event.removed_models.size();++j) { if(j)out+=',';out+=std::to_string(event.removed_models[j]); }
    return out+"]}";
}
void verify_notice(const Match& game) {
    const auto& s=game.snapshot(); const auto json=game.observe({});
    const auto pending=s.pending_grant?"{\"target\":"+std::to_string(*s.pending_grant)+"}":"null";
    check(json.find("\"pending_grant\":"+pending+",")!=std::string::npos,"pending grant leaked or changed its target-only shape");
    const auto notice=s.last_grant?event_json(*s.last_grant):"null";
    check(json.find("\"last_grant\":"+notice+",")!=std::string::npos,"last grant leaked normal model/resource or changed removed-model order");
}
void verify_origin(const MatchSnapshot& s,const MatchSnapshot& initial,std::size_t origin) {
    check(s.phenomenon_actions==0 && !s.guardian_actions && !s.guardian_payload_actions,
          "reserved grant invoked an unrelated external selector");
    check(s.fate_actions==(origin==3) && s.devil_actions==(origin==4),"grant choices changed the original Fate/Devil activation count");
    if(origin==3) check(s.last_fate && s.last_fate_event && s.last_fate->anchor.initial_state==initial.state
                       && s.last_fate->action.actor==0 && s.last_fate->action.token==0
                       && s.last_fate_event->actor==0 && s.last_fate_event->phenomenon==288,
                       "grant processing lost the original owned207 action record");
    if(origin==4) check(s.last_devil && s.last_devil_event && s.last_devil->anchor.initial_state==initial.state
                       && s.last_devil->action.target==0 && s.last_devil->action.model==240
                       && s.last_devil_event->target==0 && s.last_devil_event->model==240,
                       "grant processing overwrote the original external Devil record");
    if(s.grant_proof) {
        const auto& proof=*s.grant_proof;
        check(proof.initial_state==initial.state && proof.rng==initial.rng && proof.decisions==0
              && proof.fate_action.has_value()==(origin==3) && proof.devil_action.has_value()==(origin==4),
              "grant proof lost its original pre-action anchor or source identity");
        check(proof.grants.size()==s.grant_resolutions,"grant journal omitted or duplicated an accepted outcome");
        std::size_t prefix=0;
        for(const auto& entry:proof.grants) {
            check(entry.responses_before>=prefix && entry.responses_before<=proof.responses.size(),"grant response-prefix ordering is invalid");
            prefix=entry.responses_before;
        }
    } else check(!s.grant_resolutions,"accepted grant has no original-action proof");
    if(s.phase==MatchPhase::response) {
        check(s.batch.has_value() && !s.queue.empty(),"grant follow-up response lost its batch/frame");
        for(const auto& frame:s.queue) if(!frame.environmental) {
            check(find_player(s.state,frame.source)!=nullptr && find_player(s.state,frame.origin)!=nullptr,
                  "derived death/counter source was treated as a missing/sentinel player");
        }
    }
}
void confusion_transition(const MatchSnapshot& before,const MatchSnapshot& after,bool launch_debit) {
    for(const auto& p:before.state.players) {
        const auto* q=find_player(after.state,p.id); auto expected=p.confusion_count;
        if(launch_debit && p.id==0 && expected>0) --expected;
        if(q->dead || after.terminated) expected=0;
        check(q->confusion_count==expected,"grant/retry/response repeated a confusion debit or changed another player's count");
    }
}
struct Stats {
    std::uint64_t scenes=0,decisions=0,responses=0,purchase_commands=0,terminated=0,truncated=0;
    std::uint64_t grant_resolutions=0,grant_artifacts=0,grant_devils=0,grant_roundtrips=0,policy_refusals=0;
    std::uint64_t normal_copies=0,devil_retries=0,removed_copies=0,removed_used=0,removed_fake=0,removed207=0;
    std::uint64_t confusion_debits=0,cutoff_scenes=0,cutoff_completed=0,proof_merges=0,max_grants=0,max_phase=0;
    std::uint64_t primitive_checksum=0,checksum=0,chance_checksum=0,sampled_action_checksum=0;
    std::uint64_t sampler_checks=0,invalid_sampler_checks=0,capped_fairy_choices=0,chance_rolls=0,chance_hits=0,chance_forced=0;
    std::uint64_t counter_created=0,counter_started=0,death_bows=0,death_created=0,death_started=0,real_source0_frames=0;
    std::uint64_t revivals=0,redraws=0,redrawn_items=0,guardian_opportunities=0,auto_guardian_actions=0;
    std::uint64_t departure_rolls=0,departures=0,disease_ticks=0,gf_advances=0,dead_input_scenes=0;
    std::array<std::uint64_t,5> origins{};
    std::array<std::uint64_t,6> outcomes{};
    std::array<std::uint64_t,3> fairy_resources{};
    void append(const Stats& other) {
#define ADD(name) name+=other.name
        ADD(scenes);ADD(decisions);ADD(responses);ADD(purchase_commands);ADD(terminated);ADD(truncated);
        ADD(grant_resolutions);ADD(grant_artifacts);ADD(grant_devils);ADD(grant_roundtrips);ADD(policy_refusals);
        ADD(normal_copies);ADD(devil_retries);ADD(removed_copies);ADD(removed_used);ADD(removed_fake);ADD(removed207);
        ADD(confusion_debits);ADD(cutoff_scenes);ADD(cutoff_completed);ADD(proof_merges);
        ADD(primitive_checksum);ADD(checksum);ADD(chance_checksum);ADD(sampled_action_checksum);
        ADD(sampler_checks);ADD(invalid_sampler_checks);ADD(capped_fairy_choices);ADD(chance_rolls);ADD(chance_hits);ADD(chance_forced);
        ADD(counter_created);ADD(counter_started);ADD(death_bows);ADD(death_created);ADD(death_started);ADD(real_source0_frames);
        ADD(revivals);ADD(redraws);ADD(redrawn_items);ADD(guardian_opportunities);ADD(auto_guardian_actions);
        ADD(departure_rolls);ADD(departures);ADD(disease_ticks);ADD(gf_advances);ADD(dead_input_scenes);
#undef ADD
        max_grants=std::max(max_grants,other.max_grants);max_phase=std::max(max_phase,other.max_phase);
        for(std::size_t j=0;j<origins.size();++j)origins[j]+=other.origins[j];
        for(std::size_t j=0;j<outcomes.size();++j)outcomes[j]+=other.outcomes[j];
        for(std::size_t j=0;j<fairy_resources.size();++j)fairy_resources[j]+=other.fairy_resources[j];
    }
};
void action_hash(std::uint64_t& hash,const GrantAction& action) {
    mix(hash,action.target);mix(hash,action.model);mix(hash,action.resource.has_value());
    if(action.resource)mix(hash,static_cast<unsigned>(*action.resource));
    mix(hash,action.removed.size());for(auto token:action.removed)mix(hash,token);
}
void refuse_chance_sampler(Match& game,std::uint64_t& chance,Stats& stats) {
    const auto before=game.snapshot();const auto old_chance=chance;
    bool threw=false;try { (void)game.sample_grant(chance); } catch(const MatchError&) { threw=true; }
    check(threw && chance==old_chance && game.snapshot()==before,"wrong-phase grant sampler changed external RNG or Match");
    ++stats.invalid_sampler_checks;
}
void refuse_policy(Match& game,std::uint64_t& policy,Stats& stats) {
    const auto before=game.snapshot(); const auto old_policy=policy;
    check(before.phase==MatchPhase::grant && !before.actor && before.pending_grant,"grant pause exposed a policy actor");
    Command command{*before.pending_grant,CommandKind::pray,{},{}};
    check(!game.legal(command) && !game.confusion_legal(command),"ordinary player command was authorized in grant phase");
    bool threw=false;try { (void)game.sample_action(policy); } catch(const MatchError&) { threw=true; }
    check(threw && policy==old_policy && game.snapshot()==before,"grant-phase sampler failed policy/state atomic refusal");
    threw=false;try { (void)game.step(command); } catch(const MatchError&) { threw=true; }
    check(threw && game.snapshot()==before,"grant phase accepted or partially applied an ordinary command");
    ++stats.policy_refusals;
}
}
int main(int argc,char** argv) { try {
    if(argc>5)throw std::invalid_argument("usage: godfield_grant_sampler_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000,players=argc>2?number(argv[2]):2,seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if(count>1000000000 || players<2 || players>9 || workers>64 || seed>UINT64_MAX-(count-1))throw std::invalid_argument("grant sampler benchmark bounds exceeded");
    workers=std::min(workers,count);
    check(match_gift_weight()==499 && match_dream_compatible(114,125),"grant fixture/pool changed");
    std::vector<Stats> results(workers);std::vector<std::exception_ptr> errors(workers);std::vector<std::jthread> threads;
    const auto start=std::chrono::steady_clock::now();
    for(std::uint64_t worker=0;worker<workers;++worker)threads.emplace_back([&,worker] { try {
        MatchConfig config;config.max_decisions=96;config.external_grants=true;
        auto& stats=results[worker];
        constexpr std::array<CardId,18> cards{235,208,190,112,178,184,187,188,114,207,209,189,236,239,199,203,3,5};
        for(std::uint64_t i=worker;i<count;i+=workers) {
            const bool cutoff_probe=i%97==0,dead_last=players==9 && i%17==0;
            const auto origin=cutoff_probe?std::size_t{0}:static_cast<std::size_t>(i%5);
            auto scene_config=config;if(cutoff_probe)scene_config.max_decisions=2;
            Match game(scene_config),restored(scene_config);
            SnapshotState state;state.gf=17;state.tiebreak_gf=17;
            for(std::uint64_t seat=0;seat<players;++seat) {
                PlayerState p;p.id=seat?100+7*seat:0;p.name="G"+std::to_string(seat);p.alias="alias"+std::to_string(seat);p.user_id="user"+std::to_string(seat);
                p.team=i%3==0?(seat%2?Team::two:Team::one):Team::solo;
                p.hp=99;p.mp=99;p.cp=(seed+i+seat)%100;
                p.guardian=cutoff_probe?Guardian::none:static_cast<Guardian>(1+(seed+i+seat)%10);
                if(!cutoff_probe) {
                    if((i+seat)%4==0)p.curses.push_back(Curse::cold);
                    if((i+seat)%3==0)p.curses.push_back(Curse::fog);
                    if((i+seat)%2==0)p.curses.push_back(Curse::dream);
                    if((i+seat)%7==0)p.curses.push_back(Curse::darkcloud);
                }
                p.dead=dead_last && seat+1==players;if(p.dead)p.hp=0;
                p.confusion_count=p.dead || cutoff_probe?0:static_cast<std::int64_t>((i+seat)%3);
                auto size=4+(seed+i+seat)%15;
                if(!seat && origin==1)size=4+(seed+i)%14;
                if(!seat && cutoff_probe)size=1;
                for(std::uint64_t slot=0;slot<size;++slot) {
                    auto model=cards[slot];
                    if(!seat && slot==0)model=origin==2?6:(origin==3?207:235);
                    if(!seat && origin==1 && model==112)model=199;
                    ItemInstance item{slot,model,{},model==235 || model==236};
                    if(model==114 && has(p,Curse::dream))item.fake_model=125;
                    p.items.push_back(item);
                }
                if(!seat && origin==4)p.hp=1;
                state.players.push_back(std::move(p));
            }
            game.load_scene(std::move(state),0,seed+i);audit(game,restored);const auto initial=game.snapshot();
            const bool confused=initial.state.players.front().confusion_count>0;
            std::uint64_t chance=(seed+i)^0x9b05688c2b3e6c1fULL;
            auto chance_trace=std::uint64_t{14695981039346656037ULL},action_trace=chance_trace;
            refuse_chance_sampler(game,chance,stats);mix(chance_trace,chance);
            if(origin==3) {
                const FateAction action{0,0,{288,{},{}},{}};check(game.fate_legal(action),"owned207 grant origin is illegal");game.fate_step(action);
            } else if(origin==4) {
                const DevilAction action{0,240,{},{}};check(game.devil_legal(action),"revival-debt external origin is illegal");game.devil_step(action);
            } else {
                Command action{0,origin==0?CommandKind::sacrifice:(origin==1?CommandKind::pray:CommandKind::play),{},{}};
                if(origin!=1)action.items={0};if(origin==2)action.target=initial.state.players[1].id;
                check(confused?game.confusion_legal(action):game.legal(action),"ordinary reserved-debt origin is illegal");
                if(confused)game.confusion_step(action);else game.step(action);
            }
            audit(game,restored);verify_origin(game.snapshot(),initial,origin);confusion_transition(initial,game.snapshot(),origin!=4);
            verify_notice(game);auto primitive=world_hash(game.snapshot());mix(primitive,i);mix(primitive,origin);stats.primitive_checksum+=primitive;
            std::uint64_t policy=(seed+i)^0x510e527fade682d1ULL;
            std::uint64_t grants=0,responses=0,purchases=0;
            std::optional<DiseaseTick> first_tick;bool entered_grant=false;
            std::uint64_t frozen_opportunities=0,frozen_auto=0;
            while(!game.done() && game.snapshot().phase!=MatchPhase::main) {
                const auto before=game.snapshot();stats.max_phase=std::max(stats.max_phase,static_cast<std::uint64_t>(before.phase));
                if(before.phase==MatchPhase::grant) {
                    check(before.pending_grant && before.grant_proof && !before.actor,"grant pause lacks a recipient or original journal");
                    if(!entered_grant) {
                        first_tick=before.last_tick;frozen_opportunities=before.guardian_opportunities;frozen_auto=before.auto_guardian_actions;entered_grant=true;
                    }
                    refuse_policy(game,policy,stats);verify_notice(game);++stats.grant_roundtrips;
                    std::size_t seat=0;while(seat<before.gifts.size() && (!before.gifts[seat] || before.state.players[seat].dead))++seat;
                    check(seat<before.gifts.size() && before.state.players[seat].id==*before.pending_grant,"grant pump did not choose the lowest live input seat debt");
                    // Two independent restored clones use the same external chance state.
                    // The helper is privileged and pure; no selected payload reaches a player policy.
                    Match first(before.config),second(before.config);first.restore(before);second.restore(before);
                    auto next_chance=chance,repeat_chance=chance;
                    const auto action=first.sample_grant(next_chance),repeat=second.sample_grant(repeat_chance);
                    check(action==repeat && next_chance==repeat_chance && first.snapshot()==before
                          && second.snapshot()==before && game.snapshot()==before,
                          "grant sampler changed a Match or failed same-state/same-chance reproducibility");
                    ++stats.sampler_checks;
                    check(action.target==*before.pending_grant && (action.model==0 || (action.model>=240 && action.model<=244)),
                          "grant sampler returned a noncanonical target/model");
                    const auto outcome=action.model?static_cast<std::size_t>(action.model-239):std::size_t{0};
                    const auto* recipient=find_player(before.state,action.target);std::vector<CardId> removed;
                    if(action.model==244) {
                        check(action.resource && static_cast<unsigned>(*action.resource)<3 && action.removed.empty(),"fairy sample has inactive or invalid fields");
                        ++stats.fairy_resources[static_cast<unsigned>(*action.resource)];
                        const auto value=*action.resource==Resource::hp?recipient->hp:(*action.resource==Resource::mp?recipient->mp:recipient->cp);
                        stats.capped_fairy_choices+=value==99;
                    } else check(!action.resource,"nonfairy sample contains a resource choice");
                    if(action.model==243) {
                        check(action.removed.size()==std::min<std::size_t>(2,recipient->items.size())
                              && (action.removed.size()<2 || action.removed[0]!=action.removed[1]),"trickster sample omitted copies or selected with replacement");
                        for(const auto token:action.removed) {
                            const auto item=std::find_if(recipient->items.begin(),recipient->items.end(),[&](const auto& copy){return copy.token==token;});
                            check(item!=recipient->items.end(),"trickster sampled another owner's or an absent token");
                            removed.push_back(item->model);++stats.removed_copies;
                            stats.removed_used+=item->used;stats.removed_fake+=item->fake_model.has_value();stats.removed207+=item->model==207;
                        }
                    } else check(action.removed.empty(),"nontrickster sample contains removal tokens");
                    mix(chance_trace,chance);mix(chance_trace,next_chance);
                    action_hash(action_trace,action);
                    check(game.grant_legal(action),"sampled benchmark grant outcome is illegal");game.grant_step(action);chance=next_chance;++grants;++stats.outcomes[outcome];
                    const auto& after=game.snapshot();
                    check(after.decisions==before.decisions+1 && after.state.update_count==before.state.update_count+1
                          && after.grant_resolutions==before.grant_resolutions+1
                          && after.grant_artifacts==before.grant_artifacts+!action.model
                          && after.grant_devils==before.grant_devils+(action.model!=0),"grant choice did not charge exactly one decision/update/outcome");
                    check(after.last_grant==DevilEvent{action.target,action.model,removed},"grant body notice lost removed actual order");
                    check(after.grant_proof && after.grant_proof->grants.back().action==action
                          && after.grant_proof->grants.back().responses_before==before.grant_proof->responses.size(),"accepted choice lost its response-prefix journal position");
                    if(!action.model) {
                        const auto* now=find_player(after.state,action.target);
                        check(now->items.size()==recipient->items.size()+1 && after.next_token==before.next_token+1,"normal grant did not deliver exactly one fresh ordinary copy");
                        const auto& item=now->items.back();check(item.token==before.next_token && !item.used && match_supported(item.model)
                              && find_card(item.model)->category!=Category::devils,"ordinary grant drew a devil or retained copy");++stats.normal_copies;
                    } else if(after.phase==MatchPhase::grant && after.pending_grant==action.target)++stats.devil_retries;
                    ++stats.proof_merges;
                } else {
                    refuse_chance_sampler(game,chance,stats);
                    const auto command=game.sample_action(policy);check(game.legal(command),"grant death/counter follow-up response is illegal");
                    if(before.phase==MatchPhase::response) { check(command.kind==CommandKind::defend,"response policy proposed a main operation");++responses; }
                    else { check(before.phase==MatchPhase::purchase && (command.kind==CommandKind::accept || command.kind==CommandKind::decline),"unexpected grant-origin phase");++purchases; }
                    for(const auto& frame:before.queue)stats.real_source0_frames+=!frame.environmental && frame.source==0;
                    game.step(command);
                }
                audit(game,restored);verify_origin(game.snapshot(),initial,origin);verify_notice(game);confusion_transition(before,game.snapshot(),false);
                if(entered_grant)check(game.snapshot().last_tick==first_tick && game.snapshot().guardian_opportunities==frozen_opportunities
                                      && game.snapshot().auto_guardian_actions==frozen_auto,"grant retry reprocessed owner tick or automatic guardian opportunities");
                if(!game.done() && game.snapshot().phase!=MatchPhase::main)check(game.snapshot().state.gf==17,"GF advanced before grant/response work drained");
            }
            const auto& final=game.snapshot();
            check(final.decisions==1+grants+responses+purchases && final.grant_resolutions==grants,"scene decision/grant journal accounting failed");
            check(final.grant_artifacts+final.grant_devils==grants,"artifact and devil grant counters are not disjoint");
            if(!game.done()) {
                check(final.state.gf==(origin==4?17:18) && final.actor==final.owner,"grant completion used the wrong ordinary/external GF schedule");
                if(origin==4 && !final.state.players.front().dead)check(final.owner==0,"external Devil grant lost its surviving original owner");
            } else check(final.state.gf==17,"natural/cutoff grant advanced GF");
            if(cutoff_probe)check(final.truncated && final.decisions==scene_config.max_decisions && final.grant_resolutions==1,"sampled cutoff probe did not apply exactly one finite grant choice");
            for(const auto& p:initial.state.players)if(p.dead)check(*find_player(final.state,p.id)==p,"grant chain changed an initially dead inventory/player");
            ++stats.scenes;++stats.origins[origin];stats.decisions+=final.decisions;stats.responses+=responses;stats.purchase_commands+=purchases;
            stats.terminated+=final.terminated;stats.truncated+=final.truncated;stats.cutoff_scenes+=cutoff_probe;stats.cutoff_completed+=cutoff_probe && final.truncated;
            stats.confusion_debits+=confused && origin!=4;stats.dead_input_scenes+=dead_last;stats.max_grants=std::max(stats.max_grants,grants);
            stats.gf_advances+=final.state.gf-17;stats.disease_ticks+=final.last_tick.has_value();
#define COUNT(name) stats.name+=final.name
            COUNT(grant_resolutions);COUNT(grant_artifacts);COUNT(grant_devils);COUNT(chance_rolls);COUNT(chance_hits);COUNT(counter_created);COUNT(counter_started);
            COUNT(death_bows);COUNT(death_created);COUNT(death_started);COUNT(revivals);COUNT(redraws);COUNT(redrawn_items);COUNT(guardian_opportunities);COUNT(auto_guardian_actions);
#undef COUNT
            stats.chance_forced+=final.chance_forced_hits;stats.departure_rolls+=final.guardian_departure_rolls;stats.departures+=final.guardian_departures;
            auto hash=world_hash(final);mix(hash,i);mix(hash,origin);stats.checksum+=hash;
            mix(chance_trace,chance);mix(chance_trace,i);mix(chance_trace,grants);stats.chance_checksum+=chance_trace;
            mix(action_trace,i);mix(action_trace,grants);stats.sampled_action_checksum+=action_trace;
        }
    } catch(...) { errors[worker]=std::current_exception(); } });
    for(auto& thread:threads)thread.join();for(const auto& error:errors)if(error)std::rethrow_exception(error);
    Stats total;for(const auto& stats:results)total.append(stats);
    std::uint64_t origins=0,outcomes=0;for(auto n:total.origins)origins+=n;for(auto n:total.outcomes)outcomes+=n;
    check(total.scenes==count && origins==count && outcomes==total.grant_resolutions
          && total.decisions==count+total.responses+total.purchase_commands+total.grant_resolutions
          && total.grant_resolutions==total.grant_artifacts+total.grant_devils
          && total.normal_copies==total.grant_artifacts && total.policy_refusals==total.grant_roundtrips
          && total.grant_roundtrips==total.grant_resolutions && total.sampler_checks==total.grant_resolutions
          && total.cutoff_completed==total.cutoff_scenes
          && total.terminated+total.truncated<=count,"grant sampler benchmark aggregate accounting failed");
    if(count>=600) {
        for(auto n:total.outcomes)check(n>0,"grant sampler fixture omitted one of its six local-policy outcomes");
        for(auto n:total.fairy_resources)check(n>0,"grant sampler fixture omitted a fairy resource");
        check(total.removed_used>0,"grant sampler fixture did not select a retained miracle copy");
    }
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset<<"\nscope=experimental:local-published-mass-grant-policy-after-one-original-action;independent-external-chance-and-player-policy-rng;not-an-official-server-selector;world-selected-counter-hashes-plus-full-snapshot-restore-audit"
             <<"\ngrant_rule="<<grant_rule<<"\ngrant_sampler_rule="<<grant_sampler_rule<<"\nfate_pool_rule="<<fate_pool_rule<<"\nfate_sampler_rule="<<fate_sampler_rule<<"\nscenes="<<total.scenes<<"\nplayers="<<players<<"\nseed="<<seed<<"\nworkers="<<workers<<"\nmax_decisions=96";
#define OUTPUT(name) std::cout<<"\n" #name "="<<total.name
    OUTPUT(decisions);OUTPUT(responses);OUTPUT(purchase_commands);OUTPUT(terminated);OUTPUT(truncated);OUTPUT(grant_resolutions);OUTPUT(grant_artifacts);OUTPUT(grant_devils);
    OUTPUT(grant_roundtrips);OUTPUT(policy_refusals);OUTPUT(normal_copies);OUTPUT(devil_retries);OUTPUT(removed_copies);OUTPUT(removed_used);OUTPUT(removed_fake);OUTPUT(removed207);
    OUTPUT(confusion_debits);OUTPUT(cutoff_scenes);OUTPUT(cutoff_completed);OUTPUT(proof_merges);OUTPUT(max_grants);OUTPUT(max_phase);
    OUTPUT(chance_rolls);OUTPUT(chance_hits);OUTPUT(chance_forced);OUTPUT(counter_created);OUTPUT(counter_started);OUTPUT(death_bows);OUTPUT(death_created);OUTPUT(death_started);OUTPUT(real_source0_frames);
    OUTPUT(revivals);OUTPUT(redraws);OUTPUT(redrawn_items);OUTPUT(guardian_opportunities);OUTPUT(auto_guardian_actions);OUTPUT(departure_rolls);OUTPUT(departures);OUTPUT(disease_ticks);OUTPUT(gf_advances);OUTPUT(dead_input_scenes);
    OUTPUT(sampler_checks);OUTPUT(invalid_sampler_checks);OUTPUT(capped_fairy_choices);
    OUTPUT(primitive_checksum);OUTPUT(chance_checksum);OUTPUT(sampled_action_checksum);OUTPUT(checksum);
#undef OUTPUT
    for(std::size_t j=0;j<total.origins.size();++j)std::cout<<"\norigin"<<j<<"_scenes="<<total.origins[j];
    for(std::size_t j=0;j<total.outcomes.size();++j)std::cout<<"\noutcome"<<(j?239+j:0)<<"_grants="<<total.outcomes[j];
    for(std::size_t j=0;j<total.fairy_resources.size();++j)std::cout<<"\nfairy_resource"<<j<<"_grants="<<total.fairy_resources[j];
    std::cout<<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';
    return 0;
} catch(const std::exception& error) { std::cerr<<"grant sampler benchmark: "<<error.what()<<'\n';return 1; } }
