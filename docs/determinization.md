# C++ determinization (P3 / D6)

`Match::determinize_support(PlayerId)` returns `DeterminizeSupport` without
mutation or allocation. `Match::determinized(viewer, span<CardId>, seed)` returns
an independent, fully `restore()`-validated match. There is no C/Python API or
snapshot-schema change.

Only **two solo players** are supported. Assignment is the opponent's unused
real-model multiset, including zero copies. Every model must be in the match's
configured `gift_catalog()` (positive gift rate, executable, and 207 only with
`include_fate`). The physical count **including retained used miracles** must
fit `config.hand_limit`, not just the renderer's maximum18. Fresh tokens use
`next_token`, including the final permitted `max_wire_integer`; exhaustion is
`MatchError(invalid)`. All unsupported boundaries throw `MatchError(unsupported)`
before validating assignment. The source is unchanged on success or failure.

## Main boundary and reason precedence

First applicable reason below wins. `ok` means none applies.

| Reason | Check and rationale |
| --- | --- |
| `invalid_viewer` | Viewer must identify a player. |
| `unsupported_players` | Exactly two players, both `Team::solo`; a single assignment cannot describe multiple opponents. |
| `not_main` | On the main path, must be main, not purchase, grant, or finished. Response has the separate path below. |
| `not_actor` | Both actor and owner must equal viewer. |
| `pending_work` | `queue`, `batch`, `counters`, `deaths`, `pending_grant`, `purchase` must be empty. These contain active effects, action replay anchors, deferred counters/death attacks or purchase escrow. |
| `reserved_gifts` | Every `gifts` entry must be zero; the vector itself has one entry per seat. A debt would refer to pre-replacement physical capacity and a still-running action. |
| `replay_proof` | All five `*_proof` optionals, `last_fate`, `last_devil`, and `last_item_shuffle` must be empty. Some persist AFTER main resumes: restore replays from their old whole-world anchor and requires equality, including RNG/inventories. They cannot be retained across a replacement, and clearing them would break the existing snapshot/event contract. |
| `hidden_status` | Viewer has Fog. `project_view` hides the enemy's status because the **viewer**, not the enemy, has Fog. Enemy-only Fog is supported. |
| `hidden_confusion` | Either player's `confusion_count > 0`. Both panes expose only `confused`, while the actual remaining count changes later chance/normal turns. Even the owner cannot see it through view. |
| `hidden_own_model` | Any viewer item has `fake_model != model`. Identity disguises are supported: the disguised marker is public and actual model is already known. Enemy disguises disappear with replaced unused items. |

`validate_snapshot` already enforces empty queues, batch, debts, live actor and
owner equality in normal main states. The explicit defensive checks document
the boundary rather than depending on that implicit invariant. `advance()`
finishes tick, death/counter queues, automatic guardians and gifts before
resetting batch and advancing owner/GF. Ordinary completed `last_*` notices do
NOT mean work is pending: `clear_events()` clears them before the next action.
A direct external activation that returns to main is also a quiescent boundary
when it has no retained replay proof.

## Field-by-field information audit

The audit covers `PlayerState`, `ItemInstance`, `SnapshotState`, and every
`MatchSnapshot` field. “Historical” below means already-resolved information
cleared by the next `clear_events`, not a future hidden random outcome. D6 is
about hidden gameplay facts, not identity strings, physical implementation IDs,
validation metadata, or past accounting.

