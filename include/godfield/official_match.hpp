#pragma once
#include "godfield/official_state.hpp"
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace godfield::official {
// Local executable subset. Public client arithmetic is source-checked; turn,
// gift and death scheduling below is an explicit experimental contract.
inline constexpr const char* match_ruleset = "official-execution-subset-v39";
inline constexpr const char* match_scope = "experimental:236-models;external207;external-devils;not-official-server-equivalence";
inline constexpr const char* bounce_target_rule = "experimental:uniform-all-alive-in-input-order;includes-self-and-teammates";
inline constexpr const char* chance_rule = "experimental:targetless-all-alive-enemies-in-input-order;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll;skip-dead;cancel-on-origin-death";
inline constexpr const char* counter_rule = "experimental:actual-hp-loss;survivors-only;selection-order-fifo-before-next-target;skip-dead-source-or-target;single-target;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll";
inline constexpr const char* resource_counter_rule = "experimental:mp-self-target-with-response;cp-target-incoming-source;debit-current-target-then-full-credit-current-source;resource-debt-no-new-counter";
inline constexpr const char* damage_effect_rule = "experimental:positive-nondark-remainder;current-source-side-effect-before-target-damage;counter-basis-target-damage-only";
inline constexpr const char* flash_rule = "experimental:positive-target-hp-loss-including-lethal;idempotent-until-explicit-cure;counter-to-incoming-source;one-card-response;no-tick";
inline constexpr const char* darkcloud_rule = "experimental:force-percent-hit-without-draw-at-target-activation;redirects-no-recheck;idempotent-until-all-cure;no-tick";
inline constexpr const char* removal_rule = "experimental:unused-hand-max3;used-miracles-max2;uniform-without-replacement-in-inventory-order;bounded-draw-per-removal;empty-no-draw;no-removal-gifts";
inline constexpr const char* mana_weapon_rule = "experimental:base-2x-mp-after-composed-miracle-cost;ordered-boosts;consume-all-mp-once-at-launch;redirects-no-repayment";
inline constexpr const char* repeat_attack_rule = "experimental:two-fresh-passes-over-initial-targets;single-consumption-and-payment;counter-fifo-before-next-hit;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action";
inline constexpr const char* wide_attack_rule = "experimental:initial-alive-enemies-in-input-order;optional-alive-target-does-not-alter-roster;repeat-by-full-roster-pass;single-payment;counter-fifo-before-next-target;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action";
inline constexpr const char* random_hp_rule = "experimental:launch-once;uniform-plus10-or-minus10;zero-heals-one-damages;fixed-through-reflections;nonattack-no-defense-or-counters;hp-clips-0-99;gifts-after-action";
inline constexpr const char* revival_rule = "experimental:post-body-input-order;hp0-and-not-dead;two-not-dead-required;first-copy-once-per-boundary;hp-plus10;one-deferred-gift;counter-basis-pre-revival-loss;passive-and-undiscardable;full-hand-prayer-no-draw";
inline constexpr const char* danger_rule = "experimental:launch-once;first-alive-mortar-in-input-order;otherwise-uniform-all-alive;optional-alive-target-ignored;base99-before-ordered-boosts;consume-one-mortar-with-deferred-gift;attracted-body-no-response;redirects-no-reselection;wide-combination-unsupported";
inline constexpr const char* exchange_rule = "experimental:single-owned-exchange;targetless-self;three-resources-0-99;sum-conserved-before-revival;zero-mp-fee;no-response;one-deferred-gift;death-and-revival-before-gift";
inline constexpr const char* removal_event_rule = "experimental:resolved-body-only;public-removed-models-in-draw-order;duplicates-and-empty-preserved;clear-on-next-accepted-main;no-tokens-or-rng";
inline constexpr const char* sell_rule = "experimental:one-unused-commodity;explicit-alive-target;zero-mp-fee;one-operation-gift;foreign-response-and-reflection;buyer-debit-before-current-source-credit;fresh-token-delivery-before-death;full-recipient-uniform-existing-item-overflow";
inline constexpr const char* buy_rule = "experimental:one-operation-gift;uniform-unused-commodity-in-inventory-order;singleton-one-draw;empty-no-draw;foreign-cp-only-affordability;foreign-buyer-accept-or-decline;foreign-escrow-return-on-decline-or-cutoff;foreign-fresh-token-delivery;foreign-full-recipient-uniform-existing-item-overflow;self-immediate-inspection-after-operation-consumption;self-public-actual-model-and-unmask;self-same-slot-token-and-unused-state;self-no-cp-check-or-transfer-or-quote;self-result-before-cutoff-and-gift-after-action";
inline constexpr const char* disease_rule = "experimental:one-disease-plus-fog-flash-darkcloud-and-dream;reapply-max-incoming-or-next;heaven-reapplication-hp0;owner-only-after-whole-action;natural-and-cutoff-before-tick;fixed5-percent-before-hp;post-upgrade-tick;post-tick-revival-before-gifts";
inline constexpr const char* disease_item_rule = "experimental:response-self-curse-in-selection-order-after-consumption;prefix-before-body;death-and-revival-after-whole-body;counter-basis-body-target-loss-only;mp-boost-before-disease;no-trigger-on-trade-or-discard";
inline constexpr const char* fog_rule = "experimental:launch-once-for-explicit-enemy-play-buy-sell;uniform-alive-enemies-in-input-order;one-draw-including-singleton;self-and-allies-unchanged;chance-wide-danger-excluded-except-sale-commodity;redirects-no-reselection;sampler-sell-all-alive-without-capacity-peek";
inline constexpr const char* death_rule = "experimental:revival-before-actual-death;nonterminal-boundaries-only;consume-all-bows-in-inventory-order;copy-then-alive-enemies-in-input-order;light30-75-percent;death-fifo-before-counters-and-initial-roster;dead-source-allowed;darkcloud-forces-without-draw;redirects-no-reroll;dead-redirect-target-ends-frame;no-fog-reselection-or-gifts;owner-tick-once;natural-and-cutoff-cancel-pending";
inline constexpr const char* overflow_rule = "experimental:trade-delivery-only;full-recipient-uniform-existing-item-in-inventory-order;one-bounded-draw;all-existing-items-including-used-miracles-and-passives;cp-debit-and-credit-before-eviction;eviction-before-fresh-delivery-and-death;no-eviction-gift-or-ability;public-model-only;no-overflow-on-decline-or-cutoff-return";
inline constexpr const char* dream_rule = "experimental:compatible-face-only;key-category-element-ability-curse-cost-hit-rate-plus-atk-positive-def-positive;supported-other-models-in-id-order;gift-real-before-eligible-only-half-coin-zero-masks-before-uniform-face;singleton-no-extra-draw;face-choice-one-draw-including-one-candidate;no-retroactive-mask;trade-receipts-unmasked;legal-and-policy-visible-models;resolve-actual-values-and-price;use-quote-removal-overflow-reveal;quote-return-stays-unmasked;all-cure-clears-dream-and-reveals-hand-only-to-owner;mild-preserves;no-tick;redraw178-same-mask-policy";
inline constexpr const char* redraw_rule = "experimental:response-only178;consume-entire-selection-first;selection-order-self-dream-before-each-redraw;replace-all-current-nonempty-including-used-miracles;same-count-fresh-unused-tokens;immediate-weighted-draw-with-compatible-dream-mask;reserved-gifts-excluded-and-preserved;before-incoming-body-and-revival;no-old-hand-models-or-counts-public;owner-id-only-redraw-notice;same-action-new-items-usable;no-trigger-on-discard-or-trade;max-decisions-bounds-journal";
inline constexpr const char* guardian_rule = "experimental:canonical42-only;auto-after-ordinary-owner-tick-before-gifts;alive-enemy-sources-in-input-order;one-quarter-opportunity-draw-per-present-source;weighted6-5-4-3-2-for-first8;earth285-moon286-fixed-singleton-move-draw;uniform-alive-enemy-for-atk-curse-bribe-takecp-otherwise-alive-allies-including-self;singleton-target-draw;280-all-alive-no-target-draw;single-target-percent-and-darkcloud;no-fog-reselection;counter-death-fifo-before-next-source;stop-on-main-owner-death;no-internal-decisions-or-source-fee-gift;external-main-boundary-one-decision-no-auto-followup;earth-moon-general-selection-unsupported";
inline constexpr const char* guardian_summon_rule = "experimental:201-and239-current-target;uniform-all10-in-enum-order;one-draw-including-resummon-same-type;normal-consumption-mp-retention-and-gift";
inline constexpr const char* guardian_departure_rule = "experimental:present-guardian-on-positive-direct-body-hp-loss;source-self-damage-before-target-loss;one10-percent-draw-per-stage;lethal-included-before-revival;include-negative-random202;exclude-disease-prefix-debt-tick-and-exchange;launched-payload-unchanged;event-time-fog-viewers-filter-changes";
inline constexpr std::uint32_t match_snapshot_version = 39;
[[nodiscard]] bool match_supported(CardId id) noexcept;
inline constexpr const char* guardian_payload_rule = "experimental:externally-selected-earth-moon-ordinary-model-columns;main-boundary-one-decision;alive-source-owns-explicit-guardian;all-models-in-guardian-category-domain-and-supported;actual-main-append;max18-models;earth120-moon26-standalone-bases;trade-danger-passive-response-only-bases-unsupported;mixed-domain-and-wide-unsupported;explicit-single-alive-target;14-two-fresh-hits-same-target;single-target-percent-and-darkcloud-no-reroll;no-fog-or-dream-reselection;no-source-inventory-retention-ordinary-mp-fee-or-operation-gift;58-launch-source-mp-times2-then-mp0-once;202-one-launch-sign;fixed-origin-guardian-and-launch-values-through-redirects;ordinary-response-counter-death-redraw-and-deferred-gifts;no-owner-tick-gf-or-auto-followup;resume-owner-or-next-alive;general-selector-unsupported";
inline constexpr const char* phenomenon_rule = "experimental:external-source-free-phenomena287-288-289-290-291-292-293-294-296;main-boundary-one-injection-decision;global5-unchanged-from-v34-plus289-alive-count-plus3-immediate-atomic-no-rng;targeted291-light50-292-dark30-at75-percent-293-hpplus50-cap99;explicit-single-alive-recipient291-to294;no-target-selector-wide-or-fog-reselection;targeted3-always-response-after-single-target-chance-and-darkcloud;environmental-frame-origin-source-absent-not-player0;190-consume-and-reserve-response-gift-then-cancel-without-return-recipient;source-directed-counters-not-created-without-source;187-self-mp-counter-and112-death-use-real-player-sources;ordinary-defense-filter-revival-death-redraw-and-deferred-gifts;targeted3-original-action-anchor-and-full-response-replay;no-owner-tick-gf-auto-source-items-mp-fee-or-operation-gift;resume-owner-or-next-alive-with-natural-before-cutoff;287-direct-fever-replacement-no-upgrade-or-heavenfall;288-idempotent-fog;290-direct-hp1-not-damage;294-sum-alive-cp-then-zero-all-before-recipient-cap99;296-exact-alive-count-explicit-guardian1-to10;guardian-changes-event-time-fog-filtered;public-model-recipient-only-no-assignment-array-or-cp-total;207-selector-unsupported-owned207-requires-fate-api;289-counts-require-dedicated-confusion-command-api;295-requires-dedicated-item-shuffle-api";
inline constexpr const char* item_shuffle_rule = "experimental:external295-fixed-slot-item-permutation-only;main-boundary-one-phenomenon-decision;alive-input-order-then-raw-inventory-order;source-owner-token-pairs-exact-cover;retain-each-alive-hand-count-and-copy-model-used;dead-inventories-unchanged;all-alive-copies-get-contiguous-fresh-global-tokens-including-identity;existing-compatible-fake-preserved-only-for-dream-recipient-otherwise-private-clear;unmasked-copy-no-new-dream-mask;no-selector-or-rng-consumption-redraw-reveal-gift-cost-ability-trigger;no-resource-curse-guardian-owner-tick-gf-or-auto-changes;body-before-cutoff;zero-copies-valid-even-at-token-exhaustion;private-nonrecursive-pre-action-anchor-and-whole-snapshot-replay;public-phenomenon295-only-no-order-origin-model-count-or-token-manifest;generic-phenomenon-action295-unsupported-use-dedicated-api;server-distribution-slot-used-token-fake-policy-unverified";
inline constexpr const char* confusion_rule = "experimental:external-resolved-command-only-no-auto-selector;289-add3-to-every-alive-count-no-clamp;nonnegative-safe-wire-counts-dead-zero;main-positive-owner-only-dedicated-confusion-api;ordinary-main-and-sampler-reject-without-policy-rng-change;usual-visible-composition-mp-items-target-and-allocation-validation;one-count-debit-at-launch-before-body-and-cutoff;all-main-kinds-mapped-to-useAttackItems-including-empty-pray-and-auxiliary-discard-trade-exchange;pre-debit-anchor-and-private-confusion-marker-whole-pending-replay;responses-and-purchase-no-debit-and-use-normal-api;no-extra-selector-rng-or-operation-fee;ordinary-body-gifts-owner-tick-gf-auto-and-next-owner;revival-preserves-count-death-clears-natural-end-clears-alive-cutoff-preserves;curse-cures-and-unrelated-external-effects-preserve-count;public-confused-bool-only-with-existing-fog-dead-and-end-filter;server-command-generation-and-selection-distribution-unverified";
inline constexpr const char* fate_rule = "experimental:external-outcome-owned207-only;standard-pool236-weight499-unchanged;207-scene-only-special-capability-no-gift-or-earth-payload;main-owner-single-actual-unused-unmasked207-token;normal207-play-and-sampler-generation-unsupported-discard-and-trade-inert;explicit-one-of-ten-phenomena-no-selector-or-selector-rng;generic9-outcome-shape-and295-only-exact-post-consumption-alive-copy-manifest;reserved-operation-gift-excluded-from-shuffle;one-normal-main-decision-single207-consumption-and-one-reserved-gift-no-mp-fee;positive-confusion-one-pre-outcome-debit;source-free-targeted291-292-293-with-ordinary-response-counter-death-redraw;global6-and295-immediate-body-before-cutoff;ordinary-owner-tick-auto-guardian-deferred-gifts-gf-next-owner-after-whole-action;natural-before-cutoff-pending-response-cancellation-as-normal;private-pre207-nonrecursive-anchor-full-action-and-response-replay-through-completion;public-actor-model207-outcome-model-only-plus-existing-phenomenon-recipient-no-token-manifest-or-count;failure-rolls-back-before207-use;server-outcome-distribution-targets-and-schedule-unverified";
inline constexpr const char* devil_rule = "experimental:external240-to244-explicit-alive-target-only;main-boundary-one-external-decision;no-owned-devil-or-gift-selector-tiebreak-threshold-sacrifice-or-cascade;standard236-weight499-and-special207-unchanged;240-241-242-direct-hp-minus10-20-30-no-initial-response-def-reflection-or-counter;positive-hp-loss-uses-local-ten-percent-guardian-departure;243-exact-min2-physical-copies-distinct-owned-token-order-including-used-passives207;no-removal-rng-gift-or-ability-trigger;local-allviewer-actual-model-list-and-existing-fake-reveal-not-server-fanout-proof;244-explicit-hp-mp-cp-plus10-cap99;inactive-resource-and-removal-fields-rejected;shared208-revival112-death-and-response-counter-fifo;no-operation-cost-gift-owner-tick-gf-auto-or-confusion-debit;revival-gift-uses-existing-pool-without-new-devil-roll;resume-live-owner-or-next-alive-without-gf;natural-before-cutoff-body-before-pending-cancel;private-preaction-nonrecursive-anchor-full-response-replay-through-completion;public-model-target-removed-models-only-no-resource-token-or-anchor;failure-atomic;appearance-rate-resource-and-copy-choice-and-server-schedule-unverified";
[[nodiscard]] bool match_devil_supported(CardId id) noexcept;
[[nodiscard]] bool match_fate_supported(CardId id) noexcept;
[[nodiscard]] bool match_item_shuffle_supported(CardId id) noexcept;
[[nodiscard]] bool match_phenomenon_supported(CardId id) noexcept;
[[nodiscard]] bool match_guardian_payload_supported(Guardian guardian, CardId id) noexcept;
[[nodiscard]] bool match_guardian_supported(CardId id) noexcept;
[[nodiscard]] bool match_dream_compatible(CardId actual, CardId face) noexcept;
[[nodiscard]] std::vector<CardId> match_catalog();
[[nodiscard]] int match_gift_weight() noexcept;
struct MatchConfig {
    std::uint32_t max_decisions = 512;
    std::uint8_t initial_hand = 9, hand_limit = 18;
    int initial_hp = 40, initial_mp = 10, initial_cp = 20;
    bool operator==(const MatchConfig&) const = default;
};
struct PlayerSetup {
    PlayerId id = 0;
    Team team = Team::solo;
    std::string name;
};
enum class MatchPhase : std::uint8_t { main, response, finished, purchase };
enum class CommandKind : std::uint8_t { play, defend, discard, pray, exchange, sell, buy, accept, decline };
struct ResourceAllocation {
    std::int32_t hp = 0, mp = 0, cp = 0;
    bool operator==(const ResourceAllocation&) const = default;
};
struct Command {
    PlayerId actor = 0;
    CommandKind kind = CommandKind::play;
    std::vector<ItemToken> items;
    std::optional<PlayerId> target;
    std::optional<ResourceAllocation> allocation = std::nullopt;
    bool operator==(const Command&) const = default;
};
// Privileged externally selected action, not a player command or automatic scheduler.
struct GuardianAction {
    PlayerId source = 0;
    CardId model = 0;
    std::optional<PlayerId> target;
    bool operator==(const GuardianAction&) const = default;
};
// Privileged ordinary-model column selected externally for Earth or Moon.
struct GuardianPayloadAction {
    PlayerId source = 0;
    Guardian guardian = Guardian::none;
    std::vector<CardId> models;
    std::optional<PlayerId> target;
    bool operator==(const GuardianPayloadAction&) const = default;
};
// Source-free external global primitive or targeted launch. Assignment is privileged.
struct PhenomenonAction {
    CardId model = 0;
    std::optional<PlayerId> recipient;
    std::vector<Guardian> guardians;
    bool operator==(const PhenomenonAction&) const = default;
};
struct ItemSource {
    PlayerId owner = 0;
    ItemToken token = 0;
    bool operator==(const ItemSource&) const = default;
};
struct ItemShuffleAction {
    std::vector<ItemSource> order; // One source-qualified entry for every existing alive slot.
    bool operator==(const ItemShuffleAction&) const = default;
};
struct FateAction {
    PlayerId actor = 0;
    ItemToken token = 0;
    PhenomenonAction outcome;
    std::optional<ItemShuffleAction> shuffle; // Required only for295; post-consumption physical copies.
    bool operator==(const FateAction&) const = default;
};
struct FateEvent {
    PlayerId actor = 0;
    CardId phenomenon = 0;
    bool operator==(const FateEvent&) const = default;
};
enum class Resource : std::uint8_t { hp, mp, cp };
struct DevilAction {
    PlayerId target = 0;
    CardId model = 0;
    std::optional<Resource> resource;
    std::vector<ItemToken> removed;
    bool operator==(const DevilAction&) const = default;
};
struct DevilEvent {
    PlayerId target = 0;
    CardId model = 0;
    std::vector<CardId> removed_models; // Explicit local removal display, never physical tokens.
    bool operator==(const DevilEvent&) const = default;
};
struct RedirectFrame {
    PlayerId defender = 0;
    std::vector<CardId> models;
    std::vector<ItemToken> tokens; // Privileged consumption evidence for this response.
    PlayerId target = 0; // Derived reflection target or audited local bounce outcome.
    bool operator==(const RedirectFrame&) const = default;
};
struct EffectFrame {
    PlayerId origin = 0, source = 0, target = 0;
    std::vector<CardId> models;
    std::vector<ItemToken> tokens; // Privileged origin evidence, not part of the policy view.
    Category category = Category::weapons;
    Attribute element;
    std::int64_t power = 0;
    Ability ability = Ability::none;
    int amount = 0;
    PlayerId initial_target = 0;
    std::uint64_t redirect_rng = 0; // Privileged RNG at launch, for exact redirect replay.
    std::vector<RedirectFrame> redirects; // Reaction kind is derived from models and incoming payload.
    int hit_rate = 0; // Original display percentage; an active chance frame has already hit.
    bool is_counter = false;
    bool is_death = false; // Posthumous112 has its own rate/power and allows a dead source.
    bool forced_hit = false; // DarkCloud bypassed the original percentage draw.
    Guardian guardian = Guardian::none; // Public guardian identity for automatic and external payloads.
    bool environmental = false; // True means origin/source are absent; their canonical0 slots are unused, not player0.
    bool operator==(const EffectFrame&) const = default;
};
// Non-recursive privileged launch anchor and accepted response journal. Current
// targets/cursor/models are derived again when the batch is restored.
struct ActionBatch {
    SnapshotState initial_state;
    PlayerId owner = 0;
    std::uint64_t rng = 0, decisions = 0, next_token = 0, chance_rolls = 0, chance_hits = 0;
    std::uint64_t counter_created = 0, counter_started = 0, counter_rolls = 0, counter_hits = 0;
    std::uint64_t chance_forced_hits = 0, counter_forced_hits = 0;
    std::uint64_t revivals = 0; // Privileged cumulative count before the launch.
    std::uint64_t death_bows = 0, death_created = 0, death_started = 0, death_rolls = 0, death_hits = 0, death_forced_hits = 0;
    std::uint64_t dream_rolls = 0, dream_masks = 0;
    std::uint64_t redraws = 0, redrawn_items = 0;
    std::uint64_t guardian_actions = 0;
    std::optional<GuardianAction> guardian_action;
    std::optional<GuardianPayloadAction> guardian_payload_action;
    std::optional<PhenomenonAction> phenomenon_action; // Targeted source-free launch only.
    std::optional<FateAction> fate_action; // Owned207; source-free effect, ordinary owner schedule.
    std::optional<DevilAction> devil_action; // Direct external body, followed only by shared death/response work.
    std::int64_t guardian_payload_mp = 0; // Actual source MP at this payload launch.
    std::uint64_t guardian_payload_actions = 0;
    std::uint64_t phenomenon_actions = 0;
    std::uint64_t fate_actions = 0;
    std::uint64_t devil_actions = 0;
    std::uint64_t guardian_summons = 0, guardian_opportunities = 0, auto_guardian_actions = 0;
    std::uint64_t guardian_departure_rolls = 0, guardian_departures = 0;
    std::size_t guardian_cursor = 0;
    bool guardian_stage_started = false;
    bool tick_processed = false; // Whole-action owner tick already ran before a posthumous response.
    bool confusion_command = false; // Privileged pre-debit launch; never part of a public observation.
    Command attack;
    std::vector<CardId> models;
    std::vector<PlayerId> targets; // Ordered launch roster; attackTwice repeats the whole roster.
    int random_hp_delta = 0; // Drawn once at launch; zero for other abilities.
    std::size_t cursor = 0; // Next initial target; counter frames do not advance it.
    bool chance = false;
    std::size_t counters_created = 0;
    std::vector<Command> responses;
    bool operator==(const ActionBatch&) const = default;
};
struct ItemShuffleRecord {
    ActionBatch anchor; // Non-recursive full pre-action world, RNG and cumulative counters.
    ItemShuffleAction action;
    bool operator==(const ItemShuffleRecord&) const = default;
};
struct FateRecord {
    ActionBatch anchor;
    FateAction action;
    std::vector<Command> responses;
    bool operator==(const FateRecord&) const = default;
};
struct DevilRecord {
    ActionBatch anchor;
    DevilAction action;
    std::vector<Command> responses;
    bool operator==(const DevilRecord&) const = default;
};
struct DangerEvent {
    PlayerId origin = 0, target = 0;
    std::vector<CardId> models;
    bool mortar = false;
    std::int64_t power = 0;
    Attribute element;
    bool operator==(const DangerEvent&) const = default;
};
struct RemovalEvent {
    PlayerId origin = 0, source = 0, target = 0;
    CardId model = 0;
    std::vector<CardId> removed_models; // Public resolved results, in removal order; no physical tokens.
    bool operator==(const RemovalEvent&) const = default;
};
struct SaleEvent {
    PlayerId origin = 0, source = 0, target = 0;
    CardId model = 0;
    int price = 0;
    std::optional<CardId> overflow_model = std::nullopt;
    bool operator==(const SaleEvent&) const = default;
};
struct PurchaseOffer {
    PlayerId origin = 0, buyer = 0, seller = 0;
    ItemInstance item;
    std::size_t slot = 0; // Privileged original inventory position, never part of a policy view.
    bool operator==(const PurchaseOffer&) const = default;
};
enum class PurchaseReason : std::uint8_t { bought, declined, empty, insufficient_cp, cutoff, self_inspection };
struct PurchaseEvent {
    PlayerId origin = 0, buyer = 0, seller = 0;
    CardId model = 0;
    int price = 0;
    PurchaseReason reason = PurchaseReason::empty;
    std::optional<CardId> overflow_model = std::nullopt;
    bool operator==(const PurchaseEvent&) const = default;
};
struct DiseaseTick {
    PlayerId owner = 0;
    Curse before = Curse::none, after = Curse::none;
    int hp_before = 0, hp_after = 0;
    bool worsened = false, fallen = false;
    std::optional<ItemToken> revival_token; // Privileged separation of tick revival from the action body.
    bool operator==(const DiseaseTick&) const = default;
};
struct FogTarget {
    PlayerId owner = 0, intended = 0, target = 0;
    std::vector<CardId> models;
    bool operator==(const FogTarget&) const = default;
};
struct RevealedItem {
    PlayerId owner = 0;
    CardId face = 0, model = 0;
    bool owner_only = false; // Whole-hand cure reveals belong only to that hand owner.
    bool operator==(const RevealedItem&) const = default;
};
enum class GuardianChangeReason : std::uint8_t { summon, departure, phenomenon };
struct GuardianChange {
    PlayerId player = 0;
    Guardian before = Guardian::none, after = Guardian::none;
    GuardianChangeReason reason = GuardianChangeReason::summon;
    std::uint16_t hidden_viewers = 0; // Event-time viewer-seat mask, never serialized.
    bool operator==(const GuardianChange&) const = default;
};
struct MatchSnapshot {
    std::uint32_t version = match_snapshot_version;
    MatchConfig config;
    SnapshotState state;
    PlayerId owner = 0;
    std::optional<PlayerId> actor;
    MatchPhase phase = MatchPhase::finished;
    std::vector<EffectFrame> queue; // One payload; redirects transfer it without cloning/recasting.
    std::optional<ActionBatch> batch;
    std::vector<EffectFrame> counters; // Reserved FIFO, never part of a policy observation.
    std::vector<EffectFrame> deaths; // Posthumous FIFO, distinct from ordinary counters.
    std::vector<std::uint32_t> gifts;
    std::uint64_t rng = 0, decisions = 0, next_token = 0;
    std::uint64_t chance_rolls = 0, chance_hits = 0; // Actual percentage draws/successes, excluding forced hits.
    std::uint64_t counter_created = 0, counter_started = 0, counter_rolls = 0, counter_hits = 0;
    std::uint64_t chance_forced_hits = 0, counter_forced_hits = 0; // Activated without environmental RNG.
    std::uint64_t revivals = 0; // Completed passive consumptions, never a policy observation.
    std::uint64_t death_bows = 0, death_created = 0, death_started = 0, death_rolls = 0, death_hits = 0, death_forced_hits = 0;
    std::uint64_t dream_rolls = 0, dream_masks = 0; // Private eligible-grant coin attempts and generated masks.
    std::uint64_t redraws = 0, redrawn_items = 0; // Private replacement statistics.
    std::uint64_t guardian_actions = 0; // Private accepted canonical external activations.
    std::uint64_t guardian_payload_actions = 0;
    std::uint64_t phenomenon_actions = 0;
    std::uint64_t fate_actions = 0;
    std::uint64_t devil_actions = 0;
    std::optional<DevilRecord> last_devil; // Privileged whole direct action and response proof.
    std::optional<DevilEvent> last_devil_event;
    std::optional<FateRecord> last_fate; // Privileged full launch and response proof, persists through completion.
    std::optional<FateEvent> last_fate_event; // Public actor/207/outcome only, never physical tokens.
    std::optional<ItemShuffleRecord> last_item_shuffle; // Privileged proof, never an observation.
    std::optional<PhenomenonAction> last_phenomenon; // Assignments are private; JSON shows model/recipient only.
    std::optional<GuardianPayloadAction> last_guardian_payload;
    std::uint64_t guardian_summons = 0, guardian_opportunities = 0, auto_guardian_actions = 0;
    std::uint64_t guardian_departure_rolls = 0, guardian_departures = 0;
    std::vector<GuardianAction> last_auto_guardians; // Launched actions only, in this main action.
    std::vector<GuardianChange> last_guardian_changes; // Viewer-filtered at observation.
    std::optional<GuardianAction> last_guardian; // Public supplied action, no random selection state.
    std::vector<PlayerId> last_redraws; // Public owner IDs only, never replacement counts or inventory.
    std::vector<RevealedItem> last_reveals; // Current main action only; JSON filters owner-only hand reveals.
    std::optional<DangerEvent> last_danger; // Current main action's public launch result, without physical tokens.
    std::optional<RemovalEvent> last_removal; // Current main action's resolved removal, including an empty result.
    std::optional<SaleEvent> last_sale; // Resolved commodity and price, without its new physical token.
    std::optional<PurchaseOffer> purchase; // Privileged escrow, anchored by the complete action journal.
    std::optional<PurchaseEvent> last_purchase;
    std::optional<DiseaseTick> last_tick; // Privileged action-end accounting; never part of observe().
    std::optional<FogTarget> last_fog; // Privileged launch selection, not an extra policy observation.
    bool terminated = false, truncated = false;
    std::vector<double> rewards; // Fixed player vector order, never actor-relative.
    std::vector<PlayerId> winners;
    bool operator==(const MatchSnapshot&) const = default;
};
struct MatchStep {
    MatchPhase phase = MatchPhase::finished;
    std::optional<PlayerId> actor;
    bool terminated = false, truncated = false;
    std::uint64_t decisions = 0;
    std::vector<double> rewards;
    std::vector<PlayerId> winners;
    bool operator==(const MatchStep&) const = default;
};
enum class MatchErrorCode { invalid, unsupported };
class MatchError : public std::runtime_error {
public:
    MatchErrorCode code;
    MatchError(MatchErrorCode c, const char* message) : std::runtime_error(message), code(c) {}
};
// Actual public-client resource update semantics, without implicit death/turns.
void change_resource(PlayerState& player, Resource resource, std::int64_t delta);
class Match {
public:
    explicit Match(MatchConfig config = {}); // Initializes two solo players, seed42.
    void reset(std::span<const PlayerSetup> players, std::uint64_t seed);
    void load_scene(SnapshotState state, PlayerId owner, std::uint64_t seed);
    void restore(const MatchSnapshot& snapshot); // Validates completely, then replaces atomically.
    [[nodiscard]] const MatchSnapshot& snapshot() const noexcept { return data_; }
    [[nodiscard]] bool done() const noexcept { return data_.phase == MatchPhase::finished; }
    [[nodiscard]] bool legal(const Command& command) const noexcept;
    [[nodiscard]] Command sample_action(std::uint64_t& policy_rng) const;
    [[nodiscard]] bool devil_legal(const DevilAction& action) const noexcept;
    MatchStep devil_step(const DevilAction& action);
    [[nodiscard]] bool fate_legal(const FateAction& action) const noexcept;
    MatchStep fate_step(const FateAction& action);
    [[nodiscard]] bool confusion_legal(const Command& command) const noexcept;
    MatchStep confusion_step(const Command& command);
    [[nodiscard]] bool guardian_legal(const GuardianAction& action) const noexcept;
    [[nodiscard]] bool item_shuffle_legal(const ItemShuffleAction& action) const noexcept;
    MatchStep item_shuffle_step(const ItemShuffleAction& action);
    [[nodiscard]] bool phenomenon_legal(const PhenomenonAction& action) const noexcept;
    MatchStep phenomenon_step(const PhenomenonAction& action);
    [[nodiscard]] bool guardian_payload_legal(const GuardianPayloadAction& action) const noexcept;
    MatchStep guardian_payload_step(const GuardianPayloadAction& action);
    MatchStep guardian_step(const GuardianAction& action); // Transactional external activation.
    MatchStep step(const Command& command); // Errors leave all state, RNG and counters unchanged.
    [[nodiscard]] std::string observe(std::optional<PlayerId> viewer) const;
private:
    MatchConfig config_;
    MatchSnapshot data_;
};
}
