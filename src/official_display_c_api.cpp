#include "godfield/official_display_c_api.h"
#include "godfield/official_display.hpp"
#include "official_state_bridge.hpp"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace official = godfield::official;
struct gfo_display {
    official::ActiveDisplay value;
    explicit gfo_display(official::ActiveDisplay display) : value(std::move(display)) {}
};
static_assert(std::is_standard_layout_v<gfo_display_event> && sizeof(gfo_display_event)==88
    && sizeof(gfo_state_active_attack)==72 && offsetof(gfo_display_event,attack)==0
    && offsetof(gfo_display_event,player_id)==72 && offsetof(gfo_display_event,kind)==80
    && offsetof(gfo_display_event,reserved)==81);
namespace {
template<std::size_t N> bool zero(const uint8_t (&bytes)[N]) {
    return std::all_of(std::begin(bytes),std::end(bytes),[](auto byte){return byte==0;});
}
void valid(bool condition) { if (!condition) throw std::invalid_argument("invalid received display ABI input"); }
official::ActiveAttackState attack(const gfo_state_active_attack& wire) {
    valid(zero(wire.reserved) && wire.guardian<=10 && wire.reversed<=1 && (wire.models || !wire.model_count));
    official::ActiveAttackState result;
    valid(wire.model_count<=result.models.max_size()
        && wire.model_count<=static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max())/sizeof(uint16_t));
    result.player_id=wire.player_id;result.target_player_id=wire.target_player_id;
    result.guardian=static_cast<official::Guardian>(wire.guardian);
    result.atk=wire.atk;result.hit_rate=wire.hit_rate;result.mp=wire.mp;result.cp=wire.cp;
    result.reversed=wire.reversed!=0;result.buying_model=wire.buying_model;
    result.models.reserve(wire.model_count);
    for(std::size_t i=0;i<wire.model_count;++i)result.models.push_back(wire.models[i]);
    official::validate_active_attack(result);return result;
}
official::ActiveDisplayEvent event(const gfo_display_event& wire) {
    valid(wire.kind<=2 && zero(wire.reserved));
    if (!wire.kind) { valid(!wire.player_id);return official::NextAttack{attack(wire.attack)}; }
    const gfo_state_active_attack inactive{};
    valid(std::memcmp(&wire.attack,&inactive,sizeof inactive)==0 && wire.player_id<=official::max_wire_integer);
    if(wire.kind==1)return official::SetTargetPlayer{wire.player_id};
    return official::AdvanceGF{wire.player_id};
}
}
extern "C" {
int gfo_display_api_version(void) { return 1; }
size_t gfo_display_event_size(void) { return sizeof(gfo_display_event); }
int gfo_display_create(const gfo_state* state,uint64_t turn_player,gfo_display** out) {
    if(!state || !out)return -1;
    try {
        const auto& head=official::bridge::active_attack(*state);
        const auto frames=head?std::span<const official::ActiveAttackState>{&*head,1}:std::span<const official::ActiveAttackState>{};
        auto owned=std::make_unique<gfo_display>(official::ActiveDisplay{official::bridge::snapshot(*state),frames,turn_player});
        *out=owned.release();return 0;
    } catch(const std::invalid_argument&) { return -2; } catch(...) { return -3; }
}
int gfo_display_apply(const gfo_display* display,const gfo_display_event* events,std::size_t count,gfo_display** out) {
    if(!display || !out || (!events && count))return -1;
    try {
        std::vector<official::ActiveDisplayEvent> parsed;
        valid(count<=parsed.max_size() && count<=static_cast<std::size_t>(std::numeric_limits<std::ptrdiff_t>::max())/sizeof(gfo_display_event));
        parsed.reserve(count);
        for(std::size_t i=0;i<count;++i)parsed.push_back(event(events[i]));
        auto next=display->value;next.apply(parsed);
        auto owned=std::make_unique<gfo_display>(std::move(next));*out=owned.release();return 0;
    } catch(const std::invalid_argument&) { return -2; } catch(...) { return -3; }
}
void gfo_display_destroy(gfo_display* display) { delete display; }
int gfo_display_view_json(const gfo_display* display,char* buffer,std::size_t capacity,std::size_t* required) {
    if(!display || !required || (!buffer && capacity))return -1;
    try {
        const auto json=official::active_display_json(display->value.view());const auto size=json.size()+1;
        if(!buffer && !capacity){*required=size;return 0;}
        if(capacity<size)return -4;
        std::memcpy(buffer,json.c_str(),size);*required=size;return 0;
    } catch(const std::invalid_argument&) { return -2; } catch(...) { return -3; }
}
}
