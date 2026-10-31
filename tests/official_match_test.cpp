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
    const auto pool=match_catalog();check(pool.size()==139&&match_gift_weight()==319&&std::string_view(match_ruleset)=="official-execution-subset-v2"
        &&std::string_view(match_scope).find("not-official-server-equivalence")!=std::string_view::npos,"139model/319weight execution subset is explicitly experimental");
    std::array<int,8> counts{};int weight=0;std::set<CardId> unique;for(auto id:pool){const auto* c=find_card(id);check(c&&match_supported(id)&&unique.insert(id).second,"subset strict supported allowlist without duplicates");++counts[static_cast<int>(c->category)];weight+=c->gift_rate;}
    check(counts==std::array<int,8>{0,65,56,8,10,0,0,0}&&weight==319,"normal weapon/armor/simple sundry/miracle pool only");
    check(!match_supported(0)&&!match_supported(image("bouncing-sword"))&&!match_supported(image("aura"))&&!match_supported(image("ascension-bow"))&&!match_supported(image("guardian-pot")),"specials, percentages and guardians never silently become plain damage");
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
    const auto before=invalid.snapshot();for(int error=0;error<6;++error){auto bad=state;if(error==0)bad.players[0].items[0].model=image("bouncing-sword");if(error==1)bad.players[0].items[0].fake_model=image("aqua-shoes");
        if(error==2)bad.players[0].curses={Curse::cold};if(error==3)bad.players[0].guardian=Guardian::mars;if(error==4)bad.players[0].confusion_count=1;if(error==5)bad.tiebreak_gf=10;
        bool rejected=false;try{invalid.load_scene(bad,0,9);}catch(const MatchError&){rejected=true;}check(rejected&&invalid.snapshot()==before,"unsupported scene feature rejected atomically rather than ignored");}
}

