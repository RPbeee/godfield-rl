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
        self.assertEqual(match.ruleset, "official-execution-subset-v4"); self.assertIn("156-models", match.scope); self.assertIn("not-official-server-equivalence", match.scope)
        self.assertEqual((len(match.supported_models), match.gift_weight), (156, 337))
        self.assertIn(212, match.supported_models); self.assertNotIn(112, match.supported_models); self.assertNotIn(14, match.supported_models)
        self.assertEqual(match.status.player_ids, (0, 77)); self.assertEqual(match.actor, 0)
        own = match.observe(0); spectator = match.observe()
        self.assertEqual(own["schema"], "official-execution-view-v2")
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
        # Bounce reactions and chance remain explicitly outside v4.
        self.assertTrue({21, 34, 95, 234}.isdisjoint(match.supported_models))
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
