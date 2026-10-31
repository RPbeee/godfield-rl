#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <string_view>
#include <utility>

namespace {
using namespace godfield::official;
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL experimental official match: "<<message<<'\n';std::exit(1);}}
CardId image(std::string_view name){for(const auto& c:catalog())if(name==c.image_name)return c.id;check(false,"known public model exists");return 0;}
PlayerState player(PlayerId id,std::initializer_list<CardId> cards={},Team team=Team::solo){PlayerState p;p.id=id;p.name="p"+std::to_string(id);p.team=team;for(auto card:cards)p.items.push_back({p.items.size(),card,{},false});return p;}
SnapshotState scene(std::initializer_list<CardId> own={},std::initializer_list<CardId> enemy={}){SnapshotState s;s.players={player(0,own),player(7,enemy)};return s;}
Match fixture(SnapshotState s,MatchConfig config={}){Match m(config);m.load_scene(std::move(s),0,1234);return m;}
Command play(PlayerId actor,std::initializer_list<ItemToken> tokens,PlayerId target){return {actor,CommandKind::play,std::vector<ItemToken>(tokens),target};}
Command defend(PlayerId actor,std::initializer_list<ItemToken> tokens={}){return {actor,CommandKind::defend,std::vector<ItemToken>(tokens),{}};}
Command pray(PlayerId actor){return {actor,CommandKind::pray,{},{}};}
void roundtrip(const Match& m){Match copy(m.snapshot().config);copy.restore(m.snapshot());check(copy.snapshot()==m.snapshot(),"full execution snapshot including frame, owner, gifts, RNG and outcomes resumes exactly");}
void reject(Match& m,const Command& command){const auto before=m.snapshot();check(!m.legal(command)&&m.snapshot()==before,"legality query rejects without mutation");bool threw=false;try{(void)m.step(command);}catch(const MatchError&){threw=true;}check(threw&&m.snapshot()==before,"invalid/unsupported command transactional across resources, RNG and frames");}
CardId elemental(Category category,Element element){for(const auto& c:catalog())if(c.category==category&&c.element==element&&match_supported(c.id))return c.id;check(false,"plain supported elemental model exists");return 0;}

void scope_reset_and_local_contract(){
    const auto pool=match_catalog();check(pool.size()==132&&match_gift_weight()==308&&std::string_view(match_ruleset)=="official-execution-subset-v1"
        &&std::string_view(match_scope).find("not-official-server-equivalence")!=std::string_view::npos,"132model/308weight execution subset is explicitly experimental");
    std::array<int,8> counts{};int weight=0;std::set<CardId> unique;for(auto id:pool){const auto* c=find_card(id);check(c&&match_supported(id)&&unique.insert(id).second,"subset strict supported allowlist without duplicates");++counts[static_cast<int>(c->category)];weight+=c->gift_rate;}
    check(counts==std::array<int,8>{0,63,51,8,10,0,0,0}&&weight==308,"normal weapon/armor/simple sundry/miracle pool only");
    check(!match_supported(0)&&!match_supported(image("reflection-sword"))&&!match_supported(image("aura"))&&!match_supported(image("ascension-bow"))&&!match_supported(image("guardian-pot")),"specials, percentages and guardians never silently become plain damage");
    Match a,b;check(a.snapshot()==b.snapshot()&&!a.done()&&a.snapshot().state.players.size()==2,"default two-solo seed42 snapshot deterministic");
    for(const auto& p:a.snapshot().state.players)check(p.hp==40&&p.mp==10&&p.cp==20&&p.items.size()==9&&!p.dead,"local canonical initial resources/9hand, not a server-derived scheduling claim");
    std::vector<PlayerSetup> nine;for(unsigned i=0;i<9;++i)nine.push_back({i?100+i:0,static_cast<Team>(i%5),"seat"+std::to_string(i)});
    for(auto seed:{std::uint64_t{0},std::uint64_t{42},std::numeric_limits<std::uint64_t>::max()}){a.reset(nine,seed);b.reset(nine,seed);check(a.snapshot()==b.snapshot()&&a.snapshot().state.players.size()==9,"nine-player arbitrary IDs/teams reset deterministic at seed boundaries");roundtrip(a);}
    const auto before=a.snapshot();auto bad=nine;bad.push_back({999,Team::solo,"ten"});bool threw=false;try{a.reset(bad,1);}catch(const MatchError&){threw=true;}check(threw&&a.snapshot()==before,"ten-player reset rejected atomically");
}

void weapon_frames_colors_owner_and_fixed_rewards(){
    auto m=fixture(scene({image("bronze-club"),image("crossbow")},{}));const auto launch=m.step(play(0,{0,1},7));
    check(launch.phase==MatchPhase::response&&launch.actor==7&&m.snapshot().owner==0&&m.snapshot().queue.size()==1&&m.snapshot().queue[0].power==3
        &&m.snapshot().queue[0].origin==0&&m.snapshot().queue[0].source==0&&m.snapshot().queue[0].target==7&&m.snapshot().gifts[0]==2&&m.snapshot().state.players[0].items.empty(),"ordered normal base+plus physical copies consumed and queued while owner differs from defender");roundtrip(m);
    reject(m,defend(0));const auto finish=m.step(defend(7));check(!finish.terminated&&!finish.truncated&&m.snapshot().state.players[1].hp==37
        &&m.snapshot().state.players[0].items.size()==2&&finish.actor==7&&m.snapshot().owner==7&&m.snapshot().queue.empty(),"resolved physical attack pays two gifts, then next owner follows fixed player order");roundtrip(m);
    const auto fire=elemental(Category::weapons,Element::fire),same=elemental(Category::armor,Element::fire),opposite=elemental(Category::armor,Element::water);
    auto colored=fixture(scene({fire},{same,opposite}));colored.step(play(0,{0},7));reject(colored,defend(7,{0}));const auto raw=find_card(fire)->atk,dfs=find_card(opposite)->defense;
    colored.step(defend(7,{1}));check(colored.snapshot().state.players[1].hp==40-std::max(0,raw-dfs),"normal compatible attribute defense subtracts DEF and incompatible one rejects");roundtrip(colored);
    auto light=fixture(scene({elemental(Category::weapons,Element::light)},{elemental(Category::armor,Element::light)}));light.step(play(0,{0},7));reject(light,defend(7,{0}));light.step(defend(7));roundtrip(light);
    auto dark=fixture(scene({elemental(Category::weapons,Element::darkness)},{}));dark.step(play(0,{0},7));const auto killed=dark.step(defend(7));
    check(killed.terminated&&!killed.truncated&&killed.winners==std::vector<PlayerId>{0}&&killed.rewards==std::vector<double>{1,-1}&&dark.snapshot().state.players[1].dead,"positive Dark residual ends local match with fixed-seat rewards");roundtrip(dark);
    auto self=fixture(scene({image("bronze-club")},{}));auto s=self.snapshot().state;s.players[0].hp=1;self.load_scene(s,0,88);self.step(play(0,{0},0));check(self.snapshot().actor==0&&self.snapshot().phase==MatchPhase::response,"self attack also creates explicit defender response in experimental contract");const auto suicide=self.step(defend(0));
    check(suicide.terminated&&suicide.winners==std::vector<PlayerId>{7}&&suicide.rewards==std::vector<double>{-1,1},"self attack after own response preserves fixed-seat rewards instead of actor sign");roundtrip(self);
    SnapshotState team;team.players={player(0,{image("bronze-club")},Team::one),player(7,{},Team::one),player(9,{},Team::two)};team.players[2].hp=1;
    auto alliance=fixture(team);alliance.step(play(0,{0},9));const auto win=alliance.step(defend(9));check(win.terminated&&win.winners==std::vector<PlayerId>{0,7}&&win.rewards==std::vector<double>{1,1,-1},"last enemy death yields complete winner team, not first-alive candidate helper");roundtrip(alliance);
    SnapshotState nine;for(unsigned i=0;i<9;++i)nine.players.push_back(player(100+i,i==0?std::initializer_list<CardId>{image("bronze-club")}:std::initializer_list<CardId>{}));nine.players[1].dead=true;nine.players[1].hp=0;
    Match multi;multi.load_scene(nine,100,5);multi.step(play(100,{0},108));check(multi.snapshot().actor==108&&multi.snapshot().owner==100,"ninth seat is a real response target, not1-owner");
    const auto next=multi.step(defend(108));check(next.actor==102&&multi.snapshot().owner==102&&next.rewards==std::vector<double>(9,0),"next owner skips dead fixed-order seats, independent of last defender");roundtrip(multi);
}

void recovery_retention_capacity_and_atomicity(){
    for(const auto pair:{std::pair{"smile-dew",Resource::hp},std::pair{"smile-flower",Resource::mp}}){auto state=scene({image(pair.first)},{});state.players[0].hp=state.players[0].mp=98;auto m=fixture(state);
        m.step(play(0,{0},0));const auto& p=m.snapshot().state.players[0];check((pair.second==Resource::hp?p.hp:p.mp)==99&&p.items.size()==1&&p.items[0].token!=0,"simple physical recovery clips99, consumes unique token and requests one replacement");roundtrip(m);}
    auto state=scene({image("spring")},{});state.players[0].mp=20;auto retained=fixture(state);retained.step(play(0,{0},0));const auto& p=retained.snapshot().state.players[0];
    check(p.hp==50&&p.mp==13&&p.items.size()==2&&p.items[0].token==0&&p.items[0].used,"first Spring costs7 and retains same item token markedused plus one gift");
    retained.step(pray(7));retained.step(play(0,{0},0));check(retained.snapshot().state.players[0].hp==60&&retained.snapshot().state.players[0].mp==6
        &&std::count_if(retained.snapshot().state.players[0].items.begin(),retained.snapshot().state.players[0].items.end(),[](const auto& i){return i.used;})==1,"retained reuse pays cost again without acquiring another copy");roundtrip(retained);
    auto utility=fixture(scene({image("smile-dew")},{image("aqua-shoes")}));utility.step(play(0,{0},7));reject(utility,defend(7,{0}));utility.step(defend(7));roundtrip(utility);
    auto cp=scene({image("treasure")},{});cp.players[0].cp=98;cp.players[0].mp=5;auto money=fixture(cp);money.step(play(0,{0},0));check(money.snapshot().state.players[0].cp==99&&money.snapshot().state.players[0].mp==0,"simple CP miracle uses declared amount/cost and cap");roundtrip(money);
    auto full=scene({},{});for(unsigned n=0;n<18;++n)full.players[0].items.push_back({n,image(n==0?"spring":"aqua-shoes"),{},n==0});
    auto packed=fixture(full);const auto rng=packed.snapshot().rng;packed.step(play(0,{0},0));check(packed.snapshot().state.players[0].items.size()==18&&packed.snapshot().rng==rng,"retained cast at18 creates no overflowing draw request in local contract");roundtrip(packed);
    auto no_room=fixture(full);reject(no_room,pray(0));
    auto discard=fixture(scene({image("aqua-shoes"),image("aqua-shoes")},{}));const auto oldrng=discard.snapshot().rng;
    discard.step({0,CommandKind::discard,{0,1},{}});check(discard.snapshot().state.players[0].items.empty()&&discard.snapshot().rng==oldrng,"multiple ordinary discard earns no gifts/RNG draw");roundtrip(discard);
    auto invalid=fixture(scene({image("spring"),image("bronze-club")},{}));state=invalid.snapshot().state;state.players[0].mp=6;invalid.load_scene(state,0,9);
    reject(invalid,play(0,{0},0));reject(invalid,play(0,{1,1},7));reject(invalid,play(999,{1},7));reject(invalid,play(0,{1},999));reject(invalid,pray(0));reject(invalid,{0,CommandKind::discard,{1},{}});
    const auto before=invalid.snapshot();for(int error=0;error<6;++error){auto bad=state;if(error==0)bad.players[0].items[0].model=image("reflection-sword");if(error==1)bad.players[0].items[0].fake_model=image("aqua-shoes");
        if(error==2)bad.players[0].curses={Curse::cold};if(error==3)bad.players[0].guardian=Guardian::mars;if(error==4)bad.players[0].confusion_count=1;if(error==5)bad.tiebreak_gf=10;
        bool rejected=false;try{invalid.load_scene(bad,0,9);}catch(const MatchError&){rejected=true;}check(rejected&&invalid.snapshot()==before,"unsupported scene feature rejected atomically rather than ignored");}
}

void checkpoint_cutoff_and_seeded_multiplayer(){
    auto active=fixture(scene({image("bronze-club")},{}));active.step(play(0,{0},7));const auto exact=active.snapshot();
    for(int error=0;error<10;++error){auto bad=exact;if(error==0)++bad.version;if(error==1)bad.actor=0;if(error==2)bad.queue.push_back(bad.queue[0]);if(error==3)++bad.queue[0].power;
        if(error==4)bad.queue[0].target=999;if(error==5)bad.gifts.pop_back();if(error==6)bad.rewards.push_back(0);if(error==7)bad.gifts[0]=0;if(error==8)bad.gifts[1]=1;if(error==9)bad.decisions=0;
        bool rejected=false;try{active.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&active.snapshot()==exact,"checkpoint format/shape/provenance tampering rejects atomically");}
    auto spell=fixture(scene({image("spring")},{}));spell.step(play(0,{0},7));const auto held_exact=spell.snapshot();
    for(int error=0;error<5;++error){auto bad=held_exact;
        if(error==0)bad.state.players[0].items[0].used=false;
        if(error==1)bad.state.players[0].items.clear();
        if(error==2){bad.queue[0].target=0;bad.actor=0;}
        if(error==3)bad.state.players[0].mp=93;
        if(error==4){bad.state.players[0].items.push_back({1,image("aqua-shoes"),{},false});bad.next_token=2;bad.queue[0].tokens[0]=1;}
        bool rejected=false;try{spell.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&spell.snapshot()==held_exact,"active miracle requires retained source copy and exact pending reservation");}
    auto physical=fixture(scene({image("bronze-club"),image("crossbow")},{}));physical.step(play(0,{0,1},7));const auto physical_exact=physical.snapshot();
    for(int error=0;error<5;++error){auto bad=physical_exact;
        if(error==0)bad.queue[0].tokens.clear();
        if(error==1)bad.queue[0].tokens[1]=bad.queue[0].tokens[0];
        if(error==2)bad.queue[0].tokens[0]=bad.next_token;
        if(error==3)bad.state.players[0].items.push_back({0,image("bronze-club"),{},false});
        if(error==4){for(ItemToken n=2;n<20;++n)bad.state.players[0].items.push_back({n,image("aqua-shoes"),{},false});bad.next_token=20;bad.gifts[0]=0;}
        bool rejected=false;try{physical.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&physical.snapshot()==physical_exact,"pending physical token count/uniqueness/consumption and pre-consumption capacity strictly verified");}
    auto all_dead=scene();for(auto& p:all_dead.players){p.hp=0;p.dead=true;}auto draw=fixture(all_dead);
    check(draw.done()&&draw.snapshot().terminated&&!draw.snapshot().truncated&&draw.snapshot().winners.empty()&&draw.snapshot().rewards==std::vector<double>{0,0},"explicit all-dead scene is natural draw, not cutoff");roundtrip(draw);
    MatchConfig limit;limit.max_decisions=2;auto lethal=scene({image("bronze-club")},{});lethal.players[1].hp=1;auto done=fixture(lethal,limit);done.step(play(0,{0},7));const auto terminal=done.step(defend(7));
    check(terminal.terminated&&!terminal.truncated,"natural terminal at decision limit takes precedence over cutoff");roundtrip(done);
    auto cutoff=fixture(scene({image("bronze-club")},{}),limit);cutoff.step(play(0,{0},7));const auto truncated=cutoff.step(defend(7));check(truncated.truncated&&!truncated.terminated&&truncated.winners.empty()
        &&truncated.rewards==std::vector<double>{0,0},"cutoff has no natural winners/reward and is separate from terminal");roundtrip(cutoff);
    auto version_scene=scene({image("bronze-club")},{});version_scene.update_count=max_wire_integer;auto exhausted_update=fixture(version_scene);
    const auto version_before=exhausted_update.snapshot();const auto command=play(0,{0},7);check(exhausted_update.legal(command),"transport exhaustion is a late execution failure, not invalid command syntax");
    bool late_rejected=false;try{(void)exhausted_update.step(command);}catch(const MatchError&){late_rejected=true;}
    check(late_rejected&&exhausted_update.snapshot()==version_before,"late update counter exhaustion rolls back consumed item, queue, gifts, counters and RNG");
    auto token_scene=scene({image("bronze-club")},{});token_scene.players[0].items[0].token=max_wire_integer;auto exhausted_token=fixture(token_scene);
    exhausted_token.step(play(0,{max_wire_integer},7));const auto token_before=exhausted_token.snapshot();check(token_before.next_token==max_wire_integer+1&&exhausted_token.legal(defend(7)),"pending valid frame can expose checked token exhaustion on delayed gift");
    late_rejected=false;try{(void)exhausted_token.step(defend(7));}catch(const MatchError&){late_rejected=true;}
    check(late_rejected&&exhausted_token.snapshot()==token_before,"late gift exhaustion rolls back target HP, phase, queue, decisions, reservations and RNG exactly");
    for(unsigned count:{2u,9u})for(std::uint64_t seed=0;seed<24;++seed){MatchConfig cfg;cfg.max_decisions=128;Match a(cfg),b(cfg);std::vector<PlayerSetup> setup;
        for(unsigned i=0;i<count;++i)setup.push_back({i==0?0:100+i,count==9?static_cast<Team>(i%5):Team::solo,"p"+std::to_string(i)});
        a.reset(setup,seed);b.reset(setup,seed);std::uint64_t pa=seed^0xa5a5, pb=pa;
        while(!a.done()){const auto before=a.snapshot();const auto ca=a.sample_action(pa),cb=b.sample_action(pb);check(ca==cb&&pa==pb&&a.snapshot()==before&&a.legal(ca),"policy RNG separate from environmental RNG and seeded proposals deterministic/legal");
            const auto ra=a.step(ca),rb=b.step(cb);check(ra==rb&&a.snapshot()==b.snapshot(),"seeded2/9team execution and fixed player outcome replay");roundtrip(a);check(!a.observe(a.snapshot().state.players[0].id).empty(),"every executable snapshot connects to fresh view");}
        check(a.snapshot().terminated!=a.snapshot().truncated,"explicit terminal or cutoff reached; no silent stuck match");}
}
}
int main(){scope_reset_and_local_contract();weapon_frames_colors_owner_and_fixed_rewards();recovery_retention_capacity_and_atomicity();checkpoint_cutoff_and_seeded_multiplayer();std::cout<<"Experimental official match passed (132subset/9teams/owner/retention/atomicity/snapshot/RNG/cutoff; server scheduling not asserted).\n";}
