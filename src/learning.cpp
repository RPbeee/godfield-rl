#include "godfield/learning.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>

namespace godfield::learning {
namespace {
static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559);
constexpr std::uint64_t fnv_offset = 14695981039346656037ULL;
std::uint64_t random(std::uint64_t& state) {
    auto z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}
double uniform(std::uint64_t& state) { return static_cast<double>(random(state) >> 11) * 0x1.0p-53; }
void mix(std::uint64_t& h, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) { h ^= (v >> (8*i)) & 255; h *= 1099511628211ULL; }
}
void mix_action(std::uint64_t& h, const legacy::Action& a) {
    mix(h,static_cast<unsigned>(a.kind)); mix(h,a.base); mix(h,a.slots); mix(h,a.target);
    mix(h,a.counter_count); mix(h,a.counter_order); mix(h,a.extra_count);
    for(unsigned i=0;i<a.extra_count;++i)mix(h,a.extras[i]);
}
template<std::size_t N> double dot(const std::array<double,N>& a, const std::array<double,N>& b) {
    return std::inner_product(a.begin(),a.end(),b.begin(),0.0);
}
template<std::size_t N> void finite(const std::array<double,N>& a) {
    for (double v : a) if (!std::isfinite(v)) throw std::invalid_argument("nonfinite model/feature");
}
void validate(const Checkpoint& c) {
    validate_config(c.config);
    finite(c.model.policy.weights); finite(c.model.value); finite(c.opponent.weights);
    finite(c.policy_m); finite(c.policy_v); finite(c.policy_gradient);
    finite(c.value_m); finite(c.value_v); finite(c.value_gradient);
    for (double v : c.policy_v) if (v < 0) throw std::invalid_argument("negative Adam variance");
    for (double v : c.value_v) if (v < 0) throw std::invalid_argument("negative Adam variance");
    if (c.optimizer_steps > c.updates || c.updates != c.episodes / c.config.batch_size || c.pending_episodes != c.episodes % c.config.batch_size
        || c.wins > c.episodes || c.losses > c.episodes-c.wins || c.draws > c.episodes-c.wins-c.losses
        || c.truncations != c.episodes-c.wins-c.losses-c.draws || c.learned_decisions > c.decisions
        || c.pending_decisions > c.learned_decisions || c.config.seed > UINT64_MAX-c.episodes)
        throw std::invalid_argument("inconsistent checkpoint counters");
    if (c.decisions < c.episodes ||
        (c.episodes <= UINT64_MAX/c.config.max_decisions && c.decisions > c.episodes*c.config.max_decisions) ||
        c.pending_decisions > std::uint64_t(c.pending_episodes)*c.config.max_decisions ||
        c.optimizer_steps > c.learned_decisions)
        throw std::invalid_argument("impossible checkpoint decision counters");
    if ((!c.pending_episodes && c.pending_decisions) || (!c.pending_decisions &&
        (std::any_of(c.policy_gradient.begin(),c.policy_gradient.end(),[](double x){return x!=0;}) ||
         std::any_of(c.value_gradient.begin(),c.value_gradient.end(),[](double x){return x!=0;}))))
        throw std::invalid_argument("inconsistent pending gradient");
}
void update(Checkpoint& c) {
    ++c.updates;
    if (c.pending_decisions) {
        ++c.optimizer_steps;
        const double divisor = static_cast<double>(c.pending_decisions);
        double norm = 0;
        for (double g : c.policy_gradient) norm += (g/divisor)*(g/divisor);
        for (double g : c.value_gradient) norm += (g/divisor)*(g/divisor);
        const double scale = std::min(1.,c.config.clip_norm/(std::sqrt(norm)+1e-30))/divisor;
        const double correction1 = 1.-std::pow(.9, static_cast<double>(c.optimizer_steps));
        const double correction2 = 1.-std::pow(.999, static_cast<double>(c.optimizer_steps));
        auto adam = [&](auto& w, auto& m, auto& v, auto& gradient, double lr) {
            for (std::size_t i=0;i<w.size();++i) {
                double g=gradient[i]*scale;
                m[i]=.9*m[i]+.1*g; v[i]=.999*v[i]+.001*g*g;
                w[i]+=lr*(m[i]/correction1)/(std::sqrt(v[i]/correction2)+1e-8);
            }
        };
        adam(c.model.policy.weights,c.policy_m,c.policy_v,c.policy_gradient,c.config.learning_rate);
        adam(c.model.value,c.value_m,c.value_v,c.value_gradient,c.config.value_learning_rate);
    }
    c.policy_gradient.fill(0); c.value_gradient.fill(0);
    c.pending_decisions=0; c.pending_episodes=0;
}
struct Writer {
    std::vector<unsigned char> bytes;
    void integer(std::uint64_t v) { for(int i=0;i<8;++i) bytes.push_back(static_cast<unsigned char>(v>>(8*i))); }
    void real(double v) { integer(std::bit_cast<std::uint64_t>(v)); }
    template<std::size_t N> void array(const std::array<double,N>& a) { for(double v:a) real(v); }
};
struct Reader {
    const std::vector<unsigned char>& bytes; std::size_t pos=0;
    std::uint64_t integer() {
        if (bytes.size()-pos<8) throw std::invalid_argument("short checkpoint");
        std::uint64_t v=0; for(int i=0;i<8;++i) v|=std::uint64_t(bytes[pos++])<<(8*i); return v;
    }
    std::uint32_t small() {auto v=integer(); if(v>UINT32_MAX) throw std::invalid_argument("checkpoint integer overflow");return static_cast<std::uint32_t>(v);}
    double real(){return std::bit_cast<double>(integer());}
    template<std::size_t N> void array(std::array<double,N>& a){for(double& v:a)v=real();}
};
constexpr char fingerprint[]="legacy-dying-weapon-subset-v37/cards231/weight485/obs1368/action88/features1/256/32";
std::uint64_t fingerprint_hash() { std::uint64_t h=fnv_offset; for(unsigned char ch:std::string(fingerprint)){h^=ch;h*=1099511628211ULL;}return h; }
}
legacy::Config environment_config(std::uint32_t limit) {
    return legacy::Config{limit,10,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true};
}
void validate_config(const TrainConfig& c) {
    if (!c.candidates || c.candidates>max_candidates || !c.batch_size || c.batch_size>1000000
        || !c.snapshot_interval || !c.max_decisions || c.max_decisions>1000000
        || !std::isfinite(c.learning_rate) || c.learning_rate<=0 || c.learning_rate>1
        || !std::isfinite(c.value_learning_rate) || c.value_learning_rate<=0 || c.value_learning_rate>1
        || !std::isfinite(c.entropy) || c.entropy<0 || c.entropy>10
        || !std::isfinite(c.gamma) || c.gamma<=0 || c.gamma>1
        || !std::isfinite(c.clip_norm) || c.clip_norm<=0 || c.clip_norm>1000
        || (c.opponent!=Opponent::random && c.opponent!=Opponent::snapshot))
        throw std::invalid_argument("invalid training config");
}
Checkpoint initialize(TrainConfig config) {
    validate_config(config); Checkpoint c; c.config=config; c.rng=config.seed^0x6a09e667f3bcc909ULL; c.checksum=fnv_offset; return c;
}
PolicyFeatures action_features(const legacy::Observation& o, legacy::Action a, std::uint8_t actor) {
    if(actor>1 || static_cast<unsigned>(a.kind)>19 || a.extra_count>a.extras.size()) throw std::invalid_argument("invalid action features");
    PolicyFeatures f{};
    unsigned k=static_cast<unsigned>(a.kind); f[k]=1;
    for(unsigned i=0;i<6;++i)f[20+k*6+i]=o[i];
    f[140+k]=o[8]; f[160+k]=o[0]-o[3];
    bool self=a.target==actor, enemy=a.target==(actor^1);
    f[180]=self;f[181]=enemy;
    std::array<double,7> base{},extras{};
    bool physical=(k==1||k==2||k==3||(k>=7&&k<=9)||k==11||k==12||k==14||k==15||k==16);
    if(physical && a.base<16) {
        for(unsigned j=0;j<7;++j)base[j]=o[16+a.base*7+j];
        f[228]=o[285+a.base];f[229]=o[186+a.base];f[230]=o[132+a.base];f[231]=o[148+a.base];
        f[232]=o[318+a.base];
    }
    unsigned count=0,held=0;
    auto add=[&](unsigned slot){if(slot<16){++count;for(unsigned j=0;j<7;++j)extras[j]+=o[16+slot*7+j];}else ++held;};
    for(unsigned slot=0;slot<16;++slot)if(a.slots&(1u<<slot))add(slot);
    if(k==11||k==12||k==13)for(unsigned i=0;i<a.extra_count;++i)add(a.extras[i]);
    for(unsigned j=0;j<7;++j){f[182+j]=base[j];f[189+j]=extras[j];f[196+j]=base[j]*enemy;f[203+j]=base[j]*self;f[210+j]=extras[j]*enemy;f[217+j]=extras[j]*self;}
    f[224]=count/16.;f[225]=held/65.;f[226]=a.extra_count/65.;f[227]=a.counter_count/16.;
    f[233]=(base[1]+extras[2])*enemy;f[234]=(base[1]+extras[2])*self;
    f[235]=(base[3]+extras[3])*o[8];f[236]=(base[3]+extras[3])*(1-o[0]);
    f[237]=(base[4]+extras[4])*(1-o[0])*self;f[238]=(base[4]+extras[4])*(1-o[3])*enemy;
    f[239]=(base[5]+extras[5])*(1-o[1])*self;f[240]=f[231]*o[1];
    f[241]=f[232]*enemy;f[242]=f[232]*self;f[243]=f[232]*o[3]*enemy;
    f[244]=k==5?a.base/15.:0;f[245]=k==6?a.base+1.:0;f[246]=k==10?a.base+1.:0;f[247]=k==19?(a.base+1.)/5.:0;
    f[248]=k==12 && a.base>=16?(a.base+1.)/53.:0;
    if(k==14 && a.extra_count==3){f[249]=a.extras[0]/99.-o[0];f[250]=a.extras[1]/99.-o[1];f[251]=a.extras[2]/99.-o[2];}
    f[252]=o[6];f[253]=o[7];f[254]=o[10];f[255]=o[1016];
    for(double& v:f){if(!std::isfinite(v))throw std::invalid_argument("nonfinite observation");v=std::clamp(v,-4.,4.);}
    return f;
}
ValueFeatures state_features(const legacy::Observation& o) {
    ValueFeatures f{};f[0]=1;
    for(unsigned i=0;i<12;++i)f[1+i]=o[i];
    for(unsigned s=0;s<16;++s){if(o[16+s*7]!=0)f[20]+=1./16;for(unsigned j=0;j<7;++j)f[13+j]+=o[16+s*7+j]/16.;}
    for(unsigned j=0;j<5;++j){f[21+j]=o[236+j];f[26+j]=o[241+j];}f[31]=o[1016];
    for(double& v:f){if(!std::isfinite(v))throw std::invalid_argument("nonfinite observation");v=std::clamp(v,-4.,4.);}return f;
}
std::vector<Candidate> candidate_actions(const legacy::Env& env,std::uint64_t& rng,std::uint32_t count) {
    if(!count||count>max_candidates||env.done())throw std::invalid_argument("invalid candidate request");
    std::vector<Candidate> result;result.reserve(count); const auto o=env.observe();
    for(unsigned i=0;i<count*8 && result.size()<count;++i){
        auto a=env.sample_action(rng);
        if(!env.legal(a))throw std::logic_error("proposal was illegal");
        if(std::none_of(result.begin(),result.end(),[&](const Candidate& c){return c.action==a;}))result.push_back({a,action_features(o,a,env.state().actor)});
    }
    if(result.empty())throw std::logic_error("no legal candidate");return result;
}
std::vector<double> probabilities(const Policy& policy,const std::vector<Candidate>& candidates) {
    finite(policy.weights);if(candidates.empty())throw std::invalid_argument("empty policy candidates");
    std::vector<double> p; p.reserve(candidates.size());
    for(const auto& c:candidates){finite(c.features);double v=dot(policy.weights,c.features);if(!std::isfinite(v))throw std::invalid_argument("policy logit overflow");p.push_back(v);}
    double high=*std::max_element(p.begin(),p.end()),sum=0;
    for(double& v:p){v=std::exp(v-high);sum+=v;}for(double& v:p)v/=sum;return p;
}
PolicyFeatures policy_gradient(const Decision& d,double advantage,double entropy_coefficient) {
    if(d.candidates.empty()||d.probabilities.size()!=d.candidates.size()||d.selected>=d.candidates.size()||!std::isfinite(advantage)||!std::isfinite(entropy_coefficient))throw std::invalid_argument("invalid policy gradient");
    double h=0,sum=0;for(double p:d.probabilities){if(!std::isfinite(p)||p<0||p>1)throw std::invalid_argument("invalid probability");sum+=p;if(p>0)h-=p*std::log(p);}
    if(std::abs(sum-1)>1e-9)throw std::invalid_argument("unnormalized probability");
    PolicyFeatures g{};
    for(std::size_t j=0;j<d.candidates.size();++j){double p=d.probabilities[j];double coefficient=advantage*((j==d.selected?1.:0)-p);if(p>0)coefficient-=entropy_coefficient*p*(std::log(p)+h);for(std::size_t i=0;i<g.size();++i)g[i]+=coefficient*d.candidates[j].features[i];}finite(g);return g;
}
Decision decide(const legacy::Env& env,const Model& model,std::uint64_t& rng,std::uint32_t count,bool greedy) {
    Decision d;d.actor=env.state().actor;d.candidates=candidate_actions(env,rng,count);d.probabilities=probabilities(model.policy,d.candidates);d.value_features=state_features(env.observe());finite(model.value);d.value=std::tanh(dot(model.value,d.value_features));
    if(greedy)d.selected=std::max_element(d.probabilities.begin(),d.probabilities.end())-d.probabilities.begin();
    else {double u=uniform(rng),cumulative=0;d.selected=d.probabilities.size()-1;for(std::size_t j=0;j<d.probabilities.size();++j){cumulative+=d.probabilities[j];if(u<cumulative){d.selected=j;break;}}}return d;
}
double terminal_reward(const legacy::StepResult& r,std::uint8_t seat){if(seat>1)throw std::invalid_argument("invalid seat");return r.terminated?r.rewards[seat]:0.;}
EpisodeResult train_episode(Checkpoint& c) {
    validate(c);if(c.episodes==UINT64_MAX || c.config.seed==UINT64_MAX-c.episodes)throw std::overflow_error("episode seed overflow");
    if(c.decisions>UINT64_MAX-c.config.max_decisions)throw std::overflow_error("decision counter overflow");
    legacy::Env env(environment_config(c.config.max_decisions));env.reset(c.config.seed+c.episodes);
    std::uint64_t opponent_rng=(c.config.seed+c.episodes)^0xbb67ae8584caa73bULL;
    EpisodeResult out;out.learner_seat=c.episodes%2;
    struct Sample {Decision decision;std::uint32_t time;};std::vector<Sample> trajectory;
    legacy::StepResult result;
    while(!env.done()) {
        legacy::Action a;
        if(env.state().actor==out.learner_seat){auto d=decide(env,c.model,c.rng,c.config.candidates);a=d.candidates[d.selected].action;trajectory.push_back({std::move(d),out.decisions});}
        else if(c.config.opponent==Opponent::random)a=env.sample_action(opponent_rng);
        else {Model frozen;frozen.policy=c.opponent;auto d=decide(env,frozen,opponent_rng,c.config.candidates);a=d.candidates[d.selected].action;}
        result=env.step(a);++out.decisions;
        mix_action(c.checksum,a);
    }
    out.terminated=result.terminated;out.truncated=result.truncated;out.reward=terminal_reward(result,out.learner_seat);
    if(out.terminated){
        if(out.reward>0)++c.wins;else if(out.reward<0)++c.losses;else ++c.draws;
        for(const auto& sample:trajectory){const auto& d=sample.decision;double ret=out.reward*std::pow(c.config.gamma,out.decisions-1-sample.time);auto g=policy_gradient(d,ret-d.value,c.config.entropy);for(std::size_t i=0;i<g.size();++i)c.policy_gradient[i]+=g[i];double error=(ret-d.value)*(1-d.value*d.value);for(std::size_t i=0;i<c.value_gradient.size();++i)c.value_gradient[i]+=error*d.value_features[i];}
        out.learning_decisions=static_cast<std::uint32_t>(trajectory.size());c.pending_decisions+=trajectory.size();c.learned_decisions+=trajectory.size();
    }else if(out.truncated)++c.truncations;else throw std::logic_error("done without episode boundary");
    c.decisions+=out.decisions;++c.episodes;++c.pending_episodes;
    mix(c.checksum,out.terminated);mix(c.checksum,std::bit_cast<std::uint64_t>(out.reward));
    if(c.pending_episodes==c.config.batch_size)update(c);
    if(c.episodes%c.config.snapshot_interval==0)c.opponent=c.model.policy;
    return out;
}
void train(Checkpoint& c,std::uint64_t n){validate(c);if(n>UINT64_MAX-c.episodes || c.config.seed>UINT64_MAX-c.episodes-n)throw std::overflow_error("episode count overflow");for(std::uint64_t i=0;i<n;++i)train_episode(c);}
Evaluation evaluate(const Checkpoint& c,std::uint64_t games,std::uint64_t seed,bool greedy){
    validate(c);if(games && seed>UINT64_MAX-(games-1))throw std::overflow_error("evaluation seed overflow");
    Evaluation e;e.checksum=fnv_offset;
    for(std::uint64_t i=0;i<games;++i){legacy::Env env(environment_config(c.config.max_decisions));env.reset(seed+i);std::uint64_t rng=(seed+i)^0x3c6ef372fe94f82bULL,opp=(seed+i)^0xa54ff53a5f1d36f1ULL;legacy::StepResult r;std::uint8_t seat=i%2;
        while(!env.done()){legacy::Action a;if(env.state().actor==seat){auto d=decide(env,c.model,rng,c.config.candidates,greedy);a=d.candidates[d.selected].action;}else a=env.sample_action(opp);r=env.step(a);++e.decisions;mix_action(e.checksum,a);}
        ++e.games;mix(e.checksum,r.terminated);mix(e.checksum,r.truncated);if(r.truncated)++e.truncations;else {double reward=terminal_reward(r,seat);if(reward>0)++e.wins;else if(reward<0)++e.losses;else ++e.draws;mix(e.checksum,std::bit_cast<std::uint64_t>(reward));}
    }return e;
}
void save_checkpoint(const Checkpoint& c,const std::filesystem::path& path){
    validate(c);if(path.empty())throw std::invalid_argument("empty checkpoint path");Writer w;
    w.integer(0x3130304e524c4647ULL);w.integer(checkpoint_version);w.integer(fingerprint_hash());
    const auto& v=c.config;w.integer(v.seed);w.integer(v.candidates);w.integer(v.batch_size);w.integer(v.snapshot_interval);w.integer(v.max_decisions);w.real(v.learning_rate);w.real(v.value_learning_rate);w.real(v.entropy);w.real(v.gamma);w.real(v.clip_norm);w.integer(static_cast<unsigned>(v.opponent));
    w.array(c.model.policy.weights);w.array(c.model.value);w.array(c.opponent.weights);w.array(c.policy_m);w.array(c.policy_v);w.array(c.policy_gradient);w.array(c.value_m);w.array(c.value_v);w.array(c.value_gradient);
    for(auto v:{c.episodes,c.updates,c.optimizer_steps,c.rng,c.pending_decisions,c.wins,c.losses,c.draws,c.truncations,c.decisions,c.learned_decisions,c.checksum})w.integer(v);w.integer(c.pending_episodes);
    std::uint64_t h=fnv_offset;for(auto b:w.bytes){h^=b;h*=1099511628211ULL;}w.integer(h);
    if(!path.parent_path().empty())std::filesystem::create_directories(path.parent_path());
    std::filesystem::path directory;
    std::random_device entropy;
    for(unsigned attempt=0;attempt<100;++attempt){
        directory=path;directory+=".tmp."+std::to_string(entropy())+"."+std::to_string(attempt);
        if(std::filesystem::create_directory(directory))break;
        if(attempt==99)throw std::runtime_error("cannot reserve checkpoint temporary directory");
    }
    auto tmp=directory/"checkpoint";
    try{std::ofstream f(tmp,std::ios::binary|std::ios::trunc);f.exceptions(std::ios::badbit|std::ios::failbit);f.write(reinterpret_cast<const char*>(w.bytes.data()),static_cast<std::streamsize>(w.bytes.size()));f.close();std::filesystem::rename(tmp,path);std::filesystem::remove(directory);}catch(...){std::error_code ignored;std::filesystem::remove(tmp,ignored);std::filesystem::remove(directory,ignored);throw;}
}
Checkpoint load_checkpoint(const std::filesystem::path& path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)throw std::runtime_error("cannot open checkpoint");auto size=f.tellg();if(size<32||size>1000000)throw std::invalid_argument("invalid checkpoint size");std::vector<unsigned char>b(static_cast<std::size_t>(size));f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),size))throw std::runtime_error("cannot read checkpoint");
    Reader r{b};if(r.integer()!=0x3130304e524c4647ULL||r.integer()!=checkpoint_version||r.integer()!=fingerprint_hash())throw std::invalid_argument("checkpoint format/rules/features mismatch");
    Checkpoint c;auto& v=c.config;v.seed=r.integer();v.candidates=r.small();v.batch_size=r.small();v.snapshot_interval=r.small();v.max_decisions=r.small();v.learning_rate=r.real();v.value_learning_rate=r.real();v.entropy=r.real();v.gamma=r.real();v.clip_norm=r.real();v.opponent=static_cast<Opponent>(r.small());
    r.array(c.model.policy.weights);r.array(c.model.value);r.array(c.opponent.weights);r.array(c.policy_m);r.array(c.policy_v);r.array(c.policy_gradient);r.array(c.value_m);r.array(c.value_v);r.array(c.value_gradient);
    for(auto p:{&c.episodes,&c.updates,&c.optimizer_steps,&c.rng,&c.pending_decisions,&c.wins,&c.losses,&c.draws,&c.truncations,&c.decisions,&c.learned_decisions,&c.checksum})*p=r.integer();c.pending_episodes=r.small();
    auto end=r.pos;auto expected=r.integer();if(r.pos!=b.size())throw std::invalid_argument("trailing checkpoint data");std::uint64_t h=fnv_offset;for(std::size_t i=0;i<end;++i){h^=b[i];h*=1099511628211ULL;}if(h!=expected)throw std::invalid_argument("checkpoint checksum mismatch");validate(c);return c;
}
}
