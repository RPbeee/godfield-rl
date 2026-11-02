import copy
import json
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from official_data import OfficialData
from official_match import Match, Config, PlayerSetup, Command, MatchError, InvalidMatchError, UnsupportedMatchError
LIBRARY = sys.argv.pop(1) if len(sys.argv) > 1 else None

class MatchTests(unittest.TestCase):
    def match(self, **kwargs):
        value = Match(library=LIBRARY, **kwargs); self.addCleanup(value.close); return value

    def token(self, game, seat, model):
        return next(i["token"] for i in game.observe(seat)["board"]["hand"] if i["face"] == model)

    def test_descriptor_and_public_observation(self):
        match = self.match(players=[PlayerSetup(0, "A\0B"), PlayerSetup(77, "相手")])
        self.assertEqual(match.ruleset, "official-execution-subset-v28"); self.assertIn("233-models", match.scope); self.assertIn("not-official-server-equivalence", match.scope)
        self.assertEqual((len(match.supported_models), match.gift_weight), (233, 494))
        report = json.loads((Path(__file__).resolve().parents[1] / "data/official-v2/dream-compatibility-v28.json").read_text())
        self.assertEqual(tuple(report["supported_ids"]), match.supported_models)
        self.assertEqual((report["pool_models"], report["pool_gift_weight"]), (233, 494))
        self.assertIn(212, match.supported_models); self.assertIn(112, match.supported_models); self.assertIn(222, match.supported_models)
        self.assertEqual(match.status.player_ids, (0, 77)); self.assertEqual(match.actor, 0)
        own = match.observe(0); spectator = match.observe()
        self.assertEqual(own["schema"], "official-execution-view-v26")
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
        match.reset(178023); reference = self.match(config=config, seed=178023)
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
        self.assertEqual(event["schema"], "official-execution-view-v26")
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
        multi = self.match(config=config, players=roster, seed=127)
        reference = self.match(config=config, players=roster, seed=127)
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
        match.reset(129)
        chance = next(item["token"] for item in match.observe(0)["board"]["hand"] if item["face"] == 95)
        result = match.step(Command(0, "play", (chance,)))
        self.assertEqual((result.decisions, result.phase, result.actor, result.owner), (1, "main", 1, 1))
        self.assertIsNone(match.observe()["pending"])
        self.assertEqual([p["status"]["hp"] for p in match.observe()["board"]["players"]], [40, 40])
        # A chance miracle accepts FREE but a hit remains hit across reflection.
        match.reset(130349); reference = self.match(config=config, seed=130349)
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
        match = self.match(config=config, seed=8185)
        reference = self.match(config=config, seed=8185)
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
        match.reset(7250)
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
        match = self.match(config=config, seed=1203); reference = self.match(config=config, seed=1203)
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
        capped = self.match(config=Config(initial_hand=18, initial_mp=98), seed=1203)
        own, other = hand(capped, 0), hand(capped, 1)
        capped.step(Command(0, "play", (own[6],), 1)); capped.step(Command(1, "defend", (other[187],))); capped.step(Command(1, "defend"))
        self.assertEqual(stats(capped, "mp"), [98, 99])
        # A self-reflection of the MP counter keeps a response and applies once.
        mirror = self.match(config=config, seed=540)
        own, other = hand(mirror, 0), hand(mirror, 1)
        self.assertIn(6, own); self.assertTrue({187, 190} <= other.keys())
        mirror.step(Command(0, "play", (own[6],), 1)); mirror.step(Command(1, "defend", (other[187],)))
        mirror.step(Command(1, "defend", (other[190],)))
        self.assertEqual((mirror.actor, mirror.observe()["pending"]["source"], mirror.observe()["pending"]["target"]), (1, 1, 1))
        self.assertEqual(stats(mirror, "mp"), [0, 0])
        mirror.step(Command(1, "defend")); self.assertEqual(stats(mirror, "mp"), [0, 2])
        # Venus credits the full nominal amount after CP->MP->HP debt payment.
        cp = self.match(config=config, seed=13778)
        own, other = hand(cp, 0), hand(cp, 1)
        self.assertIn(32, own); self.assertIn(188, other)
        cp.step(Command(0, "play", (own[32],), 1)); cp.step(Command(1, "defend", (other[188],)))
        pending = cp.observe()["pending"]
        self.assertEqual((cp.actor, pending["source"], pending["target"], pending["power"], pending["amount"]), (0, 1, 0, 0, 9))
        cp.step(Command(0, "defend"))
        self.assertEqual(stats(cp, "hp"), [31, 31]); self.assertEqual(stats(cp, "mp"), [0, 0]); self.assertEqual(stats(cp, "cp"), [0, 9])
        self.assertIsNone(cp.observe()["pending"])
        cp_cap = self.match(config=Config(initial_hand=18, initial_mp=0, initial_cp=99), seed=13778)
        own, other = hand(cp_cap, 0), hand(cp_cap, 1)
        cp_cap.step(Command(0, "play", (own[32],), 1)); cp_cap.step(Command(1, "defend", (other[188],))); cp_cap.step(Command(0, "defend"))
        self.assertEqual(stats(cp_cap, "cp"), [90, 99])
        # Reflection changes creditor/debtor through the current source/target.
        reflected = self.match(config=config, seed=26332)
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
            alias = self.match(config=Config(initial_hand=18, initial_hp=hp, initial_mp=0, initial_cp=0), seed=13778)
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
            effect = self.match(config=Config(initial_hand=18, initial_hp=initial_hp, initial_mp=0), seed=237)
            own = hand(effect, 0); self.assertIn(27, own)
            effect.step(Command(0, "play", (own[27],), target))
            self.assertEqual(effect.observe()["damage_effect_rule"], rule)
            effect.step(Command(target, "defend")); self.assertEqual(hp(effect), expected)
            if initial_hp == 3: self.assertEqual(effect.status.winners, (0,))
        match.reset(237); own, other = hand(match, 0), hand(match, 1)
        self.assertIn(176, other)
        match.step(Command(0, "play", (own[27],), 1)); match.step(Command(1, "defend", (other[176],)))
        self.assertEqual(hp(match), [40, 40])  # No source heal at zero remainder.
        # Current source receives the absorption effect after reflection.
        match.reset(386); own, other = hand(match, 0), hand(match, 1)
        self.assertIn(27, own); self.assertIn(38, other)
        match.step(Command(0, "play", (own[27],), 1)); match.step(Command(1, "defend", (other[38],)))
        self.assertEqual(hp(match), [40, 40])
        self.assertEqual((match.observe()["pending"]["source"], match.observe()["pending"]["target"]), (1, 0))
        match.step(Command(0, "defend")); self.assertEqual(hp(match), [33, 47])
        # Self-heal and target damage net to zero, but body damage7 still creates counter14.
        match.reset(417); own = hand(match, 0)
        self.assertTrue({27, 184} <= own.keys())
        match.step(Command(0, "play", (own[27],), 0)); match.step(Command(0, "defend", (own[184],)))
        self.assertEqual(hp(match), [40, 40]); self.assertEqual(match.observe()["pending"]["power"], 14)
        match.step(Command(0, "defend")); self.assertEqual(hp(match), [26, 40])
        # The chance absorber keeps its ability through hit/miss handling.
        for seed, expected in ((59, [43, 37]), (267, [40, 40])):
            match.reset(seed); token = hand(match, 0)[98]
            match.step(Command(0, "play", (token,)))
            if match.status.phase == "response": match.step(Command(1, "defend"))
            self.assertEqual(hp(match), expected)
        # A paid first use consumes the declared10 MP and retains the same instance.
        paid = self.match(config=Config(initial_hand=18, initial_mp=10), seed=9344)
        paid_token = hand(paid, 0)[216]
        paid.step(Command(0, "play", (paid_token,), 1))
        self.assertEqual([p["status"]["mp"] for p in paid.observe()["board"]["players"]], [0, 10])
        paid.step(Command(1, "defend")); self.assertEqual(hp(paid), [50, 30])
        self.assertTrue(next(item for item in paid.observe(0)["board"]["hand"] if item["token"] == paid_token)["used"])
        # The held Light miracle accepts FREE, then heals the reflected current source.
        match.reset(9344); reference = self.match(config=config, seed=9344)
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
            effect = self.match(config=Config(initial_hand=18, initial_hp=initial_hp, initial_mp=0), seed=15)
            token = hand(effect, 0)[53]
            effect.step(Command(0, "play", (token,), target)); effect.step(Command(target, "defend"))
            self.assertEqual(hp(effect), expected)
            if initial_hp == 20: self.assertEqual(effect.status.winners, (1,))
        match.reset(15); own = hand(match, 0)
        self.assertTrue({53, 184} <= own.keys())
        match.step(Command(0, "play", (own[53],), 0)); match.step(Command(0, "defend", (own[184],)))
        self.assertEqual(hp(match), [12, 40])
        self.assertEqual(match.observe()["pending"]["power"], 28)  # Target14, not net28 doubled.
        match.step(Command(0, "defend")); self.assertEqual(match.status.winners, (1,))

    def test_flash_response_limit_and_free_main_cure(self):
        match = self.match(config=Config(initial_hand=18, initial_mp=0), seed=2315131)
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
        match = self.match(config=Config(initial_hand=18, initial_mp=0), seed=702528)
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
        for seed, hit in ((107, True), (28, False)):
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
        game = self.match(config=config, seed=16494)
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
            weapon = self.match(config=Config(initial_hand=18, initial_hp=hp), seed=57)
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
        game = self.match(config=config, seed=5360); reference = self.match(config=config, seed=5360)
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
        game = self.match(config=config, seed=7841); reference = self.match(config=config, seed=7841)
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
            game = self.match(config=config, seed=1294294)
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
        game = self.match(config=config, seed=30533)
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
        game = self.match(config=config, seed=43); reference = self.match(config=config, seed=43)
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
        removed_event = {"origin": 0, "source": 0, "target": 1, "model": 205, "removed_models": [166, 113, 100]}
        for viewer in (None, 0, 1):
            self.assertEqual(game.observe(viewer)["last_removal"], removed_event)
        for key in ("removed", "removed_models", "removed_slots", "removal_rng", "batch", "rng", "targets"):
            self.assertNotIn(key, game.observe()); self.assertNotIn(key, game.observe()["board"])
        game = self.match(config=config, seed=3843)
        retained = token(game, 0, 235); broom = token(game, 0, 205)
        game.step(Command(0, "play", (retained,), 0))
        for item in items(game, 1):
            command = Command(1, "discard", (item["token"],))
            if game.legal(command): game.step(command); break
        game.step(Command(0, "play", (broom,), 0))
        self.assertEqual(len(items(game, 0)), 15)
        self.assertTrue(next(i for i in items(game, 0) if i["token"] == retained)["used"])
        game = self.match(config=config, seed=1251)
        broom = token(game, 0, 205); mirror = token(game, 1, 190)
        game.step(Command(0, "play", (broom,), 1)); game.step(Command(1, "defend", (mirror,))); game.step(Command(0, "defend"))
        self.assertEqual([len(items(game, seat)) for seat in (0, 1)], [15, 18])
        # Unused miracles are ordinary Broom candidates, even when sole candidate.
        game = self.match(config=Config(initial_hand=1), seed=273197)
        self.assertEqual(items(game, 1)[0]["face"], 224); self.assertFalse(items(game, 1)[0]["used"])
        game.step(Command(0, "play", (token(game, 0, 205),), 1)); game.step(Command(1, "defend"))
        self.assertEqual([len(items(game, seat)) for seat in (0, 1)], [1, 0])
        game = self.match(config=Config(initial_hand=1), seed=435)
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
        game = self.match(config=config, seed=1881945); reference = self.match(config=config, seed=1881945)
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
        game = self.match(config=config, seed=130933)
        copies = [i["token"] for i in items(game, 0) if i["face"] == 235]
        self.assertEqual(len(copies), 2); self.assertNotEqual(*copies)
        soap = token(game, 0, 206)
        for copy in copies: game.step(Command(0, "play", (copy,), 0)); yield_other(game)
        game.step(Command(0, "play", (soap,), 0))
        self.assertEqual(len(items(game, 0)), 16); self.assertTrue(set(copies).isdisjoint(i["token"] for i in items(game, 0)))
        self.assertFalse(any(i["used"] for i in items(game, 0)))
        self.assertEqual(game.observe()["last_removal"],
                         {"origin": 0, "source": 0, "target": 0, "model": 206, "removed_models": [235, 235]})
        game = self.match(config=config, seed=1333)
        spring, soap, mirror = token(game, 0, 235), token(game, 0, 206), token(game, 1, 190)
        game.step(Command(0, "play", (spring,), 0)); yield_other(game, reserved=(mirror,))
        game.step(Command(0, "play", (soap,), 1)); game.step(Command(1, "defend", (mirror,))); game.step(Command(0, "defend"))
        self.assertEqual([len(items(game, seat)) for seat in (0, 1)], [17, 17])
        self.assertNotIn(spring, {i["token"] for i in items(game, 0)})
        self.assertEqual(game.observe()["last_removal"],
                         {"origin": 0, "source": 1, "target": 0, "model": 206, "removed_models": [235]})
        # All initially unplayed miracles are ineligible, including after self consumption.
        for target in (0, 1):
            game = self.match(config=Config(initial_hand=1), seed=321664)
            command = Command(0, "play", (token(game, 0, 206),), target)
            original_other = items(game, 1)
            game.step(command)
            if target == 1: game.step(Command(1, "defend"))
            self.assertEqual([len(items(game, seat)) for seat in (0, 1)], [1, 1])
            self.assertFalse(any(i["used"] for i in items(game, 0)))
            self.assertEqual(items(game, 1), original_other)

    def test_removal_event_lifecycle_empty_and_invalid_calls(self):
        rule = "experimental:resolved-body-only;public-removed-models-in-draw-order;duplicates-and-empty-preserved;clear-on-next-accepted-main;no-tokens-or-rng"
        game = self.match(config=Config(initial_hand=1, initial_mp=0), seed=435)
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
        self.assertEqual((other["token"], other["face"]), (1, 40))
        game.step(Command(1, "play", (other["token"],), 0))
        self.assertIsNone(game.observe()["last_removal"])
        game.reset(435)
        self.assertIsNone(game.observe()["last_removal"])

    def test_removal_event_pending_and_resolved_cutoff(self):
        game = self.match(config=Config(initial_hand=1, initial_mp=0, max_decisions=2), seed=273197)
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
        zero = self.match(config=Config(initial_hand=18, initial_mp=0), seed=20)
        token = hand(zero)[58]; zero.step(Command(0, "play", (token,), 1))
        self.assertEqual((zero.status.phase, zero.actor, zero.observe()["pending"]["power"]), ("response", 1, 0))
        self.assertEqual(zero.observe()["mana_weapon_rule"], rule)
        zero.step(Command(1, "defend"))
        self.assertEqual([p["status"]["hp"] for p in zero.observe()["board"]["players"]], [40, 40])
        # All completed-column miracle costs precede the dynamic base; order still matters afterwards.
        for models, power in (((58, 231), 16), ((58, 210), 18), ((58, 210, 231), 12), ((58, 231, 210), 10)):
            game = self.match(config=Config(initial_hand=18, initial_mp=10, initial_hp=99), seed=93265)
            own = hand(game); self.assertTrue(set(models) <= own.keys())
            command = Command(0, "play", tuple(own[id] for id in models), 1)
            self.assertTrue(game.legal(command)); game.step(command)
            self.assertEqual((game.observe()["pending"]["power"], mp(game)), (power, [0, 10]))
            game.step(Command(1, "defend"))
            self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 99 - power)
            self.assertTrue(all(next(i for i in game.observe(0)["board"]["hand"] if i["token"] == own[id])["used"] for id in models[1:]))
        for models, mana, power in (((58, 12), 0, 2), ((58, 12, 231, 204), 10, 44), ((58, 231, 204, 12), 10, 42)):
            game = self.match(config=Config(initial_hand=18, initial_mp=mana, initial_hp=99), seed=1438356)
            own = hand(game); command = Command(0, "play", tuple(own[id] for id in models), 1)
            game.step(command); self.assertEqual(game.observe()["pending"]["power"], power); self.assertEqual(mp(game), [0, mana])
            game.step(Command(1, "defend"))
            self.assertEqual(len(game.observe(0)["board"]["hand"]), 18)
            self.assertNotIn(own[58], {i["token"] for i in game.observe(0)["board"]["hand"]})
            if 204 in models:
                self.assertNotIn(own[204], {i["token"] for i in game.observe(0)["board"]["hand"]})
                self.assertTrue(next(i for i in game.observe(0)["board"]["hand"] if i["token"] == own[231])["used"])
        config = Config(initial_hand=18, initial_mp=10)
        game = self.match(config=config, seed=929); reference = self.match(config=config, seed=929)
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
        game = self.match(config=config, seed=272); reference = self.match(config=config, seed=272)
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
        paid = self.match(config=Config(initial_hand=18, initial_mp=10), seed=949); own = hand(paid)
        paid.step(Command(0, "play", (own[14], own[210]), 1))
        self.assertEqual(paid.observe()["board"]["players"][0]["status"]["mp"], 8)
        for unused in range(2):
            self.assertEqual(paid.observe()["pending"]["power"], 5); paid.step(Command(1, "defend"))
        self.assertEqual(hp(paid), [40, 30]); self.assertEqual(paid.observe()["board"]["players"][0]["status"]["mp"], 8)
        self.assertTrue(next(i for i in paid.observe(0)["board"]["hand"] if i["token"] == own[210])["used"])
        fresh = self.match(config=config, seed=40741); own, other = hand(fresh), hand(fresh, 1)
        fresh.step(Command(0, "play", (own[14], own[66]), 1)); fresh.step(Command(1, "defend", (other[189],)))
        self.assertEqual((fresh.observe()["pending"]["element"], fresh.observe()["pending"]["power"]), ("fire", 5))
        fresh.step(Command(1, "defend")); self.assertEqual(hp(fresh), [40, 30])
        reflected = self.match(config=config, seed=46); own, other = hand(reflected), hand(reflected, 1)
        reflected.step(Command(0, "play", (own[14],), 1)); reflected.step(Command(1, "defend", (other[38],))); reflected.step(Command(0, "defend"))
        pending = reflected.observe()["pending"]
        self.assertEqual((pending["source"], pending["target"], pending["reflections"]), (0, 1, 0))
        reflected.step(Command(1, "defend")); self.assertEqual(hp(reflected), [37, 37])
        for seed, mirror, winner, decisions in ((3, False, 0, 2), (46, True, 1, 3)):
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
            game = self.match(config=config, players=roster, seed=3105)
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
        invalid_game = self.match(config=config, seed=1557)
        own = {i["face"]: i["token"] for i in invalid_game.observe(0)["board"]["hand"]}
        self.assertTrue({95, 232} <= own.keys()); self.assertFalse(invalid_game.legal(Command(0, "play", (own[95], own[232]))))
        free_config = Config(initial_hand=18, initial_mp=0)
        game = self.match(config=free_config, seed=3105); reference = self.match(config=free_config, seed=3105)
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
        repeat = self.match(config=config, players=roster, seed=560); own = hand(repeat)
        repeat.step(Command(0, "play", (own[14], own[232])))
        for actor in (88, 99, 88, 99):
            self.assertEqual((repeat.actor, repeat.status.owner, repeat.observe()["pending"]["power"]), (actor, 0, 3))
            self.assertEqual(len(repeat.observe(0)["board"]["hand"]), 17)
            repeat.step(Command(actor, "defend"))
        self.assertEqual((repeat.status.decisions, repeat.actor), (5, 77))
        self.assertEqual([p["status"]["hp"] for p in repeat.observe()["board"]["players"]], [40, 40, 34, 34])
        self.assertEqual(repeat.observe()["board"]["players"][0]["status"]["mp"], 0)
        game = self.match(config=config, players=roster, seed=69258); own, enemy = hand(game), hand(game, 88)
        game.step(Command(0, "play", (own[6], own[232]))); game.step(Command(88, "defend", (enemy[184],)))
        self.assertEqual((game.actor, game.observe()["pending"]["counter"], game.observe()["pending"]["power"]), (0, True, 2))
        game.step(Command(0, "defend")); self.assertEqual((game.actor, game.observe()["pending"]["counter"]), (99, False))
        game.step(Command(99, "defend"))
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [38, 40, 39, 39])
        for mana, legal, power in ((4, False, None), (5, True, 0), (10, True, 10)):
            game = self.match(config=Config(initial_hand=18, initial_mp=mana), seed=817); own = hand(game)
            cast = Command(0, "play", (own[58], own[232]))
            self.assertEqual(game.legal(cast), legal)
            if not legal:
                before = game.observe(0)
                with self.assertRaises(InvalidMatchError): game.step(cast)
                self.assertEqual(game.observe(0), before); continue
            game.step(cast); self.assertEqual(game.observe()["pending"]["power"], power)
            self.assertEqual(game.observe()["board"]["players"][0]["status"]["mp"], 0)
            game.step(Command(1, "defend"))
        copies = self.match(config=Config(initial_hand=18, initial_mp=10), seed=48897)
        own = hand(copies); tokens = [i["token"] for i in copies.observe(0)["board"]["hand"] if i["face"] == 232]
        self.assertEqual(len(tokens), 2)
        copies.step(Command(0, "play", (own[6], *tokens)))
        self.assertEqual(copies.observe()["board"]["players"][0]["status"]["mp"], 0)
        copies.step(Command(1, "defend")); self.assertEqual(copies.status.decisions, 2)
        self.assertEqual(copies.observe()["board"]["players"][1]["status"]["hp"], 39)
        self.assertTrue(all(next(i for i in copies.observe(0)["board"]["hand"] if i["token"] == token)["used"] for token in tokens))
        for models, element in (((6, 66, 232), None), ((6, 232, 66), "fire")):
            game = self.match(config=config, seed=9711); own = hand(game)
            game.step(Command(0, "play", tuple(own[id] for id in models)))
            self.assertEqual((game.observe()["pending"]["power"], game.observe()["pending"]["element"]), (3, element))
            game.step(Command(1, "defend"))

    def test_random_hp_sign_self_foreign_clips_and_cutoff(self):
        rule = "experimental:launch-once;uniform-plus10-or-minus10;zero-heals-one-damages;fixed-through-reflections;nonattack-no-defense-or-counters;hp-clips-0-99;gifts-after-action"
        def hand(game, seat=0): return {i["face"]: i["token"] for i in game.observe(seat)["board"]["hand"]}
        for seed, delta in ((33, 10), (103, -10)):
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
            game = self.match(config=Config(initial_hand=18, initial_hp=hp), seed=33)
            token = hand(game)[202]; game.step(Command(0, "play", (token,), 0))
            self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 99)
            self.assertEqual((game.status.decisions, len(game.observe(0)["board"]["hand"])), (1, 18))
            self.assertNotIn(token, {i["token"] for i in game.observe(0)["board"]["hand"]})
        game = self.match(config=Config(initial_hand=18, initial_mp=0, max_decisions=1), seed=103)
        game.step(Command(0, "play", (hand(game)[202],), 1))
        self.assertTrue(game.status.truncated); self.assertIsNone(game.observe()["pending"])
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [40, 40])
        self.assertEqual(len(game.observe(0)["board"]["hand"]), 17)
        for target, limit, winner in ((0, 1, 1), (1, 2, 0)):
            game = self.match(config=Config(initial_hand=18, initial_hp=5, max_decisions=limit), seed=103)
            game.step(Command(0, "play", (hand(game)[202],), target))
            if target: game.step(Command(1, "defend"))
            self.assertTrue(game.status.terminated); self.assertFalse(game.status.truncated); self.assertEqual(game.status.winners, (winner,))

    def test_random_hp_reflections_nonattack_atomic_queries_and_discard(self):
        config = Config(initial_hand=18, initial_mp=0)
        def hand(game, seat=0): return {i["face"]: i["token"] for i in game.observe(seat)["board"]["hand"]}
        game = self.match(config=config, seed=501381); reference = self.match(config=config, seed=501381)
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
        discarded = self.match(config=config, seed=33); control = self.match(config=config, seed=33)
        a, b = hand(discarded), hand(control)
        discarded.step(Command(0, "discard", (a[202],))); control.step(Command(0, "discard", (b[118],)))
        healing = hand(discarded, 1)[191]
        discarded.step(Command(1, "play", (healing,), 1)); control.step(Command(1, "play", (healing,), 1))
        self.assertEqual(discarded.observe(1), control.observe(1))

    def test_passive_revival_bodies_and_deferred_copies(self):
        def items(game, seat): return game.observe(seat)["board"]["hand"]
        def token(game, seat, model): return next(i["token"] for i in items(game, seat) if i["face"] == model)
        for seed, model, hp in ((114, 57, 5), (178, 64, 40), (428, 202, 5)):
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
            game = self.match(config=Config(initial_hand=18, initial_hp=1, initial_mp=0, max_decisions=limit), seed=2393236)
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
        game = self.match(config=Config(initial_hand=1, hand_limit=1, initial_mp=0), seed=215)
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
        game = self.match(config=config, seed=29263); reference = self.match(config=config, seed=29263)
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
        unsupported = self.match(config=Config(initial_hand=18, initial_mp=99), seed=2175); cards = hand(unsupported, 0)
        command = Command(0, "play", (cards[94], cards[232])); before = unsupported.observe(0)
        self.assertFalse(unsupported.legal(command))
        with self.assertRaises(UnsupportedMatchError): unsupported.step(command)
        self.assertEqual(unsupported.observe(0), before)

    def test_danger_mortar_immediate_consumption_and_ordered_boost(self):
        def items(game, seat): return game.observe(seat)["board"]["hand"]
        def token(game, seat, model): return next(i["token"] for i in items(game, seat) if i["face"] == model)
        for seed, target, winner in ((4920, 0, 1), (2364, 1, 0)):
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
        game = self.match(config=Config(initial_hand=18, initial_hp=99, initial_mp=0), seed=296919)
        cards = tuple(token(game, 0, model) for model in (94, 231, 204, 66))
        game.step(Command(0, "play", cards)); event = game.observe()["last_danger"]
        self.assertEqual((event["power"], event["element"], event["mortar"]), (62, "fire", False))
        game.step(Command(event["target"], "defend"))
        self.assertEqual(game.observe()["board"]["players"][event["target"]]["status"]["hp"], 37)

    def test_exchange_direct_allocation_and_same_value_consumption(self):
        def hand(game): return game.observe(0)["board"]["hand"]
        for allocation in ((20, 30, 10), (40, 0, 20)):
            game = self.match(config=Config(initial_hand=18, initial_mp=0), seed=0)
            token = next(i["token"] for i in hand(game) if i["face"] == 3)
            self.assertTrue(game.legal(Command(0, "exchange", (token,), allocation=allocation)))
            result = game.step(Command(0, "exchange", (token,), allocation=allocation))
            own = game.observe()["board"]["players"][0]["status"]
            self.assertEqual(tuple(own[k] for k in ("hp", "mp", "cp")), allocation)
            self.assertEqual((result.phase, result.actor, result.decisions), ("main", 1, 1)); self.assertIsNone(game.observe()["pending"])
            self.assertEqual(len(hand(game)), 18); self.assertNotIn(token, {i["token"] for i in hand(game)})
            self.assertIn("sum-conserved-before-revival", game.observe()["exchange_rule"])
        game = self.match(config=Config(initial_hand=18, initial_mp=0, max_decisions=1), seed=0)
        token = next(i["token"] for i in hand(game) if i["face"] == 3)
        game.step(Command(0, "exchange", (token,), allocation=(20,30,10)))
        self.assertTrue(game.status.truncated); self.assertEqual(len(hand(game)),17)
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["mp"],30)

    def test_exchange_hp_zero_natural_death_and_revival(self):
        for seed, revived in ((0,False),(32,True)):
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
        game = self.match(config=Config(initial_hand=1,hand_limit=1),seed=3)
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
        game = self.match(config=Config(initial_hand=18, initial_cp=3, initial_mp=4), seed=11)
        own = game.observe(0)["board"]["hand"]
        by_token = {i["token"]: i["face"] for i in own}
        self.assertEqual((by_token[7], by_token[16]), (4, 153))
        command = Command(0, "sell", (7, 16), 0)
        before, status = game.observe(0), game.status
        invalid = (Command(0, "play", (7,), 0), Command(0, "sell", (7, 7), 0),
                   Command(0, "sell", (16, 7), 0), Command(0, "sell", (7,), 0),
                   Command(0, "sell", (7, 16)), Command(0, "sell", (7, 16), 0, (40, 4, 3)))
        for bad in invalid:
            self.assertFalse(game.legal(bad))
            with self.assertRaises(InvalidMatchError): game.step(bad)
            self.assertEqual((game.observe(0), game.status), (before, status))
        full = Command(0, "sell", (7, 16), 1)
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
        self.assertEqual(len(after), 18); self.assertTrue({7, 16}.isdisjoint(i["token"] for i in after))
        self.assertEqual(next(i for i in after if i["token"] == 36)["face"], 153)
        before, status = game.observe(0), game.status
        with self.assertRaises(InvalidMatchError): game.step(command)
        self.assertEqual((game.observe(0), game.status), (before, status))
        other = next(i for i in game.observe(1)["board"]["hand"] if game.legal(Command(1, "discard", (i["token"],))))
        game.step(Command(1, "discard", (other["token"],)))
        self.assertIsNone(game.observe()["last_sale"])
        game.reset(11); self.assertIsNone(game.observe()["last_sale"])

    def test_sell_reflected_unused_miracle_is_transferred_not_learned(self):
        game = self.match(config=Config(initial_hand=4, initial_mp=99), seed=19507)
        self.assertEqual({i["token"]: i["face"] for i in game.observe(0)["board"]["hand"]}, {0: 195, 1: 138, 2: 4, 3: 215})
        self.assertEqual(next(i["face"] for i in game.observe(1)["board"]["hand"] if i["token"] == 4), 190)
        game.step(Command(0, "sell", (2, 3), 1))
        self.assertIsNone(game.observe()["last_sale"])
        self.assertEqual((game.observe()["pending"]["power"], game.observe()["pending"]["amount"]), (0, 0))
        with self.assertRaises(InvalidMatchError): game.step(Command(1, "defend", (5,)))
        game.step(Command(1, "defend", (4,)))
        self.assertIsNone(game.observe()["last_sale"])
        game.step(Command(0, "defend"))
        expected = {"origin": 0, "source": 1, "target": 0, "model": 215, "price": 0, "overflow_model": None}
        for viewer in (None, 0, 1): self.assertEqual(game.observe(viewer)["last_sale"], expected)
        own = game.observe(0)["board"]["hand"]
        received = next(i for i in own if i["token"] == 8)
        self.assertEqual((received["face"], received["used"]), (215, False))
        self.assertTrue({2, 3}.isdisjoint(i["token"] for i in own))
        self.assertEqual([p["status"]["mp"] for p in game.observe()["board"]["players"]], [99, 99])
        self.assertEqual([len(game.observe(seat)["board"]["hand"]) for seat in (0, 1)], [4, 4])
        learned = self.match(config=Config(initial_hand=4, initial_mp=99), seed=19507)
        learned.step(Command(0, "play", (3,), 0)); learned.step(Command(0, "defend"))
        learned.step(Command(1, "discard", (4,)))
        bad = Command(0, "sell", (2, 3), 0); before = learned.observe(0)
        self.assertFalse(learned.legal(bad))
        with self.assertRaises(InvalidMatchError): learned.step(bad)
        self.assertEqual(learned.observe(0), before)

    def test_sell_delivery_before_death_and_cutoff_without_gift(self):
        for hp, cp, mp, maximum in ((40, 3, 4, 2), (5, 0, 0, 512)):
            game = self.match(config=Config(initial_hand=4, initial_hp=hp, initial_cp=cp, initial_mp=mp, max_decisions=maximum), seed=547)
            self.assertEqual({i["token"]: i["face"] for i in game.observe(0)["board"]["hand"]}, {0: 4, 1: 79, 2: 43, 3: 53})
            game.step(Command(0, "sell", (0, 2), 1)); self.assertIsNone(game.observe()["last_sale"])
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
        game = self.match(config=Config(initial_hand=4, initial_cp=99, initial_mp=0), seed=2828)
        original = game.observe(1)["board"]["hand"]
        self.assertEqual(game.observe()["buy_rule"], "experimental:foreign-only;one-operation-gift;uniform-unused-commodity;empty-no-draw;cp-only-affordability;buyer-accept-or-decline;escrow-return-on-decline-or-cutoff;fresh-token-delivery;full-recipient-uniform-existing-item-overflow")
        buy = Command(0, "buy", (self.token(game, 0, 5),), 1)
        before, status = game.observe(0), game.status
        for bad in (Command(0, "play", (self.token(game, 0, 5),), 1), Command(0, "buy", (self.token(game, 0, 5), self.token(game, 0, 5)), 1),
                    Command(0, "buy", (self.token(game, 0, 5),)), Command(0, "buy", (self.token(game, 0, 5),), 1, (40, 0, 99))):
            self.assertFalse(game.legal(bad))
            with self.assertRaises(InvalidMatchError): game.step(bad)
            self.assertEqual((game.observe(0), game.status), (before, status))
        with self.assertRaises(UnsupportedMatchError): game.step(Command(0, "buy", (self.token(game, 0, 5),), 0))
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
            game = self.match(config=Config(initial_hand=4, initial_cp=99, initial_mp=0, max_decisions=maximum), seed=2828)
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
            game.reset(2828); self.assertIsNone(game.observe()["last_purchase"])

    def test_buy_reflection_uses_current_buyer_and_seller(self):
        game = self.match(config=Config(initial_hand=4, initial_cp=99, initial_mp=0), seed=296)
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
        game = self.match(config=Config(initial_hand=1, initial_cp=0, initial_mp=0), seed=307)
        original = game.observe(1)["board"]["hand"]
        game.step(Command(0, "buy", (0,), 1)); game.step(Command(1, "defend"))
        self.assertIsNone(game.observe()["purchase"])
        self.assertEqual(game.observe()["last_purchase"], {"origin": 0, "buyer": 0, "seller": 1, "model": 6, "price": 1, "bought": False, "reason": "insufficient-cp", "overflow_model": None})
        self.assertEqual(game.observe(1)["board"]["hand"], original)
        paid = self.match(config=Config(initial_hand=1, initial_cp=5, initial_mp=0), seed=307)
        paid.step(Command(0, "buy", (0,), 1)); paid.step(Command(1, "defend")); paid.step(Command(0, "accept"))
        self.assertEqual([p["status"]["cp"] for p in paid.observe()["board"]["players"]], [4, 6])
        self.assertEqual(next(i["face"] for i in paid.observe(0)["board"]["hand"] if i["token"] == 2), 6)
        game = self.match(config=Config(initial_hand=1, initial_cp=99, initial_mp=0), seed=13522)
        game.step(Command(0, "pray")); game.step(Command(1, "discard", (1,)))
        game.step(Command(0, "buy", (0,), 1)); game.step(Command(1, "defend"))
        self.assertEqual(game.observe()["last_purchase"], {"origin": 0, "buyer": 0, "seller": 1, "model": 0, "price": 0, "bought": False, "reason": "empty", "overflow_model": None})
        self.assertEqual(game.observe(1)["board"]["hand"], [])
        game = self.match(config=Config(initial_hand=4, initial_cp=99, initial_mp=99), seed=10296)
        learned = self.token(game, 1, 235)
        game.step(Command(0, "discard", (next(i["token"] for i in game.observe(0)["board"]["hand"] if i["face"] != 5 and game.legal(Command(0, "discard", (i["token"],)))),))); game.step(Command(1, "play", (self.token(game, 1, 235),), 1))
        self.assertTrue(next(i for i in game.observe(1)["board"]["hand"] if i["token"] == learned)["used"])
        game.step(Command(0, "buy", (self.token(game, 0, 5),), 1)); game.step(Command(1, "defend"))
        self.assertTrue(next(i for i in game.observe(1)["board"]["hand"] if i["token"] == learned)["used"])
        game.step(Command(0, "decline"))
        self.assertTrue(next(i for i in game.observe(1)["board"]["hand"] if i["token"] == learned)["used"])

    def test_disease_weapon_damage_and_mild_cure_before_owner_tick(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=0), seed=29)
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
        game = self.match(config=Config(initial_hand=18, initial_mp=99), seed=771)
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
        game = self.match(config=Config(initial_hand=18, initial_mp=99, initial_cp=99), seed=186)
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
        cut = self.match(config=Config(initial_hand=18, initial_mp=99, max_decisions=1), seed=186)
        token = next(i["token"] for i in cut.observe(0)["board"]["hand"] if i["face"] == 220)
        cut.step(Command(0, "play", (token,), 0))
        self.assertTrue(cut.status.truncated); self.assertFalse(cut.status.terminated)
        status = cut.observe()["board"]["players"][0]["status"]
        self.assertEqual((status["hp"], status["curses"]), (40, ["heaven"]))

    def test_self_curse_mask_fully_defends_and_discard_is_inert(self):
        config = Config(initial_hand=4, initial_mp=0)
        game = self.match(config=config, seed=82172)
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
        discarded = self.match(config=config, seed=82172)
        discarded.step(Command(0, "discard", (next(i["token"] for i in discarded.observe(0)["board"]["hand"] if discarded.legal(Command(0,"discard",(i["token"],)))),))); discarded.step(Command(1, "discard", (self.token(discarded, 1, 168),)))
        self.assertEqual([p["status"]["curses"] for p in discarded.observe()["board"]["players"]], [[], []])

    def test_heaven_self_curse_prefix_falls_then_revives_after_full_body(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=99), seed=30423)
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
        game = self.match(config=Config(initial_hand=4, initial_mp=90), seed=19847)
        self.assertEqual(game.observe()["disease_item_rule"], rule)
        game.step(Command(0, "play", (self.token(game, 0, 198),), 1))
        before, status = game.observe(1), game.status
        with self.assertRaises(InvalidMatchError): game.step(Command(1, "defend", (self.token(game, 1, 187),)))
        self.assertEqual((game.observe(1), game.status), (before, status))
        game.step(Command(1, "defend"))
        players = game.observe()["board"]["players"]
        self.assertEqual((players[0]["status"]["mp"], players[1]["status"]["mp"]), (90, 99))
        self.assertEqual((players[1]["status"]["hp"], players[1]["status"]["curses"]), (40, ["heaven"]))
        reflected = self.match(config=Config(initial_hand=4, initial_mp=90, max_decisions=3), seed=19847)
        reflected.step(Command(0, "play", (self.token(reflected, 0, 198),), 1)); reflected.step(Command(1, "defend", (self.token(reflected, 1, 190),))); reflected.step(Command(0, "defend"))
        self.assertTrue(reflected.status.truncated)
        players = reflected.observe()["board"]["players"]
        self.assertEqual((players[0]["status"]["mp"], players[1]["status"]["mp"]), (99, 90))
        self.assertEqual((players[0]["status"]["hp"], players[0]["status"]["curses"]), (40, ["heaven"]))
        discarded = self.match(config=Config(initial_hand=4, initial_mp=90), seed=19847)
        discarded.step(Command(0, "discard", (self.token(discarded, 0, 198),)))
        status = discarded.observe()["board"]["players"][0]["status"]
        self.assertEqual((status["mp"], status["curses"]), (90, []))

    def test_fog_view_mild_cure_and_private_inventory_independent_sell(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=99), seed=9901)
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
        game = self.match(config=Config(initial_hand=18, initial_mp=99), players=roster, seed=1569)
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
        game = self.match(config=Config(initial_hand=18, initial_mp=99), seed=962781)
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
        game = self.match(config=config, seed=75)
        fan = next(i["token"] for i in game.observe(0)["board"]["hand"] if i["face"] == 97)
        game.step(Command(0, "play", (fan,))); game.step(Command(1, "defend"))
        target = game.observe()["board"]["players"][1]["status"]
        self.assertEqual((target["hp"], target["curses"]), (37, ["fog"]))
        self.assertIsNone(game.observe(1)["board"]["players"][0]["status"])
        game = self.match(config=config, seed=137)
        club = next(i["token"] for i in game.observe(0)["board"]["hand"] if i["face"] == 6)
        ring = next(i["token"] for i in game.observe(1)["board"]["hand"] if i["face"] == 182)
        game.step(Command(0, "play", (club,), 1)); game.step(Command(1, "defend", (ring,)))
        pending = game.observe()["pending"]
        self.assertEqual((pending["counter"], pending["models"], pending["power"], game.actor), (True, [182], 0, 0))
        game.step(Command(0, "defend"))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["curses"], ["fog"])
        self.assertIsNone(game.observe(0)["board"]["players"][1]["status"])

    def test_dying_weapon_main_chance_and_natural_death_priority(self):
        for seed, hit in ((121, True), (35, False)):
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
        game = self.match(config=Config(initial_hand=18, initial_hp=1, initial_mp=0), seed=13535)
        game.step(Command(0, "play", (self.token(game, 0, 112),))); game.step(Command(1, "defend"))
        self.assertTrue(game.status.terminated); self.assertEqual(game.status.winners, (0,))
        self.assertIsNone(game.observe()["pending"])
        self.assertIn(26, {i["token"] for i in game.observe(1)["board"]["hand"]})

    def test_posthumous_dark_death_has_dead_source_and_fixed_rewards(self):
        roster = [PlayerSetup(0), PlayerSetup(7), PlayerSetup(9)]
        game = self.match(config=Config(initial_hand=18, initial_hp=5, initial_mp=0), players=roster, seed=94)
        game.step(Command(0, "play", (16,), 7)); game.step(Command(7, "defend"))
        pending = game.observe()["pending"]
        self.assertEqual((game.actor, pending["source"], pending["target"], pending["power"], pending["hit_rate"], pending["death"]), (0, 7, 0, 30, 75, True))
        self.assertTrue(game.observe()["board"]["players"][1]["dead"])
        self.assertNotIn(32, {i["token"] for i in game.observe(7)["board"]["hand"]})
        for seed in range(32):
            command, _ = game.sample_action(seed); self.assertTrue(game.legal(command))
        game.step(Command(0, "defend"))
        self.assertTrue(game.status.terminated); self.assertEqual(game.status.winners, (9,))
        self.assertEqual(game.status.rewards, (-1.0, -1.0, 1.0))
        self.assertIsNone(game.observe()["pending"])
        self.assertEqual([p["status"]["hp"] for p in game.observe()["board"]["players"]], [0, 0, 5])

    def test_posthumous_reflection_to_dead_source_cancels_then_fifo_continues(self):
        roster = [PlayerSetup(0), PlayerSetup(7), PlayerSetup(9)]
        game = self.match(config=Config(initial_hand=18, initial_hp=5, initial_mp=0), players=roster, seed=31204)
        game.step(Command(0, "play", (12,), 7)); game.step(Command(7, "defend"))
        self.assertTrue(game.observe()["pending"]["death"])
        game.step(Command(0, "defend", (3,)))
        pending = game.observe()["pending"]
        self.assertEqual((game.actor, pending["source"], pending["target"], pending["power"], pending["death"]), (9, 7, 9, 30, True))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["hp"], 5)
        self.assertNotIn(3, {i["token"] for i in game.observe(0)["board"]["hand"]})
        game.step(Command(9, "defend"))
        self.assertTrue(game.status.terminated); self.assertEqual(game.status.winners, (0,))
        self.assertEqual(game.status.rewards, (1.0, -1.0, -1.0))

    def test_posthumous_revival_precedes_reservation_and_cutoff_cancels_activation(self):
        roster = [PlayerSetup(0), PlayerSetup(7), PlayerSetup(9)]
        game = self.match(config=Config(initial_hand=18, initial_hp=5, initial_mp=0), players=roster, seed=3893)
        game.step(Command(0, "play", (7,), 7)); game.step(Command(7, "defend"))
        target = game.observe()["board"]["players"][1]
        self.assertFalse(target["dead"]); self.assertEqual(target["status"]["hp"], 10)
        self.assertIn(18, {i["token"] for i in game.observe(7)["board"]["hand"]})
        self.assertNotIn(35, {i["token"] for i in game.observe(7)["board"]["hand"]})
        self.assertIsNone(game.observe()["pending"]); self.assertEqual(game.actor, 7)
        cut = self.match(config=Config(initial_hand=18, initial_hp=5, initial_mp=0, max_decisions=2), players=roster, seed=94)
        cut.step(Command(0, "play", (16,), 7)); cut.step(Command(7, "defend"))
        self.assertTrue(cut.status.truncated); self.assertFalse(cut.status.terminated)
        self.assertEqual(cut.status.rewards, (0.0, 0.0, 0.0)); self.assertIsNone(cut.observe()["pending"])
        self.assertNotIn(32, {i["token"] for i in cut.observe(7)["board"]["hand"]})
        self.assertEqual([p["status"]["hp"] for p in cut.observe()["board"]["players"]], [5, 0, 5])
        self.assertNotIn("deaths", cut.observe()); self.assertNotIn("death_created", cut.observe())

    def test_full_sale_uniform_eviction_public_model_and_query_purity(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=0, initial_cp=99), seed=11)
        original = game.observe(1)["board"]["hand"]
        command = Command(0, "sell", (7, 16), 1)
        before, status = game.observe(0), game.status
        self.assertTrue(game.legal(command)); targets = set()
        for seed in range(512):
            proposal, _ = game.sample_action(seed); self.assertTrue(game.legal(proposal))
            if proposal.kind == "sell": targets.add(proposal.target)
        self.assertEqual(targets, {0, 1}); self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(command); game.step(Command(1, "defend"))
        expected = {"origin": 0, "source": 0, "target": 1, "model": 153, "price": 10, "overflow_model": 191}
        for viewer in (None, 0, 1): self.assertEqual(game.observe(viewer)["last_sale"], expected)
        after = game.observe(1)["board"]["hand"]
        self.assertEqual({i["token"] for i in original} - {i["token"] for i in after}, {31})
        self.assertEqual(next(i["face"] for i in after if i["token"] == 36), 153)
        self.assertEqual([len(game.observe(seat)["board"]["hand"]) for seat in (0, 1)], [17, 18])
        self.assertEqual([p["status"]["cp"] for p in game.observe()["board"]["players"]], [99, 89])
        self.assertEqual(set(game.observe()["last_sale"]), set(expected))

    def test_full_sale_can_evict_used_miracle_without_replacement_or_recast(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=99), seed=1812)
        game.step(Command(0, "discard", (0,))); game.step(Command(1, "play", (19,), 1))
        self.assertTrue(next(i for i in game.observe(1)["board"]["hand"] if i["token"] == 19)["used"])
        game.step(Command(0, "sell", (13, 12), 1)); game.step(Command(1, "defend"))
        self.assertEqual(game.observe()["last_sale"]["overflow_model"], 235)
        self.assertNotIn(19, {i["token"] for i in game.observe(1)["board"]["hand"]})
        self.assertEqual(game.observe()["board"]["players"][1]["used_models"], [])
        self.assertEqual([len(game.observe(seat)["board"]["hand"]) for seat in (0, 1)], [16, 18])
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 50)
        invalid = Command(1, "play", (19,), 1); before, status = game.observe(1), game.status
        self.assertFalse(game.legal(invalid))
        with self.assertRaises(InvalidMatchError): game.step(invalid)
        self.assertEqual((game.observe(1), game.status), (before, status))

    def test_full_sale_evicts_amulet_before_death_and_does_not_trigger_it(self):
        game = self.match(config=Config(initial_hand=18, initial_hp=5, initial_mp=0, initial_cp=0), seed=5344)
        self.assertEqual(next(i["face"] for i in game.observe(1)["board"]["hand"] if i["token"] == 23), 208)
        game.step(Command(0, "sell", (2, 12), 1)); game.step(Command(1, "defend"))
        self.assertTrue(game.status.terminated); self.assertEqual(game.status.winners, (0,))
        self.assertEqual(game.observe()["last_sale"], {"origin": 0, "source": 0, "target": 1, "model": 38, "price": 5, "overflow_model": 208})
        target = game.observe(1)
        self.assertEqual(target["board"]["players"][1]["status"]["hp"], 0)
        self.assertNotIn(23, {i["token"] for i in target["board"]["hand"]})
        self.assertEqual(next(i["face"] for i in target["board"]["hand"] if i["token"] == 36), 38)
        self.assertEqual(len(game.observe(0)["board"]["hand"]), 16)

    def test_fog_foreign_sale_uses_overflow_and_keeps_covered_status_private(self):
        game = self.match(config=Config(initial_hand=18, initial_mp=99), seed=9901)
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
        game = self.match(config=Config(initial_hand=17), seed=1583)
        initial = game.observe(0)["board"]["hand"]
        self.assertFalse(any(item["disguised"] for item in initial))
        game.step(Command(0, "play", (9,), 0))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["curses"], ["dream"])
        hand = game.observe(0)["board"]["hand"]
        masked = next(item for item in hand if item["token"] == 34)
        self.assertEqual(masked, {"token": 34, "face": 54, "reference": 54, "disguised": True, "used": False})
        self.assertEqual([item["token"] for item in hand if item["disguised"]], [34])
        self.assertTrue(next(item for item in hand if item["token"] == 9)["used"])
        self.assertEqual(game.observe()["last_reveals"], [])
        game.step(Command(1, "discard", (self.token(game, 1, 195),)))
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
        self.assertEqual((pending["models"], pending["power"]), ([29], 8))
        event = [{"owner": 0, "face": 54, "model": 29}]
        for viewer in (None, 0, 1): self.assertEqual(game.observe(viewer)["last_reveals"], event)
        before, status = game.observe(0), game.status
        with self.assertRaises(InvalidMatchError): game.step(Command(0, "play", (34,), 1))
        self.assertEqual((game.observe(0), game.status), (before, status))
        game.step(Command(1, "defend"))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 32)
        self.assertEqual(game.observe()["last_reveals"], event)
        token = next(item["token"] for item in game.observe(1)["board"]["hand"] if game.legal(Command(1, "discard", (item["token"],))))
        game.step(Command(1, "discard", (token,)))
        self.assertEqual(game.observe()["last_reveals"], [])
        game.reset(1583); self.assertEqual(game.observe()["last_reveals"], [])

    def test_dream_mild_preserves_and_all_cure_reveals_hand_only_to_owner(self):
        game = self.match(config=Config(initial_hand=17), seed=1583)
        game.step(Command(0, "play", (9,), 0))
        game.step(Command(1, "discard", (self.token(game, 1, 195),)))
        game.step(Command(0, "play", (1,), 0))
        self.assertIn("dream", game.observe()["board"]["players"][0]["status"]["curses"])
        masked = next(item for item in game.observe(0)["board"]["hand"] if item["token"] == 34)
        self.assertEqual((masked["face"], masked["reference"], masked["disguised"]), (54, 54, True))
        self.assertEqual(game.observe()["last_reveals"], [])
        token = next(item["token"] for item in game.observe(1)["board"]["hand"] if game.legal(Command(1, "discard", (item["token"],))))
        game.step(Command(1, "discard", (token,)))
        hidden_count = sum(item["disguised"] for item in game.observe(0)["board"]["hand"])
        game.step(Command(0, "play", (4,), 0))
        self.assertNotIn("dream", game.observe()["board"]["players"][0]["status"]["curses"])
        hand = game.observe(0)["board"]["hand"]
        self.assertFalse(any(item["disguised"] for item in hand))
        self.assertEqual(next(item["face"] for item in hand if item["token"] == 34), 29)
        private = game.observe(0)["last_reveals"]
        self.assertEqual(len(private), hidden_count)
        self.assertIn({"owner": 0, "face": 54, "model": 29}, private)
        self.assertTrue(all(set(event) == {"owner", "face", "model"} for event in private))
        self.assertEqual(game.observe()["last_reveals"], [])
        self.assertEqual(game.observe(1)["last_reveals"], [])
        for key in ("dream_rolls", "dream_masks", "fake_models", "initial_state"):
            self.assertNotIn(key, game.observe(0))

    def test_dream_weapon_affects_future_gifts_without_retroactive_masking(self):
        game = self.match(config=Config(initial_hand=18), seed=649)
        game.step(Command(0, "play", (15,), 1))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["curses"], [])
        game.step(Command(1, "defend"))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["curses"], ["dream"])
        self.assertFalse(any(item["disguised"] for item in game.observe(1)["board"]["hand"]))
        game.step(Command(1, "play", (22,), 1))
        item = next(item for item in game.observe(1)["board"]["hand"] if item["token"] == 37)
        self.assertEqual((item["face"], item["reference"], item["disguised"]), (10, 10, True))
        self.assertEqual(game.observe()["board"]["players"][1]["status"]["hp"], 35)
        self.assertFalse(any(item["disguised"] for item in game.observe(0)["board"]["hand"]))

    def test_dream_counter_is_nonattack_and_masks_owner_replacement(self):
        game = self.match(config=Config(initial_hand=18), seed=36623)
        club, filter_item, ring = self.token(game, 0, 6), self.token(game, 1, 189), self.token(game, 1, 183)
        game.step(Command(0, "play", (club,), 1))
        game.step(Command(1, "defend", (filter_item, ring)))
        pending = game.observe()["pending"]
        self.assertEqual((game.actor, pending["counter"], pending["models"], pending["power"]), (0, True, [183], 0))
        game.step(Command(0, "defend"))
        self.assertEqual(game.observe()["board"]["players"][0]["status"]["curses"], ["dream"])
        item = next(item for item in game.observe(0)["board"]["hand"] if item["token"] == 36)
        self.assertEqual((item["face"], item["reference"], item["disguised"]), (6, 6, True))
        self.assertEqual(game.observe()["last_reveals"], [])

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
