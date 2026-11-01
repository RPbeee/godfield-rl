#include "godfield/official_catalog.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string_view>

namespace {
using namespace godfield::official;
void check(bool ok,const char* message) { if(!ok){std::cerr<<"FAIL official catalog: "<<message<<'\n';std::exit(1);} }
const Card& image(std::string_view name) { for(const auto& c:catalog())if(name==c.image_name)return c; check(false,"known visible image model exists");return catalog().front(); }
void all_records_and_distinct_namespaces() {
    check(catalog_count==296 && catalog().size()==296,"all current public item models, including nondraw display models");
    std::array<int,8> categories{},weights{}; std::set<std::string_view> images; int positive=0,total=0;
    for(std::size_t i=0;i<catalog().size();++i) {
        const auto& c=catalog()[i]; check(c.id==i+1 && card_index(c.id)==static_cast<int>(i) && find_card(c.id)==&c,"client model IDs are source index plus one");
        check(c.name && *c.name && c.image_name && *c.image_name && images.insert(c.image_name).second,"names/images present; stable image keys unique");
        const auto category=static_cast<unsigned>(c.category); check(category<8 && !category_name(c.category).empty(),"category roundtrip defined"); ++categories[category];
        check(c.atk>=0 && c.defense>=0 && c.price>=0 && c.cost>=0 && c.gift_rate>=0 && c.hit_rate>=0 && c.hit_rate<=100
            && c.appearance_rate>=0 && c.appearance_rate<=100 && c.guardian_attack_rate>=0 && c.guardian_attack_rate<=100,"all public numeric facts in declared ranges");
        if(c.element)check(!element_name(*c.element).empty(),"explicit element has source string mapping");
        if(c.ability!=Ability::none)check(!ability_name(c.ability).empty(),"all ability enum values retain original source spelling");
        if(c.curse!=Curse::none)check(!curse_name(c.curse).empty(),"curse enum retains source spelling");
        if(c.guardian!=Guardian::none)check(!guardian_name(c.guardian).empty(),"guardian enum retains source spelling");
        weights[category]+=c.gift_rate; total+=c.gift_rate; positive+=c.gift_rate>0;
    }
    check(categories==std::array<int,8>{5,107,78,19,30,5,42,10},"all eight categories counted independently");
    check(weights==std::array<int,8>{60,181,147,82,30,0,0,0} && total==500 && positive==237,
        "237 positive-gift models sum500; guardian/devil/phenomenon display records are not normal gifts");
    for(CardId id:{CardId{0},CardId{297},CardId{65535}})check(!find_card(id)&&card_index(id)==-1,"out-of-range and legacy-only IDs rejected");
    check(std::string_view(catalog_version)=="official-v2" && std::string_view(catalog_sha256).size()==64,"official facts schema/hash explicitly named");
    check(image("exchange").id==3 && image("exchange").price==5 && image("sell").id==4 && image("sell").price==5
        && image("buy").id==5 && image("buy").price==5,"official trade IDs/prices do not reuse legacy2/3/4 and price10");
    check(find_card(2)->ability==Ability::sacrifice && find_card(244)->category==Category::devils && image("sun-amulet").id==208,
        "valid but colliding legacy numeric IDs are current models, never silently translated");
}
void important_current_facts() {
    check(image("kusarigama").id==19 && image("naginata-classic").id==26 && image("stone-tablet").id==117
        && std::string_view(find_card(117)->name)=="石板", "latest official-v2 names and image keys are adopted without changing model IDs");
    for(const auto& c:catalog())check(std::string_view(c.image_name)!="chain-sickle"&&std::string_view(c.image_name)!="glaive-classic"&&std::string_view(c.image_name)!="slate", "archived image keys do not silently alias current model keys");

    const auto& spring=image("spring"); check(spring.id==235 && spring.category==Category::miracles && spring.ability==Ability::boost_hp
        && spring.ability_value==10 && spring.cost==7 && spring.guardian==Guardian::neptune && !spring.element,"current Spring10 costs7, not legacy216 cost6");
    const auto& ascension=image("ascension-bow"); check(ascension.id==112 && ascension.atk==1 && ascension.hit_rate==25
        && ascension.ability==Ability::attack_dyingly && ascension.ability_value==30 && ascension.element==Element::light,"current ordinary ascension bow is25percent, death metadata30 retained separately");
    const auto& fire=image("wand-of-ignition"); const auto& water=image("wand-of-mystic-water");
    check(fire.atk==2 && water.atk==5 && fire.is_plus_atk && water.is_plus_atk && fire.ability==Ability::set_element && water.ability==Ability::set_element
        && fire.element==Element::fire && water.element==Element::water,"current attribute setters are physical additive ATK, not legacy zero contribution");
    const auto& mortar=image("dangerous-mortar"); check(mortar.ability==Ability::attract_danger && mortar.ability_value==99 && mortar.price==1,"current mortar99 metadata differs from legacy999");
    check(image("nocturnal-broom").ability==Ability::remove_items && image("goddess-s-soap").ability==Ability::remove_used_miracles,
        "current removal model names/abilities distinct from legacy SUNDRY IDs");
    check(image("aura").cost==6 && image("aura").is_plus_atk && image("aura").ability==Ability::double_atk
        && image("mirage").cost==5 && image("mirage").is_plus_atk && image("mirage").ability==Ability::attack_every_enemy,"ordered modifier models retained as explicit abilities");
    check(ability_name(Ability::atk_by2x_mp)=="atkBy2xMP" && ability_name(Ability::remove_used_miracles)=="removeUsedMiracles"
        && curse_name(Curse::darkcloud)=="darkcloud" && element_name(Element::darkness)=="darkness", "normalization preserves camelCase and current names");
}
}
int main(){all_records_and_distinct_namespaces();important_current_facts();std::cout<<"Official catalog passed (296models/237gift/500weight/current IDs and facts).\n";}