void reflection_filter_and_immutable_origin(){
    const auto sword=image("reflection-sword"), moon=image("moonlight-axe"), mirror=image("super-mirror"), curtain=image("rainbow-curtain");
    for(const auto id:{sword,moon,image("moonlight-helm"),image("moonlight-shield"),image("moonlight-armor"),mirror,curtain})check(match_supported(id),"explicit seven-model reaction/filter extension closes supported pool");
    for(const auto id:{sword,moon}){auto main=fixture(scene({id},{}));main.step(play(0,{0},7));check(main.snapshot().queue[0].power==find_card(id)->atk,"reflection weapon retains its ordinary main ATK");main.step(defend(7));roundtrip(main);}
    auto chain=fixture(scene({image("bronze-club"),sword},{sword}));chain.step(play(0,{0},7));const auto origin=chain.snapshot().queue[0];const auto rng=chain.snapshot().rng;
    chain.step(defend(7,{0}));const auto first=chain.snapshot();check(first.owner==0&&first.actor==0&&first.queue[0].source==7&&first.queue[0].target==0&&first.queue[0].origin==0&&first.queue[0].initial_target==7
        &&first.queue[0].models==origin.models&&first.queue[0].tokens==origin.tokens&&first.queue[0].power==1&&first.queue[0].reflections.size()==1&&first.gifts==std::vector<std::uint32_t>{1,1}&&first.rng==rng,"physical reflection swaps current endpoints without recasting, drawing or changing immutable payload");roundtrip(chain);
    chain.step(defend(0,{1}));check(chain.snapshot().actor==7&&chain.snapshot().queue[0].source==0&&chain.snapshot().queue[0].target==7&&chain.snapshot().queue[0].reflections.size()==2
        &&chain.snapshot().gifts==std::vector<std::uint32_t>{2,1},"return reflection has same origin/owner and separately accounts each seat consumption");roundtrip(chain);
    chain.step(defend(7));check(chain.snapshot().state.players[1].hp==39&&chain.snapshot().state.players[0].items.size()==2&&chain.snapshot().state.players[1].items.size()==1&&chain.snapshot().owner==7,"only final receiver loses HP and all living seats receive accumulated gifts once");roundtrip(chain);
    const auto fire=elemental(Category::weapons,Element::fire), same=elemental(Category::armor,Element::fire);
    auto filter=fixture(scene({fire},{curtain,same}));filter.step(play(0,{0},7));reject(filter,defend(7,{1}));const auto raw=filter.snapshot().queue[0].power;
    filter.step(defend(7,{0,1}));check(filter.snapshot().state.players[1].hp==40-std::max<std::int64_t>(0,raw-find_card(same)->defense),"leading filter makes otherwise incompatible ordinary defense eligible");roundtrip(filter);
    auto colored=fixture(scene({fire,sword},{curtain,sword}));colored.step(play(0,{0},7));reject(colored,defend(7,{1}));colored.step(defend(7,{0,1}));check(!colored.snapshot().queue[0].element&&colored.snapshot().queue[0].power==find_card(fire)->atk,"filter neutralizes colored payload before matching physical reflection");roundtrip(colored);
    colored.step(defend(0,{1}));check(!colored.snapshot().queue[0].element,"neutralized attribute survives repeated reflection");colored.step(defend(7));roundtrip(colored);
    auto dark=fixture(scene({elemental(Category::weapons,Element::darkness)},{mirror}));dark.step(play(0,{0},7));dark.step(defend(7,{0}));check(dark.snapshot().queue[0].element==Element::darkness,"universal reflection preserves Dark attribute");const auto death=dark.step(defend(0));check(death.terminated&&death.winners==std::vector<PlayerId>{7},"reflected Dark kills current recipient and rewards fixed seats");roundtrip(dark);
    auto neutral_dark=fixture(scene({elemental(Category::weapons,Element::darkness)},{curtain}));neutral_dark.step(play(0,{0},7));const auto dark_raw=neutral_dark.snapshot().queue[0].power;neutral_dark.step(defend(7,{0}));check(!neutral_dark.done()&&neutral_dark.snapshot().state.players[1].hp==40-dark_raw,"filter removes Dark execution rather than only adding zero DEF");roundtrip(neutral_dark);
    auto self=fixture(scene({image("bronze-club"),sword},{}));self.step(play(0,{0},0));self.step(defend(0,{1}));check(self.snapshot().actor==0&&self.snapshot().queue[0].source==0&&self.snapshot().queue[0].target==0&&self.snapshot().owner==0,"self reflection remains an explicit local response even with identical endpoints");roundtrip(self);self.step(defend(0));check(self.snapshot().state.players[0].hp==39,"self reflection applies original payload exactly once");
    for(const auto id:{moon,image("moonlight-helm"),image("moonlight-shield"),image("moonlight-armor"),mirror}){auto spell=fixture(scene({image("spring")},{id}));spell.step(play(0,{0},7));const auto before=spell.snapshot();spell.step(defend(7,{0}));check(spell.snapshot().state.players[0].mp==3&&spell.snapshot().state.players[1].mp==10&&spell.snapshot().queue[0].models==before.queue[0].models&&spell.snapshot().queue[0].tokens==before.queue[0].tokens,"magic reflection costs no MP and does not acquire/recast original retained miracle");roundtrip(spell);spell.step(defend(0));check(spell.snapshot().state.players[0].hp==50&&spell.snapshot().state.players[1].hp==40,"reflected utility affects final target only");roundtrip(spell);}
    auto mismatch=fixture(scene({image("spring")},{sword}));mismatch.step(play(0,{0},7));reject(mismatch,defend(7,{0}));
    SnapshotState nine;for(unsigned i=0;i<9;++i)nine.players.push_back(player(100+i,i==0?std::initializer_list<CardId>{image("bronze-club")}:i==8?std::initializer_list<CardId>{sword}:std::initializer_list<CardId>{},static_cast<Team>(i%5)));
    Match multi;multi.load_scene(nine,100,9);multi.step(play(100,{0},108));multi.step(defend(108,{0}));check(multi.snapshot().actor==100&&multi.snapshot().owner==100&&multi.snapshot().queue[0].source==108,"ninth-seat reflection returns to arbitrary original ID, not binary seat arithmetic");roundtrip(multi);multi.step(defend(100));check(multi.snapshot().owner==101,"next turn follows original owner, independent of final response actor");
    MatchConfig cap;cap.max_decisions=2;auto cut=fixture(scene({image("bronze-club")},{sword}),cap);cut.step(play(0,{0},7));const auto cut_rng=cut.snapshot().rng;const auto result=cut.step(defend(7,{0}));check(result.truncated&&!result.terminated&&cut.snapshot().queue.empty()&&cut.snapshot().gifts==std::vector<std::uint32_t>{0,0}&&cut.snapshot().rng==cut_rng&&cut.snapshot().state.players[0].hp==40,"decision cutoff may interrupt reflection with no payload damage or gifts");roundtrip(cut);
}

