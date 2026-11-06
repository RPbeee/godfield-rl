#include "godfield/official_match.hpp"
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

namespace {
using namespace godfield::official;
void check(bool ok, const char* message) { if (!ok) { std::cerr << "FAIL match view: " << message << '\n'; std::exit(1); } }
const char* purchase_reason_name(PurchaseReason reason) {
    switch (reason) {
    case PurchaseReason::bought: return "bought";
    case PurchaseReason::declined: return "declined";
    case PurchaseReason::empty: return "empty";
    case PurchaseReason::insufficient_cp: return "insufficient-cp";
    case PurchaseReason::cutoff: return "cutoff";
    case PurchaseReason::self_inspection: return "self_inspection";
    }
    std::abort();
}
// Serialize only the public typed value; no access to Match or its snapshot.
std::string serialize(const MatchView& data_) {
    const char* phase = data_.phase == MatchPhase::main ? "main" : data_.phase == MatchPhase::response ? "response" : data_.phase == MatchPhase::purchase ? "purchase" : data_.phase==MatchPhase::grant ? "grant" : "finished";
    const auto& board = data_.board;
    std::string out = "{\"phase\":\""; out += phase;
    out += "\",\"gift_pool\":{\"include_fate\":"; out += data_.gift_pool.include_fate?"true":"false";
    out += ",\"models\":"+std::to_string(data_.gift_pool.models)+",\"weight\":"+std::to_string(data_.gift_pool.weight)+'}';
    out += ",\"local_guardian_payloads\":"; out += data_.local_guardian_payloads?"true":"false";
    out += ",\"external_artifacts\":"; out += data_.external_artifacts?"true":"false";
    out += ",\"apocalypse_active\":"; out += data_.apocalypse_active?"true":"false";
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
    if (!data_.last_devil) out+="null";
    else {
        const auto& event=*data_.last_devil;
        out+="{\"model\":"+std::to_string(event.model)+",\"target\":"+std::to_string(event.target)+",\"removed_models\":[";
        for (std::size_t i=0;i<event.removed_models.size();++i) {if(i)out+=',';out+=std::to_string(event.removed_models[i]);}
        out+="]}";
    }
    out += ",\"last_fate\":";
    if (!data_.last_fate) out+="null";
    else out+="{\"actor\":"+std::to_string(data_.last_fate->actor)+",\"model\":207,\"phenomenon\":"+std::to_string(data_.last_fate->phenomenon)+'}';
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
    bool first_change=true;
    for(const auto& event:data_.last_guardian_changes) {
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
    for (const auto& event : data_.last_reveals)  {
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
            + ",\"model\":" + std::to_string(offer.model) + ",\"price\":" + std::to_string(offer.price) + '}';
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
    if (!data_.pending) out += "null";
    else {
        const auto& e = *data_.pending;
        out += "{\"origin\":"; out+=e.origin?std::to_string(*e.origin):"null";
        out += ",\"source\":"; out+=e.source?std::to_string(*e.source):"null";
        out += ",\"target\":" + std::to_string(e.target) + ",\"models\":[";
        bool first = true; for (auto id : e.models) { if (!first) out += ','; first = false; out += std::to_string(id); }
        out += "],\"category\":\""; out += category_name(e.category); out += "\",\"element\":";
        if (e.element) { out += '"'; out += element_name(*e.element); out += '"'; } else out += "null";
        out += ",\"reflections\":" + std::to_string(e.reflections);
        out += ",\"bounces\":" + std::to_string(e.bounces);
        out += ",\"hit_rate\":" + std::to_string(e.hit_rate);
        out += ",\"counter\":"; out += e.counter ? "true" : "false";
        out += ",\"environmental\":"; out += e.environmental ? "true" : "false";
        // The flag belongs to activation, before any reflection. Redirection
        // to a visible pane must not reveal the original hidden recipient.
        out += ",\"death\":"; out += e.death ? "true" : "false";
        out += ",\"guardian\":";
        if (e.guardian == Guardian::none) out += "null";
        else { out += '"'; out += guardian_name(e.guardian); out += '"'; }
        out += ",\"forced_hit\":"; out += !e.forced_hit ? "null" : *e.forced_hit ? "true" : "false";
        out += ",\"power\":" + std::to_string(e.power) + ",\"ability\":\""; out += ability_name(e.ability);
        out += "\",\"amount\":" + std::to_string(e.amount) + '}';
    }
    out += '}'; return out;
}
std::size_t positions = 0, projections = 0, games = 0;
std::set<std::string> coverage;
void compare(const Match& game) {
    ++positions;
    const auto before = game.snapshot();
    std::vector<std::optional<PlayerId>> viewers{std::nullopt};
    for (const auto& player : before.state.players) viewers.push_back(player.id);
    for (auto viewer : viewers) {
        const auto value = game.view(viewer);
        check(game.snapshot() == before, "view alone preserves the complete snapshot and RNG");
        const auto json = game.observe(viewer);
        const auto start = json.find("\"phase\":");
        check(start != std::string::npos, "JSON phase exists");
        if (serialize(value) != "{" + json.substr(start)) {
            std::cerr << "At decision " << before.decisions << " viewer " << (viewer ? std::to_string(*viewer) : "spectator") << '\n';
            check(false, "typed serialization equals observe without constant headers");
        }
        check(game.snapshot() == before, "view and observe preserve the complete snapshot and RNG");
        ++projections;
#define SEEN(field) if (value.field) coverage.insert(#field)
        SEEN(pending_grant); SEEN(last_grant); SEEN(last_devil); SEEN(last_fate); SEEN(last_phenomenon);
        SEEN(last_guardian_payload); SEEN(last_guardian); SEEN(last_danger); SEEN(last_removal);
        SEEN(last_sale); SEEN(purchase); SEEN(last_purchase);
#undef SEEN
        if (!value.last_auto_guardian_payloads.empty()) coverage.insert("auto_payload");
        if (!value.last_auto_guardians.empty()) coverage.insert("auto_guardian");
        if (!value.last_guardian_changes.empty()) coverage.insert("guardian_change");
        if (!value.last_reveals.empty()) coverage.insert("reveal");
        if (!value.last_redraws.empty()) coverage.insert("redraw");
        if (value.pending) {
            if (value.pending->reflections) coverage.insert("reflection");
            if (value.pending->bounces) coverage.insert("bounce");
            if (!value.pending->forced_hit) coverage.insert("hidden_hit");
            if (value.pending->environmental) {
                coverage.insert("environmental");
                check(!value.pending->origin && !value.pending->source, "environment has no player source");
            }
        }
    }
}
PlayerState player(PlayerId id, std::initializer_list<CardId> models = {}) {
    PlayerState p; p.id = id; p.name = "p" + std::to_string(id);
    for (auto model : models) p.items.push_back({p.items.size(), model, {}, false});
    return p;
}
SnapshotState scene(std::initializer_list<CardId> own = {}, std::initializer_list<CardId> other = {}) {
    SnapshotState s; s.players = {player(42, own), player(0, other), player(7)}; return s;
}
Match fixture(SnapshotState state, std::uint64_t seed = 42, MatchConfig config = {}) {
    config.max_decisions = 64;
    Match game(config); game.load_scene(std::move(state), 42, seed); ++games; compare(game); return game;
}
void finish(Match& game, std::uint64_t seed = 0) {
    std::uint64_t policy = seed, chance = seed + 17;
    while (!game.done()) { compare(game); game.rollout_step(policy, chance); compare(game); }
}
void step(Match& game, Command command) { compare(game); game.step(command); compare(game); }
void random_games() {
    for (std::uint64_t seed = 0; seed < 32; ++seed) {
        MatchConfig cfg; cfg.max_decisions = 96; cfg.include_fate = seed & 1;
        cfg.external_artifacts = seed & 2; cfg.external_grants = seed & 4; cfg.local_guardian_payloads = seed & 8;
        Match game(cfg); std::vector<PlayerSetup> roster;
        for (unsigned i = 0; i < (seed % 3 == 0 ? 9u : seed % 3 == 1 ? 3u : 2u); ++i)
            roster.push_back({i == 0 ? 42u : i == 1 ? 0u : 100u + i, Team::solo, "seat" + std::to_string(i)});
        game.reset(roster, seed); ++games; compare(game); finish(game, seed);
    }
}
void special_scenes() {
    // Fog hides activation at the initial target even after reflection.
    auto state = scene({190}, {190}); state.players[0].guardian = Guardian::mars;
    state.players[0].curses = {Curse::fog}; state.players[1].curses = {Curse::darkcloud};
    auto reflected = fixture(state); reflected.guardian_step({42,245,0}); compare(reflected);
    step(reflected, {0,CommandKind::defend,{0},{}});
    check(!reflected.view(42).pending->forced_hit, "reflection preserves hidden initial activation"); finish(reflected);
    auto bounced = fixture(scene({6},{21})); step(bounced,{42,CommandKind::play,{0},0});
    step(bounced,{0,CommandKind::defend,{0},{}}); finish(bounced);
    // Public quote and accept/decline; escrow token/slot never enter MatchView.
    for (auto response : {CommandKind::accept, CommandKind::decline}) {
        auto trade = fixture(scene({5},{6})); step(trade,{42,CommandKind::buy,{0},0});
        step(trade,{0,CommandKind::defend,{},{}}); check(trade.view({}).purchase.has_value(), "BUY quote reached");
        step(trade,{42,response,{},{}}); finish(trade);
    }
    auto sale = fixture(scene({4,6},{})); step(sale,{42,CommandKind::sell,{0,1},0});
    step(sale,{0,CommandKind::defend,{},{}}); finish(sale);
    auto cure_state = scene({200},{6}); cure_state.players[1].curses = {Curse::dream}; cure_state.players[1].items[0].fake_model = 19;
    auto cured = fixture(cure_state); step(cured,{42,CommandKind::play,{0},0}); step(cured,{0,CommandKind::defend,{},{}});
    check(cured.view(42).last_reveals.empty() && cured.view({}).last_reveals.empty() && !cured.view(0).last_reveals.empty(), "whole hand reveal is owner only"); finish(cured);
    auto redraw = fixture(scene({6},{178,6})); step(redraw,{42,CommandKind::play,{0},0}); step(redraw,{0,CommandKind::defend,{0},{}}); finish(redraw);
    bool hidden_departure = false;
    for (std::uint64_t seed = 0; seed < 128 && !hidden_departure; ++seed) {
        auto changed = scene(); changed.players[0].guardian = Guardian::mars; changed.players[1].curses = {Curse::fog};
        auto game = fixture(changed,seed); game.devil_step({42,240,{},{}}); compare(game);
        hidden_departure |= !game.view({}).last_guardian_changes.empty();
        check(game.view(0).last_guardian_changes.empty(), "event-time fog mask uses seat rather than numeric ID"); finish(game,seed);
    }
    check(hidden_departure, "seed sweep exercises a hidden departure event");
    for (CardId model : {CardId{287},CardId{288},CardId{289},CardId{290},CardId{291},CardId{292},CardId{293},CardId{294},CardId{296}}) {
        auto game = fixture(scene({}, {190})); PhenomenonAction action{model,{}, {}};
        if (model >= 291 && model <= 294) action.recipient = 0;
        if (model == 296) action.guardians = {Guardian::earth,Guardian::moon,Guardian::mars};
        game.phenomenon_step(action); compare(game); finish(game);
    }
    std::set<CardId> outcomes;
    for (std::uint64_t seed = 0; seed < 256 && outcomes.size() < 10; ++seed) {
        Match probe; probe.load_scene(scene({207},{190}),42,42); auto chance = seed;
        const auto fate = probe.sample_fate(0,chance); if (!outcomes.insert(fate.outcome.model).second) continue;
        auto game = fixture(scene({207},{190})); game.fate_step(fate); compare(game); finish(game,seed);
    }
    check(outcomes.size()==10,"all ten Fate outcomes");
    auto payload_state = scene(); payload_state.players[0].guardian = Guardian::earth;
    auto payload = fixture(payload_state); payload.guardian_payload_step({42,Guardian::earth,{6},0}); compare(payload); finish(payload);
    // All supported standalone main actions, including danger and removal.
    for (auto model : match_catalog()) {
        auto game = fixture(scene({model},{190,178,187}));
        Command command{42,CommandKind::play,{0},0};
        if (!game.legal(command)) command.target = {};
        if (game.legal(command)) step(game,command);
        finish(game,model);
    }
    // Opt-in automatic payloads and active apocalypse external grants.
    for (std::uint64_t seed = 0; seed < 16; ++seed) {
        auto s = scene({},{}); s.players[1].guardian = Guardian::earth; s.players[2].guardian = Guardian::moon; s.tiebreak_gf = 0;
        MatchConfig cfg; cfg.include_fate = true; cfg.local_guardian_payloads = true; cfg.external_grants = true; cfg.external_artifacts = true;
        auto game = fixture(s,seed,cfg); finish(game,seed);
    }
}
void noninterference() {
    auto a = scene({6},{6}); a.players[1].curses = {Curse::dream};
    auto b = a; b.players[1].items = {{100,57,19,false},{0,7,6,false},{999,19,27,false}};
    auto first = fixture(a), second = fixture(b);
    for (auto viewer : {std::optional<PlayerId>{42},std::optional<PlayerId>{7},std::optional<PlayerId>{}})
        check(first.view(viewer)==second.view(viewer),"foreign unused count/model/token/fake_model do not leak");
    auto other_seed = fixture(a,999);
    for (auto viewer : {std::optional<PlayerId>{42},std::optional<PlayerId>{0},std::optional<PlayerId>{7},std::optional<PlayerId>{}})
        check(first.view(viewer)==other_seed.view(viewer),"seed and RNG do not leak");
}
}
int main() {
    random_games(); special_scenes(); noninterference();
    for (const auto* field : {"pending_grant","last_grant","last_devil","last_fate","last_phenomenon","last_guardian_payload","last_guardian","last_danger","last_removal","last_sale","purchase","last_purchase","auto_payload","auto_guardian","guardian_change","reveal","redraw","reflection","bounce","hidden_hit","environmental"})
        check(coverage.contains(field),field);
    std::cout << "Match view passed: " << games << " games/scenes, " << positions << " positions, " << projections << " viewer projections; " << coverage.size() << " special event kinds.\n";
}