| Struct / fields | Visible through `view(viewer)`? | Treatment / code evidence |
| --- | --- | --- |
| PlayerState: `id`, `team`, `dead` | Yes | Preserve. |
| PlayerState: `name`, `alias`, `user_id` | Display name only; raw identity may be absent | Preserve metadata. Execution uses player IDs/teams; these strings have no effect on choices, RNG or transitions. `user_id` is only used by the standalone viewer-resolution helper. |
| PlayerState: `hp`, `mp`, `cp`, `curses`, `guardian` | Yes, except fogged opponent | Reject hidden status. `closed_state` requires curses sorted, unique, with at most one disease; therefore displayed icon normalization loses no extra curse information. |
| PlayerState: `confusion_count` | Only `> 0` for living players | Reject any positive count on either side. Zero is determined by the visible false flag. |
| PlayerState: `items` | Own hand and opponent sorted used models; no opponent unused count | Replace only opponent unused items; preserve viewer inventory and opponent used items. |
| ItemInstance: `model`, `fake_model`, `used` (viewer) | Face, reference, disguised and used; actual model may be hidden | Reject differing own actual/face. Identity disguise contains no hidden model. |
| ItemInstance: `model`, `fake_model`, `used` (opponent) | Used actual models only (`closed_state` forbids used disguises) | Unused models/count/disguises are precisely the sampled hand. Keep used objects unchanged. |
| ItemInstance: `token`, vector order | Own tokens visible; opponent tokens/order absent; own view sorted by face/used | Physical representation, not an extra hidden card fact. See order convention below. Preserve used token identities and relative order. Allocate new tokens beyond every live token. |
| SnapshotState: `gf`, `tiebreak_gf`, `is_over`, `players` | Yes, with player filtering above | Preserve. |
| SnapshotState: `update_count` | No | Update/version bookkeeping, incremented by `complete_decision`; checked against wire integer exhaustion. Preserved, never used to select a card/effect. |
| MatchSnapshot: `version`, `config` | Version absent; config partially visible (`gift_pool`, local/external modes) | Fixed execution rules, not sampled hidden world facts. Preserve all limits/modes; no version bump. |
| MatchSnapshot: `state`, `owner`, `actor`, `phase`, `decisions`, `terminated`, `truncated` | Yes, with state filtering above | Preserve; terminal states rejected. Decisions affect cutoff and are public. |
| MatchSnapshot: `queue`, `batch`, `counters`, `deaths` | Pending effect only partially projected; journals/FIFOs private | Require empty; no in-flight launch RNG, private responses, or scheduled attacks can survive. |
| MatchSnapshot: `gifts`, `pending_grant`, `purchase` | Debt counts private; current grant recipient and purchase quote public | Require zero/absent, including private purchase escrow token/slot. |
| MatchSnapshot: `grant_proof`, `redraw_proof`, `purchase_proof`, `summon_proof`, `automatic_guardian_proof`, `last_fate`, `last_devil`, `last_item_shuffle` | Whole-state proofs absent; some have public notices | Reject retained whole-world replay anchors, including completed actions. |
| MatchSnapshot: `rng` | No | Deliberately resample future game randomness: assign seed as reset/load_scene do. External chance/policy RNG belongs to caller and is untouched. |
| MatchSnapshot: `next_token` | No | Physical identifier allocator, not a sampled gameplay fact. Preserve starting value, allocate monotonically, validate budget. |
| MatchSnapshot: `chance_rolls`, `chance_hits`, `counter_created`, `counter_started`, `counter_rolls`, `counter_hits`, `chance_forced_hits`, `counter_forced_hits`, `revivals`, `death_bows`, `death_created`, `death_started`, `death_rolls`, `death_hits`, `death_forced_hits` | No | Cumulative accounting only. Read by validation and per-action before/after deltas, never to choose future effects/cards. Preserve. |
| MatchSnapshot: `dream_rolls`, `dream_masks`, `redraws`, `redrawn_items`, `guardian_actions`, `guardian_payload_actions`, `phenomenon_actions`, `fate_actions`, `devil_actions`, `grant_resolutions`, `grant_artifacts`, `grant_devils`, `guardian_summons`, `guardian_opportunities`, `auto_guardian_actions`, `guardian_departure_rolls`, `guardian_departures` | No | Cumulative statistics and validation bounds, not hidden future state. Preserve. |
| MatchSnapshot: `last_decision` | Separate `last_decision(viewer)` API; event-time visibility mask private | Historical accepted-decision notice. Preserve full record and event-time filtering; never future effect state. |
| MatchSnapshot: `last_grant`, `last_devil_event`, `last_fate_event`, `last_guardian_payload`, `last_auto_guardians`, `last_auto_guardian_payloads`, `last_guardian`, `last_redraws`, `last_danger`, `last_removal`, `last_sale`, `last_purchase` | Yes | Resolved notices; preserve. Notices requiring a replay proof are already excluded above. |
| MatchSnapshot: `last_phenomenon` | Model/recipient visible, guardian assignments omitted | Historical result. For296 assignments have already become players' current guardians (visible unless Fog, rejected). Shuffle295 requires rejected proof. No pending assignments. |
| MatchSnapshot: `last_guardian_changes`, `last_reveals` | Filtered by viewer/owner; visibility metadata absent | Past changes/reveals, including old private hand facts. No transition consumes them as future state; preserve, including filtering. |
| MatchSnapshot: `last_tick`, `last_fog` | No | Resolved disease/revival accounting and original fog target selection. Next action clears them; no future branch uses their private details. Preserve (restore validates accounting). |
| MatchSnapshot: `rewards`, `winners` | Not in MatchView | In an accepted running main both are fixed: zero reward vector and empty winners, checked by restore. Thus no hidden outcome. |

