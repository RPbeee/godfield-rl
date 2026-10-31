#include "godfield/official_match.hpp"
#include <algorithm>
#include <array>
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
void closed_state(const SnapshotState& s, const MatchConfig& c) {
    validate_state(s);
    require(s.players.size() >= 2, "scoped match needs 2..9 players");
    require(s.gf >= 0, "negative GF outside scoped runtime");
    if (s.tiebreak_gf) unsupported("tiebreak is not implemented");
    for (const auto& p : s.players) {
        require(p.hp >= 0 && p.hp <= 99 && p.mp >= 0 && p.mp <= 99 && p.cp >= 0 && p.cp <= 99, "scene resources outside 0..99");
        require(p.dead || p.hp > 0, "HP0 without resolved death is not a quiescent scene");
        require(p.items.size() <= c.hand_limit, "scene inventory exceeds configured display limit");
        if (!p.curses.empty() || p.guardian != Guardian::none || p.confusion_count)
            unsupported("curses, guardians and confusion are not implemented in scoped matches");
        for (const auto& item : p.items) {
            if (item.fake_model || !match_supported(item.model)) unsupported("unsupported or disguised match model");
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
    s.phase = MatchPhase::finished; s.actor.reset(); s.queue.clear();
    std::fill(s.gifts.begin(), s.gifts.end(), 0);
    s.winners = result.winners;
    s.rewards.assign(s.state.players.size(), 0);
    if (!s.winners.empty()) for (std::size_t i = 0; i < s.rewards.size(); ++i)
        s.rewards[i] = std::find(s.winners.begin(), s.winners.end(), s.state.players[i].id) != s.winners.end() ? 1. : -1.;
}
const std::array<CardId, 337>& gift_tickets() {
    static const auto pool = [] {
        std::array<CardId, 337> values{};
        std::size_t n = 0, models = 0;
        for (const auto& c : catalog()) if (match_supported(c.id)) {
            ++models;
            for (int i = 0; i < c.gift_rate; ++i) {
                if (n >= values.size()) throw std::logic_error("scoped pool weight changed");
                values[n++] = c.id;
            }
        }
        if (n != values.size() || models != 156) throw std::logic_error("scoped catalog changed");
        return values;
    }();
    return pool;
}
void gift(MatchSnapshot& s, std::size_t seat) {
    auto& p = s.state.players[seat];
    require(p.items.size() < s.config.hand_limit, "gift has no reserved display slot");
    require(s.next_token <= max_wire_integer, "scoped item token exhausted");
    const auto id = gift_tickets()[bounded(s.rng, gift_tickets().size())];
    p.items.push_back({s.next_token++, id, {}, false});
}
void request_gift(MatchSnapshot& s, std::size_t seat) {
    if (s.state.players[seat].items.size() + s.gifts[seat] < s.config.hand_limit) ++s.gifts[seat];
}
void consume(MatchSnapshot& s, std::size_t seat, const std::vector<ItemToken>& tokens, bool discard) {
    auto& p = s.state.players[seat];
    for (auto token : tokens) {
        auto it = std::find_if(p.items.begin(), p.items.end(), [&](const auto& value) { return value.token == token; });
        require(it != p.items.end(), "selected item vanished");
        if (!discard && find_card(it->model)->category == Category::miracles) it->used = true;
        else p.items.erase(it);
        if (!discard) request_gift(s, seat);
    }
}
bool damaging(const EffectFrame& effect) { return effect.power > 0 || effect.category == Category::weapons; }
ResponseContext response_context(const MatchSnapshot& s) {
    require(s.queue.size() == 1, "response has no active item");
    const auto& effect = s.queue.front();
    return {false, effect.category, effect.element, damaging(effect)};
}
std::vector<CardId> prepare(const MatchSnapshot& s, const Command& command) {
    require(s.phase != MatchPhase::finished && s.actor && *s.actor == command.actor, "wrong actor or finished match");
    if (static_cast<unsigned>(command.kind) > 3) unsupported("unsupported match command kind");
    const auto& own = s.state.players[index(s, command.actor)];
    require(!own.dead, "dead actor");
    require(command.items.size() <= s.config.hand_limit, "too many selected items");
    std::unordered_set<ItemToken> selected;
    std::vector<CardId> models;
    for (auto token : command.items) {
        const auto* item = instance(own, token);
        require(item && selected.insert(token).second, "item missing or selected twice");
        models.push_back(item->model);
    }
    if (s.phase == MatchPhase::response) {
        require(command.kind == CommandKind::defend && !command.target, "response needs targetless defend");
        auto context = response_context(s);
        std::vector<VisibleSelection> prefix;
        for (auto id : models) {
            require(can_append_response(prefix, {id, false}, context), "unsupported defense composition");
            prefix.push_back({id, false});
        }
        require(magic_cost(models) <= own.mp, "insufficient defense MP");
        return models;
    }
    require(command.kind != CommandKind::defend, "defend outside response phase");
    if (command.kind == CommandKind::pray) {
        require(command.items.empty() && !command.target, "pray takes no items or target");
        require(own.items.size() < s.config.hand_limit, "full-hand prayer overflow is outside scoped runtime");
        require(std::none_of(own.items.begin(), own.items.end(), [](const auto& item) { return find_card(item.model)->category == Category::weapons; }), "visible weapon prevents prayer");
        return models;
    }
    if (command.kind == CommandKind::discard) {
        require(!models.empty() && !command.target, "discard needs items and no target");
        for (auto id : models) require(find_card(id)->category != Category::weapons, "cannot discard weapons");
        return models;
    }
    require(!models.empty() && command.target, "play needs items and explicit target");
    require(!s.state.players[index(s, *command.target)].dead, "target is dead");
    std::vector<VisibleSelection> prefix;
    for (auto id : models) {
        require(can_append_main(prefix, {id, false}), "invalid main composition");
        prefix.push_back({id, false});
    }
    require(magic_cost(models) <= own.mp, "insufficient main MP");
    return models;
}
EffectFrame effect_for(const MatchSnapshot& s, const std::vector<CardId>& models, PlayerId target, const std::vector<ItemToken>& tokens) {
    const auto& base = *find_card(models.front());
    return {s.owner, s.owner, target, models, tokens, base.category, attack_element(models), attack_power(models), base.ability, base.ability_value, target, {}};
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
        } else if (reaction == Reaction::block) {
            result.reaction = reaction;
        } else if (reaction != Reaction::none) {
            unsupported("reaction has no scoped resolver");
        } else result.defense += c.defense;
    }
    return result;
}
void resolve(MatchSnapshot& s, const EffectFrame& effect, std::int64_t defense) {
    auto& target = s.state.players[index(s, effect.target)];
    if (damaging(effect)) {
        const auto remaining = std::max<std::int64_t>(0, effect.power - defense);
        if (remaining && effect.element == Element::darkness) target.hp = 0;
        else change_resource(target, Resource::hp, -remaining);
    } else {
        switch (effect.ability) {
        case Ability::boost_hp: change_resource(target, Resource::hp, effect.amount); break;
        case Ability::boost_mp: change_resource(target, Resource::mp, effect.amount); break;
        case Ability::boost_cp: change_resource(target, Resource::cp, effect.amount); break;
        default: unsupported("effect has no scoped resolver");
        }
    }
    for (auto& p : s.state.players) if (!p.hp) p.dead = true;
}
void advance(MatchSnapshot& s) {
    for (std::size_t seat = 0; seat < s.gifts.size(); ++seat) {
        while (s.gifts[seat]) {
            if (!s.state.players[seat].dead) gift(s, seat);
            --s.gifts[seat];
        }
    }
    const auto old = index(s, s.owner);
    for (std::size_t offset = 1; offset <= s.state.players.size(); ++offset) {
        const auto seat = (old + offset) % s.state.players.size();
        if (!s.state.players[seat].dead) {
            s.owner = s.state.players[seat].id; s.actor = s.owner; s.phase = MatchPhase::main;
            require(s.state.gf < static_cast<std::int64_t>(max_wire_integer), "GF exhausted");
            ++s.state.gf;
            return;
        }
    }
    throw std::logic_error("no alive next owner without a terminal outcome");
}
void validate_snapshot(const MatchSnapshot& s) {
    require(s.version == match_snapshot_version, "unknown match snapshot version"); config_valid(s.config); closed_state(s.state, s.config);
    const auto n = s.state.players.size();
    require(s.gifts.size() == n && s.rewards.size() == n && s.decisions <= s.config.max_decisions, "invalid snapshot dimensions/counters");
    (void)index(s, s.owner);
    require(s.next_token <= max_wire_integer + 1, "invalid next token");
    for (std::size_t i = 0; i < n; ++i) {
        require(s.gifts[i] <= s.config.hand_limit && s.state.players[i].items.size() + s.gifts[i] <= s.config.hand_limit, "invalid reserved gifts");
        for (const auto& item : s.state.players[i].items) require(item.token < s.next_token, "next token collides with a live instance");
    }
    const auto natural = outcome(s.state);
    if (s.phase == MatchPhase::finished) {
        require(!s.actor && s.queue.empty() && std::all_of(s.gifts.begin(), s.gifts.end(), [](auto v) { return !v; }), "finished snapshot has pending work");
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
        require(*s.actor == s.owner && s.queue.empty() && std::all_of(s.gifts.begin(), s.gifts.end(), [](auto v) { return !v; }), "main phase has pending work");
        return;
    }
    require(s.phase == MatchPhase::response && s.queue.size() == 1, "invalid match phase/queue");
    const auto& effect = s.queue.front();
    require(effect.origin == s.owner && *s.actor == effect.target
        && !s.state.players[index(s, s.owner)].dead && s.decisions > 0 && !effect.models.empty() && effect.models.size() <= s.config.hand_limit && effect.tokens.size() == effect.models.size(), "invalid effect origin/actor");
    std::vector<VisibleSelection> prefix;
    for (auto id : effect.models) {
        if (!match_supported(id)) unsupported("unsupported queued model");
        require(can_append_main(prefix, {id, false}), "invalid queued composition"); prefix.push_back({id, false});
    }
    auto replay = effect_for(s, effect.models, effect.initial_target, effect.tokens);
    require(!s.state.players[index(s, effect.initial_target)].dead, "initial effect target is dead");
    require(damaging(replay) || replay.target != s.owner, "self boost cannot wait for a response");
    // Every reflected response consumes at least one physical item, before any
    // gifts are dealt. Bound the ledger before inspecting adversarial entries.
    require(effect.reflections.size() <= n * s.config.hand_limit && effect.reflections.size() < s.decisions, "impossible reflection history length");
    std::array<std::unordered_set<ItemToken>, player_capacity> spent;
    std::array<std::size_t, player_capacity> physical{}, uses{};
    std::array<std::int64_t, player_capacity> costs{};
    const auto evidence = [&](PlayerId player, std::span<const CardId> models, std::span<const ItemToken> tokens) {
        const auto seat = index(s, player);
        const auto& own = s.state.players[seat];
        require(!own.dead && !models.empty() && models.size() <= s.config.hand_limit && models.size() == tokens.size(), "invalid consumption evidence shape");
        std::unordered_set<ItemToken> selected;
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            const auto token = tokens[i];
            require(token < s.next_token && token <= max_wire_integer && selected.insert(token).second, "invalid queued instance evidence");
            const auto* retained = instance(own, token);
            if (find_card(models[i])->category == Category::miracles)
                require(retained && retained->used && retained->model == models[i] && !spent[seat].contains(token), "queued miracle has no retained instance");
            else {
                ++physical[seat];
                require(!retained && spent[seat].insert(token).second, "physical item present or consumed twice in reflection history");
            }
        }
        uses[seat] += models.size(); costs[seat] += magic_cost(models);
    };
    evidence(s.owner, effect.models, effect.tokens);
    for (const auto& reflection : effect.reflections) {
        require(reflection.defender == replay.target, "reflection has wrong responder");
        require(!reflection.models.empty() && reflection.models.size() <= s.config.hand_limit && reflection.models.size() == reflection.tokens.size(), "invalid reflection evidence shape");
        std::vector<VisibleSelection> defense_prefix;
        const ResponseContext context{false, replay.category, replay.element, damaging(replay)};
        for (auto id : reflection.models) {
            if (!match_supported(id)) unsupported("unsupported reflection model");
            require(can_append_response(defense_prefix, {id, false}, context), "invalid reflection composition");
            defense_prefix.push_back({id, false});
        }
        evidence(reflection.defender, reflection.models, reflection.tokens);
        require(apply_response(replay, reflection.models).reaction == Reaction::reflect, "history entry has no matching reflection");
    }
    replay.reflections = effect.reflections;
    require(effect == replay, "queued payload does not match original models and reflection history");
    for (std::size_t i = 0; i < n; ++i) {
        const auto& own = s.state.players[i];
        require(own.items.size() + physical[i] <= s.config.hand_limit, "consumption history cannot originate from a valid hand");
        const auto requests = std::min<std::size_t>(uses[i], s.config.hand_limit - own.items.size());
        require(s.gifts[i] == requests, "pending gifts do not match participant consumption");
        require(own.mp + costs[i] <= 99, "queued MP payments have no valid origin");
    }
}
MatchStep result(const MatchSnapshot& s) { return {s.phase, s.actor, s.terminated, s.truncated, s.decisions, s.rewards, s.winners}; }
}
bool match_supported(CardId id) noexcept {
    const auto* c = find_card(id);
    return c && c->gift_rate > 0 && !c->hit_rate && (c->category == Category::weapons || c->category == Category::armor || c->category == Category::sundries || c->category == Category::miracles)
        && (c->ability == Ability::none || c->ability == Ability::boost_hp || c->ability == Ability::boost_mp || c->ability == Ability::boost_cp
            || c->ability == Ability::reflect_weapon || c->ability == Ability::reflect_miracle || c->ability == Ability::reflect_anything
            || c->ability == Ability::filter_atk_element || c->ability == Ability::block_weapon || c->ability == Ability::block_miracle
            || c->ability == Ability::cut_cost || c->ability == Ability::set_element || c->ability == Ability::double_atk);
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
    const auto models = prepare(data_, command); auto s = data_; const auto seat = index(s, command.actor);
    bool completed = false;
    if (s.phase == MatchPhase::response) {
        auto& effect = s.queue.front();
        const auto response = apply_response(effect, models);
        change_resource(s.state.players[seat], Resource::mp, -magic_cost(models));
        consume(s, seat, command.items, false);
        if (response.reaction == Reaction::reflect) {
            effect.reflections.push_back({command.actor, models, command.items}); s.actor = effect.target;
        } else {
            if (response.reaction == Reaction::none) resolve(s, effect, response.defense);
            s.queue.clear(); completed = true;
        }
    } else if (command.kind == CommandKind::discard) {
        consume(s, seat, command.items, true); completed = true;
    } else if (command.kind == CommandKind::pray) {
        request_gift(s, seat); completed = true;
    } else {
        const auto effect = effect_for(s, models, *command.target, command.items);
        change_resource(s.state.players[seat], Resource::mp, -magic_cost(models)); consume(s, seat, command.items, false);
        if (damaging(effect) || effect.target != s.owner) {
            s.queue.push_back(effect); s.actor = effect.target; s.phase = MatchPhase::response;
        } else { resolve(s, effect, 0); completed = true; }
    }
    ++s.decisions; require(s.state.update_count < max_wire_integer, "update counter exhausted"); ++s.state.update_count;
    const auto natural = outcome(s.state);
    if (natural.terminal) finish(s, natural);
    else if (s.decisions == s.config.max_decisions) {
        s.truncated = true; s.phase = MatchPhase::finished; s.actor.reset(); s.queue.clear(); std::fill(s.gifts.begin(), s.gifts.end(), 0);
    } else if (completed) advance(s);
    const auto output = result(s); data_ = std::move(s); return output;
}
Command Match::sample_action(std::uint64_t& policy_rng) const {
    require(!done() && data_.actor.has_value(), "cannot sample a finished match");
    auto rng = policy_rng;
    const auto actor = *data_.actor;
    const auto& own = data_.state.players[index(data_, actor)];
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
        const auto& c = *find_card(item.model);
        if (cost(c) <= own.mp) add(kind, item.token, target);
        if (c.category == Category::miracles) {
            const std::array<VisibleSelection, 1> prefix{{{item.model, item.used}}};
            for (const auto& free : own.items) if (find_card(free.model)->ability == Ability::cut_cost
                && append(prefix, {free.model, free.used})) add(kind, item.token, target, free.token);
        }
    };
    if (data_.phase == MatchPhase::response) {
        context = response_context(data_);
        add(CommandKind::defend, {}, {});
        for (const auto& item : own.items) {
            if (can_append_response({}, {item.model, item.used}, *context)) add_usable(CommandKind::defend, item, {});
        }
    } else {
        const auto weapon = std::any_of(own.items.begin(), own.items.end(), [](const auto& item) { return find_card(item.model)->category == Category::weapons; });
        if (!weapon && own.items.size() < config_.hand_limit) add(CommandKind::pray, {}, {});
        for (const auto& item : own.items) {
            const auto& c = *find_card(item.model);
            if (c.category != Category::weapons) add(CommandKind::discard, item.token, {});
            if (can_start_main(item.model))
                for (const auto& target : data_.state.players) if (!target.dead) add_usable(CommandKind::play, item, target.id);
        }
    }
    require(count != 0, "no scoped action available");
    const auto proposal = proposals[bounded(rng, count)];
    Command selected{actor, proposal.kind, {}, proposal.has_target ? std::optional<PlayerId>{proposal.target} : std::nullopt};
    if (proposal.items) selected.items.push_back(proposal.first);
    if (proposal.items == 2) selected.items.push_back(proposal.second);
    if (proposal.items && (selected.kind == CommandKind::play || selected.kind == CommandKind::defend)) {
        const auto* base = instance(own, proposal.first);
        std::vector<VisibleSelection> prefix{{base->model, base->used}};
        prefix.reserve(own.items.size()); selected.items.reserve(own.items.size());
        auto spent = proposal.items == 2 ? 0 : cost(*find_card(base->model));
        if (proposal.items == 2) { const auto* free = instance(own, proposal.second); prefix.push_back({free->model, free->used}); }
        for (const auto& extra : own.items) if (bounded(rng, 2)) {
            if (std::find(selected.items.begin(), selected.items.end(), extra.token) != selected.items.end()) continue;
            const auto& c = *find_card(extra.model);
            const VisibleSelection visible{extra.model, extra.used};
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
                for (const auto& free : own.items) if (find_card(free.model)->ability == Ability::cut_cost
                    && std::find(selected.items.begin(), selected.items.end(), free.token) == selected.items.end()
                    && append(prefix, {free.model, free.used})) free_items[free_count++] = &free;
                prefix.pop_back();
                if (!free_count) continue;
                discount = free_items[bounded(rng, free_count)]; next_spent -= cost(c);
            }
            selected.items.push_back(extra.token); prefix.push_back(visible); spent = next_spent;
            if (discount) { selected.items.push_back(discount->token); prefix.push_back({discount->model, discount->used}); }
        }
    }
    policy_rng = rng;
    return selected;
}
std::string Match::observe(std::optional<PlayerId> viewer) const {
    const char* phase = data_.phase == MatchPhase::main ? "main" : data_.phase == MatchPhase::response ? "response" : "finished";
    std::string out = "{\"schema\":\"official-execution-view-v2\",\"ruleset\":\"";
    out += match_ruleset; out += "\",\"scope\":\""; out += match_scope; out += "\",\"phase\":\""; out += phase;
    out += "\",\"owner\":" + std::to_string(data_.owner) + ",\"actor\":";
    out += data_.actor ? std::to_string(*data_.actor) : "null";
    out += ",\"terminated\":"; out += data_.terminated ? "true" : "false";
    out += ",\"truncated\":"; out += data_.truncated ? "true" : "false";
    out += ",\"decisions\":" + std::to_string(data_.decisions) + ",\"board\":" + view_json(project_view(data_.state, viewer));
    out += ",\"pending\":";
    if (data_.queue.empty()) out += "null";
    else {
        const auto& e = data_.queue.front();
        out += "{\"origin\":" + std::to_string(e.origin) + ",\"source\":" + std::to_string(e.source) + ",\"target\":" + std::to_string(e.target) + ",\"models\":[";
        bool first = true; for (auto id : e.models) { if (!first) out += ','; first = false; out += std::to_string(id); }
        out += "],\"category\":\""; out += category_name(e.category); out += "\",\"element\":";
        if (e.element) { out += '"'; out += element_name(*e.element); out += '"'; } else out += "null";
        out += ",\"reflections\":" + std::to_string(e.reflections.size());
        out += ",\"power\":" + std::to_string(e.power) + ",\"ability\":\""; out += ability_name(e.ability);
        out += "\",\"amount\":" + std::to_string(e.amount) + '}';
    }
    out += '}'; return out;
}
}
