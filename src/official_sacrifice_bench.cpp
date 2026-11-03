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
    restored.restore(game.snapshot()); check(restored.snapshot()==game.snapshot(),"sacrifice snapshot failed full roundtrip");
}
bool active(const SnapshotState& state) { return state.tiebreak_gf && state.gf>=*state.tiebreak_gf; }
bool eligible(const ItemInstance& item) {
    const auto& card=*find_card(face_model(item));
    return card.category!=Category::weapons && card.ability!=Ability::revive && card.ability!=Ability::attract_danger;
}
void count_transition(const MatchSnapshot& before,const MatchSnapshot& after,bool debit) {
    for (const auto& player:before.state.players) {
        const auto* now=find_player(after.state,player.id); check(now!=nullptr,"operation changed roster");
        auto expected=player.confusion_count; if(debit && player.id==0 && expected>0) --expected;
        if(now->dead || after.terminated) expected=0;
        check(now->confusion_count==expected,"discard/sacrifice confusion count changed outside launch/death/end");
    }
}
struct Stats {
    std::uint64_t scenes=0,decisions=0,responses=0,purchase_commands=0,terminated=0,truncated=0;
    std::uint64_t discard_plays=0,sacrifice_plays=0,selected_copies=0,selected_used=0,selected_fake=0,selected207=0,max_selected=0;
    std::uint64_t confusion_debits=0,sampler_pair_checks=0,sampler_operation_mappings=0,isolated_scenes=0,isolated_gifts=0;
    std::uint64_t full18_used_scenes=0,full18_used_sacrifices=0,threshold_crossings=0,dead_input_scenes=0;
    std::uint64_t primitive_checksum=0,checksum=0,chance_rolls=0,chance_hits=0,chance_forced=0;
    std::uint64_t counter_created=0,counter_started=0,death_bows=0,death_created=0,death_started=0,revivals=0,redraws=0,redrawn_items=0;
    std::uint64_t guardian_opportunities=0,auto_guardian_actions=0,departure_rolls=0,departures=0,disease_ticks=0,fresh_copies=0;
    std::array<std::uint64_t,5> thresholds{};
    void append(const Stats& other) {
#define ADD(name) name+=other.name
        ADD(scenes); ADD(decisions); ADD(responses); ADD(purchase_commands); ADD(terminated); ADD(truncated);
        ADD(discard_plays); ADD(sacrifice_plays); ADD(selected_copies); ADD(selected_used); ADD(selected_fake); ADD(selected207);
        ADD(confusion_debits); ADD(sampler_pair_checks); ADD(sampler_operation_mappings); ADD(isolated_scenes); ADD(isolated_gifts);
        ADD(full18_used_scenes); ADD(full18_used_sacrifices); ADD(threshold_crossings); ADD(dead_input_scenes);
        ADD(primitive_checksum); ADD(checksum); ADD(chance_rolls); ADD(chance_hits); ADD(chance_forced);
        ADD(counter_created); ADD(counter_started); ADD(death_bows); ADD(death_created); ADD(death_started); ADD(revivals); ADD(redraws); ADD(redrawn_items);
        ADD(guardian_opportunities); ADD(auto_guardian_actions); ADD(departure_rolls); ADD(departures); ADD(disease_ticks); ADD(fresh_copies);
#undef ADD
        max_selected=std::max(max_selected,other.max_selected);
        for(std::size_t i=0;i<thresholds.size();++i) thresholds[i]+=other.thresholds[i];
    }
};
void sampler_pair(const MatchSnapshot& before,std::uint64_t policy,Stats& stats) {
    auto ordinary=before,terminal=before;
    ordinary.state.tiebreak_gf.reset(); terminal.state.tiebreak_gf=0;
    for(auto& p:ordinary.state.players) if(p.id==0) p.confusion_count=0;
    for(auto& p:terminal.state.players) if(p.id==0) p.confusion_count=0;
    Match a(before.config),b(before.config); a.restore(ordinary); b.restore(terminal);
    auto pa=policy,pb=policy; auto ca=a.sample_action(pa); const auto cb=b.sample_action(pb);
    if(ca.kind==CommandKind::discard) { ca.kind=CommandKind::sacrifice; ++stats.sampler_operation_mappings; }
    check(ca==cb && pa==pb,"threshold operation mapping changed proposal multiplicity or policy RNG");
    ++stats.sampler_pair_checks;
}
void selected_absent(const MatchSnapshot& state,const Command& command) {
    const auto* own=find_player(state.state,0);
    for(auto token:command.items) check(std::none_of(own->items.begin(),own->items.end(),[&](const auto& item) { return item.token==token; }),
                                      "selected copy was retained or restored instead of erased");
}
void isolated_receipt(const MatchSnapshot& before,const MatchSnapshot& after,const Command& command,Stats& stats) {
    const auto n=command.kind==CommandKind::sacrifice?command.items.size():0;
    check(after.phase==MatchPhase::main && after.owner==before.state.players[1].id && after.actor==after.owner
          && after.state.gf==18 && after.decisions==1 && !after.batch && after.queue.empty() && !after.last_tick
          && !after.guardian_opportunities && !after.auto_guardian_actions && !after.counter_created && !after.death_created
          && !after.revivals && !after.redraws && std::all_of(after.gifts.begin(),after.gifts.end(),[](auto value) { return value==0; }),
          "isolated operation invoked a body ability/follow-up instead of ordinary completion");
    auto expected=before.state; ++expected.update_count; expected.gf=18;
    auto& own=expected.players.front(); if(own.confusion_count>0) --own.confusion_count;
    std::erase_if(own.items,[&](const auto& item) { return std::find(command.items.begin(),command.items.end(),item.token)!=command.items.end(); });
    const auto survivors=own.items.size(); const auto& received=after.state.players.front().items;
    check(received.size()==survivors+n && after.next_token==before.next_token+n,"isolated copy/gift count was not exact");
    for(std::size_t i=0;i<n;++i) {
        const auto& item=received[survivors+i];
        check(item.token==before.next_token+i && !item.used && !item.fake_model && match_supported(item.model)
              && find_card(item.model)->category!=Category::devils,"replacement was retained, masked, nonpool or an automatic devil");
        own.items.push_back(item);
    }
    check(after.state==expected,"isolated operation changed MP/resource/curses/other copy or triggered an item ability");
    ++stats.isolated_scenes; stats.isolated_gifts+=n;
}
}
int main(int argc,char** argv) { try {
    if(argc>5) throw std::invalid_argument("usage: godfield_sacrifice_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000,players=argc>2?number(argv[2]):2,seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if(count>1000000000 || players<2 || players>9 || workers>64 || seed>UINT64_MAX-(count-1)) throw std::invalid_argument("sacrifice benchmark bounds exceeded");
    workers=std::min(workers,count);
    check(match_gift_weight()==499 && match_dream_compatible(114,125),"gift pool or compatible fake fixture changed");
    std::vector<Stats> results(workers); std::vector<std::exception_ptr> errors(workers); std::vector<std::jthread> threads;
    const auto start=std::chrono::steady_clock::now();
    for(std::uint64_t worker=0;worker<workers;++worker) threads.emplace_back([&,worker] { try {
        MatchConfig config; config.max_decisions=64; Match game(config),restored(config); auto& stats=results[worker];
        constexpr std::array<CardId,18> cards{235,114,178,168,207,3,4,5,203,236,239,190,189,187,112,208,209,6};
        constexpr std::array<std::int64_t,4> hps{1,5,40,99};
        for(std::uint64_t i=worker;i<count;i+=workers) {
            const auto threshold=static_cast<std::size_t>(i%5); const bool full18=i%29==0,isolated=full18 || i%13==0;
            const bool dead_last=players==9 && i%17==0;
            SnapshotState state; state.gf=17;
            if(threshold==1) state.tiebreak_gf=0;
            else if(threshold==2) state.tiebreak_gf=17;
            else if(threshold==3) state.tiebreak_gf=18;
            else if(threshold==4) state.tiebreak_gf=static_cast<std::int64_t>(max_wire_integer);
            for(std::uint64_t seat=0;seat<players;++seat) {
                PlayerState p; p.id=seat==0?0:100+7*seat; p.name="P"+std::to_string(seat);
                p.alias="A"+std::to_string(seat); p.user_id="user"+std::to_string(seat);
                p.team=i%3==0?(seat%2?Team::two:Team::one):Team::solo;
                p.hp=seat==0?hps[(i/5)%hps.size()]:15+static_cast<std::int64_t>((seed+i+17*seat)%85);
                p.mp=seat==0?0:99; p.cp=(seed+i+seat)%100;
                p.guardian=isolated?Guardian::none:static_cast<Guardian>(1+(seed+i+seat)%10);
                if(!isolated) {
                    if((i+seat)%4==0) p.curses.push_back(Curse::cold);
                    if((i+seat)%3==0) p.curses.push_back(Curse::fog);
                    if((i+seat)%2==0) p.curses.push_back(Curse::dream);
                    if((i+seat)%5==0) p.curses.push_back(Curse::flash);
                    if((i+seat)%7==0) p.curses.push_back(Curse::darkcloud);
                }
                p.dead=dead_last && seat+1==players; if(p.dead) p.hp=0;
                p.confusion_count=p.dead?0:static_cast<std::int64_t>((i+seat)%3);
                const auto size=(seat==0 && full18)?18:4+(seed+i+seat)%15;
                for(std::uint64_t slot=0;slot<size;++slot) {
                    const auto id=(seat==0 && full18)?CardId{235}:cards[slot];
                    ItemInstance item{slot,id,{},(seat==0 && full18)||slot==0};
                    if(id==114 && !isolated && (i+seat)%2==0) item.fake_model=125;
                    p.items.push_back(item);
                }
                state.players.push_back(std::move(p));
            }
            game.load_scene(std::move(state),0,seed+i); audit(game,restored); const auto before=game.snapshot();
            const auto stage=active(before.state); const auto confused=before.state.players.front().confusion_count>0;
            check(game.observe({}).find(std::string("\"apocalypse_active\":")+(stage?"true":"false"))!=std::string::npos,
                  "public stage differs from launch GF/threshold");
            sampler_pair(before,(seed+i)^0x5be0cd19137e2179ULL,stats);
            std::vector<ItemToken> choices;
            for(const auto& item:before.state.players.front().items) if(eligible(item)) choices.push_back(item.token);
            check(!choices.empty(),"operation fixture has no eligible physical copies");
            if(!choices.empty()) std::rotate(choices.begin(),choices.begin()+static_cast<std::ptrdiff_t>((i/5)%choices.size()),choices.end());
            const auto n=(full18 || i%11==0)?choices.size():1+(i/5)%choices.size();
            Command command{0,stage?CommandKind::sacrifice:CommandKind::discard,{}, {}};
            command.items.assign(choices.begin(),choices.begin()+static_cast<std::ptrdiff_t>(n));
            auto opposite=command; opposite.kind=stage?CommandKind::discard:CommandKind::sacrifice;
            check((confused?game.confusion_legal(command):game.legal(command))
                  && !(confused?game.confusion_legal(opposite):game.legal(opposite)),"launch operation did not use canonical stage replacement");
            if(confused) check(!game.legal(command),"confused main bypassed external control");
            for(auto token:command.items) {
                const auto& hand=before.state.players.front().items;
                const auto item=std::find_if(hand.begin(),hand.end(),[&](const auto& v) { return v.token==token; });
                check(item!=hand.end() && eligible(*item),"selected weapon/passive or nonexistent copy");
                stats.selected_used+=item->used; stats.selected_fake+=item->fake_model.has_value(); stats.selected207+=item->model==207;
            }
            if(confused) game.confusion_step(command); else game.step(command);
            audit(game,restored); selected_absent(game.snapshot(),command); count_transition(before,game.snapshot(),true);
            if(game.snapshot().batch) check(game.snapshot().batch->attack==command && game.snapshot().batch->initial_state==before.state
                                          && game.snapshot().batch->confusion_command==confused,"operation pending lost prelaunch kind/threshold/anchor");
            if(isolated) isolated_receipt(before,game.snapshot(),command,stats);
            auto primitive=world_hash(game.snapshot()); mix(primitive,i); mix(primitive,static_cast<unsigned>(command.kind)); stats.primitive_checksum+=primitive;
            std::uint64_t policy=(seed+i)^0xcbbb9d5dc1059ed8ULL;
            while(!game.done() && game.snapshot().phase!=MatchPhase::main) {
                const auto pending=game.snapshot(); const auto response=game.sample_action(policy); check(game.legal(response),"operation follow-up response is illegal");
                if(pending.phase==MatchPhase::response) { check(response.kind==CommandKind::defend,"follow-up sampled non-defense"); ++stats.responses; }
                else { check(pending.phase==MatchPhase::purchase && (response.kind==CommandKind::accept || response.kind==CommandKind::decline),"unexpected operation pending phase"); ++stats.purchase_commands; }
                game.step(response); audit(game,restored); selected_absent(game.snapshot(),command); count_transition(pending,game.snapshot(),false);
            }
            const auto& final=game.snapshot();
            check(!final.devil_actions && !final.fate_actions && !final.phenomenon_actions && !final.guardian_actions && !final.guardian_payload_actions,
                  "threshold operation invoked automatic devil or external selectors");
            for(const auto& p:before.state.players) if(p.dead) check(*find_player(final.state,p.id)==p,"operation changed an initially dead player");
            if(!game.done()) check(final.state.gf==18 && final.phase==MatchPhase::main && final.actor==final.owner,
                                   "operation failed ordinary GF/owner completion");
            else check(final.state.gf==17,"terminal/cutoff operation advanced GF");
            ++stats.scenes; ++stats.thresholds[threshold]; stats.discard_plays+=!stage; stats.sacrifice_plays+=stage;
            stats.selected_copies+=n; stats.max_selected=std::max<std::uint64_t>(stats.max_selected,n); stats.confusion_debits+=confused;
            stats.full18_used_scenes+=full18; stats.full18_used_sacrifices+=full18 && stage; stats.dead_input_scenes+=dead_last;
            stats.threshold_crossings+=!stage && active(final.state); stats.decisions+=final.decisions;
            stats.terminated+=final.terminated; stats.truncated+=final.truncated;
#define COUNT(name) stats.name+=final.name
            COUNT(chance_rolls); COUNT(chance_hits); COUNT(counter_created); COUNT(counter_started); COUNT(death_bows); COUNT(death_created); COUNT(death_started);
            COUNT(revivals); COUNT(redraws); COUNT(redrawn_items); COUNT(guardian_opportunities); COUNT(auto_guardian_actions);
#undef COUNT
            stats.chance_forced+=final.chance_forced_hits; stats.departure_rolls+=final.guardian_departure_rolls; stats.departures+=final.guardian_departures;
            stats.disease_ticks+=final.last_tick.has_value(); stats.fresh_copies+=final.next_token-before.next_token;
            auto hash=world_hash(final); mix(hash,i); mix(hash,static_cast<unsigned>(command.kind)); stats.checksum+=hash;
        }
    } catch(...) { errors[worker]=std::current_exception(); } });
    for(auto& thread:threads) thread.join(); for(const auto& error:errors) if(error) std::rethrow_exception(error);
    Stats total; for(const auto& stats:results) total.append(stats);
    std::uint64_t threshold_scenes=0; for(auto n:total.thresholds) threshold_scenes+=n;
    check(total.scenes==count && threshold_scenes==count && total.discard_plays+total.sacrifice_plays==count
          && total.decisions==count+total.responses+total.purchase_commands && total.sampler_pair_checks==count
          && total.selected_used<=total.selected_copies && total.selected_fake<=total.selected_copies
          && total.max_selected<=visible_hand_capacity && total.chance_hits<=total.chance_rolls
          && total.terminated+total.truncated<=count,"sacrifice benchmark accounting failed");
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset<<"\nscope=experimental:explicit-threshold-discard-or-sacrifice-then-normal-followups;isolated-exact-gift-receipts-and-rich-replay;not-automatic-devils-or-official-gift-schedule;world-selected-counter-hashes-plus-full-restore-audit"
             <<"\napocalypse_rule="<<apocalypse_rule<<"\ngrant_rule="<<grant_rule<<"\ngrant_sampler_rule="<<grant_sampler_rule<<"\nfate_pool_rule="<<fate_pool_rule<<"\nfate_sampler_rule="<<fate_sampler_rule<<"\nconfusion_sampler_rule="<<confusion_sampler_rule<<"\nguardian_payload_sampler_rule="<<guardian_payload_sampler_rule<<"\nscenes="<<total.scenes<<"\nplayers="<<players<<"\nseed="<<seed<<"\nworkers="<<workers;
#define OUTPUT(name) std::cout<<"\n" #name "="<<total.name
    OUTPUT(decisions); OUTPUT(responses); OUTPUT(purchase_commands); OUTPUT(terminated); OUTPUT(truncated); OUTPUT(discard_plays); OUTPUT(sacrifice_plays);
    OUTPUT(selected_copies); OUTPUT(selected_used); OUTPUT(selected_fake); OUTPUT(selected207); OUTPUT(max_selected); OUTPUT(confusion_debits);
    OUTPUT(sampler_pair_checks); OUTPUT(sampler_operation_mappings); OUTPUT(isolated_scenes); OUTPUT(isolated_gifts);
    OUTPUT(full18_used_scenes); OUTPUT(full18_used_sacrifices); OUTPUT(threshold_crossings); OUTPUT(dead_input_scenes);
    OUTPUT(chance_rolls); OUTPUT(chance_hits); OUTPUT(chance_forced); OUTPUT(counter_created); OUTPUT(counter_started); OUTPUT(death_bows); OUTPUT(death_created); OUTPUT(death_started);
    OUTPUT(revivals); OUTPUT(redraws); OUTPUT(redrawn_items); OUTPUT(guardian_opportunities); OUTPUT(auto_guardian_actions); OUTPUT(departure_rolls); OUTPUT(departures); OUTPUT(disease_ticks); OUTPUT(fresh_copies);
    OUTPUT(primitive_checksum); OUTPUT(checksum);
#undef OUTPUT
    for(std::size_t n=0;n<total.thresholds.size();++n) std::cout<<"\nthreshold"<<n<<"_scenes="<<total.thresholds[n];
    std::cout<<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';
    return 0;
} catch(const std::exception& error) { std::cerr<<"sacrifice benchmark: "<<error.what()<<'\n'; return 1; } }