### Inventory order convention

Internal order is used by uniform purchase/removal/overflow draws, first revival
copy, and posthumous bows. It is not interchangeable for a **fixed** RNG stream.
The assignment has no order, so the new representation is specified explicitly:
retain used objects in their existing relative order, then append unused copies
sorted by real model, assigning tokens in that order. Input permutations thus
produce identical snapshots. This selects a canonical representation of the
sampled hand rather than carrying the original opponent unused order into it.
Used miracles' hidden physical order only indexes uniform draws; they are not
passive revival/bow triggers. Keeping that order does not introduce a hidden
status or future scheduled effect. In particular, public used-model multisets
and all viewer fields stay identical. This API does not claim equality of
future sampled actions for different physical inventory representations.

## Tests and measurement

Build/test as in README (Release and ASan/UBSan). The new CTest entry is
`official_determinized`; its executable also provides a reproducible benchmark:

```sh
build/godfield_official_determinized_tests --measure 10000
```

Default MatchConfig (max512 decisions, initial9, limit18), game seeds0..9999,
policy RNG seed=game+101, chance RNG seed=game+203; ordinary moves use
`sample_action`. Grant/confusion routes use their existing chance helpers.
The denominator counts the active player's main boundary once, not both viewers
of that boundary. Counts below use mutually exclusive first-reason precedence.

| Result | Seat0 (118,962 main starts) | Seat1 (113,932 main starts) |
| --- | ---: | ---: |
| ok | 111,647 (93.8510%) | 106,292 (93.2942%) |
| hidden_status | 4,091 (3.4389%) | 4,157 (3.6487%) |
| hidden_own_model | 3,224 (2.7101%) | 3,483 (3.0571%) |
| Other reasons (each) | 0 (0%) | 0 (0%) |

Default gifts exclude207, so no global289 confusion or Fate proof is generated
by this policy. Those reasons are exercised by explicit fixtures; the benchmark
does not estimate their frequency in opt-in modes. Hidden-status rejection has
priority over Dream, so overlapping causes count under hidden_status.

The tests compare exact typed observations and last_decision, legal kinds,
legal_commands (two-card domain, exchange step33), and legal extension roots
and one-card prefixes. They verify random multiset assignments at 2,079
supported boundaries, complete120 sampled-world games (including opt-in Fate, external
artifact grants and local guardian payloads), check source immutability and
restore equality, and cover capacity, absent/non-gift models, include_fate,
identity/nonidentity Dream, Fog, confusion, persistent proofs, and token bounds.
Different game seeds changed future gifts in128/128 fixed-action pairs.

Known legality exception: exact94+232 is only supported when no player has a
true209. A dedicated fixture replaces the empty opponent hand with209 and
finds three disappearing legal commands (target absent/self/opponent). The
232 extension after94 disappears as well. Observation, last_decision and
legal_kinds stay equal. Tests permit ONLY this exact ordered pair as a
command difference, not an unrestricted mismatch. Existing legality and
observation semantics are unchanged.


## Ordinary response boundaries

The C++ API also supports `phase == response`, `actor == viewer`, and
`owner != viewer`. The two-solo-player restriction and the main-path reason
precedence/values are unchanged. New reasons are **appended**:
`response_special=11`, `response_random=12`, `response_history=13`.
The parent `search.py` `_SUPPORT` table does not yet contain them.

Response precedence is invalid viewer, unsupported players, not actor,
special work, random launch, resolved/nonreflection history, hidden status,
hidden confusion, hidden own model. During redirect inspection, redraw and
bounce receive special/random respectively. Support is allocation-free and
never compares RNG states or examines unplayed opponent models.

“Ordinary” here means a hand `play`, before any body resolves, optionally
followed by deterministic **reflection-only** responses (including attribute
filters and ordinary self curses). A resolved first hit of `attack_twice`,
reserved counters, and posthumous work are conservatively outside this subset.
Their small measured frequency is reported below rather than attempting to
recover card removals, revivals and private random body outcomes.

The following are excluded:

- `guardian_action`, `guardian_payload_action`, `phenomenon_action`, `fate_action`,
  `devil_action`, confusion launches, supplied summons, automatic guardian stages,
  ticked batches, grants, redraw/purchase/automatic result records; all five
  `*_proof`, `last_fate`, `last_devil`, and `last_item_shuffle` anchors.
  Buy and sell are excluded by requiring `attack.kind == play`.
