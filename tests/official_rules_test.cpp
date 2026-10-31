#include "godfield/official_rules.hpp"
#include <array>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using namespace godfield::official;
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL official rules: "<<message<<'\n';std::exit(1);}}
template<class F>void throws(F&& f,const char* message){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught,message);}
CardId image(std::string_view name){for(const auto& c:catalog())if(name==c.image_name)return c.id;check(false,"known current model exists");return 0;}
std::vector<CardId> ids(std::initializer_list<std::string_view> names){std::vector<CardId> v;for(auto n:names)v.push_back(image(n));return v;}
std::vector<VisibleSelection> selected(std::initializer_list<std::string_view> names){std::vector<VisibleSelection> v;for(auto n:names)v.push_back({image(n),false});return v;}
VisibleSelection next(std::string_view name,bool used=false){return {image(name),used};}
bool main(std::initializer_list<std::string_view> old,std::string_view n,bool used=false){return can_append_main(selected(old),next(n,used));}
bool response(std::initializer_list<std::string_view> old,std::string_view n,ResponseContext c){return can_append_response(selected(old),next(n),c);}

void composition_and_cost() {
    check(attack_power(ids({"bronze-club","wand-of-ignition"}))==3 && attack_element(ids({"bronze-club","wand-of-ignition"}))==Element::fire,
        "current setter contributes its physicalATK2 and then replaces attribute");
    check(attack_power(ids({"bronze-club","wand-of-mystic-water"}))==6,"current water setter adds5, unlike legacy zero additive setter");
    check(attack_power(ids({"bronze-club","wand-of-ignition","aura"}))==6
        && attack_power(ids({"bronze-club","aura","wand-of-ignition"}))==4,"DOUBLE order affects current accumulated raw3x2 versus1x2+2");
    check(attack_power(ids({"bronze-club","wand-of-ignition","spiritual-staff"}))==3
        && attack_element(ids({"bronze-club","wand-of-ignition","spiritual-staff"}))==Element::fire,"FREE physical ATK12 and absent element are both skipped in attack composition");
    check(!defense_element(ids({"flame-boots","spiritual-socks"})),"FREE is not skipped in defense element merge");
    check(attack_element(ids({"wand-of-ignition","rainbow-curtain"}))==Attribute{}
        && attack_element(ids({"wand-of-ignition","rainbow-curtain","wand-of-mystic-water"}))==Element::water,"filter clears attribute; later setter can replace it again");
    check(attack_power(ids({"magical-stick"}),20)==20 && attack_power(ids({"magical-stick","crossbow"}),20)==22
        && attack_power(ids({"bronze-club"}),0)==1,"positive base override only; zero sentinel falls back to base model");
    check(magic_cost({})==0 && magic_cost(ids({"spring"}))==7 && magic_cost(ids({"spring","spiritual-doll"}))==0,
        "current Spring costs7 and immediate FREE waives its cost");
    check(magic_cost(ids({"spring","spiritual-doll","aura","spiritual-socks"}))==0,"each immediate MAGIC→FREE pair is waived, not only the first pair");
    check(magic_cost(ids({"bronze-club","spring","spiritual-doll","spring"}))==7
        && magic_cost(ids({"spring","crossbow","spiritual-doll"}))==7,"FREE affects only immediate previous MAGIC, not earlier one");
    for(const auto start:{"discard","sacrifice","sell","drop"})check(magic_cost(ids({start,"spring"}))==0,"trade/addItem cost rule ignores payload MAGIC cost");
    check(magic_cost(ids({"buy","spring"}))==7,"only the client's explicit cost-exclusion categories are skipped");
    // Helpers compute a visible list. Instance uniqueness, current MP and whole
    // action legality are separate contracts and are intentionally not asserted.
    check(attack_power(ids({"bronze-club","crossbow","crossbow"}))==5,"duplicate model IDs can represent distinct physical copies");
    std::vector<CardId> large(63,image("aura"));large[0]=image("bronze-club");
    check(attack_power(large)==(std::int64_t{1}<<62),"large ordered power remains exact int64");large.push_back(image("aura"));
    throws([&]{attack_power(large);},"DOUBLE beyond int64 rejects overflow");
    const auto maximum=std::numeric_limits<std::int64_t>::max();check(attack_power(ids({"bronze-club"}),maximum)==maximum,"maximum raw override accepted without arithmetic");
    throws([&]{attack_power(ids({"bronze-club","crossbow"}),maximum);},"additive power overflow rejects");
    throws([&]{attack_power(ids({"bronze-club"}),-1);},"negative override rejected");
    throws([&]{attack_power({});},"empty attack has no first model");
    for(CardId bad:{CardId{0},CardId{297},CardId{65535}}){const std::array<CardId,1> x{bad};throws([&]{attack_power(x);},"unknown composition model rejects");throws([&]{magic_cost(x);},"unknown cost model rejects");}
}

