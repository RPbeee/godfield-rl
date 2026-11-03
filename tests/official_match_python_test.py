import copy
import json
import shutil
import tempfile
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from official_data import OfficialData
from official_state import OfficialState
from official_match import Match, Config, PlayerSetup, Command, FateRequest, RolloutResult, RolloutError, GuardianAction, GuardianPayloadAction, PhenomenonAction, ItemSource, ItemShuffleAction, FateAction, DevilAction, GrantAction, MatchError, InvalidMatchError, UnsupportedMatchError
LIBRARY = sys.argv.pop(1) if len(sys.argv) > 1 else None

def choice_envelope_reference(seed, bound):
    mask, gamma = 2**64-1, 0x9e3779b97f4a7c15
    while True:
        seed = (seed+gamma)&mask; value = seed
        value = ((value^(value>>30))*0xbf58476d1ce4e5b9)&mask
        value = ((value^(value>>27))*0x94d049bb133111eb)&mask
        value ^= value>>31
        if value >= 2**64 % bound: return value % bound, seed

def grant_reference(seed, tokens, target=0):
    """Independent integer reference: SplitMix64 rejection, then ticket/FY law."""
    mask, gamma, draws = (1 << 64)-1, 0x9e3779b97f4a7c15, 0
    def bounded(bound):
        nonlocal seed, draws
        while True:
            seed = (seed + gamma) & mask; draws += 1
            value = seed
            value = ((value ^ (value >> 30)) * 0xbf58476d1ce4e5b9) & mask
            value = ((value ^ (value >> 27)) * 0x94d049bb133111eb) & mask
            value ^= value >> 31
            if value >= (1 << 64) % bound: return value % bound
    ticket = bounded(100)
    model = 240 if ticket < 7 else 241 if ticket < 12 else 242 if ticket < 15 else 243 if ticket < 20 else 244 if ticket < 25 else 0
    removed, resource = [], None
    if model == 243:
        pool = list(tokens)
        for index in range(min(2, len(pool))):
            other = index + bounded(len(pool)-index)
            pool[index], pool[other] = pool[other], pool[index]; removed.append(pool[index])
    if model == 244: resource = ("hp", "mp", "cp")[bounded(3)]
    return GrantAction(target, model, resource, tuple(removed)), seed, ticket, draws

def fate_reference(seed, token, raw, actor=0):
    mask, gamma, draws = (1 << 64)-1, 0x9e3779b97f4a7c15, 0
    def bounded(bound):
        nonlocal seed, draws
        while True:
            seed = (seed + gamma) & mask; draws += 1
            value = seed
            value = ((value ^ (value >> 30)) * 0xbf58476d1ce4e5b9) & mask
            value = ((value ^ (value >> 27)) * 0x94d049bb133111eb) & mask
            value ^= value >> 31
            if value >= (1 << 64) % bound: return value % bound
    model = 287 + bounded(10)
    alive = [player for player in raw["players"] if not player.get("dead", False)]
    recipient, guardians, shuffle = None, (), None
    if 291 <= model <= 294: recipient = alive[bounded(len(alive))]["id"]
    if model == 296:
        names = ("mars", "mercury", "jupiter", "saturn", "uranus", "pluto", "neptune", "venus", "earth", "moon")
        guardians = tuple(names[bounded(10)] for unused in alive)
    if model == 295:
        order = [ItemSource(player["id"], item["id"]) for player in alive for item in player.get("items", [])
                 if (player["id"], item["id"]) != (actor, token)]
        for length in range(len(order), 1, -1):
            other = bounded(length); order[length-1], order[other] = order[other], order[length-1]
        shuffle = ItemShuffleAction(tuple(order))
    return FateAction(actor, token, PhenomenonAction(model, recipient, guardians), shuffle), seed, draws

def guardian_payload_reference(seed, source_id, raw, cards, bases):
    mask, gamma, draws = (1 << 64)-1, 0x9e3779b97f4a7c15, 0
    def bounded(bound):
        nonlocal seed, draws
        while True:
            seed = (seed + gamma) & mask; draws += 1
            value = seed
            value = ((value ^ (value >> 30)) * 0xbf58476d1ce4e5b9) & mask
            value = ((value ^ (value >> 27)) * 0x94d049bb133111eb) & mask
            value ^= value >> 31
            if value >= (1 << 64) % bound: return value % bound
    source = next(player for player in raw["players"] if player["id"] == source_id)
    guardian = source["guardian"]; models = [bases[guardian][bounded(len(bases[guardian]))]]
    base = cards[models[0]]
    # Under the strict current domain, only an ordinary nonchance Earth weapon
    # can append; its fixed candidate set contains ordinary plus-ATK cards.
    extras = [model for model, card in sorted(cards.items()) if card.get("giftRate", 0) > 0 and model != 207
              and card["category"] in ("trade", "weapons", "armor", "sundries") and card.get("isPlusAtk", False)
              and card.get("ability", "none") not in ("exchange", "sell", "buy", "danger")]
    if guardian == "earth" and base["category"] == "weapons" and not base.get("hitRate", 0):
        while len(models) < 18 and bounded(2): models.append(extras[bounded(len(extras))])
    default_enemy = not base.get("hitRate", 0) and (base.get("atk", 0) > 0 or base["category"] == "weapons"
        or base.get("ability", "none") in ("sell", "buy", "boostCPToEnemy", "takeCP", "addCurse", "removeItems", "removeUsedMiracles"))
    hostile = sum(cards[model].get("atk", 0) for model in models) > 0 or default_enemy
    def enemy(player): return (source.get("team", 0) == 0 and player["id"] != source_id) or source.get("team", 0) != player.get("team", 0)
    targets = [player["id"] for player in raw["players"] if not player.get("dead", False) and enemy(player) == hostile]
    target = targets[bounded(len(targets))]
    return GuardianPayloadAction(source_id, guardian, tuple(models), target), seed, draws