- Danger (including Wide) can inspect a hidden mortar or sample its target;
  the HP gamble samples its launch sign. Ordinary percentage hits consume RNG,
  except a publicly DarkCloud-forced hit. Fogged ordinary single-target launches
  consume RNG even with only one enemy. All are `response_random`.
- A response history must consist entirely of the currently pending frame's
  redirects, and each must end in reflection without any bounce. A bounce draw
  is rejected even if its outcome happens to equal a deterministic target.
  Redraw is excluded. These tests use public launch/response cards and status,
  not whether the privileged RNG happened to change.

### Information audit for in-flight fields

| Field | Visible to viewer? / depends on opponent unused hand? | Handling |
| --- | --- | --- |
| `queue` active origin/source/target/models/category/element/power/ability/amount, hit/forced flags | Pending payload is public; does not depend on unused cards in this subset | Reconstruct by replay; exact typed view comparison. |
| `queue.tokens`, `redirects` models/tokens/order | Selected cards and response order were public when played; physical IDs are implementation details | Preserve selected instances and journal commands. No resolved bodies or bounce history. |
| `queue.redirect_rng` | Private future redirect validation state | Recreate from the seeded anchor; never retain original. |
| `batch.initial_state.players[*].items` | Own inventory and opponent used/selected cards known; remaining opponent cards/count hidden | Keep used items and **all** opponent items referenced by launch/response tokens; replace every remaining item with sorted assignment and fresh tokens. No opponent unused old model/disguise survives. |
| `batch.initial_state` other player/board fields | Public status and historical bookkeeping, with D6 Fog/confusion/own-disguise exclusions | Preserve; replay to current state. Reflection-only history does not resolve hidden bodies. |
| `batch.attack`, `models`, `targets`, `cursor`, `responses`, `chance`, `random_hp_delta` | Public played cards, routing and past response order; no unused-hand dependency after rejecting Danger | Replay deterministic launch and journal; do not transplant derived state. |
| `batch` cumulative counters / `decisions` | Historical counts, not future gameplay secrets | Preserve anchor counts; replay increments. |
| `batch.rng`, `next_token` | RNG hidden; tokens physical bookkeeping | Replace RNG with seed; allocate from source current `next_token`, and put advanced allocator in new anchor. |
| `batch` special actions, guardian/tick cursors, supplied outcomes, grants/resolutions | May hold private plans/results or full old inventories | Reject as `response_special`. No external chance result is resampled; every such result-bearing boundary is excluded in this subset. |
| `counters`, `deaths` | FIFOs are private; effect provenance becomes public when activated | Require absent and active frame not counter/death; reflection-only history creates neither. Reject resolved history. |
| `gifts` | Reservation counts hidden and depend on original physical capacity | **Recompute**, including cap saturation, from rebuilt anchor and public consumption. They are a consequence of the sampled hand and known actions, not an independent hidden fact. No gifts have been drawn yet. |
| `pending_grant`, `purchase`, five `*_proof`, `last_fate/devil/item_shuffle` | Private inventories, escrow, or outcome plans may occur | Reject. |
| `last_*` notices / `last_decision` | Viewer-filtered resolved events or last accepted decision | Generate again by replay, then require exact `view(viewer)` and `last_decision(viewer)` equality. |

### Reconstruction, validation and random state

Assignment always means the **current** opponent unused real-model multiset.
It excludes consumed weapons and used miracles. Selected weapons removed since
launch are added back only in the launch anchor, with original tokens, so they
are consumed exactly once by replay. Selected miracles keep their pre-launch
used flag and disguise until replay reveals/consumes them. Existing used items
and selected items retain their relative anchor order, then fresh unused items
are appended in ascending model order as on the main path.

Both the launch and current inventories must fit `hand_limit`. In particular,
a sample that fills the current hand to18 after a consumed weapon would make
the pre-action hand19, and is `MatchError(invalid)`. This is a sample rejection,
not a reason to reject the original supported boundary. Token bounds and gift
catalog validation retain the main contract.

Build a fresh main snapshot with `snapshot_from_anchor`, replace the inventory
and **set anchor RNG to seed**, replay the launch and every recorded response,
then run `restore()`'s full validation. Supported paths cannot consume game RNG;
an explicit post-replay assertion also checks this. Consequently current RNG,
batch RNG, and active `redirect_rng` all come from seed, with no original RNG
copy remaining. Future distribution and redirects use the new stream. The
caller's external chance stream is untouched. No schema/version change is needed.

