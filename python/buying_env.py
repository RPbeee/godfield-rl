"""ctypes bridge for legacy-buying-subset-v32, separate from the synthetic API.

Observation buffers are reused: copy them before storing in replay memory.
DEFEND uses base=the special reaction slot (255 for ordinary), slots=preceding armor.
CAST uses main index0..14 as before and15 for retained221, slots0,target=seat.
DEFEND_CAST uses local defensive index (0=210, 1=215), prefix slots and target=255.
ATTACK_SET_* chooses a reachable final attribute with all extra boost/setter slots.
This represents equivalent attack orders; it does not preserve an XML selection order.
Removal195 is only in DEFEND/DEFEND_CAST prefix slots, before any special base.
Conditional mask components must be validated together with legal().
All six counters are DEFEND prefix slots. Action.counter_order lists selected
counter slots in FIFO order, including when a special suppresses them. Main
actions use (). The native action is 88 bytes; gx/gy/gz/gu/gv/gw remain 16 bytes and gl..gk remain 6 bytes.
CAST_ADDITIVE(kind10) uses retained local0=197/1=217.
ATTACK_ORDERED(kind11) uses a WEAPON base, zero slots and1..65 ordered extras.
Extras0..15 reference unique pre-consumption hand slots; 16/17 reference reusable
retained197/217, including acquisition earlier in the same action.
The new native action is 88 bytes. MP, acquisition and attribute order matter.
DOUBLE_ATK214 is modifier-only, using a hand ref or reusable retained ref18.
It costs6MP, doubles current damage, then combines the NE attribute.
It is never a standalone USE or CAST_ADDITIVE.
WIDE_ATK209 is also modifier-only, using a hand ref or retained ref19, for 5 MP.
It resets attribute to NE and finishes as rate100 to the enemy, even for self
selection. NE wide weapons allow 20/39/215 without 195; no hit reroll on reflection.
LastChance retains the original base ID and records wide provenance separately.
MAGIC_FREE is supported across all5 carriers. Only the first noninitial Free
waives its immediate predecessor; a Free after a special response is unconsumed.
CAST_ORDERED(kind12) uses hand0..15,held19716/21717,or old-main-held32+i.
DEFEND_ORDERED(kind13) uses a sequence of hand refs/held21020/held21521.
An action may hold65 extras; the65th uses offset17, reserved in all older APIs.
Forgetting241 is a single USE action, with no MP cost or retained copy.
It removes one copy across all22 retained kinds with count-weighted selection.
Only first196 may respond; ordinary defense/195/prefix196 are illegal.
Public results include the removed ID; population and ticket prefixes stay hidden.
EXCHANGE(kind14) consumes a hand2 base, target255, zero slots/counter fields.
Its three extras are finalHP/MP/YEN, each0..99 with the original total preserved.
It resolves immediately on self; HP0 is legal. Unused/tail reference bytes stay0.
Old kinds still use references0..21; resource values are not ordered references.
REVIVE244 is passive. A death hook scans fixedseat0 then1: with at least2
not-dead players, it consumes the first244 and adds10HP without earning a gift.
It is manually discardable but is not a normal USE or new action kind.
REMOVE_ITEMS240 is a single USE hand base with target0/1. The final target
loses one uniformly selected physical card, with no gift; empty hands draw no RNG.
Only first196 may respond; sample after its consumption. Removed ID is public,
while slot/population and future fixture tickets remain hidden.
ABSORB_HP26/42/103 are weapons and221 is MAGIC; positive residual heals
current source before target damage, capped99 and not clipped by target HP.
CAST base15 and FREE heldbase47 use retained221. Existing indices are stable.
Spell221 permits only Free carriers, not boost/setter/extra MAGIC.
Guaranteed26/42 support existing ordered components; chance103 has no extras.
DAMAGE_TO_SELF53 is a guaranteed NE weapon base. Positive residual is
applied to current source first, then target; self-target applies twice.
Death/revival waits until both applications finish. Actual clipped losses are
recorded separately; source/target/current raw survive typed reflection.
PESTLE99 uses ATTACK with no extras and target0/1, ignored on selection.
First not.dead245 holder in fixed seat order locks LIGHT999; absent mortar,
a random alive seat including self receives LIGHT30. No hit RNG is drawn.
MORTAR245 selection does not consume it. DISCARD245 is accepted but skips its
consumption, so245-only selection is a no-op turn end with no replacement gift.
Responses to mortar999 follow server acceptance; Flash UI fidelity is unverified.
SELL(kind15) uses a hand3 base and one distinct goods slot bit, target0/1,
with zero extras/counterfields. Goods may be any physical hand copy. Both cards
are consumed before response; only the trade card earns a gift. The final
recipient pays YEN->MP->HP, receives a fresh goods copy if under16, and then
current source is credited, capped99. Delivery failure does not refund payment.
Self sale follows that same order. Only first196 responds to the untyped trade.
CAST15/FREE47 for retained221 remain unchanged; SELL15 is a different kind.
BUY(kind16) uses a hand4 base and target0/1, zero other fields. First196 may
reflect. The selected physical item/price become public before phase2 PURCHASE,
where current buyer alone chooses BUY_DECLINE17 or BUY_ACCEPT18, default fields.
Self BUY only samples. Empty and insufficient funds do not offer a choice.
Price0 still requires approval. Acceptance deletes seller's selected copy,
credits seller, deals buyer a fresh copy, then charges buyer; caps99/16 apply.
No disease/gifts/turn progress occurs while waiting. Tickets and slots stay private.
Only already public queued attacks are observed, never future RNG outcomes.
Options are component masks, not complete legal-action masks. Validate composed
actions with legal(). Rewards are [fixed player 0, fixed player 1] for each row.
Light/Darkness use the new suffix features, preserving older feature layouts.
Glory and DarkCloud are public state flags; hidden outcome prefixes are never observed.
This is an old-version subset profile, not current official God Field fidelity.
"""
from __future__ import annotations