class MatchTests(unittest.TestCase):
    def match(self, **kwargs):
        value = Match(library=LIBRARY, **kwargs); self.addCleanup(value.close); return value

    def token(self, game, seat, model):
        return next(i["token"] for i in game.observe(seat)["board"]["hand"] if i["face"] == model)

    def test_descriptor_and_public_observation(self):
        match = self.match(players=[PlayerSetup(0, "A\0B"), PlayerSetup(77, "相手")])
        self.assertEqual(match.ruleset, "official-execution-subset-v47"); self.assertIn("236-default-or237-opt-in", match.scope); self.assertIn("not-official-server-equivalence", match.scope)
        self.assertEqual((len(match.supported_models), match.gift_weight), (236, 499))
        report = json.loads((Path(__file__).resolve().parents[1] / "data/official-v2/dream-compatibility-v32.json").read_text())
        self.assertEqual(tuple(report["supported_ids"]), match.supported_models)
        self.assertEqual((report["pool_models"], report["pool_gift_weight"]), (236, 499))
        self.assertIn(212, match.supported_models); self.assertIn(112, match.supported_models); self.assertIn(222, match.supported_models)
        self.assertEqual(match.status.player_ids, (0, 77)); self.assertEqual(match.actor, 0)
        own = match.observe(0); spectator = match.observe()
        self.assertEqual(own["schema"], "official-execution-view-v45")
        facts = OfficialData(LIBRARY)
        self.assertEqual(own["catalog_version"], "official-v2")
        self.assertEqual(own["catalog_sha256"], facts.catalog_sha256)
        self.assertEqual(own["client_sha256"], facts.client_sha256)
        self.assertEqual(own["board"]["players"][0]["name"], "A\0B")
        self.assertEqual(len(own["board"]["hand"]), 9); self.assertEqual(spectator["board"]["hand"], [])
        self.assertIsNone(own["pending"])
        for key in ("rng", "gifts", "next_token", "queue"):
            self.assertNotIn(key, own); self.assertNotIn(key, own["board"])
        self.assertNotIn("items", own["board"]["players"][1])

    def test_reaction_pool_reset_and_reflection_continuation(self):
        added = {38, 39, 156, 167, 174, 189, 190}
        match = self.match(config=Config(initial_hand=18, initial_hp=99))
        self.assertTrue(added <= set(match.supported_models))
        # Dream weapons, counter and retained miracle belong to this explicit subset.
        self.assertTrue({36, 80, 183, 222} <= set(match.supported_models))
        seen = set(); fixture = None
        for seed in range(4096):
            match.reset(seed)
            own = match.observe(0)["board"]["hand"]
            other = match.observe(1)["board"]["hand"]
            seen.update(item["face"] for item in own + other)
            self.assertTrue(seen <= set(match.supported_models))
            weapon = next((item for item in own if item["face"] == 6), None)
            mirror = next((item for item in other if item["face"] == 38), None)
            if fixture is None and weapon is not None and mirror is not None:
                fixture = (seed, weapon["token"], mirror["token"])
            if added <= seen and fixture is not None: break
        self.assertTrue(added <= seen, "all seven added models must actually be gifted")
        self.assertIsNotNone(fixture, "seeded reset must expose a public reflection fixture")
        seed, weapon, mirror = fixture
        match.reset(seed)
        first = match.step(Command(0, "play", (weapon,), 1))
        self.assertEqual((first.phase, first.actor, first.owner), ("response", 1, 0))
        pending = match.observe()["pending"]
        before_mp = [p["status"]["mp"] for p in match.observe()["board"]["players"]]
        reflected = match.step(Command(1, "defend", (mirror,)))
        self.assertEqual((reflected.phase, reflected.actor, reflected.owner), ("response", 0, 0))
        after = match.observe()["pending"]
        self.assertEqual((after["origin"], after["source"], after["target"]), (0, 1, 0))
        self.assertEqual((after["power"], after["models"]), (pending["power"], pending["models"]))
        self.assertEqual([p["status"]["mp"] for p in match.observe()["board"]["players"]], before_mp)
        own_before = match.observe(0); status = match.status
        with self.assertRaises(InvalidMatchError): match.step(Command(1, "defend"))
        self.assertEqual((match.observe(0), match.status), (own_before, status))
        resolved = match.step(Command(0, "defend"))
        self.assertEqual((resolved.phase, resolved.actor, resolved.owner), ("main", 1, 1))
        self.assertIsNone(match.observe()["pending"])
        players = match.observe()["board"]["players"]
        self.assertEqual([p["status"]["hp"] for p in players], [98, 99])

    def test_block_pool_and_retained_wall_response(self):
        added = {42, 50, 55, 56, 165, 171, 177, 179, 233}
        match = self.match(config=Config(initial_hand=18, initial_hp=99))
        self.assertTrue(added <= set(match.supported_models))
        seen = set(); fixture = None
        for seed in range(4096):
            match.reset(seed)
            own = match.observe(0)["board"]["hand"]
            other = match.observe(1)["board"]["hand"]
            seen.update(item["face"] for item in own + other)
            self.assertTrue(seen <= set(match.supported_models))
            weapon = next((item for item in own if item["face"] == 6), None)
            wall = next((item for item in other if item["face"] == 233), None)
            if fixture is None and weapon is not None and wall is not None:
                fixture = (seed, weapon["token"], wall["token"])
            if added <= seen and fixture is not None: break
        self.assertTrue(added <= seen, "all nine block models must actually be gifted")
        self.assertIsNotNone(fixture)
        seed, weapon, wall = fixture
        match.reset(seed)
        match.step(Command(0, "play", (weapon,), 1))
        blocked = match.step(Command(1, "defend", (wall,)))
        self.assertEqual((blocked.phase, blocked.actor, blocked.owner), ("main", 1, 1))
        self.assertIsNone(match.observe()["pending"])
        players = match.observe()["board"]["players"]
        self.assertEqual([p["status"]["hp"] for p in players], [99, 99])
        self.assertEqual([p["status"]["mp"] for p in players], [10, 4])
        wall_item = next(item for item in match.observe(1)["board"]["hand"] if item["token"] == wall)
        self.assertEqual((wall_item["face"], wall_item["used"]), (233, True))
        # The retained response-only miracle must not become a main heal/attack.
        command = Command(1, "play", (wall,), 0)
        before = match.observe(1); status = match.status
        self.assertFalse(match.legal(command))
        with self.assertRaises(InvalidMatchError): match.step(command)
        self.assertEqual((match.observe(1), match.status), (before, status))

    def test_composition_pool_zero_mp_free_and_plain_weapon_role(self):
        added = {46, 66, 83, 143, 163, 173, 204, 231}
        config = Config(initial_hand=18, initial_mp=0)
        match = self.match(config=config)
        self.assertTrue(added <= set(match.supported_models))
        seen = set(); main_fixture = None; wall_fixture = None; roles = {}
        for seed in range(16384):
            match.reset(seed)
            own = match.observe(0)["board"]["hand"]
            other = match.observe(1)["board"]["hand"]
            seen.update(item["face"] for item in own + other)
            self.assertTrue(seen <= set(match.supported_models))
            by_model = {item["face"]: item["token"] for item in own}
            enemy = {item["face"]: item["token"] for item in other}
            if main_fixture is None and {235, 204} <= by_model.keys():
                main_fixture = (seed, by_model[235], by_model[204])
            if wall_fixture is None and 6 in by_model and {233, 204} <= enemy.keys():
                wall_fixture = (seed, by_model[6], enemy[233], enemy[204])
            for id in (46, 204, 231):
                if id in by_model and id not in roles: roles[id] = (seed, by_model[id])
            if added <= seen and main_fixture is not None and wall_fixture is not None and len(roles) == 3: break
        self.assertTrue(added <= seen, "all eight composition models must actually be gifted")
        self.assertIsNotNone(main_fixture); self.assertIsNotNone(wall_fixture); self.assertEqual(len(roles), 3)
        for id in (204, 231):
            seed, token = roles[id]; match.reset(seed)
            command = Command(0, "play", (token,), 0)
            self.assertFalse(match.legal(command))
            before = match.observe(0)
            with self.assertRaises(InvalidMatchError): match.step(command)
            self.assertEqual(match.observe(0), before)
        seed, token = roles[46]; match.reset(seed)
        match.step(Command(0, "play", (token,), 1))
        self.assertEqual((match.observe()["pending"]["power"], match.observe()["pending"]["models"]), (12, [46]))
        seed, spring, free = main_fixture; match.reset(seed)
        reference = self.match(config=config, seed=seed)
        command = Command(0, "play", (spring, free), 0)
        self.assertTrue(match.legal(command))
        before = match.observe(0); before_status = match.status; sampled_pair = False
        for policy_seed in range(4096):
            sampled, rng = match.sample_action(policy_seed)
            self.assertTrue(match.legal(sampled))
            self.assertEqual((sampled, rng), match.sample_action(policy_seed))
            if sampled.kind == "play" and sampled.items == (spring, free):
                sampled_pair = True; break
        self.assertTrue(sampled_pair, "zero-MP sampler must reach miracle+FREE pair")
        self.assertEqual((match.observe(0), match.status), (before, before_status))
        self.assertEqual(match.step(command), reference.step(command))
        # Includes the deferred replacement draw, proving sampling left game RNG untouched.
        self.assertEqual(match.observe(0), reference.observe(0))
        hand = match.observe(0)["board"]["hand"]
        self.assertEqual(len(hand), 18)
        self.assertNotIn(free, {item["token"] for item in hand})
        self.assertTrue(next(item for item in hand if item["token"] == spring)["used"])
        self.assertEqual(match.observe()["board"]["players"][0]["status"]["hp"], 50)
        self.assertEqual(match.observe()["board"]["players"][0]["status"]["mp"], 0)
        seed, weapon, wall, free = wall_fixture; match.reset(seed)
        match.step(Command(0, "play", (weapon,), 1))
        defense = Command(1, "defend", (wall, free))
        self.assertTrue(match.legal(defense)); sampled_pair = False
        for policy_seed in range(4096):
            sampled, unused = match.sample_action(policy_seed)
            self.assertTrue(match.legal(sampled))
            if sampled == defense: sampled_pair = True; break
        self.assertTrue(sampled_pair, "zero-MP sampler must reach retained-wall+FREE response")
        result = match.step(defense)
        self.assertEqual((result.phase, result.actor, result.owner), ("main", 1, 1))
        hand = match.observe(1)["board"]["hand"]
        self.assertEqual(len(hand), 18); self.assertNotIn(free, {item["token"] for item in hand})
        self.assertTrue(next(item for item in hand if item["token"] == wall)["used"])
        players = match.observe()["board"]["players"]
        self.assertEqual([p["status"]["hp"] for p in players], [40, 40])
        self.assertEqual([p["status"]["mp"] for p in players], [0, 0])

    def test_bounce_pool_zero_mp_free_and_environment_rng(self):
        added = {21, 34, 114, 125, 137, 151, 164, 234}
        config = Config(initial_hand=18, initial_mp=0)
        match = self.match(config=config)
        self.assertTrue(added <= set(match.supported_models))
        seen = set()
        for seed in range(4096):
            match.reset(seed)
            seen.update(item["face"] for player in (0, 1) for item in match.observe(player)["board"]["hand"])
            self.assertTrue(seen <= set(match.supported_models))
            if added <= seen: break
        self.assertTrue(added <= seen, "all eight bounce models must actually be gifted")
        # Fixed public reset fixture: both sides can play a miracle+FREE at MP0.
        # Tokens are obtained only from that player's public observation.
        match.reset(511832); reference = self.match(config=config, seed=511832)
        own = {item["face"]: item["token"] for item in match.observe(0)["board"]["hand"]}
        other = {item["face"]: item["token"] for item in match.observe(1)["board"]["hand"]}
        self.assertTrue({235, 204} <= own.keys()); self.assertTrue({234, 173} <= other.keys())
        cast = Command(0, "play", (own[235], own[204]), 1)
        self.assertEqual(match.step(cast), reference.step(cast))
        bounce = Command(1, "defend", (other[234], other[173]))
        self.assertTrue(match.legal(bounce)); self.assertFalse(match.legal(Command(1, "defend", (other[234],))))
        before = match.observe(1); status = match.status
        with self.assertRaises(InvalidMatchError): match.step(Command(1, "defend", (other[234],)))
        sampled_pair = False
        for policy_seed in range(4096):
            sampled, rng = match.sample_action(policy_seed)
            self.assertTrue(match.legal(sampled))
            self.assertEqual((sampled, rng), match.sample_action(policy_seed))
            if sampled == bounce: sampled_pair = True; break
        self.assertTrue(sampled_pair, "zero-MP sampler must reach Turbulence+FREE")
        self.assertEqual((match.observe(1), match.status), (before, status))
        self.assertEqual(match.step(bounce), reference.step(bounce))
        event = match.observe(1); pending = event["pending"]
        self.assertEqual(event["schema"], "official-execution-view-v45")
        self.assertEqual(event["bounce_target_rule"], "experimental:uniform-all-alive-in-input-order;includes-self-and-teammates")
        self.assertEqual((event["phase"], event["owner"]), ("response", 0))
        self.assertIn(event["actor"], (0, 1))
        self.assertEqual((pending["origin"], pending["source"], pending["target"]), (0, 1, event["actor"]))
        self.assertEqual((pending["bounces"], pending["reflections"]), (1, 0))
        self.assertEqual((pending["models"], pending["amount"]), ([235, 204], 10))
        for key in ("redirect_rng", "redirects", "tokens", "rng", "gifts"):
            self.assertNotIn(key, event); self.assertNotIn(key, pending)
        hand = event["board"]["hand"]
        self.assertEqual(len(hand), 17)  # Replacement remains deferred during response.
        self.assertNotIn(other[173], {item["token"] for item in hand})
        self.assertTrue(next(item for item in hand if item["token"] == other[234])["used"])
        self.assertEqual([p["status"]["mp"] for p in event["board"]["players"]], [0, 0])
        self.assertEqual([p["status"]["hp"] for p in event["board"]["players"]], [40, 40])
        # Self-bounce also waits for this explicit response, then resolves once.
        target = event["actor"]; accept = Command(target, "defend")
        self.assertEqual(match.step(accept), reference.step(accept))
        self.assertEqual((match.status.phase, match.actor, match.status.owner), ("main", 1, 1))
        for player in (0, 1):
            self.assertEqual(match.observe(player), reference.observe(player))
            self.assertEqual(len(match.observe(player)["board"]["hand"]), 18)
        hp = [p["status"]["hp"] for p in match.observe()["board"]["players"]]
        self.assertEqual(hp, [50 if player == target else 40 for player in (0, 1)])

    def test_chance_pool_targetless_batch_roles_and_free_redirect(self):
        added = {95, 96, *range(99, 112), *range(225, 231)}
        config = Config(initial_hand=18, initial_mp=0)
        match = self.match(config=config)
        self.assertTrue(added <= set(match.supported_models))
        seen = set(); defense_fixture = None
        for seed in range(4096):
            match.reset(seed)
            own = {item["face"]: item["token"] for item in match.observe(0)["board"]["hand"]}
            other = {item["face"]: item["token"] for item in match.observe(1)["board"]["hand"]}
            seen.update(own); seen.update(other)
            self.assertTrue(seen <= set(match.supported_models))
            if defense_fixture is None and 6 in own and 108 in other:
                defense_fixture = (seed, own[6], other[108])
            if added <= seen and defense_fixture is not None: break
        self.assertTrue(added <= seen, "all twenty-one pure chance models are gifted")
        self.assertIsNotNone(defense_fixture)
        seed, weapon, defense = defense_fixture; match.reset(seed)
        match.step(Command(0, "play", (weapon,), 1))
        self.assertEqual(match.observe()["pending"]["hit_rate"], 0)
        self.assertTrue(match.legal(Command(1, "defend", (defense,))))
        match.step(Command(1, "defend", (defense,)))
        self.assertEqual([p["status"]["hp"] for p in match.observe()["board"]["players"]], [40, 40])
        # Chance targets only enemies: teammate77 and source0 never enter this batch.
        roster = [PlayerSetup(0, team=1), PlayerSetup(77, team=1), PlayerSetup(88), PlayerSetup(99, team=2)]
        multi = self.match(config=config, players=roster, seed=716)
        reference = self.match(config=config, players=roster, seed=716)
        own = multi.observe(0)["board"]["hand"]
        chance = next(item["token"] for item in own if item["face"] == 95)
        command = Command(0, "play", (chance,))
        before = multi.observe(0); status = multi.status
        for invalid in (Command(0, "play", (chance,), 99), Command(0, "play", (chance, next(item["token"] for item in own if item["token"] != chance)))):
            self.assertFalse(multi.legal(invalid))
            with self.assertRaises(InvalidMatchError): multi.step(invalid)
            self.assertEqual((multi.observe(0), multi.status), (before, status))
        reached = False
        for policy_seed in range(4096):
            proposal, rng = multi.sample_action(policy_seed)
            self.assertTrue(multi.legal(proposal)); self.assertEqual((proposal, rng), multi.sample_action(policy_seed))
            if proposal == command: reached = True; break
        self.assertTrue(reached, "sampler emits targetless chance commands")
        self.assertEqual(multi.step(command), reference.step(command))
        event = multi.observe(); pending = event["pending"]
        self.assertEqual(event["chance_rule"], "experimental:targetless-all-alive-enemies-in-input-order;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll;skip-dead;cancel-on-origin-death")
        self.assertEqual((multi.status.phase, multi.actor, multi.status.owner), ("response", 99, 0))
        self.assertEqual((pending["hit_rate"], pending["source"], pending["target"]), (75, 0, 99))
        for key in ("chance", "chance_rolls", "chance_hits", "targets", "cursor", "initial_state", "responses", "rng"):
            self.assertNotIn(key, event); self.assertNotIn(key, pending)
        self.assertEqual(len(multi.observe(0)["board"]["hand"]), 17)
        accept = Command(99, "defend")
        self.assertEqual(multi.step(accept), reference.step(accept))
        self.assertEqual(multi.observe(0), reference.observe(0))
        self.assertEqual((multi.actor, multi.status.owner), (77, 77))
        self.assertEqual([p["status"]["hp"] for p in multi.observe()["board"]["players"]], [40, 40, 40, 39])
        self.assertEqual(len(multi.observe(0)["board"]["hand"]), 18)
        # An all-miss attempt consumes one decision and gives no response window.
        match.reset(703)
        chance = next(item["token"] for item in match.observe(0)["board"]["hand"] if item["face"] == 95)
        result = match.step(Command(0, "play", (chance,)))
        self.assertEqual((result.decisions, result.phase, result.actor, result.owner), (1, "main", 1, 1))
        self.assertIsNone(match.observe()["pending"])
        self.assertEqual([p["status"]["hp"] for p in match.observe()["board"]["players"]], [40, 40])
        # A chance miracle accepts FREE but a hit remains hit across reflection.
        match.reset(4302); reference = self.match(config=config, seed=4302)
        own = {item["face"]: item["token"] for item in match.observe(0)["board"]["hand"]}
        other = {item["face"]: item["token"] for item in match.observe(1)["board"]["hand"]}
        self.assertTrue({225, 204} <= own.keys()); self.assertIn(39, other)
        command = Command(0, "play", (own[225], own[204]))
        reached = False
        for policy_seed in range(4096):
            proposal, unused = match.sample_action(policy_seed); self.assertTrue(match.legal(proposal))
            if proposal == command: reached = True; break
        self.assertTrue(reached, "sampler emits targetless chance miracle+FREE at MP0")
        self.assertEqual(match.step(command), reference.step(command))
        self.assertEqual((match.actor, match.observe()["pending"]["hit_rate"]), (1, 75))
        reflect = Command(1, "defend", (other[39],))
        self.assertEqual(match.step(reflect), reference.step(reflect))
        pending = match.observe()["pending"]
        self.assertEqual((match.actor, pending["source"], pending["target"], pending["hit_rate"]), (0, 1, 0, 75))
        self.assertEqual((pending["reflections"], pending["bounces"]), (1, 0))
        self.assertEqual(match.step(Command(0, "defend")), reference.step(Command(0, "defend")))
        self.assertEqual(match.observe(0), reference.observe(0))
        hand = match.observe(0)["board"]["hand"]
        self.assertEqual(len(hand), 18); self.assertNotIn(own[204], {item["token"] for item in hand})
        self.assertTrue(next(item for item in hand if item["token"] == own[225])["used"])
        self.assertEqual([p["status"]["mp"] for p in match.observe()["board"]["players"]], [0, 0])
        self.assertEqual([p["status"]["hp"] for p in match.observe()["board"]["players"]], [35, 40])

    def test_damage_counters_fifo_hp_loss_and_hidden_reservations(self):
        config = Config(initial_hand=18, initial_mp=0)
        match = self.match(config=config, seed=54105)
        reference = self.match(config=config, seed=54105)
        self.assertTrue({181, 184} <= set(match.supported_models))
        own = {item["face"]: item["token"] for item in match.observe(0)["board"]["hand"]}
        other = {item["face"]: item["token"] for item in match.observe(1)["board"]["hand"]}
        self.assertIn(32, own); self.assertTrue({118, 181, 184} <= other.keys())
        attack = Command(0, "play", (own[32],), 1)
        self.assertEqual(match.step(attack), reference.step(attack))
        before = match.observe(1); status = match.status
        for policy_seed in range(64):
            proposal, rng = match.sample_action(policy_seed)
            self.assertTrue(match.legal(proposal)); self.assertEqual((proposal, rng), match.sample_action(policy_seed))
        self.assertEqual((match.observe(1), match.status), (before, status))
        defense = Command(1, "defend", (other[118], other[181], other[184]))
        self.assertTrue(match.legal(defense))
        self.assertEqual(match.step(defense), reference.step(defense))
        event = match.observe(1); pending = event["pending"]
        self.assertEqual(event["counter_rule"], "experimental:actual-hp-loss;survivors-only;selection-order-fifo-before-next-target;skip-dead-source-or-target;single-target;darkcloud-forces-without-draw;roll-before-response;redirects-no-reroll")
        self.assertEqual((event["phase"], event["actor"], event["owner"]), ("response", 0, 0))
        self.assertTrue(pending["counter"])
        self.assertEqual((pending["origin"], pending["source"], pending["target"]), (1, 1, 0))
        self.assertEqual((pending["models"], pending["power"], pending["hit_rate"], pending["element"]), ([181], 7, 75, "fire"))
        self.assertEqual([p["status"]["hp"] for p in event["board"]["players"]], [40, 33])
        self.assertEqual(len(event["board"]["hand"]), 15)
        self.assertTrue({other[118], other[181], other[184]}.isdisjoint(item["token"] for item in event["board"]["hand"]))
        for key in ("batch", "counters", "counter_created", "counter_started", "counter_rolls", "counter_hits", "responses", "initial_state", "targets", "rng"):
            self.assertNotIn(key, event); self.assertNotIn(key, pending)
        accept = Command(0, "defend")
        self.assertEqual(match.step(accept), reference.step(accept))
        event = match.observe(); pending = event["pending"]
        self.assertEqual((match.actor, match.status.owner), (0, 0))
        self.assertTrue(pending["counter"])
        self.assertEqual((pending["models"], pending["power"], pending["hit_rate"], pending["element"]), ([184], 14, 0, "stone"))
        self.assertEqual([p["status"]["hp"] for p in event["board"]["players"]], [33, 33])
        self.assertEqual(match.step(accept), reference.step(accept))
        self.assertEqual((match.status.phase, match.actor, match.status.owner), ("main", 1, 1))
        self.assertIsNone(match.observe()["pending"])
        self.assertEqual([p["status"]["hp"] for p in match.observe()["board"]["players"]], [19, 33])
        self.assertEqual([p["status"]["mp"] for p in match.observe()["board"]["players"]], [0, 0])
        for player in (0, 1):
            self.assertEqual(match.observe(player), reference.observe(player))
            self.assertEqual(len(match.observe(player)["board"]["hand"]), 18)
        # The missed Mars counter never creates a response decision.
        match.reset(58017)
        own = {item["face"]: item["token"] for item in match.observe(0)["board"]["hand"]}
        other = {item["face"]: item["token"] for item in match.observe(1)["board"]["hand"]}
        self.assertIn(6, own); self.assertIn(181, other); self.assertIn(184, own)
        main_counter = Command(0, "play", (own[184],))
        self.assertFalse(match.legal(main_counter))
        with self.assertRaises(InvalidMatchError): match.step(main_counter)
        match.step(Command(0, "play", (own[6],), 1))
        result = match.step(Command(1, "defend", (other[181],)))
        self.assertEqual((result.decisions, result.phase, result.actor, result.owner), (2, "main", 1, 1))
        self.assertIsNone(match.observe()["pending"])
        self.assertEqual([p["status"]["hp"] for p in match.observe()["board"]["players"]], [40, 39])

    def test_resource_counters_response_direction_alias_and_death(self):
        rule = "experimental:mp-self-target-with-response;cp-target-incoming-source;debit-current-target-then-full-credit-current-source;resource-debt-no-new-counter"
        def hand(match, seat):
            return {item["face"]: item["token"] for item in match.observe(seat)["board"]["hand"]}
        def stats(match, name):
            return [p["status"][name] for p in match.observe()["board"]["players"]]
        # Neptune emits a zero-power, explicit self-target response, not a guardian.
        config = Config(initial_hand=18, initial_mp=0, initial_cp=0)
        match = self.match(config=config, seed=1004); reference = self.match(config=config, seed=1004)
        self.assertTrue({187, 188} <= set(match.supported_models))
        own, other = hand(match, 0), hand(match, 1)
        self.assertIn(6, own); self.assertIn(187, other)
        attack = Command(0, "play", (own[6],), 1); defense = Command(1, "defend", (other[187],))
        self.assertEqual(match.step(attack), reference.step(attack))
        self.assertEqual(match.step(defense), reference.step(defense))
        event = match.observe(1); pending = event["pending"]
        self.assertEqual(event["resource_counter_rule"], rule)
        self.assertEqual((match.status.phase, match.actor, match.status.owner), ("response", 1, 0))
        self.assertEqual((pending["origin"], pending["source"], pending["target"], pending["power"], pending["amount"]), (1, 1, 1, 0, 2))
        self.assertTrue(pending["counter"]); self.assertEqual(pending["models"], [187])
        self.assertEqual(stats(match, "mp"), [0, 0]); self.assertEqual(stats(match, "guardian"), ["", ""])
        self.assertFalse(match.legal(Command(1, "defend", (other[142],))))
        before = match.observe(1); before_status = match.status
        with self.assertRaises(InvalidMatchError): match.step(Command(1, "defend", (other[142],)))
        for policy_seed in range(64):
            proposal, rng = match.sample_action(policy_seed)
            self.assertTrue(match.legal(proposal)); self.assertEqual((proposal, rng), match.sample_action(policy_seed))
        self.assertEqual((match.observe(1), match.status), (before, before_status))
        for key in ("batch", "counters", "counter_created", "counter_started", "counter_rolls", "counter_hits", "rng"):
            self.assertNotIn(key, event); self.assertNotIn(key, pending)
        accept = Command(1, "defend")
        self.assertEqual(match.step(accept), reference.step(accept))
        self.assertEqual(stats(match, "hp"), [40, 39]); self.assertEqual(stats(match, "mp"), [0, 2])
        self.assertEqual((match.status.phase, match.actor, match.status.owner), ("main", 1, 1))
        for seat in (0, 1):
            self.assertEqual(match.observe(seat), reference.observe(seat))
            self.assertEqual(len(match.observe(seat)["board"]["hand"]), 18)
        capped = self.match(config=Config(initial_hand=18, initial_mp=98), seed=1004)
        own, other = hand(capped, 0), hand(capped, 1)
        capped.step(Command(0, "play", (own[6],), 1)); capped.step(Command(1, "defend", (other[187],))); capped.step(Command(1, "defend"))
        self.assertEqual(stats(capped, "mp"), [98, 99])
        # A self-reflection of the MP counter keeps a response and applies once.
        mirror = self.match(config=config, seed=7340)
        own, other = hand(mirror, 0), hand(mirror, 1)
        self.assertIn(6, own); self.assertTrue({187, 190} <= other.keys())
        mirror.step(Command(0, "play", (own[6],), 1)); mirror.step(Command(1, "defend", (other[187],)))
        mirror.step(Command(1, "defend", (other[190],)))
        self.assertEqual((mirror.actor, mirror.observe()["pending"]["source"], mirror.observe()["pending"]["target"]), (1, 1, 1))
        self.assertEqual(stats(mirror, "mp"), [0, 0])
        mirror.step(Command(1, "defend")); self.assertEqual(stats(mirror, "mp"), [0, 2])
        # Venus credits the full nominal amount after CP->MP->HP debt payment.
        cp = self.match(config=config, seed=14656)
        own, other = hand(cp, 0), hand(cp, 1)
        self.assertIn(32, own); self.assertIn(188, other)
        cp.step(Command(0, "play", (own[32],), 1)); cp.step(Command(1, "defend", (other[188],)))
        pending = cp.observe()["pending"]
        self.assertEqual((cp.actor, pending["source"], pending["target"], pending["power"], pending["amount"]), (0, 1, 0, 0, 9))
        cp.step(Command(0, "defend"))
        self.assertEqual(stats(cp, "hp"), [31, 31]); self.assertEqual(stats(cp, "mp"), [0, 0]); self.assertEqual(stats(cp, "cp"), [0, 9])
        self.assertIsNone(cp.observe()["pending"])
        cp_cap = self.match(config=Config(initial_hand=18, initial_mp=0, initial_cp=99), seed=14656)
        own, other = hand(cp_cap, 0), hand(cp_cap, 1)
        cp_cap.step(Command(0, "play", (own[32],), 1)); cp_cap.step(Command(1, "defend", (other[188],))); cp_cap.step(Command(0, "defend"))
        self.assertEqual(stats(cp_cap, "cp"), [90, 99])
        # Reflection changes creditor/debtor through the current source/target.
        reflected = self.match(config=config, seed=1794)
        own, other = hand(reflected, 0), hand(reflected, 1)
        self.assertTrue({6, 190} <= own.keys()); self.assertIn(188, other)
        reflected.step(Command(0, "play", (own[6],), 1)); reflected.step(Command(1, "defend", (other[188],)))
        reflected.step(Command(0, "defend", (own[190],)))
        pending = reflected.observe()["pending"]
        self.assertEqual((pending["origin"], pending["source"], pending["target"], pending["amount"]), (1, 0, 1, 1))
        reflected.step(Command(1, "defend"))
        self.assertEqual(stats(reflected, "cp"), [1, 0]); self.assertEqual(stats(reflected, "hp"), [40, 38])
        # Aliased source==target still debits first; later credit cannot undo HP0.
        for hp in (40, 2):
            alias = self.match(config=Config(initial_hand=18, initial_hp=hp, initial_mp=0, initial_cp=0), seed=14656)
            own = hand(alias, 0)
            alias.step(Command(0, "play", (own[6],), 0)); alias.step(Command(0, "defend", (own[188],)))
            pending = alias.observe()["pending"]
            self.assertEqual((pending["source"], pending["target"], pending["amount"]), (0, 0, 1))
            result = alias.step(Command(0, "defend"))
            self.assertEqual(stats(alias, "cp"), [1, 0]); self.assertEqual(stats(alias, "hp"), [hp - 2, hp])
            if hp == 2:
                self.assertTrue(result.terminated); self.assertEqual((result.winners, result.rewards), ((1,), (-1, 1)))
                self.assertTrue(alias.observe()["board"]["players"][0]["dead"])
            else:
                self.assertEqual((result.phase, result.actor), ("main", 1))

    def test_damage_effects_absorption_raw_order_self_damage_and_counters(self):
        rule = "experimental:positive-nondark-remainder;current-source-side-effect-before-target-damage;counter-basis-target-damage-only"
        config = Config(initial_hand=18, initial_mp=0)
        def hand(match, player): return {item["face"]: item["token"] for item in match.observe(player)["board"]["hand"]}
        def hp(match): return [p["status"]["hp"] for p in match.observe()["board"]["players"]]
        match = self.match(config=config); seen = set(); added = {27, 45, 98, 216, 53}
        self.assertTrue(added <= set(match.supported_models))
        for seed in range(4096):
            match.reset(seed); seen.update(hand(match, 0)); seen.update(hand(match, 1))
            self.assertTrue(seen <= set(match.supported_models))
            if added <= seen: break
        self.assertTrue(added <= seen, "all five complete damage-effect models are gifted")
        for initial_hp, target, expected in ((40, 1, [47, 33]), (98, 1, [99, 91]), (98, 0, [92, 98]), (3, 1, [10, 0])):
            effect = self.match(config=Config(initial_hand=18, initial_hp=initial_hp, initial_mp=0), seed=1121)
            own = hand(effect, 0); self.assertIn(27, own)
            effect.step(Command(0, "play", (own[27],), target))
            self.assertEqual(effect.observe()["damage_effect_rule"], rule)
            effect.step(Command(target, "defend")); self.assertEqual(hp(effect), expected)
            if initial_hp == 3: self.assertEqual(effect.status.winners, (0,))
        match.reset(1121); own, other = hand(match, 0), hand(match, 1)
        self.assertIn(176, other)
        match.step(Command(0, "play", (own[27],), 1)); match.step(Command(1, "defend", (other[176],)))
        self.assertEqual(hp(match), [40, 40])  # No source heal at zero remainder.
        # Current source receives the absorption effect after reflection.
        match.reset(182); own, other = hand(match, 0), hand(match, 1)
        self.assertIn(27, own); self.assertIn(38, other)
        match.step(Command(0, "play", (own[27],), 1)); match.step(Command(1, "defend", (other[38],)))
        self.assertEqual(hp(match), [40, 40])
        self.assertEqual((match.observe()["pending"]["source"], match.observe()["pending"]["target"]), (1, 0))
        match.step(Command(0, "defend")); self.assertEqual(hp(match), [33, 47])
        # Self-heal and target damage net to zero, but body damage7 still creates counter14.
        match.reset(1094); own = hand(match, 0)
        self.assertTrue({27, 184} <= own.keys())
        match.step(Command(0, "play", (own[27],), 0)); match.step(Command(0, "defend", (own[184],)))
        self.assertEqual(hp(match), [40, 40]); self.assertEqual(match.observe()["pending"]["power"], 14)
        match.step(Command(0, "defend")); self.assertEqual(hp(match), [26, 40])
        # The chance absorber keeps its ability through hit/miss handling.
        for seed, expected in ((2, [43, 37]), (214, [40, 40])):
            match.reset(seed); token = hand(match, 0)[98]
            match.step(Command(0, "play", (token,)))
            if match.status.phase == "response": match.step(Command(1, "defend"))
            self.assertEqual(hp(match), expected)
        # A paid first use consumes the declared10 MP and retains the same instance.
        paid = self.match(config=Config(initial_hand=18, initial_mp=10), seed=9249)
        paid_token = hand(paid, 0)[216]
        paid.step(Command(0, "play", (paid_token,), 1))
        self.assertEqual([p["status"]["mp"] for p in paid.observe()["board"]["players"]], [0, 10])
        paid.step(Command(1, "defend")); self.assertEqual(hp(paid), [50, 30])
        self.assertTrue(next(item for item in paid.observe(0)["board"]["hand"] if item["token"] == paid_token)["used"])
        # The held Light miracle accepts FREE, then heals the reflected current source.
        match.reset(9249); reference = self.match(config=config, seed=9249)
        own, other = hand(match, 0), hand(match, 1)
        self.assertTrue({216, 204} <= own.keys()); self.assertIn(39, other)
        cast = Command(0, "play", (own[216], own[204]), 1)
        before = match.observe(0)
        for policy_seed in range(64):
            proposal, rng = match.sample_action(policy_seed)
            self.assertTrue(match.legal(proposal)); self.assertEqual((proposal, rng), match.sample_action(policy_seed))
        self.assertEqual(match.observe(0), before)
        self.assertEqual(match.step(cast), reference.step(cast))
        reflect = Command(1, "defend", (other[39],))
        self.assertEqual(match.step(reflect), reference.step(reflect)); self.assertEqual(hp(match), [40, 40])
        self.assertEqual(match.step(Command(0, "defend")), reference.step(Command(0, "defend")))
        self.assertEqual(hp(match), [30, 50]); self.assertEqual(match.observe(0), reference.observe(0))
        self.assertEqual([p["status"]["mp"] for p in match.observe()["board"]["players"]], [0, 0])
        own_hand = match.observe(0)["board"]["hand"]
        self.assertEqual(len(own_hand), 18); self.assertNotIn(own[204], {item["token"] for item in own_hand})
        self.assertTrue(next(item for item in own_hand if item["token"] == own[216])["used"])
        # Self-damage uses two sequential raw subtractions when source==target.
        for initial_hp, target, expected in ((40, 1, [26, 26]), (40, 0, [12, 40]), (20, 0, [0, 20])):
            effect = self.match(config=Config(initial_hand=18, initial_hp=initial_hp, initial_mp=0), seed=79)
            token = hand(effect, 0)[53]
            effect.step(Command(0, "play", (token,), target)); effect.step(Command(target, "defend"))
            self.assertEqual(hp(effect), expected)
            if initial_hp == 20: self.assertEqual(effect.status.winners, (1,))
        match.reset(4115); own = hand(match, 0)
        self.assertTrue({53, 184} <= own.keys())
        match.step(Command(0, "play", (own[53],), 0)); match.step(Command(0, "defend", (own[184],)))
        self.assertEqual(hp(match), [12, 40])
        self.assertEqual(match.observe()["pending"]["power"], 28)  # Target14, not net28 doubled.
        match.step(Command(0, "defend")); self.assertEqual(match.status.winners, (1,))

    def test_flash_response_limit_and_free_main_cure(self):
        match = self.match(config=Config(initial_hand=18, initial_mp=0), seed=1292501)
        def hand(seat): return match.observe(seat)["board"]["hand"]
        own = {item["face"]: item["token"] for item in hand(0)}
        other = {item["face"]: item["token"] for item in hand(1)}
        armor = tuple(item["token"] for item in hand(0) if item["face"] in {113,118,123,124,130,135,136,142,149,150,154,155,161,162,166,170,172,176,180})[:2]
        self.assertEqual(len(armor), 2)
        self.assertTrue({70, 237, 204} <= own.keys())
        match.step(Command(0, "play", (own[70],), 0)); match.step(Command(0, "defend"))
        self.assertEqual(match.observe()["board"]["players"][0]["status"]["curses"], ["flash"])
        match.step(Command(1, "play", (other[6],), 0))
        before, status = match.observe(0), match.status
        invalid = Command(0, "defend", armor)
        self.assertFalse(match.legal(invalid))
        with self.assertRaises(InvalidMatchError): match.step(invalid)
        self.assertEqual((match.observe(0), match.status), (before, status))
        for seed in range(64):
            command, _ = match.sample_action(seed)
            self.assertTrue(match.legal(command)); self.assertLessEqual(len(command.items), 1)
        self.assertEqual((match.observe(0), match.status), (before, status))
        match.step(Command(0, "defend"))
        self.assertEqual(match.observe()["board"]["players"][0]["status"]["curses"], ["flash"])
        # The restriction is on responses: main miracle+FREE still clears Flash.
        match.step(Command(0, "play", (own[237], own[204]), 0))
        pane = match.observe()["board"]["players"][0]["status"]
        self.assertEqual((pane["curses"], pane["mp"], pane["hp"]), ([], 0, 37))
        self.assertTrue(next(i for i in hand(0) if i["token"] == own[237])["used"])
        self.assertNotIn(own[204], {i["token"] for i in hand(0)})
        match.step(Command(1, "play", (other[21],), 0))
        self.assertTrue(match.legal(Command(0, "defend", armor)))
        match.step(Command(0, "defend", armor))
        self.assertEqual(match.observe()["board"]["players"][0]["status"]["curses"], [])

    def test_flash_counter_reflection_and_chance(self):
        match = self.match(config=Config(initial_hand=18, initial_mp=0), seed=1967907)
        def tokens(seat): return {i["face"]: i["token"] for i in match.observe(seat)["board"]["hand"]}
        own, other = tokens(0), tokens(1)
        match.step(Command(0, "play", (own[6],), 1))
        match.step(Command(1, "defend", (other[185], other[184])))
        event = match.observe(); effect = event["pending"]
        self.assertEqual((effect["models"], effect["power"], effect["amount"], effect["category"]), ([185], 0, 0, "armor"))
        self.assertIn("one-card-response", event["flash_rule"])
        before, status = match.observe(0), match.status
        invalid = Command(0, "defend", (own[113],))
        self.assertFalse(match.legal(invalid))
        with self.assertRaises(InvalidMatchError): match.step(invalid)
        self.assertEqual((match.observe(0), match.status), (before, status))
        match.step(Command(0, "defend", (own[190],))); match.step(Command(1, "defend"))
        self.assertEqual([p["status"]["curses"] for p in match.observe()["board"]["players"]], [[], ["flash"]])
        self.assertEqual(match.observe()["pending"]["models"], [184])
        match.step(Command(0, "defend"))
        for seed, hit in ((534, True), (90, False)):
            chance = self.match(config=Config(initial_hand=18, initial_mp=3), seed=seed)
            token = next(i["token"] for i in chance.observe(0)["board"]["hand"] if i["face"] == 224)
            chance.step(Command(0, "play", (token,)))
            if hit:
                self.assertEqual(chance.observe()["pending"]["hit_rate"], 25)
                chance.step(Command(1, "defend"))
            pane = chance.observe()["board"]["players"][1]["status"]
            self.assertEqual((pane["hp"], pane["curses"]), (39, ["flash"]) if hit else (40, []))
            self.assertEqual(chance.observe()["board"]["players"][0]["status"]["mp"], 0)

    def test_darkcloud_damage_coexist_dedup_and_cure(self):
        def hand(game, seat): return {i["face"]: i["token"] for i in game.observe(seat)["board"]["hand"]}
        def curses(game): return [p["status"]["curses"] for p in game.observe()["board"]["players"]]
        def yield_other(game):
            self.assertEqual((game.status.phase, game.actor), ("main", 1))
            for item in game.observe(1)["board"]["hand"]:
                command = Command(1, "discard", (item["token"],))
                if game.legal(command): game.step(command); return
            self.fail("public reset fixture needs an ordinary discard to pass turn")
        config = Config(initial_hand=18, initial_mp=10)
        game = self.match(config=config, seed=36583)
        self.assertTrue({41, 186, 223} <= set(game.supported_models))
        own = hand(game, 0); self.assertTrue({70, 223, 199, 200} <= own.keys())
        game.step(Command(0, "play", (own[70],), 0)); game.step(Command(0, "defend"))
        self.assertEqual(curses(game), [["flash"], []]); yield_other(game)
        for unused in range(2):
            game.step(Command(0, "play", (own[223],), 0))
            self.assertEqual(curses(game), [["flash", "darkcloud"], []]); yield_other(game)
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["mp"], 0)
        self.assertTrue(next(i for i in game.observe(0)["board"]["hand"] if i["token"] == own[223])["used"])
        game.step(Command(0, "play", (own[199],), 0)); self.assertEqual(curses(game), [["darkcloud"], []]); yield_other(game)
        game.step(Command(0, "play", (own[200],), 0)); self.assertEqual(curses(game), [[], []])
        # Damage application includes a target killed by the original body.
        for hp in (40, 5):
            weapon = self.match(config=Config(initial_hand=18, initial_hp=hp), seed=9)
            token = hand(weapon, 0)[41]
            weapon.step(Command(0, "play", (token,), 1)); weapon.step(Command(1, "defend"))
            self.assertEqual(curses(weapon), [[], ["darkcloud"]])
            self.assertEqual(weapon.observe()["board"]["players"][1]["status"]["hp"], max(0, hp - 11))
            if hp == 5: self.assertEqual(weapon.status.winners, (0,))

    def test_darkcloud_forced_chance_counter_and_redirect(self):
        rule = "experimental:force-percent-hit-without-draw-at-target-activation;redirects-no-recheck;idempotent-until-all-cure;no-tick"
        config = Config(initial_hand=18, initial_mp=5)
        def hand(game, seat): return {i["face"]: i["token"] for i in game.observe(seat)["board"]["hand"]}
        def advance_pair(game, reference, reserved=()):
            for item in game.observe(1)["board"]["hand"]:
                command = Command(1, "discard", (item["token"],))
                if item["token"] not in reserved and game.legal(command):
                    self.assertEqual(game.step(command), reference.step(command)); return
            self.fail("no public discard for fixture turn")
        game = self.match(config=config, seed=3230); reference = self.match(config=config, seed=3230)
        own, other = hand(game, 0), hand(game, 1)
        self.assertTrue({223, 95} <= own.keys()); self.assertIn(190, other)
        for command in (Command(0, "play", (own[223],), 1), Command(1, "defend")):
            self.assertEqual(game.step(command), reference.step(command))
        advance_pair(game, reference, (other[190],))
        before = game.observe(0); status = game.status
        for seed in range(64):
            proposal, rng = game.sample_action(seed)
            self.assertTrue(game.legal(proposal)); self.assertEqual((proposal, rng), game.sample_action(seed))
        self.assertEqual((game.observe(0), game.status), (before, status))
        chance = Command(0, "play", (own[95],))
        self.assertEqual(game.step(chance), reference.step(chance))
        event = game.observe(); pending = event["pending"]
        self.assertEqual(event["darkcloud_rule"], rule)
        self.assertEqual((game.actor, pending["hit_rate"], pending["forced_hit"], pending["counter"]), (1, 75, True, False))
        for key in ("chance_forced_hits", "counter_forced_hits", "chance_hits", "counter_hits", "chance_rolls", "counter_rolls", "batch", "counters", "rng"):
            self.assertNotIn(key, event); self.assertNotIn(key, pending)
        reflected = Command(1, "defend", (other[190],))
        self.assertEqual(game.step(reflected), reference.step(reflected))
        self.assertEqual((game.actor, game.observe()["pending"]["forced_hit"]), (0, True))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["curses"], [])
        self.assertEqual(game.step(Command(0, "defend")), reference.step(Command(0, "defend")))
        self.assertEqual(game.observe(0), reference.observe(0))
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [39, 40])
        game = self.match(config=config, seed=11865); reference = self.match(config=config, seed=11865)
        own, other = hand(game, 0), hand(game, 1)
        self.assertTrue({223, 6} <= own.keys()); self.assertIn(181, other)
        self.assertEqual(game.step(Command(0, "play", (own[223],), 0)), reference.step(Command(0, "play", (own[223],), 0)))
        advance_pair(game, reference, (other[181],))
        for command in (Command(0, "play", (own[6],), 1), Command(1, "defend", (other[181],))):
            self.assertEqual(game.step(command), reference.step(command))
        pending = game.observe()["pending"]
        self.assertEqual((pending["models"], pending["hit_rate"], pending["forced_hit"], pending["counter"]), ([181], 75, True, True))
        self.assertEqual(game.step(Command(0, "defend")), reference.step(Command(0, "defend")))
        self.assertEqual(game.observe(0), reference.observe(0))
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [39, 39])

    def test_darkcloud_miracle_free_block_reflect_and_counter(self):
        def hand(game, seat): return {i["face"]: i["token"] for i in game.observe(seat)["board"]["hand"]}
        config = Config(initial_hand=18, initial_mp=0)
        for reflector in (39, 55):
            game = self.match(config=config, seed=6138700)
            own, other = hand(game, 0), hand(game, 1)
            self.assertTrue({223, 204} <= own.keys()); self.assertIn(reflector, other)
            game.step(Command(0, "play", (own[223], own[204]), 1))
            self.assertEqual([p["status"]["curses"] for p in game.observe()["board"]["players"]], [[], []])
            game.step(Command(1, "defend", (other[reflector],)))
            if reflector == 39: game.step(Command(0, "defend"))
            self.assertEqual([p["status"]["curses"] for p in game.observe()["board"]["players"]], [["darkcloud"], []] if reflector == 39 else [[], []])
            self.assertEqual([p["status"]["mp"] for p in game.observe()["board"]["players"]], [0, 0])
            self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [40, 40])
            items = game.observe(0)["board"]["hand"]
            self.assertEqual(len(items), 18); self.assertNotIn(own[204], {i["token"] for i in items})
            self.assertTrue(next(i for i in items if i["token"] == own[223])["used"])
        game = self.match(config=config, seed=22345)
        own, other = hand(game, 0), hand(game, 1)
        self.assertTrue({6, 190, 161} <= own.keys()); self.assertIn(186, other)
        game.step(Command(0, "play", (own[6],), 1)); game.step(Command(1, "defend", (other[186],)))
        pending = game.observe()["pending"]
        self.assertEqual((pending["models"], pending["power"], pending["amount"], pending["counter"]), ([186], 0, 0, True))
        invalid = Command(0, "defend", (own[161],)); before = game.observe(0); status = game.status
        self.assertFalse(game.legal(invalid))
        with self.assertRaises(InvalidMatchError): game.step(invalid)
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(Command(0, "defend", (own[190],))); game.step(Command(1, "defend"))
        self.assertEqual([p["status"]["curses"] for p in game.observe()["board"]["players"]], [[], ["darkcloud"]])
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [40, 39])

    def test_unused_item_removal_self_reflect_sole_and_no_recipient_gifts(self):
        rule = "experimental:unused-hand-max3;used-miracles-max2;uniform-without-replacement-in-inventory-order;bounded-draw-per-removal;empty-no-draw;no-removal-gifts"
        def items(game, seat): return game.observe(seat)["board"]["hand"]
        def token(game, seat, model): return next(i["token"] for i in items(game, seat) if i["face"] == model)
        config = Config(initial_hand=18, initial_mp=99)
        game = self.match(config=config, seed=33); reference = self.match(config=config, seed=33)
        self.assertTrue({205, 206} <= set(game.supported_models))
        source = token(game, 0, 205); original = {i["token"] for i in items(game, 1)}
        cast = Command(0, "play", (source,), 1)
        self.assertEqual(game.step(cast), reference.step(cast))
        event = game.observe(); self.assertEqual(event["removal_rule"], rule)
        self.assertEqual((event["pending"]["power"], event["pending"]["amount"]), (0, 3))
        self.assertIsNone(event["last_removal"])
        before = game.observe(1)
        for seed in range(64):
            proposal, rng = game.sample_action(seed)
            self.assertTrue(game.legal(proposal)); self.assertEqual((proposal, rng), game.sample_action(seed))
        self.assertEqual(game.observe(1), before)
        self.assertEqual(game.step(Command(1, "defend")), reference.step(Command(1, "defend")))
        remaining = {i["token"] for i in items(game, 1)}
        self.assertEqual((len(remaining), len(original - remaining)), (15, 3)); self.assertTrue(remaining <= original)
        self.assertEqual(len(items(game, 0)), 18)
        self.assertEqual(game.observe(0), reference.observe(0)); self.assertEqual(game.observe(1), reference.observe(1))
        removed_event = {"origin": 0, "source": 0, "target": 1, "model": 205, "removed_models": [40, 113, 65]}
        for viewer in (None, 0, 1):
            self.assertEqual(game.observe(viewer)["last_removal"], removed_event)
        for key in ("removed", "removed_models", "removed_slots", "removal_rng", "batch", "rng", "targets"):
            self.assertNotIn(key, game.observe()); self.assertNotIn(key, game.observe()["board"])
        game = self.match(config=config, seed=690)
        retained = token(game, 0, 235); broom = token(game, 0, 205)
        game.step(Command(0, "play", (retained,), 0))
        for item in items(game, 1):
            command = Command(1, "discard", (item["token"],))
            if game.legal(command): game.step(command); break
        game.step(Command(0, "play", (broom,), 0))
        self.assertEqual(len(items(game, 0)), 15)
        self.assertTrue(next(i for i in items(game, 0) if i["token"] == retained)["used"])
        game = self.match(config=config, seed=557)
        broom = token(game, 0, 205); mirror = token(game, 1, 190)
        game.step(Command(0, "play", (broom,), 1)); game.step(Command(1, "defend", (mirror,))); game.step(Command(0, "defend"))
        self.assertEqual([len(items(game, seat)) for seat in (0, 1)], [15, 18])
        # Unused miracles are ordinary Broom candidates, even when sole candidate.
        game = self.match(config=Config(initial_hand=1), seed=101314)
        self.assertEqual(items(game, 1)[0]["face"], 224); self.assertFalse(items(game, 1)[0]["used"])
        game.step(Command(0, "play", (token(game, 0, 205),), 1)); game.step(Command(1, "defend"))
        self.assertEqual([len(items(game, seat)) for seat in (0, 1)], [1, 0])
        game = self.match(config=Config(initial_hand=1), seed=33)
        game.step(Command(0, "play", (token(game, 0, 205),), 0))
        self.assertEqual(len(items(game, 0)), 1)  # Consume before eligibility: no self victim.

    def test_used_miracle_removal_copy_identity_maximum_empty_and_reflect(self):
        def items(game, seat): return game.observe(seat)["board"]["hand"]
        def token(game, seat, model): return next(i["token"] for i in items(game, seat) if i["face"] == model)
        def yield_other(game, reference=None, reserved=()):
            for item in items(game, 1):
                command = Command(1, "discard", (item["token"],))
                if item["token"] not in reserved and game.legal(command):
                    result = game.step(command)
                    if reference is not None: self.assertEqual(result, reference.step(command))
                    return
            self.fail("public fixture needs a discard")
        config = Config(initial_hand=18, initial_mp=99)
        game = self.match(config=config, seed=1143935); reference = self.match(config=config, seed=1143935)
        learned = {token(game, 0, model) for model in (235, 237, 238)}; soap = token(game, 0, 206)
        for model in (235, 237, 238):
            command = Command(0, "play", (token(game, 0, model),), 0)
            self.assertEqual(game.step(command), reference.step(command)); yield_other(game, reference)
        self.assertEqual({i["token"] for i in items(game, 0) if i["used"]}, learned)
        unused = {i["token"] for i in items(game, 0) if not i["used"]} - {soap}
        before = game.observe(0)
        for seed in range(64):
            proposal, _ = game.sample_action(seed); self.assertTrue(game.legal(proposal))
        self.assertEqual(game.observe(0), before)
        self.assertEqual(game.step(Command(0, "play", (soap,), 0)), reference.step(Command(0, "play", (soap,), 0)))
        remaining = {i["token"] for i in items(game, 0)}
        removed = learned - remaining
        self.assertEqual((len(items(game, 0)), len(removed)), (16, 2)); self.assertTrue(unused <= remaining)
        self.assertEqual(game.observe(0), reference.observe(0))
        yield_other(game)
        invalid = Command(0, "play", (next(iter(removed)),), 0)
        before = game.observe(0); status = game.status
        self.assertFalse(game.legal(invalid))
        with self.assertRaises(InvalidMatchError): game.step(invalid)
        self.assertEqual((game.observe(0), game.status), (before, status))
        game = self.match(config=config, seed=23945)
        copies = [i["token"] for i in items(game, 0) if i["face"] == 235]
        self.assertEqual(len(copies), 2); self.assertNotEqual(*copies)
        soap = token(game, 0, 206)
        for copy in copies: game.step(Command(0, "play", (copy,), 0)); yield_other(game)
        game.step(Command(0, "play", (soap,), 0))
        self.assertEqual(len(items(game, 0)), 16); self.assertTrue(set(copies).isdisjoint(i["token"] for i in items(game, 0)))
        self.assertFalse(any(i["used"] for i in items(game, 0)))
        self.assertEqual(game.observe()["last_removal"],
                         {"origin": 0, "source": 0, "target": 0, "model": 206, "removed_models": [235, 235]})
        game = self.match(config=config, seed=115843)
        spring, soap, mirror = token(game, 0, 235), token(game, 0, 206), token(game, 1, 190)
        game.step(Command(0, "play", (spring,), 0)); yield_other(game, reserved=(mirror,))
        game.step(Command(0, "play", (soap,), 1)); game.step(Command(1, "defend", (mirror,))); game.step(Command(0, "defend"))
        self.assertEqual([len(items(game, seat)) for seat in (0, 1)], [17, 17])
        self.assertNotIn(spring, {i["token"] for i in items(game, 0)})
        self.assertEqual(game.observe()["last_removal"],
                         {"origin": 0, "source": 1, "target": 0, "model": 206, "removed_models": [235]})
        # All initially unplayed miracles are ineligible, including after self consumption.
        for target in (0, 1):
            game = self.match(config=Config(initial_hand=1), seed=61757)
            command = Command(0, "play", (token(game, 0, 206),), target)
            original_other = items(game, 1)
            game.step(command)
            if target == 1: game.step(Command(1, "defend"))
            self.assertEqual([len(items(game, seat)) for seat in (0, 1)], [1, 1])
            self.assertFalse(any(i["used"] for i in items(game, 0)))
            self.assertEqual(items(game, 1), original_other)

    def test_removal_event_lifecycle_empty_and_invalid_calls(self):
        rule = "experimental:resolved-body-only;public-removed-models-in-draw-order;duplicates-and-empty-preserved;clear-on-next-accepted-main;no-tokens-or-rng"
        game = self.match(config=Config(initial_hand=1, initial_mp=0), seed=33)
        self.assertIsNone(game.observe()["last_removal"])
        self.assertEqual(game.observe()["removal_event_rule"], rule)
        self.assertEqual(game.observe(0)["board"]["hand"][0]["face"], 205)
        game.step(Command(0, "play", (0,), 0))
        expected = {"origin": 0, "source": 0, "target": 0, "model": 205, "removed_models": []}
        for viewer in (None, 0, 1): self.assertEqual(game.observe(viewer)["last_removal"], expected)
        before, status = game.observe(0), game.status
        invalid = Command(0, "pray")  # The accepted removal already advanced to player1.
        self.assertFalse(game.legal(invalid))
        with self.assertRaises(InvalidMatchError): game.step(invalid)
        self.assertEqual((game.observe(0), game.status), (before, status))
        detached = game.observe(); detached["last_removal"]["removed_models"].append(999)
        self.assertEqual(game.observe()["last_removal"], expected)
        other = game.observe(1)["board"]["hand"][0]
        self.assertEqual((other["token"], other["face"]), (1, 52))
        game.step(Command(1, "play", (other["token"],), 0))
        self.assertIsNone(game.observe()["last_removal"])
        game.reset(33)
        self.assertIsNone(game.observe()["last_removal"])

    def test_removal_event_pending_and_resolved_cutoff(self):
        game = self.match(config=Config(initial_hand=1, initial_mp=0, max_decisions=2), seed=101314)
        self.assertEqual(game.observe(1)["board"]["hand"][0]["face"], 224)
        game.step(Command(0, "play", (0,), 1))
        for viewer in (None, 0, 1): self.assertIsNone(game.observe(viewer)["last_removal"])
        before, status = game.observe(1), game.status
        invalid = Command(0, "defend")
        self.assertFalse(game.legal(invalid))
        with self.assertRaises(InvalidMatchError): game.step(invalid)
        self.assertEqual((game.observe(1), game.status), (before, status))
        game.step(Command(1, "defend"))
        self.assertTrue(game.status.truncated); self.assertFalse(game.status.terminated)
        expected = {"origin": 0, "source": 0, "target": 1, "model": 205, "removed_models": [224]}
        for viewer in (None, 0, 1):
            event = game.observe(viewer)["last_removal"]
            self.assertEqual(event, expected)
            self.assertEqual(set(event), {"origin", "source", "target", "model", "removed_models"})
        self.assertEqual([game.observe(seat)["board"]["hand"] for seat in (0, 1)], [[], []])

    def test_mana_weapon_zero_mp_ordered_cost_free_and_redirect(self):
        rule = "experimental:base-2x-mp-after-composed-miracle-cost;ordered-boosts;consume-all-mp-once-at-launch;redirects-no-repayment"
        def hand(game, seat=0): return {i["face"]: i["token"] for i in game.observe(seat)["board"]["hand"]}
        def mp(game): return [p["status"]["mp"] for p in game.observe()["board"]["players"]]
        zero = self.match(config=Config(initial_hand=18, initial_mp=0), seed=62)
        token = hand(zero)[58]; zero.step(Command(0, "play", (token,), 1))
        self.assertEqual((zero.status.phase, zero.actor, zero.observe()["pending"]["power"]), ("response", 1, 0))
        self.assertEqual(zero.observe()["mana_weapon_rule"], rule)
        zero.step(Command(1, "defend"))
        self.assertEqual([p["status"]["hp"] for p in zero.observe()["board"]["players"]], [40, 40])
        # All completed-column miracle costs precede the dynamic base; order still matters afterwards.
        for models, power in (((58, 231), 16), ((58, 210), 18), ((58, 210, 231), 12), ((58, 231, 210), 10)):
            game = self.match(config=Config(initial_hand=18, initial_mp=10, initial_hp=99), seed=128946)
            own = hand(game); self.assertTrue(set(models) <= own.keys())
            command = Command(0, "play", tuple(own[id] for id in models), 1)
            self.assertTrue(game.legal(command)); game.step(command)
            self.assertEqual((game.observe()["pending"]["power"], mp(game)), (power, [0, 10]))
            game.step(Command(1, "defend"))
            self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 99 - power)
            self.assertTrue(all(next(i for i in game.observe(0)["board"]["hand"] if i["token"] == own[id])["used"] for id in models[1:]))
        for models, mana, power in (((58, 12), 0, 2), ((58, 12, 231, 204), 10, 44), ((58, 231, 204, 12), 10, 42)):
            game = self.match(config=Config(initial_hand=18, initial_mp=mana, initial_hp=99), seed=1908520)
            own = hand(game); command = Command(0, "play", tuple(own[id] for id in models), 1)
            game.step(command); self.assertEqual(game.observe()["pending"]["power"], power); self.assertEqual(mp(game), [0, mana])
            game.step(Command(1, "defend"))
            self.assertEqual(len(game.observe(0)["board"]["hand"]), 18)
            self.assertNotIn(own[58], {i["token"] for i in game.observe(0)["board"]["hand"]})
            if 204 in models:
                self.assertNotIn(own[204], {i["token"] for i in game.observe(0)["board"]["hand"]})
                self.assertTrue(next(i for i in game.observe(0)["board"]["hand"] if i["token"] == own[231])["used"])
        config = Config(initial_hand=18, initial_mp=10)
        game = self.match(config=config, seed=69); reference = self.match(config=config, seed=69)
        own, other = hand(game), hand(game, 1); self.assertIn(38, other)
        before = game.observe(0)
        for seed in range(64):
            proposal, rng = game.sample_action(seed)
            self.assertTrue(game.legal(proposal)); self.assertEqual((proposal, rng), game.sample_action(seed))
        self.assertEqual(game.observe(0), before)
        for command in (Command(0, "play", (own[58],), 1), Command(1, "defend", (other[38],))):
            self.assertEqual(game.step(command), reference.step(command))
        pending = game.observe()["pending"]
        self.assertEqual((pending["source"], pending["target"], pending["power"], mp(game)), (1, 0, 20, [0, 10]))
        self.assertEqual(game.step(Command(0, "defend")), reference.step(Command(0, "defend")))
        self.assertEqual(game.observe(0), reference.observe(0))
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [20, 40])

    def test_repeat_weapon_single_payment_fresh_frames_counter_and_endings(self):
        rule = "experimental:two-fresh-passes-over-initial-targets;single-consumption-and-payment;counter-fifo-before-next-hit;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action"
        def hand(game, seat=0): return {i["face"]: i["token"] for i in game.observe(seat)["board"]["hand"]}
        def hp(game): return [p["status"]["hp"] for p in game.observe()["board"]["players"]]
        config = Config(initial_hand=18, initial_mp=0)
        game = self.match(config=config, seed=813); reference = self.match(config=config, seed=813)
        self.assertTrue({14, 58} <= set(game.supported_models))
        own, other = hand(game), hand(game, 1); self.assertIn(184, other)
        for command in (Command(0, "play", (own[14],), 1), Command(1, "defend", (other[184],))):
            self.assertEqual(game.step(command), reference.step(command))
        pending = game.observe()["pending"]
        self.assertEqual(game.observe()["repeat_attack_rule"], rule)
        self.assertEqual((game.actor, pending["counter"], pending["power"]), (0, True, 6)); self.assertEqual(hp(game), [40, 37])
        self.assertEqual(game.step(Command(0, "defend")), reference.step(Command(0, "defend")))
        pending = game.observe()["pending"]
        self.assertEqual((game.actor, pending["models"], pending["power"], pending["counter"]), (1, [14], 3, False))
        self.assertEqual(hp(game), [34, 37]); self.assertEqual(len(game.observe(0)["board"]["hand"]), 17)
        self.assertEqual(game.step(Command(1, "defend")), reference.step(Command(1, "defend")))
        self.assertEqual(hp(game), [34, 34]); self.assertEqual((game.actor, game.status.owner, game.status.decisions), (1, 1, 4))
        for seat in (0, 1):
            self.assertEqual(game.observe(seat), reference.observe(seat)); self.assertEqual(len(game.observe(seat)["board"]["hand"]), 18)
        paid = self.match(config=Config(initial_hand=18, initial_mp=10), seed=2553); own = hand(paid)
        paid.step(Command(0, "play", (own[14], own[210]), 1))
        self.assertEqual(paid.observe()["board"]["players"][0]["status"]["mp"], 8)
        for unused in range(2):
            self.assertEqual(paid.observe()["pending"]["power"], 5); paid.step(Command(1, "defend"))
        self.assertEqual(hp(paid), [40, 30]); self.assertEqual(paid.observe()["board"]["players"][0]["status"]["mp"], 8)
        self.assertTrue(next(i for i in paid.observe(0)["board"]["hand"] if i["token"] == own[210])["used"])
        fresh = self.match(config=config, seed=4997); own, other = hand(fresh), hand(fresh, 1)
        fresh.step(Command(0, "play", (own[14], own[66]), 1)); fresh.step(Command(1, "defend", (other[189],)))
        self.assertEqual((fresh.observe()["pending"]["element"], fresh.observe()["pending"]["power"]), ("fire", 5))
        fresh.step(Command(1, "defend")); self.assertEqual(hp(fresh), [40, 30])
        reflected = self.match(config=config, seed=813); own, other = hand(reflected), hand(reflected, 1)
        reflected.step(Command(0, "play", (own[14],), 1)); reflected.step(Command(1, "defend", (other[38],))); reflected.step(Command(0, "defend"))
        pending = reflected.observe()["pending"]
        self.assertEqual((pending["source"], pending["target"], pending["reflections"]), (0, 1, 0))
        reflected.step(Command(1, "defend")); self.assertEqual(hp(reflected), [37, 37])
        for seed, mirror, winner, decisions in ((16, False, 0, 2), (813, True, 1, 3)):
            ending = self.match(config=Config(initial_hand=18, initial_mp=0, initial_hp=3), seed=seed)
            own, other = hand(ending), hand(ending, 1)
            ending.step(Command(0, "play", (own[14],), 1))
            if mirror: ending.step(Command(1, "defend", (other[38],)))
            ending.step(Command(0 if mirror else 1, "defend"))
            self.assertTrue(ending.done); self.assertEqual((ending.status.winners, ending.status.decisions), ((winner,), decisions))

    def test_wide_optional_target_roster_and_canonical_sampler(self):
        rule = "experimental:initial-alive-enemies-in-input-order;optional-alive-target-does-not-alter-roster;repeat-by-full-roster-pass;single-payment;counter-fifo-before-next-target;dead-target-skip;origin-death-cancels-rest;gifts-after-whole-action"
        roster = [PlayerSetup(0, team=1), PlayerSetup(77, team=1), PlayerSetup(88), PlayerSetup(99, team=2)]
        config = Config(initial_hand=18, initial_mp=5)
        baseline = None
        for target in (None, 0, 77, 88, 99):
            game = self.match(config=config, players=roster, seed=15216)
            own = {i["face"]: i["token"] for i in game.observe(0)["board"]["hand"]}
            self.assertIn(232, game.supported_models); self.assertTrue({6, 232} <= own.keys())
            invalid = Command(0, "play", (own[232],), 88)
            self.assertFalse(game.legal(invalid)); self.assertFalse(game.legal(Command(0, "play", (own[6], own[232]), 9999)))
            before = game.observe(0); status = game.status
            with self.assertRaises(InvalidMatchError): game.step(invalid)
            self.assertEqual((game.observe(0), game.status), (before, status))
            for seed in range(64):
                proposal, _ = game.sample_action(seed); self.assertTrue(game.legal(proposal))
                if proposal.kind == "play" and own[232] in proposal.items: self.assertIsNone(proposal.target)
            command = Command(0, "play", (own[6], own[232]), target)
            self.assertTrue(game.legal(command)); game.step(command)
            trace = [game.observe(0)]
            self.assertEqual(trace[0]["wide_attack_rule"], rule)
            self.assertEqual(trace[0]["board"]["players"][0]["status"]["mp"], 0)
            for actor in (88, 99):
                event = game.observe(); pending = event["pending"]
                self.assertEqual((game.actor, game.status.owner, pending["power"], pending["hit_rate"], pending["forced_hit"]), (actor, 0, 1, 0, False))
                self.assertEqual(len(game.observe(0)["board"]["hand"]), 17)
                for key in ("batch", "targets", "cursor", "rng", "chance_rolls", "chance_forced_hits"):
                    self.assertNotIn(key, event); self.assertNotIn(key, pending)
                game.step(Command(actor, "defend")); trace.append(game.observe(0))
            self.assertEqual((game.status.decisions, game.actor, game.status.owner), (3, 77, 77))
            self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [40, 40, 39, 39])
            self.assertEqual(len(game.observe(0)["board"]["hand"]), 18)
            self.assertTrue(next(i for i in game.observe(0)["board"]["hand"] if i["token"] == own[232])["used"])
            if baseline is None: baseline = trace
            else: self.assertEqual(trace, baseline)
        # A percentage base still forbids Mirage, and the completion is not a standalone cast.
        invalid_game = self.match(config=config, seed=3857)
        own = {i["face"]: i["token"] for i in invalid_game.observe(0)["board"]["hand"]}
        self.assertTrue({95, 232} <= own.keys()); self.assertFalse(invalid_game.legal(Command(0, "play", (own[95], own[232]))))
        free_config = Config(initial_hand=18, initial_mp=0)
        game = self.match(config=free_config, seed=15216); reference = self.match(config=free_config, seed=15216)
        own = {i["face"]: i["token"] for i in game.observe(0)["board"]["hand"]}
        self.assertTrue({6, 232, 204} <= own.keys()); before = game.observe(0); reached = False
        for seed in range(4096):
            proposal, rng = game.sample_action(seed)
            self.assertTrue(game.legal(proposal)); self.assertEqual((proposal, rng), game.sample_action(seed))
            if own[232] in proposal.items:
                self.assertIsNone(proposal.target)
                if own[204] in proposal.items: reached = True; break
        self.assertTrue(reached, "MP0 sampler atomically adds Mirage+FREE then clears target")
        self.assertEqual(game.observe(0), before)
        cast = Command(0, "play", (own[6], own[232], own[204]))
        self.assertEqual(game.step(cast), reference.step(cast)); self.assertEqual(game.step(Command(1, "defend")), reference.step(Command(1, "defend")))
        self.assertEqual(game.observe(0), reference.observe(0))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["mp"], 0)

    def test_wide_repeat_counter_mana_copies_and_ordered_element(self):
        def hand(game, seat=0): return {i["face"]: i["token"] for i in game.observe(seat)["board"]["hand"]}
        roster = [PlayerSetup(0, team=1), PlayerSetup(77, team=1), PlayerSetup(88), PlayerSetup(99, team=2)]
        config = Config(initial_hand=18, initial_mp=5)
        repeat = self.match(config=config, players=roster, seed=3307); own = hand(repeat)
        repeat.step(Command(0, "play", (own[14], own[232])))
        for actor in (88, 99, 88, 99):
            self.assertEqual((repeat.actor, repeat.status.owner, repeat.observe()["pending"]["power"]), (actor, 0, 3))
            self.assertEqual(len(repeat.observe(0)["board"]["hand"]), 17)
            repeat.step(Command(actor, "defend"))
        self.assertEqual((repeat.status.decisions, repeat.actor), (5, 77))
        self.assertEqual([p["status"]["hp"] for p in repeat.observe()["board"]["players"]], [40, 40, 34, 34])
        self.assertEqual(repeat.observe()["board"]["players"][0]["status"]["mp"], 0)
        game = self.match(config=config, players=roster, seed=149293); own, enemy = hand(game), hand(game, 88)
        game.step(Command(0, "play", (own[6], own[232]))); game.step(Command(88, "defend", (enemy[184],)))
        self.assertEqual((game.actor, game.observe()["pending"]["counter"], game.observe()["pending"]["power"]), (0, True, 2))
        game.step(Command(0, "defend")); self.assertEqual((game.actor, game.observe()["pending"]["counter"]), (99, False))
        game.step(Command(99, "defend"))
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [38, 40, 39, 39])
        for mana, legal, power in ((4, False, None), (5, True, 0), (10, True, 10)):
            game = self.match(config=Config(initial_hand=18, initial_mp=mana), seed=69); own = hand(game)
            cast = Command(0, "play", (own[58], own[232]))
            self.assertEqual(game.legal(cast), legal)
            if not legal:
                before = game.observe(0)
                with self.assertRaises(InvalidMatchError): game.step(cast)
                self.assertEqual(game.observe(0), before); continue
            game.step(cast); self.assertEqual(game.observe()["pending"]["power"], power)
            self.assertEqual(game.observe()["board"]["players"][0]["status"]["mp"], 0)
            game.step(Command(1, "defend"))
        copies = self.match(config=Config(initial_hand=18, initial_mp=10), seed=170013)
        own = hand(copies); tokens = [i["token"] for i in copies.observe(0)["board"]["hand"] if i["face"] == 232]
        self.assertEqual(len(tokens), 2)
        copies.step(Command(0, "play", (own[6], *tokens)))
        self.assertEqual(copies.observe()["board"]["players"][0]["status"]["mp"], 0)
        copies.step(Command(1, "defend")); self.assertEqual(copies.status.decisions, 2)
        self.assertEqual(copies.observe()["board"]["players"][1]["status"]["hp"], 39)
        self.assertTrue(all(next(i for i in copies.observe(0)["board"]["hand"] if i["token"] == token)["used"] for token in tokens))
        for models, element in (((6, 66, 232), None), ((6, 232, 66), "fire")):
            game = self.match(config=config, seed=54753); own = hand(game)
            game.step(Command(0, "play", tuple(own[id] for id in models)))
            self.assertEqual((game.observe()["pending"]["power"], game.observe()["pending"]["element"]), (3, element))
            game.step(Command(1, "defend"))

    def test_random_hp_sign_self_foreign_clips_and_cutoff(self):
        rule = "experimental:launch-once;uniform-plus10-or-minus10;zero-heals-one-damages;fixed-through-reflections;nonattack-no-defense-or-counters;hp-clips-0-99;gifts-after-action"
        def hand(game, seat=0): return {i["face"]: i["token"] for i in game.observe(seat)["board"]["hand"]}
        for seed, delta in ((54, 10), (34, -10)):
            for target in (0, 1):
                game = self.match(config=Config(initial_hand=18, initial_mp=0), seed=seed)
                self.assertIn(202, game.supported_models); token = hand(game)[202]
                result = game.step(Command(0, "play", (token,), target))
                self.assertEqual(game.observe()["random_hp_rule"], rule)
                if target == 1:
                    pending = game.observe()["pending"]
                    self.assertEqual((result.actor, pending["amount"], pending["power"], pending["category"]), (1, delta, 0, "sundries"))
                    self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [40, 40])
                    result = game.step(Command(1, "defend"))
                self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [40 + delta if i == target else 40 for i in (0, 1)])
                self.assertEqual((result.decisions, result.actor), (1 if target == 0 else 2, 1))
                items = game.observe(0)["board"]["hand"]
                self.assertEqual(len(items), 18); self.assertNotIn(token, {i["token"] for i in items}); self.assertFalse(any(i["used"] for i in items))
        for hp in (98, 99):
            game = self.match(config=Config(initial_hand=18, initial_hp=hp), seed=54)
            token = hand(game)[202]; game.step(Command(0, "play", (token,), 0))
            self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 99)
            self.assertEqual((game.status.decisions, len(game.observe(0)["board"]["hand"])), (1, 18))
            self.assertNotIn(token, {i["token"] for i in game.observe(0)["board"]["hand"]})
        game = self.match(config=Config(initial_hand=18, initial_mp=0, max_decisions=1), seed=34)
        game.step(Command(0, "play", (hand(game)[202],), 1))
        self.assertTrue(game.status.truncated); self.assertIsNone(game.observe()["pending"])
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [40, 40])
        self.assertEqual(len(game.observe(0)["board"]["hand"]), 17)
        for target, limit, winner in ((0, 1, 1), (1, 2, 0)):
            game = self.match(config=Config(initial_hand=18, initial_hp=5, max_decisions=limit), seed=34)
            game.step(Command(0, "play", (hand(game)[202],), target))
            if target: game.step(Command(1, "defend"))
            self.assertTrue(game.status.terminated); self.assertFalse(game.status.truncated); self.assertEqual(game.status.winners, (winner,))

    def test_random_hp_reflections_nonattack_atomic_queries_and_discard(self):
        config = Config(initial_hand=18, initial_mp=0)
        def hand(game, seat=0): return {i["face"]: i["token"] for i in game.observe(seat)["board"]["hand"]}
        game = self.match(config=config, seed=709465); reference = self.match(config=config, seed=709465)
        own, other = hand(game), hand(game, 1)
        self.assertTrue({202, 190} <= own.keys()); self.assertTrue({190, 118, 184} <= other.keys())
        before = game.observe(0); status = game.status
        invalid = Command(0, "play", (own[202], own[190]), 1)
        self.assertFalse(game.legal(invalid))
        with self.assertRaises(InvalidMatchError): game.step(invalid)
        for seed in range(64):
            proposal, rng = game.sample_action(seed)
            self.assertTrue(game.legal(proposal)); self.assertEqual((proposal, rng), game.sample_action(seed))
        self.assertEqual((game.observe(0), game.status), (before, status))
        cast = Command(0, "play", (own[202],), 1)
        self.assertEqual(game.step(cast), reference.step(cast))
        delta = game.observe()["pending"]["amount"]; self.assertIn(delta, (-10, 10))
        before = game.observe(1); status = game.status
        for model in (118, 184):
            invalid = Command(1, "defend", (other[model],))
            self.assertFalse(game.legal(invalid))
            with self.assertRaises(InvalidMatchError): game.step(invalid)
        self.assertEqual((game.observe(1), game.status), (before, status))
        for actor, token in ((1, other[190]), (0, own[190])):
            reflect = Command(actor, "defend", (token,))
            self.assertEqual(game.step(reflect), reference.step(reflect))
            self.assertEqual((game.observe()["pending"]["amount"], game.observe()["pending"]["power"]), (delta, 0))
        self.assertEqual(game.observe()["pending"]["reflections"], 2)
        self.assertEqual(game.step(Command(1, "defend")), reference.step(Command(1, "defend")))
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [40, 40 + delta])
        for viewer in (0, 1): self.assertEqual(game.observe(viewer), reference.observe(viewer))
        # Discard202 has the same RNG schedule as discarding an ordinary armor.
        discarded = self.match(config=config, seed=937); control = self.match(config=config, seed=937)
        a, b = hand(discarded), hand(control)
        discarded.step(Command(0, "discard", (a[202],))); control.step(Command(0, "discard", (b[118],)))
        healing = hand(discarded, 1)[191]
        discarded.step(Command(1, "play", (healing,), 1)); control.step(Command(1, "play", (healing,), 1))
        self.assertEqual(discarded.observe(1), control.observe(1))

    def test_passive_revival_bodies_and_deferred_copies(self):
        def items(game, seat): return game.observe(seat)["board"]["hand"]
        def token(game, seat, model): return next(i["token"] for i in items(game, seat) if i["face"] == model)
        for seed, model, hp in ((1288, 57, 5), (380, 64, 40), (1205, 202, 5)):
            game = self.match(config=Config(initial_hand=18, initial_hp=hp, initial_mp=0), seed=seed)
            amulet = token(game, 1, 208)
            game.step(Command(0, "play", (token(game, 0, model),), 1))
            if model == 202: self.assertEqual(game.observe()["pending"]["amount"], -10)
            self.assertFalse(game.legal(Command(1, "defend", (amulet,))))
            game.step(Command(1, "defend"))
            player = game.observe()["board"]["players"][1]
            self.assertEqual(player["status"]["hp"], 10); self.assertFalse(player["dead"])
            self.assertFalse(game.done); self.assertEqual(game.actor, 1)
            self.assertEqual(len(items(game, 1)), 18); self.assertNotIn(amulet, {i["token"] for i in items(game, 1)})
            self.assertNotIn("revivals", game.observe()); self.assertIn("one-deferred-gift", game.observe()["revival_rule"])
        for limit in (512, 2):
            game = self.match(config=Config(initial_hand=18, initial_hp=1, initial_mp=0, max_decisions=limit), seed=1043582)
            copies = [i["token"] for i in items(game, 1) if i["face"] == 208]
            self.assertGreaterEqual(len(copies), 2)
            game.step(Command(0, "play", (token(game, 0, 14), token(game, 0, 203)), 1)); game.step(Command(1, "defend"))
            self.assertEqual(len(items(game, 1)), 17); self.assertNotIn(copies[0], {i["token"] for i in items(game, 1)})
            self.assertIn(copies[1], {i["token"] for i in items(game, 1)})
            if limit == 2:
                self.assertTrue(game.status.truncated); self.assertIsNone(game.observe()["pending"])
            else:
                self.assertEqual(game.observe()["pending"]["power"], 13); game.step(Command(1, "defend"))
                self.assertFalse(game.done); self.assertEqual(len(items(game, 1)), 18)
                self.assertTrue(set(copies[:2]).isdisjoint(i["token"] for i in items(game, 1)))
            self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 10)

    def test_full_passive_hand_prayer_and_manual_rejections(self):
        game = self.match(config=Config(initial_hand=1, hand_limit=1, initial_mp=0), seed=29)
        self.assertEqual(game.observe(0)["board"]["hand"][0]["face"], 208)
        before, status = game.observe(0), game.status
        for command in (Command(0, "play", (0,), 0), Command(0, "discard", (0,))):
            self.assertFalse(game.legal(command))
            with self.assertRaises(InvalidMatchError): game.step(command)
        for seed in range(32):
            command, _ = game.sample_action(seed); self.assertEqual(command, Command(0, "pray")); self.assertTrue(game.legal(command))
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(Command(0, "pray")); self.assertEqual(game.actor, 1)
        self.assertEqual(game.observe(0)["board"]["hand"], before["board"]["hand"])

    def test_danger_public_selection_reflection_and_wide_gap(self):
        config = Config(initial_hand=18, initial_hp=99, initial_mp=0)
        game = self.match(config=config, seed=10783); reference = self.match(config=config, seed=10783)
        def hand(game, seat): return {i["face"]: i["token"] for i in game.observe(seat)["board"]["hand"]}
        own, other = hand(game, 0), hand(game, 1)
        for seed in range(64):
            proposal, _ = game.sample_action(seed); self.assertTrue(game.legal(proposal))
            if proposal.kind == "play" and proposal.items[0] == own[94]: self.assertIsNone(proposal.target)
        game.step(Command(0, "play", (own[94],)))
        reference.step(Command(0, "play", (own[94],), 0))
        self.assertEqual(game.observe(), reference.observe())
        event = game.observe()["last_danger"]
        self.assertEqual(event, dict(origin=0, target=1, models=[94], mortar=False, power=30, element="light"))
        self.assertEqual(set(event), {"origin", "target", "models", "mortar", "power", "element"})
        for value in (game, reference): value.step(Command(1, "defend", (other[189], other[38])))
        self.assertEqual((game.actor, game.observe()["pending"]["element"]), (0, None))
        self.assertEqual(game.observe()["last_danger"], event)
        for value in (game, reference): value.step(Command(0, "defend"))
        self.assertEqual(game.observe(0), reference.observe(0)); self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 69)
        unsupported = self.match(config=Config(initial_hand=18, initial_mp=99), seed=275); cards = hand(unsupported, 0)
        command = Command(0, "play", (cards[94], cards[232])); before = unsupported.observe(0)
        self.assertFalse(unsupported.legal(command))
        with self.assertRaises(UnsupportedMatchError): unsupported.step(command)
        self.assertEqual(unsupported.observe(0), before)

    def test_danger_mortar_immediate_consumption_and_ordered_boost(self):
        def items(game, seat): return game.observe(seat)["board"]["hand"]
        def token(game, seat, model): return next(i["token"] for i in items(game, seat) if i["face"] == model)
        for seed, target, winner in ((2644, 0, 1), (74, 1, 0)):
            game = self.match(config=Config(initial_hand=18, initial_hp=99), seed=seed)
            mortar = token(game, target, 209)
            if target == 0:
                for command in (Command(0, "play", (mortar,), 0), Command(0, "discard", (mortar,))):
                    self.assertFalse(game.legal(command))
                    with self.assertRaises(InvalidMatchError): game.step(command)
            game.step(Command(0, "play", (token(game, 0, 94),)))
            self.assertTrue(game.status.terminated); self.assertEqual(game.status.decisions, 1)
            self.assertEqual(game.status.winners, (winner,)); self.assertIsNone(game.observe()["pending"])
            self.assertEqual((game.observe()["last_danger"]["target"], game.observe()["last_danger"]["power"], game.observe()["last_danger"]["mortar"]), (target, 99, True))
            self.assertNotIn(mortar, {i["token"] for i in items(game, target)})
            self.assertEqual(len(items(game, target)), 16 if target == 0 else 17)
        game = self.match(config=Config(initial_hand=18, initial_hp=99, initial_mp=0), seed=327731)
        cards = tuple(token(game, 0, model) for model in (94, 231, 204, 66))
        game.step(Command(0, "play", cards)); event = game.observe()["last_danger"]
        self.assertEqual((event["power"], event["element"], event["mortar"]), (62, "fire", False))
        game.step(Command(event["target"], "defend"))
        self.assertEqual(game.observe()["board"]["players"][event["target"]]["status"]["hp"], 37)

    def test_exchange_direct_allocation_and_same_value_consumption(self):
        def hand(game): return game.observe(0)["board"]["hand"]
        for allocation in ((20, 30, 10), (40, 0, 20)):
            game = self.match(config=Config(initial_hand=18, initial_mp=0), seed=16)
            token = next(i["token"] for i in hand(game) if i["face"] == 3)
            self.assertTrue(game.legal(Command(0, "exchange", (token,), allocation=allocation)))
            result = game.step(Command(0, "exchange", (token,), allocation=allocation))
            own = game.observe()["board"]["players"][0]["status"]
            self.assertEqual(tuple(own[k] for k in ("hp", "mp", "cp")), allocation)
            self.assertEqual((result.phase, result.actor, result.decisions), ("main", 1, 1)); self.assertIsNone(game.observe()["pending"])
            self.assertEqual(len(hand(game)), 18); self.assertNotIn(token, {i["token"] for i in hand(game)})
            self.assertIn("sum-conserved-before-revival", game.observe()["exchange_rule"])
        game = self.match(config=Config(initial_hand=18, initial_mp=0, max_decisions=1), seed=16)
        token = next(i["token"] for i in hand(game) if i["face"] == 3)
        game.step(Command(0, "exchange", (token,), allocation=(20,30,10)))
        self.assertTrue(game.status.truncated); self.assertEqual(len(hand(game)),17)
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["mp"],30)

    def test_exchange_hp_zero_natural_death_and_revival(self):
        for seed, revived in ((0,False),(136,True)):
            game = self.match(config=Config(initial_hand=18, initial_hp=5, initial_mp=0, initial_cp=0, max_decisions=1), seed=seed)
            items = game.observe(0)["board"]["hand"]; token = next(i["token"] for i in items if i["face"] == 3)
            self.assertEqual(any(i["face"]==208 for i in items),revived)
            game.step(Command(0,"exchange",(token,),allocation=(0,5,0)))
            own = game.observe()["board"]["players"][0]
            self.assertEqual((own["status"]["hp"],own["status"]["mp"],own["status"]["cp"]),(10 if revived else 0,5,0))
            self.assertEqual((game.status.terminated,game.status.truncated),(not revived,revived))
            self.assertEqual(len(game.observe(0)["board"]["hand"]),16 if revived else 17)
            if not revived:self.assertEqual(game.status.winners,(1,))

    def test_exchange_allocation_transport_and_policy_purity(self):
        game = self.match(config=Config(initial_hand=1,hand_limit=1),seed=0)
        self.assertEqual(game.observe(0)["board"]["hand"][0]["face"],3)
        before,status = game.observe(0),game.status
        for command in (Command(0,"exchange",(0,)),Command(0,"exchange",(0,),0,(40,10,20)),
                        Command(0,"exchange",(0,),allocation=(10,20,31)),Command(0,"play",(0,),0),
                        Command(0,"pray",allocation=(40,10,20))):
            self.assertFalse(game.legal(command))
            with self.assertRaises(InvalidMatchError):game.step(command)
        for allocation in ((1,2),(1,2,3,4),(True,0,69),(100,0,0),(-1,51,20),(1.0,49,20),iter((40,10,20))):
            with self.assertRaises(InvalidMatchError):game.step(Command(0,"exchange",(0,),allocation=allocation))
        seen = False
        for seed in range(128):
            command,rng = game.sample_action(seed); self.assertEqual((command,rng),game.sample_action(seed));self.assertTrue(game.legal(command))
            if command.kind=="exchange":
                seen=True;self.assertEqual(command.items,(0,));self.assertIsNone(command.target)
                self.assertEqual(sum(command.allocation),70);self.assertTrue(all(0<=x<=99 for x in command.allocation))
            else:self.assertIsNone(command.allocation)
        self.assertTrue(seen);self.assertEqual((game.observe(0),game.status),(before,status))

    def test_sell_self_debt_transport_and_policy_atomicity(self):
        game = self.match(config=Config(initial_hand=18, initial_cp=3, initial_mp=4), seed=395)
        own = game.observe(0)["board"]["hand"]
        by_token = {i["token"]: i["face"] for i in own}
        self.assertEqual((by_token[0], by_token[15]), (4, 153))
        command = Command(0, "sell", (0, 15), 0)
        before, status = game.observe(0), game.status
        invalid = (Command(0, "play", (0,), 0), Command(0, "sell", (0, 0), 0),
                   Command(0, "sell", (15, 0), 0), Command(0, "sell", (0,), 0),
                   Command(0, "sell", (0, 15)), Command(0, "sell", (0, 15), 0, (40, 4, 3)))
        for bad in invalid:
            self.assertFalse(game.legal(bad))
            with self.assertRaises(InvalidMatchError): game.step(bad)
            self.assertEqual((game.observe(0), game.status), (before, status))
        full = Command(0, "sell", (0, 15), 1)
        self.assertTrue(game.legal(full))
        self.assertEqual((game.observe(0), game.status), (before, status))
        saw_sell = False
        for seed in range(128):
            proposal, rng = game.sample_action(seed)
            self.assertEqual((proposal, rng), game.sample_action(seed)); self.assertTrue(game.legal(proposal))
            if proposal.kind == "sell":
                saw_sell = True
                self.assertEqual(len(proposal.items), 2); self.assertEqual(len(set(proposal.items)), 2)
                self.assertIn(proposal.target, (0, 1)); self.assertIsNone(proposal.allocation)
        self.assertTrue(saw_sell); self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(command)
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 37)
        self.assertEqual({k: game.observe()["board"]["players"][0]["status"][k] for k in ("mp", "cp")}, {"mp": 0, "cp": 10})
        expected = {"origin": 0, "source": 0, "target": 0, "model": 153, "price": 10, "overflow_model": None}
        for viewer in (None, 0, 1): self.assertEqual(game.observe(viewer)["last_sale"], expected)
        after = game.observe(0)["board"]["hand"]
        self.assertEqual(len(after), 18); self.assertTrue({0, 15}.isdisjoint(i["token"] for i in after))
        self.assertEqual(next(i for i in after if i["token"] == 36)["face"], 153)
        before, status = game.observe(0), game.status
        with self.assertRaises(InvalidMatchError): game.step(command)
        self.assertEqual((game.observe(0), game.status), (before, status))
        other = next(i for i in game.observe(1)["board"]["hand"] if game.legal(Command(1, "discard", (i["token"],))))
        game.step(Command(1, "discard", (other["token"],)))
        self.assertIsNone(game.observe()["last_sale"])
        game.reset(395); self.assertIsNone(game.observe()["last_sale"])

    def test_sell_reflected_unused_miracle_is_transferred_not_learned(self):
        game = self.match(config=Config(initial_hand=4, initial_mp=99), seed=190782)
        self.assertEqual({i["token"]: i["face"] for i in game.observe(0)["board"]["hand"]}, {0: 215, 1: 4, 2: 221, 3: 31})
        self.assertEqual(next(i["face"] for i in game.observe(1)["board"]["hand"] if i["token"] == 4), 190)
        game.step(Command(0, "sell", (1, 0), 1))
        self.assertIsNone(game.observe()["last_sale"])
        self.assertEqual((game.observe()["pending"]["power"], game.observe()["pending"]["amount"]), (0, 0))
        with self.assertRaises(InvalidMatchError): game.step(Command(1, "defend", (6,)))
        game.step(Command(1, "defend", (4,)))
        self.assertIsNone(game.observe()["last_sale"])
        game.step(Command(0, "defend"))
        expected = {"origin": 0, "source": 1, "target": 0, "model": 215, "price": 0, "overflow_model": None}
        for viewer in (None, 0, 1): self.assertEqual(game.observe(viewer)["last_sale"], expected)
        own = game.observe(0)["board"]["hand"]
        received = next(i for i in own if i["token"] == 8)
        self.assertEqual((received["face"], received["used"]), (215, False))
        self.assertTrue({0, 1}.isdisjoint(i["token"] for i in own))
        self.assertEqual([p["status"]["mp"] for p in game.observe()["board"]["players"]], [99, 99])
        self.assertEqual([len(game.observe(seat)["board"]["hand"]) for seat in (1, 0)], [4, 4])
        learned = self.match(config=Config(initial_hand=4, initial_mp=99), seed=190782)
        learned.step(Command(0, "play", (0,), 0)); learned.step(Command(0, "defend"))
        learned.step(Command(1, "discard", (4,)))
        bad = Command(0, "sell", (1, 0), 0); before = learned.observe(0)
        self.assertFalse(learned.legal(bad))
        with self.assertRaises(InvalidMatchError): learned.step(bad)
        self.assertEqual(learned.observe(0), before)

    def test_sell_delivery_before_death_and_cutoff_without_gift(self):
        for hp, cp, mp, maximum in ((40, 3, 4, 2), (5, 0, 0, 512)):
            game = self.match(config=Config(initial_hand=4, initial_hp=hp, initial_cp=cp, initial_mp=mp, max_decisions=maximum), seed=51)
            self.assertEqual({i["token"]: i["face"] for i in game.observe(0)["board"]["hand"]}, {0: 123, 1: 43, 2: 26, 3: 4})
            game.step(Command(0, "sell", (3, 1), 1)); self.assertIsNone(game.observe()["last_sale"])
            game.step(Command(1, "defend"))
            self.assertEqual(game.observe()["last_sale"], {"origin": 0, "source": 0, "target": 1, "model": 43, "price": 10, "overflow_model": None})
            received = next(i for i in game.observe(1)["board"]["hand"] if i["token"] == 8)
            self.assertEqual((received["face"], received["used"]), (43, False))
            self.assertEqual([len(game.observe(seat)["board"]["hand"]) for seat in (0, 1)], [2, 5])
            if maximum == 2:
                self.assertTrue(game.status.truncated); self.assertFalse(game.status.terminated)
                self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 37)
            else:
                self.assertTrue(game.status.terminated); self.assertFalse(game.status.truncated)
                self.assertEqual(game.status.winners, (0,))
                self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 0)

    def test_buy_quote_accept_public_transport_and_atomic_queries(self):
        game = self.match(config=Config(initial_hand=4, initial_cp=99, initial_mp=0), seed=3440)
        original = game.observe(1)["board"]["hand"]
        self.assertEqual(game.observe()["buy_rule"], "experimental:one-operation-gift;uniform-unused-commodity-in-inventory-order;singleton-one-draw;empty-no-draw;foreign-cp-only-affordability;foreign-buyer-accept-or-decline;foreign-escrow-return-on-decline-or-cutoff;foreign-fresh-token-delivery;foreign-full-recipient-uniform-existing-item-overflow;self-immediate-inspection-after-operation-consumption;self-public-actual-model-and-unmask;self-same-slot-token-and-unused-state;self-no-cp-check-or-transfer-or-quote;self-result-before-cutoff-and-gift-after-action")
        buy = Command(0, "buy", (self.token(game, 0, 5),), 1)
        before, status = game.observe(0), game.status
        for bad in (Command(0, "play", (self.token(game, 0, 5),), 1), Command(0, "buy", (self.token(game, 0, 5), self.token(game, 0, 5)), 1),
                    Command(0, "buy", (self.token(game, 0, 5),)), Command(0, "buy", (self.token(game, 0, 5),), 1, (40, 0, 99))):
            self.assertFalse(game.legal(bad))
            with self.assertRaises(InvalidMatchError): game.step(bad)
            self.assertEqual((game.observe(0), game.status), (before, status))
        self.assertTrue(game.legal(Command(0, "buy", (self.token(game, 0, 5),), 0)))
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(buy); self.assertIsNone(game.observe()["purchase"]); self.assertIsNone(game.observe()["last_purchase"])
        game.step(Command(1, "defend"))
        quote = game.observe()["purchase"]
        self.assertEqual(set(quote), {"origin", "buyer", "seller", "model", "price"})
        self.assertEqual((quote["origin"], quote["buyer"], quote["seller"]), (0, 0, 1))
        self.assertEqual((game.status.phase, game.actor, game.status.owner), ("purchase", 0, 0))
        self.assertIsNone(game.observe()["pending"]); self.assertIsNone(game.observe()["last_purchase"])
        for viewer in (None, 0, 1): self.assertEqual(game.observe(viewer)["purchase"], quote)
        remaining = game.observe(1)["board"]["hand"]
        missing = [i for i in original if i["token"] not in {v["token"] for v in remaining}]
        self.assertEqual(len(missing), 1); self.assertEqual(missing[0]["face"], quote["model"])
        before, status = game.observe(0), game.status
        for bad in (Command(1, "accept"), Command(0, "accept", (0,)), Command(0, "decline", (), 1), Command(0, "pray")):
            self.assertFalse(game.legal(bad))
            with self.assertRaises(InvalidMatchError): game.step(bad)
            self.assertEqual((game.observe(0), game.status), (before, status))
        seen = set()
        for seed in range(32):
            proposal, rng = game.sample_action(seed)
            self.assertEqual((proposal, rng), game.sample_action(seed)); self.assertTrue(game.legal(proposal))
            seen.add(proposal.kind); self.assertFalse(proposal.items); self.assertIsNone(proposal.target); self.assertIsNone(proposal.allocation)
        self.assertEqual(seen, {"accept", "decline"}); self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(Command(0, "accept"))
        self.assertIsNone(game.observe()["purchase"])
        self.assertEqual(game.observe()["last_purchase"], dict(quote, bought=True, reason="bought", overflow_model=None))
        received = next(i for i in game.observe(0)["board"]["hand"] if i["token"] == 8)
        self.assertEqual((received["face"], received["used"]), (quote["model"], False))
        self.assertEqual([len(game.observe(seat)["board"]["hand"]) for seat in (0, 1)], [5, 3])
        self.assertEqual([p["status"]["cp"] for p in game.observe()["board"]["players"]], [99 - quote["price"], 99])

    def test_buy_decline_and_cutoff_restore_exact_escrow_copy(self):
        for maximum in (512, 2, 1):
            game = self.match(config=Config(initial_hand=4, initial_cp=99, initial_mp=0, max_decisions=maximum), seed=3440)
            original = game.observe(1)["board"]["hand"]
            game.step(Command(0, "buy", (self.token(game, 0, 5),), 1))
            if maximum == 1:
                self.assertTrue(game.status.truncated); self.assertIsNone(game.observe()["last_purchase"])
            else:
                game.step(Command(1, "defend"))
                if maximum == 512:
                    quote = game.observe()["purchase"]; game.step(Command(0, "decline"))
                    self.assertEqual(game.observe()["last_purchase"], dict(quote, bought=False, reason="declined", overflow_model=None))
                else:
                    self.assertTrue(game.status.truncated)
                    self.assertEqual(game.observe()["last_purchase"]["reason"], "cutoff")
            self.assertIsNone(game.observe()["purchase"])
            self.assertEqual(game.observe(1)["board"]["hand"], original)
            self.assertEqual([p["status"]["cp"] for p in game.observe()["board"]["players"]], [99, 99])
            self.assertEqual(len(game.observe(0)["board"]["hand"]), 4 if maximum == 512 else 3)
            if maximum == 512:
                item = next(i for i in original if game.legal(Command(1, "discard", (i["token"],))))
                game.step(Command(1, "discard", (item["token"],)))
                self.assertIsNone(game.observe()["last_purchase"])
            game.reset(3440); self.assertIsNone(game.observe()["last_purchase"])

    def test_buy_reflection_uses_current_buyer_and_seller(self):
        game = self.match(config=Config(initial_hand=4, initial_cp=99, initial_mp=0), seed=23962)
        game.step(Command(0, "buy", (self.token(game, 0, 5),), 1)); game.step(Command(1, "defend", (self.token(game, 1, 190),)))
        self.assertIsNone(game.observe()["purchase"])
        game.step(Command(0, "defend"))
        quote = game.observe()["purchase"]
        self.assertEqual((quote["origin"], quote["buyer"], quote["seller"], game.actor), (0, 1, 0, 1))
        game.step(Command(1, "accept"))
        self.assertEqual(game.observe()["last_purchase"], dict(quote, bought=True, reason="bought", overflow_model=None))
        received = next(i for i in game.observe(1)["board"]["hand"] if i["token"] == 8)
        self.assertEqual((received["face"], received["used"]), (quote["model"], False))
        self.assertEqual([len(game.observe(seat)["board"]["hand"]) for seat in (0, 1)], [3, 5])
        self.assertEqual([p["status"]["mp"] for p in game.observe()["board"]["players"]], [0, 0])

    def test_buy_empty_insufficient_cp_and_used_miracle_exclusion(self):
        game = self.match(config=Config(initial_hand=1, initial_cp=0, initial_mp=0), seed=3049)
        original = game.observe(1)["board"]["hand"]
        game.step(Command(0, "buy", (0,), 1)); game.step(Command(1, "defend"))
        self.assertIsNone(game.observe()["purchase"])
        self.assertEqual(game.observe()["last_purchase"], {"origin": 0, "buyer": 0, "seller": 1, "model": 6, "price": 1, "bought": False, "reason": "insufficient-cp", "overflow_model": None})
        self.assertEqual(game.observe(1)["board"]["hand"], original)
        paid = self.match(config=Config(initial_hand=1, initial_cp=5, initial_mp=0), seed=3049)
        paid.step(Command(0, "buy", (0,), 1)); paid.step(Command(1, "defend")); paid.step(Command(0, "accept"))
        self.assertEqual([p["status"]["cp"] for p in paid.observe()["board"]["players"]], [4, 6])
        self.assertEqual(next(i["face"] for i in paid.observe(0)["board"]["hand"] if i["token"] == 2), 6)
        game = self.match(config=Config(initial_hand=1, initial_cp=99, initial_mp=0), seed=783)
        game.step(Command(0, "pray")); game.step(Command(1, "discard", (1,)))
        game.step(Command(0, "buy", (0,), 1)); game.step(Command(1, "defend"))
        self.assertEqual(game.observe()["last_purchase"], {"origin": 0, "buyer": 0, "seller": 1, "model": 0, "price": 0, "bought": False, "reason": "empty", "overflow_model": None})
        self.assertEqual(game.observe(1)["board"]["hand"], [])
        game = self.match(config=Config(initial_hand=4, initial_cp=99, initial_mp=99), seed=8468)
        learned = self.token(game, 1, 235)
        game.step(Command(0, "discard", (next(i["token"] for i in game.observe(0)["board"]["hand"] if i["face"] != 5 and game.legal(Command(0, "discard", (i["token"],)))),))); game.step(Command(1, "play", (self.token(game, 1, 235),), 1))
        self.assertTrue(next(i for i in game.observe(1)["board"]["hand"] if i["token"] == learned)["used"])
        game.step(Command(0, "buy", (self.token(game, 0, 5),), 1)); game.step(Command(1, "defend"))
        self.assertTrue(next(i for i in game.observe(1)["board"]["hand"] if i["token"] == learned)["used"])
        game.step(Command(0, "decline"))
        self.assertTrue(next(i for i in game.observe(1)["board"]["hand"] if i["token"] == learned)["used"])

    def test_disease_weapon_damage_and_mild_cure_before_owner_tick(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=0), seed=39)
        self.assertTrue({30, 33, 48, 62, 219, 220} <= set(game.supported_models))
        own = {i["face"]: i["token"] for i in game.observe(0)["board"]["hand"]}
        other = {i["face"]: i["token"] for i in game.observe(1)["board"]["hand"]}
        game.step(Command(0, "play", (own[33],), 1))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["curses"], [])
        game.step(Command(1, "defend"))
        target = game.observe()["board"]["players"][1]["status"]
        self.assertEqual((target["hp"], target["curses"]), (31, ["cold"]))
        game.step(Command(1, "play", (other[199],), 1))
        target = game.observe()["board"]["players"][1]["status"]
        self.assertEqual((target["hp"], target["curses"]), (31, []))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 40)
        self.assertNotIn("last_tick", game.observe()); self.assertNotIn("revival_token", game.observe())

    def test_disease_retained_cold_upgrades_and_heaven_reapplication_falls(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=99), seed=113)
        wind = next(i["token"] for i in game.observe(0)["board"]["hand"] if i["face"] == 219)
        for curse, hp in (("cold", 39), ("fever", 37), ("hell", 32), ("heaven", 37)):
            game.step(Command(0, "play", (wind,), 0))
            own = game.observe()["board"]["players"][0]["status"]
            self.assertEqual((own["curses"], own["hp"]), ([curse], hp))
            self.assertTrue(next(i for i in game.observe(0)["board"]["hand"] if i["token"] == wind)["used"])
            self.assertNotIn("last_tick", game.observe())
            discard = next(i["token"] for i in game.observe(1)["board"]["hand"] if game.legal(Command(1, "discard", (i["token"],))))
            game.step(Command(1, "discard", (discard,)))
        game.step(Command(0, "play", (wind,), 0))
        self.assertTrue(game.status.terminated); self.assertEqual(game.status.winners, (1,))
        own = game.observe()["board"]["players"][0]["status"]
        self.assertEqual((own["hp"], own["mp"], own["curses"]), (0, 69, ["heaven"]))

    def test_disease_heaven_quote_wait_and_cutoff_tick_priority(self):
        rule = "experimental:one-disease-plus-fog-flash-darkcloud-and-dream;reapply-max-incoming-or-next;heaven-reapplication-hp0;owner-only-after-whole-action;natural-and-cutoff-before-tick;fixed5-percent-before-hp;post-upgrade-tick;post-tick-revival-before-gifts"
        game = self.match(config=Config(initial_hand=18, initial_mp=99, initial_cp=99), seed=112)
        own = {i["face"]: i["token"] for i in game.observe(0)["board"]["hand"]}
        self.assertEqual(game.observe()["disease_rule"], rule)
        game.step(Command(0, "play", (own[220],), 0))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 45)
        discard = next(i["token"] for i in game.observe(1)["board"]["hand"] if game.legal(Command(1, "discard", (i["token"],))))
        game.step(Command(1, "discard", (discard,)))
        game.step(Command(0, "buy", (own[5],), 1)); game.step(Command(1, "defend"))
        self.assertEqual(game.status.phase, "purchase")
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 45)
        before = game.observe(0)
        for seed in range(16):
            proposal, _ = game.sample_action(seed); self.assertTrue(game.legal(proposal))
        self.assertEqual(game.observe(0), before)
        game.step(Command(0, "decline"))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 50)
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["curses"], ["heaven"])
        self.assertNotIn("last_tick", game.observe())
        cut = self.match(config=Config(initial_hand=18, initial_mp=99, max_decisions=1), seed=112)
        token = next(i["token"] for i in cut.observe(0)["board"]["hand"] if i["face"] == 220)
        cut.step(Command(0, "play", (token,), 0))
        self.assertTrue(cut.status.truncated); self.assertFalse(cut.status.terminated)
        status = cut.observe()["board"]["players"][0]["status"]
        self.assertEqual((status["hp"], status["curses"]), (40, ["heaven"]))

    def test_self_curse_mask_fully_defends_and_discard_is_inert(self):
        config = Config(initial_hand=4, initial_mp=0)
        game = self.match(config=config, seed=92506)
        self.assertTrue({168, 198} <= set(game.supported_models))
        game.step(Command(0, "play", (self.token(game, 0, 6),), 1))
        before, status = game.observe(1), game.status
        for invalid in (Command(1, "defend", (self.token(game, 1, 168), self.token(game, 1, 21))), Command(1, "defend", (self.token(game, 1, 21), self.token(game, 1, 168)))):
            self.assertFalse(game.legal(invalid))
            with self.assertRaises(InvalidMatchError): game.step(invalid)
            self.assertEqual((game.observe(1), game.status), (before, status))
        for seed in range(32):
            candidate, _ = game.sample_action(seed); self.assertTrue(game.legal(candidate))
        self.assertEqual((game.observe(1), game.status), (before, status))
        mask = self.token(game, 1, 168)
        game.step(Command(1, "defend", (mask,)))
        target = game.observe()["board"]["players"][1]["status"]
        self.assertEqual((target["hp"], target["curses"]), (40, ["fever"]))
        self.assertNotIn(mask, {i["token"] for i in game.observe(1)["board"]["hand"]})
        discarded = self.match(config=config, seed=92506)
        discarded.step(Command(0, "discard", (next(i["token"] for i in discarded.observe(0)["board"]["hand"] if discarded.legal(Command(0,"discard",(i["token"],)))),))); discarded.step(Command(1, "discard", (self.token(discarded, 1, 168),)))
        self.assertEqual([p["status"]["curses"] for p in discarded.observe()["board"]["players"]], [[], []])

    def test_heaven_self_curse_prefix_falls_then_revives_after_full_body(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=99), seed=86483)
        heaven, mask, amulet = (self.token(game, 0, model) for model in (220,168,208))
        club = self.token(game, 1, 6)
        game.step(Command(0, "play", (heaven,), 0))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 45)
        game.step(Command(1, "play", (club,), 0))
        game.step(Command(0, "defend", (mask,)))
        own = game.observe(0); status = own["board"]["players"][0]["status"]
        self.assertEqual((status["hp"], status["curses"]), (10, ["heaven"]))
        self.assertFalse(game.done); self.assertEqual(game.actor, 0)
        self.assertTrue({mask, amulet}.isdisjoint(i["token"] for i in own["board"]["hand"]))
        self.assertEqual(len(own["board"]["hand"]), 18)
        self.assertIsNone(own["pending"]); self.assertNotIn("revivals", own); self.assertNotIn("last_tick", own)

    def test_heaven_herb_boost_order_reflection_and_cutoff(self):
        rule = "experimental:response-self-curse-in-selection-order-after-consumption;prefix-before-body;death-and-revival-after-whole-body;counter-basis-body-target-loss-only;mp-boost-before-disease;no-trigger-on-trade-or-discard"
        game = self.match(config=Config(initial_hand=4, initial_mp=90), seed=7479)
        self.assertEqual(game.observe()["disease_item_rule"], rule)
        game.step(Command(0, "play", (self.token(game, 0, 198),), 1))
        before, status = game.observe(1), game.status
        with self.assertRaises(InvalidMatchError): game.step(Command(1, "defend", (next(i["token"] for i in game.observe(1)["board"]["hand"] if not game.legal(Command(1, "defend", (i["token"],)))),)))
        self.assertEqual((game.observe(1), game.status), (before, status))
        game.step(Command(1, "defend"))
        players = game.observe()["board"]["players"]
        self.assertEqual((players[0]["status"]["mp"], players[1]["status"]["mp"]), (90, 99))
        self.assertEqual((players[1]["status"]["hp"], players[1]["status"]["curses"]), (40, ["heaven"]))
        reflected = self.match(config=Config(initial_hand=4, initial_mp=90, max_decisions=3), seed=7479)
        reflected.step(Command(0, "play", (self.token(reflected, 0, 198),), 1)); reflected.step(Command(1, "defend", (self.token(reflected, 1, 190),))); reflected.step(Command(0, "defend"))
        self.assertTrue(reflected.status.truncated)
        players = reflected.observe()["board"]["players"]
        self.assertEqual((players[0]["status"]["mp"], players[1]["status"]["mp"]), (99, 90))
        self.assertEqual((players[0]["status"]["hp"], players[0]["status"]["curses"]), (40, ["heaven"]))
        discarded = self.match(config=Config(initial_hand=4, initial_mp=90), seed=7479)
        discarded.step(Command(0, "discard", (self.token(discarded, 0, 198),)))
        status = discarded.observe()["board"]["players"][0]["status"]
        self.assertEqual((status["mp"], status["curses"]), (90, []))

    def test_fog_view_mild_cure_and_private_inventory_independent_sell(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=99), seed=487975)
        hand = lambda seat: game.observe(seat)["board"]["hand"]
        own = {i["face"]: i["token"] for i in hand(0)}
        spring = next(i["token"] for i in hand(1) if i["face"] == 235)
        identity = {k: game.observe()["board"]["players"][1][k] for k in ("name", "team")}
        game.step(Command(0, "play", (own[221],), 0))
        covered = game.observe(0)["board"]["players"][1]
        self.assertTrue(covered["fogged"]); self.assertIsNone(covered["status"])
        self.assertEqual({k: covered[k] for k in identity}, identity)
        self.assertIsNotNone(game.observe()["board"]["players"][1]["status"])
        game.step(Command(1, "play", (spring,), 1))
        self.assertIn(235, game.observe(0)["board"]["players"][1]["used_models"])
        before, status = game.observe(0), game.status
        foreign = Command(0, "sell", (own[4], own[6]), 1)
        self.assertTrue(game.legal(foreign))
        self.assertEqual((game.observe(0), game.status), (before, status))
        seen_sell = False
        for seed in range(1024):
            proposal, _ = game.sample_action(seed); self.assertTrue(game.legal(proposal))
            if proposal.kind == "sell": seen_sell = True; self.assertIn(proposal.target, (0, 1))
        self.assertTrue(seen_sell); self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(Command(0, "play", (own[199],), 0))
        shown = game.observe(0)["board"]["players"][1]
        self.assertFalse(shown["fogged"]); self.assertEqual(shown["status"]["hp"], 50)
        self.assertNotIn("last_fog", game.observe(0))

    def test_fog_enemy_reselection_and_false_forced_flag_redaction(self):
        roster = [PlayerSetup(0), PlayerSetup(77), PlayerSetup(88)]
        game = self.match(config=Config(initial_hand=18, initial_mp=99), players=roster, seed=1243)
        own = {i["face"]: i["token"] for i in game.observe(0)["board"]["hand"]}
        game.step(Command(0, "play", (own[221],), 0))
        for actor in (77, 88):
            token = next(i["token"] for i in game.observe(actor)["board"]["hand"] if game.legal(Command(actor, "discard", (i["token"],))))
            game.step(Command(actor, "discard", (token,)))
        before = game.observe(0)
        for seed in range(32):
            proposal, _ = game.sample_action(seed); self.assertTrue(game.legal(proposal))
        self.assertEqual(game.observe(0), before)
        game.step(Command(0, "play", (own[6],), 77))
        self.assertEqual(game.actor, 88)
        self.assertIsNone(game.observe(0)["pending"]["forced_hit"])
        self.assertIs(game.observe()["pending"]["forced_hit"], False)
        self.assertIs(game.observe(88)["pending"]["forced_hit"], False)
        self.assertNotIn("last_fog", game.observe(0))
        game.step(Command(88, "defend"))
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [40, 40, 39])

    def test_fog_forced_hit_stays_redacted_after_reflection_to_self(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=99), seed=6772081)
        own = {i["face"]: i["token"] for i in game.observe(0)["board"]["hand"]}
        mirror = next(i["token"] for i in game.observe(1)["board"]["hand"] if i["face"] == 190)
        def yield_other():
            token = next(i["token"] for i in game.observe(1)["board"]["hand"] if i["token"] != mirror and game.legal(Command(1, "discard", (i["token"],))))
            game.step(Command(1, "discard", (token,)))
        game.step(Command(0, "play", (own[223],), 1)); game.step(Command(1, "defend")); yield_other()
        game.step(Command(0, "play", (own[221],), 0)); yield_other()
        game.step(Command(0, "play", (own[95],)))
        self.assertIsNone(game.observe(0)["pending"]["forced_hit"])
        self.assertIs(game.observe()["pending"]["forced_hit"], True)
        game.step(Command(1, "defend", (mirror,)))
        self.assertEqual((game.actor, game.observe()["pending"]["target"]), (0, 0))
        self.assertIsNone(game.observe(0)["pending"]["forced_hit"])
        self.assertIs(game.observe()["pending"]["forced_hit"], True)
        self.assertIs(game.observe(1)["pending"]["forced_hit"], True)
        self.assertEqual(game.observe()["pending"]["hit_rate"], 75)
        game.step(Command(0, "defend"))
        self.assertEqual(game.observe(0)["board"]["players"][0]["status"]["hp"], 39)

    def test_fog_chance_weapon_and_counter_use_existing_fifo(self):
        config = Config(initial_hand=18, initial_mp=99)
        game = self.match(config=config, seed=2)
        fan = next(i["token"] for i in game.observe(0)["board"]["hand"] if i["face"] == 97)
        game.step(Command(0, "play", (fan,))); game.step(Command(1, "defend"))
        target = game.observe()["board"]["players"][1]["status"]
        self.assertEqual((target["hp"], target["curses"]), (37, ["fog"]))
        self.assertIsNone(game.observe(1)["board"]["players"][0]["status"])
        game = self.match(config=config, seed=1252)
        club = next(i["token"] for i in game.observe(0)["board"]["hand"] if i["face"] == 6)
        ring = next(i["token"] for i in game.observe(1)["board"]["hand"] if i["face"] == 182)
        game.step(Command(0, "play", (club,), 1)); game.step(Command(1, "defend", (ring,)))
        pending = game.observe()["pending"]
        self.assertEqual((pending["counter"], pending["models"], pending["power"], game.actor), (True, [182], 0, 0))
        game.step(Command(0, "defend"))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["curses"], ["fog"])
        self.assertIsNone(game.observe(0)["board"]["players"][1]["status"])

    def test_dying_weapon_main_chance_and_natural_death_priority(self):
        for seed, hit in ((216, True), (71, False)):
            game = self.match(config=Config(initial_hand=18, initial_mp=0), seed=seed)
            token = next(i["token"] for i in game.observe(0)["board"]["hand"] if i["face"] == 112)
            invalid = Command(0, "play", (token,), 1); before = game.observe(0)
            self.assertFalse(game.legal(invalid))
            with self.assertRaises(InvalidMatchError): game.step(invalid)
            self.assertEqual(game.observe(0), before)
            game.step(Command(0, "play", (token,)))
            if hit:
                pending = game.observe()["pending"]
                self.assertEqual((pending["models"], pending["power"], pending["element"], pending["hit_rate"], pending["death"]), ([112], 1, "light", 25, False))
                game.step(Command(1, "defend"))
            else: self.assertIsNone(game.observe()["pending"])
            self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [40, 39 if hit else 40])
            self.assertNotIn(token, {i["token"] for i in game.observe(0)["board"]["hand"]})
        game = self.match(config=Config(initial_hand=18, initial_hp=1, initial_mp=0), seed=1626)
        game.step(Command(0, "play", (self.token(game, 0, 112),))); game.step(Command(1, "defend"))
        self.assertTrue(game.status.terminated); self.assertEqual(game.status.winners, (0,))
        self.assertIsNone(game.observe()["pending"])
        self.assertIn(self.token(game, 1, 112), {i["token"] for i in game.observe(1)["board"]["hand"]})

    def test_posthumous_dark_death_has_dead_source_and_fixed_rewards(self):
        roster = [PlayerSetup(0), PlayerSetup(7), PlayerSetup(9)]
        game = self.match(config=Config(initial_hand=18, initial_hp=5, initial_mp=0), players=roster, seed=5045)
        game.step(Command(0, "play", (self.token(game, 0, 64),), 7)); game.step(Command(7, "defend"))
        pending = game.observe()["pending"]
        self.assertEqual((game.actor, pending["source"], pending["target"], pending["power"], pending["hit_rate"], pending["death"]), (0, 7, 0, 30, 75, True))
        self.assertTrue(game.observe()["board"]["players"][1]["dead"])
        self.assertNotIn(30, {i["token"] for i in game.observe(7)["board"]["hand"]})
        for seed in range(32):
            command, _ = game.sample_action(seed); self.assertTrue(game.legal(command))
        game.step(Command(0, "defend"))
        self.assertTrue(game.status.terminated); self.assertEqual(game.status.winners, (9,))
        self.assertEqual(game.status.rewards, (-1.0, -1.0, 1.0))
        self.assertIsNone(game.observe()["pending"])
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [0, 0, 5])

    def test_posthumous_reflection_to_dead_source_cancels_then_fifo_continues(self):
        roster = [PlayerSetup(0), PlayerSetup(7), PlayerSetup(9)]
        game = self.match(config=Config(initial_hand=18, initial_hp=5, initial_mp=0), players=roster, seed=111506)
        game.step(Command(0, "play", (self.token(game, 0, 64),), 7)); game.step(Command(7, "defend"))
        self.assertTrue(game.observe()["pending"]["death"])
        game.step(Command(0, "defend", (self.token(game, 0, 190),)))
        pending = game.observe()["pending"]
        self.assertEqual((game.actor, pending["source"], pending["target"], pending["power"], pending["death"]), (9, 7, 9, 30, True))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 5)
        self.assertNotIn(3, {i["token"] for i in game.observe(0)["board"]["hand"]})
        game.step(Command(9, "defend"))
        self.assertTrue(game.status.terminated); self.assertEqual(game.status.winners, (0,))
        self.assertEqual(game.status.rewards, (1.0, -1.0, -1.0))

    def test_posthumous_revival_precedes_reservation_and_cutoff_cancels_activation(self):
        roster = [PlayerSetup(0), PlayerSetup(7), PlayerSetup(9)]
        game = self.match(config=Config(initial_hand=18, initial_hp=5, initial_mp=0), players=roster, seed=22085)
        game.step(Command(0, "play", (self.token(game, 0, 64),), 7)); game.step(Command(7, "defend"))
        target = game.observe()["board"]["players"][1]
        self.assertFalse(target["dead"]); self.assertEqual(target["status"]["hp"], 10)
        self.assertIn(19, {i["token"] for i in game.observe(7)["board"]["hand"]})
        self.assertNotIn(18, {i["token"] for i in game.observe(7)["board"]["hand"]})
        self.assertIsNone(game.observe()["pending"]); self.assertEqual(game.actor, 7)
        cut = self.match(config=Config(initial_hand=18, initial_hp=5, initial_mp=0, max_decisions=2), players=roster, seed=5045)
        cut.step(Command(0, "play", (self.token(cut, 0, 64),), 7)); cut.step(Command(7, "defend"))
        self.assertTrue(cut.status.truncated); self.assertFalse(cut.status.terminated)
        self.assertEqual(cut.status.rewards, (0.0, 0.0, 0.0)); self.assertIsNone(cut.observe()["pending"])
        self.assertNotIn(30, {i["token"] for i in cut.observe(7)["board"]["hand"]})
        self.assertEqual([p["status"]["hp"] for p in cut.observe()["board"]["players"]], [5, 0, 5])
        self.assertNotIn("deaths", cut.observe()); self.assertNotIn("death_created", cut.observe())

    def test_full_sale_uniform_eviction_public_model_and_query_purity(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=0, initial_cp=99), seed=395)
        original = game.observe(1)["board"]["hand"]
        command = Command(0, "sell", (0, 15), 1)
        before, status = game.observe(0), game.status
        self.assertTrue(game.legal(command)); targets = set()
        for seed in range(512):
            proposal, _ = game.sample_action(seed); self.assertTrue(game.legal(proposal))
            if proposal.kind == "sell": targets.add(proposal.target)
        self.assertEqual(targets, {0, 1}); self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(command); game.step(Command(1, "defend"))
        expected = {"origin": 0, "source": 0, "target": 1, "model": 153, "price": 10, "overflow_model": 181}
        for viewer in (None, 0, 1): self.assertEqual(game.observe(viewer)["last_sale"], expected)
        after = game.observe(1)["board"]["hand"]
        self.assertEqual({i["token"] for i in original} - {i["token"] for i in after}, {19})
        self.assertEqual(next(i["face"] for i in after if i["token"] == 36), 153)
        self.assertEqual([len(game.observe(seat)["board"]["hand"]) for seat in (0, 1)], [17, 18])
        self.assertEqual([p["status"]["cp"] for p in game.observe()["board"]["players"]], [99, 89])
        self.assertEqual(set(game.observe()["last_sale"]), set(expected))

    def test_full_sale_can_evict_used_miracle_without_replacement_or_recast(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=99), seed=7786)
        game.step(Command(0, "discard", (2,))); game.step(Command(1, "play", (24,), 1))
        self.assertTrue(next(i for i in game.observe(1)["board"]["hand"] if i["token"] == 24)["used"])
        game.step(Command(0, "sell", (12, 5), 1)); game.step(Command(1, "defend"))
        self.assertEqual(game.observe()["last_sale"]["overflow_model"], 235)
        self.assertNotIn(24, {i["token"] for i in game.observe(1)["board"]["hand"]})
        self.assertEqual(game.observe()["board"]["players"][1]["used_models"], [])
        self.assertEqual([len(game.observe(seat)["board"]["hand"]) for seat in (0, 1)], [16, 18])
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 50)
        invalid = Command(1, "play", (24,), 1); before, status = game.observe(1), game.status
        self.assertFalse(game.legal(invalid))
        with self.assertRaises(InvalidMatchError): game.step(invalid)
        self.assertEqual((game.observe(1), game.status), (before, status))

    def test_full_sale_evicts_amulet_before_death_and_does_not_trigger_it(self):
        game = self.match(config=Config(initial_hand=18, initial_hp=5, initial_mp=0, initial_cp=0), seed=83881)
        self.assertEqual(next(i["face"] for i in game.observe(1)["board"]["hand"] if i["token"] == 26), 208)
        game.step(Command(0, "sell", (11, 6), 1)); game.step(Command(1, "defend"))
        self.assertTrue(game.status.terminated); self.assertEqual(game.status.winners, (0,))
        self.assertEqual(game.observe()["last_sale"], {"origin": 0, "source": 0, "target": 1, "model": 38, "price": 5, "overflow_model": 208})
        target = game.observe(1)
        self.assertEqual(target["board"]["players"][1]["status"]["hp"], 0)
        self.assertNotIn(26, {i["token"] for i in target["board"]["hand"]})
        self.assertEqual(next(i["face"] for i in target["board"]["hand"] if i["token"] == 36), 38)
        self.assertEqual(len(game.observe(0)["board"]["hand"]), 16)

    def test_fog_foreign_sale_uses_overflow_and_keeps_covered_status_private(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=99), seed=487975)
        own = {i["face"]: i["token"] for i in game.observe(0)["board"]["hand"]}
        spring = next(i["token"] for i in game.observe(1)["board"]["hand"] if i["face"] == 235)
        game.step(Command(0, "play", (own[221],), 0)); game.step(Command(1, "play", (spring,), 1))
        original = game.observe(1)["board"]["hand"]
        command = Command(0, "sell", (own[4], own[6]), 1)
        before = game.observe(0); self.assertTrue(game.legal(command)); self.assertEqual(game.observe(0), before)
        game.step(command); game.step(Command(1, "defend"))
        event = game.observe(0)["last_sale"]
        self.assertEqual((event["origin"], event["source"], event["target"], event["model"]), (0, 0, 1, 6))
        self.assertIn(event["overflow_model"], {i["face"] for i in original})
        self.assertEqual(game.observe()["last_sale"], event)
        self.assertIsNone(game.observe(0)["board"]["players"][1]["status"])
        self.assertEqual(len(game.observe(1)["board"]["hand"]), 18)
        self.assertNotIn("last_fog", game.observe(0))

    def test_dream_gift_uses_visible_face_then_reveals_actual_attack(self):
        game = self.match(config=Config(initial_hand=17), seed=7614)
        initial = game.observe(0)["board"]["hand"]
        self.assertFalse(any(item["disguised"] for item in initial))
        game.step(Command(0, "play", (15,), 0))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["curses"], ["dream"])
        hand = game.observe(0)["board"]["hand"]
        masked = next(item for item in hand if item["token"] == 34)
        self.assertEqual(masked, {"token": 34, "face": 8, "reference": 8, "disguised": True, "used": False})
        self.assertEqual([item["token"] for item in hand if item["disguised"]], [34])
        self.assertTrue(next(item for item in hand if item["token"] == 15)["used"])
        self.assertEqual(game.observe()["last_reveals"], [])
        game.step(Command(1, "discard", (next(i["token"] for i in game.observe(1)["board"]["hand"] if game.legal(Command(1, "discard", (i["token"],)))),)))
        before, status = game.observe(0), game.status
        attack = Command(0, "play", (34,), 1)
        self.assertTrue(game.legal(attack))
        for policy_seed in range(64):
            proposal, rng = game.sample_action(policy_seed)
            self.assertTrue(game.legal(proposal))
            self.assertEqual((proposal, rng), game.sample_action(policy_seed))
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(attack)
        pending = game.observe()["pending"]
        self.assertEqual((pending["models"], pending["power"]), ([26], 7))
        event = [{"owner": 0, "face": 8, "model": 26}]
        for viewer in (None, 0, 1): self.assertEqual(game.observe(viewer)["last_reveals"], event)
        before, status = game.observe(0), game.status
        with self.assertRaises(InvalidMatchError): game.step(Command(0, "play", (34,), 1))
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(Command(1, "defend"))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 33)
        self.assertEqual(game.observe()["last_reveals"], event)
        token = next(item["token"] for item in game.observe(1)["board"]["hand"] if game.legal(Command(1, "discard", (item["token"],))))
        game.step(Command(1, "discard", (token,)))
        self.assertEqual(game.observe()["last_reveals"], [])
        game.reset(7614); self.assertEqual(game.observe()["last_reveals"], [])

    def test_dream_mild_preserves_and_all_cure_reveals_hand_only_to_owner(self):
        game = self.match(config=Config(initial_hand=17), seed=7614)
        game.step(Command(0, "play", (15,), 0))
        game.step(Command(1, "discard", (next(i["token"] for i in game.observe(1)["board"]["hand"] if game.legal(Command(1, "discard", (i["token"],)))),)))
        game.step(Command(0, "play", (16,), 0))
        self.assertIn("dream", game.observe()["board"]["players"][0]["status"]["curses"])
        masked = next(item for item in game.observe(0)["board"]["hand"] if item["token"] == 34)
        self.assertEqual((masked["face"], masked["reference"], masked["disguised"]), (8, 8, True))
        self.assertEqual(game.observe()["last_reveals"], [])
        token = next(item["token"] for item in game.observe(1)["board"]["hand"] if game.legal(Command(1, "discard", (item["token"],))))
        game.step(Command(1, "discard", (token,)))
        hidden_count = sum(item["disguised"] for item in game.observe(0)["board"]["hand"])
        game.step(Command(0, "play", (4,), 0))
        self.assertNotIn("dream", game.observe()["board"]["players"][0]["status"]["curses"])
        hand = game.observe(0)["board"]["hand"]
        self.assertFalse(any(item["disguised"] for item in hand))
        self.assertEqual(next(item["face"] for item in hand if item["token"] == 34), 26)
        private = game.observe(0)["last_reveals"]
        self.assertEqual(len(private), hidden_count)
        self.assertIn({"owner": 0, "face": 8, "model": 26}, private)
        self.assertTrue(all(set(event) == {"owner", "face", "model"} for event in private))
        self.assertEqual(game.observe()["last_reveals"], [])
        self.assertEqual(game.observe(1)["last_reveals"], [])
        for key in ("dream_rolls", "dream_masks", "fake_models", "initial_state"):
            self.assertNotIn(key, game.observe(0))

    def test_dream_weapon_affects_future_gifts_without_retroactive_masking(self):
        game = self.match(config=Config(initial_hand=18), seed=282)
        game.step(Command(0, "play", (0,), 1))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["curses"], [])
        game.step(Command(1, "defend"))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["curses"], ["dream"])
        self.assertFalse(any(item["disguised"] for item in game.observe(1)["board"]["hand"]))
        game.step(Command(1, "play", (22,), 1))
        item = next(item for item in game.observe(1)["board"]["hand"] if item["token"] == 37)
        self.assertEqual((item["face"], item["reference"], item["disguised"]), (32, 32, True))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 35)
        self.assertFalse(any(item["disguised"] for item in game.observe(0)["board"]["hand"]))

    def test_dream_counter_is_nonattack_and_masks_owner_replacement(self):
        game = self.match(config=Config(initial_hand=18), seed=101329)
        club, filter_item, ring = self.token(game, 0, 6), self.token(game, 1, 189), self.token(game, 1, 183)
        game.step(Command(0, "play", (club,), 1))
        game.step(Command(1, "defend", (filter_item, ring)))
        pending = game.observe()["pending"]
        self.assertEqual((game.actor, pending["counter"], pending["models"], pending["power"]), (0, True, [183], 0))
        game.step(Command(0, "defend"))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["curses"], ["dream"])
        item = next(item for item in game.observe(0)["board"]["hand"] if item["token"] == 36)
        self.assertEqual((item["face"], item["reference"], item["disguised"]), (54, 54, True))
        self.assertEqual(game.observe()["last_reveals"], [])

    def test_redraw_replaces_used_miracle_and_preserves_reserved_gifts(self):
        config = Config(initial_hand=4)
        game = self.match(config=config, seed=703521)
        reference = self.match(config=config, seed=703521)
        for command in (Command(0, "discard", (3,)), Command(1, "play", (7,), 1), Command(0, "play", (1,), 1)):
            self.assertEqual(game.step(command), reference.step(command))
        before = game.observe(1)
        self.assertEqual(len(before["board"]["hand"]), 5)
        self.assertTrue(next(item for item in before["board"]["hand"] if item["token"] == 7)["used"])
        self.assertEqual(before["board"]["players"][1]["used_models"], [235])
        self.assertEqual(before["last_redraws"], [])
        status = game.status
        for policy_seed in range(64):
            proposal, rng = game.sample_action(policy_seed)
            self.assertTrue(game.legal(proposal)); self.assertEqual((proposal, rng), game.sample_action(policy_seed))
        self.assertEqual((game.observe(1), game.status), (before, status))
        self.assertEqual(game.step(Command(1, "defend", (6,))), reference.step(Command(1, "defend", (6,))))
        self.assertEqual(game.observe(1), reference.observe(1))
        hand = game.observe(1)["board"]["hand"]
        self.assertEqual({item["token"] for item in hand}, {9, 10, 11, 12, 14})
        self.assertTrue(all(not item["used"] for item in hand))
        self.assertEqual(game.observe()["board"]["players"][1]["used_models"], [])
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["curses"], ["dream"])
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 50)
        self.assertEqual(len(game.observe(0)["board"]["hand"]), 3)
        for viewer in (None, 0, 1):
            view = game.observe(viewer)
            self.assertEqual(view["last_redraws"], [1]); self.assertEqual(view["last_reveals"], [])
            for private in ("redraws", "redrawn_items", "redraw_models", "redraw_tokens", "rng"):
                self.assertNotIn(private, view)
        self.assertNotIn("items", game.observe(0)["board"]["players"][1])
        self.assertFalse(game.legal(Command(1, "play", (7,), 1)))

    def test_redraw_copies_replace_in_order_and_cutoff_keeps_immediate_draws(self):
        traces = []
        for maximum in (512, 2):
            game = self.match(config=Config(initial_hand=4, max_decisions=maximum), seed=302658)
            game.step(Command(0, "play", (1,), 1))
            before, status = game.observe(1), game.status
            duplicate = Command(1, "defend", (4, 4))
            self.assertFalse(game.legal(duplicate))
            with self.assertRaises(InvalidMatchError): game.step(duplicate)
            self.assertEqual((game.observe(1), game.status), (before, status))
            game.step(Command(1, "defend", (4, 5)))
            view = game.observe(1); hand = view["board"]["hand"]
            self.assertEqual(view["last_redraws"], [1, 1])
            self.assertEqual(view["board"]["players"][1]["status"]["hp"], 40)
            self.assertEqual(view["board"]["players"][1]["status"]["curses"], ["dream"])
            self.assertEqual({item["token"] for item in hand}, {10, 11, 13, 14} if maximum == 512 else {10, 11})
            self.assertTrue(all(not item["used"] for item in hand))
            traces.append([item for item in hand if item["token"] in (10, 11)])
            self.assertEqual((game.status.truncated, game.status.terminated), (maximum == 2, False))
            self.assertEqual(len(game.observe(0)["board"]["hand"]), 4 if maximum == 512 else 3)
            self.assertIsNone(view["pending"])
            if maximum == 2: self.assertEqual(game.status.rewards, (0.0, 0.0))
        self.assertEqual(traces[0], traces[1])

    def test_redraw_armor_manual_role_discard_and_event_lifetime(self):
        game = self.match(config=Config(initial_hand=4), seed=703521)
        game.step(Command(0, "discard", (3,)))
        before, status = game.observe(1), game.status
        bad = Command(1, "play", (6,), 1)
        self.assertFalse(game.legal(bad))
        with self.assertRaises(InvalidMatchError): game.step(bad)
        self.assertEqual((game.observe(1), game.status), (before, status))
        game.step(Command(1, "discard", (6,)))
        self.assertEqual(game.observe()["last_redraws"], [])
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["curses"], [])
        self.assertEqual(len(game.observe(1)["board"]["hand"]), 3)
        game.reset(703521)
        game.step(Command(0, "play", (1,), 1)); game.step(Command(1, "defend", (6,)))
        self.assertEqual(game.observe()["last_redraws"], [1])
        before, status = game.observe(1), game.status
        with self.assertRaises(InvalidMatchError): game.step(Command(0, "pray"))
        self.assertEqual((game.observe(1), game.status), (before, status))
        next_command, _ = game.sample_action(23)
        game.step(next_command)
        self.assertEqual(game.observe()["last_redraws"], [])
        game.reset(703521); self.assertEqual(game.observe()["last_redraws"], [])

    def test_self_buy_cp_zero_preserves_commodity_and_never_offers_purchase(self):
        game = self.match(config=Config(initial_hand=4, initial_mp=0, initial_cp=0), seed=3440)
        initial = game.observe(0)["board"]["hand"]
        operation = self.token(game, 0, 5)
        inspect = Command(0, "buy", (operation,), 0)
        before, status = game.observe(0), game.status
        self.assertTrue(game.legal(inspect)); targets = set()
        for policy_seed in range(128):
            proposal, rng = game.sample_action(policy_seed)
            self.assertTrue(game.legal(proposal)); self.assertEqual((proposal, rng), game.sample_action(policy_seed))
            if proposal.kind == "buy": targets.add(proposal.target)
        self.assertEqual(targets, {0, 1})
        self.assertEqual((game.observe(0), game.status), (before, status))
        result = game.step(inspect)
        expected = {"origin": 0, "buyer": 0, "seller": 0, "model": 195, "price": 1,
                    "bought": False, "reason": "self_inspection", "overflow_model": None}
        for viewer in (None, 0, 1):
            view = game.observe(viewer)
            self.assertEqual(view["last_purchase"], expected)
            self.assertIsNone(view["purchase"]); self.assertIsNone(view["pending"])
        self.assertEqual((result.phase, result.actor, result.decisions), ("main", 1, 1))
        hand = game.observe(0)["board"]["hand"]
        self.assertEqual({item["token"] for item in hand}, {0, 1, 3, 8})
        selected = next(item for item in initial if item["token"] == 3)
        self.assertEqual(next(item for item in hand if item["token"] == 3), selected)
        self.assertFalse(selected["used"])
        self.assertEqual([p["status"]["cp"] for p in game.observe()["board"]["players"]], [0, 0])
        before, status = game.observe(0), game.status
        for kind in ("accept", "decline"):
            command = Command(1, kind)
            self.assertFalse(game.legal(command))
            with self.assertRaises(InvalidMatchError): game.step(command)
            self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(Command(1, "discard", (4,)))
        self.assertIsNone(game.observe()["last_purchase"])

    def test_self_buy_empty_and_cutoff_keep_resolved_inspection_without_transfer(self):
        config = Config(initial_hand=1, initial_mp=0, initial_cp=0)
        game = self.match(config=config, seed=3049)
        control = self.match(config=config, seed=3049)
        game.step(Command(0, "buy", (0,), 0)); control.step(Command(0, "pray"))
        self.assertEqual(game.observe()["last_purchase"], {"origin": 0, "buyer": 0, "seller": 0,
                         "model": 0, "price": 0, "bought": False, "reason": "empty", "overflow_model": None})
        # Empty self inspection consumes no selection RNG; both actions draw the same normal gift.
        gift = lambda value: next(item for item in value.observe(0)["board"]["hand"] if item["token"] == 2)
        self.assertEqual(gift(game), gift(control)); self.assertEqual(len(game.observe(0)["board"]["hand"]), 1)
        for seed, hand_count, expected_reason in ((3049, 1, "empty"), (3440, 4, "self_inspection")):
            cut = self.match(config=Config(initial_hand=hand_count, initial_mp=0, initial_cp=0, max_decisions=1), seed=seed)
            original = cut.observe(0)["board"]["hand"]
            operation = self.token(cut, 0, 5)
            cut.step(Command(0, "buy", (operation,), 0))
            self.assertTrue(cut.status.truncated); self.assertFalse(cut.status.terminated)
            self.assertEqual(cut.status.rewards, (0.0, 0.0))
            self.assertEqual(cut.observe()["last_purchase"]["reason"], expected_reason)
            self.assertIsNone(cut.observe()["purchase"])
            self.assertEqual(cut.observe(0)["board"]["hand"], [item for item in original if item["token"] != operation])
            if hand_count == 4:
                self.assertEqual((cut.observe()["last_purchase"]["model"], cut.observe()["last_purchase"]["price"]), (195, 1))

    def test_self_buy_excludes_retained_miracle_and_keeps_the_only_unused_copy(self):
        game = self.match(config=Config(initial_hand=2, initial_cp=0), seed=11929)
        game.step(Command(0, "play", (0,), 0))
        retained = next(item for item in game.observe(0)["board"]["hand"] if item["token"] == 0)
        selected = next(item for item in game.observe(0)["board"]["hand"] if item["token"] == 4)
        self.assertTrue(retained["used"]); self.assertEqual(selected["face"], 87)
        game.step(Command(1, "discard", (2,)))
        mp_before = game.observe()["board"]["players"][0]["status"]["mp"]
        game.step(Command(0, "buy", (1,), 0))
        self.assertEqual(game.observe()["last_purchase"], {"origin": 0, "buyer": 0, "seller": 0,
                         "model": 87, "price": 15, "bought": False, "reason": "self_inspection", "overflow_model": None})
        hand = game.observe(0)["board"]["hand"]
        self.assertEqual(next(item for item in hand if item["token"] == 0), retained)
        self.assertEqual(next(item for item in hand if item["token"] == 4), selected)
        self.assertEqual({item["token"] for item in hand}, {0, 4, 5})
        self.assertEqual(game.observe()["board"]["players"][0]["used_models"], [235])
        self.assertEqual((game.observe()["board"]["players"][0]["status"]["mp"], game.observe()["board"]["players"][0]["status"]["cp"]), (mp_before, 0))
        self.assertIsNone(game.observe()["purchase"])

    def test_self_buy_reveals_actual_masked_copy_without_moving_it_even_at_cutoff(self):
        for maximum in (512, 3):
            game = self.match(config=Config(initial_hand=17, initial_cp=0, max_decisions=maximum), seed=16297)
            game.step(Command(0, "play", (9,), 0)); game.step(Command(1, "discard", (18,)))
            before = game.observe(0)["board"]["hand"]
            selected = next(item for item in before if item["token"] == 34)
            self.assertEqual((selected["face"], selected["reference"], selected["disguised"]), (88, 88, True))
            command = Command(0, "buy", (14,), 0)
            before_view, status = game.observe(0), game.status
            self.assertTrue(game.legal(command)); self.assertEqual((game.observe(0), game.status), (before_view, status))
            game.step(command)
            expected = {"origin": 0, "buyer": 0, "seller": 0, "model": 84, "price": 10,
                        "bought": False, "reason": "self_inspection", "overflow_model": None}
            revealed = [{"owner": 0, "face": 88, "model": 84}]
            for viewer in (None, 0, 1):
                view = game.observe(viewer)
                self.assertEqual(view["last_purchase"], expected); self.assertEqual(view["last_reveals"], revealed)
                self.assertIsNone(view["purchase"]); self.assertIsNone(view["pending"])
            hand = game.observe(0)["board"]["hand"]
            self.assertEqual(next(item for item in hand if item["token"] == 34),
                             {"token": 34, "face": 84, "reference": 84, "disguised": False, "used": False})
            unchanged = {item["token"]: item for item in before if item["token"] not in (14, 34)}
            self.assertEqual({item["token"]: item for item in hand if item["token"] in unchanged}, unchanged)
            self.assertEqual(len(hand), 18 if maximum == 512 else 17)
            self.assertEqual(35 in {item["token"] for item in hand}, maximum == 512)
            self.assertEqual(game.observe()["board"]["players"][0]["status"]["cp"], 0)
            self.assertEqual((game.status.truncated, game.status.terminated), (maximum == 3, False))

    def test_guardian_scene_import_and_all_42_explicit_models(self):
        game = self.match(config=Config(initial_hand=0, max_decisions=64))
        self.assertEqual(game.supported_guardian_models, tuple(range(245, 287)))
        self.assertTrue(set(game.supported_guardian_models).isdisjoint(game.supported_models))
        data = OfficialData(LIBRARY)
        for model in game.supported_guardian_models:
            with self.subTest(model=model):
                card = data.get_card(model)
                raw = {"gf": 31, "players": [
                    {"id": 0, "name": "owner", "hp": 99, "mp": 99, "cp": 10},
                    {"id": 7, "name": "guardian source", "hp": 99, "mp": 99, "cp": 10, "guardian": card.guardian},
                    {"id": 9, "name": "third", "hp": 99, "mp": 99, "cp": 10}]}
                with OfficialState(raw, library=LIBRARY) as scene:
                    loaded = game.load_scene(scene, 0, seed=model)
                self.assertEqual((loaded.actor, loaded.owner, loaded.decisions), (0, 0, 0))
                self.assertIsNone(game.observe()["last_guardian"])
                before = game.observe(0)
                proposal, _ = game.sample_action(model)
                self.assertIsInstance(proposal, Command)
                self.assertEqual(game.observe(0), before)
                action = GuardianAction(7, model, None if model == 280 else 0)
                self.assertTrue(game.guardian_legal(action)); self.assertEqual(game.observe(0), before)
                game.guardian_step(action)
                pending = game.observe()["pending"]
                if pending is not None:
                    self.assertEqual((pending["guardian"], pending["source"], pending["models"]), (card.guardian, 7, [model]))
                    self.assertEqual(game.actor, 0)
                    game.step(Command(0, "defend"))
                self.assertEqual(game.status.phase, "main")
                self.assertEqual(game.observe()["board"]["gf"], 31)
                self.assertEqual(game.observe()["last_guardian"], {"source": 7, "model": model,
                                 "guardian": card.guardian, "target": action.target})
                self.assertEqual(game.observe()["board"]["players"][1]["status"]["guardian"], card.guardian)
                self.assertEqual(game.observe(7)["board"]["hand"], [])

    def test_scene_import_deep_copy_scope_rejections_and_library_ownership(self):
        game = self.match(config=Config(initial_hand=0, max_decisions=33))
        raw = {"gf": 13, "players": [
            {"id": 0, "name": "scene\0source", "hp": 40, "mp": 10, "cp": 20, "guardian": "mars", "items": [{"id": 100, "modelId": 6}]},
            {"id": 7, "name": "other", "hp": 40, "mp": 10, "cp": 20}]}
        scene = OfficialState(raw, library=LIBRARY)
        raw["players"][0]["name"] = "changed after construction"
        game.load_scene(scene, 7, seed=91); scene.close()
        self.assertEqual((game.status.owner, game.status.actor, game.status.player_ids), (7, 7, (0, 7)))
        self.assertEqual(game.observe()["board"]["players"][0]["name"], "scene\0source")
        self.assertEqual(game.observe(0)["board"]["hand"][0]["token"], 100)
        before, status = game.observe(0), game.status
        for value in (scene, {}, None):
            with self.assertRaises(InvalidMatchError): game.load_scene(value, 0)
            self.assertEqual((game.observe(0), game.status), (before, status))
        base = {"players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20}, {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
        bad_scenes = []
        unsupported = copy.deepcopy(base); unsupported["players"][0]["items"] = [{"id": 0, "modelId": 1}]; bad_scenes.append(unsupported)
        oversized = copy.deepcopy(base); oversized["players"][1]["items"] = [{"id": i, "modelId": 6} for i in range(19)]; bad_scenes.append(oversized)
        tied = copy.deepcopy(base); tied["tiebreakGF"] = -1; bad_scenes.append(tied)
        active = copy.deepcopy(base); active["attacks"] = [{"playerId": 0, "itemModelIds": [6], "targetPlayerId": 7}]; bad_scenes.append(active)
        for raw_scene in bad_scenes:
            with OfficialState(raw_scene, library=LIBRARY) as invalid_scene:
                with self.assertRaises(MatchError): game.load_scene(invalid_scene, 0)
            self.assertEqual((game.observe(0), game.status), (before, status))
        with OfficialState(base, library=LIBRARY) as valid_scene:
            for owner, seed in ((999, 1), (True, 1), (0, True), (0, 2**64)):
                with self.assertRaises(InvalidMatchError): game.load_scene(valid_scene, owner, seed)
                self.assertEqual((game.observe(0), game.status), (before, status))
        library = Path(LIBRARY) if LIBRARY is not None else Path(__file__).resolve().parents[1] / "build" / ("libgodfield.dylib" if sys.platform == "darwin" else "libgodfield.so")
        with tempfile.TemporaryDirectory(prefix="godfield-foreign-state-") as folder:
            foreign_library = Path(folder) / library.name
            shutil.copy2(library, foreign_library)
            with OfficialState(base, library=foreign_library) as foreign:
                with self.assertRaises(InvalidMatchError): game.load_scene(foreign, 0)
            self.assertEqual((game.observe(0), game.status), (before, status))
        reset = game.reset(16)
        self.assertEqual(reset.player_ids, (0, 7))
        self.assertEqual(game.observe()["board"]["players"][0]["name"], "scene\0source")
        self.assertEqual(game.observe(0)["board"]["hand"], [])

    def test_guardian_response_reflection_preserves_owner_and_skips_owner_tick(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"gf": 13, "players": [
            {"id": 0, "hp": 40, "mp": 10, "cp": 20, "curses": ["cold"], "items": [{"id": 0, "modelId": 190}]},
            {"id": 7, "hp": 40, "mp": 10, "cp": 20, "guardian": "jupiter"}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, seed=534)
        action = GuardianAction(7, 256, 0)
        game.guardian_step(action)
        pending = game.observe()["pending"]
        self.assertEqual((game.actor, game.status.owner, pending["source"], pending["guardian"]), (0, 0, 7, "jupiter"))
        before, status = game.observe(0), game.status
        self.assertFalse(game.guardian_legal(action))
        with self.assertRaises(InvalidMatchError): game.guardian_step(action)
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(Command(0, "defend", (0,)))
        pending = game.observe()["pending"]
        self.assertEqual((game.actor, pending["source"], pending["target"], pending["guardian"]), (7, 0, 7, "jupiter"))
        game.step(Command(7, "defend"))
        self.assertEqual((game.actor, game.status.owner), (0, 0))
        view = game.observe()
        self.assertEqual(view["board"]["gf"], 13)
        self.assertEqual([p["status"]["hp"] for p in view["board"]["players"]], [40, 38])
        self.assertEqual(view["board"]["players"][0]["status"]["curses"], ["cold"])
        self.assertEqual(view["last_guardian"], {"source": 7, "model": 256, "guardian": "jupiter", "target": 0})
        self.assertEqual(len(game.observe(0)["board"]["hand"]), 1)
        self.assertEqual(game.observe(7)["board"]["hand"], [])
        proposal, _ = game.sample_action(1); game.step(proposal)
        self.assertIsNone(game.observe()["last_guardian"])

    def test_guardian_global_cp_shape_strict_types_and_no_autonomous_activation(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"gf": 7, "players": [
            {"id": 0, "hp": 40, "mp": 10, "cp": 98, "curses": ["cold"]},
            {"id": 7, "hp": 40, "mp": 10, "cp": 99, "guardian": "venus"},
            {"id": 9, "hp": 40, "mp": 10, "cp": 0}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0)
        before, status = game.observe(0), game.status
        for action in (GuardianAction(7, 280, 0), GuardianAction(0, 280), GuardianAction(7, 283), GuardianAction(7, 245, 0)):
            self.assertFalse(game.guardian_legal(action))
            with self.assertRaises(InvalidMatchError): game.guardian_step(action)
            self.assertEqual((game.observe(0), game.status), (before, status))
        unsupported = GuardianAction(7, 201, 0)
        self.assertFalse(game.guardian_legal(unsupported))
        with self.assertRaises(UnsupportedMatchError): game.guardian_step(unsupported)
        for malformed in (None, Command(0, "pray"), GuardianAction(True, 280), GuardianAction(7, True), GuardianAction(7, 65536), GuardianAction(7, 280, True)):
            with self.assertRaises(InvalidMatchError): game.guardian_legal(malformed)
            with self.assertRaises(InvalidMatchError): game.guardian_step(malformed)
            self.assertEqual((game.observe(0), game.status), (before, status))
        game.guardian_step(GuardianAction(7, 280))
        view = game.observe()
        self.assertEqual([p["status"]["cp"] for p in view["board"]["players"]], [99, 99, 1])
        self.assertEqual((game.actor, game.status.owner, view["board"]["gf"]), (0, 0, 7))
        self.assertEqual(view["board"]["players"][0]["status"]["hp"], 40)
        self.assertIsNone(view["pending"])
        self.assertEqual(view["last_guardian"]["target"], None)
        # Only ordinary commands are sampled. An ordinary accepted main clears the event.
        proposal, _ = game.sample_action(0); self.assertEqual(proposal, Command(0, "pray"))
        game.step(proposal); self.assertIsNone(game.observe()["last_guardian"])
        self.assertEqual(game.observe()["board"]["gf"], 8)
        game.close()
        with self.assertRaises(InvalidMatchError): game.guardian_legal(GuardianAction(7, 280))
        with self.assertRaises(InvalidMatchError): game.guardian_step(GuardianAction(7, 280))

    def test_guardian_summon_all_ten_resummon_and_retained_release(self):
        game = self.match(config=Config(initial_hand=0, max_decisions=1))
        seen = set()
        for seed in range(128):
            raw = {"players": [{"id": 0, "hp": 40, "mp": 15, "cp": 20, "guardian": "mars",
                                "items": [{"id": 10, "modelId": 201}]}, {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, seed)
            before = game.observe(0)
            self.assertTrue(game.legal(Command(0, "play", (10,), 0)))
            self.assertEqual(game.observe(0), before)
            game.step(Command(0, "play", (10,), 0))
            event = game.observe()["last_guardian_changes"]
            self.assertEqual(len(event), 1)
            self.assertEqual({k: event[0][k] for k in ("player", "before", "reason")},
                             {"player": 0, "before": "mars", "reason": "summon"})
            seen.add(event[0]["after"])
            self.assertEqual(game.observe(0)["board"]["hand"], [])
            self.assertTrue(game.status.truncated)
            if len(seen) == 10: break
        self.assertEqual(seen, {"mars", "mercury", "jupiter", "saturn", "uranus", "pluto", "neptune", "venus", "earth", "moon"})
        for free in (False, True):
            raw["players"][0].update(mp=0 if free else 15, items=[{"id": 10, "modelId": 239}] +
                                    ([{"id": 11, "modelId": 204}] if free else []))
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 2)
            game.step(Command(0, "play", (10, 11) if free else (10,), 0))
            self.assertEqual(game.observe(0)["board"]["hand"],
                             [{"token": 10, "face": 239, "reference": 239, "disguised": False, "used": True}])
            self.assertEqual(game.observe()["board"]["players"][0]["status"]["mp"], 0)
            self.assertEqual(game.observe()["last_guardian_changes"][0]["after"], "mars")

    def test_guardian_auto_response_keeps_owner_tick_and_deferred_gift(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"gf": 7, "players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "curses": ["cold"]},
                                     {"id": 7, "hp": 40, "mp": 10, "cp": 20, "guardian": "moon"}]}
        # The first roll is the disease upgrade check; seed0 then activates Moon.
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.step(Command(0, "pray"))
        view = game.observe(0)
        action = {"source": 7, "model": 286, "guardian": "moon", "target": 0}
        self.assertEqual(view["last_auto_guardians"], [action]); self.assertEqual(view["last_guardian"], action)
        self.assertEqual((game.status.decisions, game.status.owner, game.actor, view["board"]["gf"]), (1, 0, 0, 7))
        self.assertEqual(view["board"]["hand"], [])
        self.assertEqual(view["board"]["players"][0]["status"]["hp"], 39)
        self.assertEqual((view["pending"]["models"], view["pending"]["power"]), ([286], 10))
        before, status = game.observe(0), game.status
        with self.assertRaises(InvalidMatchError): game.step(Command(7, "defend"))
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(Command(0, "defend"))
        view = game.observe(0)
        self.assertEqual((game.status.decisions, game.status.owner, game.actor, view["board"]["gf"]), (2, 7, 7, 8))
        self.assertEqual(view["board"]["players"][0]["status"]["hp"], 29)
        self.assertEqual(len(view["board"]["hand"]), 1)
        self.assertEqual(view["last_auto_guardians"], [action])
        for key in ("guardian_summons", "guardian_opportunities", "auto_guardian_actions", "guardian_departure_rolls", "guardian_departures"):
            self.assertNotIn(key, view)

    def test_guardian_departure_event_filters_event_time_fog(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "curses": ["fog"],
                             "items": [{"id": 10, "modelId": 6}]},
                            {"id": 7, "hp": 40, "mp": 10, "cp": 20, "guardian": "mars"}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.step(Command(0, "play", (10,), 7))
        self.assertEqual(game.observe()["last_guardian_changes"], [])
        game.step(Command(7, "defend"))
        event = [{"player": 7, "before": "mars", "after": None, "reason": "departure"}]
        self.assertEqual(game.observe()["last_guardian_changes"], event)
        self.assertEqual(game.observe(7)["last_guardian_changes"], event)
        self.assertEqual(game.observe(0)["last_guardian_changes"], [])
        self.assertIsNone(game.observe(0)["board"]["players"][1]["status"])
        self.assertEqual(game.observe()["last_auto_guardians"], [])
        before, status = game.observe(), game.status
        with self.assertRaises(InvalidMatchError): game.step(Command(0, "pray"))
        self.assertEqual((game.observe(), game.status), (before, status))

    def test_guardian_external_cp_debt_does_not_depart_or_auto_follow(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"gf": 9, "players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "guardian": "venus"},
                                     {"id": 7, "hp": 40, "mp": 0, "cp": 0, "guardian": "mars"}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.guardian_step(GuardianAction(0, 282, 7)); game.step(Command(7, "defend"))
        view = game.observe()
        self.assertEqual((game.status.owner, game.actor, view["board"]["gf"]), (0, 0, 9))
        self.assertEqual(view["board"]["players"][1]["status"]["hp"], 37)
        self.assertEqual(view["board"]["players"][1]["status"]["guardian"], "mars")
        self.assertEqual(view["last_guardian_changes"], []); self.assertEqual(view["last_auto_guardians"], [])

    def test_guardian_payload_descriptor_strict_shapes_and_atomic_rejection(self):
        game = self.match(config=Config(initial_hand=0))
        self.assertEqual({name: len(models) for name, models in game.supported_guardian_payload_models.items()},
                         {"earth": 120, "moon": 26})
        self.assertIn(6, game.supported_guardian_payload_models["earth"])
        self.assertNotIn(3, game.supported_guardian_payload_models["earth"])
        self.assertNotIn(94, game.supported_guardian_payload_models["earth"])
        self.assertIn(235, game.supported_guardian_payload_models["moon"])
        self.assertTrue({231, 232, 233, 234}.isdisjoint(game.supported_guardian_payload_models["moon"]))
        raw = {"players": [{"id": 0, "hp": 40, "mp": 0, "cp": 20},
                             {"id": 7, "hp": 40, "mp": 0, "cp": 20, "guardian": "earth"}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0)
        before, status = game.observe(7), game.status
        for malformed in (None, GuardianAction(7, 286, 0), GuardianPayloadAction(True, "earth", (6,), 0),
                          GuardianPayloadAction(7, "EARTH", (6,), 0), GuardianPayloadAction(7, 9, (6,), 0),
                          GuardianPayloadAction(7, "earth", [] , 0), GuardianPayloadAction(7, "earth", (), 0),
                          GuardianPayloadAction(7, "earth", (6,) * 19, 0), GuardianPayloadAction(7, "earth", (True,), 0),
                          GuardianPayloadAction(7, "earth", (65536,), 0), GuardianPayloadAction(7, "earth", (6,), False)):
            with self.assertRaises(InvalidMatchError): game.guardian_payload_legal(malformed)
            with self.assertRaises(InvalidMatchError): game.guardian_payload_step(malformed)
        for rejected in (GuardianPayloadAction(7, "moon", (235,), 7), GuardianPayloadAction(7, "earth", (6,)),
                         GuardianPayloadAction(7, "earth", (3,), 0), GuardianPayloadAction(7, "earth", (94,), 0),
                         GuardianPayloadAction(7, "earth", (6, 231), 0), GuardianPayloadAction(7, "earth", (14, 232), 0)):
            self.assertFalse(game.guardian_payload_legal(rejected))
            with self.assertRaises(MatchError): game.guardian_payload_step(rejected)
        self.assertEqual((game.observe(7), game.status), (before, status))

    def test_guardian_payload_earth_mana_composition_and_reflection_keep_launch_values(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"gf": 12, "players": [{"id": 0, "hp": 99, "mp": 4, "cp": 20, "curses": ["cold"],
                                       "items": [{"id": 10, "modelId": 190}]},
                                      {"id": 7, "hp": 99, "mp": 10, "cp": 20, "guardian": "earth"}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        action = GuardianPayloadAction(7, "earth", (58, 203, 83), 0)
        self.assertTrue(game.guardian_payload_legal(action)); game.guardian_payload_step(action)
        view = game.observe()
        expected = {"source": 7, "guardian": "earth", "models": [58, 203, 83], "target": 0}
        self.assertEqual(view["last_guardian_payload"], expected); self.assertIsNone(view["last_guardian"])
        self.assertEqual((view["pending"]["source"], view["pending"]["origin"], view["pending"]["power"], view["pending"]["element"], view["pending"]["guardian"]),
                         (7, 7, 35, "water", "earth"))
        self.assertEqual((game.status.owner, game.actor, game.status.decisions), (0, 0, 1))
        self.assertEqual(game.observe(7)["board"]["hand"], [])
        self.assertEqual(view["board"]["players"][1]["status"]["mp"], 0)
        game.step(Command(0, "defend", (10,)))
        pending = game.observe()["pending"]
        self.assertEqual((pending["source"], pending["target"], pending["power"], pending["guardian"]), (0, 7, 35, "earth"))
        game.step(Command(7, "defend"))
        view = game.observe()
        self.assertEqual((game.status.owner, game.actor, view["board"]["gf"]), (0, 0, 12))
        self.assertEqual([p["status"]["hp"] for p in view["board"]["players"]], [99, 64])
        self.assertEqual(view["last_guardian_payload"], expected)
        self.assertEqual(len(game.observe(0)["board"]["hand"]), 1)
        self.assertEqual(game.observe(7)["board"]["hand"], [])

    def test_guardian_payload_moon_has_miracle_reactions_without_cost_or_retention(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"gf": 3, "players": [{"id": 0, "hp": 40, "mp": 0, "cp": 20, "items": [{"id": 10, "modelId": 171}]},
                                     {"id": 7, "hp": 40, "mp": 0, "cp": 20, "guardian": "moon"}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        cast = GuardianPayloadAction(7, "moon", (235,), 0)
        game.guardian_payload_step(cast)
        self.assertEqual((game.observe()["pending"]["category"], game.observe()["pending"]["guardian"]), ("miracles", "moon"))
        self.assertEqual(game.observe(7)["board"]["hand"], [])
        game.step(Command(0, "defend", (10,)))
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [40, 40])
        self.assertEqual(game.observe(7)["board"]["hand"], [])
        self.assertEqual(game.observe()["board"]["players"][1]["used_models"], [])
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.guardian_payload_step(GuardianPayloadAction(7, "moon", (235,), 7))
        self.assertEqual((game.actor, game.status.owner, game.observe()["board"]["gf"]), (0, 0, 3))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 50)
        self.assertEqual(game.observe(7)["board"]["hand"], [])
        self.assertEqual(game.observe()["last_auto_guardians"], [])

    def test_guardian_payload_all_actual_client_projection_vectors(self):
        # These are supplied model columns, not observations of a selector distribution.
        report = json.loads((Path(__file__).resolve().parents[1] /
                             "data/official-v2/guardian-payload-client-v33.json").read_text())
        game = self.match(config=Config(initial_hand=0, initial_hp=99, initial_mp=99, initial_cp=99))
        contract = report["local_adapter_contract"]
        for guardian in ("earth", "moon"):
            self.assertEqual(game.supported_guardian_payload_models[guardian], tuple(contract[guardian + "_base_ids"]))
        accepted = 0
        self.assertEqual(len(report["cases"]), 173)
        for index, case in enumerate(report["cases"]):
            with self.subTest(index=index, label=case["label"], models=case["models"]):
                projection = case["projection"]
                raw = {"players": [{"id": 17, "hp": 99, "mp": projection["source_mp_before"], "cp": 99,
                                      "guardian": case["guardian"]},
                                     {"id": 23, "hp": 99, "mp": 99, "cp": 99, "curses": ["darkcloud"]}]}
                with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 17, 0)
                action = GuardianPayloadAction(17, case["guardian"], tuple(case["models"]), 23)
                before, status = game.observe(17), game.status
                try: legal = game.guardian_payload_legal(action)
                except InvalidMatchError: legal = False  # Includes the19-model transport rejection.
                self.assertEqual(legal, case["validation"]["adapter_accepted"])
                self.assertEqual((game.observe(17), game.status), (before, status))
                if not legal:
                    with self.assertRaises(MatchError): game.guardian_payload_step(action)
                    self.assertEqual((game.observe(17), game.status), (before, status))
                    continue
                accepted += 1
                game.guardian_payload_step(action)
                view = game.observe()
                self.assertEqual((game.status.phase, game.actor), ("response", 23))
                pending = view["pending"]
                # The client omits the attack-summary widget for nonattack payloads.
                expected_rate = projection["summary"][1] if projection["summary"] is not None else 0
                self.assertEqual((pending["models"], pending["category"], pending["element"], pending["power"],
                                  pending["hit_rate"], pending["guardian"]),
                                 (projection["received_model_ids"], projection["effective_category"], projection["element"],
                                  projection["power"], expected_rate, case["guardian"]))
                self.assertEqual(view["board"]["players"][0]["status"]["mp"], projection["source_mp_after"])
        self.assertEqual(accepted, sum(case["validation"]["adapter_accepted"] for case in report["cases"]))

    def test_guardian_payload_repeat_cutoff_and_late_failure_rollback(self):
        game = self.match(config=Config(initial_hand=0, max_decisions=2))
        raw = {"players": [{"id": 0, "hp": 40, "mp": 0, "cp": 20},
                             {"id": 7, "hp": 40, "mp": 0, "cp": 20, "guardian": "earth"}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.guardian_payload_step(GuardianPayloadAction(7, "earth", (14,), 0))
        game.step(Command(0, "defend"))
        self.assertTrue(game.status.truncated); self.assertIsNone(game.observe()["pending"])
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 37)
        self.assertEqual(game.observe(7)["board"]["hand"], [])
        raw["updateCount"] = 9007199254740991
        raw["players"][1]["guardian"] = "moon"
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        before, status = game.observe(7), game.status
        action = GuardianPayloadAction(7, "moon", (235,), 7)
        self.assertTrue(game.guardian_payload_legal(action))
        with self.assertRaises(InvalidMatchError): game.guardian_payload_step(action)
        self.assertEqual((game.observe(7), game.status), (before, status))

    def test_phenomenon_direct_fever_fog_and_hp_assignment_all_alive(self):
        game = self.match(config=Config(initial_hand=0))
        ids = (0, 7, 23, 91, 105, 888, 9007199254740991, 42, 3)
        raw = {"gf": 12, "players": [{"id": player, "team": 1 if index % 2 == 0 else 2,
                 "hp": 0 if index == 8 else 11 + index, "mp": 20 + index, "cp": 30 + index,
                 "dead": index == 8, "guardian": "mars", "curses": ["heaven"] if index == 0 else ["cold"],
                 "items": [{"id": 100 + index * 2, "modelId": 208}, {"id": 101 + index * 2, "modelId": 112}]}
                 for index, player in enumerate(ids)]}
        raw["players"][0]["curses"] += ["dream", "flash", "darkcloud"]
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 7)
        before = game.observe()["board"]["players"]
        hands = [game.observe(player)["board"]["hand"] for player in ids]
        game.phenomenon_step(PhenomenonAction(287))
        after = game.observe()["board"]["players"]
        for index, (old, new) in enumerate(zip(before, after)):
            self.assertEqual({key: old["status"][key] for key in ("hp", "mp", "cp", "guardian")},
                             {key: new["status"][key] for key in ("hp", "mp", "cp", "guardian")})
            self.assertEqual(new["status"]["curses"], old["status"]["curses"] if index == 8 else
                             (["fever", "dream", "flash", "darkcloud"] if index == 0 else ["fever"]))
        self.assertEqual([game.observe(player)["board"]["hand"] for player in ids], hands)
        self.assertEqual(game.observe()["last_phenomenon"], {"model": 287, "recipient": None})
        game.phenomenon_step(PhenomenonAction(288))
        fogged = game.observe()["board"]
        game.phenomenon_step(PhenomenonAction(288))
        self.assertEqual(game.observe()["board"], fogged)
        self.assertNotIn("fog", fogged["players"][8]["status"]["curses"])
        self.assertTrue(all("fog" in player["status"]["curses"] for player in fogged["players"][:8]))
        game.phenomenon_step(PhenomenonAction(290))
        view = game.observe()
        self.assertEqual([player["status"]["hp"] for player in view["board"]["players"]], [1] * 8 + [0])
        self.assertEqual([game.observe(player)["board"]["hand"] for player in ids], hands)
        self.assertEqual((game.actor, game.status.owner, view["board"]["gf"]), (0, 0, 12))
        self.assertEqual(view["last_auto_guardians"], []); self.assertEqual(view["last_guardian_changes"], [])

    def test_phenomenon_cp_collection_uses_original_alive_sum_before_credit(self):
        game = self.match(config=Config(initial_hand=0))
        ids = (0, 23, 9007199254740991, 91)
        for balances in ((3, 5, 7), (99, 99, 99), (0, 0, 0)):
            for recipient in ids[:3]:
                raw = {"gf": 8, "players": [{"id": player, "hp": 0 if index == 3 else 40,
                    "mp": 21 + index, "cp": 99 if index == 3 else balances[index], "dead": index == 3}
                    for index, player in enumerate(ids)]}
                with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 2)
                game.phenomenon_step(PhenomenonAction(294, recipient))
                view = game.observe()
                self.assertEqual([player["status"]["cp"] for player in view["board"]["players"]],
                                 [min(99, sum(balances)) if player == recipient else (99 if index == 3 else 0)
                                  for index, player in enumerate(ids)])
                self.assertEqual([player["status"]["mp"] for player in view["board"]["players"]], [21, 22, 23, 24])
                self.assertEqual([player["status"]["hp"] for player in view["board"]["players"]], [40, 40, 40, 0])
                self.assertEqual(view["last_phenomenon"], {"model": 294, "recipient": recipient})
                self.assertEqual((game.actor, game.status.owner, view["board"]["gf"]), (0, 0, 8))
                self.assertTrue(all(game.observe(player)["board"]["hand"] == [] for player in ids))

    def test_phenomenon_guardian_assignments_follow_alive_order_and_filter_fog(self):
        game = self.match(config=Config(initial_hand=0))
        ids = (0, 7, 23, 91, 105, 888, 9007199254740991, 42, 3)
        raw = {"players": [{"id": player, "hp": 0 if index == 8 else 40, "mp": 10, "cp": 20,
                            "dead": index == 8, "guardian": "mars", "curses": ["fog"] if index == 0 else []}
                            for index, player in enumerate(ids)]}
        names = ("mars", "moon", "earth", "venus", "neptune", "pluto", "uranus", "saturn")
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0)
        game.phenomenon_step(PhenomenonAction(296, guardians=names))
        view = game.observe()
        self.assertEqual([player["status"]["guardian"] for player in view["board"]["players"]], list(names) + ["mars"])
        self.assertEqual(view["last_phenomenon"], {"model": 296, "recipient": None})
        self.assertEqual(len(view["last_guardian_changes"]), 8)
        self.assertTrue(all(event["reason"] == "phenomenon" for event in view["last_guardian_changes"]))
        self.assertEqual([event["player"] for event in game.observe(0)["last_guardian_changes"]], [0])
        self.assertEqual(view["last_guardian_changes"][0], {"player": 0, "before": "mars", "after": "mars", "reason": "phenomenon"})
        for private in ("guardians", "assignments", "cp_total", "phenomenon_actions", "rng"):
            self.assertNotIn(private, view); self.assertNotIn(private, view["last_phenomenon"])

    def test_phenomenon_strict_fields_pending_exclusion_and_late_atomicity(self):
        game = self.match(config=Config(initial_hand=0))
        self.assertEqual(game.supported_phenomenon_models, (287, 288, 289, 290, 291, 292, 293, 294, 296))
        self.assertTrue(set(game.supported_phenomenon_models).isdisjoint(game.supported_models))
        raw = {"players": [{"id": 0, "hp": 40, "mp": 99, "cp": 99, "guardian": "earth"},
                             {"id": 23, "hp": 40, "mp": 99, "cp": 99}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0)
        before, status = game.observe(0), game.status
        for malformed in (None, PhenomenonAction(True), PhenomenonAction(65536), PhenomenonAction(294, False),
                          PhenomenonAction(296, guardians=["mars", "moon"]), PhenomenonAction(296, guardians=("none", "moon")),
                          PhenomenonAction(296, guardians=(1, "moon")), PhenomenonAction(296, guardians=("mars",) * 10)):
            with self.assertRaises(InvalidMatchError): game.phenomenon_legal(malformed)
            with self.assertRaises(InvalidMatchError): game.phenomenon_step(malformed)
        for invalid in (PhenomenonAction(207), PhenomenonAction(294), PhenomenonAction(287, 0), PhenomenonAction(294, 999),
                        PhenomenonAction(296), PhenomenonAction(296, guardians=("mars",)),
                        PhenomenonAction(287, guardians=("mars", "moon"))):
            self.assertFalse(game.phenomenon_legal(invalid))
            with self.assertRaises(MatchError): game.phenomenon_step(invalid)
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.guardian_payload_step(GuardianPayloadAction(0, "earth", (6,), 23))
        before, status = game.observe(0), game.status
        self.assertFalse(game.phenomenon_legal(PhenomenonAction(288)))
        with self.assertRaises(InvalidMatchError): game.phenomenon_step(PhenomenonAction(288))
        self.assertEqual((game.observe(0), game.status), (before, status))
        raw["updateCount"] = 9007199254740991
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0)
        before, status = game.observe(0), game.status
        action = PhenomenonAction(296, guardians=("moon", "mars"))
        self.assertTrue(game.phenomenon_legal(action))
        with self.assertRaises(InvalidMatchError): game.phenomenon_step(action)
        self.assertEqual((game.observe(0), game.status), (before, status))

    def test_phenomenon_actual_client_vectors_with_explicit_match_scene_domain(self):
        # Client receiver inputs include scenes outside the executable Match
        # domain. Keep those cases explicit rather than repairing HP or diseases.
        report = json.loads((Path(__file__).resolve().parents[1] /
                             "data/official-v2/phenomenon-client-v34.json").read_text())
        game = self.match(config=Config(initial_hand=0, initial_hp=99, initial_mp=99, initial_cp=99))
        team = {"solo": 0, "red": 1, "blue": 2}
        curse_order = {name: index for index, name in enumerate(("cold", "fever", "hell", "heaven", "fog", "dream", "flash", "darkcloud"))}
        counts = {"applied": 0, "rejected": 0, "scene_outside": 0}
        self.assertEqual(len(report["cases"]), 298)
        for index, case in enumerate(report["cases"]):
            with self.subTest(index=index, label=case["label"], input=case["input"]):
                players = [{"id": player["id"], "userId": player["user_id"], "team": team[player["team"]],
                            "hp": player["hp"], "mp": player["mp"], "cp": player["cp"], "dead": player["dead"],
                            "curses": sorted(player["curses"], key=curse_order.__getitem__), "guardian": player["guardian"], "items": []}
                           for player in case["before"]]
                raw = {"gf": 19, "players": players}
                alive = [player for player in players if not player["dead"]]
                actor = alive[0]["id"] if alive else players[0]["id"]
                noncanonical = any((not player["dead"] and player["hp"] == 0)
                    or len(player["curses"]) != len(set(player["curses"]))
                    or len([curse for curse in player["curses"] if curse in ("cold", "fever", "hell", "heaven")]) > 1
                    for player in players)
                natural = len(alive) <= 1 or (alive[0]["team"] != 0 and all(player["team"] == alive[0]["team"] for player in alive))
                with OfficialState(raw, library=LIBRARY) as scene:
                    if noncanonical:
                        before, status = game.observe(), game.status
                        with self.assertRaises(MatchError): game.load_scene(scene, actor, 0)
                        self.assertEqual((game.observe(), game.status), (before, status))
                        counts["scene_outside"] += 1
                        continue
                    game.load_scene(scene, actor, 0)
                if natural:
                    self.assertTrue(game.status.terminated); self.assertFalse(game.status.truncated)
                    self.assertFalse(game.phenomenon_legal(PhenomenonAction(288)))
                    counts["scene_outside"] += 1
                    continue
                self.assertFalse(game.done)
                arguments = dict(case["input"])
                if type(arguments.get("guardians")) is list:
                    # A JSON array becomes a typed tuple without dropping or
                    # substituting any null/unknown entries from the wire case.
                    arguments["guardians"] = tuple(arguments["guardians"])
                action = PhenomenonAction(**arguments)
                before, status = game.observe(), game.status
                try: legal = game.phenomenon_legal(action)
                except InvalidMatchError: legal = False
                self.assertEqual(legal, case["adapter_shape"]["valid"])
                self.assertEqual((game.observe(), game.status), (before, status))
                if not legal:
                    with self.assertRaises(MatchError): game.phenomenon_step(action)
                    self.assertEqual((game.observe(), game.status), (before, status))
                    counts["rejected"] += 1
                    continue
                game.phenomenon_step(action)
                view = game.observe()
                self.assertFalse(case["result"]["error"])
                self.assertEqual([player["id"] for player in view["board"]["players"]], [player["id"] for player in case["result"]["after"]])
                for actual, expected in zip(view["board"]["players"], case["result"]["after"]):
                    self.assertEqual((actual["id"], actual["dead"]), (expected["id"], expected["dead"]))
                    self.assertEqual(tuple(actual["status"][key] for key in ("hp", "mp", "cp", "guardian")),
                                     tuple(expected[key] for key in ("hp", "mp", "cp", "guardian")))
                    self.assertEqual(set(actual["status"]["curses"]), set(expected["curses"]))
                self.assertEqual(view["last_phenomenon"], {"model": action.model, "recipient": action.recipient})
                self.assertEqual((game.status.owner, game.actor, game.status.decisions, view["board"]["gf"]), (actor, actor, 1, 19))
                self.assertEqual(view["last_auto_guardians"], [])
                counts["applied"] += 1
        self.assertEqual(counts, {"applied": 265, "rejected": 11, "scene_outside": 22})

    def test_targeted_phenomenon_source_is_absent_recovery_waits_and_mirror_cancels(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"gf": 9, "players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "curses": ["cold"]},
                                     {"id": 7, "hp": 98, "mp": 10, "cp": 20, "items": [{"id": 10, "modelId": 190}]},
                                     {"id": 23, "hp": 99, "mp": 10, "cp": 20}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.phenomenon_step(PhenomenonAction(293, 7))
        pending = game.observe()["pending"]
        self.assertEqual((game.actor, game.status.owner, pending["source"], pending["origin"], pending["environmental"]),
                         (7, 0, None, None, True))
        self.assertEqual((pending["models"], pending["category"], pending["power"], pending["amount"]), ([293], "phenomena", 0, 50))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 98)
        game.step(Command(7, "defend", (10,)))
        view = game.observe()
        self.assertIsNone(view["pending"])
        self.assertEqual([player["status"]["hp"] for player in view["board"]["players"]], [40, 98, 99])
        self.assertEqual((game.actor, game.status.owner, view["board"]["gf"]), (0, 0, 9))
        self.assertEqual(game.observe(0)["board"]["hand"], [])
        self.assertEqual(len(game.observe(7)["board"]["hand"]), 1)
        self.assertNotIn(10, {item["token"] for item in game.observe(7)["board"]["hand"]})
        self.assertEqual(view["last_phenomenon"], {"model": 293, "recipient": 7})
        for recipient, expected in ((7, [40, 99, 99]), (0, [90, 98, 99])):
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
            game.phenomenon_step(PhenomenonAction(293, recipient))
            self.assertEqual((game.status.phase, game.actor), ("response", recipient))
            game.step(Command(recipient, "defend"))
            self.assertEqual([player["status"]["hp"] for player in game.observe()["board"]["players"]], expected)
            self.assertEqual(game.observe()["board"]["gf"], 9)

    def test_targeted_light_phenomenon_filters_and_only_self_mp_counter_has_a_source(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"players": [{"id": 0, "hp": 99, "mp": 10, "cp": 20},
                             {"id": 7, "hp": 99, "mp": 0, "cp": 20, "items": [{"id": 10 + index, "modelId": model}
                                 for index, model in enumerate((189, 184, 181, 185, 186, 188, 187))]},
                             {"id": 23, "hp": 99, "mp": 10, "cp": 20}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.phenomenon_step(PhenomenonAction(291, 7))
        self.assertEqual((game.observe()["pending"]["power"], game.observe()["pending"]["element"]), (50, "light"))
        self.assertFalse(game.legal(Command(7, "defend", (11,))))
        response = Command(7, "defend", (10, 11, 12, 13, 14, 15))
        self.assertTrue(game.legal(response)); game.step(response)
        self.assertEqual((game.status.phase, game.actor, game.status.owner), ("main", 0, 0))
        self.assertEqual([player["status"]["hp"] for player in game.observe()["board"]["players"]], [99, 49, 99])
        self.assertEqual(game.observe(0)["board"]["hand"], [])
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.phenomenon_step(PhenomenonAction(291, 7)); game.step(Command(7, "defend", (10, 16)))
        pending = game.observe()["pending"]
        self.assertEqual((game.actor, pending["counter"], pending["environmental"], pending["origin"], pending["source"], pending["target"]),
                         (7, True, False, 7, 7, 7))
        self.assertEqual((pending["models"], pending["amount"]), ([187], 100))
        game.step(Command(7, "defend"))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["mp"], 99)
        raw["players"][0]["items"] = [{"id": 1, "modelId": 6}]
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.step(Command(0, "play", (1,), 7))
        pending = game.observe()["pending"]
        self.assertEqual((pending["origin"], pending["source"], pending["environmental"]), (0, 0, False))

    def test_targeted_dark_phenomenon_force_revival_and_real_zero_death_source(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"gf": 9, "players": [{"id": 0, "hp": 5, "mp": 0, "cp": 0, "curses": ["darkcloud"],
                 "items": [{"id": 1, "modelId": 112}, {"id": 2, "modelId": 208}]},
                 {"id": 7, "hp": 99, "mp": 0, "cp": 0}, {"id": 23, "hp": 99, "mp": 0, "cp": 0}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.phenomenon_step(PhenomenonAction(292, 0))
        pending = game.observe()["pending"]
        self.assertEqual((pending["source"], pending["forced_hit"], pending["hit_rate"], pending["element"]), (None, True, 75, "darkness"))
        game.step(Command(0, "defend"))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 10)
        self.assertIn(1, {item["token"] for item in game.observe(0)["board"]["hand"]})
        self.assertNotIn(2, {item["token"] for item in game.observe(0)["board"]["hand"]})
        self.assertEqual((game.actor, game.status.owner, game.observe()["board"]["gf"]), (0, 0, 9))
        raw["players"][0]["items"] = [{"id": 1, "modelId": 112}]
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.phenomenon_step(PhenomenonAction(292, 0)); game.step(Command(0, "defend"))
        pending = game.observe()["pending"]
        self.assertEqual((pending["death"], pending["environmental"], pending["source"], pending["origin"]), (True, False, 0, 0))
        responses = 0
        while game.status.phase == "response":
            self.assertLess(responses, 2); responses += 1
            pending = game.observe()["pending"]
            self.assertEqual((pending["source"], pending["environmental"]), (0, False))
            game.step(Command(game.actor, "defend"))
        self.assertTrue(game.observe()["board"]["players"][0]["dead"])
        self.assertEqual((game.actor, game.status.owner, game.observe()["board"]["gf"]), (7, 7, 9))
        self.assertEqual(game.observe()["last_phenomenon"], {"model": 292, "recipient": 0})
        raw["players"][0]["curses"] = []
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 123)
        game.phenomenon_step(PhenomenonAction(292, 7))
        self.assertEqual((game.status.phase, game.actor, game.status.decisions), ("main", 0, 1))
        self.assertEqual([player["status"]["hp"] for player in game.observe()["board"]["players"]], [5, 99, 99])

    def test_targeted_phenomenon_cutoff_fields_and_late_draw_rollback(self):
        raw = {"players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20},
                             {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
        for model in (291, 292, 293):
            game = self.match(config=Config(initial_hand=0, max_decisions=1))
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
            before, status = game.observe(), game.status
            for invalid in (PhenomenonAction(model), PhenomenonAction(model, 999), PhenomenonAction(model, 7, ("mars",))):
                self.assertFalse(game.phenomenon_legal(invalid))
                with self.assertRaises(InvalidMatchError): game.phenomenon_step(invalid)
            self.assertEqual((game.observe(), game.status), (before, status))
            game.phenomenon_step(PhenomenonAction(model, 0))
            self.assertTrue(game.status.truncated); self.assertFalse(game.status.terminated)
            self.assertIsNone(game.observe()["pending"])
            self.assertEqual([player["status"]["hp"] for player in game.observe()["board"]["players"]], [40, 40])
            self.assertEqual(game.observe()["last_phenomenon"], {"model": model, "recipient": 0})
        game = self.match(config=Config(initial_hand=0))
        raw["updateCount"] = 9007199254740991
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        before, status = game.observe(), game.status
        action = PhenomenonAction(292, 7)
        self.assertTrue(game.phenomenon_legal(action))
        with self.assertRaises(InvalidMatchError): game.phenomenon_step(action)
        self.assertEqual((game.observe(), game.status), (before, status))

    def test_targeted_phenomenon_actual_client_response_predicates_all_models(self):
        # Compare the six source-free frame projections and append predicates.
        # Numeric, decoder and reflect-rendering probes are separate evidence;
        # this does not turn their receiver behavior into an engine policy proof.
        report = json.loads((Path(__file__).resolve().parents[1] /
                             "data/official-v2/targeted-phenomenon-client-v35.json").read_text())
        frames = [frame for frame in report["frames"] if frame["environmental_annotation"]]
        self.assertEqual(len(frames), 6)
        game = self.match(config=Config(initial_hand=0, initial_hp=99, initial_mp=99, initial_cp=99))
        self.assertEqual(len(game.supported_models), 236)
        projections = queries = 0
        for frame in frames:
            projection = frame["projection"]
            curses = (["flash"] if frame["flash_context"] else []) + (["darkcloud"] if frame["model"] == 292 else [])
            raw = {"players": [{"id": 0, "hp": 99, "mp": 99, "cp": 99},
                                 {"id": 42, "hp": 99, "mp": 99, "cp": 99, "curses": curses}]}
            for model in game.supported_models:
                with self.subTest(phenomenon=frame["model"], flash=frame["flash_context"], response=model):
                    raw["players"][1]["items"] = [{"id": 10, "modelId": model}]
                    with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
                    game.phenomenon_step(PhenomenonAction(frame["model"], 42))
                    pending = game.observe()["pending"]
                    self.assertEqual((game.actor, game.status.owner, pending["origin"], pending["source"], pending["environmental"]),
                                     (42, 0, None, None, True))
                    self.assertEqual((pending["models"], pending["category"], pending["element"], pending["power"],
                                      pending["hit_rate"], pending["guardian"]),
                                     (frame["raw_attack"]["itemModelIds"], projection["effective_category"], projection["element"],
                                      projection["power"], projection["hit_rate"], projection["guardian_art"][0]))
                    before, status = game.observe(42), game.status
                    self.assertEqual(game.legal(Command(42, "defend", (10,))), model in frame["response"]["allowed_from_empty"])
                    self.assertEqual((game.observe(42), game.status), (before, status))
                    queries += 1
                    if frame["response"]["filter_prefix_is_legal"]:
                        raw["players"][1]["items"] = [{"id": 10, "modelId": 189}, {"id": 11, "modelId": model}]
                        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
                        game.phenomenon_step(PhenomenonAction(frame["model"], 42))
                        before, status = game.observe(42), game.status
                        self.assertTrue(game.legal(Command(42, "defend", (10,))))
                        self.assertEqual(game.legal(Command(42, "defend", (10, 11))), model in frame["response"]["allowed_after_filter"])
                        self.assertEqual((game.observe(42), game.status), (before, status))
                        queries += 1
            projections += 1
        self.assertEqual((projections, queries), (6, 2360))

    def test_item_shuffle_pairs_fresh_tokens_dead_exclusion_and_retained_recast(self):
        game = self.match(config=Config(initial_hand=0))
        self.assertEqual(game.supported_item_shuffle_models, (295,))
        self.assertNotIn(295, game.supported_phenomenon_models)
        raw = {"gf": 9, "players": [{"id": 0, "hp": 40, "mp": 99, "cp": 20, "items": [
                   {"id": 0, "modelId": 6}, {"id": 1, "modelId": 235, "used": True}]},
                   {"id": 7, "hp": 40, "mp": 99, "cp": 20, "items": [{"id": 0, "modelId": 5}, {"id": 1, "modelId": 152}]},
                   {"id": 23, "hp": 0, "mp": 99, "cp": 20, "dead": True, "items": [{"id": 0, "modelId": 208}]}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 3)
        dead = game.observe(23)["board"]["hand"]
        action = ItemShuffleAction((ItemSource(7, 0), ItemSource(7, 1), ItemSource(0, 1), ItemSource(0, 0)))
        before, status = game.observe(0), game.status
        self.assertTrue(game.item_shuffle_legal(action)); self.assertEqual((game.observe(0), game.status), (before, status))
        game.item_shuffle_step(action)
        self.assertEqual({item["token"]: (item["face"], item["used"]) for item in game.observe(0)["board"]["hand"]},
                         {2: (5, False), 3: (152, False)})
        self.assertEqual({item["token"]: (item["face"], item["used"]) for item in game.observe(7)["board"]["hand"]},
                         {4: (235, True), 5: (6, False)})
        self.assertEqual(game.observe(23)["board"]["hand"], dead)
        view = game.observe()
        self.assertEqual((game.actor, game.status.owner, game.status.decisions, view["board"]["gf"]), (0, 0, 1, 9))
        self.assertEqual(view["last_phenomenon"], {"model": 295, "recipient": None})
        self.assertEqual(view["last_reveals"], []); self.assertEqual(view["last_redraws"], [])
        self.assertEqual(view["board"]["players"][1]["used_models"], [235])
        for private in ("order", "sources", "count", "last_item_shuffle", "rng"):
            self.assertNotIn(private, view); self.assertNotIn(private, view["last_phenomenon"])
        game.step(Command(0, "discard", (3,)))
        self.assertFalse(game.legal(Command(7, "play", (1,), 7)))
        game.step(Command(7, "play", (4,), 7))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["mp"], 92)
        self.assertTrue(next(item for item in game.observe(7)["board"]["hand"] if item["token"] == 4)["used"])

    def test_item_shuffle_dream_fake_attributes_move_without_new_masks_or_public_reveal(self):
        game = self.match(config=Config(initial_hand=0))
        for recipient_dream in (False, True):
            raw = {"players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "curses": ["fog", "dream"], "items": [
                        {"id": 0, "modelId": 23, "fakeModelId": 13}, {"id": 1, "modelId": 6}]},
                        {"id": 7, "hp": 40, "mp": 10, "cp": 20, "curses": ["dream"] if recipient_dream else [],
                         "items": [{"id": 0, "modelId": 152}]}]}
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
            game.item_shuffle_step(ItemShuffleAction((ItemSource(7, 0), ItemSource(0, 1), ItemSource(0, 0))))
            own = game.observe(0)["board"]["hand"]
            self.assertEqual({item["token"]: item["face"] for item in own}, {2: 152, 3: 6})
            self.assertFalse(any(item["disguised"] for item in own))
            incoming = game.observe(7)["board"]["hand"]
            self.assertEqual(incoming, [{"token": 4, "face": 13 if recipient_dream else 23,
                                        "reference": 13 if recipient_dream else 23, "disguised": recipient_dream, "used": False}])
            for viewer in (None, 0, 7):
                view = game.observe(viewer)
                self.assertEqual(view["last_reveals"], []); self.assertEqual(view["last_redraws"], [])
                self.assertEqual(view["last_phenomenon"], {"model": 295, "recipient": None})
            self.assertIsNone(game.observe(0)["board"]["players"][1]["status"])

    def test_item_shuffle_manifest_strict_exact_cover_and_pending_rejection(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "items": [{"id": 0, "modelId": 6}]},
                             {"id": 7, "hp": 40, "mp": 10, "cp": 20, "items": [{"id": 0, "modelId": 191}]},
                             {"id": 23, "hp": 0, "mp": 10, "cp": 20, "dead": True, "items": [{"id": 0, "modelId": 208}]}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0)
        before, status = game.observe(0), game.status
        for malformed in (None, ItemShuffleAction([]), ItemShuffleAction(((0, 0),)), ItemShuffleAction((ItemSource(True, 0),)),
                          ItemShuffleAction((ItemSource(0, False),)), ItemShuffleAction((ItemSource(-1, 0),)),
                          ItemShuffleAction((ItemSource(0, 9007199254740992),)), ItemShuffleAction((ItemSource(0, 0),) * 163)):
            with self.assertRaises(InvalidMatchError): game.item_shuffle_legal(malformed)
            with self.assertRaises(InvalidMatchError): game.item_shuffle_step(malformed)
        for order in ((), (ItemSource(0, 0),), (ItemSource(0, 0), ItemSource(0, 0)),
                      (ItemSource(0, 0), ItemSource(999, 0)), (ItemSource(0, 0), ItemSource(7, 99)),
                      (ItemSource(0, 0), ItemSource(23, 0))):
            action = ItemShuffleAction(order)
            self.assertFalse(game.item_shuffle_legal(action))
            with self.assertRaises(MatchError): game.item_shuffle_step(action)
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(Command(0, "play", (0,), 7))
        before, status = game.observe(0), game.status
        self.assertFalse(game.item_shuffle_legal(ItemShuffleAction()))
        with self.assertRaises(InvalidMatchError): game.item_shuffle_step(ItemShuffleAction())
        self.assertEqual((game.observe(0), game.status), (before, status))

    def test_item_shuffle_capacity_empty_exhaustion_cutoff_and_late_rollback(self):
        game = self.match(config=Config(initial_hand=0))
        ids = (0, 7, 23, 42, 100, 108, 999, 9007199254740991, 3)
        raw = {"players": [{"id": player, "hp": 99, "mp": 99, "cp": 99,
                            "items": [{"id": token, "modelId": 6} for token in range(18)]} for player in ids]}
        order = tuple(reversed(tuple(ItemSource(player, token) for player in ids for token in range(18))))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0)
        self.assertTrue(game.item_shuffle_legal(ItemShuffleAction(order))); game.item_shuffle_step(ItemShuffleAction(order))
        hands = [game.observe(player)["board"]["hand"] for player in ids]
        self.assertTrue(all(len(hand) == 18 for hand in hands))
        self.assertEqual({item["token"] for hand in hands for item in hand}, set(range(18, 180)))
        small = {"players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "items": [{"id": 0, "modelId": 6}]},
                               {"id": 7, "hp": 40, "mp": 10, "cp": 20, "items": [{"id": 0, "modelId": 191}]},
                               {"id": 23, "hp": 0, "mp": 10, "cp": 20, "dead": True,
                                "items": [{"id": 9007199254740991, "modelId": 208}]}]}
        action = ItemShuffleAction((ItemSource(7, 0), ItemSource(0, 0)))
        with OfficialState(small, library=LIBRARY) as scene: game.load_scene(scene, 0)
        before, status = game.observe(0), game.status
        self.assertTrue(game.item_shuffle_legal(action))
        with self.assertRaises(MatchError): game.item_shuffle_step(action)
        self.assertEqual((game.observe(0), game.status), (before, status))
        small["players"][0]["items"] = []; small["players"][1]["items"] = []
        with OfficialState(small, library=LIBRARY) as scene: game.load_scene(scene, 0)
        self.assertTrue(game.item_shuffle_legal(ItemShuffleAction())); game.item_shuffle_step(ItemShuffleAction())
        self.assertEqual(game.observe()["last_phenomenon"], {"model": 295, "recipient": None})
        self.assertEqual(game.observe(23)["board"]["hand"][0]["token"], 9007199254740991)
        small["players"][2]["items"] = []
        for index, model in ((0, 6), (1, 191)): small["players"][index]["items"] = [{"id": 0, "modelId": model}]
        small["updateCount"] = 9007199254740991
        with OfficialState(small, library=LIBRARY) as scene: game.load_scene(scene, 0)
        before, status = game.observe(0), game.status
        with self.assertRaises(InvalidMatchError): game.item_shuffle_step(action)
        self.assertEqual((game.observe(0), game.status), (before, status))
        small.pop("updateCount")
        cut = self.match(config=Config(initial_hand=0, max_decisions=1))
        with OfficialState(small, library=LIBRARY) as scene: cut.load_scene(scene, 0)
        cut.item_shuffle_step(action)
        self.assertTrue(cut.status.truncated); self.assertFalse(cut.status.terminated)
        self.assertEqual({item["token"]: item["face"] for item in cut.observe(0)["board"]["hand"]}, {1: 191})
        self.assertEqual(cut.observe()["last_phenomenon"], {"model": 295, "recipient": None})

    def test_confusion_global_increment_and_supplied_main_replaces_no_selector(self):
        game = self.match(config=Config(initial_hand=0, hand_limit=1))
        raw = {"gf": 9, "players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "confusionCount": 1,
                    "items": [{"id": 0, "modelId": 208}]},
                    {"id": 7, "hp": 40, "mp": 10, "cp": 20, "confusionCount": 2, "items": [{"id": 0, "modelId": 208}]},
                    {"id": 23, "hp": 0, "mp": 10, "cp": 20, "dead": True}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.phenomenon_step(PhenomenonAction(289))
        self.assertEqual(game.observe()["last_phenomenon"], {"model": 289, "recipient": None})
        self.assertEqual([p["status"]["confused"] for p in game.observe()["board"]["players"]], [True, True, False])
        before, status = game.observe(0), game.status
        command = Command(0, "pray")
        self.assertFalse(game.legal(command)); self.assertTrue(game.confusion_legal(command))
        with self.assertRaises(UnsupportedMatchError): game.step(command)
        with self.assertRaises(UnsupportedMatchError): game.sample_action(123)
        self.assertEqual((game.observe(0), game.status), (before, status))
        remaining = {0: 4, 7: 5}; commands = 0
        while any(remaining.values()):
            owner = game.actor; action = Command(owner, "pray")
            if remaining[owner]:
                self.assertTrue(game.confusion_legal(action)); game.confusion_step(action); remaining[owner] -= 1
            else:
                self.assertFalse(game.confusion_legal(action)); self.assertTrue(game.legal(action)); game.step(action)
            commands += 1; self.assertLessEqual(commands, 10)
            status_by_id = {p["id"]: p["status"] for p in game.observe()["board"]["players"]}
            self.assertEqual([status_by_id[player]["confused"] for player in (0, 7)], [remaining[0] > 0, remaining[7] > 0])
        self.assertEqual(commands, 10)
        for private in ("confusion_count", "confusionCount", "confusion_command"):
            self.assertNotIn(private, game.observe()); self.assertNotIn(private, game.observe()["board"]["players"][0]["status"])

    def test_confusion_one_decay_per_main_responses_and_purchase_do_not_decay(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"players": [{"id": 0, "hp": 40, "mp": 14, "cp": 99, "confusionCount": 2,
                    "items": [{"id": 0, "modelId": 235}, {"id": 1, "modelId": 204}]},
                    {"id": 7, "hp": 40, "mp": 99, "cp": 99, "confusionCount": 1,
                     "items": [{"id": 0, "modelId": 190}, {"id": 1, "modelId": 152}]}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.confusion_step(Command(0, "play", (0, 1), 7))
        self.assertTrue(game.observe()["board"]["players"][0]["status"]["confused"])
        self.assertFalse(game.confusion_legal(Command(7, "defend", (0,))))
        game.step(Command(7, "defend", (0,))); game.step(Command(0, "defend"))
        self.assertEqual([p["status"]["confused"] for p in game.observe()["board"]["players"]], [True, True])
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["mp"], 14)
        game.confusion_step(Command(7, "discard", (1,)))
        game.confusion_step(Command(0, "play", (0,), 0))
        self.assertEqual([p["status"]["confused"] for p in game.observe()["board"]["players"]], [False, False])
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["mp"], 7)
        raw["players"][0]["items"] = [{"id": 0, "modelId": 5}]
        raw["players"][1]["items"] = [{"id": 0, "modelId": 233}]
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.confusion_step(Command(0, "buy", (0,), 7)); game.step(Command(7, "defend"))
        self.assertEqual(game.status.phase, "purchase")
        self.assertFalse(game.confusion_legal(Command(0, "accept")))
        self.assertTrue(game.legal(Command(0, "accept"))); game.step(Command(0, "accept"))
        self.assertEqual([p["status"]["confused"] for p in game.observe()["board"]["players"]], [True, True])

    def test_confusion_cures_revival_natural_finish_and_cutoff(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "confusionCount": 3,
                    "curses": ["cold", "fog", "dream", "flash", "darkcloud"],
                    "items": [{"id": 0, "modelId": 200}, {"id": 1, "modelId": 199}]},
                    {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.confusion_step(Command(0, "play", (0,), 0)); game.step(Command(7, "pray"))
        game.confusion_step(Command(0, "play", (1,), 0))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["curses"], [])
        self.assertTrue(game.observe()["board"]["players"][0]["status"]["confused"])
        raw = {"players": [{"id": 0, "hp": 5, "mp": 10, "cp": 20, "confusionCount": 1,
                              "items": [{"id": 0, "modelId": 208}]}, {"id": 7, "hp": 99, "mp": 10, "cp": 20}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.phenomenon_step(PhenomenonAction(291, 0)); game.step(Command(0, "defend"))
        self.assertEqual((game.observe()["board"]["players"][0]["status"]["hp"], game.observe()["board"]["players"][0]["status"]["confused"]), (10, True))
        raw = {"players": [{"id": 0, "team": 1, "hp": 40, "mp": 10, "cp": 20, "confusionCount": 2,
                    "items": [{"id": 0, "modelId": 64}]},
                    {"id": 7, "team": 1, "hp": 40, "mp": 10, "cp": 20, "confusionCount": 3},
                    {"id": 23, "team": 2, "hp": 40, "mp": 10, "cp": 20, "confusionCount": 4}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.confusion_step(Command(0, "play", (0,), 23)); game.step(Command(23, "defend"))
        self.assertTrue(game.status.terminated); self.assertEqual(game.status.winners, (0, 7))
        self.assertFalse(any(p["status"]["confused"] for p in game.observe()["board"]["players"]))
        cut = self.match(config=Config(initial_hand=0, max_decisions=1))
        raw["players"][0]["items"] = []
        with OfficialState(raw, library=LIBRARY) as scene: cut.load_scene(scene, 0, 0)
        cut.confusion_step(Command(0, "pray"))
        self.assertTrue(cut.status.truncated); self.assertTrue(cut.observe()["board"]["players"][0]["status"]["confused"])

    def test_confusion_overflow_external_preservation_and_late_atomicity(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "confusionCount": 9007199254740989,
                    "items": [{"id": 0, "modelId": 6}]}, {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        before, status = game.observe(0), game.status
        self.assertFalse(game.phenomenon_legal(PhenomenonAction(289)))
        with self.assertRaises(InvalidMatchError): game.phenomenon_step(PhenomenonAction(289))
        for invalid in (Command(7, "pray"), Command(0, "defend"), Command(0, "discard", (0,))):
            self.assertFalse(game.confusion_legal(invalid))
            with self.assertRaises(InvalidMatchError): game.confusion_step(invalid)
        self.assertEqual((game.observe(0), game.status), (before, status))
        raw["players"][0]["confusionCount"] = 1
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.item_shuffle_step(ItemShuffleAction((ItemSource(0, 0),)))
        self.assertTrue(game.observe()["board"]["players"][0]["status"]["confused"])
        raw["updateCount"] = 9007199254740991
        raw["players"][0]["items"] = [{"id": 0, "modelId": 95}]
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        before, status = game.observe(0), game.status
        action = Command(0, "play", (0,))
        self.assertTrue(game.confusion_legal(action))
        with self.assertRaises(InvalidMatchError): game.confusion_step(action)
        self.assertEqual((game.observe(0), game.status), (before, status))

    def test_fate_owned207_global_body_then_normal_tick_gift_and_confusion_debit(self):
        game = self.match(config=Config(initial_hand=0))
        self.assertEqual(game.supported_fate_models, (207,)); self.assertNotIn(207, game.supported_models)
        raw = {"gf": 9, "players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "confusionCount": 1,
                    "curses": ["cold"], "items": [{"id": 0, "modelId": 207}, {"id": 1, "modelId": 191}]},
                    {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        action = FateAction(0, 0, PhenomenonAction(287)); before, status = game.observe(0), game.status
        self.assertFalse(game.legal(Command(0, "play", (0,), 0)))
        self.assertTrue(game.fate_legal(action)); self.assertEqual((game.observe(0), game.status), (before, status))
        game.fate_step(action); view = game.observe()
        self.assertEqual((game.actor, game.status.owner, game.status.decisions, view["board"]["gf"]), (7, 7, 1, 10))
        self.assertEqual([player["status"]["hp"] for player in view["board"]["players"]], [38, 40])
        self.assertFalse(view["board"]["players"][0]["status"]["confused"])
        self.assertEqual(view["last_fate"], {"actor": 0, "model": 207, "phenomenon": 287})
        self.assertEqual(view["last_phenomenon"], {"model": 287, "recipient": None})
        self.assertEqual(len(game.observe(0)["board"]["hand"]), 2)
        self.assertNotIn(0, {item["token"] for item in game.observe(0)["board"]["hand"]})
        raw["players"][0]["confusionCount"] = 9007199254740989
        raw["players"][0]["curses"] = []
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        self.assertFalse(game.phenomenon_legal(PhenomenonAction(289)))
        action = FateAction(0, 0, PhenomenonAction(289))
        self.assertTrue(game.fate_legal(action)); game.fate_step(action)
        self.assertTrue(game.observe()["board"]["players"][0]["status"]["confused"])

    def test_fate_targeted_waits_then_resumes_ordinary_owner_and_cutoff(self):
        raw = {"gf": 9, "players": [{"id": 0, "hp": 99, "mp": 10, "cp": 20, "curses": ["cold"],
                    "items": [{"id": 0, "modelId": 207}]},
                    {"id": 7, "hp": 99, "mp": 10, "cp": 20, "items": [{"id": 0, "modelId": 190}]}]}
        game = self.match(config=Config(initial_hand=0))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        action = FateAction(0, 0, PhenomenonAction(291, 7)); game.fate_step(action)
        view = game.observe()
        self.assertEqual((game.actor, game.status.owner, view["board"]["gf"]), (7, 0, 9))
        self.assertEqual((view["pending"]["source"], view["pending"]["origin"], view["pending"]["environmental"]), (None, None, True))
        self.assertEqual(game.observe(0)["board"]["hand"], [])
        self.assertEqual(view["board"]["players"][0]["status"]["hp"], 99)
        game.step(Command(7, "defend", (0,)))
        view = game.observe()
        self.assertEqual((game.actor, game.status.owner, view["board"]["gf"]), (7, 7, 10))
        self.assertEqual([player["status"]["hp"] for player in view["board"]["players"]], [98, 99])
        self.assertEqual([len(game.observe(player)["board"]["hand"]) for player in (0, 7)], [1, 1])
        self.assertEqual(view["last_fate"], {"actor": 0, "model": 207, "phenomenon": 291})
        cut = self.match(config=Config(initial_hand=0, max_decisions=1))
        with OfficialState(raw, library=LIBRARY) as scene: cut.load_scene(scene, 0, 0)
        cut.fate_step(action)
        self.assertTrue(cut.status.truncated); self.assertIsNone(cut.observe()["pending"])
        self.assertEqual(cut.observe(0)["board"]["hand"], [])
        self.assertEqual(cut.observe()["board"]["players"][0]["status"]["hp"], 99)

    def test_fate_shuffle_uses_postconsumption_slots_and_excludes_reserved_gift(self):
        raw = {"gf": 9, "players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20,
                    "items": [{"id": 0, "modelId": 207}, {"id": 1, "modelId": 235, "used": True}]},
                    {"id": 7, "hp": 40, "mp": 10, "cp": 20, "items": [{"id": 0, "modelId": 6}, {"id": 1, "modelId": 152}]},
                    {"id": 23, "hp": 0, "mp": 10, "cp": 20, "dead": True, "items": [{"id": 5, "modelId": 208}]}]}
        shuffle = ItemShuffleAction((ItemSource(7, 0), ItemSource(0, 1), ItemSource(7, 1)))
        action = FateAction(0, 0, PhenomenonAction(295), shuffle)
        for maximum in (512, 1):
            game = self.match(config=Config(initial_hand=0, max_decisions=maximum))
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
            self.assertTrue(game.fate_legal(action)); game.fate_step(action)
            own = game.observe(0)["board"]["hand"]
            self.assertEqual(next(item["face"] for item in own if item["token"] == 6), 6)
            self.assertEqual({item["token"]: (item["face"], item["used"]) for item in game.observe(7)["board"]["hand"]},
                             {7: (235, True), 8: (152, False)})
            self.assertEqual({item["token"] for item in own}, {6, 9} if maximum != 1 else {6})
            self.assertEqual(game.observe()["last_fate"], {"actor": 0, "model": 207, "phenomenon": 295})
            self.assertEqual(game.observe(23)["board"]["hand"][0]["token"], 5)
            if maximum == 1: self.assertTrue(game.status.truncated)
            else:
                game.step(Command(7, "play", (7,), 7))
                self.assertEqual(game.observe()["board"]["players"][1]["status"]["mp"], 3)

    def test_fate_strict_whole_operation_rollback_sampler_and_commodity_roles(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "items": [
                    {"id": 0, "modelId": 207}, {"id": 1, "modelId": 4}, {"id": 2, "modelId": 5}]},
                    {"id": 7, "hp": 40, "mp": 10, "cp": 20, "items": [{"id": 0, "modelId": 152}]}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        before, status = game.observe(0), game.status
        for malformed in (None, FateAction(True, 0, PhenomenonAction(287)), FateAction(0, False, PhenomenonAction(287)),
                          FateAction(0, 0, {}), FateAction(0, 0, PhenomenonAction(295), ()),
                          FateAction(0, 0, PhenomenonAction(295)),
                          FateAction(0, 0, PhenomenonAction(287), ItemShuffleAction())):
            with self.assertRaises(InvalidMatchError): game.fate_legal(malformed)
            with self.assertRaises(InvalidMatchError): game.fate_step(malformed)
        for invalid in (FateAction(7, 0, PhenomenonAction(287)), FateAction(0, 1, PhenomenonAction(287)),
                        FateAction(0, 0, PhenomenonAction(207)),
                        FateAction(0, 0, PhenomenonAction(295), ItemShuffleAction((ItemSource(0, 0),)))):
            self.assertFalse(game.fate_legal(invalid))
            with self.assertRaises(MatchError): game.fate_step(invalid)
        for seed in range(64):
            proposal, unused = game.sample_action(seed)
            self.assertTrue(game.legal(proposal)); self.assertFalse(proposal.kind == "play" and 0 in proposal.items)
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(Command(0, "sell", (1, 0), 0))
        self.assertEqual(game.observe()["last_sale"]["model"], 207); self.assertIsNone(game.observe()["last_fate"])
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.step(Command(0, "discard", (0,))); self.assertIsNone(game.observe()["last_fate"])
        raw["updateCount"] = 9007199254740991; raw["players"][0]["confusionCount"] = 1
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        before, status = game.observe(0), game.status
        action = FateAction(0, 0, PhenomenonAction(290)); self.assertTrue(game.fate_legal(action))
        with self.assertRaises(InvalidMatchError): game.fate_step(action)
        self.assertEqual((game.observe(0), game.status), (before, status))
        overflow = self.match(config=Config(initial_hand=0, hand_limit=2))
        full = {"players": [{"id": 0, "hp": 40, "mp": 10, "cp": 99, "items": [
                    {"id": 0, "modelId": 4}, {"id": 1, "modelId": 6}]},
                    {"id": 7, "hp": 40, "mp": 10, "cp": 99, "items": [
                    {"id": 0, "modelId": 207}, {"id": 1, "modelId": 207}]}]}
        with OfficialState(full, library=LIBRARY) as scene: overflow.load_scene(scene, 0, 0)
        overflow.step(Command(0, "sell", (0, 1), 7)); overflow.step(Command(7, "defend"))
        self.assertEqual(overflow.observe()["last_sale"]["overflow_model"], 207)
        self.assertIsNone(overflow.observe()["last_fate"])

    def test_devil_fixed_hp_damage_is_immediate_external_and_has_no_counter(self):
        for model, amount in ((240, 10), (241, 20), (242, 30)):
            game = self.match(config=Config(initial_hand=0))
            raw = {"gf": 9, "players": [
                {"id": 0, "hp": 99, "mp": 11, "cp": 23, "confusionCount": 1, "curses": ["cold"]},
                {"id": 42, "hp": 99, "mp": 12, "cp": 24, "items": [
                    {"id": 0, "modelId": 190}, {"id": 1, "modelId": 184}, {"id": 2, "modelId": 187}]}]}
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
            self.assertEqual(game.supported_devil_models, (240, 241, 242, 243, 244))
            self.assertFalse(set(game.supported_devil_models) & set(game.supported_models))
            before, status = game.observe(), game.status
            action = DevilAction(42, model)
            self.assertTrue(game.devil_legal(action)); self.assertEqual((game.observe(), game.status), (before, status))
            game.devil_step(action); view = game.observe()
            self.assertEqual((game.status.owner, game.status.actor, game.status.phase, game.status.decisions), (0, 0, "main", 1))
            self.assertEqual(view["board"]["gf"], 9); self.assertIsNone(view["pending"])
            self.assertEqual([p["status"]["hp"] for p in view["board"]["players"]], [99, 99-amount])
            self.assertEqual([(p["status"]["mp"], p["status"]["cp"]) for p in view["board"]["players"]], [(11, 23), (12, 24)])
            self.assertEqual(view["last_devil"], {"model": model, "target": 42, "removed_models": []})
            self.assertTrue(game.observe(0)["board"]["players"][0]["status"]["confused"])
            self.assertEqual(len(game.observe(42)["board"]["hand"]), 3)
            self.assertEqual(view["last_auto_guardians"], [])

    def test_devil_exact_two_physical_removals_preserve_unselected_passives(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20},
                {"id": 7, "hp": 40, "mp": 10, "cp": 20, "curses": ["dream"], "items": [
                    {"id": 0, "modelId": 235, "used": True}, {"id": 1, "modelId": 23, "fakeModelId": 13},
                    {"id": 2, "modelId": 207}, {"id": 3, "modelId": 208}, {"id": 4, "modelId": 112}, {"id": 5, "modelId": 209}]}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        before, status = game.observe(7), game.status
        for tokens in ((), (0,), (0, 0), (0, 99)):
            action = DevilAction(7, 243, removed=tokens)
            self.assertFalse(game.devil_legal(action))
            with self.assertRaises(InvalidMatchError): game.devil_step(action)
            self.assertEqual((game.observe(7), game.status), (before, status))
        game.devil_step(DevilAction(7, 243, removed=(1, 0)))
        view = game.observe(); own = game.observe(7)
        self.assertEqual(view["last_devil"], {"model": 243, "target": 7, "removed_models": [23, 235]})
        self.assertEqual(view["last_reveals"], [{"owner": 7, "face": 13, "model": 23}])
        self.assertEqual({item["token"] for item in own["board"]["hand"]}, {2, 3, 4, 5})
        self.assertEqual(own["board"]["players"][1]["used_models"], [])
        self.assertIsNone(view["last_fate"]); self.assertIsNone(view["pending"])
        for count in range(4):
            raw["players"][1]["items"] = [{"id": i, "modelId": 207} for i in range(count)]
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
            game.devil_step(DevilAction(7, 243, removed=tuple(range(min(2, count)))))
            self.assertEqual(game.observe()["last_devil"]["removed_models"], [207]*min(2, count))
            self.assertEqual(len(game.observe(7)["board"]["hand"]), max(0, count-2))

    def test_devil_resource_boost_caps_and_fog_hides_private_choice(self):
        raw = {"gf": 8, "players": [
            {"id": 0, "hp": 40, "mp": 10, "cp": 20, "curses": ["cold", "fog"], "confusionCount": 3},
            {"id": 7, "hp": 95, "mp": 95, "cp": 95}]}
        hidden = []
        for resource in ("hp", "mp", "cp"):
            game = self.match(config=Config(initial_hand=0))
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
            game.devil_step(DevilAction(7, 244, resource))
            view = game.observe(); target = view["board"]["players"][1]["status"]
            self.assertEqual(tuple(target[key] for key in ("hp", "mp", "cp")),
                             tuple(99 if key == resource else 95 for key in ("hp", "mp", "cp")))
            self.assertEqual(view["last_devil"], {"model": 244, "target": 7, "removed_models": []})
            self.assertEqual((game.status.owner, game.status.actor, view["board"]["gf"]), (0, 0, 8))
            hidden.append(game.observe(0))
        self.assertEqual(hidden[0], hidden[1]); self.assertEqual(hidden[1], hidden[2])

    def test_devil_revival_and_posthumous_responses_use_existing_death_boundary(self):
        raw = {"gf": 9, "players": [
            {"id": 0, "hp": 99, "mp": 10, "cp": 20, "curses": ["darkcloud"], "items": [{"id": 0, "modelId": 190}]},
            {"id": 7, "hp": 5, "mp": 10, "cp": 20, "items": [{"id": 0, "modelId": 112}]},
            {"id": 23, "hp": 99, "mp": 10, "cp": 20, "curses": ["darkcloud"]}]}
        game = self.match(config=Config(initial_hand=0))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.devil_step(DevilAction(7, 240)); view = game.observe(); pending = view["pending"]
        self.assertEqual((pending["source"], pending["target"], pending["death"], pending["environmental"]), (7, 0, True, False))
        self.assertEqual(pending["models"], [112]); self.assertEqual(game.status.owner, 0)
        self.assertFalse(game.devil_legal(DevilAction(0, 244, "hp")))
        game.step(Command(0, "defend", (0,)))
        self.assertEqual(game.observe()["pending"]["target"], 23)
        game.step(Command(23, "defend"))
        self.assertEqual(game.status.phase, "main"); self.assertEqual(game.status.owner, 0)
        self.assertEqual(game.observe()["board"]["players"][2]["status"]["hp"], 69)
        self.assertEqual(game.observe()["board"]["gf"], 9)
        raw["players"][1]["items"].append({"id": 1, "modelId": 208})
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.devil_step(DevilAction(7, 242)); self.assertIsNone(game.observe()["pending"])
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 10)
        self.assertIn(0, {item["token"] for item in game.observe(7)["board"]["hand"]})
        self.assertNotIn(1, {item["token"] for item in game.observe(7)["board"]["hand"]})
        raw["players"][1]["items"] = [{"id": 0, "modelId": 112}]
        cut = self.match(config=Config(initial_hand=0, max_decisions=1))
        with OfficialState(raw, library=LIBRARY) as scene: cut.load_scene(scene, 0, 0)
        cut.devil_step(DevilAction(7, 240)); self.assertTrue(cut.status.truncated)
        self.assertIsNone(cut.observe()["pending"]); self.assertEqual(cut.observe(7)["board"]["hand"], [])

    def test_devil_strict_variants_and_late_failure_restore_whole_world(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20},
                {"id": 7, "hp": 40, "mp": 10, "cp": 20, "items": [{"id": 0, "modelId": 207}]}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        before, status = game.observe(), game.status
        for malformed in (None, {}, DevilAction(True, 240), DevilAction(0, True), DevilAction(0, 244),
                DevilAction(0, 244, "HP"), DevilAction(0, 244, False), DevilAction(0, 240, "hp"),
                DevilAction(0, 240, removed=(0,)), DevilAction(7, 243, removed=[0]),
                DevilAction(7, 243, removed=(True,)), DevilAction(7, 243, removed=(0, 1, 2))):
            with self.assertRaises(InvalidMatchError): game.devil_legal(malformed)
            with self.assertRaises(InvalidMatchError): game.devil_step(malformed)
            self.assertEqual((game.observe(), game.status), (before, status))
        for invalid in (DevilAction(99, 240), DevilAction(0, 239), DevilAction(7, 243, removed=(99,))):
            self.assertFalse(game.devil_legal(invalid))
            with self.assertRaises(MatchError): game.devil_step(invalid)
            self.assertEqual((game.observe(), game.status), (before, status))
        raw["updateCount"] = 9007199254740991
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        before, status = game.observe(), game.status
        self.assertTrue(game.devil_legal(DevilAction(0, 244, "cp")))
        with self.assertRaises(InvalidMatchError): game.devil_step(DevilAction(0, 244, "cp"))
        self.assertEqual((game.observe(), game.status), (before, status))
        del raw["updateCount"]
        raw["players"][0]["items"] = [{"id": 9007199254740991, "modelId": 6}]
        raw["players"][1].update(hp=5, items=[{"id": 0, "modelId": 208}])
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        before, status = game.observe(7), game.status
        self.assertTrue(game.devil_legal(DevilAction(7, 240)))
        with self.assertRaises(InvalidMatchError): game.devil_step(DevilAction(7, 240))
        self.assertEqual((game.observe(7), game.status), (before, status))

    def test_sacrifice_threshold_domain_and_discard_boundary_are_exclusive(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"players": [{"id": 0, "hp": 40, "mp": 0, "cp": 20, "items": [{"id": 0, "modelId": 152}]},
                            {"id": 7, "hp": 40, "mp": 0, "cp": 20}]}
        for gf, threshold in ((0, None), (0, 0), (2, 3), (3, 3), (4, 3), (0, 9007199254740991), (9007199254740991, 9007199254740991)):
            raw["gf"] = gf; raw["tiebreakGF"] = threshold
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
            active = threshold is not None and gf >= threshold
            before, status = game.observe(0), game.status
            self.assertEqual(before["apocalypse_active"], active)
            self.assertEqual(before["board"]["tiebreak_gf"], threshold)
            self.assertEqual(game.legal(Command(0, "discard", (0,))), not active)
            self.assertEqual(game.legal(Command(0, "sacrifice", (0,))), active)
            wrong = Command(0, "discard" if active else "sacrifice", (0,))
            with self.assertRaises(InvalidMatchError): game.step(wrong)
            self.assertEqual((game.observe(0), game.status), (before, status))
        raw["gf"] = 0; raw["tiebreakGF"] = -1
        with OfficialState(raw, library=LIBRARY) as scene:
            with self.assertRaises(InvalidMatchError): game.load_scene(scene, 0, 0)
        self.assertEqual((game.observe(0), game.status), (before, status))
        raw["tiebreakGF"] = 9007199254740992
        with self.assertRaises(ValueError): OfficialState(raw, library=LIBRARY)

    def test_sacrifice_removes_used_fake_and207_then_replaces_each_copy(self):
        game = self.match(config=Config(initial_hand=0))
        raw = {"gf": 4, "tiebreakGF": 4, "players": [
            {"id": 0, "hp": 40, "mp": 0, "cp": 20, "curses": ["dream"], "items": [
                {"id": 0, "modelId": 235, "used": True}, {"id": 1, "modelId": 23, "fakeModelId": 13},
                {"id": 2, "modelId": 207}, {"id": 3, "modelId": 208}, {"id": 4, "modelId": 209}, {"id": 5, "modelId": 6}]},
            {"id": 7, "hp": 40, "mp": 0, "cp": 20}]}
        # Dream compatibility preserves category, so a disguised weapon remains unsacrificable.
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        for token in (1, 3, 4, 5): self.assertFalse(game.legal(Command(0, "sacrifice", (token,))))
        raw["players"][0]["items"][1] = {"id": 1, "modelId": 118, "fakeModelId": 123}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        # Both armor IDs belong to the pinned neutral ordinary-defense class.
        self.assertTrue(game.legal(Command(0, "sacrifice", (0, 1, 2))))
        game.step(Command(0, "sacrifice", (0, 1, 2)))
        view, hand = game.observe(), game.observe(0)["board"]["hand"]
        self.assertEqual({i["token"] for i in hand}, {3, 4, 5, 6, 7, 8})
        self.assertFalse(any(i["used"] for i in hand)); self.assertEqual(view["board"]["players"][0]["used_models"], [])
        self.assertEqual(view["last_reveals"], [{"owner": 0, "face": 123, "model": 118}])
        self.assertEqual((game.actor, view["board"]["gf"], view["board"]["players"][0]["status"]["mp"]), (7, 5, 0))
        self.assertIsNone(view["last_fate"]); self.assertIsNone(view["last_devil"])
        for maximum in (512, 1):
            full = self.match(config=Config(initial_hand=0, max_decisions=maximum))
            raw["players"][0]["items"] = [{"id": i, "modelId": 235, "used": True} for i in range(18)]
            raw["players"][0]["curses"] = []
            with OfficialState(raw, library=LIBRARY) as scene: full.load_scene(scene, 0, 0)
            full.step(Command(0, "sacrifice", tuple(range(18))))
            hand = full.observe(0)["board"]["hand"]
            self.assertEqual({item["token"] for item in hand}, set(range(18, 36)) if maximum != 1 else set())
            self.assertTrue(all(not item["used"] for item in hand))
            self.assertEqual(full.status.truncated, maximum == 1)

    def test_sacrifice_confusion_tick_death_and_revival_use_normal_turn_rules(self):
        raw = {"gf": 4, "tiebreakGF": 0, "players": [
            {"id": 0, "hp": 40, "mp": 0, "cp": 20, "curses": ["cold"], "confusionCount": 1,
             "items": [{"id": 0, "modelId": 152}]}, {"id": 7, "hp": 40, "mp": 0, "cp": 20}]}
        game = self.match(config=Config(initial_hand=0))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        command = Command(0, "sacrifice", (0,)); before, status = game.observe(0), game.status
        self.assertFalse(game.legal(command)); self.assertTrue(game.confusion_legal(command))
        with self.assertRaises(UnsupportedMatchError): game.step(command)
        with self.assertRaises(UnsupportedMatchError): game.sample_action(3)
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.confusion_step(command); view = game.observe()
        self.assertFalse(view["board"]["players"][0]["status"]["confused"])
        self.assertEqual(view["board"]["players"][0]["status"]["hp"], 39)
        self.assertEqual((game.actor, view["board"]["gf"], len(game.observe(0)["board"]["hand"])), (7, 5, 1))
        raw["players"][0].update(hp=1, confusionCount=0)
        for amulet in (False, True):
            raw["players"][0]["items"] = [{"id": 0, "modelId": 152}] + ([{"id": 1, "modelId": 208}] if amulet else [])
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
            game.step(command)
            if amulet:
                self.assertFalse(game.done); self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 10)
                self.assertEqual({item["token"] for item in game.observe(0)["board"]["hand"]}, {2, 3})
            else:
                self.assertTrue(game.status.terminated); self.assertEqual(game.status.winners, (7,))
                self.assertEqual(game.observe(0)["board"]["hand"], [])
                self.assertTrue(game.observe()["apocalypse_active"])

    def test_sacrifice_sampler_maps_discard_without_extra_policy_rng(self):
        raw = {"gf": 0, "players": [{"id": 0, "hp": 40, "mp": 20, "cp": 20, "items": [
                    {"id": 0, "modelId": 235, "used": True}, {"id": 1, "modelId": 152}]},
                    {"id": 7, "hp": 40, "mp": 20, "cp": 20}]}
        inactive, active = self.match(config=Config(initial_hand=0)), self.match(config=Config(initial_hand=0))
        with OfficialState(raw, library=LIBRARY) as scene: inactive.load_scene(scene, 0, 3)
        raw["tiebreakGF"] = 0
        with OfficialState(raw, library=LIBRARY) as scene: active.load_scene(scene, 0, 3)
        before = (inactive.observe(0), active.observe(0)); seen = False
        for rng in range(64):
            ordinary, ra = inactive.sample_action(rng); apocalypse, rb = active.sample_action(rng)
            expected = Command(ordinary.actor, "sacrifice" if ordinary.kind == "discard" else ordinary.kind,
                               ordinary.items, ordinary.target, ordinary.allocation)
            self.assertEqual((apocalypse, rb), (expected, ra))
            self.assertTrue(inactive.legal(ordinary)); self.assertTrue(active.legal(apocalypse))
            self.assertNotEqual(ordinary.kind, "sacrifice"); self.assertNotEqual(apocalypse.kind, "discard")
            seen |= apocalypse.kind == "sacrifice"
        self.assertTrue(seen); self.assertEqual((inactive.observe(0), active.observe(0)), before)

    def test_sacrifice_invalid_shape_cutoff_and_late_exhaustion_are_atomic(self):
        raw = {"gf": 4, "tiebreakGF": 0, "players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20,
                    "items": [{"id": 0, "modelId": 152}]}, {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
        game = self.match(config=Config(initial_hand=0))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        before, status = game.observe(0), game.status
        for command in (Command(0, "sacrifice"), Command(0, "sacrifice", (0, 0)), Command(0, "sacrifice", (0,), 0),
                        Command(0, "sacrifice", (0,), allocation=(40, 10, 20)), Command(7, "sacrifice", (0,))):
            self.assertFalse(game.legal(command))
            with self.assertRaises(InvalidMatchError): game.step(command)
            self.assertEqual((game.observe(0), game.status), (before, status))
        for exhausted in ("update", "gf", "token"):
            altered = copy.deepcopy(raw)
            if exhausted == "update": altered["updateCount"] = 9007199254740991
            if exhausted == "gf": altered["gf"] = 9007199254740991
            if exhausted == "token": altered["players"][0]["items"].append({"id": 9007199254740991, "modelId": 6})
            with OfficialState(altered, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
            before, status = game.observe(0), game.status; command = Command(0, "sacrifice", (0,))
            self.assertTrue(game.legal(command))
            with self.assertRaises(InvalidMatchError): game.step(command)
            self.assertEqual((game.observe(0), game.status), (before, status))
        cutoff = self.match(config=Config(initial_hand=0, max_decisions=1))
        with OfficialState(altered, library=LIBRARY) as scene: cutoff.load_scene(scene, 0, 0)
        cutoff.step(command); self.assertTrue(cutoff.status.truncated)
        self.assertTrue(cutoff.observe()["apocalypse_active"])
        self.assertEqual({item["token"] for item in cutoff.observe(0)["board"]["hand"]}, {9007199254740991})

    def test_external_grant_normal_wait_has_no_actor_and_advances_once(self):
        game = self.match(config=Config(initial_hand=0, external_grants=True))
        self.assertEqual(game.supported_grant_models, (0, 240, 241, 242, 243, 244))
        raw = {"gf": 0, "tiebreakGF": 0, "players": [
            {"id": 0, "hp": 40, "mp": 10, "cp": 20, "curses": ["cold"], "items": [{"id": 0, "modelId": 152}]},
            {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.step(Command(0, "sacrifice", (0,)))
        view, status = game.observe(0), game.status
        self.assertEqual((status.phase, status.actor, status.owner, status.decisions), ("grant", None, 0, 1))
        self.assertFalse(game.done); self.assertEqual(view["pending_grant"], {"target": 0})
        self.assertIsNone(view["pending"]); self.assertIsNone(view["last_grant"])
        self.assertEqual(view["board"]["hand"], []); self.assertEqual(view["board"]["players"][0]["status"]["hp"], 39)
        self.assertFalse(game.legal(Command(0, "pray")))
        with self.assertRaises(MatchError): game.step(Command(0, "pray"))
        with self.assertRaises(UnsupportedMatchError): game.sample_action(123)
        self.assertEqual((game.observe(0), game.status), (view, status))
        action = GrantAction(0); self.assertTrue(game.grant_legal(action))
        self.assertEqual((game.observe(0), game.status), (view, status))
        game.grant_step(action); view = game.observe(0)
        self.assertEqual((game.status.phase, game.actor, game.status.owner, game.status.decisions), ("main", 7, 7, 2))
        self.assertEqual(view["board"]["gf"], 1); self.assertEqual(view["board"]["players"][0]["status"]["hp"], 39)
        self.assertEqual(len(view["board"]["hand"]), 1); self.assertIsNone(view["pending_grant"])
        self.assertEqual(view["last_grant"], {"target": 0, "devil": None, "removed_models": []})
        self.assertFalse(game.grant_legal(action))

    def test_external_grant_disabled_and_prethreshold_keep_existing_draws(self):
        base = {"gf": 0, "players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "curses": ["dream"]},
                                      {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
        for threshold in (None, 10):
            raw = copy.deepcopy(base); raw["tiebreakGF"] = threshold
            games = [self.match(config=Config(initial_hand=0, external_grants=flag)) for flag in (False, True)]
            for game in games:
                with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
                game.step(Command(0, "pray")); self.assertEqual(game.status.phase, "main")
                self.assertIsNone(game.observe()["pending_grant"])
                game.step(Command(7, "pray"))
            self.assertEqual(games[0].observe(0)["board"], games[1].observe(0)["board"])
            self.assertEqual(games[0].observe(7)["board"], games[1].observe(7)["board"])
        raw = copy.deepcopy(base); raw["tiebreakGF"] = 0
        old = self.match(config=Config(initial_hand=0)); opted = self.match(config=Config(initial_hand=0, external_grants=True))
        for game in (old, opted):
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
            game.step(Command(0, "pray"))
        self.assertEqual(old.status.phase, "main"); self.assertEqual(opted.status.phase, "grant")
        opted.grant_step(GrantAction(0))
        self.assertEqual(old.observe(0)["board"], opted.observe(0)["board"])
        for invalid in (0, 1, None, "true"):
            with self.assertRaises(InvalidMatchError): self.match(config=Config(external_grants=invalid))

    def test_external_grant_devil_retry_costs_decisions_and_cutoff_cancels_debt(self):
        game = self.match(config=Config(initial_hand=0, max_decisions=4, external_grants=True))
        raw = {"gf": 0, "tiebreakGF": 0, "players": [{"id": 0, "hp": 99, "mp": 99, "cp": 99},
                                                     {"id": 7, "hp": 99, "mp": 99, "cp": 99}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.step(Command(0, "pray"))
        for decision in (2, 3, 4):
            game.grant_step(GrantAction(0, 244, "hp"))
            self.assertEqual(game.status.decisions, decision)
            self.assertEqual(game.observe(0)["board"]["hand"], [])
            self.assertEqual(game.observe()["last_grant"], {"target": 0, "devil": 244, "removed_models": []})
            self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 99)
            if decision < 4: self.assertEqual(game.observe()["pending_grant"], {"target": 0})
        self.assertTrue(game.status.truncated); self.assertFalse(game.status.terminated)
        self.assertIsNone(game.observe()["pending_grant"]); self.assertEqual(game.observe()["board"]["gf"], 0)

    def test_external_grant_trickster_selects_current_inventory_not_reserved_artifact(self):
        game = self.match(config=Config(initial_hand=0, external_grants=True))
        raw = {"gf": 0, "tiebreakGF": 0, "players": [
            {"id": 0, "hp": 40, "mp": 0, "cp": 20, "items": [
                {"id": 0, "modelId": 235, "used": True}, {"id": 1, "modelId": 207}, {"id": 2, "modelId": 152}]},
            {"id": 7, "hp": 40, "mp": 0, "cp": 20}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.step(Command(0, "sacrifice", (2,))); before, status = game.observe(0), game.status
        bad = GrantAction(0, 243, removed=(0, 3))
        self.assertFalse(game.grant_legal(bad))
        with self.assertRaises(InvalidMatchError): game.grant_step(bad)
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.grant_step(GrantAction(0, 243, removed=(1, 0)))
        self.assertEqual(game.observe()["last_grant"], {"target": 0, "devil": 243, "removed_models": [207, 235]})
        self.assertEqual(game.observe(0)["board"]["hand"], []); self.assertEqual(game.status.phase, "grant")
        self.assertIsNone(game.observe()["last_fate"])
        game.grant_step(GrantAction(0, 243))
        self.assertEqual(game.observe()["last_grant"]["removed_models"], [])
        game.grant_step(GrantAction(0))
        self.assertEqual({item["token"] for item in game.observe(0)["board"]["hand"]}, {3})
        self.assertEqual(game.status.phase, "main")

    def test_external_grant_revival_adds_debt_and_death_responses_resume(self):
        raw = {"gf": 0, "tiebreakGF": 0, "players": [
            {"id": 0, "hp": 5, "mp": 10, "cp": 20, "confusionCount": 2,
             "items": [{"id": 0, "modelId": 208}, {"id": 1, "modelId": 152}]},
            {"id": 7, "hp": 99, "mp": 10, "cp": 20}]}
        game = self.match(config=Config(initial_hand=0, external_grants=True))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.confusion_step(Command(0, "sacrifice", (1,))); game.grant_step(GrantAction(0, 240))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 10)
        self.assertTrue(game.observe()["board"]["players"][0]["status"]["confused"])
        self.assertEqual(game.observe(0)["board"]["hand"], [])
        game.grant_step(GrantAction(0)); self.assertEqual(game.status.phase, "grant")
        game.grant_step(GrantAction(0)); self.assertEqual(game.status.phase, "main")
        self.assertEqual({item["token"] for item in game.observe(0)["board"]["hand"]}, {2, 3})
        self.assertEqual(game.observe()["board"]["gf"], 1)
        raw["players"][0].update(confusionCount=0, items=[{"id": 0, "modelId": 112}, {"id": 1, "modelId": 152}])
        raw["players"][1].update(curses=["darkcloud"], items=[{"id": 0, "modelId": 190}])
        raw["players"].append({"id": 23, "hp": 99, "mp": 10, "cp": 20, "curses": ["darkcloud"]})
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.step(Command(0, "sacrifice", (1,))); game.grant_step(GrantAction(0, 240))
        pending = game.observe()["pending"]
        self.assertEqual((game.actor, pending["source"], pending["death"], pending["environmental"]), (7, 0, True, False))
        self.assertIsNone(game.observe()["pending_grant"])
        game.step(Command(7, "defend", (0,))); self.assertEqual(game.actor, 23)
        game.step(Command(23, "defend"))
        # Mirror consumption creates one ordinary debt for seat7 after the dead owner's debt is canceled.
        self.assertEqual(game.observe()["pending_grant"], {"target": 7})
        game.grant_step(GrantAction(7))
        self.assertEqual((game.status.phase, game.status.owner, game.observe()["board"]["gf"]), ("main", 7, 1))
        self.assertEqual(game.observe()["board"]["players"][2]["status"]["hp"], 69)

    def test_external_grant_preserves_fate_and_standalone_devil_origins(self):
        raw = {"gf": 0, "tiebreakGF": 0, "players": [
            {"id": 0, "hp": 40, "mp": 10, "cp": 20, "items": [{"id": 0, "modelId": 207}]},
            {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
        game = self.match(config=Config(initial_hand=0, external_grants=True))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.fate_step(FateAction(0, 0, PhenomenonAction(290)))
        notice = game.observe()["last_fate"]; self.assertEqual(game.status.phase, "grant")
        game.grant_step(GrantAction(0, 244, "hp")); self.assertEqual(game.observe()["last_fate"], notice)
        game.grant_step(GrantAction(0)); self.assertEqual(game.observe()["last_fate"], notice)
        self.assertEqual((game.status.owner, game.observe()["board"]["gf"]), (7, 1))
        raw["players"][0]["items"] = []
        raw["players"][1].update(hp=5, items=[{"id": 0, "modelId": 208}])
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        game.devil_step(DevilAction(7, 240)); notice = game.observe()["last_devil"]
        self.assertEqual(game.observe()["pending_grant"], {"target": 7})
        game.grant_step(GrantAction(7, 244, "cp")); game.grant_step(GrantAction(7))
        self.assertEqual(game.observe()["last_devil"], notice)
        self.assertEqual((game.status.phase, game.status.owner, game.observe()["board"]["gf"]), ("main", 0, 0))

    def test_external_grant_strict_variants_and_late_errors_keep_pending_state(self):
        raw = {"gf": 0, "tiebreakGF": 0, "players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20,
                    "items": [{"id": 0, "modelId": 152}]}, {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
        game = self.match(config=Config(initial_hand=0, external_grants=True))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        self.assertFalse(game.grant_legal(GrantAction(0)))
        game.step(Command(0, "sacrifice", (0,))); before, status = game.observe(0), game.status
        for malformed in (None, DevilAction(0, 240), GrantAction(True), GrantAction(0, False), GrantAction(0, 0, "hp"),
                          GrantAction(0, 0, removed=(0,)), GrantAction(0, 244), GrantAction(0, 243, removed=[0])):
            with self.assertRaises(InvalidMatchError): game.grant_legal(malformed)
            with self.assertRaises(InvalidMatchError): game.grant_step(malformed)
        for invalid in (GrantAction(7), GrantAction(99), GrantAction(0, 239), GrantAction(0, 243, removed=(0,))):
            self.assertFalse(game.grant_legal(invalid))
            with self.assertRaises(MatchError): game.grant_step(invalid)
        self.assertEqual((game.observe(0), game.status), (before, status))
        for exhausted in ("update", "gf", "token"):
            altered = copy.deepcopy(raw)
            if exhausted == "update": altered["updateCount"] = 9007199254740990
            if exhausted == "gf": altered["gf"] = 9007199254740991
            if exhausted == "token": altered["players"][0]["items"].append({"id": 9007199254740991, "modelId": 6})
            with OfficialState(altered, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
            game.step(Command(0, "sacrifice", (0,))); before, status = game.observe(0), game.status
            self.assertEqual(status.phase, "grant"); self.assertTrue(game.grant_legal(GrantAction(0)))
            with self.assertRaises(InvalidMatchError): game.grant_step(GrantAction(0))
            self.assertEqual((game.observe(0), game.status), (before, status))
            self.assertNotIn(0, {item["token"] for item in before["board"]["hand"]})

    def test_sample_grant_all_ticket_boundaries_independent_reference_and_no_mutation(self):
        raw = {"gf": 0, "tiebreakGF": 0, "players": [
            {"id": 0, "hp": 99, "mp": 99, "cp": 99, "curses": ["dream"], "items": [
                {"id": 0, "modelId": 235, "used": True}, {"id": 901, "modelId": 207},
                {"id": 77, "modelId": 118, "fakeModelId": 123}, {"id": 1, "modelId": 152}]},
            {"id": 7, "hp": 99, "mp": 99, "cp": 99}]}
        game, control = [self.match(config=Config(initial_hand=0, external_grants=True)) for _ in range(2)]
        for match in (game, control):
            with OfficialState(raw, library=LIBRARY) as scene: match.load_scene(scene, 0, 42)
            match.step(Command(0, "sacrifice", (1,)))
        before, status = game.observe(0), game.status
        witnesses = {}
        for seed in range(10000):
            ticket = grant_reference(seed, ())[2]; witnesses.setdefault(ticket, seed)
            if len(witnesses) == 100: break
        self.assertEqual(len(witnesses), 100); counts = {model: 0 for model in (0, 240, 241, 242, 243, 244)}
        for ticket, seed in sorted(witnesses.items()):
            expected, rng, unused, draws = grant_reference(seed, (0, 901, 77))
            actual = game.sample_grant(seed)
            self.assertEqual(actual, (expected, rng)); self.assertEqual(game.sample_grant(seed), actual)
            self.assertTrue(game.grant_legal(actual[0])); counts[actual[0].model] += 1
        self.assertEqual(counts, {0: 75, 240: 7, 241: 5, 242: 3, 243: 5, 244: 5})
        self.assertEqual({game.sample_grant(seed)[0].resource for seed in (8, 12, 106)}, {"hp", "mp", "cp"})
        rejection_seed = 7046029254386353131
        expected, rng, ticket, draws = grant_reference(rejection_seed, (0, 901, 77))
        self.assertEqual((draws, ticket), (2, 35)); self.assertEqual(game.sample_grant(rejection_seed), (expected, rng))
        self.assertEqual(game.sample_grant(2**64-1), grant_reference(2**64-1, (0, 901, 77))[:2])
        self.assertEqual((game.observe(0), game.status), (before, status))
        for match in (game, control): match.grant_step(GrantAction(0))
        self.assertEqual((game.observe(0), game.status), (control.observe(0), control.status))

    def test_sample_grant_partial_fisher_yates_empty_single_and_full_pending_inventory(self):
        tokens = (0, 901, 77, *range(100, 114))
        for count in (0, 1, 2, 17):
            models = [235, 207, 118, 208, 209, 6] + [152]*11
            inventory = [{"id": token, "modelId": models[index], **({"used": True} if index == 0 else {}),
                          **({"fakeModelId": 123} if index == 2 else {})} for index, token in enumerate(tokens[:count])]
            raw = {"gf": 0, "tiebreakGF": 0, "players": [
                {"id": 0, "hp": 99, "mp": 99, "cp": 99, "curses": ["dream"],
                 "items": inventory + [{"id": 999, "modelId": 152}]}, {"id": 7, "hp": 99, "mp": 99, "cp": 99}]}
            game = self.match(config=Config(initial_hand=0, external_grants=True))
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
            game.step(Command(0, "sacrifice", (999,)))
            before, status = game.observe(0), game.status
            expected, rng, ticket, draws = grant_reference(5, tokens[:count])
            self.assertEqual(ticket, 18); self.assertEqual(draws, 1 + min(2, count))
            self.assertEqual(game.sample_grant(5), (expected, rng)); self.assertTrue(game.grant_legal(expected))
            seen = set()
            for seed in range(50000 if count else 0):
                reference, state, unused, unused_draws = grant_reference(seed, tokens[:count])
                if reference.model != 243 or not (set(reference.removed)-seen): continue
                self.assertEqual(game.sample_grant(seed), (reference, state)); seen.update(reference.removed)
                if seen == set(tokens[:count]): break
            self.assertEqual(seen, set(tokens[:count])); self.assertEqual((game.observe(0), game.status), (before, status))
            game.grant_step(expected)
            self.assertEqual(game.observe()["last_grant"]["removed_models"],
                             [next(item["modelId"] for item in inventory if item["id"] == token) for token in expected.removed])
            self.assertEqual(game.status.phase, "grant")

    def test_sample_grant_wrong_phases_strict_seeds_and_future_failure_are_atomic(self):
        game = self.match(config=Config(initial_hand=0, external_grants=True))
        raw = {"gf": 0, "tiebreakGF": 0, "players": [
            {"id": 0, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 6}, {"id": 1, "modelId": 5}]},
            {"id": 7, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 152}]}]}
        for kind in ("main", "response", "purchase"):
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
            if kind == "response": game.step(Command(0, "play", (0,), 7))
            if kind == "purchase": game.step(Command(0, "buy", (1,), 7)); game.step(Command(7, "defend"))
            self.assertEqual(game.status.phase, kind); before, status = game.observe(0), game.status
            with self.assertRaises(InvalidMatchError): game.sample_grant(0)
            self.assertEqual((game.observe(0), game.status), (before, status))
        cutoff = self.match(config=Config(initial_hand=0, external_grants=True, max_decisions=1))
        cutoff.step(Command(0, "pray")); self.assertTrue(cutoff.done)
        with self.assertRaises(InvalidMatchError): cutoff.sample_grant(0)
        raw["players"][0]["items"] = [{"id": 0, "modelId": 152}, {"id": 9007199254740991, "modelId": 6}]
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        game.step(Command(0, "sacrifice", (0,))); before, status = game.observe(0), game.status
        for invalid in (True, False, -1, 2**64, 0.0, "0", None):
            with self.assertRaises(InvalidMatchError): game.sample_grant(invalid)
        selected, state = game.sample_grant(0)
        self.assertEqual(selected, GrantAction(0)); self.assertEqual(state, 11400714819323198485)
        self.assertTrue(game.grant_legal(selected))
        with self.assertRaises(InvalidMatchError): game.grant_step(selected)
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.close()
        with self.assertRaises(InvalidMatchError): game.sample_grant(0)

    def test_include_fate_configured_pool_and_base_command_descriptors_are_distinct(self):
        data = OfficialData(LIBRARY)
        base = self.match(seed=42); explicit = self.match(seed=42, config=Config(include_fate=False))
        self.assertEqual((base.observe(0), base.status), (explicit.observe(0), explicit.status))
        for flag in (False, True):
            game = self.match(config=Config(include_fate=flag))
            expected = tuple(card.id for card in data if card.gift_rate > 0) if flag else game.supported_models
            self.assertEqual((len(game.gift_models), game.gift_weight), (237, 500) if flag else (236, 499))
            self.assertEqual(game.gift_models, expected); self.assertEqual(len(game.supported_models), 236)
            self.assertNotIn(207, game.supported_models); self.assertEqual(207 in game.gift_models, flag)
            self.assertEqual(game.observe()["gift_pool"], {"include_fate": flag, "models": len(expected), "weight": game.gift_weight})
            self.assertEqual(sum(data.get_card(model).gift_rate for model in game.gift_models), game.gift_weight)
            self.assertNotIn(207, game.supported_guardian_payload_models["earth"])
            self.assertNotIn(207, game.supported_guardian_payload_models["moon"])
        for invalid in (0, 1, None, "true"):
            with self.assertRaises(InvalidMatchError): self.match(config=Config(include_fate=invalid))

    def test_include_fate_reset_and_dream_prayer_draw207_unmasked_then_use_fate(self):
        reset = self.match(seed=1464, config=Config(initial_hand=1, include_fate=True))
        self.assertEqual([(item["token"], item["face"], item["disguised"], item["used"]) for item in reset.observe(0)["board"]["hand"]],
                         [(0, 207, False, False)])
        raw = {"gf": 0, "players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "curses": ["dream"]},
                                    {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
        game = self.match(config=Config(initial_hand=0, include_fate=True))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 1464)
        game.step(Command(0, "pray")); hand = game.observe(0)["board"]["hand"]
        self.assertEqual([(i["token"], i["face"], i["reference"], i["disguised"], i["used"]) for i in hand], [(0, 207, 207, False, False)])
        game.step(Command(7, "pray"))
        self.assertEqual(game.observe(7)["board"]["hand"][0]["face"], 231)  # next bounded500, no207 Dream coin draw
        before, status = game.observe(0), game.status
        self.assertFalse(game.legal(Command(0, "play", (0,), 0)))
        for rng in range(64):
            command, unused = game.sample_action(rng)
            self.assertFalse(command.kind == "play" and 0 in command.items); self.assertTrue(game.legal(command))
        self.assertEqual((game.observe(0), game.status), (before, status))
        action = FateAction(0, 0, PhenomenonAction(290)); self.assertTrue(game.fate_legal(action)); game.fate_step(action)
        self.assertEqual(game.observe()["last_fate"], {"actor": 0, "model": 207, "phenomenon": 290})
        self.assertNotIn(0, {item["token"] for item in game.observe(0)["board"]["hand"]})

    def test_include_fate_immediate_redraw_and_revival_replacement_use_same_pool(self):
        game = self.match(config=Config(initial_hand=0, include_fate=True))
        raw = {"gf": 0, "players": [{"id": 0, "hp": 99, "mp": 10, "cp": 20, "items": [{"id": 0, "modelId": 6}]},
                  {"id": 7, "hp": 99, "mp": 10, "cp": 20, "items": [{"id": 0, "modelId": 178}, {"id": 1, "modelId": 152}]}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 1464)
        game.step(Command(0, "play", (0,), 7)); game.step(Command(7, "defend", (0,)))
        drawn = next(item for item in game.observe(7)["board"]["hand"] if item["token"] == 2)
        self.assertEqual((drawn["face"], drawn["disguised"], drawn["used"]), (207, False, False))
        self.assertEqual(game.observe()["last_redraws"], [7])
        self.assertEqual(game.observe(0)["board"]["hand"][0]["face"], 231)
        self.assertTrue(game.fate_legal(FateAction(7, 2, PhenomenonAction(290))))
        raw["players"][0].update(hp=40, mp=10, cp=20, items=[{"id": 0, "modelId": 3}, {"id": 1, "modelId": 208}])
        raw["players"][1]["items"] = []
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 442)
        game.step(Command(0, "exchange", (0,), allocation=(0, 50, 20)))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 10)
        hand = game.observe(0)["board"]["hand"]
        self.assertEqual({item["token"]: item["face"] for item in hand}, {2: 4, 3: 207})
        self.assertTrue(all(not item["used"] and not item["disguised"] for item in hand))

    def test_include_fate_external_normal_grant_uses500_pool_without_exposing_model(self):
        game = self.match(config=Config(initial_hand=0, include_fate=True, external_grants=True))
        raw = {"gf": 0, "tiebreakGF": 0, "players": [{"id": 0, "hp": 40, "mp": 10, "cp": 20, "curses": ["dream"]},
                                                     {"id": 7, "hp": 40, "mp": 10, "cp": 20}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 1464)
        game.step(Command(0, "pray")); self.assertEqual(game.status.phase, "grant")
        chosen, chance_rng = game.sample_grant(0); self.assertEqual(chosen, GrantAction(0))
        game.grant_step(chosen)
        hand = game.observe(0)["board"]["hand"]
        self.assertEqual([(item["face"], item["disguised"], item["used"]) for item in hand], [(207, False, False)])
        self.assertEqual(game.observe()["last_grant"], {"target": 0, "devil": None, "removed_models": []})
        self.assertEqual((len(game.gift_models), game.gift_weight), (237, 500))
        game.reset(1464)
        self.assertEqual(game.observe()["gift_pool"], {"include_fate": True, "models": 237, "weight": 500})

    def test_sample_fate_all_outcomes_targets_guardians_and_reference_nonmutation(self):
        raw = {"gf": 0, "players": [
            {"id": 0, "team": 1, "hp": 99, "mp": 99, "cp": 99, "confusionCount": 1, "curses": ["dream"], "items": [
                {"id": 0, "modelId": 207}, {"id": 2, "modelId": 235, "used": True}, {"id": 9, "modelId": 207},
                {"id": 5, "modelId": 118, "fakeModelId": 123}]},
            {"id": 7, "team": 1, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 152}, {"id": 1, "modelId": 235, "used": True}]},
            {"id": 42, "team": 2, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 6}, {"id": 3, "modelId": 209}]},
            {"id": 23, "team": 2, "hp": 0, "mp": 99, "cp": 99, "dead": True, "items": [{"id": 0, "modelId": 208}]}]}
        game, control = [self.match(config=Config(initial_hand=0)) for _ in range(2)]
        for match in (game, control):
            with OfficialState(raw, library=LIBRARY) as scene: match.load_scene(scene, 0, 42)
        before, status = game.observe(0), game.status; models, recipients, guardians = set(), set(), set()
        for seed in (*range(256), 2**64-1, 7046029254386353131):
            expected, rng, draws = fate_reference(seed, 0, raw)
            self.assertEqual(game.sample_fate(0, seed), (expected, rng)); self.assertTrue(game.fate_legal(expected))
            self.assertEqual(game.sample_fate(0, seed), (expected, rng)); models.add(expected.outcome.model)
            if expected.outcome.recipient is not None: recipients.add(expected.outcome.recipient)
            guardians.update(expected.outcome.guardians)
            if expected.shuffle:
                self.assertEqual(set(expected.shuffle.order), {ItemSource(0, 2), ItemSource(0, 9), ItemSource(0, 5),
                    ItemSource(7, 0), ItemSource(7, 1), ItemSource(42, 0), ItemSource(42, 3)})
        self.assertEqual(models, set(range(287, 297))); self.assertEqual(recipients, {0, 7, 42})
        self.assertEqual(len(guardians), 10); self.assertEqual((game.observe(0), game.status), (before, status))
        for match in (game, control): match.fate_step(FateAction(0, 0, PhenomenonAction(290)))
        self.assertEqual((game.observe(0), game.status), (control.observe(0), control.status))
        self.assertFalse(game.observe()["board"]["players"][0]["status"]["confused"])

    def test_sample_fate_empty_singleton_and161_source_manifest_are_owned_copies(self):
        for count in (0, 1, 161):
            if count < 2:
                players = [{"id": 0, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 207}]},
                           {"id": 7, "hp": 99, "mp": 99, "cp": 99,
                            "items": [{"id": 0, "modelId": 235, "used": True}] if count else []}]
            else:
                players = [{"id": seat*17, "hp": 99, "mp": 99, "cp": 99, "items": [
                    {"id": token, "modelId": 207 if seat == 0 and token == 0 else 235,
                     "used": not (seat == 0 and token == 0)} for token in range(18)]} for seat in range(9)]
            raw = {"players": players}; game = self.match(config=Config(initial_hand=0))
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
            before, status = game.observe(0), game.status
            expected, rng, draws = fate_reference(4, 0, raw)
            chosen, actual_rng = game.sample_fate(0, 4)
            self.assertEqual((chosen, actual_rng), (expected, rng)); self.assertEqual(chosen.outcome.model, 295)
            self.assertEqual(len(chosen.shuffle.order), count); self.assertEqual(draws, 1 + max(0, count-1))
            self.assertNotIn(ItemSource(0, 0), chosen.shuffle.order); self.assertTrue(game.fate_legal(chosen))
            game.sample_fate(0, 17)  # A fresh native buffer must not alter the previous returned tuple.
            self.assertEqual(chosen, expected); self.assertEqual((game.observe(0), game.status), (before, status))
            game.fate_step(chosen); self.assertEqual(game.observe()["last_fate"]["phenomenon"], 295)

    def test_sample_fate_invalid_inputs_overflow_without_reroll_and_future_step_failure(self):
        raw = {"gf": 0, "tiebreakGF": 0, "players": [
            {"id": 0, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 207},
                {"id": 1, "modelId": 6}, {"id": 2, "modelId": 5}, {"id": 3, "modelId": 152}]},
            {"id": 7, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 152}]}]}
        game = self.match(config=Config(initial_hand=0, external_grants=True))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        before, status = game.observe(0), game.status
        for invalid in (True, False, -1, 9007199254740992, 0.0, "0", None):
            with self.assertRaises(InvalidMatchError): game.sample_fate(invalid, 4)
        for invalid in (True, False, -1, 2**64, 0.0, "0", None):
            with self.assertRaises(InvalidMatchError): game.sample_fate(0, invalid)
        for token in (1, 2, 999):
            with self.assertRaises(InvalidMatchError): game.sample_fate(token, 4)
        self.assertEqual((game.observe(0), game.status), (before, status))
        for phase in ("response", "purchase", "grant"):
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
            if phase == "response": game.step(Command(0, "play", (1,), 7))
            elif phase == "purchase": game.step(Command(0, "buy", (2,), 7)); game.step(Command(7, "defend"))
            else: game.step(Command(0, "sacrifice", (3,)))
            self.assertEqual(game.status.phase, phase); before, status = game.observe(0), game.status
            with self.assertRaises(InvalidMatchError): game.sample_fate(0, 4)
            self.assertEqual((game.observe(0), game.status), (before, status))
        cut = self.match(config=Config(initial_hand=0, max_decisions=1))
        with OfficialState(raw, library=LIBRARY) as scene: cut.load_scene(scene, 0, 42)
        cut.step(Command(0, "play", (1,), 7))
        with self.assertRaises(InvalidMatchError): cut.sample_fate(0, 4)
        overflow = copy.deepcopy(raw); overflow["players"][1]["confusionCount"] = 9007199254740991
        with OfficialState(overflow, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        before, status = game.observe(0), game.status
        self.assertEqual(fate_reference(6, 0, overflow)[0].outcome.model, 289)
        with self.assertRaises(InvalidMatchError): game.sample_fate(0, 6)
        self.assertEqual((game.observe(0), game.status), (before, status))
        raw["players"][0]["items"] = [{"id": 0, "modelId": 207}, {"id": 9007199254740991, "modelId": 6}]
        raw["players"][1]["items"] = []; raw["players"][0]["confusionCount"] = 1
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        before, status = game.observe(0), game.status; chosen, chance_rng = game.sample_fate(0, 4)
        self.assertEqual(chosen.outcome.model, 295); self.assertTrue(game.fate_legal(chosen))
        with self.assertRaises(InvalidMatchError): game.fate_step(chosen)
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.close()
        with self.assertRaises(InvalidMatchError): game.sample_fate(0, 4)

    def test_sample_confusion_matches_count0_visible_policy_including_free_trade_and_dream(self):
        source = {"id": 0, "team": 1, "hp": 99, "mp": 0, "cp": 99, "confusionCount": 1,
                  "curses": ["fog", "dream"], "items": [
            {"id": 0, "modelId": 6}, {"id": 1, "modelId": 231}, {"id": 2, "modelId": 204},
            {"id": 3, "modelId": 143}, {"id": 4, "modelId": 3}, {"id": 5, "modelId": 4},
            {"id": 6, "modelId": 5}, {"id": 7, "modelId": 235, "used": True}, {"id": 8, "modelId": 83},
            {"id": 9, "modelId": 118, "fakeModelId": 123}, {"id": 10, "modelId": 58},
            {"id": 11, "modelId": 232}, {"id": 12, "modelId": 14}, {"id": 13, "modelId": 207},
            {"id": 14, "modelId": 53}, {"id": 15, "modelId": 238}, {"id": 16, "modelId": 152}, {"id": 17, "modelId": 234}]}
        raw = {"gf": 0, "tiebreakGF": 0, "players": [source,
            {"id": 7, "team": 1, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 152}]},
            {"id": 42, "team": 2, "hp": 99, "mp": 99, "cp": 99}]}
        normal_raw = copy.deepcopy(raw); normal_raw["players"][0]["confusionCount"] = 0
        confused, normal = self.match(config=Config(initial_hand=0)), self.match(config=Config(initial_hand=0))
        for game, scene_raw in ((confused, raw), (normal, normal_raw)):
            with OfficialState(scene_raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        before, status = confused.observe(0), confused.status; normal_before = normal.observe(0)
        seen, free_pair, chosen_sacrifice = set(), False, None
        for seed in range(4096):
            selected, rng = confused.sample_confusion(seed); reference, normal_rng = normal.sample_action(seed)
            self.assertEqual((selected, rng), (reference, normal_rng)); self.assertTrue(confused.confusion_legal(selected))
            self.assertTrue(normal.legal(selected)); self.assertFalse(confused.legal(selected))
            self.assertNotEqual(selected.kind, "discard"); seen.add(selected.kind)
            if selected.kind == "play":
                self.assertNotIn(13, selected.items)
                free_pair |= bool(set(selected.items) & {2, 3}) and bool(set(selected.items) & {1, 7, 11})
            if selected.kind == "sacrifice": chosen_sacrifice = selected
            if {"play", "exchange", "sell", "buy", "sacrifice"} <= seen and free_pair: break
        self.assertTrue({"play", "exchange", "sell", "buy", "sacrifice"} <= seen); self.assertTrue(free_pair)
        self.assertEqual((confused.observe(0), confused.status), (before, status)); self.assertEqual(normal.observe(0), normal_before)
        with self.assertRaises(UnsupportedMatchError): confused.sample_action(42)
        confused.confusion_step(chosen_sacrifice); normal.step(chosen_sacrifice)
        self.assertEqual((confused.observe(0), confused.status), (normal.observe(0), normal.status))
        self.assertFalse(confused.observe()["board"]["players"][0]["status"]["confused"])

    def test_sample_confusion_bounded_rejection_strict_seed_and_late_failure(self):
        raw = {"gf": 0, "players": [{"id": 0, "hp": 99, "mp": 0, "cp": 99, "confusionCount": 1,
                    "items": [{"id": 0, "modelId": 152}, {"id": 1, "modelId": 118}]},
                    {"id": 7, "hp": 99, "mp": 0, "cp": 99}]}
        game = self.match(config=Config(initial_hand=0))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        before, status = game.observe(0), game.status
        self.assertEqual(game.sample_confusion(7046029254386353131), (Command(0, "discard", (0,)), 11400714819323198485))
        for invalid in (True, False, -1, 2**64, 0.0, "0", None):
            with self.assertRaises(InvalidMatchError): game.sample_confusion(invalid)
        self.assertEqual(game.sample_confusion(2**64-1), game.sample_confusion(2**64-1))
        self.assertEqual((game.observe(0), game.status), (before, status))
        for exhausted in ("update", "gf", "token"):
            altered = copy.deepcopy(raw); altered["tiebreakGF"] = 0
            altered["players"][0]["items"] = [{"id": 0, "modelId": 152}]
            if exhausted == "update": altered["updateCount"] = 9007199254740991
            if exhausted == "gf": altered["gf"] = 9007199254740991
            if exhausted == "token": altered["players"][0]["items"].append({"id": 9007199254740991, "modelId": 6})
            with OfficialState(altered, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
            before, status = game.observe(0), game.status
            for seed in range(64):
                selected, next_rng = game.sample_confusion(seed)
                if selected.kind == "sacrifice": break
            self.assertEqual(selected.kind, "sacrifice"); self.assertTrue(game.confusion_legal(selected))
            with self.assertRaises(InvalidMatchError): game.confusion_step(selected)
            self.assertEqual((game.observe(0), game.status), (before, status))
        game.close()
        with self.assertRaises(InvalidMatchError): game.sample_confusion(0)

    def test_sample_confusion_rejects_nonconfused_main_and_every_nonmain_phase(self):
        raw = {"gf": 0, "tiebreakGF": 0, "players": [
            {"id": 0, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 6}, {"id": 1, "modelId": 5}, {"id": 2, "modelId": 152}]},
            {"id": 7, "hp": 99, "mp": 99, "cp": 99, "confusionCount": 2, "items": [{"id": 0, "modelId": 152}]}]}
        game = self.match(config=Config(initial_hand=0, external_grants=True))
        for phase in ("main", "response", "purchase", "grant"):
            scene_raw = copy.deepcopy(raw)
            if phase in ("purchase", "grant"): scene_raw["players"][0]["confusionCount"] = 2
            with OfficialState(scene_raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
            if phase == "response": game.step(Command(0, "play", (0,), 7))
            elif phase == "purchase": game.confusion_step(Command(0, "buy", (1,), 7)); game.step(Command(7, "defend"))
            elif phase == "grant": game.confusion_step(Command(0, "sacrifice", (2,)))
            self.assertEqual(game.status.phase, phase); before, status = game.observe(0), game.status
            with self.assertRaises(InvalidMatchError): game.sample_confusion(0)
            self.assertEqual((game.observe(0), game.status), (before, status))
            if phase in ("response", "purchase"):
                selected, unused = game.sample_action(42); self.assertTrue(game.legal(selected))
        cutoff = self.match(config=Config(initial_hand=0, max_decisions=1))
        raw["players"][0]["confusionCount"] = 2
        with OfficialState(raw, library=LIBRARY) as scene: cutoff.load_scene(scene, 0, 42)
        cutoff.confusion_step(Command(0, "sacrifice", (2,))); self.assertTrue(cutoff.status.truncated)
        self.assertTrue(cutoff.observe()["board"]["players"][0]["status"]["confused"])
        with self.assertRaises(InvalidMatchError): cutoff.sample_confusion(0)

    def test_sample_guardian_payload_matches_independent_domain_and_rng_reference(self):
        cards = {card["id"]: card for card in json.loads((Path(__file__).resolve().parents[1]/"data/official-v2/cards.json").read_text())}
        contract = json.loads((Path(__file__).resolve().parents[1]/"data/official-v2/guardian-payload-client-v33.json").read_text())["local_adapter_contract"]
        bases = {guardian: contract[guardian+"_base_ids"] for guardian in ("earth", "moon")}
        raw = {"players": [{"id": 0, "team": 2, "hp": 99, "mp": 0, "cp": 0, "confusionCount": 1},
            {"id": 17, "team": 1, "hp": 99, "mp": 0, "cp": 0, "confusionCount": 7, "guardian": "earth"},
            {"id": 23, "team": 1, "hp": 99, "mp": 0, "cp": 0}, {"id": 42, "team": 2, "hp": 99, "mp": 0, "cp": 0},
            {"id": 99, "team": 2, "hp": 0, "mp": 0, "cp": 0, "dead": True}]}
        game = self.match(config=Config(initial_hand=0)); targets = set()
        for guardian in ("earth", "moon"):
            raw["players"][1]["guardian"] = guardian
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
            self.assertEqual(tuple(bases[guardian]), game.supported_guardian_payload_models[guardian])
            before, status = game.observe(17), game.status; witnesses = {}
            for seed in range(10000):
                expected, rng, draws = guardian_payload_reference(seed, 17, raw, cards, bases)
                witnesses.setdefault(expected.models[0], seed)
                if len(witnesses) == len(bases[guardian]): break
            self.assertEqual(set(witnesses), set(bases[guardian]))
            for seed in (*witnesses.values(), 0, 1, 6, 42, 201, 38596, 7046029254386353131, 2**64-1):
                expected, rng, draws = guardian_payload_reference(seed, 17, raw, cards, bases)
                self.assertEqual(game.sample_guardian_payload(17, seed), (expected, rng))
                self.assertTrue(game.guardian_payload_legal(expected)); targets.add(expected.target)
                self.assertNotEqual(expected.target, 99)
                if guardian == "moon": self.assertEqual(len(expected.models), 1)
            self.assertEqual((game.observe(17), game.status), (before, status))
        self.assertEqual(targets, {0, 17, 23, 42})
        raw["players"][1]["guardian"] = "earth"
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        maximal, rng = game.sample_guardian_payload(17, 38596)
        self.assertEqual(len(maximal.models), 18); self.assertLess(len(set(maximal.models)), 18)
        self.assertIn(150, maximal.models)
        self.assertEqual(game.sample_guardian_payload(17, 6)[0].models, (38, 162))
        for player in raw["players"]: player["team"] = 0
        raw["players"][1]["guardian"] = "moon"
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        for seed in range(32):
            expected, rng, draws = guardian_payload_reference(seed, 17, raw, cards, bases)
            self.assertEqual(game.sample_guardian_payload(17, seed), (expected, rng))
            if expected.target == 17: self.assertEqual(draws, 2)  # singleton ally bucket still draws
        raw["players"][0]["guardian"] = "earth"
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        expected, rng, draws = guardian_payload_reference(0, 0, raw, cards, bases)
        self.assertEqual(game.sample_guardian_payload(0, 0), (expected, rng))

    def test_sample_guardian_payload_ignores_mp_inventory_pool_and_owner_for_choice(self):
        raw = {"players": [{"id": 0, "team": 2, "hp": 99, "mp": 99, "cp": 99, "confusionCount": 1},
            {"id": 17, "team": 1, "hp": 99, "mp": 0, "cp": 0, "guardian": "earth", "confusionCount": 3},
            {"id": 23, "team": 1, "hp": 99, "mp": 99, "cp": 99}, {"id": 42, "team": 2, "hp": 99, "mp": 99, "cp": 99}]}
        high = copy.deepcopy(raw); high["players"][1].update(mp=99, cp=99, items=[{"id": i, "modelId": 152} for i in range(18)])
        low_game = self.match(config=Config(initial_hand=0)); high_game = self.match(config=Config(initial_hand=0, include_fate=True))
        for game, scene_raw in ((low_game, raw), (high_game, high)):
            with OfficialState(scene_raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        before = (low_game.observe(17), high_game.observe(17))
        for seed in range(128): self.assertEqual(low_game.sample_guardian_payload(17, seed), high_game.sample_guardian_payload(17, seed))
        self.assertEqual((low_game.observe(17), high_game.observe(17)), before)
        for game, power in ((low_game, 0), (high_game, 198)):
            chosen, rng = game.sample_guardian_payload(17, 201)
            self.assertEqual(chosen, GuardianPayloadAction(17, "earth", (58,), 0)); self.assertTrue(game.guardian_payload_legal(chosen))
            hand = game.observe(17)["board"]["hand"]
            game.guardian_payload_step(chosen); view = game.observe()
            self.assertEqual((game.status.owner, game.actor, view["pending"]["source"], view["pending"]["power"]), (0, 0, 17, power))
            self.assertEqual(view["board"]["players"][1]["status"]["mp"], 0)
            self.assertEqual(view["board"]["players"][0]["status"]["mp"], 99)
            self.assertEqual(game.observe(17)["board"]["hand"], hand)
            self.assertTrue(view["board"]["players"][0]["status"]["confused"])

    def test_sample_guardian_payload_bad_sources_phases_and_late_step_failure_are_atomic(self):
        raw = {"gf": 0, "tiebreakGF": 0, "players": [{"id": 0, "hp": 99, "mp": 99, "cp": 99,
                 "items": [{"id": 0, "modelId": 6}, {"id": 1, "modelId": 5}, {"id": 2, "modelId": 152}]},
            {"id": 17, "hp": 99, "mp": 99, "cp": 99, "guardian": "earth", "items": [{"id": 0, "modelId": 152}]},
            {"id": 23, "hp": 99, "mp": 99, "cp": 99, "guardian": "mars"},
            {"id": 42, "hp": 0, "mp": 99, "cp": 99, "guardian": "moon", "dead": True}]}
        game = self.match(config=Config(initial_hand=0, external_grants=True))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        before, status = game.observe(), game.status
        for bad_source in (True, False, -1, 9007199254740992, "17", None, 0, 23, 42, 999):
            with self.assertRaises(InvalidMatchError): game.sample_guardian_payload(bad_source, 0)
        for bad_rng in (True, False, -1, 2**64, 0.0, "0", None):
            with self.assertRaises(InvalidMatchError): game.sample_guardian_payload(17, bad_rng)
        self.assertEqual((game.observe(), game.status), (before, status))
        for phase in ("response", "purchase", "grant"):
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
            if phase == "response": game.step(Command(0, "play", (0,), 17))
            elif phase == "purchase": game.step(Command(0, "buy", (1,), 17)); game.step(Command(17, "defend"))
            else: game.step(Command(0, "sacrifice", (2,)))
            self.assertEqual(game.status.phase, phase); before, status = game.observe(), game.status
            with self.assertRaises(InvalidMatchError): game.sample_guardian_payload(17, 0)
            self.assertEqual((game.observe(), game.status), (before, status))
        cutoff = self.match(config=Config(initial_hand=0, max_decisions=1))
        with OfficialState(raw, library=LIBRARY) as scene: cutoff.load_scene(scene, 0, 42)
        cutoff.step(Command(0, "play", (0,), 17))
        with self.assertRaises(InvalidMatchError): cutoff.sample_guardian_payload(17, 0)
        raw["updateCount"] = 9007199254740991
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        before, status = game.observe(17), game.status; selected, rng = game.sample_guardian_payload(17, 201)
        self.assertTrue(game.guardian_payload_legal(selected))
        with self.assertRaises(InvalidMatchError): game.guardian_payload_step(selected)
        self.assertEqual((game.observe(17), game.status), (before, status))
        game.close()
        with self.assertRaises(InvalidMatchError): game.sample_guardian_payload(17, 0)

    def test_local_guardian_payload_config_is_strict_default_false_and_reset_stable(self):
        for bad in (0, 1, None, "true", [], {}):
            with self.assertRaises(InvalidMatchError): self.match(config=Config(local_guardian_payloads=bad))
        default = self.match(config=Config(initial_hand=0))
        explicit = self.match(config=Config(initial_hand=0, local_guardian_payloads=False))
        raw = {"gf": 7, "players": [{"id": 0, "hp": 99, "mp": 0, "cp": 0, "curses": ["cold"]},
                                    {"id": 7, "hp": 99, "mp": 0, "cp": 0, "guardian": "moon"}]}
        for game in (default, explicit):
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 0)
        self.assertEqual(default.observe(), explicit.observe())
        for command in (Command(0, "pray"), Command(0, "defend")):
            self.assertEqual(default.step(command), explicit.step(command))
            self.assertEqual(default.observe(), explicit.observe())
            self.assertFalse(default.observe()["local_guardian_payloads"])
            self.assertEqual(default.observe()["last_auto_guardian_payloads"], [])
        self.assertEqual(default.observe()["last_auto_guardians"][0]["model"], 286)
        enabled = self.match(config=Config(initial_hand=0, local_guardian_payloads=True))
        self.assertTrue(enabled.observe()["local_guardian_payloads"])
        enabled.reset(123)
        self.assertTrue(enabled.observe()["local_guardian_payloads"])
        self.assertEqual(enabled.observe()["last_auto_guardian_payloads"], [])
        self.assertIn("experimental:", enabled.observe()["guardian_payload_auto_rule"])

    def test_local_guardian_auto_payload_uses_game_rng_once_without_source_cost_or_gift(self):
        cards = {card["id"]: card for card in json.loads((Path(__file__).resolve().parents[1]/"data/official-v2/cards.json").read_text())}
        contract = json.loads((Path(__file__).resolve().parents[1]/"data/official-v2/guardian-payload-client-v33.json").read_text())["local_adapter_contract"]
        bases = {guardian: contract[guardian+"_base_ids"] for guardian in ("earth", "moon")}
        raw = {"gf": 7, "players": [
            {"id": 0, "team": 2, "hp": 99, "mp": 0, "cp": 0, "items": [{"id": i, "modelId": 235, "used": True} for i in range(18)]},
            {"id": 17, "team": 1, "hp": 99, "mp": 0, "cp": 0, "guardian": "earth"},
            {"id": 23, "team": 1, "hp": 99, "mp": 0, "cp": 0}, {"id": 42, "team": 2, "hp": 99, "mp": 0, "cp": 0}]}
        game = self.match(config=Config(initial_hand=0, local_guardian_payloads=True))
        for guardian in ("earth", "moon"):
            raw["players"][1]["guardian"] = guardian
            with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 2)
            # Seed2's first bounded100 is10: opportunity succeeds. The full
            # owner prayer owes no gift, so the following payload uses this RNG.
            expected, after, draws = guardian_payload_reference((2+0x9e3779b97f4a7c15) % 2**64, 17, raw, cards, bases)
            self.assertEqual(expected.models, (32, 9, 31) if guardian == "earth" else (236,))
            before_hand = game.observe(0)["board"]["hand"]
            _, external_rng = game.sample_guardian_payload(17, 123)
            self.assertEqual(game.sample_guardian_payload(17, 123)[1], external_rng)
            game.step(Command(0, "pray")); view = game.observe()
            event = {"source": 17, "guardian": guardian, "models": list(expected.models), "target": expected.target}
            self.assertEqual(view["last_auto_guardian_payloads"], [event])
            self.assertIsNone(view["last_guardian_payload"])
            self.assertEqual(view["last_auto_guardians"], [])
            self.assertEqual((game.status.decisions, game.status.owner, game.actor, view["board"]["gf"]), (1, 0, expected.target, 7))
            self.assertEqual((view["pending"]["models"], view["pending"]["source"], view["pending"]["guardian"]), (list(expected.models), 17, guardian))
            self.assertEqual(game.observe(17)["board"]["hand"], [])
            self.assertEqual(view["board"]["players"][1]["status"]["mp"], 0)
            before, status = game.observe(), game.status
            with self.assertRaises(InvalidMatchError): game.step(Command(999, "defend"))
            self.assertEqual((game.observe(), game.status), (before, status))
            game.step(Command(expected.target, "defend")); view = game.observe()
            self.assertEqual((game.status.decisions, game.status.owner, game.actor, view["board"]["gf"]), (2, 17, 17, 8))
            self.assertEqual(game.observe(0)["board"]["hand"], before_hand)
            self.assertEqual(game.observe(17)["board"]["hand"], [])
            self.assertEqual(view["last_auto_guardian_payloads"], [event])
            statuses = {player["id"]: player["status"] for player in view["board"]["players"]}
            self.assertEqual(statuses[42]["hp"], 81 if guardian == "earth" else 99)
            self.assertEqual(statuses[23]["cp"], 10 if guardian == "moon" else 0)

    def test_player_choice_descriptors_envelope_and_ordinary_sampler_parity(self):
        game = self.match(); before, status = game.observe(0), game.status
        self.assertEqual(game.driver_api_version, 1)
        self.assertTrue(game.choice_rule.startswith("experimental:")); self.assertTrue(game.driver_rule.startswith("experimental:"))
        for seed in (*range(64), 7046029254386353131, 2**64-1):
            self.assertEqual(game.sample_choice(seed), game.sample_action(seed))
        self.assertEqual((game.observe(0), game.status), (before, status))
        raw = {"players": [{"id": 0, "hp": 99, "mp": 99, "cp": 99,
                            "items": [{"id": 0, "modelId": 207}, {"id": 7, "modelId": 207}]},
                           {"id": 23, "hp": 99, "mp": 99, "cp": 99}]}
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        before, status = game.observe(0), game.status; kinds = set()
        for seed in (*range(64), 7046029254386353131):
            branch, next_rng = choice_envelope_reference(seed, 3)
            expected = (FateRequest(0, (0, 7)[branch-1]), next_rng) if branch else game.sample_action(next_rng)
            choice, rng = game.sample_choice(seed)
            self.assertEqual((choice, rng), expected); self.assertTrue(game.choice_legal(choice))
            kinds.add(choice.token if isinstance(choice, FateRequest) else "command")
        self.assertEqual(kinds, {0, 7, "command"})
        self.assertEqual((game.observe(0), game.status), (before, status))
        self.assertTrue(game.choice_legal(FateRequest(0, 0)))
        for choice in (FateRequest(23, 0), FateRequest(0, 99)): self.assertFalse(game.choice_legal(choice))

    def test_rollout_step_matches_explicit_dispatch_in_all_live_phases(self):
        basic = {"players": [{"id": 0, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 207}]},
                              {"id": 7, "hp": 99, "mp": 99, "cp": 99}]}
        for phase in ("fate-main", "confused-main", "response", "purchase", "grant"):
            raw = copy.deepcopy(basic); commands = []
            if phase == "confused-main": raw["players"][0].update(confusionCount=2, items=[])
            if phase in ("response", "purchase"):
                raw["players"][0]["items"] = [{"id": 0, "modelId": 5}]
                raw["players"][1]["items"] = [{"id": 1, "modelId": 6}]
                commands = [Command(0, "buy", (0,), 7)]
                if phase == "purchase": commands.append(Command(7, "defend"))
            if phase == "grant":
                raw.update(gf=0, tiebreakGF=0); raw["players"][0]["items"] = []
                commands = [Command(0, "pray")]
            manual = self.match(config=Config(initial_hand=0, external_grants=True))
            driven = self.match(config=Config(initial_hand=0, external_grants=True))
            for game in (manual, driven):
                with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
                for command in commands: game.step(command)
            policy, chance = 0, 2; old_decisions = manual.status.decisions
            if phase == "grant":
                self.assertEqual(manual.status.phase, "grant")
                grant, next_chance = manual.sample_grant(chance)
                expected_status, next_policy = manual.grant_step(grant), policy
            elif phase == "confused-main":
                command, next_chance = manual.sample_confusion(chance)
                expected_status, next_policy = manual.confusion_step(command), policy
            else:
                choice, next_policy = manual.sample_choice(policy)
                if phase == "fate-main": self.assertIsInstance(choice, FateRequest)
                expected_status, next_chance = manual.choice_step(choice, chance)
            self.assertEqual(driven.rollout_step(policy, chance), (expected_status, next_policy, next_chance))
            self.assertEqual(driven.status.decisions, old_decisions+1)
            self.assertEqual(driven.observe(), manual.observe())
            self.assertEqual(driven.observe(0), manual.observe(0))

    def test_choice_driver_atomic_failure_and_rollout_error_preserves_progress(self):
        raw = {"players": [{"id": 0, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 207}]},
                           {"id": 7, "hp": 99, "mp": 99, "cp": 99, "confusionCount": 9007199254740991}]}
        game = self.match(config=Config(initial_hand=0))
        with OfficialState(raw, library=LIBRARY) as scene: game.load_scene(scene, 0, 42)
        self.assertTrue(game.choice_legal(FateRequest(0, 0)))
        before, status = game.observe(0), game.status
        # policy0 chooses the request; chance6 selects289, whose +3 is invalid.
        with self.assertRaises(InvalidMatchError): game.rollout_step(0, 6)
        self.assertEqual((game.observe(0), game.status), (before, status))
        with self.assertRaises(RolloutError) as caught: game.chance_rollout(0, 6, max_steps=1)
        error = caught.exception
        self.assertEqual(error.progress, RolloutResult(status, 0, 6, 0))
        self.assertIs(error.cause, error.__cause__); self.assertIsInstance(error.cause, InvalidMatchError)
        late = {"gf": 9007199254740990, "players": [{"id": 0, "hp": 99}, {"id": 7, "hp": 99}]}
        manual = self.match(config=Config(initial_hand=0))
        for value in (game, manual):
            with OfficialState(late, library=LIBRARY) as scene: value.load_scene(scene, 0, 42)
        first_status, first_policy, first_chance = manual.rollout_step(1, 2)
        self.assertEqual(manual.observe()["board"]["gf"], 9007199254740991)
        with self.assertRaises(RolloutError) as caught: game.chance_rollout(1, 2, max_steps=2)
        self.assertEqual(caught.exception.progress, RolloutResult(first_status, first_policy, first_chance, 1))
        self.assertEqual((game.status, game.observe()), (manual.status, manual.observe()))
        with self.assertRaises((AttributeError, TypeError)): caught.exception.progress.steps = 99

    def test_chance_rollout_resume_bounds_and_strict_driver_inputs(self):
        config = Config(initial_hand=0, max_decisions=32, include_fate=True, external_grants=True)
        raw = {"gf": 0, "tiebreakGF": 0, "players": [{"id": 0, "hp": 99, "mp": 99, "cp": 99, "items": [{"id": 0, "modelId": 207}]},
                                                       {"id": 7, "hp": 99, "mp": 99, "cp": 99}]}
        one, split = self.match(config=config), self.match(config=config)
        for value in (one, split):
            with OfficialState(raw, library=LIBRARY) as scene: value.load_scene(scene, 0, 42)
        initial, view = split.status, split.observe()
        self.assertEqual(split.chance_rollout(0, 2, max_steps=0), RolloutResult(initial, 0, 2, 0))
        self.assertEqual(split.observe(), view)
        complete = one.chance_rollout(0, 2, max_steps=16)
        first = split.chance_rollout(0, 2, max_steps=4)
        second = split.chance_rollout(first.policy_rng, first.chance_rng, max_steps=12)
        self.assertEqual((second.status, second.policy_rng, second.chance_rng, first.steps+second.steps),
                         (complete.status, complete.policy_rng, complete.chance_rng, complete.steps))
        self.assertEqual(split.observe(), one.observe())
        for bad in (True, -1, 2**64, 1.0):
            for call in (lambda: split.sample_choice(bad), lambda: split.choice_step(Command(0, "pray"), bad),
                         lambda: split.rollout_step(bad, 2), lambda: split.rollout_step(1, bad),
                         lambda: split.chance_rollout(bad, 2), lambda: split.chance_rollout(1, bad),
                         lambda: split.chance_rollout(1, 2, max_steps=bad)):
                with self.assertRaises(InvalidMatchError): call()
        for choice in (None, {}, (0, 0), FateRequest(True, 0), FateRequest(0, True), FateRequest(0, 2**53)):
            with self.assertRaises(InvalidMatchError): split.choice_legal(choice)
            with self.assertRaises(InvalidMatchError): split.choice_step(choice, 2)
        split.close()
        for call in (lambda: split.choice_legal(FateRequest(0, 0)), lambda: split.sample_choice(0),
                     lambda: split.choice_step(FateRequest(0, 0), 2), lambda: split.rollout_step(0, 2), lambda: split.chance_rollout()):
            with self.assertRaises(InvalidMatchError): call()

    def test_sampler_pure_policy_stream_and_seeded_replay(self):
        a, b = self.match(), self.match()
        initial = a.observe(0); status = a.status
        one, r1 = a.sample_action(123); two, r2 = a.sample_action(123)
        self.assertEqual((one, r1), (two, r2)); self.assertEqual(a.observe(0), initial); self.assertEqual(a.status, status)
        ra = rb = 999
        for unused in range(a.config.max_decisions):
            if a.done: break
            ca, ra = a.sample_action(ra); cb, rb = b.sample_action(rb)
            self.assertEqual((ca, ra), (cb, rb)); self.assertTrue(a.legal(ca))
            self.assertEqual(a.step(ca), b.step(cb)); self.assertEqual(a.observe(), b.observe())
        self.assertTrue(a.done)
        a.reset(42); self.assertEqual(a.observe(0), initial); self.assertEqual(a.status, status)

    def test_invalid_commands_and_reset_are_atomic(self):
        match = self.match(); before = match.observe(0); status = match.status
        invalid = [Command(999, "pray"), Command(0, "play", (), 1), Command(0, "defend"),
                   Command(0, "pray", (1,)), Command(0, "play", (9999,), 1)]
        for command in invalid:
            with self.subTest(command=command):
                self.assertFalse(match.legal(command))
                with self.assertRaises(InvalidMatchError): match.step(command)
                self.assertEqual(match.observe(0), before); self.assertEqual(match.status, status)
        for players in ([], [PlayerSetup(3), PlayerSetup(3)], [PlayerSetup(i) for i in range(10)]):
            with self.subTest(players=players), self.assertRaises(InvalidMatchError): match.reset(200, players)
            self.assertEqual(match.status, status); self.assertEqual(match.observe(0), before)
        with self.assertRaises(UnsupportedMatchError): match.reset(players=[{"id": 1, "items": []}, {"id": 2}])
        self.assertEqual(match.observe(0), before)

    def test_strict_transport_types(self):
        for kwargs in ({"seed": True}, {"seed": -1}, {"seed": 2**64}, {"config": {}}, {"config": Config(max_decisions=0)},
                       {"config": Config(initial_hand=19)}, {"players": [PlayerSetup(0, "\ud800"), PlayerSetup(1)]},
                       {"players": [PlayerSetup(True), PlayerSetup(1)]}, {"players": [PlayerSetup(0, team=5), PlayerSetup(1)]}):
            with self.subTest(kwargs=kwargs), self.assertRaises(MatchError): self.match(**kwargs)
        match = self.match(); initial = match.status
        for command in (Command(True, "pray"), Command(0, "unknown"), Command(0, "play", (0,) * 19, 1), Command(0, "play", (False,), 1), Command(0, "play", (), True)):
            with self.subTest(command=command), self.assertRaises(MatchError): match.step(command)
            self.assertEqual(match.status, initial)
        for viewer in (True, -1, 999):
            with self.subTest(viewer=viewer), self.assertRaises(MatchError): match.observe(viewer)
        for rng in (True, -1, 2**64):
            with self.subTest(rng=rng), self.assertRaises(MatchError): match.sample_action(rng)

    def test_truncation_fixed_seat_reward_and_done(self):
        match = self.match(config=Config(max_decisions=1, initial_hand=0), players=[PlayerSetup(30), PlayerSetup(5)])
        result = match.step(Command(30, "pray"))
        self.assertTrue(result.truncated); self.assertFalse(result.terminated); self.assertTrue(result.done)
        self.assertEqual(result.player_ids, (30, 5)); self.assertEqual(result.rewards, (0, 0)); self.assertIsNone(result.actor)
        self.assertEqual(result.reward_by_player, {30: 0, 5: 0})
        with self.assertRaises(InvalidMatchError): match.sample_action(1)
        self.assertEqual(match.status, result)
        self.assertFalse(match.legal(Command(30, "pray")))

    def test_nine_players_teams_and_random_rollout(self):
        players = [PlayerSetup(i * 101, str(i), i % 3 + 1) for i in range(9)]
        match = self.match(players=players, config=Config(max_decisions=256))
        status, rng = match.random_rollout(777)
        self.assertTrue(status.done); self.assertEqual(status.player_ids, tuple(p.id for p in players)); self.assertEqual(len(status.rewards), 9)
        self.assertTrue(set(status.winners) <= set(status.player_ids)); self.assertTrue(all(r in (-1, 0, 1) for r in status.rewards))
        self.assertIsInstance(rng, int)
        self.assertEqual(match.observe()["board"]["hand"], [])

    def test_team_terminal_and_player_zero(self):
        match = self.match(players=[PlayerSetup(0, team=2), PlayerSetup(88, team=2)])
        result = match.status
        self.assertTrue(result.terminated); self.assertTrue(result.done)
        self.assertEqual(result.winners, (0, 88)); self.assertEqual(result.rewards, (1, 1))
        self.assertIsNone(match.actor)

    def test_partial_rollout_and_close(self):
        match = self.match(); before = match.status
        status, rng = match.random_rollout(42, max_steps=0)
        self.assertEqual((status, rng), (before, 42))
        status, rng = match.random_rollout(42, max_steps=2)
        self.assertEqual(status.decisions, 2)
        match.close(); match.close(); self.assertTrue(match.closed)
        for method in (lambda: match.observe(), lambda: match.step(Command(0, "pray")), lambda: match.reset(), lambda: match.sample_action(0)):
            with self.assertRaises(MatchError): method()
        with self.match() as context: self.assertFalse(context.closed)
        self.assertTrue(context.closed)

if __name__ == "__main__": unittest.main()
