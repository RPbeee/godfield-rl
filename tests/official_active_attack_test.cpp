#include "godfield/official_state.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <string>
#include <vector>

namespace {
using namespace godfield::official;
void check(bool value,const char* message){if(!value){std::cerr<<"FAIL official active attack: "<<message<<'\n';std::exit(1);}}
template<class E,class F>void throws_as(F&& function,const char* message){bool caught=false;try{function();}catch(const E&){caught=true;}check(caught,message);}
SnapshotState board(){
    SnapshotState state;state.gf=17;state.update_count=42;
    PlayerState source;source.id=0;source.name="source";source.user_id="private-source-user";source.team=Team::one;
    PlayerState target;target.id=7;target.name="target";target.alias="target alias";target.user_id="private-target-user";target.team=Team::two;
    state.players={source,target};return state;
}
ActiveAttackState frame(std::vector<CardId> models){ActiveAttackState out;out.player_id=0;out.target_player_id=7;out.models=std::move(models);return out;}
ActiveAttackView show(const SnapshotState& state,const ActiveAttackState& attack){
    const std::array<ActiveAttackState,1> head{attack};const auto view=project_active_attack(state,head);
    check(view.has_value(),"expected a displayed head frame");return *view;
}
ActiveAttackView show(const ActiveAttackState& attack){return show(board(),attack);}
std::string json(const ActiveAttackView& view){return active_attack_json(std::optional<ActiveAttackView>{view});}

void ordinary_prayer_and_sanitized_json(){
    const auto state=board();auto attack=frame({6,9});attack.reversed=true;
    const auto before_state=state;const auto before_attack=attack;const auto view=show(state,attack);
    check(state==before_state && attack==before_attack,"projection mutates its board or received frame");
    check(view.source==0 && view.target==7 && view.guardian==Guardian::none && view.reversed
          && view.models==std::vector<CardId>({6,9}) && view.summary_models==view.models
          && view.is_attack && !view.prayer && view.power==2 && view.hit_rate==0
          && view.incoming_category==Category::weapons && !view.incoming_element,"ordinary model composition and source0 display");
    check(view.cards.size()==2 && view.cards[0].model==6 && !view.cards[0].atk && !view.cards[0].hit_rate
          && !view.cards[0].mp && !view.cards[0].cp && !view.cards[0].from_mp,"ordinary cards contain no fabricated dynamic override");
    const auto encoded=json(view);
    check(encoded.starts_with("{\"schema\":\"official-active-attack-view-v1\",\"attack\":{")
          && encoded.find("\"source\":0")!=std::string::npos && encoded.find("\"reversed\":true")!=std::string::npos
          && encoded.find("\"power\":2,\"hit_rate\":0")!=std::string::npos
          && encoded.find("private-source-user")==std::string::npos,"JSON uses a separate schema and only sanitized public fields");
    auto empty=frame({});empty.atk=77;empty.hit_rate=99;empty.mp=-12;empty.cp=34;
    const auto prayer=show(empty);
    check(prayer.prayer && !prayer.is_attack && prayer.models.empty() && prayer.cards.empty()
          && prayer.summary_models.empty() && !prayer.power && !prayer.hit_rate && !prayer.incoming_category
          && prayer.target==7,"empty head is prayer even with positive received numeric fields");
    check(json(prayer).find("\"response\":null")!=std::string::npos,"prayer has no fabricated response context");
    const std::array<ActiveAttackState,0> none{};
    check(!project_active_attack(state,none) && active_attack_json(std::nullopt)=="{\"schema\":\"official-active-attack-view-v1\",\"attack\":null}","no head serializes as null");
}

void fate_prefix_is_dropped_exactly_once(){
    auto attack=frame({207});const auto single=show(attack);
    check(single.models==std::vector<CardId>{207} && single.summary_models==single.models && !single.is_attack
          && single.incoming_category==Category::sundries && !single.power,"single207 is not removed or turned into a phenomenon");
    attack.models={207,6};const auto compound=show(attack);
    check(compound.models==std::vector<CardId>({207,6}) && compound.cards.size()==2
          && compound.summary_models==std::vector<CardId>{6} && compound.power==1
          && compound.incoming_category==Category::weapons,"compound207 retains all visible cards but drops one summary prefix");
    attack.models={207,207,6};const auto triple=show(attack);
    check(triple.models.size()==3 && triple.cards.size()==3 && triple.summary_models==std::vector<CardId>({207,6})
          && !triple.is_attack && !triple.power && triple.incoming_category==Category::sundries,"triple207 does not recursively unwrap the summary");
    attack.atk=3;attack.hit_rate=42;const auto dynamic=show(attack);
    check(dynamic.is_attack && dynamic.power==4 && dynamic.hit_rate==42
          && dynamic.incoming_category==Category::sundries && !dynamic.cards[0].atk,"positive wire ATK can activate a summary without repainting an ordinary card");
    attack=frame({207,207});check(show(attack).summary_models==std::vector<CardId>{207},"two207 entries leave one summary model");
    attack=frame({6,207});check(show(attack).summary_models==attack.models,"a nonleading207 is never removed");
}

void mana_and_dying_card_numeric_branches(){
    auto attack=frame({58});attack.atk=-17;attack.hit_rate=-9;attack.mp=888;attack.cp=777;
    const auto negative=show(attack);
    check(negative.cards[0].atk==-17 && negative.cards[0].from_mp && !negative.cards[0].mp
          && negative.is_attack && negative.power==0 && negative.hit_rate==0,"58 shows signed received ATK but nonpositive override gives zero summary power");
    attack.atk=0;const auto zero=show(attack);check(zero.cards[0].atk.has_value() && *zero.cards[0].atk==0
          && zero.power.has_value() && *zero.power==0,"58 numeric zero is distinct from an absent dynamic label");
    attack.atk=20;check(show(attack).cards[0].atk==20 && show(attack).power==20,"58 uses received ATK rather than board MP or raw mp");
    auto bow=frame({112});const auto ordinary=show(bow);
    check(ordinary.cards[0].atk==1 && ordinary.cards[0].hit_rate==25 && ordinary.power==1 && ordinary.hit_rate==25,"112 default card and summary paint the static1/25 values");
    bow.atk=1;bow.hit_rate=25;
    check(show(bow)==ordinary && json(show(bow))==json(ordinary),"112 raw0/0 and explicit1/25 have indistinguishable public display");
    bow.atk=-2;bow.hit_rate=-8;
    check(show(bow)==ordinary,"negative112 overrides use the same final static card labels");
    bow.atk=30;bow.hit_rate=-7;const auto dying=show(bow);
    check(dying.cards[0].atk==30 && dying.cards[0].hit_rate==-7 && dying.power==30 && dying.hit_rate==25,"positive112 ATK paints signed raw card rate while summary nonpositive rate falls back");
    bow.atk=0;bow.hit_rate=75;const auto rate_only=show(bow);
    check(rate_only.cards[0].atk==1 && rate_only.cards[0].hit_rate==25 && rate_only.power==1 && rate_only.hit_rate==75,"112 summary hit override does not invent a card override when ATK is nonpositive");
}

void counter_labels_do_not_recompute_effects(){
    for(CardId id:{CardId{181},CardId{184}}){
        auto attack=frame({id});attack.atk=7;attack.hit_rate=80;const auto positive=show(attack);
        check(positive.cards[0].atk==7 && !positive.cards[0].hit_rate && positive.is_attack
              && positive.power==7 && positive.hit_rate==80 && positive.incoming_category==Category::armor,"counter ATK is already a received amount, not damage to double again");
        attack.atk=-7;const auto negative=show(attack);
        check(negative.cards[0].atk==-7 && !negative.is_attack && !negative.power && !negative.hit_rate,"signed nonzero counter label does not turn a nonpositive body into an attack");
        attack.atk=0;check(!show(attack).cards[0].atk,"zero counter ATK retains its generic card label");
    }
    auto mp=frame({187});mp.mp=-4;mp.cp=91;mp.hit_rate=90;const auto mp_view=show(mp);
    check(mp_view.cards[0].mp==-4 && !mp_view.cards[0].cp && !mp_view.is_attack && !mp_view.power
          && mp_view.guardian==Guardian::none,"187 paints signed MP only and its Neptune metadata does not become an action guardian");
    mp.mp=0;check(!show(mp).cards[0].mp,"zero187 MP keeps the generic label");
    auto cp=frame({188});cp.cp=9;cp.mp=93;const auto cp_view=show(cp);
    check(cp_view.cards[0].cp==9 && !cp_view.cards[0].mp && !cp_view.is_attack,"188 paints CP without inferring theft direction or MP");
    cp.cp=-9;check(show(cp).cards[0].cp==-9,"negative received CP follows the nonzero card branch");
    cp.cp=0;check(!show(cp).cards[0].cp,"zero188 CP keeps the generic label");
    for(const auto [id,curse]:std::array<std::pair<CardId,Curse>,4>{{{182,Curse::fog},{183,Curse::dream},{185,Curse::flash},{186,Curse::darkcloud}}}){
        const auto view=show(frame({id}));check(view.cards[0].curse==curse && !view.cards[0].atk && !view.is_attack
              && view.incoming_category==Category::armor,"curse-counter card paints its own curse without a fabricated ATK trigger");
    }
}

void summary_category_percent_and_positive_overrides(){
    auto attack=frame({264});attack.hit_rate=75;attack.guardian=Guardian::saturn;const auto percent=show(attack);
    check(percent.is_attack && percent.power==15 && percent.hit_rate==75 && percent.guardian==Guardian::saturn
          && percent.incoming_category==Category::weapons && percent.incoming_element==Element::stone,"categoryWeapons remains weapons with a received percentage");
    attack=frame({286,225});const auto moon=show(attack);
    check(moon.power==15 && moon.hit_rate==0 && moon.incoming_category==Category::weapons,"later percentage MC does not replace the base summary rate or effective category");
    attack=frame({187});attack.atk=5;attack.hit_rate=127;const auto dynamic=show(attack);
    check(dynamic.is_attack && dynamic.power==5 && dynamic.hit_rate==127 && !dynamic.cards[0].atk
          && dynamic.incoming_category==Category::armor,"positive dynamic ATK creates an attack summary without a new card or category");
    attack.atk=0;const auto nonattack=show(attack);
    check(!nonattack.is_attack && !nonattack.power && !nonattack.hit_rate,"positive hit alone does not create an attack");
    attack=frame({181});attack.hit_rate=-10;const auto inactive=show(attack);
    check(!inactive.is_attack && inactive.incoming_element==Element::fire
          && json(inactive).find("\"summary\":{\"models\":[181],\"prayer\":false,\"is_attack\":false,\"element\":null")!=std::string::npos
          && json(inactive).find("\"incoming_element\":\"fire\",\"incoming_is_attack\":false")!=std::string::npos,"hidden summary element and nonattack response context are distinct");
}

void lookup_ending_head_only_and_large_columns(){
    auto state=board();auto attack=frame({5});attack.target_player_id=0;attack.buying_model=207;
    auto absent=show(state,attack);check(!absent.target && !absent.buying_model,"wire target0 is a sentinel even when real player0 exists");
    attack.target_player_id=7;check(show(state,attack).target==7 && show(state,attack).buying_model==207,"known positive target permits the received commodity display");
    attack.buying_model=0;check(!show(state,attack).buying_model,"zero commodity is absent");
    attack.player_id=99;throws_as<std::invalid_argument>([&]{(void)show(state,attack);},"unknown source must be looked up");
    attack.player_id=0;attack.target_player_id=99;throws_as<std::invalid_argument>([&]{(void)show(state,attack);},"unknown positive target must be looked up");
    attack.player_id=99;state.is_over=true;const std::array<ActiveAttackState,1> unknown{attack};
    check(!project_active_attack(state,unknown),"ending suppresses source/target lookups before missing-player errors");
    state=board();state.players[0].dead=true;state.players[0].hp=0;state.players[1].dead=true;state.players[1].hp=0;
    check(show(state,frame({112})).source==0,"fresh display is not actor authorization and can show dead participants");
    state=board();auto bad=frame({0});bad.player_id=max_wire_integer+1;bad.atk=std::numeric_limits<std::int64_t>::max();
    const std::array<ActiveAttackState,2> tail{frame({6}),bad};
    check(project_active_attack(state,tail)==std::optional<ActiveAttackView>{show(frame({6}))},"tail frames are not decoded, validated or composed");
    throws_as<std::invalid_argument>([&]{validate_active_attack(bad);},"ignored tail is independently invalid rather than silently valid");
    const std::array<ActiveAttackState,2> prayer_head{frame({}),frame({6})};
    check(project_active_attack(state,prayer_head)->prayer,"empty first frame is not replaced by a later attack");
    auto many=frame(std::vector<CardId>(19,6));const auto large=show(many);
    check(large.cards.size()==19 && large.models.size()==19 && large.summary_models.size()==19 && large.power==19,"public model columns have no18-card hand/token-selection cap");
}

void public_frame_and_unpainted_wire_noninterference(){
    const auto state=board();auto attack=frame({6});const auto expected=show(state,attack);const auto expected_json=json(expected);
    attack.atk=-static_cast<std::int64_t>(max_wire_integer);attack.hit_rate=-456;attack.mp=max_wire_integer;attack.cp=-654321;
    check(show(state,attack)==expected && json(show(state,attack))==expected_json,"unpainted ATK/hit/MP/CP do not survive as raw display fields");
    auto private_board=state;private_board.gf=999;private_board.update_count=777;private_board.tiebreak_gf=50;
    private_board.players[0].name="different source name";private_board.players[0].user_id="hidden-user-A";private_board.players[0].alias="different alias";
    private_board.players[0].hp=1;private_board.players[0].mp=99;private_board.players[0].cp=0;private_board.players[0].guardian=Guardian::moon;
    private_board.players[0].curses={Curse::fog,Curse::dream};private_board.players[0].confusion_count=99;
    private_board.players[0].items={{123,58,6,false},{124,235,{},true}};
    private_board.players[1].name="hidden target";private_board.players[1].user_id="hidden-user-B";private_board.players[1].hp=90;
    private_board.players[1].curses={Curse::heaven,Curse::fog};private_board.players[1].items={{999,207,{},false}};
    check(show(private_board,attack)==expected && json(show(private_board,attack))==expected_json,"Fog/resources/names/user IDs/actual or fake inventory cannot alter an already-public frame");
    auto prayer=frame({});prayer.target_player_id=0;const auto clean=show(prayer);prayer.atk=71;prayer.hit_rate=98;prayer.mp=-23;prayer.cp=45;
    check(show(prayer)==clean,"prayer does not expose unused numeric wire values");
    auto neutral=frame({207});const auto clean_nonattack=show(neutral);neutral.atk=-1;neutral.hit_rate=99;neutral.mp=88;neutral.cp=77;
    check(show(neutral)==clean_nonattack,"nonattack207 does not leak unrelated numeric fields");
    auto mana=frame({58});mana.atk=12;const auto mana_view=show(state,mana);private_board.players[0].mp=0;
    check(show(private_board,mana)==mana_view,"58 public label is the received ATK, not a hidden board-MP oracle");
    check(json(show(private_board,attack)).find("hidden-user")==std::string::npos,"active JSON contains no board identity strings");
}

void strict_wire_validation_and_checked_power_overflow(){
    const auto good=frame({6});const auto state=board();
    for(unsigned error=0;error<11;++error){auto bad=good;
        if(error==0)bad.player_id=max_wire_integer+1;if(error==1)bad.target_player_id=max_wire_integer+1;
        if(error==2)bad.guardian=static_cast<Guardian>(11);if(error==3)bad.models={0};if(error==4)bad.models={297};
        if(error==5){bad.buying_model=297;bad.target_player_id=0;}if(error==6)bad.atk=static_cast<std::int64_t>(max_wire_integer)+1;
        if(error==7)bad.hit_rate=-static_cast<std::int64_t>(max_wire_integer)-1;if(error==8)bad.mp=std::numeric_limits<std::int64_t>::min();
        if(error==9)bad.cp=std::numeric_limits<std::int64_t>::max();if(error==10)bad.guardian=static_cast<Guardian>(255);
        const auto copy=bad;throws_as<std::invalid_argument>([&]{(void)show(state,bad);},"unsafe received ID/model/guardian/numeric field rejected");check(bad==copy,"validation failure mutated its received input");
    }
    auto ended=state;ended.is_over=true;auto malformed=good;malformed.models={297};const std::array<ActiveAttackState,1> head{malformed};
    throws_as<std::invalid_argument>([&]{(void)project_active_attack(ended,head);},"ending skips roster lookup, not strict validation of the inspected head");
    auto wide=good;wide.atk=max_wire_integer;wide.models.insert(wide.models.end(),10,231);
    const auto exact=show(wide);check(exact.power==static_cast<std::int64_t>(max_wire_integer)*1024,"safe wire override may produce a checked int64 power larger than the wire-safe input domain");
    wide.models.push_back(231);const auto before=wide;
    throws_as<std::overflow_error>([&]{(void)show(wide);},"display composition rejects int64 overflow instead of wrapping");check(wide==before,"overflow failure changed the source frame");
    auto dormant=frame({3});dormant.models.insert(dormant.models.end(),70,231);
    check(!show(dormant).is_attack && !show(dormant).power,"nonattack summary does not evaluate an unpainted overflowing power");
    dormant.atk=1;throws_as<std::overflow_error>([&]{(void)show(dormant);},"positive dynamic ATK activates checked composition even for a nonweapon base");
}
}
int main(){
    ordinary_prayer_and_sanitized_json();
    fate_prefix_is_dropped_exactly_once();
    mana_and_dying_card_numeric_branches();
    counter_labels_do_not_recompute_effects();
    summary_category_percent_and_positive_overrides();
    lookup_ending_head_only_and_large_columns();
    public_frame_and_unpainted_wire_noninterference();
    strict_wire_validation_and_checked_power_overflow();
    std::cout<<"Official active attack passed (8 boundary/noninterference groups; source display, no timing or engine authorization).\n";
}