void merge_and_all_attributes() {
    std::array<Attribute,7> elements{Attribute{},Element::fire,Element::water,Element::wood,Element::stone,Element::light,Element::darkness};
    for(const auto a:elements)for(const auto b:elements){
        const auto merged=merge_elements(a,b);check(merged==merge_elements(b,a),"ordinary attribute combination symmetric");
        if(a==b)check(merged==a,"same attribute including none/dark remains unchanged");
        else if(!a||!b||a==Element::darkness||b==Element::darkness)check(!merged,"none/dark mixed with a different attribute neutralizes");
        else if(a==Element::light)check(merged==b,"Light substitutes other basic attribute");
        else if(b==Element::light)check(merged==a,"Light substitute is symmetric");
        else check(!merged,"different basic nonLight attributes neutralize");
    }
    const auto water=image("aqua-shoes"),fire=image("flame-boots"),light=image("shining-high-heels"),dark=image("pluto-ring");
    check(attribute_defense_allowed(water,Element::fire)&&!attribute_defense_allowed(fire,Element::fire)
        &&attribute_defense_allowed(light,Element::fire)&&attribute_defense_allowed(fire,Element::water),"opposite four elements and Light substitute defense");
    for(const auto& c:catalog())check(!attribute_defense_allowed(c.id,Element::light),"Light attack permits no attribute defense, including Dark counters");
    check(!defense_element(ids({"shining-high-heels","pluto-ring"}))&&!attribute_defense_allowed(dark,Element::light),
        "Dark+Light defense becomes neutral, and Dark alone still cannot revive legacy light-vsDark defense exception");
    check(attribute_defense_allowed(water,{})&&attribute_defense_allowed(water,Element::darkness),"neutral/Dark incoming attribute accepts any attribute");
}