import ctypes as c
from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path
import sys


class Phase(IntEnum):
    MAIN = 0
    RESPONSE = 1
    PURCHASE = 2


class ActionKind(IntEnum):
    PRAY = 0
    ATTACK = 1
    DEFEND = 2
    USE = 3
    DISCARD = 4
    CAST = 5
    DEFEND_CAST = 6
    ATTACK_SET_FIRE = 7
    ATTACK_SET_WATER = 8
    ATTACK_SET_NEUTRAL = 9
    CAST_ADDITIVE = 10
    ATTACK_ORDERED = 11
    CAST_ORDERED = 12
    DEFEND_ORDERED = 13
    EXCHANGE = 14
    SELL = 15
    BUY = 16
    BUY_DECLINE = 17
    BUY_ACCEPT = 18


@dataclass(frozen=True)
class Action:
    kind: ActionKind
    base: int = 255
    slots: int = 0
    target: int = 255
    counter_order: tuple[int, ...] = ()
    extras: tuple[int, ...] = ()

    def __post_init__(self):
        for name, value, limit in (("kind", self.kind, 18), ("base", self.base, 255),
                                   ("slots", self.slots, 65535), ("target", self.target, 255)):
            if not isinstance(value, int) or isinstance(value, bool) or not 0 <= value <= limit:
                raise ValueError(f"{name} must be an integer in [0,{limit}]")
        order = tuple(self.counter_order)
        if len(order) > 16 or any(type(slot) is not int or not 0 <= slot < 16 for slot in order):
            raise ValueError("counter_order must contain at most 16 integer hand slots in [0,15]")
        object.__setattr__(self, "counter_order", order)
        extras = tuple(self.extras)
        limit = 99 if self.kind == ActionKind.EXCHANGE else 21
        if len(extras) > 65 or any(type(ref) is not int or not 0 <= ref <= limit for ref in extras):
            raise ValueError(f"extras must contain at most65 integers in [0,{limit}]")
        object.__setattr__(self, "extras", extras)


class _NativeAction(c.Structure):
    _fields_ = [("kind", c.c_uint8), ("base", c.c_uint8), ("slots", c.c_uint16),
                ("target", c.c_uint8), ("reserved", c.c_uint8),
                ("counter_count", c.c_uint8), ("reserved2", c.c_uint8),
                ("counter_order", c.c_uint64), ("extra_count", c.c_uint8),
                ("extra_tail", c.c_uint8), ("reserved_extra", c.c_uint8 * 6), ("extras", c.c_uint8 * 64)]


