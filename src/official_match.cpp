#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <unordered_set>

namespace godfield::official {
namespace {
[[noreturn]] void invalid(const char* message) { throw MatchError(MatchErrorCode::invalid, message); }
[[noreturn]] void unsupported(const char* message) { throw MatchError(MatchErrorCode::unsupported, message); }
void require(bool value, const char* message) { if (!value) invalid(message); }
void config_valid(const MatchConfig& c) {
    require(c.max_decisions && c.hand_limit && c.hand_limit <= visible_hand_capacity && c.initial_hand <= c.hand_limit,
            "invalid scoped match limits");
    require(c.initial_hp >= 1 && c.initial_hp <= 99 && c.initial_mp >= 0 && c.initial_mp <= 99 && c.initial_cp >= 0 && c.initial_cp <= 99,
            "invalid scoped initial resources");
}
std::uint64_t random(std::uint64_t& state) {
    auto z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}
std::uint64_t bounded(std::uint64_t& rng, std::uint64_t n) {
    require(n != 0, "empty random selection");
    const auto threshold = (std::uint64_t{0} - n) % n;
    std::uint64_t value;
    do { value = random(rng); } while (value < threshold);
    return value % n;
}
std::size_t index(const MatchSnapshot& s, PlayerId id) {
    for (std::size_t i = 0; i < s.state.players.size(); ++i) if (s.state.players[i].id == id) return i;
    invalid("unknown match player");
}
const ItemInstance* instance(const PlayerState& p, ItemToken token) {
    const auto it = std::find_if(p.items.begin(), p.items.end(), [&](const auto& item) { return item.token == token; });
    return it == p.items.end() ? nullptr : &*it;
}
bool has_curse(const PlayerState& p, Curse curse) {
    return std::find(p.curses.begin(), p.curses.end(), curse) != p.curses.end();
}
bool disease(Curse curse) { return curse >= Curse::cold && curse <= Curse::heaven; }
bool supported_curse(Curse curse) { return disease(curse) || curse == Curse::fog || curse == Curse::dream || curse == Curse::flash || curse == Curse::darkcloud; }
void add_curse(PlayerState& p, Curse curse) {
    require(supported_curse(curse), "unsupported effect curse");
    if (disease(curse)) {
        const auto current = std::find_if(p.curses.begin(), p.curses.end(), disease);
        if (current != p.curses.end()) {
            if (*current == Curse::heaven) { p.hp = 0; return; }
            curse = std::max(curse, static_cast<Curse>(static_cast<int>(*current) + 1));
            p.curses.erase(current);
        }
    }
    if (!has_curse(p, curse)) p.curses.insert(std::lower_bound(p.curses.begin(), p.curses.end(), curse), curse);
}
void change_guardian(MatchSnapshot& s, PlayerState& player, Guardian after, GuardianChangeReason reason) {
    GuardianChange event{player.id,player.guardian,after,reason,0};
    for (std::size_t seat=0; seat<s.state.players.size(); ++seat) {
        const auto& viewer=s.state.players[seat];
        if (!viewer.dead && !s.state.is_over && !player.dead && has_curse(viewer,Curse::fog) && is_enemy(player,viewer))
            event.hidden_viewers |= static_cast<std::uint16_t>(1u<<seat);
    }
    player.guardian=after; s.last_guardian_changes.push_back(event);
}
void guardian_damage(MatchSnapshot& s, PlayerState& player, std::int64_t loss) {
    if (loss<=0 || player.guardian==Guardian::none) return;
    ++s.guardian_departure_rolls;
    if (bounded(s.rng,100)<10) { ++s.guardian_departures; change_guardian(s,player,Guardian::none,GuardianChangeReason::departure); }
}

void closed_state(const SnapshotState& s, const MatchConfig& c) {
    validate_state(s);
    require(s.players.size() >= 2, "scoped match needs 2..9 players");
    require(s.gf >= 0, "negative GF outside scoped runtime");
    if (s.tiebreak_gf) unsupported("tiebreak is not implemented");
    for (const auto& p : s.players) {
        require(p.hp >= 0 && p.hp <= 99 && p.mp >= 0 && p.mp <= 99 && p.cp >= 0 && p.cp <= 99, "scene resources outside 0..99");
        require(p.dead || p.hp > 0, "HP0 without resolved death is not a quiescent scene");
        require(p.items.size() <= c.hand_limit, "scene inventory exceeds configured display limit");
        if (std::any_of(p.curses.begin(), p.curses.end(), [](auto curse) { return !supported_curse(curse); })
            || p.confusion_count)
            unsupported("only diseases, Fog, Dream, Flash and DarkCloud are implemented among scoped curses and confusion");
        require(p.curses.size() <= 5 && std::count_if(p.curses.begin(), p.curses.end(), disease) <= 1 && std::is_sorted(p.curses.begin(), p.curses.end())
            && std::adjacent_find(p.curses.begin(), p.curses.end()) == p.curses.end(), "noncanonical scoped curses");
        for (const auto& item : p.items) {
            if (!match_supported(item.model)) unsupported("unsupported match model");
            require(!item.fake_model || (has_curse(p, Curse::dream) && !item.used
                && match_dream_compatible(item.model, *item.fake_model)), "invalid compatible Dream disguise");
            require(!item.used || find_card(item.model)->category == Category::miracles, "only miracles can be retained in scoped matches");
        }
    }
}
struct Outcome { bool terminal = false; std::vector<PlayerId> winners; };
Outcome outcome(const SnapshotState& state) {
    std::vector<const PlayerState*> alive;
    for (const auto& p : state.players) if (!p.dead) alive.push_back(&p);
    if (alive.empty()) return {true, {}};
    const auto* first = alive.front();
    if (first->team == Team::solo) {
        if (alive.size() == 1) return {true, {first->id}};
        return {};
    }
    if (std::any_of(alive.begin(), alive.end(), [&](auto p) { return p->team != first->team; })) return {};
    Outcome result{true, {}};
    for (const auto& p : state.players) if (p.team == first->team) result.winners.push_back(p.id);
    return result;
}
void finish(MatchSnapshot& s, const Outcome& result) {
    s.terminated = true; s.truncated = false; s.state.is_over = true;
    s.phase = MatchPhase::finished; s.actor.reset(); s.queue.clear(); s.batch.reset(); s.counters.clear(); s.deaths.clear(); s.purchase.reset();
    std::fill(s.gifts.begin(), s.gifts.end(), 0);
    s.winners = result.winners;
    s.rewards.assign(s.state.players.size(), 0);
    if (!s.winners.empty()) for (std::size_t i = 0; i < s.rewards.size(); ++i)
        s.rewards[i] = std::find(s.winners.begin(), s.winners.end(), s.state.players[i].id) != s.winners.end() ? 1. : -1.;
}
const std::array<CardId, 499>& gift_tickets() {
    static const auto pool = [] {
        std::array<CardId, 499> values{};
        std::size_t n = 0, models = 0;
        for (const auto& c : catalog()) if (match_supported(c.id)) {
            ++models;
            for (int i = 0; i < c.gift_rate; ++i) {
                if (n >= values.size()) throw std::logic_error("scoped pool weight changed");
                values[n++] = c.id;
            }
        }
        if (n != values.size() || models != 236) throw std::logic_error("scoped catalog changed");
        return values;
    }();
    return pool;
}
const std::vector<CardId>& dream_faces(CardId actual) {
    static const auto choices = [] {
        std::array<std::vector<CardId>, catalog_count + 1> result;
        for (const auto& a : catalog()) for (const auto& b : catalog())
            if (match_dream_compatible(a.id, b.id)) result[a.id].push_back(b.id);
        return result;
    }();
    return choices[actual];
}
void reveal_item(MatchSnapshot& s, PlayerId owner, ItemInstance& item, bool owner_only = false) {
    if (!item.fake_model) return;
    s.last_reveals.push_back({owner,*item.fake_model,item.model,owner_only});
    item.fake_model.reset();
}
void gift(MatchSnapshot& s, std::size_t seat) {
    auto& p = s.state.players[seat];
    require(p.items.size() < s.config.hand_limit, "gift has no reserved display slot");
    require(s.next_token <= max_wire_integer, "scoped item token exhausted");
    const auto id = gift_tickets()[bounded(s.rng, gift_tickets().size())];
    std::optional<CardId> face;
    if (has_curse(p, Curse::dream)) {
        const auto& choices = dream_faces(id);
        if (!choices.empty()) {
            ++s.dream_rolls;
            if (bounded(s.rng, 2) == 0) { face = choices[bounded(s.rng, choices.size())]; ++s.dream_masks; }
        }
    }
    p.items.push_back({s.next_token++, id, face, false});
}
void redraw(MatchSnapshot& s, std::size_t seat) {
    auto& owner = s.state.players[seat];
    const auto count = owner.items.size();
    // Replacement is private: clearing discarded masks must not publish their true IDs.
    owner.items.clear();
    ++s.redraws; s.redrawn_items += count; s.last_redraws.push_back(owner.id);
    for (std::size_t i = 0; i < count; ++i) gift(s, seat);
}

void request_gift(MatchSnapshot& s, std::size_t seat) {
    if (s.state.players[seat].items.size() + s.gifts[seat] < s.config.hand_limit) ++s.gifts[seat];
}
void consume(MatchSnapshot& s, std::size_t seat, const std::vector<ItemToken>& tokens, bool discard) {
    auto& p = s.state.players[seat];
    for (auto token : tokens) {
        auto it = std::find_if(p.items.begin(), p.items.end(), [&](const auto& value) { return value.token == token; });
        require(it != p.items.end(), "selected item vanished");
        reveal_item(s, p.id, *it);
        if (!discard && find_card(it->model)->category == Category::miracles) it->used = true;
        else p.items.erase(it);
        if (!discard) request_gift(s, seat);
    }
}
bool damaging(const EffectFrame& effect) { return effect.power > 0 || effect.category == Category::weapons; }
bool discardable(const Card& card) {
    return card.category != Category::weapons && card.ability != Ability::revive && card.ability != Ability::attract_danger;
}
bool has_wide(std::span<const CardId> models) {
    return std::any_of(models.begin(), models.end(), [](auto model) { return find_card(model)->ability == Ability::attack_every_enemy; });
}
std::vector<PlayerId> enemy_targets(const MatchSnapshot& s) {
    const auto& owner = s.state.players[index(s, s.owner)];
    std::vector<PlayerId> targets;
    for (const auto& player : s.state.players) if (!player.dead && player.id != owner.id
        && (owner.team == Team::solo || player.team != owner.team)) targets.push_back(player.id);
    return targets;
}
ResponseContext response_context(const MatchSnapshot& s) {
    require(s.queue.size() == 1, "response has no active item");
    const auto& effect = s.queue.front();
    return {has_curse(s.state.players[index(s, effect.target)], Curse::flash), effect.category, effect.element, damaging(effect)};
}
std::vector<CardId> prepare(const MatchSnapshot& s, const Command& command) {
    require(s.phase != MatchPhase::finished && s.actor && *s.actor == command.actor, "wrong actor or finished match");
    if (static_cast<unsigned>(command.kind) > 8) unsupported("unsupported match command kind");
    require(command.kind == CommandKind::exchange || !command.allocation, "only exchange accepts a resource allocation");
    const auto& own = s.state.players[index(s, command.actor)];
    require(!own.dead, "dead actor");
    require(command.items.size() <= s.config.hand_limit, "too many selected items");
    std::unordered_set<ItemToken> selected;
    std::vector<CardId> models, actual_models;
    for (auto token : command.items) {
        const auto* item = instance(own, token);
        require(item && selected.insert(token).second, "item missing or selected twice");
        models.push_back(face_model(*item));
        actual_models.push_back(item->model);
    }
    if (s.phase == MatchPhase::purchase) {
        require(s.purchase && (command.kind == CommandKind::accept || command.kind == CommandKind::decline)
            && command.items.empty() && !command.target, "purchase needs a targetless accept or decline");
        if (command.kind == CommandKind::accept) {
            require(own.cp >= find_card(s.purchase->item.model)->price, "insufficient purchase CP");
        }
        return actual_models;
    }
    require(command.kind != CommandKind::accept && command.kind != CommandKind::decline, "purchase decision outside purchase phase");
    if (s.phase == MatchPhase::response) {
        require(command.kind == CommandKind::defend && !command.target, "response needs targetless defend");
        auto context = response_context(s);
        std::vector<VisibleSelection> prefix;
        for (auto id : models) {
            require(can_append_response(prefix, {id, false}, context), "unsupported defense composition");
            prefix.push_back({id, false});
        }
        require(magic_cost(models) <= own.mp, "insufficient defense MP");
        return actual_models;
    }
    require(command.kind != CommandKind::defend, "defend outside response phase");
    if (command.kind == CommandKind::pray) {
        require(command.items.empty() && !command.target, "pray takes no items or target");
        require(std::none_of(own.items.begin(), own.items.end(), [](const auto& item) { return find_card(face_model(item))->category == Category::weapons; }), "visible weapon prevents prayer");
        return actual_models;
    }
    if (command.kind == CommandKind::discard) {
        require(!models.empty() && !command.target, "discard needs items and no target");
        for (auto id : models) require(discardable(*find_card(id)), "cannot discard weapons or passive artifacts");
        return actual_models;
    }
    if (command.kind == CommandKind::exchange) {
        require(!command.target && command.allocation && models.size() == 1 && find_card(models.front())->ability == Ability::exchange,
            "exchange requires one owned exchange card, allocation and no target");
        const auto& a = *command.allocation;
        require(a.hp >= 0 && a.hp <= 99 && a.mp >= 0 && a.mp <= 99 && a.cp >= 0 && a.cp <= 99, "exchange allocation outside0..99");
        require(std::int64_t{a.hp} + a.mp + a.cp == own.hp + own.mp + own.cp, "exchange must conserve the current resource sum");
        return actual_models;
    }
    if (command.kind == CommandKind::sell) {
        require(models.size() == 2 && models.front() == 4 && command.target, "sell needs one operation, one commodity and a target");
        const auto* commodity = instance(own, command.items[1]);
        require(!commodity->used && can_append_main(std::array<VisibleSelection,1>{{{4,false}}}, {face_model(*commodity),false}), "used miracles cannot be sold");
        const auto& buyer = s.state.players[index(s, *command.target)];
        require(!buyer.dead, "sale target is dead");
        return actual_models;
    }
    if (command.kind == CommandKind::buy) {
        require(models.size() == 1 && models.front() == 5 && command.target, "buy needs one operation and explicit target");
        const auto& seller = s.state.players[index(s, *command.target)];
        require(!seller.dead, "purchase target is dead");
        return actual_models;
    }
    require(!models.empty(), "play needs items");
    require(find_card(models.front())->ability != Ability::exchange && find_card(models.front())->ability != Ability::sell && find_card(models.front())->ability != Ability::buy, "trade needs its explicit command");
    if (find_card(models.front())->ability == Ability::danger) {
        if (has_wide(models)) unsupported("danger with Wide is outside the scoped runtime");
        if (command.target) require(!s.state.players[index(s, *command.target)].dead, "target is dead");
    } else if (find_card(models.front())->hit_rate) {
        require(!command.target && !enemy_targets(s).empty(), "chance play needs no target and at least one alive enemy");
    } else if (has_wide(models)) {
        require(!enemy_targets(s).empty(), "wide play needs at least one alive enemy");
        if (command.target) require(!s.state.players[index(s, *command.target)].dead, "target is dead");
    } else {
        require(command.target.has_value(), "play needs explicit target");
        require(!s.state.players[index(s, *command.target)].dead, "target is dead");
    }
    std::vector<VisibleSelection> prefix;
    for (auto id : models) {
        require(can_append_main(prefix, {id, false}), "invalid main composition");
        prefix.push_back({id, false});
    }
    require(magic_cost(models) <= own.mp, "insufficient main MP");
    return actual_models;
}
EffectFrame effect_for(const MatchSnapshot& s, const std::vector<CardId>& models, PlayerId target,
                      const std::vector<ItemToken>& tokens, std::int64_t launch_mp, int random_hp_delta, PlayerId source, Guardian guardian = Guardian::none, bool charge_magic = true) {
    const auto& base = *find_card(models.front());
    const auto amount = base.ability == Ability::sell ? find_card(models[1])->price : base.ability == Ability::remove_items ? 3
        : base.ability == Ability::remove_used_miracles ? 2
        : base.ability == Ability::boost_hp_or_deal_damage ? random_hp_delta : base.ability_value;
    const auto power = base.ability == Ability::sell ? 0 : base.ability == Ability::atk_by2x_mp
        ? attack_power(models, 2 * (launch_mp - (charge_magic ? magic_cost(models) : 0))) : attack_power(models);
    return {source, source, target, models, tokens, base.category, base.ability == Ability::sell ? Attribute{} : attack_element(models), power, base.ability, amount, target, s.rng, {}, base.hit_rate, false, false, false, guardian};
}
bool payload_domain(Guardian guardian, const Card& card) {
    return guardian==Guardian::earth ? (card.category==Category::trade || card.category==Category::weapons || card.category==Category::armor || card.category==Category::sundries)
        : guardian==Guardian::moon && card.category==Category::miracles;
}
void payload_models(const GuardianPayloadAction& action) {
    require(action.guardian==Guardian::earth || action.guardian==Guardian::moon,"payload requires Earth or Moon");
    require(!action.models.empty() && action.models.size()<=visible_hand_capacity,"invalid payload column size");
    std::vector<VisibleSelection> prefix;
    for (const auto model:action.models) {
        if (!match_supported(model) || !payload_domain(action.guardian,*find_card(model))) unsupported("payload model outside guardian category domain");
        const auto ability=find_card(model)->ability;
        if (ability==Ability::exchange || ability==Ability::sell || ability==Ability::buy || ability==Ability::danger)
            unsupported("trade and danger guardian payloads are not implemented");
        require(can_append_main(prefix,{model,false}),"invalid ordinary guardian composition");
        prefix.push_back({model,false});
    }
    require(action.target.has_value(),"ordinary guardian payload requires explicit target");
}
void prepare_guardian_payload(const MatchSnapshot& s, const GuardianPayloadAction& action) {
    require(s.phase==MatchPhase::main && !s.terminated && !s.truncated,"guardian payload needs a main boundary");
    payload_models(action);
    const auto& source=s.state.players[index(s,action.source)];
    require(!source.dead && source.guardian==action.guardian,"payload source does not own matching guardian");
    require(!s.state.players[index(s,*action.target)].dead,"guardian payload target is dead");
}
EffectFrame payload_effect_for(const MatchSnapshot& s, const GuardianPayloadAction& action, PlayerId target,
                              std::int64_t launch_mp, int random_hp_delta) {
    return effect_for(s,action.models,target,{},launch_mp,random_hp_delta,action.source,action.guardian,false);
}
void prepare_guardian(const MatchSnapshot& s, const GuardianAction& action) {
    require(s.phase == MatchPhase::main && !s.terminated && !s.truncated, "guardian activation needs a main boundary");
    if (!match_guardian_supported(action.model)) unsupported("unsupported canonical guardian action");
    const auto& card = *find_card(action.model);
    const auto& source = s.state.players[index(s, action.source)];
    require(!source.dead && source.guardian == card.guardian, "guardian action source does not own matching guardian");
    if (card.ability == Ability::boost_cp_of_everybody) require(!action.target, "global guardian action takes no target");
    else {
        require(action.target.has_value(), "guardian action requires explicit target");
        require(!s.state.players[index(s, *action.target)].dead, "guardian action target is dead");
    }
}
EffectFrame guardian_effect_for(const GuardianAction& action, PlayerId target, std::uint64_t rng) {
    const auto& card = *find_card(action.model);
    EffectFrame effect;
    effect.origin = effect.source = action.source; effect.target = effect.initial_target = target;
    effect.models = {action.model}; effect.category = card.ability == Ability::category_weapons ? Category::weapons : Category::guardians;
    effect.element = card.element; effect.power = card.atk; effect.ability = card.ability;
    effect.amount = card.ability_value; effect.hit_rate = card.hit_rate; effect.redirect_rng = rng; effect.guardian = card.guardian;
    return effect;
}

PlayerId sample_bounce_target(const MatchSnapshot& s, std::uint64_t& rng) {
    std::array<PlayerId, player_capacity> alive;
    std::size_t count = 0;
    for (const auto& player : s.state.players) if (!player.dead) alive[count++] = player.id;
    return alive[bounded(rng, count)];
}
struct ResponseResult { std::int64_t defense = 0; Reaction reaction = Reaction::none; };
// Composition is validated separately by the public-client append predicate.
// A filter belongs to the incoming payload; a reflector never contributes ATK/DEF.
ResponseResult apply_response(EffectFrame& effect, std::span<const CardId> models) {
    ResponseResult result;
    for (auto id : models) {
        const auto& c = *find_card(id);
        if (c.ability == Ability::filter_atk_element) effect.element.reset();
        const auto reaction = reaction_for(id, effect.category, effect.element);
        if (reaction == Reaction::reflect) {
            std::swap(effect.source, effect.target);
            result.reaction = reaction;
        } else if (reaction == Reaction::bounce) {
            effect.source = effect.target; // Next target is sampled by the caller.
            result.reaction = reaction;
        } else if (reaction == Reaction::block) {
            result.reaction = reaction;
        } else if (reaction != Reaction::none) {
            unsupported("reaction has no scoped resolver");
        } else result.defense += c.defense;
    }
    return result;
}
void remove_items(MatchSnapshot& s, PlayerState& target, const EffectFrame& effect) {
    RemovalEvent event{effect.origin, effect.source, effect.target, effect.models.front(), {}};
    const auto ability = effect.ability;
    const auto count = effect.amount;
    for (int removed = 0; removed < count; ++removed) {
        std::array<std::size_t, visible_hand_capacity> candidates;
        std::size_t size = 0;
        for (std::size_t i = 0; i < target.items.size(); ++i) {
            const auto& item = target.items[i];
            const auto eligible = ability == Ability::remove_items ? !item.used
                : item.used && find_card(item.model)->category == Category::miracles;
            if (eligible) candidates[size++] = i;
        }
        if (!size) break;
        const auto selected = candidates[bounded(s.rng, size)];
        // Stable inventory order and physical-copy identity are part of the local RNG contract.
        reveal_item(s, target.id, target.items[selected]);
        event.removed_models.push_back(target.items[selected].model);
        target.items.erase(target.items.begin() + static_cast<std::ptrdiff_t>(selected));
    }
    s.last_removal = std::move(event);
}
void finalize_deaths(MatchSnapshot& s) {
    std::vector<std::size_t> died;
    auto alive = std::count_if(s.state.players.begin(), s.state.players.end(), [](const auto& player) { return !player.dead; });
    for (std::size_t seat = 0; seat < s.state.players.size(); ++seat) {
        auto& player = s.state.players[seat];
        if (player.dead || player.hp) continue;
        const auto amulet = std::find_if(player.items.begin(), player.items.end(), [](const auto& item) {
            return find_card(item.model)->ability == Ability::revive;
        });
        if (alive >= 2 && amulet != player.items.end()) {
            const auto amount = find_card(amulet->model)->ability_value;
            player.items.erase(amulet);
            change_resource(player, Resource::hp, amount);
            request_gift(s, seat);
            ++s.revivals;
        } else { player.dead = true; --alive; died.push_back(seat); }
    }
    // The public client renders a server-provided posthumous attack. Its gate,
    // copy count and scheduling here are the explicit local death_rule.
    if (outcome(s.state).terminal) return;
    for (auto seat : died) {
        auto& source = s.state.players[seat];
        for (auto item = source.items.begin(); item != source.items.end();) {
            if (find_card(item->model)->ability != Ability::attack_dyingly) { ++item; continue; }
            const auto token = item->token;
            item = source.items.erase(item); // No replacement for a dead user.
            ++s.death_bows;
            for (const auto& target : s.state.players) if (!target.dead && is_enemy(source, target)) {
                EffectFrame effect;
                effect.origin = effect.source = source.id; effect.target = effect.initial_target = target.id;
                effect.models = {112}; effect.tokens = {token}; effect.category = Category::weapons;
                effect.element = Element::light; effect.power = 30; effect.ability = Ability::attack_dyingly;
                effect.hit_rate = 75; effect.is_death = true;
                s.deaths.push_back(std::move(effect)); ++s.death_created;
            }
        }
    }
}
const char* purchase_reason_name(PurchaseReason reason) {
    switch (reason) {
    case PurchaseReason::bought: return "bought";
    case PurchaseReason::declined: return "declined";
    case PurchaseReason::empty: return "empty";
    case PurchaseReason::insufficient_cp: return "insufficient-cp";
    case PurchaseReason::cutoff: return "cutoff";
    case PurchaseReason::self_inspection: return "self_inspection";
    }
    invalid("unknown purchase result");
}
// The client removes a server-provided overflow item before delivery. The
// uniform choice over every existing item is the local overflow_rule.
std::optional<CardId> deliver_item(MatchSnapshot& s, PlayerId recipient, CardId model) {
    auto& target = s.state.players[index(s, recipient)];
    require(s.next_token <= max_wire_integer, "scoped item token exhausted");
    std::optional<CardId> overflow;
    if (target.items.size() == s.config.hand_limit) {
        const auto slot = bounded(s.rng, target.items.size());
        reveal_item(s, target.id, target.items[slot]);
        overflow = target.items[slot].model;
        target.items.erase(target.items.begin() + static_cast<std::ptrdiff_t>(slot));
    }
    target.items.push_back({s.next_token++,model,{},false});
    const auto seat = index(s, recipient);
    s.gifts[seat] = std::min(s.gifts[seat], static_cast<std::uint32_t>(s.config.hand_limit - target.items.size()));
    return overflow;
}
void complete_purchase(MatchSnapshot& s, PurchaseReason reason) {
    const auto offer = *s.purchase;
    const auto price = find_card(offer.item.model)->price;
    auto& buyer = s.state.players[index(s, offer.buyer)];
    auto& seller = s.state.players[index(s, offer.seller)];
    std::optional<CardId> overflow;
    if (reason == PurchaseReason::bought) {
        require(buyer.cp >= price, "insufficient purchase CP");
        require(s.next_token <= max_wire_integer, "scoped item token exhausted");
        change_resource(buyer, Resource::cp, -price);
        change_resource(seller, Resource::cp, price);
        overflow = deliver_item(s, buyer.id, offer.item.model);
    } else {
        require(seller.items.size() < s.config.hand_limit && offer.slot <= seller.items.size(), "purchase return has no reserved slot");
        seller.items.insert(seller.items.begin() + static_cast<std::ptrdiff_t>(offer.slot), offer.item);
    }
    s.last_purchase = PurchaseEvent{offer.origin,offer.buyer,offer.seller,offer.item.model,price,reason,overflow};
    s.purchase.reset();
    finalize_deaths(s);
}
void begin_purchase(MatchSnapshot& s, const EffectFrame& effect) {
    auto& seller = s.state.players[index(s, effect.target)];
    std::array<std::size_t, visible_hand_capacity> candidates;
    std::size_t count = 0;
    for (std::size_t i = 0; i < seller.items.size(); ++i) if (!seller.items[i].used) candidates[count++] = i;
    if (!count) { s.last_purchase = PurchaseEvent{effect.origin,effect.source,effect.target,0,0,PurchaseReason::empty}; return; }
    const auto slot = candidates[bounded(s.rng, count)];
    reveal_item(s, seller.id, seller.items[slot]);
    if (effect.source == effect.target) {
        // A self-directed BUY inspects one existing copy; it never enters escrow.
        const auto model = seller.items[slot].model;
        s.last_purchase = PurchaseEvent{effect.origin,effect.source,effect.target,model,find_card(model)->price,PurchaseReason::self_inspection};
        return;
    }
    s.purchase = PurchaseOffer{effect.origin,effect.source,effect.target,seller.items[slot],slot};
    seller.items.erase(seller.items.begin() + static_cast<std::ptrdiff_t>(slot));
    const auto& buyer = s.state.players[index(s, effect.source)];
    if (buyer.cp < find_card(s.purchase->item.model)->price) complete_purchase(s, PurchaseReason::insufficient_cp);
    else { s.phase = MatchPhase::purchase; s.actor = effect.source; }
}
// Counter generation uses the target damage stage before passive revival,
// rather than net HP across source healing/self-damage and revival.
std::int64_t resolve(MatchSnapshot& s, const EffectFrame& effect, std::int64_t defense) {
    auto& target = s.state.players[index(s, effect.target)];
    std::int64_t target_damage = 0;
    if (damaging(effect)) {
        const auto remaining = std::max<std::int64_t>(0, effect.power - defense);
        if (remaining && effect.element != Element::darkness) {
            if (effect.ability == Ability::absorb_hp)
                change_resource(s.state.players[index(s, effect.source)], Resource::hp, remaining);
            else if (effect.ability == Ability::deal_same_damage) {
                auto& source=s.state.players[index(s,effect.source)]; const auto hp=source.hp;
                change_resource(source,Resource::hp,-remaining); guardian_damage(s,source,hp-source.hp);
            }
        }
        const auto hp = target.hp;
        if (remaining && effect.element == Element::darkness) target.hp = 0;
        else change_resource(target, Resource::hp, -remaining);
        target_damage = hp - target.hp;
        if (target_damage > 0 && effect.ability == Ability::add_curse_on_damage) add_curse(target, find_card(effect.models.front())->curse);
        guardian_damage(s,target,target_damage);
    } else {
        switch (effect.ability) {
        case Ability::boost_hp: change_resource(target, Resource::hp, effect.amount); break;
        case Ability::boost_hp_or_deal_damage: { const auto hp=target.hp; change_resource(target, Resource::hp, effect.amount); guardian_damage(s,target,hp-target.hp); break; }
        case Ability::boost_mp: change_resource(target, Resource::mp, effect.amount); break;
        case Ability::boost_mp_and_add_curse:
            change_resource(target, Resource::mp, effect.amount);
            add_curse(target, find_card(effect.models.front())->curse);
            break;
        case Ability::boost_cp: case Ability::boost_cp_to_enemy: case Ability::boost_cp_of_everybody:
            change_resource(target, Resource::cp, effect.amount); break;
        case Ability::add_item: request_gift(s, index(s, target.id)); break;
        case Ability::set_guardian:
            ++s.guardian_summons;
            change_guardian(s,target,static_cast<Guardian>(1+bounded(s.rng,10)),GuardianChangeReason::summon);
            break;
        case Ability::take_cp:
            change_resource(target, Resource::cp, -effect.amount);
            change_resource(s.state.players[index(s, effect.source)], Resource::cp, effect.amount);
            break;
        case Ability::counter_boost2x_mp: change_resource(target, Resource::mp, effect.amount); break;
        case Ability::counter_take_cp:
            change_resource(target, Resource::cp, -effect.amount);
            change_resource(s.state.players[index(s, effect.source)], Resource::cp, effect.amount);
            break;
        case Ability::add_curse: case Ability::counter_curse: add_curse(target, find_card(effect.models.front())->curse); break;
        case Ability::remove_mild_curses:
            std::erase_if(target.curses, [](auto curse) { return curse == Curse::cold || curse == Curse::fever || curse == Curse::fog || curse == Curse::flash; });
            break;
        case Ability::remove_all_curses:
            for (auto& item : target.items) reveal_item(s, target.id, item, true);
            target.curses.clear(); break;
        case Ability::remove_items: case Ability::remove_used_miracles:
            remove_items(s, target, effect);
            break;
        case Ability::sell: {
            require(s.next_token <= max_wire_integer, "scoped item token exhausted");
            const auto model = effect.models[1];
            change_resource(target, Resource::cp, -effect.amount);
            change_resource(s.state.players[index(s, effect.source)], Resource::cp, effect.amount);
            const auto overflow = deliver_item(s, target.id, model);
            s.last_sale = SaleEvent{effect.origin,effect.source,effect.target,model,effect.amount,overflow};
            break;
        }
        case Ability::buy: begin_purchase(s, effect); break;
        default: unsupported("effect has no scoped resolver");
        }
    }
    finalize_deaths(s);
    return target_damage;
}
bool next_action_effect(MatchSnapshot& s);
bool next_automatic_guardian(MatchSnapshot& s);
void advance(MatchSnapshot& s) {
    auto& owner = s.state.players[index(s, s.owner)];
    const auto external = s.batch->guardian_action.has_value() || s.batch->guardian_payload_action.has_value();
    const auto current = std::find_if(owner.curses.begin(), owner.curses.end(), disease);
    if (!s.batch->tick_processed && !owner.dead && current != owner.curses.end()) {
        DiseaseTick tick; tick.owner = owner.id; tick.before = *current; tick.hp_before = static_cast<int>(owner.hp);
        tick.worsened = bounded(s.rng, 100) < 5;
        if (tick.worsened) { tick.fallen = tick.before == Curse::heaven; add_curse(owner, tick.before); }
        tick.after = *std::find_if(owner.curses.begin(), owner.curses.end(), disease);
        if (!tick.fallen) {
            const auto amount = tick.after == Curse::cold ? -1 : tick.after == Curse::fever ? -2 : tick.after == Curse::hell ? -5 : 5;
            change_resource(owner, Resource::hp, amount);
        }
        const auto revivals_before = s.revivals;
        if (!owner.hp) for (const auto& item : owner.items) if (find_card(item.model)->ability == Ability::revive) { tick.revival_token = item.token; break; }
        finalize_deaths(s);
        if (s.revivals == revivals_before) tick.revival_token.reset();
        tick.hp_after = static_cast<int>(owner.hp);
        s.last_tick = tick;
        const auto natural = outcome(s.state);
        if (natural.terminal) { finish(s, natural); return; }
    }
    s.batch->tick_processed = true;
    if (next_action_effect(s)) return;
    if (!external && next_automatic_guardian(s)) return;
    for (std::size_t seat = 0; seat < s.gifts.size(); ++seat) {
        while (s.gifts[seat]) {
            if (!s.state.players[seat].dead) gift(s, seat);
            --s.gifts[seat];
        }
    }
    s.batch.reset();
    if (external && !owner.dead) { s.actor = s.owner; s.phase = MatchPhase::main; return; }
    const auto old = index(s, s.owner);
    for (std::size_t offset = 1; offset <= s.state.players.size(); ++offset) {
        const auto seat = (old + offset) % s.state.players.size();
        if (!s.state.players[seat].dead) {
            s.owner = s.state.players[seat].id; s.actor = s.owner; s.phase = MatchPhase::main;
            if (!external) {
                require(s.state.gf < static_cast<std::int64_t>(max_wire_integer), "GF exhausted");
                ++s.state.gf;
            }
            return;
        }
    }
    throw std::logic_error("no alive next owner without a terminal outcome");
}
bool next_action_effect(MatchSnapshot& s) {
    auto& batch = *s.batch;
    const auto origin = batch.guardian_action ? batch.guardian_action->source : batch.guardian_payload_action ? batch.guardian_payload_action->source : batch.owner;
    if (s.state.players[index(s, origin)].dead) batch.cursor = batch.targets.size();
    while (!s.deaths.empty()) {
        auto effect = std::move(s.deaths.front()); s.deaths.erase(s.deaths.begin());
        if (s.state.players[index(s, effect.target)].dead) continue;
        effect.forced_hit = has_curse(s.state.players[index(s, effect.target)], Curse::darkcloud);
        if (effect.forced_hit) ++s.death_forced_hits;
        else {
            ++s.death_rolls;
            if (bounded(s.rng, 100) >= static_cast<std::uint64_t>(effect.hit_rate)) continue;
            ++s.death_hits;
        }
        ++s.death_started; effect.redirect_rng = s.rng;
        s.actor = effect.target; s.phase = MatchPhase::response; s.queue.push_back(std::move(effect));
        return true;
    }
    while (!s.counters.empty()) {
        auto effect = std::move(s.counters.front()); s.counters.erase(s.counters.begin());
        if (s.state.players[index(s, effect.source)].dead || s.state.players[index(s, effect.target)].dead) continue;
        if (effect.hit_rate) {
            effect.forced_hit = has_curse(s.state.players[index(s, effect.target)], Curse::darkcloud);
            if (effect.forced_hit) ++s.counter_forced_hits;
            else {
                ++s.counter_rolls;
                if (bounded(s.rng, 100) >= static_cast<std::uint64_t>(effect.hit_rate)) continue;
                ++s.counter_hits;
            }
        }
        ++s.counter_started; effect.redirect_rng = s.rng;
        s.actor = effect.target; s.phase = MatchPhase::response; s.queue.push_back(std::move(effect));
        return true;
    }
    const auto rate = batch.models.empty() ? 0 : find_card(batch.models.front())->hit_rate;
    while (batch.cursor < batch.targets.size()) {
        const auto target = batch.targets[batch.cursor++];
        if (s.state.players[index(s, target)].dead) continue;
        const auto forced = batch.chance && has_curse(s.state.players[index(s, target)], Curse::darkcloud);
        if (forced) ++s.chance_forced_hits;
        else if (batch.chance) {
            ++s.chance_rolls;
            if (bounded(s.rng, 100) >= static_cast<std::uint64_t>(rate)) continue;
            ++s.chance_hits;
        }
        // Original payloads use the launch anchor, never a source's later MP
        // or a previous hit's redirected/filtered frame.
        if (batch.guardian_action) s.queue.push_back(guardian_effect_for(*batch.guardian_action, target, s.rng));
        else if (batch.guardian_payload_action) s.queue.push_back(payload_effect_for(s,*batch.guardian_payload_action,target,batch.guardian_payload_mp,batch.random_hp_delta));
        else s.queue.push_back(effect_for(s, batch.models, target, batch.attack.items,
                                        find_player(batch.initial_state, batch.owner)->mp, batch.random_hp_delta, batch.owner));
        s.queue.back().forced_hit = forced;
        s.actor = target; s.phase = MatchPhase::response;
        return true;
    }
    return false;
}
const std::vector<CardId>& guardian_moves(Guardian guardian) {
    static const auto moves=[] {
        std::array<std::vector<CardId>,11> result;
        for (const auto& card:catalog()) if (match_guardian_supported(card.id)) {
            const auto weight=card.guardian_attack_rate ? card.guardian_attack_rate : 1;
            for (int i=0;i<weight;++i) result[static_cast<std::size_t>(card.guardian)].push_back(card.id);
        }
        for (std::size_t i=1;i<result.size();++i)
            if (result[i].size()!=(i<=8?20u:1u)) throw std::logic_error("canonical guardian table changed");
        return result;
    }();
    return moves[static_cast<std::size_t>(guardian)];
}
bool next_automatic_guardian(MatchSnapshot& s) {
    auto& batch=*s.batch; batch.guardian_stage_started=true;
    if (s.state.players[index(s,batch.owner)].dead) { batch.guardian_cursor=s.state.players.size(); return false; }
    while (batch.guardian_cursor<s.state.players.size()) {
        const auto seat=batch.guardian_cursor++; const auto& source=s.state.players[seat];
        if (source.dead || source.guardian==Guardian::none || !is_enemy(source,s.state.players[index(s,batch.owner)])) continue;
        ++s.guardian_opportunities;
        if (bounded(s.rng,100)>=25) continue;
        const auto& moves=guardian_moves(source.guardian); const auto model=moves[bounded(s.rng,moves.size())];
        const auto& card=*find_card(model); GuardianAction action{source.id,model,{}};
        if (card.ability!=Ability::boost_cp_of_everybody) {
            const auto hostile=card.atk>0 || card.ability==Ability::add_curse || card.ability==Ability::boost_cp_to_enemy || card.ability==Ability::take_cp;
            std::array<PlayerId,player_capacity> targets{}; std::size_t count=0;
            for (const auto& target:s.state.players) if (!target.dead && is_enemy(source,target)==hostile) targets[count++]=target.id;
            require(count>0,"automatic guardian has no target"); action.target=targets[bounded(s.rng,count)];
        }
        ++s.auto_guardian_actions; s.last_guardian=action; s.last_auto_guardians.push_back(action);
        if (!action.target) {
            for (auto target:alive_players(s.state)) resolve(s,guardian_effect_for(action,target,s.rng),0);
            continue;
        }
        auto effect=guardian_effect_for(action,*action.target,s.rng);
        if (effect.hit_rate) {
            effect.forced_hit=has_curse(s.state.players[index(s,*action.target)],Curse::darkcloud);
            if (effect.forced_hit) ++s.chance_forced_hits;
            else { ++s.chance_rolls; if (bounded(s.rng,100)>=static_cast<std::uint64_t>(effect.hit_rate)) continue; ++s.chance_hits; }
        }
        effect.redirect_rng=s.rng;
        if (!damaging(effect) && effect.source==effect.target) { resolve(s,effect,0); continue; }
        s.queue.push_back(std::move(effect)); s.phase=MatchPhase::response; s.actor=*action.target;
        return true;
    }
    return false;
}

void reserve_counters(MatchSnapshot& s, const EffectFrame& incoming, const Command& response,
                      std::span<const CardId> models, std::int64_t damage) {
    if (!damaging(incoming) || damage <= 0 || s.state.players[index(s, response.actor)].dead) return;
    for (std::size_t i = 0; i < models.size(); ++i) {
        const auto& card = *find_card(models[i]);
        if (card.ability != Ability::counter_atk && card.ability != Ability::counter2x_atk
            && card.ability != Ability::counter_boost2x_mp && card.ability != Ability::counter_take_cp
            && card.ability != Ability::counter_curse) continue;
        EffectFrame effect;
        effect.origin = response.actor; effect.source = response.actor; effect.target = incoming.source;
        effect.models = {card.id}; effect.tokens = {response.items[i]}; effect.category = Category::armor;
        effect.element = card.element;
        if (card.ability == Ability::counter_atk || card.ability == Ability::counter2x_atk)
            effect.power = damage * (card.ability == Ability::counter2x_atk ? 2 : 1);
        else if (card.ability != Ability::counter_curse) {
            effect.amount = static_cast<int>(damage * (card.ability == Ability::counter_boost2x_mp ? 2 : 1));
            if (card.ability == Ability::counter_boost2x_mp) effect.target = response.actor;
        }
        effect.ability = card.ability; effect.initial_target = effect.target; effect.hit_rate = card.hit_rate;
        effect.is_counter = true;
        s.counters.push_back(std::move(effect)); ++s.counter_created; ++s.batch->counters_created;
    }
}
void clear_events(MatchSnapshot& s) {
    s.last_danger.reset(); s.last_removal.reset(); s.last_sale.reset(); s.last_purchase.reset();
    s.last_tick.reset(); s.last_fog.reset(); s.last_reveals.clear(); s.last_redraws.clear(); s.last_guardian.reset(); s.last_guardian_payload.reset();
    s.last_auto_guardians.clear(); s.last_guardian_changes.clear();
}
ActionBatch action_anchor(const MatchSnapshot& s) {
    ActionBatch batch;
    batch.initial_state = s.state; batch.owner = s.owner; batch.rng = s.rng;
    batch.decisions = s.decisions; batch.next_token = s.next_token;
    batch.chance_rolls = s.chance_rolls; batch.chance_hits = s.chance_hits;
    batch.counter_created = s.counter_created; batch.counter_started = s.counter_started;
    batch.counter_rolls = s.counter_rolls; batch.counter_hits = s.counter_hits;
    batch.chance_forced_hits = s.chance_forced_hits; batch.counter_forced_hits = s.counter_forced_hits;
    batch.revivals = s.revivals;
    batch.dream_rolls = s.dream_rolls; batch.dream_masks = s.dream_masks;
    batch.redraws = s.redraws; batch.redrawn_items = s.redrawn_items;
    batch.guardian_actions = s.guardian_actions; batch.guardian_payload_actions=s.guardian_payload_actions;
    batch.guardian_summons=s.guardian_summons; batch.guardian_opportunities=s.guardian_opportunities; batch.auto_guardian_actions=s.auto_guardian_actions;
    batch.guardian_departure_rolls=s.guardian_departure_rolls; batch.guardian_departures=s.guardian_departures;
    batch.death_bows = s.death_bows; batch.death_created = s.death_created; batch.death_started = s.death_started;
    batch.death_rolls = s.death_rolls; batch.death_hits = s.death_hits; batch.death_forced_hits = s.death_forced_hits;
    return batch;
}
void complete_decision(MatchSnapshot& s, bool completed) {
    ++s.decisions; require(s.state.update_count < max_wire_integer, "update counter exhausted"); ++s.state.update_count;
    const auto natural = outcome(s.state);
    if (natural.terminal) finish(s, natural);
    else if (s.decisions == s.config.max_decisions) {
        if (s.purchase) complete_purchase(s, PurchaseReason::cutoff);
        s.truncated = true; s.phase = MatchPhase::finished; s.actor.reset(); s.queue.clear(); s.batch.reset(); s.counters.clear(); s.deaths.clear(); std::fill(s.gifts.begin(), s.gifts.end(), 0);
    } else if (completed) {
        if (!next_action_effect(s)) advance(s);
    }
}
void execute_guardian(MatchSnapshot& s, const GuardianAction& action) {
    prepare_guardian(s, action);
    clear_events(s); auto batch = action_anchor(s);
    batch.guardian_action = action; batch.models = {action.model}; batch.tick_processed = true;
    batch.chance = find_card(action.model)->hit_rate > 0;
    if (action.target) batch.targets = {*action.target};
    s.batch = std::move(batch); s.last_guardian = action; ++s.guardian_actions;
    bool completed = false;
    const auto& card = *find_card(action.model);
    if (card.ability == Ability::boost_cp_of_everybody) {
        // This canonical global CP effect is atomic and has no response targets.
        const auto alive = alive_players(s.state);
        for (auto target : alive) resolve(s, guardian_effect_for(action,target,s.rng), 0);
        completed = true;
    } else {
        auto effect = guardian_effect_for(action,*action.target,s.rng);
        if (!damaging(effect) && *action.target == action.source && !s.batch->chance) {
            s.batch->cursor = s.batch->targets.size(); resolve(s,effect,0); completed = true;
        } else completed = !next_action_effect(s);
    }
    complete_decision(s, completed);
}
void execute_guardian_payload(MatchSnapshot& s, const GuardianPayloadAction& action) {
    prepare_guardian_payload(s,action);
    clear_events(s); auto batch=action_anchor(s);
    batch.guardian_payload_action=action; batch.models=action.models; batch.tick_processed=true;
    batch.guardian_payload_mp=s.state.players[index(s,action.source)].mp;
    const auto& base=*find_card(action.models.front());
    batch.random_hp_delta=base.ability==Ability::boost_hp_or_deal_damage ? (bounded(s.rng,2)==0?10:-10) : 0;
    batch.chance=base.hit_rate>0; batch.targets={*action.target};
    if (base.ability==Ability::attack_twice) batch.targets.push_back(*action.target);
    s.batch=std::move(batch); s.last_guardian_payload=action; ++s.guardian_payload_actions;
    auto effect=payload_effect_for(s,action,*action.target,s.batch->guardian_payload_mp,s.batch->random_hp_delta);
    if (base.ability==Ability::atk_by2x_mp) s.state.players[index(s,action.source)].mp=0;
    bool completed=false;
    if (!damaging(effect) && action.source==*action.target && !s.batch->chance) {
        s.batch->cursor=s.batch->targets.size(); resolve(s,effect,0); completed=true;
    } else completed=!next_action_effect(s);
    complete_decision(s,completed);
}
// Used by both the live transactional step and privileged journal replay.
void execute(MatchSnapshot& s, const Command& command) {
    const auto models = prepare(s, command); const auto seat = index(s, command.actor);
    if (s.phase == MatchPhase::main) clear_events(s);
    bool completed = false;
    if (s.phase == MatchPhase::main) {
        auto batch = action_anchor(s); batch.attack = command; batch.models = models; s.batch = std::move(batch);
    }
    if (s.phase == MatchPhase::purchase) {
        s.batch->responses.push_back(command);
        complete_purchase(s, command.kind == CommandKind::accept ? PurchaseReason::bought : PurchaseReason::declined);
        completed = true;
    } else if (s.phase == MatchPhase::response) {
        s.batch->responses.push_back(command);
        auto& effect = s.queue.front();
        const auto response = apply_response(effect, models);
        change_resource(s.state.players[seat], Resource::mp, -magic_cost(models));
        consume(s, seat, command.items, false);
        for (auto model : models) {
            const auto& card = *find_card(model);
            if (card.ability == Ability::self_curse || card.ability == Ability::self_curse_and_redraw) {
                add_curse(s.state.players[seat], card.curse);
                if (card.ability == Ability::self_curse_and_redraw) redraw(s, seat);
            }
        }
        if (response.reaction == Reaction::reflect || response.reaction == Reaction::bounce) {
            if (response.reaction == Reaction::bounce) effect.target = sample_bounce_target(s, s.rng);
            effect.redirects.push_back({command.actor, models, command.items, effect.target});
            if (s.state.players[index(s, effect.target)].dead) { s.queue.clear(); completed = true; }
            else s.actor = effect.target;
        } else {
            if (response.reaction == Reaction::none) {
                const auto damage = resolve(s, effect, response.defense);
                reserve_counters(s, effect, command, models, damage);
            }
            s.queue.clear(); completed = !s.purchase;
        }
    } else if (command.kind == CommandKind::discard) {
        consume(s, seat, command.items, true); completed = true;
    } else if (command.kind == CommandKind::pray) {
        request_gift(s, seat); completed = true;
    } else if (command.kind == CommandKind::exchange) {
        consume(s, seat, command.items, true); request_gift(s, seat);
        const auto& allocation = *command.allocation;
        auto& own = s.state.players[seat];
        own.hp = allocation.hp; own.mp = allocation.mp; own.cp = allocation.cp;
        finalize_deaths(s); completed = true;
    } else {
        // Preserve the pre-draw anchor for journal replay. Payload construction
        // stays pure so activating a frame cannot draw the outcome a second time.
        const auto launch_rng = s.rng;
        auto target = command.target.value_or(s.owner);
        const auto& base = *find_card(models.front());
        if ((command.kind == CommandKind::play || command.kind == CommandKind::buy || command.kind == CommandKind::sell) && command.target
            && has_curse(s.state.players[seat], Curse::fog) && is_enemy(s.state.players[seat], s.state.players[index(s, target)])
            && (command.kind == CommandKind::sell || (!base.hit_rate && base.ability != Ability::danger && !has_wide(models)))) {
            const auto enemies = enemy_targets(s);
            const auto intended = target;
            target = enemies[bounded(s.rng, enemies.size())];
            s.last_fog = FogTarget{s.owner,intended,target,models};
        }
        const auto random_hp_delta = find_card(models.front())->ability == Ability::boost_hp_or_deal_damage
            ? (bounded(s.rng, 2) == 0 ? 10 : -10) : 0;
        s.batch->random_hp_delta = random_hp_delta;
        std::optional<ItemToken> mortar;
        if (find_card(models.front())->ability == Ability::danger) {
            for (const auto& player : s.state.players) if (!player.dead) {
                const auto item = std::find_if(player.items.begin(), player.items.end(), [](const auto& value) {
                    return find_card(value.model)->ability == Ability::attract_danger;
                });
                if (item != player.items.end()) { target = player.id; mortar = item->token; break; }
            }
            if (!mortar) target = sample_bounce_target(s, s.rng);
        }
        auto original = effect_for(s, models, target, command.items, s.state.players[seat].mp, random_hp_delta, s.owner);
        const auto consume_launch = [&] {
            if (command.kind == CommandKind::sell) { consume(s, seat, command.items, true); request_gift(s, seat); }
            else consume(s, seat, command.items, false);
        };
        if (find_card(models.front())->ability == Ability::danger) {
            if (mortar) original.power = attack_power(models, 99);
            s.last_danger = DangerEvent{s.owner, target, models, mortar.has_value(), original.power, original.element};
        }
        const auto play_cost = original.ability == Ability::atk_by2x_mp ? s.state.players[seat].mp : magic_cost(models);
        if (mortar) {
            change_resource(s.state.players[seat], Resource::mp, -play_cost); consume_launch();
            consume(s, index(s, target), {*mortar}, false);
            resolve(s, original, 0); completed = true;
        } else if (find_card(models.front())->hit_rate || damaging(original) || original.target != s.owner) {
            auto& batch = *s.batch;
            require(batch.rng == launch_rng, "launch journal lost its pre-draw anchor");
            batch.random_hp_delta = random_hp_delta;
            batch.chance = find_card(models.front())->hit_rate != 0;
            batch.targets = batch.chance || (command.kind != CommandKind::sell && has_wide(models)) ? enemy_targets(s) : std::vector<PlayerId>{target};
            if (original.ability == Ability::attack_twice) {
                const auto first_pass = batch.targets;
                batch.targets.insert(batch.targets.end(), first_pass.begin(), first_pass.end());
            }
            change_resource(s.state.players[seat], Resource::mp, -play_cost); consume_launch();
            completed = !next_action_effect(s);
        } else {
            change_resource(s.state.players[seat], Resource::mp, -play_cost); consume_launch();
            resolve(s, original, 0); completed = true;
        }
    }
    complete_decision(s, completed);
}
void validate_snapshot(const MatchSnapshot& s) {
    require(s.version == match_snapshot_version, "unknown match snapshot version"); config_valid(s.config); closed_state(s.state, s.config);
    const auto n = s.state.players.size();
    require(s.gifts.size() == n && s.rewards.size() == n && s.decisions <= s.config.max_decisions, "invalid snapshot dimensions/counters");
    require(s.chance_hits <= s.chance_rolls && s.chance_rolls <= 2 * player_capacity * s.decisions, "invalid chance attempt counters");
    require(s.chance_forced_hits <= 2 * player_capacity * s.decisions - s.chance_rolls, "invalid forced chance count");
    require(s.counter_hits <= s.counter_rolls && s.counter_rolls <= s.counter_created
        && s.counter_hits <= s.counter_started && s.counter_started <= s.counter_created
        && s.counter_created <= s.config.hand_limit * s.decisions, "invalid counter statistics");
    require(s.counter_forced_hits <= s.counter_started - s.counter_hits, "invalid forced counter count");
    require(s.revivals <= player_capacity * s.decisions, "invalid revival count");
    require(s.guardian_actions <= s.decisions && s.guardian_payload_actions<=s.decisions-s.guardian_actions,"invalid guardian activation count");
    if (s.last_guardian_payload) {
        require(s.guardian_payload_actions>0 && s.decisions>0,"invalid guardian payload event");
        payload_models(*s.last_guardian_payload); (void)index(s,s.last_guardian_payload->source); (void)index(s,*s.last_guardian_payload->target);
    }
    require(s.guardian_summons<=s.decisions && s.guardian_opportunities<=player_capacity*s.decisions
        && s.auto_guardian_actions<=s.guardian_opportunities && s.guardian_departures<=s.guardian_departure_rolls
        && s.guardian_departure_rolls<=2*s.decisions,"invalid guardian lifecycle counts");
    require(s.last_auto_guardians.size()<=n && s.last_auto_guardians.size()<=s.auto_guardian_actions,"invalid automatic guardian log");
    std::size_t previous_seat=0; bool first_auto=true;
    for (const auto& action:s.last_auto_guardians) {
        require(match_guardian_supported(action.model),"invalid automatic model"); const auto seat=index(s,action.source);
        require(first_auto || seat>previous_seat,"automatic source order invalid"); first_auto=false; previous_seat=seat;
        if (find_card(action.model)->ability==Ability::boost_cp_of_everybody) require(!action.target,"automatic global target");
        else { require(action.target.has_value(),"automatic target missing"); (void)index(s,*action.target); }
    }
    require(s.last_guardian_changes.size()<=3*s.decisions,"oversized guardian change log");
    for (const auto& event:s.last_guardian_changes) {
        (void)index(s,event.player);
        require(static_cast<unsigned>(event.before)<=10 && static_cast<unsigned>(event.after)<=10
            && static_cast<unsigned>(event.reason)<=1 && (event.hidden_viewers>>n)==0,"invalid guardian change");
        if (event.reason==GuardianChangeReason::summon) require(event.after!=Guardian::none,"empty summoned guardian");
        else require(event.before!=Guardian::none && event.after==Guardian::none,"invalid departed guardian");
    }
    if (!s.last_auto_guardians.empty()) require(s.last_guardian && *s.last_guardian==s.last_auto_guardians.back(),"latest automatic guardian mismatch");
    if (s.last_guardian) {
        require((s.guardian_actions > 0 || s.auto_guardian_actions>0) && s.decisions > 0 && match_guardian_supported(s.last_guardian->model), "invalid guardian event");
        (void)index(s,s.last_guardian->source);
        if (find_card(s.last_guardian->model)->ability == Ability::boost_cp_of_everybody)
            require(!s.last_guardian->target, "global guardian event has target");
        else { require(s.last_guardian->target.has_value(), "guardian event missing target"); (void)index(s,*s.last_guardian->target); }
    }
    require(s.dream_masks <= s.dream_rolls && s.dream_rolls <= s.next_token, "invalid Dream gift statistics");
    require(s.redraws <= s.config.hand_limit * s.decisions && s.redrawn_items <= s.config.hand_limit * s.redraws && s.redrawn_items <= s.next_token,
        "invalid redraw replacement statistics");
    require(s.last_redraws.size() <= s.redraws && s.last_redraws.size() <= s.config.hand_limit * s.decisions,
        "invalid redraw event boundary");
    for (auto owner : s.last_redraws) (void)index(s, owner);
    // Immediate replacements can reveal fresh cards later in the same action.
    require(s.last_reveals.size() <= n * s.config.hand_limit * s.decisions, "invalid reveal event boundary");
    for (const auto& event : s.last_reveals) {
        (void)index(s, event.owner);
        require(match_dream_compatible(event.model,event.face), "invalid compatible reveal models");
    }
    const auto death_bound = player_capacity * (player_capacity - 1) * s.config.hand_limit;
    require(s.death_bows <= player_capacity * s.config.hand_limit && s.death_bows <= s.death_created
        && s.death_created <= (player_capacity - 1) * s.death_bows && s.death_created <= death_bound && s.death_hits <= s.death_rolls
        && s.death_rolls <= s.death_created && s.death_started == s.death_hits + s.death_forced_hits
        && s.death_forced_hits <= s.death_created - s.death_rolls && s.deaths.size() <= s.death_created, "invalid posthumous counts");
    if (s.last_removal) {
        require(s.decisions > 0 && s.phase != MatchPhase::purchase && !s.last_danger && !s.last_sale && !s.last_purchase, "invalid removal event boundary");
        const auto& event = *s.last_removal;
        (void)index(s, event.origin); (void)index(s, event.source); (void)index(s, event.target);
        require(event.model == 205 || event.model == 206, "invalid removal event model");
        require(event.removed_models.size() <= (event.model == 205 ? 3u : 2u), "oversized removal event");
        for (auto model : event.removed_models)
            require(match_supported(model) && (event.model != 206 || find_card(model)->category == Category::miracles), "invalid removed model");
    }
    if (s.last_sale) {
        require(s.decisions > 0 && s.phase != MatchPhase::purchase && !s.last_danger && !s.last_removal && !s.last_purchase, "invalid sale event boundary");
        const auto& event = *s.last_sale;
        (void)index(s, event.origin); (void)index(s, event.source); (void)index(s, event.target);
        require(match_supported(event.model) && event.price == find_card(event.model)->price, "invalid sale event commodity or price");
        require(!event.overflow_model || match_supported(*event.overflow_model), "invalid sale overflow model");
    }
    if (s.last_purchase) {
        require(s.decisions > 0 && s.phase != MatchPhase::purchase
            && !s.last_danger && !s.last_removal && !s.last_sale, "invalid purchase event boundary");
        const auto& event = *s.last_purchase;
        (void)index(s, event.origin); (void)index(s, event.buyer); (void)index(s, event.seller);
        require(static_cast<unsigned>(event.reason) <= 5, "invalid purchase result reason");
        if (event.reason == PurchaseReason::self_inspection)
            require(event.buyer == event.seller && event.origin == event.buyer, "invalid self inspection participants");
        else if (event.reason != PurchaseReason::empty)
            require(event.buyer != event.seller, "foreign purchase result aliases its participants");
        else require(event.buyer != event.seller || event.origin == event.buyer, "invalid self empty inspection origin");
        require(!event.overflow_model || (event.reason == PurchaseReason::bought && match_supported(*event.overflow_model)), "invalid purchase overflow model");
        if (event.reason == PurchaseReason::empty) require(event.model == 0 && event.price == 0, "empty purchase has a commodity");
        else require(match_supported(event.model) && event.price == find_card(event.model)->price, "invalid purchase result price");
    }
    if (s.last_danger) {
        require(s.decisions > 0, "danger event precedes any accepted action");
        const auto& event = *s.last_danger;
        (void)index(s, event.origin); (void)index(s, event.target);
        require(!event.models.empty() && event.models.size() <= s.config.hand_limit && event.models.front() == 94, "invalid danger event shape");
        std::vector<VisibleSelection> prefix;
        for (auto model : event.models) {
            require(match_supported(model) && can_append_main(prefix, {model,false}), "invalid danger event composition");
            prefix.push_back({model,false});
        }
        require(!has_wide(event.models) && event.power == attack_power(event.models, event.mortar ? 99 : 30)
            && event.element == attack_element(event.models), "invalid danger event payload");
    }
    if (s.last_fog) {
        const auto& event = *s.last_fog;
        const auto& owner = s.state.players[index(s, event.owner)];
        require(s.decisions > 0 && !s.last_danger
            && is_enemy(owner, s.state.players[index(s, event.intended)]) && is_enemy(owner, s.state.players[index(s, event.target)]), "invalid Fog selection endpoints");
        require(!event.models.empty() && event.models.size() <= s.config.hand_limit, "invalid Fog selection models");
        std::vector<VisibleSelection> prefix;
        for (auto model : event.models) { require(match_supported(model) && can_append_main(prefix,{model,false}), "invalid Fog selection composition"); prefix.push_back({model,false}); }
        const auto& base = *find_card(event.models.front());
        require(!base.hit_rate && base.ability != Ability::danger && base.ability != Ability::exchange
            && (base.ability == Ability::sell || !has_wide(event.models))
            && (base.ability != Ability::buy || event.models.size() == 1)
            && (base.ability != Ability::sell || event.models.size() == 2)
            && (!s.last_sale || (base.ability == Ability::sell && s.last_sale->origin == event.owner)), "invalid Fog selection role");
    }
    (void)index(s, s.owner);
    require(s.next_token <= max_wire_integer + 1, "invalid next token");
    for (std::size_t i = 0; i < n; ++i) {
        require(s.gifts[i] <= s.config.hand_limit && s.state.players[i].items.size() + s.gifts[i] <= s.config.hand_limit, "invalid reserved gifts");
        for (const auto& item : s.state.players[i].items) require(item.token < s.next_token, "next token collides with a live instance");
    }
    if (s.last_tick) {
        const auto& tick = *s.last_tick;
        const auto& owner = s.state.players[index(s, tick.owner)];
        require(s.decisions > 0 && (s.phase == MatchPhase::main || s.phase == MatchPhase::finished || (s.batch && s.batch->tick_processed))
            && disease(tick.before) && disease(tick.after) && (s.last_auto_guardians.empty() ? has_curse(owner,tick.after) : true)
            && tick.hp_before >= 1 && tick.hp_before <= 99 && tick.hp_after >= 0 && tick.hp_after <= 99
            && (!s.last_auto_guardians.empty() || (owner.hp==tick.hp_after && owner.dead==(tick.hp_after==0))),
            "invalid disease tick boundary");
        const auto after = tick.worsened && tick.before != Curse::heaven ? static_cast<Curse>(static_cast<int>(tick.before) + 1) : tick.before;
        require(tick.after == after && tick.fallen == (tick.worsened && tick.before == Curse::heaven), "invalid disease tick progression");
        const auto amount = tick.after == Curse::cold ? -1 : tick.after == Curse::fever ? -2 : tick.after == Curse::hell ? -5 : 5;
        const auto hp = tick.fallen ? 0 : std::clamp(tick.hp_before + amount, 0, 99);
        require(tick.hp_after == (tick.revival_token ? 10 : hp) && (!tick.revival_token || (hp == 0 && s.revivals > 0 && *tick.revival_token < s.next_token && !instance(owner, *tick.revival_token))), "invalid disease tick HP or revival");
    }
    require(s.purchase.has_value() == (s.phase == MatchPhase::purchase), "purchase escrow outside purchase phase");
    if (s.purchase) {
        const auto& offer = *s.purchase;
        (void)index(s, offer.origin);
        const auto& buyer = s.state.players[index(s, offer.buyer)];
        const auto& seller = s.state.players[index(s, offer.seller)];
        require(offer.buyer != offer.seller && !buyer.dead && !seller.dead && s.actor == offer.buyer
            && !s.last_danger && !s.last_removal && !s.last_sale && !s.last_purchase, "invalid purchase participants");
        require(match_supported(offer.item.model) && !offer.item.used && !offer.item.fake_model
            && offer.item.token < s.next_token && !instance(seller, offer.item.token) && offer.slot <= seller.items.size()
            && seller.items.size() + s.gifts[index(s, offer.seller)] < s.config.hand_limit, "invalid purchase escrow");
        require(buyer.cp >= find_card(offer.item.model)->price, "unaffordable purchase quote");
    }
    const auto natural = outcome(s.state);
    if (s.phase == MatchPhase::finished) {
        require(!s.actor && s.queue.empty() && !s.batch && s.counters.empty() && s.deaths.empty() && std::all_of(s.gifts.begin(), s.gifts.end(), [](auto v) { return !v; }), "finished snapshot has pending work");
        require(s.terminated != s.truncated && s.state.is_over == s.terminated, "invalid episode boundary");
        if (s.terminated) {
            require(natural.terminal && s.winners == natural.winners, "invalid natural outcome");
            MatchSnapshot expected = s; finish(expected, natural); require(s.rewards == expected.rewards, "invalid terminal rewards");
        } else require(!natural.terminal && s.decisions == s.config.max_decisions && s.winners.empty()
            && std::all_of(s.rewards.begin(), s.rewards.end(), [](auto v) { return v == 0; }), "invalid truncation");
        return;
    }
    require(!natural.terminal && !s.terminated && !s.truncated && !s.state.is_over && s.decisions < s.config.max_decisions && s.winners.empty()
        && std::all_of(s.rewards.begin(), s.rewards.end(), [](auto v) { return v == 0; }), "invalid running match");
    require(s.actor && !s.state.players[index(s, *s.actor)].dead, "missing live actor");
    if (s.phase == MatchPhase::main) {
        require(*s.actor == s.owner && s.queue.empty() && !s.batch && s.counters.empty() && s.deaths.empty() && std::all_of(s.gifts.begin(), s.gifts.end(), [](auto v) { return !v; }), "main phase has pending work");
        return;
    }
    require((s.phase == MatchPhase::response && s.queue.size() == 1) || (s.phase == MatchPhase::purchase && s.queue.empty()), "invalid match phase/queue");
    {
        require(s.batch.has_value(), "pending effect is missing its launch journal");
        const auto& batch = *s.batch;
        require(batch.owner == s.owner && batch.initial_state.players.size() == n
            && !(batch.guardian_action && batch.guardian_payload_action)
            && (batch.guardian_action ? (batch.models == std::vector<CardId>{batch.guardian_action->model} && batch.attack == Command{})
                : batch.guardian_payload_action ? (batch.models==batch.guardian_payload_action->models && batch.models.size()<=visible_hand_capacity && batch.attack==Command{})
                : (batch.models.size() <= s.config.hand_limit && batch.attack.items.size() == batch.models.size()))
            && batch.targets.size() <= 2 * (n - 1) && batch.cursor <= batch.targets.size(), "invalid action batch shape");
        require(batch.guardian_cursor<=n && (batch.guardian_stage_started || !batch.guardian_cursor)
            && (!batch.guardian_stage_started || (batch.tick_processed && batch.cursor==batch.targets.size()))
            && (!(batch.guardian_action || batch.guardian_payload_action) || (!batch.guardian_stage_started && !batch.guardian_cursor))
            && batch.guardian_payload_mp>=0 && batch.guardian_payload_mp<=99
            && (batch.guardian_payload_action || batch.guardian_payload_mp==0),"invalid guardian stage");
        require(batch.decisions < s.decisions, "invalid action decision anchor");
        const auto action_decisions = s.decisions - batch.decisions;
        require(batch.counters_created <= action_decisions * s.config.hand_limit && s.counters.size() <= batch.counters_created
            && batch.counter_created <= s.counter_created && s.counter_created - batch.counter_created == batch.counters_created, "invalid reserved counter count");
        require(batch.revivals <= s.revivals && s.revivals - batch.revivals <= n * action_decisions, "invalid action revival count");
        require(batch.decisions < s.decisions && batch.responses.size() == s.decisions - batch.decisions - 1
            // Redraws can replenish physical responses and MP-counter sources.
            // The configured episode decision bound applies to every accepted command.
            && batch.responses.size() < s.config.max_decisions, "invalid action journal length");
        for (const auto& response : batch.responses) require(response.items.size() <= s.config.hand_limit, "oversized action response");
        closed_state(batch.initial_state, s.config); // Bound/validate the anchor before copying it.
        MatchSnapshot replay; replay.config = s.config; replay.state = batch.initial_state;
        replay.owner = batch.owner; replay.actor = batch.owner; replay.phase = MatchPhase::main;
        replay.rng = batch.rng; replay.decisions = batch.decisions; replay.next_token = batch.next_token;
        replay.chance_rolls = batch.chance_rolls; replay.chance_hits = batch.chance_hits;
        replay.counter_created = batch.counter_created; replay.counter_started = batch.counter_started;
        replay.counter_rolls = batch.counter_rolls; replay.counter_hits = batch.counter_hits;
        replay.chance_forced_hits = batch.chance_forced_hits; replay.counter_forced_hits = batch.counter_forced_hits;
        replay.revivals = batch.revivals;
        replay.dream_rolls = batch.dream_rolls; replay.dream_masks = batch.dream_masks;
        replay.redraws = batch.redraws; replay.redrawn_items = batch.redrawn_items;
        replay.guardian_actions = batch.guardian_actions; replay.guardian_payload_actions=batch.guardian_payload_actions;
        replay.guardian_summons=batch.guardian_summons; replay.guardian_opportunities=batch.guardian_opportunities; replay.auto_guardian_actions=batch.auto_guardian_actions;
        replay.guardian_departure_rolls=batch.guardian_departure_rolls; replay.guardian_departures=batch.guardian_departures;
        replay.death_bows = batch.death_bows; replay.death_created = batch.death_created; replay.death_started = batch.death_started;
        replay.death_rolls = batch.death_rolls; replay.death_hits = batch.death_hits; replay.death_forced_hits = batch.death_forced_hits;
        replay.gifts.assign(n, 0); replay.rewards.assign(n, 0);
        validate_snapshot(replay); // Non-recursive anchor has no batch or pending effect.
        require(batch.attack.kind != CommandKind::defend && batch.attack.kind != CommandKind::accept
            && batch.attack.kind != CommandKind::decline, "action journal has no valid main command");
        if (batch.guardian_payload_action) execute_guardian_payload(replay,*batch.guardian_payload_action);
        else if (batch.guardian_action) execute_guardian(replay,*batch.guardian_action);
        else execute(replay, batch.attack);
        for (const auto& response : batch.responses) {
            require(replay.batch.has_value(), "action journal continues after its batch ended");
            execute(replay, response);
        }
        require(replay == s, "action journal does not reproduce full snapshot");
        return;
    }

}
MatchStep result(const MatchSnapshot& s) { return {s.phase, s.actor, s.terminated, s.truncated, s.decisions, s.rewards, s.winners}; }
}
bool match_supported(CardId id) noexcept {
    const auto* c = find_card(id);
    const auto curse = c && supported_curse(c->curse)
        && (c->ability == Ability::add_curse_on_damage || c->ability == Ability::counter_curse || c->ability == Ability::add_curse);
    return c && c->gift_rate > 0 && (!c->hit_rate || c->ability == Ability::none || c->ability == Ability::counter_atk || c->ability == Ability::absorb_hp || c->ability == Ability::attack_dyingly || curse) && (c->category == Category::weapons || c->category == Category::armor || c->category == Category::sundries || c->category == Category::miracles || (c->category == Category::trade && (c->ability == Ability::exchange || c->ability == Ability::sell || c->ability == Ability::buy)))
        && (c->ability == Ability::none || c->ability == Ability::boost_hp || c->ability == Ability::boost_mp || c->ability == Ability::boost_cp
            || c->ability == Ability::reflect_weapon || c->ability == Ability::reflect_miracle || c->ability == Ability::reflect_anything
            || c->ability == Ability::filter_atk_element || c->ability == Ability::block_weapon || c->ability == Ability::block_miracle
            || c->ability == Ability::cut_cost || c->ability == Ability::set_element || c->ability == Ability::double_atk
            || c->ability == Ability::bounce_weapon || c->ability == Ability::bounce_miracle
            || c->ability == Ability::counter_atk || c->ability == Ability::counter2x_atk
            || c->ability == Ability::counter_boost2x_mp || c->ability == Ability::counter_take_cp
            || c->ability == Ability::absorb_hp || c->ability == Ability::deal_same_damage || c->ability == Ability::attack_dyingly || curse
            || c->ability == Ability::set_guardian || c->ability == Ability::self_curse || c->ability == Ability::self_curse_and_redraw || c->ability == Ability::boost_mp_and_add_curse
            || c->ability == Ability::remove_mild_curses || c->ability == Ability::remove_all_curses
            || c->ability == Ability::remove_items || c->ability == Ability::remove_used_miracles
            || c->ability == Ability::atk_by2x_mp || c->ability == Ability::attack_twice || c->ability == Ability::attack_every_enemy
            || c->ability == Ability::boost_hp_or_deal_damage || c->ability == Ability::revive
            || c->ability == Ability::danger || c->ability == Ability::attract_danger || c->ability == Ability::exchange || c->ability == Ability::sell || c->ability == Ability::buy);
}
bool match_guardian_payload_supported(Guardian guardian, CardId id) noexcept {
    const auto* card=find_card(id);
    return card && match_supported(id) && payload_domain(guardian,*card) && can_start_main(id)
        && card->ability!=Ability::exchange && card->ability!=Ability::sell && card->ability!=Ability::buy && card->ability!=Ability::danger;
}
bool match_guardian_supported(CardId id) noexcept {
    const auto* card = find_card(id);
    return id >= 245 && id <= 286 && card && card->category == Category::guardians;
}
bool match_dream_compatible(CardId actual, CardId face) noexcept {
    if (actual == face || !match_supported(actual) || !match_supported(face)) return false;
    const auto& a = *find_card(actual); const auto& b = *find_card(face);
    return a.category == b.category && a.element == b.element && a.ability == b.ability && a.curse == b.curse
        && a.cost == b.cost && a.hit_rate == b.hit_rate && a.is_plus_atk == b.is_plus_atk
        && (a.atk > 0) == (b.atk > 0) && (a.defense > 0) == (b.defense > 0);
}
std::vector<CardId> match_catalog() { std::vector<CardId> result; for (const auto& c : catalog()) if (match_supported(c.id)) result.push_back(c.id); return result; }
int match_gift_weight() noexcept { int weight = 0; for (const auto& c : catalog()) if (match_supported(c.id)) weight += c.gift_rate; return weight; }
void change_resource(PlayerState& p, Resource resource, std::int64_t delta) {
    require(static_cast<unsigned>(resource) <= 2 && p.hp >= 0 && p.hp <= 99 && p.mp >= 0 && p.mp <= 99 && p.cp >= 0 && p.cp <= 99, "resource update outside scoped domain");
    auto apply = [](std::int64_t& value, std::int64_t amount) {
        if (amount >= 0) { value = amount >= 99 - value ? 99 : value + amount; return std::int64_t{0}; }
        if (amount < -value) { const auto debt = amount + value; value = 0; return debt; }
        value += amount; return std::int64_t{0};
    };
    if (resource == Resource::cp) delta = apply(p.cp, delta);
    if (resource != Resource::hp) delta = apply(p.mp, delta);
    (void)apply(p.hp, delta);
}
Match::Match(MatchConfig config) : config_(config) {
    config_valid(config); const std::array<PlayerSetup, 2> players{{{0, Team::solo, "A"}, {1, Team::solo, "B"}}}; reset(players, 42);
}
void Match::reset(std::span<const PlayerSetup> players, std::uint64_t seed) {
    require(players.size() >= 2 && players.size() <= player_capacity, "scoped match needs 2..9 players");
    MatchSnapshot s; s.config = config_; s.state.gf = 1; s.rng = seed;
    for (const auto& setup : players) {
        PlayerState p; p.id = setup.id; p.team = setup.team; p.name = setup.name;
        p.hp = config_.initial_hp; p.mp = config_.initial_mp; p.cp = config_.initial_cp; s.state.players.push_back(std::move(p));
    }
    s.owner = players.front().id; s.actor = s.owner; s.phase = MatchPhase::main;
    s.gifts.assign(players.size(), 0); s.rewards.assign(players.size(), 0);
    closed_state(s.state, config_);
    for (std::size_t seat = 0; seat < players.size(); ++seat) for (unsigned i = 0; i < config_.initial_hand; ++i) gift(s, seat);
    const auto natural = outcome(s.state); if (natural.terminal) finish(s, natural);
    validate_snapshot(s); data_ = std::move(s);
}
void Match::load_scene(SnapshotState state, PlayerId owner, std::uint64_t seed) {
    require(!state.is_over, "load_scene expects an unresolved quiescent scene");
    closed_state(state, config_);
    MatchSnapshot s; s.config = config_; s.state = std::move(state); s.owner = owner; s.rng = seed;
    (void)index(s, owner); s.actor = owner; s.phase = MatchPhase::main;
    s.gifts.assign(s.state.players.size(), 0); s.rewards.assign(s.state.players.size(), 0);
    for (const auto& p : s.state.players) for (const auto& item : p.items) s.next_token = std::max(s.next_token, item.token + 1);
    const auto natural = outcome(s.state); if (natural.terminal) finish(s, natural);
    else require(!s.state.players[index(s, owner)].dead, "scene owner is dead");
    validate_snapshot(s); data_ = std::move(s);
}
void Match::restore(const MatchSnapshot& snapshot) { validate_snapshot(snapshot); auto s = snapshot; data_ = std::move(s); config_ = data_.config; }
bool Match::legal(const Command& command) const noexcept { try { (void)prepare(data_, command); return true; } catch (...) { return false; } }
MatchStep Match::step(const Command& command) {
    auto s = data_; execute(s, command);
    const auto output = result(s); data_ = std::move(s); return output;
}
bool Match::guardian_legal(const GuardianAction& action) const noexcept {
    try { prepare_guardian(data_,action); return true; } catch (...) { return false; }
}
MatchStep Match::guardian_step(const GuardianAction& action) {
    auto next = data_; execute_guardian(next,action); const auto output = result(next); data_ = std::move(next); return output;
}
Command Match::sample_action(std::uint64_t& policy_rng) const {
    require(!done() && data_.actor.has_value(), "cannot sample a finished match");
    auto rng = policy_rng;
    const auto actor = *data_.actor;
    const auto& own = data_.state.players[index(data_, actor)];
    if (data_.phase == MatchPhase::purchase) {
        const auto accept = own.items.size() < data_.config.hand_limit && bounded(rng, 2) == 0;
        Command selected{actor,accept ? CommandKind::accept : CommandKind::decline,{},{}};
        policy_rng = rng;
        return selected;
    }
    struct Proposal { ItemToken first, second; PlayerId target; CommandKind kind; std::uint8_t items; bool has_target; };
    // A miracle and a FREE are disjoint instances: at most floor(H*H/4)
    // initial pairs, each with at most P live targets. Do not value-initialize
    // this POD buffer; only the written prefix is ever read.
    constexpr auto pair_limit = visible_hand_capacity * visible_hand_capacity / 4;
    std::array<Proposal, 1 + visible_hand_capacity * (1 + player_capacity) + pair_limit * player_capacity> proposals;
    std::size_t count = 0;
    const auto cost = [](const Card& c) { return c.category == Category::miracles ? c.cost : 0; };
    const auto add = [&](CommandKind kind, std::optional<ItemToken> token, std::optional<PlayerId> target, std::optional<ItemToken> free = {}) {
        if (count == proposals.size()) throw std::logic_error("scoped sampler capacity exceeded");
        proposals[count++] = {token.value_or(0), free.value_or(0), target.value_or(0), kind,
            static_cast<std::uint8_t>(token ? (free ? 2 : 1) : 0), target.has_value()};
    };
    std::optional<ResponseContext> context;
    const auto append = [&](std::span<const VisibleSelection> prefix, VisibleSelection next) {
        return context ? can_append_response(prefix, next, *context) : can_append_main(prefix, next);
    };
    const auto add_usable = [&](CommandKind kind, const ItemInstance& item, std::optional<PlayerId> target) {
        const auto& c = *find_card(face_model(item));
        if (cost(c) <= own.mp) add(kind, item.token, target);
        if (c.category == Category::miracles) {
            const std::array<VisibleSelection, 1> prefix{{{face_model(item), item.used}}};
            for (const auto& free : own.items) if (find_card(face_model(free))->ability == Ability::cut_cost
                && append(prefix, {face_model(free), free.used})) add(kind, item.token, target, free.token);
        }
    };
    if (data_.phase == MatchPhase::response) {
        context = response_context(data_);
        add(CommandKind::defend, {}, {});
        for (const auto& item : own.items) {
            if (can_append_response({}, {face_model(item), item.used}, *context)) add_usable(CommandKind::defend, item, {});
        }
    } else {
        const auto weapon = std::any_of(own.items.begin(), own.items.end(), [](const auto& item) { return find_card(face_model(item))->category == Category::weapons; });
        if (!weapon) add(CommandKind::pray, {}, {});
        for (const auto& item : own.items) {
            const auto& c = *find_card(face_model(item));
            if (discardable(c)) add(CommandKind::discard, item.token, {});
            if (c.ability == Ability::exchange) add(CommandKind::exchange, item.token, {});
            else if (c.ability == Ability::sell) {
                if (std::any_of(own.items.begin(), own.items.end(), [&](const auto& commodity) { return commodity.token != item.token && !commodity.used; }))
                    add(CommandKind::sell, item.token, {});
            }
            else if (c.ability == Ability::buy) {
                for (const auto& target : data_.state.players) if (!target.dead) add(CommandKind::buy,item.token,target.id);
            }
            else if (can_start_main(face_model(item))) {
                if (c.hit_rate) add_usable(CommandKind::play, item, {});
                else for (const auto& target : data_.state.players) if (!target.dead) add_usable(CommandKind::play, item, target.id);
            }
        }
    }
    require(count != 0, "no scoped action available");
    const auto proposal = proposals[bounded(rng, count)];
    Command selected{actor, proposal.kind, {}, proposal.has_target ? std::optional<PlayerId>{proposal.target} : std::nullopt, {}};
    if (proposal.items) selected.items.push_back(proposal.first);
    if (proposal.items == 2) selected.items.push_back(proposal.second);
    if (selected.kind == CommandKind::sell) {
        std::array<ItemToken, visible_hand_capacity> commodities;
        std::size_t eligible = 0;
        for (const auto& commodity : own.items) if (commodity.token != proposal.first && !commodity.used) commodities[eligible++] = commodity.token;
        selected.items.push_back(commodities[bounded(rng, eligible)]);
        std::array<PlayerId, player_capacity> targets;
        std::size_t alive = 0;
        for (const auto& target : data_.state.players) if (!target.dead) targets[alive++] = target.id;
        selected.target = targets[bounded(rng, alive)]; // Never inspect another player's inventory.
    }
    if (proposal.items && (selected.kind == CommandKind::play || selected.kind == CommandKind::defend)) {
        const auto* base = instance(own, proposal.first);
        std::vector<VisibleSelection> prefix{{face_model(*base), base->used}};
        prefix.reserve(own.items.size()); selected.items.reserve(own.items.size());
        auto spent = proposal.items == 2 ? 0 : cost(*find_card(face_model(*base)));
        if (proposal.items == 2) { const auto* free = instance(own, proposal.second); prefix.push_back({face_model(*free), free->used}); }
        for (const auto& extra : own.items) if (bounded(rng, 2)) {
            if (std::find(selected.items.begin(), selected.items.end(), extra.token) != selected.items.end()) continue;
            const auto& c = *find_card(face_model(extra));
            if (find_card(face_model(*base))->ability == Ability::danger && c.ability == Ability::attack_every_enemy) continue;
            const VisibleSelection visible{face_model(extra), extra.used};
            if (!append(prefix, visible)) continue;
            auto next_spent = spent + cost(c);
            if (c.ability == Ability::cut_cost) next_spent -= cost(*find_card(prefix.back().model));
            const ItemInstance* discount = nullptr;
            if (next_spent > own.mp) {
                if (c.category != Category::miracles) continue;
                // MP readiness applies to the completed column. Pair an
                // unaffordable extra miracle with an unused FREE atomically.
                std::array<const ItemInstance*, visible_hand_capacity> free_items;
                std::size_t free_count = 0;
                prefix.push_back(visible);
                for (const auto& free : own.items) if (find_card(face_model(free))->ability == Ability::cut_cost
                    && std::find(selected.items.begin(), selected.items.end(), free.token) == selected.items.end()
                    && append(prefix, {face_model(free), free.used})) free_items[free_count++] = &free;
                prefix.pop_back();
                if (!free_count) continue;
                discount = free_items[bounded(rng, free_count)]; next_spent -= cost(c);
            }
            selected.items.push_back(extra.token); prefix.push_back(visible); spent = next_spent;
            if (discount) { selected.items.push_back(discount->token); prefix.push_back({face_model(*discount), discount->used}); }
        }
        if (selected.kind == CommandKind::play && (find_card(face_model(*base))->ability == Ability::danger || std::any_of(prefix.begin(), prefix.end(), [](const auto& item) {
            return find_card(item.model)->ability == Ability::attack_every_enemy;
        }))) selected.target.reset();
    }
    if (selected.kind == CommandKind::exchange) {
        const auto total = own.hp + own.mp + own.cp;
        const auto hp_min = std::max<std::int64_t>(0, total - 198), hp_max = std::min<std::int64_t>(99, total);
        const auto hp = hp_min + static_cast<std::int64_t>(bounded(rng, hp_max - hp_min + 1));
        const auto remaining = total - hp;
        const auto mp_min = std::max<std::int64_t>(0, remaining - 99), mp_max = std::min<std::int64_t>(99, remaining);
        const auto mp = mp_min + static_cast<std::int64_t>(bounded(rng, mp_max - mp_min + 1));
        selected.allocation = ResourceAllocation{static_cast<std::int32_t>(hp),static_cast<std::int32_t>(mp),static_cast<std::int32_t>(remaining - mp)};
    }
    policy_rng = rng;
    return selected;
}
bool Match::guardian_payload_legal(const GuardianPayloadAction& action) const noexcept {
    try { prepare_guardian_payload(data_,action); return true; } catch (...) { return false; }
}
MatchStep Match::guardian_payload_step(const GuardianPayloadAction& action) {
    auto next=data_; execute_guardian_payload(next,action); auto output=result(next); data_=std::move(next); return output;
}
std::string Match::observe(std::optional<PlayerId> viewer) const {
    const char* phase = data_.phase == MatchPhase::main ? "main" : data_.phase == MatchPhase::response ? "response" : data_.phase == MatchPhase::purchase ? "purchase" : "finished";
    const auto board = project_view(data_.state, viewer);
    std::string out = "{\"schema\":\"official-execution-view-v31\",\"ruleset\":\"";
    out += match_ruleset; out += "\",\"scope\":\""; out += match_scope;
    out += "\",\"catalog_version\":\""; out += catalog_version;
    out += "\",\"catalog_sha256\":\""; out += catalog_sha256;
    out += "\",\"client_sha256\":\""; out += client_sha256;
    out += "\",\"bounce_target_rule\":\""; out += bounce_target_rule;
    out += "\",\"chance_rule\":\""; out += chance_rule;
    out += "\",\"counter_rule\":\""; out += counter_rule;
    out += "\",\"resource_counter_rule\":\""; out += resource_counter_rule;
    out += "\",\"damage_effect_rule\":\""; out += damage_effect_rule;
    out += "\",\"flash_rule\":\""; out += flash_rule;
    out += "\",\"darkcloud_rule\":\""; out += darkcloud_rule;
    out += "\",\"removal_rule\":\""; out += removal_rule;
    out += "\",\"mana_weapon_rule\":\""; out += mana_weapon_rule;
    out += "\",\"repeat_attack_rule\":\""; out += repeat_attack_rule;
    out += "\",\"wide_attack_rule\":\""; out += wide_attack_rule;
    out += "\",\"random_hp_rule\":\""; out += random_hp_rule;
    out += "\",\"revival_rule\":\""; out += revival_rule;
    out += "\",\"danger_rule\":\""; out += danger_rule;
    out += "\",\"exchange_rule\":\""; out += exchange_rule;
    out += "\",\"removal_event_rule\":\""; out += removal_event_rule;
    out += "\",\"sell_rule\":\""; out += sell_rule;
    out += "\",\"buy_rule\":\""; out += buy_rule;
    out += "\",\"disease_rule\":\""; out += disease_rule;
    out += "\",\"disease_item_rule\":\""; out += disease_item_rule;
    out += "\",\"fog_rule\":\""; out += fog_rule;
    out += "\",\"death_rule\":\""; out += death_rule;
    out += "\",\"overflow_rule\":\""; out += overflow_rule;
    out += "\",\"dream_rule\":\""; out += dream_rule;
    out += "\",\"redraw_rule\":\""; out += redraw_rule;
    out += "\",\"guardian_rule\":\""; out += guardian_rule;
    out += "\",\"guardian_summon_rule\":\""; out += guardian_summon_rule;
    out += "\",\"guardian_departure_rule\":\""; out += guardian_departure_rule;
    out += "\",\"guardian_payload_rule\":\""; out += guardian_payload_rule; out += "\",\"phase\":\""; out += phase;
    out += "\",\"owner\":" + std::to_string(data_.owner) + ",\"actor\":";
    out += data_.actor ? std::to_string(*data_.actor) : "null";
    out += ",\"terminated\":"; out += data_.terminated ? "true" : "false";
    out += ",\"truncated\":"; out += data_.truncated ? "true" : "false";
    out += ",\"decisions\":" + std::to_string(data_.decisions) + ",\"board\":" + view_json(board);
    const auto guardian_json=[&](const GuardianAction& action) {
        out += "{\"source\":"+std::to_string(action.source)+",\"model\":"+std::to_string(action.model)+",\"guardian\":\"";
        out += guardian_name(find_card(action.model)->guardian); out += "\",\"target\":";
        out += action.target ? std::to_string(*action.target) : "null"; out += '}';
    };
    out += ",\"last_guardian_payload\":";
    if (!data_.last_guardian_payload) out+="null";
    else {
        const auto& action=*data_.last_guardian_payload;
        out+="{\"source\":"+std::to_string(action.source)+",\"guardian\":\"";
        out+=guardian_name(action.guardian); out+="\",\"models\":[";
        for (std::size_t i=0;i<action.models.size();++i) { if(i)out+=',';out+=std::to_string(action.models[i]); }
        out+="],\"target\":";out+=action.target?std::to_string(*action.target):"null";out+='}';
    }
    out += ",\"last_auto_guardians\":[";
    for (std::size_t i=0;i<data_.last_auto_guardians.size();++i) { if(i)out+=','; guardian_json(data_.last_auto_guardians[i]); } out+=']';
    out += ",\"last_guardian_changes\":[";
    bool first_change=true; std::uint16_t viewer_bit=0;
    if(viewer) for(std::size_t i=0;i<data_.state.players.size();++i) if(data_.state.players[i].id==*viewer)viewer_bit=static_cast<std::uint16_t>(1u<<i);
    for(const auto& event:data_.last_guardian_changes) if(!(event.hidden_viewers&viewer_bit)) {
        if(!first_change)out+=','; first_change=false;
        out += "{\"player\":"+std::to_string(event.player)+",\"before\":";
        if(event.before==Guardian::none)out+="null";else{out+='"';out+=guardian_name(event.before);out+='"';}
        out+=",\"after\":";
        if(event.after==Guardian::none)out+="null";else{out+='"';out+=guardian_name(event.after);out+='"';}
        out+=",\"reason\":\"";out+=event.reason==GuardianChangeReason::summon?"summon":"departure";out+="\"}";
    }
    out+=']';
    out += ",\"last_guardian\":";
    if (!data_.last_guardian) out += "null";
    else {
        const auto& action = *data_.last_guardian;
        out += "{\"source\":" + std::to_string(action.source) + ",\"model\":" + std::to_string(action.model) + ",\"guardian\":\"";
        out += guardian_name(find_card(action.model)->guardian); out += "\",\"target\":";
        out += action.target ? std::to_string(*action.target) : "null"; out += '}';
    }
    out += ",\"last_redraws\":[";
    for (std::size_t i = 0; i < data_.last_redraws.size(); ++i) {
        if (i) out += ',';
        out += std::to_string(data_.last_redraws[i]);
    }
    out += ']';
    out += ",\"last_reveals\":[";
    bool first_reveal = true;
    for (const auto& event : data_.last_reveals) if (!event.owner_only || viewer == event.owner) {
        if (!first_reveal) out += ',';
        first_reveal = false;
        out += "{\"owner\":" + std::to_string(event.owner) + ",\"face\":" + std::to_string(event.face) + ",\"model\":" + std::to_string(event.model) + '}';
    }
    out += ']';
    out += ",\"last_danger\":";
    if (!data_.last_danger) out += "null";
    else {
        const auto& event = *data_.last_danger;
        out += "{\"origin\":" + std::to_string(event.origin) + ",\"target\":" + std::to_string(event.target) + ",\"models\":[";
        bool first = true; for (auto model : event.models) { if (!first) out += ','; first = false; out += std::to_string(model); }
        out += "],\"mortar\":"; out += event.mortar ? "true" : "false";
        out += ",\"power\":" + std::to_string(event.power) + ",\"element\":";
        if (event.element) { out += '"'; out += element_name(*event.element); out += '"'; } else out += "null";
        out += '}';
    }
    out += ",\"last_removal\":";
    if (!data_.last_removal) out += "null";
    else {
        const auto& event = *data_.last_removal;
        out += "{\"origin\":" + std::to_string(event.origin) + ",\"source\":" + std::to_string(event.source)
            + ",\"target\":" + std::to_string(event.target) + ",\"model\":" + std::to_string(event.model) + ",\"removed_models\":[";
        bool first = true; for (auto model : event.removed_models) { if (!first) out += ','; first = false; out += std::to_string(model); }
        out += "]}";
    }
    out += ",\"last_sale\":";
    if (!data_.last_sale) out += "null";
    else {
        const auto& event = *data_.last_sale;
        out += "{\"origin\":" + std::to_string(event.origin) + ",\"source\":" + std::to_string(event.source) + ",\"target\":" + std::to_string(event.target)
            + ",\"model\":" + std::to_string(event.model) + ",\"price\":" + std::to_string(event.price) + ",\"overflow_model\":";
        out += event.overflow_model ? std::to_string(*event.overflow_model) : "null"; out += '}';
    }
    out += ",\"purchase\":";
    if (!data_.purchase) out += "null";
    else {
        const auto& offer = *data_.purchase;
        out += "{\"origin\":" + std::to_string(offer.origin) + ",\"buyer\":" + std::to_string(offer.buyer) + ",\"seller\":" + std::to_string(offer.seller)
            + ",\"model\":" + std::to_string(offer.item.model) + ",\"price\":" + std::to_string(find_card(offer.item.model)->price) + '}';
    }
    out += ",\"last_purchase\":";
    if (!data_.last_purchase) out += "null";
    else {
        const auto& event = *data_.last_purchase;
        out += "{\"origin\":" + std::to_string(event.origin) + ",\"buyer\":" + std::to_string(event.buyer) + ",\"seller\":" + std::to_string(event.seller)
            + ",\"model\":" + std::to_string(event.model) + ",\"price\":" + std::to_string(event.price) + ",\"bought\":";
        out += event.reason == PurchaseReason::bought ? "true" : "false";
        out += ",\"reason\":\""; out += purchase_reason_name(event.reason); out += "\",\"overflow_model\":";
        out += event.overflow_model ? std::to_string(*event.overflow_model) : "null"; out += '}';
    }
    out += ",\"pending\":";
    if (data_.queue.empty()) out += "null";
    else {
        const auto& e = data_.queue.front();
        out += "{\"origin\":" + std::to_string(e.origin) + ",\"source\":" + std::to_string(e.source) + ",\"target\":" + std::to_string(e.target) + ",\"models\":[";
        bool first = true; for (auto id : e.models) { if (!first) out += ','; first = false; out += std::to_string(id); }
        out += "],\"category\":\""; out += category_name(e.category); out += "\",\"element\":";
        if (e.element) { out += '"'; out += element_name(*e.element); out += '"'; } else out += "null";
        // A validated redirect contains exactly one matching reaction; only
        // filters and an optional FREE suffix may accompany it.
        std::size_t bounces = 0;
        for (const auto& redirect : e.redirects) for (auto model : redirect.models) {
            const auto ability = find_card(model)->ability;
            if (ability == Ability::bounce_weapon || ability == Ability::bounce_miracle) ++bounces;
        }
        out += ",\"reflections\":" + std::to_string(e.redirects.size() - bounces);
        out += ",\"bounces\":" + std::to_string(bounces);
        out += ",\"hit_rate\":" + std::to_string(e.hit_rate);
        out += ",\"counter\":"; out += e.is_counter ? "true" : "false";
        // The flag belongs to activation, before any reflection. Redirection
        // to a visible pane must not reveal the original hidden recipient.
        out += ",\"death\":"; out += e.is_death ? "true" : "false";
        out += ",\"guardian\":";
        if (e.guardian == Guardian::none) out += "null";
        else { out += '"'; out += guardian_name(e.guardian); out += '"'; }
        const auto hidden_activation = std::any_of(board.players.begin(), board.players.end(), [&](const auto& player) { return player.id == e.initial_target && player.fogged; });
        out += ",\"forced_hit\":"; out += hidden_activation ? "null" : e.forced_hit ? "true" : "false";
        out += ",\"power\":" + std::to_string(e.power) + ",\"ability\":\""; out += ability_name(e.ability);
        out += "\",\"amount\":" + std::to_string(e.amount) + '}';
    }
    out += '}'; return out;
}
}
