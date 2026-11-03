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
// hash: ItemShuffleRecord and pending journals are checked by full restore.
std::uint64_t world_hash(const MatchSnapshot& s) {
    std::uint64_t hash=14695981039346656037ULL;
    mix(hash,s.version); mix(hash,s.state.gf); mix(hash,s.state.update_count); mix(hash,s.state.is_over);
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
    mix(hash,s.terminated); mix(hash,s.truncated); mix(hash,s.phenomenon_actions);
    mix(hash,s.chance_rolls); mix(hash,s.chance_hits); mix(hash,s.chance_forced_hits);
    mix(hash,s.counter_created); mix(hash,s.counter_started); mix(hash,s.counter_rolls); mix(hash,s.counter_hits); mix(hash,s.counter_forced_hits);
    mix(hash,s.death_bows); mix(hash,s.death_created); mix(hash,s.death_started); mix(hash,s.death_rolls); mix(hash,s.death_hits); mix(hash,s.death_forced_hits);
    mix(hash,s.revivals); mix(hash,s.redraws); mix(hash,s.redrawn_items); mix(hash,s.dream_rolls); mix(hash,s.dream_masks);
    mix(hash,s.guardian_summons); mix(hash,s.guardian_opportunities); mix(hash,s.auto_guardian_actions);
    mix(hash,s.guardian_departure_rolls); mix(hash,s.guardian_departures);
    return hash;
}
void audit(const Match& game, Match& restored) {
    restored.restore(game.snapshot()); check(restored.snapshot()==game.snapshot(),"item shuffle snapshot failed full roundtrip");
}
bool dreaming(const PlayerState& player) {
    return std::find(player.curses.begin(),player.curses.end(),Curse::dream)!=player.curses.end();
}
const ItemInstance& source_item(const SnapshotState& state, const ItemSource& source) {
    const auto* owner=find_player(state,source.owner); check(owner && !owner->dead,"manifest references a missing/dead owner");
    const auto item=std::find_if(owner->items.begin(),owner->items.end(),[&](const auto& value) { return value.token==source.token; });
    check(item!=owner->items.end(),"manifest source token missing"); return *item;
}
std::vector<std::pair<CardId,bool>> live_models(const SnapshotState& state) {
    std::vector<std::pair<CardId,bool>> result;
    for (const auto& p:state.players) if (!p.dead) for (const auto& item:p.items) result.emplace_back(item.model,item.used);
    std::sort(result.begin(),result.end()); return result;
}
struct Stats {
    std::uint64_t scenes=0,decisions=0,pending_commands=0,responses=0,purchase_commands=0,terminated=0,truncated=0;
    std::uint64_t copies=0,empty_scenes=0,max_copies=0,dead_input_scenes=0,used_copies=0,unused_miracles=0;
    std::uint64_t masked_preserved=0,masked_cleared=0,unmasked_to_dream=0,old_token_checks=0;
    std::uint64_t primitive_checksum=0,checksum=0,manifest_checksum=0;
    std::uint64_t chance_rolls=0,chance_hits=0,chance_forced=0,counter_created=0,counter_started=0;
    std::uint64_t death_bows=0,death_created=0,death_started=0,revivals=0,redraws=0;
    std::uint64_t guardian_opportunities=0,auto_guardian_actions=0,departure_rolls=0,departures=0,disease_ticks=0;
    std::array<std::uint64_t,4> patterns{};
    void append(const Stats& other) {
#define ADD(name) name+=other.name
        ADD(scenes); ADD(decisions); ADD(pending_commands); ADD(responses); ADD(purchase_commands); ADD(terminated); ADD(truncated);
        ADD(copies); ADD(empty_scenes); ADD(dead_input_scenes); ADD(used_copies); ADD(unused_miracles);
        ADD(masked_preserved); ADD(masked_cleared); ADD(unmasked_to_dream); ADD(old_token_checks);
        ADD(primitive_checksum); ADD(checksum); ADD(manifest_checksum); ADD(chance_rolls); ADD(chance_hits); ADD(chance_forced);
        ADD(counter_created); ADD(counter_started); ADD(death_bows); ADD(death_created); ADD(death_started); ADD(revivals); ADD(redraws);
        ADD(guardian_opportunities); ADD(auto_guardian_actions); ADD(departure_rolls); ADD(departures); ADD(disease_ticks);
#undef ADD
        max_copies=std::max(max_copies,other.max_copies);
        for (std::size_t i=0;i<patterns.size();++i) patterns[i]+=other.patterns[i];
    }
};
void verify_shuffle(const MatchSnapshot& before, const MatchSnapshot& after,
                    const ItemShuffleAction& action, Stats& stats) {
    check(after.decisions==1 && after.phase==MatchPhase::main && after.owner==before.owner && after.actor==before.actor,
          "shuffle advanced ordinary owner or response phase");
    check(after.phenomenon_actions==1 && after.rng==before.rng && after.state.gf==before.state.gf
          && after.next_token==before.next_token+action.order.size(),"shuffle changed RNG/GF or fresh-token budget");
    check(after.last_phenomenon==PhenomenonAction{295,{},{}},"shuffle notice exposed more than model295");
    check(after.last_item_shuffle.has_value() && after.last_item_shuffle->action==action
          && after.last_item_shuffle->anchor.initial_state==before.state
          && after.last_item_shuffle->anchor.rng==before.rng && after.last_item_shuffle->anchor.next_token==before.next_token,
          "shuffle lost its private pre-action evidence");
    check(after.gifts==before.gifts && after.guardian_actions==0 && after.guardian_payload_actions==0
          && after.guardian_opportunities==0 && after.auto_guardian_actions==0 && after.guardian_summons==0
          && after.guardian_departure_rolls==0 && after.counter_created==0 && after.death_created==0
          && after.revivals==0 && after.redraws==0 && after.last_reveals.empty() && !after.last_tick,
          "shuffle invoked a resource/inventory/guardian ability or public reveal");
    auto expected=before.state; ++expected.update_count;
    std::size_t cursor=0;
    for (auto& player:expected.players) if (!player.dead) {
        for (auto& slot:player.items) {
            auto copy=source_item(before.state,action.order[cursor]);
            copy.token=before.next_token+cursor; ++cursor;
            stats.used_copies+=copy.used;
            stats.unused_miracles+=!copy.used && find_card(copy.model)->category==Category::miracles;
            if (copy.fake_model) {
                if (dreaming(player)) ++stats.masked_preserved;
                else { copy.fake_model.reset(); ++stats.masked_cleared; }
            } else stats.unmasked_to_dream+=dreaming(player);
            slot=copy;
        }
    }
    check(cursor==action.order.size() && after.state==expected,"shuffle failed fixed-count copy/fake/used/dead preservation");
    check(live_models(before.state)==live_models(after.state),"shuffle changed the model/used multiset");
    for (const auto& player:before.state.players) if (!player.dead) for (const auto& old:player.items) {
        const auto* recipient=find_player(after.state,player.id);
        check(std::none_of(recipient->items.begin(),recipient->items.end(),[&](const auto& item) { return item.token==old.token; }),
              "an old live ownership token remained usable"); ++stats.old_token_checks;
    }
}
}
int main(int argc,char** argv) { try {
    if (argc>5) throw std::invalid_argument("usage: godfield_item_shuffle_bench [scenes=10000] [players=2] [seed=42] [workers=1]");
    const auto count=argc>1?number(argv[1]):10000, players=argc>2?number(argv[2]):2, seed=argc>3?number(argv[3],false):42;
    auto workers=argc>4?number(argv[4]):1;
    if (count>1000000000 || players<2 || players>9 || workers>64 || seed>UINT64_MAX-(count-1))
        throw std::invalid_argument("item shuffle benchmark bounds exceeded");
    workers=std::min(workers,count);
    check(match_dream_compatible(6,7),"masked fixture is outside the compatible Dream class");
    std::vector<Stats> results(workers); std::vector<std::exception_ptr> errors(workers); std::vector<std::jthread> threads;
    const auto start=std::chrono::steady_clock::now();
    for (std::uint64_t worker=0;worker<workers;++worker) threads.emplace_back([&,worker] { try {
        MatchConfig config; config.max_decisions=64; Match game(config),restored(config); auto& stats=results[worker];
        constexpr std::array<CardId,18> cards{6,235,235,208,112,190,178,184,187,188,3,4,5,203,236,239,66,58};
        for (std::uint64_t i=worker;i<count;i+=workers) {
            SnapshotState state; state.gf=17;
            const bool all_empty=i%97==0, dead_last=players==9 && i%17==0;
            for (std::uint64_t seat=0;seat<players;++seat) {
                PlayerState p; p.id=seat==0?0:100+7*seat; p.name="P"+std::to_string(seat);
                p.alias="A"+std::to_string(seat); p.user_id="user"+std::to_string(seat);
                p.team=i%3==0?(seat%2?Team::two:Team::one):Team::solo;
                p.hp=15+static_cast<std::int64_t>((seed+i+17*seat)%85); p.mp=99; p.cp=(seed+i+seat)%100;
                p.guardian=static_cast<Guardian>(1+(seed+i+seat)%10);
                if ((i+seat)%4==0) p.curses.push_back(Curse::cold);
                if ((i+seat)%3==0) p.curses.push_back(Curse::fog);
                if ((i+seat)%2==0) p.curses.push_back(Curse::dream);
                p.dead=dead_last && seat+1==players; if (p.dead) p.hp=0;
                const auto size=all_empty?0:(i%101==1?18:((i+seat)%19==0?0:4+(seed+i+seat)%15));
                for (std::uint64_t slot=0;slot<size;++slot) {
                    ItemInstance item{slot,cards[slot],{},slot==2};
                    if (slot==0 && dreaming(p)) item.fake_model=7;
                    p.items.push_back(item);
                }
                state.players.push_back(std::move(p));
            }
            game.load_scene(std::move(state),0,seed+i); audit(game,restored);
            const auto before=game.snapshot(); ItemShuffleAction action;
            for (const auto& p:before.state.players) if (!p.dead) for (const auto& item:p.items) action.order.push_back({p.id,item.token});
            const auto pattern=static_cast<std::size_t>(i%4);
            if (pattern==1) std::reverse(action.order.begin(),action.order.end());
            else if (pattern==2 && !action.order.empty()) std::rotate(action.order.begin(),action.order.begin()+1+i%action.order.size(),action.order.end());
            else if (pattern==3) {
                const auto source=action.order; action.order.clear();
                for (std::size_t parity=0;parity<2;++parity) for (std::size_t j=parity;j<source.size();j+=2) action.order.push_back(source[j]);
            }
            check(game.item_shuffle_legal(action),"externally supplied permutation is not legal");
            game.item_shuffle_step(action); audit(game,restored); verify_shuffle(before,game.snapshot(),action,stats);
            auto primitive=world_hash(game.snapshot()); mix(primitive,i); stats.primitive_checksum+=primitive;
            auto manifest=world_hash(before); mix(manifest,i); mix(manifest,action.order.size());
            for (const auto& source:action.order) { mix(manifest,source.owner); mix(manifest,source.token); }
            stats.manifest_checksum+=manifest;
            stats.copies+=action.order.size(); stats.max_copies=std::max<std::uint64_t>(stats.max_copies,action.order.size());
            stats.empty_scenes+=action.order.empty(); stats.dead_input_scenes+=dead_last; ++stats.patterns[pattern];
            std::uint64_t policy=(seed+i)^0x3c6ef372fe94f82bULL;
            const auto main=game.sample_action(policy); check(game.legal(main),"post-shuffle ordinary sampler command is illegal");
            check(main.kind!=CommandKind::defend && main.kind!=CommandKind::accept && main.kind!=CommandKind::decline,
                  "ordinary main sampler selected a pending-only command");
            game.step(main); audit(game,restored);
            check(!game.snapshot().last_item_shuffle,"accepted ordinary main did not clear shuffle proof");
            while (!game.done() && game.snapshot().phase!=MatchPhase::main) {
                const auto command=game.sample_action(policy); check(game.legal(command),"post-shuffle response/purchase command is illegal");
                if (game.snapshot().phase==MatchPhase::response) { check(command.kind==CommandKind::defend,"response sampled a non-defense"); ++stats.responses; }
                else { check(game.snapshot().phase==MatchPhase::purchase && (command.kind==CommandKind::accept || command.kind==CommandKind::decline),"unexpected pending phase"); ++stats.purchase_commands; }
                game.step(command); audit(game,restored); ++stats.pending_commands;
            }
            const auto& final=game.snapshot();
            check(final.phenomenon_actions==1 && final.guardian_actions==0 && final.guardian_payload_actions==0,
                  "ordinary follow-up invented an external activation");
            ++stats.scenes; stats.decisions+=final.decisions; stats.terminated+=final.terminated; stats.truncated+=final.truncated;
            stats.chance_rolls+=final.chance_rolls; stats.chance_hits+=final.chance_hits; stats.chance_forced+=final.chance_forced_hits;
            stats.counter_created+=final.counter_created; stats.counter_started+=final.counter_started;
            stats.death_bows+=final.death_bows; stats.death_created+=final.death_created; stats.death_started+=final.death_started;
            stats.revivals+=final.revivals; stats.redraws+=final.redraws; stats.guardian_opportunities+=final.guardian_opportunities;
            stats.auto_guardian_actions+=final.auto_guardian_actions; stats.departure_rolls+=final.guardian_departure_rolls;
            stats.departures+=final.guardian_departures; stats.disease_ticks+=final.last_tick.has_value();
            auto hash=world_hash(final); mix(hash,i); stats.checksum+=hash;
        }
    } catch (...) { errors[worker]=std::current_exception(); } });
    for (auto& thread:threads) thread.join(); for (const auto& error:errors) if (error) std::rethrow_exception(error);
    Stats total; for (const auto& stats:results) total.append(stats);
    std::uint64_t patterned=0; for (auto n:total.patterns) patterned+=n;
    check(total.scenes==count && patterned==count && total.decisions==2*count+total.pending_commands
          && total.pending_commands==total.responses+total.purchase_commands && total.old_token_checks==total.copies
          && total.max_copies<=players*visible_hand_capacity && total.chance_hits<=total.chance_rolls
          && total.terminated+total.truncated<=count,"item shuffle benchmark accounting failed");
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"ruleset="<<match_ruleset<<"\nscope=experimental:external-explicit295-permutations-then-one-ordinary-action;private-manifest-and-world-slot-hashes-plus-full-restore-audit;not-server-selector-or-rng-distribution"
             <<"\nitem_shuffle_rule="<<item_shuffle_rule<<"\nconfusion_rule="<<confusion_rule<<"\nfate_rule="<<fate_rule<<"\nscenes="<<total.scenes<<"\nplayers="<<players<<"\nseed="<<seed<<"\nworkers="<<workers;
#define OUTPUT(name) std::cout<<"\n" #name "="<<total.name
    OUTPUT(decisions); OUTPUT(pending_commands); OUTPUT(responses); OUTPUT(purchase_commands); OUTPUT(terminated); OUTPUT(truncated);
    OUTPUT(copies); OUTPUT(empty_scenes); OUTPUT(max_copies); OUTPUT(dead_input_scenes); OUTPUT(used_copies); OUTPUT(unused_miracles);
    OUTPUT(masked_preserved); OUTPUT(masked_cleared); OUTPUT(unmasked_to_dream); OUTPUT(old_token_checks);
    OUTPUT(chance_rolls); OUTPUT(chance_hits); OUTPUT(chance_forced); OUTPUT(counter_created); OUTPUT(counter_started);
    OUTPUT(death_bows); OUTPUT(death_created); OUTPUT(death_started); OUTPUT(revivals); OUTPUT(redraws);
    OUTPUT(guardian_opportunities); OUTPUT(auto_guardian_actions); OUTPUT(departure_rolls); OUTPUT(departures); OUTPUT(disease_ticks);
    OUTPUT(primitive_checksum); OUTPUT(manifest_checksum); OUTPUT(checksum);
#undef OUTPUT
    for (std::size_t i=0;i<total.patterns.size();++i) std::cout<<"\npattern"<<i<<"_scenes="<<total.patterns[i];
    std::cout<<"\nseconds="<<seconds<<"\ndecisions_per_second="<<(seconds?total.decisions/seconds:0)<<'\n';
    return 0;
} catch (const std::exception& error) { std::cerr<<"item shuffle benchmark: "<<error.what()<<'\n'; return 1; } }
