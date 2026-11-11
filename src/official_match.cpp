#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <tuple>
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
std::uint16_t guardian_hidden_viewers(const MatchSnapshot& s, const PlayerState& player) {
    std::uint16_t hidden=0;
    for (std::size_t seat=0; seat<s.state.players.size(); ++seat) {
        const auto& viewer=s.state.players[seat];
        if (!viewer.dead && !s.state.is_over && !player.dead && has_curse(viewer,Curse::fog) && is_enemy(player,viewer))
            hidden |= static_cast<std::uint16_t>(1u<<seat);
    }
    return hidden;
}
void change_guardian(MatchSnapshot& s, PlayerState& player, Guardian after, GuardianChangeReason reason) {
    const GuardianChange event{player.id,player.guardian,after,reason,guardian_hidden_viewers(s,player)};
    player.guardian=after; s.last_guardian_changes.push_back(event);
}
void guardian_damage(MatchSnapshot& s, PlayerState& player, std::int64_t loss) {
    if (loss<=0 || player.guardian==Guardian::none) return;
    ++s.guardian_departure_rolls;
    if (bounded(s.rng,100)<10) { ++s.guardian_departures; change_guardian(s,player,Guardian::none,GuardianChangeReason::departure); }
}

bool inventory_supported(CardId id) { return match_supported(id) || match_fate_supported(id); }
void closed_state(const SnapshotState& s, const MatchConfig& c) {
    validate_state(s);
    require(s.players.size() >= 2, "scoped match needs 2..9 players");
    require(s.gf >= 0, "negative GF outside scoped runtime");
    require(!s.tiebreak_gf || *s.tiebreak_gf>=0,"negative apocalypse threshold outside scoped runtime");
    for (const auto& p : s.players) {
        require(p.hp >= 0 && p.hp <= 99 && p.mp >= 0 && p.mp <= 99 && p.cp >= 0 && p.cp <= 99, "scene resources outside 0..99");
        require(p.dead || p.hp > 0, "HP0 without resolved death is not a quiescent scene");
        require(p.items.size() <= c.hand_limit, "scene inventory exceeds configured display limit");
        if (std::any_of(p.curses.begin(), p.curses.end(), [](auto curse) { return !supported_curse(curse); }))
            unsupported("only diseases, Fog, Dream, Flash and DarkCloud are implemented among scoped curses");
        require(p.confusion_count >= 0 && (!p.dead || !p.confusion_count), "invalid scoped confusion count");
        require(p.curses.size() <= 5 && std::count_if(p.curses.begin(), p.curses.end(), disease) <= 1 && std::is_sorted(p.curses.begin(), p.curses.end())
            && std::adjacent_find(p.curses.begin(), p.curses.end()) == p.curses.end(), "noncanonical scoped curses");
        for (const auto& item : p.items) {
            if (!inventory_supported(item.model)) unsupported("unsupported match model");
            require(!item.fake_model || (has_curse(p, Curse::dream) && !item.used
                && match_dream_disguise_supported(item.model, *item.fake_model)), "invalid supported Dream disguise");
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
void sync_grant_proof(MatchSnapshot& s) {
    if (s.grant_proof && s.batch) s.grant_proof=*s.batch;
    if (s.redraw_proof && s.batch) s.redraw_proof=*s.batch;
    if (s.purchase_proof && s.batch) s.purchase_proof=*s.batch;
    if (s.summon_proof && s.batch) s.summon_proof=*s.batch;
    if (s.automatic_guardian_proof && s.batch) s.automatic_guardian_proof=*s.batch;
}
void finish(MatchSnapshot& s, const Outcome& result) {
    sync_grant_proof(s); s.pending_grant.reset();
    s.terminated = true; s.truncated = false; s.state.is_over = true;
    s.phase = MatchPhase::finished; s.actor.reset(); s.queue.clear(); s.batch.reset(); s.counters.clear(); s.deaths.clear(); s.purchase.reset();
    std::fill(s.gifts.begin(), s.gifts.end(), 0);
    for (auto& player:s.state.players) if (!player.dead) player.confusion_count=0;
    s.winners = result.winners;
    s.rewards.assign(s.state.players.size(), 0);
    if (!s.winners.empty()) for (std::size_t i = 0; i < s.rewards.size(); ++i)
        s.rewards[i] = std::find(s.winners.begin(), s.winners.end(), s.state.players[i].id) != s.winners.end() ? 1. : -1.;
}
bool gift_model_supported(CardId id, bool include_fate) {
    const auto* card=find_card(id);
    return card && card->gift_rate>0 && (match_supported(id) || (include_fate && match_fate_supported(id)));
}
template<std::size_t Tickets, std::size_t Models>
std::array<CardId,Tickets> make_gift_tickets(bool include_fate) {
    std::array<CardId,Tickets> values{};
    std::size_t n=0,models=0;
    for (const auto& card:catalog()) if (gift_model_supported(card.id,include_fate)) {
        ++models;
        for (int i=0; i<card.gift_rate; ++i) {
            if (n>=values.size()) throw std::logic_error("scoped pool weight changed");
            values[n++]=card.id;
        }
    }
    if (n!=values.size() || models!=Models) throw std::logic_error("scoped catalog changed");
    return values;
}
std::span<const CardId> gift_tickets(bool include_fate) {
    if (include_fate) {
        static const auto pool=make_gift_tickets<500,237>(true);
        return pool;
    }
    static const auto pool=make_gift_tickets<499,236>(false);
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
    const auto tickets=gift_tickets(s.config.include_fate);
    const auto id = tickets[bounded(s.rng,tickets.size())];
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
void redraw_supplied(MatchSnapshot& s, std::size_t seat, const RedrawResult& result) {
    auto& owner=s.state.players[seat];
    require(result.items.size()==owner.items.size(),"supplied redraw physical count changed");
    const auto count=owner.items.size();
    owner.items.clear(); // As with local redraw, do not reveal the old private hand.
    ++s.redraws; s.redrawn_items+=count; s.last_redraws.push_back(owner.id);
    for(const auto& item:result.items) {
        require(s.next_token<=max_wire_integer,"scoped item token exhausted");
        owner.items.push_back({s.next_token++,item.model,item.fake_model,false});
    }
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
bool apocalypse_active(const SnapshotState& state) { return state.tiebreak_gf && state.gf>=*state.tiebreak_gf; }
bool devil_grants_enabled(const MatchSnapshot& s) { return s.config.external_grants && apocalypse_active(s.state); }
bool external_grant_pause(const MatchSnapshot& s) { return s.config.external_artifacts || devil_grants_enabled(s); }
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
// v91 only observes the exact 94+232 pair in a two-player board without
// any mortar. Do not infer mortar priority or multi-enemy routing from it.
bool scoped_danger_wide(const MatchSnapshot& s, std::span<const CardId> models) {
    return models.size() == 2 && models[0] == 94 && models[1] == 232
        && s.state.players.size() == 2 && enemy_targets(s).size() == 1
        && std::none_of(s.state.players.begin(), s.state.players.end(), [](const auto& player) {
            return std::any_of(player.items.begin(), player.items.end(), [](const auto& item) {
                return item.model == 209;
            });
        });
}
ResponseContext response_context(const MatchSnapshot& s) {
    require(s.queue.size() == 1, "response has no active item");
    const auto& effect = s.queue.front();
    return {has_curse(s.state.players[index(s, effect.target)], Curse::flash), effect.category, effect.element, damaging(effect)};
}
std::vector<CardId> prepare(const MatchSnapshot& s, const Command& command, bool confusion_command=false) {
    if (s.phase==MatchPhase::grant) unsupported("grant outcome must be supplied by an external chance driver");
    require(s.phase != MatchPhase::finished && s.actor && *s.actor == command.actor, "wrong actor or finished match");
    if (static_cast<unsigned>(command.kind) > 9) unsupported("unsupported match command kind");
    require(command.kind == CommandKind::exchange || !command.allocation, "only exchange accepts a resource allocation");
    const auto& own = s.state.players[index(s, command.actor)];
    require(!own.dead, "dead actor");
    if (confusion_command) require(s.phase==MatchPhase::main && own.confusion_count>0, "resolved confusion command requires a confused main actor");
    else if (s.phase==MatchPhase::main && own.confusion_count>0) unsupported("confused main actor requires an externally resolved confusion command");
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
    if (command.kind == CommandKind::discard || command.kind==CommandKind::sacrifice) {
        require((command.kind==CommandKind::sacrifice)==apocalypse_active(s.state),"discard or sacrifice does not match the current apocalypse stage");
        require(!models.empty() && !command.target, "discard or sacrifice needs items and no target");
        for (auto id : models) require(discardable(*find_card(id)), "cannot discard or sacrifice weapons or passive artifacts");
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
    if (find_card(actual_models.front())->ability==Ability::call_phenomenon) unsupported("owned207 requires a supplied fate outcome");
    require(find_card(models.front())->ability != Ability::exchange && find_card(models.front())->ability != Ability::sell && find_card(models.front())->ability != Ability::buy, "trade needs its explicit command");
    if (find_card(models.front())->ability == Ability::danger) {
        if (has_wide(models) && !scoped_danger_wide(s, models)) unsupported("danger with Wide is outside the scoped runtime");
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
void prepare_summon_result(const MatchSnapshot& s,const Command& command,const std::vector<CardId>& models,Guardian guardian) {
    require(s.phase==MatchPhase::main && command.kind==CommandKind::play && command.items.size()==1
        && models.size()==1 && (models.front()==201 || models.front()==239),"supplied summon requires a single ordinary201 or239 main play");
    require(static_cast<unsigned>(guardian)>=1 && static_cast<unsigned>(guardian)<=10,"supplied summon needs a known nonempty guardian");
}
void automatic_guardian_plan_shape(const MatchSnapshot& s,const std::vector<AutomaticGuardianOutcome>& outcomes) {
    require(outcomes.size()<=player_capacity,"oversized automatic guardian result plan");
    for(const auto& outcome:outcomes) {
        (void)index(s,outcome.source);
        require((outcome.model==0 || match_guardian_supported(outcome.model)) && (outcome.model || !outcome.target),"invalid canonical automatic guardian result");
        if(outcome.target) (void)index(s,*outcome.target);
    }
}
void prepare_automatic_guardian_response(const MatchSnapshot& s,const std::vector<AutomaticGuardianOutcome>& outcomes) {
    if(s.config.local_guardian_payloads) unsupported("supplied canonical guardian stage does not support local payload mode");
    require(s.phase==MatchPhase::response && s.batch && !s.batch->guardian_stage_started
        && !s.batch->automatic_guardian_outcomes && s.batch->resolved_automatic_guardians.empty()
        && !s.batch->guardian_action && !s.batch->guardian_payload_action && !s.batch->phenomenon_action
        && !s.batch->devil_action && !s.batch->fate_action,"supplied guardian outcomes require an ordinary response before the first stage");
    automatic_guardian_plan_shape(s,outcomes);
}
void validate_automatic_guardian_entry(const MatchSnapshot& s,const std::vector<AutomaticGuardianOutcome>& outcomes) {
    automatic_guardian_plan_shape(s,outcomes);
    const auto& owner=s.state.players[index(s,s.batch->owner)];require(!owner.dead,"supplied guardian stage has no live original owner");
    std::size_t cursor=0;
    for(const auto& source:s.state.players) {
        if(source.dead || source.guardian==Guardian::none || !is_enemy(source,owner)) continue;
        require(cursor<outcomes.size() && outcomes[cursor].source==source.id,"automatic guardian results do not cover eligible sources in input order");
        const auto& outcome=outcomes[cursor++];if(!outcome.model) continue;
        const auto& card=*find_card(outcome.model);require(card.guardian==source.guardian,"automatic result belongs to a different guardian");
        if(card.ability==Ability::boost_cp_of_everybody) require(!outcome.target,"global guardian result must be targetless");
        else {
            require(outcome.target.has_value(),"automatic guardian result needs a target");
            const auto& target=s.state.players[index(s,*outcome.target)];
            const bool hostile=card.atk>0 || card.ability==Ability::add_curse || card.ability==Ability::boost_cp_to_enemy || card.ability==Ability::take_cp;
            require(!target.dead && is_enemy(source,target)==hostile,"automatic guardian target is outside its normal alive target pool");
        }
    }
    require(cursor==outcomes.size(),"unused extra automatic guardian source result");
}
void redraw_result_shape(const MatchSnapshot& s, const RedrawResult& result) {
    require(result.items.size()<=s.config.hand_limit,"oversized supplied redraw");
    for(const auto& item:result.items) {
        if(!gift_model_supported(item.model,s.config.include_fate)) unsupported("redraw model outside configured gift pool");
        if(item.fake_model && !match_dream_disguise_supported(item.model,*item.fake_model))
            unsupported("unsupported supplied redraw disguise");
    }
}
void prepare_redraw_response(const MatchSnapshot& s, const Command& command, const std::vector<CardId>& models,
                            const std::vector<RedrawResult>& results) {
    require(s.phase==MatchPhase::response && command.kind==CommandKind::defend && s.batch,
            "supplied redraw needs a normal response boundary");
    const auto hats=std::count_if(models.begin(),models.end(),[](CardId model){return find_card(model)->ability==Ability::self_curse_and_redraw;});
    require(hats>0 && results.size()==static_cast<std::size_t>(hats),"one redraw result is required for each selected178");
    const auto& owner=s.state.players[index(s,command.actor)];
    const auto removed=std::count_if(models.begin(),models.end(),[](CardId model){return find_card(model)->category!=Category::miracles;});
    const auto count=owner.items.size()-static_cast<std::size_t>(removed);
    for(const auto& result:results) {
        redraw_result_shape(s,result);
        require(result.items.size()==count,"supplied redraw must replace every remaining physical copy");
    }
    const auto tokens=count*results.size(); // At most hand_limit squared.
    require(!tokens || (s.next_token<=max_wire_integer && tokens<=max_wire_integer+1-s.next_token),
            "supplied redraw token budget exhausted");
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
GuardianPayloadAction select_guardian_payload(const MatchSnapshot& data, PlayerId source, std::uint64_t& chance_rng) {
    const auto& player=data.state.players[index(data,source)];
    require(!player.dead && (player.guardian==Guardian::earth || player.guardian==Guardian::moon),"payload sampler requires an alive Earth or Moon source");
    auto rng=chance_rng;
    GuardianPayloadAction selected{source,player.guardian,{},{}};
    std::array<CardId,catalog_count> candidates; std::size_t count=0;
    for (const auto& card:catalog()) if (match_guardian_payload_supported(selected.guardian,card.id)) candidates[count++]=card.id;
    require(count>0,"empty guardian base candidate set");
    selected.models.reserve(visible_hand_capacity);
    selected.models.push_back(candidates[bounded(rng,count)]);
    std::vector<VisibleSelection> prefix; prefix.reserve(visible_hand_capacity);
    prefix.push_back({selected.models.front(),false});
    while (selected.models.size()<visible_hand_capacity) {
        count=0;
        for (const auto& card:catalog()) if (match_supported(card.id) && payload_domain(selected.guardian,card)
            && card.ability!=Ability::exchange && card.ability!=Ability::sell && card.ability!=Ability::buy && card.ability!=Ability::danger
            && can_append_main(prefix,{card.id,false})) candidates[count++]=card.id;
        if (!count || bounded(rng,2)==0) break;
        const auto model=candidates[bounded(rng,count)]; selected.models.push_back(model); prefix.push_back({model,false});
    }
    const bool hostile=attack_power(selected.models)>0 || targets_enemy_by_default(selected.models.front());
    std::array<PlayerId,player_capacity> targets; count=0;
    for (const auto& target:data.state.players) if (!target.dead && is_enemy(player,target)==hostile) targets[count++]=target.id;
    require(count>0,"guardian payload has no live target candidates");
    selected.target=targets[bounded(rng,count)];
    payload_models(selected);
    chance_rng=rng;
    return selected;
}
EffectFrame payload_effect_for(const MatchSnapshot& s, const GuardianPayloadAction& action, PlayerId target,
                              std::int64_t launch_mp, int random_hp_delta) {
    return effect_for(s,action.models,target,{},launch_mp,random_hp_delta,action.source,action.guardian,false);
}
std::vector<ItemInstance> prepare_item_shuffle(const MatchSnapshot& s, const ItemShuffleAction& action) {
    require(s.phase==MatchPhase::main && !s.terminated && !s.truncated,"item shuffle requires a main boundary");
    require(action.order.size()<=player_capacity*visible_hand_capacity,"oversized shuffle manifest");
    std::vector<ItemSource> keys; std::vector<ItemInstance> copies;
    for (const auto& player:s.state.players) if (!player.dead) for (const auto& item:player.items) {
        keys.push_back({player.id,item.token}); copies.push_back(item);
    }
    require(action.order.size()==keys.size(),"shuffle manifest must cover every alive physical copy");
    std::array<bool,player_capacity*visible_hand_capacity> seen{}; std::vector<ItemInstance> ordered; ordered.reserve(copies.size());
    for (const auto& source:action.order) {
        require(source.owner<=max_wire_integer && source.token<=max_wire_integer,"invalid shuffle source identity");
        const auto found=std::find(keys.begin(),keys.end(),source);
        require(found!=keys.end(),"shuffle source is unknown or dead");
        const auto slot=static_cast<std::size_t>(found-keys.begin());
        require(!seen[slot],"shuffle source used twice"); seen[slot]=true; ordered.push_back(copies[slot]);
    }
    return ordered;
}
bool targeted_phenomenon(CardId model) { return model>=291 && model<=293; }
void phenomenon_shape(const MatchSnapshot& s, const PhenomenonAction& action, bool historical=false) {
    if (!match_phenomenon_supported(action.model)) unsupported("unsupported global phenomenon primitive");
    if (action.model==294 || targeted_phenomenon(action.model)) {
        require(action.recipient.has_value(),"phenomenon requires an explicit recipient");
        const auto& recipient=s.state.players[index(s,*action.recipient)];
        require((historical && targeted_phenomenon(action.model)) || !recipient.dead,"phenomenon recipient is dead");
    } else require(!action.recipient,"this phenomenon takes no recipient");
    if (action.model==296) {
        const auto count=std::count_if(s.state.players.begin(),s.state.players.end(),[](const auto& p){return !p.dead;});
        require(action.guardians.size()==static_cast<std::size_t>(count),"guardian assignment count must equal alive roster");
        for (const auto guardian:action.guardians) require(guardian>=Guardian::mars && guardian<=Guardian::moon,"assignment requires a known present guardian");
    } else require(action.guardians.empty(),"this phenomenon takes no guardian assignments");
}
void prepare_phenomenon(const MatchSnapshot& s, const PhenomenonAction& action) {
    require(s.phase==MatchPhase::main && !s.terminated && !s.truncated,"phenomenon requires a main boundary");
    phenomenon_shape(s,action);
    if (action.model==289) for (const auto& player:s.state.players) if (!player.dead)
        require(player.confusion_count<=static_cast<std::int64_t>(max_wire_integer)-3,"confusion count exhausted");
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

EffectFrame phenomenon_effect_for(const PhenomenonAction& action, std::uint64_t rng) {
    const auto& card=*find_card(action.model); EffectFrame effect;
    effect.target=effect.initial_target=*action.recipient; effect.models={action.model};
    effect.category=Category::phenomena; effect.element=card.element; effect.power=card.atk;
    effect.ability=card.ability; effect.amount=card.ability_value; effect.hit_rate=card.hit_rate;
    effect.redirect_rng=rng; effect.environmental=true;
    return effect;
}
PlayerId sample_bounce_target(const MatchSnapshot& s, std::uint64_t& rng) {
    std::array<PlayerId, player_capacity> alive;
    std::size_t count = 0;
    for (const auto& player : s.state.players) if (!player.dead) alive[count++] = player.id;
    return alive[bounded(rng, count)];
}
struct ResponseResult { std::int64_t defense = 0; Reaction reaction = Reaction::none; bool environmental_cancel = false; };
// Composition is validated separately by the public-client append predicate.
// A filter belongs to the incoming payload; a reflector never contributes ATK/DEF.
ResponseResult apply_response(EffectFrame& effect, std::span<const CardId> models) {
    ResponseResult result;
    for (auto id : models) {
        const auto& c = *find_card(id);
        if (c.ability == Ability::filter_atk_element) effect.element.reset();
        const auto reaction = reaction_for(id, effect.category, effect.element);
        if (reaction == Reaction::reflect) {
            if (effect.environmental) result.environmental_cancel=true;
            else std::swap(effect.source, effect.target);
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
        } else { player.dead = true; player.confusion_count=0; --alive; died.push_back(seat); }
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
struct SuppliedPurchaseSelection {
    const std::optional<ItemToken>* commodity = nullptr;
    bool used = false;
};
std::optional<std::size_t> supplied_purchase_slot(const MatchSnapshot& s, const EffectFrame& effect,
                                               std::optional<ItemToken> commodity) {
    const auto& seller=s.state.players[index(s,effect.target)];
    const bool any=std::any_of(seller.items.begin(),seller.items.end(),[](const auto& item){return !item.used;});
    if(!any) { require(!commodity,"empty seller cannot supply a commodity"); return {}; }
    require(commodity && *commodity<=max_wire_integer,"nonempty seller needs a safe supplied commodity token");
    for(std::size_t slot=0;slot<seller.items.size();++slot) if(seller.items[slot].token==*commodity) {
        require(!seller.items[slot].used,"used miracle is not a purchase commodity"); return slot;
    }
    invalid("supplied purchase commodity is not owned by the current seller");
}
void prepare_purchase_response(const MatchSnapshot& s,const Command& command,const std::vector<CardId>& models,
                               std::optional<ItemToken> commodity) {
    require(s.phase==MatchPhase::response && command.kind==CommandKind::defend && s.batch
        &&s.queue.size()==1 && s.queue.front().ability==Ability::buy,"supplied purchase needs a pending BUY response");
    auto effect=s.queue.front(); const auto response=apply_response(effect,models);
    require(response.reaction==Reaction::none && !response.environmental_cancel,
            "purchase selection is not used by a redirecting or canceled response");
    (void)supplied_purchase_slot(s,effect,commodity);
}
void begin_purchase(MatchSnapshot& s, const EffectFrame& effect, SuppliedPurchaseSelection* supplied=nullptr) {
    auto& seller = s.state.players[index(s, effect.target)];
    std::array<std::size_t, visible_hand_capacity> candidates;
    std::size_t count = 0;
    for (std::size_t i = 0; i < seller.items.size(); ++i) if (!seller.items[i].used) candidates[count++] = i;
    std::optional<std::size_t> supplied_slot;
    if(supplied) {
        require(!supplied->used && supplied->commodity,"supplied BUY selection must execute exactly once");
        supplied_slot=supplied_purchase_slot(s,effect,*supplied->commodity); supplied->used=true;
    }
    if (!count) { s.last_purchase = PurchaseEvent{effect.origin,effect.source,effect.target,0,0,PurchaseReason::empty}; return; }
    const auto slot = supplied ? *supplied_slot : candidates[bounded(s.rng, count)];
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
std::int64_t resolve(MatchSnapshot& s, const EffectFrame& effect, std::int64_t defense, SuppliedPurchaseSelection* purchase_selection=nullptr) {
    auto& target = s.state.players[index(s, effect.target)];
    std::int64_t target_damage = 0;
    if (damaging(effect)) {
        const auto remaining = std::max<std::int64_t>(0, effect.power - defense);
        // Observed self27 applies its full nominal damage then the full heal,
        // even when the intermediate HP clips to0. Extend that alias arithmetic
        // to the same absorption trait, without an intermediate death hook.
        const bool self_absorption = remaining && effect.element != Element::darkness
            && effect.ability == Ability::absorb_hp && effect.source == effect.target;
        if (remaining && effect.element != Element::darkness) {
            if (effect.ability == Ability::absorb_hp && !self_absorption)
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
        if (self_absorption) change_resource(target,Resource::hp,remaining);
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
        case Ability::set_guardian: {
            const bool supplied=s.batch && s.batch->supplied_summon && !effect.environmental
                && !effect.is_counter && !effect.is_death && effect.guardian==Guardian::none
                && effect.origin==s.batch->owner && effect.models==s.batch->models && effect.tokens==s.batch->attack.items;
            const auto guardian=supplied?*s.batch->supplied_summon:static_cast<Guardian>(1+bounded(s.rng,10));
            ++s.guardian_summons;
            change_guardian(s,target,guardian,GuardianChangeReason::summon);
            break;
        }
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
        case Ability::buy: begin_purchase(s, effect,purchase_selection); break;
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
    const auto external = s.batch->guardian_action.has_value() || s.batch->guardian_payload_action.has_value() || s.batch->phenomenon_action.has_value() || s.batch->devil_action.has_value();
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
            if (s.state.players[seat].dead) { s.gifts[seat]=0; break; }
            if (external_grant_pause(s)) {
                s.pending_grant=s.state.players[seat].id; s.actor.reset(); s.phase=MatchPhase::grant;
                s.grant_proof=*s.batch; return;
            }
            gift(s,seat); --s.gifts[seat];
        }
    }
    sync_grant_proof(s); s.batch.reset();
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
    if (!batch.phenomenon_action && !batch.fate_action && !batch.devil_action) {
        const auto origin = batch.guardian_action ? batch.guardian_action->source : batch.guardian_payload_action ? batch.guardian_payload_action->source : batch.owner;
        if (s.state.players[index(s,origin)].dead) batch.cursor=batch.targets.size();
    }
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
        if (batch.fate_action) s.queue.push_back(phenomenon_effect_for(batch.fate_action->outcome,s.rng));
        else if (batch.phenomenon_action) s.queue.push_back(phenomenon_effect_for(*batch.phenomenon_action,s.rng));
        else if (batch.guardian_action) s.queue.push_back(guardian_effect_for(*batch.guardian_action, target, s.rng));
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
    auto& batch=*s.batch;
    if(!batch.guardian_stage_started && batch.automatic_guardian_outcomes) validate_automatic_guardian_entry(s,*batch.automatic_guardian_outcomes);
    batch.guardian_stage_started=true;
    for (;;) {
        if (s.state.players[index(s,batch.owner)].dead) { batch.automatic_payload.reset(); batch.guardian_cursor=s.state.players.size(); return false; }
        if (batch.automatic_payload) {
            auto& launch=*batch.automatic_payload;
            const auto& action=launch.action;
            const auto& base=*find_card(action.models.front());
            const auto hits=base.ability==Ability::attack_twice?2:1;
            if (launch.cursor>=hits || s.state.players[index(s,action.source)].dead || s.state.players[index(s,*action.target)].dead) {
                batch.automatic_payload.reset(); continue;
            }
            ++launch.cursor;
            auto effect=payload_effect_for(s,action,*action.target,launch.launch_mp,launch.random_hp_delta);
            if (effect.hit_rate) {
                effect.forced_hit=has_curse(s.state.players[index(s,*action.target)],Curse::darkcloud);
                if (effect.forced_hit) ++s.chance_forced_hits;
                else { ++s.chance_rolls; if (bounded(s.rng,100)>=static_cast<std::uint64_t>(effect.hit_rate)) continue; ++s.chance_hits; }
            }
            effect.redirect_rng=s.rng;
            if (!damaging(effect) && effect.source==effect.target) {
                resolve(s,effect,0);
                const auto natural=outcome(s.state);
                if (natural.terminal) { finish(s,natural); return true; }
                if (next_action_effect(s)) return true;
                continue;
            }
            s.queue.push_back(std::move(effect)); s.phase=MatchPhase::response; s.actor=*action.target;
            return true;
        }
        if (batch.guardian_cursor==s.state.players.size()) return false;
        const auto seat=batch.guardian_cursor++; const auto& source=s.state.players[seat];
        if (source.dead || source.guardian==Guardian::none || !is_enemy(source,s.state.players[index(s,batch.owner)])) continue;
        ++s.guardian_opportunities;
        const AutomaticGuardianOutcome* supplied=nullptr;
        if(batch.automatic_guardian_outcomes) {
            const auto& outcomes=*batch.automatic_guardian_outcomes;
            const auto it=std::find_if(outcomes.begin(),outcomes.end(),[&](const auto& outcome){return outcome.source==source.id;});
            require(it!=outcomes.end(),"eligible source is absent from supplied guardian plan"); supplied=&*it;
            if(!supplied->model) continue;
            if(find_card(supplied->model)->guardian!=source.guardian
                || (supplied->target && s.state.players[index(s,*supplied->target)].dead)) continue;
        } else if (bounded(s.rng,100)>=25) continue;
        if (!supplied && s.config.local_guardian_payloads && (source.guardian==Guardian::earth || source.guardian==Guardian::moon)) {
            auto action=select_guardian_payload(s,source.id,s.rng);
            const auto ability=find_card(action.models.front())->ability;
            AutomaticGuardianPayload launch{action,source.mp,ability==Ability::boost_hp_or_deal_damage?(bounded(s.rng,2)==0?10:-10):0,0};
            if (ability==Ability::atk_by2x_mp) s.state.players[seat].mp=0;
            ++s.auto_guardian_actions; s.last_auto_guardian_payloads.push_back(std::move(action));
            batch.automatic_payload=std::move(launch); continue;
        }
        const auto& moves=guardian_moves(source.guardian); const auto model=supplied?supplied->model:moves[bounded(s.rng,moves.size())];
        const auto& card=*find_card(model); GuardianAction action{source.id,model,supplied?supplied->target:std::optional<PlayerId>{}};
        if (!supplied && card.ability!=Ability::boost_cp_of_everybody) {
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
}

void reserve_counters(MatchSnapshot& s, const EffectFrame& incoming, const Command& response,
                      std::span<const CardId> models, std::int64_t damage) {
    if (!damaging(incoming) || damage <= 0 || s.state.players[index(s, response.actor)].dead) return;
    for (std::size_t i = 0; i < models.size(); ++i) {
        const auto& card = *find_card(models[i]);
        if (card.ability != Ability::counter_atk && card.ability != Ability::counter2x_atk
            && card.ability != Ability::counter_boost2x_mp && card.ability != Ability::counter_take_cp
            && card.ability != Ability::counter_curse) continue;
        // The UI still permits these armor selections. Without an incoming
        // player source only the MP counter has a defined (self) recipient.
        if (incoming.environmental && card.ability!=Ability::counter_boost2x_mp) continue;
        EffectFrame effect;
        effect.origin = response.actor; effect.source = response.actor;
        effect.target = card.ability==Ability::counter_boost2x_mp ? response.actor : incoming.source;
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
    s.last_tick.reset(); s.last_fog.reset(); s.last_reveals.clear(); s.last_redraws.clear(); s.last_guardian.reset(); s.last_guardian_payload.reset(); s.last_phenomenon.reset(); s.last_item_shuffle.reset(); s.last_fate.reset(); s.last_fate_event.reset(); s.last_devil.reset(); s.last_devil_event.reset(); s.pending_grant.reset(); s.grant_proof.reset(); s.redraw_proof.reset(); s.purchase_proof.reset(); s.summon_proof.reset(); s.automatic_guardian_proof.reset(); s.last_grant.reset();
    s.last_auto_guardians.clear(); s.last_auto_guardian_payloads.clear(); s.last_guardian_changes.clear();
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
    batch.guardian_actions = s.guardian_actions; batch.guardian_payload_actions=s.guardian_payload_actions; batch.phenomenon_actions=s.phenomenon_actions; batch.fate_actions=s.fate_actions; batch.devil_actions=s.devil_actions; batch.grant_resolutions=s.grant_resolutions; batch.grant_artifacts=s.grant_artifacts; batch.grant_devils=s.grant_devils;
    batch.guardian_summons=s.guardian_summons; batch.guardian_opportunities=s.guardian_opportunities; batch.auto_guardian_actions=s.auto_guardian_actions;
    batch.guardian_departure_rolls=s.guardian_departure_rolls; batch.guardian_departures=s.guardian_departures;
    batch.death_bows = s.death_bows; batch.death_created = s.death_created; batch.death_started = s.death_started;
    batch.death_rolls = s.death_rolls; batch.death_hits = s.death_hits; batch.death_forced_hits = s.death_forced_hits;
    return batch;
}
MatchSnapshot snapshot_from_anchor(const ActionBatch& batch, const MatchConfig& config) {
    MatchSnapshot replay; replay.config = config; replay.state = batch.initial_state;
    replay.owner = batch.owner; replay.actor = batch.owner; replay.phase = MatchPhase::main;
    replay.rng = batch.rng; replay.decisions = batch.decisions; replay.next_token = batch.next_token;
    replay.chance_rolls = batch.chance_rolls; replay.chance_hits = batch.chance_hits;
    replay.counter_created = batch.counter_created; replay.counter_started = batch.counter_started;
    replay.counter_rolls = batch.counter_rolls; replay.counter_hits = batch.counter_hits;
    replay.chance_forced_hits = batch.chance_forced_hits; replay.counter_forced_hits = batch.counter_forced_hits;
    replay.revivals = batch.revivals;
    replay.dream_rolls = batch.dream_rolls; replay.dream_masks = batch.dream_masks;
    replay.redraws = batch.redraws; replay.redrawn_items = batch.redrawn_items;
    replay.guardian_actions = batch.guardian_actions; replay.guardian_payload_actions=batch.guardian_payload_actions; replay.phenomenon_actions=batch.phenomenon_actions; replay.fate_actions=batch.fate_actions; replay.devil_actions=batch.devil_actions; replay.grant_resolutions=batch.grant_resolutions; replay.grant_artifacts=batch.grant_artifacts; replay.grant_devils=batch.grant_devils;
    replay.guardian_summons=batch.guardian_summons; replay.guardian_opportunities=batch.guardian_opportunities; replay.auto_guardian_actions=batch.auto_guardian_actions;
    replay.guardian_departure_rolls=batch.guardian_departure_rolls; replay.guardian_departures=batch.guardian_departures;
    replay.death_bows = batch.death_bows; replay.death_created = batch.death_created; replay.death_started = batch.death_started;
    replay.death_rolls = batch.death_rolls; replay.death_hits = batch.death_hits; replay.death_forced_hits = batch.death_forced_hits;
    replay.gifts.assign(batch.initial_state.players.size(), 0); replay.rewards.assign(batch.initial_state.players.size(), 0);
    return replay;
}
void complete_decision(MatchSnapshot& s, bool completed) {
    ++s.decisions; require(s.state.update_count < max_wire_integer, "update counter exhausted"); ++s.state.update_count;
    const auto natural = outcome(s.state);
    if (natural.terminal) finish(s, natural);
    else if (s.decisions == s.config.max_decisions) {
        if (s.purchase) complete_purchase(s, PurchaseReason::cutoff);
        sync_grant_proof(s); s.pending_grant.reset();
        s.truncated = true; s.phase = MatchPhase::finished; s.actor.reset(); s.queue.clear(); s.batch.reset(); s.counters.clear(); s.deaths.clear(); std::fill(s.gifts.begin(), s.gifts.end(), 0);
    } else if (completed) {
        if (!next_action_effect(s)) advance(s);
    }
    sync_grant_proof(s);
}
DecisionRecord decision_record(const MatchSnapshot& s, DecisionSource source) {
    DecisionRecord record;
    record.value.decision=s.decisions+1;
    record.value.gf=s.state.gf;
    record.value.phase=s.phase;
    record.value.source=source;
    return record;
}
void record_command(MatchSnapshot& s, const Command& command, DecisionSource source) {
    auto record=decision_record(s,source);
    auto& value=record.value;
    value.actor=command.actor; value.kind=command.kind;
    const auto& own=s.state.players[index(s,command.actor)];
    for (auto token:command.items) value.faces.push_back(face_model(*instance(own,token)));
    if (command.kind==CommandKind::exchange) { value.allocation=command.allocation; value.target=command.actor; }
    if (command.kind==CommandKind::sell || command.kind==CommandKind::buy) value.target=command.target;
    if (command.kind==CommandKind::play && !value.faces.empty()) {
        const auto& base=*find_card(value.faces.front());
        if (!base.hit_rate && !has_wide(value.faces) && base.ability!=Ability::danger) value.target=command.target;
    }
    if (command.kind==CommandKind::discard || command.kind==CommandKind::sacrifice) {
        for (std::size_t i=0;i<s.state.players.size();++i) {
            const auto board=project_view(s.state,s.state.players[i].id);
            for (const auto& player:board.players) if (player.id==command.actor && player.fogged)
                record.hidden_viewers|=static_cast<std::uint16_t>(1u<<i);
        }
    }
    s.last_decision=std::move(record);
}
void execute_guardian(MatchSnapshot& s, const GuardianAction& action) {
    s.last_decision=decision_record(s,DecisionSource::guardian);
    { auto& value=s.last_decision->value; value.actor=action.source; value.faces={action.model}; value.target=action.target; }
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
    s.last_decision=decision_record(s,DecisionSource::guardian_payload);
    { auto& value=s.last_decision->value; value.actor=action.source; value.faces=action.models; value.target=action.target; }
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
void apply_item_shuffle_body(MatchSnapshot& s, const ItemShuffleAction& action) {
    auto copies=prepare_item_shuffle(s,action);
    require(s.next_token<=max_wire_integer+1 && copies.size()<=max_wire_integer+1-s.next_token,"shuffle token budget exhausted");
    std::vector<std::vector<ItemInstance>> hands(s.state.players.size()); std::size_t cursor=0;
    for (std::size_t seat=0;seat<s.state.players.size();++seat) {
        const auto& player=s.state.players[seat];
        if (player.dead) { hands[seat]=player.items; continue; }
        hands[seat].reserve(player.items.size());
        for (std::size_t slot=0;slot<player.items.size();++slot) {
            auto item=copies[cursor++]; item.token=s.next_token++;
            if (!has_curse(player,Curse::dream)) item.fake_model.reset();
            hands[seat].push_back(std::move(item));
        }
    }
    for (std::size_t seat=0;seat<hands.size();++seat) s.state.players[seat].items=std::move(hands[seat]);
}
void execute_item_shuffle(MatchSnapshot& s, const ItemShuffleAction& action) {
    s.last_decision=decision_record(s,DecisionSource::item_shuffle);
    { auto& value=s.last_decision->value; value.outcome_model=295; }
    clear_events(s); auto anchor=action_anchor(s); apply_item_shuffle_body(s,action);
    s.last_item_shuffle=ItemShuffleRecord{std::move(anchor),action};
    s.last_phenomenon=PhenomenonAction{295,{},{}}; ++s.phenomenon_actions;
    complete_decision(s,false);
}
void apply_global_phenomenon_body(MatchSnapshot& s, const PhenomenonAction& action) {
    std::int64_t collected=0;
    if (action.model==294) for (const auto& player:s.state.players) if (!player.dead) collected+=player.cp;
    std::size_t assignment=0;
    for (auto& player:s.state.players) if (!player.dead) {
        switch (action.model) {
        case 287:
            std::erase_if(player.curses,disease);
            player.curses.insert(std::lower_bound(player.curses.begin(),player.curses.end(),Curse::fever),Curse::fever);
            break;
        case 288: add_curse(player,Curse::fog); break;
        case 289: player.confusion_count+=3; break;
        case 290: player.hp=1; break;
        case 294: player.cp=0; break;
        case 296: change_guardian(s,player,action.guardians[assignment++],GuardianChangeReason::phenomenon); break;
        default: throw std::logic_error("validated phenomenon table changed");
        }
    }
    if (action.model==294) change_resource(s.state.players[index(s,*action.recipient)],Resource::cp,collected);
}
void execute_phenomenon(MatchSnapshot& s, const PhenomenonAction& action) {
    s.last_decision=decision_record(s,DecisionSource::phenomenon);
    { auto& value=s.last_decision->value; value.outcome_model=action.model; value.target=action.recipient; }
    prepare_phenomenon(s,action); clear_events(s);
    if (targeted_phenomenon(action.model)) {
        auto batch=action_anchor(s); batch.phenomenon_action=action; batch.models={action.model};
        batch.tick_processed=true; batch.chance=find_card(action.model)->hit_rate>0; batch.targets={*action.recipient};
        s.batch=std::move(batch); s.last_phenomenon=action; ++s.phenomenon_actions;
        const auto completed=!next_action_effect(s); complete_decision(s,completed); return;
    }
    s.last_phenomenon=action; ++s.phenomenon_actions;
    apply_global_phenomenon_body(s,action);
    // These six primitives never create HP0/death or pending work. Keep the
    // ordinary owner and complete only the external decision/cutoff boundary.
    complete_decision(s,false);
}
std::size_t prepare_fate_source(const MatchSnapshot& s, PlayerId actor, ItemToken token) {
    require(s.phase==MatchPhase::main && !s.terminated && !s.truncated && s.actor==actor && s.owner==actor,"fate needs the main owner");
    const auto seat=index(s,actor); const auto& player=s.state.players[seat]; const auto* item=instance(player,token);
    require(!player.dead && item && item->model==207 && !item->used && !item->fake_model,"fate requires one actual unused unmasked owned207");
    return seat;
}
void prepare_fate(const MatchSnapshot& s, const FateAction& action) {
    const auto seat=prepare_fate_source(s,action.actor,action.token);
    require((action.outcome.model==295)==action.shuffle.has_value(),"only295 requires a shuffle payload");
    auto after=s;
    if (after.state.players[seat].confusion_count>0) --after.state.players[seat].confusion_count;
    consume(after,seat,{action.token},false);
    if (action.shuffle) {
        require(!action.outcome.recipient && action.outcome.guardians.empty(),"shuffle outcome has inactive generic fields");
        (void)prepare_item_shuffle(after,*action.shuffle);
    } else prepare_phenomenon(after,action.outcome);
}
void execute_fate(MatchSnapshot& s, const FateAction& action) {
    s.last_decision=decision_record(s,DecisionSource::fate);
    { auto& value=s.last_decision->value; value.actor=action.actor; value.faces={207}; value.outcome_model=action.outcome.model; }
    prepare_fate(s,action); clear_events(s); auto anchor=action_anchor(s); auto batch=anchor;
    batch.fate_action=action; batch.attack={action.actor,CommandKind::play,{action.token},{}};
    batch.models={action.outcome.model}; batch.confusion_command=s.state.players[index(s,action.actor)].confusion_count>0;
    if (batch.confusion_command) --s.state.players[index(s,action.actor)].confusion_count;
    consume(s,index(s,action.actor),{action.token},false);
    s.last_fate=FateRecord{std::move(anchor),action,{}}; s.last_fate_event=FateEvent{action.actor,action.outcome.model};
    s.last_phenomenon=action.outcome; ++s.fate_actions;
    bool completed=true;
    if (targeted_phenomenon(action.outcome.model)) {
        batch.targets={*action.outcome.recipient}; batch.chance=find_card(action.outcome.model)->hit_rate>0;
        s.batch=std::move(batch); completed=!next_action_effect(s);
    } else {
        s.batch=std::move(batch);
        if (action.shuffle) apply_item_shuffle_body(s,*action.shuffle);
        else apply_global_phenomenon_body(s,action.outcome);
    }
    complete_decision(s,completed);
}
void devil_shape(const MatchSnapshot& s, const DevilAction& action) {
    if (!match_devil_supported(action.model)) unsupported("unsupported devil model");
    const auto& target=s.state.players[index(s,action.target)]; require(!target.dead,"devil recipient is dead");
    require(action.removed.size()<=2,"oversized devil removal manifest");
    if (action.model==244) require(action.resource && static_cast<unsigned>(*action.resource)<=2,"fairy requires an explicit resource");
    else require(!action.resource,"only fairy accepts a resource");
    if (action.model==243) {
        require(action.removed.size()==std::min<std::size_t>(2,target.items.size()),"trickster needs exactly two available physical copies");
        for (std::size_t i=0;i<action.removed.size();++i)
            require(action.removed[i]<=max_wire_integer && instance(target,action.removed[i])
                && (!i || action.removed[i]!=action.removed[0]),"unknown or duplicate devil removal token");
    } else require(action.removed.empty(),"only trickster accepts removed copies");
}
void prepare_devil(const MatchSnapshot& s, const DevilAction& action) {
    require(s.phase==MatchPhase::main && !s.terminated && !s.truncated,"devil effect requires a main boundary"); devil_shape(s,action);
}
DevilEvent apply_devil_body(MatchSnapshot& s, const DevilAction& action) {
    DevilEvent event{action.target,action.model,{}}; auto& target=s.state.players[index(s,action.target)];
    if (action.model<=242) {
        const auto hp=target.hp; change_resource(target,Resource::hp,-find_card(action.model)->ability_value);
        guardian_damage(s,target,hp-target.hp);
    } else if (action.model==243) {
        for (const auto token:action.removed) {
            auto item=std::find_if(target.items.begin(),target.items.end(),[&](const auto& copy){return copy.token==token;});
            reveal_item(s,target.id,*item); event.removed_models.push_back(item->model); target.items.erase(item);
        }
    } else change_resource(target,*action.resource,find_card(action.model)->ability_value);
    finalize_deaths(s); return event;
}
void execute_devil(MatchSnapshot& s, const DevilAction& action) {
    s.last_decision=decision_record(s,DecisionSource::devil);
    { auto& value=s.last_decision->value; value.target=action.target; value.outcome_model=action.model; }
    prepare_devil(s,action); clear_events(s); auto anchor=action_anchor(s); auto batch=anchor;
    batch.devil_action=action; batch.models={action.model}; batch.tick_processed=true; s.batch=std::move(batch);
    s.last_devil=DevilRecord{std::move(anchor),action,{}}; s.last_devil_event=DevilEvent{action.target,action.model,{}};
    ++s.devil_actions; s.last_devil_event=apply_devil_body(s,action); complete_decision(s,true);
}
// Used by both the live transactional step and privileged journal replay.
void execute(MatchSnapshot& s, const Command& command, bool confusion_command=false,
             const std::vector<RedrawResult>* supplied_redraws=nullptr,
             const std::optional<ItemToken>* supplied_purchase=nullptr,
             std::optional<Guardian> supplied_summon={},
             const std::vector<AutomaticGuardianOutcome>* supplied_automatic_guardians=nullptr) {
    const auto models = prepare(s, command,confusion_command); const auto seat = index(s, command.actor);
    if(supplied_redraws) prepare_redraw_response(s,command,models,*supplied_redraws);
    if(supplied_purchase) prepare_purchase_response(s,command,models,*supplied_purchase);
    if(supplied_summon) { require(!confusion_command,"supplied summon is an ordinary main command"); prepare_summon_result(s,command,models,*supplied_summon); }
    if(supplied_automatic_guardians) prepare_automatic_guardian_response(s,*supplied_automatic_guardians);
    const auto source=confusion_command?DecisionSource::confusion
        :supplied_redraws?DecisionSource::redraw_response
        :supplied_purchase?DecisionSource::purchase_response
        :supplied_summon?DecisionSource::summon_result
        :supplied_automatic_guardians?DecisionSource::automatic_guardian_response:DecisionSource::player;
    record_command(s,command,source);
    SuppliedPurchaseSelection purchase_selection{supplied_purchase,false};
    if (s.phase == MatchPhase::main) clear_events(s);
    else if (s.last_fate && s.batch && s.batch->fate_action) s.last_fate->responses.push_back(command);
    else if (s.last_devil && s.batch && s.batch->devil_action) s.last_devil->responses.push_back(command);
    bool completed = false;
    if (s.phase == MatchPhase::main) {
        auto batch = action_anchor(s); batch.attack = command; batch.models = models;
        batch.confusion_command=confusion_command; batch.supplied_summon=supplied_summon; s.batch = std::move(batch);
        if(supplied_summon) s.summon_proof=*s.batch;
        if (confusion_command) --s.state.players[seat].confusion_count;
    }
    if (s.phase == MatchPhase::purchase) {
        s.batch->responses.push_back(command);
        complete_purchase(s, command.kind == CommandKind::accept ? PurchaseReason::bought : PurchaseReason::declined);
        completed = true;
    } else if (s.phase == MatchPhase::response) {
        const auto response_index=s.batch->responses.size();
        s.batch->responses.push_back(command);
        if(supplied_automatic_guardians) {
            s.batch->automatic_guardian_outcomes=*supplied_automatic_guardians;
            s.batch->resolved_automatic_guardians.push_back({response_index,*supplied_automatic_guardians});
            s.automatic_guardian_proof=*s.batch;
        }
        if(supplied_redraws) {
            s.batch->resolved_redraws.push_back({response_index,*supplied_redraws});
            s.redraw_proof=*s.batch;
        }
        if(supplied_purchase) {
            s.batch->resolved_purchases.push_back({response_index,*supplied_purchase});
            s.purchase_proof=*s.batch;
        }
        auto& effect = s.queue.front();
        const auto response = apply_response(effect, models);
        change_resource(s.state.players[seat], Resource::mp, -magic_cost(models));
        consume(s, seat, command.items, false);
        std::size_t redraw_cursor=0;
        for (auto model : models) {
            const auto& card = *find_card(model);
            if (card.ability == Ability::self_curse || card.ability == Ability::self_curse_and_redraw) {
                add_curse(s.state.players[seat], card.curse);
                if (card.ability == Ability::self_curse_and_redraw) {
                    if(supplied_redraws) redraw_supplied(s,seat,(*supplied_redraws)[redraw_cursor++]);
                    else redraw(s, seat);
                }
            }
        }
        if (response.environmental_cancel) {
            s.queue.clear(); completed=true;
        } else if (response.reaction == Reaction::reflect || response.reaction == Reaction::bounce) {
            if (response.reaction == Reaction::bounce) effect.target = sample_bounce_target(s, s.rng);
            effect.redirects.push_back({command.actor, models, command.items, effect.target});
            if (s.state.players[index(s, effect.target)].dead) { s.queue.clear(); completed = true; }
            else s.actor = effect.target;
        } else {
            if (response.reaction == Reaction::none) {
                const auto damage = resolve(s, effect, response.defense,supplied_purchase?&purchase_selection:nullptr);
                reserve_counters(s, effect, command, models, damage);
            }
            s.queue.clear(); completed = !s.purchase;
        }
    } else if (command.kind == CommandKind::discard || command.kind==CommandKind::sacrifice) {
        consume(s, seat, command.items, true);
        if (command.kind==CommandKind::sacrifice) for (std::size_t i=0;i<command.items.size();++i) request_gift(s,seat);
        completed = true;
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
            s.last_decision->value.target=target;
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
            if (!mortar) target = has_wide(models) ? enemy_targets(s).front() : sample_bounce_target(s, s.rng);
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
        } else if (command.kind == CommandKind::play && command.target == s.owner && target == s.owner
            && ((base.category == Category::weapons && (base.ability == Ability::none || base.ability == Ability::bounce_weapon || (base.id == 34 && base.ability == Ability::bounce_miracle) || base.ability == Ability::reflect_weapon || base.ability == Ability::reflect_miracle || base.ability == Ability::block_miracle || base.ability == Ability::set_element || base.ability == Ability::cut_cost
                    || base.ability == Ability::absorb_hp || base.ability == Ability::attack_twice
                    || base.ability == Ability::atk_by2x_mp || base.ability == Ability::deal_same_damage
                    || (base.ability == Ability::add_curse_on_damage && (base.curse == Curse::dream || base.curse == Curse::darkcloud || base.curse == Curse::cold || base.curse == Curse::hell || base.curse == Curse::fog || base.curse == Curse::flash))))
                || (base.category == Category::miracles && (base.ability == Ability::none || base.ability == Ability::absorb_hp)))
            && !base.hit_rate && !has_wide(models)) {
            // Extend the observed self-play routing to this explicit allowlist;
            // special body arithmetic remains the existing experimental rule.
            // Pay/consume once and keep the original launch payload even after
            // MP changes or revival. Redirects and external entries stay separate.
            change_resource(s.state.players[seat], Resource::mp, -play_cost); consume_launch();
            const unsigned hits = base.ability == Ability::attack_twice ? 2 : 1;
            for (unsigned hit = 0; hit < hits; ++hit) {
                const auto fresh = original;
                resolve(s, fresh, 0);
                if (outcome(s.state).terminal || s.state.players[seat].dead) break;
            }
            completed = true;
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
    require(!supplied_purchase || purchase_selection.used,"supplied purchase selection was not executed");
    complete_decision(s, completed);
    if(supplied_automatic_guardians) require(s.automatic_guardian_proof && s.automatic_guardian_proof->guardian_stage_started,"supplied guardian stage was not reached by this response");
}
void prepare_grant(const MatchSnapshot& s, const GrantAction& action) {
    require(s.phase==MatchPhase::grant && !s.actor && s.pending_grant==action.target && s.batch
        && external_grant_pause(s) && !s.terminated && !s.truncated,"grant needs the current environment recipient");
    const auto seat=index(s,action.target); require(!s.state.players[seat].dead && s.gifts[seat]>0,"missing live gift debt");
    if (!match_grant_supported(action.model)) unsupported("unsupported grant outcome");
    if (!match_devil_supported(action.model)) {
        require(!action.resource && action.removed.empty(),"ordinary grant has inactive devil fields");
        if (action.model) {
            if (!gift_model_supported(action.model,s.config.include_fate)) unsupported("explicit ordinary model is outside the configured gift pool");
            if (has_curse(s.state.players[seat],Curse::dream)) unsupported("explicit ordinary grant cannot infer a Dream disguise");
        }
    } else {
        if (!devil_grants_enabled(s)) unsupported("devil grant requires the apocalypse external-grants mode");
        devil_shape(s,action);
    }
}
void prepare_artifact_grant(const MatchSnapshot& s, const ArtifactGrantAction& action) {
    // Reuse only the ordinary sentinel's phase/debt checks, never its chooser.
    prepare_grant(s,GrantAction{action.target,0,{},{}});
    if (!match_artifact_grant_supported(action.model) || !gift_model_supported(action.model,s.config.include_fate))
        unsupported("artifact grant model is outside the configured gift pool");
    if (action.fake_model) {
        const auto& recipient=s.state.players[index(s,action.target)];
        if (!has_curse(recipient,Curse::dream) || !match_dream_disguise_supported(action.model,*action.fake_model))
            unsupported("artifact grant needs a supported Dream face");
    }
}
void deliver_artifact(MatchSnapshot& s, std::size_t seat, CardId model, std::optional<CardId> face = {}) {
    auto& recipient=s.state.players[seat];
    require(recipient.items.size()<s.config.hand_limit,"gift has no reserved display slot");
    require(s.next_token<=max_wire_integer,"scoped item token exhausted");
    recipient.items.push_back({s.next_token++,model,face,false});
}
void execute_grant(MatchSnapshot& s, const GrantAction& action) {
    s.last_decision=decision_record(s,DecisionSource::grant);
    { auto& value=s.last_decision->value; value.target=action.target; if (match_devil_supported(action.model)) value.outcome_model=action.model; }
    prepare_grant(s,action); const auto seat=index(s,action.target);
    s.batch->grants.push_back({s.batch->responses.size(),action});
    s.pending_grant.reset(); s.phase=MatchPhase::main; s.actor=s.owner; ++s.grant_resolutions;
    if (!match_devil_supported(action.model)) {
        if (!action.model) gift(s,seat);
        else deliver_artifact(s,seat,action.model);
        --s.gifts[seat]; ++s.grant_artifacts; s.last_grant=DevilEvent{action.target,0,{}};
    } else {
        ++s.grant_devils; s.last_grant=apply_devil_body(s,action);
        if (s.state.players[seat].dead) s.gifts[seat]=0;
    }
    complete_decision(s,true);
}
void execute_artifact_grant(MatchSnapshot& s, const ArtifactGrantAction& action) {
    s.last_decision=decision_record(s,DecisionSource::artifact_grant);
    { auto& value=s.last_decision->value; value.target=action.target; }
    prepare_artifact_grant(s,action); const auto seat=index(s,action.target);
    s.batch->grants.push_back({s.batch->responses.size(),action});
    s.pending_grant.reset(); s.phase=MatchPhase::main; s.actor=s.owner; ++s.grant_resolutions;
    deliver_artifact(s,seat,action.model,action.fake_model);
    --s.gifts[seat]; ++s.grant_artifacts; s.last_grant=DevilEvent{action.target,0,{}};
    complete_decision(s,true);
}
void replay_launch(MatchSnapshot& replay,const ActionBatch& journal) {
    const auto sources=static_cast<unsigned>(journal.fate_action.has_value())+static_cast<unsigned>(journal.devil_action.has_value())
        +static_cast<unsigned>(journal.phenomenon_action.has_value())+static_cast<unsigned>(journal.guardian_action.has_value())
        +static_cast<unsigned>(journal.guardian_payload_action.has_value());
    require(sources<=1 && (!journal.supplied_summon || sources==0),"mixed original grant action sources");
    if (journal.fate_action) execute_fate(replay,*journal.fate_action);
    else if (journal.devil_action) execute_devil(replay,*journal.devil_action);
    else if (journal.phenomenon_action) execute_phenomenon(replay,*journal.phenomenon_action);
    else if (journal.guardian_payload_action) execute_guardian_payload(replay,*journal.guardian_payload_action);
    else if (journal.guardian_action) execute_guardian(replay,*journal.guardian_action);
    else execute(replay,journal.attack,journal.confusion_command,nullptr,nullptr,journal.supplied_summon);
}
void replay_response(MatchSnapshot& replay,const ActionBatch& journal,std::size_t response_index) {
    const std::vector<RedrawResult>* supplied=nullptr;
    for(const auto& entry:journal.resolved_redraws) if(entry.response_index==response_index) {
        supplied=&entry.results; break;
    }
    const std::optional<ItemToken>* purchase=nullptr;
    for(const auto& entry:journal.resolved_purchases) if(entry.response_index==response_index) {
        purchase=&entry.commodity; break;
    }
    const std::vector<AutomaticGuardianOutcome>* automatic=nullptr;
    for(const auto& entry:journal.resolved_automatic_guardians) if(entry.response_index==response_index) { automatic=&entry.outcomes; break; }
    execute(replay,journal.responses[response_index],false,supplied,purchase,{},automatic);
}
void validate_snapshot(const MatchSnapshot& s) {
    require(s.version == match_snapshot_version, "unknown match snapshot version"); config_valid(s.config); closed_state(s.state, s.config);
    const auto n = s.state.players.size();
    if (s.last_decision) {
        const auto& record=*s.last_decision; const auto& value=record.value;
        require(value.decision>0 && value.decision==s.decisions && value.gf>=0
            && value.phase!=MatchPhase::finished && static_cast<unsigned>(value.phase)<=4
            && static_cast<unsigned>(value.source)<=static_cast<unsigned>(DecisionSource::automatic_guardian_response)
            && !value.faces_hidden && value.faces.size()<=visible_hand_capacity
            && (record.hidden_viewers>>n)==0,"invalid last decision record");
        if(value.actor) (void)index(s,*value.actor);
        if(value.target) (void)index(s,*value.target);
        if(value.kind) require(static_cast<unsigned>(*value.kind)<=static_cast<unsigned>(CommandKind::sacrifice),"invalid decision kind");
        for(auto face:value.faces) require(find_card(face),"invalid decision face");
        if(value.outcome_model) require(find_card(*value.outcome_model),"invalid decision outcome");
        require(!record.hidden_viewers || (value.kind==CommandKind::discard || value.kind==CommandKind::sacrifice),"invalid decision visibility");
        if(value.allocation) {
            const auto& a=*value.allocation;
            require(value.kind==CommandKind::exchange && a.hp>=0 && a.hp<=99 && a.mp>=0 && a.mp<=99 && a.cp>=0 && a.cp<=99,"invalid decision allocation");
        }
        switch(value.source) {
        case DecisionSource::player: case DecisionSource::confusion:
        case DecisionSource::redraw_response: case DecisionSource::purchase_response:
        case DecisionSource::summon_result: case DecisionSource::automatic_guardian_response:
            require(value.actor && value.kind && !value.outcome_model,"invalid command decision");
            if(value.kind==CommandKind::pray || value.kind==CommandKind::accept || value.kind==CommandKind::decline)
                require(value.faces.empty() && !value.target && !value.allocation,"invalid itemless decision");
            if(value.kind==CommandKind::discard || value.kind==CommandKind::sacrifice)
                require(!value.target && !value.faces.empty(),"invalid discard decision");
            break;
        case DecisionSource::fate:
            require(value.actor && !value.kind && value.faces==std::vector<CardId>{207} && !value.target
                && value.outcome_model && *value.outcome_model>=287 && *value.outcome_model<=296,"invalid fate decision"); break;
        case DecisionSource::devil: case DecisionSource::grant: case DecisionSource::artifact_grant:
            require(!value.actor && !value.kind && value.faces.empty() && value.target
                && (!value.outcome_model || match_devil_supported(*value.outcome_model))
                && (value.source!=DecisionSource::devil || value.outcome_model)
                && (value.source!=DecisionSource::artifact_grant || !value.outcome_model),"invalid grant/devil decision"); break;
        case DecisionSource::guardian: case DecisionSource::guardian_payload:
            require(value.actor && !value.kind && !value.faces.empty() && !value.outcome_model,"invalid guardian decision"); break;
        case DecisionSource::phenomenon: case DecisionSource::item_shuffle:
            require(!value.actor && !value.kind && value.faces.empty() && value.outcome_model
                && *value.outcome_model>=287 && *value.outcome_model<=296
                && (value.source!=DecisionSource::item_shuffle || (*value.outcome_model==295 && !value.target)),"invalid phenomenon decision"); break;
        }
        require(!value.allocation || value.kind==CommandKind::exchange,"inactive decision allocation");
    }
    const auto validate_automatic_payload=[&](const ActionBatch& batch) {
        if (!batch.automatic_payload) return;
        const auto& launch=*batch.automatic_payload;
        payload_models(launch.action);
        const auto& base=*find_card(launch.action.models.front());
        require(s.config.local_guardian_payloads && batch.guardian_stage_started && batch.tick_processed
            && !batch.guardian_action && !batch.guardian_payload_action && !batch.phenomenon_action && !batch.devil_action
            && batch.guardian_cursor==index(s,launch.action.source)+1 && launch.launch_mp>=0 && launch.launch_mp<=99
            && launch.cursor>0 && launch.cursor<=(base.ability==Ability::attack_twice?2:1)
            && (base.ability==Ability::boost_hp_or_deal_damage ? (launch.random_hp_delta==10 || launch.random_hp_delta==-10) : launch.random_hp_delta==0)
            && !s.last_auto_guardian_payloads.empty() && s.last_auto_guardian_payloads.back()==launch.action,"invalid automatic payload launch");
        (void)index(s,*launch.action.target);
    };
    if (s.batch) validate_automatic_payload(*s.batch);
    if (s.grant_proof) validate_automatic_payload(*s.grant_proof);
    if (s.redraw_proof) validate_automatic_payload(*s.redraw_proof);
    if (s.purchase_proof) validate_automatic_payload(*s.purchase_proof);
    if (s.summon_proof) validate_automatic_payload(*s.summon_proof);
    if (s.automatic_guardian_proof) validate_automatic_payload(*s.automatic_guardian_proof);
    for(const auto* journal:{&s.batch,&s.grant_proof,&s.redraw_proof,&s.purchase_proof,&s.summon_proof,&s.automatic_guardian_proof})if(*journal) {
        if(!(**journal).resolved_redraws.empty()) require(s.redraw_proof.has_value(),"supplied redraw outcomes have no proof");
        if(!(**journal).resolved_purchases.empty()) require(s.purchase_proof.has_value(),"supplied purchase outcomes have no proof");
        if((**journal).supplied_summon) require(s.summon_proof.has_value(),"supplied summon result has no proof");
        if((**journal).automatic_guardian_outcomes || !(**journal).resolved_automatic_guardians.empty()) require(s.automatic_guardian_proof.has_value(),"automatic guardian outcomes have no proof");
    }
    require(s.gifts.size() == n && s.rewards.size() == n && s.decisions <= s.config.max_decisions, "invalid snapshot dimensions/counters");
    require(s.chance_hits <= s.chance_rolls && s.chance_rolls <= 2 * player_capacity * s.decisions, "invalid chance attempt counters");
    require(s.chance_forced_hits <= 2 * player_capacity * s.decisions - s.chance_rolls, "invalid forced chance count");
    require(s.counter_hits <= s.counter_rolls && s.counter_rolls <= s.counter_created
        && s.counter_hits <= s.counter_started && s.counter_started <= s.counter_created
        && s.counter_created <= s.config.hand_limit * s.decisions, "invalid counter statistics");
    require(s.counter_forced_hits <= s.counter_started - s.counter_hits, "invalid forced counter count");
    require(s.revivals <= player_capacity * s.decisions, "invalid revival count");
    require(s.guardian_actions <= s.decisions && s.guardian_payload_actions<=s.decisions-s.guardian_actions
        && s.phenomenon_actions<=s.decisions-s.guardian_actions-s.guardian_payload_actions
        && s.fate_actions<=s.decisions-s.guardian_actions-s.guardian_payload_actions-s.phenomenon_actions
        && s.devil_actions<=s.decisions-s.guardian_actions-s.guardian_payload_actions-s.phenomenon_actions-s.fate_actions
        && s.grant_resolutions<=s.decisions-s.guardian_actions-s.guardian_payload_actions-s.phenomenon_actions-s.fate_actions-s.devil_actions,"invalid external activation counts");
    require(s.grant_artifacts<=s.grant_resolutions && s.grant_devils==s.grant_resolutions-s.grant_artifacts
        && (!s.grant_resolutions || external_grant_pause(s))
        && (!s.grant_devils || devil_grants_enabled(s)),"invalid grant outcome counters");
    require(s.pending_grant.has_value()==(s.phase==MatchPhase::grant)
        && (!(s.pending_grant || s.last_grant) || s.grant_proof.has_value()),"grant phase or notice lacks its journal");
    if (s.grant_proof || s.redraw_proof || s.purchase_proof || s.summon_proof || s.automatic_guardian_proof) {
        if(s.grant_proof) require(s.config.external_grants || s.config.external_artifacts,"grant proof without external grant mode");
        const auto& journal=s.purchase_proof?*s.purchase_proof:s.redraw_proof?*s.redraw_proof:s.summon_proof?*s.summon_proof:s.automatic_guardian_proof?*s.automatic_guardian_proof:*s.grant_proof;
        for(const auto* proof:{&s.grant_proof,&s.redraw_proof,&s.purchase_proof,&s.summon_proof,&s.automatic_guardian_proof})if(*proof)
            require(**proof==journal,"external result journals differ");
        require(s.redraw_proof.has_value()==!journal.resolved_redraws.empty(),"supplied redraw proof/outcomes mismatch");
        require(s.purchase_proof.has_value()==!journal.resolved_purchases.empty(),"supplied purchase proof/outcomes mismatch");
        require(s.summon_proof.has_value()==journal.supplied_summon.has_value(),"supplied summon proof/result mismatch");
        require(s.automatic_guardian_proof.has_value()==!journal.resolved_automatic_guardians.empty()
            && journal.automatic_guardian_outcomes.has_value()==!journal.resolved_automatic_guardians.empty(),"automatic guardian proof/plan mismatch");
        require(journal.resolved_automatic_guardians.size()<=1,"multiple supplied guardian stages in one batch");
        if(journal.automatic_guardian_outcomes) {
            const auto& entry=journal.resolved_automatic_guardians.front();
            require(entry.response_index<journal.responses.size() && entry.outcomes==*journal.automatic_guardian_outcomes
                && journal.guardian_stage_started && !s.config.local_guardian_payloads,"invalid supplied guardian stage journal");
            automatic_guardian_plan_shape(s,entry.outcomes);
        }
        require(s.grant_proof || journal.grants.empty(),"grant entries without grant proof");
        require(journal.initial_state.players.size()==n && journal.decisions<s.decisions
            && journal.attack.items.size()<=s.config.hand_limit && journal.models.size()<=visible_hand_capacity
            && journal.targets.size()<=2*(n-1) && journal.responses.size()<s.config.max_decisions && journal.grants.size()<s.config.max_decisions
            && journal.responses.size()+journal.grants.size()==s.decisions-journal.decisions-1,"invalid original grant journal shape");
        for (const auto& player:journal.initial_state.players) require(player.items.size()<=s.config.hand_limit,"oversized grant anchor inventory");
        for (const auto& response:journal.responses) require(response.items.size()<=s.config.hand_limit,"oversized grant journal response");
        require(journal.resolved_redraws.size()<=journal.responses.size(),"too many supplied redraw response entries");
        std::optional<std::size_t> previous_redraw;
        for(const auto& entry:journal.resolved_redraws) {
            require(entry.response_index<journal.responses.size() && (!previous_redraw || entry.response_index>*previous_redraw)
                &&!entry.results.empty() && entry.results.size()<=s.config.hand_limit,"invalid supplied redraw response index/count");
            for(const auto& result:entry.results) redraw_result_shape(s,result);
            previous_redraw=entry.response_index;
        }
        require(journal.resolved_purchases.size()<=journal.responses.size(),"too many supplied purchase entries");
        std::optional<std::size_t> previous_purchase;
        for(const auto& entry:journal.resolved_purchases) {
            require(entry.response_index<journal.responses.size() && (!previous_purchase || entry.response_index>*previous_purchase)
                &&(!entry.commodity || *entry.commodity<=max_wire_integer),"invalid supplied purchase index/token");
            previous_purchase=entry.response_index;
        }
        for (const auto& entry:journal.grants) {
            require(entry.responses_before<=journal.responses.size(),"oversized grant resolution entry");
            if (const auto* ordinary=std::get_if<GrantAction>(&entry.action))
                require(ordinary->removed.size()<=2,"oversized grant resolution selection");
        }
        if (journal.fate_action) { require(journal.fate_action->outcome.guardians.size()<=player_capacity,"oversized original fate array");
            if (journal.fate_action->shuffle) require(journal.fate_action->shuffle->order.size()<=player_capacity*visible_hand_capacity,"oversized original fate shuffle"); }
        if (journal.devil_action) require(journal.devil_action->removed.size()<=2,"oversized original devil selection");
        if (journal.phenomenon_action) require(journal.phenomenon_action->guardians.size()<=player_capacity,"oversized original phenomenon array");
        if (journal.guardian_payload_action) require(journal.guardian_payload_action->models.size()<=visible_hand_capacity,"oversized original guardian payload");
        closed_state(journal.initial_state,s.config); auto replay=snapshot_from_anchor(journal,s.config); validate_snapshot(replay); replay_launch(replay,journal);
        std::size_t response_cursor=0;
        for (const auto& entry:journal.grants) {
            require(entry.responses_before>=response_cursor,"grant journal response prefix moved backwards");
            while (response_cursor<entry.responses_before) replay_response(replay,journal,response_cursor++);
            std::visit([&](const auto& action) {
                if constexpr (std::is_same_v<std::decay_t<decltype(action)>,GrantAction>) execute_grant(replay,action);
                else execute_artifact_grant(replay,action);
            },entry.action);
        }
        while (response_cursor<journal.responses.size()) replay_response(replay,journal,response_cursor++);
        require(replay==s,"external result journal does not reproduce the complete snapshot"); return;
    }
    require(s.last_fate.has_value()==s.last_fate_event.has_value(),"fate notice and launch proof must coexist");
    require(s.last_devil.has_value()==s.last_devil_event.has_value(),"devil notice and launch proof must coexist");
    if (s.last_devil) {
        const auto& record=*s.last_devil; const auto& anchor=record.anchor;
        require(!s.last_fate && !s.last_item_shuffle && anchor.initial_state.players.size()==n
            && record.action.removed.size()<=2 && s.last_devil_event->removed_models.size()<=2
            && record.responses.size()<s.config.max_decisions,"invalid devil proof shape");
        for (const auto& player:anchor.initial_state.players) require(player.items.size()<=s.config.hand_limit,"oversized devil anchor hand");
        for (const auto& response:record.responses) require(response.items.size()<=s.config.hand_limit,"oversized devil response");
        closed_state(anchor.initial_state,s.config); auto replay=snapshot_from_anchor(anchor,s.config); validate_snapshot(replay);
        require(action_anchor(replay)==anchor,"noncanonical devil anchor"); execute_devil(replay,record.action);
        for (const auto& response:record.responses) {
            require(replay.batch && replay.batch->devil_action && replay.phase!=MatchPhase::main,"devil journal continues past completion");
            execute(replay,response);
        }
        require(replay==s,"devil proof does not reproduce the full snapshot"); return;
    }
    if (s.last_fate) {
        const auto& record=*s.last_fate; const auto& anchor=record.anchor;
        require(!s.last_item_shuffle && anchor.initial_state.players.size()==n
            && s.last_fate_event==FateEvent{record.action.actor,record.action.outcome.model}
            && record.responses.size()<s.config.max_decisions,"invalid fate proof shape");
        if (record.action.shuffle) require(record.action.shuffle->order.size()<=player_capacity*visible_hand_capacity,"oversized fate shuffle proof");
        require(record.action.outcome.guardians.size()<=player_capacity,"oversized fate assignment proof");
        for (const auto& player:anchor.initial_state.players) require(player.items.size()<=s.config.hand_limit,"oversized fate anchor hand");
        for (const auto& response:record.responses) require(response.items.size()<=s.config.hand_limit,"oversized fate response");
        closed_state(anchor.initial_state,s.config); auto replay=snapshot_from_anchor(anchor,s.config); validate_snapshot(replay);
        require(action_anchor(replay)==anchor,"noncanonical fate anchor"); execute_fate(replay,record.action);
        for (const auto& response:record.responses) {
            require(replay.batch && replay.batch->fate_action && replay.phase!=MatchPhase::main,"fate proof continues past completion");
            execute(replay,response);
        }
        require(replay==s,"fate proof does not reproduce the full snapshot"); return;
    }
    if (s.last_item_shuffle) {
        const auto& record=*s.last_item_shuffle; const auto& anchor=record.anchor;
        require(!s.batch && s.last_phenomenon==PhenomenonAction{295,{},{}}
            && anchor.initial_state.players.size()==n && record.action.order.size()<=player_capacity*visible_hand_capacity,"invalid shuffle proof shape");
        for (const auto& player:anchor.initial_state.players) require(player.items.size()<=s.config.hand_limit,"oversized shuffle anchor inventory");
        closed_state(anchor.initial_state,s.config);
        auto replay=snapshot_from_anchor(anchor,s.config); validate_snapshot(replay);
        require(action_anchor(replay)==anchor,"shuffle proof has noncanonical anchor fields");
        execute_item_shuffle(replay,record.action);
        require(replay==s,"shuffle proof does not reproduce the complete snapshot");
        return;
    }
    if (s.last_guardian_payload) {
        require(s.guardian_payload_actions>0 && s.decisions>0,"invalid guardian payload event");
        payload_models(*s.last_guardian_payload); (void)index(s,s.last_guardian_payload->source); (void)index(s,*s.last_guardian_payload->target);
    }
    require(s.guardian_summons<=player_capacity*s.decisions && s.guardian_opportunities<=player_capacity*s.decisions
        && s.auto_guardian_actions<=s.guardian_opportunities && s.guardian_departures<=s.guardian_departure_rolls
        && s.guardian_departure_rolls<=2*player_capacity*s.decisions,"invalid guardian lifecycle counts");
    require(s.last_auto_guardians.size()+s.last_auto_guardian_payloads.size()<=n
        && s.last_auto_guardians.size()+s.last_auto_guardian_payloads.size()<=s.auto_guardian_actions
        && (s.config.local_guardian_payloads || s.last_auto_guardian_payloads.empty()),"invalid automatic guardian log");
    std::uint16_t automatic_sources=0;
    std::size_t previous_seat=0; bool first_auto=true;
    for (const auto& action:s.last_auto_guardians) {
        require(match_guardian_supported(action.model),"invalid automatic model"); const auto seat=index(s,action.source);
        require(!s.config.local_guardian_payloads || (action.model!=285 && action.model!=286),"canonical Earth/Moon in local automatic mode");
        automatic_sources |= static_cast<std::uint16_t>(1u<<seat);
        require(first_auto || seat>previous_seat,"automatic source order invalid"); first_auto=false; previous_seat=seat;
        if (find_card(action.model)->ability==Ability::boost_cp_of_everybody) require(!action.target,"automatic global target");
        else { require(action.target.has_value(),"automatic target missing"); (void)index(s,*action.target); }
    }
    first_auto=true; previous_seat=0;
    for (const auto& action:s.last_auto_guardian_payloads) {
        payload_models(action); const auto seat=index(s,action.source);
        require((first_auto || seat>previous_seat) && !(automatic_sources&(1u<<seat)),"automatic payload source order invalid");
        first_auto=false; previous_seat=seat;
        automatic_sources |= static_cast<std::uint16_t>(1u<<seat);
        (void)index(s,*action.target);
    }
    require(s.last_guardian_changes.size()<=player_capacity*s.decisions,"oversized guardian change log");
    for (const auto& event:s.last_guardian_changes) {
        (void)index(s,event.player);
        require(static_cast<unsigned>(event.before)<=10 && static_cast<unsigned>(event.after)<=10
            && static_cast<unsigned>(event.reason)<=2 && (event.hidden_viewers>>n)==0,"invalid guardian change");
        if (event.reason==GuardianChangeReason::summon) require(event.after!=Guardian::none,"empty summoned guardian");
        else if (event.reason==GuardianChangeReason::departure) require(event.before!=Guardian::none && event.after==Guardian::none,"invalid departed guardian");
        else require(event.after!=Guardian::none && s.last_phenomenon && s.last_phenomenon->model==296,"invalid phenomenon guardian change");
    }
    if (s.last_phenomenon) {
        const auto& action=*s.last_phenomenon; phenomenon_shape(s,action,targeted_phenomenon(action.model));
        require(s.phenomenon_actions>0 && s.decisions>0 && !s.last_guardian && !s.last_guardian_payload
            && s.last_auto_guardians.empty() && s.last_auto_guardian_payloads.empty() && !s.last_tick && !s.last_fog,"invalid phenomenon announcement");
        if (!targeted_phenomenon(action.model)) {
            require(!s.terminated
                && (s.phase==MatchPhase::main || s.truncated) && !s.batch
                && !s.last_guardian && !s.last_guardian_payload && s.last_auto_guardians.empty() && s.last_auto_guardian_payloads.empty()
                && !s.last_tick && !s.last_fog && s.last_reveals.empty() && s.last_redraws.empty()
                && !s.last_sale && !s.last_purchase && !s.last_danger && !s.last_removal,"invalid completed phenomenon boundary");
            std::size_t assigned=0;
            for (const auto& player:s.state.players) if (!player.dead) {
                if (action.model==287) require(has_curse(player,Curse::fever),"Fever replacement postcondition missing");
                if (action.model==288) require(has_curse(player,Curse::fog),"global Fog postcondition missing");
                if (action.model==289) require(player.confusion_count>=3,"global confusion increment postcondition missing");
                if (action.model==290) require(player.hp==1,"global HP setter postcondition missing");
                if (action.model==294 && player.id!=*action.recipient) require(player.cp==0,"collection debit postcondition missing");
                if (action.model==296) {
                    require(assigned<s.last_guardian_changes.size(),"missing global guardian change");
                    const auto& event=s.last_guardian_changes[assigned];
                    require(event.player==player.id && event.after==action.guardians[assigned] && player.guardian==event.after
                        && event.reason==GuardianChangeReason::phenomenon && event.hidden_viewers==guardian_hidden_viewers(s,player),"global guardian assignment postcondition mismatch");
                    ++assigned;
                }
            }
            require(s.last_guardian_changes.size()==assigned,"unexpected guardian changes for global primitive");
        }
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
        require(match_dream_disguise_supported(event.model,event.face), "invalid supported reveal models");
    }
    const auto death_bound = player_capacity * (player_capacity - 1) * s.config.hand_limit;
    require(s.death_bows <= player_capacity * s.config.hand_limit && s.death_bows <= s.death_created
        && s.death_created <= (player_capacity - 1) * s.death_bows && s.death_created <= death_bound && s.death_hits <= s.death_rolls
        && s.death_rolls <= s.death_created && s.death_started == s.death_hits + s.death_forced_hits
        && s.death_forced_hits <= s.death_created - s.death_rolls && s.deaths.size() <= s.death_created, "invalid posthumous counts");
    if (s.last_removal) {
        const auto& removal=*s.last_removal;
        const bool automatic=std::any_of(s.last_auto_guardian_payloads.begin(),s.last_auto_guardian_payloads.end(),[&](const auto& a){return a.source==removal.origin && a.models.front()==removal.model;});
        require(s.decisions > 0 && s.phase != MatchPhase::purchase && (automatic || (!s.last_danger && !s.last_sale && !s.last_purchase)), "invalid removal event boundary");
        const auto& event = *s.last_removal;
        (void)index(s, event.origin); (void)index(s, event.source); (void)index(s, event.target);
        require(event.model == 205 || event.model == 206, "invalid removal event model");
        require(event.removed_models.size() <= (event.model == 205 ? 3u : 2u), "oversized removal event");
        for (auto model : event.removed_models)
            require(inventory_supported(model) && (event.model != 206 || find_card(model)->category == Category::miracles), "invalid removed model");
    }
    if (s.last_sale) {
        require(s.decisions > 0 && s.phase != MatchPhase::purchase && !s.last_danger && (!s.last_removal || !s.last_auto_guardian_payloads.empty()) && !s.last_purchase, "invalid sale event boundary");
        const auto& event = *s.last_sale;
        (void)index(s, event.origin); (void)index(s, event.source); (void)index(s, event.target);
        require(inventory_supported(event.model) && event.price == find_card(event.model)->price, "invalid sale event commodity or price");
        require(!event.overflow_model || inventory_supported(*event.overflow_model), "invalid sale overflow model");
    }
    if (s.last_purchase) {
        require(s.decisions > 0 && s.phase != MatchPhase::purchase
            && !s.last_danger && (!s.last_removal || !s.last_auto_guardian_payloads.empty()) && !s.last_sale, "invalid purchase event boundary");
        const auto& event = *s.last_purchase;
        (void)index(s, event.origin); (void)index(s, event.buyer); (void)index(s, event.seller);
        require(static_cast<unsigned>(event.reason) <= 5, "invalid purchase result reason");
        if (event.reason == PurchaseReason::self_inspection)
            require(event.buyer == event.seller && event.origin == event.buyer, "invalid self inspection participants");
        else if (event.reason != PurchaseReason::empty)
            require(event.buyer != event.seller, "foreign purchase result aliases its participants");
        else require(event.buyer != event.seller || event.origin == event.buyer, "invalid self empty inspection origin");
        require(!event.overflow_model || (event.reason == PurchaseReason::bought && inventory_supported(*event.overflow_model)), "invalid purchase overflow model");
        if (event.reason == PurchaseReason::empty) require(event.model == 0 && event.price == 0, "empty purchase has a commodity");
        else require(inventory_supported(event.model) && event.price == find_card(event.model)->price, "invalid purchase result price");
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
        require((!has_wide(event.models) || (event.models == std::vector<CardId>{94,232}
                && s.state.players.size() == 2 && !event.mortar
                && is_enemy(s.state.players[index(s, event.origin)], s.state.players[index(s, event.target)]))) && event.power == attack_power(event.models, event.mortar ? 99 : 30)
            && event.element == attack_element(event.models), "invalid danger event payload");
    }
    if (s.last_fog) {
        const auto& event = *s.last_fog;
        const auto& owner = s.state.players[index(s, event.owner)];
        require(s.decisions > 0 && !s.last_danger
            && is_enemy(owner, s.state.players[index(s, event.intended)]) && is_enemy(owner, s.state.players[index(s, event.target)]), "invalid Fog selection endpoints");
        require(!event.models.empty() && event.models.size() <= s.config.hand_limit, "invalid Fog selection models");
        std::vector<VisibleSelection> prefix;
        for (auto model : event.models) { require(inventory_supported(model) && can_append_main(prefix,{model,false}), "invalid Fog selection composition"); prefix.push_back({model,false}); }
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
            && disease(tick.before) && disease(tick.after) && ((s.last_auto_guardians.empty() && s.last_auto_guardian_payloads.empty()) ? has_curse(owner,tick.after) : true)
            && tick.hp_before >= 1 && tick.hp_before <= 99 && tick.hp_after >= 0 && tick.hp_after <= 99
            && (!s.last_auto_guardians.empty() || !s.last_auto_guardian_payloads.empty() || (owner.hp==tick.hp_after && owner.dead==(tick.hp_after==0))),
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
        require(inventory_supported(offer.item.model) && !offer.item.used && !offer.item.fake_model
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
            MatchSnapshot expected = s; finish(expected, natural); require(s.rewards == expected.rewards && s.state==expected.state, "invalid terminal rewards or uncleared confusion");
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
        require(batch.grants.empty() && !batch.fate_action && !batch.devil_action && batch.owner == s.owner && batch.initial_state.players.size() == n
            && !(batch.guardian_action && batch.guardian_payload_action)
            && !(batch.phenomenon_action && (batch.guardian_action || batch.guardian_payload_action))
            && !(batch.confusion_command && (batch.phenomenon_action || batch.guardian_action || batch.guardian_payload_action))
            && (batch.phenomenon_action ? (targeted_phenomenon(batch.phenomenon_action->model)
                && batch.models==std::vector<CardId>{batch.phenomenon_action->model} && batch.attack==Command{})
                : batch.guardian_action ? (batch.models == std::vector<CardId>{batch.guardian_action->model} && batch.attack == Command{})
                : batch.guardian_payload_action ? (batch.models==batch.guardian_payload_action->models && batch.models.size()<=visible_hand_capacity && batch.attack==Command{})
                : (batch.models.size() <= s.config.hand_limit && batch.attack.items.size() == batch.models.size()))
            && batch.targets.size() <= 2 * (n - 1) && batch.cursor <= batch.targets.size(), "invalid action batch shape");
        require(batch.guardian_cursor<=n && (batch.guardian_stage_started || !batch.guardian_cursor)
            && (!batch.guardian_stage_started || (batch.tick_processed && batch.cursor==batch.targets.size()))
            && (!(batch.guardian_action || batch.guardian_payload_action || batch.phenomenon_action) || (!batch.guardian_stage_started && !batch.guardian_cursor))
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
        auto replay=snapshot_from_anchor(batch,s.config);
        validate_snapshot(replay); // Non-recursive anchor has no batch or pending effect.
        require(batch.attack.kind != CommandKind::defend && batch.attack.kind != CommandKind::accept
            && batch.attack.kind != CommandKind::decline, "action journal has no valid main command");
        if (batch.phenomenon_action) execute_phenomenon(replay,*batch.phenomenon_action);
        else if (batch.guardian_payload_action) execute_guardian_payload(replay,*batch.guardian_payload_action);
        else if (batch.guardian_action) execute_guardian(replay,*batch.guardian_action);
        else execute(replay, batch.attack,batch.confusion_command,nullptr,nullptr,batch.supplied_summon);
        for (std::size_t response_index=0;response_index<batch.responses.size();++response_index) {
            require(replay.batch.has_value(), "action journal continues after its batch ended");
            replay_response(replay,batch,response_index);
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
bool match_artifact_grant_supported(CardId id) noexcept { return gift_model_supported(id,true); }
bool match_grant_supported(CardId id) noexcept { return id==0 || match_devil_supported(id) || gift_model_supported(id,true); }
bool match_devil_supported(CardId id) noexcept { return id>=240 && id<=244; }
bool match_fate_supported(CardId id) noexcept { return id==207; }
bool match_item_shuffle_supported(CardId id) noexcept { return id==295; }
bool match_phenomenon_supported(CardId id) noexcept { return (id>=287 && id<=294) || id==296; }
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
bool match_dream_disguise_supported(CardId actual, CardId face) noexcept {
    return match_dream_compatible(actual,face) || (actual==face && match_supported(actual))
        || (actual==19 && face==27);
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
DeterminizeSupport Match::determinize_support(PlayerId viewer, bool assign_own_disguises) const noexcept {
    const auto& s = data_;
    const auto* own = find_player(s.state, viewer);
    if (!own) return DeterminizeSupport::invalid_viewer;
    if (s.state.players.size() != 2 || std::any_of(s.state.players.begin(), s.state.players.end(),
            [](const auto& p) { return p.team != Team::solo; })) return DeterminizeSupport::unsupported_players;
    if (s.phase == MatchPhase::response) {
        if (s.actor != viewer || s.owner == viewer) return DeterminizeSupport::not_actor;
        if (!s.batch) return DeterminizeSupport::response_special;
        const auto& b = *s.batch;
        if (s.grant_proof || s.redraw_proof || s.purchase_proof || s.summon_proof || s.automatic_guardian_proof
                || s.last_fate || s.last_devil || s.last_item_shuffle || s.pending_grant || s.purchase
                || b.guardian_action || b.guardian_payload_action || b.phenomenon_action || b.fate_action
                || b.devil_action || b.confusion_command || b.supplied_summon || b.automatic_guardian_outcomes
                || b.guardian_stage_started || b.automatic_payload || b.tick_processed
                || !b.grants.empty() || !b.resolved_redraws.empty() || !b.resolved_purchases.empty()
                || !b.resolved_automatic_guardians.empty() || b.attack.kind != CommandKind::play)
            return DeterminizeSupport::response_special;
        // Public launch cards/status determine these draws. Never inspect RNG
        // equality or the opponent's unplayed cards to decide support.
        const auto& base = *find_card(b.models.front());
        if (base.ability == Ability::danger)
            return DeterminizeSupport::response_random;
        // An unresolved ordinary payload and reflection-only history cannot
        // resolve bodies, redraw, reserve counters/deaths, tick, or draw gifts.
        if (s.queue.size() != 1 || !s.counters.empty() || !s.deaths.empty()
                || s.queue.front().is_counter || s.queue.front().is_death
                || s.queue.front().redirects.size() != b.responses.size())
            return DeterminizeSupport::response_history;
        auto element = attack_element(b.models);
        for (const auto& redirect : s.queue.front().redirects) {
            auto reaction = Reaction::none;
            for (auto model : redirect.models) {
                const auto& card = *find_card(model);
                if (card.ability == Ability::self_curse_and_redraw) return DeterminizeSupport::response_special;
                if (card.ability == Ability::filter_atk_element) element.reset();
                const auto next = reaction_for(model, base.category, element);
                if (next == Reaction::bounce) return DeterminizeSupport::response_random;
                if (next != Reaction::none) reaction = next;
            }
            if (reaction != Reaction::reflect) return DeterminizeSupport::response_history;
        }
    } else {
        if (s.phase != MatchPhase::main) return DeterminizeSupport::not_main;
        if (s.actor != viewer || s.owner != viewer) return DeterminizeSupport::not_actor;
        if (!s.queue.empty() || s.batch || !s.counters.empty() || !s.deaths.empty() || s.pending_grant || s.purchase)
            return DeterminizeSupport::pending_work;
        if (std::any_of(s.gifts.begin(), s.gifts.end(), [](auto count) { return count != 0; }))
            return DeterminizeSupport::reserved_gifts;
        // Completed external actions also retain whole-snapshot replay anchors.
        if (s.grant_proof || s.redraw_proof || s.purchase_proof || s.summon_proof || s.automatic_guardian_proof
                || s.last_fate || s.last_devil || s.last_item_shuffle) return DeterminizeSupport::replay_proof;
    }
    // project_view hides enemy status when the VIEWER is fogged.
    if (has_curse(*own, Curse::fog)) return DeterminizeSupport::hidden_status;
    if (std::any_of(s.state.players.begin(), s.state.players.end(), [](const auto& p) { return p.confusion_count > 0; }))
        return DeterminizeSupport::hidden_confusion;
    if (!assign_own_disguises && std::any_of(own->items.begin(), own->items.end(), [](const auto& item) {
            return item.fake_model && *item.fake_model != item.model;
        })) return DeterminizeSupport::hidden_own_model;
    return DeterminizeSupport::ok;
}
Match Match::determinized(PlayerId viewer, std::span<const CardId> assignment, std::uint64_t seed, bool normalize_tokens) const {
    return determinized_impl(viewer, assignment, std::nullopt, seed, normalize_tokens);
}
Match Match::determinized(PlayerId viewer, std::span<const CardId> assignment,
        std::span<const CardId> own_disguises, std::uint64_t seed, bool normalize_tokens) const {
    return determinized_impl(viewer, assignment, own_disguises, seed, normalize_tokens);
}
Match Match::determinized_impl(PlayerId viewer, std::span<const CardId> assignment,
        std::optional<std::span<const CardId>> own_disguises, std::uint64_t seed, bool normalize_tokens) const {
    if (determinize_support(viewer, own_disguises.has_value()) != DeterminizeSupport::ok) unsupported("determinization is unsupported at this boundary");
    const bool response = data_.phase == MatchPhase::response;
    auto s = response ? snapshot_from_anchor(*data_.batch, config_) : data_;
    auto& own = s.state.players[s.state.players.front().id == viewer ? 0 : 1];
    if (own_disguises) {
        std::size_t n=0;
        for (const auto& current : view(viewer).board.hand) if (current.disguised) {
            require(n < own_disguises->size(), "missing own disguise assignment");
            const auto model=(*own_disguises)[n++];
            require(gift_supported(model) && match_dream_disguise_supported(model, current.face),
                    "incompatible own disguise assignment");
            auto item=std::find_if(own.items.begin(),own.items.end(),[&](const auto& i){return i.token==current.token;});
            require(item!=own.items.end() && item->fake_model==current.face, "own disguise missing in anchor");
            item->model=model;
        }
        require(n == own_disguises->size(), "extra own disguise assignment");
    }
    auto& other = s.state.players[s.state.players.front().id == viewer ? 1 : 0];
    // Only public, already selected cards survive in the opponent's anchor.
    // Consumed weapons no longer exist now; consumed miracles are retained used.
    const auto touched = [&](ItemToken token) {
        if (!response) return false;
        const auto& b = *data_.batch;
        if (std::find(b.attack.items.begin(), b.attack.items.end(), token) != b.attack.items.end()) return true;
        for (const auto& command : b.responses) if (command.actor == other.id
                && std::find(command.items.begin(), command.items.end(), token) != command.items.end()) return true;
        return false;
    };
    std::erase_if(other.items, [&](const auto& item) { return !item.used && !touched(item.token); });
    require(assignment.size() <= s.config.hand_limit - other.items.size(), "determinized launch inventory exceeds hand limit");
    for (auto model : assignment) require(gift_supported(model), "determinized model outside configured gift pool");
    s.next_token = data_.next_token;
    if (normalize_tokens) {
        // Normalize before replay so current and anchor inventories/allocators
        // satisfy restore's equality proof. Public selected IDs stay unchanged.
        ItemToken next = 1;
        for (const auto& player : s.state.players) for (const auto& item : player.items)
            next = std::max(next, item.token + 1);
        next = std::max({next, s.dream_rolls, s.redrawn_items});
        if (s.last_tick && s.last_tick->revival_token)
            next = std::max(next, *s.last_tick->revival_token + 1);
        s.next_token = next;
    }
    require(assignment.size() <= max_wire_integer + 1 - s.next_token, "determinized token budget exhausted");
    std::vector<CardId> models(assignment.begin(), assignment.end());
    std::sort(models.begin(), models.end());
    for (auto model : models) other.items.push_back({s.next_token++, model, {}, false});
    s.rng = seed; // Reset the anchor too: no real past RNG survives in a world.
    if (response) {
        const auto& journal = *data_.batch;
        const auto& base = *find_card(journal.models.front());
        const bool conditioned = base.hit_rate || base.ability == Ability::boost_hp_or_deal_damage;
        if (conditioned) {
            // An ordinary percentage miss has no response boundary: existence
            // of this pending payload publicly proves a hit. HP gamble's sign
            // is also public in PendingView.amount (even under Fog). Condition
            // a fresh stream on those facts, never on the original RNG/roll.
            // Rejection rather than transplanting a payload keeps restore's
            // ordinary launch journal proof valid without changing game rules.
            const auto amount = view(viewer).pending->amount;
            auto sampler = seed;
            for (;;) {
                // Probe only launch draws. Executing a rejected miss would
                // unnecessarily resolve gifts/ticks and could exhaust tokens.
                auto probe = s.rng;
                if (journal.attack.target && *journal.attack.target != journal.owner
                        && has_curse(*find_player(s.state, journal.owner), Curse::fog)
                        && !base.hit_rate && !has_wide(journal.models))
                    (void)bounded(probe, 1); // exactly one enemy in this domain
                const auto sign = base.ability == Ability::boost_hp_or_deal_damage
                    ? (bounded(probe, 2) == 0 ? 10 : -10) : 0;
                const bool hit = !base.hit_rate
                    || has_curse(*find_player(s.state, viewer), Curse::darkcloud)
                    || bounded(probe, 100) < static_cast<std::uint64_t>(base.hit_rate);
                if (hit && (base.ability != Ability::boost_hp_or_deal_damage || sign == amount)) break;
                s.rng = random(sampler);
            }
        }
        replay_launch(s, journal);
        for (std::size_t i=0; i<journal.responses.size(); ++i) replay_response(s, journal, i);
    }
    Match result = *this;
    result.restore(s);
    if (response) {
        require(result.view(viewer) == view(viewer), "determinized response observation differs");
        require(result.last_decision(viewer) == last_decision(viewer), "determinized response decision differs");
        require(own_disguises || find_player(result.data_.state, viewer)->items == find_player(data_.state, viewer)->items,
                "determinized response actor inventory differs");
        // Response legality only uses the actor's items/MP/Flash and the active
        // payload's category/element/power (prepare/response_context). These are
        // covered by the exact view comparison; no opponent inventory is read.
        require(result.legal_kinds() == legal_kinds(), "determinized response kinds differ");
    }
    if (own_disguises) {
        require(result.view(viewer)==view(viewer), "determinized own observation differs");
        require(result.last_decision(viewer)==last_decision(viewer), "determinized own decision differs");
        require(result.legal_kinds()==legal_kinds(), "determinized own kinds differ");
        // SelectionTree reads faces/used/disguised/order, public resources and
        // response context. Its only true-model predicates are call_phenomenon
        // at the base and mortar presence for the exact 94+232 exception.
        // Check those invariants and that exception explicitly, avoiding an
        // exponential enumeration of all discard/defense permutations.
        const auto& before=find_player(data_.state,viewer)->items;
        const auto& after=find_player(result.data_.state,viewer)->items;
        require(before.size()==after.size(), "determinized own inventory size differs");
        for(std::size_t i=0;i<before.size();++i) {
            auto a=before[i], b=after[i];
            require((find_card(a.model)->ability==Ability::call_phenomenon)==
                    (find_card(b.model)->ability==Ability::call_phenomenon)
                    && (a.model==209)==(b.model==209), "determinized own legality predicate differs");
            a.model=a.fake_model.value_or(a.model);b.model=b.fake_model.value_or(b.model);
            require(a==b, "determinized own visible inventory differs");
        }
        for (bool canonical : {false,true}) for (auto kind : legal_kinds()) {
            const auto root=legal_extensions(kind,{}, {canonical});
            require(result.legal_extensions(kind,{}, {canonical})==root, "determinized own mask differs");
            for(auto token:root.next) {
                const std::array prefix{token};
                const auto node=legal_extensions(kind,prefix,{canonical});
                require(result.legal_extensions(kind,prefix,{canonical})==node, "determinized own prefix differs");
                if(kind==CommandKind::play && face_model(*instance(*find_player(data_.state,viewer),token))==94)
                    for(auto next:node.next) if(face_model(*instance(*find_player(data_.state,viewer),next))==232) {
                        const std::array pair{token,next};
                        require(result.legal_extensions(kind,pair,{canonical})==legal_extensions(kind,pair,{canonical}),
                                "determinized own Danger mask differs");
                    }
            }
        }
    }
    return result;
}
bool Match::gift_supported(CardId model) const noexcept { return gift_model_supported(model,data_.config.include_fate); }
std::vector<CardId> Match::gift_catalog() const {
    std::vector<CardId> models;
    for (const auto& card:catalog()) if (gift_supported(card.id)) models.push_back(card.id);
    return models;
}
int Match::gift_weight() const noexcept { return data_.config.include_fate?500:499; }
bool Match::legal(const Command& command) const noexcept { try { (void)prepare(data_, command); return true; } catch (...) { return false; } }
MatchStep Match::step(const Command& command) {
    auto s = data_; execute(s, command);
    const auto output = result(s); data_ = std::move(s); return output;
}
bool Match::redraw_response_legal(const RedrawResponseAction& action) const noexcept {
    try {
        const auto models=prepare(data_,action.response);
        prepare_redraw_response(data_,action.response,models,action.results); return true;
    } catch (...) { return false; }
}
MatchStep Match::redraw_response_step(const RedrawResponseAction& action) {
    auto next=data_; execute(next,action.response,false,&action.results);
    const auto output=result(next); data_=std::move(next); return output;
}
bool Match::purchase_response_legal(const PurchaseResponseAction& action) const noexcept {
    try {
        const auto models=prepare(data_,action.response);
        prepare_purchase_response(data_,action.response,models,action.commodity); return true;
    } catch (...) { return false; }
}
MatchStep Match::purchase_response_step(const PurchaseResponseAction& action) {
    auto next=data_; execute(next,action.response,false,nullptr,&action.commodity);
    const auto output=result(next); data_=std::move(next); return output;
}
bool Match::summon_result_legal(const SummonResultAction& action) const noexcept {
    try { const auto models=prepare(data_,action.command); prepare_summon_result(data_,action.command,models,action.guardian); return true; }
    catch (...) { return false; }
}
MatchStep Match::summon_result_step(const SummonResultAction& action) {
    auto next=data_; execute(next,action.command,false,nullptr,nullptr,action.guardian);
    const auto output=result(next); data_=std::move(next); return output;
}
bool Match::automatic_guardian_response_legal(const AutomaticGuardianResponseAction& action) const noexcept {
    try { auto next=data_; execute(next,action.response,false,nullptr,nullptr,{},&action.outcomes); return true; }
    catch (...) { return false; }
}
MatchStep Match::automatic_guardian_response_step(const AutomaticGuardianResponseAction& action) {
    auto next=data_; execute(next,action.response,false,nullptr,nullptr,{},&action.outcomes);
    const auto output=result(next); data_=std::move(next); return output;
}
bool Match::choice_legal(const PlayerChoice& choice) const noexcept {
    try {
        if (const auto* command=std::get_if<Command>(&choice)) { (void)prepare(data_,*command); return true; }
        const auto& request=std::get<FateRequest>(choice);
        const auto seat=prepare_fate_source(data_,request.actor,request.token);
        require(data_.state.players[seat].confusion_count==0,"confused main owner cannot make a player choice");
        return true;
    } catch (...) { return false; }
}
PlayerChoice Match::sample_choice(std::uint64_t& policy_rng) const {
    // Delegation preserves the exact old policy/RNG law outside ordinary main.
    if (data_.phase!=MatchPhase::main) return sample_action(policy_rng);
    require(!done() && data_.actor==data_.owner,"player choice needs the main owner");
    const auto& own=data_.state.players[index(data_,data_.owner)];
    if (own.confusion_count>0) unsupported("confused main selection belongs to the chance driver");
    std::array<ItemToken,visible_hand_capacity> tokens; std::size_t count=0;
    for (const auto& item:own.items) if (!item.used && !item.fake_model && face_model(item)==207) tokens[count++]=item.token;
    if (!count) return sample_action(policy_rng);
    auto rng=policy_rng;
    const auto ticket=bounded(rng,count+1);
    PlayerChoice choice=ticket?PlayerChoice{FateRequest{own.id,tokens[ticket-1]}}:PlayerChoice{sample_action(rng)};
    require(choice_legal(choice),"sampled player choice is not legal");
    policy_rng=rng; return choice;
}
void Match::apply_choice(const PlayerChoice& choice, std::uint64_t& chance_rng) {
    if (const auto* command=std::get_if<Command>(&choice)) { execute(data_,*command); return; }
    const auto& request=std::get<FateRequest>(choice);
    const auto seat=prepare_fate_source(data_,request.actor,request.token);
    require(data_.state.players[seat].confusion_count==0,"confused main owner cannot make a player choice");
    const auto outcome=sample_fate(request.token,chance_rng);
    execute_fate(data_,outcome);
}
MatchStep Match::choice_step(const PlayerChoice& choice, std::uint64_t& chance_rng) {
    auto next=*this; auto chance=chance_rng;
    next.apply_choice(choice,chance);
    auto output=result(next.data_);
    data_=std::move(next.data_); chance_rng=chance; return output;
}
MatchStep Match::rollout_step(std::uint64_t& policy_rng, std::uint64_t& chance_rng) {
    require(&policy_rng!=&chance_rng,"policy and chance streams require distinct storage");
    require(!done(),"cannot drive a finished match");
    auto next=*this; auto policy=policy_rng; auto chance=chance_rng;
    if (next.data_.phase==MatchPhase::grant) execute_grant(next.data_,next.sample_grant(chance));
    else if (next.data_.phase==MatchPhase::main && next.data_.state.players[index(next.data_,next.data_.owner)].confusion_count>0)
        execute(next.data_,next.sample_confusion(chance),true);
    else next.apply_choice(next.sample_choice(policy),chance);
    auto output=result(next.data_);
    data_=std::move(next.data_); policy_rng=policy; chance_rng=chance; return output;
}
GrantAction Match::sample_grant(std::uint64_t& chance_rng) const {
    GrantAction selected{data_.pending_grant.value_or(0),0,{},{}};
    prepare_grant(data_,selected);
    if (!devil_grants_enabled(data_)) return selected;
    auto rng=chance_rng;
    auto ticket=bounded(rng,100);
    for (CardId model=240; model<=244; ++model) {
        const auto mass=static_cast<std::uint64_t>(find_card(model)->appearance_rate);
        if (ticket<mass) { selected.model=model; break; }
        ticket-=mass;
    }
    if (selected.model==243) {
        const auto& items=data_.state.players[index(data_,selected.target)].items;
        std::array<ItemToken,visible_hand_capacity> tokens;
        for (std::size_t i=0; i<items.size(); ++i) tokens[i]=items[i].token;
        const auto count=std::min<std::size_t>(2,items.size());
        selected.removed.reserve(count);
        for (std::size_t i=0; i<count; ++i) {
            const auto choice=i+bounded(rng,items.size()-i);
            std::swap(tokens[i],tokens[choice]); selected.removed.push_back(tokens[i]);
        }
    } else if (selected.model==244) selected.resource=static_cast<Resource>(bounded(rng,3));
    chance_rng=rng;
    return selected;
}
bool Match::grant_legal(const GrantAction& action) const noexcept {
    try { prepare_grant(data_,action); return true; } catch (...) { return false; }
}
MatchStep Match::grant_step(const GrantAction& action) {
    auto next=data_; execute_grant(next,action); auto output=result(next); data_=std::move(next); return output;
}
bool Match::artifact_grant_legal(const ArtifactGrantAction& action) const noexcept {
    try { prepare_artifact_grant(data_,action); return true; } catch (...) { return false; }
}
MatchStep Match::artifact_grant_step(const ArtifactGrantAction& action) {
    auto next=data_; execute_artifact_grant(next,action); auto output=result(next); data_=std::move(next); return output;
}
bool Match::devil_legal(const DevilAction& action) const noexcept {
    try { prepare_devil(data_,action); return true; } catch (...) { return false; }
}
MatchStep Match::devil_step(const DevilAction& action) {
    auto next=data_; execute_devil(next,action); auto output=result(next); data_=std::move(next); return output;
}
FateAction Match::sample_fate(ItemToken token, std::uint64_t& chance_rng) const {
    (void)prepare_fate_source(data_,data_.owner,token);
    auto rng=chance_rng;
    FateAction selected{data_.owner,token,{static_cast<CardId>(287+bounded(rng,10)),{},{}},{}};
    if (selected.outcome.model>=291 && selected.outcome.model<=294) {
        std::array<PlayerId,player_capacity> alive; std::size_t count=0;
        for (const auto& player:data_.state.players) if (!player.dead) alive[count++]=player.id;
        selected.outcome.recipient=alive[bounded(rng,count)];
    } else if (selected.outcome.model==296) {
        for (const auto& player:data_.state.players) if (!player.dead)
            selected.outcome.guardians.push_back(static_cast<Guardian>(1+bounded(rng,10)));
    } else if (selected.outcome.model==295) {
        selected.shuffle=ItemShuffleAction{};
        auto& order=selected.shuffle->order;
        for (const auto& player:data_.state.players) if (!player.dead) for (const auto& item:player.items)
            if (player.id!=data_.owner || item.token!=token) order.push_back({player.id,item.token});
        for (std::size_t count=order.size();count>1;--count) std::swap(order[count-1],order[bounded(rng,count)]);
    }
    prepare_fate(data_,selected);
    chance_rng=rng;
    return selected;
}
bool Match::fate_legal(const FateAction& action) const noexcept {
    try { prepare_fate(data_,action); return true; } catch (...) { return false; }
}
MatchStep Match::fate_step(const FateAction& action) {
    auto next=data_; execute_fate(next,action); auto output=result(next); data_=std::move(next); return output;
}
bool Match::confusion_legal(const Command& command) const noexcept {
    try { (void)prepare(data_,command,true); return true; } catch (...) { return false; }
}
MatchStep Match::confusion_step(const Command& command) {
    auto next=data_; execute(next,command,true); auto output=result(next); data_=std::move(next); return output;
}
bool Match::guardian_legal(const GuardianAction& action) const noexcept {
    try { prepare_guardian(data_,action); return true; } catch (...) { return false; }
}
MatchStep Match::guardian_step(const GuardianAction& action) {
    auto next = data_; execute_guardian(next,action); const auto output = result(next); data_ = std::move(next); return output;
}
namespace {
// A prefix's paid miracles are immutable except for its last card: only an
// immediately following FREE can cancel that card. No longer search is needed.
struct SelectionTree {
    const MatchSnapshot& s;
    const PlayerState& own;
    CommandKind kind;
    bool canonical;
    std::vector<const ItemInstance*> items;
    std::vector<VisibleSelection> visible;
    std::vector<CardId> models;
    std::optional<ResponseContext> context;

    bool selected(const ItemInstance& item) const {
        return std::find(items.begin(),items.end(),&item)!=items.end();
    }
    bool exchangeable(const ItemInstance& item, std::size_t position) const {
        const auto& card=*find_card(face_model(item));
        // Disguises and all effectful abilities are visible barriers. Counters,
        // reactions and redraw/self curses execute in selection order.
        // Play's base is fixed; ordinary extras only add ATK/merge elements.
        return (kind==CommandKind::defend || (kind==CommandKind::play && position>0))
            && !item.fake_model && card.category!=Category::miracles && card.ability==Ability::none;
    }
    bool appendable(const ItemInstance& item) const {
        if (selected(item) || items.size()>=s.config.hand_limit) return false;
        if (canonical) {
            for (const auto& earlier:own.items) {
                if (&earlier==&item) break;
                if (!selected(earlier) && face_model(earlier)==face_model(item)
                    && earlier.fake_model.has_value()==item.fake_model.has_value() && earlier.used==item.used) return false;
            }
            if (!items.empty()) {
                if ((kind==CommandKind::discard || kind==CommandKind::sacrifice) && items.back()>=&item) return false;
                if (exchangeable(item,items.size()) && exchangeable(*items.back(),items.size()-1)) {
                    const auto key=[](const ItemInstance* i) { return std::tuple{face_model(*i),i->fake_model.has_value(),i->used,i}; };
                    if (!(key(items.back())<key(&item))) return false;
                }
            }
        }
        const auto id=face_model(item);
        if (s.phase==MatchPhase::purchase) return false;
        if (s.phase==MatchPhase::response)
            return kind==CommandKind::defend && can_append_response(visible,{id,false},*context);
        if (kind==CommandKind::discard || kind==CommandKind::sacrifice)
            return (kind==CommandKind::sacrifice)==apocalypse_active(s.state) && discardable(*find_card(id));
        if (kind==CommandKind::exchange) return items.empty() && find_card(id)->ability==Ability::exchange;
        if (kind==CommandKind::buy) return items.empty() && id==5;
        if (kind==CommandKind::sell) return items.empty() ? id==4 : items.size()==1 && !item.used
            && can_append_main(visible,{id,false});
        if (kind!=CommandKind::play) return false;
        if (items.empty() && (find_card(item.model)->ability==Ability::call_phenomenon
            || find_card(id)->ability==Ability::exchange || find_card(id)->ability==Ability::sell || find_card(id)->ability==Ability::buy)) return false;
        return can_append_main(visible,{id,false});
    }
    void push(const ItemInstance& item) { items.push_back(&item);models.push_back(face_model(item));visible.push_back({models.back(),false}); }
    void pop() { items.pop_back();models.pop_back();visible.pop_back(); }
    SelectionNode finish() const {
        SelectionNode n;
        const auto targetless=[&] { n.can_finish=true;n.targets.push_back(std::nullopt); };
        if (s.phase==MatchPhase::purchase) {
            if (items.empty() && (kind==CommandKind::decline || (kind==CommandKind::accept && s.purchase
                && own.cp>=find_card(s.purchase->item.model)->price))) targetless();
        } else if (s.phase==MatchPhase::response) {
            if (kind==CommandKind::defend && magic_cost(models)<=own.mp) targetless();
        } else if (kind==CommandKind::pray) {
            if (items.empty() && std::none_of(own.items.begin(),own.items.end(),[](const auto& i){return find_card(face_model(i))->category==Category::weapons;})) targetless();
        } else if (kind==CommandKind::discard || kind==CommandKind::sacrifice) {
            if (!items.empty()) targetless();
        } else if (kind==CommandKind::exchange) {
            const auto total=own.hp+own.mp+own.cp;
            if (items.size()==1 && total>=0 && total<=297) { targetless();n.exchange=ExchangeDomain{total}; }
        } else if ((kind==CommandKind::buy && items.size()==1) || (kind==CommandKind::sell && items.size()==2)
            || (kind==CommandKind::play && !items.empty() && magic_cost(models)<=own.mp)) {
            bool explicit_only=true, no_explicit=false;
            if (kind==CommandKind::play) {
                const auto& base=*find_card(models.front());
                if (base.ability==Ability::danger) {
                    if (has_wide(models) && !scoped_danger_wide(s,models)) return n;
                    explicit_only=false;
                } else if (base.hit_rate || has_wide(models)) {
                    if (enemy_targets(s).empty()) return n;
                    explicit_only=false;no_explicit=base.hit_rate;
                }
            }
            if (!explicit_only) n.targets.push_back(std::nullopt);
            if (!no_explicit) for (const auto& p:s.state.players) if (!p.dead) n.targets.push_back(p.id);
            std::sort(n.targets.begin(),n.targets.end());n.can_finish=!n.targets.empty();
        }
        return n;
    }
    bool reachable() {
        if (finish().can_finish) return true;
        // Only sale's commodity and an immediate FREE can repair an incomplete
        // nonempty prefix. Added attack material cannot repair targets/support.
        if (items.empty()) return false;
        if (kind!=CommandKind::sell && (models.empty() || find_card(models.back())->category!=Category::miracles)) return false;
        for (const auto& item:own.items) {
            if (kind!=CommandKind::sell && find_card(face_model(item))->ability!=Ability::cut_cost) continue;
            if (!appendable(item)) continue;
            push(item);const bool ok=finish().can_finish;pop();if(ok)return true;
        }
        return false;
    }
    SelectionNode node() {
        auto n=finish();
        for (const auto& item:own.items) if (appendable(item)) {
            push(item);const bool ok=reachable();pop();if(ok)n.next.push_back(item.token);
        }
        return n;
    }
};
void selection_boundary(const MatchSnapshot& s) {
    if(s.phase==MatchPhase::grant) unsupported("grant selection must be supplied by an external chance driver");
    require(s.phase!=MatchPhase::finished && s.actor,"cannot select in a finished match");
    if(s.phase==MatchPhase::main && s.state.players[index(s,*s.actor)].confusion_count>0)
        unsupported("confusion command selection must be supplied externally");
}
}
namespace {
std::vector<CommandKind> selection_kinds(const MatchSnapshot& data_) {
    selection_boundary(data_);
    std::vector<CommandKind> kinds;
    for(unsigned k=0;k<=9;++k) {
        SelectionTree tree{data_,data_.state.players[index(data_,*data_.actor)],static_cast<CommandKind>(k),false,{},{},{},{}};
        if(data_.phase==MatchPhase::response) tree.context=response_context(data_);
        const auto n=tree.node();if(n.can_finish || !n.next.empty()) kinds.push_back(tree.kind);
    }
    return kinds;
}
SelectionNode selection_extensions(const MatchSnapshot& data_,CommandKind kind,std::span<const ItemToken> prefix,SelectionOptions options) {
    selection_boundary(data_);
    SelectionTree tree{data_,data_.state.players[index(data_,*data_.actor)],kind,options.canonical,{},{},{},{}};
    if(data_.phase==MatchPhase::response) tree.context=response_context(data_);
    auto n=tree.node();require(n.can_finish || !n.next.empty(),"command kind has no legal selection");
    for(auto token:prefix) {
        require(std::find(n.next.begin(),n.next.end(),token)!=n.next.end(),"prefix is outside the selection tree");
        tree.push(*instance(tree.own,token));n=tree.node();
    }
    return n;
}
}
std::vector<CommandKind> Match::legal_kinds() const { return selection_kinds(data_); }
SelectionNode Match::legal_extensions(CommandKind k,std::span<const ItemToken> p,SelectionOptions o) const { return selection_extensions(data_,k,p,o); }
std::vector<CommandKind> PublicSelection::legal_kinds() const { return selection_kinds(data_); }
SelectionNode PublicSelection::legal_extensions(CommandKind k,std::span<const ItemToken> p,SelectionOptions o) const { return selection_extensions(data_,k,p,o); }
PublicSelection::PublicSelection(const MatchView& v) {
    require(v.board.viewer && v.actor==v.board.viewer,"public selection requires the viewer's turn");
    require(v.board.players.size()==2,"public selection requires two seats");
    data_.phase=v.phase;data_.actor=v.actor;data_.owner=v.owner;
    data_.state.gf=v.board.gf;data_.state.tiebreak_gf=v.board.tiebreak_gf;
    ItemToken next=100;
    for(const auto& p:v.board.players) {
        PlayerState x;x.id=p.id;x.dead=p.dead;x.team=p.team;
        if(p.status) {const auto& a=*p.status;x.hp=a.hp;x.mp=a.mp;x.cp=a.cp;x.curses=a.curses;x.guardian=a.guardian;x.confusion_count=a.confused;}
        if(p.id==*v.board.viewer) {
            require(p.status.has_value() && !p.dead,"public actor has no visible live status");
            for(const auto& h:v.board.hand) x.items.push_back({h.token,h.face,h.disguised?std::optional<CardId>{h.face}:std::nullopt,h.used});
        } else for(auto model:p.used_models)x.items.push_back({next++,model,{},true});
        data_.state.players.push_back(std::move(x));
    }
    validate_state(data_.state);
    if(v.phase==MatchPhase::response) {
        require(v.pending && v.pending->target==*v.actor,"missing public response");
        const auto& p=*v.pending;EffectFrame f;
        f.target=p.target;f.category=p.category;f.element=p.element;f.ability=p.ability;f.power=p.power;
        data_.queue.push_back(f);
    } else if(v.phase==MatchPhase::purchase) {
        require(v.purchase && v.purchase->buyer==*v.actor && find_card(v.purchase->model),"missing public offer");
        const auto& p=*v.purchase;data_.purchase=PurchaseOffer{p.origin,p.buyer,p.seller,{0,p.model,{},false},0};
    } else require(v.phase==MatchPhase::main,"unsupported public selection phase");
}
std::vector<Command> Match::legal_commands(std::size_t max_items, int exchange_step) const {
    require(max_items >= 1 && max_items <= 4, "max_items must be in1..4");
    require(exchange_step >= 0, "exchange_step must be nonnegative");
    if (data_.phase == MatchPhase::grant) unsupported("grant selection must be supplied by an external chance driver");
    require(!done() && data_.actor.has_value(), "cannot enumerate a finished match");
    const auto actor = *data_.actor;
    const auto& own = data_.state.players[index(data_, actor)];
    if (data_.phase == MatchPhase::main && own.confusion_count > 0)
        unsupported("confusion command selection must be supplied externally");
    std::vector<Command> commands;
    const auto add = [&](const Command& command) { if (legal(command)) commands.push_back(command); };
    if (data_.phase == MatchPhase::purchase) {
        add({actor, CommandKind::accept, {}, {}});
        add({actor, CommandKind::decline, {}, {}});
    } else {
        const bool response = data_.phase == MatchPhase::response;
        std::optional<ResponseContext> context;
        if (response) context = response_context(data_);
        Command command{actor, response ? CommandKind::defend : CommandKind::play, {}, {}};
        if (response) add(command);
        else add({actor, CommandKind::pray, {}, {}});
        // prepare() intentionally validates visible faces with used=false. Match
        // that exact contract here, including reusable miracles and Dream faces.
        std::vector<VisibleSelection> prefix;
        const auto visit = [&](auto&& self) -> void {
            if (command.items.size() >= max_items || command.items.size() >= data_.config.hand_limit) return;
            for (const auto& item : own.items) {
                if (std::find(command.items.begin(), command.items.end(), item.token) != command.items.end()) continue;
                const VisibleSelection next{face_model(item), false};
                if (!(response ? can_append_response(prefix, next, *context) : can_append_main(prefix, next))) continue;
                prefix.push_back(next); command.items.push_back(item.token);
                command.target.reset();
                add(command);
                if (!response) for (const auto& target : data_.state.players) if (!target.dead) {
                    command.target = target.id; add(command);
                }
                // Do not prune on MP: a subsequent FREE may make a prefix affordable.
                self(self);
                command.target.reset(); command.items.pop_back(); prefix.pop_back();
            }
        };
        visit(visit);
        if (!response) {
            Command discard{actor, apocalypse_active(data_.state) ? CommandKind::sacrifice : CommandKind::discard, {}, {}};
            const auto discards = [&](auto&& self) -> void {
                if (discard.items.size() >= max_items || discard.items.size() >= data_.config.hand_limit) return;
                for (const auto& item : own.items) {
                    if (!discardable(*find_card(face_model(item))) || std::find(discard.items.begin(), discard.items.end(), item.token) != discard.items.end()) continue;
                    discard.items.push_back(item.token); add(discard); self(self); discard.items.pop_back();
                }
            };
            discards(discards);
            for (const auto& item : own.items) {
                const auto model = face_model(item);
                if (model == 4 && max_items >= 2) for (const auto& commodity : own.items) {
                    if (item.token == commodity.token) continue;
                    for (const auto& target : data_.state.players) if (!target.dead)
                        add({actor, CommandKind::sell, {item.token, commodity.token}, target.id});
                }
                if (model == 5) for (const auto& target : data_.state.players) if (!target.dead)
                    add({actor, CommandKind::buy, {item.token}, target.id});
                if (find_card(model)->ability == Ability::exchange && exchange_step) {
                    const auto total = own.hp + own.mp + own.cp;
                    for (int hp = 0; hp <= 99 && hp <= total; hp += exchange_step)
                        for (int mp = 0; mp <= 99 && hp + mp <= total; mp += exchange_step) {
                            const int cp = total - hp - mp;
                            if (cp <= 99) add({actor, CommandKind::exchange, {item.token}, {}, ResourceAllocation{hp, mp, cp}});
                        }
                }
            }
        }
    }
    std::sort(commands.begin(), commands.end(), [](const Command& a, const Command& b) {
        if (a.kind != b.kind) return a.kind < b.kind;
        if (a.items != b.items) return a.items < b.items;
        if (a.target != b.target) return a.target < b.target;
        if (a.allocation && b.allocation) {
            if (a.allocation->hp != b.allocation->hp) return a.allocation->hp < b.allocation->hp;
            if (a.allocation->mp != b.allocation->mp) return a.allocation->mp < b.allocation->mp;
            return a.allocation->cp < b.allocation->cp;
        }
        return a.allocation.has_value() < b.allocation.has_value();
    });
    return commands;
}
Command Match::sample_action(std::uint64_t& policy_rng) const { return sample_command(policy_rng,false); }
Command Match::sample_confusion(std::uint64_t& chance_rng) const {
    require(data_.phase==MatchPhase::main && !done() && data_.actor==data_.owner
        && data_.state.players[index(data_,data_.owner)].confusion_count>0,"confusion sampling requires the positive-count main owner");
    auto rng=chance_rng;
    auto command=sample_command(rng,true);
    (void)prepare(data_,command,true);
    chance_rng=rng;
    return command;
}
Command Match::sample_command(std::uint64_t& policy_rng, bool allow_confused) const {
    if (data_.phase==MatchPhase::grant) unsupported("grant selection must be supplied by an external chance driver");
    require(!done() && data_.actor.has_value(), "cannot sample a finished match");
    auto rng = policy_rng;
    const auto actor = *data_.actor;
    const auto& own = data_.state.players[index(data_, actor)];
    if (!allow_confused && data_.phase==MatchPhase::main && own.confusion_count>0)
        unsupported("confusion command selection must be supplied externally");
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
            if (discardable(c)) add(apocalypse_active(data_.state)?CommandKind::sacrifice:CommandKind::discard, item.token, {});
            if (c.ability == Ability::exchange) add(CommandKind::exchange, item.token, {});
            else if (c.ability == Ability::sell) {
                if (std::any_of(own.items.begin(), own.items.end(), [&](const auto& commodity) { return commodity.token != item.token && !commodity.used; }))
                    add(CommandKind::sell, item.token, {});
            }
            else if (c.ability == Ability::buy) {
                for (const auto& target : data_.state.players) if (!target.dead) add(CommandKind::buy,item.token,target.id);
            }
            else if (c.ability!=Ability::call_phenomenon && can_start_main(face_model(item))) {
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
bool Match::item_shuffle_legal(const ItemShuffleAction& action) const noexcept {
    try { (void)prepare_item_shuffle(data_,action); return true; } catch (...) { return false; }
}
MatchStep Match::item_shuffle_step(const ItemShuffleAction& action) {
    auto next=data_; execute_item_shuffle(next,action); auto output=result(next); data_=std::move(next); return output;
}
bool Match::phenomenon_legal(const PhenomenonAction& action) const noexcept {
    try { prepare_phenomenon(data_,action); return true; } catch (...) { return false; }
}
MatchStep Match::phenomenon_step(const PhenomenonAction& action) {
    auto next=data_; execute_phenomenon(next,action); auto output=result(next); data_=std::move(next); return output;
}
GuardianPayloadAction Match::sample_guardian_payload(PlayerId source, std::uint64_t& chance_rng) const {
    require(data_.phase==MatchPhase::main && !done(),"guardian payload sampling needs a main boundary");
    auto rng=chance_rng;
    auto selected=select_guardian_payload(data_,source,rng);
    prepare_guardian_payload(data_,selected);
    chance_rng=rng;
    return selected;
}
bool Match::guardian_payload_legal(const GuardianPayloadAction& action) const noexcept {
    try { prepare_guardian_payload(data_,action); return true; } catch (...) { return false; }
}
MatchStep Match::guardian_payload_step(const GuardianPayloadAction& action) {
    auto next=data_; execute_guardian_payload(next,action); auto output=result(next); data_=std::move(next); return output;
}
std::optional<PublicDecision> Match::last_decision(std::optional<PlayerId> viewer) const {
    std::uint16_t bit=0;
    if (viewer) bit=static_cast<std::uint16_t>(1u<<index(data_,*viewer));
    if (!data_.last_decision) return std::nullopt;
    auto value=data_.last_decision->value;
    if (data_.last_decision->hidden_viewers & bit) { value.faces.clear(); value.faces_hidden=true; }
    return value;
}
MatchView Match::view(std::optional<PlayerId> viewer) const {
    MatchView result;
    result.phase = data_.phase;
    result.gift_pool = {data_.config.include_fate, data_.config.include_fate ? 237 : 236, gift_weight()};
    result.local_guardian_payloads = data_.config.local_guardian_payloads;
    result.external_artifacts = data_.config.external_artifacts;
    result.apocalypse_active = apocalypse_active(data_.state);
    result.owner = data_.owner;
    result.actor = data_.actor;
    result.terminated = data_.terminated;
    result.truncated = data_.truncated;
    result.decisions = data_.decisions;
    result.board = project_view(data_.state, viewer);
    result.pending_grant = data_.pending_grant;
    result.last_grant = data_.last_grant;
    result.last_devil = data_.last_devil_event;
    result.last_fate = data_.last_fate_event;
    if (data_.last_phenomenon) result.last_phenomenon = PhenomenonView{data_.last_phenomenon->model, data_.last_phenomenon->recipient};
    result.last_guardian_payload = data_.last_guardian_payload;
    result.last_auto_guardian_payloads = data_.last_auto_guardian_payloads;
    result.last_auto_guardians = data_.last_auto_guardians;
    std::uint16_t viewer_bit = 0;
    if (viewer) for (std::size_t i = 0; i < data_.state.players.size(); ++i)
        if (data_.state.players[i].id == *viewer) viewer_bit = static_cast<std::uint16_t>(1u << i);
    for (const auto& event : data_.last_guardian_changes) if (!(event.hidden_viewers & viewer_bit))
        result.last_guardian_changes.push_back({event.player, event.before, event.after, event.reason});
    result.last_guardian = data_.last_guardian;
    result.last_redraws = data_.last_redraws;
    for (const auto& event : data_.last_reveals) if (!event.owner_only || viewer == event.owner)
        result.last_reveals.push_back({event.owner, event.face, event.model});
    result.last_danger = data_.last_danger;
    result.last_removal = data_.last_removal;
    result.last_sale = data_.last_sale;
    if (data_.purchase) {
        const auto& offer = *data_.purchase;
        result.purchase = PurchaseOfferView{offer.origin, offer.buyer, offer.seller, offer.item.model, find_card(offer.item.model)->price};
    }
    result.last_purchase = data_.last_purchase;
    if (!data_.queue.empty()) {
        const auto& frame = data_.queue.front();
        PendingView pending;
        if (!frame.environmental) { pending.origin = frame.origin; pending.source = frame.source; }
        pending.target = frame.target;
        pending.models = frame.models;
        pending.category = frame.category;
        pending.element = frame.element;
        for (const auto& redirect : frame.redirects) for (auto model : redirect.models) {
            const auto ability = find_card(model)->ability;
            if (ability == Ability::bounce_weapon || ability == Ability::bounce_miracle) ++pending.bounces;
        }
        pending.reflections = frame.redirects.size() - pending.bounces;
        pending.hit_rate = frame.hit_rate;
        pending.counter = frame.is_counter;
        pending.environmental = frame.environmental;
        pending.death = frame.is_death;
        pending.guardian = frame.guardian;
        const auto hidden = std::any_of(result.board.players.begin(), result.board.players.end(), [&](const auto& player) {
            return player.id == frame.initial_target && player.fogged;
        });
        if (!hidden) pending.forced_hit = frame.forced_hit;
        pending.power = frame.power;
        pending.ability = frame.ability;
        pending.amount = frame.amount;
        result.pending = std::move(pending);
    }
    return result;
}

std::string Match::observe(std::optional<PlayerId> viewer) const {
    const char* phase = data_.phase == MatchPhase::main ? "main" : data_.phase == MatchPhase::response ? "response" : data_.phase == MatchPhase::purchase ? "purchase" : data_.phase==MatchPhase::grant ? "grant" : "finished";
    const auto board = project_view(data_.state, viewer);
    std::string out = "{\"schema\":\"official-execution-view-v69\",\"ruleset\":\"";
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
    out += "\",\"self_attack_rule\":\""; out += self_attack_rule;
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
    out += "\",\"redraw_response_rule\":\""; out += redraw_response_rule;
    out += "\",\"summon_result_rule\":\""; out += summon_result_rule;
    out += "\",\"automatic_guardian_response_rule\":\""; out += automatic_guardian_response_rule;
    out += "\",\"purchase_response_rule\":\""; out += purchase_response_rule;
    out += "\",\"guardian_rule\":\""; out += guardian_rule;
    out += "\",\"guardian_summon_rule\":\""; out += guardian_summon_rule;
    out += "\",\"guardian_departure_rule\":\""; out += guardian_departure_rule;
    out += "\",\"guardian_payload_rule\":\""; out += guardian_payload_rule;
    out += "\",\"phenomenon_rule\":\""; out += phenomenon_rule;
    out += "\",\"item_shuffle_rule\":\""; out += item_shuffle_rule;
    out += "\",\"confusion_rule\":\""; out += confusion_rule;
    out += "\",\"fate_rule\":\""; out += fate_rule;
    out += "\",\"devil_rule\":\""; out += devil_rule;
    out += "\",\"apocalypse_rule\":\""; out += apocalypse_rule;
    out += "\",\"grant_rule\":\""; out += grant_rule;
    out += "\",\"artifact_grant_rule\":\""; out += artifact_grant_rule;
    out += "\",\"normal_grant_rule\":\""; out += normal_grant_rule;
    out += "\",\"grant_sampler_rule\":\""; out += grant_sampler_rule;
    out += "\",\"fate_pool_rule\":\""; out += fate_pool_rule;
    out += "\",\"fate_sampler_rule\":\""; out += fate_sampler_rule;
    out += "\",\"confusion_sampler_rule\":\""; out += confusion_sampler_rule;
    out += "\",\"guardian_payload_sampler_rule\":\""; out += guardian_payload_sampler_rule;
    out += "\",\"guardian_payload_auto_rule\":\""; out += guardian_payload_auto_rule; out += "\",\"phase\":\""; out += phase;
    out += "\",\"gift_pool\":{\"include_fate\":"; out += data_.config.include_fate?"true":"false";
    out += ",\"models\":"+std::to_string(data_.config.include_fate?237:236)+",\"weight\":"+std::to_string(gift_weight())+'}';
    out += ",\"local_guardian_payloads\":"; out += data_.config.local_guardian_payloads?"true":"false";
    out += ",\"external_artifacts\":"; out += data_.config.external_artifacts?"true":"false";
    out += ",\"apocalypse_active\":"; out += apocalypse_active(data_.state)?"true":"false";
    out += ",\"owner\":" + std::to_string(data_.owner) + ",\"actor\":";
    out += data_.actor ? std::to_string(*data_.actor) : "null";
    out += ",\"terminated\":"; out += data_.terminated ? "true" : "false";
    out += ",\"truncated\":"; out += data_.truncated ? "true" : "false";
    out += ",\"decisions\":" + std::to_string(data_.decisions) + ",\"board\":" + view_json(board);
    const auto guardian_json=[&](const GuardianAction& action) {
        out += "{\"source\":"+std::to_string(action.source)+",\"model\":"+std::to_string(action.model)+",\"guardian\":\"";
        out += guardian_name(find_card(action.model)->guardian); out += "\",\"target\":";
        out += action.target ? std::to_string(*action.target) : "null"; out += '}';
    };
    out += ",\"pending_grant\":";
    if (data_.pending_grant) out+="{\"target\":"+std::to_string(*data_.pending_grant)+'}'; else out+="null";
    out += ",\"last_grant\":";
    if (!data_.last_grant) out+="null";
    else {
        const auto& event=*data_.last_grant; out+="{\"target\":"+std::to_string(event.target)+",\"devil\":";
        out+=event.model?std::to_string(event.model):"null"; out+=",\"removed_models\":[";
        for (std::size_t i=0;i<event.removed_models.size();++i) {if(i)out+=',';out+=std::to_string(event.removed_models[i]);}
        out+="]}";
    }
    out += ",\"last_devil\":";
    if (!data_.last_devil_event) out+="null";
    else {
        const auto& event=*data_.last_devil_event;
        out+="{\"model\":"+std::to_string(event.model)+",\"target\":"+std::to_string(event.target)+",\"removed_models\":[";
        for (std::size_t i=0;i<event.removed_models.size();++i) {if(i)out+=',';out+=std::to_string(event.removed_models[i]);}
        out+="]}";
    }
    out += ",\"last_fate\":";
    if (!data_.last_fate_event) out+="null";
    else out+="{\"actor\":"+std::to_string(data_.last_fate_event->actor)+",\"model\":207,\"phenomenon\":"+std::to_string(data_.last_fate_event->phenomenon)+'}';
    out += ",\"last_phenomenon\":";
    if (!data_.last_phenomenon) out+="null";
    else {
        const auto& action=*data_.last_phenomenon;
        out+="{\"model\":"+std::to_string(action.model)+",\"recipient\":";
        out+=action.recipient?std::to_string(*action.recipient):"null"; out+='}';
    }
    auto payload_json=[&](const GuardianPayloadAction& action) {
        out+="{\"source\":"+std::to_string(action.source)+",\"guardian\":\"";
        out+=guardian_name(action.guardian); out+="\",\"models\":[";
        for (std::size_t i=0;i<action.models.size();++i) { if(i)out+=',';out+=std::to_string(action.models[i]); }
        out+="],\"target\":";out+=action.target?std::to_string(*action.target):"null";out+='}';
    };
    out += ",\"last_guardian_payload\":";
    if (!data_.last_guardian_payload) out+="null"; else payload_json(*data_.last_guardian_payload);
    out += ",\"last_auto_guardian_payloads\":[";
    for (std::size_t i=0;i<data_.last_auto_guardian_payloads.size();++i) {if(i)out+=',';payload_json(data_.last_auto_guardian_payloads[i]);} out+=']';
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
        out+=",\"reason\":\"";out+=event.reason==GuardianChangeReason::summon?"summon":event.reason==GuardianChangeReason::departure?"departure":"phenomenon";out+="\"}";
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
        out += "{\"origin\":"; out+=e.environmental?"null":std::to_string(e.origin);
        out += ",\"source\":"; out+=e.environmental?"null":std::to_string(e.source);
        out += ",\"target\":" + std::to_string(e.target) + ",\"models\":[";
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
        out += ",\"environmental\":"; out += e.environmental ? "true" : "false";
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
