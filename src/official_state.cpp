#include "godfield/official_state.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

namespace godfield::official {
namespace {
void integer(std::int64_t v) {
    constexpr auto limit = static_cast<std::int64_t>(max_wire_integer);
    if (v < -limit || v > limit) throw std::invalid_argument("state integer outside JS safe range");
}
void utf8(std::string_view text) {
    for (std::size_t i = 0; i < text.size();) {
        const auto lead = static_cast<unsigned char>(text[i++]);
        if (lead < 0x80) continue;
        unsigned trailing = 0, codepoint = 0, minimum = 0;
        if (lead >= 0xc2 && lead <= 0xdf) { trailing = 1; codepoint = lead & 31; minimum = 0x80; }
        else if (lead >= 0xe0 && lead <= 0xef) { trailing = 2; codepoint = lead & 15; minimum = 0x800; }
        else if (lead >= 0xf0 && lead <= 0xf4) { trailing = 3; codepoint = lead & 7; minimum = 0x10000; }
        else throw std::invalid_argument("invalid snapshot UTF-8");
        if (text.size() - i < trailing) throw std::invalid_argument("short snapshot UTF-8");
        while (trailing--) {
            const auto byte = static_cast<unsigned char>(text[i++]);
            if ((byte & 0xc0) != 0x80) throw std::invalid_argument("invalid UTF-8 continuation");
            codepoint = (codepoint << 6) | (byte & 63);
        }
        if (codepoint < minimum || codepoint > 0x10ffff || (codepoint >= 0xd800 && codepoint <= 0xdfff))
            throw std::invalid_argument("invalid UTF-8 code point");
    }
}
bool disease(Curse c) noexcept { return c >= Curse::cold && c <= Curse::heaven; }
std::vector<Curse> displayed_curses(const std::vector<Curse>& input) {
    std::vector<Curse> out;
    std::array<bool, 9> seen{};
    for (auto c : input) {
        if (seen[static_cast<unsigned>(c)]) continue;
        seen[static_cast<unsigned>(c)] = true;
        if (disease(c)) std::erase_if(out, [](Curse v) { return disease(v); });
        if (std::find(out.begin(), out.end(), c) == out.end()) out.push_back(c);
    }
    std::sort(out.begin(), out.end());
    return out;
}
const PlayerState& player(const SnapshotState& state, PlayerId id) {
    const auto* p = find_player(state, id);
    if (!p) throw std::invalid_argument("unknown snapshot player");
    return *p;
}
const ItemInstance* item(const PlayerState& p, ItemToken token) {
    auto it = std::find_if(p.items.begin(), p.items.end(), [&](const auto& v) { return v.token == token; });
    return it == p.items.end() ? nullptr : &*it;
}
bool visible_selection(const PlayerState& p, std::span<const ItemToken> tokens, std::vector<VisibleSelection>& out) {
    std::unordered_set<ItemToken> seen;
    for (auto token : tokens) {
        const auto* instance = item(p, token);
        if (!instance || !seen.insert(token).second) return false;
        out.push_back({face_model(*instance), instance->used});
    }
    return true;
}
void quoted(std::string& out, std::string_view text) {
    utf8(text);
    static constexpr char hex[] = "0123456789abcdef";
    out += '"';
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') { out += '\\'; out += static_cast<char>(c); }
        else if (c < 32) { out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15]; }
        else out += static_cast<char>(c);
    }
    out += '"';
}
template<class T> void number(std::string& out, T v) {
    std::array<char, 32> buffer{};
    auto r = std::to_chars(buffer.data(), buffer.data() + buffer.size(), v);
    if (r.ec != std::errc{}) throw std::logic_error("JSON integer conversion");
    out.append(buffer.data(), r.ptr);
}
void boolean(std::string& out, bool value) { out += value ? "true" : "false"; }
}
void validate_active_attack(const ActiveAttackState& attack) {
    if (attack.player_id>max_wire_integer || attack.target_player_id>max_wire_integer
        || static_cast<unsigned>(attack.guardian)>10 || (attack.buying_model && !find_card(attack.buying_model)))
        throw std::invalid_argument("invalid received active attack identity/model");
    integer(attack.atk); integer(attack.hit_rate); integer(attack.mp); integer(attack.cp);
    for (auto id:attack.models) if (!find_card(id)) throw std::invalid_argument("invalid received active attack model");
}
std::optional<ActiveAttackView> project_active_attack(const SnapshotState& board, std::span<const ActiveAttackState> attacks) {
    if (attacks.empty()) return std::nullopt;
    const auto& frame=attacks.front(); validate_active_attack(frame);
    if (board.is_over) return std::nullopt;
    (void)player(board,frame.player_id);
    if (frame.target_player_id>0) (void)player(board,frame.target_player_id);
    ActiveAttackView view; view.source=frame.player_id; view.guardian=frame.guardian; view.models=frame.models;
    view.reversed=frame.reversed; view.summary_models=frame.models;
    view.cards.reserve(frame.models.size());
    for (auto id:frame.models) {
        const auto& card=*find_card(id); ActiveAttackCardView displayed; displayed.model=id;
        switch(card.ability) {
        case Ability::atk_by2x_mp: displayed.atk=frame.atk; displayed.from_mp=true; break;
        case Ability::attack_dyingly: displayed.atk=frame.atk>0?frame.atk:card.atk; displayed.hit_rate=frame.atk>0?frame.hit_rate:card.hit_rate; break;
        case Ability::counter_atk: case Ability::counter2x_atk: if(frame.atk!=0)displayed.atk=frame.atk; break;
        case Ability::counter_boost2x_mp: if(frame.mp!=0)displayed.mp=frame.mp; break;
        case Ability::counter_take_cp: if(frame.cp!=0)displayed.cp=frame.cp; break;
        case Ability::counter_curse: displayed.curse=card.curse; break;
        default: break;
        }
        view.cards.push_back(std::move(displayed));
    }
    view.prayer=frame.models.empty();
    if (frame.target_player_id>0) {
        view.target=frame.target_player_id;
        if (frame.buying_model) view.buying_model=frame.buying_model;
    }
    if (view.prayer) return view;
    if (view.summary_models.size()>=2 && find_card(view.summary_models.front())->ability==Ability::call_phenomenon)
        view.summary_models.erase(view.summary_models.begin());
    const auto& base=*find_card(view.summary_models.front());
    view.is_attack=base.atk>0 || base.category==Category::weapons || frame.atk>0;
    view.incoming_category=base.ability==Ability::category_weapons?Category::weapons:base.category;
    view.incoming_element=attack_element(view.summary_models);
    if (view.is_attack) {
        // The browser treats a nonpositive dynamic base as "no override".
        view.power=attack_power(view.summary_models,std::max<std::int64_t>(0,frame.atk));
        view.hit_rate=frame.hit_rate>0?frame.hit_rate:base.hit_rate;
    }
    return view;
}
std::string active_attack_json(const std::optional<ActiveAttackView>& view) {
    std::string out="{\"schema\":\"official-active-attack-view-v1\",\"attack\":";
    if (!view) return out+"null}";
    const auto& v=*view;
    const auto models=[&](const std::vector<CardId>& ids){out+='[';for(std::size_t i=0;i<ids.size();++i){if(i)out+=',';number(out,ids[i]);}out+=']';};
    const auto element=[&](Attribute value){if(value)quoted(out,element_name(*value));else out+="null";};
    out+="{\"source\":";number(out,v.source);out+=",\"guardian\":";
    if(v.guardian==Guardian::none)out+="null";else quoted(out,guardian_name(v.guardian));
    out+=",\"models\":";models(v.models);
    out+=",\"cards\":[";
    const auto optional_number=[&](std::optional<std::int64_t> value){if(value)number(out,*value);else out+="null";};
    for(std::size_t i=0;i<v.cards.size();++i){if(i)out+=',';const auto& c=v.cards[i];out+="{\"model\":";number(out,c.model);
        out+=",\"atk\":";optional_number(c.atk);out+=",\"from_mp\":";boolean(out,c.from_mp);
        out+=",\"hit_rate\":";optional_number(c.hit_rate);out+=",\"mp\":";optional_number(c.mp);out+=",\"cp\":";optional_number(c.cp);
        out+=",\"curse\":";if(c.curse)quoted(out,curse_name(*c.curse));else out+="null";out+='}';}
    out+="]";
    out+=",\"reversed\":";boolean(out,v.reversed);
    out+=",\"target\":";if(v.target)number(out,*v.target);else out+="null";
    out+=",\"buying_model\":";if(v.buying_model)number(out,*v.buying_model);else out+="null";
    out+=",\"summary\":{\"models\":";models(v.summary_models);
    out+=",\"prayer\":";boolean(out,v.prayer);out+=",\"is_attack\":";boolean(out,v.is_attack);
    out+=",\"element\":";element(v.is_attack?v.incoming_element:Attribute{});
    out+=",\"power\":";if(v.power)number(out,*v.power);else out+="null";
    out+=",\"hit_rate\":";if(v.hit_rate)number(out,*v.hit_rate);else out+="null";
    out+="},\"response\":";
    if(!v.incoming_category)out+="null";
    else {out+="{\"incoming_category\":";quoted(out,category_name(*v.incoming_category));out+=",\"incoming_element\":";element(v.incoming_element);
        out+=",\"incoming_is_attack\":";boolean(out,v.is_attack);out+='}';}
    out+="}}";return out;
}
void validate_state(const SnapshotState& state) {
    if (state.players.size() > player_capacity || state.update_count > max_wire_integer) throw std::invalid_argument("invalid snapshot size/version counter");
    integer(state.gf); if (state.tiebreak_gf) integer(*state.tiebreak_gf);
    std::unordered_set<PlayerId> ids;
    for (const auto& p : state.players) {
        if (p.id > max_wire_integer || !ids.insert(p.id).second || static_cast<unsigned>(p.team) > 4 || static_cast<unsigned>(p.guardian) > 10)
            throw std::invalid_argument("invalid player ID/team/guardian");
        utf8(p.name); if (p.alias) utf8(*p.alias); if (p.user_id) utf8(*p.user_id);
        integer(p.hp); integer(p.mp); integer(p.cp); integer(p.confusion_count);
        for (auto c : p.curses) if (c == Curse::none || static_cast<unsigned>(c) > 8) throw std::invalid_argument("unknown snapshot curse");
        std::unordered_set<ItemToken> tokens;
        for (const auto& instance : p.items) {
            if (instance.token > max_wire_integer || !tokens.insert(instance.token).second || !find_card(instance.model) ||
                (instance.fake_model && !find_card(*instance.fake_model))) throw std::invalid_argument("invalid snapshot item");
        }
    }
}
const PlayerState* find_player(const SnapshotState& state, PlayerId id) noexcept {
    auto p = std::find_if(state.players.begin(), state.players.end(), [&](const auto& value) { return value.id == id; });
    return p == state.players.end() ? nullptr : &*p;
}
std::optional<PlayerId> resolve_viewer(const SnapshotState& state, std::string_view user_id) {
    validate_state(state);
    for (const auto& p : state.players) if (p.user_id && *p.user_id == user_id) return p.id;
    return std::nullopt;
}
bool is_enemy(const PlayerState& a, const PlayerState& b) noexcept { return (a.team == Team::solo && a.id != b.id) || a.team != b.team; }
std::vector<PlayerId> alive_players(const SnapshotState& state) {
    validate_state(state); std::vector<PlayerId> out;
    for (const auto& p : state.players) if (!p.dead) out.push_back(p.id);
    return out;
}
std::vector<PlayerId> enemy_players(const SnapshotState& state, PlayerId viewer) {
    validate_state(state); const auto& own = player(state, viewer); std::vector<PlayerId> out;
    for (const auto& p : state.players) if (!p.dead && is_enemy(p, own)) out.push_back(p.id);
    return out;
}
std::vector<PlayerId> ending_display_candidates(const SnapshotState& state) {
    validate_state(state); std::vector<PlayerId> out;
    auto first = std::find_if(state.players.begin(), state.players.end(), [](const auto& p) { return !p.dead; });
    if (first == state.players.end()) return out;
    if (first->team == Team::solo) return {first->id};
    for (const auto& p : state.players) if (p.team == first->team) out.push_back(p.id);
    return out;
}
CardId face_model(const ItemInstance& instance) {
    auto id = instance.fake_model.value_or(instance.model);
    if (!find_card(id)) throw std::invalid_argument("invalid displayed model");
    return id;
}
SnapshotView project_view(const SnapshotState& state, std::optional<PlayerId> viewer) {
    validate_state(state);
    const PlayerState* own = viewer ? &player(state, *viewer) : nullptr;
    if (own && own->items.size() > visible_hand_capacity) throw std::invalid_argument("own inventory exceeds fresh renderer domain");
    const bool fog = own && !own->dead && !state.is_over && std::find(own->curses.begin(), own->curses.end(), Curse::fog) != own->curses.end();
    SnapshotView out; out.viewer = viewer; out.gf = state.gf; out.tiebreak_gf = state.tiebreak_gf;
    out.is_over = state.is_over;
    for (const auto& p : state.players) {
        PlayerView v; v.id = p.id; v.team = p.team; v.dead = p.dead;
        v.display_name = own && p.id != own->id && p.alias ? *p.alias : p.name;
        v.fogged = fog && !p.dead && is_enemy(p, *own);
        if (!v.fogged) v.status = StatusPane{p.hp, p.mp, p.cp, displayed_curses(p.curses), p.guardian, !p.dead && p.confusion_count > 0};
        for (const auto& instance : p.items) if (instance.used) v.used_models.push_back(face_model(instance));
        std::sort(v.used_models.begin(), v.used_models.end());
        out.players.push_back(std::move(v));
    }
    if (own) for (const auto& instance : own->items) {
        const auto face = face_model(instance);
        out.hand.push_back({instance.token, face, own->dead || state.is_over ? instance.model : face, instance.fake_model.has_value(), instance.used});
    }
    std::stable_sort(out.hand.begin(), out.hand.end(), [](const auto& a, const auto& b) {
        if (a.used != b.used) return !a.used;
        return a.face < b.face;
    });
    return out;
}
bool can_append_hand(const SnapshotState& state, PlayerId viewer, std::span<const ItemToken> selected, ItemToken next,
                     std::optional<ResponseContext> response) {
    validate_state(state); const auto& own = player(state, viewer);
    if (own.items.size() > visible_hand_capacity) throw std::invalid_argument("own inventory exceeds fresh renderer domain");
    if (response) response->flash = std::find(own.curses.begin(), own.curses.end(), Curse::flash) != own.curses.end();
    if (own.dead || state.is_over || std::find(selected.begin(), selected.end(), next) != selected.end()) return false;
    std::vector<VisibleSelection> visible;
    if (!visible_selection(own, selected, visible)) return false;
    const auto* added = item(own, next);
    if (!added) return false;
    VisibleSelection proposed{face_model(*added), added->used};
    return response ? can_append_response(visible, proposed, *response) : can_append_main(visible, proposed);
}
bool selection_ready(const SnapshotState& state, PlayerId viewer, std::span<const ItemToken> selected,
                     std::optional<ResponseContext> response) {
    validate_state(state); const auto& own = player(state, viewer);
    if (own.items.size() > visible_hand_capacity) throw std::invalid_argument("own inventory exceeds fresh renderer domain");
    if (response) response->flash = std::find(own.curses.begin(), own.curses.end(), Curse::flash) != own.curses.end();
    if (own.dead || state.is_over) return false;
    std::vector<VisibleSelection> visible;
    if (!visible_selection(own, selected, visible)) return false;
    std::vector<VisibleSelection> prefix;
    std::vector<CardId> ids;
    for (const auto& v : visible) {
        if (!(response ? can_append_response(prefix, v, *response) : can_append_main(prefix, v))) return false;
        prefix.push_back(v); ids.push_back(v.model);
    }
    if (own.mp < magic_cost(ids)) return false;
    if (response) return true;
    if (ids.empty()) return std::none_of(own.items.begin(), own.items.end(), [](const auto& v) { return find_card(face_model(v))->category == Category::weapons; });
    const auto ability = find_card(ids.front())->ability;
    if (ability == Ability::discard || ability == Ability::sacrifice) return ids.size() >= 2;
    if (ability == Ability::sell) return ids.size() == 2;
    return true;
}
std::string view_json(const SnapshotView& view) {
    std::string out = "{\"schema\":\"official-fresh-view-v1\",\"viewer\":";
    if (view.viewer) number(out, *view.viewer); else out += "null";
    out += ",\"gf\":"; number(out, view.gf); out += ",\"tiebreak_gf\":";
    if (view.tiebreak_gf) number(out, *view.tiebreak_gf); else out += "null";
    out += ",\"is_over\":"; boolean(out, view.is_over);
    out += ",\"players\":[";
    bool first = true;
    for (const auto& p : view.players) {
        if (!first) out += ','; first = false;
        out += "{\"id\":"; number(out, p.id); out += ",\"name\":"; quoted(out, p.display_name);
        out += ",\"team\":"; number(out, static_cast<unsigned>(p.team)); out += ",\"dead\":"; boolean(out, p.dead);
        out += ",\"fogged\":"; boolean(out, p.fogged); out += ",\"status\":";
        if (!p.status) out += "null";
        else {
            const auto& s = *p.status;
            out += "{\"hp\":"; number(out, s.hp); out += ",\"mp\":"; number(out, s.mp); out += ",\"cp\":"; number(out, s.cp);
            out += ",\"curses\":["; bool next = false;
            for (auto curse : s.curses) { if (next) out += ','; next = true; quoted(out, curse_name(curse)); }
            out += "],\"guardian\":"; quoted(out, guardian_name(s.guardian)); out += ",\"confused\":"; boolean(out, s.confused); out += '}';
        }
        out += ",\"used_models\":["; bool next = false;
        for (auto id : p.used_models) { if (next) out += ','; next = true; number(out, id); }
        out += "]}";
    }
    out += "],\"hand\":["; first = true;
    for (const auto& v : view.hand) {
        if (!first) out += ','; first = false;
        out += "{\"token\":"; number(out, v.token); out += ",\"face\":"; number(out, v.face); out += ",\"reference\":"; number(out, v.reference);
        out += ",\"disguised\":"; boolean(out, v.disguised); out += ",\"used\":"; boolean(out, v.used); out += '}';
    }
    out += "]}"; return out;
}
}