class BuyingBatchEnv:
    def __init__(self, count: int = 64, max_decisions: int = 512,
                 library: str | None = None):
        self._handle = None
        if type(count) is not int or not 1 <= count <= c.c_size_t(-1).value:
            raise ValueError("count must be a positive size_t integer")
        if type(max_decisions) is not int or not 1 <= max_decisions <= 0xFFFFFFFF:
            raise ValueError("max_decisions must be in [1,2**32-1]")
        ext = "dylib" if sys.platform == "darwin" else "so"
        path = Path(library) if library else Path(__file__).resolve().parents[1] / "build" / f"libgodfield.{ext}"
        lib = self._lib = c.CDLL(str(path))
        for name in ("g32_abi_version", "g32_observation_size", "g32_options_width", "g32_hand_capacity"):
            func = getattr(lib, name)
            func.argtypes = []
            func.restype = c.c_int
        lib.g32_action_size.argtypes = []
        lib.g32_action_size.restype = c.c_size_t
        lib.g32_ruleset.argtypes = []
        lib.g32_ruleset.restype = c.c_char_p
        if (lib.g32_abi_version() != 32 or lib.g32_action_size() != c.sizeof(_NativeAction)
                or lib.g32_observation_size() != 1251 or lib.g32_options_width() != 34
                or lib.g32_hand_capacity() != 16):
            raise RuntimeError("unsupported legacy native ABI or dimensions")
        lib.g32_create.argtypes = [c.c_size_t, c.c_uint32]
        lib.g32_create.restype = c.c_void_p
        lib.g32_destroy.argtypes = [c.c_void_p]
        lib.g32_destroy.restype = None
        lib.g32_reset_at.argtypes = [c.c_void_p, c.c_size_t, c.c_uint64]
        lib.g32_reset_at.restype = c.c_int
        lib.g32_script_hit_rolls.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_uint8), c.c_size_t]
        lib.g32_script_hit_rolls.restype = c.c_int
        lib.g32_script_bounce_targets.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_uint8), c.c_size_t]
        lib.g32_script_bounce_targets.restype = c.c_int
        lib.g32_script_disease_rolls.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_uint8), c.c_size_t]
        lib.g32_script_disease_rolls.restype = c.c_int
        lib.g32_script_forget_tickets.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_uint64), c.c_size_t]
        lib.g32_script_forget_tickets.restype = c.c_int
        lib.g32_script_item_removal_tickets.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_uint64), c.c_size_t]
        lib.g32_script_item_removal_tickets.restype = c.c_int
        lib.g32_script_buy_tickets.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_uint64), c.c_size_t]
        lib.g32_script_buy_tickets.restype = c.c_int
        lib.g32_script_pestle_targets.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_uint8), c.c_size_t]
        lib.g32_script_pestle_targets.restype = c.c_int
        lib.g32_observe.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_float),
                                  c.POINTER(c.c_uint16), c.POINTER(c.c_uint8),
                                  c.POINTER(c.c_uint8), c.POINTER(c.c_uint8)]
        lib.g32_observe.restype = c.c_int
        lib.g32_legal.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(_NativeAction), c.POINTER(c.c_uint8)]
        lib.g32_legal.restype = c.c_int
        lib.g32_sample_actions.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(c.c_uint64), c.POINTER(_NativeAction)]
        lib.g32_sample_actions.restype = c.c_int
        lib.g32_step.argtypes = [c.c_void_p, c.c_size_t, c.POINTER(_NativeAction),
                               c.POINTER(c.c_float), c.POINTER(c.c_uint8), c.POINTER(c.c_uint8)]
        lib.g32_step.restype = c.c_int
        self.count = count
        self.observation_size = lib.g32_observation_size()
        self.options_width = lib.g32_options_width()
        self.hand_capacity = lib.g32_hand_capacity()
        self.ruleset = lib.g32_ruleset().decode("ascii")
        self._obs = (c.c_float * (count * self.observation_size))()
        self._options = (c.c_uint16 * (count * self.options_width))()
        self._actors = (c.c_uint8 * count)()
        self._phases = (c.c_uint8 * count)()
        self._pray = (c.c_uint8 * count)()
        self._actions = (_NativeAction * count)()
        self._legal = (c.c_uint8 * count)()
        self._rewards = (c.c_float * (count * 2))()
        self._terminated = (c.c_uint8 * count)()
        self._truncated = (c.c_uint8 * count)()
        self._policy_seeds = (c.c_uint64 * count)(*range(count))
        self._handle = lib.g32_create(count, max_decisions)
        if not self._handle:
            raise MemoryError("native batch allocation failed")

    @staticmethod
    def _check(status):
        if status:
            raise ValueError(f"native legacy environment error {status}")

    def _open(self):
        if not self._handle:
            raise RuntimeError("environment is closed")

    def reset_at(self, index: int, seed: int):
        self._open()
        if type(index) is not int or not 0 <= index < self.count:
            raise ValueError("index out of range")
        if type(seed) is not int or not 0 <= seed <= 0xFFFFFFFFFFFFFFFF:
            raise ValueError("seed must be uint64")
        self._check(self._lib.g32_reset_at(self._handle, index, seed))
        self._policy_seeds[index] = seed ^ 0xA0761D6478BD642F

    def reset(self, seed: int = 0):
        self._open()
        if type(seed) is not int or not 0 <= seed <= 0xFFFFFFFFFFFFFFFF - self.count + 1:
            raise ValueError("seed range overflow")
        for i in range(self.count):
            self.reset_at(i, seed + i)
        return self.observe()

    def script_hit_rolls(self, index: int, rolls):
        """Fixture-only prefix of at most 64 integer outcomes in [1,100].

        Empty input clears the prefix. This controls future draws, so never give
        the prefix to a learning policy or treat it as observed game information.
        """
        self._open()
        if type(index) is not int or not 0 <= index < self.count:
            raise ValueError("index out of range")
        values = tuple(rolls)
        if len(values) > 64:
            raise ValueError("at most 64 scripted hit outcomes are supported")
        if any(type(value) is not int or not 1 <= value <= 100 for value in values):
            raise ValueError("hit outcomes must be integers in [1,100]")
        native = (c.c_uint8 * len(values))(*values)
        self._check(self._lib.g32_script_hit_rolls(self._handle, index, native, len(values)))

    def script_forget_tickets(self, index: int, tickets):
        """Set up to64 hidden uint64 tickets; each is reduced modulo population.

        Empty input clears the prefix. A zero population consumes no ticket.
        This fixture-only information must never enter a learning observation.
        """
        self._open()
        if type(index) is not int or not 0 <= index < self.count:
            raise ValueError("index out of range")
        values = tuple(tickets)
        if len(values) > 64 or any(type(value) is not int or not 0 <= value <= 0xFFFFFFFFFFFFFFFF for value in values):
            raise ValueError("tickets must contain at most64 uint64 integers")
        native = (c.c_uint64 * len(values))(*values)
        self._check(self._lib.g32_script_forget_tickets(self._handle, index, native, len(values)))

    def script_item_removal_tickets(self, index: int, tickets):
        """Set up to64 hidden uint64 tickets, reduced modulo final hand count.

        Empty input clears the prefix; empty hands consume no ticket. Do not
        expose fixture tickets to a learning policy.
        """
        self._open()
        if type(index) is not int or not 0 <= index < self.count:
            raise ValueError("index out of range")
        values = tuple(tickets)
        if len(values) > 64 or any(type(value) is not int or not 0 <= value <= 0xFFFFFFFFFFFFFFFF for value in values):
            raise ValueError("tickets must contain at most64 uint64 integers")
        native = (c.c_uint64 * len(values))(*values)
        self._check(self._lib.g32_script_item_removal_tickets(self._handle, index, native, len(values)))

    def script_buy_tickets(self, index: int, tickets):
        """Set up to64 hidden uint64 tickets, reduced modulo final hand count.

        Empty input clears the prefix; empty hands consume no ticket. Do not
        expose fixture tickets to a learning policy.
        """
        self._open()
        if type(index) is not int or not 0 <= index < self.count:
            raise ValueError("index out of range")
        values = tuple(tickets)
        if len(values) > 64 or any(type(value) is not int or not 0 <= value <= 0xFFFFFFFFFFFFFFFF for value in values):
            raise ValueError("tickets must contain at most64 uint64 integers")
        native = (c.c_uint64 * len(values))(*values)
        self._check(self._lib.g32_script_buy_tickets(self._handle, index, native, len(values)))

    def script_pestle_targets(self, index: int, targets):
        """Set up to64 hidden no-mortar target seats0/1; locks consume none.

        Empty input clears the prefix. Never expose future targets to a policy.
        """
        self._open()
        if type(index) is not int or not 0 <= index < self.count:
            raise ValueError("index out of range")
        values = tuple(targets)
        if len(values) > 64 or any(type(value) is not int or value not in (0, 1) for value in values):
            raise ValueError("targets must contain at most64 integer seats0/1")
        native = (c.c_uint8 * len(values))(*values)
        self._check(self._lib.g32_script_pestle_targets(self._handle, index, native, len(values)))

    def observe(self):
        """(features[n,1251], options[n,34], actor[n], phase[n], pray[n]).

        All are flat reusable ctypes buffers. phase: 0 main, 1 response, 2 purchase approval.
        Mask words0..31 preserve SELLING. Word32 lists buy4 bases and word33
        purchase choices (NO bit0, YES bit1); full action constraints still require legal().
        """
        self._open()
        self._check(self._lib.g32_observe(self._handle, self.count, self._obs,
                                        self._options, self._actors, self._phases, self._pray))
        return self._obs, self._options, self._actors, self._phases, self._pray

    def script_bounce_targets(self, index: int, targets):
        """Fixture-only prefix of at most 64 fixed seat targets (0 or 1).

        Empty input clears the prefix; future targets are hidden from the policy.
        """
        self._open()
        if type(index) is not int or not 0 <= index < self.count:
            raise ValueError("index out of range")
        values = tuple(targets)
        if len(values) > 64:
            raise ValueError("at most 64 scripted bounce targets are supported")
        if any(type(value) is not int or value not in (0, 1) for value in values):
            raise ValueError("bounce targets must be integer seats 0 or 1")
        native = (c.c_uint8 * len(values))(*values)
        self._check(self._lib.g32_script_bounce_targets(self._handle, index, native, len(values)))

    def script_disease_rolls(self, index: int, rolls):
        """Fixture-only prefix of up to 64 legacy disease outcomes in [0,99].

        An empty prefix clears it. Unlike hit rolls, zero is a valid outcome.
        Future outcomes and the internal worsening counter are hidden from policy.
        """
        self._open()
        if type(index) is not int or not 0 <= index < self.count:
            raise ValueError("index out of range")
        values = tuple(rolls)
        if len(values) > 64:
            raise ValueError("at most 64 scripted disease outcomes are supported")
        if any(type(value) is not int or not 0 <= value <= 99 for value in values):
            raise ValueError("disease outcomes must be integers in [0,99]")
        native = (c.c_uint8 * len(values))(*values)
        self._check(self._lib.g32_script_disease_rolls(self._handle, index, native, len(values)))

    def _encode(self, actions):
        if len(actions) != self.count:
            raise ValueError("expected one semantic Action per environment")
        for i, action in enumerate(actions):
            if not isinstance(action, Action):
                raise TypeError("actions must contain Action objects")
            packed = sum(slot << (4 * position) for position, slot in enumerate(action.counter_order))
            self._actions[i] = _NativeAction(action.kind, action.base, action.slots, action.target,
                                             0, len(action.counter_order), 0, packed, len(action.extras),
                                             action.extras[64] if len(action.extras) == 65 else 0,
                                             (c.c_uint8 * 6)(), (c.c_uint8 * 64)(*action.extras[:64]))

    def legal(self, actions):
        """Validate composed actions; unlike options, this checks all constraints."""
        self._open()
        self._encode(actions)
        self._check(self._lib.g32_legal(self._handle, self.count, self._actions, self._legal))
        return tuple(bool(x) for x in self._legal)

    def sample_actions(self):
        """Hierarchical native random policy; advances only private policy seeds."""
        self._open()
        self._check(self._lib.g32_sample_actions(self._handle, self.count,
                                               self._policy_seeds, self._actions))
        return [Action(ActionKind(a.kind), a.base, a.slots, a.target,
                       tuple((a.counter_order >> (4 * i)) & 15 for i in range(a.counter_count)),
                       tuple(a.extras[:min(a.extra_count, 64)]) + ((a.extra_tail,) if a.extra_count == 65 else ()))
                for a in self._actions]

    def step(self, actions):
        self._open()
        self._encode(actions)
        self._check(self._lib.g32_step(self._handle, self.count, self._actions,
                                     self._rewards, self._terminated, self._truncated))
        return self._rewards, self._terminated, self._truncated

    def random_step(self):
        return self.step(self.sample_actions())

    def close(self):
        if self._handle:
            self._lib.g32_destroy(self._handle)
            self._handle = None

    def __enter__(self):
        self._open()
        return self

    def __exit__(self, *_):
        self.close()

    def __del__(self):
        self.close()


