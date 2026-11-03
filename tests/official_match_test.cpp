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
    const auto pool=match_catalog();check(pool.size()==236&&match_gift_weight()==499&&std::string_view(match_ruleset)=="official-execution-subset-v41"
        &&std::string_view(match_scope).find("not-official-server-equivalence")!=std::string_view::npos,"236model/499weight execution subset is explicitly experimental");
    std::array<int,8> counts{};int weight=0;std::set<CardId> unique;for(auto id:pool){const auto* c=find_card(id);check(c&&match_supported(id)&&unique.insert(id).second,"subset strict supported allowlist without duplicates");++counts[static_cast<int>(c->category)];weight+=c->gift_rate;}
    check(counts==std::array<int,8>{3,107,78,18,30,0,0,0}&&weight==499,"exchange, sell and buy are admitted among trade cards");
    check(match_supported(3)&&match_supported(4)&&match_supported(5)&&!match_supported(1)&&!match_supported(2),"trade artifacts do not enable sacrifice");
    check(!match_supported(0)&&match_supported(image("dreaming-hat"))&&!match_supported(image("string-of-fate")),"specials, percentages and guardians never silently become plain damage");
    Match provenance;
    const auto metadata=provenance.observe(0);
    check(metadata.find("\"catalog_version\":\"official-v2\"")!=std::string::npos
        &&metadata.find("\"catalog_sha256\":\""+std::string(catalog_sha256)+"\"")!=std::string::npos
        &&metadata.find("\"client_sha256\":\""+std::string(client_sha256)+"\"")!=std::string::npos,"execution view identifies its pinned catalog and public client without revealing environmental state");
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
    auto no_room=fixture(full);const auto full_rng=no_room.snapshot().rng;no_room.step(pray(0));check(no_room.snapshot().state.players[0].items.size()==18&&no_room.snapshot().rng==full_rng&&no_room.snapshot().owner==7,"full no-weapon hand prayer advances turn without draw or overflow");roundtrip(no_room);
    auto discard=fixture(scene({image("aqua-shoes"),image("aqua-shoes")},{}));const auto oldrng=discard.snapshot().rng;
    discard.step({0,CommandKind::discard,{0,1},{}});check(discard.snapshot().state.players[0].items.empty()&&discard.snapshot().rng==oldrng,"multiple ordinary discard earns no gifts/RNG draw");roundtrip(discard);
    auto invalid=fixture(scene({image("spring"),image("bronze-club")},{}));state=invalid.snapshot().state;state.players[0].mp=6;invalid.load_scene(state,0,9);
    reject(invalid,play(0,{0},0));reject(invalid,play(0,{1,1},7));reject(invalid,play(999,{1},7));reject(invalid,play(0,{1},999));reject(invalid,pray(0));reject(invalid,{0,CommandKind::discard,{1},{}});
    const auto before=invalid.snapshot();for(int error=0;error<6;++error){auto bad=state;if(error==0)bad.players[0].items[0].model=1;if(error==1)bad.players[0].items[0].fake_model=image("aqua-shoes");
        if(error==2)bad.players[0].curses={Curse::cold,Curse::fever};if(error==3)bad.players[0].mp=100;if(error==4)bad.players[0].confusion_count=-1;if(error==5)bad.tiebreak_gf=-1;
        bool rejected=false;try{invalid.load_scene(bad,0,9);}catch(const MatchError&){rejected=true;}check(rejected&&invalid.snapshot()==before,"unsupported scene feature rejected atomically rather than ignored");}
}

void reflection_filter_and_immutable_origin(){
    const auto sword=image("reflection-sword"), moon=image("moonlight-axe"), mirror=image("super-mirror"), curtain=image("rainbow-curtain");
    for(const auto id:{sword,moon,image("moonlight-helm"),image("moonlight-shield"),image("moonlight-armor"),mirror,curtain})check(match_supported(id),"explicit seven-model reaction/filter extension closes supported pool");
    for(const auto id:{sword,moon}){auto main=fixture(scene({id},{}));main.step(play(0,{0},7));check(main.snapshot().queue[0].power==find_card(id)->atk,"reflection weapon retains its ordinary main ATK");main.step(defend(7));roundtrip(main);}
    auto chain=fixture(scene({image("bronze-club"),sword},{sword}));chain.step(play(0,{0},7));const auto origin=chain.snapshot().queue[0];const auto rng=chain.snapshot().rng;
    chain.step(defend(7,{0}));const auto first=chain.snapshot();check(first.owner==0&&first.actor==0&&first.queue[0].source==7&&first.queue[0].target==0&&first.queue[0].origin==0&&first.queue[0].initial_target==7
        &&first.queue[0].models==origin.models&&first.queue[0].tokens==origin.tokens&&first.queue[0].power==1&&first.queue[0].redirects.size()==1&&first.gifts==std::vector<std::uint32_t>{1,1}&&first.rng==rng,"physical reflection swaps current endpoints without recasting, drawing or changing immutable payload");roundtrip(chain);
    chain.step(defend(0,{1}));check(chain.snapshot().actor==7&&chain.snapshot().queue[0].source==0&&chain.snapshot().queue[0].target==7&&chain.snapshot().queue[0].redirects.size()==2
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
    check(filters.snapshot().queue[0].redirects[0].models==std::vector<CardId>{curtain,curtain,sword}&&filters.snapshot().gifts[1]==3&&!filters.snapshot().queue[0].element,"multiple leading filters followed by reflector preserve canonical ordered evidence");roundtrip(filters);filters.step(defend(0));roundtrip(filters);
    auto light=fixture(scene({elemental(Category::weapons,Element::light)},{curtain,ordinary}));light.step(play(0,{0},7));reject(light,defend(7,{1}));const auto raw=light.snapshot().queue[0].power;
    light.step(defend(7,{0,1}));check(light.snapshot().state.players[1].hp==40-std::max<std::int64_t>(0,raw-find_card(ordinary)->defense),"filter allows ordinary armor against otherwise undefendable Light");roundtrip(light);
    auto healing=scene({image("smile-dew")},{mirror});healing.players[0].hp=98;auto utility=fixture(healing);utility.step(play(0,{0},7));const auto before=utility.snapshot().queue[0];utility.step(defend(7,{0}));
    const auto& after=utility.snapshot().queue[0];check(after.category==Category::sundries&&after.ability==Ability::boost_hp&&after.amount==before.amount&&after.models==before.models&&after.tokens==before.tokens&&after.source==7&&after.target==0,"universal reflector preserves sundry recovery category/value and original physical payload");roundtrip(utility);
    utility.step(defend(0));check(utility.snapshot().state.players[0].hp==99&&utility.snapshot().state.players[1].hp==40,"reflected sundry recovery applies cap only to final recipient");roundtrip(utility);
    MatchConfig small;small.hand_limit=3;small.initial_hand=0;auto finite=fixture(scene({image("bronze-club"),sword,sword},{sword,sword,sword}),small);finite.step(play(0,{0},7));roundtrip(finite);
    for(const auto [actor,token]:{std::pair<PlayerId,ItemToken>{7,0},{0,1},{7,1},{0,2},{7,2}}){finite.step(defend(actor,{token}));roundtrip(finite);}
    check(finite.snapshot().state.players[0].items.empty()&&finite.snapshot().state.players[1].items.empty()&&finite.snapshot().gifts==std::vector<std::uint32_t>{3,3}&&finite.snapshot().queue[0].redirects.size()==5,"small-cap chain consumes both complete hands with bounded per-seat replacement debt");
    const auto exhausted=finite.snapshot();auto forged=exhausted;forged.queue[0].redirects[1].tokens[0]=forged.queue[0].tokens[0];bool threw=false;try{finite.restore(forged);}catch(const MatchError&){threw=true;}
    check(threw&&finite.snapshot()==exhausted,"same-seat original physical token cannot be reused by later reflection");
    finite.step(defend(0));check(finite.snapshot().state.players[0].hp==39&&finite.snapshot().state.players[0].items.size()==3&&finite.snapshot().state.players[1].items.size()==3,"final resolution delivers each reserved gift once without exceeding small hand limit");roundtrip(finite);
    auto update=fixture(scene({image("bronze-club"),sword},{sword}));update.step(play(0,{0},7));update.step(defend(7,{0}));auto latest=update.snapshot();latest.state.update_count=max_wire_integer;latest.batch->initial_state.update_count=max_wire_integer-(latest.decisions-latest.batch->decisions);update.restore(latest);
    const auto update_before=update.snapshot();check(update.legal(defend(0,{1})),"pending reflected frame still has syntactically valid next reflection before counter exhaustion");threw=false;try{update.step(defend(0,{1}));}catch(const MatchError&){threw=true;}
    check(threw&&update.snapshot()==update_before,"late update exhaustion rolls back second reflection, consumed carrier, ledger, gifts and RNG");
    auto token_scene=scene({image("bronze-club")},{sword});token_scene.players[0].items[0].token=max_wire_integer;auto tokens=fixture(token_scene);tokens.step(play(0,{max_wire_integer},7));tokens.step(defend(7,{0}));roundtrip(tokens);
    const auto token_before=tokens.snapshot();check(tokens.legal(defend(0))&&token_before.queue[0].redirects.size()==1,"token exhaustion fixture has a valid reflected ledger before final response");threw=false;try{tokens.step(defend(0));}catch(const MatchError&){threw=true;}
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
    check(chain.snapshot().queue[0].redirects.size()==35&&chain.snapshot().actor==0&&chain.snapshot().gifts==std::vector<std::uint32_t>{18,18}
        &&chain.snapshot().state.players[0].items.empty()&&chain.snapshot().state.players[1].items.empty(),"35 physical reflections exceed old duel32 limit while respecting both eighteen-card inventories");
    const auto exact=chain.snapshot();for(std::uint32_t version=1;version<match_snapshot_version;++version){auto old=exact;old.version=version;bool rejected=false;try{chain.restore(old);}catch(const MatchError&){rejected=true;}
    check(rejected&&chain.snapshot()==exact,"older checkpoints are explicitly rejected atomically");}
    chain.step(defend(0));check(chain.snapshot().state.players[0].hp==39&&chain.snapshot().state.players[0].items.size()==18&&chain.snapshot().state.players[1].items.size()==18&&chain.snapshot().owner==7,"long chain resolves original one-point payload and refills both hands exactly to capacity");roundtrip(chain);
}

void reflection_snapshot_adversaries(){
    const auto sword=image("reflection-sword"), curtain=image("rainbow-curtain");
    auto m=fixture(scene({image("bronze-club"),sword},{sword}));m.step(play(0,{0},7));m.step(defend(7,{0}));m.step(defend(0,{1}));const auto exact=m.snapshot();
    for(int error=0;error<15;++error){auto bad=exact;auto& effect=bad.queue[0];
        if(error==0)effect.source=7;if(error==1)effect.target=0;if(error==2)effect.initial_target=0;if(error==3)effect.origin=7;
        if(error==4)effect.redirects[0].defender=0;if(error==5)effect.redirects[0].models[0]=image("bronze-club");
        if(error==6)effect.redirects[0].tokens.clear();if(error==7)effect.redirects[0].tokens[0]=bad.next_token;
        if(error==8)effect.redirects[1].tokens[0]=effect.tokens[0];
        if(error==9)bad.state.players[1].items.push_back({0,sword,{},false});
        if(error==10)bad.gifts[1]=0;if(error==11)bad.gifts[0]=1;if(error==12)bad.decisions=2;
        if(error==13)effect.redirects.push_back(effect.redirects[0]);if(error==14)effect.redirects[0].models.push_back(curtain);
        bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"reflection transcript endpoint/type/token/payment/gift/decision forgery rejects transactionally");}
    auto spell=fixture(scene({image("spring")},{image("moonlight-helm")}));spell.step(play(0,{0},7));spell.step(defend(7,{0}));const auto saved=spell.snapshot();
    auto bad=saved;bad.state.players[0].mp=93;bool rejected=false;try{spell.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&spell.snapshot()==saved,"reflected miracle MP provenance remains charged to immutable origin");
    auto different=saved;different.rng^=0xfedcba9876543210ULL;different.queue[0].redirect_rng=different.rng;different.batch->rng=different.rng;Match copy;copy.restore(different);std::uint64_t a=456,b=a;const auto before=spell.snapshot();const auto ca=spell.sample_action(a),cb=copy.sample_action(b);
    check(ca==cb&&a==b&&spell.snapshot()==before&&spell.legal(ca),"reaction sampler depends only on visible own inventory and pending payload, not environmental future RNG");
}

void blocking_roles_retention_and_reflection(){
    const auto wall=image("wall"), curtain=image("rainbow-curtain"), sword=image("reflection-sword");
    const std::array<CardId,8> blockers{image("angel-knife"),image("angel-sword"),image("angel-axe"),image("angel-bow"),image("angel-gauntlet"),image("angel-cap"),image("angel-shield"),image("angel-armor")};
    for(auto id:blockers){check(match_supported(id),"all eight explicit magic-block physical models supported");auto m=fixture(scene({image("spring")},{id}));m.step(play(0,{0},7));m.step(defend(7,{0}));
        check(m.snapshot().queue.empty()&&m.snapshot().state.players[0].mp==3&&m.snapshot().state.players[0].items[0].used&&m.snapshot().state.players[0].hp==40&&m.snapshot().state.players[1].hp==40&&m.snapshot().state.players[1].items.size()==1,"matching magic block cancels recovery but retains original MP payment/holding and physical replacement");roundtrip(m);}
    for(auto id:{blockers[0],blockers[1],blockers[2],blockers[3]}){auto m=fixture(scene({id},{}));m.step(play(0,{0},7));check(m.snapshot().queue[0].power==find_card(id)->atk,"block weapon retains declared ordinary main attack");m.step(defend(7));roundtrip(m);}
    auto plus=fixture(scene({image("bronze-club"),image("angel-bow")},{}));plus.step(play(0,{0,1},7));check(plus.snapshot().queue[0].power==16,"angel bow can contribute plus15 without conferring block ability to attack payload");plus.step(defend(7));roundtrip(plus);
    for(auto id:{blockers[4],blockers[5],blockers[6],blockers[7]}){auto m=fixture(scene({image("bronze-club")},{id}));m.step(play(0,{0},7));m.step(defend(7,{0}));check(m.snapshot().state.players[1].hp==40,"nonmatching magic-block armor remains ordinary weapon defense");roundtrip(m);}
    auto state=scene({image("bronze-club"),image("bronze-club")},{wall,image("smile-dew")});state.players[1].mp=12;auto retained=fixture(state);reject(retained,play(0,{0},999));retained.step(play(0,{0},7));retained.step(defend(7,{0}));
    check(retained.snapshot().state.players[1].mp==6&&retained.snapshot().state.players[1].items[0].used&&retained.snapshot().state.players[1].items[0].model==wall,"first Wall costs6 and retains exact physical miracle token");roundtrip(retained);
    retained.step(play(7,{1},7));check(retained.snapshot().owner==0,"known defender recovery advances actual turn back to original owner without scene reload");retained.step(play(0,{1},7));retained.step(defend(7,{0}));
    check(retained.snapshot().state.players[1].mp==0&&std::count_if(retained.snapshot().state.players[1].items.begin(),retained.snapshot().state.players[1].items.end(),[&](auto& item){return item.model==wall&&item.token==0&&item.used;})==1,"retained Wall pays6 again while retaining same token without duplicating ownership");roundtrip(retained);
    auto main=fixture(scene({wall},{}));reject(main,play(0,{0},7));check(main.legal(pray(0)),"response-only Wall does not become main play or prevent no-weapon prayer");
    auto poor=scene({image("bronze-club")},{wall});poor.players[1].mp=5;auto insufficient=fixture(poor);insufficient.step(play(0,{0},7));reject(insufficient,defend(7,{0}));insufficient.step(defend(7));roundtrip(insufficient);
    auto colored=fixture(scene({elemental(Category::weapons,Element::fire)},{curtain,wall}));colored.step(play(0,{0},7));reject(colored,defend(7,{1}));colored.step(defend(7,{0,1}));check(colored.snapshot().state.players[1].hp==40&&colored.snapshot().state.players[1].mp==4&&colored.snapshot().state.players[1].items[0].used,"leading filter enables Wall on colored weapon and charges cost exactly once");roundtrip(colored);
    auto full=scene({image("bronze-club")},{wall});for(ItemToken i=1;i<18;++i)full.players[1].items.push_back({i,image("aqua-shoes"),{},false});auto packed=fixture(full);const auto next=packed.snapshot().next_token;packed.step(play(0,{0},7));packed.step(defend(7,{0}));
    check(packed.snapshot().state.players[1].items.size()==18&&packed.snapshot().next_token==next+1&&packed.snapshot().state.players[1].items[0].used,"full-hand retained block grants no defender gift while physical attacker gets its one replacement");roundtrip(packed);
    auto bounced=fixture(scene({image("bronze-club"),wall},{sword}));bounced.step(play(0,{0},7));bounced.step(defend(7,{0}));roundtrip(bounced);bounced.step(defend(0,{1}));
    check(bounced.snapshot().queue.empty()&&bounced.snapshot().state.players[0].hp==40&&bounced.snapshot().state.players[0].mp==4&&bounced.snapshot().owner==7,"Wall can terminate a reflected weapon without reapplying original payload");roundtrip(bounced);
    auto magic=fixture(scene({image("spring"),image("angel-knife")},{image("moonlight-helm")}));magic.step(play(0,{0},7));magic.step(defend(7,{0}));roundtrip(magic);magic.step(defend(0,{1}));check(magic.snapshot().state.players[0].hp==40&&magic.snapshot().state.players[1].hp==40&&magic.snapshot().state.players[0].mp==3,"magic block after reflection cancels utility while preserving initial caster payment");roundtrip(magic);
    for(const auto name:{"fireball","darkness","meteor","treasure"}){auto state=scene({image(name)},{image("angel-knife")});state.players[0].mp=99;state.players[1].cp=98;auto canceled=fixture(state);canceled.step(play(0,{0},7));const auto paid=canceled.snapshot().state.players[0].mp;
        canceled.step(defend(7,{0}));check(canceled.snapshot().state.players[0].hp==40&&canceled.snapshot().state.players[1].hp==40&&canceled.snapshot().state.players[1].cp==98&&canceled.snapshot().state.players[0].mp==paid,"magic block cancels Fire/Dark/Light damage and CP recovery without refunding declared cost");roundtrip(canceled);}
    auto ordering=fixture(scene({image("bronze-club")},{image("aqua-shoes"),curtain,wall}));ordering.step(play(0,{0},7));reject(ordering,defend(7,{0,2}));reject(ordering,defend(7,{0,1,2}));reject(ordering,defend(7,{2,0}));
    auto suffix=fixture(scene({image("spring")},{image("angel-knife"),image("aqua-shoes")}));suffix.step(play(0,{0},7));reject(suffix,defend(7,{0,1}));
    auto selfstate=scene({image("bronze-club"),wall},{});selfstate.players[0].mp=6;auto self=fixture(selfstate);self.step(play(0,{0},0));self.step(defend(0,{1}));check(self.snapshot().state.players[0].hp==40&&self.snapshot().state.players[0].mp==0&&self.snapshot().state.players[0].items[0].used&&self.snapshot().owner==7,"exactly6MP Wall cancels self weapon and retains itself through actual next turn");roundtrip(self);
    SnapshotState nine;for(unsigned i=0;i<9;++i)nine.players.push_back(player(100+i,i==0?std::initializer_list<CardId>{image("bronze-club"),sword}:i==8?std::initializer_list<CardId>{sword,wall}:std::initializer_list<CardId>{},static_cast<Team>(i%5)));
    nine.players[8].mp=6;Match team;team.load_scene(nine,100,99);team.step(play(100,{0},108));team.step(defend(108,{0}));roundtrip(team);team.step(defend(100,{1}));roundtrip(team);
    const auto pending=team.snapshot();auto fake=pending;fake.queue[0].redirects[0].models[0]=wall;fake.queue[0].redirects[0].tokens[0]=1;fake.state.players[8].items[0].used=true;bool invalid_ledger=false;try{team.restore(fake);}catch(const MatchError&){invalid_ledger=true;}
    check(invalid_ledger&&team.snapshot()==pending,"block action cannot be forged into a pending reflection transcript even with an owned retained Wall token");
    team.step(defend(108,{1}));check(team.snapshot().owner==101&&team.snapshot().queue.empty()&&team.snapshot().state.players[0].hp==40&&team.snapshot().state.players[8].hp==40&&team.snapshot().state.players[0].items.size()==2&&team.snapshot().state.players[8].items.size()==3&&team.snapshot().state.players[8].mp==0,"ninth seat blocks after two reflections, pays all participant gifts and advances from immutable original owner");roundtrip(team);
    MatchConfig pair;pair.initial_hand=0;pair.hand_limit=2;auto crowded=fixture(scene({image("bronze-club")},{curtain,wall}),pair);const auto next_pair=crowded.snapshot().next_token;crowded.step(play(0,{0},7));crowded.step(defend(7,{0,1}));
    check(crowded.snapshot().state.players[1].items.size()==2&&crowded.snapshot().state.players[1].items[0].token==1&&crowded.snapshot().state.players[1].items[0].used&&crowded.snapshot().state.players[1].mp==4&&crowded.snapshot().next_token==next_pair+2,"cap2 filter plus retained Wall replaces only the single freed defender slot, not both used models");roundtrip(crowded);
    MatchConfig small;small.initial_hand=0;small.hand_limit=1;auto minimal=fixture(scene({image("bronze-club")},{wall}),small);minimal.step(play(0,{0},7));minimal.step(defend(7,{0}));check(minimal.snapshot().state.players[0].items.size()==1&&minimal.snapshot().state.players[1].items.size()==1,"one-card capacity permits retained blocker without overfilling");roundtrip(minimal);
    MatchConfig limit;limit.max_decisions=2;auto cut=fixture(scene({image("bronze-club")},{wall}),limit);cut.step(play(0,{0},7));const auto before_rng=cut.snapshot().rng;const auto clipped=cut.step(defend(7,{0}));check(clipped.truncated&&!clipped.terminated&&cut.snapshot().queue.empty()&&cut.snapshot().state.players[1].mp==4&&cut.snapshot().state.players[1].items[0].used&&cut.snapshot().rng==before_rng,"block at cutoff commits cost/retention but suppresses gifts and damage");roundtrip(cut);
    auto update=fixture(scene({image("spring")},{blockers[0]}));update.step(play(0,{0},7));auto exhausted=update.snapshot();exhausted.state.update_count=max_wire_integer;exhausted.batch->initial_state.update_count=max_wire_integer-(exhausted.decisions-exhausted.batch->decisions);update.restore(exhausted);const auto exact=update.snapshot();bool rejected=false;try{update.step(defend(7,{0}));}catch(const MatchError&){rejected=true;}
    check(rejected&&update.snapshot()==exact,"late update failure rolls back block consumption and payload cancellation");
    auto scarce=scene({image("bronze-club")},{wall});scarce.players[0].items[0].token=max_wire_integer;auto gifts=fixture(scarce);gifts.step(play(0,{max_wire_integer},7));const auto gift_before=gifts.snapshot();rejected=false;try{gifts.step(defend(7,{0}));}catch(const MatchError&){rejected=true;}
    check(rejected&&gifts.snapshot()==gift_before,"late replacement exhaustion rolls back retained block MP, used flag, queue and RNG");
}

void ordered_free_setter_and_double(){
    const auto aura=image("aura"),doll=image("spiritual-doll"),staff=image("spiritual-staff"),fire=image("wand-of-ignition"),water=image("wand-of-mystic-water");
    const std::array<CardId,5> frees{staff,image("spiritual-socks"),image("spiritual-hood"),image("spiritual-sash"),doll};
    for(auto carrier:frees){auto state=scene({image("fireball"),carrier},{});state.players[0].mp=0;auto m=fixture(state);check(m.legal(play(0,{0,1},7)),"final MC immediately followed by FREE is legal even if its isolated prefix is unaffordable");m.step(play(0,{0,1},7));
        check(m.snapshot().state.players[0].mp==0&&m.snapshot().queue[0].power==2&&m.snapshot().queue[0].element==Element::fire&&m.snapshot().state.players[0].items.size()==1&&m.snapshot().state.players[0].items[0].used&&m.snapshot().gifts[0]==2,"all five FREE roles waive immediate cost, contribute no attack/attribute and are physically consumed");roundtrip(m);m.step(defend(7));check(m.snapshot().state.players[1].hp==38,"FREE weapon ATK never adds to preceding miracle");roundtrip(m);}
    auto sword=fixture(scene({staff},{}));sword.step(play(0,{0},7));check(sword.snapshot().queue[0].power==12,"FREE staff retains its own ordinary weapon base12");sword.step(defend(7));
    for(auto armor:{frees[1],frees[2],frees[3]}){auto m=fixture(scene({image("bronze-club")},{armor}));m.step(play(0,{0},7));m.step(defend(7,{0}));check(m.snapshot().state.players[1].hp==40,"FREE armor retains ordinary physical DEF");roundtrip(m);}
    auto forbidden=fixture(scene({aura,doll},{}));reject(forbidden,play(0,{0},7));reject(forbidden,play(0,{1},0));
    for(bool double_last:{false,true}){auto state=scene({image("bronze-club"),aura,image("crossbow")},{});state.players[0].mp=6;auto m=fixture(state);m.step(double_last?play(0,{0,2,1},7):play(0,{0,1,2},7));
        check(m.snapshot().queue[0].power==(double_last?6:4)&&m.snapshot().state.players[0].mp==0&&m.snapshot().state.players[0].items.size()==1&&m.snapshot().state.players[0].items[0].used,"Aura doubles only the power accumulated before its ordered position and retains itself for six MP");roundtrip(m);m.step(defend(7));roundtrip(m);}
    for(bool water_last:{false,true}){auto m=fixture(scene({image("bronze-club"),fire,water},{}));m.step(water_last?play(0,{0,1,2},7):play(0,{0,2,1},7));check(m.snapshot().queue[0].power==8&&m.snapshot().queue[0].element==(water_last?Element::water:Element::fire),"current setters add their ATK and overwrite attribute in selected order");roundtrip(m);m.step(defend(7));}
    for(bool setter_last:{false,true}){const auto base=elemental(Category::weapons,Element::fire);auto state=scene({base,aura,fire},{});state.players[0].mp=6;auto m=fixture(state);m.step(setter_last?play(0,{0,1,2},7):play(0,{0,2,1},7));
        check(m.snapshot().queue[0].power==(setter_last?2*find_card(base)->atk+2:2*(find_card(base)->atk+2))&&m.snapshot().queue[0].element==(setter_last?Attribute{Element::fire}:Attribute{}),"Aura uses normal neutral attribute merge: setter after Aura restores Fire, Aura after setter neutralizes it");roundtrip(m);m.step(defend(7));}
    auto staffstate=scene({staff,aura},{});staffstate.players[0].mp=0;auto unpaid=fixture(staffstate);reject(unpaid,play(0,{0,1},7));staffstate.players[0].mp=6;auto charged=fixture(staffstate);charged.step(play(0,{0,1},7));check(charged.snapshot().queue[0].power==24&&charged.snapshot().state.players[0].mp==0,"FREE ability on the base staff does not waive a following miracle");roundtrip(charged);
    auto againstate=scene({image("bronze-club"),image("bronze-club"),aura},{image("smile-dew")});againstate.players[0].mp=12;auto again=fixture(againstate);again.step(play(0,{0,2},7));again.step(defend(7));again.step(play(7,{0},7));again.step(play(0,{1,2},7));
    check(again.snapshot().queue[0].power==2&&again.snapshot().state.players[0].mp==0&&std::count_if(again.snapshot().state.players[0].items.begin(),again.snapshot().state.players[0].items.end(),[&](auto& i){return i.token==2&&i.model==aura&&i.used;})==1,"retained Aura is reused in a second actual turn with the same token and another six-MP charge");roundtrip(again);again.step(defend(7));roundtrip(again);
    auto many=scene({image("bronze-club"),aura,doll,aura,frees[1]},{});many.players[0].mp=0;auto pairs=fixture(many);pairs.step(play(0,{0,1,2,3,4},7));check(pairs.snapshot().queue[0].power==4&&pairs.snapshot().state.players[0].mp==0&&pairs.snapshot().state.players[0].items.size()==2&&std::all_of(pairs.snapshot().state.players[0].items.begin(),pairs.snapshot().state.players[0].items.end(),[](auto& i){return i.used;}),"multiple immediate MC/FREE pairs are independently waived in one physical-token-unique sequence");roundtrip(pairs);
    const auto exact=pairs.snapshot();auto bad=exact;bad.queue[0].models[2]=image("crossbow");bool rejected=false;try{pairs.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&pairs.snapshot()==exact,"restore recomputes FREE provenance rather than trusting a waived-cost or power claim");pairs.step(defend(7));roundtrip(pairs);
    auto separated=scene({image("bronze-club"),aura,image("crossbow"),aura,doll},{});separated.players[0].mp=5;auto missing=fixture(separated);reject(missing,play(0,{0,1,2,3,4},7));separated.players[0].mp=6;auto paid=fixture(separated);paid.step(play(0,{0,1,2,3,4},7));check(paid.snapshot().queue[0].power==8&&paid.snapshot().state.players[0].mp==0,"later FREE waives only immediately preceding second Aura, leaving first Aura's six-MP payment intact");roundtrip(paid);
    auto reflected=scene({image("bronze-club"),aura,doll,fire},{image("rainbow-curtain"),image("reflection-sword")});reflected.players[0].mp=0;auto reflect=fixture(reflected);reflect.step(play(0,{0,1,2,3},7));const auto initial=reflect.snapshot().queue[0];reflect.step(defend(7,{0,1}));check(reflect.snapshot().queue[0].power==4&&!reflect.snapshot().queue[0].element&&reflect.snapshot().queue[0].models==initial.models&&reflect.snapshot().queue[0].tokens==initial.tokens&&reflect.snapshot().state.players[0].mp==0,"filter and reflection preserve precomputed ordered/FREE attack rather than recomposing or recharging it");roundtrip(reflect);reflect.step(defend(0));check(reflect.snapshot().state.players[0].hp==36,"reflected composed payload resolves once");roundtrip(reflect);
}

void free_response_capacity_sampler_and_atomicity(){
    const auto wall=image("wall"),doll=image("spiritual-doll");auto state=scene({image("bronze-club")},{wall,doll});state.players[1].mp=0;auto m=fixture(state);m.step(play(0,{0},7));reject(m,defend(7,{0}));check(m.legal(defend(7,{0,1})),"final FREE makes zero-MP Wall response legal");
    const auto before=m.snapshot();bool found=false;for(std::uint64_t seed=0;seed<256;++seed){auto rng=seed;const auto command=m.sample_action(rng);check(m.legal(command)&&m.snapshot()==before,"zero-MP response sampler only returns legal commands without changing environment");found|=command.items==std::vector<ItemToken>{0,1};}check(found,"zero-MP sampler can reach Wall/FREE pair rather than filtering out unaffordable prefix");
    m.step(defend(7,{0,1}));check(m.snapshot().state.players[1].hp==40&&m.snapshot().state.players[1].mp==0&&m.snapshot().state.players[1].items.size()==3&&m.snapshot().state.players[1].items[0].token==0&&m.snapshot().state.players[1].items[0].used,"block/FREE local contract consumes suffix, retains Wall and replaces both selected uses");roundtrip(m);
    auto mainstate=scene({image("spring"),doll},{});mainstate.players[0].mp=0;auto main=fixture(mainstate);const auto main_before=main.snapshot();found=false;for(std::uint64_t seed=0;seed<256;++seed){auto rng=seed;const auto command=main.sample_action(rng);check(main.legal(command)&&main.snapshot()==main_before,"zero-MP main sampler does not mutate environmental RNG or holdings");found|=command.kind==CommandKind::play&&command.items==std::vector<ItemToken>{0,1};}check(found,"zero-MP sampler reaches recovery/FREE pairs");main.step(play(0,{0,1},0));check(main.snapshot().state.players[0].hp==50&&main.snapshot().state.players[0].mp==0,"free retained recovery executes without MP debt");roundtrip(main);
    MatchConfig cap;cap.initial_hand=0;cap.hand_limit=2;auto small=fixture(state,cap);small.step(play(0,{0},7));const auto token=small.snapshot().next_token;small.step(defend(7,{0,1}));check(small.snapshot().state.players[1].items.size()==2&&small.snapshot().next_token==token+2,"full cap2 Wall/FREE consumes carrier but reserves only one freed defender slot plus attacker replacement");roundtrip(small);
    auto late=fixture(state);late.step(play(0,{0},7));auto exhausted=late.snapshot();exhausted.state.update_count=max_wire_integer;exhausted.batch->initial_state.update_count=max_wire_integer-(exhausted.decisions-exhausted.batch->decisions);late.restore(exhausted);const auto saved=late.snapshot();bool rejected=false;try{late.step(defend(7,{0,1}));}catch(const MatchError&){rejected=true;}check(rejected&&late.snapshot()==saved,"late failure restores both retained MC and consumed FREE suffix transactionally");
    auto lookahead_state=scene({image("bronze-club"),doll,doll,image("aura"),image("aura")},{});lookahead_state.players[0].mp=0;auto lookahead=fixture(lookahead_state);const auto lookahead_before=lookahead.snapshot();bool extra_pair=false,both_pairs=false;
    for(std::uint64_t seed=0;seed<=512;++seed){auto rng=seed;const auto command=lookahead.sample_action(rng);check(lookahead.legal(command)&&lookahead.snapshot()==lookahead_before,"zero-MP sampler rechecks earlier-scanned FREE carriers for later extra miracles without state mutation");
        if(command.kind==CommandKind::play&&!command.items.empty()&&command.items[0]==0){const auto& ids=command.items;extra_pair|=ids.size()>=3&&(ids[1]==3||ids[1]==4)&&(ids[2]==1||ids[2]==2);both_pairs|=ids.size()==5;}}
    check(extra_pair&&both_pairs,"weapon-start sampler reaches both one and two extra Aura/FREE atomic pairs even when all carriers precede miracles in inventory order");
    SnapshotState matrix;for(unsigned i=0;i<9;++i)matrix.players.push_back(player(200+i));matrix.players[0].mp=0;for(ItemToken token=0;token<18;++token)matrix.players[0].items.push_back({token,token<9?image("spring"):doll,{},false});
    Match many_pairs;many_pairs.load_scene(matrix,200,7);const auto matrix_before=many_pairs.snapshot();for(std::uint64_t seed=0;seed<128;++seed){auto rng=seed;const auto command=many_pairs.sample_action(rng);check(many_pairs.legal(command)&&many_pairs.snapshot()==matrix_before,"nine-player full-hand 81 FREE pairs times nine targets fit sampler proposal capacity and preserve state");}
    SnapshotState nine;for(unsigned i=0;i<9;++i)nine.players.push_back(player(100+i,i==0?std::initializer_list<CardId>{image("bronze-club"),image("aura"),doll}:i==8?std::initializer_list<CardId>{wall,doll}:std::initializer_list<CardId>{},static_cast<Team>(i%5)));nine.players[0].mp=nine.players[8].mp=0;
    Match team;team.load_scene(nine,100,4);team.step(play(100,{0,1,2},108));roundtrip(team);team.step(defend(108,{0,1}));check(team.snapshot().owner==101&&team.snapshot().state.players[8].hp==40&&team.snapshot().state.players[0].mp==0&&team.snapshot().state.players[8].mp==0,"nine-seat composed FREE attack and FREE block retain actor/owner and exact zero-MP semantics");roundtrip(team);
}

void bounce_targets_roles_and_rng_provenance(){
    const auto bounce=image("bouncing-sword"),harpoon=image("sky-harpoon"),turbulence=image("turbulence"),mirror=image("super-mirror");
    for(auto id:{bounce,harpoon}){auto m=fixture(scene({id},{}));m.step(play(0,{0},7));check(m.snapshot().queue[0].power==find_card(id)->atk,"physical bounce weapon preserves ordinary base ATK");m.step(defend(7));roundtrip(m);}
    auto plus=fixture(scene({image("bronze-club"),harpoon},{}));plus.step(play(0,{0,1},7));check(plus.snapshot().queue[0].power==10&&plus.snapshot().queue[0].ability==Ability::none,"harpoon extra adds9 once without conferring its magic reaction on original weapon");plus.step(defend(7));
    for(const auto name:{"sky-boots","sky-gauntlet","sky-helm","sky-shield","sky-armor"}){auto m=fixture(scene({image("bronze-club")},{image(name)}));m.step(play(0,{0},7));m.step(defend(7,{0}));check(m.snapshot().state.players[1].hp==40,"nonmatching magic-bounce armor retains ordinary neutral weapon DEF");roundtrip(m);}
    const std::array<PlayerId,9> ids{900,7,0,55,3,400,19,88,2};const std::array<unsigned,16> target_indices{7,5,4,0,4,8,5,3,4,4,1,3,0,4,5,2};std::set<PlayerId> destinations;
    for(std::uint64_t seed=0;seed<128;++seed){SnapshotState state;for(unsigned i=0;i<9;++i)state.players.push_back(player(ids[i],i==0?std::initializer_list<CardId>{image("bronze-club")}:i==8?std::initializer_list<CardId>{bounce}:std::initializer_list<CardId>{},static_cast<Team>(i%3+1)));
        Match m;m.load_scene(state,900,seed);m.step(play(900,{0},2));const auto original=m.snapshot().queue[0];m.step(defend(2,{0}));const auto& s=m.snapshot();const auto& e=s.queue[0];destinations.insert(e.target);
        check(s.owner==900&&s.actor==e.target&&e.origin==900&&e.source==2&&e.initial_target==2&&e.power==1&&e.models==original.models&&e.tokens==original.tokens&&e.redirect_rng==seed&&e.redirects.size()==1&&e.redirects[0].target==e.target,"bounce changes current source and audited destination while immutable origin/owner/payload remain");
        if(seed<target_indices.size())check(e.target==ids[target_indices[seed]],"local SplitMix bounce samples uniformly by fixed input seat order, not ID sort or enemy-only filtering");
        check(s.rng==seed+0x9e3779b97f4a7c15ULL,"one bounded bounce draw advances only environmental RNG");roundtrip(m);
        const auto public_view=m.observe(900);check(public_view.find("\"schema\":\"official-execution-view-v39\"")!=std::string::npos&&public_view.find("\"bounces\":1")!=std::string::npos&&public_view.find("\"reflections\":0")!=std::string::npos&&public_view.find(bounce_target_rule)!=std::string::npos&&public_view.find("redirect_rng")==std::string::npos,"view declares experimental bounce rule and public counts while hiding RNG replay anchor");
        if(e.target==2)check(s.actor==2&&s.phase==MatchPhase::response&&s.state.players[8].hp==40,"self bounce is another explicit response rather than automatic same-step damage");
        if(seed==0){const auto exact=s;for(int error=0;error<7;++error){auto bad=exact;if(error==0)bad.rng^=1;if(error==1)bad.queue[0].redirect_rng^=1;if(error==2)bad.queue[0].redirects[0].target=900;if(error==3)bad.queue[0].source=900;if(error==4)bad.queue[0].redirects[0].defender=900;if(error==5)bad.queue[0].redirects[0].models[0]=mirror;if(error==6)bad.queue[0].target=900;bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"bounce RNG anchor/outcome/kind/endpoint forgery rejects atomically");}}
        m.step(defend(e.target));roundtrip(m);
    }
    check(destinations==std::set<PlayerId>(ids.begin(),ids.end()),"bounce can reach every live seat including teammates, previous source, self and third parties");
    auto dead=scene({image("bronze-club")},{bounce});dead.players.push_back(player(99));dead.players[2].hp=0;dead.players[2].dead=true;for(std::uint64_t seed=0;seed<32;++seed){Match m;m.load_scene(dead,0,seed);m.step(play(0,{0},7));m.step(defend(7,{0}));check(m.snapshot().queue[0].target!=99,"dead seats excluded from local all-alive bounce population");roundtrip(m);}
    auto color=fixture(scene({elemental(Category::weapons,Element::fire)},{image("rainbow-curtain"),bounce}));color.step(play(0,{0},7));reject(color,defend(7,{1}));color.step(defend(7,{0,1}));check(!color.snapshot().queue[0].element&&color.snapshot().queue[0].source==7,"filter allows colored weapon bounce and keeps payload neutral after rerouting");roundtrip(color);
    auto mixedstate=scene({image("spring"),image("moonlight-axe")},{image("sky-boots"),mirror});Match mixed;mixed.load_scene(mixedstate,0,2);mixed.step(play(0,{0},7));mixed.step(defend(7,{0}));check(mixed.snapshot().actor==0,"deterministic duel bounce reaches previous source");roundtrip(mixed);mixed.step(defend(0,{1}));roundtrip(mixed);mixed.step(defend(7,{1}));roundtrip(mixed);mixed.step(defend(0));check(mixed.snapshot().state.players[0].hp==50&&mixed.snapshot().state.players[0].mp==3&&mixed.snapshot().owner==7,"bounce and two physical reflections preserve utility kind/value, owner and original MP charge");roundtrip(mixed);
    auto main=fixture(scene({turbulence},{}));reject(main,play(0,{0},7));
}

void retained_bounce_exceeds_physical_limit_and_free(){
    const auto turbulence=image("turbulence"),doll=image("spiritual-doll");bool reached=false;
    for(std::uint64_t seed=0;seed<256&&!reached;++seed){auto state=scene({image("spring"),turbulence},{turbulence});state.players[0].mp=state.players[1].mp=99;Match m;m.load_scene(state,0,seed);m.step(play(0,{0},7));unsigned redirects=0;
        while(redirects<37){const auto actor=*m.snapshot().actor;const auto seat=actor==0?0:1;if(m.snapshot().state.players[seat].mp<5)break;m.step(defend(actor,{actor==0?ItemToken{1}:ItemToken{0}}));++redirects;roundtrip(m);}
        if(redirects==37){reached=true;check(m.snapshot().queue[0].redirects.size()==37&&m.snapshot().state.players[0].mp==2&&m.snapshot().state.players[1].mp==4&&m.snapshot().gifts==std::vector<std::uint32_t>{16,17},"retained Turbulence reaches37 paid responses beyond two-seat36 physical bound with exact MP and capped gifts");
            const auto saved=m.snapshot();check(std::count_if(saved.queue[0].redirects.begin(),saved.queue[0].redirects.end(),[](auto& row){return row.defender==0&&row.tokens==std::vector<ItemToken>{1};})==18&&std::count_if(saved.queue[0].redirects.begin(),saved.queue[0].redirects.end(),[](auto& row){return row.defender==7&&row.tokens==std::vector<ItemToken>{0};})==19,"same retained miracle token may legally recur across separate response decisions");
            for(int error=0;error<6;++error){auto bad=saved;if(error==0)++bad.state.players[0].mp;if(error==1)++bad.state.players[1].mp;if(error==2)bad.state.players[0].items[1].used=false;if(error==3)--bad.gifts[1];if(error==4)bad.queue[0].redirects.back().models[0]=image("wall");if(error==5){auto& row=*std::find_if(bad.queue[0].redirects.begin(),bad.queue[0].redirects.end(),[](auto& entry){return entry.defender==0;});row.tokens[0]=0;}bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==saved,"long retained bounce checkpoint rejects excess MP, unretained ownership, gift, token and model forgery atomically");}
            roundtrip(m);m.step(defend(*m.snapshot().actor));check(m.snapshot().state.players[0].items.size()==18&&m.snapshot().state.players[1].items.size()==18,"retained bounce chain resolves once and fills reserved slots without ownership duplication");roundtrip(m);}}
    check(reached,"deterministic finite seed search reaches37 retained bounce responses without artificial chain cap");
    auto darkstate=scene({image("darkness")},{turbulence});Match dark;dark.load_scene(darkstate,0,2);dark.step(play(0,{0},7));dark.step(defend(7,{0}));const auto dark_pending=dark.snapshot();check(dark_pending.actor==0&&dark_pending.queue[0].source==7&&dark_pending.queue[0].target==0&&dark_pending.queue[0].element==Element::darkness&&dark_pending.state.players[0].mp==5&&dark_pending.state.players[1].mp==5,"Dark miracle bounce needs no filter, preserves Dark and charges original caster and retained Turbulence independently");roundtrip(dark);
    const auto lethal=dark.step(defend(0));check(lethal.terminated&&!lethal.truncated&&lethal.winners==std::vector<PlayerId>{7}&&lethal.rewards==std::vector<double>{-1,1}&&dark.snapshot().state.players[0].dead&&dark.snapshot().state.players[0].hp==0&&dark.snapshot().state.players[0].items.size()==1&&dark.snapshot().state.players[1].items.size()==1&&dark.snapshot().state.players[0].items[0].used&&dark.snapshot().state.players[1].items[0].used&&dark.snapshot().gifts==std::vector<std::uint32_t>{0,0}&&dark.snapshot().next_token==dark_pending.next_token&&dark.snapshot().rng==dark_pending.rng,"bounced Dark kills immutable owner, fixes seat rewards and cancels all gifts without undoing paid MP, holdings or prior bounce RNG draw");roundtrip(dark);
    auto state=scene({image("spring"),image("angel-knife")},{turbulence,doll,image("angel-knife")});state.players[1].mp=0;auto free=fixture(state);free.step(play(0,{0},7));reject(free,defend(7,{0}));free.step(defend(7,{0,1}));check(free.snapshot().state.players[1].mp==0&&free.snapshot().state.players[1].items[0].used&&free.snapshot().queue[0].redirects.size()==1,"FREE suffix waives Turbulence5MP while retaining miracle and consuming carrier");roundtrip(free);const auto actor=*free.snapshot().actor;free.step(defend(actor,{actor==0?ItemToken{1}:ItemToken{2}}));check(free.snapshot().state.players[0].hp==40&&free.snapshot().state.players[1].hp==40,"either local bounce recipient may cancel retained utility with matching block");roundtrip(free);
    MatchConfig cap;cap.max_decisions=2;auto cutstate=scene({image("spring")},{turbulence});auto cut=fixture(cutstate,cap);cut.step(play(0,{0},7));const auto before=cut.snapshot().rng;cut.step(defend(7,{0}));check(cut.snapshot().truncated&&!cut.snapshot().terminated&&cut.snapshot().queue.empty()&&cut.snapshot().rng!=before&&cut.snapshot().state.players[1].mp==5,"bounce at cutoff commits RNG/cost but does not resolve payload or gifts");roundtrip(cut);
    auto late=fixture(scene({image("spring")},{turbulence}));late.step(play(0,{0},7));auto exhausted=late.snapshot();exhausted.state.update_count=max_wire_integer;exhausted.batch->initial_state.update_count=max_wire_integer-(exhausted.decisions-exhausted.batch->decisions);late.restore(exhausted);const auto exact=late.snapshot();bool rejected=false;try{late.step(defend(7,{0}));}catch(const MatchError&){rejected=true;}check(rejected&&late.snapshot()==exact,"late bounce failure rolls back random draw, MP, used ownership, ledger and gifts");
}

Command chance_play(PlayerId actor,std::initializer_list<ItemToken> tokens){return {actor,CommandKind::play,std::vector<ItemToken>(tokens),{}};}
void chance_scope_boundaries_and_single_cost(){
    unsigned weapons=0,miracles=0;for(const auto& card:catalog())if(card.hit_rate&&card.ability==Ability::none&&(card.category==Category::weapons||card.category==Category::miracles)&&card.gift_rate){check(match_supported(card.id),"all21 plain chance cards belong to strict supported pool");card.category==Category::weapons?++weapons:++miracles;
        auto state=scene({card.id,image("crossbow")},{});state.players[0].mp=99;Match m;m.load_scene(state,0,2);reject(m,play(0,{0},7));if(card.category==Category::weapons)reject(m,chance_play(0,{0,1}));m.step(chance_play(0,{0}));
        check(m.snapshot().phase==MatchPhase::response&&m.snapshot().actor==7&&m.snapshot().batch&&m.snapshot().chance_rolls==1&&m.snapshot().chance_hits==1&&m.snapshot().queue[0].hit_rate==card.hit_rate&&m.snapshot().queue[0].power==card.atk&&m.snapshot().state.players[0].mp==99-card.cost,"targetless hit pays original cost exactly once and publishes hit-qualified response with original rate");roundtrip(m);m.step(defend(7));roundtrip(m);}
    check(weapons==15&&miracles==6,"pure chance extension adds precisely15 physical and6 miracle models");
    struct Boundary{const char* name;std::uint64_t seed;bool hit;};for(const auto b:{Boundary{"lightning-kids",100,true},{"lightning-kids",136,false},{"shadow-hand",156,true},{"shadow-hand",72,false},{"spark-bag",27,true},{"spark-bag",123,false}}){Match m;m.load_scene(scene({image(b.name)},{}),0,b.seed);m.step(chance_play(0,{0}));check(m.snapshot().chance_rolls==1&&m.snapshot().chance_hits==static_cast<unsigned>(b.hit)&&(m.snapshot().phase==MatchPhase::response)==b.hit,"bounded100 roll strictly below25/50/75 hits; exact threshold misses");roundtrip(m);}
    Match miss;miss.load_scene(scene({image("lightning-kids")},{}),0,0);miss.step(chance_play(0,{0}));check(miss.snapshot().owner==7&&!miss.snapshot().batch&&miss.snapshot().queue.empty()&&miss.snapshot().state.players[1].hp==40&&miss.snapshot().decisions==1&&miss.snapshot().state.players[0].items.size()==1,"all-miss attack needs no response, clears batch and pays physical replacement once");roundtrip(miss);
    auto free_state=scene({image("smoke"),image("spiritual-doll")},{image("moonlight-axe")});free_state.players[0].mp=0;Match free;free.load_scene(free_state,0,2);free.step(chance_play(0,{0,1}));check(free.snapshot().state.players[0].mp==0&&free.snapshot().state.players[0].items[0].used&&free.snapshot().gifts[0]==2,"chance miracle permits immediate FREE while charging/acquiring only once at launch");const auto hit_rng=free.snapshot().rng;free.step(defend(7,{0}));check(free.snapshot().chance_rolls==1&&free.snapshot().chance_hits==1&&free.snapshot().rng==hit_rng&&free.snapshot().queue[0].hit_rate==75&&free.snapshot().queue[0].target==0,"hit-qualified magic reflection preserves display rate and never rerolls original chance");roundtrip(free);free.step(defend(0));check(free.snapshot().state.players[0].hp==35&&free.snapshot().state.players[0].mp==0,"redirected FREE chance payload resolves once on final recipient");roundtrip(free);
    auto defense=fixture(scene({image("bronze-club")},{image("jinn-s-rocking-horse")}));defense.step(play(0,{0},7));defense.step(defend(7,{0}));check(defense.snapshot().state.players[1].hp==40&&defense.snapshot().chance_rolls==0,"chance weapon108 retains ordinary DEF6 without a defensive chance draw");roundtrip(defense);
}

void chance_multiplayer_journal_and_privacy(){
    SnapshotState state;for(unsigned i=0;i<9;++i)state.players.push_back(player(100+i,i==0?std::initializer_list<CardId>{image("spark-bag")}:std::initializer_list<CardId>{},i==0||i==1||i==5?Team::one:i==3||i==7?Team::two:Team::solo));state.players[2].dead=true;state.players[2].hp=0;state.players[3].hp=1;
    Match m;m.load_scene(state,100,0);m.step(chance_play(100,{0}));check(m.snapshot().batch->targets==std::vector<PlayerId>{103,104,106,107,108}&&m.snapshot().actor==103,"launch roster excludes owner, allies and already-dead seats and preserves input order");roundtrip(m);const auto next=m.snapshot().next_token;m.step(defend(103));
    check(m.snapshot().state.players[3].dead&&m.snapshot().actor==104&&m.snapshot().chance_rolls==2&&m.snapshot().chance_hits==2&&m.snapshot().gifts[0]==1&&m.snapshot().next_token==next,"after first target death, journal validates changed HP/dead state and next hit while all gifts remain deferred");roundtrip(m);
    const auto exact=m.snapshot();for(int error=0;error<17;++error){auto bad=exact;if(error==0)bad.batch.reset();if(error==1)++bad.batch->initial_state.players[3].hp;if(error==2)bad.batch->owner=103;if(error==3)bad.batch->attack.target=103;if(error==4)bad.batch->models[0]=image("shadow-hand");if(error==5)std::swap(bad.batch->targets[0],bad.batch->targets[1]);if(error==6)--bad.batch->cursor;if(error==7)bad.batch->responses.clear();if(error==8)bad.batch->responses[0].actor=104;if(error==9)bad.batch->rng^=1;if(error==10)--bad.chance_rolls;if(error==11)++bad.chance_hits;if(error==12)++bad.next_token;if(error==13)++bad.queue[0].hit_rate;if(error==14)bad.batch->initial_state.update_count=max_wire_integer;if(error==15)++bad.batch->decisions;if(error==16)bad.gifts[0]=0;bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"chance launch anchor, accepted journal, cursor, HP, RNG and derived current state are verified atomically");}
    m.step(defend(104));check(m.snapshot().actor==107&&m.snapshot().chance_rolls==4&&m.snapshot().chance_hits==3&&m.snapshot().state.players[6].hp==40,"missed middle target consumes one roll but no response decision");roundtrip(m);m.step(defend(107));check(m.snapshot().actor==108,"last roster target receives its own hit-qualified response");roundtrip(m);m.step(defend(108));
    check(!m.snapshot().batch&&m.snapshot().queue.empty()&&m.snapshot().owner==101&&m.snapshot().chance_rolls==5&&m.snapshot().chance_hits==4&&m.snapshot().decisions==5&&m.snapshot().state.players[0].items.size()==1,"whole batch ends once after four responses/five attempts, then gifts and owner advance once");roundtrip(m);
    Match a,b;const auto public_state=scene({image("smoke")},{image("moonlight-helm")});a.load_scene(public_state,0,2);b.load_scene(public_state,0,5);a.step(chance_play(0,{0}));b.step(chance_play(0,{0}));check(a.observe(7)==b.observe(7)&&a.observe(7).find(chance_rule)!=std::string::npos&&a.observe(7).find("initial_state")==std::string::npos&&a.observe(7).find("redirect_rng")==std::string::npos,"identical public hit states conceal different future RNG and privileged chance launch journal");
    const auto a_before=a.snapshot(),b_before=b.snapshot();std::uint64_t pa=17,pb=17;const auto ca=a.sample_action(pa),cb=b.sample_action(pb);check(ca==cb&&pa==pb&&a.legal(ca)&&b.legal(cb)&&a.snapshot()==a_before&&b.snapshot()==b_before,"chance policy observes own/public state without environmental future-RNG interference");
}

void chance_dead_skip_origin_cancel_cutoff_and_rollback(){
    auto skipped=scene({image("shadow-hand")},{image("rainbow-curtain"),image("bouncing-sword")});skipped.players.push_back(player(9));skipped.players[2].hp=1;Match skip;skip.load_scene(skipped,0,2);skip.step(chance_play(0,{0}));skip.step(defend(7,{0,1}));check(skip.snapshot().actor==9&&!skip.snapshot().queue[0].element&&skip.snapshot().chance_rolls==1,"hit plus bounce may reroute neutralized chance payload to a later original roster target without reroll");roundtrip(skip);skip.step(defend(9));check(skip.snapshot().state.players[2].dead&&!skip.snapshot().batch&&skip.snapshot().chance_rolls==1&&skip.snapshot().chance_hits==1&&skip.snapshot().owner==7,"later roster target killed by redirect is skipped without another hit draw");roundtrip(skip);
    auto canceled=scene({image("shadow-hand")},{image("super-mirror")});canceled.players[0].hp=1;canceled.players.push_back(player(9));Match cancel;cancel.load_scene(canceled,0,0);cancel.step(chance_play(0,{0}));cancel.step(defend(7,{0}));roundtrip(cancel);cancel.step(defend(0));check(cancel.snapshot().state.players[0].dead&&!cancel.snapshot().terminated&&!cancel.snapshot().batch&&cancel.snapshot().chance_rolls==1&&cancel.snapshot().state.players[2].hp==40&&cancel.snapshot().owner==7,"origin death cancels unattempted targets even though multiple opposing survivors keep match running");roundtrip(cancel);
    MatchConfig limit;limit.max_decisions=2;auto lethal=scene({image("spark-bag")},{});lethal.players[1].hp=1;Match natural(limit);natural.load_scene(lethal,0,0);natural.step(chance_play(0,{0}));natural.step(defend(7));check(natural.snapshot().terminated&&!natural.snapshot().truncated&&!natural.snapshot().batch&&natural.snapshot().gifts==std::vector<std::uint32_t>{0,0},"natural terminal after chance target wins over cutoff and cancels pending gifts");roundtrip(natural);
    auto multi=scene({image("spark-bag")},{});multi.players.push_back(player(9));Match cut(limit);cut.load_scene(multi,0,0);cut.step(chance_play(0,{0}));const auto pending=cut.snapshot();cut.step(defend(7));check(cut.snapshot().truncated&&!cut.snapshot().batch&&cut.snapshot().chance_rolls==1&&cut.snapshot().rng==pending.rng&&cut.snapshot().state.players[1].hp==39&&cut.snapshot().state.players[2].hp==40,"cutoff after current target applies HP but prevents next target roll and all gifts");roundtrip(cut);
    MatchConfig launch_limit;launch_limit.max_decisions=1;auto launch_state=scene({image("lightning-kids")},{});launch_state.players.push_back(player(9));Match launch_cut(launch_limit);launch_cut.load_scene(launch_state,0,1);const auto launch_before=launch_cut.snapshot();launch_cut.step(chance_play(0,{0}));
    check(launch_cut.snapshot().truncated&&!launch_cut.snapshot().terminated&&launch_cut.snapshot().decisions==1&&launch_cut.snapshot().chance_rolls==2&&launch_cut.snapshot().chance_hits==1&&launch_cut.snapshot().rng==std::uint64_t{1}+std::uint64_t{2}*0x9e3779b97f4a7c15ULL&&!launch_cut.snapshot().batch&&launch_cut.snapshot().queue.empty()&&launch_cut.snapshot().gifts==std::vector<std::uint32_t>{0,0,0}&&launch_cut.snapshot().next_token==launch_before.next_token&&std::all_of(launch_cut.snapshot().state.players.begin(),launch_cut.snapshot().state.players.end(),[](auto& p){return p.hp==40&&!p.dead;}),"launch command with limit1 drains first miss then rolls next hit before cutoff, clears journal/queue and suppresses HP effects and all gifts");roundtrip(launch_cut);
    Match late;late.load_scene(scene({image("spark-bag")},{}),0,0);late.step(chance_play(0,{0}));auto bad=late.snapshot();bad.state.update_count=max_wire_integer;bad.batch->initial_state.update_count=max_wire_integer-1;late.restore(bad);const auto exact=late.snapshot();bool rejected=false;try{late.step(defend(7));}catch(const MatchError&){rejected=true;}check(rejected&&late.snapshot()==exact,"late response counter failure rolls back chance HP, accepted-response journal, batch cursor, gifts and RNG");
}

void counter_damage_fifo_and_reactions(){
    const auto saturn=image("saturn-ring"),mars=image("mars-ring"),curtain=image("rainbow-curtain"),mirror=image("super-mirror");
    auto base=fixture(scene({image("bronze-club")},{saturn}));base.step(play(0,{0},7));base.step(defend(7,{0}));const auto frame=base.snapshot().queue[0];
    check(base.snapshot().batch&&!base.snapshot().batch->chance&&frame.is_counter&&frame.origin==7&&frame.source==7&&frame.target==0&&frame.category==Category::armor&&frame.element==Element::stone&&frame.power==2&&frame.hit_rate==0&&base.snapshot().state.players[1].hp==39&&base.snapshot().counter_created==1&&base.snapshot().gifts==std::vector<std::uint32_t>{1,1},"positive actual HP loss with DEF0 Saturn reserves one2x counter before gifts while original turn owner stays fixed");roundtrip(base);base.step(defend(0));check(base.snapshot().state.players[0].hp==38&&base.snapshot().state.players[1].hp==39&&base.snapshot().owner==7&&!base.snapshot().batch&&base.snapshot().counters.empty(),"counter resolves once then original action gifts/turn complete");roundtrip(base);
    auto netstate=scene({image("angel-axe")},{saturn,image("aqua-shoes")});netstate.players[1].hp=15;auto net=fixture(netstate);net.step(play(0,{0},7));net.step(defend(7,{0,1}));check(net.snapshot().state.players[1].hp==1&&net.snapshot().queue[0].power==28,"counter scales actual14 HP lost after DEF, not incoming15 or remaining1HP");roundtrip(net);
    auto zero=fixture(scene({image("bronze-club")},{saturn,image("aqua-shoes")}));zero.step(play(0,{0},7));zero.step(defend(7,{0,1}));check(zero.snapshot().counter_created==0&&zero.snapshot().state.players[1].hp==40&&zero.snapshot().queue.empty(),"full defense consumes selected counter but never creates a zero-loss counter");roundtrip(zero);
    auto fatalstate=scene({image("bronze-club")},{saturn});fatalstate.players[1].hp=1;auto fatal=fixture(fatalstate);fatal.step(play(0,{0},7));fatal.step(defend(7,{0}));check(fatal.snapshot().terminated&&fatal.snapshot().counter_created==0&&fatal.snapshot().state.players[1].dead,"lethal loss does not reserve a counter from a dead receiver");roundtrip(fatal);
    for(bool saturn_first:{false,true}){Match m;m.load_scene(scene({image("bronze-club")},{mars,saturn}),0,27);m.step(play(0,{0},7));m.step(saturn_first?defend(7,{1,0}):defend(7,{0,1}));check(m.snapshot().counter_created==2&&m.snapshot().counters.size()==1&&m.snapshot().queue[0].models==std::vector<CardId>{saturn_first?saturn:mars}&&m.snapshot().counters[0].models==std::vector<CardId>{saturn_first?mars:saturn}&&m.snapshot().counter_rolls==(saturn_first?0u:1u),"counter FIFO follows selected physical order and Mars rolls only at its actual start");roundtrip(m);m.step(defend(0));roundtrip(m);m.step(defend(0));check(m.snapshot().state.players[0].hp==37&&m.snapshot().state.players[1].hp==39&&m.snapshot().counter_rolls==1&&m.snapshot().counter_hits==1,"mixed counter order preserves each payload and one independent Mars hit roll");roundtrip(m);}
    auto recursive=scene({image("bronze-club"),curtain,saturn},{mars,saturn});Match fifo;fifo.load_scene(recursive,0,27);fifo.step(play(0,{0},7));fifo.step(defend(7,{0,1}));fifo.step(defend(0,{1,2}));check(fifo.snapshot().queue[0].origin==7&&fifo.snapshot().queue[0].power==2&&fifo.snapshot().counters.size()==1&&fifo.snapshot().counters[0].origin==0&&fifo.snapshot().counters[0].target==7&&fifo.snapshot().counter_created==3,"child counter is appended behind already-reserved sibling rather than depth-first execution");roundtrip(fifo);fifo.step(defend(0));roundtrip(fifo);fifo.step(defend(7));check(fifo.snapshot().state.players[0].hp==37&&fifo.snapshot().state.players[1].hp==37&&fifo.snapshot().owner==7,"counter-on-counter FIFO completes before turn advance");roundtrip(fifo);
    auto typed=fixture(scene({image("bronze-club"),curtain,image("reflection-sword"),image("moonlight-axe"),image("wall"),image("turbulence"),image("bouncing-sword"),mirror},{saturn}));typed.step(play(0,{0},7));typed.step(defend(7,{0}));reject(typed,defend(0,{1,2}));for(ItemToken token:{3u,4u,5u,6u})reject(typed,defend(0,{token}));typed.step(defend(0,{7}));check(typed.snapshot().queue[0].is_counter&&typed.snapshot().queue[0].origin==7&&typed.snapshot().queue[0].source==0&&typed.snapshot().queue[0].target==7&&typed.snapshot().queue[0].category==Category::armor&&typed.snapshot().queue[0].power==2,"only universal reflection redirects counter while immutable counter origin/category/power survive");roundtrip(typed);typed.step(defend(7));check(typed.snapshot().state.players[1].hp==37,"reflected counter affects new target once");roundtrip(typed);
    auto self=fixture(scene({image("bronze-club"),saturn},{}));self.step(play(0,{0},0));self.step(defend(0,{1}));check(self.snapshot().queue[0].is_counter&&self.snapshot().actor==0&&self.snapshot().queue[0].origin==0&&self.snapshot().queue[0].target==0,"self-hit survivor can create explicit self counter");roundtrip(self);self.step(defend(0));check(self.snapshot().state.players[0].hp==37&&self.snapshot().owner==7,"self original and counter damage apply sequentially before next turn");roundtrip(self);
}

void counter_chance_death_journal_and_atomicity(){
    const auto saturn=image("saturn-ring"),mars=image("mars-ring");for(const auto item:{std::pair<std::uint64_t,bool>{27,true},{123,false}}){Match m;m.load_scene(scene({image("bronze-club")},{mars}),0,item.first);m.step(play(0,{0},7));m.step(defend(7,{0}));check(m.snapshot().counter_created==1&&m.snapshot().counter_rolls==1&&m.snapshot().counter_hits==static_cast<unsigned>(item.second)&&m.snapshot().chance_rolls==0,"Mars uses its own strict75 threshold and counter metrics, separate from initial chance attempts");if(item.second){check(m.snapshot().queue[0].is_counter&&m.snapshot().queue[0].hit_rate==75&&m.snapshot().queue[0].power==1,"Mars hit exposes original rate with actual-loss power");roundtrip(m);m.step(defend(0));}check(m.snapshot().state.players[0].hp==(item.second?39:40)&&m.snapshot().state.players[1].hp==39&&!m.snapshot().batch,"Mars miss needs no response and hit resolves one body");roundtrip(m);}
    auto deadsource=scene({image("bronze-club"),image("super-mirror")},{saturn,mars});deadsource.players[1].hp=3;deadsource.players.push_back(player(9));auto skipped=fixture(deadsource);skipped.step(play(0,{0},7));skipped.step(defend(7,{0,1}));skipped.step(defend(0,{1}));skipped.step(defend(7));check(skipped.snapshot().state.players[1].dead&&skipped.snapshot().counter_created==2&&skipped.snapshot().counter_rolls==0&&skipped.snapshot().owner==9&&!skipped.snapshot().batch,"queued Mars with dead source is skipped before any chance draw after earlier reflected counter kills its creator");roundtrip(skipped);
    auto deadtarget=scene({image("bronze-club")},{saturn,mars});deadtarget.players[0].hp=1;deadtarget.players.push_back(player(9));auto lost=fixture(deadtarget);lost.step(play(0,{0},7));lost.step(defend(7,{0,1}));lost.step(defend(0));check(lost.snapshot().state.players[0].dead&&lost.snapshot().counter_rolls==0&&!lost.snapshot().terminated&&lost.snapshot().owner==7,"remaining queued counter with dead target is skipped with no draw while surviving opponents keep match running");roundtrip(lost);
    Match pending;pending.load_scene(scene({image("bronze-club")},{mars,saturn}),0,27);pending.step(play(0,{0},7));pending.step(defend(7,{0,1}));const auto exact=pending.snapshot();for(int error=0;error<15;++error){auto bad=exact;if(error==0)bad.batch.reset();if(error==1)bad.queue[0].is_counter=false;if(error==2)++bad.queue[0].power;if(error==3)bad.queue[0].origin=0;if(error==4)bad.queue[0].category=Category::weapons;if(error==5)bad.counters.clear();if(error==6)bad.counters[0].target=7;if(error==7)bad.counters[0].models[0]=mars;if(error==8)++bad.counter_created;if(error==9)++bad.counter_started;if(error==10)++bad.counter_rolls;if(error==11)++bad.counter_hits;if(error==12)bad.batch->responses.clear();if(error==13)++bad.batch->counters_created;if(error==14)bad.gifts[1]=0;bool rejected=false;try{pending.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==exact,"global action replay rejects counter type/power/origin/category/FIFO/history/metrics and payment forgery atomically");}roundtrip(pending);
    auto overflow=exact;overflow.state.update_count=max_wire_integer;overflow.batch->initial_state.update_count=max_wire_integer-(overflow.decisions-overflow.batch->decisions);pending.restore(overflow);const auto late=pending.snapshot();bool rejected=false;try{pending.step(defend(0));}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==late,"late failure rolls back resolved counter HP, next FIFO pop/RNG, journal and reservations");
    auto initial=scene({image("plant-cup")},{saturn});initial.players.push_back(player(9));Match mixed;mixed.load_scene(initial,0,0);mixed.step(chance_play(0,{0}));mixed.step(defend(7,{0}));check(mixed.snapshot().queue[0].is_counter&&mixed.snapshot().actor==0&&mixed.snapshot().chance_rolls==1,"reserved survivor counter starts before next initial chance target gets its roll");roundtrip(mixed);mixed.step(defend(0));check(!mixed.snapshot().queue[0].is_counter&&mixed.snapshot().actor==9&&mixed.snapshot().chance_rolls==2&&mixed.snapshot().batch->responses.size()==2,"next initial target starts only after counter FIFO drains, with all responses in shared journal");roundtrip(mixed);mixed.step(defend(9));roundtrip(mixed);
}

void counter_generation_exceeds_old_duel_limit(){
    SnapshotState state;for(PlayerId id=0;id<9;++id){auto p=player(id);p.hp=99;if(id==0)p.items.push_back({0,image("spark-bag"),{},false});else{p.items.push_back({0,image("rainbow-curtain"),{},false});for(ItemToken token=1;token<18;++token)p.items.push_back({token,image("saturn-ring"),{},false});}state.players.push_back(std::move(p));}
    Match m;m.load_scene(state,0,0);m.step(chance_play(0,{0}));roundtrip(m);unsigned responses=0;
    while(m.snapshot().batch&&responses<100){const auto actor=*m.snapshot().actor;Command command=defend(actor);if(actor!=0){check(!m.snapshot().queue[0].is_counter,"all counter payloads in large fixture target immutable original source");for(ItemToken token=0;token<18;++token)command.items.push_back(token);}m.step(command);++responses;roundtrip(m);}
    check(responses<100&&!m.snapshot().batch&&m.snapshot().queue.empty()&&m.snapshot().counters.empty()&&m.snapshot().counter_created==51&&m.snapshot().counter_started==50&&m.snapshot().chance_rolls==4&&m.snapshot().chance_hits==3&&m.snapshot().state.players[0].dead&&m.snapshot().state.players[0].hp==0&&m.snapshot().owner==1&&!m.snapshot().terminated,"nine-seat action generates51 counters beyond old32 cap, kills original owner on50th start, skips dead-target remainder and cancels unattempted initial roster");
    check(m.snapshot().state.players[1].items.size()==18&&m.snapshot().state.players[2].items.size()==18&&m.snapshot().state.players[4].items.size()==18&&m.snapshot().gifts==std::vector<std::uint32_t>(9,0),"all surviving counter creators refill exactly once after entire action while dead owner receives no gift");roundtrip(m);
}

void resource_counter_mp_cp_and_alias(){
    const auto neptune=image("neptune-ring"),venus=image("venus-ring"),mirror=image("super-mirror"),curtain=image("rainbow-curtain"),saturn=image("saturn-ring");
    for(int mp:{0,98}){auto state=scene({image("reflection-sword")},{neptune,mirror,image("aqua-shoes"),curtain,saturn,venus});state.players[1].mp=mp;auto m=fixture(state);m.step(play(0,{0},7));m.step(defend(7,{0}));
        const auto& e=m.snapshot().queue[0];check(e.is_counter&&e.power==0&&e.amount==20&&e.ability==Ability::counter_boost2x_mp&&e.category==Category::armor&&!e.element&&e.origin==7&&e.source==7&&e.target==7&&m.snapshot().actor==7&&m.snapshot().state.players[1].mp==mp&&m.snapshot().state.players[1].hp==30&&m.snapshot().state.players[1].guardian==Guardian::none,"MP counter reserves2x actual loss as nonattack self response without prematurely adding MP or summoning guardian");roundtrip(m);
        for(ItemToken token:{2u,3u,4u,5u})reject(m,defend(7,{token}));m.step(defend(7,{1}));check(m.snapshot().state.players[1].mp==mp&&m.snapshot().queue[0].source==7&&m.snapshot().queue[0].target==7&&m.snapshot().queue[0].amount==20,"self universal reflection continues MP response and preserves amount without adding MP");roundtrip(m);m.step(defend(7));check(m.snapshot().state.players[1].mp==std::min(99,mp+20)&&m.snapshot().state.players[1].hp==30&&m.snapshot().counter_created==1,"MP counter applies once on final pass and clips99");roundtrip(m);}
    auto capstate=scene({image("final-tusk"),image("angel-bow"),image("angel-bow"),image("angel-bow"),image("angel-bow"),image("angel-bow"),image("angel-bow")},{neptune});capstate.players[1].hp=99;capstate.players[1].mp=0;auto capped=fixture(capstate);capped.step(play(0,{0,1,2,3,4,5,6},7));capped.step(defend(7,{0}));check(capped.snapshot().state.players[1].hp==1&&capped.snapshot().queue[0].amount==196,"surviving98 HP loss produces maximum reachable196 resource amount within conservative198 bound");roundtrip(capped);capped.step(defend(7));check(capped.snapshot().state.players[1].mp==99&&capped.snapshot().state.players[1].hp==1,"large MP nominal amount caps resource without touching HP");roundtrip(capped);
    auto state=scene({image("reflection-sword")},{venus});state.players[0].cp=state.players[0].mp=2;state.players[1].cp=95;auto take=fixture(state);take.step(play(0,{0},7));take.step(defend(7,{0}));check(take.snapshot().queue[0].power==0&&take.snapshot().queue[0].amount==10&&take.snapshot().queue[0].ability==Ability::counter_take_cp&&take.snapshot().actor==0&&take.snapshot().state.players[0].cp==2,"CP counter waits as nonattack response directed at incoming source");roundtrip(take);take.step(defend(0));check(take.snapshot().state.players[0].cp==0&&take.snapshot().state.players[0].mp==0&&take.snapshot().state.players[0].hp==34&&take.snapshot().state.players[1].cp==99&&take.snapshot().state.players[1].hp==30&&take.snapshot().counter_created==1,"CP debit spills through MP and HP while full nominal credit clips source99, independent of actual target CP");roundtrip(take);
    auto reflected=scene({image("reflection-sword"),mirror},{venus});reflected.players[0].cp=2;auto reverse=fixture(reflected);reverse.step(play(0,{0},7));reverse.step(defend(7,{0}));reverse.step(defend(0,{1}));check(reverse.snapshot().queue[0].origin==7&&reverse.snapshot().queue[0].source==0&&reverse.snapshot().queue[0].target==7&&reverse.snapshot().queue[0].amount==10,"universal reflection reverses current CP collection endpoints while counter origin/amount stay immutable");roundtrip(reverse);reverse.step(defend(7));check(reverse.snapshot().state.players[0].cp==12&&reverse.snapshot().state.players[1].cp==10&&reverse.snapshot().state.players[1].hp==30,"reflected CP counter debits current target then credits current source");roundtrip(reverse);
    auto selfstate=scene({image("reflection-sword"),venus},{});selfstate.players[0].cp=3;selfstate.players[0].mp=4;auto self=fixture(selfstate);self.step(play(0,{0},0));self.step(defend(0,{1}));check(self.snapshot().state.players[0].hp==30,"self weapon HP loss precedes CP counter payment");self.step(defend(0));check(self.snapshot().state.players[0].cp==10&&self.snapshot().state.players[0].mp==0&&self.snapshot().state.players[0].hp==27&&self.snapshot().counter_created==1,"self CP counter debits3CP/4MP/3HP before crediting nominal10CP, without debt-triggered new counter");roundtrip(self);
    auto deadstate=scene({image("reflection-sword"),venus},{});deadstate.players[0].hp=15;deadstate.players[0].cp=deadstate.players[0].mp=0;auto dead=fixture(deadstate);dead.step(play(0,{0},0));dead.step(defend(0,{1}));dead.step(defend(0));check(dead.snapshot().terminated&&dead.snapshot().winners==std::vector<PlayerId>{7}&&dead.snapshot().state.players[0].hp==0&&dead.snapshot().state.players[0].dead&&dead.snapshot().state.players[0].cp==10&&dead.snapshot().gifts==std::vector<std::uint32_t>{0,0},"self CP debt kills at zero HP; later nominal CP credit does not revive or grant terminal gifts");roundtrip(dead);
}

void resource_counter_dead_skip_and_journal(){
    const auto neptune=image("neptune-ring"),venus=image("venus-ring"),saturn=image("saturn-ring");auto state=scene({image("bronze-club"),image("super-mirror")},{saturn,neptune,venus});state.players[1].hp=3;state.players[1].mp=state.players[1].cp=0;state.players.push_back(player(9));auto skip=fixture(state);skip.step(play(0,{0},7));skip.step(defend(7,{0,1,2}));skip.step(defend(0,{1}));skip.step(defend(7));check(skip.snapshot().state.players[1].dead&&skip.snapshot().state.players[1].mp==0&&skip.snapshot().state.players[1].cp==0&&skip.snapshot().counter_created==3&&skip.snapshot().counter_started==1&&skip.snapshot().owner==9,"dead creator skips queued self MP and CP frames before any resource transfer");roundtrip(skip);
    auto target=scene({image("bronze-club")},{saturn,venus});target.players[0].hp=1;target.players[1].cp=5;target.players.push_back(player(9));auto missing=fixture(target);missing.step(play(0,{0},7));missing.step(defend(7,{0,1}));missing.step(defend(0));check(missing.snapshot().state.players[0].dead&&missing.snapshot().state.players[1].cp==5&&missing.snapshot().counter_started==1,"dead target skips queued CP frame without crediting surviving creator");roundtrip(missing);
    auto pending=fixture(scene({image("reflection-sword")},{neptune}));pending.step(play(0,{0},7));pending.step(defend(7,{0}));const auto exact=pending.snapshot();for(int error=0;error<10;++error){auto bad=exact;if(error==0)++bad.queue[0].amount;if(error==1)bad.queue[0].power=20;if(error==2)bad.queue[0].ability=Ability::counter_take_cp;if(error==3)bad.queue[0].target=0;if(error==4)bad.queue[0].category=Category::miracles;if(error==5)++bad.state.players[1].mp;if(error==6)bad.state.players[1].guardian=Guardian::neptune;if(error==7)bad.batch->responses.clear();if(error==8)++bad.counter_started;if(error==9)bad.queue[0].is_counter=false;bool rejected=false;try{pending.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==exact,"journal verifies resource amount/type/endpoints/MP/no-guardian and exact counter history atomically");}
    check(pending.observe(7).find(resource_counter_rule)!=std::string::npos,"view declares explicit local resource-counter contract");auto overflow=exact;overflow.state.update_count=max_wire_integer;overflow.batch->initial_state.update_count=max_wire_integer-(overflow.decisions-overflow.batch->decisions);pending.restore(overflow);const auto saved=pending.snapshot();bool rejected=false;try{pending.step(defend(7));}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==saved,"late resource-counter resolution failure rolls back MP, FIFO, journal, gifts and RNG");
}

void mp_refill_extends_retained_redirect_budget(){
    bool extended=false;for(std::uint64_t seed=0;seed<128&&!extended;++seed){SnapshotState state;state.players={player(0,{image("ice-age"),image("turbulence"),image("rainbow-curtain"),image("neptune-ring")}),player(7,{image("turbulence"),image("rainbow-curtain"),image("neptune-ring")}),player(9,{image("turbulence"),image("rainbow-curtain"),image("neptune-ring")})};for(auto& p:state.players)p.hp=p.mp=99;Match m;m.load_scene(state,0,seed);m.step(chance_play(0,{0}));std::array<unsigned,3> paid{};unsigned responses=0;bool replenished=false;
        while(m.snapshot().batch&&responses<300){const auto actor=*m.snapshot().actor;const auto seat=actor==0?0:actor==7?1:2;const auto& p=m.snapshot().state.players[seat];Command command=defend(actor);
            if(m.snapshot().queue[0].is_counter){check(m.snapshot().queue[0].ability==Ability::counter_boost2x_mp,"refill fixture only creates MP resource counters");replenished=true;}
            else if(p.mp>=5){const auto it=std::find_if(p.items.begin(),p.items.end(),[](auto& i){return i.model==image("turbulence");});command.items={it->token};++paid[seat];}
            else{const auto filter=std::find_if(p.items.begin(),p.items.end(),[](auto& i){return i.model==image("rainbow-curtain");});const auto counter=std::find_if(p.items.begin(),p.items.end(),[](auto& i){return i.model==image("neptune-ring");});if(filter!=p.items.end()&&counter!=p.items.end())command.items={filter->token,counter->token};}
            m.step(command);++responses;roundtrip(m);}
        check(responses<300,"MP-refund retained-cast stress remains finite under bounded resources");if(replenished&&*std::max_element(paid.begin(),paid.end())>19){extended=true;check(!m.snapshot().batch&&m.snapshot().queue.empty()&&m.snapshot().counters.empty()&&!m.snapshot().truncated,"MP refund legitimately enables more than19 paid Turbulence casts by one participant in a single completed action");roundtrip(m);}}
    check(extended,"finite seeded scenario reaches MP-counter refill followed by old-budget-exceeding retained recasts");
}

void absorption_stages_reflection_and_retention(){
    const auto ghost=image("ghost-sword"),real=image("real-ghost-sword"),vine=image("vine-shoot"),spell=image("absorption"),saturn=image("saturn-ring");
    for(auto id:{ghost,real}){auto state=scene({id},{});state.players[0].hp=40;state.players[1].hp=2;auto m=fixture(state);m.step(play(0,{0},7));check(m.snapshot().state.players[0].hp==40,"absorption side effect does not run at launch");m.step(defend(7));check(m.snapshot().state.players[0].hp==40+find_card(id)->atk&&m.snapshot().state.players[1].hp==0&&m.snapshot().terminated,"overkill absorption credits whole positive remainder before clipping target HP loss");roundtrip(m);}
    auto selfstate=scene({ghost},{});selfstate.players[0].hp=95;auto self=fixture(selfstate);self.step(play(0,{0},0));self.step(defend(0));check(self.snapshot().state.players[0].hp==92,"self absorption runs95 to99 healing then99 to92 damage, not net cancellation before cap");roundtrip(self);
    auto net=fixture(scene({ghost,saturn},{}));net.step(play(0,{0},0));net.step(defend(0,{1}));check(net.snapshot().state.players[0].hp==40&&net.snapshot().queue[0].is_counter&&net.snapshot().queue[0].power==14&&net.snapshot().queue[0].ability==Ability::counter2x_atk,"self absorption net HP zero still creates2x7 counter from target-damage stage only");roundtrip(net);net.step(defend(0));check(net.snapshot().state.players[0].hp==26,"derived counter never inherits parent's absorption ability");roundtrip(net);
    auto blocked=fixture(scene({ghost},{image("aqua-shoes"),image("god-shield")}));blocked.step(play(0,{0},7));blocked.step(defend(7,{1}));check(blocked.snapshot().state.players[0].hp==40&&blocked.snapshot().state.players[1].hp==40,"zero positive remainder gives no absorption healing");roundtrip(blocked);
    auto reflectstate=scene({ghost},{image("reflection-sword")});reflectstate.players[0].hp=30;reflectstate.players[1].hp=79;auto reflected=fixture(reflectstate);reflected.step(play(0,{0},7));reflected.step(defend(7,{0}));check(reflected.snapshot().state.players[0].hp==30&&reflected.snapshot().state.players[1].hp==79&&reflected.snapshot().queue[0].ability==Ability::absorb_hp,"reflection continuation never fires damage side effect before final body");roundtrip(reflected);reflected.step(defend(0));check(reflected.snapshot().state.players[0].hp==23&&reflected.snapshot().state.players[1].hp==86,"absorption credits current reflected source rather than immutable original owner");roundtrip(reflected);
    for(const auto trial:{std::pair<std::uint64_t,bool>{27,true},{123,false}}){auto state=scene({vine},{});state.players[0].hp=50;Match m;m.load_scene(state,0,trial.first);m.step(chance_play(0,{0}));if(trial.second)m.step(defend(7));check(m.snapshot().state.players[0].hp==(trial.second?53:50)&&m.snapshot().state.players[1].hp==(trial.second?37:40)&&m.snapshot().chance_rolls==1&&m.snapshot().chance_hits==static_cast<unsigned>(trial.second),"vine75 percent miss has no side effect while hit resolves exactly one absorption");roundtrip(m);}
    auto vine_state=scene({vine},{image("rainbow-curtain"),image("super-mirror")});vine_state.players[0].hp=50;vine_state.players[1].hp=80;Match filtered;filtered.load_scene(vine_state,0,27);filtered.step(chance_play(0,{0}));filtered.step(defend(7,{0,1}));const auto rng=filtered.snapshot().rng;check(!filtered.snapshot().queue[0].element&&filtered.snapshot().queue[0].hit_rate==75&&filtered.snapshot().state.players[1].hp==80,"vine filter/reflection preserves hit proof and defers healing");roundtrip(filtered);filtered.step(defend(0));check(filtered.snapshot().state.players[0].hp==47&&filtered.snapshot().state.players[1].hp==83&&filtered.snapshot().chance_rolls==1&&filtered.snapshot().rng!=rng,"filtered reflected vine resolves unchanged3 power, with only eventual gifts consuming later RNG");roundtrip(filtered);
    auto magic=scene({spell},{image("moonlight-helm")});auto m=fixture(magic);m.step(play(0,{0},7));m.step(defend(7,{0}));check(m.snapshot().state.players[0].mp==0&&m.snapshot().state.players[0].items[0].used,"absorption miracle retains original token and pays10 once across reflection");roundtrip(m);m.step(defend(0));check(m.snapshot().state.players[0].hp==30&&m.snapshot().state.players[1].hp==50,"reflected Light absorption heals current source and damages current target once");roundtrip(m);
    auto free_state=scene({spell,image("spiritual-doll")},{image("angel-knife")});free_state.players[0].mp=0;auto free=fixture(free_state);free.step(play(0,{0,1},7));free.step(defend(7,{0}));check(free.snapshot().state.players[0].mp==0&&free.snapshot().state.players[0].items[0].used&&free.snapshot().state.players[0].hp==40&&free.snapshot().state.players[1].hp==40,"FREE retained absorption may be blocked with both healing and damage canceled");roundtrip(free);
    auto again_state=scene({spell},{image("smile-dew")});again_state.players[0].mp=20;auto again=fixture(again_state);again.step(play(0,{0},0));again.step(defend(0));again.step(play(7,{0},7));again.step(play(0,{0},0));again.step(defend(0));check(again.snapshot().state.players[0].mp==0&&again.snapshot().state.players[0].hp==40&&std::count_if(again.snapshot().state.players[0].items.begin(),again.snapshot().state.players[0].items.end(),[&](auto& i){return i.token==0&&i.model==spell&&i.used;})==1,"same retained absorption token pays10 on each real turn while self stages preserve uncapped HP");roundtrip(again);
}

void same_damage_stages_deaths_and_journal(){
    const auto evil=image("evil-broadsword"),saturn=image("saturn-ring");auto self=fixture(scene({evil,saturn},{}));self.step(play(0,{0},0));check(self.snapshot().state.players[0].hp==40,"self-damage weapon has no launch-time HP debit");self.step(defend(0,{1}));check(self.snapshot().state.players[0].hp==12&&self.snapshot().queue[0].power==28&&self.snapshot().queue[0].is_counter,"53 self alias applies40 to26 source then26 to12 target and counter uses14 rather than28 HP loss");roundtrip(self);self.step(defend(0));check(self.snapshot().state.players[0].dead&&self.snapshot().state.players[1].hp==40&&self.snapshot().winners==std::vector<PlayerId>{7},"counter generated from53 does not inherit same-damage side effect");roundtrip(self);
    auto both=scene({evil},{});both.players[0].hp=both.players[1].hp=5;auto draw=fixture(both);draw.step(play(0,{0},7));draw.step(defend(7));check(draw.snapshot().terminated&&!draw.snapshot().truncated&&draw.snapshot().state.players[0].dead&&draw.snapshot().state.players[1].dead&&draw.snapshot().winners.empty()&&draw.snapshot().rewards==std::vector<double>{0,0},"source-side death never suppresses target-stage damage and simultaneous deaths produce fixed-seat draw");roundtrip(draw);
    auto three=scene({evil},{saturn});three.players[0].hp=5;three.players.push_back(player(9));auto deadsource=fixture(three);deadsource.step(play(0,{0},7));deadsource.step(defend(7,{0}));check(deadsource.snapshot().state.players[0].dead&&deadsource.snapshot().state.players[1].hp==26&&deadsource.snapshot().counter_created==1&&deadsource.snapshot().counter_started==0&&deadsource.snapshot().owner==7&&!deadsource.snapshot().terminated,"surviving target creates counter after original source dies, then dead-target skip cancels it and next surviving owner advances");roundtrip(deadsource);
    auto clipped=scene({evil,saturn},{});clipped.players[0].hp=10;auto zero=fixture(clipped);zero.step(play(0,{0},0));zero.step(defend(0,{1}));check(zero.snapshot().state.players[0].hp==0&&zero.snapshot().counter_created==0,"source stage may exhaust alias HP before target stage; zero target-stage loss creates no counter");roundtrip(zero);
    auto reflected=fixture(scene({evil},{image("reflection-sword")}));reflected.step(play(0,{0},7));reflected.step(defend(7,{0}));check(reflected.snapshot().state.players[0].hp==40&&reflected.snapshot().state.players[1].hp==40,"53 side effects wait through physical reflection");reflected.step(defend(0));check(reflected.snapshot().state.players[0].hp==26&&reflected.snapshot().state.players[1].hp==26,"reflected53 applies to current source then current target exactly once");roundtrip(reflected);
    auto pending=fixture(scene({image("ghost-sword")},{}));pending.step(play(0,{0},7));const auto exact=pending.snapshot();for(int error=0;error<9;++error){auto bad=exact;if(error==0)bad.queue[0].ability=Ability::deal_same_damage;if(error==1)bad.queue[0].models[0]=image("real-ghost-sword");if(error==2)bad.queue[0].power=12;if(error==3)bad.queue[0].element=Element::darkness;if(error==4)++bad.state.players[0].hp;if(error==5)--bad.state.players[1].hp;if(error==6)bad.queue[0].origin=7;if(error==7)++bad.queue[0].amount;if(error==8)bad.batch->responses.push_back(defend(7));bool rejected=false;try{pending.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==exact,"full action replay rejects side-effect identity, composition, stage HP and history forgery atomically");}
    check(pending.observe(0).find(damage_effect_rule)!=std::string::npos,"view declares current-source side effects and target-stage counter basis as local contract");auto late=exact;late.state.update_count=max_wire_integer;late.batch->initial_state.update_count=max_wire_integer-(late.decisions-late.batch->decisions);pending.restore(late);const auto before=pending.snapshot();bool rejected=false;try{pending.step(defend(7));}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==before,"late update failure rolls back source healing, target damage, counters, journal, gifts and RNG");
}

void composed_damage_effect_and_block_cancel(){
    auto state=scene({image("ghost-sword"),image("aura"),image("spiritual-doll"),image("wand-of-mystic-water"),image("aqua-shoes")},{image("rainbow-curtain"),image("reflection-sword")});state.players[0].hp=60;state.players[0].mp=0;state.players[1].hp=70;auto m=fixture(state);m.step(play(0,{0,1,2,3},7));
    check(m.snapshot().queue[0].power==19&&m.snapshot().queue[0].element==Element::water&&m.snapshot().queue[0].ability==Ability::absorb_hp&&m.snapshot().state.players[0].mp==0,"ordered absorption preserves7 doubled to14 plus Water setter5 and waives Aura with immediate FREE");roundtrip(m);m.step(defend(7,{0,1}));check(m.snapshot().queue[0].power==19&&!m.snapshot().queue[0].element&&m.snapshot().state.players[0].hp==60&&m.snapshot().state.players[1].hp==70,"filter/reflection carries computed composition while side effect remains deferred");roundtrip(m);m.step(defend(0,{4}));
    check(m.snapshot().state.players[0].hp==42&&m.snapshot().state.players[1].hp==88,"final DEF1 leaves18 computed remainder credited to current source then debited from target, not basic7 or raw19");roundtrip(m);
    auto blocked=fixture(scene({image("evil-broadsword")},{image("wall")}));blocked.step(play(0,{0},7));blocked.step(defend(7,{0}));check(blocked.snapshot().state.players[0].hp==40&&blocked.snapshot().state.players[1].hp==40&&blocked.snapshot().state.players[1].mp==4&&blocked.snapshot().state.players[1].items[0].used,"Wall cancels53 source self-damage together with target body while retaining normal cost/holding");roundtrip(blocked);
}

void flash_damage_selection_and_cures(){
    const auto dagger=image("flash-dagger"),ring=image("uranus-ring"),mirror=image("super-mirror");
    for(auto id:{dagger,ring,image("flash"),image("smile-shell"),image("heart-shell"),image("tone"),image("song")})
        check(match_supported(id),"all seven Flash and cure models have complete supported roles");
    auto state=scene({dagger},{image("aqua-shoes"),image("rainbow-curtain"),mirror});
    auto m=fixture(state);m.step(play(0,{0},7));check(m.snapshot().state.players[1].curses.empty(),"Flash does not apply before response");
    m.step(defend(7));check(m.snapshot().state.players[1].hp==38&&m.snapshot().state.players[1].curses==std::vector<Curse>{Curse::flash},"positive target HP loss applies Flash");roundtrip(m);
    check(m.observe(7).find("\"curses\":[\"flash\"]")!=std::string::npos&&m.observe({}).find(flash_rule)!=std::string::npos,"Flash status and experimental rule are public");
    auto flashed=scene({image("bronze-club")},{image("aqua-shoes"),image("rainbow-curtain"),mirror});flashed.players[1].curses={Curse::flash};
    auto limited=fixture(flashed);limited.step(play(0,{0},7));reject(limited,defend(7,{0,1}));reject(limited,defend(7,{1,2}));
    limited.step(defend(7,{0}));check(limited.snapshot().state.players[1].curses==std::vector<Curse>{Curse::flash},"ordinary damage or defense does not implicitly cure Flash");roundtrip(limited);
    auto repeat=scene({dagger},{});repeat.players[1].curses={Curse::flash};auto idem=fixture(repeat);idem.step(play(0,{0},7));idem.step(defend(7));
    check(idem.snapshot().state.players[1].curses.size()==1,"Flash reapplication is idempotent");roundtrip(idem);
    auto lethal=scene({dagger},{});lethal.players[1].hp=1;auto dead=fixture(lethal);dead.step(play(0,{0},7));dead.step(defend(7));
    check(dead.snapshot().state.players[1].dead&&dead.snapshot().state.players[1].curses==std::vector<Curse>{Curse::flash},"local positive-HP-loss rule retains Flash even on lethal damage");roundtrip(dead);
    auto zero=fixture(scene({dagger},{image("rainbow-curtain"),image("god-shield")}));zero.step(play(0,{0},7));zero.step(defend(7,{0,1}));
    check(zero.snapshot().state.players[1].hp==40&&zero.snapshot().state.players[1].curses.empty(),"filtered zero damage does not apply Flash");roundtrip(zero);
    auto reflected=fixture(scene({dagger},{mirror}));reflected.step(play(0,{0},7));reflected.step(defend(7,{0}));
    check(reflected.snapshot().state.players[0].curses.empty(),"Flash waits through redirect");reflected.step(defend(0));
    check(reflected.snapshot().state.players[0].hp==38&&reflected.snapshot().state.players[0].curses==std::vector<Curse>{Curse::flash}&&reflected.snapshot().state.players[1].curses.empty(),"reflected damage applies Flash only to current target");roundtrip(reflected);
    auto main=scene({image("bronze-club"),image("crossbow")},{});main.players[0].curses={Curse::flash};auto multi=fixture(main);
    multi.step(play(0,{0,1},7));multi.step(defend(7));check(multi.snapshot().state.players[1].hp==37,"Flash restricts response selection, not main composition");roundtrip(multi);
    auto nofree=scene({image("spring")},{image("turbulence"),image("spiritual-doll")});nofree.players[1].mp=0;nofree.players[1].curses={Curse::flash};auto sampler=fixture(nofree);
    sampler.step(play(0,{0},7));reject(sampler,defend(7,{0,1}));const auto frozen=sampler.snapshot();
    for(std::uint64_t rng=0;rng<64;++rng){auto policy=rng;const auto command=sampler.sample_action(policy);check(command.items.empty()&&sampler.legal(command),"flashed MP0 responder cannot sample unaffordable miracle plus FREE");}
    check(sampler.snapshot()==frozen,"Flash sampler does not mutate game state or RNG");sampler.step(defend(7));roundtrip(sampler);
    for(auto id:{image("smile-shell"),image("heart-shell"),image("tone"),image("song")}){
        auto cure=scene({id},{});cure.players[0].curses={Curse::flash};cure.players[0].mp=20;auto clean=fixture(cure);clean.step(play(0,{0},0));
        check(clean.snapshot().state.players[0].curses.empty()&&clean.snapshot().state.players[0].mp==20-find_card(id)->cost,"mild and all cures remove Flash in the closed Flash universe");roundtrip(clean);
        if(find_card(id)->category==Category::miracles){check(clean.snapshot().state.players[0].items[0].used,"cure miracle retains its token");clean.step(pray(7));clean.step(play(0,{0},0));check(clean.snapshot().state.players[0].mp==20-2*find_card(id)->cost,"retained cure pays again even when there is nothing to remove");roundtrip(clean);}
    }
    auto paid=scene({image("tone"),image("spiritual-doll")},{});paid.players[0].mp=0;paid.players[0].curses={Curse::flash};auto free=fixture(paid);free.step(play(0,{0,1},0));
    check(free.snapshot().state.players[0].curses.empty()&&free.snapshot().state.players[0].mp==0&&free.snapshot().state.players[0].items[0].used,"flashed main actor may cure with miracle plus FREE");roundtrip(free);
    auto foreign=scene({image("smile-shell")},{mirror});for(auto& p:foreign.players)p.curses={Curse::flash};auto reverse=fixture(foreign);reverse.step(play(0,{0},7));reverse.step(defend(7,{0}));reverse.step(defend(0));
    check(reverse.snapshot().state.players[0].curses.empty()&&reverse.snapshot().state.players[1].curses==std::vector<Curse>{Curse::flash},"universal reflection redirects a nonattack cure");roundtrip(reverse);
    auto blocked=scene({image("tone")},{image("angel-knife")});for(auto& p:blocked.players)p.curses={Curse::flash};auto keep=fixture(blocked);
    keep.step(play(0,{0},7));keep.step(defend(7,{0}));check(keep.snapshot().state.players[0].curses==std::vector<Curse>{Curse::flash}&&keep.snapshot().state.players[1].curses==std::vector<Curse>{Curse::flash}&&keep.snapshot().state.players[0].mp==8,"miracle block cancels foreign cure while preserving original cost and both Flash states");roundtrip(keep);
}

void flash_counter_chance_and_journal(){
    auto fifo=fixture(scene({image("bronze-club"),elemental(Category::armor,Element::wood),elemental(Category::armor,Element::wood)},{image("uranus-ring"),image("saturn-ring")}));
    fifo.step(play(0,{0},7));fifo.step(defend(7,{0,1}));
    check(fifo.snapshot().queue[0].ability==Ability::counter_curse&&fifo.snapshot().queue[0].category==Category::armor&&fifo.snapshot().queue[0].power==0&&fifo.snapshot().queue[0].amount==0&&fifo.snapshot().queue[0].element==Element::light,"Flash counter is a nonattack armor payload");
    reject(fifo,defend(0,{1}));const auto waiting=fifo.snapshot();roundtrip(fifo);fifo.step(defend(0));
    check(fifo.snapshot().state.players[0].curses==std::vector<Curse>{Curse::flash}&&fifo.snapshot().queue[0].ability==Ability::counter2x_atk,"Flash FIFO effect precedes subsequent damage counter");
    reject(fifo,defend(0,{1,2}));fifo.step(defend(0,{1}));roundtrip(fifo);
    auto reflect=fixture(scene({image("bronze-club"),image("super-mirror")},{image("uranus-ring")}));reflect.step(play(0,{0},7));reflect.step(defend(7,{0}));reflect.step(defend(0,{1}));reflect.step(defend(7));
    check(reflect.snapshot().state.players[0].curses.empty()&&reflect.snapshot().state.players[1].curses==std::vector<Curse>{Curse::flash},"190 redirects Flash counter without creating another counter");roundtrip(reflect);
    for(const auto trial:{std::pair<std::uint64_t,bool>{100,true},{136,false}}){auto state=scene({image("flash")},{});state.players[0].mp=3;Match m;m.load_scene(state,0,trial.first);m.step(chance_play(0,{0}));if(trial.second)m.step(defend(7));
        check(m.snapshot().state.players[1].hp==(trial.second?39:40)&&m.snapshot().state.players[1].curses.empty()!=trial.second&&m.snapshot().chance_rolls==1,"Flash25 chance applies state only after hit and positive damage");roundtrip(m);}
    Match pending;pending.restore(waiting);const auto exact=pending.snapshot();
    for(int error=0;error<6;++error){auto bad=exact;if(error==0)bad.state.players[0].curses={Curse::flash};if(error==1)bad.queue[0].power=1;if(error==2)bad.queue[0].ability=Ability::counter_atk;if(error==3)bad.queue[0].target=7;if(error==4)bad.state.players[1].curses={Curse::dream};if(error==5)bad.state.players[1].curses={Curse::flash,Curse::flash};bool rejected=false;try{pending.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==exact,"replay rejects forged Flash, other curses and altered counter payload atomically");}
    auto late=exact;late.state.update_count=max_wire_integer;late.batch->initial_state.update_count=max_wire_integer-(late.decisions-late.batch->decisions);pending.restore(late);const auto before=pending.snapshot();bool rejected=false;try{pending.step(defend(0));}catch(const MatchError&){rejected=true;}
    check(rejected&&pending.snapshot()==before,"late failure rolls back Flash application, FIFO, journal and RNG");
    auto duplicate=scene({},{});duplicate.players[0].curses={Curse::flash,Curse::flash};rejected=false;try{pending.load_scene(duplicate,0,1);}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==before,"duplicate Flash import is rejected atomically rather than silently canonicalized");
}

void darkcloud_forced_chance_and_mixed_roster(){
    const auto spark=image("spark-bag");auto plain=scene({spark},{});Match control;control.load_scene(plain,0,123);control.step(chance_play(0,{0}));check(control.snapshot().chance_rolls==1&&control.snapshot().chance_hits==0&&control.snapshot().chance_forced_hits==0&&!control.snapshot().batch,"seed123 normal75 roll misses without DarkCloud");
    auto state=scene({spark},{image("super-mirror")});state.players[1].curses={Curse::darkcloud};Match forced;forced.load_scene(state,0,123);forced.step(chance_play(0,{0}));check(forced.snapshot().actor==7&&forced.snapshot().queue[0].forced_hit&&forced.snapshot().queue[0].hit_rate==75&&forced.snapshot().rng==123&&forced.snapshot().chance_rolls==0&&forced.snapshot().chance_hits==0&&forced.snapshot().chance_forced_hits==1,"DarkCloud forces original75 hit with zero draw and separate metric");roundtrip(forced);forced.step(defend(7,{0}));check(forced.snapshot().actor==0&&forced.snapshot().queue[0].forced_hit&&forced.snapshot().queue[0].hit_rate==75&&forced.snapshot().rng==123&&!forced.snapshot().state.players[0].curses.size(),"reflection preserves forced proof and original rate without rechecking unmarked destination");roundtrip(forced);forced.step(defend(0));check(forced.snapshot().state.players[0].hp==39&&forced.snapshot().state.players[1].curses==std::vector<Curse>{Curse::darkcloud}&&forced.snapshot().chance_forced_hits==1&&forced.snapshot().chance_rolls==0,"forced redirected body resolves once and normal damage does not remove DarkCloud");roundtrip(forced);
    SnapshotState nine;for(PlayerId id=0;id<9;++id){auto p=player(id,id==0?std::initializer_list<CardId>{spark}:std::initializer_list<CardId>{});if(id%2)p.curses={Curse::darkcloud};nine.players.push_back(std::move(p));}Match mixed;mixed.load_scene(nine,0,123);mixed.step(chance_play(0,{0}));
    for(PlayerId actor:{1u,3u,4u,5u,6u,7u,8u}){check(mixed.snapshot().actor==actor&&mixed.snapshot().queue[0].forced_hit==static_cast<bool>(actor%2),"mixed fixed-seat roster alternates forced hits with independent ordinary percentage draws");roundtrip(mixed);if(actor==8)check(mixed.snapshot().rng==std::uint64_t{123}+std::uint64_t{4}*0x9e3779b97f4a7c15ULL,"only four non-Dark target activations advance environment RNG before gifts");mixed.step(defend(actor));}
    check(mixed.snapshot().chance_rolls==4&&mixed.snapshot().chance_hits==3&&mixed.snapshot().chance_forced_hits==4&&mixed.snapshot().state.players[2].hp==40&&mixed.snapshot().owner==1&&!mixed.snapshot().batch,"nine-player mixed action has4 draws,3 drawn hits,4 forced hits and one genuine miss");roundtrip(mixed);
}

void darkcloud_damage_sources_and_cures(){
    const auto hex=image("hexagon-doom"),dark=image("dark-cloud"),mirror=image("super-mirror");
    auto m=fixture(scene({hex},{}));m.step(play(0,{0},7));check(m.snapshot().state.players[1].curses.empty(),"DarkCloud on-damage payload waits until positive target damage");m.step(defend(7));check(m.snapshot().state.players[1].hp==29&&m.snapshot().state.players[1].curses==std::vector<Curse>{Curse::darkcloud},"hexagon11 applies DarkCloud without Dark execution");roundtrip(m);
    auto zero=fixture(scene({hex},{image("god-shield")}));zero.step(play(0,{0},7));zero.step(defend(7,{0}));check(zero.snapshot().state.players[1].hp==40&&zero.snapshot().state.players[1].curses.empty(),"fully defended hexagon never applies DarkCloud");roundtrip(zero);
    auto lethal=scene({hex},{});lethal.players[1].hp=1;auto dead=fixture(lethal);dead.step(play(0,{0},7));dead.step(defend(7));check(dead.snapshot().state.players[1].dead&&dead.snapshot().state.players[1].curses==std::vector<Curse>{Curse::darkcloud},"positive lethal damage still adds DarkCloud under explicit local rule");roundtrip(dead);
    auto reflect=fixture(scene({hex},{mirror}));reflect.step(play(0,{0},7));reflect.step(defend(7,{0}));reflect.step(defend(0));check(reflect.snapshot().state.players[0].hp==29&&reflect.snapshot().state.players[0].curses==std::vector<Curse>{Curse::darkcloud}&&reflect.snapshot().state.players[1].curses.empty(),"reflected hexagon adds DarkCloud to final target only");roundtrip(reflect);
    auto repeat=scene({hex},{});repeat.players[1].curses={Curse::flash,Curse::darkcloud};auto idem=fixture(repeat);idem.step(play(0,{0},7));idem.step(defend(7));check(idem.snapshot().state.players[1].curses==std::vector<Curse>({Curse::flash,Curse::darkcloud}),"on-damage reapplication preserves canonical unique Flash plus DarkCloud");roundtrip(idem);
    auto own=fixture(scene({dark},{image("smile-dew")}));own.step(play(0,{0},0));check(own.snapshot().state.players[0].hp==40&&own.snapshot().state.players[0].mp==5&&own.snapshot().state.players[0].items[0].used&&own.snapshot().state.players[0].curses==std::vector<Curse>{Curse::darkcloud}&&!own.snapshot().batch,"DarkCloud miracle self-use is immediate nonattack, pays5 and retains token");roundtrip(own);own.step(play(7,{0},7));own.step(play(0,{0},0));check(own.snapshot().state.players[0].mp==0&&own.snapshot().state.players[0].curses.size()==1,"same retained DarkCloud miracle pays again and remains idempotent");roundtrip(own);
    auto free_state=scene({dark,image("spiritual-doll")},{});free_state.players[0].mp=0;free_state.players[0].curses={Curse::flash};auto free=fixture(free_state);free.step(play(0,{0,1},0));check(free.snapshot().state.players[0].mp==0&&free.snapshot().state.players[0].items[0].used&&free.snapshot().state.players[0].curses==std::vector<Curse>({Curse::flash,Curse::darkcloud}),"Flash permits main DarkCloud plus FREE and canonical mixed state");roundtrip(free);
    auto blocked=fixture(scene({dark},{image("angel-knife")}));blocked.step(play(0,{0},7));blocked.step(defend(7,{0}));check(blocked.snapshot().state.players[0].mp==5&&blocked.snapshot().state.players[0].items[0].used&&blocked.snapshot().state.players[1].hp==40&&blocked.snapshot().state.players[1].curses.empty(),"magic blocker cancels nonattack DarkCloud without refunding cost or retention");roundtrip(blocked);
    auto magic=fixture(scene({dark},{image("moonlight-helm")}));magic.step(play(0,{0},7));magic.step(defend(7,{0}));roundtrip(magic);magic.step(defend(0));check(magic.snapshot().state.players[0].curses==std::vector<Curse>{Curse::darkcloud}&&magic.snapshot().state.players[1].curses.empty()&&magic.snapshot().state.players[0].hp==40,"magic reflection redirects pure DarkCloud without treating Darkness element as damage");roundtrip(magic);
    for(const auto name:{"smile-shell","heart-shell","tone","song"}){auto state=scene({image(name)},{});state.players[0].mp=20;state.players[0].curses={Curse::flash,Curse::darkcloud};auto cure=fixture(state);cure.step(play(0,{0},0));const auto mild=find_card(image(name))->ability==Ability::remove_mild_curses;check(cure.snapshot().state.players[0].curses==(mild?std::vector<Curse>{Curse::darkcloud}:std::vector<Curse>{}),"mild cures remove only Flash whereas all cures remove Flash and DarkCloud");roundtrip(cure);}
}

void darkcloud_counter_fifo_forced_and_selection(){
    const auto pluto=image("pluto-ring"),uranus=image("uranus-ring"),mars=image("mars-ring");auto state=scene({image("bronze-club"),image("aqua-shoes"),image("aqua-shoes")},{pluto,uranus,mars});Match fifo;fifo.load_scene(state,0,123);fifo.step(play(0,{0},7));fifo.step(defend(7,{0,1,2}));
    check(fifo.snapshot().queue[0].models==std::vector<CardId>{pluto}&&fifo.snapshot().queue[0].power==0&&fifo.snapshot().queue[0].amount==0&&fifo.snapshot().queue[0].category==Category::armor&&fifo.snapshot().queue[0].element==Element::darkness&&fifo.snapshot().state.players[0].hp==40,"Pluto is pure nonattack Darkness counter rather than immediate Dark damage");roundtrip(fifo);reject(fifo,defend(0,{1}));fifo.step(defend(0));check(fifo.snapshot().state.players[0].hp==40&&fifo.snapshot().state.players[0].curses==std::vector<Curse>{Curse::darkcloud}&&fifo.snapshot().queue[0].models==std::vector<CardId>{uranus},"first FIFO pure Dark counter adds state without HP loss before Flash counter");roundtrip(fifo);fifo.step(defend(0));
    check(fifo.snapshot().state.players[0].curses==std::vector<Curse>({Curse::flash,Curse::darkcloud})&&fifo.snapshot().queue[0].models==std::vector<CardId>{mars}&&fifo.snapshot().queue[0].forced_hit&&fifo.snapshot().queue[0].hit_rate==75&&fifo.snapshot().counter_forced_hits==1&&fifo.snapshot().counter_rolls==0&&fifo.snapshot().counter_hits==0&&fifo.snapshot().rng==123,"same FIFO applies Flash/Dark before Mars activation, forcing original75 hit without a draw");roundtrip(fifo);reject(fifo,defend(0,{1,2}));fifo.step(defend(0,{1}));check(fifo.snapshot().state.players[0].hp==40&&fifo.snapshot().state.players[0].curses==std::vector<Curse>({Curse::flash,Curse::darkcloud}),"new Flash enforces one-card defense and Dark persists through blocked ordinary damage");roundtrip(fifo);
    auto reflect=fixture(scene({image("bronze-club"),image("super-mirror"),image("rainbow-curtain"),image("aqua-shoes")},{pluto}));reflect.step(play(0,{0},7));reflect.step(defend(7,{0}));reject(reflect,defend(0,{2}));reject(reflect,defend(0,{3}));reflect.step(defend(0,{1}));roundtrip(reflect);reflect.step(defend(7));check(reflect.snapshot().state.players[0].curses.empty()&&reflect.snapshot().state.players[1].hp==39&&reflect.snapshot().state.players[1].curses==std::vector<Curse>{Curse::darkcloud},"190 alone redirects pure Pluto curse without Dark execution or nested counter");roundtrip(reflect);
}

void darkcloud_canonical_journal_and_cutoff(){
    auto state=scene({image("spark-bag")},{});state.players[1].curses={Curse::darkcloud};Match m;m.load_scene(state,0,123);m.step(chance_play(0,{0}));const auto exact=m.snapshot();for(int error=0;error<12;++error){auto bad=exact;if(error==0)bad.queue[0].forced_hit=false;if(error==1)bad.queue[0].hit_rate=100;if(error==2)bad.chance_forced_hits=0;if(error==3)bad.chance_rolls=1;if(error==4)bad.chance_hits=1;if(error==5)++bad.batch->chance_forced_hits;if(error==6)bad.batch->initial_state.players[1].curses.clear();if(error==7)bad.batch->rng^=1;if(error==8)bad.queue[0].redirect_rng^=1;if(error==9)++bad.rng;if(error==10)bad.state.players[1].curses.clear();if(error==11)++bad.counter_forced_hits;bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"replay rejects forged forced proof, counters, initial curse, RNG anchors and current state atomically");}
    for(const auto curses:{std::vector<Curse>{Curse::darkcloud,Curse::darkcloud},std::vector<Curse>{Curse::darkcloud,Curse::flash},std::vector<Curse>{Curse::flash,Curse::darkcloud,Curse::flash},std::vector<Curse>{Curse::darkcloud,Curse::dream}}){auto invalid=scene();invalid.players[0].curses=curses;bool rejected=false;try{m.load_scene(invalid,0,1);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"runtime rejects duplicate, unsorted and unsupported curses");}
    auto late=state;late.update_count=max_wire_integer;Match failed;failed.load_scene(late,0,123);const auto before=failed.snapshot();check(failed.legal(chance_play(0,{0})),"forced launch can fail only at late transport boundary");bool rejected=false;try{failed.step(chance_play(0,{0}));}catch(const MatchError&){rejected=true;}check(rejected&&failed.snapshot()==before,"late launch rollback restores consumed item, forced counter, RNG and new journal");
    MatchConfig limit;limit.max_decisions=1;Match cut(limit);cut.load_scene(state,0,123);cut.step(chance_play(0,{0}));check(cut.snapshot().truncated&&!cut.snapshot().terminated&&cut.snapshot().chance_forced_hits==1&&cut.snapshot().chance_rolls==0&&cut.snapshot().chance_hits==0&&cut.snapshot().rng==123&&!cut.snapshot().batch&&cut.snapshot().queue.empty()&&cut.snapshot().state.players[1].hp==40,"forced hit survives launch cutoff accounting while no response HP or gifts execute");roundtrip(cut);
    auto counter_state=scene({image("bronze-club")},{image("pluto-ring"),image("mars-ring")});MatchConfig counter_limit;counter_limit.max_decisions=4;Match counter(counter_limit);counter.load_scene(counter_state,0,123);counter.step(play(0,{0},7));counter.step(defend(7,{0,1}));counter.step(defend(0));counter.step(defend(0));check(counter.snapshot().truncated&&counter.snapshot().counter_forced_hits==1&&counter.snapshot().counter_rolls==0&&counter.snapshot().counter_hits==0&&counter.snapshot().state.players[0].hp==39&&counter.snapshot().rng==123&&!counter.snapshot().batch,"counter forced-hit metric and no-draw RNG persist through command-boundary cutoff");roundtrip(counter);
}

void removal_counts_tokens_and_rng(){
    const auto broom=image("nocturnal-broom"),soap=image("goddess-s-soap");MatchConfig cutoff;cutoff.max_decisions=2;
    for(unsigned n=0;n<=5;++n){auto state=scene({broom},{});for(ItemToken token=1;token<=n;++token)state.players[1].items.push_back({token,image("bronze-club"),{},false});state.players[1].items.push_back({90,image("spring"),{},true});Match m(cutoff);m.load_scene(state,0,0);m.step(play(0,{0},7));check(m.snapshot().queue[0].amount==3&&m.snapshot().queue[0].category==Category::sundries&&m.snapshot().queue[0].power==0,"broom has derived max3 amount as nonattack sundry");roundtrip(m);m.step(defend(7));
        std::vector<ItemToken> remaining;if(n==4)remaining={2};if(n==5)remaining={3,5};remaining.push_back(90);std::vector<ItemToken> actual;for(auto& item:m.snapshot().state.players[1].items)actual.push_back(item.token);
        check(actual==remaining&&m.snapshot().state.players[1].items.back().used&&m.snapshot().rng==std::uint64_t{std::min(3u,n)}*0x9e3779b97f4a7c15ULL&&m.snapshot().truncated&&m.snapshot().next_token==91,"broom stable token sampling without replacement handles0..5 duplicate copies, preserves used miracle and draws once even for sole candidate");roundtrip(m);}
    for(unsigned n=0;n<=5;++n){auto state=scene({soap},{});for(ItemToken token=1;token<=n;++token)state.players[1].items.push_back({token,image("spring"),{},true});state.players[1].items.push_back({90,image("dark-cloud"),{},false});state.players[1].items.push_back({91,image("bronze-club"),{},false});Match m(cutoff);m.load_scene(state,0,0);m.step(play(0,{0},7));check(m.snapshot().queue[0].amount==2,"soap has derived max2 used-copy amount");m.step(defend(7));
        std::vector<ItemToken> remaining;if(n==3)remaining={3};if(n==4)remaining={2,3};if(n==5)remaining={3,4,5};remaining.push_back(90);remaining.push_back(91);std::vector<ItemToken> actual;for(auto& item:m.snapshot().state.players[1].items)actual.push_back(item.token);
        check(actual==remaining&&!m.snapshot().state.players[1].items[m.snapshot().state.players[1].items.size()-2].used&&m.snapshot().rng==std::uint64_t{std::min(2u,n)}*0x9e3779b97f4a7c15ULL,"soap uniformly removes physical used-miracle copies only, with stable order and empty/sole draw contract");roundtrip(m);}
    MatchConfig one;one.max_decisions=1;auto own=scene({broom,image("spring")},{});own.players[0].items[1].used=true;Match used(one);used.load_scene(own,0,0);used.step(play(0,{0},0));check(used.snapshot().state.players[0].items.size()==1&&used.snapshot().state.players[0].items[0].token==1&&used.snapshot().state.players[0].items[0].used&&used.snapshot().rng==0,"self broom consumes itself before pool construction and never removes already-used miracle");roundtrip(used);
    auto ownsoap=scene({soap,image("spring"),image("spring"),image("bronze-club")},{});ownsoap.players[0].items[1].used=true;Match self(one);self.load_scene(ownsoap,0,0);self.step(play(0,{0},0));check(self.snapshot().state.players[0].items.size()==2&&self.snapshot().state.players[0].items[0].token==2&&!self.snapshot().state.players[0].items[0].used&&self.snapshot().state.players[0].items[1].token==3&&self.snapshot().rng==0x9e3779b97f4a7c15ULL,"self soap removes used1 only after own consumption, leaves unused miracle unretained and draws exactly once before cutoff");roundtrip(self);
}

void removal_retention_gifts_and_status(){
    const auto broom=image("nocturnal-broom"),soap=image("goddess-s-soap");auto foreign=scene({broom},{});for(ItemToken token=1;token<=4;++token)foreign.players[1].items.push_back({token,image("bronze-club"),{},false});Match m;m.load_scene(foreign,0,0);m.step(play(0,{0},7));m.step(defend(7));check(m.snapshot().state.players[0].items.size()==1&&m.snapshot().state.players[1].items.size()==1&&m.snapshot().state.players[1].items[0].token==2&&m.snapshot().next_token==6,"three deleted enemy copies create no replacement gifts; only original physical use receives one");roundtrip(m);
    auto own=fixture(scene({broom,image("bronze-club"),image("bronze-club"),image("bronze-club")},{}));own.step(play(0,{0},0));check(own.snapshot().state.players[0].items.size()==1&&own.snapshot().state.players[0].items[0].token==4&&own.snapshot().next_token==5,"self broom removes three post-consumption candidates then keeps newly dealt gift rather than resweeping it");roundtrip(own);
    auto state=scene({soap},{});state.players[1].items={{5,image("dark-cloud"),{},true},{6,image("spring"),{},false},{7,image("spring"),{},true},{8,image("dark-cloud"),{},false},{9,image("bronze-club"),{},false}};state.players[1].curses={Curse::flash,Curse::darkcloud};auto retained=fixture(state);retained.step(play(0,{0},7));retained.step(defend(7));check(retained.snapshot().state.players[1].items.size()==3&&retained.snapshot().state.players[1].items[0].token==6&&!retained.snapshot().state.players[1].items[0].used&&retained.snapshot().state.players[1].items[1].token==8&&!retained.snapshot().state.players[1].items[1].used&&retained.snapshot().state.players[1].curses==std::vector<Curse>({Curse::flash,Curse::darkcloud}),"forgetting used223 removes held token without curing existing DarkCloud or retaining unrelated unused miracles");roundtrip(retained);reject(retained,play(7,{5},7));reject(retained,play(7,{7},7));retained.step(play(7,{6},7));check(retained.snapshot().state.players[1].mp==3&&retained.snapshot().state.players[1].hp==50&&retained.snapshot().state.players[1].items[0].token==6&&retained.snapshot().state.players[1].items[0].used,"surviving unused Spring can still be acquired normally after other copies were removed");roundtrip(retained);
}

void removal_reflection_and_journal_atomicity(){
    const auto broom=image("nocturnal-broom"),soap=image("goddess-s-soap"),mirror=image("super-mirror");auto state=scene({broom,image("bronze-club"),image("bronze-club"),image("spring")},{mirror,image("bronze-club"),image("aqua-shoes"),image("rainbow-curtain"),image("wall"),image("turbulence")});state.players[0].items[3].used=true;auto m=fixture(state);m.step(play(0,{0},7));for(ItemToken token:{1u,2u,3u,4u,5u})reject(m,defend(7,{token}));m.step(defend(7,{0}));check(m.snapshot().queue[0].target==0&&m.snapshot().queue[0].source==7&&m.snapshot().queue[0].amount==3&&m.snapshot().state.players[1].items[0].token==1,"only190 redirects sundry removal and is already consumed before final pool construction");roundtrip(m);m.step(defend(0));check(m.snapshot().state.players[0].items.size()==2&&m.snapshot().state.players[0].items[0].token==3&&m.snapshot().state.players[0].items[0].used&&m.snapshot().state.players[1].items.size()==6&&m.snapshot().state.players[1].items[0].token==1,"reflected broom removes currenttarget unused copies only and preserves original/190 pending gifts");roundtrip(m);
    auto used=scene({soap,image("dark-cloud"),image("spring")},{mirror,image("spring")});used.players[0].items[1].used=true;used.players[0].curses={Curse::darkcloud};used.players[1].items[1].used=true;auto reflected=fixture(used);reflected.step(play(0,{0},7));reflected.step(defend(7,{0}));roundtrip(reflected);reflected.step(defend(0));check(reflected.snapshot().state.players[0].items[0].token==2&&!reflected.snapshot().state.players[0].items[0].used&&reflected.snapshot().state.players[0].curses==std::vector<Curse>{Curse::darkcloud}&&reflected.snapshot().state.players[1].items[0].token==1&&reflected.snapshot().state.players[1].items[0].used,"reflected soap forgets currenttarget used copy, leaves foreign held copy and persistent DarkCloud state");roundtrip(reflected);
    auto target=scene({broom},{image("bronze-club"),image("bronze-club"),image("bronze-club"),image("bronze-club")});auto pending=fixture(target);pending.step(play(0,{0},7));const auto exact=pending.snapshot();for(int error=0;error<9;++error){auto bad=exact;if(error==0)bad.queue[0].amount=2;if(error==1)bad.queue[0].ability=Ability::remove_used_miracles;if(error==2)bad.queue[0].target=0;if(error==3)++bad.rng;if(error==4)++bad.queue[0].tokens[0];if(error==5)bad.state.players[1].items[0].token=200;if(error==6)bad.state.players[1].items.erase(bad.state.players[1].items.begin());if(error==7)bad.batch->attack.items[0]=99;if(error==8)++bad.batch->rng;bool rejected=false;try{pending.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==exact,"journal rejects removal count/kind/target/RNG/token and pre-body inventory forgery atomically");}
    check(pending.observe(0).find(removal_rule)!=std::string::npos,"view advertises local removal and RNG policy without privileged inventory history");auto exhausted=exact;exhausted.state.update_count=max_wire_integer;exhausted.batch->initial_state.update_count=max_wire_integer-(exhausted.decisions-exhausted.batch->decisions);pending.restore(exhausted);const auto before=pending.snapshot();bool rejected=false;try{pending.step(defend(7));}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==before,"late update failure rolls back all three removal draws, stable erases, journal and reservations");
    auto tokens=target;tokens.players[0].items[0].token=max_wire_integer;auto no_tokens=fixture(tokens);no_tokens.step(play(0,{max_wire_integer},7));const auto before_gift=no_tokens.snapshot();rejected=false;try{no_tokens.step(defend(7));}catch(const MatchError&){rejected=true;}check(rejected&&no_tokens.snapshot()==before_gift,"late gift token exhaustion also restores body removals and environmental RNG");
}

void mana_weapon_immutable_power_and_payment(){
    const auto stick=image("magical-stick"),aura=image("aura"),doll=image("spiritual-doll");
    struct Case{std::vector<CardId> cards;std::vector<ItemToken> selected;int power;};for(const auto& c:std::vector<Case>{{{stick,image("crossbow"),aura},{0,1,2},20},{{stick,aura,image("crossbow")},{0,1,2},18},{{stick,aura,doll,image("crossbow")},{0,1,2,3},42},{{stick,image("fireball")},{0,1},18}}){auto state=scene();for(std::size_t i=0;i<c.cards.size();++i)state.players[0].items.push_back({i,c.cards[i],{},false});auto m=fixture(state);m.step({0,CommandKind::play,c.selected,7});check(m.snapshot().state.players[0].mp==0&&m.snapshot().queue[0].power==c.power&&m.snapshot().queue[0].ability==Ability::atk_by2x_mp,"mana weapon bases ordered power on twice(original MP minus final MC cost) and burns whole original MP exactly once");roundtrip(m);m.step(defend(7));roundtrip(m);}
    auto zero_state=scene({stick,image("crossbow")},{image("aqua-shoes")});zero_state.players[0].mp=0;auto zero=fixture(zero_state);reject(zero,pray(0));zero.step(play(0,{0},7));check(zero.snapshot().queue[0].power==0&&zero.snapshot().phase==MatchPhase::response&&zero.snapshot().queue[0].category==Category::weapons,"zero-MP stick remains a weapon with explicit zero-power response");check(zero.observe(0).find(mana_weapon_rule)!=std::string::npos,"view declares immutable mana-weapon payment and composition rule");roundtrip(zero);zero.step(defend(7,{0}));check(zero.snapshot().state.players[1].hp==40&&zero.snapshot().counter_created==0,"zero-power body can consume defense but inflicts no HP loss or counter");roundtrip(zero);
    auto boosted=fixture(zero_state);boosted.step(play(0,{0,1},7));check(boosted.snapshot().queue[0].power==2&&boosted.snapshot().state.players[0].mp==0,"zero-MP weapon still adds ordinary boost after zero base");boosted.step(defend(7));roundtrip(boosted);
    auto poor=scene({stick,aura,doll},{});poor.players[0].mp=5;auto free=fixture(poor);reject(free,play(0,{0,1},7));free.step(play(0,{0,1,2},7));check(free.snapshot().queue[0].power==20&&free.snapshot().state.players[0].mp==0&&free.snapshot().state.players[0].items[0].used,"complete FREE pair permits otherwise unaffordable Aura and preserves all five MP in doubled base before burning it");roundtrip(free);
    auto reflection=scene({stick},{image("reflection-sword")});reflection.players[1].mp=99;auto back=fixture(reflection);back.step(play(0,{0},7));back.step(defend(7,{0}));check(back.snapshot().queue[0].power==20&&back.snapshot().state.players[0].mp==0&&back.snapshot().state.players[1].mp==99,"reflected stick preserves original paid power instead of recomputing from current source99MP or original source0MP");roundtrip(back);back.step(defend(0));check(back.snapshot().state.players[0].hp==20,"reflected original20 power resolves once");roundtrip(back);
    auto gain=fixture(scene({stick,image("neptune-ring")},{}));gain.step(play(0,{0},0));gain.step(defend(0,{1}));check(gain.snapshot().state.players[0].hp==20&&gain.snapshot().state.players[0].mp==0&&gain.snapshot().queue[0].is_counter&&gain.snapshot().queue[0].power==0&&gain.snapshot().queue[0].amount==40,"spent mana weapon generates independent MP counter without inheriting dynamic weapon base");roundtrip(gain);gain.step(defend(0));check(gain.snapshot().state.players[0].mp==40&&gain.snapshot().state.players[0].hp==20&&!gain.snapshot().batch&&gain.snapshot().decisions==3,"post-body MP refund does not replay or recompute original mana attack");roundtrip(gain);
}

void twice_weapon_fresh_frames_costs_and_order(){
    const auto saw=image("saw-boom-boom");auto plain=fixture(scene({saw},{image("aqua-shoes"),image("aqua-shoes")}));plain.step(play(0,{0},7));check(plain.snapshot().batch->targets==std::vector<PlayerId>{7,7}&&plain.snapshot().gifts[0]==1,"twice weapon creates duplicate explicit roster but consumes physical base and requests gift once");check(plain.observe(0).find(repeat_attack_rule)!=std::string::npos,"view declares repeated fresh-payload scheduling contract");plain.step(defend(7,{0}));check(plain.snapshot().state.players[1].hp==38&&plain.snapshot().queue[0].power==3&&plain.snapshot().gifts==std::vector<std::uint32_t>{1,1},"second hit starts fresh original3 power rather than first residual2; gifts remain deferred");roundtrip(plain);plain.step(defend(7,{1}));check(plain.snapshot().state.players[1].hp==36&&plain.snapshot().state.players[0].items.size()==1&&plain.snapshot().state.players[1].items.size()==2&&plain.snapshot().owner==7,"two hits require two separately consumed defenses and one final gift/owner transition");roundtrip(plain);
    auto colored=fixture(scene({saw,image("wand-of-mystic-water")},{image("rainbow-curtain"),image("aqua-shoes")}));colored.step(play(0,{0,1},7));colored.step(defend(7,{0,1}));check(colored.snapshot().state.players[1].hp==33&&colored.snapshot().queue[0].power==8&&colored.snapshot().queue[0].element==Element::water&&colored.snapshot().queue[0].redirects.empty(),"second hit restores original Water8 after first hit filter/DEF1 reduced only its own payload");roundtrip(colored);colored.step(defend(7));check(colored.snapshot().state.players[1].hp==25,"fresh second8 payload is applied separately");roundtrip(colored);
    auto reflected=fixture(scene({saw},{image("reflection-sword")}));reflected.step(play(0,{0},7));reflected.step(defend(7,{0}));reflected.step(defend(0));check(reflected.snapshot().state.players[0].hp==37&&reflected.snapshot().actor==7&&reflected.snapshot().queue[0].source==0&&reflected.snapshot().queue[0].target==7&&reflected.snapshot().queue[0].power==3&&reflected.snapshot().queue[0].redirects.empty(),"second hit returns to original source/target after first redirected hit resolves");roundtrip(reflected);reflected.step(defend(7));check(reflected.snapshot().state.players[1].hp==37,"each target receives exactly its own final3 body");roundtrip(reflected);
    auto wallstate=scene({saw,image("aura")},{image("wall")});wallstate.players[0].mp=6;wallstate.players[1].mp=12;auto wall=fixture(wallstate);wall.step(play(0,{0,1},7));wall.step(defend(7,{0}));check(wall.snapshot().queue[0].power==6&&wall.snapshot().state.players[0].mp==0&&wall.snapshot().state.players[1].mp==6&&wall.snapshot().state.players[0].items[0].used,"Aura is paid and retained once while each Wall response pays independently");roundtrip(wall);wall.step(defend(7,{0}));check(wall.snapshot().state.players[0].mp==0&&wall.snapshot().state.players[1].mp==0&&wall.snapshot().state.players[0].hp==40&&wall.snapshot().state.players[1].hp==40&&wall.snapshot().state.players[0].items.size()==3&&wall.snapshot().state.players[1].items.size()==3,"two retained Walls cancel separate hits and request two gifts without reusing main MC cost");roundtrip(wall);
    auto mpstate=scene({saw,image("aura"),image("neptune-ring")},{});mpstate.players[0].mp=12;auto mp=fixture(mpstate);mp.step(play(0,{0,1},0));mp.step(defend(0,{2}));check(mp.snapshot().queue[0].is_counter&&mp.snapshot().queue[0].amount==12,"first hit counter FIFO starts before second initial hit");mp.step(defend(0));check(!mp.snapshot().queue[0].is_counter&&mp.snapshot().queue[0].power==6&&mp.snapshot().state.players[0].mp==18,"MP recovery between hits does not change immutable power or pay original Aura twice");roundtrip(mp);mp.step(defend(0));check(mp.snapshot().state.players[0].hp==28&&mp.snapshot().state.players[0].mp==18,"fresh second body follows completed counter with original6 power");roundtrip(mp);
}

void twice_death_cutoff_and_mana_journal(){
    const auto saw=image("saw-boom-boom"),stick=image("magical-stick");auto duel=scene({saw},{});duel.players[1].hp=3;auto natural=fixture(duel);natural.step(play(0,{0},7));natural.step(defend(7));check(natural.snapshot().terminated&&natural.snapshot().decisions==2&&natural.snapshot().state.players[0].items.empty()&&natural.snapshot().next_token==1,"first lethal hit ends duel naturally before second body or gifts");roundtrip(natural);
    auto three=duel;three.players.push_back(player(9));auto skipped=fixture(three);skipped.step(play(0,{0},7));skipped.step(defend(7));check(!skipped.snapshot().terminated&&skipped.snapshot().owner==9&&skipped.snapshot().decisions==2&&skipped.snapshot().state.players[0].items.size()==1&&!skipped.snapshot().batch,"dead target is skipped on second roster entry in ongoing three-player game");roundtrip(skipped);
    auto dying=scene({saw},{image("saturn-ring")});dying.players[0].hp=2;dying.players.push_back(player(9));auto canceled=fixture(dying);canceled.step(play(0,{0},7));canceled.step(defend(7,{0}));roundtrip(canceled);canceled.step(defend(0));check(canceled.snapshot().state.players[0].dead&&canceled.snapshot().state.players[1].hp==37&&!canceled.snapshot().terminated&&canceled.snapshot().owner==7&&!canceled.snapshot().batch,"counter killing original owner cancels unstarted second hit while other survivors continue");roundtrip(canceled);
    MatchConfig limit;limit.max_decisions=2;auto cut=fixture(scene({saw},{}),limit);cut.step(play(0,{0},7));const auto rng=cut.snapshot().rng;cut.step(defend(7));check(cut.snapshot().truncated&&cut.snapshot().state.players[1].hp==37&&cut.snapshot().rng==rng&&cut.snapshot().state.players[0].items.empty()&&!cut.snapshot().batch,"cutoff after first response applies only first body and suppresses second hit/gifts");roundtrip(cut);
    auto pending=fixture(scene({stick,image("aura"),image("crossbow")},{}));pending.step(play(0,{0,1,2},7));const auto exact=pending.snapshot();for(int error=0;error<8;++error){auto bad=exact;if(error==0)++bad.state.players[0].mp;if(error==1)++bad.batch->initial_state.players[0].mp;if(error==2)++bad.queue[0].power;if(error==3)bad.batch->targets.push_back(7);if(error==4)bad.queue[0].models[0]=saw;if(error==5)bad.batch->attack.items.erase(bad.batch->attack.items.begin()+1);if(error==6)bad.batch->initial_state.players[0].mp=5;if(error==7)bad.queue[0].source=7;bool rejected=false;try{pending.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==exact,"replay rejects mana power/payment anchor, duplicate roster and composition/source forgery");}
    auto second=fixture(scene({saw},{}));second.step(play(0,{0},7));second.step(defend(7));const auto saved=second.snapshot();for(int error=0;error<4;++error){auto bad=saved;if(error==0)bad.batch->targets.pop_back();if(error==1)bad.batch->targets[1]=0;if(error==2)--bad.batch->cursor;if(error==3)bad.batch->responses.clear();bool rejected=false;try{second.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&second.snapshot()==saved,"second-hit snapshot requires canonical duplicate roster, cursor and accepted first response");}
    auto late=exact;late.state.update_count=max_wire_integer;late.batch->initial_state.update_count=max_wire_integer-(late.decisions-late.batch->decisions);pending.restore(late);const auto before=pending.snapshot();bool rejected=false;try{pending.step(defend(7));}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==before,"late body failure preserves already-burnt MP, immutable power and pending journal atomically");
    auto launch=scene({stick},{});launch.update_count=max_wire_integer;auto failed=fixture(launch);const auto launch_before=failed.snapshot();rejected=false;try{failed.step(play(0,{0},7));}catch(const MatchError&){rejected=true;}check(rejected&&failed.snapshot()==launch_before,"late launch failure rolls back whole-MP burn and weapon consumption");
}

void wide_target_modes_payment_and_free(){
    const auto wide=image("mirage");SnapshotState state;state.players={player(0,{image("bronze-club"),wide},Team::one),player(7,{},Team::two),player(9,{},Team::one),player(42)};state.players[1].curses={Curse::darkcloud};state.players[3].dead=true;state.players[3].hp=0;
    for(const auto target:{std::optional<PlayerId>{},std::optional<PlayerId>{0},std::optional<PlayerId>{7},std::optional<PlayerId>{9}}){auto m=fixture(state);m.step({0,CommandKind::play,{0,1},target});check(m.snapshot().batch->targets==std::vector<PlayerId>{7}&&!m.snapshot().batch->chance&&m.snapshot().state.players[0].mp==5&&m.snapshot().queue[0].hit_rate==0&&!m.snapshot().queue[0].forced_hit&&m.snapshot().chance_rolls==0&&m.snapshot().chance_forced_hits==0,"Wide accepts no target or any known alive hint, expands only enemies, and guarantees body without percentage/forced draws");roundtrip(m);m.step(defend(7));roundtrip(m);}
    auto bad=fixture(state);reject(bad,play(0,{0,1},999));reject(bad,play(0,{0,1},42));auto alone=fixture(scene({wide},{}));reject(alone,play(0,{0},7));auto percent=fixture(scene({image("spark-bag"),wide,image("spiritual-doll")},{}));reject(percent,chance_play(0,{0,1}));reject(percent,chance_play(0,{0,1,2}));
    auto multi_state=scene({image("bronze-club"),wide,wide},{});multi_state.players.push_back(player(9));auto multi=fixture(multi_state);multi.step(chance_play(0,{0,1,2}));check(multi.snapshot().batch->targets==std::vector<PlayerId>{7,9}&&multi.snapshot().state.players[0].mp==0&&multi.snapshot().state.players[0].items.size()==2&&std::all_of(multi.snapshot().state.players[0].items.begin(),multi.snapshot().state.players[0].items.end(),[](auto& i){return i.used;})&&multi.snapshot().gifts[0]==3,"multiple Wide copies retain/pay independently while expansion is idempotent");roundtrip(multi);multi.step(defend(7));multi.step(defend(9));check(multi.snapshot().state.players[0].items.size()==5&&multi.snapshot().state.players[1].hp==39&&multi.snapshot().state.players[2].hp==39,"Wide gifts occur once for original uses, not per target");roundtrip(multi);
    auto free_state=scene({image("bronze-club"),wide,image("spiritual-doll")},{});free_state.players[0].mp=0;auto free=fixture(free_state);const auto before=free.snapshot();bool reached=false;for(std::uint64_t seed=0;seed<256;++seed){auto policy=seed;auto c=free.sample_action(policy);check(free.legal(c)&&free.snapshot()==before,"Wide lookahead sampler remains legal and environment immutable");if(c.kind==CommandKind::play&&std::find(c.items.begin(),c.items.end(),ItemToken{1})!=c.items.end()){check(!c.target,"sampler canonicalizes Wide targetless even when appended through FREE atomic pair");reached=true;}}check(reached,"zero-MP weapon plus Wide/FREE is reachable by sampler");free.step(chance_play(0,{0,1,2}));check(free.snapshot().queue[0].power==1&&free.snapshot().state.players[0].mp==0,"completed Wide/FREE column waives five-MP cost");roundtrip(free);
    for(bool discounted:{false,true}){auto mana_state=scene({image("magical-stick"),wide,image("spiritual-doll")},{});mana_state.players.push_back(player(9));auto mana=fixture(mana_state);mana.step(discounted?chance_play(0,{0,1,2}):chance_play(0,{0,1}));const int power=discounted?20:10;check(mana.snapshot().queue[0].power==power&&mana.snapshot().state.players[0].mp==0,"58 Wide uses original MP minus final composed cost before burning all MP once");mana.step(defend(7));check(mana.snapshot().queue[0].power==power&&mana.snapshot().state.players[0].mp==0,"next Wide target retains immutable mana payload despite post-launch MP zero");roundtrip(mana);mana.step(defend(9));roundtrip(mana);}
}

void wide_twice_sixteen_fresh_frames(){
    SnapshotState state;for(PlayerId id=0;id<9;++id){auto p=player(id,id==0?std::initializer_list<CardId>{image("saw-boom-boom"),image("mirage"),image("wand-of-mystic-water")}:id==1?std::initializer_list<CardId>{image("rainbow-curtain")}:std::initializer_list<CardId>{});p.hp=99;if(id)p.curses={Curse::darkcloud};state.players.push_back(std::move(p));}Match m;m.load_scene(state,0,55);m.step(chance_play(0,{0,1,2}));std::vector<PlayerId> roster;for(int pass=0;pass<2;++pass)for(PlayerId id=1;id<9;++id)roster.push_back(id);check(m.snapshot().batch->targets==roster&&roster.size()==16&&m.snapshot().gifts[0]==3,"14 plus Wide expands two complete input-order enemy passes rather than doubling each adjacent target");roundtrip(m);
    for(std::size_t i=0;i<roster.size();++i){check(m.snapshot().actor==roster[i]&&m.snapshot().queue[0].source==0&&m.snapshot().queue[0].target==roster[i]&&m.snapshot().queue[0].power==8&&m.snapshot().queue[0].element==Element::water&&m.snapshot().queue[0].redirects.empty()&&m.snapshot().rng==55,"all sixteen frames restore original source, setter color, power and no-draw Wide semantics");m.step(i==0?defend(1,{0}):defend(roster[i]));roundtrip(m);if(i<15)check(m.snapshot().state.players[0].items.size()==1&&m.snapshot().next_token==3,"no per-target replenishment occurs before whole action ends");}
    check(!m.snapshot().batch&&m.snapshot().owner==1&&m.snapshot().state.players[0].items.size()==4&&m.snapshot().state.players[1].items.size()==1&&m.snapshot().chance_rolls==0&&m.snapshot().chance_forced_hits==0,"sixteen completed frames use one original cost/holding/gift phase and no DarkCloud force metrics");for(std::size_t i=1;i<9;++i)check(m.snapshot().state.players[i].hp==83,"every enemy receives two separate original eight-power bodies");roundtrip(m);
}

void wide_counter_death_and_journal(){
    const auto saw=image("saw-boom-boom"),wide=image("mirage");auto state=scene({saw,wide},{image("saturn-ring")});state.players.push_back(player(9));auto fifo=fixture(state);fifo.step(chance_play(0,{0,1}));fifo.step(defend(7,{0}));check(fifo.snapshot().queue[0].is_counter&&fifo.snapshot().actor==0&&fifo.snapshot().batch->cursor==1,"counter FIFO starts before next initial Wide target");roundtrip(fifo);fifo.step(defend(0));check(fifo.snapshot().actor==9&&!fifo.snapshot().queue[0].is_counter&&fifo.snapshot().queue[0].power==3,"next enemy starts only after counter is resolved");roundtrip(fifo);fifo.step(defend(9));fifo.step(defend(7));fifo.step(defend(9));check(fifo.snapshot().state.players[0].hp==34&&fifo.snapshot().state.players[1].hp==34&&fifo.snapshot().state.players[2].hp==34&&fifo.snapshot().owner==7,"two Wide passes preserve independent body/counter damage and original turn order");roundtrip(fifo);
    auto dying=state;dying.players[0].hp=2;auto cancel=fixture(dying);cancel.step(chance_play(0,{0,1}));cancel.step(defend(7,{0}));cancel.step(defend(0));check(cancel.snapshot().state.players[0].dead&&cancel.snapshot().state.players[1].hp==37&&cancel.snapshot().state.players[2].hp==40&&!cancel.snapshot().batch&&cancel.snapshot().owner==7,"counter death of origin cancels all unstarted Wide targets and second pass");roundtrip(cancel);
    auto killed=scene({saw,wide},{});killed.players[1].hp=3;killed.players.push_back(player(9));auto skip=fixture(killed);skip.step(chance_play(0,{0,1}));skip.step(defend(7));check(skip.snapshot().actor==9&&skip.snapshot().state.players[1].dead,"first-pass killed enemy is skipped later without extra response");skip.step(defend(9));check(skip.snapshot().actor==9&&skip.snapshot().batch->cursor==4,"second pass skips dead first enemy and reaches remaining enemy");skip.step(defend(9));check(skip.snapshot().state.players[2].hp==34&&!skip.snapshot().batch&&skip.snapshot().owner==9,"only living second-pass enemy receives second body");roundtrip(skip);
    MatchConfig limit;limit.max_decisions=2;auto cut=fixture(state,limit);cut.step(chance_play(0,{0,1}));const auto rng=cut.snapshot().rng;cut.step(defend(7));check(cut.snapshot().truncated&&!cut.snapshot().batch&&cut.snapshot().state.players[1].hp==37&&cut.snapshot().state.players[2].hp==40&&cut.snapshot().rng==rng,"command cutoff cancels unstarted expanded roster and gifts");roundtrip(cut);
    auto pending=fixture(state);pending.step(chance_play(0,{0,1}));const auto exact=pending.snapshot();for(int error=0;error<6;++error){auto bad=exact;if(error==0)bad.batch->targets.pop_back();if(error==1)std::swap(bad.batch->targets[0],bad.batch->targets[1]);if(error==2)bad.batch->attack.target=999;if(error==3)++bad.queue[0].power;if(error==4)++bad.batch->cursor;if(error==5)++bad.state.players[0].mp;bool rejected=false;try{pending.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==exact,"journal verifies expanded pass count, seat order, target hint, cursor, immutable payload and payment");}
    auto late=exact;late.state.update_count=max_wire_integer;late.batch->initial_state.update_count=max_wire_integer-(late.decisions-late.batch->decisions);pending.restore(late);const auto before=pending.snapshot();bool rejected=false;try{pending.step(defend(7));}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==before,"late Wide response failure restores current body, roster, journal, costs and RNG");
}

void random_hp_sign_clips_and_death(){
    const auto tear=image("thump-thump-tear");MatchConfig limit;limit.max_decisions=2;for(const auto trial:{std::pair<std::uint64_t,int>{0,-10},{2,10}}){Match m(limit);m.load_scene(scene({tear},{}),0,trial.first);m.step(play(0,{0},7));check(m.snapshot().queue[0].amount==trial.second&&m.snapshot().batch->random_hp_delta==trial.second&&m.snapshot().queue[0].power==0&&m.snapshot().queue[0].category==Category::sundries&&m.snapshot().rng==trial.first+0x9e3779b97f4a7c15ULL,"random HP launch maps zero bit to plus10 and one bit to minus10 with one signed nonattack payload draw");roundtrip(m);m.step(defend(7));check(m.snapshot().state.players[1].hp==40+trial.second&&m.snapshot().state.players[0].hp==40&&m.snapshot().state.players[0].mp==10&&m.snapshot().counter_created==0&&m.snapshot().truncated&&m.snapshot().rng==trial.first+0x9e3779b97f4a7c15ULL,"signed HP body applies once without defense/counters and cutoff suppresses gift RNG");roundtrip(m);}
    MatchConfig one;one.max_decisions=1;auto cap=scene({tear},{});cap.players[0].hp=95;Match healing(one);healing.load_scene(cap,0,2);healing.step(play(0,{0},0));check(healing.snapshot().state.players[0].hp==99&&healing.snapshot().truncated&&healing.snapshot().rng==std::uint64_t{2}+0x9e3779b97f4a7c15ULL,"self immediate plus10 caps99 and still consumes only one launch draw before cutoff");roundtrip(healing);
    for(bool self:{false,true}){auto state=scene({tear},{});state.players[self?0:1].hp=5;MatchConfig death_limit;death_limit.max_decisions=self?1:2;Match dead(death_limit);dead.load_scene(state,0,0);dead.step(play(0,{0},self?0:7));if(!self)dead.step(defend(7));check(dead.snapshot().terminated&&!dead.snapshot().truncated&&dead.snapshot().state.players[self?0:1].hp==0&&dead.snapshot().winners==std::vector<PlayerId>{self?PlayerId{7}:PlayerId{0}}&&dead.snapshot().gifts==std::vector<std::uint32_t>{0,0},"negative HP clip kills correct current recipient with fixed winners and natural outcome before cutoff");roundtrip(dead);}
}

void random_hp_reflections_fixed_and_journal(){
    const auto tear=image("thump-thump-tear");auto state=scene({tear,image("super-mirror")},{image("super-mirror"),image("aqua-shoes"),image("rainbow-curtain"),image("saturn-ring"),image("wall"),image("turbulence"),image("moonlight-axe")});Match m;m.load_scene(state,0,0);m.step(play(0,{0},7));const auto rng=m.snapshot().rng;for(ItemToken token:{1u,2u,3u,4u,5u,6u})reject(m,defend(7,{token}));m.step(defend(7,{0}));roundtrip(m);m.step(defend(0,{1}));check(m.snapshot().queue[0].amount==-10&&m.snapshot().batch->random_hp_delta==-10&&m.snapshot().rng==rng&&m.snapshot().queue[0].target==7&&m.snapshot().queue[0].redirects.size()==2&&m.snapshot().state.players[0].hp==40&&m.snapshot().state.players[1].hp==40,"two universal reflections preserve original sign and never redraw or apply HP before final pass");roundtrip(m);
    check(m.observe(0).find(random_hp_rule)!=std::string::npos&&m.observe(0).find("\"amount\":-10")!=std::string::npos,"signed result and explicit random HP contract are public while launch anchor stays privileged");const auto exact=m.snapshot();for(int error=0;error<9;++error){auto bad=exact;if(error==0)bad.batch->random_hp_delta=10;if(error==1)bad.queue[0].amount=10;if(error==2)bad.batch->rng^=1;if(error==3)bad.rng^=1;if(error==4)bad.queue[0].power=10;if(error==5)bad.queue[0].ability=Ability::boost_hp;if(error==6)bad.batch->responses.clear();if(error==7)++bad.batch->initial_state.players[0].hp;if(error==8)bad.queue[0].tokens[0]=1;bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"journal recomputes random sign and rejects signed payload, RNG, type and history forgery atomically");}
    m.step(defend(7));check(m.snapshot().state.players[1].hp==30&&m.snapshot().state.players[0].hp==40&&m.snapshot().counter_created==0&&!m.snapshot().batch,"reflected negative nonattack body applies original minus10 once with no counter");roundtrip(m);
}

void random_hp_query_cutoff_and_rollback(){
    const auto tear=image("thump-thump-tear");auto state=scene({tear,image("aqua-shoes")},{});Match m;m.load_scene(state,0,0);const auto initial=m.snapshot();for(int i=0;i<16;++i)check(m.legal(play(0,{0},7))&&m.snapshot()==initial,"legal random HP queries do not sample future RNG");reject(m,play(0,{0},999));reject(m,play(0,{0,0},7));reject(m,play(0,{0,1},7));
    MatchConfig one;one.max_decisions=1;Match cut(one);cut.load_scene(state,0,0);cut.step(play(0,{0},7));check(cut.snapshot().truncated&&!cut.snapshot().batch&&cut.snapshot().queue.empty()&&cut.snapshot().state.players[1].hp==40&&cut.snapshot().rng==0x9e3779b97f4a7c15ULL&&cut.snapshot().gifts==std::vector<std::uint32_t>{0,0},"foreign launch cutoff preserves one sign draw while canceling unresponded HP body and gifts");roundtrip(cut);
    auto failed_state=state;failed_state.update_count=max_wire_integer;Match failed;failed.load_scene(failed_state,0,0);const auto before=failed.snapshot();bool rejected=false;try{failed.step(play(0,{0},7));}catch(const MatchError&){rejected=true;}check(rejected&&failed.snapshot()==before,"late launch counter failure rolls back RNG sign, consumption, journal and gift request");
    m.step(play(0,{0},7));auto late=m.snapshot();late.state.update_count=max_wire_integer;late.batch->initial_state.update_count=max_wire_integer-(late.decisions-late.batch->decisions);m.restore(late);const auto pending=m.snapshot();rejected=false;try{m.step(defend(7));}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==pending,"late body failure rolls back HP change and accepted response while preserving previously committed random sign");
    auto exhausted=scene({tear},{});exhausted.players[0].items[0].token=max_wire_integer;Match tokens;tokens.load_scene(exhausted,0,0);tokens.step(play(0,{max_wire_integer},7));const auto token_before=tokens.snapshot();rejected=false;try{tokens.step(defend(7));}catch(const MatchError&){rejected=true;}check(rejected&&tokens.snapshot()==token_before,"late gift exhaustion restores random HP body, pending sign, counters and environment RNG");
}

void revival_passive_boundaries_and_prayer(){
    const auto amulet=image("sun-amulet");
    for(const auto attack:{image("bronze-club"),image("pri-pri-pricker"),image("thump-thump-tear")}){
        auto state=scene({attack},{amulet});state.players[1].hp=attack==image("pri-pri-pricker")?40:1;
        state.players[1].curses={Curse::flash,Curse::darkcloud};Match m;m.load_scene(state,0,0);
        m.step(play(0,{0},7));reject(m,defend(7,{0}));roundtrip(m);m.step(defend(7));
        check(m.snapshot().revivals==1&&!m.snapshot().state.players[1].dead&&m.snapshot().state.players[1].hp==10
            &&m.snapshot().state.players[1].curses==state.players[1].curses&&m.snapshot().state.players[1].items.size()==1
            &&m.snapshot().state.players[1].items[0].token!=0&&m.snapshot().owner==7&&!m.done(),
            "normal, Dark and negative HP bodies consume passive amulet, preserve curses and replace it only after the action");roundtrip(m);
    }
    auto full=scene();for(ItemToken i=0;i<18;++i)full.players[0].items.push_back({i,amulet,{},false});auto prayer=fixture(full);
    reject(prayer,play(0,{0},0));reject(prayer,{0,CommandKind::discard,{0},{}});const auto before=prayer.snapshot();
    for(std::uint64_t seed=0;seed<16;++seed){auto rng=seed;const auto command=prayer.sample_action(rng);
        check(command==pray(0)&&prayer.legal(command)&&prayer.snapshot()==before,"eighteen passive amulets still have exactly one legal prayer proposal");}
    prayer.step(pray(0));check(prayer.snapshot().state.players[0].items==before.state.players[0].items&&prayer.snapshot().rng==before.rng
        &&prayer.snapshot().next_token==before.next_token&&prayer.snapshot().owner==7&&prayer.snapshot().revivals==0,
        "full nonweapon prayer advances owner without consuming amulet, allocating gifts or drawing environmental RNG");roundtrip(prayer);
    auto dead=scene({image("bronze-club")},{amulet});dead.players[1].hp=0;dead.players[1].dead=true;dead.players.push_back(player(9));auto untouched=fixture(dead);
    untouched.step(play(0,{0},9));untouched.step(defend(9));check(untouched.snapshot().revivals==0&&untouched.snapshot().state.players[1].dead
        &&untouched.snapshot().state.players[1].items==dead.players[1].items,"already-dead holder is never revived by a later body's death scan");roundtrip(untouched);
}

void revival_counters_repeats_and_simultaneous(){
    const auto amulet=image("sun-amulet");auto state=scene({image("evil-broadsword")},{amulet,image("saturn-ring")});state.players[1].hp=5;
    auto counter=fixture(state);counter.step(play(0,{0},7));counter.step(defend(7,{1}));
    check(counter.snapshot().revivals==1&&counter.snapshot().state.players[1].hp==10&&!counter.snapshot().state.players[1].dead
        &&counter.snapshot().queue[0].is_counter&&counter.snapshot().queue[0].power==10&&counter.snapshot().actor==0
        &&counter.snapshot().gifts==std::vector<std::uint32_t>{1,2}&&counter.snapshot().state.players[1].items.empty(),
        "revived survivor counters from pre-revival actual loss five while amulet and ring gifts remain deferred");roundtrip(counter);
    counter.step(defend(0));check(counter.snapshot().state.players[0].hp==16&&counter.snapshot().counter_created==1
        &&counter.snapshot().state.players[1].items.size()==2,"source self-damage, survivor counter and deferred revival gift each apply once");roundtrip(counter);
    auto repeated=scene({image("saw-boom-boom"),image("strength-powder")},{amulet,amulet});repeated.players[1].hp=1;auto twice=fixture(repeated);
    twice.step(play(0,{0,1},7));const auto rng=twice.snapshot().rng;twice.step(defend(7));
    check(twice.snapshot().revivals==1&&twice.snapshot().state.players[1].hp==10&&twice.snapshot().state.players[1].items.size()==1
        &&twice.snapshot().state.players[1].items[0].token==1&&twice.snapshot().gifts[1]==1&&twice.snapshot().rng==rng,
        "first lethal repeat hit consumes only the first copy with one gift reserved and no revival RNG");roundtrip(twice);
    twice.step(defend(7));check(twice.snapshot().revivals==2&&twice.snapshot().state.players[1].hp==10&&!twice.done()
        &&twice.snapshot().state.players[1].items.size()==2&&twice.snapshot().owner==7,"second lethal hit can consume the next amulet before the single whole-action gift phase");roundtrip(twice);
    for(unsigned mask=0;mask<4;++mask)for(unsigned owner=0;owner<2;++owner){SnapshotState both;both.players={player(0),player(7)};
        for(unsigned seat=0;seat<2;++seat){both.players[seat].hp=5;if(mask&(1u<<seat))both.players[seat].items.push_back({0,amulet,{},false});}
        both.players[owner].items.push_back({10,image("evil-broadsword"),{},false});Match m;m.load_scene(both,both.players[owner].id,77);
        m.step(play(both.players[owner].id,{10},both.players[1-owner].id));m.step(defend(both.players[1-owner].id));
        const bool first=(mask&1)!=0,second=first&&(mask&2)!=0;
        check(m.snapshot().state.players[0].dead==!first&&m.snapshot().state.players[1].dead==!second
            &&m.snapshot().revivals==static_cast<unsigned>(first)+static_cast<unsigned>(second),
            "simultaneous HP0 uses input-seat order and current not-dead count independently of the original owner's seat");
        if(mask==2)check(m.snapshot().state.players[1].items.size()==1,"last not-dead holder keeps its unusable amulet rather than reviving alone");roundtrip(m);
    }
}

void revival_journal_cutoff_and_rollback(){
    const auto amulet=image("sun-amulet");auto state=scene({image("saw-boom-boom"),image("strength-powder")},{amulet,amulet});state.players[1].hp=1;
    auto m=fixture(state);m.step(play(0,{0,1},7));m.step(defend(7));const auto exact=m.snapshot();
    check(m.observe(0).find(revival_rule)!=std::string::npos&&m.observe(0).find("\"revivals\"")==std::string::npos,"revival contract is public while native consumption counter stays privileged");
    for(int error=0;error<9;++error){auto bad=exact;if(error==0)bad.revivals=0;if(error==1)bad.batch->revivals=1;
        if(error==2)bad.state.players[1].hp=0;if(error==3)bad.state.players[1].items.push_back({0,amulet,{},false});
        if(error==4)bad.gifts[1]=0;if(error==5)bad.rng^=1;if(error==6)bad.batch->initial_state.players[1].items.erase(bad.batch->initial_state.players[1].items.begin());
        if(error==7)++bad.queue[0].power;if(error==8)bad.batch->responses.clear();bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}
        check(rejected&&m.snapshot()==exact,"whole-action replay rejects forged revival count, anchor, HP, consumed copy, gift reservation and response history");}
    MatchConfig limit;limit.max_decisions=2;auto cut=fixture(state,limit);cut.step(play(0,{0,1},7));const auto rng=cut.snapshot().rng;cut.step(defend(7));
    check(cut.snapshot().truncated&&!cut.snapshot().terminated&&cut.snapshot().revivals==1&&cut.snapshot().state.players[1].hp==10
        &&cut.snapshot().state.players[1].items.size()==1&&cut.snapshot().gifts==std::vector<std::uint32_t>{0,0}&&cut.snapshot().rng==rng,
        "cutoff preserves committed revival and consumption while canceling the next hit and all deferred gifts");roundtrip(cut);
    auto late=exact;late.state.update_count=max_wire_integer;late.batch->initial_state.update_count=max_wire_integer-(late.decisions-late.batch->decisions);m.restore(late);
    const auto before=m.snapshot();bool rejected=false;try{m.step(defend(7));}catch(const MatchError&){rejected=true;}
    check(rejected&&m.snapshot()==before,"late update failure rolls back the second revival, copy consumption, gifts, HP and counters");
    state.players[0].items[0].token=max_wire_integer;state.players[0].items[1].token=max_wire_integer-1;auto exhausted=fixture(state);
    exhausted.step(play(0,{max_wire_integer,max_wire_integer-1},7));exhausted.step(defend(7));const auto pending=exhausted.snapshot();rejected=false;
    try{exhausted.step(defend(7));}catch(const MatchError&){rejected=true;}
    check(rejected&&exhausted.snapshot()==pending,"late gift token exhaustion rolls back the final body and auto-consumption exactly");
}

void danger_random_targets_reflections_and_visibility(){
    const auto pestle=image("dangerous-pestle"),mirror=image("super-mirror");SnapshotState state;
    for(auto id:{90u,2u,700u,17u,4u,99u,55u,3u,8u})state.players.push_back(player(id,{},id==90||id==2?Team::one:Team::solo));
    state.players[0].items={{0,pestle,{},false},{1,mirror,{},false}};state.players[7].items={{0,mirror,{},false}};
    Match m,hint;m.load_scene(state,90,0);hint.load_scene(state,90,0);m.step({90,CommandKind::play,{0},{}});hint.step(play(90,{0},90));
    check(m.snapshot().actor==3&&m.snapshot().last_danger->target==3&&!m.snapshot().last_danger->mortar
        &&m.snapshot().queue[0].power==30&&m.snapshot().queue[0].element==Element::light
        &&m.snapshot().rng==0x9e3779b97f4a7c15ULL&&m.observe({})==hint.observe({}),"danger ignores alive hints and takes one bounded draw from all input-order alive seats");roundtrip(m);
    const auto rng=m.snapshot().rng;m.step(defend(3,{0}));roundtrip(m);m.step(defend(90,{1}));roundtrip(m);
    check(m.snapshot().rng==rng&&m.snapshot().last_danger->target==3&&m.snapshot().queue[0].target==3
        &&m.snapshot().queue[0].power==30&&m.snapshot().queue[0].redirects.size()==2,"reflections preserve danger payload and initial public result without reselection");
    m.step(defend(3));check(m.snapshot().state.players[7].hp==10&&m.snapshot().owner==2&&m.snapshot().last_danger.has_value(),"danger result remains after action completion");roundtrip(m);
    m.step(pray(2));check(!m.snapshot().last_danger,"next accepted main action clears the public danger result");roundtrip(m);
    std::set<PlayerId> seen;for(std::uint64_t seed=0;seed<128;++seed){Match a;a.load_scene(state,90,seed);a.step({90,CommandKind::play,{0},{}});seen.insert(*a.snapshot().actor);roundtrip(a);}
    check(seen.size()==9&&seen.contains(90)&&seen.contains(2),"uniform danger proposals include the caster, teammates and all other live seats");
}

void danger_mortar_priority_composition_and_revival(){
    const auto pestle=image("dangerous-pestle"),mortar=image("dangerous-mortar"),amulet=image("sun-amulet");
    SnapshotState self;self.players={player(0,{pestle,image("aura"),image("spiritual-doll"),mortar,amulet}),player(7,{mortar}),player(9,{mortar})};self.players[0].mp=0;
    MatchConfig cap;cap.max_decisions=1;auto m=fixture(self,cap);const auto rng=m.snapshot().rng;m.step(play(0,{0,1,2},9));
    check(m.snapshot().truncated&&m.snapshot().last_danger->target==0&&m.snapshot().last_danger->mortar
        &&m.snapshot().last_danger->power==198&&!m.snapshot().last_danger->element&&m.snapshot().rng==rng
        &&m.snapshot().state.players[0].hp==10&&m.snapshot().revivals==1&&m.snapshot().state.players[0].items.size()==1
        &&m.snapshot().state.players[1].items==self.players[1].items&&m.snapshot().state.players[2].items==self.players[2].items,
        "first mortar can belong to the caster; base99 doubles before an immediate body, revival and cutoff without target RNG");roundtrip(m);
    SnapshotState state;state.players={player(0,{pestle,image("crossbow"),image("aura"),image("spiritual-doll")}),player(7,{mortar,mortar,amulet}),player(9,{mortar})};state.players[0].mp=0;
    auto boosted=fixture(state,cap);boosted.step({0,CommandKind::play,{0,1,2,3},{}});
    check(boosted.snapshot().last_danger->target==7&&boosted.snapshot().last_danger->power==202
        &&boosted.snapshot().state.players[1].hp==10&&boosted.snapshot().state.players[1].items.size()==1
        &&boosted.snapshot().state.players[1].items[0].token==1&&boosted.snapshot().counter_created==0,
        "first holder consumes just its first mortar; ordered boost and Aura operate on99 and the passive hit has no defense counter");roundtrip(boosted);
    auto normal=fixture(state);normal.step(play(0,{0},7));check(normal.snapshot().last_danger->power==99&&!normal.snapshot().batch
        &&normal.snapshot().state.players[0].items.size()==4&&normal.snapshot().state.players[1].items.size()==3
        &&normal.snapshot().owner==7,"mortar and amulet reserve separate replacements until the whole immediate action ends");roundtrip(normal);
    state.players[1].dead=true;state.players[1].hp=0;auto skip=fixture(state,cap);skip.step({0,CommandKind::play,{0},{}});
    check(skip.snapshot().last_danger->target==9&&skip.snapshot().last_danger->power==99&&skip.snapshot().state.players[1].items==state.players[1].items,"dead mortar holder never takes priority or loses a copy");roundtrip(skip);
}

void danger_selection_gap_journal_and_atomicity(){
    const auto pestle=image("dangerous-pestle"),mortar=image("dangerous-mortar");auto state=scene({pestle,image("super-mirror"),image("mirage")},{image("super-mirror")});
    Match m;m.load_scene(state,0,0);const auto initial=m.snapshot();reject(m,play(0,{0},999));reject(m,{0,CommandKind::discard,{0},{}});
    auto invented=initial;invented.last_danger=DangerEvent{0,7,{pestle},false,30,Element::light};bool initial_rejected=false;
    try{m.restore(invented);}catch(const MatchError&){initial_rejected=true;}check(initial_rejected&&m.snapshot()==initial,"an unplayed initial scene cannot invent a danger launch result");
    bool unsupported_gap=false;try{m.step({0,CommandKind::play,{0,2},{}});}catch(const MatchError& e){unsupported_gap=e.code==MatchErrorCode::unsupported;}
    check(unsupported_gap&&m.snapshot()==initial,"client-valid danger plus Wide is explicitly unsupported, never silently prioritized");
    for(std::uint64_t seed=0;seed<64;++seed){auto policy=seed;const auto command=m.sample_action(policy);check(m.legal(command)&&m.snapshot()==initial,"danger sampler only proposes supported operations without environmental draws");if(command.kind==CommandKind::play)check(!command.target&&command.items.size()==1,"danger sampler canonicalizes targetless and omits Wide");}
    m.step({0,CommandKind::play,{0},{}});m.step(defend(7,{0}));const auto exact=m.snapshot();
    for(int error=0;error<9;++error){auto bad=exact;if(error==0)++bad.last_danger->power;if(error==1)bad.last_danger->target=0;
        if(error==2)bad.last_danger->mortar=true;if(error==3)bad.last_danger->models.push_back(image("mirage"));if(error==4)bad.last_danger->origin=7;
        if(error==5)bad.batch->rng^=1;if(error==6)++bad.queue[0].power;if(error==7)bad.batch->targets[0]=0;
        if(error==8){bad.batch->initial_state.players[0].items.push_back({3,mortar,{},false});bad.batch->next_token=4;}
        bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"danger journal verifies random anchor, holder history, payload, original target and public event atomically");}
    auto passive=fixture(scene({mortar},{}));reject(passive,play(0,{0},0));reject(passive,{0,CommandKind::discard,{0},{}});check(passive.legal(pray(0)),"mortar is passive and undiscardable but does not prevent prayer");
    auto late=state;late.update_count=max_wire_integer;Match failed;failed.load_scene(late,0,0);const auto before=failed.snapshot();bool rejected=false;
    try{failed.step({0,CommandKind::play,{0},{}});}catch(const MatchError&){rejected=true;}check(rejected&&failed.snapshot()==before,"late launch failure restores target RNG, event and selected items");
    auto exhausted=scene({pestle},{mortar,image("sun-amulet")});exhausted.players[0].items[0].token=max_wire_integer;auto tokens=fixture(exhausted);const auto original=tokens.snapshot();rejected=false;
    try{tokens.step(play(0,{max_wire_integer},7));}catch(const MatchError&){rejected=true;}check(rejected&&tokens.snapshot()==original,"late gift failure rolls back attracted body, both passives, event and revival counter");
}

void exchange_allocation_fee_revival_and_noop(){
    auto state=scene({3,image("sun-amulet"),image("spring"),image("spiritual-doll")},{});state.players[0].curses={Curse::flash,Curse::darkcloud};
    Command command{0,CommandKind::exchange,{0},{},ResourceAllocation{50,0,20}};auto m=fixture(state);m.step(command);
    check(m.snapshot().state.players[0].hp==50&&m.snapshot().state.players[0].mp==0&&m.snapshot().state.players[0].cp==20
        &&m.snapshot().state.players[0].items.size()==4&&m.snapshot().state.players[0].curses==state.players[0].curses
        &&m.snapshot().owner==7&&m.snapshot().queue.empty()&&!m.snapshot().batch&&m.snapshot().counter_created==0,
        "exchange directly reallocates sum70 without price or MP fee, response, retention or curse changes");roundtrip(m);
    auto revived=fixture(state);command.allocation=ResourceAllocation{0,70,0};revived.step(command);
    check(revived.snapshot().state.players[0].hp==10&&revived.snapshot().state.players[0].mp==70&&revived.snapshot().state.players[0].cp==0
        &&revived.snapshot().revivals==1&&revived.snapshot().state.players[0].items.size()==4&&!revived.done(),
        "HP0 allocation runs the normal passive revival hook after sum conservation, reserving separate card and amulet gifts");roundtrip(revived);
    MatchConfig limit;limit.max_decisions=1;auto dying=fixture(scene({3},{}),limit);const auto rng=dying.snapshot().rng;dying.step(command);
    check(dying.snapshot().terminated&&!dying.snapshot().truncated&&dying.snapshot().winners==std::vector<PlayerId>{7}
        &&dying.snapshot().state.players[0].mp==70&&dying.snapshot().rng==rng&&dying.snapshot().state.players[0].items.empty(),
        "HP0 without revival is a natural loss before cutoff, with no debt cascade or terminal replacement");roundtrip(dying);
    auto noop=fixture(scene({3},{}));command.allocation=ResourceAllocation{40,10,20};noop.step(command);
    check(noop.snapshot().decisions==1&&noop.snapshot().owner==7&&noop.snapshot().state.players[0].items.size()==1
        &&noop.snapshot().state.players[0].items[0].token!=0&&noop.snapshot().state.players[0].hp==40,
        "same-value exchange still consumes its physical card, receives one replacement and advances the turn");roundtrip(noop);
}

void exchange_invalid_roles_and_policy_allocations(){
    auto state=scene({3,image("spring"),image("spiritual-doll")},{});auto m=fixture(state);const auto before=m.snapshot();
    const Command valid{0,CommandKind::exchange,{0},{},ResourceAllocation{30,30,10}};
    for(int error=0;error<10;++error){auto bad=valid;if(error==0)bad.allocation.reset();if(error==1)bad.target=0;
        if(error==2)bad.items={1};if(error==3)bad.items={0,2};if(error==4)bad.items={0,0};if(error==5)bad.items.clear();
        if(error==6)bad.allocation=ResourceAllocation{-1,51,20};if(error==7)bad.allocation=ResourceAllocation{100,0,0};
        if(error==8)bad.allocation=ResourceAllocation{30,30,11};if(error==9)bad.items={999};reject(m,bad);}
    reject(m,play(0,{0},0));auto unrelated=pray(0);unrelated.allocation=ResourceAllocation{40,10,20};reject(m,unrelated);
    for(std::uint64_t seed=0;seed<128;++seed){auto policy=seed;const auto command=m.sample_action(policy);check(m.legal(command)&&m.snapshot()==before,"exchange proposals never consume environmental RNG or hidden state");
        if(command.kind==CommandKind::exchange){const auto& a=*command.allocation;check(!command.target&&command.items==std::vector<ItemToken>{0}
            &&a.hp>=0&&a.hp<=99&&a.mp>=0&&a.mp<=99&&a.cp>=0&&a.cp<=99&&a.hp+a.mp+a.cp==70,"sampled exchange is a feasible allocation for the current sum");}
        else check(!command.allocation,"ordinary sampled commands have no allocation payload");}
    for(const auto values:{ResourceAllocation{99,99,99},ResourceAllocation{1,0,0}}){auto bounded=scene({3},{});bounded.players[0].hp=values.hp;bounded.players[0].mp=values.mp;bounded.players[0].cp=values.cp;auto a=fixture(bounded);bool found=false;
        for(std::uint64_t seed=0;seed<64;++seed){auto policy=seed;const auto command=a.sample_action(policy);check(a.legal(command),"boundary resource sums produce valid sampled commands");if(command.kind==CommandKind::exchange){found=true;const auto& r=*command.allocation;check(r.hp+r.mp+r.cp==values.hp+values.mp+values.cp,"policy conserves both minimum and saturated resource sums");}}
        check(found,"exchange is reachable from the sampler even for extreme sums");}
    auto response=fixture(scene({image("bronze-club")},{3}));response.step(play(0,{0},7));auto bad=valid;bad.actor=7;reject(response,bad);
}

void exchange_cutoff_and_atomic_failures(){
    const Command command{0,CommandKind::exchange,{0},{},ResourceAllocation{10,60,0}};MatchConfig limit;limit.max_decisions=1;auto cut=fixture(scene({3},{}),limit);const auto rng=cut.snapshot().rng;cut.step(command);
    check(cut.snapshot().truncated&&!cut.snapshot().terminated&&cut.snapshot().state.players[0].hp==10&&cut.snapshot().state.players[0].mp==60
        &&cut.snapshot().state.players[0].items.empty()&&cut.snapshot().gifts==std::vector<std::uint32_t>{0,0}&&cut.snapshot().rng==rng,
        "exchange commits allocation and physical consumption before cutoff cancels the deferred gift");roundtrip(cut);
    auto late=scene({3},{});late.update_count=max_wire_integer;auto failed=fixture(late);const auto before=failed.snapshot();bool rejected=false;
    try{failed.step(command);}catch(const MatchError&){rejected=true;}check(rejected&&failed.snapshot()==before,"late exchange update failure restores all three resources, card and gift reservation");
    auto exhausted=scene({3,image("sun-amulet")},{});exhausted.players[0].items[0].token=max_wire_integer;auto tokens=fixture(exhausted);const auto original=tokens.snapshot();auto zero=command;zero.items={max_wire_integer};zero.allocation=ResourceAllocation{0,70,0};rejected=false;
    try{tokens.step(zero);}catch(const MatchError&){rejected=true;}check(rejected&&tokens.snapshot()==original,"gift failure rolls back allocation, exchange card, automatic amulet consumption and revival counter");
    auto pending=fixture(scene({image("bronze-club")},{image("super-mirror")}));pending.step(play(0,{0},7));pending.step(defend(7,{0}));const auto exact=pending.snapshot();
    for(bool response:{false,true}){auto bad=exact;if(response)bad.batch->responses[0].allocation=ResourceAllocation{40,10,20};else bad.batch->attack.allocation=ResourceAllocation{40,10,20};rejected=false;
        try{pending.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==exact,"non-exchange journal commands cannot smuggle resource allocations");}
}

void removal_event_public_order_duplicates_and_reflection(){
    MatchConfig cap;cap.max_decisions=2;auto state=scene({205},{6,189,235,6});
    for(auto& item:state.players[1].items)item.token+=1000000;
    Match m(cap);m.load_scene(state,0,0);m.step(play(0,{0},7));
    check(!m.snapshot().last_removal&&m.observe({}).find("\"last_removal\":null")!=std::string::npos,"pending removal never reveals future selected artifacts");
    m.step(defend(7));const RemovalEvent expected{0,0,7,205,{6,6,235}};
    check(m.snapshot().last_removal==expected&&m.snapshot().state.players[1].items.size()==1&&m.snapshot().state.players[1].items[0].model==189
        &&m.snapshot().rng==3*0x9e3779b97f4a7c15ULL,"public model order follows actual draws, preserves duplicates and leaves the unselected artifact secret");roundtrip(m);
    const std::string event="\"last_removal\":{\"origin\":0,\"source\":0,\"target\":7,\"model\":205,\"removed_models\":[6,6,235]}";
    for(const auto viewer:{std::optional<PlayerId>{},std::optional<PlayerId>{0},std::optional<PlayerId>{7}}){const auto view=m.observe(viewer);
        check(view.find(event)!=std::string::npos&&view.find(removal_event_rule)!=std::string::npos&&(viewer==7||view.find("100000")==std::string::npos)
            &&view.find("redirect_rng")==std::string::npos,"all viewers see the same resolved model event without removal tokens or RNG");}
    auto soap=scene({206},{235,235,223,235});soap.players[1].items[0].used=true;soap.players[1].items[1].used=true;
    auto wash=fixture(soap);wash.step(play(0,{0},7));wash.step(defend(7));
    check(wash.snapshot().last_removal==RemovalEvent{0,0,7,206,{235,235}},"soap reveals each removed used-miracle copy, leaving unused miracles outside its pool");roundtrip(wash);
    auto reflected=fixture(scene({205,6,6},{190}));reflected.step(play(0,{0},7));reflected.step(defend(7,{0}));
    check(!reflected.snapshot().last_removal,"redirected removal still has no result before final response");roundtrip(reflected);reflected.step(defend(0));
    check(reflected.snapshot().last_removal==RemovalEvent{0,7,0,205,{6,6}},"resolved event keeps immutable origin but final reflected source and target");roundtrip(reflected);
}

void removal_event_lifetime_empty_and_rollback(){
    for(int kind=0;kind<4;++kind){auto m=fixture(scene({206},{3,199}));m.step(play(0,{0},7));m.step(defend(7));
        check(m.snapshot().last_removal==RemovalEvent{0,0,7,206,{}},"resolved empty soap creates an explicit empty event");roundtrip(m);
        reject(m,pray(0));const auto exact=m.snapshot();std::uint64_t policy=42;(void)m.sample_action(policy);
        check(m.snapshot()==exact,"queries and rejected actions preserve the previous public event");
        const auto command=kind==0?play(7,{1},7):kind==1?Command{7,CommandKind::discard,{0},{}}:kind==2?pray(7)
            :Command{7,CommandKind::exchange,{0},{},ResourceAllocation{40,10,20}};
        check(m.legal(command),"every tested next-main command is feasible");m.step(command);check(!m.snapshot().last_removal,"any accepted main clears the previous removal event");roundtrip(m);}
    auto m=fixture(scene({206},{}));m.step(play(0,{0},7));m.step(defend(7));auto exhausted=m.snapshot();exhausted.state.update_count=max_wire_integer;m.restore(exhausted);const auto before=m.snapshot();
    bool threw=false;try{m.step(pray(7));}catch(const MatchError&){threw=true;}check(threw&&m.snapshot()==before,"late next-main failure restores the cleared previous event atomically");
    m.load_scene(scene(),0,1234);check(!m.snapshot().last_removal,"load_scene clears the previous event");
    const std::array<PlayerSetup,2> setup{{{0,Team::solo,"a"},{7,Team::solo,"b"}}};m.reset(setup,0);check(!m.snapshot().last_removal,"reset clears removal event");
    MatchConfig cap;cap.max_decisions=1;auto cut=fixture(scene({205},{}),cap);const auto rng=cut.snapshot().rng;cut.step(play(0,{0},0));
    check(cut.snapshot().last_removal==RemovalEvent{0,0,0,205,{}}&&cut.snapshot().truncated&&cut.snapshot().rng==rng,"self-removal excludes consumed broom, records empty result and survives cutoff without gift draws");roundtrip(cut);
    auto pending=fixture(scene({205},{6}),cap);pending.step(play(0,{0},7));check(pending.snapshot().truncated&&!pending.snapshot().last_removal,"launch cutoff before resolution never invents removal results");roundtrip(pending);
    auto failed=scene({205},{6,6});failed.players[0].items[0].token=max_wire_integer;auto tokens=fixture(failed);tokens.step(play(0,{max_wire_integer},7));const auto saved=tokens.snapshot();threw=false;
    try{tokens.step(defend(7));}catch(const MatchError&){threw=true;}check(threw&&tokens.snapshot()==saved&&!tokens.snapshot().last_removal,"late gift failure rolls back newly revealed models along with removal state and RNG");
}

void removal_event_snapshot_guards(){
    auto m=fixture(scene({206},{235,235}));auto state=m.snapshot().state;for(auto& item:state.players[1].items)item.used=true;m.load_scene(state,0,0);m.step(play(0,{0},7));m.step(defend(7));const auto exact=m.snapshot();
    for(int error=0;error<10;++error){auto bad=exact;auto& event=*bad.last_removal;
        if(error==0)bad.decisions=0;if(error==1)event.origin=999;if(error==2)event.source=999;if(error==3)event.target=999;if(error==4)event.model=199;
        if(error==5)event.removed_models={235,235,235};if(error==6)event.removed_models={6};if(error==7)event.removed_models={0};if(error==8)event.removed_models={207};
        if(error==9)bad.last_danger=DangerEvent{0,7,{94},false,30,Element::light};
        bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"removal event boundaries/endpoints/kind/count/unsupported or nonmiracle IDs and danger coexistence reject atomically");}
    auto broom=fixture(scene({205},{6,6,6,6}));broom.step(play(0,{0},7));broom.step(defend(7));auto oversized=broom.snapshot();oversized.last_removal->removed_models.push_back(6);bool rejected=false;
    try{broom.restore(oversized);}catch(const MatchError&){rejected=true;}check(rejected,"broom event cannot exceed three results");
    auto pending=fixture(scene({205},{6}));pending.step(play(0,{0},7));auto forged=pending.snapshot();forged.last_removal=RemovalEvent{0,0,7,205,{6}};const auto before=pending.snapshot();rejected=false;
    try{pending.restore(forged);}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==before,"pending journal cannot forge an already resolved removal event");roundtrip(pending);
}

void sell_arithmetic_transfer_and_commodity_is_not_cast(){
    const Command command{0,CommandKind::sell,{0,1},0};auto state=scene({4,27},{});state.players[0].mp=4;state.players[0].cp=3;auto self=fixture(state);self.step(command);
    const auto& own=self.snapshot().state.players[0];check(own.hp==37&&own.mp==0&&own.cp==10&&own.items.size()==2&&own.items[0].token==2&&own.items[0].model==27
        &&self.snapshot().last_sale==SaleEvent{0,0,0,27,10}&&self.snapshot().next_token==4,"self SELL pays CP then MP then HP before full nominal credit, remints commodity and draws only one operation replacement");roundtrip(self);
    for(CardId product:{CardId{3},CardId{4},CardId{58},CardId{64},CardId{94},CardId{215},CardId{231},CardId{232}}){auto st=scene({4,product},{113});st.players[0].mp=0;st.players[0].cp=98;
        st.players[1].cp=3;st.players[1].mp=4;Match m; m.load_scene(st,0,1234);m.step({0,CommandKind::sell,{0,1},7});const auto& e=m.snapshot().queue.front();
        check(e.category==Category::trade&&!e.element&&e.power==0&&e.amount==find_card(product)->price&&m.snapshot().state.players[0].mp==0&&m.snapshot().state.players[0].hp==40
            &&m.snapshot().gifts==std::vector<std::uint32_t>{1,0}&&m.snapshot().batch->targets==std::vector<PlayerId>{7},"sale fixes price but never inherits commodity damage, percent, MP, Wide, or magic cost; one gift only");
        reject(m,defend(7,{0}));roundtrip(m);m.step(defend(7));const auto& buyer=m.snapshot().state.players[1];
        check(m.snapshot().last_sale==SaleEvent{0,0,7,product,find_card(product)->price}&&buyer.items.back().model==product&&!buyer.items.back().used
            &&m.snapshot().state.players[0].items.size()==1&&m.snapshot().state.players[0].mp==0&&m.snapshot().counter_created==0&&!m.snapshot().last_danger&&!m.snapshot().last_removal,"unused commodities transfer with fresh tokens and no cast, counter, danger, or removal effect");roundtrip(m);}
    auto paid=scene({4,27},{});paid.players[0].cp=98;paid.players[1].cp=3;paid.players[1].mp=4;auto foreign=fixture(paid);foreign.step({0,CommandKind::sell,{0,1},7});foreign.step(defend(7));
    check(foreign.snapshot().state.players[0].cp==99&&foreign.snapshot().state.players[1].hp==37&&foreign.snapshot().state.players[1].mp==0&&foreign.snapshot().state.players[1].cp==0,"foreign buyer debt does not reduce full seller credit, which clips at99");
    auto full=scene({4,235},{});full.players[0].mp=0;MatchConfig cap;cap.hand_limit=cap.initial_hand=2;auto packed=fixture(full,cap);packed.step(command);
    check(packed.snapshot().state.players[0].items.size()==2&&packed.snapshot().state.players[0].items[0].model==235&&!packed.snapshot().state.players[0].items[0].used
        &&packed.snapshot().state.players[0].hp==40&&packed.snapshot().state.players[0].mp==0,"full self sale has post-consumption room, keeps commodity MC unused, and never pays its cast cost");roundtrip(packed);
}

void sell_reflections_delivery_revival_and_cutoff(){
    auto state=scene({4,232,190},{190});state.players.push_back(player(99));auto m=fixture(state);m.step({0,CommandKind::sell,{0,1},7});m.step(defend(7,{0}));roundtrip(m);m.step(defend(0,{2}));roundtrip(m);m.step(defend(7));
    check(m.snapshot().last_sale==SaleEvent{0,0,7,232,0}&&m.snapshot().state.players[1].items.front().model==232&&m.snapshot().state.players[2].hp==40
        &&!m.snapshot().batch&&m.snapshot().state.players[0].items.size()==2,"two universal reflections retain one commodity and one target; sold Wide never creates enemy roster or extra gifts");roundtrip(m);
    MatchConfig cap;cap.max_decisions=2;auto revive=scene({4,208},{});revive.players[1].hp=5;revive.players[1].mp=revive.players[1].cp=0;auto saved=fixture(revive,cap);const auto rng=saved.snapshot().rng;saved.step({0,CommandKind::sell,{0,1},7});saved.step(defend(7));
    check(saved.snapshot().truncated&&!saved.snapshot().terminated&&saved.snapshot().state.players[1].hp==10&&saved.snapshot().revivals==1
        &&saved.snapshot().state.players[1].items.empty()&&saved.snapshot().last_sale==SaleEvent{0,0,7,208,10}&&saved.snapshot().rng==rng,"freshly delivered amulet can rescue invoice HP0 before cutoff; direct delivery and event survive, gifts are cancelled");roundtrip(saved);
    auto lethal=scene({4,27},{});lethal.players[1].hp=5;lethal.players[1].mp=lethal.players[1].cp=0;auto dead=fixture(lethal,cap);dead.step({0,CommandKind::sell,{0,1},7});dead.step(defend(7));
    check(dead.snapshot().terminated&&!dead.snapshot().truncated&&dead.snapshot().winners==std::vector<PlayerId>{0}&&dead.snapshot().state.players[1].items.back().model==27
        &&dead.snapshot().last_sale==SaleEvent{0,0,7,27,10},"invoice death is finalized after commodity delivery; natural outcome wins over cutoff");roundtrip(dead);
    MatchConfig launch;launch.max_decisions=1;auto pending=fixture(scene({4,27},{}),launch);const auto old=pending.snapshot();pending.step({0,CommandKind::sell,{0,1},7});
    check(pending.snapshot().truncated&&!pending.snapshot().last_sale&&pending.snapshot().next_token==old.next_token&&pending.snapshot().rng==old.rng,"launch cutoff consumes both selections but never bills, delivers, draws or publishes a body result");roundtrip(pending);
}

void sell_invalid_roles_sampler_and_journal(){
    auto m=fixture(scene({4,235,204},{}));Command valid{0,CommandKind::sell,{0,1},7};
    for(int error=0;error<9;++error){auto bad=valid;if(error==0)bad.items.clear();if(error==1)bad.items={0};if(error==2)bad.items={0,1,2};if(error==3)bad.items={0,0};if(error==4)bad.items={1,0};
        if(error==5)bad.target.reset();if(error==6)bad.target=999;if(error==7)bad.allocation=ResourceAllocation{40,10,20};if(error==8)bad.kind=CommandKind::play;reject(m,bad);}
    auto st=m.snapshot().state;st.players[0].items[1].used=true;auto learned=fixture(st);reject(learned,valid);
    auto full=scene({4,27},{});for(unsigned i=0;i<18;++i)full.players[1].items.push_back({i,113,{},false});auto blocked=fixture(full);const auto before=blocked.snapshot();
    check(blocked.legal(valid)&&blocked.snapshot()==before,"foreign full recipient does not create a hidden-capacity legality oracle");
    bool seen=false;for(std::uint64_t seed=0;seed<256;++seed){auto policy=seed;const auto command=blocked.sample_action(policy);check(blocked.legal(command)&&blocked.snapshot()==before,"policy sampling is pure with full recipients");if(command.kind==CommandKind::sell){seen=true;check(command.target&&command.items.size()==2&&command.items[0]!=command.items[1]&&!command.allocation,"SELL policy chooses alive targets and unused distinct products without reading capacity");}}check(seen,"SELL is reachable without enumerating commodity-target cross products");
    blocked.step(valid);blocked.step(defend(7));check(blocked.snapshot().state.players[1].items.size()==18&&blocked.snapshot().last_sale->overflow_model==113,"full recipient loses one existing model then receives commodity");roundtrip(blocked);
    auto single=fixture(scene({4},{}));for(std::uint64_t seed=0;seed<64;++seed){auto policy=seed;check(single.sample_action(policy).kind!=CommandKind::sell,"operation alone cannot generate a sell command");}
    auto active=fixture(scene({4,27},{190}));active.step(valid);active.step(defend(7,{0}));const auto exact=active.snapshot();
    for(int error=0;error<9;++error){auto bad=exact;if(error==0)bad.batch->attack.kind=CommandKind::play;if(error==1)bad.batch->attack.items[1]=99;if(error==2)++bad.queue[0].amount;
        if(error==3)bad.queue[0].power=7;if(error==4)bad.queue[0].element=Element::fire;if(error==5)bad.queue[0].models[1]=45;if(error==6)++bad.batch->next_token;
        if(error==7)bad.last_sale=SaleEvent{0,7,0,27,10};if(error==8)bad.batch->attack.allocation=ResourceAllocation{40,10,20};bool rejected=false;
        try{active.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&active.snapshot()==exact,"SELL anchor, price, product, prefix, token, allocation and premature public result are checked by full replay");}
}

void sell_event_guards_and_atomic_late_failures(){
    const Command self{0,CommandKind::sell,{0,1},0};auto m=fixture(scene({4,27},{}));m.step(self);const auto exact=m.snapshot();
    for(int error=0;error<8;++error){auto bad=exact;auto& event=*bad.last_sale;if(error==0)bad.decisions=0;if(error==1)event.origin=999;if(error==2)event.source=999;if(error==3)event.target=999;
        if(error==4)event.model=207;if(error==5)++event.price;if(error==6)bad.last_removal=RemovalEvent{0,0,7,205,{}};if(error==7)bad.last_danger=DangerEvent{0,7,{94},false,30,Element::light};bool rejected=false;
        try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"sale public event validates price, supported product, endpoints, positive decision and exclusive event type");}
    reject(m,pray(0));auto exhausted=exact;exhausted.state.update_count=max_wire_integer;m.restore(exhausted);const auto previous=m.snapshot();bool failed=false;try{m.step(pray(7));}catch(const MatchError&){failed=true;}
    check(failed&&m.snapshot()==previous,"late next-main failure restores prior sale event");m.restore(exact);m.step(pray(7));check(!m.snapshot().last_sale,"accepted next main clears prior sale event");roundtrip(m);
    m.load_scene(scene(),0,0);check(!m.snapshot().last_sale,"load clears sale event");
    for(bool after_delivery:{false,true}){auto state=scene({4,27},{});state.players[0].items[0].token=max_wire_integer-(after_delivery?1:0);auto bad=fixture(state);const auto op=state.players[0].items[0].token;
        bad.step({0,CommandKind::sell,{op,1},7});const auto before=bad.snapshot();failed=false;try{bad.step(defend(7));}catch(const MatchError&){failed=true;}
        check(failed&&bad.snapshot()==before&&!bad.snapshot().last_sale,"late delivery or deferred gift token exhaustion rolls back invoice, commodity, public event, journal and RNG");}
}

void buy_quote_accept_decline_and_public_escrow(){
    auto state=scene({5},{6,27,235});state.players[1].items[2].used=true;MatchConfig cap;cap.max_decisions=3;Match m(cap);m.load_scene(state,0,0);const auto original=m.snapshot();
    m.step({0,CommandKind::buy,{0},7});check(m.snapshot().phase==MatchPhase::response&&!m.snapshot().purchase&&m.snapshot().gifts[0]==1,"BUY consumes one operation with one gift and waits for a nonattack response");
    m.step(defend(7));const auto quote=m.snapshot();check(quote.phase==MatchPhase::purchase&&quote.actor==0&&quote.queue.empty()&&quote.batch&&quote.purchase
        &&quote.purchase->item.model==27&&quote.purchase->item.token==1&&quote.purchase->slot==1&&quote.state.players[1].items.size()==2&&quote.rng==0x9e3779b97f4a7c15ULL,"one unused physical commodity is selected into escrow, with original token and slot hidden from the public quote");roundtrip(m);
    const std::string public_quote="\"purchase\":{\"origin\":0,\"buyer\":0,\"seller\":7,\"model\":27,\"price\":10}";
    for(const auto viewer:{std::optional<PlayerId>{},std::optional<PlayerId>{0},std::optional<PlayerId>{7}}){const auto view=m.observe(viewer);check(view.find(public_quote)!=std::string::npos&&view.find(buy_rule)!=std::string::npos&&view.find("\"slot\"")==std::string::npos&&view.find("redirect_rng")==std::string::npos,"every local viewer sees the same offered model and price without escrow position or RNG");}
    std::uint64_t policy=0;const auto before=m.snapshot();const auto command=m.sample_action(policy);check(m.snapshot()==before&&m.legal(command)&&(command.kind==CommandKind::accept||command.kind==CommandKind::decline),"quote sampling uses only separate policy RNG");
    m.step({0,CommandKind::accept,{},{}});check(m.snapshot().truncated&&m.snapshot().last_purchase==PurchaseEvent{0,0,7,27,10,PurchaseReason::bought}
        &&m.snapshot().state.players[0].cp==10&&m.snapshot().state.players[1].cp==30&&m.snapshot().state.players[0].items.size()==1
        &&m.snapshot().state.players[0].items[0].token==3&&m.snapshot().rng==quote.rng&&!m.snapshot().purchase,"accept debits buyer then credits seller and delivers one fresh unused copy before cutoff cancels gifts");roundtrip(m);
    m.restore(quote);m.step({0,CommandKind::decline,{},{}});check(m.snapshot().truncated&&m.snapshot().state.players[1].items==original.state.players[1].items
        &&m.snapshot().state.players[0].cp==20&&m.snapshot().state.players[1].cp==20&&m.snapshot().next_token==original.next_token&&m.snapshot().rng==quote.rng
        &&m.snapshot().last_purchase==PurchaseEvent{0,0,7,27,10,PurchaseReason::declined},"decline restores the exact original token, order and money without an extra draw");roundtrip(m);
}

void buy_empty_affordability_and_unused_miracles(){
    MatchConfig cap;cap.max_decisions=2;auto used=scene({5},{235});used.players[1].items[0].used=true;auto empty=fixture(used,cap);const auto before=empty.snapshot();empty.step({0,CommandKind::buy,{0},7});empty.step(defend(7));
    check(empty.snapshot().truncated&&!empty.snapshot().purchase&&empty.snapshot().rng==before.rng&&empty.snapshot().state.players[1].items==before.state.players[1].items
        &&empty.snapshot().last_purchase==PurchaseEvent{0,0,7,0,0,PurchaseReason::empty},"used miracles are outside the BUY population; empty means no draw, no offer and no money");roundtrip(empty);
    auto poor=scene({5},{27});poor.players[0].hp=poor.players[0].mp=99;poor.players[0].cp=9;auto cannot=fixture(poor,cap);const auto old=cannot.snapshot();cannot.step({0,CommandKind::buy,{0},7});cannot.step(defend(7));
    check(cannot.snapshot().truncated&&!cannot.snapshot().purchase&&cannot.snapshot().state.players[0].hp==99&&cannot.snapshot().state.players[0].mp==99
        &&cannot.snapshot().state.players[0].cp==9&&cannot.snapshot().state.players[1].items==old.state.players[1].items&&cannot.snapshot().next_token==old.next_token
        &&cannot.snapshot().last_purchase==PurchaseEvent{0,0,7,27,10,PurchaseReason::insufficient_cp},"BUY affordability uses CP only and restores the selection instead of borrowing MP or HP");roundtrip(cannot);
    auto free=scene({5},{215});free.players[0].mp=free.players[0].cp=0;auto miracle=fixture(free);miracle.step({0,CommandKind::buy,{0},7});miracle.step(defend(7));
    check(miracle.snapshot().phase==MatchPhase::purchase&&miracle.snapshot().purchase->item.model==215,"an unused miracle with no price can be offered at CP0");miracle.step({0,CommandKind::accept,{},{}});
    check(miracle.snapshot().state.players[0].items[0].model==215&&!miracle.snapshot().state.players[0].items[0].used&&miracle.snapshot().state.players[0].mp==0
        &&miracle.snapshot().state.players[0].hp==40&&miracle.snapshot().state.players[1].hp==40,"purchased miracle is delivered unused without casting or paying its MP cost");roundtrip(miracle);
    reject(miracle,{7,CommandKind::accept,{},{}});miracle.step(pray(7));check(!miracle.snapshot().last_purchase,"next accepted main clears purchase result");
}

void buy_reflection_ownership_and_quote_journal(){
    auto m=fixture(scene({5,6},{190}));m.step({0,CommandKind::buy,{0},7});m.step(defend(7,{0}));check(m.snapshot().queue[0].source==7&&m.snapshot().queue[0].target==0,"universal reflection exchanges purchase roles without selecting a commodity yet");roundtrip(m);m.step(defend(0));
    check(m.snapshot().phase==MatchPhase::purchase&&m.snapshot().actor==7&&m.snapshot().owner==0&&m.snapshot().purchase->buyer==7&&m.snapshot().purchase->seller==0
        &&m.snapshot().purchase->item.model==6&&m.snapshot().purchase->origin==0,"reflected BUY asks the current buyer, independently of original owner and final defender");roundtrip(m);
    const auto exact=m.snapshot();for(int error=0;error<12;++error){auto bad=exact;auto& offer=*bad.purchase;if(error==0)offer.slot=18;if(error==1)offer.item.token=99;if(error==2)offer.item.model=27;
        if(error==3)offer.item.used=true;if(error==4)offer.item.fake_model=6;if(error==5)offer.buyer=0;if(error==6)offer.seller=7;if(error==7)offer.origin=999;
        if(error==8)bad.actor=0;if(error==9)++bad.rng;if(error==10)bad.last_purchase=PurchaseEvent{0,7,0,6,1,PurchaseReason::bought};if(error==11)bad.batch->responses.back().kind=CommandKind::accept;
        bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"purchase full replay rejects forged escrow identity/order, price model, parties, actor, draw and premature decision/event");}
    m.step({7,CommandKind::accept,{},{}});check(m.snapshot().last_purchase==PurchaseEvent{0,7,0,6,1,PurchaseReason::bought}
        &&m.snapshot().state.players[0].cp==21&&m.snapshot().state.players[1].cp==19&&m.snapshot().state.players[0].items.size()==1&&m.snapshot().state.players[1].items.size()==2,"reflection preserves BUY operation gift while buyer receives one commodity plus its mirror replacement");roundtrip(m);
}

void buy_roles_cutoff_returns_and_late_failure(){
    auto m=fixture(scene({5},{}));const Command buy{0,CommandKind::buy,{0},7};for(int error=0;error<6;++error){auto bad=buy;if(error==0)bad.items.clear();if(error==1)bad.items={0,0};if(error==2)bad.target.reset();if(error==3)bad.target=999;
        if(error==4)bad.kind=CommandKind::play;if(error==5)bad.allocation=ResourceAllocation{40,10,20};reject(m,bad);}
    auto self=buy;self.target=0;const auto old=m.snapshot();check(m.legal(self)&&m.snapshot()==old,"self BUY input is accepted without mutating the query state");
    reject(m,{0,CommandKind::accept,{},{}});reject(m,{0,CommandKind::decline,{},{}});
    for(unsigned limit:{1u,2u}){MatchConfig cap;cap.max_decisions=limit;auto cut=fixture(scene({5},{27}),cap);const auto initial=cut.snapshot();cut.step(buy);if(limit==2)cut.step(defend(7));
        check(cut.snapshot().truncated&&!cut.snapshot().purchase&&cut.snapshot().state.players[1].items==initial.state.players[1].items,"cutoff never strands a selected commodity in escrow");
        check(limit==1?!cut.snapshot().last_purchase:cut.snapshot().last_purchase==PurchaseEvent{0,0,7,27,10,PurchaseReason::cutoff},"only an actually selected quote creates the automatic cutoff return result");roundtrip(cut);}
    auto active=fixture(scene({5},{27}));active.step(buy);active.step(defend(7));const auto quote=active.snapshot();for(const auto& bad:std::vector<Command>{{7,CommandKind::accept,{},{}},{0,CommandKind::accept,{0},{}},{0,CommandKind::accept,{},7},{0,CommandKind::pray,{},{}},{0,CommandKind::defend,{},{}},{0,CommandKind::decline,{},{},ResourceAllocation{40,10,20}}})reject(active,bad);
    for(bool after_delivery:{false,true}){auto state=scene({5},{27});state.players[0].items[0].token=max_wire_integer-(after_delivery?1:0);auto exhausted=fixture(state);exhausted.step({0,CommandKind::buy,{state.players[0].items[0].token},7});exhausted.step(defend(7));const auto saved=exhausted.snapshot();bool failed=false;
        try{exhausted.step({0,CommandKind::accept,{},{}});}catch(const MatchError&){failed=true;}check(failed&&exhausted.snapshot()==saved&&exhausted.snapshot().purchase,"late delivery/gift failure keeps complete escrow and rolls back CP, delivery token, decision and result");}
    auto bad=quote;bad.state.update_count=max_wire_integer;bad.batch->initial_state.update_count=max_wire_integer-(bad.decisions-bad.batch->decisions);active.restore(bad);const auto before=active.snapshot();bool failed=false;
    try{active.step({0,CommandKind::decline,{},{}});}catch(const MatchError&){failed=true;}check(failed&&active.snapshot()==before,"late declined-return failure restores escrow rather than duplicating the returned item");
    active.load_scene(scene(),0,0);check(!active.snapshot().purchase&&!active.snapshot().last_purchase,"scene loading clears offer and public result");
}

void disease_reapplication_cures_and_damage_counter_basis(){
    for(const auto before:{Curse::none,Curse::cold,Curse::fever,Curse::hell,Curse::heaven}){auto state=scene({219},{});state.players[0].mp=99;if(before!=Curse::none)state.players[1].curses={before};state.players.push_back(player(99));MatchConfig cap;cap.max_decisions=2;auto m=fixture(state,cap);m.step(play(0,{0},7));m.step(defend(7));
        const auto after=before==Curse::none?Curse::cold:before==Curse::heaven?Curse::heaven:static_cast<Curse>(static_cast<int>(before)+1);
        check(m.snapshot().state.players[1].curses==std::vector<Curse>{after}&&m.snapshot().state.players[1].hp==(before==Curse::heaven?0:40)&&!m.snapshot().last_tick,"Cold reapplication advances one disease, Heaven reapplication falls to0, and command cutoff skips owner tick");roundtrip(m);}
    auto higher=fixture(scene({220},{}));auto st=higher.snapshot().state;st.players[0].mp=99;st.players[1].curses={Curse::cold};higher.load_scene(st,0,0);higher.step(play(0,{0},7));higher.step(defend(7));
    check(higher.snapshot().state.players[1].curses==std::vector<Curse>{Curse::heaven}&&higher.snapshot().state.players[1].hp==40,"higher incoming disease wins without premature Heaven fall");roundtrip(higher);
    for(const auto status:{Curse::cold,Curse::fever,Curse::hell,Curse::heaven})for(bool all:{false,true}){auto state=scene({all?CardId{238}:CardId{237}},{});state.players[0].curses={status,Curse::flash,Curse::darkcloud};MatchConfig cap;cap.max_decisions=1;auto m=fixture(state,cap);m.step(play(0,{0},0));
        const auto expected=all?std::vector<Curse>{}:status==Curse::cold||status==Curse::fever?std::vector<Curse>{Curse::darkcloud}:std::vector<Curse>{status,Curse::darkcloud};
        check(m.snapshot().state.players[0].curses==expected&&m.snapshot().state.players[0].hp==40&&!m.snapshot().last_tick,"mild removes Cold/Fever/Flash only; all removes Heaven without reapplication fall");roundtrip(m);}
    auto blocked=fixture(scene({33},{176}));blocked.step(play(0,{0},7));blocked.step(defend(7,{0}));check(blocked.snapshot().state.players[1].hp==40&&blocked.snapshot().state.players[1].curses.empty(),"fully defended disease weapon does not add a curse");
    auto cursed=scene({33},{208,184});cursed.players[1].curses={Curse::heaven};auto counter=fixture(cursed);counter.step(play(0,{0},7));counter.step(defend(7,{1}));
    check(counter.snapshot().state.players[1].hp==10&&counter.snapshot().revivals==1&&counter.snapshot().queue[0].power==18&&counter.snapshot().counter_created==1,"Heaven fall can revive before survivor counter, whose basis is regular9 damage rather than fall damage");roundtrip(counter);counter.step(defend(0));roundtrip(counter);
}

void disease_fixed_roll_post_upgrade_owner_tick(){
    for(bool worsen:{false,true})for(const auto status:{Curse::cold,Curse::fever,Curse::hell,Curse::heaven}){auto state=scene({235},{});state.players[0].items[0].used=true;state.players[0].curses={status};state.players[1].curses={Curse::heaven};MatchConfig cap;cap.hand_limit=cap.initial_hand=1;Match m(cap);const auto seed=worsen?29u:0u;m.load_scene(state,0,seed);m.step(pray(0));
        const auto next=worsen&&status!=Curse::heaven?static_cast<Curse>(static_cast<int>(status)+1):status;const auto fallen=worsen&&status==Curse::heaven;
        const auto amount=next==Curse::cold?-1:next==Curse::fever?-2:next==Curse::hell?-5:5;
        check(m.snapshot().last_tick&&m.snapshot().last_tick->owner==0&&m.snapshot().last_tick->before==status&&m.snapshot().last_tick->after==next
            &&m.snapshot().last_tick->worsened==worsen&&m.snapshot().last_tick->fallen==fallen&&m.snapshot().state.players[0].hp==(fallen?0:40+amount)
            &&m.snapshot().state.players[1].hp==40&&m.snapshot().rng==std::uint64_t{seed}+0x9e3779b97f4a7c15ULL,"only owner ticks once with fixed5 percent, upgrade before HP and no Heaven heal after fall");
        check(m.observe({}).find(disease_rule)!=std::string::npos&&m.observe({}).find("last_tick")==std::string::npos&&m.observe({}).find("revival_token")==std::string::npos,"public board/rule never exposes private tick accounting or physical tokens");roundtrip(m);}
    auto capstate=scene({235},{});capstate.players[0].items[0].used=true;capstate.players[0].curses={Curse::heaven};capstate.players[0].hp=98;MatchConfig full;full.hand_limit=full.initial_hand=1;Match cap(full);cap.load_scene(capstate,0,0);cap.step(pray(0));check(cap.snapshot().state.players[0].hp==99,"Heaven healing clips at99");
    auto other=scene({235},{});other.players[0].items[0].used=true;other.players[0].curses={Curse::flash,Curse::darkcloud};Match no_tick(full);no_tick.load_scene(other,0,0);no_tick.step(pray(0));check(!no_tick.snapshot().last_tick&&no_tick.snapshot().rng==0,"non-disease curses never consume natural disease draws");roundtrip(no_tick);
}

void disease_action_boundaries_quotes_repeats_and_cutoff(){
    auto state=scene({14},{});state.players[0].curses={Curse::cold};Match m;m.load_scene(state,0,0);m.step(play(0,{0},7));m.step(defend(7));check(!m.snapshot().last_tick&&m.snapshot().state.players[0].hp==40,"first repeat hit has no owner tick");roundtrip(m);m.step(defend(7));check(m.snapshot().last_tick&&m.snapshot().state.players[0].hp==39,"whole two-hit action has exactly one owner tick");roundtrip(m);
    auto buy=scene({5},{6});buy.players[0].curses={Curse::cold};auto quote=fixture(buy);quote.step({0,CommandKind::buy,{0},7});quote.step(defend(7));const auto before=quote.snapshot();check(before.purchase&&!before.last_tick&&before.state.players[0].hp==40,"BUY offer does not advance disease time");quote.step({0,CommandKind::decline,{},{}});check(quote.snapshot().last_tick&&quote.snapshot().last_tick->owner==0&&quote.snapshot().state.players[0].hp<40,"purchase decision finishes the original owner's action before ticking");roundtrip(quote);
    auto reflected=scene({5,6},{190});reflected.players[0].curses={Curse::cold};reflected.players[1].curses={Curse::heaven};auto r=fixture(reflected);r.step({0,CommandKind::buy,{0},7});r.step(defend(7,{0}));r.step(defend(0));r.step({7,CommandKind::decline,{},{}});
    check(r.snapshot().last_tick&&r.snapshot().last_tick->owner==0&&r.snapshot().state.players[1].hp==40,"reflected purchase decision still ticks main owner, not the current buyer");roundtrip(r);
    for(bool lethal:{false,true}){auto st=scene({6},{});st.players[0].hp=1;st.players[0].curses={Curse::hell};if(lethal)st.players[1].hp=1;MatchConfig cap;cap.max_decisions=2;auto c=fixture(st,cap);const auto rng=c.snapshot().rng;c.step(play(0,{0},7));c.step(defend(7));
        check(!c.snapshot().last_tick&&c.snapshot().rng==rng&&c.snapshot().state.players[0].hp==1&&(lethal?c.snapshot().terminated:c.snapshot().truncated),"body natural outcome and decision cutoff take priority over a completed-action disease tick");roundtrip(c);}
    auto dying=scene({53},{184});dying.players[0].hp=1;dying.players[0].curses={Curse::cold};auto dead=fixture(dying);dead.step(play(0,{0},7));dead.step(defend(7,{0}));
    check(dead.snapshot().terminated&&!dead.snapshot().last_tick,"owner dead from action body never draws a disease roll");roundtrip(dead);
}

void disease_tick_revivals_and_body_accounting(){
    auto state=scene({202,208,208},{});state.players[0].hp=1;state.players[0].curses={Curse::heaven};Match m;m.load_scene(state,0,0);m.step(play(0,{0},0));
    check(m.snapshot().revivals==2&&m.snapshot().last_tick&&m.snapshot().last_tick->fallen&&m.snapshot().last_tick->revival_token==2
        &&m.snapshot().last_tick->hp_before==10&&m.snapshot().last_tick->hp_after==10&&m.snapshot().state.players[0].hp==10,"randomHP body rescue and subsequent Heaven-fall rescue use distinct physical amulets at two boundaries");
    const auto& t=*m.snapshot().last_tick;const auto body_delta=m.snapshot().state.players[0].hp-1-10*static_cast<std::int64_t>(m.snapshot().revivals-1)-(t.hp_after-t.hp_before);
    check(body_delta==-1,"tick HP and tick rescue can be separated from randomHP body loss");roundtrip(m);
    auto swept=scene({205,208,6,6,6},{});swept.players[0].hp=1;swept.players[0].curses={Curse::cold};Match broom;broom.load_scene(swept,0,2);broom.step(play(0,{0},0));
    check(broom.snapshot().last_removal==RemovalEvent{0,0,0,205,{6,6,6}}&&broom.snapshot().last_tick&&broom.snapshot().last_tick->revival_token==1
        &&broom.snapshot().revivals==1,"self broom removes three copies while a fourth physical disappearance is the independent tick amulet");roundtrip(broom);
    auto fatal=scene();fatal.players[0].hp=1;fatal.players[0].curses={Curse::cold};Match end;end.load_scene(fatal,0,0);end.step(pray(0));
    check(end.snapshot().terminated&&end.snapshot().winners==std::vector<PlayerId>{7}&&end.snapshot().last_tick&&end.snapshot().last_tick->hp_after==0
        &&end.snapshot().state.players[0].items.empty()&&end.snapshot().gifts==std::vector<std::uint32_t>{0,0}&&end.snapshot().rng==0x9e3779b97f4a7c15ULL,"tick death cancels the pending prayer gift without its draw");roundtrip(end);
}

void disease_canonical_tick_guards_and_atomic_failure(){
    auto s=scene();s.players[0].curses={Curse::cold,Curse::flash,Curse::darkcloud};auto valid=fixture(s);roundtrip(valid);
    for(const auto& curses:std::vector<std::vector<Curse>>{{Curse::cold,Curse::fever},{Curse::flash,Curse::cold},{Curse::cold,Curse::cold},{Curse::hell,Curse::heaven}}){auto bad=s;bad.players[0].curses=curses;const auto exact=valid.snapshot();bool rejected=false;try{valid.load_scene(bad,0,0);}catch(const MatchError&){rejected=true;}check(rejected&&valid.snapshot()==exact,"only one sorted unique disease and supported non-diseases form a canonical state");}
    auto full=scene({235},{});full.players[0].items[0].used=true;full.players[0].curses={Curse::cold};MatchConfig cap;cap.hand_limit=cap.initial_hand=1;Match m(cap);m.load_scene(full,0,0);m.step(pray(0));const auto exact=m.snapshot();
    for(int error=0;error<8;++error){auto bad=exact;auto& tick=*bad.last_tick;if(error==0)tick.before=Curse::none;if(error==1)tick.after=Curse::hell;if(error==2)tick.hp_before=0;if(error==3){tick.hp_after=99;bad.state.players[0].hp=99;}
        if(error==4)tick.worsened=true;if(error==5)tick.fallen=true;if(error==6)tick.revival_token=0;if(error==7)tick.owner=999;bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"private tick validates disease progression, HP, endpoints and vanished revival copy without claiming complete history");}
    auto late=scene({3},{});late.players[0].hp=5;late.players[0].curses={Curse::cold};late.players[0].items[0].token=max_wire_integer;auto failed=fixture(late);const auto before=failed.snapshot();bool rejected=false;
    try{failed.step({0,CommandKind::exchange,{max_wire_integer},{},ResourceAllocation{5,10,20}});}catch(const MatchError&){rejected=true;}check(rejected&&failed.snapshot()==before&&!failed.snapshot().last_tick,"late gift failure rolls back completed action, owner tick HP and random draw atomically");
    auto pending=fixture(scene({6},{}));pending.step(play(0,{0},7));auto forged=pending.snapshot();forged.last_tick=exact.last_tick;const auto old=pending.snapshot();rejected=false;try{pending.restore(forged);}catch(const MatchError&){rejected=true;}check(rejected&&pending.snapshot()==old,"pending action cannot carry a previous private disease tick");
}

void fever_mask_full_defense_copies_and_append_boundaries(){
    auto m=fixture(scene({6},{168}));m.step(play(0,{0},7));reject(m,play(7,{0},0));m.step(defend(7,{0}));
    check(m.snapshot().state.players[1].hp==40&&m.snapshot().state.players[1].curses==std::vector<Curse>{Curse::fever}&&!m.snapshot().last_tick,"ordinary mask fully blocks one damage but still applies Fever to its user, who is not the main owner");roundtrip(m);
    auto copies=fixture(scene({6},{168,168,168,168,208,208}));copies.step(play(0,{0},7));copies.step(defend(7,{0,1,2,3}));
    check(copies.snapshot().revivals==1&&copies.snapshot().state.players[1].hp==10&&copies.snapshot().state.players[1].curses==std::vector<Curse>{Curse::heaven}
        &&std::any_of(copies.snapshot().state.players[1].items.begin(),copies.snapshot().state.players[1].items.end(),[](const auto&i){return i.token==5&&i.model==208;}),"four mask copies apply Fever/Hell/Heaven/fall in selection order, with one whole-body rescue rather than per-copy rescue");roundtrip(copies);
    for(const auto ids:{std::vector<CardId>{168,190},std::vector<CardId>{190,168},std::vector<CardId>{168,233},std::vector<CardId>{233,168},std::vector<CardId>{168,234},std::vector<CardId>{189,168,190}}){
        std::vector<VisibleSelection> prefix;bool accepted=true;for(auto id:ids){if(!can_append_response(prefix,{id,false},{false,Category::weapons,{},true})){accepted=false;break;}prefix.push_back({id,false});}check(!accepted,"mask cannot legally share a response with reflect/block/bounce, so prefix HP0 cannot escape as a waiting frame");}
    const auto fire=elemental(Category::weapons,Element::fire);auto filter=fixture(scene({fire},{189,168}));filter.step(play(0,{0},7));reject(filter,defend(7,{1}));filter.step(defend(7,{0,1}));
    check(filter.snapshot().state.players[1].curses==std::vector<Curse>{Curse::fever},"leading filter makes ordinary Fire mask eligible without disabling self-curse");roundtrip(filter);
    auto flash=scene({6},{168,168});flash.players[1].curses={Curse::flash};auto one=fixture(flash);one.step(play(0,{0},7));reject(one,defend(7,{0,1}));one.step(defend(7,{0}));check(one.snapshot().state.players[1].curses==std::vector<Curse>{Curse::fever,Curse::flash},"Flash remains a one-card restriction independent of Fever");roundtrip(one);
}

void fever_mask_prefix_fall_body_counter_and_self_alias(){
    auto st=scene({45},{168,184,208});st.players[1].curses={Curse::heaven};auto normal=fixture(st);normal.step(play(0,{0},7));normal.step(defend(7,{0,1}));
    check(normal.snapshot().state.players[1].hp==10&&normal.snapshot().state.players[0].hp==42&&normal.snapshot().counter_created==0&&normal.snapshot().revivals==1,"nonalias prefix fall is excluded from target body loss and counter basis while source absorbs the positive remainder");roundtrip(normal);
    auto state=scene({45,168,184,208},{});state.players[0].curses={Curse::heaven};Match self;self.load_scene(state,0,0);self.step(play(0,{0},0));self.step(defend(0,{1,2}));
    check(self.snapshot().phase==MatchPhase::response&&self.snapshot().queue[0].is_counter&&self.snapshot().queue[0].power==4&&self.snapshot().state.players[0].hp==10
        &&self.snapshot().revivals==1&&!self.snapshot().last_tick,"selfalias prefix0 then absorb2 then actual targetloss2 legitimately creates counter4 after whole-body rescue");roundtrip(self);
    const auto exact=self.snapshot();for(int error=0;error<5;++error){auto bad=exact;if(error==0)bad.batch->responses[0].items={2};if(error==1)bad.batch->initial_state.players[0].curses={Curse::fever};
        if(error==2)bad.state.players[0].hp=9;if(error==3)bad.state.players[0].curses={Curse::fever};if(error==4)bad.queue[0].power=0;bool rejected=false;try{self.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&self.snapshot()==exact,"journal replay includes self-curse prefix, temporary HP0, source-heal alias and derived counter basis");}
    self.step(defend(0));check(self.snapshot().state.players[0].hp==11&&self.snapshot().last_tick&&self.snapshot().last_tick->before==Curse::heaven,"only after counter completion does original owner's Heaven tick heal five");roundtrip(self);
}

void heaven_herb_mp_first_reflection_rescue_and_cutoff(){
    MatchConfig cap;cap.max_decisions=1;auto state=scene({198},{});state.players[0].mp=90;state.players[0].curses={Curse::cold};auto clipped=fixture(state,cap);const auto rng=clipped.snapshot().rng;clipped.step(play(0,{0},0));
    check(clipped.snapshot().truncated&&clipped.snapshot().state.players[0].mp==99&&clipped.snapshot().state.players[0].hp==40&&clipped.snapshot().state.players[0].curses==std::vector<Curse>{Curse::heaven}
        &&clipped.snapshot().rng==rng&&!clipped.snapshot().last_tick,"herb clips MP+20 before upgrading Cold to Heaven, with no fee/draw and cutoff before disease tick");roundtrip(clipped);
    auto fatal=state;fatal.players[0].curses={Curse::heaven};auto die=fixture(fatal,cap);die.step(play(0,{0},0));check(die.snapshot().terminated&&!die.snapshot().truncated&&die.snapshot().state.players[0].hp==0
        &&die.snapshot().state.players[0].mp==99&&!die.snapshot().last_tick,"Heaven reapplication death does not undo the prior MP gain and precedes cutoff/tick");roundtrip(die);
    auto both=scene({198,208,208},{});both.players[0].mp=5;both.players[0].curses={Curse::heaven};Match double_rescue;double_rescue.load_scene(both,0,29);double_rescue.step(play(0,{0},0));
    check(double_rescue.snapshot().state.players[0].mp==25&&double_rescue.snapshot().state.players[0].hp==10&&double_rescue.snapshot().revivals==2
        &&double_rescue.snapshot().last_tick&&double_rescue.snapshot().last_tick->fallen&&double_rescue.snapshot().last_tick->revival_token==2,"herb body fall and later natural Heaven fall consume two different amulets without losing MP gain");roundtrip(double_rescue);
    auto reflected=scene({198},{190,168});reflected.players[0].mp=90;reflected.players[1].mp=0;Match m;m.load_scene(reflected,0,0);m.step(play(0,{0},7));reject(m,defend(7,{1}));m.step(defend(7,{0}));roundtrip(m);m.step(defend(0));
    check(m.snapshot().state.players[0].mp==99&&m.snapshot().state.players[0].hp==45&&m.snapshot().state.players[0].curses==std::vector<Curse>{Curse::heaven}
        &&m.snapshot().state.players[1].mp==0&&m.snapshot().state.players[1].curses.empty()&&m.observe({}).find(disease_item_rule)!=std::string::npos,"reflected herb affects current receiver and only the completed original-owner action ticks");roundtrip(m);
}

void disease_items_nonuse_roles_and_atomic_failures(){
    for(CardId model:{CardId{168},CardId{198}}){auto discard=fixture(scene({model},{}));if(model==168)reject(discard,play(0,{0},0));discard.step({0,CommandKind::discard,{0},{}});check(discard.snapshot().state.players[0].curses.empty()&&discard.snapshot().state.players[0].mp==10,"discarding disease items never invokes their use effect");roundtrip(discard);
        auto sale=fixture(scene({4,model},{}));sale.step({0,CommandKind::sell,{0,1},7});sale.step(defend(7));check(sale.snapshot().state.players[0].curses.empty()&&sale.snapshot().state.players[1].curses.empty()&&sale.snapshot().state.players[1].mp==10,"selling mask/herb does not equip or cast its commodity");roundtrip(sale);
        auto buy=fixture(scene({5},{model}));buy.step({0,CommandKind::buy,{0},7});buy.step(defend(7));buy.step({0,CommandKind::accept,{},{}});check(buy.snapshot().state.players[0].curses.empty()&&buy.snapshot().state.players[0].mp==10,"purchased mask/herb remains an unused artifact without applying disease or MP");roundtrip(buy);}
    auto late=scene({6},{168,208});late.players[1].curses={Curse::heaven};late.update_count=max_wire_integer-1;auto masked=fixture(late);masked.step(play(0,{0},7));const auto before=masked.snapshot();bool failed=false;try{masked.step(defend(7,{0}));}catch(const MatchError&){failed=true;}
    check(failed&&masked.snapshot()==before,"late response update failure restores mask, prefix fall, amulet rescue, reservations and full journal");
    auto exhausted=scene({198},{});exhausted.players[0].items[0].token=max_wire_integer;exhausted.players[0].mp=90;exhausted.players[0].curses={Curse::cold};auto herb=fixture(exhausted);const auto original=herb.snapshot();failed=false;
    try{herb.step(play(0,{max_wire_integer},0));}catch(const MatchError&){failed=true;}check(failed&&herb.snapshot()==original&&!herb.snapshot().last_tick,"late gift failure rolls back MP, disease upgrade, body consume and later Heaven tick/RNG");
}

void fog_launch_targets_singleton_team_repeat_and_rng(){
    constexpr std::uint64_t draw=0x9e3779b97f4a7c15ULL;
    SnapshotState state;state.players={player(0,{6},Team::one),player(7,{},Team::two),player(9,{},Team::one),player(42),player(55,{},Team::two)};
    state.players[0].curses={Curse::fog};state.players[4].dead=true;state.players[4].hp=0;
    Match m;m.load_scene(state,0,0);m.step(play(0,{0},7));
    check(m.snapshot().actor==42&&m.snapshot().rng==draw&&m.snapshot().last_fog==FogTarget{0,7,42,{6}}
        &&m.snapshot().batch->targets==std::vector<PlayerId>{42},"Fog selects once among alive enemies in input order, excluding allies and dead seats");roundtrip(m);
    const auto exact=m.snapshot();for(int error=0;error<10;++error){auto bad=exact;
        if(error==0)bad.last_fog.reset();if(error==1)bad.last_fog->target=9;if(error==2)bad.last_fog->intended=0;
        if(error==3)bad.last_fog->models={95};if(error==4)bad.last_fog->models.push_back(232);if(error==5)bad.last_fog->owner=999;
        if(error==6)bad.rng^=1;if(error==7)bad.batch->rng^=1;if(error==8)bad.batch->targets[0]=7;if(error==9)bad.last_fog->models.clear();
        bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"Fog record and RNG are verified by full pending replay");}
    for(PlayerId target:{0u,9u}){Match direct;direct.load_scene(state,0,0);direct.step(play(0,{0},target));
        check(direct.snapshot().actor==target&&direct.snapshot().rng==0&&!direct.snapshot().last_fog,"Fog leaves explicit self and allied hints unchanged without a draw");roundtrip(direct);}
    auto single=scene({6},{});single.players[0].curses={Curse::fog};Match singleton;singleton.load_scene(single,0,0);singleton.step(play(0,{0},7));
    check(singleton.snapshot().rng==draw&&singleton.snapshot().last_fog==FogTarget{0,7,7,{6}},"Fog singleton enemy still consumes one launch draw");roundtrip(singleton);
    state.players[0].items[0].model=14;Match repeat;repeat.load_scene(state,0,0);repeat.step(play(0,{0},7));
    check(repeat.snapshot().batch->targets==std::vector<PlayerId>{42,42},"twice repeats the selected Fog recipient");repeat.step(defend(42));
    check(repeat.snapshot().actor==42&&repeat.snapshot().rng==draw&&repeat.snapshot().last_fog->target==42,"second hit does not draw another Fog target");roundtrip(repeat);
    repeat.step(defend(42));check(repeat.snapshot().state.players[3].hp==34&&repeat.snapshot().last_fog->target==42,"both repeated bodies reach selected enemy and record survives completion");roundtrip(repeat);
    repeat.step(pray(7));check(!repeat.snapshot().last_fog,"next accepted main clears private Fog record");roundtrip(repeat);
}

void fog_random202_order_exceptions_and_redirects(){
    constexpr std::uint64_t draw=0x9e3779b97f4a7c15ULL;
    SnapshotState state;state.players={player(0,{202,190}),player(7),player(42,{190})};state.players[0].curses={Curse::fog};
    Match m;m.load_scene(state,0,0);m.step(play(0,{0},7));const auto launched=m.snapshot();
    check(launched.actor==42&&launched.queue[0].amount==10&&launched.rng==draw*2,"Fog target draw precedes random HP sign draw");roundtrip(m);
    m.step(defend(42,{0}));m.step(defend(0,{1}));check(m.snapshot().actor==42&&m.snapshot().rng==launched.rng&&m.snapshot().last_fog==launched.last_fog
        &&m.snapshot().queue[0].amount==10,"reflections preserve selected target, immutable sign and Fog draw count");roundtrip(m);
    auto chance=state;chance.players[0].items={};chance.players[0].items.push_back({0,95,{},false});chance.players[1].curses=chance.players[2].curses={Curse::darkcloud};
    Match pct;pct.load_scene(chance,0,0);pct.step({0,CommandKind::play,{0},{}});
    check(!pct.snapshot().last_fog&&pct.snapshot().rng==0&&pct.snapshot().queue[0].forced_hit&&pct.snapshot().batch->targets==std::vector<PlayerId>{7,42},"targetless percentage attack keeps its enemy roster and no extra Fog draw");roundtrip(pct);
    auto wide=state;wide.players[0]=player(0,{6,232});wide.players[0].curses={Curse::fog};wide.players[0].mp=99;Match all;all.load_scene(wide,0,0);all.step(play(0,{0,1},7));
    check(!all.snapshot().last_fog&&all.snapshot().rng==0&&all.snapshot().batch->targets==std::vector<PlayerId>{7,42},"Wide excludes one-target Fog selection");roundtrip(all);
    auto danger=state;danger.players[0]=player(0,{94});danger.players[0].curses={Curse::fog};danger.players[2]=player(42,{209});
    MatchConfig cap;cap.max_decisions=1;Match mortar(cap);mortar.load_scene(danger,0,0);mortar.step(play(0,{0},7));
    check(mortar.snapshot().truncated&&mortar.snapshot().last_danger->target==42&&!mortar.snapshot().last_fog&&mortar.snapshot().rng==0,"Danger mortar priority is independent of Fog");roundtrip(mortar);
    auto buy=state;buy.players[0]=player(0,{5});buy.players[0].curses={Curse::fog};buy.players[1]=player(7,{27});buy.players[2]=player(42,{6});
    MatchConfig quote_cap;quote_cap.max_decisions=3;Match purchase(quote_cap);purchase.load_scene(buy,0,0);purchase.step({0,CommandKind::buy,{0},7});
    check(purchase.snapshot().actor==42&&purchase.snapshot().last_fog==FogTarget{0,7,42,{5}},"BUY uses selected Fog seller rather than hinted inventory");purchase.step(defend(42));
    check(purchase.snapshot().purchase&&purchase.snapshot().purchase->seller==42&&purchase.snapshot().purchase->item.model==6&&purchase.snapshot().rng==draw*2,"BUY reveals only actual selected seller candidate after a separate commodity draw");
    const auto quoted=purchase.snapshot();purchase.step({0,CommandKind::decline,{},{}});check(purchase.snapshot().truncated&&purchase.snapshot().rng==quoted.rng&&purchase.snapshot().last_fog==quoted.last_fog,"purchase decision does not reselect Fog target");roundtrip(purchase);
}

void fog_projection_and_query_noninterference(){
    SnapshotState first;first.players={player(0,{4,6,221,235},Team::one),player(7,{6},Team::two),player(9,{},Team::one),player(42,{235})};first.players[0].curses={Curse::fog};
    auto second=first;second.players[1].hp=1;second.players[1].mp=99;second.players[1].cp=0;second.players[1].curses={Curse::heaven,Curse::darkcloud};
    second.players[1].items.clear();for(ItemToken n=0;n<18;++n)second.players[1].items.push_back({n,6,{},false});second.players[3].hp=98;second.players[3].mp=0;second.players[3].cp=99;
    Match a,b;a.load_scene(first,0,0);b.load_scene(second,0,0);check(a.observe(0)==b.observe(0)&&a.observe({})!=b.observe({})&&a.observe(9)!=b.observe(9),"Fog hides enemy resources, curses and unused capacity while spectator and allied viewer retain their own visibility");
    const auto before_a=a.snapshot(),before_b=b.snapshot();bool sampled_sell=false;
    for(std::uint64_t seed=0;seed<256;++seed){auto pa=seed,pb=seed;const auto ca=a.sample_action(pa),cb=b.sample_action(pb);
        check(ca==cb&&pa==pb&&a.legal(ca)&&b.legal(cb)&&a.snapshot()==before_a&&b.snapshot()==before_b,"same Fog information yields identical pure policy proposals and legal answers");
        if(ca.kind==CommandKind::sell){sampled_sell=true;check(ca.target.has_value(),"sampler SELL targets alive recipients without inspecting hidden capacity");}}
    check(sampled_sell,"Fog policy can still self-sell");
    for(PlayerId target:{7u,9u,42u}){Command sale{0,CommandKind::sell,{0,1},target};
        check(a.legal(sale)&&b.legal(sale)&&a.snapshot()==before_a&&b.snapshot()==before_b,"Fog foreign SELL is legal independently of hidden recipient resources and capacity");
        for(ItemToken token:{1u,2u,3u})check(a.legal(play(0,{token},target))==b.legal(play(0,{token},target)),"ordinary Fog target legality depends only on revealed relationship and own resources");}
    check(a.legal({0,CommandKind::sell,{0,1},0})&&b.legal({0,CommandKind::sell,{0,1},0}),"self SELL remains available under Fog");
    auto clear_a=first,clear_b=first;clear_a.players[0].curses.clear();clear_b.players[0].curses.clear();clear_b.players[1].items=second.players[1].items;
    Match visible_a,visible_b;visible_a.load_scene(clear_a,0,0);visible_b.load_scene(clear_b,0,0);
    check(visible_a.observe(0)==visible_b.observe(0),"unused enemy capacity is hidden without Fog as well");
    for(std::uint64_t seed=0;seed<256;++seed){auto pa=seed,pb=seed;const auto ca=visible_a.sample_action(pa),cb=visible_b.sample_action(pb);
        check(ca==cb&&pa==pb&&visible_a.legal(ca)&&visible_b.legal(cb),"all-state sampler avoids foreign SELL capacity dependency");}
    const auto view=a.observe(0);check(view.find(fog_rule)!=std::string::npos&&view.find("last_fog")==std::string::npos&&view.find("\"status\":null")!=std::string::npos,"view declares experimental Fog schedule without exposing private selection record");
}

void fog_forced_hit_redaction_survives_reflection(){
    auto normal=scene({95},{190});normal.players[0].curses={Curse::fog};auto dark=normal;dark.players[1].curses={Curse::darkcloud};
    Match a,b;a.load_scene(normal,0,0);b.load_scene(dark,0,0);check(a.observe(0)==b.observe(0),"hidden Dark state is initially indistinguishable");
    a.step({0,CommandKind::play,{0},{}});b.step({0,CommandKind::play,{0},{}});
    check(!a.snapshot().queue[0].forced_hit&&b.snapshot().queue[0].forced_hit&&a.snapshot().rng!=b.snapshot().rng,"same public hit can come from natural percentage draw or hidden Dark forcing");
    check(a.observe(0)==b.observe(0)&&a.observe(0).find("\"forced_hit\":null")!=std::string::npos,"both true and false flags are redacted for hidden activation target");
    check(a.observe({}).find("\"forced_hit\":false")!=std::string::npos&&b.observe(7).find("\"forced_hit\":true")!=std::string::npos,"spectator and original recipient can inspect their visible flag");roundtrip(a);roundtrip(b);
    a.step(defend(7,{0}));b.step(defend(7,{0}));check(a.snapshot().actor==0&&a.observe(0)==b.observe(0)&&a.observe(0).find("\"forced_hit\":null")!=std::string::npos,"reflection back to visible self must not disclose hidden initial-target Dark forcing");roundtrip(a);roundtrip(b);
}

void fog_application_cures_journal_and_atomicity(){
    for(CardId model:{CardId{72},CardId{97}}){auto state=scene({model},{});if(model==97)state.players[1].curses={Curse::darkcloud};MatchConfig cap;cap.max_decisions=2;auto m=fixture(state,cap);
        m.step(model==97?Command{0,CommandKind::play,{0},{}}:play(0,{0},7));m.step(defend(7));const auto& p=m.snapshot().state.players[1];
        check(p.hp==37&&std::find(p.curses.begin(),p.curses.end(),Curse::fog)!=p.curses.end(),"Fog damage weapons apply on actual HP loss including forced percentage hit");roundtrip(m);}
    auto blocked=fixture(scene({72},{image("rainbow-curtain"),176}));blocked.step(play(0,{0},7));blocked.step(defend(7,{0,1}));check(blocked.snapshot().state.players[1].hp==40&&blocked.snapshot().state.players[1].curses.empty(),"fully defended Fog weapon adds no curse");roundtrip(blocked);
    auto state=scene({6},{182});state.players[0].curses=state.players[1].curses={Curse::fog};MatchConfig cap;cap.max_decisions=3;Match counter(cap);counter.load_scene(state,0,0);
    counter.step(play(0,{0},7));const auto rng=counter.snapshot().rng;counter.step(defend(7,{0}));
    check(counter.snapshot().queue[0].is_counter&&counter.snapshot().queue[0].target==0&&counter.snapshot().queue[0].ability==Ability::counter_curse
        &&counter.snapshot().rng==rng&&counter.snapshot().last_fog->target==7,"Fog counter does not reselect even when counter owner is Fogged");roundtrip(counter);counter.step(defend(0));
    check(counter.snapshot().state.players[0].hp==40&&counter.snapshot().state.players[0].curses==std::vector<Curse>{Curse::fog}&&counter.snapshot().rng==rng&&!counter.snapshot().last_tick,"Fog reapplication is idempotent and has no disease tick or HP damage");roundtrip(counter);
    MatchConfig one;one.max_decisions=1;auto heaven=scene({221},{});heaven.players[0].curses={Curse::heaven};auto self=fixture(heaven,one);self.step(play(0,{0},0));
    check(self.snapshot().state.players[0].hp==40&&self.snapshot().state.players[0].mp==7&&self.snapshot().state.players[0].curses==std::vector<Curse>{Curse::heaven,Curse::fog}&&!self.snapshot().last_tick,"Fog miracle is independent of Heaven reapplication and cutoff skips disease tick");roundtrip(self);
    for(CardId model:{CardId{237},CardId{238}}){auto st=scene({model},{});st.players[0].curses={Curse::hell,Curse::fog,Curse::flash,Curse::darkcloud};auto cure=fixture(st,one);cure.step(play(0,{0},0));
        check(cure.snapshot().state.players[0].curses==(model==237?std::vector<Curse>{Curse::hell,Curse::darkcloud}:std::vector<Curse>{}),"four canonical curses support mild Fog removal and complete cure");roundtrip(cure);}
    auto reflected_cure=scene({237},{190});reflected_cure.players[0].curses={Curse::fog};auto cured=fixture(reflected_cure);cured.step(play(0,{0},7));const auto fog_event=cured.snapshot().last_fog;
    reject(cured,pray(0));cured.step(defend(7,{0}));cured.step(defend(0));
    check(cured.snapshot().state.players[0].curses.empty()&&cured.snapshot().last_fog==fog_event,"reflected cure can remove source Fog while the same-action selection record remains valid");roundtrip(cured);
    auto pure=scene({235},{});pure.players[0].items[0].used=true;pure.players[0].curses={Curse::fog};MatchConfig packed;packed.hand_limit=packed.initial_hand=1;Match no_tick(packed);no_tick.load_scene(pure,0,0);no_tick.step(pray(0));
    check(no_tick.snapshot().rng==0&&no_tick.snapshot().state.players[0].hp==40&&!no_tick.snapshot().last_tick&&no_tick.snapshot().owner==7,"Fog alone has no natural action-end worsening draw or HP tick");roundtrip(no_tick);
    auto late=scene({6},{});late.players[0].curses={Curse::fog};late.update_count=max_wire_integer;Match exhausted;exhausted.load_scene(late,0,0);const auto before=exhausted.snapshot();bool failed=false;
    try{exhausted.step(play(0,{0},7));}catch(const MatchError&){failed=true;}check(failed&&exhausted.snapshot()==before,"late launch failure rolls back Fog target draw, record, consumption and frame");
    late.update_count=0;late.players[0].items[0].token=max_wire_integer;Match gifts;gifts.load_scene(late,0,0);gifts.step(play(0,{max_wire_integer},7));const auto pending=gifts.snapshot();failed=false;
    try{gifts.step(defend(7));}catch(const MatchError&){failed=true;}check(failed&&gifts.snapshot()==pending,"late gift failure preserves already selected Fog target, RNG and pending journal");roundtrip(gifts);
}

void death_main_rate_copies_and_target_roster(){
    check(match_supported(112),"112 is admitted with both normal and posthumous roles");
    auto normal=scene({112},{});normal.players[1].curses={Curse::darkcloud};MatchConfig cap;cap.max_decisions=2;Match main(cap);main.load_scene(normal,0,0);
    main.step({0,CommandKind::play,{0},{}});check(main.snapshot().queue[0].power==1&&main.snapshot().queue[0].hit_rate==25
        &&main.snapshot().queue[0].element==Element::light&&!main.snapshot().queue[0].is_death&&main.snapshot().rng==0,"ordinary112 is a targetless Light1/25 percent weapon, distinct from its death payload");roundtrip(main);
    reject(main,play(0,{0},7));main.step(defend(7));check(main.snapshot().state.players[1].hp==39&&main.snapshot().death_created==0,"ordinary112 body does not create death work without an actual death");roundtrip(main);
    for(const auto trial:{std::pair<std::uint64_t,std::uint64_t>{0,0},{2,1}}){MatchConfig one;one.max_decisions=1;Match rolled(one);rolled.load_scene(scene({112},{}),0,trial.first);rolled.step({0,CommandKind::play,{0},{}});
        check(rolled.snapshot().chance_rolls==1&&rolled.snapshot().chance_hits==trial.second&&rolled.snapshot().rng==trial.first+0x9e3779b97f4a7c15ULL&&rolled.snapshot().death_created==0,"ordinary112 draws at25 percent once before launch cutoff");roundtrip(rolled);}
    SnapshotState unforced;unforced.players={player(0,{64}),player(7,{112}),player(42)};unforced.players[0].hp=unforced.players[2].hp=99;Match miss;miss.load_scene(unforced,0,4);miss.step(play(0,{0},7));miss.step(defend(7));
    check(miss.snapshot().actor==42&&miss.snapshot().death_rolls==2&&miss.snapshot().death_hits==1&&miss.snapshot().death_started==1&&!miss.snapshot().queue[0].forced_hit
        &&miss.snapshot().rng==4+2*0x9e3779b97f4a7c15ULL,"death75 rolls78 miss then4 hit in input target order, separately from main25");roundtrip(miss);
    SnapshotState state;state.players={player(0,{64}),player(7,{112,112},Team::one),player(9,{},Team::one),player(42,{},Team::two),player(55,{},Team::two)};
    for(auto seat:{0u,3u}){state.players[seat].hp=99;state.players[seat].curses={Curse::darkcloud};}state.players[4].dead=true;state.players[4].hp=0;
    Match m;m.load_scene(state,0,0);m.step(play(0,{0},7));m.step(defend(7));const auto start=m.snapshot();
    check(start.state.players[1].dead&&start.state.players[1].items.empty()&&start.death_created==4&&start.deaths.size()==3&&start.gifts[1]==0
        &&start.queue[0].is_death&&!start.queue[0].is_counter&&start.queue[0].origin==7&&start.queue[0].source==7&&start.queue[0].target==0
        &&start.queue[0].power==30&&start.queue[0].hit_rate==75&&start.queue[0].element==Element::light&&start.queue[0].tokens==std::vector<ItemToken>{0}
        &&start.deaths[0].target==42&&start.deaths[1].tokens==std::vector<ItemToken>{1}&&start.deaths[1].target==0,
        "all copies are consumed without gifts, copy-then-live-enemy roster excludes allies and dead targets");roundtrip(m);
    for(PlayerId target:{0u,42u,0u,42u}){check(m.snapshot().actor==target,"posthumous FIFO follows copy and input seat order");m.step(defend(target));roundtrip(m);}
    check(m.snapshot().state.players[0].hp==39&&m.snapshot().state.players[3].hp==39&&m.snapshot().state.players[2].hp==40
        &&m.snapshot().death_started==4&&m.snapshot().death_forced_hits==4&&m.snapshot().death_rolls==0&&m.snapshot().owner==9,
        "dead origin continues all copies while ordinary owner/next-seat and team effects remain distinct");
    const auto public_view=start;Match visible;visible.restore(public_view);const auto json=visible.observe(0);
    check(json.find(death_rule)!=std::string::npos&&json.find("\"death\":true")!=std::string::npos&&json.find("death_created")==std::string::npos
        &&json.find("tick_processed")==std::string::npos&&json.find("\"deaths\"")==std::string::npos,"only active death flag and payload are public, not future FIFO/counters/journal");
}

void death_redirects_counters_and_origin_death_chains(){
    SnapshotState state;state.players={player(0,{64,190}),player(7,{112}),player(42)};state.players[0].hp=state.players[2].hp=99;
    state.players[0].curses=state.players[2].curses={Curse::darkcloud};auto mirror=fixture(state);mirror.step(play(0,{0},7));mirror.step(defend(7));const auto rng=mirror.snapshot().rng;
    mirror.step(defend(0,{1}));check(mirror.snapshot().actor==42&&mirror.snapshot().queue[0].is_death&&mirror.snapshot().queue[0].origin==7
        &&mirror.snapshot().death_started==2&&mirror.snapshot().rng==rng,"reflection back to dead origin ends that frame immediately and never asks a dead actor to respond");roundtrip(mirror);mirror.step(defend(42));roundtrip(mirror);
    for(CardId ring:{CardId{187},CardId{188}}){state.players[0]=player(0,{64,189,ring});state.players[0].hp=99;state.players[0].mp=0;state.players[0].curses={Curse::darkcloud};auto m=fixture(state);
        m.step(play(0,{0},7));m.step(defend(7));m.step(defend(0,{1,2}));check(m.snapshot().actor==42&&m.snapshot().queue[0].is_death&&m.snapshot().counter_created==1,"reserved normal counter waits behind remaining death FIFO");roundtrip(m);m.step(defend(42));
        if(ring==187){check(m.snapshot().actor==0&&!m.snapshot().queue[0].is_death&&m.snapshot().queue[0].is_counter&&m.snapshot().queue[0].amount==60,"surviving defender self-MP counter can follow a dead-source attack");roundtrip(m);m.step(defend(0));check(m.snapshot().state.players[0].mp==60,"normal MP counter applies exactly once");}
        else check(m.snapshot().phase==MatchPhase::main&&m.snapshot().counter_started==0,"normal CP counter aimed at dead incoming source still skips");roundtrip(m);}
    state.players[0]=player(0,{64,189,21});state.players[0].hp=99;state.players[0].curses={Curse::darkcloud};Match bounce;bounce.load_scene(state,0,0);bounce.step(play(0,{0},7));bounce.step(defend(7));bounce.step(defend(0,{1,2}));
    check(bounce.snapshot().actor==42&&bounce.snapshot().queue[0].source==0&&bounce.snapshot().queue[0].origin==7&&bounce.snapshot().queue[0].is_death
        &&bounce.snapshot().queue[0].hit_rate==75&&bounce.snapshot().rng==0x9e3779b97f4a7c15ULL,"neutralized death payload bounces only among alive seats without rerolling its percentage or origin");roundtrip(bounce);
    SnapshotState chain;chain.players={player(0,{64,112}),player(7,{112}),player(42),player(55)};chain.players[0].hp=10;
    for(auto& p:chain.players){p.curses={Curse::darkcloud};if(p.id>=42)p.hp=99;}auto m=fixture(chain);m.step(play(0,{0},7));m.step(defend(7));
    for(PlayerId target:{0u,42u,55u,42u,55u}){check(m.snapshot().actor==target&&m.snapshot().queue[0].is_death,"new actual-death work appends behind the existing death roster even after original owner dies");m.step(defend(target));roundtrip(m);}
    check(m.snapshot().state.players[0].dead&&m.snapshot().state.players[1].dead&&m.snapshot().state.players[2].hp==39&&m.snapshot().state.players[3].hp==39
        &&m.snapshot().death_created==5&&m.snapshot().death_forced_hits==5&&m.snapshot().owner==42,"posthumous chain survives owner death while original normal roster is canceled");
}

void death_general_main_and_single_owner_tick_journals(){
    SnapshotState state;state.players={player(0,{3,112}),player(7),player(42)};for(auto& p:state.players)if(p.id){p.hp=99;p.curses={Curse::darkcloud};}
    auto exchange=fixture(state);exchange.step({0,CommandKind::exchange,{0},{},ResourceAllocation{0,5,65}});
    check(exchange.snapshot().state.players[0].dead&&exchange.snapshot().queue[0].is_death&&exchange.snapshot().batch->attack.kind==CommandKind::exchange
        &&exchange.snapshot().batch->targets.empty()&&!exchange.snapshot().batch->tick_processed,"HP0 exchange has a general pre-state journal even without an original attack roster");roundtrip(exchange);
    exchange.step(defend(7));exchange.step(defend(42));check(exchange.snapshot().owner==7&&!exchange.snapshot().last_tick,"dead exchange owner has no later disease tick");roundtrip(exchange);
    state.players[0]=player(0,{118,112});state.players[0].hp=1;state.players[0].curses={Curse::cold};Match tick;tick.load_scene(state,0,0);tick.step({0,CommandKind::discard,{0},{}});
    const auto pending=tick.snapshot();check(pending.last_tick&&pending.last_tick->hp_after==0&&pending.batch->tick_processed&&pending.queue[0].is_death
        &&pending.batch->attack.kind==CommandKind::discard&&pending.death_created==2&&pending.rng==0x9e3779b97f4a7c15ULL,"discard-triggered owner tick can die and suspend in death response with exactly one worsening draw");roundtrip(tick);
    tick.step(defend(7));tick.step(defend(42));check(tick.snapshot().rng==pending.rng&&tick.snapshot().last_tick==pending.last_tick&&tick.snapshot().owner==7,"post-tick death responses never tick the dead owner again");roundtrip(tick);
    MatchConfig cap;cap.max_decisions=2;Match cut(cap);cut.load_scene(state,0,0);cut.step({0,CommandKind::discard,{0},{}});cut.step(defend(7));
    check(cut.snapshot().truncated&&cut.snapshot().last_tick&&cut.snapshot().deaths.empty()&&cut.snapshot().death_started==1,"later cutoff preserves completed owner tick while canceling unresolved posthumous work");roundtrip(cut);
    bool broom_seen=false;for(std::uint64_t seed=0;seed<64&&!broom_seen;++seed){auto broom_state=state;broom_state.players[0]=player(0,{205,112,118,118,118,118});broom_state.players[0].hp=1;broom_state.players[0].curses={Curse::cold};Match broom;broom.load_scene(broom_state,0,seed);broom.step(play(0,{0},0));
        if(broom.snapshot().death_bows){broom_seen=true;check(broom.snapshot().last_removal&&broom.snapshot().last_removal->removed_models.size()==3&&broom.snapshot().state.players[0].items.size()==1
            &&broom.snapshot().last_tick&&broom.snapshot().batch->tick_processed&&broom.snapshot().queue[0].is_death,"self broom event excludes later owner-tick bow consumption while both records persist through death response");roundtrip(broom);}}
    check(broom_seen,"seeded broom can leave112 for the later owner tick");
    state.players[0]=player(0,{202,112});state.players[0].hp=5;Match random;random.load_scene(state,0,0);random.step(play(0,{0},0));
    check(random.snapshot().state.players[0].dead&&random.snapshot().batch->random_hp_delta==-10&&random.snapshot().queue[0].is_death,"self random-HP lethal body retains sign and full immediate-action journal");roundtrip(random);
    state.players[0]=player(0,{4,27,112});state.players[0].hp=1;state.players[0].mp=state.players[0].cp=0;auto sale=fixture(state);sale.step({0,CommandKind::sell,{0,1},0});
    check(sale.snapshot().last_sale&&sale.snapshot().phase==MatchPhase::response&&sale.snapshot().state.players[0].dead&&sale.snapshot().queue[0].is_death,"self sale debt can retain public invoice while dead buyer starts posthumous responses");roundtrip(sale);
}

void death_natural_cutoff_revival_and_simultaneous_boundaries(){
    auto duel=fixture(scene({64},{112}));duel.step(play(0,{0},7));duel.step(defend(7));
    check(duel.snapshot().terminated&&duel.snapshot().winners==std::vector<PlayerId>{0}&&duel.snapshot().state.players[1].items.size()==1&&duel.snapshot().death_created==0,"natural duel outcome precedes bow consumption and does not cause a speculative posthumous draw");roundtrip(duel);
    SnapshotState state;state.players={player(0,{64}),player(7,{112,208}),player(42)};auto saved=fixture(state);saved.step(play(0,{0},7));saved.step(defend(7));
    check(!saved.snapshot().state.players[1].dead&&saved.snapshot().state.players[1].hp==10&&saved.snapshot().death_created==0&&saved.snapshot().revivals==1
        &&std::any_of(saved.snapshot().state.players[1].items.begin(),saved.snapshot().state.players[1].items.end(),[](const auto& i){return i.model==112;}),"208 revival occurs first and leaves surviving112 untriggered");roundtrip(saved);
    state.players[1]=player(7,{112});MatchConfig cap;cap.max_decisions=2;Match cut(cap);cut.load_scene(state,0,0);cut.step(play(0,{0},7));cut.step(defend(7));
    check(cut.snapshot().truncated&&cut.snapshot().state.players[1].items.empty()&&cut.snapshot().death_created==2&&cut.snapshot().death_started==0
        &&cut.snapshot().death_rolls==0&&cut.snapshot().rng==0&&cut.snapshot().deaths.empty(),"nonterminal body reserves/consumes112 before cutoff cancels activation without RNG or response");roundtrip(cut);
    SnapshotState pair;pair.players={player(0,{53,112}),player(7,{112}),player(42),player(55)};pair.players[0].hp=pair.players[1].hp=1;
    for(auto seat:{2u,3u}){pair.players[seat].hp=99;pair.players[seat].curses={Curse::darkcloud};}auto both=fixture(pair);both.step(play(0,{0},7));both.step(defend(7));
    check(both.snapshot().death_created==4&&both.snapshot().queue[0].origin==0&&both.snapshot().deaths[1].origin==7&&both.snapshot().state.players[0].dead&&both.snapshot().state.players[1].dead,
        "simultaneous self-damage and target death finalize all flags before input-seat posthumous roster collection");roundtrip(both);
    state.players[1].dead=true;state.players[1].hp=0;auto loaded=fixture(state);check(loaded.snapshot().death_created==0&&loaded.snapshot().state.players[1].items.size()==1,"loading an already dead scene never replays an earlier death transition");roundtrip(loaded);
}

void death_snapshot_adversaries_and_atomic_late_failures(){
    SnapshotState state;state.players={player(0,{64}),player(7,{112,112}),player(42)};for(auto seat:{0u,2u}){state.players[seat].hp=99;state.players[seat].curses={Curse::darkcloud};}
    auto m=fixture(state);m.step(play(0,{0},7));m.step(defend(7));const auto exact=m.snapshot();
    for(int error=0;error<16;++error){auto bad=exact;
        if(error==0)bad.queue[0].is_death=false;if(error==1)bad.queue[0].power=1;if(error==2)bad.queue[0].hit_rate=25;if(error==3)bad.queue[0].source=0;
        if(error==4)bad.queue[0].tokens[0]=999;if(error==5)bad.deaths.clear();if(error==6)std::swap(bad.deaths[0],bad.deaths[1]);if(error==7)++bad.death_created;
        if(error==8)--bad.death_forced_hits;if(error==9)bad.rng^=1;if(error==10)++bad.batch->death_created;if(error==11)bad.batch->tick_processed=true;
        if(error==12)bad.batch->attack.kind=CommandKind::pray;if(error==13)bad.batch->initial_state.players[1].items.pop_back();if(error==14)bad.queue[0].is_counter=true;
        if(error==15)bad.death_created=player_capacity*(player_capacity-1)*bad.config.hand_limit+1;
        bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"full death replay rejects flag/rate/power/consumed-copy/roster/RNG/anchor/stage/stat forgery");}
    auto late=state;late.update_count=max_wire_integer-1;auto update=fixture(late);update.step(play(0,{0},7));const auto before=update.snapshot();bool failed=false;
    try{update.step(defend(7));}catch(const MatchError&){failed=true;}check(failed&&update.snapshot()==before,"late update exhaustion rolls back victim death, all bow consumption, death FIFO and activation RNG/stats");roundtrip(update);
    state.players[1]=player(7,{112});state.players[0].items[0].token=max_wire_integer;auto gifts=fixture(state);gifts.step(play(0,{max_wire_integer},7));gifts.step(defend(7));gifts.step(defend(0));const auto pending=gifts.snapshot();failed=false;
    try{gifts.step(defend(42));}catch(const MatchError&){failed=true;}check(failed&&gifts.snapshot()==pending,"late gift token failure restores last posthumous body, queue, counters, owner and RNG");roundtrip(gifts);
}

void death_nine_seat_large_fifo_and_natural_cancellation(){
    SnapshotState state;for(unsigned seat=0;seat<9;++seat){auto p=player(seat?100+seat:0);p.hp=1;p.curses={Curse::darkcloud};
        for(ItemToken token=0;token<18;++token)p.items.push_back({token,seat==0&&token==0?CardId{64}:CardId{112},{},false});state.players.push_back(std::move(p));}
    Match m;m.load_scene(state,0,0);m.step(play(0,{0},101));m.step(defend(101));check(m.snapshot().death_created==144&&m.snapshot().deaths.size()==143,"18 physical copies expand to144 frames, above old32 limits");roundtrip(m);
    for(PlayerId target:{0u,102u,103u,104u,105u,106u,107u}){check(m.snapshot().actor==target&&m.snapshot().queue[0].is_death,"large death FIFO skips dead targets and remains active after owner death");m.step(defend(target));roundtrip(m);}
    check(m.snapshot().terminated&&m.snapshot().winners==std::vector<PlayerId>{108}&&m.snapshot().death_created==623&&m.snapshot().death_started==7
        &&m.snapshot().death_forced_hits==7&&m.snapshot().death_rolls==0&&m.snapshot().rng==0&&m.snapshot().deaths.empty()
        &&m.snapshot().state.players[7].items.size()==18,"last natural outcome cancels remaining FIFO and leaves final-terminal death bows unconsumed without extra rolls");
}

void trade_overflow_inventory_order_types_and_gift_accounting(){
    constexpr std::uint64_t draw=0x9e3779b97f4a7c15ULL;const std::array<std::uint64_t,6> seeds{19,0,5,3,2,1};
    const std::array<CardId,6> models{235,208,209,112,118,118};
    MatchConfig cap;cap.initial_hand=cap.hand_limit=6;cap.max_decisions=2;
    for(std::size_t slot=0;slot<models.size();++slot){auto state=scene({4,6},{});state.players[1].hp=99;
        for(ItemToken n=0;n<6;++n)state.players[1].items.push_back({10+n,models[n],{},n==0});Match m(cap);m.load_scene(state,0,seeds[slot]);
        const Command sale{0,CommandKind::sell,{0,1},7};check(m.legal(sale),"full recipients are legal regardless of protected passives or retained miracles");m.step(sale);roundtrip(m);m.step(defend(7));
        const auto& result=m.snapshot();const auto& target=result.state.players[1];
        check(result.last_sale->overflow_model==models[slot]&&result.rng==seeds[slot]+draw&&target.items.size()==6&&target.items.back().model==6
            &&target.items.back().token==16&&!target.items.back().used&&std::none_of(target.items.begin(),target.items.end(),[&](const auto& item){return item.token==10+slot;}),
            "one full-delivery bounded draw selects inventory-order existing copy, then adds a fresh unused commodity");
        check(target.hp==99&&target.mp==10&&target.cp==19&&result.revivals==0&&result.death_bows==0&&result.death_created==0,"evicted235/208/209/112 never invokes its ability or a replacement gift");
        const auto board=project_view(result.state,7);check(board.players[1].used_models==(slot==0?std::vector<CardId>{}:std::vector<CardId>{235}),"used miracle eviction updates its separate public pane");
        check(m.observe({}).find("\"overflow_model\":"+std::to_string(models[slot]))!=std::string::npos&&m.observe(0).find("eviction_index")==std::string::npos,"overflow model is public without an inventory index");roundtrip(m);}
    auto sparse=scene({4,6},{118});Match no_overflow(cap);no_overflow.load_scene(sparse,0,0);no_overflow.step({0,CommandKind::sell,{0,1},7});no_overflow.step(defend(7));
    check(!no_overflow.snapshot().last_sale->overflow_model&&no_overflow.snapshot().rng==0&&no_overflow.observe({}).find("\"overflow_model\":null")!=std::string::npos,"nonfull delivery takes no overflow draw and declares a nullable result");roundtrip(no_overflow);
    cap.max_decisions=512;auto full=scene({4,6},{118,118,118,118,118,118});Match gifts(cap);gifts.load_scene(full,0,0);gifts.step({0,CommandKind::sell,{0,1},7});gifts.step(defend(7));
    check(gifts.snapshot().state.players[0].items.size()==1&&gifts.snapshot().state.players[1].items.size()==6&&gifts.snapshot().rng==2*draw,"overflow adds no replacement; only the operation's deferred gift follows the eviction draw");roundtrip(gifts);
    MatchConfig redirect_cap;redirect_cap.initial_hand=redirect_cap.hand_limit=2;redirect_cap.max_decisions=3;auto redirect_state=scene({4,6},{190,118});Match redirect(redirect_cap);redirect.load_scene(redirect_state,0,0);
    redirect.step({0,CommandKind::sell,{0,1},7});redirect.step(defend(7,{0}));redirect.step(defend(0));
    check(redirect.snapshot().last_sale->target==0&&!redirect.snapshot().last_sale->overflow_model&&redirect.snapshot().rng==0,"initially full buyer can reflect; overflow is chosen only at the actual final recipient after responses");roundtrip(redirect);
}

void trade_overflow_debt_eviction_revival_and_death_order(){
    MatchConfig cap;cap.initial_hand=cap.hand_limit=2;cap.max_decisions=2;
    for(std::uint64_t seed:{0u,2u}){SnapshotState state;state.players={player(0,{4,27}),player(7,{208,112}),player(42)};state.players[1].hp=1;state.players[1].mp=state.players[1].cp=0;
        Match m(cap);m.load_scene(state,0,seed);m.step({0,CommandKind::sell,{0,1},7});m.step(defend(7));const auto& s=m.snapshot();
        check(s.state.players[0].cp==30&&s.last_sale->price==10&&s.last_sale->overflow_model==(seed==0?112:208),"CP debt and full source credit precede the selected overflow event");
        if(seed==0)check(!s.state.players[1].dead&&s.state.players[1].hp==10&&s.revivals==1&&s.death_bows==0,"evicted112 never attacks; surviving208 rescues after delivery");
        else check(s.state.players[1].dead&&s.revivals==0&&s.death_bows==1&&s.death_created==2&&s.death_started==0,"evicting208 prevents rescue and remaining112 is reserved only at actual death, before cutoff");roundtrip(m);}
    for(CardId product:{CardId{208},CardId{112}}){SnapshotState state;state.players={player(0,{4,product}),player(7,{208,118}),player(42)};state.players[1].hp=1;state.players[1].mp=state.players[1].cp=0;Match m(cap);m.load_scene(state,0,2);
        m.step({0,CommandKind::sell,{0,1},7});m.step(defend(7));const auto& s=m.snapshot();check(s.last_sale->overflow_model==208,"old208 can be evicted before a different fresh passive arrives");
        if(product==208)check(s.revivals==1&&!s.state.players[1].dead&&s.state.players[1].hp==10,"newly delivered208 participates in rescue after eviction");
        else check(s.revivals==0&&s.state.players[1].dead&&s.death_bows==1&&s.death_created==2,"newly delivered112 participates in actual death after eviction");roundtrip(m);}
}

void fog_sell_launch_commodity_roles_reflections_and_overflow(){
    constexpr std::uint64_t draw=0x9e3779b97f4a7c15ULL;MatchConfig cap;cap.initial_hand=cap.hand_limit=2;cap.max_decisions=2;
    for(CardId product:{CardId{232},CardId{94},CardId{112},CardId{14},CardId{202},CardId{198},CardId{208}}){SnapshotState state;state.players={player(0,{4,product},Team::one),player(7,{},Team::two),player(9,{},Team::one),player(42,{208,118},Team::two)};
        state.players[0].curses={Curse::fog};state.players[3].cp=99;state.players[3].hp=99;Match m(cap);m.load_scene(state,0,0);m.step({0,CommandKind::sell,{0,1},7});
        check(m.snapshot().actor==42&&m.snapshot().batch->targets==std::vector<PlayerId>{42}&&m.snapshot().last_fog==FogTarget{0,7,42,{4,product}}
            &&m.snapshot().rng==draw&&m.snapshot().state.players[0].mp==10,"Fog SELL selects once from enemies and never casts commodity Wide/chance/Danger/repeat/utility abilities");roundtrip(m);
        m.step(defend(42));check(m.snapshot().last_sale->target==42&&m.snapshot().last_sale->model==product&&m.snapshot().last_sale->overflow_model==208
            &&m.snapshot().rng==2*draw&&m.snapshot().last_fog->target==42&&m.snapshot().state.players[3].hp==99&&m.snapshot().state.players[3].curses.empty(),"selected full buyer takes one independent overflow draw after Fog, without commodity side effects");roundtrip(m);}
    for(PlayerId target:{0u,9u}){SnapshotState state;state.players={player(0,{4,232},Team::one),player(7),player(9,{},Team::one)};state.players[0].curses={Curse::fog};auto direct_cap=cap;direct_cap.max_decisions=target==0?1:2;Match m(direct_cap);m.load_scene(state,0,0);m.step({0,CommandKind::sell,{0,1},target});
        check(!m.snapshot().last_fog&&m.snapshot().rng==0,"Fog leaves self and ally SELL hints unchanged without a draw");if(target==9)m.step(defend(9));roundtrip(m);}
    auto state=scene({4,27,190},{});state.players.push_back(player(42,{190}));state.players[0].curses={Curse::fog};MatchConfig limit;limit.max_decisions=5;Match reflection(limit);reflection.load_scene(state,0,0);
    reflection.step({0,CommandKind::sell,{0,1},7});reflection.step(defend(42,{0}));roundtrip(reflection);reflection.step(defend(0,{2}));roundtrip(reflection);reflection.step(defend(42));
    check(reflection.snapshot().last_sale->source==0&&reflection.snapshot().last_sale->target==42&&!reflection.snapshot().last_sale->overflow_model
        &&reflection.snapshot().last_fog->target==42,"reflections preserve Fog launch selection and deliver only at the final nonfull buyer");roundtrip(reflection);
}

void overflow_event_full_replay_and_late_atomic_failures(){
    MatchConfig cap;cap.initial_hand=cap.hand_limit=2;SnapshotState state;state.players={player(0,{4,27}),player(7,{208,112}),player(42)};state.players[1].hp=1;state.players[1].mp=state.players[1].cp=0;
    for(auto seat:{0u,2u}){state.players[seat].hp=99;state.players[seat].curses={Curse::darkcloud};}Match m(cap);m.load_scene(state,0,2);m.step({0,CommandKind::sell,{0,1},7});m.step(defend(7));const auto exact=m.snapshot();
    check(exact.phase==MatchPhase::response&&exact.queue[0].is_death&&exact.last_sale->overflow_model==208,"resolved overflow event can coexist with later posthumous responses");roundtrip(m);
    for(int error=0;error<7;++error){auto bad=exact;if(error==0)bad.last_sale->overflow_model=112;if(error==1)bad.last_sale->overflow_model.reset();if(error==2)bad.last_sale->overflow_model=207;
        if(error==3)bad.rng^=1;if(error==4)bad.batch->rng^=1;if(error==5)std::swap(bad.batch->initial_state.players[1].items[0],bad.batch->initial_state.players[1].items[1]);if(error==6)bad.last_sale->overflow_model=0;
        bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"full replay rejects invented overflow result, inventory-order/RNG anchor and unsupported model atomically");}
    state.players[1]=player(7,{118,118});state.update_count=max_wire_integer-1;Match update(cap);update.load_scene(state,0,2);update.step({0,CommandKind::sell,{0,1},7});const auto before=update.snapshot();bool failed=false;
    try{update.step(defend(7));}catch(const MatchError&){failed=true;}check(failed&&update.snapshot()==before,"late update failure rolls back CP, eviction RNG/copy, fresh delivery, public result and death work");roundtrip(update);
    state.update_count=0;state.players[0].items[0].token=max_wire_integer-1;Match gifts(cap);gifts.load_scene(state,0,2);gifts.step({0,CommandKind::sell,{max_wire_integer-1,1},7});const auto pending=gifts.snapshot();failed=false;
    try{gifts.step(defend(7));}catch(const MatchError&){failed=true;}check(failed&&gifts.snapshot()==pending,"deferred gift exhaustion after a successful full delivery restores eviction/result/fresh token and RNG together");roundtrip(gifts);
    auto ordinary=fixture(scene({5},{6}));ordinary.step({0,CommandKind::buy,{0},7});ordinary.step(defend(7));ordinary.step({0,CommandKind::accept,{},{}});
    check(ordinary.snapshot().last_purchase&&!ordinary.snapshot().last_purchase->overflow_model&&ordinary.observe({}).find("\"overflow_model\":null")!=std::string::npos,"reachable BUY retains no-overflow result; generic full delivery helper is shared without fabricating a full quote");roundtrip(ordinary);
    for(std::uint32_t limit:{2u,3u}){auto stock=scene({5},{});for(ItemToken n=0;n<18;++n)stock.players[1].items.push_back({10+n,118,{},false});MatchConfig return_cap;return_cap.max_decisions=limit;Match returned(return_cap);returned.load_scene(stock,0,0);
        returned.step({0,CommandKind::buy,{0},7});returned.step(defend(7));if(limit==3)returned.step({0,CommandKind::decline,{},{}});
        check(returned.snapshot().state.players[1].items==stock.players[1].items&&!returned.snapshot().last_purchase->overflow_model&&returned.snapshot().rng==0x9e3779b97f4a7c15ULL
            &&returned.snapshot().last_purchase->reason==(limit==2?PurchaseReason::cutoff:PurchaseReason::declined),"decline or unresolved-cutoff return restores full seller's original slot/token without eviction or an extra draw");roundtrip(returned);}

}

void trade_capacity_queries_and_sampler_use_only_visible_inputs(){
    auto sparse=scene({4,6,232,5},{235});sparse.players[1].items[0].used=true;auto full=sparse;
    for(ItemToken token=1;token<18;++token)full.players[1].items.push_back({token,118,{},false});Match a,b;a.load_scene(sparse,0,0);b.load_scene(full,0,0);
    check(a.observe(0)==b.observe(0),"foreign unused capacity is hidden while used miracle pane is identical");const auto sa=a.snapshot(),sb=b.snapshot();std::set<PlayerId> sell_targets;
    for(std::uint64_t seed=0;seed<512;++seed){auto pa=seed,pb=seed;const auto ca=a.sample_action(pa),cb=b.sample_action(pb);
        check(ca==cb&&pa==pb&&a.legal(ca)&&b.legal(cb)&&a.snapshot()==sa&&b.snapshot()==sb,"all-state policy stream and legality answers do not inspect hidden recipient capacity");if(ca.kind==CommandKind::sell)sell_targets.insert(*ca.target);}
    check(sell_targets==std::set<PlayerId>{0,7},"SELL sampler reaches self and full foreign recipients without filtering by hidden counts");
    for(ItemToken product:{1u,2u})for(PlayerId target:{0u,7u}){Command sale{0,CommandKind::sell,{0,product},target};check(a.legal(sale)&&b.legal(sale),"explicit foreign and self SELL remain legal for every hidden capacity");}
    a.step({0,CommandKind::sell,{0,1},7});b.step({0,CommandKind::sell,{0,1},7});check(a.observe(0)==b.observe(0)&&a.snapshot().rng==b.snapshot().rng,"accepted foreign launch still does not reveal whether final delivery will overflow");roundtrip(a);roundtrip(b);
    check(a.observe(0).find(overflow_rule)!=std::string::npos,"public view exposes the experimental overflow rule");
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
void dream_compatibility_predicates_and_payloads(){
    const auto pool=match_catalog();std::size_t pairs=0,eligible=0;
    const std::array<std::vector<VisibleSelection>,5> prefixes{{{},{{143,false}},{{187,false}},{{6,false}},{{190,false}}}};
    for(auto actual:pool){bool any=false;for(auto face:pool)if(match_dream_compatible(actual,face)){
        any=true;++pairs;check(actual!=face&&match_dream_compatible(face,actual),"Dream relation symmetric and irreflexive");
        check(can_start_main(actual)==can_start_main(face)&&targets_enemy_by_default(actual)==targets_enemy_by_default(face),"compatible faces preserve start/default target predicates");
        const std::array<CardId,1> a{actual},f{face};check(magic_cost(a)==magic_cost(f),"compatible masks preserve quoted MP selection budget");
        for(bool used:{false,true})for(const auto& prefix:prefixes){
            check(can_append_main(prefix,{actual,used})==can_append_main(prefix,{face,used}),"main append cannot distinguish compatible hidden real models");
            for(unsigned category=0;category<8;++category)for(unsigned element=0;element<7;++element)for(bool attack:{false,true})for(bool flash:{false,true}){
                ResponseContext c{flash,static_cast<Category>(category),element?Attribute{static_cast<Element>(element-1)}:Attribute{},attack};
                check(can_append_response(prefix,{actual,used},c)==can_append_response(prefix,{face,used},c),"response append equivalent across all compatible pairs and public contexts");
            }
        }
        for(auto next:pool){const std::array<VisibleSelection,1> aselected{{{actual,false}}},fselected{{{face,false}}};
            check(can_append_main(aselected,{next,false})==can_append_main(fselected,{next,false}),"selected compatible base preserves main extension predicate");}
    }eligible+=any;}
    check(pairs==992&&eligible==146&&pool.size()-eligible==90,"runtime Dream partition matches pinned public-client oracle");
    check(match_dream_compatible(6,57)&&find_card(6)->atk!=find_card(57)->atk&&find_card(6)->price!=find_card(57)->price,"predicate equivalence deliberately does not imply equal payload or price");
    check(!match_dream_compatible(6,6)&&!match_dream_compatible(6,178)&&!match_dream_compatible(6,222),"singleton, unsupported and mismatched face pairs reject");
}

void dream_gift_rng_and_no_retroactive_mask(){
    constexpr std::uint64_t draw=0x9e3779b97f4a7c15ULL;std::array<bool,4> found{};
    for(std::uint64_t seed=0;seed<4096&&!std::all_of(found.begin(),found.end(),[](bool v){return v;});++seed){
        auto state=scene({222,6},{});Match m;m.load_scene(state,0,seed);m.step(play(0,{0},0));const auto& s=m.snapshot();const auto& p=s.state.players[0];const auto& gift=p.items.back();
        check(p.curses==std::vector<Curse>{Curse::dream}&&p.items[0].used&&!p.items[0].fake_model&&!p.items[1].fake_model,"Dream source retains miracle and never masks existing inventory retroactively");
        check(!s.last_tick&&s.last_reveals.empty()&&s.dream_rolls<=1&&s.dream_masks<=s.dream_rolls,"Dream gift private counters independent from disease ticks or reveal events");
        std::size_t candidates=0;for(auto face:match_catalog())candidates+=match_dream_compatible(gift.model,face);
        if(!candidates){found[0]=true;check(s.dream_rolls==0&&!gift.fake_model&&s.rng==seed+draw,"singleton gift consumes only weighted actual draw");}
        else if(!gift.fake_model){found[1]=true;check(s.dream_rolls==1&&s.dream_masks==0&&s.rng==seed+2*draw,"eligible normal gift consumes actual and mask coin only");}
        else {found[2]=true;check(s.dream_rolls==1&&s.dream_masks==1&&match_dream_compatible(gift.model,*gift.fake_model)&&s.rng==seed+3*draw,"masked gift consumes actual then coin then candidate draw");if(candidates==1)found[3]=true;
            const auto projected=project_view(s.state,0);const auto it=std::find_if(projected.hand.begin(),projected.hand.end(),[&](const auto& i){return i.token==gift.token;});
            check(it!=projected.hand.end()&&it->face==*gift.fake_model&&it->reference==*gift.fake_model&&it->disguised,"living Dream owner sees face rather than actual gift");}
        roundtrip(m);
    }
    check(std::all_of(found.begin(),found.end(),[](bool v){return v;}),"seed sweep witnesses singleton, unmasked, masked and single-candidate masked branches");
    auto ordinary=fixture(scene({191},{}));const auto rng=ordinary.snapshot().rng;ordinary.step(play(0,{0},0));check(ordinary.snapshot().rng==rng+draw&&ordinary.snapshot().dream_rolls==0&&!ordinary.snapshot().state.players[0].items.back().fake_model,"non-Dream gift has no extra mask RNG");
    auto late=scene({222},{});late.gf=max_wire_integer;auto failure=fixture(late);const auto exact=failure.snapshot();bool threw=false;try{failure.step(play(0,{0},0));}catch(const MatchError&){threw=true;}
    check(threw&&failure.snapshot()==exact,"late GF failure rolls back Dream application, gift masks, counters, resources and all RNG");
}

void dream_same_visible_policy_and_actual_execution(){
    auto state=scene({6},{});state.players[0].curses={Curse::dream};state.players[0].items[0].fake_model=19;
    auto other=state;other.players[0].items[0].model=57;auto a=fixture(state),b=fixture(other);
    check(a.observe(0)==b.observe(0)&&a.observe({})==b.observe({}),"same visible Dream worlds conceal actual power and price");
    std::uint64_t pa=5,pb=5;for(unsigned i=0;i<512;++i){const auto sa=a.snapshot(),sb=b.snapshot();const auto ca=a.sample_action(pa),cb=b.sample_action(pb);check(ca==cb&&pa==pb&&a.legal(ca)&&b.legal(cb)&&a.snapshot()==sa&&b.snapshot()==sb,"policy and legal queries use only visible models and never mutate worlds");}
    for(auto* m:{&a,&b}){m->step(play(0,{0},7));check(m->snapshot().queue[0].power==find_card(m==&a?6:57)->atk&&m->snapshot().last_reveals==std::vector<RevealedItem>{{0,19,static_cast<CardId>(m==&a?6:57),false}},"launch reveals face and actual while executing actual payload");roundtrip(*m);reject(*m,defend(0));m->step(defend(7));check(m->snapshot().state.players[1].hp==40-find_card(m==&a?6:57)->atk,"target HP derives from actual weapon rather than its disguise");roundtrip(*m);}
    a.step(pray(7));check(a.snapshot().last_reveals.empty(),"next accepted main clears previous reveal events");
    CardId low=0,high=0;for(auto x:match_catalog())for(auto y:match_catalog())if(match_dream_compatible(x,y)&&find_card(x)->category==Category::armor&&!find_card(x)->element&&find_card(x)->defense<find_card(y)->defense){low=x;high=y;break;}
    check(low&&high,"compatible armor with unequal DEF exists");auto armored=scene({57},{low});armored.players[1].curses={Curse::dream};armored.players[1].items[0].fake_model=high;
    auto m=fixture(armored);m.step(play(0,{0},7));m.step(defend(7,{0}));check(m.snapshot().state.players[1].hp==40-std::max(0,find_card(57)->atk-find_card(low)->defense)&&m.snapshot().last_reveals==std::vector<RevealedItem>{{7,high,low,false}},"response uses actual DEF and publicly reveals consumed disguise");roundtrip(m);
}

void dream_sources_cures_and_owner_only_revelation(){
    for(CardId source:{CardId{36},CardId{80}}){auto m=fixture(scene({source},{}));m.step(play(0,{0},7));m.step(defend(7));check(m.snapshot().state.players[1].curses==std::vector<Curse>{Curse::dream},"positive weapon HP loss applies Dream");roundtrip(m);}
    auto blocked=fixture(scene({36},{image("god-shield")}));blocked.step(play(0,{0},7));blocked.step(defend(7,{0}));check(blocked.snapshot().state.players[1].curses.empty(),"fully blocked Dream weapon has no onDamage curse");
    auto mild_state=scene({199,6},{});mild_state.players[0].curses={Curse::cold,Curse::fog,Curse::dream,Curse::flash,Curse::darkcloud};mild_state.players[0].items[1].fake_model=57;
    auto mild=fixture(mild_state);mild.step(play(0,{0},0));check(mild.snapshot().state.players[0].curses==std::vector<Curse>{Curse::dream,Curse::darkcloud}&&mild.snapshot().state.players[0].items[0].fake_model==57&&mild.snapshot().last_reveals.empty(),"mild cure preserves Dream masks and DarkCloud");roundtrip(mild);
    MatchConfig cap;cap.max_decisions=2;
    auto state=scene({200},{6});state.players[1].curses={Curse::dream};state.players[1].items[0].fake_model=19;auto other=state;other.players[1].items[0].model=57;other.players[1].items.push_back({1,7,19,false});
    auto a=fixture(state,cap),b=fixture(other,cap);for(auto* m:{&a,&b}){m->step(play(0,{0},7));m->step(defend(7));check(m->snapshot().state.players[1].curses.empty()&&std::none_of(m->snapshot().state.players[1].items.begin(),m->snapshot().state.players[1].items.end(),[](const auto& i){return i.fake_model.has_value();}),"all cure removes Dream and clears all remaining hand masks");check(m->observe(0).find("\"last_reveals\":[]")!=std::string::npos&&m->observe({}).find("\"last_reveals\":[]")!=std::string::npos,"all cure owner-only revelations leave no public count or model leak");check(!m->snapshot().last_reveals.empty()&&m->snapshot().last_reveals[0].owner_only&&m->observe(7).find("\"last_reveals\":[]")==std::string::npos,"only hand owner receives actual models from all cure");roundtrip(*m);}
    check(a.observe(0)==b.observe(0)&&a.observe({})==b.observe({}),"foreign and spectator views cannot distinguish cured hidden inventory contents or counts");
}

void dream_trade_quotes_removal_and_overflow(){
    MatchConfig cap;cap.max_decisions=2;auto state=scene({4,6},{});state.players[0].curses={Curse::dream};state.players[0].items[1].fake_model=57;
    auto sale=fixture(state,cap);sale.step({0,CommandKind::sell,{0,1},7});sale.step(defend(7));check(sale.snapshot().last_sale&&sale.snapshot().last_sale->model==6&&sale.snapshot().last_sale->price==find_card(6)->price&&sale.snapshot().last_reveals==std::vector<RevealedItem>{{0,57,6,false}}&&sale.snapshot().state.players[1].items.back().model==6&&!sale.snapshot().state.players[1].items.back().fake_model,"sale reveals and prices actual commodity, receipt remains unmasked");roundtrip(sale);
    auto quote_state=scene({5},{6});quote_state.players[1].curses={Curse::dream};quote_state.players[1].items[0].fake_model=57;
    auto buy=fixture(quote_state);buy.step({0,CommandKind::buy,{0},7});buy.step(defend(7));check(buy.snapshot().phase==MatchPhase::purchase&&buy.snapshot().purchase->item.model==6&&!buy.snapshot().purchase->item.fake_model&&buy.snapshot().last_reveals==std::vector<RevealedItem>{{7,57,6,false}},"BUY selection unmasks escrow and publishes actual quote");roundtrip(buy);const auto token=buy.snapshot().purchase->item.token;buy.step({0,CommandKind::decline,{},{}});check(buy.snapshot().state.players[1].items[0].token==token&&!buy.snapshot().state.players[1].items[0].fake_model&&buy.snapshot().dream_rolls==0,"declined quote returns same slot/token already unmasked without mask RNG");roundtrip(buy);
    auto remove_state=scene({205},{6});remove_state.players[1].curses={Curse::dream};remove_state.players[1].items[0].fake_model=57;auto removal=fixture(remove_state,cap);removal.step(play(0,{0},7));removal.step(defend(7));check(removal.snapshot().last_removal->removed_models==std::vector<CardId>{6}&&removal.snapshot().last_reveals==std::vector<RevealedItem>{{7,57,6,false}},"removed disguised copy publishes actual model and reveal without replacement");roundtrip(removal);
    MatchConfig small;small.hand_limit=1;small.initial_hand=0;small.max_decisions=2;auto over=scene({4,7},{6});small.hand_limit=2;over.players[1].items.push_back({1,6,57,false});over.players[1].curses={Curse::dream};over.players[1].items[0].fake_model=57;
    auto overflow=fixture(over,small);overflow.step({0,CommandKind::sell,{0,1},7});overflow.step(defend(7));check(overflow.snapshot().last_sale->overflow_model==6&&overflow.snapshot().last_reveals.size()==1&&overflow.snapshot().last_reveals[0]==RevealedItem{7,57,6,false},"full trade eviction reveals actual removed copy once");roundtrip(overflow);
}

void dream_checkpoint_guards_and_transactional_replay(){
    auto state=scene({6},{});state.players[0].curses={Curse::dream};state.players[0].items[0].fake_model=57;auto m=fixture(state);m.step(play(0,{0},7));const auto exact=m.snapshot();
    for(unsigned error=0;error<12;++error){auto bad=exact;
        if(error==0)bad.last_reveals.clear();if(error==1)bad.last_reveals[0].owner_only=true;if(error==2)bad.last_reveals[0].model=7;if(error==3)bad.last_reveals[0].face=19;if(error==4)bad.last_reveals[0].owner=7;
        if(error==5)bad.batch->initial_state.players[0].items[0].fake_model=19;if(error==6)bad.batch->initial_state.players[0].items[0].model=7;if(error==7)bad.dream_rolls=1;if(error==8)bad.dream_masks=1;if(error==9)bad.batch->dream_rolls=1;if(error==10)bad.last_reveals.push_back(bad.last_reveals[0]);if(error==11)bad.rng^=1;
        bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"pending full replay rejects forged Dream reveal, original mask, statistics and RNG atomically");}
    for(unsigned error=0;error<6;++error){auto bad=state;if(error==0)bad.players[0].curses.clear();if(error==1)bad.players[0].items[0].fake_model=6;if(error==2)bad.players[0].items[0].fake_model=178;if(error==3)bad.players[0].items[0].fake_model=143;if(error==4)bad.players[0].items[0].used=true;if(error==5)bad.players[0].curses={Curse::dream,Curse::dream};bool rejected=false;try{m.load_scene(bad,0,1);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"canonical execution rejects unsafe, stale, used, duplicate and unsupported mask scenes");}
    m.step(defend(7));roundtrip(m);
}

void redraw_replaces_used_inventory_and_preserves_reserved_gifts(){
    MatchConfig cap;cap.max_decisions=2;
    auto state=scene({57},{178,6,215,234});state.players[1].items[2].used=true;state.players[1].items[3].used=true;
    auto m=fixture(state,cap);m.step(play(0,{0},7));const auto next=m.snapshot().next_token;m.step(defend(7,{0}));const auto& s=m.snapshot();
    check(s.truncated&&s.redraws==1&&s.redrawn_items==3&&s.next_token==next+3&&s.state.players[1].items.size()==3
        &&s.state.players[1].hp==24&&s.state.players[1].curses==std::vector<Curse>{Curse::dream},"hat DEF14 precedes body while all three remaining copies including used miracles receive immediate replacements");
    check(std::all_of(s.state.players[1].items.begin(),s.state.players[1].items.end(),[&](const auto& item){return item.token>=next&&!item.used;})
        &&s.gifts==std::vector<std::uint32_t>{0,0}&&s.last_redraws==std::vector<PlayerId>{7},"cutoff cancels deferred gifts but preserves already replaced unused fresh-token inventory");roundtrip(m);
    auto ordinary=fixture(state);ordinary.step(play(0,{0},7));ordinary.step(defend(7,{0}));
    check(ordinary.snapshot().redrawn_items==3&&ordinary.snapshot().state.players[1].items.size()==4&&ordinary.snapshot().state.players[0].items.size()==1,"hat reservation is excluded from replacement and paid once after the complete action");roundtrip(ordinary);
    auto empty=fixture(scene({6},{178}),cap);empty.step(play(0,{0},7));const auto rng=empty.snapshot().rng;empty.step(defend(7,{0}));
    check(empty.snapshot().redraws==1&&empty.snapshot().redrawn_items==0&&empty.snapshot().rng==rng&&empty.snapshot().state.players[1].items.empty(),"empty replacement publishes one activation without a random draw or early reserved gift");roundtrip(empty);
}

void redraw_consumes_whole_selection_before_each_ordered_prefix(){
    MatchConfig cap;cap.max_decisions=2;auto state=scene({57},{178,178,143,215});state.players[1].items[3].used=true;
    auto m=fixture(state,cap);m.step(play(0,{0},7));const auto next=m.snapshot().next_token;m.step(defend(7,{0,1,2}));
    check(m.snapshot().redraws==2&&m.snapshot().redrawn_items==2&&m.snapshot().next_token==next+2&&m.snapshot().last_redraws==std::vector<PlayerId>{7,7}
        &&m.snapshot().state.players[1].items.size()==1&&m.snapshot().state.players[1].items[0].token==next+1&&m.snapshot().state.players[1].hp==40,"two hats replace the one unselected retained copy twice after all three selections are consumed, preserving original summed DEF");roundtrip(m);
    for(bool reverse:{false,true}){auto state=scene({6},{168,178,6});auto mixed=fixture(state,cap);mixed.step(play(0,{0},7));mixed.step(defend(7,reverse?std::initializer_list<ItemToken>{1,0}:std::initializer_list<ItemToken>{0,1}));
        check(mixed.snapshot().redraws==1&&mixed.snapshot().redrawn_items==1&&mixed.snapshot().state.players[1].curses==std::vector<Curse>{Curse::fever,Curse::dream},"Fever mask and Dream hat apply both ordered prefix effects after complete selection consumption");roundtrip(mixed);}
    auto main=fixture(scene({178},{}));reject(main,play(0,{0},0));const auto rng=main.snapshot().rng;main.step({0,CommandKind::discard,{0},{}});
    check(main.snapshot().redraws==0&&main.snapshot().last_redraws.empty()&&main.snapshot().state.players[0].curses.empty()&&main.snapshot().rng==rng,"discarding a hat never invokes response-only Dream or redraw");roundtrip(main);
}

void redraw_fresh_responses_and_passive_revival_are_immediate(){
    bool fresh=false,revival=false;
    for(std::uint64_t seed=0;seed<16384&&(!fresh||!revival);++seed){
        if(!fresh){auto state=scene({14},{178,6,215});state.players[1].items[2].used=true;Match m;m.load_scene(state,0,seed);m.step(play(0,{0},7));m.step(defend(7,{0}));
            const auto it=std::find_if(m.snapshot().state.players[1].items.begin(),m.snapshot().state.players[1].items.end(),[](const auto& item){return item.model==178;});
            if(it!=m.snapshot().state.players[1].items.end()){const auto token=it->token;fresh=true;check(token>=3&&m.snapshot().phase==MatchPhase::response&&m.snapshot().actor==7&&m.snapshot().gifts[1]==1,"first hat's replacement is selectable during the second original attack before deferred gift");roundtrip(m);
                m.step(defend(7,{token}));check(m.snapshot().redraws==2&&m.snapshot().redrawn_items==3&&m.snapshot().state.players[1].items.size()==3&&m.snapshot().last_redraws==std::vector<PlayerId>{7,7},"freshly drawn second hat resolves in same anchored action and gifts are delivered only at its end");roundtrip(m);}}
        if(!revival){MatchConfig cap;cap.max_decisions=2;auto state=scene({57},{178,6});state.players[1].hp=1;Match m(cap);m.load_scene(state,0,seed);m.step(play(0,{0},7));m.step(defend(7,{0}));
            if(m.snapshot().revivals){revival=true;check(m.snapshot().revivals==1&&m.snapshot().state.players[1].hp==10&&!m.snapshot().state.players[1].dead&&m.snapshot().redrawn_items==1&&m.snapshot().state.players[1].items.empty(),"amulet drawn by prefix redraw is available to revive incoming lethal body before cutoff");roundtrip(m);}}
    }
    check(fresh&&revival,"seed sweep witnesses same-action fresh response and newly drawn passive revival");
}

void redraw_private_inventory_and_public_event_lifetime(){
    MatchConfig cap;cap.max_decisions=2;auto state=scene({6},{178,6});state.players[1].curses={Curse::dream};state.players[1].items[1].fake_model=57;
    auto other=state;other.players[1].items[1].model=57;other.players[1].items[1].fake_model=6;other.players[1].items.push_back({2,7,19,false});auto a=fixture(state,cap),b=fixture(other,cap);
    check(a.observe(0)==b.observe(0)&&a.observe({})==b.observe({}),"foreign hidden Dream inventory and counts are initially indistinguishable");
    for(auto* m:{&a,&b}){m->step(play(0,{0},7));m->step(defend(7,{0}));check(m->snapshot().last_reveals.empty()&&m->observe({}).find("\"last_redraws\":[7]")!=std::string::npos,"redraw publishes only owner ID and never reveals discarded masks");for(auto key:{"\"redrawn_items\":","\"redraws\":","\"next_token\":","\"dream_rolls\":"})check(m->observe({}).find(key)==std::string::npos,"public replacement notice excludes counts, statistics and tokens");roundtrip(*m);}
    check(a.observe(0)==b.observe(0)&&a.observe({})==b.observe({})&&a.observe(7)!=b.observe(7),"replacement count and actual inventory remain private to owner after visible-identical activation");
    auto live=fixture(state);live.step(play(0,{0},7));live.step(defend(7,{0}));const auto before=live.snapshot();reject(live,pray(0));check(live.snapshot()==before,"failed next main preserves previous redraw notice and new inventory");std::uint64_t policy=99;live.step(live.sample_action(policy));check(live.snapshot().last_redraws.empty(),"next accepted main clears previous redraw notice");roundtrip(live);
}

void redraw_trade_and_original_payload_survive_replacement(){
    MatchConfig cap;cap.max_decisions=2;auto sale=fixture(scene({4,178},{}),cap);sale.step({0,CommandKind::sell,{0,1},7});sale.step(defend(7));
    check(sale.snapshot().redraws==0&&sale.snapshot().last_redraws.empty()&&sale.snapshot().state.players[1].curses.empty()&&sale.snapshot().last_sale->model==178&&sale.snapshot().state.players[1].items.back().model==178,"sale transfers hat as unused commodity without activating its prefix");roundtrip(sale);
    auto buy=fixture(scene({5},{178}));buy.step({0,CommandKind::buy,{0},7});buy.step(defend(7));buy.step({0,CommandKind::accept,{},{}});
    check(buy.snapshot().redraws==0&&buy.snapshot().last_redraws.empty()&&buy.snapshot().state.players[0].curses.empty(),"purchasing a hat never casts or equips it");roundtrip(buy);
    bool erased_redirect_carrier=false;
    for(std::uint64_t seed=0;seed<64&&!erased_redirect_carrier;++seed){auto state=scene({214},{234,178,6});state.players[1].mp=99;Match m;m.load_scene(state,0,seed);m.step(play(0,{0},7));m.step(defend(7,{0}));
        if(m.snapshot().actor!=7)continue;erased_redirect_carrier=true;check(m.snapshot().queue[0].redirects[0].models==std::vector<CardId>{234}&&m.snapshot().state.players[1].items[0].used,"self bounce retains the original miracle before later replacement");roundtrip(m);m.step(defend(7,{1}));
        check(m.snapshot().redrawn_items==2&&std::none_of(m.snapshot().state.players[1].items.begin(),m.snapshot().state.players[1].items.end(),[](const auto& item){return item.used;}),"hat can erase an already recorded used redirect carrier without rewriting its journal or recasting it");roundtrip(m);}
    check(erased_redirect_carrier,"seed sweep witnesses redrawn retained redirect evidence");
    auto state=scene({14},{178,181,215});state.players[1].items[2].used=true;auto counter=fixture(state);counter.step(play(0,{0},7));counter.step(defend(7,{0,1}));
    // Hat defense blocks this body, so a selected zero-power counter is consumed but not generated.
    check(counter.snapshot().counter_created==0&&counter.snapshot().redrawn_items==1&&counter.snapshot().queue[0].models==std::vector<CardId>{14},"replacement never rewrites original twice-attack payload or invents a blocked counter");roundtrip(counter);counter.step(defend(7));roundtrip(counter);
    auto attack=scene({57},{178,184,215});attack.players[1].items[2].used=true;auto reflected=fixture(attack);reflected.step(play(0,{0},7));reflected.step(defend(7,{0,1}));
    check(reflected.snapshot().queue[0].is_counter&&reflected.snapshot().queue[0].models==std::vector<CardId>{184}&&reflected.snapshot().queue[0].power==32&&reflected.snapshot().counter_created==1,"selected counter reserves immutable actual model and damage after redraw removed all old retained inventory");roundtrip(reflected);reflected.step(defend(0));roundtrip(reflected);
}

void redraw_snapshot_adversaries_and_late_atomic_failures(){
    auto state=scene({14},{178,6,215});state.players[1].items[2].used=true;auto m=fixture(state);m.step(play(0,{0},7));m.step(defend(7,{0}));const auto exact=m.snapshot();
    for(unsigned error=0;error<13;++error){auto bad=exact;
        if(error==0)bad.last_redraws.clear();if(error==1)bad.last_redraws[0]=0;if(error==2)bad.last_redraws.push_back(7);if(error==3)++bad.redraws;if(error==4)++bad.redrawn_items;if(error==5)++bad.batch->redraws;if(error==6)++bad.batch->redrawn_items;if(error==7)bad.batch->initial_state.players[1].items[0].model=179;if(error==8)bad.state.players[1].items[0].used=true;if(error==9)bad.state.players[1].items[0].token=0;if(error==10)++bad.next_token;if(error==11)++bad.gifts[1];if(error==12)bad.rng^=1;
        bool rejected=false;try{m.restore(bad);}catch(const MatchError&){rejected=true;}check(rejected&&m.snapshot()==exact,"full pending replay rejects forged replacement event, statistics, originals, new records, reservations and RNG");}
    for(unsigned failure=0;failure<3;++failure){auto state=scene({6},{178,6,6});if(failure==0)state.players[1].items[1].token=max_wire_integer-1;if(failure==1)state.update_count=max_wire_integer-1;if(failure==2)state.gf=max_wire_integer;
        auto late=fixture(state);late.step(play(0,{0},7));const auto before=late.snapshot();bool threw=false;try{late.step(defend(7,{0}));}catch(const MatchError&){threw=true;}check(threw&&late.snapshot()==before,"late partial-draw token, update and GF exhaustion restore old hand, disguise, stats, journal, RNG and reservation");}
    for(std::uint64_t seed=0;seed<32;++seed){auto state=scene({14,57,190},{178,178,215,181});state.players[0].mp=state.players[1].mp=99;MatchConfig cap;cap.max_decisions=64;Match run(cap);run.load_scene(state,0,seed);run.step(play(0,{0},7));run.step(defend(7,{0}));roundtrip(run);std::uint64_t policy=seed;while(!run.done()){const auto before=run.snapshot();const auto command=run.sample_action(policy);check(run.legal(command)&&run.snapshot()==before,"fresh replacement policy remains legal and pure through later play");run.step(command);roundtrip(run);}}
}

void self_buy_inspects_same_copy_without_money_or_quote(){
    constexpr std::uint64_t draw=0x9e3779b97f4a7c15ULL;MatchConfig cap;cap.max_decisions=1;
    for(int cp:{0,99})for(CardId model:{CardId{5},CardId{57},CardId{178},CardId{208},CardId{209},CardId{235},CardId{236}}){auto state=scene({5,model},{});state.players[0].cp=cp;state.players[0].mp=0;auto m=fixture(state,cap);const auto before=m.snapshot();const Command command{0,CommandKind::buy,{0},0};
        check(m.legal(command)&&m.snapshot()==before,"self BUY is legal at MP0/CP0 and legality never consumes or inspects anything");m.step(command);const auto& s=m.snapshot();
        check(s.truncated&&s.decisions==1&&s.rng==before.rng+draw&&s.next_token==before.next_token&&!s.purchase&&s.queue.empty()&&!s.batch
            &&s.state.players[0].items==std::vector<ItemInstance>{{1,model,{},false}}&&s.state.players[0].cp==cp&&s.state.players[0].mp==0&&s.state.players[0].hp==40,"self inspection consumes only BUY and one candidate RNG draw, preserves existing copy and all resources, and never creates escrow");
        check(s.last_purchase==PurchaseEvent{0,0,0,model,find_card(model)->price,PurchaseReason::self_inspection}&&m.observe({}).find("\"reason\":\"self_inspection\"")!=std::string::npos&&m.observe({}).find("\"bought\":false")!=std::string::npos,"self inspection publishes actual model and price with explicit non-transfer reason");roundtrip(m);}
    auto normal=fixture(scene({5,57},{}));normal.step({0,CommandKind::buy,{0},0});
    check(normal.snapshot().owner==7&&normal.snapshot().state.players[0].items.size()==2&&normal.snapshot().state.players[0].items[0].token==1&&normal.snapshot().state.players[0].items[0].model==57&&normal.snapshot().next_token==3,"ordinary self inspection pays exactly one operation gift after its body without moving the selected copy");roundtrip(normal);
}

void self_buy_empty_and_used_miracles_preserve_candidate_order(){
    constexpr std::uint64_t draw=0x9e3779b97f4a7c15ULL;MatchConfig cap;cap.max_decisions=1;
    for(bool used:{false,true}){auto state=scene({5},{});if(used){state.players[0].items.push_back({9,235,{},true});state.players[0].cp=0;}auto m=fixture(state,cap);const auto before=m.snapshot();m.step({0,CommandKind::buy,{0},0});
        auto expected=state.players[0].items;expected.erase(expected.begin());check(m.snapshot().rng==before.rng&&m.snapshot().state.players[0].items==expected&&m.snapshot().last_purchase==PurchaseEvent{0,0,0,0,0,PurchaseReason::empty},"empty self inspection excludes all used miracles and draws nothing");roundtrip(m);}
    auto empty=fixture(scene({5},{})),prayer=fixture(scene({},{}));empty.step({0,CommandKind::buy,{0},0});prayer.step(pray(0));
    check(empty.snapshot().rng==prayer.snapshot().rng&&empty.snapshot().state.players[0].items[0].model==prayer.snapshot().state.players[0].items[0].model,"empty self inspection has exactly the ordinary deferred gift RNG and no hidden selection draw");
    std::set<CardId> selected;
    for(std::uint64_t seed=0;seed<128;++seed){auto state=scene({6,5,235,236,57},{});state.players[0].items[2].used=true;Match m(cap);m.load_scene(state,0,seed);const auto rng=m.snapshot().rng;m.step({0,CommandKind::buy,{1},0});auto expected=state.players[0].items;expected.erase(expected.begin()+1);selected.insert(m.snapshot().last_purchase->model);
        check(m.snapshot().state.players[0].items==expected&&m.snapshot().rng==rng+draw&&m.snapshot().last_purchase->model!=235,"selection preserves post-operation inventory order and every used flag regardless of selected slot");roundtrip(m);}
    check(selected==std::set<CardId>{6,236,57},"seed sweep reaches every eligible unused commodity while excluding used miracle");
}

void self_buy_dream_reveals_only_chosen_copy_and_queries_use_faces(){
    MatchConfig cap;cap.max_decisions=1;auto state=scene({5,6},{});state.players[0].curses={Curse::dream};state.players[0].items[1].fake_model=19;auto other=state;other.players[0].items[1].model=57;
    auto a=fixture(state,cap),b=fixture(other,cap);check(a.observe(0)==b.observe(0)&&a.observe({})==b.observe({}),"self inspector initially cannot distinguish same-face actual power or price");
    std::uint64_t pa=123,pb=123;for(unsigned i=0;i<256;++i){const auto sa=a.snapshot(),sb=b.snapshot();const auto ca=a.sample_action(pa),cb=b.sample_action(pb);check(ca==cb&&pa==pb&&a.legal(ca)&&b.legal(cb)&&a.snapshot()==sa&&b.snapshot()==sb,"self BUY proposal and legal queries use displayed models and never peek at actual price or reveal masks");}
    for(auto* m:{&a,&b}){const auto actual=static_cast<CardId>(m==&a?6:57);m->step({0,CommandKind::buy,{0},0});check(m->snapshot().last_reveals==std::vector<RevealedItem>{{0,19,actual,false}}&&m->snapshot().state.players[0].items==std::vector<ItemInstance>{{1,actual,{},false}}&&m->snapshot().last_purchase->model==actual&&m->snapshot().dream_rolls==0,"self inspection publicly unmasks the chosen existing copy and resolves its actual price without new masks at cutoff");roundtrip(*m);}
    auto multiple=scene({5,6,6,57},{});multiple.players[0].curses={Curse::dream};multiple.players[0].items[1].fake_model=19;multiple.players[0].items[2].fake_model=7;multiple.players[0].items[3].fake_model=8;
    std::set<ItemToken> tokens;
    for(std::uint64_t seed=0;seed<128;++seed){Match m(cap);m.load_scene(multiple,0,seed);m.step({0,CommandKind::buy,{0},0});check(m.snapshot().last_reveals.size()==1&&m.snapshot().state.players[0].items.size()==3,"duplicate-model commodities still reveal exactly one physical copy");std::size_t revealed=0;
        for(std::size_t i=0;i<3;++i){const auto& item=m.snapshot().state.players[0].items[i];check(item.token==multiple.players[0].items[i+1].token&&item.model==multiple.players[0].items[i+1].model,"inspection does not reorder copies or replace token identity");if(!item.fake_model){++revealed;tokens.insert(item.token);}}
        check(revealed==1,"unchosen duplicate masks remain private and intact");roundtrip(m);}
    check(tokens==std::set<ItemToken>{1,2,3},"candidate sampling is over physical copies, including duplicates");
}

void self_buy_fog_team_capacity_and_sampler_do_not_peek(){
    constexpr std::uint64_t draw=0x9e3779b97f4a7c15ULL;MatchConfig cap;cap.max_decisions=1;cap.hand_limit=2;cap.initial_hand=0;
    SnapshotState state;state.players={player(max_wire_integer,{5,57},Team::one),player(7,{},Team::two),player(9,{},Team::one),player(42,{},Team::two),player(55,{},Team::two)};state.players[0].curses={Curse::fog};state.players[4].dead=true;state.players[4].hp=0;
    Match m(cap);m.load_scene(state,max_wire_integer,0);const auto original=m.snapshot();const Command self{max_wire_integer,CommandKind::buy,{0},max_wire_integer};check(m.legal(self)&&m.snapshot()==original,"self BUY remains legal with full hand, Fog and arbitrary safe-integer owner ID");
    reject(m,{max_wire_integer,CommandKind::buy,{0},55});reject(m,{max_wire_integer,CommandKind::buy,{0},999});m.step(self);
    check(m.snapshot().rng==draw&&!m.snapshot().last_fog&&m.snapshot().last_purchase->buyer==max_wire_integer&&m.snapshot().last_purchase->seller==max_wire_integer,"self target bypasses Fog reselection even with multiple enemies and teammates");roundtrip(m);
    auto own=scene({5,6},{}),foreign=own;for(ItemToken i=0;i<18;++i)foreign.players[1].items.push_back({i,57,{},false});auto a=fixture(own),b=fixture(foreign);std::uint64_t pa=42,pb=42;std::set<PlayerId> buyers;
    for(unsigned i=0;i<1024;++i){const auto sa=a.snapshot(),sb=b.snapshot();const auto ca=a.sample_action(pa),cb=b.sample_action(pb);check(ca==cb&&pa==pb&&a.legal(ca)&&b.legal(cb)&&a.snapshot()==sa&&b.snapshot()==sb,"self and foreign BUY proposals depend only on visible own inventory and public alive roster");if(ca.kind==CommandKind::buy)buyers.insert(*ca.target);}
    check(buyers==std::set<PlayerId>{0,7},"sampler actually includes self and foreign living targets");
}

void self_buy_events_failures_cutoff_and_shape_guards(){
    auto state=scene({5,6},{});state.players[0].curses={Curse::dream};state.players[0].items[1].fake_model=57;
    auto m=fixture(state);m.step({0,CommandKind::buy,{0},0});const auto event=m.snapshot();for(const auto& command:std::vector<Command>{{7,CommandKind::accept,{},{}},{7,CommandKind::decline,{},{}},{7,CommandKind::defend,{},{}}})reject(m,command);check(m.snapshot()==event,"self inspection never leaves an accept/decline/response input phase");
    for(unsigned error=0;error<9;++error){auto bad=event;auto& e=*bad.last_purchase;if(error==0)e.reason=PurchaseReason::bought;if(error==1)e.buyer=7;if(error==2)e.seller=7;if(error==3)e.origin=7;if(error==4)e.model=207;if(error==5)++e.price;if(error==6)e.model=0;if(error==7)e.reason=static_cast<PurchaseReason>(6);if(error==8)e.overflow_model=6;bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==event,"quiescent self inspection enforces reason, aliases, origin, supported commodity, price and absence of overflow");}
    m.step(pray(7));check(!m.snapshot().last_purchase&&m.snapshot().last_reveals.empty(),"next accepted main clears inspection result and public reveal");roundtrip(m);
    for(unsigned failure=0;failure<3;++failure){auto state=scene({5,6},{});state.players[0].curses={Curse::dream};state.players[0].items[1].fake_model=57;if(failure==0)state.players[0].items[1].token=max_wire_integer;if(failure==1)state.update_count=max_wire_integer;if(failure==2)state.gf=max_wire_integer;
        auto late=fixture(state);const auto exact=late.snapshot();bool threw=false;try{late.step({0,CommandKind::buy,{0},0});}catch(const MatchError&){threw=true;}check(threw&&late.snapshot()==exact,"late gift/update/GF failure rolls back self inspection, unmask, selected operation, resources, event and RNG");}
    MatchConfig cap;cap.max_decisions=1;auto token=state;token.players[0].items[1].token=max_wire_integer;auto cut=fixture(token,cap);cut.step({0,CommandKind::buy,{0},0});check(cut.snapshot().truncated&&cut.snapshot().last_purchase->reason==PurchaseReason::self_inspection&&!cut.snapshot().state.players[0].items[0].fake_model&&cut.snapshot().next_token==max_wire_integer+1,"cutoff preserves completed inspection and old maximum token while cancelling only deferred gift");roundtrip(cut);
    auto sick=scene({5,6},{});sick.players[0].hp=1;sick.players[0].curses={Curse::cold};auto death=fixture(sick);death.step({0,CommandKind::buy,{0},0});check(death.done()&&death.snapshot().terminated&&death.snapshot().state.players[0].dead&&death.snapshot().last_purchase->reason==PurchaseReason::self_inspection&&death.snapshot().state.players[0].items.size()==1,"inspection resolves before original-owner disease death and survives cancellation of operation gift");roundtrip(death);
    auto empty=fixture(scene({5},{}),cap);empty.step({0,CommandKind::buy,{0},0});auto bad=empty.snapshot();bad.last_purchase->origin=7;const auto exact=empty.snapshot();bool failed=false;try{empty.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&empty.snapshot()==exact,"empty self inspection still requires its main owner as origin");
}

void self_buy_seeded_rollouts_remain_replayable(){
    for(unsigned count:{2u,9u})for(std::uint64_t seed=0;seed<16;++seed){SnapshotState state;for(unsigned seat=0;seat<count;++seat)state.players.push_back(player(seat?100+seat:0,seat?std::initializer_list<CardId>{190,57,235}:std::initializer_list<CardId>{5,5,6,235,208},count==9?static_cast<Team>(seat%5):Team::solo));state.players[0].curses={Curse::cold,Curse::fog,Curse::dream};state.players[0].items[2].fake_model=57;state.players[0].items[3].used=true;state.players[0].cp=0;
        MatchConfig cap;cap.max_decisions=48;Match a(cap),b(cap);a.load_scene(state,0,seed);b.load_scene(state,0,seed);const Command command{0,CommandKind::buy,{0},0};check(a.step(command)==b.step(command)&&a.snapshot()==b.snapshot(),"seeded self inspection produces identical complete state with Dream/Fog/disease and used miracles");roundtrip(a);std::uint64_t pa=seed,pb=seed;
        while(!a.done()){const auto ca=a.sample_action(pa),cb=b.sample_action(pb);check(ca==cb&&pa==pb&&a.legal(ca)&&b.legal(cb),"self-enabled policy remains deterministic and legal");check(a.step(ca)==b.step(cb)&&a.snapshot()==b.snapshot(),"later commands replay complete state after self inspection");roundtrip(a);}}
}

Match guardian_fixture(CardId model, std::initializer_list<CardId> defense={},MatchConfig config={},std::uint64_t seed=1234){
    auto state=scene({},defense);state.players[0].guardian=find_card(model)->guardian;Match m(config);m.load_scene(state,7,seed);return m;
}
void reject_guardian(Match& m,const GuardianAction& action){const auto before=m.snapshot();check(!m.guardian_legal(action)&&m.snapshot()==before,"external legality query is pure");bool threw=false;try{m.guardian_step(action);}catch(const MatchError&){threw=true;}check(threw&&m.snapshot()==before,"invalid external activation is transactional");}

void guardian_all_canonical_actions_and_distinct_inventory_pool(){
    std::size_t total=0;for(const auto& card:catalog()){check(match_guardian_supported(card.id)==(card.id>=245&&card.id<=286),"only the42 canonical guardian rows are external capabilities");if(!match_guardian_supported(card.id))continue;++total;
        check(!match_supported(card.id)&&!can_start_main(card.id),"canonical guardian rows never enter ordinary inventory or player main selection");
        auto state=scene();state.players[0].guardian=card.guardian;for(auto& p:state.players)p.hp=p.mp=p.cp=99;state.players[1].curses={Curse::darkcloud};Match m;m.load_scene(state,7,1234);
        GuardianAction action{0,card.id,card.ability==Ability::boost_cp_of_everybody?std::nullopt:std::optional<PlayerId>{(card.atk>0||targets_enemy_by_default(card.id))?7u:0u}};const auto before=m.snapshot();check(m.guardian_legal(action)&&m.snapshot()==before,"all42 fixed metadata actions have a valid explicit scenario");m.guardian_step(action);roundtrip(m);
        check(m.snapshot().guardian_actions==1&&m.snapshot().last_guardian==action&&m.snapshot().state.players[0].guardian==card.guardian,"activation records its public source/model but never randomly replaces or removes a persistent guardian");
        if(m.snapshot().phase==MatchPhase::response){const auto& e=m.snapshot().queue[0];check(e.source==0&&e.origin==0&&e.models==std::vector<CardId>{card.id}&&e.tokens.empty()&&e.power==card.atk&&e.guardian==card.guardian&&e.category==(card.ability==Ability::category_weapons?Category::weapons:Category::guardians),"canonical payload preserves guardian identity, metadata power and effective category without physical tokens");m.step(defend(*m.snapshot().actor));roundtrip(m);}
        check(m.done()||m.snapshot().phase==MatchPhase::main,"pass resolves each canonical single-target action");check(m.snapshot().state.gf==0&&!m.snapshot().last_tick&&m.snapshot().state.players[0].mp==99&&m.snapshot().state.players[0].items.size()==(card.id==285?1u:0u),"external activation has no ordinary GF/tick, source fee, source consumption or operation gift");}
    check(total==42&&match_catalog().size()==236&&match_gift_weight()==499,"guardian42 coverage does not renormalize the236 artifact gift pool");
    auto state=scene({245},{});Match closed;const auto before=closed.snapshot();bool failed=false;try{closed.load_scene(state,0,1);}catch(const MatchError&){failed=true;}check(failed&&closed.snapshot()==before,"guardian action IDs remain rejected as physical inventory");
}

void guardian_reaction_categories_and_immutable_identity(){
    auto ordinary=guardian_fixture(255,{38,39,233,190,189});ordinary.guardian_step({0,255,7});for(ItemToken token:{0u,1u,2u})reject(ordinary,defend(7,{token}));reject(ordinary,defend(7,{4,0}));ordinary.step(defend(7,{3}));
    check(ordinary.snapshot().queue[0].source==7&&ordinary.snapshot().queue[0].target==0&&ordinary.snapshot().queue[0].guardian==Guardian::jupiter&&ordinary.snapshot().queue[0].models==std::vector<CardId>{255},"universal reflection redirects guardian payload while preserving public guardian identity");roundtrip(ordinary);ordinary.step(defend(0));check(ordinary.snapshot().owner==7&&ordinary.snapshot().actor==7&&ordinary.snapshot().state.gf==0&&ordinary.snapshot().state.players[0].hp==39,"external response returns to ordinary owner without advancing GF");roundtrip(ordinary);
    auto weapon=guardian_fixture(264,{189,38});weapon.guardian_step({0,264,7});reject(weapon,defend(7,{1}));check(weapon.legal(defend(7,{0,1})),"categoryWeapons guardian264 supports filter then physical reflection");weapon.step(defend(7,{0,1}));check(weapon.snapshot().queue[0].category==Category::weapons&&!weapon.snapshot().queue[0].element&&weapon.snapshot().queue[0].guardian==Guardian::saturn,"weapon override and guardian identity survive filter/reflection");roundtrip(weapon);weapon.step(defend(0));roundtrip(weapon);
    auto moon=guardian_fixture(286,{38});moon.guardian_step({0,286,7});moon.step(defend(7,{0}));check(moon.snapshot().queue[0].category==Category::weapons&&moon.snapshot().queue[0].power==10&&moon.snapshot().queue[0].guardian==Guardian::moon,"canonical Moon286 is neutral weapon payload rather than miracle");roundtrip(moon);moon.step(defend(0));roundtrip(moon);
    auto light=guardian_fixture(265,{image("god-shield"),189});light.guardian_step({0,265,7});reject(light,defend(7,{0}));light.step(defend(7,{1,0}));check(light.snapshot().state.players[1].hp==40,"Light guardian still obeys public attribute defense/filter predicate");roundtrip(light);
    auto cure=guardian_fixture(275,{image("god-shield"),190});cure.guardian_step({0,275,7});reject(cure,defend(7,{0}));cure.step(defend(7,{1}));check(cure.snapshot().queue[0].power==0&&cure.snapshot().queue[0].category==Category::guardians,"pure guardian utility permits universal reflection only");roundtrip(cure);cure.step(defend(0));roundtrip(cure);
}

void guardian_chance_privacy_and_no_opportunity_sampling(){
    bool hit=false,miss=false;constexpr std::uint64_t draw=0x9e3779b97f4a7c15ULL;
    for(std::uint64_t seed=0;seed<128&&(!hit||!miss);++seed){auto m=guardian_fixture(245,{}, {},seed);m.guardian_step({0,245,7});check(m.snapshot().chance_rolls==1&&m.snapshot().rng==seed+draw&&m.snapshot().guardian_actions==1,"only the supplied action's percentage is sampled, without type/move/25percent opportunity draws");if(m.snapshot().phase==MatchPhase::response){hit=true;check(m.snapshot().chance_hits==1,"pending external percentage frame already hit");roundtrip(m);m.step(defend(7));}else{miss=true;check(m.snapshot().chance_hits==0&&m.snapshot().state.players[1].hp==40,"external miss resumes the owner without a response or damage");}check(m.snapshot().owner==7&&m.snapshot().state.gf==0,"external hit or miss never advances ordinary owner/GF");roundtrip(m);}
    check(hit&&miss,"seed sweep witnesses external chance hit and miss");auto state=scene({190},{190});state.players[0].guardian=Guardian::mars;state.players[0].curses={Curse::fog};state.players[1].curses={Curse::darkcloud};Match m;m.load_scene(state,7,0);m.guardian_step({0,245,7});check(m.snapshot().rng==0&&m.snapshot().chance_forced_hits==1&&m.observe(0).find("\"forced_hit\":null")!=std::string::npos,"DarkCloud forces without RNG while Fog hides forced activation from enemy view");m.step(defend(7,{0}));check(m.snapshot().rng==0&&m.snapshot().queue[0].forced_hit&&m.observe(0).find("\"forced_hit\":null")!=std::string::npos,"reflection keeps original hidden activation and does not reroll");roundtrip(m);m.step(defend(0));roundtrip(m);
}

void guardian_venus_cp_global_credit_and_debt_order(){
    auto state=scene();state.players[0].guardian=Guardian::venus;state.players[0].cp=1;state.players[1].cp=0;Match bribe;bribe.load_scene(state,7,1);bribe.guardian_step({0,281,7});bribe.step(defend(7));check(bribe.snapshot().state.players[0].cp==1&&bribe.snapshot().state.players[1].cp==5,"Venus bribe credits target without debiting source");roundtrip(bribe);
    Match fine;fine.load_scene(state,7,1);fine.guardian_step({0,282,7});fine.step(defend(7));check(fine.snapshot().state.players[0].cp==4&&fine.snapshot().state.players[1].cp==0&&fine.snapshot().state.players[1].mp==7,"Venus fine debits target CP debt before full nominal source credit");roundtrip(fine);
    state.players[1].items={{0,190,{},false}};Match reverse;reverse.load_scene(state,7,1);reverse.guardian_step({0,282,7});reverse.step(defend(7,{0}));reverse.step(defend(0));check(reverse.snapshot().state.players[0].cp==0&&reverse.snapshot().state.players[0].mp==8&&reverse.snapshot().state.players[1].cp==3,"reflected fine transfers current source/target roles and preserves nominal debt amount");roundtrip(reverse);
    state.players.push_back(player(9));state.players[0].cp=98;state.players[1].cp=99;state.players[2].dead=true;state.players[2].hp=0;state.players[2].cp=8;Match global;global.load_scene(state,7,1);global.guardian_step({0,280,{}});check(global.snapshot().phase==MatchPhase::main&&global.snapshot().decisions==1&&global.snapshot().state.players[0].cp==99&&global.snapshot().state.players[1].cp==99&&global.snapshot().state.players[2].cp==8&&global.snapshot().rng==1,"coin scattering atomically credits all living players including source, clips99 and excludes dead players");roundtrip(global);
}

void guardian_cure_earth_gifts_and_ordinary_owner_are_separate(){
    auto state=scene({6},{});state.players[0].guardian=Guardian::neptune;state.players[0].curses={Curse::dream};state.players[0].items[0].fake_model=57;state.players[1].curses={Curse::cold};state.players[1].hp=1;state.gf=max_wire_integer;Match m;m.load_scene(state,7,0);m.guardian_step({0,275,0});
    check(m.snapshot().owner==7&&m.snapshot().actor==7&&m.snapshot().state.gf==max_wire_integer&&!m.snapshot().last_tick&&m.snapshot().state.players[1].hp==1&&m.snapshot().state.players[0].curses.empty()&&!m.snapshot().state.players[0].items[0].fake_model,"external self cure skips ordinary-owner tick and GF and never consumes source inventory");check(m.snapshot().last_reveals==std::vector<RevealedItem>{{0,57,6,true}}&&m.observe(7).find("\"last_reveals\":[]")!=std::string::npos,"guardian all-cure reveals remaining hand to owner only");roundtrip(m);m.step(pray(7));check(m.snapshot().terminated&&m.snapshot().state.players[1].dead&&!m.snapshot().last_guardian,"only a later ordinary main performs its disease tick and clears prior guardian event");roundtrip(m);
    MatchConfig full;full.hand_limit=2;full.initial_hand=0;auto packed=scene({6,6},{});packed.players[0].guardian=Guardian::earth;Match no_room(full);no_room.load_scene(packed,7,0);no_room.guardian_step({0,285,0});check(no_room.snapshot().rng==0&&no_room.snapshot().state.players[0].items==packed.players[0].items,"canonical Earth285 at capacity neither evicts nor draws");roundtrip(no_room);
    auto earth=guardian_fixture(285,{190});earth.guardian_step({0,285,7});earth.step(defend(7,{0}));earth.step(defend(0));check(earth.snapshot().state.players[0].items.size()==1&&earth.snapshot().state.players[1].items.size()==1&&earth.snapshot().owner==7&&earth.snapshot().state.gf==0,"reflected Earth grant and consumed defensive mirror each receive their separate deferred gift");roundtrip(earth);
    auto debt=scene({},{});debt.players[0].guardian=Guardian::venus;debt.players[1].cp=debt.players[1].mp=0;debt.players[1].hp=1;Match lethal;lethal.load_scene(debt,7,0);lethal.guardian_step({0,282,7});lethal.step(defend(7));check(lethal.snapshot().terminated&&lethal.snapshot().state.players[1].dead&&lethal.snapshot().state.players[0].cp==23,"nonattack guardian CP debt may kill but still credits source before natural outcome");roundtrip(lethal);
}

void guardian_counters_death_and_resume_owner(){
    auto counter=guardian_fixture(255,{184});counter.guardian_step({0,255,7});counter.step(defend(7,{0}));check(counter.snapshot().queue[0].is_counter&&counter.snapshot().queue[0].guardian==Guardian::none&&counter.snapshot().queue[0].origin==7&&counter.snapshot().queue[0].power==2,"ordinary defender counter is distinct from guardian origin and carries no inherited guardian identity");roundtrip(counter);counter.step(defend(0));check(counter.snapshot().owner==7&&counter.snapshot().state.gf==0,"counter completion resumes external action's original ordinary owner");roundtrip(counter);
    SnapshotState state;state.players={player(0),player(7,{112}),player(9)};state.players[0].guardian=Guardian::pluto;state.players[0].curses={Curse::darkcloud};state.players[2].curses={Curse::darkcloud};Match death;death.load_scene(state,7,0);death.guardian_step({0,271,7});death.step(defend(7));check(death.snapshot().state.players[1].dead&&death.snapshot().queue[0].is_death&&death.snapshot().queue[0].source==7&&death.snapshot().queue[0].guardian==Guardian::none,"guardian kill can start ordinary-owner posthumous FIFO before resuming next turn");roundtrip(death);death.step(defend(0));roundtrip(death);death.step(defend(9));check(death.snapshot().owner==9&&death.snapshot().actor==9&&death.snapshot().state.gf==0&&death.snapshot().guardian_actions==1,"dead ordinary owner resumes next living input seat after all posthumous responses without GF advancement");roundtrip(death);
    state.players[1].items={{0,190,{},false}};state.players[0].curses.clear();state.players[2].curses.clear();Match reflected;reflected.load_scene(state,7,0);reflected.guardian_step({0,271,7});reflected.step(defend(7,{0}));reflected.step(defend(0));check(reflected.snapshot().state.players[0].dead&&reflected.snapshot().owner==7&&!reflected.done()&&reflected.snapshot().state.gf==0,"dead guardian source does not steal or advance the surviving ordinary owner's turn");roundtrip(reflected);
}

void guardian_domains_cutoff_snapshot_forgery_and_rollback(){
    auto m=guardian_fixture(255);for(auto action:std::vector<GuardianAction>{{0,0,7},{0,201,7},{0,244,7},{0,287,7},{7,255,0},{0,255,{}},{0,255,999},{999,255,7}})reject_guardian(m,action);
    auto wrong=scene();wrong.players[0].guardian=Guardian::mars;Match mismatch;mismatch.load_scene(wrong,7,0);reject_guardian(mismatch,{0,255,7});auto global=guardian_fixture(280);reject_guardian(global,{0,280,7});
    m.guardian_step({0,255,7});const auto exact=m.snapshot();reject_guardian(m,{0,255,7});for(unsigned error=0;error<14;++error){auto bad=exact;if(error==0)bad.batch->guardian_action->source=7;if(error==1)bad.batch->guardian_action->model=256;if(error==2)bad.batch->guardian_action->target=0;if(error==3)bad.batch->guardian_action.reset();if(error==4)bad.batch->attack.items={0};if(error==5)bad.queue[0].guardian=Guardian::mars;if(error==6)bad.queue[0].category=Category::weapons;if(error==7)++bad.queue[0].power;if(error==8)bad.batch->initial_state.players[0].guardian=Guardian::mars;if(error==9)++bad.guardian_actions;if(error==10)++bad.batch->guardian_actions;if(error==11)bad.last_guardian.reset();if(error==12)bad.batch->tick_processed=false;if(error==13)++bad.rng;bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"external anchor replay rejects forged guardian payload/category/source/type/counters/events/tick and RNG");}
    auto broken=scene();broken.players[0].guardian=Guardian::jupiter;broken.update_count=max_wire_integer;Match late;late.load_scene(broken,7,0);const auto before=late.snapshot();bool failed=false;try{late.guardian_step({0,255,7});}catch(const MatchError&){failed=true;}check(failed&&late.snapshot()==before,"late external update exhaustion rolls back activation, queue, event and counters");
    auto token=scene({},{});token.players[0].guardian=Guardian::jupiter;token.players[1].items={{max_wire_integer,190,{},false}};Match gifts;gifts.load_scene(token,7,0);gifts.guardian_step({0,255,7});gifts.step(defend(7,{max_wire_integer}));const auto pending=gifts.snapshot();failed=false;try{gifts.step(defend(0));}catch(const MatchError&){failed=true;}check(failed&&gifts.snapshot()==pending,"late defensive gift failure restores reflected guardian frame and all body effects");
    MatchConfig cap;cap.max_decisions=1;auto cut=guardian_fixture(255,{},cap);cut.guardian_step({0,255,7});check(cut.snapshot().truncated&&cut.snapshot().state.players[1].hp==40&&cut.snapshot().queue.empty()&&cut.snapshot().last_guardian==GuardianAction{0,255,7},"cutoff may cancel a pending external attack while preserving public activation");roundtrip(cut);auto heal=guardian_fixture(276,{},cap);heal.guardian_step({0,276,0});check(heal.snapshot().truncated&&heal.snapshot().state.players[0].hp==45,"immediate self guardian recovery resolves before cutoff");roundtrip(heal);auto grant=guardian_fixture(285,{},cap);grant.guardian_step({0,285,0});check(grant.snapshot().truncated&&grant.snapshot().state.players[0].items.empty(),"cutoff cancels deferred Earth gift");roundtrip(grant);
}

void guardian_seeded_complete_rollouts_remain_deterministic(){
    for(unsigned count:{2u,9u})for(std::uint64_t seed=0;seed<32;++seed){const auto model=static_cast<CardId>(245+seed%42);SnapshotState state;for(unsigned seat=0;seat<count;++seat)state.players.push_back(player(seat?100+seat:0,{190,189,178,184,235}));state.players[0].guardian=find_card(model)->guardian;for(auto& p:state.players){p.hp=99;p.mp=99;}MatchConfig cap;cap.max_decisions=64;Match a(cap),b(cap);a.load_scene(state,101,seed);b.load_scene(state,101,seed);GuardianAction action{0,model,model==280?std::nullopt:std::optional<PlayerId>{(find_card(model)->atk>0||targets_enemy_by_default(model))?101u:0u}};check(a.guardian_step(action)==b.guardian_step(action)&&a.snapshot()==b.snapshot(),"external fixed action replays initial full state");roundtrip(a);std::uint64_t pa=seed,pb=seed;
        while(!a.done()&&a.snapshot().phase!=MatchPhase::main){const auto ca=a.sample_action(pa),cb=b.sample_action(pb);check(ca==cb&&pa==pb&&a.legal(ca)&&b.legal(cb),"external response sampler uses ordinary visible state and remains deterministic");check(a.step(ca)==b.step(cb)&&a.snapshot()==b.snapshot(),"guardian reflection/counter/redraw chain has deterministic full-state replay");roundtrip(a);}if(!a.done()){std::uint64_t policy=123;a.step(a.sample_action(policy));roundtrip(a);check(a.snapshot().guardian_actions==1,"later ordinary action never invents automatic guardian opportunity or summoning");}}
}

void guardian_summon_all_ten_and_normal_cast_roles(){
    constexpr std::uint64_t draw=0x9e3779b97f4a7c15ULL;MatchConfig cap;cap.max_decisions=1;std::set<Guardian> types;bool same=false;
    for(std::uint64_t seed=0;seed<256&&(types.size()<10||!same);++seed){auto state=scene({201},{});state.players[0].guardian=Guardian::mars;Match m(cap);m.load_scene(state,0,seed);m.step(play(0,{0},0));const auto type=m.snapshot().state.players[0].guardian;types.insert(type);same|=type==Guardian::mars;
        check(m.snapshot().guardian_summons==1&&m.snapshot().guardian_opportunities==0&&m.snapshot().rng==seed+draw&&m.snapshot().state.players[0].items.empty()&&m.snapshot().last_guardian_changes==std::vector<GuardianChange>{{0,Guardian::mars,type,GuardianChangeReason::summon,0}},"pot consumes one copy and uniformly selects one of all ten types before cutoff, including the current type");roundtrip(m);}
    check(types.size()==10&&same,"seed sweep reaches all ten summon types and same-type resummon");
    for(bool free:{false,true}){auto state=free?scene({239,204},{}):scene({239},{});state.players[0].mp=free?0:15;auto m=fixture(state,cap);m.step(free?play(0,{0,1},0):play(0,{0},0));check(m.snapshot().state.players[0].mp==0&&m.snapshot().state.players[0].items==std::vector<ItemInstance>{{0,239,{},true}}&&m.snapshot().guardian_summons==1,"Release pays MP15 or adjacent FREE, retains the same miracle, and summons once");roundtrip(m);}
    auto poor=scene({239},{});poor.players[0].mp=14;auto invalid=fixture(poor);reject(invalid,play(0,{0},0));
    auto reflected=fixture(scene({201},{190}));reflected.step(play(0,{0},7));check(reflected.snapshot().guardian_summons==0,"foreign summon chooses its type only at the resolved body");reflected.step(defend(7,{0}));roundtrip(reflected);reflected.step(defend(0));check(reflected.snapshot().state.players[0].guardian!=Guardian::none&&reflected.snapshot().state.players[1].guardian==Guardian::none&&reflected.snapshot().guardian_summons==1&&reflected.snapshot().guardian_opportunities==0,"reflected summon affects current receiver and does not act after that receiver's own main");roundtrip(reflected);
    auto discarded=fixture(scene({201},{}));discarded.step({0,CommandKind::discard,{0},{}});check(discarded.snapshot().guardian_summons==0,"discarding pot never summons");auto sold=fixture(scene({4,201},{}));sold.step({0,CommandKind::sell,{0,1},7});sold.step(defend(7));check(sold.snapshot().guardian_summons==0&&sold.snapshot().state.players[1].guardian==Guardian::none,"selling pot transfers an unused commodity without summoning");roundtrip(sold);
}

void guardian_auto_all_types_draw_order_and_once_per_source(){
    constexpr std::uint64_t draw=0x9e3779b97f4a7c15ULL;
    for(unsigned type=1;type<=10;++type){std::set<CardId> models;bool miss=false;const auto required=type<=8?5u:1u;
        for(std::uint64_t seed=0;seed<2048&&(models.size()<required||!miss);++seed){auto state=scene();for(auto& p:state.players)p.hp=p.mp=p.cp=99;state.players[0].curses={Curse::darkcloud};state.players[1].guardian=static_cast<Guardian>(type);Match m;m.load_scene(state,0,seed);m.step(pray(0));const auto& s=m.snapshot();check(s.guardian_opportunities==1&&s.guardian_actions==0,"one eligible enemy source gets one opportunity, without an external activation decision");
            if(!s.auto_guardian_actions){miss=true;check(s.decisions==1&&s.rng==seed+2*draw&&s.last_auto_guardians.empty()&&s.state.players[0].items.size()==1,"missed quarter opportunity draws no move or target and proceeds to one main gift");}
            else {const auto& action=s.last_auto_guardians[0];models.insert(action.model);check(find_card(action.model)->guardian==static_cast<Guardian>(type)&&s.auto_guardian_actions==1&&s.decisions==1,"successful opportunity selects a canonical move belonging to the unchanged source type");
                if(s.phase==MatchPhase::response){check(s.rng==seed+3*draw&&s.gifts[0]==1&&s.state.gf==0&&s.batch->guardian_stage_started&&s.batch->guardian_cursor==2&&s.batch->tick_processed&&s.batch->attack.kind==CommandKind::pray,"quarter, move and singleton target consume exactly three draws before body, without paying gifts or advancing owner");roundtrip(m);m.step(defend(*m.snapshot().actor));}
                else check(s.phase==MatchPhase::main&&s.state.gf==1,"self utility/global CP completes internally before normal owner advance");}
            roundtrip(m);
        }
        check(models.size()==required&&miss,"each of all ten types reaches every canonical move and can skip its opportunity");}
    bool multiple=false;for(std::uint64_t seed=0;seed<64&&!multiple;++seed){SnapshotState state;for(unsigned i=0;i<9;++i){auto p=player(i?100+i:0);p.hp=p.mp=p.cp=99;if(i)p.guardian=Guardian::neptune;state.players.push_back(p);}Match m;m.load_scene(state,0,seed);m.step(pray(0));multiple=m.snapshot().auto_guardian_actions>=2;
        check(m.snapshot().guardian_opportunities==8&&m.snapshot().decisions==1&&m.snapshot().rng==seed+(9+2*m.snapshot().auto_guardian_actions)*draw&&m.snapshot().owner==101&&m.snapshot().state.gf==1,"all eight eligible sources are processed once in input order, with singleton ally draws and a single final main gift");roundtrip(m);}
    check(multiple,"one normal decision can execute multiple self-utility guardian moves without extra decisions");
}

void guardian_departure_actual_stages_and_event_time_visibility(){
    bool left=false;for(std::uint64_t seed=0;seed<256&&!left;++seed){auto state=scene({6},{});state.players[0].curses={Curse::fog};state.players[1].guardian=Guardian::mars;Match m;m.load_scene(state,0,seed);m.step(play(0,{0},7));m.step(defend(7));if(!m.snapshot().guardian_departures)continue;left=true;
        check(m.snapshot().guardian_departure_rolls==1&&m.snapshot().guardian_opportunities==0&&m.snapshot().last_guardian_changes==std::vector<GuardianChange>{{7,Guardian::mars,Guardian::none,GuardianChangeReason::departure,1}},"positive target HP loss can retire its guardian before enemy-turn opportunities and records historical Fog mask");check(m.observe(0).find("\"last_guardian_changes\":[]")!=std::string::npos&&m.observe(7).find("\"reason\":\"departure\"")!=std::string::npos&&m.observe({}).find("\"reason\":\"departure\"")!=std::string::npos,"hidden change entry is completely filtered while owner and spectator receive it");roundtrip(m);}
    check(left,"seed sweep witnesses target guardian departure");
    bool historical=false;for(std::uint64_t seed=0;seed<4096&&!historical;++seed){auto state=scene({53},{});for(auto& p:state.players)p.hp=p.mp=p.cp=99;state.players[0].guardian=Guardian::mars;state.players[1].guardian=Guardian::neptune;state.players[1].curses={Curse::fog};Match m;m.load_scene(state,0,seed);m.step(play(0,{0},7));m.step(defend(7));if(m.snapshot().state.players[0].guardian!=Guardian::none||m.snapshot().last_auto_guardians.size()!=1||m.snapshot().last_auto_guardians[0].model!=275)continue;historical=true;
        check(m.snapshot().state.players[1].curses.empty()&&m.snapshot().last_guardian_changes[0].hidden_viewers==2&&m.observe(7).find("\"last_guardian_changes\":[]")!=std::string::npos,"later automatic cure of the viewer's Fog never discloses an earlier hidden departure");roundtrip(m);}
    check(historical,"seed sweep witnesses departure hidden at event time followed by same-action Fog cure");
    auto self=scene({27},{});self.players[0].hp=20;self.players[0].guardian=Guardian::mars;auto alias=fixture(self);alias.step(play(0,{0},0));alias.step(defend(0));check(alias.snapshot().state.players[0].hp==20&&alias.snapshot().guardian_departure_rolls==1,"self absorption net HP0 still has a positive target loss stage for retirement");roundtrip(alias);
    for(std::uint64_t seed=0;seed<16;++seed){auto state=scene({202},{});state.players[0].guardian=Guardian::mars;Match m;m.load_scene(state,0,seed);m.step(play(0,{0},0));check(m.snapshot().guardian_departure_rolls==(m.snapshot().state.players[0].hp<40?1u:0u),"negative random202 is direct HP loss while its healing outcome never rolls departure");roundtrip(m);}
    auto debt=scene();debt.players[0].guardian=Guardian::venus;debt.players[1].guardian=Guardian::mars;debt.players[1].cp=debt.players[1].mp=0;auto fine=fixture(debt);fine.guardian_step({0,282,7});fine.step(defend(7));check(fine.snapshot().state.players[1].hp==37&&fine.snapshot().guardian_departure_rolls==0&&fine.snapshot().guardian_opportunities==0,"CP debt does not roll departure, and external activation has no automatic follow-up");roundtrip(fine);
    auto sick=scene();sick.players[0].guardian=Guardian::mars;sick.players[0].curses={Curse::cold};auto tick=fixture(sick);tick.step(pray(0));check(tick.snapshot().last_tick&&tick.snapshot().guardian_departure_rolls==0,"disease tick loss never rolls departure");roundtrip(tick);
    auto exchange=scene({3},{});exchange.players[0].guardian=Guardian::mars;exchange.players[0].hp=1;exchange.players[0].mp=exchange.players[0].cp=0;auto fall=fixture(exchange);fall.step({0,CommandKind::exchange,{0},{},ResourceAllocation{0,1,0}});check(fall.done()&&fall.snapshot().guardian_departure_rolls==0,"deliberate exchange HP allocation is not a direct damage retirement stage");roundtrip(fall);
}

void guardian_auto_tick_history_counter_fifo_and_owner_death(){
    bool attacked=false;for(std::uint64_t seed=0;seed<256&&!attacked;++seed){auto state=scene();state.players[0].curses={Curse::cold,Curse::darkcloud};state.players[1].guardian=Guardian::mars;Match m;m.load_scene(state,0,seed);m.step(pray(0));if(m.snapshot().phase!=MatchPhase::response)continue;attacked=true;const auto tick=*m.snapshot().last_tick;const auto power=m.snapshot().queue[0].power;check(m.snapshot().state.players[0].items.empty()&&m.snapshot().gifts[0]==1&&m.snapshot().state.players[0].hp==tick.hp_after,"owner tick is completed and prayer gift stays reserved while automatic attack awaits response");roundtrip(m);m.step(defend(0));check(m.snapshot().last_tick==tick&&m.snapshot().state.players[0].hp==tick.hp_after-power&&m.snapshot().state.players[0].items.size()==1&&m.snapshot().decisions==2&&m.snapshot().owner==7&&m.snapshot().state.gf==1,"guardian body may change current HP after historical tick, with exactly one final gift and owner advance");roundtrip(m);}
    check(attacked,"automatic damage follows an owner tick without duplicating it");
    bool countered=false;for(std::uint64_t seed=0;seed<512&&!countered;++seed){SnapshotState state;state.players={player(0,{184}),player(7),player(9)};for(auto& p:state.players)p.hp=99;state.players[1].guardian=Guardian::jupiter;state.players[2].guardian=Guardian::neptune;Match m;m.load_scene(state,0,seed);m.step(pray(0));if(m.snapshot().phase!=MatchPhase::response||m.snapshot().queue[0].models!=std::vector<CardId>{255}||m.snapshot().actor!=0)continue;countered=true;m.step(defend(0,{0}));check(m.snapshot().queue[0].is_counter&&m.snapshot().guardian_opportunities==1&&m.snapshot().gifts[0]==2,"counter FIFO runs before the next guardian source or either main/response gift");roundtrip(m);m.step(defend(7));check(m.snapshot().guardian_opportunities==2&&m.snapshot().state.players[0].items.size()==2&&m.snapshot().state.gf==1&&m.snapshot().owner==7,"next source is considered once after counter and both gifts are delivered after all guardian work");roundtrip(m);}
    check(countered,"automatic damaging move can generate ordinary counter FIFO");
    bool posthumous=false;for(std::uint64_t seed=0;seed<256&&!posthumous;++seed){SnapshotState state;state.players={player(0,{191,112}),player(7),player(9)};for(auto& p:state.players){p.hp=99;p.curses={Curse::darkcloud};}state.players[1].guardian=Guardian::pluto;state.players[2].guardian=Guardian::neptune;Match m;m.load_scene(state,0,seed);m.step(play(0,{0},0));if(m.snapshot().phase!=MatchPhase::response||!m.snapshot().queue[0].power||m.snapshot().actor!=0)continue;posthumous=true;m.step(defend(0));check(m.snapshot().state.players[0].dead&&m.snapshot().queue[0].is_death&&m.snapshot().guardian_opportunities==1,"automatic Dark kill starts owner's posthumous FIFO before considering future guardian sources");roundtrip(m);m.step(defend(7));roundtrip(m);m.step(defend(9));check(m.snapshot().guardian_opportunities==1&&m.snapshot().owner==7&&m.snapshot().state.gf==1&&m.snapshot().state.players[0].items.empty(),"actual owner death cancels later opportunities and main gift after completing death FIFO");roundtrip(m);}
    check(posthumous,"automatic guardian attack reaches owner death and posthumous responses");
}

void guardian_auto_snapshot_forgery_and_late_failures(){
    bool covered=false;for(std::uint64_t seed=0;seed<256&&!covered;++seed){auto state=scene();state.players[0].curses={Curse::cold,Curse::darkcloud};state.players[1].guardian=Guardian::mars;Match m;m.load_scene(state,0,seed);m.step(pray(0));if(m.snapshot().phase!=MatchPhase::response)continue;covered=true;const auto exact=m.snapshot();for(unsigned error=0;error<15;++error){auto bad=exact;
            if(error==0)--bad.batch->guardian_cursor;if(error==1)bad.batch->guardian_stage_started=false;if(error==2)bad.batch->tick_processed=false;if(error==3)++bad.batch->guardian_opportunities;if(error==4)++bad.guardian_opportunities;if(error==5)++bad.auto_guardian_actions;if(error==6)++bad.last_auto_guardians[0].model;if(error==7)bad.last_auto_guardians[0].source=0;if(error==8)bad.last_guardian.reset();if(error==9)bad.queue[0].guardian=Guardian::jupiter;if(error==10)++bad.last_tick->hp_after;if(error==11)bad.last_guardian_changes.push_back({7,Guardian::mars,Guardian::none,GuardianChangeReason::departure,0});if(error==12)++bad.guardian_departure_rolls;if(error==13)++bad.batch->guardian_summons;if(error==14)++bad.rng;
            bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"same-main replay rejects altered auto cursor/stage/log/type/tick/statistics/visibility and RNG");}}
    check(covered,"pending automatic frame is covered by full original-main replay");
    for(unsigned reason=0;reason<2;++reason){auto state=scene({201},{});if(reason==0)state.update_count=max_wire_integer;else state.players[0].items[0].token=max_wire_integer;auto m=fixture(state);const auto exact=m.snapshot();bool failed=false;try{m.step(play(0,{state.players[0].items[0].token},0));}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"late update/gift failure restores summon type, consumed pot, public change and RNG");}
    bool healed=false;for(std::uint64_t seed=0;seed<256&&!healed;++seed){auto state=scene();state.players[1].guardian=Guardian::neptune;state.players[1].hp=10;state.players[1].mp=0;Match probe;probe.load_scene(state,0,seed);probe.step(pray(0));if(probe.snapshot().last_auto_guardians.empty()||probe.snapshot().last_auto_guardians[0].model==275)continue;healed=true;state.gf=max_wire_integer;Match m;m.load_scene(state,0,seed);const auto exact=m.snapshot();bool failed=false;try{m.step(pray(0));}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"late GF exhaustion rolls back completed internal guardian healing, opportunity/move/target draws, gifts and event logs");}
    check(healed,"internal self recovery rollback is exercised");
    bool exhausted=false;for(std::uint64_t seed=0;seed<256&&!exhausted;++seed){auto state=scene({191},{});state.players[0].hp=99;state.players[0].curses={Curse::darkcloud};state.players[0].items[0].token=max_wire_integer;state.players[1].guardian=Guardian::mars;Match m;m.load_scene(state,0,seed);try{m.step(play(0,{max_wire_integer},0));}catch(const MatchError&){continue;}if(m.snapshot().phase!=MatchPhase::response)continue;exhausted=true;const auto exact=m.snapshot();bool failed=false;try{m.step(defend(0));}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"late final gift failure restores pending automatic frame and completed owner's tick/consumption anchor");roundtrip(m);}
    check(exhausted,"automatic response rollback reaches deferred token exhaustion");
}

void guardian_auto_seeded_full_turns_replay_all_private_state(){
    for(unsigned count:{2u,9u})for(std::uint64_t seed=0;seed<16;++seed){SnapshotState state;for(unsigned i=0;i<count;++i){auto p=player(i?100+i:0,{201,239,204,6,190,189,178,184,235,208},count==9?static_cast<Team>(i%5):Team::solo);p.hp=p.mp=p.cp=99;p.guardian=static_cast<Guardian>(1+(seed+i)%10);if(i%2==0){p.curses={Curse::fog,Curse::dream};p.items[3].fake_model=57;}state.players.push_back(std::move(p));}MatchConfig cap;cap.max_decisions=96;Match a(cap),b(cap);a.load_scene(state,0,seed);b.load_scene(state,0,seed);check(a.step(play(0,{0},0))==b.step(play(0,{0},0))&&a.snapshot()==b.snapshot(),"summon and initial enemy guardian phase are deterministic from identical state/RNG");roundtrip(a);std::uint64_t pa=seed,pb=seed;
        while(!a.done()){const auto before=a.snapshot();const auto ca=a.sample_action(pa),cb=b.sample_action(pb);check(ca==cb&&pa==pb&&a.legal(ca)&&b.legal(cb)&&a.snapshot()==before,"player policy/legality remain pure while automatic guardian state is private");check(a.step(ca)==b.step(cb)&&a.snapshot()==b.snapshot(),"all subsequent NPC/main/response outcomes and event-time privacy masks are deterministic");roundtrip(a);}}
}

void guardian_payload_all_bases_and_inventory_independence(){
    unsigned earth=0,moon=0;
    for(const auto& card:catalog())for(auto type:{Guardian::earth,Guardian::moon})if(match_guardian_payload_supported(type,card.id)){
        type==Guardian::earth?++earth:++moon;auto state=scene({191,239},{});for(auto& p:state.players)p.hp=p.mp=p.cp=99;state.players[0].guardian=type;
        Match m;m.load_scene(state,7,31);const auto own_items=m.snapshot().state.players[0].items;GuardianPayloadAction action{0,type,{card.id},targets_enemy_by_default(card.id)?7u:0u};const auto before=m.snapshot();
        check(m.guardian_payload_legal(action)&&m.snapshot()==before,"ordinary guardian legality is pure and does not require the selected model in inventory");m.guardian_payload_step(action);roundtrip(m);
        unsigned responses=0;while(!m.done()&&m.snapshot().phase==MatchPhase::response){check(++responses<=2,"single ordinary payload terminates after at most two pass responses");m.step(defend(*m.snapshot().actor));roundtrip(m);}
        check(m.snapshot().guardian_payload_actions==1&&m.snapshot().guardian_actions==0&&m.snapshot().guardian_opportunities==0&&m.snapshot().auto_guardian_actions==0&&m.snapshot().state.gf==0&&!m.snapshot().last_tick,"all supported ordinary bases use one external activation without an ordinary tick, GF or automatic follow-up");
        check(m.snapshot().state.players[0].items==own_items&&m.snapshot().last_guardian_payload==action&&!m.snapshot().last_guardian,"ordinary external model is never obtained, retained or consumed as a physical source item");
        const auto view=m.observe({});check(view.find(guardian_payload_rule)!=std::string::npos&&view.find("\"last_guardian_payload\":{\"source\":0")!=std::string::npos&&view.find("guardian_payload_actions")==std::string::npos&&view.find("guardian_payload_mp")==std::string::npos,"selected ordinary payload is public while native counters and launch MP remain private");
    }
    check(earth==120&&moon==26,"standalone capabilities are exactly Earth120 and Moon26");for(auto type:{Guardian::none,Guardian::mars,static_cast<Guardian>(255)})check(!match_guardian_payload_supported(type,6),"non Earth/Moon capability is empty");
}
void guardian_payload_domains_shapes_and_strict_composition(){
    auto state=scene();state.players[0].guardian=Guardian::earth;Match m;m.load_scene(state,7,5);
    auto reject_payload=[&](GuardianPayloadAction action){const auto before=m.snapshot();check(!m.guardian_payload_legal(action)&&m.snapshot()==before,"invalid payload legality is pure");bool failed=false;try{m.guardian_payload_step(action);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==before,"invalid payload leaves complete state unchanged");};
    for(auto action:std::vector<GuardianPayloadAction>{{0,Guardian::earth,{},7},{0,Guardian::earth,{6},{}},{0,Guardian::moon,{235},7},{7,Guardian::earth,{6},0},{999,Guardian::earth,{6},0},{0,Guardian::earth,{6},999},{0,Guardian::mars,{6},7},{0,Guardian::earth,{3},7},{0,Guardian::earth,{4,6},7},{0,Guardian::earth,{5},7},{0,Guardian::earth,{94},7},{0,Guardian::earth,{178},7},{0,Guardian::earth,{203},7},{0,Guardian::earth,{204},7},{0,Guardian::earth,{208},7},{0,Guardian::earth,{6,210},7},{0,Guardian::earth,{6,231},7},{0,Guardian::earth,{6,232},7},{0,Guardian::earth,{285,6},7},{0,Guardian::earth,{201,6},7},{0,Guardian::earth,{207},7}})reject_payload(action);
    GuardianPayloadAction large{0,Guardian::earth,{6},7};for(unsigned i=1;i<19;++i)large.models.push_back(203);reject_payload(large);large.models.pop_back();check(m.guardian_payload_legal(large),"exactly18 syntactically valid model entries are allowed without owning copies");
    state.players[0].guardian=Guardian::moon;m.load_scene(state,7,5);for(auto models:std::vector<std::vector<CardId>>{{231},{232},{233},{234},{235,204},{210,231},{210,232},{286,210},{6}})reject_payload({0,Guardian::moon,models,7});
    check(m.guardian_payload_legal({0,Guardian::moon,{239},0}),"Moon may externally summon with MP0 without being charged a normal casting fee");
}
void guardian_payload_mana_reflection_and_launch_context(){
    auto state=scene({191},{190});for(auto& p:state.players)p.hp=99;state.gf=11;state.players[0].guardian=Guardian::earth;state.players[0].mp=17;state.players[0].curses={Curse::fog};state.players[1].curses={Curse::cold};Match m;m.load_scene(state,7,13);
    GuardianPayloadAction action{0,Guardian::earth,{58,66,203},7};m.guardian_payload_step(action);check(m.snapshot().queue[0].origin==0&&m.snapshot().queue[0].source==0&&m.snapshot().queue[0].power==46&&!m.snapshot().queue[0].element&&m.snapshot().queue[0].category==Category::weapons&&m.snapshot().queue[0].guardian==Guardian::earth&&m.snapshot().queue[0].tokens.empty()&&m.snapshot().state.players[0].mp==0,"ordinary mana payload uses actual source MP, ordered extras, original category and no fake inventory tokens");roundtrip(m);
    m.step(defend(7,{0}));check(m.snapshot().queue[0].source==7&&m.snapshot().queue[0].origin==0&&m.snapshot().queue[0].target==0&&m.snapshot().queue[0].power==46&&m.snapshot().gifts[1]==1&&m.snapshot().state.players[1].items.empty(),"reflection keeps captured payload power/identity and defers only the defender's item gift");roundtrip(m);m.step(defend(0));
    check(m.snapshot().state.players[0].hp==53&&m.snapshot().state.players[0].mp==0&&m.snapshot().state.players[0].items.size()==1&&m.snapshot().state.players[1].items.size()==1&&m.snapshot().owner==7&&m.snapshot().state.gf==11&&!m.snapshot().last_tick&&m.snapshot().last_guardian_payload==action,"external mana body does not retick the ordinary owner, charge twice or grant source operation gifts");roundtrip(m);
    state.players[0].guardian=Guardian::moon;state.players[0].mp=0;state.players[0].items.clear();state.players[1].items={{0,50,{},false}};m.load_scene(state,7,0);m.guardian_payload_step({0,Guardian::moon,{235},7});check(m.snapshot().queue[0].category==Category::miracles&&m.snapshot().queue[0].guardian==Guardian::moon&&m.legal(defend(7,{0})),"ordinary Moon recovery is a miracle for miracle-only reactions");m.step(defend(7,{0}));check(m.snapshot().state.players[0].items.empty()&&m.snapshot().state.players[0].mp==0,"Moon miracle is never retained or charged as a player spell");roundtrip(m);
}
void guardian_payload_repeat_counter_fifo_and_fixed_identity(){
    SnapshotState state;state.players={player(0),player(7,{184}),player(9)};for(auto& p:state.players)p.hp=99;state.players[0].guardian=Guardian::earth;Match m;m.load_scene(state,9,24);GuardianPayloadAction action{0,Guardian::earth,{14,83,203},7};m.guardian_payload_step(action);check(m.snapshot().batch->targets==std::vector<PlayerId>{7,7}&&m.snapshot().queue[0].power==18,"ordinary repeat builds two fresh hits toward the explicit target");roundtrip(m);
    m.step(defend(7,{0}));check(m.snapshot().queue[0].is_counter&&m.snapshot().batch->cursor==1&&m.snapshot().gifts[1]==1,"counter FIFO precedes the second ordinary guardian hit and its reserved response gift");roundtrip(m);m.step(defend(0));check(!m.snapshot().queue[0].is_counter&&m.snapshot().state.players[0].guardian==Guardian::none&&m.snapshot().queue[0].power==18&&m.snapshot().queue[0].guardian==Guardian::earth&&m.snapshot().queue[0].source==0,"second hit rebuilds original launch even if source's guardian departs during the counter");roundtrip(m);m.step(defend(7));check(m.snapshot().owner==9&&m.snapshot().state.gf==0&&m.snapshot().state.players[1].hp==63&&m.snapshot().state.players[2].hp==99&&m.snapshot().decisions==4,"repeat does not steal the third player's ordinary turn or widen its target roster");roundtrip(m);
}
void guardian_payload_snapshot_forgery_cutoff_and_late_rollback(){
    auto state=scene({191},{190});state.players[0].guardian=Guardian::earth;state.players[0].mp=17;for(auto& p:state.players)p.hp=99;Match m;m.load_scene(state,7,0);m.guardian_payload_step({0,Guardian::earth,{58,66,203},7});const auto exact=m.snapshot();
    for(unsigned error=0;error<14;++error){auto bad=exact;if(error==0)++bad.batch->guardian_payload_mp;if(error==1)bad.batch->guardian_payload_action->source=7;if(error==2)bad.batch->guardian_payload_action->guardian=Guardian::moon;if(error==3)bad.batch->guardian_payload_action->models={58,83,203};if(error==4)++bad.batch->guardian_payload_actions;if(error==5)++bad.guardian_payload_actions;if(error==6)bad.last_guardian_payload.reset();if(error==7)bad.last_guardian_payload->models={6};if(error==8)bad.queue[0].guardian=Guardian::moon;if(error==9)bad.batch->tick_processed=false;if(error==10)bad.batch->guardian_action=GuardianAction{0,285,0};if(error==11)++bad.queue[0].power;if(error==12)++bad.rng;if(error==13)bad.batch->initial_state.players[0].mp=18;bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"original-source payload journal rejects altered identity, mana, model/order, counters, event, metadata and RNG");}
    state.update_count=max_wire_integer;Match late;late.load_scene(state,7,0);const auto before=late.snapshot();bool failed=false;try{late.guardian_payload_step({0,Guardian::earth,{58},7});}catch(const MatchError&){failed=true;}check(failed&&late.snapshot()==before,"late update exhaustion rolls back payload mana drain and activation");
    state.update_count=0;state.players[0].items[0].token=max_wire_integer;late.load_scene(state,7,0);late.guardian_payload_step({0,Guardian::earth,{58},7});late.step(defend(7,{0}));const auto pending=late.snapshot();failed=false;try{late.step(defend(0));}catch(const MatchError&){failed=true;}check(failed&&late.snapshot()==pending,"late response gift exhaustion restores reflected payload and original mana anchor");roundtrip(late);
    MatchConfig cap;cap.max_decisions=1;state.players[0].items.clear();Match cut(cap);cut.load_scene(state,7,0);cut.guardian_payload_step({0,Guardian::earth,{58},7});check(cut.snapshot().truncated&&cut.snapshot().state.players[0].mp==0&&cut.snapshot().state.players[1].hp==99&&cut.snapshot().last_guardian_payload.has_value(),"cutoff cancels the pending body while preserving committed external launch and one-time mana drain");roundtrip(cut);
}
void guardian_payload_summon_random_and_seeded_replay(){
    for(unsigned count:{2u,9u})for(std::uint64_t seed=0;seed<16;++seed){SnapshotState state;for(unsigned i=0;i<count;++i){auto p=player(i?100+i:0,{190,189,178,184,235});p.hp=p.mp=p.cp=99;state.players.push_back(p);}const auto moon=seed%2==0;state.players[0].guardian=moon?Guardian::moon:Guardian::earth;state.players[0].curses={Curse::fog};const CardId model=moon?static_cast<CardId>(seed%4==0?239:235):static_cast<CardId>(seed%4==1?202:201);GuardianPayloadAction action{0,state.players[0].guardian,{model},0};MatchConfig cap;cap.max_decisions=64;Match a(cap),b(cap);a.load_scene(state,101,seed);b.load_scene(state,101,seed);check(a.guardian_payload_step(action)==b.guardian_payload_step(action)&&a.snapshot()==b.snapshot(),"self summon/random/recovery payload replays every private field");roundtrip(a);check(a.snapshot().last_guardian_payload==action&&a.snapshot().state.players[0].items==state.players[0].items&&a.snapshot().guardian_opportunities==0,"summon or departure never rewrites the already launched guardian identity or triggers extra automatic opportunities");
        if(model==201||model==239)check(a.snapshot().guardian_summons==1&&a.snapshot().last_guardian_changes.size()==1,"ordinary guardian summoning reaches the existing ten-type primitive once");std::uint64_t pa=seed,pb=seed;
        while(!a.done()){const auto before=a.snapshot();const auto ca=a.sample_action(pa),cb=b.sample_action(pb);check(ca==cb&&pa==pb&&a.legal(ca)&&a.snapshot()==before,"ordinary player policy remains pure after external ordinary guardian payload");check(a.step(ca)==b.step(cb)&&a.snapshot()==b.snapshot(),"later normal auto/response/main play remains reproducible after external general-model launch");roundtrip(a);}}
}

SnapshotState phenomenon_scene(unsigned count,bool dead_last=false){
    SnapshotState s;s.gf=37;
    for(unsigned i=0;i<count;++i){auto p=player(i?100+13*i:0,{190,178,208,112,239},i%2?Team::two:Team::one);p.hp=7+i;p.mp=33+i;p.cp=11*i;p.guardian=static_cast<Guardian>(1+i%10);p.curses={static_cast<Curse>(1+i%4)};
        if(i%2==0)p.curses.insert(p.curses.end(),{Curse::fog,Curse::dream,Curse::flash,Curse::darkcloud});else p.curses.push_back(Curse::dream);
        if(dead_last&&i==count-1){p.dead=true;p.hp=0;}s.players.push_back(std::move(p));}
    return s;
}
PhenomenonAction global_action(CardId model,const SnapshotState& state){
    PhenomenonAction a{model,{},{}};if(model==294)a.recipient=state.players.front().id;
    if(model==296)for(const auto& p:state.players)if(!p.dead)a.guardians.push_back(p.guardian);return a;
}
void phenomenon_capabilities_fever_replacement_and_atomic_roles(){
    const std::set<CardId> supported{287,288,289,290,291,292,293,294,296};for(CardId id=0;id<=300;++id)check(match_phenomenon_supported(id)==supported.contains(id),"only the nine implemented global or targeted phenomena are advertised");for(auto id:supported)check(!match_supported(id),"phenomena are never ordinary hand/gift models");check(!match_supported(207)&&match_catalog().size()==236&&match_gift_weight()==499,"Fate selector and ordinary gift pool remain unchanged");
    auto state=phenomenon_scene(9,true);Match m;m.load_scene(state,0,123);const auto before=m.snapshot();PhenomenonAction a{287,{},{}};check(m.phenomenon_legal(a)&&m.snapshot()==before,"global legality is pure");m.phenomenon_step(a);
    check(m.snapshot().phenomenon_actions==1&&m.snapshot().decisions==1&&m.snapshot().state.update_count==1&&m.snapshot().rng==123&&m.snapshot().state.gf==37&&m.snapshot().owner==0&&m.snapshot().actor==0&&m.snapshot().phase==MatchPhase::main&&!m.snapshot().batch,"source-free global pulse consumes exactly one decision without RNG, ordinary turn or batch");
    for(std::size_t i=0;i<state.players.size();++i){auto expected=state.players[i];if(!expected.dead){std::erase_if(expected.curses,[](auto c){return c>=Curse::cold&&c<=Curse::heaven;});expected.curses.insert(expected.curses.begin(),Curse::fever);}check(m.snapshot().state.players[i]==expected,"Fever directly replaces all existing disease ranks including Heaven without HP, inventory, guardian, revive or death effects");}
    check(m.snapshot().revivals==0&&m.snapshot().death_bows==0&&m.snapshot().guardian_departure_rolls==0&&m.snapshot().guardian_opportunities==0&&m.snapshot().last_phenomenon==a&&!m.snapshot().last_tick,"direct replacement never calls ordinary disease reapplication, HP loss triggers or automatic guardians");roundtrip(m);
}
void phenomenon_fog_idempotence_and_hp_setter_are_not_damage(){
    auto state=phenomenon_scene(9,true);Match m;m.load_scene(state,0,19);m.phenomenon_step({288,{},{}});const auto first=m.snapshot();for(const auto& p:first.state.players){const auto& old=*find_player(state,p.id);auto expected=old;if(!p.dead&&std::find(expected.curses.begin(),expected.curses.end(),Curse::fog)==expected.curses.end())expected.curses.insert(std::lower_bound(expected.curses.begin(),expected.curses.end(),Curse::fog),Curse::fog);check(p==expected,"global Fog includes every alive team and leaves dead records untouched");}
    m.phenomenon_step({288,{},{}});check(m.snapshot().state.players==first.state.players&&m.snapshot().phenomenon_actions==2&&m.snapshot().rng==19,"repeated global Fog is idempotent without an outcome draw");roundtrip(m);const auto before=m.snapshot();m.phenomenon_step({290,{},{}});
    for(std::size_t i=0;i<state.players.size();++i){auto expected=before.state.players[i];if(!expected.dead)expected.hp=1;check(m.snapshot().state.players[i]==expected,"HP1 is a direct alive-only setter, not damage or a change to MP/CP/inventory/guardian");}
    check(m.snapshot().guardian_departure_rolls==0&&m.snapshot().counter_created==0&&m.snapshot().revivals==0&&m.snapshot().death_created==0&&m.snapshot().rng==19&&m.snapshot().owner==0&&m.snapshot().state.gf==37,"HP1 cannot trigger departure, counter, revival, death or ordinary progression");roundtrip(m);
}
void phenomenon_cp_collection_recipient_alias_debt_and_cap(){
    for(unsigned count:{2u,9u})for(unsigned recipient=0;recipient<count;++recipient){auto state=phenomenon_scene(count);if(count==9){state.players.back().dead=true;state.players.back().hp=0;state.players.back().cp=99;if(recipient==count-1)continue;}std::int64_t total=0;for(auto& p:state.players){if(!p.dead){p.cp=17+static_cast<int>(p.id%83);total+=p.cp;}p.mp=0;}Match m;m.load_scene(state,0,7);const auto target=state.players[recipient].id;m.phenomenon_step({294,target,{}});
        for(std::size_t i=0;i<state.players.size();++i){auto expected=state.players[i];if(!expected.dead)expected.cp=expected.id==target?std::min<std::int64_t>(99,total):0;check(m.snapshot().state.players[i]==expected,"collection captures alive sum before zeroing every CP including recipient, caps positive credit and never creates MP/HP debt");}
        const auto public_view=m.observe({});check(public_view.find("\"last_phenomenon\":{\"model\":294,\"recipient\":"+std::to_string(target)+"}")!=std::string::npos&&public_view.find("collected_cp")==std::string::npos,"collection publishes the chosen recipient without a hidden CP total");check(m.snapshot().rng==7&&m.snapshot().state.gf==37&&m.snapshot().phenomenon_actions==1,"deterministic collection does not consume RNG or advance GF");roundtrip(m);}
    auto zeros=phenomenon_scene(2);for(auto& p:zeros.players)p.cp=0;Match m;m.load_scene(zeros,0,0);m.phenomenon_step({294,0,{}});check(m.snapshot().state.players[0].cp==0&&m.snapshot().state.players[1].cp==0,"zero collection is a valid direct identity case");roundtrip(m);
}
void phenomenon_guardian_nine_assignments_event_time_fog_and_cutoff(){
    auto state=phenomenon_scene(9);auto action=global_action(296,state);action.guardians[1]=Guardian::moon;Match m;m.load_scene(state,0,31);m.phenomenon_step(action);const auto exact=m.snapshot();check(exact.last_guardian_changes.size()==9&&exact.guardian_summons==0&&exact.guardian_payload_actions==0&&exact.guardian_actions==0&&exact.guardian_opportunities==0&&exact.rng==31,"nine explicit assignments include same-type replacements without ordinary summon counters or extra RNG");
    const auto fog=m.observe(0),spectator=m.observe({});check(fog.find("\"last_phenomenon\":{\"model\":296,\"recipient\":null}")!=std::string::npos&&fog.find("\"guardians\":")==std::string::npos,"global announcement never serializes its privileged guardian assignment array");
    for(unsigned i=0;i<9;++i){const auto& event=exact.last_guardian_changes[i];check(event.player==state.players[i].id&&event.before==state.players[i].guardian&&event.after==action.guardians[i]&&event.reason==GuardianChangeReason::phenomenon,"guardian change records preserve alive input order and same-type before/after");const auto marker="\"player\":"+std::to_string(event.player)+",";check((fog.find(marker)!=std::string::npos)==(i%2==0)&&spectator.find(marker)!=std::string::npos,"event-time Fog exposes only the viewer and its teammates; spectator sees all changes");}
    roundtrip(m);
    for(unsigned error=0;error<11;++error){auto bad=exact;if(error==0)bad.last_phenomenon->guardians.pop_back();if(error==1)bad.last_phenomenon->guardians[0]=Guardian::none;if(error==2)bad.last_guardian_changes.pop_back();if(error==3)bad.last_guardian_changes[1].hidden_viewers=0;if(error==4)bad.last_guardian_changes[1].after=Guardian::mars;if(error==5)bad.state.players[1].guardian=Guardian::mars;if(error==6)bad.last_guardian_changes[0].reason=GuardianChangeReason::summon;if(error==7)bad.phenomenon_actions=0;if(error==8)bad.guardian_actions=1;if(error==9)bad.last_phenomenon->recipient=0;if(error==10)bad.last_phenomenon->model=287;bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"completed global postconditions reject forged assignment count/type/result/mask/event/count and inactive fields");}
    MatchConfig cap;cap.max_decisions=1;Match cut(cap);cut.load_scene(state,0,31);cut.phenomenon_step(action);check(cut.snapshot().truncated&&cut.snapshot().last_guardian_changes.size()==9&&cut.snapshot().last_phenomenon==action&&cut.snapshot().state.gf==37&&cut.snapshot().rng==31,"cutoff occurs after all nine atomic assignments, preserving their filtered history");roundtrip(cut);
}
void phenomenon_strict_shapes_pending_exclusion_and_late_rollback(){
    auto state=phenomenon_scene(9,true);Match m;m.load_scene(state,0,5);m.phenomenon_step({288,{},{}});
    const auto reject_global=[&](PhenomenonAction action){const auto before=m.snapshot();check(!m.phenomenon_legal(action)&&m.snapshot()==before,"invalid global legality leaves the previous action intact");bool failed=false;try{m.phenomenon_step(action);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==before,"invalid global shape leaves the complete state and public event unchanged");};
    for(auto a:std::vector<PhenomenonAction>{{0,{},{}},{207,{},{}},{297,{},{}},{291,{},{}},{292,{},{}},{293,{},{}},{295,{},{}},{287,0,{}},{288,{}, {Guardian::mars}},{290,0,{}},{294,{},{}},{294,state.players.back().id,{}},{294,999,{}},{294,0,{Guardian::mars}},{296,{},{}},{296,{},std::vector<Guardian>(7,Guardian::mars)},{296,{},std::vector<Guardian>(9,Guardian::mars)},{296,0,std::vector<Guardian>(8,Guardian::mars)},{296,{},std::vector<Guardian>(8,Guardian::none)},{296,{},std::vector<Guardian>(8,static_cast<Guardian>(255))}})reject_global(a);
    m=fixture(scene({6},{}));m.step(play(0,{0},7));reject_global({290,{},{}});m=fixture(scene({5},{6}));m.step({0,CommandKind::buy,{0},7});m.step(defend(7));check(m.snapshot().phase==MatchPhase::purchase,"purchase is genuinely pending before a global rejection");reject_global({290,{},{}});
    for(auto model:{CardId{287},CardId{288},CardId{290},CardId{294},CardId{296}}){auto broken=state;broken.update_count=max_wire_integer;Match late;late.load_scene(broken,0,7);const auto before=late.snapshot();const auto action=global_action(model,broken);check(late.phenomenon_legal(action),"update counter exhaustion is late rather than an input-shape error");bool failed=false;try{late.phenomenon_step(action);}catch(const MatchError&){failed=true;}check(failed&&late.snapshot()==before,"late update failure rolls back every already-applied global resource/curse/guardian and event");}
}
void phenomenon_prefix_counters_survive_all_later_action_replay(){
    for(unsigned count:{2u,9u})for(std::uint64_t seed=0;seed<16;++seed){auto state=phenomenon_scene(count);MatchConfig cap;cap.max_decisions=96;Match a(cap),b(cap);a.load_scene(state,0,seed);b.load_scene(state,0,seed);
        for(auto model:{CardId{287},CardId{288},CardId{290},CardId{294},CardId{296}}){auto action=global_action(model,a.snapshot().state);if(model==296){action.guardians[0]=Guardian::earth;for(unsigned i=1;i<count;++i)action.guardians[i]=static_cast<Guardian>(1+(seed+i)%10);}check(a.phenomenon_step(action)==b.phenomenon_step(action)&&a.snapshot()==b.snapshot(),"each source-free deterministic global prefix replays the complete private state");roundtrip(a);}
        check(a.snapshot().phenomenon_actions==5&&a.snapshot().rng==seed&&a.snapshot().state.gf==37,"global prefixes add no RNG consumption or ordinary turns");GuardianPayloadAction payload{0,Guardian::earth,{58},state.players[1].id};check(a.guardian_payload_step(payload)==b.guardian_payload_step(payload)&&a.snapshot()==b.snapshot(),"later external payload carries the global cumulative prefix into its launch anchor");roundtrip(a);std::uint64_t pa=seed,pb=seed;
        while(!a.done()){const auto before=a.snapshot();const auto ca=a.sample_action(pa),cb=b.sample_action(pb);check(ca==cb&&pa==pb&&a.legal(ca)&&a.snapshot()==before,"policy remains pure after deterministic global effects");check(a.step(ca)==b.step(cb)&&a.snapshot()==b.snapshot()&&a.snapshot().phenomenon_actions==5,"all normal/counter/death/redraw/auto continuations retain the global prefix exactly");roundtrip(a);}}
}

void targeted_phenomenon_source_absence_and_light_filter(){
    SnapshotState state;state.gf=17;state.players={player(7,{113,189,190,38,233,234}),player(9),player(23)};for(auto& p:state.players)p.hp=99;state.players[2].curses={Curse::cold};Match m;m.load_scene(state,23,5);m.phenomenon_step({291,7,{}});const auto& s=m.snapshot();
    check(!find_player(s.state,0)&&s.queue[0].environmental&&s.queue[0].origin==0&&s.queue[0].source==0&&s.queue[0].target==7&&s.queue[0].category==Category::phenomena&&s.queue[0].element==Element::light&&s.queue[0].power==50&&s.queue[0].guardian==Guardian::none,"environmental presence flag, rather than ID0, defines absent origin/source for the exact Light50 payload");
    check(s.batch->phenomenon_action==PhenomenonAction{291,7,{}}&&s.batch->tick_processed&&s.owner==23&&s.actor==7&&s.rng==5,"targeted world launch has its own external journal without using the ordinary owner as source");const auto view=m.observe({});check(view.find("\"origin\":null,\"source\":null,\"target\":7")!=std::string::npos&&view.find("\"environmental\":true")!=std::string::npos,"public world source is null even when roster has no player0");roundtrip(m);
    reject(m,defend(7,{0}));reject(m,defend(7,{3}));reject(m,defend(7,{4}));reject(m,defend(7,{5}));check(m.legal(defend(7,{1,0})),"Light rejects raw ordinary DEF and weapon/MC reactions but admits filter followed by DEF");m.step(defend(7,{1,0}));check(m.snapshot().state.players[0].hp==50&&m.snapshot().state.players[0].items.size()==6&&m.snapshot().owner==23&&m.snapshot().actor==23&&m.snapshot().state.gf==17&&m.snapshot().state.players[2].hp==99&&!m.snapshot().last_tick,"filtered Light applies49 actual damage and only response gifts, without ticking or advancing an unrelated ordinary owner");roundtrip(m);
}
void targeted_phenomenon_dark_single_chance_force_and_full_defense(){
    bool hit=false,miss=false;constexpr std::uint64_t draw=0x9e3779b97f4a7c15ULL;
    for(std::uint64_t seed=0;seed<64&&(!hit||!miss);++seed){auto state=scene();state.players[0].hp=99;Match m;m.load_scene(state,7,seed);m.phenomenon_step({292,0,{}});const auto pending=m.snapshot().phase==MatchPhase::response;hit|=pending;miss|=!pending;check(m.snapshot().chance_rolls==1&&m.snapshot().chance_hits==(pending?1u:0u)&&m.snapshot().chance_forced_hits==0&&m.snapshot().rng==seed+draw,"BlackHole rolls once for its explicit target, without an enemy roster or target draw");roundtrip(m);
        if(pending){m.step(defend(0));check(m.snapshot().terminated&&m.snapshot().state.players[0].dead&&m.snapshot().state.players[1].hp==40&&m.snapshot().last_phenomenon==PhenomenonAction{292,0,{}},"positive Dark remainder kills only its chosen target and completed history accepts a dead recipient");roundtrip(m);}}
    check(hit&&miss,"BlackHole seeds cover hit and miss");auto state=scene();state.players[0].curses={Curse::darkcloud};state.players[0].hp=99;const auto armor=std::find_if(catalog().begin(),catalog().end(),[](const auto& c){return c.category==Category::armor&&c.defense>=15&&c.ability==Ability::none;})->id;state.players[0].items={{0,armor,{},false},{1,armor,{},false}};Match m;m.load_scene(state,7,0);m.phenomenon_step({292,0,{}});check(m.snapshot().chance_rolls==0&&m.snapshot().chance_forced_hits==1&&m.snapshot().rng==0&&m.snapshot().queue[0].forced_hit,"DarkCloud forces the world hit without a draw");m.step(defend(0,{0,1}));check(m.snapshot().state.players[0].hp==99&&!m.snapshot().state.players[0].dead&&m.snapshot().guardian_departure_rolls==0,"sufficient normal defense prevents the Dark HP0 body and departure");roundtrip(m);
}
void targeted_phenomenon_warm_response_cap_and_environmental_cancel(){
    for(int hp:{5,70,99})for(bool mirror:{false,true}){auto state=mirror?scene({190},{}):scene();state.players[0].hp=hp;state.players[1].curses={Curse::cold};Match m;m.load_scene(state,7,41);m.phenomenon_step({293,0,{}});check(m.snapshot().phase==MatchPhase::response&&m.snapshot().queue[0].environmental&&m.snapshot().queue[0].power==0&&m.snapshot().queue[0].amount==50&&m.snapshot().queue[0].guardian==Guardian::none&&m.snapshot().state.players[0].hp==hp,"WarmCurrent is a selected single-target source-free response, never an implicit global/self heal");roundtrip(m);m.step(mirror?defend(0,{0}):defend(0));
        check(m.snapshot().state.players[0].hp==(mirror?hp:std::min(99,hp+50))&&m.snapshot().state.players[0].items.size()==(mirror?1u:0u)&&m.snapshot().owner==7&&m.snapshot().state.gf==0&&m.snapshot().state.players[1].hp==40&&!m.snapshot().last_tick&&m.snapshot().phenomenon_actions==1,"190 consumes/reserves its response gift then cancels absent-source recovery; pass clips HP+50 without operation gifts or owner tick");roundtrip(m);}
    auto m=fixture(scene({6},{190}));m.step(play(0,{0},7));check(!m.snapshot().queue[0].environmental&&m.observe({}).find("\"origin\":0,\"source\":0,\"target\":7")!=std::string::npos,"a real player0 source remains numeric and distinct from environmental null");m.step(defend(7,{0}));check(m.snapshot().phase==MatchPhase::response&&m.snapshot().actor==0&&!m.snapshot().queue[0].environmental,"ordinary reflection still returns to the real player0 instead of cancelling");roundtrip(m);
}
void targeted_phenomenon_source_counters_skip_but_self_mp_is_real(){
    for(CardId counter:{181,182,183,184,185,186,188}){auto state=scene({counter,208},{});state.players[0].curses={Curse::darkcloud};state.players[0].mp=0;Match m;m.load_scene(state,7,5);m.phenomenon_step({292,0,{}});check(m.legal(defend(0,{0})),"public counter armor selection is preserved against Dark world attacks");m.step(defend(0,{0}));check(m.snapshot().revivals==1&&m.snapshot().state.players[0].hp==10&&m.snapshot().state.players[0].items.size()==2&&m.snapshot().counter_created==0&&m.snapshot().counter_started==0&&m.snapshot().state.players[1].hp==40&&m.snapshot().state.players[1].mp==10&&m.snapshot().state.players[1].cp==20,"absent source suppresses all seven source-directed counters after normal consumption, body and revival");roundtrip(m);}
    auto state=scene({187,208,190},{});state.players[0].curses={Curse::darkcloud};state.players[0].mp=0;Match m;m.load_scene(state,7,5);m.phenomenon_step({292,0,{}});m.step(defend(0,{0}));check(m.snapshot().queue[0].is_counter&&!m.snapshot().queue[0].environmental&&m.snapshot().queue[0].source==0&&m.snapshot().queue[0].target==0&&m.snapshot().queue[0].origin==0&&m.snapshot().queue[0].amount==80&&m.snapshot().counter_created==1,"187 creates a genuine player0 self-counter from actual40 HP loss, not an inherited world source");check(m.observe({}).find("\"origin\":0,\"source\":0,\"target\":0")!=std::string::npos,"derived self-counter publicly identifies real player0");roundtrip(m);m.step(defend(0,{2}));check(m.snapshot().phase==MatchPhase::response&&!m.snapshot().queue[0].environmental&&m.snapshot().actor==0,"reflecting a genuine self-counter does not use environmental cancellation");roundtrip(m);m.step(defend(0));check(m.snapshot().state.players[0].mp==80&&m.snapshot().owner==7&&m.snapshot().state.gf==0&&!m.snapshot().last_tick,"self MP effect resolves after its own response and preserves the ordinary owner");roundtrip(m);
}
void targeted_phenomenon_owner_death_fifo_and_cutoff(){
    SnapshotState state;state.gf=13;state.players={player(0,{112}),player(7),player(9)};for(auto& p:state.players){p.hp=99;p.curses={Curse::darkcloud};}state.players[0].hp=40;Match m;m.load_scene(state,0,7);m.phenomenon_step({292,0,{}});m.step(defend(0));check(m.snapshot().state.players[0].dead&&m.snapshot().queue[0].is_death&&!m.snapshot().queue[0].environmental&&m.snapshot().queue[0].source==0&&m.snapshot().death_created==2&&m.snapshot().actor==7,"world kill of the ordinary owner starts genuine posthumous player0 FIFO before returning to a survivor");roundtrip(m);m.step(defend(7));roundtrip(m);m.step(defend(9));check(m.snapshot().phase==MatchPhase::main&&m.snapshot().owner==7&&m.snapshot().state.gf==13&&m.snapshot().state.players[1].hp==69&&m.snapshot().state.players[2].hp==69&&!m.snapshot().last_tick,"all death responses finish without inventing a world origin or ordinary tick/GF advance");roundtrip(m);
    auto duel=fixture(scene({112},{}));duel.phenomenon_step({291,0,{}});duel.step(defend(0));check(duel.snapshot().terminated&&duel.snapshot().death_created==0&&duel.snapshot().last_phenomenon==PhenomenonAction{291,0,{}},"natural terminal precedes posthumous generation for a world kill");roundtrip(duel);
    MatchConfig cap;cap.max_decisions=1;auto original=scene();original.players[0].hp=5;Match cut(cap);cut.load_scene(original,7,31);cut.phenomenon_step({293,0,{}});check(cut.snapshot().truncated&&cut.snapshot().state.players[0].hp==5&&cut.snapshot().last_phenomenon==PhenomenonAction{293,0,{}},"targeted cutoff cancels the pending body, unlike immediate global setters");roundtrip(cut);
}
void targeted_phenomenon_replay_forgery_rollback_and_seeded_turns(){
    auto state=scene({},{});state.players.push_back(player(9));Match m;m.load_scene(state,7,3);m.phenomenon_step({291,0,{}});const auto exact=m.snapshot();for(unsigned error=0;error<13;++error){auto bad=exact;if(error==0)bad.queue[0].environmental=false;if(error==1)bad.queue[0].source=7;if(error==2)bad.queue[0].origin=7;if(error==3)bad.queue[0].guardian=Guardian::neptune;if(error==4)bad.batch->phenomenon_action.reset();if(error==5)bad.batch->phenomenon_action->recipient=9;if(error==6)bad.batch->tick_processed=false;if(error==7)bad.last_phenomenon.reset();if(error==8)bad.last_phenomenon->model=293;if(error==9)++bad.queue[0].power;if(error==10)++bad.rng;if(error==11)++bad.phenomenon_actions;if(error==12)bad.batch->guardian_payload_action=GuardianPayloadAction{0,Guardian::earth,{6},7};bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"source-free original action replay rejects forged presence/unused IDs/identity/target/phase/counters and RNG");}
    for(CardId model:{291,292,293}){auto broken=scene();broken.update_count=max_wire_integer;broken.players[0].curses={Curse::darkcloud};Match late;late.load_scene(broken,7,7);const auto before=late.snapshot();bool failed=false;try{late.phenomenon_step({model,0,{}});}catch(const MatchError&){failed=true;}check(failed&&late.snapshot()==before,"late launch failure rolls back source-free frame, chance activation, RNG and public event");}
    auto tokens=scene({191},{190});tokens.players[0].items[0].token=max_wire_integer;Match late;late.load_scene(tokens,0,0);late.phenomenon_step({293,7,{}});const auto pending=late.snapshot();bool failed=false;try{late.step(defend(7,{0}));}catch(const MatchError&){failed=true;}check(failed&&late.snapshot()==pending,"late cancellation gift exhaustion restores the entire environmental response transaction");roundtrip(late);
    for(unsigned count:{2u,9u})for(std::uint64_t seed=0;seed<16;++seed){SnapshotState roster;for(unsigned i=0;i<count;++i){auto p=player(i?100+i:0,{190,189,178,187,188,208,112,235});p.hp=p.mp=p.cp=99;if(i%2==0)p.curses={Curse::fog,Curse::darkcloud};p.guardian=static_cast<Guardian>(1+(seed+i)%10);roster.players.push_back(p);}MatchConfig cap;cap.max_decisions=64;Match a(cap),b(cap);a.load_scene(roster,roster.players.back().id,seed);b.load_scene(roster,roster.players.back().id,seed);const PhenomenonAction action{static_cast<CardId>(291+seed%3),0,{}};check(a.phenomenon_step(action)==b.phenomenon_step(action)&&a.snapshot()==b.snapshot(),"all targeted model launches reproduce full private state");roundtrip(a);std::uint64_t pa=seed,pb=seed;while(!a.done()){const auto before=a.snapshot();const auto ca=a.sample_action(pa),cb=b.sample_action(pb);check(ca==cb&&pa==pb&&a.legal(ca)&&a.snapshot()==before,"player policy remains pure across environmental and genuine derived sources");check(a.step(ca)==b.step(cb)&&a.snapshot()==b.snapshot(),"all later source-free/normal/counter/death/auto outcomes replay identically");roundtrip(a);}}
}

ItemShuffleAction shuffle_order(const SnapshotState& state){ItemShuffleAction a;for(const auto& p:state.players)if(!p.dead)for(const auto& item:p.items)a.order.push_back({p.id,item.token});return a;}
void item_shuffle_source_pairs_fresh_tokens_and_preserved_copies(){
    check(match_item_shuffle_supported(295)&&!match_item_shuffle_supported(294)&&!match_phenomenon_supported(295)&&match_catalog().size()==236&&match_gift_weight()==499,"295 has a dedicated typed capability without widening ordinary or generic phenotype inputs");
    auto state=scene({6,235,208},{7,239});state.players[0].items[1].used=true;state.players[0].guardian=Guardian::earth;state.players[1].guardian=Guardian::moon;state.players[0].curses={Curse::cold};state.players[1].curses={Curse::hell};state.gf=19;Match m;m.load_scene(state,0,17);const auto before=m.snapshot();ItemShuffleAction action{{{7,0},{0,1},{7,1},{0,2},{0,0}}};check(m.item_shuffle_legal(action)&&m.snapshot()==before,"source-qualified manifest legality is pure despite duplicate token0 across owners");m.item_shuffle_step(action);
    check(m.snapshot().state.players[0].items==std::vector<ItemInstance>{{3,7,{},false},{4,235,{},true},{5,239,{},false}}&&m.snapshot().state.players[1].items==std::vector<ItemInstance>{{6,208,{},false},{7,6,{},false}},"raw destination counts are fixed, exact copies move once, and fresh globally unique tokens resolve source-local collisions");
    auto expected=state;expected.update_count=1;expected.players[0].items=m.snapshot().state.players[0].items;expected.players[1].items=m.snapshot().state.players[1].items;check(m.snapshot().state==expected&&m.snapshot().next_token==8&&m.snapshot().rng==17&&m.snapshot().decisions==1&&m.snapshot().phenomenon_actions==1&&m.snapshot().owner==0&&m.snapshot().actor==0&&m.snapshot().phase==MatchPhase::main,"shuffle changes only inventories, next token and the single accepted decision/update boundary");
    check(m.snapshot().last_item_shuffle&&m.snapshot().last_item_shuffle->anchor.initial_state==state&&m.snapshot().last_item_shuffle->action==action&&m.snapshot().last_phenomenon==PhenomenonAction{295,{},{}},"private nonrecursive proof owns the complete old world and manifest");action.order.clear();roundtrip(m);
    const auto own=m.observe(0),other=m.observe(7),spectator=m.observe({});check(own.find(item_shuffle_rule)!=std::string::npos&&own.find("\"last_phenomenon\":{\"model\":295,\"recipient\":null}")!=std::string::npos&&own.find("\"last_item_shuffle\"")==std::string::npos&&spectator.find("\"hand\":[]")!=std::string::npos&&other.find("\"used_models\":[235]")!=std::string::npos,"only the phenomenon announcement and existing own-hand/public-used projections expose the result, never the source manifest");
    check(m.snapshot().dream_rolls==0&&m.snapshot().redraws==0&&m.snapshot().last_reveals.empty()&&m.snapshot().guardian_summons==0&&m.snapshot().guardian_opportunities==0&&m.snapshot().revivals==0&&m.snapshot().death_bows==0&&!m.snapshot().last_tick,"moving retained and passive copies never activates abilities, generates gifts, redraws or a turn tick");reject(m,play(0,{0},7));
}
void item_shuffle_dream_boundary_and_retained_recast(){
    for(bool destination_dream:{false,true}){SnapshotState state;state.players={player(0,{6,235}),player(7,{8}),player(9,{191})};state.players[0].curses={Curse::dream};state.players[1].curses={Curse::dream};if(destination_dream)state.players[2].curses={Curse::dream};state.players[0].items[0].fake_model=7;state.players[0].items[1].used=true;state.players[1].mp=7;Match m;m.load_scene(state,7,21);m.item_shuffle_step({{{7,0},{9,0},{0,1},{0,0}}});
        const auto& p=m.snapshot().state.players;check(p[0].items==std::vector<ItemInstance>{{2,8,{},false},{3,191,{},false}}&&p[1].items==std::vector<ItemInstance>{{4,235,{},true}}&&p[2].items==std::vector<ItemInstance>{{5,6,destination_dream?std::optional<CardId>{7}:std::nullopt,false}},"existing masks survive only a Dream recipient, unmasked incoming copies stay unmasked, and used miracles remain used");check(m.snapshot().rng==21&&m.snapshot().dream_rolls==0&&m.snapshot().last_reveals.empty()&&m.observe({}).find("\"last_reveals\":[]")!=std::string::npos,"private mask clearing does not draw new disguises or publicly reveal another owner's copy");roundtrip(m);
        m.step(play(7,{4},7));check(m.snapshot().state.players[1].hp==50&&m.snapshot().state.players[1].mp==0&&m.snapshot().state.players[1].items[0].token==4&&m.snapshot().state.players[1].items[0].used&&m.snapshot().phenomenon_actions==1&&!m.snapshot().last_item_shuffle,"the new owner can recast a transferred used miracle, paying normal MP after the shuffle event clears");roundtrip(m);}
}
void item_shuffle_capacity_dead_zero_and_token_budget(){
    SnapshotState state;for(unsigned i=0;i<9;++i){auto p=player(i?100+i:0);for(unsigned slot=0;slot<18;++slot)p.items.push_back({slot,static_cast<CardId>(slot%2?235:6),{},slot%2!=0});state.players.push_back(p);}Match m;m.load_scene(state,0,0);auto action=shuffle_order(state);std::reverse(action.order.begin(),action.order.end());m.item_shuffle_step(action);check(m.snapshot().next_token==180&&m.snapshot().last_item_shuffle->action.order.size()==162,"all162 live display slots are permuted and freshly numbered without changing capacity");for(unsigned i=0;i<9;++i)for(unsigned slot=0;slot<18;++slot)check(m.snapshot().state.players[i].items[slot].token==18+i*18+slot,"fresh IDs follow recipient input/raw slot order");roundtrip(m);
    state.players.back().dead=true;state.players.back().hp=0;m.load_scene(state,0,0);action=shuffle_order(state);std::reverse(action.order.begin(),action.order.end());m.item_shuffle_step(action);check(m.snapshot().state.players.back()==state.players.back()&&m.snapshot().next_token==162,"dead inventories stay byte-for-byte unchanged and contribute no transfer entries or new IDs");roundtrip(m);
    SnapshotState empty;empty.players={player(0),player(7),player(9,{191})};empty.players[2].dead=true;empty.players[2].hp=0;empty.players[2].items[0].token=max_wire_integer;m.load_scene(empty,0,4);check(m.item_shuffle_legal({}),"empty live-slot manifest is valid even if a dead copy exhausted the token space");m.item_shuffle_step({});check(m.snapshot().next_token==max_wire_integer+1&&m.snapshot().state.players[2]==empty.players[2]&&m.snapshot().rng==4,"zero-copy operation allocates no IDs or RNG and preserves dead max-token inventory");roundtrip(m);
    auto exhausted=scene({191},{});exhausted.players[0].items[0].token=max_wire_integer;m.load_scene(exhausted,0,4);const auto before=m.snapshot();action=shuffle_order(exhausted);check(m.item_shuffle_legal(action),"fresh-token availability is a transactional execution constraint rather than an invalid permutation");bool failed=false;try{m.item_shuffle_step(action);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==before,"nonempty shuffle at token exhaustion rolls back the whole state");
    exhausted.players[0].items[0].token=max_wire_integer-1;m.load_scene(exhausted,0,4);m.item_shuffle_step(shuffle_order(exhausted));check(m.snapshot().state.players[0].items[0].token==max_wire_integer&&m.snapshot().next_token==max_wire_integer+1,"the final representable token can be allocated exactly once");roundtrip(m);
}
void item_shuffle_invalid_cover_pending_and_rollback(){
    SnapshotState state;state.players={player(0,{6}),player(7,{7}),player(9,{191})};state.players[2].dead=true;state.players[2].hp=0;Match m;m.load_scene(state,0,0);
    const auto reject_shuffle=[&](ItemShuffleAction action){const auto before=m.snapshot();check(!m.item_shuffle_legal(action)&&m.snapshot()==before,"invalid private manifest query is pure");bool failed=false;try{m.item_shuffle_step(action);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==before,"invalid source cover never changes previous inventory, proof, event or counters");};
    for(auto action:std::vector<ItemShuffleAction>{{},{{{0,0}}},{{{0,0},{0,0}}},{{{9,0},{7,0}}},{{{0,0},{7,999}}},{{{999,0},{7,0}}},{{{max_wire_integer+1,0},{7,0}}},{{{0,max_wire_integer+1},{7,0}}},{{{0,0},{7,0},{9,0}}}})reject_shuffle(action);reject_shuffle({std::vector<ItemSource>(163,{0,0})});
    m=fixture(scene({6},{}));m.step(play(0,{0},7));reject_shuffle({});m=fixture(scene({5},{6}));m.step({0,CommandKind::buy,{0},7});m.step(defend(7));check(m.snapshot().phase==MatchPhase::purchase,"purchase escrow is pending before private shuffle rejection");reject_shuffle({});
    auto broken=state;broken.update_count=max_wire_integer;m.load_scene(broken,0,17);const auto before=m.snapshot();const auto action=shuffle_order(broken);check(m.item_shuffle_legal(action),"late update limit is not a malformed manifest");bool failed=false;try{m.item_shuffle_step(action);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==before,"late update exhaustion rolls back all moved models, masks, fresh tokens, proof and counters");
    MatchConfig cap;cap.max_decisions=1;Match cut(cap);cut.load_scene(state,0,4);cut.item_shuffle_step({{{7,0},{0,0}}});check(cut.snapshot().truncated&&cut.snapshot().state.players[0].items[0].model==7&&cut.snapshot().state.players[0].items[0].token==1&&cut.snapshot().last_item_shuffle&&cut.snapshot().rng==4,"shuffle body and its proof commit before decision cutoff");roundtrip(cut);
}
void item_shuffle_full_proof_rejects_tampering(){
    auto state=scene({6,235},{7});state.players[0].items[1].used=true;Match m;m.load_scene(state,0,21);m.item_shuffle_step({{{7,0},{0,1},{0,0}}});const auto exact=m.snapshot();
    for(unsigned error=0;error<18;++error){auto bad=exact;if(error==0)bad.last_item_shuffle->action.order[0]=bad.last_item_shuffle->action.order[1];if(error==1)++bad.last_item_shuffle->anchor.next_token;if(error==2)bad.last_item_shuffle->anchor.initial_state.players[0].items[0].model=8;if(error==3)bad.state.players[0].items[0].token=bad.next_token;if(error==4)bad.state.players[0].items[0].model=8;if(error==5)bad.state.players[0].items[1].used=false;if(error==6)++bad.state.players[0].hp;if(error==7)++bad.rng;if(error==8)++bad.next_token;if(error==9)bad.phenomenon_actions=0;if(error==10)bad.last_phenomenon.reset();if(error==11)bad.last_item_shuffle.reset();if(error==12)bad.last_item_shuffle->anchor.models={295};if(error==13)bad.last_item_shuffle->anchor.responses.push_back(defend(0));if(error==14){auto& items=bad.last_item_shuffle->anchor.initial_state.players[0].items;while(items.size()<19)items.push_back({items.size(),6,{},false});}if(error==15)bad.last_item_shuffle->anchor.initial_state.players.push_back(player(999));if(error==16)bad.last_item_shuffle->anchor.tick_processed=true;if(error==17)bad.last_item_shuffle->anchor.phenomenon_actions=1;
        bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"nonrecursive old-world/manifest replay rejects altered cover, inventory, unrelated resources, RNG, counters, IDs and noncanonical anchor fields");}
}
void item_shuffle_prefix_and_all_later_actions_replay(){
    for(unsigned count:{2u,9u})for(std::uint64_t seed=0;seed<16;++seed){SnapshotState state;for(unsigned i=0;i<count;++i){auto p=player(i?100+i:0,{6,190,178,208,112,235});p.items[5].used=true;p.hp=p.mp=p.cp=99;if(i%2==0)p.curses={Curse::fog,Curse::dream};p.guardian=static_cast<Guardian>(1+(seed+i)%10);state.players.push_back(p);}MatchConfig cap;cap.max_decisions=96;Match a(cap),b(cap);a.load_scene(state,0,seed);b.load_scene(state,0,seed);
        for(unsigned pulse=0;pulse<3;++pulse){auto action=shuffle_order(a.snapshot().state);std::rotate(action.order.begin(),action.order.begin()+static_cast<std::ptrdiff_t>((seed+pulse)%action.order.size()),action.order.end());check(a.item_shuffle_step(action)==b.item_shuffle_step(action)&&a.snapshot()==b.snapshot(),"repeated completed shuffle proofs preserve identical whole-world values and prefix counters");roundtrip(a);}check(a.snapshot().phenomenon_actions==3&&a.snapshot().rng==seed,"explicit permutation prefixes add no random draws");std::uint64_t pa=seed,pb=seed;
        while(!a.done()){const auto before=a.snapshot();const auto ca=a.sample_action(pa),cb=b.sample_action(pb);check(ca==cb&&pa==pb&&a.legal(ca)&&a.snapshot()==before,"ordinary visible policy remains pure while a private manifest proof is present");check(a.step(ca)==b.step(cb)&&a.snapshot()==b.snapshot()&&a.snapshot().phenomenon_actions==3&&!a.snapshot().last_item_shuffle,"subsequent ordinary/counter/death/redraw/auto replay uses the shuffled world and clears only the old proof");roundtrip(a);}}
}

// This proposal adapter belongs only to the privileged test controller. It does
// not implement the server's automatic confusion selector.
Command confusion_test_proposal(const Match& game,std::uint64_t& policy) {
    auto s=game.snapshot(); const auto actor=*s.actor;
    for(auto& p:s.state.players)if(p.id==actor)p.confusion_count=0;
    s.last_phenomenon.reset(); s.last_item_shuffle.reset();
    Match controller(s.config); controller.restore(s);return controller.sample_action(policy);
}
void confusion_addition_scope_and_projection(){
    auto s=phenomenon_scene(9,true);
    for(std::size_t i=0;i<s.players.size();++i)s.players[i].confusion_count=s.players[i].dead?0:static_cast<std::int64_t>(i);
    Match m;m.load_scene(s,0,42);const auto before=m.snapshot();
    check(m.phenomenon_legal({289,{},{}})&&m.snapshot()==before,"confusion addition legality is pure");
    m.phenomenon_step({289,{},{}});auto expected=s;++expected.update_count;
    for(auto& p:expected.players)if(!p.dead)p.confusion_count+=3;
    check(m.snapshot().state==expected&&m.snapshot().rng==42&&m.snapshot().owner==0&&m.snapshot().phenomenon_actions==1
        &&!m.snapshot().batch&&!m.snapshot().last_tick,"289 adds three to alive counts only with no ordinary scheduling or random draw");
    check(m.observe(0).find(confusion_rule)!=std::string::npos&&m.observe({}).find("confusionCount")==std::string::npos
        &&m.observe(0).find("confusion_command")==std::string::npos,"public metadata declares policy scope while hiding raw durations and replay marker");roundtrip(m);
    m.phenomenon_step({289,{},{}});for(const auto& p:m.snapshot().state.players)
        check(p.confusion_count==find_player(s,p.id)->confusion_count+(p.dead?0:6),"reapplication adds rather than clamps to three");roundtrip(m);
    auto bad=m.snapshot();bad.state.players[0].confusion_count=2;bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}
    check(failed,"completed confusion event requires its positive increment postcondition");
}
void confusion_api_gate_overflow_and_transaction(){
    auto s=scene({6},{});s.players[0].confusion_count=3;Match m;m.load_scene(s,0,7);const auto before=m.snapshot();const auto cmd=play(0,{0},7);
    reject(m,cmd);check(m.confusion_legal(cmd)&&m.snapshot()==before,"only externally supplied confusion command may launch a confused owner");
    std::uint64_t policy=19;bool failed=false;try{(void)m.sample_action(policy);}catch(const MatchError& e){failed=e.code==MatchErrorCode::unsupported;}
    check(failed&&policy==19&&m.snapshot()==before,"normal sampler leaves state and policy RNG untouched when no server confusion selector exists");
    for(auto invalid:std::vector<Command>{pray(0),play(7,{0},0),play(0,{999},7),defend(0),play(0,{0},999)}){
        check(!m.confusion_legal(invalid),"external resolution still enforces normal actor phase items target and prayer rules");failed=false;
        try{m.confusion_step(invalid);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==before,"invalid external command is atomic");}
    auto zero=s;zero.players[0].confusion_count=0;m.load_scene(zero,0,7);check(m.legal(cmd)&&!m.confusion_legal(cmd),"unconfused actor uses the normal API exclusively");
    auto limit=s;limit.players[0].confusion_count=max_wire_integer-3;m.load_scene(limit,0,7);m.phenomenon_step({289,{},{}});check(m.snapshot().state.players[0].confusion_count==max_wire_integer,"last safe global increment is exact");roundtrip(m);
    const auto full=m.snapshot();check(!m.phenomenon_legal({289,{},{}}),"289 preflights every live count before modifying any player");failed=false;
    try{m.phenomenon_step({289,{},{}});}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==full,"count exhaustion preserves notice counters and all state");
    auto late=s;late.update_count=max_wire_integer;m.load_scene(late,0,7);const auto exhausted=m.snapshot();failed=false;
    try{m.confusion_step(cmd);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exhausted,"late update exhaustion rolls back debit consumption launch and RNG");
    for(unsigned error=0;error<2;++error){auto invalid=s;if(error==0)invalid.players[0].confusion_count=-1;else{invalid.players[1].dead=true;invalid.players[1].hp=0;invalid.players[1].confusion_count=1;}
        failed=false;try{m.load_scene(invalid,0,7);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exhausted,"negative or dead positive confusion count cannot enter the closed runtime");}
}
void confusion_all_main_kinds_debit_and_cures_preserve(){
    for(const auto kind:{CommandKind::pray,CommandKind::discard,CommandKind::exchange,CommandKind::play}){
        auto s=scene(kind==CommandKind::exchange?std::initializer_list<CardId>{3}:kind==CommandKind::play?std::initializer_list<CardId>{200}:std::initializer_list<CardId>{199},{});
        s.players[0].confusion_count=3;s.players[0].curses={Curse::fog,Curse::dream};Match m;m.load_scene(s,0,7);
        Command cmd=kind==CommandKind::pray?pray(0):kind==CommandKind::discard?Command{0,kind,{0},{}}:kind==CommandKind::exchange?Command{0,kind,{0},{},ResourceAllocation{40,10,20}}:play(0,{0},0);
        check(m.confusion_legal(cmd),"supplied auxiliary/prayer command uses explicit local useAttackItems mapping");m.confusion_step(cmd);
        check(m.snapshot().state.players[0].confusion_count==2&&m.snapshot().decisions==1&&m.snapshot().owner==7,"all supplied main kinds debit once and then perform ordinary completion");
        if(kind==CommandKind::play)check(m.snapshot().state.players[0].curses.empty(),"all-cure removes curses without clearing remaining confusion");roundtrip(m);
    }
    auto s=scene({},{});s.players[0].confusion_count=3;s.players[0].guardian=Guardian::neptune;s.players[0].curses={Curse::fog};Match m;m.load_scene(s,0,9);
    m.guardian_step({0,275,0});check(m.snapshot().state.players[0].confusion_count==3&&m.snapshot().state.players[0].curses.empty(),"external cure does not consume a confused main turn");
    m.item_shuffle_step({});m.phenomenon_step({290,{},{}});check(m.snapshot().state.players[0].confusion_count==3,"unrelated external effects preserve confusion count");roundtrip(m);
}
void confusion_launch_response_replay_and_tamper(){
    auto s=scene({6,190},{190});s.players[0].confusion_count=s.players[1].confusion_count=3;Match m;m.load_scene(s,0,7);
    m.confusion_step(play(0,{0},7));check(m.snapshot().phase==MatchPhase::response&&m.snapshot().batch->confusion_command
        &&m.snapshot().batch->initial_state.players[0].confusion_count==3&&m.snapshot().state.players[0].confusion_count==2,"pending launch anchor precedes the sole count debit");roundtrip(m);
    const auto exact=m.snapshot();for(unsigned error=0;error<6;++error){auto bad=exact;if(error==0)bad.batch->confusion_command=false;if(error==1)bad.state.players[0].confusion_count=3;if(error==2)bad.batch->initial_state.players[0].confusion_count=2;if(error==3)bad.batch->initial_state.players[0].confusion_count=0;if(error==4)bad.batch->phenomenon_action=PhenomenonAction{291,7,{}};if(error==5)bad.batch->attack.actor=7;
        bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"confusion replay rejects omitted marker forged count anchor and external-domain mixing");}
    check(!m.confusion_legal(defend(7,{0}))&&m.legal(defend(7,{0})),"confused responder remains on ordinary response API");m.step(defend(7,{0}));
    check(m.snapshot().actor==0&&m.snapshot().state.players[1].confusion_count==3,"reflector's count is not spent on a defense");roundtrip(m);
    m.step(defend(0,{1}));check(m.snapshot().state.players[0].confusion_count==2&&m.snapshot().state.players[1].confusion_count==3,"repeated owner response never repeats launch debit");roundtrip(m);
    while(m.snapshot().phase==MatchPhase::response){m.step(defend(*m.snapshot().actor));roundtrip(m);}
}
void confusion_purchase_and_sale_resolution(){
    for(bool buy:{false,true}){
        auto s=buy?scene({5},{6}):scene({4,6},{});for(auto& p:s.players){p.confusion_count=3;p.cp=99;}
        Match m;m.load_scene(s,0,9);Command cmd=buy?Command{0,CommandKind::buy,{0},7}:Command{0,CommandKind::sell,{0,1},7};m.confusion_step(cmd);
        check(m.snapshot().state.players[0].confusion_count==2,"trade launch debits exactly one confused turn");roundtrip(m);m.step(defend(7));roundtrip(m);
        if(buy){check(m.snapshot().phase==MatchPhase::purchase&&m.snapshot().actor==0,"external confusion buy retains ordinary buyer decision escrow");m.step({0,CommandKind::accept,{},{}});roundtrip(m);}
        check(m.snapshot().phase==MatchPhase::main&&m.snapshot().state.players[0].confusion_count==2&&m.snapshot().state.players[1].confusion_count==3,"trade defense delivery and purchase decision do not debit either participant again");
    }
}
void confusion_death_revival_end_and_cutoff(){
    for(bool revive:{false,true}){
        auto s=revive?scene({208},{}):scene({},{});s.players.push_back(player(9));for(auto& p:s.players)p.confusion_count=3;Match m;m.load_scene(s,7,9);
        m.phenomenon_step({291,0,{}});m.step(defend(0));
        check(m.snapshot().state.players[0].confusion_count==(revive?3:0)&&m.snapshot().state.players[0].dead!=revive
            &&m.snapshot().state.players[1].confusion_count==3,"only actual death clears confusion while revival and other players preserve it");roundtrip(m);
    }
    auto s=scene({},{});for(auto& p:s.players)p.confusion_count=3;Match m;m.load_scene(s,7,9);m.phenomenon_step({291,0,{}});m.step(defend(0));
    check(m.done()&&m.snapshot().terminated&&m.snapshot().state.players[0].confusion_count==0&&m.snapshot().state.players[1].confusion_count==0,"natural end clears every survivor's confusion");roundtrip(m);
    auto bad=m.snapshot();bad.state.players[1].confusion_count=1;bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed,"natural terminal snapshot requires end-event count clear");
    MatchConfig cap;cap.max_decisions=1;Match cut(cap);cut.load_scene(s,0,9);cut.confusion_step(pray(0));check(cut.snapshot().truncated&&cut.snapshot().state.players[0].confusion_count==2&&cut.snapshot().state.players[1].confusion_count==3,"decision cutoff preserves remaining counts after the launch debit");roundtrip(cut);
}
void confusion_seeded_external_controller_rollouts(){
    for(unsigned players:{2u,9u})for(std::uint64_t seed=0;seed<20;++seed){MatchConfig c;c.max_decisions=128;Match a(c),b(c);std::vector<PlayerSetup> roster;
        for(unsigned i=0;i<players;++i)roster.push_back({i?100+7*i:0,Team::solo,"P"});a.reset(roster,seed);a.phenomenon_step({289,{},{}});b.restore(a.snapshot());std::uint64_t pa=seed,pb=seed;
        while(!a.done()){
            const auto before=a.snapshot();const auto* owner=find_player(before.state,before.owner);const bool confused=before.phase==MatchPhase::main&&owner->confusion_count>0;
            const auto ca=confused?confusion_test_proposal(a,pa):a.sample_action(pa),cb=confused?confusion_test_proposal(b,pb):b.sample_action(pb);
            check(ca==cb&&pa==pb&&a.snapshot()==before,"privileged test controller proposals are reproducible and state-pure");
            check((confused?a.confusion_step(ca):a.step(ca))==(confused?b.confusion_step(cb):b.step(cb))&&a.snapshot()==b.snapshot(),"mixed normal and external-confusion commands reproduce full queues counters guardian and RNG");roundtrip(a);
        }
    }
}

void fate_special_scene_capability_and_sampler_boundary(){
    check(match_fate_supported(207)&&!match_fate_supported(0)&&!match_fate_supported(201)&&!match_fate_supported(289)
        &&!match_supported(207)&&match_catalog().size()==236&&match_gift_weight()==499
        &&!match_guardian_payload_supported(Guardian::earth,207),"207 special ownership capability is distinct from unchanged automatic pool and Earth payload");
    auto m=fixture(scene({207,6},{}));roundtrip(m);reject(m,play(0,{0},0));reject(m,play(0,{0},7));
    std::uint64_t policy=0;for(unsigned i=0;i<400;++i){const auto before=m.snapshot();const auto cmd=m.sample_action(policy);
        check(m.legal(cmd)&&!(cmd.kind==CommandKind::play&&std::find(cmd.items.begin(),cmd.items.end(),0)!=cmd.items.end())&&m.snapshot()==before,"ordinary sampler treats special207 as an inert commodity and never invents a selected phenomenon");}
    check(m.observe(0).find(fate_rule)!=std::string::npos&&m.observe({}).find("\"last_fate\":null")!=std::string::npos,"view declares special fate contract without a fabricated event");
}
void fate_global_bodies_cutoff_and_ordinary_schedule(){
    for(CardId model:{287,288,289,290,294,296}){
        auto s=scene({207,235},{7});s.players[0].items[1].used=true;s.players[0].cp=70;s.players[1].cp=80;
        MatchConfig cap;cap.max_decisions=1;Match m(cap);m.load_scene(s,0,7);PhenomenonAction outcome{model,{},{}};
        if(model==294)outcome.recipient=7;if(model==296)outcome.guardians={Guardian::mars,Guardian::moon};
        FateAction action{0,0,outcome,{}};const auto before=m.snapshot();check(m.fate_legal(action)&&m.snapshot()==before,"fate global legality is pure");m.fate_step(action);
        check(m.snapshot().truncated&&m.snapshot().decisions==1&&m.snapshot().fate_actions==1&&m.snapshot().phenomenon_actions==0
            &&m.snapshot().state.players[0].items.size()==1&&m.snapshot().state.players[0].items[0].used&&m.snapshot().rng==7&&m.snapshot().state.gf==0,"207 consumption and immediate global body precede cutoff without a second external decision or gift draw");
        if(model==287)check(m.snapshot().state.players[0].curses==std::vector<Curse>{Curse::fever},"fate Fever body runs before ordinary tick cancellation");
        if(model==288)check(m.snapshot().state.players[1].curses==std::vector<Curse>{Curse::fog},"fate globalFog includes other player");
        if(model==289)check(m.snapshot().state.players[0].confusion_count==3&&m.snapshot().state.players[1].confusion_count==3,"fate confusion applies to caller and peers");
        if(model==290)check(m.snapshot().state.players[0].hp==1&&m.snapshot().state.players[1].hp==1,"fate HP setter applies globally");
        if(model==294)check(m.snapshot().state.players[0].cp==0&&m.snapshot().state.players[1].cp==99,"fate collection debits every alive before capped credit");
        if(model==296)check(m.snapshot().last_guardian_changes.size()==2&&m.snapshot().state.players[1].guardian==Guardian::moon,"fate guardian array covers the live roster once");roundtrip(m);
    }
    auto m=fixture(scene({207,235},{7}));m.fate_step({0,0,{290,{},{}},{}});
    check(m.snapshot().owner==7&&m.snapshot().state.gf==1&&m.snapshot().state.players[0].items.size()==2
        &&m.snapshot().next_token==3&&m.snapshot().state.players[0].mp==10&&m.snapshot().last_fate&&m.snapshot().last_fate_event==FateEvent{0,290},"completed fate gets one ordinary gift and next-owner GF progression");roundtrip(m);
    auto sick=scene({207},{});sick.players[0].curses={Curse::fever};auto tick=fixture(sick);tick.fate_step({0,0,{288,{},{}},{}});
    check(tick.snapshot().last_tick&&tick.snapshot().owner==7&&tick.snapshot().state.gf==1,"fate wrapper performs the existing ordinary owner tick instead of external injection scheduling");roundtrip(tick);
}
void fate_targeted_source_free_response_and_replay(){
    for(CardId model:{291,292,293}){
        auto s=scene({207},{190});s.players[0].confusion_count=1;s.players[1].curses={Curse::darkcloud};auto m=fixture(s);
        m.fate_step({0,0,{model,7,{}},{}});check(m.snapshot().phase==MatchPhase::response&&m.snapshot().queue[0].environmental
            &&m.snapshot().queue[0].origin==0&&m.snapshot().queue[0].source==0&&m.snapshot().batch->fate_action
            &&m.snapshot().batch->confusion_command&&m.snapshot().last_fate->anchor.initial_state.players[0].items[0].model==207
            &&m.snapshot().state.players[0].confusion_count==0&&m.snapshot().gifts[0]==1,"targeted fate retains pre-consumption proof and one confusion debit, with an absent effect source distinct from caller0");roundtrip(m);
        m.step(defend(7,{0}));check(m.snapshot().owner==7&&m.snapshot().state.gf==1&&m.snapshot().last_fate->responses.size()==1
            &&m.snapshot().state.players[0].items.size()==1&&m.snapshot().state.players[1].items.size()==1&&m.snapshot().fate_actions==1,"190 cancels world frame but both reserved gifts and ordinary turn completion survive");roundtrip(m);
        check(m.observe({}).find("\"last_fate\":{\"actor\":0,\"model\":207,\"phenomenon\":"+std::to_string(model)+"}")!=std::string::npos
            &&m.observe({}).find("last_fate_event")==std::string::npos&&m.observe({}).find("\"anchor\"")==std::string::npos,"public fate notice omits token, journal, assignment and source manifest");
    }
}
void fate_shuffle_post_consumption_copy_and_gift_order(){
    auto s=scene({207,235,6},{7,208});s.players[0].items[1].used=true;s.players[0].curses={Curse::dream};s.players[0].items[2].fake_model=7;
    s.players.push_back(player(9,{207}));s.players.back().dead=true;s.players.back().hp=0;const auto dead=s.players.back();auto m=fixture(s);
    FateAction action{0,0,{295,{},{}},ItemShuffleAction{{{7,0},{7,1},{0,1},{0,2}}}};m.fate_step(action);const auto& p=m.snapshot().state.players;
    check(p[0].items.size()==3&&p[0].items[0].model==7&&p[0].items[0].token==3&&p[0].items[1].model==208&&p[0].items[1].token==4
        &&p[1].items[0].model==235&&p[1].items[0].used&&p[1].items[0].token==5&&p[1].items[1].model==6&&!p[1].items[1].fake_model
        &&p[1].items[1].token==6&&p[2]==dead&&m.snapshot().next_token==8&&!m.snapshot().last_item_shuffle,"fate295 permutes only remaining physical copies then grants operation gift, preserving used/dead and privately clearing incoming nonDream masks");roundtrip(m);
    check(m.snapshot().last_fate->action==action&&m.snapshot().last_fate->anchor.initial_state.players[0].items.size()==3
        &&m.snapshot().owner==7&&m.snapshot().state.gf==1,"original207 anchor remains the proof across shuffle and ordinary completion");
    auto bad=action;bad.shuffle->order.push_back({0,0});auto again=fixture(s);const auto before=again.snapshot();check(!again.fate_legal(bad),"manifest may not include consumed207 or reserved future gift");bool failed=false;try{again.fate_step(bad);}catch(const MatchError&){failed=true;}check(failed&&again.snapshot()==before,"bad post-consumption manifest preserves pre207 ownership");
}
void fate_confusion_revival_death_and_natural_end(){
    auto s=scene({207},{});s.players[0].confusion_count=max_wire_integer-2;auto m=fixture(s);check(m.fate_legal({0,0,{289,{},{}},{}}),"debit happens before checked+3 so MAX-2 caller is admissible");m.fate_step({0,0,{289,{},{}},{}});
    check(m.snapshot().state.players[0].confusion_count==max_wire_integer&&m.snapshot().state.players[1].confusion_count==3,"confused207 launch debits once then applies global289");roundtrip(m);
    for(bool revive:{false,true}){
        auto state=revive?scene({207},{208}):scene({207},{});state.players.push_back(player(9));state.players[1].confusion_count=3;auto game=fixture(state);game.fate_step({0,0,{291,7,{}},{}});game.step(defend(7));
        check(game.snapshot().state.players[1].confusion_count==(revive?3:0)&&game.snapshot().state.players[1].dead!=revive
            &&game.snapshot().owner==(revive?7:9)&&game.snapshot().state.gf==1,"targeted fate uses common revival/death then ordinary next-alive owner schedule");roundtrip(game);
    }
    auto duel=fixture(scene({207},{}));duel.fate_step({0,0,{291,7,{}},{}});duel.step(defend(7));check(duel.snapshot().terminated&&duel.snapshot().last_fate
        &&duel.snapshot().state.players[0].items.empty()&&duel.snapshot().fate_actions==1,"natural end cancels reserved operation gift and retains complete fate proof");roundtrip(duel);
}
void fate_invalid_shapes_and_late_failure_atomicity(){
    auto s=scene({207},{});auto m=fixture(s);const auto before=m.snapshot();
    for(auto a:std::vector<FateAction>{{7,0,{290,{},{}},{}},{0,99,{290,{},{}},{}},{0,0,{207,{},{}},{}},{0,0,{295,{},{}},{}},{0,0,{287,{},{}},ItemShuffleAction{}},{0,0,{291,999,{}},{}},{0,0,{296,{},{}},{}},{0,0,{295,7,{}},ItemShuffleAction{}},{0,0,{295,{},{}},ItemShuffleAction{{{0,0}}}}}){
        check(!m.fate_legal(a),"wrong actor/token/model/outcome inactive fields and manifest fail before use");bool failed=false;try{m.fate_step(a);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==before,"invalid fate action preserves the whole pre-use snapshot");}
    s.gf=max_wire_integer;m.load_scene(s,0,7);const auto gf=m.snapshot();bool failed=false;try{m.fate_step({0,0,{290,{},{}},{}});}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==gf,"late ordinary GF failure rolls back207 body gift RNG proof and cumulative counters");
    s.gf=0;s.players[0].items[0].token=max_wire_integer;m.load_scene(s,0,7);const auto tokens=m.snapshot();FateAction empty{0,max_wire_integer,{295,{},{}},ItemShuffleAction{}};
    check(m.fate_legal(empty),"empty post-consumption shuffle needs no fresh token");failed=false;try{m.fate_step(empty);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==tokens,"late operation gift exhaustion rolls back even a valid empty shuffle");
    MatchConfig cap;cap.max_decisions=1;Match cut(cap);cut.load_scene(s,0,7);cut.fate_step(empty);check(cut.snapshot().truncated&&cut.snapshot().next_token==max_wire_integer+1&&cut.snapshot().state.players[0].items.empty(),"cutoff cancels gift after successful empty shuffle without requiring a token");roundtrip(cut);
}
void fate_full_launch_response_and_completed_proof_tamper(){
    auto m=fixture(scene({207},{190}));m.fate_step({0,0,{291,7,{}},{}});const auto exact=m.snapshot();
    for(unsigned error=0;error<17;++error){auto bad=exact;if(error==0)bad.last_fate.reset();if(error==1)bad.last_fate_event.reset();if(error==2)bad.last_fate->action.token=99;if(error==3)bad.last_fate->anchor.initial_state.players[0].items[0].model=6;if(error==4)bad.last_fate->anchor.models={207};if(error==5)bad.last_fate->anchor.fate_action=bad.last_fate->action;if(error==6)bad.last_fate->anchor.confusion_command=true;if(error==7)bad.batch->fate_action.reset();if(error==8)bad.batch->fate_action->outcome.model=293;if(error==9)bad.queue[0].environmental=false;if(error==10)bad.state.players[0].hp=39;if(error==11)++bad.rng;if(error==12)bad.fate_actions=0;if(error==13)bad.last_fate_event->phenomenon=293;if(error==14)bad.last_fate->responses.push_back(defend(0));if(error==15)bad.last_fate->anchor.responses.push_back(defend(7));if(error==16)bad.last_phenomenon.reset();
        bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"fate proof rejects missing notices altered identities anchor markers source flags counters and response journal");}
    m.step(defend(7,{0}));const auto done=m.snapshot();auto bad=done;bad.last_fate->responses.push_back(defend(7));bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==done,"completed fate proof rejects journal continuation after ordinary advance");roundtrip(m);
    m.step(pray(7));check(!m.snapshot().last_fate&&!m.snapshot().last_fate_event&&m.snapshot().fate_actions==1,"next accepted main clears prior launch proof while keeping cumulative fate activations");roundtrip(m);
}
void fate207_inert_discard_trade_fog_and_removal(){
    auto discard=fixture(scene({207},{}));discard.step({0,CommandKind::discard,{0},{}});check(discard.snapshot().fate_actions==0&&!discard.snapshot().last_phenomenon,"discarding207 invokes no phenomenon");roundtrip(discard);
    auto sell_state=scene({4,207},{});sell_state.players[0].curses={Curse::fog};sell_state.players[1].cp=99;auto sell=fixture(sell_state);sell.step({0,CommandKind::sell,{0,1},7});roundtrip(sell);sell.step(defend(7));check(sell.snapshot().last_sale&&sell.snapshot().last_sale->model==207&&sell.snapshot().state.players[1].items[0].model==207&&sell.snapshot().fate_actions==0,"Fog sale moves207 as an inert commodity");roundtrip(sell);
    auto buy_state=scene({5},{207});buy_state.players[0].cp=99;auto buy=fixture(buy_state);buy.step({0,CommandKind::buy,{0},7});roundtrip(buy);buy.step(defend(7));roundtrip(buy);buy.step({0,CommandKind::accept,{},{}});check(buy.snapshot().last_purchase&&buy.snapshot().last_purchase->model==207&&buy.snapshot().fate_actions==0,"purchase escrow delivery preserves special207 without activation");roundtrip(buy);
    auto remove=fixture(scene({205},{207}));remove.step(play(0,{0},7));remove.step(defend(7));check(remove.snapshot().last_removal&&remove.snapshot().last_removal->removed_models==std::vector<CardId>{207}&&remove.snapshot().fate_actions==0,"broom removal may report special207 without invoking it");roundtrip(remove);
}

void fate_guardian_followups_and_repeated_counter_anchors(){
    bool saw_auto_response=false,saw_player_zero_source=false;
    for(std::uint64_t seed=0;seed<32;++seed){
        auto s=scene({207,207},{207});s.players.push_back(player(9,{207}));for(auto& p:s.players){p.hp=p.mp=99;p.cp=40;}
        Match m;m.load_scene(s,0,seed);m.fate_step({0,0,{296,{},{Guardian::moon,Guardian::moon,Guardian::moon}}, {}});roundtrip(m);
        while(!m.done()&&m.snapshot().phase!=MatchPhase::main){
            if(!m.snapshot().last_auto_guardians.empty()){
                saw_auto_response=true;const auto& e=m.snapshot().queue.front();check(!e.environmental,"guardian follow-up uses its actual player source instead of the original fate's environmental source");
                saw_player_zero_source|=e.source==0;
            }
            std::uint64_t policy=seed;const auto command=m.sample_action(policy);m.step(command);roundtrip(m);
        }
        check(!m.done()&&m.snapshot().owner==7&&m.snapshot().fate_actions==1,"first nonlethal fate completes exactly one ordinary turn through all guardian responses");
        m.fate_step({7,0,{288,{},{}},{}});roundtrip(m);
        while(!m.done()&&m.snapshot().phase!=MatchPhase::main){
            if(!m.snapshot().last_auto_guardians.empty()){
                saw_auto_response=true;const auto& e=m.snapshot().queue.front();check(!e.environmental,"second fate guardian remains a sourced response");saw_player_zero_source|=e.source==0;
            }
            std::uint64_t policy=seed;const auto command=m.sample_action(policy);m.step(command);roundtrip(m);
        }
        check(m.snapshot().fate_actions==2&&m.snapshot().last_fate->anchor.fate_actions==1&&m.snapshot().phenomenon_actions==0,"a second fate proof preserves previous cumulative activations without counting an external injection");roundtrip(m);
    }
    check(saw_auto_response&&saw_player_zero_source,"seed sweep exercises guardian responses and real PlayerID0 sources after fate body completion");
}

void devil_capability_direct_damage_and_no_response(){
    for(CardId id=0;id<=300;++id)check(match_devil_supported(id)==(id>=240&&id<=244),"only five devil models are external capabilities");
    check(match_catalog().size()==236&&match_gift_weight()==499&&!match_supported(240)&&!match_guardian_payload_supported(Guardian::earth,240),"devils do not enter ordinary gifts, inventories or guardian payloads");
    for(CardId model:{240,241,242}){
        auto s=scene({190,184,187,208},{});s.gf=17;s.players[0].hp=99;s.players[0].mp=13;s.players[0].cp=17;s.players[0].confusion_count=2;s.players[0].curses={Curse::darkcloud};
        auto m=fixture(s);const auto before=m.snapshot();DevilAction a{0,model,{},{}};check(m.devil_legal(a)&&m.snapshot()==before,"devil legality is pure and resolves actual player0");m.devil_step(a);
        check(m.snapshot().state.players[0].hp==99-find_card(model)->ability_value&&m.snapshot().state.players[0].mp==13&&m.snapshot().state.players[0].cp==17
            &&m.snapshot().phase==MatchPhase::main&&m.snapshot().owner==0&&m.snapshot().state.gf==17&&m.snapshot().rng==1234
            &&m.snapshot().state.players[0].items==s.players[0].items&&m.snapshot().state.players[0].confusion_count==2
            &&m.snapshot().counter_created==0&&m.snapshot().chance_rolls==0&&m.snapshot().devil_actions==1&&!m.snapshot().last_tick,
            "fixed HP damage bypasses armor190/counter/DarkCloud and does not debit MP, inventory, confusion or ordinary turn");roundtrip(m);
        check(m.observe({}).find(devil_rule)!=std::string::npos&&m.observe({}).find("\"last_devil\":{\"model\":"+std::to_string(model)+",\"target\":0,\"removed_models\":[]}")!=std::string::npos,"public devil notice identifies the model and real target without source or manifest");
    }
}
void devil_fairy_three_resources_clip_and_private_choice(){
    for(auto resource:{Resource::hp,Resource::mp,Resource::cp})for(int amount:{0,1,95,99}){
        auto s=scene({},{});s.players[0].hp=amount?amount:40;s.players[0].mp=amount;s.players[0].cp=amount;s.players[0].confusion_count=3;s.players[1].curses={Curse::fog};
        auto expected=s.players[0];if(resource==Resource::hp)expected.hp=std::min<std::int64_t>(99,expected.hp+10);if(resource==Resource::mp)expected.mp=std::min<std::int64_t>(99,expected.mp+10);if(resource==Resource::cp)expected.cp=std::min<std::int64_t>(99,expected.cp+10);
        auto m=fixture(s);m.devil_step({0,244,resource,{}});check(m.snapshot().state.players[0]==expected&&m.snapshot().rng==1234&&m.snapshot().next_token==0&&m.snapshot().owner==0,"fairy credits exactly one supplied resource, clips99, and keeps all other state");roundtrip(m);
        const auto view=m.observe(7);check(view.find("\"resource\"")==std::string::npos&&view.find("\"target\":0,\"removed_models\":[]")!=std::string::npos,"Fog observer receives no privileged resource choice in the announcement");
    }
}
void devil_trickster_exact_copies_used_fake_and_inert_passives(){
    auto s=scene({207,208,112,235,6},{});s.players[0].items[3].used=true;s.players[0].items[4].fake_model=7;s.players[0].curses={Curse::dream};s.players[0].confusion_count=2;
    s.players.push_back(player(9,{207}));s.players.back().dead=true;s.players.back().hp=0;auto m=fixture(s);m.devil_step({0,243,{}, {3,4}});
    auto expected=s.players[0];expected.items.erase(expected.items.begin()+3,expected.items.end());check(m.snapshot().state.players[0]==expected&&m.snapshot().state.players[2]==s.players[2]
        &&m.snapshot().rng==1234&&m.snapshot().next_token==5&&m.snapshot().revivals==0&&m.snapshot().death_bows==0&&m.snapshot().gifts==std::vector<std::uint32_t>(3,0),"trickster removes the supplied used/fake copies and leaves all passives, resources, dead copies and reserved gifts inert");
    check(m.snapshot().last_devil_event->removed_models==std::vector<CardId>{235,6}&&m.snapshot().last_reveals==std::vector<RevealedItem>{{0,7,6,false}}
        &&m.observe({}).find("\"removed_models\":[235,6]")!=std::string::npos,"removal order and shared fake reveal are the explicit local display fanout");roundtrip(m);
    m.devil_step({0,243,{}, {1,2}});check(m.snapshot().state.players[0].items.size()==1&&m.snapshot().state.players[0].items[0].model==207&&m.snapshot().revivals==0&&m.snapshot().death_created==0,"removing208 and112 invokes neither ability nor replacement");roundtrip(m);
    m.devil_step({0,243,{}, {0}});check(m.snapshot().state.players[0].items.empty()&&m.snapshot().fate_actions==0&&m.snapshot().last_devil_event->removed_models==std::vector<CardId>{207},"one remaining207 is removed without calling a phenomenon");roundtrip(m);
    m.devil_step({0,243,{}, {}});check(m.snapshot().devil_actions==4&&m.snapshot().last_devil_event->removed_models.empty(),"empty hand has an exact empty manifest");roundtrip(m);
}
void devil_bad_shapes_pending_and_late_atomic_failure(){
    auto s=scene({6,235},{});s.players[0].items[1].used=true;auto m=fixture(s);const auto before=m.snapshot();
    for(auto a:std::vector<DevilAction>{{0,239,{},{}},{999,240,{},{}},{0,240,Resource::mp,{}},{0,241,{}, {0}},{0,244,{},{}},{0,244,static_cast<Resource>(255),{}},{0,244,Resource::hp,{0}},{0,243,Resource::cp,{0,1}},{0,243,{},{}},{0,243,{}, {0}},{0,243,{}, {0,0}},{0,243,{}, {0,999}},{0,243,{}, {0,1,2}},{0,243,{}, {0,max_wire_integer+1}}}){
        check(!m.devil_legal(a),"devil model, recipient, inactive field, resource, count and token identity must all be canonical");bool failed=false;try{m.devil_step(a);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==before,"malformed devil effect is atomic");}
    auto dead=s;dead.players.push_back(player(9));dead.players.back().dead=true;dead.players.back().hp=0;m.load_scene(dead,0,3);check(!m.devil_legal({9,244,Resource::hp,{}}),"fairy cannot resurrect a dead recipient");
    m=fixture(scene({6},{}));m.step(play(0,{0},7));check(!m.devil_legal({7,240,{},{}}),"external devil cannot interrupt response work");
    s.update_count=max_wire_integer;m.load_scene(s,0,3);const auto exhausted=m.snapshot();bool failed=false;try{m.devil_step({0,243,{}, {0,1}});}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exhausted,"late update limit restores removed copies, reveals, notice and proof");
    auto revival=scene({208},{});revival.players[0].hp=5;revival.players[0].items[0].token=max_wire_integer;m.load_scene(revival,0,3);const auto no_tokens=m.snapshot();failed=false;try{m.devil_step({0,240,{},{}});}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==no_tokens,"late revival gift exhaustion rolls back HP, amulet consumption and devil counters");
    MatchConfig cap;cap.max_decisions=1;Match cut(cap);cut.load_scene(revival,0,3);cut.devil_step({0,240,{},{}});check(cut.snapshot().truncated&&cut.snapshot().state.players[0].hp==10&&cut.snapshot().state.players[0].items.empty()&&cut.snapshot().next_token==max_wire_integer+1,"cutoff applies damage and revival before canceling gift without a fresh ID");roundtrip(cut);
}
void devil_revival_and_natural_end_boundaries(){
    auto s=scene({208,190},{});s.gf=17;s.players[0].hp=5;s.players[0].confusion_count=2;s.players[1].curses={Curse::fever};Match m;m.load_scene(s,7,3);m.devil_step({0,240,{},{}});
    check(m.snapshot().state.players[0].hp==10&&m.snapshot().state.players[0].confusion_count==2&&m.snapshot().state.players[0].items.size()==2&&m.snapshot().revivals==1
        &&m.snapshot().owner==7&&m.snapshot().actor==7&&m.snapshot().state.gf==17&&!m.snapshot().last_tick&&m.snapshot().guardian_opportunities==0&&m.snapshot().last_devil->responses.empty(),"208 restoration gets only its shared gift, with no ordinary owner tick, GF or new devil cascade");roundtrip(m);
    auto duel=fixture(scene({112},{}));auto state=duel.snapshot().state;state.players[0].hp=5;state.players[1].confusion_count=2;duel.load_scene(state,0,3);duel.devil_step({0,240,{},{}});
    check(duel.snapshot().terminated&&duel.snapshot().winners==std::vector<PlayerId>{7}&&duel.snapshot().death_created==0&&duel.snapshot().state.players[0].items.size()==1&&duel.snapshot().state.players[1].confusion_count==0,"natural duel result precedes death-bow reservation and clears survivor confusion");roundtrip(duel);
}
void devil_posthumous_fifo_real_source_zero_and_owner_resume(){
    for(PlayerId owner:{PlayerId{0},PlayerId{7}}){
        auto s=scene({112},{187,189});s.players.push_back(player(9));s.gf=17;s.players[0].hp=5;s.players[0].confusion_count=2;s.players[1].curses={Curse::darkcloud,Curse::fever};std::sort(s.players[1].curses.begin(),s.players[1].curses.end());s.players[2].curses={Curse::darkcloud};for(auto& p:s.players)if(p.id)p.hp=99;
        Match m;m.load_scene(s,owner,3);m.devil_step({0,240,{},{}});check(m.snapshot().phase==MatchPhase::response&&m.snapshot().queue[0].is_death&&!m.snapshot().queue[0].environmental
            &&m.snapshot().queue[0].source==0&&m.snapshot().queue[0].origin==0&&m.snapshot().batch->devil_action&&m.snapshot().batch->tick_processed,"direct devil has no response, but shared112 launches an actual dead player0 source");roundtrip(m);
        const auto pending=m.snapshot();for(unsigned error=0;error<4;++error){auto bad=pending;if(error==0){bad.last_devil.reset();bad.last_devil_event.reset();}if(error==1)bad.batch->devil_action.reset();if(error==2)bad.queue[0].environmental=true;if(error==3)bad.last_devil->action.target=7;bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==pending,"pending devil proof rejects stripped origin, forged source absence and wrong recipient");}

        m.step(defend(7,{1,0}));roundtrip(m);check(m.snapshot().actor==9&&m.snapshot().queue[0].is_death,"remaining death targets precede MP counter FIFO");m.step(defend(9));roundtrip(m);
        check(m.snapshot().queue[0].is_counter&&m.snapshot().actor==7,"surviving187 response creates a genuine self MP counter");m.step(defend(7));roundtrip(m);
        check(m.snapshot().phase==MatchPhase::main&&m.snapshot().owner==7&&m.snapshot().state.gf==17&&m.snapshot().counter_created==1&&m.snapshot().death_created==2
            &&m.snapshot().last_devil->responses.size()==3&&!m.snapshot().last_tick&&m.snapshot().guardian_opportunities==0&&m.snapshot().state.players[0].confusion_count==0,"external completion resumes the live owner or next alive without ordinary tick/GF/auto");
    }
}
void devil_guardian_departure_local_rule_and_fog_filter(){
    bool departed=false,stayed=false;
    for(std::uint64_t seed=0;seed<128;++seed){auto s=scene({},{});s.players[0].guardian=Guardian::mars;s.players[1].curses={Curse::fog};Match m;m.load_scene(s,7,seed);m.devil_step({0,240,{},{}});
        check(m.snapshot().guardian_departure_rolls==1&&m.snapshot().guardian_opportunities==0,"only positive HP damage uses the explicit local departure draw, not ordinary auto opportunity");
        if(m.snapshot().state.players[0].guardian==Guardian::none){departed=true;check(m.snapshot().last_guardian_changes.size()==1&&m.snapshot().last_guardian_changes[0].hidden_viewers==2
            &&m.observe(7).find("\"last_guardian_changes\":[]")!=std::string::npos,"departure event uses event-time Fog viewer filtering");}else stayed=true;roundtrip(m);
    }
    check(departed&&stayed,"seed sweep covers both branches of local ten-percent departure");
}
void devil_full_proof_tamper_and_following_anchor_prefixes(){
    auto s=scene({207,235,6},{});s.players[0].items[1].used=true;auto m=fixture(s);m.devil_step({0,243,{}, {1,2}});const auto exact=m.snapshot();
    for(unsigned error=0;error<15;++error){auto bad=exact;if(error==0)bad.last_devil.reset();if(error==1)bad.last_devil_event.reset();if(error==2)bad.last_devil->action.removed={0,2};if(error==3)bad.last_devil_event->removed_models={6,235};if(error==4)bad.last_devil->anchor.models={243};if(error==5)bad.last_devil->anchor.devil_action=bad.last_devil->action;if(error==6)bad.last_devil->anchor.confusion_command=true;if(error==7)bad.last_devil->responses.push_back(defend(0));if(error==8)bad.state.players[0].hp=39;if(error==9)++bad.rng;if(error==10)bad.devil_actions=0;if(error==11)bad.last_devil_event->target=7;if(error==12)bad.last_devil->anchor.devil_actions=1;if(error==13)bad.last_devil->action.resource=Resource::hp;if(error==14)bad.last_devil->action.model=244;
        bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"completed devil proof rejects altered identities models state journal counters and inactive fields");}
    m.fate_step({0,0,{288,{},{}},{}});check(m.snapshot().fate_actions==1&&m.snapshot().devil_actions==1&&!m.snapshot().last_devil,"laterFate clears notice while its anchor preserves completed devil prefix");roundtrip(m);
    m.devil_step({7,244,Resource::cp,{}});check(m.snapshot().last_devil->anchor.fate_actions==1&&m.snapshot().last_devil->anchor.devil_actions==1&&m.snapshot().devil_actions==2,"later devil proof carries both prior activation types");roundtrip(m);
    m.item_shuffle_step(shuffle_order(m.snapshot().state));check(m.snapshot().devil_actions==2&&!m.snapshot().last_devil,"standalone shuffle retains devil cumulative history without recursive proof");roundtrip(m);
}

void sacrifice_threshold_scene_and_transition_boundaries(){
    for(auto threshold:std::vector<std::optional<std::int64_t>>{{},0,1,17,18,static_cast<std::int64_t>(max_wire_integer)})for(std::int64_t gf:{0,16,17,18}){
        auto s=scene({199},{199});s.gf=gf;s.tiebreak_gf=threshold;auto m=fixture(s);const bool active=threshold&&gf>=*threshold;
        check(m.legal({0,active?CommandKind::sacrifice:CommandKind::discard,{0},{}})&&!m.legal({0,active?CommandKind::discard:CommandKind::sacrifice,{0},{}}),"only the launch-stage canonical operation is accepted");
        check(m.observe({}).find(std::string("\"apocalypse_active\":")+(active?"true":"false"))!=std::string::npos,"public apocalypse bool is derived from the same threshold as legality");roundtrip(m);
    }
    auto s=scene({199},{199});s.gf=17;s.tiebreak_gf=18;auto m=fixture(s);m.step({0,CommandKind::discard,{0},{}});
    check(m.snapshot().state.gf==18&&m.snapshot().owner==7&&m.snapshot().state.players[0].items.empty()&&m.snapshot().rng==1234
        &&m.legal({7,CommandKind::sacrifice,{0},{}})&&!m.legal({7,CommandKind::discard,{0},{}}),"prethreshold discard grants nothing and switches the next owner at the GF boundary");roundtrip(m);
    const auto exact=m.snapshot();auto bad=m.snapshot().state;bad.tiebreak_gf=-1;bool failed=false;try{m.load_scene(bad,7,0);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"negative Match threshold rejects atomically");
    m.reset(std::array<PlayerSetup,2>{{{0,Team::solo,"A"},{7,Team::solo,"B"}}},0);check(!m.snapshot().state.tiebreak_gf&&m.observe({}).find("\"apocalypse_active\":false")!=std::string::npos,"reset restores the unchanged no-threshold default");
}
void sacrifice_multi_used_fake_inert_and_zero_fee(){
    CardId armor=0,face=0;for(auto a:match_catalog())for(auto b:match_catalog())if(!armor&&find_card(a)->category==Category::armor&&match_dream_compatible(a,b)){armor=a;face=b;}
    check(armor&&face,"compatible masked armor fixture exists");auto s=scene({235,236,178,168,207},{});s.players[0].items.push_back({5,armor,face,false});s.players[0].items[0].used=true;s.players[0].mp=0;s.players[0].curses={Curse::dream,Curse::flash};s.tiebreak_gf=0;
    auto m=fixture(s);const Command command{0,CommandKind::sacrifice,{0,1,2,3,4,5},{}};check(m.legal(command),"Flash does not restrict main sacrifice and zeroMP does not charge selected miracles");m.step(command);
    check(m.snapshot().state.players[0].items.size()==6&&m.snapshot().next_token==12&&m.snapshot().state.players[0].mp==0
        &&m.snapshot().state.players[0].curses==s.players[0].curses&&m.snapshot().redraws==0&&m.snapshot().fate_actions==0&&m.snapshot().devil_actions==0
        &&m.snapshot().last_reveals==std::vector<RevealedItem>{{0,face,armor,false}}&&m.snapshot().owner==7&&m.snapshot().state.gf==1,"all selected physical copies are consumed, one normal gift per copy is deferred, and selected abilities do not fire");
    for(const auto& item:m.snapshot().state.players[0].items)check(item.token>=6&&!item.used&&match_supported(item.model),"new gifts are fresh unused ordinary copies, without207 or a new devil roll");roundtrip(m);
}
void sacrifice_full18_and_cutoff_cancels_all_gifts(){
    SnapshotState s;auto p=player(0);for(ItemToken i=0;i<18;++i)p.items.push_back({i,235,{},true});p.mp=0;s.players={p,player(7)};s.tiebreak_gf=0;
    Command command{0,CommandKind::sacrifice,{},{}};for(ItemToken i=0;i<18;++i)command.items.push_back(i);auto m=fixture(s);m.step(command);
    check(m.snapshot().state.players[0].items.size()==18&&m.snapshot().next_token==36&&m.snapshot().state.players[0].mp==0,"eighteen used miracles are fully removed and replaced without retention or MP charge");for(const auto& item:m.snapshot().state.players[0].items)check(item.token>=18&&!item.used,"every maximum-width replacement is fresh and unused");roundtrip(m);
    MatchConfig cap;cap.max_decisions=1;Match cut(cap);cut.load_scene(s,0,7);cut.step(command);check(cut.snapshot().truncated&&cut.snapshot().state.players[0].items.empty()&&cut.snapshot().next_token==18&&cut.snapshot().rng==7&&cut.snapshot().state.gf==0,"cutoff commits all sacrifices before canceling all deferred gifts and owner advance");roundtrip(cut);
}
void sacrifice_role_identity_and_phase_rejections(){
    auto s=scene({6,208,209,199,235},{});s.tiebreak_gf=0;auto m=fixture(s);
    for(auto command:std::vector<Command>{{0,CommandKind::discard,{3},{}},{0,CommandKind::sacrifice,{},{}},{0,CommandKind::sacrifice,{3,3},{}},{0,CommandKind::sacrifice,{999},{}},{0,CommandKind::sacrifice,{0},{}},{0,CommandKind::sacrifice,{1},{}},{0,CommandKind::sacrifice,{2},{}},{0,CommandKind::sacrifice,{3},7},{7,CommandKind::sacrifice,{3},{}},{0,CommandKind::sacrifice,{3},{},ResourceAllocation{40,10,20}}})reject(m,command);
    check(!match_supported(2)&&m.legal({0,CommandKind::sacrifice,{3,4},{}}),"the virtual operation2 is not an owned artifact and selected copies are the whole payload");
    m.step(play(0,{0},7));check(m.snapshot().phase==MatchPhase::response,"ordinary attack supplies a pending rejection scene");reject(m,{7,CommandKind::sacrifice,{},{}});roundtrip(m);
    auto pre=s;pre.tiebreak_gf.reset();auto inactive=fixture(pre);reject(inactive,{0,CommandKind::sacrifice,{3},{}});
}
void sacrifice_confusion_one_debit_and_sampler_parity(){
    auto s=scene({235,207,199,6},{});s.tiebreak_gf=0;s.players[0].items[0].used=true;s.players[0].mp=0;s.players[0].confusion_count=3;auto m=fixture(s);Command operation{0,CommandKind::sacrifice,{0,1,2},{}};
    reject(m,operation);check(m.confusion_legal(operation),"confused actor may receive an externally resolved sacrifice");m.confusion_step(operation);check(m.snapshot().state.players[0].confusion_count==2&&m.snapshot().state.players[0].items.size()==4&&m.snapshot().owner==7,"multi-copy sacrifice debits one confusion turn and grants three copies");roundtrip(m);
    s.players[0].confusion_count=0;auto active=fixture(s);s.tiebreak_gf.reset();auto inactive=fixture(s);bool saw=false;
    for(std::uint64_t seed=0;seed<512;++seed){auto a=seed,b=seed;const auto x=inactive.sample_action(a);auto y=active.sample_action(b);check(active.legal(y)&&inactive.legal(x),"both stage samplers return legal operations");
        if(x.kind==CommandKind::discard){check(y.kind==CommandKind::sacrifice,"only discard proposal kind changes at apocalypse");y.kind=CommandKind::discard;saw=true;}
        check(x==y&&a==b,"threshold operation replacement changes no proposal identity, weighting or policy RNG consumption");}
    check(saw,"parity seed sweep exercises actual sacrifice proposals");
}
void sacrifice_tick_death_revival_and_natural_completion(){
    for(bool revive:{false,true}){
        auto s=revive?scene({199,112,208},{}):scene({199,112},{});s.players.push_back(player(9));s.gf=17;s.tiebreak_gf=17;s.players[0].hp=1;s.players[0].curses={Curse::cold};s.players[1].hp=s.players[2].hp=99;s.players[1].curses=s.players[2].curses={Curse::darkcloud};auto m=fixture(s);m.step({0,CommandKind::sacrifice,{0},{}});roundtrip(m);
        if(!revive){check(m.snapshot().batch&&m.snapshot().batch->attack.kind==CommandKind::sacrifice&&m.snapshot().batch->tick_processed&&m.snapshot().queue[0].is_death,"sacrifice owner tick can suspend in the shared death112 FIFO with the original launch kind");while(!m.done()&&m.snapshot().phase!=MatchPhase::main){m.step(defend(*m.snapshot().actor));roundtrip(m);}}
        check(m.snapshot().owner==7&&m.snapshot().state.gf==18&&m.snapshot().last_tick&&m.snapshot().state.players[0].dead!=revive,"ordinary owner tick/death or revival completes before next owner GF");
        if(revive)check(m.snapshot().revivals==1&&m.snapshot().next_token==5&&m.snapshot().state.players[0].items.size()==3,"surviving208 adds its own deferred gift in addition to the sacrifice replacement");else check(m.snapshot().state.players[0].items.empty()&&m.snapshot().next_token==2,"dead sacrificer receives no deferred gift");roundtrip(m);
    }
    auto s=scene({199,112},{});s.players[0].hp=1;s.players[0].curses={Curse::cold};s.tiebreak_gf=0;auto m=fixture(s);m.step({0,CommandKind::sacrifice,{0},{}});check(m.snapshot().terminated&&m.snapshot().state.players[0].items.size()==1&&m.snapshot().death_created==0&&m.snapshot().next_token==2,"natural duel outcome precedes death-bow consumption and cancels sacrifice gift");roundtrip(m);
}
void sacrifice_exhaustion_rollback_and_launch_threshold_replay(){
    for(unsigned limit=0;limit<3;++limit){auto s=scene({199},{});s.tiebreak_gf=0;if(limit==0)s.update_count=max_wire_integer;if(limit==1)s.players[0].items[0].token=max_wire_integer;if(limit==2)s.gf=max_wire_integer;auto m=fixture(s);const auto before=m.snapshot();const auto token=s.players[0].items[0].token;check(m.legal({0,CommandKind::sacrifice,{token},{}}),"late counter/token/GF constraints do not make a well-formed selection noncanonical");bool failed=false;try{m.step({0,CommandKind::sacrifice,{token},{}});}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==before,"late failure rolls back consumed copies, all gift RNG and the stage snapshot");}
    bool pending=false;for(std::uint64_t seed=0;seed<128&&!pending;++seed){auto s=scene({235,199},{});s.players[0].items[0].used=true;s.players[0].hp=s.players[1].hp=99;s.players[1].guardian=Guardian::mars;s.tiebreak_gf=0;Match m;m.load_scene(s,0,seed);m.step({0,CommandKind::sacrifice,{0},{}});if(m.snapshot().phase!=MatchPhase::response)continue;pending=true;roundtrip(m);const auto exact=m.snapshot();
        for(unsigned error=0;error<4;++error){auto bad=exact;if(error==0)bad.batch->attack.kind=CommandKind::discard;if(error==1)bad.batch->initial_state.tiebreak_gf.reset();if(error==2)bad.batch->initial_state.tiebreak_gf=1;if(error==3)++bad.gifts[0];bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"whole pending replay rejects the wrong stage, launch kind and reserved copy count");}
        while(!m.done()&&m.snapshot().phase!=MatchPhase::main){std::uint64_t rng=seed;const auto action=m.sample_action(rng);m.step(action);roundtrip(m);}check(m.snapshot().owner==7&&m.snapshot().state.gf==1,"sacrifice resumes after guardian response before a single GF advance");}
    check(pending,"seed sweep covers a sacrifice paused during automatic guardian follow-up");
}

Match grant_fixture(SnapshotState state,std::uint64_t seed=7,std::uint32_t max_decisions=128,PlayerId owner=0){MatchConfig config;config.external_grants=true;config.max_decisions=max_decisions;Match match(config);match.load_scene(std::move(state),owner,seed);return match;}
void grant_phase_normal_draw_and_original_turn_completion(){
    auto state=scene({199},{});state.tiebreak_gf=0;auto game=grant_fixture(state);auto reference=fixture(state);reference.load_scene(state,0,7);reference.step({0,CommandKind::sacrifice,{0},{}});
    game.step({0,CommandKind::sacrifice,{0},{}});const auto pending=game.snapshot();check(pending.phase==MatchPhase::grant&&!pending.actor&&pending.pending_grant==0&&pending.gifts[0]==1
        &&pending.state.players[0].items.empty()&&pending.rng==7&&pending.next_token==1&&pending.decisions==1&&pending.grant_proof,"ordinary action pauses before any grant chooser draw with no player actor");roundtrip(game);
    check(game.observe({}).find("\"phase\":\"grant\"")!=std::string::npos&&game.observe({}).find("\"pending_grant\":{\"target\":0}")!=std::string::npos,"pending chance view exposes only its current real recipient0");
    auto policy=19ULL;bool failed=false;try{(void)game.sample_action(policy);}catch(const MatchError& e){failed=e.code==MatchErrorCode::unsupported;}check(failed&&policy==19&&game.snapshot()==pending,"player sampler cannot choose a grant or mutate policy RNG");reject(game,pray(0));
    game.grant_step({0,0,{},{}});auto expected=reference.snapshot().state;expected.update_count=game.snapshot().state.update_count;
    check(game.snapshot().state==expected&&game.snapshot().rng==reference.snapshot().rng&&game.snapshot().next_token==reference.snapshot().next_token
        &&game.snapshot().owner==7&&game.snapshot().decisions==2&&game.snapshot().grant_resolutions==1&&game.snapshot().grant_artifacts==1&&game.snapshot().grant_devils==0,"explicit ordinary outcome uses the unchanged chooser then advances the original turn once");roundtrip(game);
    check(game.snapshot().grant_proof->grants.size()==1&&game.observe({}).find("\"last_grant\":{\"target\":0,\"devil\":null,\"removed_models\":[]}")!=std::string::npos,"completed proof persists while ordinary actual model is absent from notice");
}
void grant_disabled_prethreshold_and_initial_distribution_parity(){
    auto state=scene({},{});state.tiebreak_gf=1;MatchConfig cfg;cfg.external_grants=true;Match external(cfg),plain;external.load_scene(state,0,11);plain.load_scene(state,0,11);external.step(pray(0));plain.step(pray(0));
    check(external.snapshot().state==plain.snapshot().state&&external.snapshot().rng==plain.snapshot().rng&&external.snapshot().phase==MatchPhase::main
        &&external.snapshot().grant_resolutions==0&&!external.snapshot().grant_proof&&external.snapshot().state.gf==1,"opt-in below threshold does not consume an extra draw or pause before the GF boundary");roundtrip(external);
    external.step(pray(7));check(external.snapshot().phase==MatchPhase::grant&&external.snapshot().pending_grant==7,"next owner starts reserved grant control after threshold is reached");roundtrip(external);
    Match a(cfg),b;check(a.snapshot().state==b.snapshot().state&&a.snapshot().rng==b.snapshot().rng&&!a.snapshot().state.tiebreak_gf,"initial reset grants are deliberately outside reserved apocalypse mode");
    auto active=scene({},{});active.tiebreak_gf=0;plain.load_scene(active,0,7);plain.step(pray(0));check(plain.snapshot().phase==MatchPhase::main&&!plain.snapshot().grant_proof,"disabled active scene keeps the old closed grant path");
    for(CardId id=0;id<=300;++id)check(match_grant_supported(id)==(id==0||(id>=240&&id<=244)),"grant choices are local normal sentinel0 or one of five supplied devils");
}
void grant_retry_trickster_and_decision_cutoff_finite(){
    auto state=scene({199,235,207,208,112},{});state.players[0].items[1].used=true;state.tiebreak_gf=0;auto game=grant_fixture(state);game.step({0,CommandKind::sacrifice,{0},{}});
    game.grant_step({0,244,Resource::cp,{}});check(game.snapshot().phase==MatchPhase::grant&&game.snapshot().gifts[0]==1&&game.snapshot().state.players[0].cp==30&&game.snapshot().next_token==5
        &&game.snapshot().grant_devils==1&&game.snapshot().devil_actions==0&&game.snapshot().state.gf==0,"fairy preserves the original reserved slot without a physical devil token or standalone activation");roundtrip(game);
    game.grant_step({0,243,{}, {1,2}});check(game.snapshot().gifts[0]==1&&game.snapshot().state.players[0].items.size()==2&&game.snapshot().last_grant->removed_models==std::vector<CardId>{235,207}
        &&game.snapshot().revivals==0&&game.snapshot().death_created==0,"trickster removes supplied physical copies without creating replacement debts or passive effects");roundtrip(game);
    game.grant_step({0,0,{},{}});check(game.snapshot().state.players[0].items.size()==3&&game.snapshot().grant_artifacts==1&&game.snapshot().grant_devils==2&&game.snapshot().owner==7,"only a normal grant closes the original slot after devil retries");roundtrip(game);
    auto capstate=scene({199},{});capstate.tiebreak_gf=0;auto cap=grant_fixture(capstate,7,5);cap.step({0,CommandKind::sacrifice,{0},{}});for(unsigned i=0;i<4;++i){cap.grant_step({0,244,Resource::hp,{}});roundtrip(cap);}
    check(cap.snapshot().truncated&&cap.snapshot().decisions==5&&cap.snapshot().grant_devils==4&&cap.snapshot().grant_artifacts==0&&!cap.snapshot().pending_grant
        &&cap.snapshot().state.players[0].items.empty()&&cap.snapshot().next_token==1&&cap.snapshot().rng==7,"an arbitrarily repeating fairy sequence reaches the explicit decision cutoff without forced ordinary draw");
}
void grant_revival_debt_historical_tick_and_posthumous_interleave(){
    auto state=scene({199,208,112},{});state.tiebreak_gf=0;state.players[0].hp=5;state.players[0].curses={Curse::cold};auto game=grant_fixture(state);game.step({0,CommandKind::sacrifice,{0},{}});
    check(game.snapshot().last_tick&&game.snapshot().last_tick->hp_after==4,"original owner tick precedes the first reserved gift");game.grant_step({0,240,{},{}});
    check(game.snapshot().state.players[0].hp==10&&game.snapshot().last_tick->hp_after==4&&game.snapshot().gifts[0]==2&&game.snapshot().revivals==1&&game.snapshot().phase==MatchPhase::grant,"revival adds a separate debt while original tick remains historical rather than being repeated");roundtrip(game);
    game.grant_step({0,0,{},{}});roundtrip(game);game.grant_step({0,0,{},{}});check(game.snapshot().state.players[0].items.size()==3&&game.snapshot().next_token==5&&game.snapshot().state.gf==1&&game.snapshot().owner==7,"original plus revival debts finish before a single owner/GF advance");roundtrip(game);
    auto lethal=scene({199,112},{190});lethal.players.push_back(player(9));lethal.tiebreak_gf=0;lethal.players[0].hp=5;lethal.players[0].curses={Curse::cold};lethal.players[1].hp=lethal.players[2].hp=99;lethal.players[1].curses=lethal.players[2].curses={Curse::darkcloud};auto death=grant_fixture(lethal);
    death.step({0,CommandKind::sacrifice,{0},{}});death.grant_step({0,240,{},{}});check(death.snapshot().phase==MatchPhase::response&&death.snapshot().queue[0].is_death
        &&death.snapshot().queue[0].source==0&&!death.snapshot().queue[0].environmental&&death.snapshot().gifts[0]==0,"dead recipient loses all debts and real player0's112 response precedes another gift");roundtrip(death);
    death.step(defend(7,{0}));roundtrip(death);death.step(defend(9));check(death.snapshot().phase==MatchPhase::grant&&!death.snapshot().actor&&death.snapshot().pending_grant==7,"after death FIFO, surviving reflector receives its reserved replacement before owner advance");roundtrip(death);
    death.grant_step({7,0,{},{}});check(death.snapshot().owner==7&&death.snapshot().state.gf==1&&death.snapshot().grant_proof->grants[1].responses_before==2
        &&death.snapshot().grant_proof->responses.size()==2&&death.snapshot().grant_resolutions==2,"ordered response prefixes preserve grant-death-response-grant interleaving");roundtrip(death);
}
void grant_fate_devil_guardian_and_world_original_sources(){
    auto state=scene({207},{});state.tiebreak_gf=0;auto fate=grant_fixture(state);fate.fate_step({0,0,{290,{},{}},{}});check(fate.snapshot().phase==MatchPhase::grant&&fate.snapshot().last_fate,"owned Fate keeps its original identity when its operation gift pauses");
    fate.grant_step({0,244,Resource::hp,{}});fate.grant_step({0,0,{},{}});check(fate.snapshot().last_fate&&fate.snapshot().fate_actions==1&&fate.snapshot().devil_actions==0&&fate.snapshot().state.players[0].hp==11
        &&fate.snapshot().last_phenomenon->model==290&&fate.snapshot().owner==7&&fate.snapshot().state.gf==1,"grant replay covers changed global postcondition and original ordinary Fate schedule");roundtrip(fate);
    auto rescued=scene({208},{});rescued.tiebreak_gf=0;rescued.players[0].hp=5;auto devil=grant_fixture(rescued,7,128,7);devil.devil_step({0,240,{},{}});devil.grant_step({0,244,Resource::mp,{}});devil.grant_step({0,0,{},{}});
    check(devil.snapshot().last_devil&&devil.snapshot().last_devil_event->model==240&&devil.snapshot().devil_actions==1&&devil.snapshot().grant_devils==1&&devil.snapshot().owner==7&&devil.snapshot().state.gf==0,"nested grant devil does not overwrite standalone devil proof or external schedule");roundtrip(devil);
    for(unsigned source=0;source<3;++source){auto s=scene({208},{});s.tiebreak_gf=0;s.players[0].hp=1;s.players[0].curses={Curse::darkcloud};s.players[1].guardian=source==2?Guardian::earth:Guardian::mars;auto m=grant_fixture(s,7,128,7);
        if(source==0)m.phenomenon_step({291,0,{}});else if(source==1)m.guardian_step({7,249,0});else m.guardian_payload_step({7,Guardian::earth,{6},0});
        check(m.snapshot().phase==MatchPhase::response,"external source produces a body response before revival grant");m.step(defend(0));check(m.snapshot().phase==MatchPhase::grant,"world/guardian/payload source resumes at reserved gift phase");roundtrip(m);m.grant_step({0,0,{},{}});
        check(m.snapshot().owner==7&&m.snapshot().state.gf==0&&m.snapshot().grant_resolutions==1,"external source keeps no GF/ordinary tick after its grant");roundtrip(m);
    }
    MatchConfig narrow_cfg;narrow_cfg.hand_limit=1;narrow_cfg.initial_hand=0;narrow_cfg.external_grants=true;Match narrow(narrow_cfg);
    auto narrow_state=scene({},{120});narrow_state.tiebreak_gf=0;narrow_state.players[0].guardian=Guardian::earth;narrow.load_scene(narrow_state,0,7);
    narrow.guardian_payload_step({0,Guardian::earth,{6,12},7});narrow.step(defend(7,{0}));
    check(narrow.snapshot().phase==MatchPhase::grant&&narrow.snapshot().grant_proof->models.size()==2,"external payload columns can exceed the physical hand limit without enlarging inventory");roundtrip(narrow);
    narrow.grant_step({7,0,{},{}});check(narrow.snapshot().owner==0&&narrow.snapshot().state.players[1].items.size()==1,"narrow-hand external payload preserves its original grant completion schedule");roundtrip(narrow);

}
void grant_shape_and_pending_state_atomic_rollback(){
    auto state=scene({199,235,207},{});state.players[0].items[1].used=true;state.tiebreak_gf=0;auto game=grant_fixture(state);check(!game.grant_legal({0,0,{},{}}),"ordinary main is not a grant chance point");game.step({0,CommandKind::sacrifice,{0},{}});const auto pending=game.snapshot();
    for(auto action:std::vector<GrantAction>{{7,0,{},{}},{0,207,{},{}},{0,0,Resource::hp,{}},{0,0,{}, {1}},{0,244,{},{}},{0,244,static_cast<Resource>(255),{}},{0,243,{}, {1}},{0,243,{}, {1,1}},{0,243,{}, {1,99}}}){
        check(!game.grant_legal(action),"only canonical choices for the current recipient are accepted");bool failed=false;try{game.grant_step(action);}catch(const MatchError&){failed=true;}check(failed&&game.snapshot()==pending,"bad grant resolution preserves the existing paused action rather than rewinding its launch");}
    for(unsigned limit=0;limit<3;++limit){auto s=scene({199},{});s.tiebreak_gf=0;if(limit==0)s.update_count=max_wire_integer-1;if(limit==1)s.players[0].items[0].token=max_wire_integer;if(limit==2)s.gf=max_wire_integer;auto m=grant_fixture(s);m.step({0,CommandKind::sacrifice,{s.players[0].items[0].token},{}});const auto pause=m.snapshot();bool failed=false;try{m.grant_step({0,0,{},{}});}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==pause&&m.snapshot().state.players[0].items.empty(),"late update/token/GF error restores pending state, not the already accepted sacrifice");roundtrip(m);}
}
void grant_full_ordered_proof_tamper_and_next_action_prefix(){
    auto state=scene({199},{});state.tiebreak_gf=0;auto m=grant_fixture(state);m.step({0,CommandKind::sacrifice,{0},{}});m.grant_step({0,244,Resource::cp,{}});m.grant_step({0,244,Resource::mp,{}});const auto exact=m.snapshot();
    for(unsigned error=0;error<16;++error){auto bad=exact;if(error==0)bad.pending_grant=7;if(error==1)bad.actor=0;if(error==2)bad.config.external_grants=false;if(error==3)bad.grant_proof.reset();if(error==4)bad.last_grant->model=0;if(error==5)bad.grant_proof->grants.pop_back();if(error==6)bad.grant_proof->grants[0].responses_before=1;if(error==7)bad.grant_proof->grants[0].action.target=7;if(error==8)bad.grant_proof->attack.kind=CommandKind::discard;if(error==9)bad.grant_proof->initial_state.tiebreak_gf.reset();if(error==10)++bad.grant_proof->initial_state.players[0].cp;if(error==11)bad.grant_artifacts=1;if(error==12)++bad.next_token;if(error==13)++bad.rng;if(error==14)++bad.gifts[0];if(error==15)bad.grant_proof->fate_action=FateAction{0,0,{290,{},{}},{}};
        bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed&&m.snapshot()==exact,"grant proof rejects changed stage, source, actor, choice order, counters and hidden continuation");}
    m.grant_step({0,0,{},{}});roundtrip(m);check(m.snapshot().grant_proof&&m.snapshot().phase==MatchPhase::main&&m.snapshot().grant_resolutions==3,"completed original action retains its grant journal");
    auto bad=m.snapshot();bad.last_grant->model=207;bool failed=false;try{m.restore(bad);}catch(const MatchError&){failed=true;}check(failed,"ordinary model cannot be smuggled into the private/public grant notice");
    m.step(pray(7));check(m.snapshot().phase==MatchPhase::grant&&m.snapshot().grant_proof->grant_resolutions==3&&m.snapshot().grant_proof->grants.empty()&&!m.snapshot().last_grant,"next original action resets completed journal and carries only cumulative grant prefix");roundtrip(m);m.grant_step({7,0,{},{}});roundtrip(m);
}
void grant_dream_privacy_and_immediate_delivery_exceptions(){
    bool masked=false;for(std::uint64_t seed=0;seed<128&&!masked;++seed){auto s=scene({},{});s.tiebreak_gf=0;s.players[0].curses={Curse::dream};auto m=grant_fixture(s,seed);m.step(pray(0));m.grant_step({0,0,{},{}});masked=m.snapshot().state.players[0].items[0].fake_model.has_value();
        if(masked){check(m.observe(0).find("\"last_grant\":{\"target\":0,\"devil\":null,\"removed_models\":[]}")!=std::string::npos,"grant notice does not reveal a Dream owner's actual ordinary model");roundtrip(m);}}
    check(masked,"seed sweep exercises ordinary compatible Dream masking after external grant choice");
    auto s=scene({6},{178,235});s.players[1].items[1].used=true;s.tiebreak_gf=0;auto redraw=grant_fixture(s);redraw.step(play(0,{0},7));redraw.step(defend(7,{0}));
    check(redraw.snapshot().phase==MatchPhase::grant&&redraw.snapshot().pending_grant==0&&redraw.snapshot().redraws==1&&redraw.snapshot().state.players[1].items.size()==1
        &&redraw.snapshot().state.players[1].items[0].token==2&&!redraw.snapshot().state.players[1].items[0].used&&redraw.snapshot().grant_resolutions==0,"178 redraw still grants immediately before the original body's reserved gift stage");roundtrip(redraw);
    redraw.grant_step({0,0,{},{}});redraw.grant_step({7,0,{},{}});roundtrip(redraw);
    auto trade_state=scene({4,6},{235});trade_state.tiebreak_gf=0;trade_state.players[1].items[0].used=true;trade_state.players[1].cp=99;auto trade=grant_fixture(trade_state);trade.step({0,CommandKind::sell,{0,1},7});trade.step(defend(7));
    check(trade.snapshot().phase==MatchPhase::grant&&trade.snapshot().pending_grant==0&&trade.snapshot().last_sale&&trade.snapshot().state.players[1].items.back().model==6
        &&trade.snapshot().gifts[0]==1&&trade.snapshot().gifts[1]==0&&trade.snapshot().grant_resolutions==0,"trade delivery remains immediate and only reserved replacements enter grant control");roundtrip(trade);trade.grant_step({0,0,{},{}});roundtrip(trade);
}

}
int main(){
    scope_reset_and_local_contract();
    weapon_frames_colors_owner_and_fixed_rewards();
    recovery_retention_capacity_and_atomicity();
    reflection_filter_and_immutable_origin();
    reflection_snapshot_adversaries();
    reflection_chain_exceeds_legacy_limit();
    reflection_role_capacity_and_late_failures();
    blocking_roles_retention_and_reflection();
    ordered_free_setter_and_double();
    free_response_capacity_sampler_and_atomicity();
    bounce_targets_roles_and_rng_provenance();
    retained_bounce_exceeds_physical_limit_and_free();
    chance_scope_boundaries_and_single_cost();
    chance_multiplayer_journal_and_privacy();
    chance_dead_skip_origin_cancel_cutoff_and_rollback();
    counter_damage_fifo_and_reactions();
    counter_chance_death_journal_and_atomicity();
    counter_generation_exceeds_old_duel_limit();
    resource_counter_mp_cp_and_alias();
    resource_counter_dead_skip_and_journal();
    mp_refill_extends_retained_redirect_budget();
    absorption_stages_reflection_and_retention();
    same_damage_stages_deaths_and_journal();
    composed_damage_effect_and_block_cancel();
    flash_damage_selection_and_cures();
    flash_counter_chance_and_journal();
    darkcloud_forced_chance_and_mixed_roster();
    darkcloud_damage_sources_and_cures();
    darkcloud_counter_fifo_forced_and_selection();
    darkcloud_canonical_journal_and_cutoff();
    removal_counts_tokens_and_rng();
    removal_retention_gifts_and_status();
    removal_reflection_and_journal_atomicity();
    mana_weapon_immutable_power_and_payment();
    twice_weapon_fresh_frames_costs_and_order();
    twice_death_cutoff_and_mana_journal();
    wide_target_modes_payment_and_free();
    wide_twice_sixteen_fresh_frames();
    wide_counter_death_and_journal();
    random_hp_sign_clips_and_death();
    random_hp_reflections_fixed_and_journal();
    random_hp_query_cutoff_and_rollback();
    revival_passive_boundaries_and_prayer();
    revival_counters_repeats_and_simultaneous();
    revival_journal_cutoff_and_rollback();
    danger_random_targets_reflections_and_visibility();
    danger_mortar_priority_composition_and_revival();
    danger_selection_gap_journal_and_atomicity();
    exchange_allocation_fee_revival_and_noop();
    exchange_invalid_roles_and_policy_allocations();
    exchange_cutoff_and_atomic_failures();
    removal_event_public_order_duplicates_and_reflection();
    removal_event_lifetime_empty_and_rollback();
    removal_event_snapshot_guards();
    sell_arithmetic_transfer_and_commodity_is_not_cast();
    sell_reflections_delivery_revival_and_cutoff();
    sell_invalid_roles_sampler_and_journal();
    sell_event_guards_and_atomic_late_failures();
    buy_quote_accept_decline_and_public_escrow();
    buy_empty_affordability_and_unused_miracles();
    buy_reflection_ownership_and_quote_journal();
    buy_roles_cutoff_returns_and_late_failure();
    disease_reapplication_cures_and_damage_counter_basis();
    disease_fixed_roll_post_upgrade_owner_tick();
    disease_action_boundaries_quotes_repeats_and_cutoff();
    disease_tick_revivals_and_body_accounting();
    disease_canonical_tick_guards_and_atomic_failure();
    fever_mask_full_defense_copies_and_append_boundaries();
    fever_mask_prefix_fall_body_counter_and_self_alias();
    heaven_herb_mp_first_reflection_rescue_and_cutoff();
    disease_items_nonuse_roles_and_atomic_failures();
    fog_launch_targets_singleton_team_repeat_and_rng();
    fog_random202_order_exceptions_and_redirects();
    fog_projection_and_query_noninterference();
    fog_forced_hit_redaction_survives_reflection();
    fog_application_cures_journal_and_atomicity();
    death_main_rate_copies_and_target_roster();
    death_redirects_counters_and_origin_death_chains();
    death_general_main_and_single_owner_tick_journals();
    death_natural_cutoff_revival_and_simultaneous_boundaries();
    death_snapshot_adversaries_and_atomic_late_failures();
    death_nine_seat_large_fifo_and_natural_cancellation();
    trade_overflow_inventory_order_types_and_gift_accounting();
    trade_overflow_debt_eviction_revival_and_death_order();
    fog_sell_launch_commodity_roles_reflections_and_overflow();
    overflow_event_full_replay_and_late_atomic_failures();
    trade_capacity_queries_and_sampler_use_only_visible_inputs();
    dream_compatibility_predicates_and_payloads();
    dream_gift_rng_and_no_retroactive_mask();
    dream_same_visible_policy_and_actual_execution();
    dream_sources_cures_and_owner_only_revelation();
    dream_trade_quotes_removal_and_overflow();
    dream_checkpoint_guards_and_transactional_replay();
    redraw_replaces_used_inventory_and_preserves_reserved_gifts();
    redraw_consumes_whole_selection_before_each_ordered_prefix();
    redraw_fresh_responses_and_passive_revival_are_immediate();
    redraw_private_inventory_and_public_event_lifetime();
    redraw_trade_and_original_payload_survive_replacement();
    redraw_snapshot_adversaries_and_late_atomic_failures();
    self_buy_inspects_same_copy_without_money_or_quote();
    self_buy_empty_and_used_miracles_preserve_candidate_order();
    self_buy_dream_reveals_only_chosen_copy_and_queries_use_faces();
    self_buy_fog_team_capacity_and_sampler_do_not_peek();
    self_buy_events_failures_cutoff_and_shape_guards();
    self_buy_seeded_rollouts_remain_replayable();
    guardian_all_canonical_actions_and_distinct_inventory_pool();
    guardian_reaction_categories_and_immutable_identity();
    guardian_chance_privacy_and_no_opportunity_sampling();
    guardian_venus_cp_global_credit_and_debt_order();
    guardian_cure_earth_gifts_and_ordinary_owner_are_separate();
    guardian_counters_death_and_resume_owner();
    guardian_domains_cutoff_snapshot_forgery_and_rollback();
    guardian_seeded_complete_rollouts_remain_deterministic();
    guardian_summon_all_ten_and_normal_cast_roles();
    guardian_auto_all_types_draw_order_and_once_per_source();
    guardian_departure_actual_stages_and_event_time_visibility();
    guardian_auto_tick_history_counter_fifo_and_owner_death();
    guardian_auto_snapshot_forgery_and_late_failures();
    guardian_auto_seeded_full_turns_replay_all_private_state();
    checkpoint_cutoff_and_seeded_multiplayer();
    guardian_payload_all_bases_and_inventory_independence();
    guardian_payload_domains_shapes_and_strict_composition();
    guardian_payload_mana_reflection_and_launch_context();
    guardian_payload_repeat_counter_fifo_and_fixed_identity();
    guardian_payload_snapshot_forgery_cutoff_and_late_rollback();
    guardian_payload_summon_random_and_seeded_replay();
    phenomenon_capabilities_fever_replacement_and_atomic_roles();
    phenomenon_fog_idempotence_and_hp_setter_are_not_damage();
    phenomenon_cp_collection_recipient_alias_debt_and_cap();
    phenomenon_guardian_nine_assignments_event_time_fog_and_cutoff();
    phenomenon_strict_shapes_pending_exclusion_and_late_rollback();
    phenomenon_prefix_counters_survive_all_later_action_replay();
    targeted_phenomenon_source_absence_and_light_filter();
    targeted_phenomenon_dark_single_chance_force_and_full_defense();
    targeted_phenomenon_warm_response_cap_and_environmental_cancel();
    targeted_phenomenon_source_counters_skip_but_self_mp_is_real();
    targeted_phenomenon_owner_death_fifo_and_cutoff();
    targeted_phenomenon_replay_forgery_rollback_and_seeded_turns();
    item_shuffle_source_pairs_fresh_tokens_and_preserved_copies();
    item_shuffle_dream_boundary_and_retained_recast();
    item_shuffle_capacity_dead_zero_and_token_budget();
    item_shuffle_invalid_cover_pending_and_rollback();
    item_shuffle_full_proof_rejects_tampering();
    item_shuffle_prefix_and_all_later_actions_replay();
    confusion_addition_scope_and_projection();
    confusion_api_gate_overflow_and_transaction();
    confusion_all_main_kinds_debit_and_cures_preserve();
    confusion_launch_response_replay_and_tamper();
    confusion_purchase_and_sale_resolution();
    confusion_death_revival_end_and_cutoff();
    confusion_seeded_external_controller_rollouts();
    fate_special_scene_capability_and_sampler_boundary();
    fate_global_bodies_cutoff_and_ordinary_schedule();
    fate_targeted_source_free_response_and_replay();
    fate_shuffle_post_consumption_copy_and_gift_order();
    fate_confusion_revival_death_and_natural_end();
    fate_invalid_shapes_and_late_failure_atomicity();
    fate_full_launch_response_and_completed_proof_tamper();
    fate207_inert_discard_trade_fog_and_removal();
    fate_guardian_followups_and_repeated_counter_anchors();
    devil_capability_direct_damage_and_no_response();
    devil_fairy_three_resources_clip_and_private_choice();
    devil_trickster_exact_copies_used_fake_and_inert_passives();
    devil_bad_shapes_pending_and_late_atomic_failure();
    devil_revival_and_natural_end_boundaries();
    devil_posthumous_fifo_real_source_zero_and_owner_resume();
    devil_guardian_departure_local_rule_and_fog_filter();
    devil_full_proof_tamper_and_following_anchor_prefixes();
    sacrifice_threshold_scene_and_transition_boundaries();
    sacrifice_multi_used_fake_inert_and_zero_fee();
    sacrifice_full18_and_cutoff_cancels_all_gifts();
    sacrifice_role_identity_and_phase_rejections();
    sacrifice_confusion_one_debit_and_sampler_parity();
    sacrifice_tick_death_revival_and_natural_completion();
    sacrifice_exhaustion_rollback_and_launch_threshold_replay();
    grant_phase_normal_draw_and_original_turn_completion();
    grant_disabled_prethreshold_and_initial_distribution_parity();
    grant_retry_trickster_and_decision_cutoff_finite();
    grant_revival_debt_historical_tick_and_posthumous_interleave();
    grant_fate_devil_guardian_and_world_original_sources();
    grant_shape_and_pending_state_atomic_rollback();
    grant_full_ordered_proof_tamper_and_next_action_prefix();
    grant_dream_privacy_and_immediate_delivery_exceptions();





    std::cout<<"Experimental official match passed (236subset/canonical-guardian-auto/self-buy/redraw/Dream/overflow/death/Fog/privacy/exchange/danger/revival/randomHP/wide/twice/mana/removal/Flash/DarkCloud/forced/damage-effects/resource-counter/counter/chance/bounce/ordered/FREE/reflection/9teams/snapshot/RNG/cutoff; server scheduling not asserted).\n";
}