void reflection_role_capacity_and_late_failures(){
    const auto sword=image("reflection-sword"), moon_armor=image("moonlight-helm"), mirror=image("super-mirror"), curtain=image("rainbow-curtain"), ordinary=image("aqua-shoes");
    auto armor=fixture(scene({image("bronze-club")},{moon_armor}));armor.step(play(0,{0},7));armor.step(defend(7,{0}));
    check(armor.snapshot().state.players[1].hp==40&&armor.snapshot().queue.empty(),"nonmatching magic reflector armor retains ordinary DEF against neutral weapon");roundtrip(armor);
    auto ordering=fixture(scene({image("bronze-club")},{ordinary,curtain,sword}));ordering.step(play(0,{0},7));reject(ordering,defend(7,{0,1}));reject(ordering,defend(7,{0,2}));
    auto filters=fixture(scene({image("bronze-club")},{curtain,curtain,sword}));filters.step(play(0,{0},7));filters.step(defend(7,{0,1,2}));
    check(filters.snapshot().queue[0].reflections[0].models==std::vector<CardId>{curtain,curtain,sword}&&filters.snapshot().gifts[1]==3&&!filters.snapshot().queue[0].element,"multiple leading filters followed by reflector preserve canonical ordered evidence");roundtrip(filters);filters.step(defend(0));roundtrip(filters);
    auto light=fixture(scene({elemental(Category::weapons,Element::light)},{curtain,ordinary}));light.step(play(0,{0},7));reject(light,defend(7,{1}));const auto raw=light.snapshot().queue[0].power;
    light.step(defend(7,{0,1}));check(light.snapshot().state.players[1].hp==40-std::max<std::int64_t>(0,raw-find_card(ordinary)->defense),"filter allows ordinary armor against otherwise undefendable Light");roundtrip(light);
    auto healing=scene({image("smile-dew")},{mirror});healing.players[0].hp=98;auto utility=fixture(healing);utility.step(play(0,{0},7));const auto before=utility.snapshot().queue[0];utility.step(defend(7,{0}));
    const auto& after=utility.snapshot().queue[0];check(after.category==Category::sundries&&after.ability==Ability::boost_hp&&after.amount==before.amount&&after.models==before.models&&after.tokens==before.tokens&&after.source==7&&after.target==0,"universal reflector preserves sundry recovery category/value and original physical payload");roundtrip(utility);
    utility.step(defend(0));check(utility.snapshot().state.players[0].hp==99&&utility.snapshot().state.players[1].hp==40,"reflected sundry recovery applies cap only to final recipient");roundtrip(utility);
    MatchConfig small;small.hand_limit=3;small.initial_hand=0;auto finite=fixture(scene({image("bronze-club"),sword,sword},{sword,sword,sword}),small);finite.step(play(0,{0},7));roundtrip(finite);
    for(const auto [actor,token]:{std::pair<PlayerId,ItemToken>{7,0},{0,1},{7,1},{0,2},{7,2}}){finite.step(defend(actor,{token}));roundtrip(finite);}
    check(finite.snapshot().state.players[0].items.empty()&&finite.snapshot().state.players[1].items.empty()&&finite.snapshot().gifts==std::vector<std::uint32_t>{3,3}&&finite.snapshot().queue[0].reflections.size()==5,"small-cap chain consumes both complete hands with bounded per-seat replacement debt");
    const auto exhausted=finite.snapshot();auto forged=exhausted;forged.queue[0].reflections[1].tokens[0]=forged.queue[0].tokens[0];bool threw=false;try{finite.restore(forged);}catch(const MatchError&){threw=true;}
    check(threw&&finite.snapshot()==exhausted,"same-seat original physical token cannot be reused by later reflection");
    finite.step(defend(0));check(finite.snapshot().state.players[0].hp==39&&finite.snapshot().state.players[0].items.size()==3&&finite.snapshot().state.players[1].items.size()==3,"final resolution delivers each reserved gift once without exceeding small hand limit");roundtrip(finite);
    auto update=fixture(scene({image("bronze-club"),sword},{sword}));update.step(play(0,{0},7));update.step(defend(7,{0}));auto latest=update.snapshot();latest.state.update_count=max_wire_integer;update.restore(latest);
    const auto update_before=update.snapshot();check(update.legal(defend(0,{1})),"pending reflected frame still has syntactically valid next reflection before counter exhaustion");threw=false;try{update.step(defend(0,{1}));}catch(const MatchError&){threw=true;}
    check(threw&&update.snapshot()==update_before,"late update exhaustion rolls back second reflection, consumed carrier, ledger, gifts and RNG");
    auto token_scene=scene({image("bronze-club")},{sword});token_scene.players[0].items[0].token=max_wire_integer;auto tokens=fixture(token_scene);tokens.step(play(0,{max_wire_integer},7));tokens.step(defend(7,{0}));roundtrip(tokens);
    const auto token_before=tokens.snapshot();check(tokens.legal(defend(0))&&token_before.queue[0].reflections.size()==1,"token exhaustion fixture has a valid reflected ledger before final response");threw=false;try{tokens.step(defend(0));}catch(const MatchError&){threw=true;}
    check(threw&&tokens.snapshot()==token_before,"late gift exhaustion after reflected payload rolls back HP, complete ledger, phase, reservations and RNG");
}

