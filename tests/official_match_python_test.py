import copy
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from official_match import Match, Config, PlayerSetup, Command, MatchError, InvalidMatchError, UnsupportedMatchError
LIBRARY = sys.argv.pop(1) if len(sys.argv) > 1 else None

class MatchTests(unittest.TestCase):
    def match(self, **kwargs):
        value = Match(library=LIBRARY, **kwargs); self.addCleanup(value.close); return value

    def test_descriptor_and_public_observation(self):
        match = self.match(players=[PlayerSetup(0, "A\0B"), PlayerSetup(77, "相手")])
        self.assertEqual(match.ruleset, "official-execution-subset-v9"); self.assertIn("194-models", match.scope); self.assertIn("not-official-server-equivalence", match.scope)
        self.assertEqual((len(match.supported_models), match.gift_weight), (194, 381))
        self.assertIn(212, match.supported_models); self.assertNotIn(112, match.supported_models); self.assertNotIn(14, match.supported_models)
        self.assertEqual(match.status.player_ids, (0, 77)); self.assertEqual(match.actor, 0)
        own = match.observe(0); spectator = match.observe()
        self.assertEqual(own["schema"], "official-execution-view-v7")
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
        # Death-passive chance, curse counters remain outside v8.
        self.assertTrue({112, 182, 183, 185, 186}.isdisjoint(match.supported_models))
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
        for seed in range(8192):
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
        match.reset(35918); reference = self.match(config=config, seed=35918)
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
        self.assertEqual(event["schema"], "official-execution-view-v7")
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
        multi = self.match(config=config, players=roster, seed=21)
        reference = self.match(config=config, players=roster, seed=21)
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
        self.assertEqual(event["chance_rule"], "experimental:targetless-all-alive-enemies-in-input-order;roll-before-response;redirects-no-reroll;skip-dead;cancel-on-origin-death")
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
        match.reset(63)
        chance = next(item["token"] for item in match.observe(0)["board"]["hand"] if item["face"] == 95)
        result = match.step(Command(0, "play", (chance,)))
        self.assertEqual((result.decisions, result.phase, result.actor, result.owner), (1, "main", 1, 1))
        self.assertIsNone(match.observe()["pending"])
        self.assertEqual([p["status"]["hp"] for p in match.observe()["board"]["players"]], [40, 40])
        # A chance miracle accepts FREE but a hit remains hit across reflection.
        match.reset(602); reference = self.match(config=config, seed=602)
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
        match = self.match(config=config, seed=1281)
        reference = self.match(config=config, seed=1281)
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
        self.assertEqual(event["counter_rule"], "experimental:actual-hp-loss;survivors-only;selection-order-fifo-before-next-target;skip-dead-source-or-target;single-target;roll-before-response;redirects-no-reroll")
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
        match.reset(4023)
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
        match = self.match(config=config, seed=201); reference = self.match(config=config, seed=201)
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
        capped = self.match(config=Config(initial_hand=18, initial_mp=98), seed=201)
        own, other = hand(capped, 0), hand(capped, 1)
        capped.step(Command(0, "play", (own[6],), 1)); capped.step(Command(1, "defend", (other[187],))); capped.step(Command(1, "defend"))
        self.assertEqual(stats(capped, "mp"), [98, 99])
        # A self-reflection of the MP counter keeps a response and applies once.
        mirror = self.match(config=config, seed=1885)
        own, other = hand(mirror, 0), hand(mirror, 1)
        self.assertIn(6, own); self.assertTrue({187, 190} <= other.keys())
        mirror.step(Command(0, "play", (own[6],), 1)); mirror.step(Command(1, "defend", (other[187],)))
        mirror.step(Command(1, "defend", (other[190],)))
        self.assertEqual((mirror.actor, mirror.observe()["pending"]["source"], mirror.observe()["pending"]["target"]), (1, 1, 1))
        self.assertEqual(stats(mirror, "mp"), [0, 0])
        mirror.step(Command(1, "defend")); self.assertEqual(stats(mirror, "mp"), [0, 2])
        # Venus credits the full nominal amount after CP->MP->HP debt payment.
        cp = self.match(config=config, seed=5422)
        own, other = hand(cp, 0), hand(cp, 1)
        self.assertIn(32, own); self.assertIn(188, other)
        cp.step(Command(0, "play", (own[32],), 1)); cp.step(Command(1, "defend", (other[188],)))
        pending = cp.observe()["pending"]
        self.assertEqual((cp.actor, pending["source"], pending["target"], pending["power"], pending["amount"]), (0, 1, 0, 0, 9))
        cp.step(Command(0, "defend"))
        self.assertEqual(stats(cp, "hp"), [31, 31]); self.assertEqual(stats(cp, "mp"), [0, 0]); self.assertEqual(stats(cp, "cp"), [0, 9])
        self.assertIsNone(cp.observe()["pending"])
        cp_cap = self.match(config=Config(initial_hand=18, initial_mp=0, initial_cp=99), seed=5422)
        own, other = hand(cp_cap, 0), hand(cp_cap, 1)
        cp_cap.step(Command(0, "play", (own[32],), 1)); cp_cap.step(Command(1, "defend", (other[188],))); cp_cap.step(Command(0, "defend"))
        self.assertEqual(stats(cp_cap, "cp"), [90, 99])
        # Reflection changes creditor/debtor through the current source/target.
        reflected = self.match(config=config, seed=1077)
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
            alias = self.match(config=Config(initial_hand=18, initial_hp=hp, initial_mp=0, initial_cp=0), seed=5422)
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
            effect = self.match(config=Config(initial_hand=18, initial_hp=initial_hp, initial_mp=0), seed=17)
            own = hand(effect, 0); self.assertIn(27, own)
            effect.step(Command(0, "play", (own[27],), target))
            self.assertEqual(effect.observe()["damage_effect_rule"], rule)
            effect.step(Command(target, "defend")); self.assertEqual(hp(effect), expected)
            if initial_hp == 3: self.assertEqual(effect.status.winners, (0,))
        match.reset(17); own, other = hand(match, 0), hand(match, 1)
        self.assertIn(176, other)
        match.step(Command(0, "play", (own[27],), 1)); match.step(Command(1, "defend", (other[176],)))
        self.assertEqual(hp(match), [40, 40])  # No source heal at zero remainder.
        # Current source receives the absorption effect after reflection.
        match.reset(427); own, other = hand(match, 0), hand(match, 1)
        self.assertIn(27, own); self.assertIn(38, other)
        match.step(Command(0, "play", (own[27],), 1)); match.step(Command(1, "defend", (other[38],)))
        self.assertEqual(hp(match), [40, 40])
        self.assertEqual((match.observe()["pending"]["source"], match.observe()["pending"]["target"]), (1, 0))
        match.step(Command(0, "defend")); self.assertEqual(hp(match), [33, 47])
        # Self-heal and target damage net to zero, but body damage7 still creates counter14.
        match.reset(1317); own = hand(match, 0)
        self.assertTrue({27, 184} <= own.keys())
        match.step(Command(0, "play", (own[27],), 0)); match.step(Command(0, "defend", (own[184],)))
        self.assertEqual(hp(match), [40, 40]); self.assertEqual(match.observe()["pending"]["power"], 14)
        match.step(Command(0, "defend")); self.assertEqual(hp(match), [26, 40])
        # The chance absorber keeps its ability through hit/miss handling.
        for seed, expected in ((27, [43, 37]), (126, [40, 40])):
            match.reset(seed); token = hand(match, 0)[98]
            match.step(Command(0, "play", (token,)))
            if match.status.phase == "response": match.step(Command(1, "defend"))
            self.assertEqual(hp(match), expected)
        # A paid first use consumes the declared10 MP and retains the same instance.
        paid = self.match(config=Config(initial_hand=18, initial_mp=10), seed=1461)
        paid_token = hand(paid, 0)[216]
        paid.step(Command(0, "play", (paid_token,), 1))
        self.assertEqual([p["status"]["mp"] for p in paid.observe()["board"]["players"]], [0, 10])
        paid.step(Command(1, "defend")); self.assertEqual(hp(paid), [50, 30])
        self.assertTrue(next(item for item in paid.observe(0)["board"]["hand"] if item["token"] == paid_token)["used"])
        # The held Light miracle accepts FREE, then heals the reflected current source.
        match.reset(1461); reference = self.match(config=config, seed=1461)
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
            effect = self.match(config=Config(initial_hand=18, initial_hp=initial_hp, initial_mp=0), seed=25)
            token = hand(effect, 0)[53]
            effect.step(Command(0, "play", (token,), target)); effect.step(Command(target, "defend"))
            self.assertEqual(hp(effect), expected)
            if initial_hp == 20: self.assertEqual(effect.status.winners, (1,))
        match.reset(240); own = hand(match, 0)
        self.assertTrue({53, 184} <= own.keys())
        match.step(Command(0, "play", (own[53],), 0)); match.step(Command(0, "defend", (own[184],)))
        self.assertEqual(hp(match), [12, 40])
        self.assertEqual(match.observe()["pending"]["power"], 28)  # Target14, not net28 doubled.
        match.step(Command(0, "defend")); self.assertEqual(match.status.winners, (1,))

    def test_sampler_pure_policy_stream_and_seeded_replay(self):
        a, b = self.match(), self.match()
        initial = a.observe(0); status = a.status
        one, r1 = a.sample_action(123); two, r2 = a.sample_action(123)
        self.assertEqual((one, r1), (two, r2)); self.assertEqual(a.observe(0), initial); self.assertEqual(a.status, status)
        ra = rb = 999
        for unused in range(256):
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
            with self.subTest(players=players), self.assertRaises(InvalidMatchError): match.reset(1, players)
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
