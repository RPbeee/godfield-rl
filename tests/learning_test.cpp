#include "godfield/learning.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using namespace godfield::learning;
namespace legacy = godfield::legacy;
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL learning: " << message << '\n'; std::exit(1); } }
template<class F> void throws(F&& f, const char* message) { bool caught = false; try { f(); } catch (const std::exception&) { caught = true; } check(caught,message); }
struct Temporary {
    std::filesystem::path path = std::filesystem::temp_directory_path() / ("godfield-learning-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Temporary() { std::filesystem::create_directories(path); }
    ~Temporary() { std::error_code ignored; std::filesystem::remove_all(path,ignored); }
};
std::vector<unsigned char> read(const std::filesystem::path& p) {
    std::ifstream f(p,std::ios::binary); check(bool(f),"open checkpoint bytes"); return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
void write(const std::filesystem::path& p, const std::vector<unsigned char>& b) {
    std::ofstream f(p,std::ios::binary|std::ios::trunc); f.write(reinterpret_cast<const char*>(b.data()),static_cast<std::streamsize>(b.size())); check(bool(f),"write deliberate malformed checkpoint");
}
void integer(std::vector<unsigned char>& b, std::size_t offset, std::uint64_t value) { check(offset+8<=b.size(),"test mutation in bounds"); for (int n=0;n<8;++n) b[offset+n] = static_cast<unsigned char>(value>>(8*n)); }
void reseal(std::vector<unsigned char>& b) { std::uint64_t h=14695981039346656037ULL; for (std::size_t i=0;i+8<b.size();++i) { h^=b[i]; h*=1099511628211ULL; } integer(b,b.size()-8,h); }
legacy::Env fixture() {
    legacy::Env env(environment_config()); auto s=env.snapshot(); s.state=legacy::State{};
    const std::array<legacy::CardId,9> ids{5,120,198,202,3,4,238,197,239}; auto& p=s.state.players[0];
    std::copy(ids.begin(),ids.end(),p.hand.begin()); p.hand_count=ids.size(); p.mp=40;
    s.state.players[1].hand[0]=120; s.state.players[1].hand[1]=196; s.state.players[1].hand_count=2;
    check(env.restore(s),"restore deterministic learning candidate fixture"); return env;
}

void configuration_and_fixed_seat_rewards() {
    const auto cfg=environment_config(73); check(cfg.max_decisions==73 && cfg.initial_hand==10 && cfg.miracles && cfg.elements && cfg.chance && cfg.reflection
        && cfg.disease && cfg.harms && cfg.celestial && cfg.offensive_miracles && cfg.defensive_miracles && cfg.attribute_change && cfg.attribute_removal
        && cfg.counters && cfg.counter_extensions && cfg.money_counter && cfg.magic_reflection && cfg.any_reflection && cfg.mana_weapon && cfg.magic_block
        && cfg.additive_miracles && cfg.doubling_miracles && cfg.wide_miracles && cfg.magic_free && cfg.forgetting && cfg.exchanges && cfg.revival && cfg.item_removal
        && cfg.hp_absorption && cfg.self_damage && cfg.pestle_mortar && cfg.selling && cfg.buying && cfg.hp_gamble && cfg.magic_bounce && cfg.sundry_boost
        && cfg.status_miracles && cfg.dying_weapon, "training freezes all37 opt-in flags and requested decision limit");
    check(environment_config().max_decisions==512 && std::string_view(legacy::Env(cfg).profile())==legacy::dying_weapon_ruleset,"v37/512 default learning environment");
    check(initialize()==initialize() && initialize().config==TrainConfig{},"initial policy, optimizer, RNG and counters deterministic");
    for (int error=0;error<13;++error) {
        auto c=TrainConfig{};
        if(error==0)c.candidates=0; if(error==1)c.candidates=max_candidates+1; if(error==2)c.batch_size=0; if(error==3)c.snapshot_interval=0;
        if(error==4)c.max_decisions=0; if(error==5)c.opponent=static_cast<Opponent>(2); if(error==6)c.learning_rate=-.1; if(error==7)c.value_learning_rate=0;
        if(error==8)c.entropy=-.1; if(error==9)c.gamma=1.001; if(error==10)c.gamma=0; if(error==11)c.clip_norm=0; if(error==12)c.learning_rate=std::numeric_limits<double>::quiet_NaN();
        throws([&]{validate_config(c);},"invalid training parameters rejected before stepping");
    }
    legacy::StepResult r; r.terminated=true; r.actor=1; r.rewards={1,-1}; check(terminal_reward(r,0)==1 && terminal_reward(r,1)==-1,"terminal reward follows fixed seat, not responding actor");
    r.actor=0; check(terminal_reward(r,0)==1 && terminal_reward(r,1)==-1,"actor change cannot flip fixed-seat reward");
    r.rewards={0,0}; check(terminal_reward(r,0)==0 && terminal_reward(r,1)==0,"simultaneous death draw scores zero");
    r.terminated=false; r.truncated=true; r.rewards={1,-1}; check(terminal_reward(r,0)==0 && terminal_reward(r,1)==0,"truncation is excluded from terminal reward");
    throws([&]{terminal_reward(r,2);},"invalid fixed seat rejected");
}

double objective(const Policy& p, const Decision& d, double advantage, double entropy) {
    const auto probabilities_here=probabilities(p,d.candidates); double h=0; for(double x:probabilities_here) if(x>0)h-=x*std::log(x);
    return advantage*std::log(probabilities_here[d.selected])+entropy*h;
}
void finite_difference_policy_and_entropy() {
    Decision d; d.candidates.resize(3); d.selected=1; Policy policy;
    for(std::size_t j=0;j<d.candidates.size();++j) for(std::size_t i=0;i<policy_feature_count;++i)
        d.candidates[j].features[i]=std::sin((j+1.)*(i+1.)*.17)*.4 + (j==1 ? .1 : -.03);
    for(std::size_t i=0;i<policy_feature_count;++i)policy.weights[i]=std::cos(i*.41)*.03;
    for(double advantage:{-.7,0.,.9}) for(double entropy:{0.,.13}) {
        d.probabilities=probabilities(policy,d.candidates); const auto g=policy_gradient(d,advantage,entropy);
        for(std::size_t i=0;i<policy_feature_count;++i) {
            auto plus=policy,minus=policy; constexpr double epsilon=1e-6; plus.weights[i]+=epsilon; minus.weights[i]-=epsilon;
            const double numeric=(objective(plus,d,advantage,entropy)-objective(minus,d,advantage,entropy))/(2*epsilon);
            check(std::abs(g[i]-numeric)<2e-7,"policy ascent derivative including entropy agrees with independent central finite difference");
        }
    }
    auto bad=d; bad.probabilities[0]+=.1; throws([&]{policy_gradient(bad,1,.1);},"unnormalized probabilities cannot silently produce gradient");
    Policy large; large.weights.fill(10000); const auto p=probabilities(large,d.candidates);
    check(std::abs(std::accumulate(p.begin(),p.end(),0.)-1)<1e-12 && std::all_of(p.begin(),p.end(),[](double x){return std::isfinite(x)&&x>=0&&x<=1;}),"softmax stable at large finite logits");
}

void candidates_privacy_and_nonmutation() {
    auto env=fixture(); const auto before=env.snapshot(); const auto observation=env.observe(); std::uint64_t rng=12345,other_rng=rng; auto candidates=candidate_actions(env,rng,12);
    check(env.snapshot()==before && !candidates.empty() && candidates.size()<=12,"candidate generation changes policy RNG only, never environment/checkpoint/tapes");
    for(std::size_t i=0;i<candidates.size();++i) {
        check(env.legal(candidates[i].action) && candidates[i].features==action_features(env.observe(),candidates[i].action,env.state().actor),"every candidate legal and features derive only from observation+action+actor");
        for(std::size_t j=0;j<i;++j)check(!(candidates[i].action==candidates[j].action),"duplicate physical Action removed from candidate set");
        for(double value:candidates[i].features)check(std::isfinite(value),"candidate features finite");
    }
    auto hidden=before; hidden.state.players[1].hand[0]=244; hidden.state.players[1].hand[1]=117; hidden.state.players[1].status_abilities[0]=7;
    hidden.scripted_hits[0]=100; hidden.hit_count=1; check(env.restore(hidden) && env.observe()==observation,"enemy hand/retained identities and future hit tape are private");
    const auto hidden_before=env.snapshot(); const auto hidden_candidates=candidate_actions(env,other_rng,12);
    check(hidden_candidates==candidates && other_rng==rng && env.snapshot()==hidden_before,"private state cannot alter policy candidates/features or proposal RNG use");
    Model model; model.policy.weights[0]=.3; model.value.fill(.02); std::uint64_t one=456,two=one;
    const auto d=decide(env,model,one,12),e=decide(env,model,two,12);
    check(d.candidates==e.candidates && d.probabilities==e.probabilities && d.selected==e.selected && one==two
        && d.actor==env.state().actor && d.selected<d.candidates.size() && env.legal(d.candidates[d.selected].action)
        && d.value_features==state_features(observation) && std::isfinite(d.value) && env.snapshot()==hidden_before,"stochastic decision deterministic and environment-pure");
    auto own_observation=observation; own_observation[0]=.17f;
    check(state_features(own_observation)!=state_features(observation),"state baseline features depend on observed public/own resources");
    check(action_features(observation,{legacy::ActionKind::use,0,0,0},0)==action_features(observation,{legacy::ActionKind::use,0,0,1},1),
        "equivalent self-target actions use actor-relative features, never absolute seat labels");
    throws([&]{candidate_actions(env,one,0);},"empty candidate request rejected");
    throws([&]{candidate_actions(env,one,max_candidates+1);},"oversized candidate request rejected");
    own_observation[0]=std::numeric_limits<float>::quiet_NaN();
    throws([&]{state_features(own_observation);},"nonfinite observation rejected by baseline");
    throws([&]{action_features(own_observation,candidates[0].action,0);},"nonfinite observation rejected by policy features");
}

void exact_checkpoint_resume_evaluation_and_truncation() {
    Temporary temporary; TrainConfig cfg; cfg.seed=123; cfg.candidates=6; cfg.batch_size=8; cfg.snapshot_interval=16;
    auto direct=initialize(cfg),split=initialize(cfg); train(split,37);
    check(split.episodes==37 && split.updates==4 && split.pending_episodes==5 && split.pending_decisions>0,"mid-batch37 retains gradients and RNG after snapshot32");
    const auto path=temporary.path/"nested"/"checkpoint.gflearn"; save_checkpoint(split,path); auto resumed=load_checkpoint(path); check(resumed==split,"all optimizer moments, gradients, opponent, RNG and mid-batch state roundtrip");
    train(resumed,59); train(direct,96); check(resumed==direct && direct.episodes==96 && direct.updates==12 && direct.pending_episodes==0 && direct.pending_decisions==0
        && direct.model.policy==direct.opponent && direct.learned_decisions>0 && direct.model!=initialize(cfg).model,"37+59 resume exactly equals96 uninterrupted episodes across batch and frozen-opponent updates");
    const auto unchanged=direct; for(bool greedy:{false,true}) {
        const auto a=evaluate(direct,16,900,greedy),b=evaluate(direct,16,900,greedy); check(a==b && direct==unchanged && a.games==16
            && a.wins+a.losses+a.draws+a.truncations==a.games && a.decisions>0,"evaluation deterministic, seat balanced, and cannot modify training checkpoint or RNG");
    }
    const auto bytes=read(path);
    check(bytes.size()>200 && std::distance(std::filesystem::directory_iterator(path.parent_path()),std::filesystem::directory_iterator{})==1,
        "checkpoint save leaves only its target file, with no random temporary reservation directories or files");
    for(int error=0;error<7;++error) {
        auto bad=bytes;
        if(error==0)bad[0]^=1;
        if(error==1){integer(bad,8,checkpoint_version+1);reseal(bad);}
        if(error==2){bad[16]^=1;reseal(bad);}
        if(error==3)bad[bad.size()/2]^=1;
        if(error==4)bad.resize(bad.size()/2);
        if(error==5)bad.push_back(0);
        if(error==6){integer(bad,112,0x7ff8000000000000ULL);reseal(bad);}
        const auto invalid=temporary.path/"invalid.gflearn"; write(invalid,bad); throws([&]{load_checkpoint(invalid);},"bad magic/version/fingerprint/checksum/truncation/trailing/nonfinite state rejected");
        check(read(path)==bytes,"failed load cannot alter valid original checkpoint");
    }
    auto malformed=initialize(); malformed.policy_gradient[0]=1; throws([&]{save_checkpoint(malformed,path);},"gradient without pending decisions rejected");
    malformed=initialize(); malformed.pending_decisions=malformed.learned_decisions=malformed.decisions=1;
    throws([&]{save_checkpoint(malformed,path);},"pending samples with zero pending episodes rejected");
    for(int error=0;error<5;++error) {
        auto impossible=initialize();
        if(error==0)impossible.decisions=UINT64_MAX;
        if(error==1) { impossible.episodes=2; impossible.pending_episodes=2; impossible.truncations=2; impossible.decisions=1; }
        if(error==2) { impossible.episodes=1; impossible.pending_episodes=1; impossible.truncations=1; impossible.decisions=513; }
        if(error==3) { impossible.episodes=65; impossible.updates=1; impossible.pending_episodes=1; impossible.wins=65;
            impossible.decisions=impossible.learned_decisions=10000; impossible.pending_decisions=513; impossible.policy_gradient[0]=1; }
        if(error==4) { impossible.episodes=64; impossible.updates=1; impossible.wins=64; impossible.decisions=64; impossible.optimizer_steps=1; }
        throws([&]{save_checkpoint(impossible,path);},"impossible total/pending decision counts and optimizer steps rejected before checkpoint replacement");
    }
    check(read(path)==bytes,"failed save validates before replacing an existing valid checkpoint");
    auto overflow_counter=initialize(); overflow_counter.episodes=UINT64_MAX/overflow_counter.config.max_decisions+1;
    overflow_counter.updates=overflow_counter.episodes/overflow_counter.config.batch_size;
    overflow_counter.pending_episodes=overflow_counter.episodes%overflow_counter.config.batch_size;
    overflow_counter.wins=overflow_counter.episodes; overflow_counter.decisions=UINT64_MAX-255;
    const auto counter_before=overflow_counter;
    throws([&]{train_episode(overflow_counter);},"decision accumulation overflow rejected before beginning an episode");
    check(overflow_counter==counter_before,"counter overflow rejection leaves model, RNG and pending batch unchanged");
    auto truncated_cfg=cfg; truncated_cfg.max_decisions=1; truncated_cfg.batch_size=1; truncated_cfg.snapshot_interval=1; auto truncated=initialize(truncated_cfg);
    std::uint64_t excluded=0; for(int n=0;n<20;++n) {
        const auto old=truncated; const auto r=train_episode(truncated);
        if(r.truncated) { ++excluded; check(!r.terminated && !r.reward && !r.learning_decisions && truncated.model==old.model && truncated.policy_m==old.policy_m
            && truncated.policy_v==old.policy_v && truncated.value_m==old.value_m && truncated.value_v==old.value_v && truncated.optimizer_steps==old.optimizer_steps
            && truncated.learned_decisions==old.learned_decisions && !truncated.pending_decisions && truncated.truncations==old.truncations+1,
            "truncated trajectory excluded from return, baseline/policy gradients and optimizer moments even at batch boundary"); }
        check(truncated.updates==truncated.episodes && truncated.pending_episodes==0,"empty batches count separately from actual Adam optimizer steps");
    }
    check(excluded,"short-horizon test actually visits excluded truncations");
    throws([&]{evaluate(direct,2,UINT64_MAX);},"evaluation seed overflow rejected");
    auto overflow=initialize(cfg); throws([&]{train(overflow,UINT64_MAX);},"training episode/seed overflow rejected before work");
}
}
int main() {
    configuration_and_fixed_seat_rewards(); finite_difference_policy_and_entropy(); candidates_privacy_and_nonmutation(); exact_checkpoint_resume_evaluation_and_truncation();
    std::cout << "Learning passed (entropy finite differences/privacy/legal proposals/midbatch resume/checkpoint corruption/evaluation/truncate exclusion).\n";
}
