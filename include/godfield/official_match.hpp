#pragma once
#include "godfield/official_state.hpp"
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
#include <variant>

namespace godfield::official {
// Local executable subset. Public client arithmetic is source-checked; turn,
// gift and death scheduling below is an explicit experimental contract.
inline constexpr const char* match_ruleset = "official-execution-subset-v55";
inline constexpr const char* match_scope = "experimental:236-default-or237-opt-in;external207;external-devils;threshold-sacrifice;external-reserved-grants;not-official-server-equivalence";
inline constexpr const char* self_attack_rule = "experimental:ordinary-main-explicit-self-target;weapon-base-ability-none-reflect-weapon-cut-cost-absorb-hp-attack-twice-atk-by2x-mp-deal-same-damage-or-miracle-base-ability-none-absorb-hp;hit-rate0-no-wide;immediate-zero-defense-one-decision;twice-fresh-launch-payload-two-bodies-stop-on-natural-or-owner-death;other-self-specials-and-redirects-keep-existing-response-route";
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
inline constexpr const char* guardian_rule = "experimental:canonical42-only;auto-after-ordinary-owner-tick-before-gifts;alive-enemy-sources-in-input-order;one-quarter-opportunity-draw-per-present-source;weighted6-5-4-3-2-for-first8;earth285-moon286-fixed-singleton-move-draw;uniform-alive-enemy-for-atk-curse-bribe-takecp-otherwise-alive-allies-including-self;singleton-target-draw;280-all-alive-no-target-draw;single-target-percent-and-darkcloud;no-fog-reselection;counter-death-fifo-before-next-source;stop-on-main-owner-death;no-internal-decisions-or-source-fee-gift;external-main-boundary-one-decision-no-auto-followup;earth-moon-official-general-selection-unsupported-default-canonical-auto-unchanged";
inline constexpr const char* guardian_summon_rule = "experimental:201-and239-current-target;uniform-all10-in-enum-order;one-draw-including-resummon-same-type;normal-consumption-mp-retention-and-gift";
inline constexpr const char* guardian_departure_rule = "experimental:present-guardian-on-positive-direct-body-hp-loss;source-self-damage-before-target-loss;one10-percent-draw-per-stage;lethal-included-before-revival;include-negative-random202;exclude-disease-prefix-debt-tick-and-exchange;launched-payload-unchanged;event-time-fog-viewers-filter-changes";
inline constexpr std::uint32_t match_snapshot_version = 55;
inline constexpr const char* guardian_payload_auto_rule = "experimental:opt-in-local-guardian-payloads-default-false;existing-enemy-input-order-quarter-opportunity-after-owner-tick-before-deferred-gifts;only-successful-earth-moon-opportunities-use-shared-local120-26-selector;automatic-selection-and-body-use-game-rng-replayed-from-main-anchor;external-const-helper-keeps-caller-owned-chance-stream;no-additional-decision-source-items-ordinary-mp-fee-or-operation-gift;58-saves-launch-mp-and-drains-once-202-draws-one-sign-after-target;14-two-fresh-original-frames-with-death-then-counter-fifo-between-hits;fixed-launch-models-guardian-mp-sign-through-redirection-and-departure;stop-remaining-and-future-sources-on-main-owner-death-stop-remaining-on-source-or-target-death;self-nondamaging-body-immediate-with-shared-death-pump;launched-payload-log-only-no-future-rng-or-private-launch-values;external-payload-counter-unchanged-auto-counter-includes-both-kinds;other8-guardians-and-default-canonical285286-unchanged;uniform-base-geometric-extras-and-target-distribution-local-not-official-server-equivalence";
inline constexpr const char* choice_rule = "experimental:player-choice-v1-command-or-fate-request-actor-token-only;no-outcome-recipient-guardian-assignment-or-shuffle-in-player-choice;normal-main-owner-count0-visible-unused-unmasked207;gift-flag-does-not-gate-already-owned207;inventory-order-per-copy-fate-envelopes-plus-one-normal-command-envelope;uniform-bounded-Nplus1-zero-delegates-old-command-policy-rest-select-one-token;no207-response-purchase-delegate-old-policy-with-no-extra-draw;ordinary-discard-and-trade207-roles-preserved;not-uniform-over-all-legal-actions-or-official-policy;grant-and-positive-confusion-main-refuse-with-policy-unchanged;pure-world-game-rng-and-chance-unchanged;request-preflight-does-not-probe-future-outcome-validity;sample-and-choice-step-separate-transactions";
inline constexpr const char* driver_rule = "experimental:local-driver-api1-one-accepted-decision;grant-uses-chance-sample-grant-step;positive-confusion-main-uses-chance-existing-confusion-policy-no207;other-main-response-purchase-use-policy-player-choice;fate-request-resolved-only-by-chance-existing-fate-helper;ordinary-command-does-not-use-external-chance;game-rng-remains-owned-by-match;copy-world-and-trial-policy-and-chance-commit-only-after-full-step-and-result;failure-preserves-world-both-streams-and-outputs-no-reroll;distinct-policy-chance-storage-required-equal-values-allowed;no-new-phase-decision-fee-gift-or-observation-fields;save-match-snapshot-policy-chance-and-driver-api-policy-rule-for-resume;multi-step-rollout-is-sequence-of-transactions-not-one-transaction;existing-sample-action-confusion-fate-grant-and-random-rollout-unchanged;not-all-legal-actions-all-gift-devils-or-official-server-equivalence";
[[nodiscard]] bool match_supported(CardId id) noexcept;
inline constexpr const char* guardian_payload_rule = "experimental:externally-selected-earth-moon-ordinary-model-columns;main-boundary-one-decision;alive-source-owns-explicit-guardian;all-models-in-guardian-category-domain-and-supported;actual-main-append;max18-models;earth120-moon26-standalone-bases;trade-danger-passive-response-only-bases-unsupported;mixed-domain-and-wide-unsupported;explicit-single-alive-target;14-two-fresh-hits-same-target;single-target-percent-and-darkcloud-no-reroll;no-fog-or-dream-reselection;no-source-inventory-retention-ordinary-mp-fee-or-operation-gift;58-launch-source-mp-times2-then-mp0-once;202-one-launch-sign;fixed-origin-guardian-and-launch-values-through-redirects;ordinary-response-counter-death-redraw-and-deferred-gifts;no-owner-tick-gf-or-auto-followup;resume-owner-or-next-alive;official-general-selector-unsupported-local-sampler-separate";
inline constexpr const char* phenomenon_rule = "experimental:external-source-free-phenomena287-288-289-290-291-292-293-294-296;main-boundary-one-injection-decision;global5-unchanged-from-v34-plus289-alive-count-plus3-immediate-atomic-no-rng;targeted291-light50-292-dark30-at75-percent-293-hpplus50-cap99;explicit-single-alive-recipient291-to294;no-target-selector-wide-or-fog-reselection;targeted3-always-response-after-single-target-chance-and-darkcloud;environmental-frame-origin-source-absent-not-player0;190-consume-and-reserve-response-gift-then-cancel-without-return-recipient;source-directed-counters-not-created-without-source;187-self-mp-counter-and112-death-use-real-player-sources;ordinary-defense-filter-revival-death-redraw-and-deferred-gifts;targeted3-original-action-anchor-and-full-response-replay;no-owner-tick-gf-auto-source-items-mp-fee-or-operation-gift;resume-owner-or-next-alive-with-natural-before-cutoff;287-direct-fever-replacement-no-upgrade-or-heavenfall;288-idempotent-fog;290-direct-hp1-not-damage;294-sum-alive-cp-then-zero-all-before-recipient-cap99;296-exact-alive-count-explicit-guardian1-to10;guardian-changes-event-time-fog-filtered;public-model-recipient-only-no-assignment-array-or-cp-total;207-official-selector-unsupported-owned207-requires-fate-api;289-counts-require-dedicated-confusion-command-api;295-requires-dedicated-item-shuffle-api";
inline constexpr const char* item_shuffle_rule = "experimental:external295-fixed-slot-item-permutation-only;main-boundary-one-phenomenon-decision;alive-input-order-then-raw-inventory-order;source-owner-token-pairs-exact-cover;retain-each-alive-hand-count-and-copy-model-used;dead-inventories-unchanged;all-alive-copies-get-contiguous-fresh-global-tokens-including-identity;existing-compatible-fake-preserved-only-for-dream-recipient-otherwise-private-clear;unmasked-copy-no-new-dream-mask;no-selector-or-rng-consumption-redraw-reveal-gift-cost-ability-trigger;no-resource-curse-guardian-owner-tick-gf-or-auto-changes;body-before-cutoff;zero-copies-valid-even-at-token-exhaustion;private-nonrecursive-pre-action-anchor-and-whole-snapshot-replay;public-phenomenon295-only-no-order-origin-model-count-or-token-manifest;generic-phenomenon-action295-unsupported-use-dedicated-api;server-distribution-slot-used-token-fake-policy-unverified";
inline constexpr const char* confusion_rule = "experimental:external-resolved-command-only-no-internal-selector;289-add3-to-every-alive-count-no-clamp;nonnegative-safe-wire-counts-dead-zero;main-positive-owner-only-dedicated-confusion-api;ordinary-main-and-player-sampler-reject-without-policy-rng-change;usual-visible-composition-mp-items-target-and-allocation-validation;one-count-debit-at-launch-before-body-and-cutoff;all-main-kinds-mapped-to-useAttackItems-including-empty-pray-and-auxiliary-discard-trade-exchange;pre-debit-anchor-and-private-confusion-marker-whole-pending-replay;responses-and-purchase-no-debit-and-use-normal-api;no-extra-selector-rng-or-operation-fee;ordinary-body-gifts-owner-tick-gf-auto-and-next-owner;revival-preserves-count-death-clears-natural-end-clears-alive-cutoff-preserves;curse-cures-and-unrelated-external-effects-preserve-count;public-confused-bool-only-with-existing-fog-dead-and-end-filter;server-command-generation-and-selection-distribution-unverified";
inline constexpr const char* fate_rule = "experimental:external-outcome-owned207-only;default-pool236-weight499-optional237-weight500;207-owned-special-capability-opt-in-gift-no-earth-payload;main-owner-single-actual-unused-unmasked207-token;normal207-play-and-sampler-generation-unsupported-discard-and-trade-inert;explicit-one-of-ten-phenomena-no-internal-selector-or-selector-rng;generic9-outcome-shape-and295-only-exact-post-consumption-alive-copy-manifest;reserved-operation-gift-excluded-from-shuffle;one-normal-main-decision-single207-consumption-and-one-reserved-gift-no-mp-fee;positive-confusion-one-pre-outcome-debit;source-free-targeted291-292-293-with-ordinary-response-counter-death-redraw;global6-and295-immediate-body-before-cutoff;ordinary-owner-tick-auto-guardian-deferred-gifts-gf-next-owner-after-whole-action;natural-before-cutoff-pending-response-cancellation-as-normal;private-pre207-nonrecursive-anchor-full-action-and-response-replay-through-completion;public-actor-model207-outcome-model-only-plus-existing-phenomenon-recipient-no-token-manifest-or-count;failure-rolls-back-before207-use;server-outcome-distribution-targets-and-schedule-unverified";
inline constexpr const char* devil_rule = "experimental:external240-to244-explicit-alive-target-only;main-boundary-one-external-decision;no-owned-devil-or-gift-selector-tiebreak-threshold-sacrifice-or-cascade;default236-weight499-optional237-weight500-and-special207-capability;240-241-242-direct-hp-minus10-20-30-no-initial-response-def-reflection-or-counter;positive-hp-loss-uses-local-ten-percent-guardian-departure;243-exact-min2-physical-copies-distinct-owned-token-order-including-used-passives207;no-removal-rng-gift-or-ability-trigger;local-allviewer-actual-model-list-and-existing-fake-reveal-not-server-fanout-proof;244-explicit-hp-mp-cp-plus10-cap99;inactive-resource-and-removal-fields-rejected;shared208-revival112-death-and-response-counter-fifo;no-operation-cost-gift-owner-tick-gf-auto-or-confusion-debit;revival-gift-uses-existing-pool-without-new-devil-roll;resume-live-owner-or-next-alive-without-gf;natural-before-cutoff-body-before-pending-cancel;private-preaction-nonrecursive-anchor-full-response-replay-through-completion;public-model-target-removed-models-only-no-resource-token-or-anchor;failure-atomic;appearance-rate-resource-and-copy-choice-and-server-schedule-unverified";
inline constexpr const char* apocalypse_rule = "experimental:decoded-nonnegative-safe-tiebreak-gf-or-null;active-iff-threshold-present-and-current-gf-at-least-threshold;raw0-active-from-gf0-local-domain-not-an-extra-ui-preset;default-reset-threshold-null-and-standard236-weight499-special207-unchanged;main-discard-before-threshold-sacrifice9-at-or-after-threshold;canonical-ui-operation-replacement-not-server-manual-opcode-acceptance-proof;virtual2-not-owned-or-gifted;one-or-more-distinct-owned-physical-tokens-no-target-allocation-or-mp-fee;visible-discard-eligibility-excludes-weapons208209-includes-used-miracles-compatible-dream-only;erase-all-selected-copies-including-used-then-reserve-one-gift-per-copy;shared-selected-fake-reveal-no-item-ability-retention-or-self-curse-trigger;ordinary-termination-cutoff-owner-tick-revival-death-auto-guardian-deferred-gift-gf-next-owner-schedule;natural-and-cutoff-cancel-gifts-as-normal;confusion-api-one-launch-debit;sampler-single-copy-operation-proposals-switch-kind-without-extra-selector-rng;private-prelaunch-gf-threshold-command-and-whole-pending-journal-replay;public-derived-apocalypse-active-bool-no-private-gift-manifest;no-devil-appearance-selection-gift-cascade-or207-distribution;server-gift-and-turn-order-unverified";
inline constexpr const char* grant_rule = "experimental:opt-in-external-grants-default-false;reserved-gift-pump-apocalypse-with-external-grants-or-all-gf-with-external-artifacts;devils-only-with-external-grants-and-active-apocalypse;reset-immediate-redraw-and-trade-unchanged;phase4-grant-actor-absent-pending-recipient-only;explicit-model0-normal-configured236-weight499-or237-weight500-draw-or-configured-positive-gift-model-or240-to244-devil-payload;explicit-ordinary-nondream-unused-fresh-local-token-no-game-rng;explicit-ordinary-dream-unsupported;no-appearance-resource-or-copy-selector-rng;model0-ordinary-grant-fills-one-debt-with-existing-compatible-dream-mask;explicit-ordinary-fills-one-debt-without-selector-or-disguise;devil-retains-original-slot-for-retry-no-physical-devil-token;208-adds-separate-debt-243-adds-no-replacement;dead-debts-cancelled;shared-direct-devil-body-local-guardian-departure-and-death-response-counter-fifo;resume-lowest-live-input-seat-with-positive-debt;one-accepted-grant-is-one-decision-and-update-for-finite-max-decisions-cutoff;no-repeat-owner-tick-auto-or-confusion-debit-original-gf-advance-only-after-drain;natural-before-cutoff-body-before-work-cancel;source-fate-devil-records-preserved;private-nonrecursive-full-original-action-response-prefix-grant-journal-through-completion;public-pending-target-last-grant-target-devil-or-null-removed-models-only;normal-model-token-resource-remaining-count-and-future-choices-private;normal-commands-and-sampler-refuse-grant-phase-with-policy-rng-unchanged;failed-resolution-rolls-back-to-current-pending-state;25percent-and7-5-3-5-5-ui-rates-not-an-engine-selector;server-distribution-retry-and-order-unverified";
inline constexpr const char* grant_sampler_rule = "experimental:privileged-const-grant-phase-only-local-sampler;caller-owned-chance-rng-separate-from-game-and-policy;normal-only-mode-returns0-without-chance-draw;apocalypse-external-grants-splitmix64-rejection-bounded100-one-ticket;published-absolute-appearance-mass240-7-241-5-242-3-243-5-244-5-ordinary75;id-order-devils-first-ordinary-last;243-uniform-ordered-physical-copies-without-replacement-min2-raw-inventory-partial-fisher-yates-one-draw-per-copy-including-singleton-used-passives-and207-eligible;244-uniform-hp-mp-cp-one-draw-including-capped-resources;resource-and-copy-distributions-and-rng-staging-local-not-official;normal0-defers-actual-artifact-and-compatible-dream-draw-to-grant-step;no-match-state-game-rng-policy-rng-decision-or-public-event-mutation;sampling-failure-preserves-caller-rng;sample-and-step-separate-transactions;chance-rng-not-in-snapshot-controller-must-save-it;existing-explicit-grant-payloads-and-default-behavior-unchanged;no-reset-redraw-or-trade-devil-coverage-no-official-fate-selector;not-official-server-selector-equivalence";
inline constexpr const char* fate_pool_rule = "experimental:opt-in-include-fate-default-false;default236-models-weight499-unchanged;enabled-all237-positive-giftrate-models-weight500-in-catalog-id-order;207-one-ticket-at467-unused-and-unmasked;shared-gift-entry-for-reset-immediate178-redraw-reserved-revival-and-external-normal0;no207-compatible-dream-candidates-no-mask-coin-or-face-draw;existing-non207-compatible-dream-policy-unchanged;normal-command-capability-and-global236-weight499-descriptors-unchanged;current-match-gift-supported-catalog-weight-queries;public-static-gift-pool-flag-count-weight-only;owned207-use-requires-existing-explicit-fate-api;no-normal207-play-or-player-sampler-generation-no-earth-moon207-payload;fate-step-needs-explicit-outcome-even-with-local-chance-helper;no-reset-redraw-or-trade-devil-coverage-added;pool-frequencies-not-official-server-rng-or-schedule-equivalence";
inline constexpr const char* fate_sampler_rule = "experimental:privileged-const-owned207-main-owner-token-local-fate-sampler;caller-owned-chance-rng-separate-from-game-and-policy;splitmix64-rejection-bounded10-model287-plus-ticket-uniform-local-not-published-rates;291-to294-one-uniform-alive-input-seat-recipient-including-owner-and-allies-singleton-draw;296-one-uniform-present-guardian1-to10-per-alive-input-seat-including-existing-type;295-alive-input-seat-raw-copy-source-qualified-manifest-excludes-only-selected-owner-token207-no-reserved-gifts;295-backward-fisher-yates-from-N-to2-one-bounded-i-draw-per-swap-empty-and-singleton-no-draw;used-passive-compatible-fake-and-other207-copies-eligible-dead-copies-excluded;confused-owner-allowed-launch-debit-only-in-fate-step;prepare-fate-validation-before-chance-commit-no-reroll-on-domain-overflow;no-match-state-game-policy-rng-decision-or-public-event-mutation;sampling-failure-preserves-chance-stream;sample-and-step-separate-transactions-late-execution-failure-possible;external-chance-rng-not-in-match-snapshot-controller-checkpoints-it;normal-player-sampler-still-does-not-select207;no-official-confusion-or-earth-moon-general-selector;all-outcome-recipient-guardian-and-permutation-laws-local-not-official-server-equivalence";
inline constexpr const char* confusion_sampler_rule = "experimental:privileged-const-positive-confused-main-owner-only-local-command-sampler;caller-owned-chance-rng-separate-from-game-and-player-policy;shared-visible-normal-proposal-generator-same-order-multiplicity-extra-item-coins-free-mp-target-sell-and-exchange-laws;not-uniform-over-all-legal-actions-and-not-an-official-confusion-distribution;count-gate-bypass-only-no-count-zeroing-or-history-rewrite;final-confusion-prepare-validation-before-chance-commit;no-match-state-game-rng-policy-rng-count-decision-journal-or-public-event-mutation;sampling-failure-preserves-chance-stream-no-reroll;apply-with-existing-confusion-step-one-launch-debit-before-body-and-cutoff;response-purchase-and-grant-not-sampled-here-use-existing-apis;207-fate-generation-excluded-existing-explicit-fate-api-remains;active-threshold-discard-proposal-maps-to-sacrifice-with-same-draws;sample-and-step-separate-transactions-late-step-failure-possible;external-chance-stream-not-in-match-snapshot-controller-checkpoints-it;default-player-sampler-and-existing-manual-confusion-commands-unchanged;server-command-selection-mp-target-and-event-schedule-unverified";
inline constexpr const char* guardian_payload_sampler_rule = "experimental:privileged-const-main-boundary-alive-earth-or-moon-source-inferred-not-necessarily-owner;caller-owned-chance-rng-separate-from-game-and-policy;uniform-id-ordered-existing-supported120-earth-or26-moon-base;not-giftrate-weighted-no-source-mp-hand-slot-confusion-or-curse-filter;prefix-under18-scan-id-order-same-domain-supported-ordinary-append-excluding-trade-and-danger;empty-extra-candidates-stop-without-coin;nonempty-bounded2-zero-stops-one-appends-uniform-candidate-duplicates-allowed;earth88-bases-have26-extras-including-addon-armor-moon-no-extras-and-singleton;18-cap-independent-of-config-hand-limit-no-coin-after-cap;hostile-iff-composed-attack-power-positive-or-base-default-enemy-else-friendly;one-uniform-live-input-seat-enemy-of-source-or-ally-including-source-target-singleton-still-draws;202-sign-and58-mp-effects-deferred-to-body-no-game-rng-peek;final-existing-guardian-payload-prepare-before-chance-commit-no-reroll;no-match-state-game-rng-policy-rng-decision-journal-or-public-event-mutation;sampling-failure-preserves-chance-stream;sample-and-step-separate-transactions-late-step-failure-possible;controller-saves-external-chance-stream-alongside-match-snapshot;existing-64byte-payload-output-no-inventory-tokens-or-mp-payment-added;canonical-auto285286-default-unchanged-helper-separate-from-opt-in-automatic-scheduler;local-narrow-subset-not-all-earthmoon-trade-armor-passive-fate-wrapper-or-official-server-selector-equivalence";
inline constexpr const char* artifact_grant_rule = "experimental:externally-supplied-configured-positive-gift-model-and-optional-compatible-face;current-pending-recipient-only;fake-requires-dream-and-existing-nine-key-compatibility;absent-fake-is-explicit-unmasked-even-in-dream;fresh-local-token-unused-one-debt-one-decision;no-game-or-chance-rng-no-local-dream-draw-statistics;private-typed-full-grant-journal;public-normal-grant-notice-hides-model-and-face;noncompatible-dream-and-official-selector-distribution-unverified";
inline constexpr const char* normal_grant_rule = "experimental:external-artifacts-default-false;pause-reserved-gift-pump-at-all-gf-without-changing-reset-immediate-redraw-or-trade-delivery;normal-only-unless-external-grants-and-apocalypse-active;normal-only-sampler-returns-model0-without-chance-rng;explicit-artifact-or-existing-normal0-or-configured-positive-model;devils-require-existing-apocalypse-mode;one-resolution-one-decision-cutoff-may-cancel-remaining-debt;private-original-anchor-replay-validates-event-time-mode;default-gift-rng-and-sampler-laws-unchanged";
[[nodiscard]] bool match_artifact_grant_supported(CardId id) noexcept;
[[nodiscard]] bool match_grant_supported(CardId id) noexcept;
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
    bool external_grants = false; // Pause active-apocalypse deferred gifts for an external chance driver.
    bool include_fate = false; // Add owned207 to every ordinary gift pool, without enabling a selector.
    bool local_guardian_payloads = false; // Opt-in local Earth/Moon selection at existing automatic opportunities.
    bool external_artifacts = false; // Pause deferred ordinary gifts at every GF; no devil permission by itself.
    bool operator==(const MatchConfig&) const = default;
};
struct PlayerSetup {
    PlayerId id = 0;
    Team team = Team::solo;
    std::string name;
};
enum class MatchPhase : std::uint8_t { main, response, finished, purchase, grant };
enum class CommandKind : std::uint8_t { play, defend, discard, pray, exchange, sell, buy, accept, decline, sacrifice };
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
// A player's intention to use207. Its resolved outcome remains external chance.
struct FateRequest {
    PlayerId actor = 0;
    ItemToken token = 0;
    bool operator==(const FateRequest&) const = default;
};
using PlayerChoice = std::variant<Command,FateRequest>;
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
// Private launch values for the automatic payload's remaining original hits.
struct AutomaticGuardianPayload {
    GuardianPayloadAction action;
    std::int64_t launch_mp = 0;
    int random_hp_delta = 0;
    std::uint8_t cursor = 0;
    bool operator==(const AutomaticGuardianPayload&) const = default;
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
using GrantAction = DevilAction; // model0 draws; configured positive-gift IDs deliver explicitly;240..244 are devils.
struct ArtifactGrantAction {
    PlayerId target = 0;
    CardId model = 0;
    std::optional<CardId> fake_model;
    bool operator==(const ArtifactGrantAction&) const = default;
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
struct GrantResolution {
    std::size_t responses_before = 0;
    std::variant<GrantAction, ArtifactGrantAction> action;
    bool operator==(const GrantResolution&) const = default;
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
    std::uint64_t grant_resolutions = 0, grant_artifacts = 0, grant_devils = 0;
    std::uint64_t guardian_summons = 0, guardian_opportunities = 0, auto_guardian_actions = 0;
    std::uint64_t guardian_departure_rolls = 0, guardian_departures = 0;
    std::size_t guardian_cursor = 0;
    bool guardian_stage_started = false;
    std::optional<AutomaticGuardianPayload> automatic_payload;
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
    std::vector<GrantResolution> grants; // Ordered between responses by the prefix length.
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
    std::uint64_t grant_resolutions = 0, grant_artifacts = 0, grant_devils = 0;
    std::optional<PlayerId> pending_grant; // One environment recipient; no policy actor in phase4.
    std::optional<ActionBatch> grant_proof; // Full original action journal, retained through completion.
    std::optional<DevilEvent> last_grant; // model0 hides any ordinary model; explicit ordinary inputs remain in the private journal.
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
    std::vector<GuardianPayloadAction> last_auto_guardian_payloads; // Opt-in launches only, never future selections.
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
    [[nodiscard]] bool gift_supported(CardId model) const noexcept;
    [[nodiscard]] std::vector<CardId> gift_catalog() const;
    [[nodiscard]] int gift_weight() const noexcept;
    [[nodiscard]] bool legal(const Command& command) const noexcept;
    [[nodiscard]] Command sample_action(std::uint64_t& policy_rng) const;
    [[nodiscard]] bool choice_legal(const PlayerChoice& choice) const noexcept;
    [[nodiscard]] PlayerChoice sample_choice(std::uint64_t& policy_rng) const;
    MatchStep choice_step(const PlayerChoice& choice, std::uint64_t& chance_rng);
    // One accepted decision; world and both external streams commit together.
    // Distinct RNG storage is required, even when their initial values coincide.
    MatchStep rollout_step(std::uint64_t& policy_rng, std::uint64_t& chance_rng);
    // Privileged local chance policy. No match mutation; caller saves this RNG.
    [[nodiscard]] GrantAction sample_grant(std::uint64_t& chance_rng) const;
    [[nodiscard]] bool grant_legal(const GrantAction& action) const noexcept;
    MatchStep grant_step(const GrantAction& action);
    [[nodiscard]] bool artifact_grant_legal(const ArtifactGrantAction& action) const noexcept;
    MatchStep artifact_grant_step(const ArtifactGrantAction& action);
    [[nodiscard]] bool devil_legal(const DevilAction& action) const noexcept;
    MatchStep devil_step(const DevilAction& action);
    [[nodiscard]] FateAction sample_fate(ItemToken token, std::uint64_t& chance_rng) const;
    [[nodiscard]] bool fate_legal(const FateAction& action) const noexcept;
    MatchStep fate_step(const FateAction& action);
    [[nodiscard]] Command sample_confusion(std::uint64_t& chance_rng) const;
    [[nodiscard]] bool confusion_legal(const Command& command) const noexcept;
    MatchStep confusion_step(const Command& command);
    [[nodiscard]] bool guardian_legal(const GuardianAction& action) const noexcept;
    [[nodiscard]] bool item_shuffle_legal(const ItemShuffleAction& action) const noexcept;
    MatchStep item_shuffle_step(const ItemShuffleAction& action);
    [[nodiscard]] bool phenomenon_legal(const PhenomenonAction& action) const noexcept;
    MatchStep phenomenon_step(const PhenomenonAction& action);
    [[nodiscard]] GuardianPayloadAction sample_guardian_payload(PlayerId source, std::uint64_t& chance_rng) const;
    [[nodiscard]] bool guardian_payload_legal(const GuardianPayloadAction& action) const noexcept;
    MatchStep guardian_payload_step(const GuardianPayloadAction& action);
    MatchStep guardian_step(const GuardianAction& action); // Transactional external activation.
    MatchStep step(const Command& command); // Errors leave all state, RNG and counters unchanged.
    [[nodiscard]] std::string observe(std::optional<PlayerId> viewer) const;
private:
    void apply_choice(const PlayerChoice& choice, std::uint64_t& chance_rng); // Only on a transaction copy.
    [[nodiscard]] Command sample_command(std::uint64_t& rng, bool allow_confused) const;
    MatchConfig config_;
    MatchSnapshot data_;
};
}
