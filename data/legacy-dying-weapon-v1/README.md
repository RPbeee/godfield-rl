# legacy-dying-weapon-v1

Pinned old CSV ID117 retains LIGHT ATK1,75%,DYING_ATTACK30,price10,weight1.
The complete extra is preserved in `dying_amount`; it is not an ordinary boost or MP cost.
Only ordinary use is reachable in two-player duel. `handlePlayersDeath` revives244 and continues,
or assigns dead before testing alive>=2 for117; `getAliveCount` counts not.dead.
Thus117 never launches its death queue in duel, even with244/simultaneous HP0/counters/disease.
Multiplayer requires implementation of the death queue and is outside this profile.

`manifest.json` fixes the original CSV SHA256 plus fixed-commit room/turn code hashes.
The immutable CSV is archived in the preceding v36 dataset. Source implementations are not bundled.
All36 prior catalog fingerprints and the old216 sale-price rows remain unchanged.
Combined pool231 types/weight485. IDs/names/weights do not assert current official equivalence.

Run `python3 data/legacy-dying-weapon-v1/generate.py --check` from the project root.