The implementation checks exact `view(viewer)`, `last_decision(viewer)`, actor
inventory (including vector order and tokens), and `legal_kinds` before returning.
All response `legal_extensions` (including canonical mode) and `legal_commands`
are functions only of that unchanged ordered actor inventory, MP/Flash,
hand limit, and the visible incoming category/element/damaging context:
see `prepare`, `response_context`, and `SelectionTree`'s response branches.
Thus the exact view and actor-inventory checks imply equality of the entire
response legality tree without enumerating it on every sample. Tests also
compare enumerations, roots/one-card prefixes, and complete response trees for
hands of at most four cards. No response legality exception was found; the
existing main94+232 exception and its three command differences remain unchanged.

Response fixtures cover weapons, retained miracles, repeated reflections,
filters, forced hits, invalid models/capacity/token budget, special/hidden/random
rejections, exact full-snapshot reconstruction modulo RNG and fresh-token/order
representation, and seeded future gifts. Random response tests use200 games
with mixed external-artifact, Fate and local-guardian settings. True-hand
whole-snapshot comparisons skip unplayed enemy disguises, since removing these
is deliberately part of the existing determinization contract.

### Response measurement

Use the same `--measure 10000` command and game/policy/chance streams as above.
The measurement RNG for uniformly sized, uniformly sampled catalog-model
multisets is778 and does not affect play. The first10,000 supported boundaries
of each type are timed, including restore and observation guards.

| Decisions by seat | Seat0 | Seat1 |
| --- | ---: | ---: |
| main starts | 118,962 | 113,932 |
| response | 36,998 | 37,865 |
| purchase | 1,844 | 1,701 |
| grant | 0 | 0 |

Default local gifts do not expose a separate grant boundary. Finished is not a
decision. Main counts and every main rejection count exactly match the P3 table.

| Response first reason | Seat0 | Seat1 |
| --- | ---: | ---: |
| ok | 26,123 (70.6065%) | 26,821 (70.8332%) |
| not_actor (response to own action) | 1,838 (4.9678%) | 1,817 (4.7986%) |
| response_special | 3,119 (8.4302%) | 3,267 (8.6280%) |
| response_random | 4,044 (10.9303%) | 4,147 (10.9521%) |
| response_history | 207 (0.5595%) | 185 (0.4886%) |
| hidden_status | 887 (2.3974%) | 827 (2.1841%) |
| hidden_own_model | 780 (2.1082%) | 801 (2.1154%) |
| other reasons, each | 0 | 0 |

The largest omitted categories are random launches (~10.94%) and special work
(~8.53%). The extra conservative history restriction omits only~0.52% of all
responses. Percentages include own-owner responses, which are explicitly
outside the requested owner/opponent scope.

Of10,000 random assignments per boundary type, main rejects0, response539
(5.39%), all due to launch physical capacity. An otherwise idle Release run measured main2.4982us and response12.0181us
per attempt (including invalid samples), about4.81x. Successful samples alone
were2.4982us and11.9150us respectively. Timing depends on machine load. Randomized correctness tests accepted
1,013 response assignments, rejected68, reconstructed1,073 truth assignments,
and continued191 response worlds to terminal/cutoff. Different seeds changed
future gifts in125/128 response fixture pairs (main remains128/128).

Parent integration caveat: its current `VecEnv::load_worlds()` renumbers only
current opponent unused tokens and `next_token`, then calls `restore()`. On
response worlds this leaves `batch.initial_state`/`batch.next_token` inconsistent,
so `gfrl_search` fails with “self-play root failed loading”. The unchanged parent
passes that C++ test against the original P3 engine. The next integration task
must update that canonicalization together with `_SUPPORT`; no parent files
are modified here.


Final validation: standalone goodfield Release build and all165 CTest tests pass.
The parent Release CMake build succeeds; CTest passes16/17, with only the
response-world token canonicalization failure above. Parent pytest passes153,
fails6, skips1: oracle-world loading, two noise-root diagnostics, and three
exact pre-task-game/report expectations. The latter expect the old main-only
search trigger sequence, which changes when response support becomes public.
Against an isolated build of the original P3 goodfield, the same parent passes
17/17 CTest and159 pytest tests (1 skipped). Parent production code, tests,
C/Python APIs and snapshot version have not been changed. Consequently the
requested parent-all-green completion criterion remains unmet until its
response integration is updated.