void reaction_categories_and_visible_main_selection() {
    for(const auto a:std::array<Attribute,3>{Attribute{},Element::fire,Element::light}){
        check(reaction_for(image("super-mirror"),Category::guardians,a)==Reaction::reflect,"universal mirror ignores incoming category/attribute");
        check(reaction_for(image("moonlight-axe"),Category::miracles,a)==Reaction::reflect
            &&reaction_for(image("sky-boots"),Category::miracles,a)==Reaction::bounce
            &&reaction_for(image("angel-armor"),Category::miracles,a)==Reaction::block,"matching MAGIC stages accept colored magic");
        check(reaction_for(image("reflection-sword"),Category::weapons,a)==(a?Reaction::none:Reaction::reflect)
            &&reaction_for(image("bouncing-sword"),Category::weapons,a)==(a?Reaction::none:Reaction::bounce)
            &&reaction_for(image("wall"),Category::weapons,a)==(a?Reaction::none:Reaction::block),"WEAPON stages require current neutral attribute");
    }
    check(reaction_for(image("moonlight-axe"),Category::weapons,{})==Reaction::none
        &&reaction_for(image("reflection-sword"),Category::miracles,{})==Reaction::none
        &&reaction_for(image("wall"),Category::armor,{})==Reaction::none,"response namespaces never conflate weapon/magic/counter categories");
    check(can_start_main(image("bronze-club"))&&can_start_main(image("spring"))&&!can_start_main(image("aura"))&&!can_start_main(image("wall"))
        &&!can_start_main(image("sun-amulet"))&&!can_start_main(image("dangerous-mortar"))&&!can_start_main(image("strength-powder")),"visible starting-role exclusions match current client");
    check(main({"bronze-club"},"wand-of-ignition")&&main({"bronze-club"},"aura")&&!main({"ascension-bow"},"aura")
        &&!main({"spring"},"crossbow")&&main({"spring"},"spiritual-doll"),"guaranteed WEAPON accepts plus models; MAGIC accepts only immediate FREE suffix");
    for(const auto trade:{"discard","sacrifice"})check(!main({trade},"bronze-club")&&!main({trade},"sun-amulet")&&!main({trade},"dangerous-mortar")
        &&main({trade},"aqua-shoes")&&main({trade},"spring"),"current discard/sacrifice excludes weapon and both passive sundries");
    check(main({"sell"},"sun-amulet")&&main({"sell"},"spring")&&!main({"sell"},"spring",true)&&!main({"sell","spring"},"crossbow")
        &&!main({"sell"},"discard")&&!main({"sell"},"sacrifice"),"sell permits one physical goods model but never used miracle or action pseudo-models");
    check(targets_enemy_by_default(image("bronze-club"))&&!targets_enemy_by_default(image("ascension-bow"))
        &&targets_enemy_by_default(image("nocturnal-broom"))&&!targets_enemy_by_default(image("spring")),"target hint is a client model property, not a server outcome");
}

void response_append_order_and_flash() {
    ResponseContext magic;magic.incoming_category=Category::miracles;magic.incoming_element=Element::fire;
    check(response({},"sky-boots",magic)&&!response({"aqua-shoes"},"sky-boots",magic),"current matching stage must be first or follow filter, not arbitrary ordinary prefix");
    check(!response({},"flame-boots",magic)&&response({},"aqua-shoes",magic),"ordinary response requires opposite attribute");
    check(!response({"sky-boots"},"aqua-shoes",magic),"nothing ordinary follows an already selected reaction");
    ResponseContext light;light.incoming_element=Element::light;
    check(!response({},"shining-high-heels",light)&&!response({},"pluto-ring",light)&&response({},"rainbow-curtain",light),"Light normal defense blocked; filter may be first");
    check(response({"rainbow-curtain"},"rainbow-curtain",light)&&response({"rainbow-curtain","rainbow-curtain"},"reflection-sword",light),"leading filters compose and permit neutral WEAPON reaction");
    check(!response({"rainbow-curtain","aqua-shoes"},"reflection-sword",light)&&!response({"aqua-shoes"},"rainbow-curtain",light),"filter and special placement obey last-selected restriction");
    ResponseContext neutral;
    check(response({},"wall",neutral)&&response({"wall"},"spiritual-doll",neutral),"FREE may follow a selected MAGIC reaction");
    neutral.flash=true;check(response({},"wall",neutral)&&!response({"wall"},"spiritual-doll",neutral)&&!response({"rainbow-curtain"},"rainbow-curtain",neutral),"flash is checked before FREE exception and limits full selected list");
    magic.incoming_is_attack=false;check(!response({},"aqua-shoes",magic)&&response({},"sky-boots",magic),"nonattack MAGIC permits reaction but no ordinary defense");
    const auto visible=next("aqua-shoes");check(can_append_response({},visible,ResponseContext{false,Category::weapons,Element::fire,true}),"selection consumes only the supplied visible model, not hidden physical identity");
    throws([&]{reaction_for(0,Category::weapons,{});},"invalid reaction model rejected");
}
}
int main(){composition_and_cost();merge_and_all_attributes();reaction_categories_and_visible_main_selection();response_append_order_and_flash();std::cout<<"Official rules passed (client composition/selection kernel, not complete server transitions).\n";}