void reflection_chain_exceeds_legacy_limit(){
    auto state=scene();const auto sword=image("reflection-sword");
    state.players[0].items.push_back({0,image("bronze-club"),{},false});
    for(ItemToken token=1;token<18;++token)state.players[0].items.push_back({token,sword,{},false});
    for(ItemToken token=0;token<18;++token)state.players[1].items.push_back({token,sword,{},false});
    auto chain=fixture(state);chain.step(play(0,{0},7));roundtrip(chain);
    for(ItemToken token=0;token<18;++token){
        chain.step(defend(7,{token}));roundtrip(chain);
        check(chain.snapshot().owner==0&&chain.snapshot().queue[0].origin==0&&chain.snapshot().queue[0].initial_target==7&&chain.snapshot().queue[0].power==1,"long reflection chain preserves owner, origin and base power");
        if(token<17){chain.step(defend(0,{token+1}));roundtrip(chain);}
    }
    check(chain.snapshot().queue[0].reflections.size()==35&&chain.snapshot().actor==0&&chain.snapshot().gifts==std::vector<std::uint32_t>{18,18}
        &&chain.snapshot().state.players[0].items.empty()&&chain.snapshot().state.players[1].items.empty(),"35 physical reflections exceed old duel32 limit while respecting both eighteen-card inventories");
    const auto exact=chain.snapshot();auto old=exact;old.version=1;bool rejected=false;try{chain.restore(old);}catch(const MatchError&){rejected=true;}
    check(rejected&&chain.snapshot()==exact,"v1 checkpoint is explicitly rejected atomically by reflection-capable v2 schema");
    chain.step(defend(0));check(chain.snapshot().state.players[0].hp==39&&chain.snapshot().state.players[0].items.size()==18&&chain.snapshot().state.players[1].items.size()==18&&chain.snapshot().owner==7,"long chain resolves original one-point payload and refills both hands exactly to capacity");roundtrip(chain);
}

void reflection_snapshot_adversaries(){
    const auto sword=image("reflection-sword"), curtain=image("rainbow-curtain");
    auto m=fixture(scene({image("bronze-club"),sword},{sword}));m.step(play(0,{0},7));m.step(defend(7,{0}));m.step(defend(0,{1}));const auto exact=m.snapshot();
    for(int error=0;error<15;++error){auto bad=exact;auto& effect=bad.queue[0];
        if(error==0)effect.source=7;if(error==1)effect.target=0;if(error==2)effect.initial_target=0;if(error==3)effect.origin=7;
        if(error==4)effect.reflections[0].defender=0;if(error==5)effect.reflections[0].models[0]=image("bronze-club");
        if(error==6)effect.reflections[0].tokens.clear();if(error==7)effect.reflections[0].tokens[0]=bad.next_token;
        if(error==8)effect.reflections[1].tokens[0]=effect.tokens[0];
        if(error==9)bad.state.players[1].items.push_back({0,sword,{},false});
        if(error==10)bad.gifts[1]=0;if(error==11)bad.gifts[0]=1;if(error==12)bad.decisions=2;
        if(error==13)effect.reflections.push_back(effect.reflections[0]);if(error==14)effect.reflections[0].models.push_back(curtain);
        bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"reflection transcript endpoint/type/token/payment/gift/decision forgery rejects transactionally");}
    auto spell=fixture(scene({image("spring")},{image("moonlight-helm")}));spell.step(play(0,{0},7));spell.step(defend(7,{0}));const auto saved=spell.snapshot();
    auto bad=saved;bad.state.players[0].mp=93;bool rejected=false;try{spell.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&spell.snapshot()==saved,"reflected miracle MP provenance remains charged to immutable origin");
    auto different=saved;different.rng^=0xfedcba9876543210ULL;Match copy;copy.restore(different);std::uint64_t a=456,b=a;const auto before=spell.snapshot();const auto ca=spell.sample_action(a),cb=copy.sample_action(b);
    check(ca==cb&&a==b&&spell.snapshot()==before&&spell.legal(ca),"reaction sampler depends only on visible own inventory and pending payload, not environmental future RNG");
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
int main(){scope_reset_and_local_contract();weapon_frames_colors_owner_and_fixed_rewards();recovery_retention_capacity_and_atomicity();reflection_filter_and_immutable_origin();reflection_snapshot_adversaries();reflection_chain_exceeds_legacy_limit();reflection_role_capacity_and_late_failures();checkpoint_cutoff_and_seeded_multiplayer();std::cout<<"Experimental official match passed (139subset/reflection/filter/9teams/owner/retention/atomicity/snapshot/RNG/cutoff; server scheduling not asserted).\n";}