if __name__ == "__main__":
    episodes = 0
    ordered = 0
    additive_casts = 0
    doubling_uses = 0
    wide_uses = 0
    free_casts = 0
    ordered_defenses = 0
    forget_uses = 0
    exchange_uses = 0
    item_removal_uses = 0
    self_damage_uses = 0
    pestle_uses = 0
    mortar_discards = 0
    purchases = 0
    with BuyingBatchEnv(16, max_decisions=64) as env:
        env.reset(42)
        for tick in range(128):
            env.observe()
            actions = env.sample_actions()
            assert all(env.legal(actions))
            purchases += sum(action.kind == ActionKind.BUY for action in actions)
            pestle_uses += sum(action.kind == ActionKind.ATTACK and env._obs[i * env.observation_size + 1136 + action.base] > 0 for i, action in enumerate(actions) if action.base < 16)
            mortar_discards += sum(action.kind == ActionKind.DISCARD and any(action.slots & (1 << slot) and env._obs[i * env.observation_size + 1152 + slot] > 0 for slot in range(16)) for i, action in enumerate(actions))
            self_damage_uses += sum(action.kind in (ActionKind.ATTACK, ActionKind.ATTACK_SET_FIRE, ActionKind.ATTACK_SET_WATER, ActionKind.ATTACK_SET_NEUTRAL, ActionKind.ATTACK_ORDERED) and env._obs[i * env.observation_size + 1111 + action.base] > 0 for i, action in enumerate(actions) if action.base < 16)
            item_removal_uses += sum(action.kind == ActionKind.USE and env._obs[i * env.observation_size + 1063 + action.base] > 0 for i, action in enumerate(actions) if action.base < 16)
            exchange_uses += sum(action.kind == ActionKind.EXCHANGE for action in actions)
            forget_uses += sum(action.kind == ActionKind.USE and env._obs[i * env.observation_size + 994 + action.base] > 0 for i, action in enumerate(actions) if action.base < 16)
            ordered += sum(action.kind == ActionKind.ATTACK_ORDERED for action in actions)
            additive_casts += sum(action.kind == ActionKind.CAST_ADDITIVE for action in actions)
            free_casts += sum(action.kind == ActionKind.CAST_ORDERED for action in actions)
            ordered_defenses += sum(action.kind == ActionKind.DEFEND_ORDERED for action in actions)
            doubling_uses += sum(ref == 18 or (ref < 16 and env._obs[i * env.observation_size + 904 + ref] > 0)
                                 for i, action in enumerate(actions) if action.kind == ActionKind.ATTACK_ORDERED for ref in action.extras)
            wide_uses += sum(ref == 19 or (ref < 16 and env._obs[i * env.observation_size + 925 + ref] > 0)
                             for i, action in enumerate(actions) if action.kind == ActionKind.ATTACK_ORDERED for ref in action.extras)
            _, terminal, truncated = env.step(actions)
            for i in range(env.count):
                if terminal[i] or truncated[i]:
                    episodes += 1
                    env.reset_at(i, 100000 + tick * env.count + i)
    print(f"Buying native batch smoke passed: 2048 decisions, {episodes} episodes, "
          f"{ordered} ordered attacks, {additive_casts} additive recasts, "
          f"{doubling_uses} doubling uses, {wide_uses} wide uses, "
          f"{free_casts} free casts, {ordered_defenses} ordered defenses, {forget_uses} forget uses, {exchange_uses} exchanges, {item_removal_uses} item removals, {self_damage_uses} self-damage attacks, {pestle_uses} pestle attacks, {mortar_discards} mortar discards, {purchases} buys")
