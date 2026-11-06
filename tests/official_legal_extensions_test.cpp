#include "godfield/official_match.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

using namespace godfield::official;
namespace {
void check(bool ok,const char* message) { if(!ok) { std::cerr<<"FAIL selection: "<<message<<'\n';std::exit(1); } }
auto key(const Command& c) {
    return std::tuple{c.kind,c.items,c.target,c.allocation ? c.allocation->hp : -1,
        c.allocation ? c.allocation->mp : -1,c.allocation ? c.allocation->cp : -1};
}
void sort(std::vector<Command>& commands) { std::sort(commands.begin(),commands.end(),[](const auto& a,const auto& b){return key(a)<key(b);}); }
const PlayerState& own(const MatchSnapshot& s) {
    return *std::find_if(s.state.players.begin(),s.state.players.end(),[&](const auto& p){return p.id==*s.actor;});
}
const ItemInstance& item(const MatchSnapshot& s,ItemToken token) {
    const auto& p=own(s);return *std::find_if(p.items.begin(),p.items.end(),[&](const auto& i){return i.token==token;});
}
std::size_t scenes=0,full_scenes=0,steps=0,exceptions=0,walks=0;
std::size_t hidden_pairs_excluded=0,leak_pairs=0,leak_nodes=0;
std::size_t raw_sum=0,canonical_sum=0,raw_max=0,canonical_max=0;
std::map<MatchPhase,std::size_t> phases;
std::vector<Command> tree(const Match& game,std::size_t limit,bool canonical,int exchange_step=33) {
    const auto s=game.snapshot();std::vector<Command> out;
    for(auto kind:game.legal_kinds()) {
        Command c{*s.actor,kind,{},{}};
        const auto visit=[&](auto&& self)->void {
            const auto n=game.legal_extensions(kind,c.items,{canonical});
            check(n.can_finish || !n.next.empty(),"no dead ends");
            check(n.can_finish==!n.targets.empty(),"targets iff finish");
            check(std::is_sorted(n.targets.begin(),n.targets.end()),"target order");
            if(n.can_finish) for(auto target:n.targets) {
                c.target=target;
                if(n.exchange) {
                    check(kind==CommandKind::exchange,"exchange domain kind");
                    for(int hp=0;hp<=99;hp+=exchange_step) for(int mp=0;mp<=99;mp+=exchange_step) {
                        const auto cp=n.exchange->total-hp-mp;
                        if(cp>=0 && cp<=99) { c.allocation=ResourceAllocation{hp,mp,static_cast<int>(cp)};out.push_back(c); }
                    }
                    c.allocation.reset();
                } else out.push_back(c);
            }
            if(c.items.size()==limit) return;
            for(auto token:n.next) { c.items.push_back(token);self(self);c.items.pop_back(); }
        };
        visit(visit);
    }
    sort(out);return out;
}
void membership(const Match& game,const Command& c) {
    const auto kinds=game.legal_kinds();check(std::find(kinds.begin(),kinds.end(),c.kind)!=kinds.end(),"kind membership");
    std::vector<ItemToken> prefix;
    for(auto token:c.items) {
        auto n=game.legal_extensions(c.kind,prefix);check(std::find(n.next.begin(),n.next.end(),token)!=n.next.end(),"sample token membership");prefix.push_back(token);
    }
    auto n=game.legal_extensions(c.kind,prefix);
    check(n.can_finish && std::find(n.targets.begin(),n.targets.end(),c.target)!=n.targets.end(),"sample finish membership");
    if(c.allocation) check(n.exchange && c.allocation->hp+c.allocation->mp+c.allocation->cp==n.exchange->total,"sample allocation");
}
std::string signature(const MatchSnapshot& s,const Command& c) {
    std::ostringstream out;out<<static_cast<int>(c.kind)<<':'<<(c.target ? std::to_string(*c.target) : "-")<<':';
    if(c.allocation) out<<c.allocation->hp<<','<<c.allocation->mp<<','<<c.allocation->cp;
    std::vector<std::tuple<CardId,bool,bool>> multiset;
    std::vector<CardId> faces;
    for(auto t:c.items) {const auto& i=item(s,t);multiset.emplace_back(face_model(i),i.fake_model.has_value(),i.used);faces.push_back(face_model(i));}
    std::sort(multiset.begin(),multiset.end());
    for(auto [face,disguised,used]:multiset) out<<'/'<<face<<','<<disguised<<','<<used;
    out<<" cost="<<((c.kind==CommandKind::play || c.kind==CommandKind::defend) ? magic_cost(faces) : 0);
    if(c.kind==CommandKind::play) {
        out<<" power="<<attack_power(faces)<<" element="<<(attack_element(faces) ? static_cast<int>(*attack_element(faces)) : -1);
        std::vector<CardId> run;
        const auto flush=[&] {std::sort(run.begin(),run.end());for(auto face:run)out<<" p="<<face;run.clear();};
        for(std::size_t j=0;j<c.items.size();++j) {
            const auto& i=item(s,c.items[j]);
            const auto& face=*find_card(face_model(i));
            if(j>0 && !i.fake_model && face.category!=Category::miracles && face.ability==Ability::none) run.push_back(face.id);
            else {flush();out<<" barrier="<<face.id<<','<<i.fake_model.has_value()<<','<<i.used;}
        }
        flush();
    }
    if(c.kind==CommandKind::defend) {
        out<<" element="<<(faces.empty() || !defense_element(faces) ? -1 : static_cast<int>(*defense_element(faces)));
        std::vector<CardId> run;
        const auto flush=[&] {std::sort(run.begin(),run.end());for(auto id:run)out<<" d="<<id;run.clear();};
        for(auto token:c.items) {
            const auto& i=item(s,token);const auto face=face_model(i);
            const auto& card=*find_card(face);
            if(!i.fake_model && card.category!=Category::miracles && card.ability==Ability::none) run.push_back(face);
            else {flush();out<<" barrier="<<face<<','<<i.fake_model.has_value()<<','<<i.used;}
        }
        flush();
    }
    return out.str();
}
// Visible copies may consume/reveal different actual models. Keep the strict
// effect oracle only when the selected hidden contents (and effectful order)
// agree; never use this privileged key to define canonical coverage.
std::string hidden_contents(const MatchSnapshot& s,const Command& c) {
    std::vector<std::tuple<CardId,CardId,bool>> contents;
    std::ostringstream order;
    for(auto token:c.items) {
        const auto& i=item(s,token);
        if(!i.fake_model) continue;
        contents.emplace_back(face_model(i),i.model,i.used);
        if(c.kind==CommandKind::play || c.kind==CommandKind::defend)
            order<<'/'<<face_model(i)<<','<<i.model<<','<<i.used;
    }
    std::sort(contents.begin(),contents.end());
    for(auto [face,actual,used]:contents) order<<" selected="<<face<<','<<actual<<','<<used;
    return order.str();
}
void result_equal(const Match& a,const Match& b) {
    const auto x=a.snapshot(),y=b.snapshot();check(x.phase==y.phase && x.actor==y.actor,"step phase/actor");
    auto ap=a.view({}).pending,bp=b.view({}).pending;
    if(ap)std::sort(ap->models.begin(),ap->models.end());
    if(bp)std::sort(bp->models.begin(),bp->models.end());
    check(ap==bp,"step public pending");
    check(x.rng==y.rng && x.gifts==y.gifts,"step chance/gifts");
    for(std::size_t j=0;j<x.state.players.size();++j) {
        const auto& p=x.state.players[j];const auto& q=y.state.players[j];
        check(std::tie(p.hp,p.mp,p.cp,p.dead,p.curses,p.guardian)==std::tie(q.hp,q.mp,q.cp,q.dead,q.curses,q.guardian),"step player effects");
        const auto hand=[](const auto& player) {std::vector<std::tuple<CardId,std::optional<CardId>,bool>> v;for(const auto& i:player.items)v.emplace_back(i.model,i.fake_model,i.used);std::sort(v.begin(),v.end());return v;};
        check(hand(p)==hand(q),"step hand multiset");
    }
}
void exhaustive(const Match& game,const std::vector<Command>& expected,std::size_t limit) {
    auto s=game.snapshot();Command c{*s.actor,CommandKind::play,{},{}};std::vector<Command> actual;
    const auto visit=[&](auto&& self)->void {
        for(unsigned k=0;k<=9;++k) {
            c.kind=static_cast<CommandKind>(k);
            if(c.kind==CommandKind::exchange) {
                for(int hp=0;hp<=99;hp+=33)for(int mp=0;mp<=99;mp+=33) {
                    const auto cp=own(s).hp+own(s).mp+own(s).cp-hp-mp;
                    if(cp>=0 && cp<=99) {c.target.reset();c.allocation=ResourceAllocation{hp,mp,static_cast<int>(cp)};if(game.legal(c))actual.push_back(c);}
                }
                c.allocation.reset();
            } else {
                c.target.reset();if(game.legal(c))actual.push_back(c);
                for(const auto& p:s.state.players){c.target=p.id;if(game.legal(c))actual.push_back(c);}
            }
        }
        if(c.items.size()==limit)return;
        for(const auto& i:own(s).items) if(std::find(c.items.begin(),c.items.end(),i.token)==c.items.end()) {c.items.push_back(i.token);self(self);c.items.pop_back();}
    };
    visit(visit);sort(actual);check(actual==expected,"unpruned exhaustive oracle");++full_scenes;
}
void rejects(const Match& game,CommandKind kind,std::vector<ItemToken> prefix,bool canonical=false) {
    bool caught=false;try{(void)game.legal_extensions(kind,prefix,{canonical});}catch(const MatchError& e){caught=e.code==MatchErrorCode::invalid;}
    check(caught,"invalid prefix rejected");
}
void audit(const Match& game,bool full=false) {
    const auto before=game.snapshot();++scenes;++phases[before.phase];
    auto raw=tree(game,4,false),canon=tree(game,4,true);
    check(raw==game.legal_commands(4,33),"legal_commands(4) equality");
    raw_sum+=raw.size();canonical_sum+=canon.size();raw_max=std::max(raw_max,raw.size());canonical_max=std::max(canonical_max,canon.size());
    const auto allraw=full ? tree(game,own(before).items.size(),false) : raw;
    const auto allcanon=full ? tree(game,own(before).items.size(),true) : canon;
    if(full) {exhaustive(game,allraw,own(before).items.size());std::cout<<"full oracle hand="<<own(before).items.size()<<" commands="<<allraw.size()<<std::endl;}
    std::map<std::string,Command> signatures;
    for(const auto& c:allcanon) {check(game.legal(c),"canonical command legal");check(std::binary_search(allraw.begin(),allraw.end(),c,[](const auto& a,const auto& b){return key(a)<key(b);}),"canonical subset");signatures.emplace(signature(before,c),c);}
    std::set<std::string> raw_signatures;
    std::size_t compared=0;
    for(const auto& c:allraw) {
        auto sig=signature(before,c);raw_signatures.insert(sig);const auto it=signatures.find(sig);if(it==signatures.end())std::cerr<<"scene="<<scenes<<" phase="<<static_cast<int>(before.phase)<<" signature="<<sig<<"\n";check(it!=signatures.end(),"canonical signatures cover raw");
        if(c!=it->second && hidden_contents(before,c)!=hidden_contents(before,it->second)) {
            ++hidden_pairs_excluded;
            continue;
        }
        if(c!=it->second && compared<(full ? 80u : 12u)) {
            Match a,b;a.restore(before);b.restore(before);a.step(c);b.step(it->second);result_equal(a,b);++steps;++compared;
        }
    }
    check(raw_signatures.size()==signatures.size(),"signature set equality");
    std::uint64_t rng=42;
    for(int trial=0;trial<4;++trial) membership(game,game.sample_action(rng));
    for(bool canonical:{false,true}) for(int trial=0;trial<5;++trial) {
        auto kinds=game.legal_kinds();rng=rng*6364136223846793005ULL+1;auto kind=kinds[rng%kinds.size()];Command c{*before.actor,kind,{},{}};
        for(;;) {
            const auto n=game.legal_extensions(kind,c.items,{canonical});check(n.can_finish || !n.next.empty(),"random walk no dead end");rng=rng*6364136223846793005ULL+1;
            if(n.can_finish && (n.next.empty() || (rng&1))) {
                c.target=n.targets[rng%n.targets.size()];
                if(n.exchange) {auto total=n.exchange->total;const auto hp=std::min<std::int64_t>(99,total);total-=hp;const auto mp=std::min<std::int64_t>(99,total);c.allocation=ResourceAllocation{static_cast<int>(hp),static_cast<int>(mp),static_cast<int>(total-mp)};}
                check(game.legal(c),"walk completion legal");++walks;break;
            }
            c.items.push_back(n.next[rng%n.next.size()]);check(c.items.size()<=own(before).items.size(),"walk finite");
        }
    }
    for(auto kind:game.legal_kinds()) {rejects(game,kind,{99999999});const auto n=game.legal_extensions(kind,{});if(!n.next.empty())rejects(game,kind,{n.next.front(),n.next.front()});}
    rejects(game,static_cast<CommandKind>(255),{});
    check(before==game.snapshot(),"selection queries preserve snapshot and RNG");
}
CardId image(std::string_view name) {for(const auto& c:catalog())if(name==c.image_name)return c.id;check(false,"known image");return 0;}
PlayerState player(PlayerId id,std::initializer_list<CardId> cards) {PlayerState p;p.id=id;for(auto c:cards)p.items.push_back({p.items.size(),c,{},false});return p;}
Match fixture(std::initializer_list<CardId> cards,std::initializer_list<CardId> other={},bool apocalypse=false) {
    SnapshotState s;s.players={player(42,cards),player(7,other)};if(apocalypse)s.tiebreak_gf=0;
    Match m;m.load_scene(s,42,1234);return m;
}
void boundary(const Match& game,MatchErrorCode code) {
    const auto before=game.snapshot();bool caught=false;try{(void)game.legal_kinds();}catch(const MatchError& e){caught=e.code==code;}check(caught,"kinds boundary exception");
    caught=false;try{(void)game.legal_extensions(CommandKind::play,{});}catch(const MatchError& e){caught=e.code==code;}check(caught && before==game.snapshot(),"extensions boundary exception/purity");++exceptions;
}
void play_runs() {
    const auto club=image("bronze-club");
    const std::vector<CardId> extras={9,12,63,68,78,84,87,124,203};
    for(std::size_t i=0;i<extras.size();++i) for(std::size_t j=i+1;j<extras.size();++j) {
        auto game=fixture({club,extras[i],extras[j],66});
        rejects(game,CommandKind::play,{0,2,1},true);
        check(game.legal_extensions(CommandKind::play,std::vector<ItemToken>{0,2,3,1},{true}).can_finish,"ability barrier preserves separate runs");
        if(can_start_main(extras[j])) check(game.legal_extensions(CommandKind::play,std::vector<ItemToken>{2,1},{true}).can_finish,"play base is fixed, not sorted");
        for(auto target:{PlayerId{42},PlayerId{7}}) {
            Command x{42,CommandKind::play,{0,1,2},target},y{42,CommandKind::play,{0,2,1},target};
            check(signature(game.snapshot(),x)==signature(game.snapshot(),y),"ordinary play run signature");
            Match a=game,b=game;a.step(x);b.step(y);result_equal(a,b);++steps;
            if(target==7) {
                check(a.view({}).pending->models==std::vector<CardId>{club,extras[i],extras[j]},"pending preserves selected canonical order");
                a.step({7,CommandKind::defend,{},{}});b.step({7,CommandKind::defend,{},{}});result_equal(a,b);++steps;
            }
        }
    }
    auto masked=fixture({club,12,15});auto s=masked.snapshot().state;
    s.players[0].curses={Curse::dream};s.players[0].items[1].fake_model=9;masked.load_scene(s,42,1234);
    check(masked.legal_extensions(CommandKind::play,std::vector<ItemToken>{0,2,1},{true}).can_finish,"Dream face is a play run barrier");
    check(masked.legal_extensions(CommandKind::play,std::vector<ItemToken>{0,1,2},{true}).can_finish,"Dream barrier preserves either selection order");
}
// Compare every reachable prefix, including all node fields, without a depth
// cap. Both scenes have identical owner observations and physical token order.
void leak_tree_equal(const Match& a,const Match& b) {
    const auto before_a=a.snapshot(),before_b=b.snapshot();
    check(before_a!=before_b,"leak scenes differ in hidden state");
    check(a.view(before_a.actor).board==b.view(before_b.actor).board,"leak scenes have identical owner views");
    const auto kinds=a.legal_kinds();check(kinds==b.legal_kinds(),"hidden models preserve legal kinds");
    ++leak_pairs;
    for(auto kind:kinds) {
        std::vector<ItemToken> prefix;
        const auto visit=[&](auto&& self)->void {
            const auto x=a.legal_extensions(kind,prefix,{true});
            const auto y=b.legal_extensions(kind,prefix,{true});
            check(x==y,"hidden models preserve every canonical node");++leak_nodes;
            for(auto token:x.next) {prefix.push_back(token);self(self);prefix.pop_back();}
        };
        visit(visit);
    }
    check(before_a==a.snapshot() && before_b==b.snapshot(),"leak queries preserve state/RNG");
}
void dream_leaks() {
    const auto compare=[](SnapshotState state,std::size_t changed,CardId actual,bool response=false) {
        Match a,b;a.load_scene(state,42,1234);
        state.players[response ? 1 : 0].items[changed].model=actual;
        b.load_scene(state,42,1234);
        if(response) {
            a.step({42,CommandKind::play,{0},7});b.step({42,CommandKind::play,{0},7});
        }
        leak_tree_equal(a,b);audit(a,true);audit(b,true);
    };
    // Same visible disguised copies: actuals (12,12) versus (12,15).
    auto duplicate=fixture({6,12,12,9}).snapshot().state;
    duplicate.players[0].curses={Curse::dream};
    duplicate.players[0].items[1].fake_model=9;duplicate.players[0].items[2].fake_model=9;
    compare(duplicate,2,15);

    // A masked +ATK sits between ordinary +ATKs, including descending faces.
    auto run=fixture({6,18,12,9}).snapshot().state;
    run.players[0].curses={Curse::dream};run.players[0].items[2].fake_model=9;
    compare(run,2,15);
    Match barrier;barrier.load_scene(run,42,1234);
    check(barrier.legal_extensions(CommandKind::play,std::vector<ItemToken>{0,1,2,3},{true}).can_finish,"masked +ATK splits descending ordinary runs");

    // Received 19-as-27 changes the hidden ability as well as the actual ID.
    auto ability=fixture({19,9,12}).snapshot().state;
    ability.players[0].curses={Curse::dream};ability.players[0].items[0].fake_model=27;
    compare(ability,0,27);

    // Exercise the same duplicate and barrier rules in response selections.
    auto defense=fixture({6},{113,113,118,123}).snapshot().state;
    defense.players[1].curses={Curse::dream};
    defense.players[1].items[0].fake_model=113;defense.players[1].items[1].fake_model=113;
    compare(defense,1,118,true);

    // Disguised remains observable even when the face equals the actual model.
    auto marker=fixture({6,9,9}).snapshot().state;
    marker.players[0].curses={Curse::dream};marker.players[0].items[1].fake_model=9;
    Match distinct;distinct.load_scene(marker,42,1234);
    const auto root=distinct.legal_extensions(CommandKind::play,{}, {true});
    check(std::find(root.next.begin(),root.next.end(),1)!=root.next.end()
        && std::find(root.next.begin(),root.next.end(),2)!=root.next.end(),"visible disguise flag distinguishes copies");
    check(hidden_pairs_excluded>0,"hidden-effect exclusions exercised");
}
void fixtures() {
    const auto club=image("bronze-club"),free=image("spiritual-doll"),spring=image("spring");
    // Six/eight-card unpruned oracles include >4 completions and duplicate X,Y,X.
    audit(fixture({club,9,9,9,9,9}),true);
    audit(fixture({3,4,5,199,199,200}),true);
    audit(fixture({199,200,199,200,199,200}),true);
    audit(fixture({199,200,199,200,199,200}, {},true),true);
    audit(fixture({club,9,9,9,9,9,9,9}),true);
    for(bool apocalypse:{false,true}) {
        auto m=fixture({spring,free,235,204,199,200},{},apocalypse);auto s=m.snapshot().state;s.players[0].mp=0;s.players[0].items[2].used=true;m.load_scene(s,42,1234);audit(m);
    }
    for(bool flash:{false,true}) {
        auto m=fixture({club},{image("aqua-shoes"),image("rainbow-curtain"),image("super-mirror"),image("saturn-ring"),spring,free});
        auto s=m.snapshot().state;s.players[1].mp=0;if(flash)s.players[1].curses={Curse::flash};m.load_scene(s,42,1234);m.step({42,CommandKind::play,{0},7});audit(m,true);
    }
    auto armor=fixture({club},{image("aqua-shoes"),image("sky-boots"),image("aqua-shoes"),image("saturn-ring"),image("rainbow-curtain"),image("super-mirror")});
    armor.step({42,CommandKind::play,{0},7});audit(armor,true);rejects(armor,CommandKind::defend,{2},true);
    for(auto curse:{Curse::fog,Curse::dream}) {
        auto m=fixture({club,9,204,free,199,207});auto s=m.snapshot().state;s.players[0].curses={curse};
        if(curse==Curse::dream) {s.players[0].items[0].model=19;s.players[0].items[0].fake_model=27;}
        m.load_scene(s,42,1234);audit(m);
    }
    audit(fixture({94,232,204,9,free,3}));
    auto ordered=fixture({club,231,free,66,83,9});auto os=ordered.snapshot().state;
    os.players[0].mp=0;ordered.load_scene(os,42,1234);audit(ordered,true);
    const auto free_only=ordered.legal_extensions(CommandKind::play,std::vector<ItemToken>{0,1});
    check(!free_only.can_finish && free_only.next==std::vector<ItemToken>{2},"only immediate FREE repairs unpaid miracle");
    rejects(ordered,CommandKind::play,{0,1,5});
    MatchConfig short_cfg;short_cfg.hand_limit=1;short_cfg.initial_hand=1;Match short_hand(short_cfg);short_hand.load_scene(fixture({club}).snapshot().state,42,1);
    check(short_hand.legal_extensions(CommandKind::play,{}).next.size()<=1,"hand limit");

    // No enemies: chance and Wide must never enter next.
    auto solo=fixture({112,232,94,club});auto ss=solo.snapshot().state;ss.players[0].team=ss.players[1].team=Team::one;solo.load_scene(ss,42,1);boundary(solo,MatchErrorCode::invalid);
    auto sale=fixture({5},{club});sale.step({42,CommandKind::buy,{0},7});sale.step({7,CommandKind::defend,{},{}});check(sale.snapshot().phase==MatchPhase::purchase,"purchase fixture");audit(sale);
    auto confused=fixture({club});auto cs=confused.snapshot().state;cs.players[0].confusion_count=1;confused.load_scene(cs,42,1);boundary(confused,MatchErrorCode::unsupported);
    MatchConfig cfg;cfg.external_artifacts=true;cfg.initial_hand=0;Match grant(cfg);grant.step({0,CommandKind::pray,{},{}});boundary(grant,MatchErrorCode::unsupported);
    cfg={};cfg.max_decisions=1;Match finished(cfg);std::uint64_t rng=1;finished.step(finished.sample_action(rng));boundary(finished,MatchErrorCode::invalid);
}
void games(bool benchmark) {
    struct Timing {double ns=0;std::size_t calls=0;};std::map<std::pair<bool,std::size_t>,Timing> timing;
    for(std::uint64_t seed=0;seed<8;++seed) {
        MatchConfig cfg;cfg.max_decisions=40;cfg.include_fate=seed&1;cfg.external_grants=seed&2;cfg.external_artifacts=seed&4;
        Match m(cfg);m.reset(std::vector<PlayerSetup>{{42,Team::solo,"a"},{7,Team::solo,"b"},{90,Team::solo,"c"}},seed);
        std::uint64_t policy=seed+11,chance=seed+31;
        while(!m.done()) {
            const auto s=m.snapshot();const bool exceptional=s.phase==MatchPhase::grant || (s.phase==MatchPhase::main && own(s).confusion_count);
            if(exceptional)boundary(m,MatchErrorCode::unsupported);
            else if(!benchmark)audit(m);
            else for(bool canonical:{false,true}) {
                // Time both roots and reachable, nonempty sampled prefixes.
                auto c=m.sample_action(policy);std::vector<ItemToken> prefix;
                for(std::size_t depth=1;depth<c.items.size();++depth) {
                    auto n=m.legal_extensions(c.kind,prefix,{canonical});if(n.next.empty())break;
                    prefix.push_back(n.next[(seed+depth)%n.next.size()]);
                }
                const auto start=std::chrono::steady_clock::now();
                for(int i=0;i<100;++i)(void)m.legal_extensions(c.kind,prefix,{canonical});
                const auto end=std::chrono::steady_clock::now();auto& t=timing[{canonical,own(s).items.size()}];
                t.ns+=std::chrono::duration<double,std::nano>(end-start).count();t.calls+=100;
            }
            m.rollout_step(policy,chance);
        }
        if(!benchmark)boundary(m,MatchErrorCode::invalid);
    }
    for(const auto& [k,t]:timing)std::cout<<"canonical="<<k.first<<" hand="<<k.second<<" calls="<<t.calls<<" us/call="<<t.ns/t.calls/1000<<'\n';
}
}
int main(int argc,char** argv) {
    if(argc==2 && std::string_view(argv[1])=="--benchmark") {games(true);return 0;}
    play_runs();dream_leaks();fixtures();games(false);
    std::cout<<"scenes="<<scenes<<" full_oracles="<<full_scenes<<" step_pairs="<<steps<<" walks="<<walks<<" exceptions="<<exceptions<<'\n';
    std::cout<<"leak_pairs="<<leak_pairs<<" leak_nodes="<<leak_nodes<<" hidden_effect_pairs_excluded="<<hidden_pairs_excluded<<'\n';
    for(const auto& [phase,count]:phases)std::cout<<"phase="<<static_cast<int>(phase)<<" scenes="<<count<<'\n';
    std::cout<<"tree<=4 raw mean="<<double(raw_sum)/scenes<<" max="<<raw_max<<" canonical mean="<<double(canonical_sum)/scenes<<" max="<<canonical_max<<'\n';
}
