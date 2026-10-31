# Legacy standalone offensive miracles v1

This independent allowlist adds 13 positive-damage historical miracles, all
with legacy weight1. Six are guaranteed attacks and seven have positive hit
rates. ID200 preserves Light, 25% and Glory together. Earlier catalogs and
fingerprints remain unchanged, including the original two utility miracles.

| Old ID | Element | Damage | MP cost | Hit-rate field | Extra | Local index / shared cast slot |
| --- | --- | --- | --- | --- | --- | --- |
| 198 | Water | 4 | 2 | 0 | None | 0 / 2 |
| 199 | Wood | 6 | 3 | 0 | None | 1 / 3 |
| 200 | Light | 1 | 3 | 25 | Glory | 2 / 4 |
| 203 | Stone | 8 | 4 | 0 | None | 3 / 5 |
| 204 | Fire | 5 | 4 | 75 | None | 4 / 6 |
| 205 | Light | 10 | 4 | 25 | None | 5 / 7 |
| 206 | Fire | 10 | 5 | 0 | None | 6 / 8 |
| 207 | Dark | 5 | 5 | 0 | None | 7 / 9 |
| 218 | Water | 9 | 7 | 75 | None | 8 / 10 |
| 219 | Stone | 13 | 8 | 50 | None | 9 / 11 |
| 222 | Water | 25 | 12 | 0 | None | 10 / 12 |
| 223 | Fire | 16 | 12 | 75 | None | 11 / 13 |
| 226 | Water | 30 | 30 | 75 | None | 12 / 14 |

The zero hit-rate field is the historical guaranteed-hit sentinel, not a 0%
success rate. Positive-rate miracles use the same historical all-enemy
construction as percentage weapons: in a duel the target normalizes to the
opponent, rather than generating repeated hits on one target. Glory requires
the proper post-hit damage condition; it is not unconditional on casting.

Only standalone hand use and retained recasting belong to this scope. ID197
and ID217 have `INCREASE_ATK` and can also participate in weapon/miracle
composition, so they remain wholly excluded. ID221 has `ABSORB_HP` and is
excluded without deleting its healing effect. Non-damage, wide/double,
defensive, summon and other status miracles are not admitted.

Old utility slots remain ID216 -> 0 and ID220 -> 1. The new catalog returns
local indices 0..12; the engine's shared cast slots are 2..14. This keeps all
15 types inside a uint16 ability mask. The original `Miracle` type, catalog
fingerprint and utility ABI are unchanged. No acquisition-order semantics are
added by this representation.

`source-rows.csv` preserves original fields, including price, and zero-based
indices from the historical emulator's
[ItemData.CSV](https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV).
`catalog.csv` normalizes the runtime fields and stable local indices. Technical
labels are not verified current official names or IDs. The generator checks
normalization, index mapping, the old utility fingerprint and ID disjointness
from all eight earlier catalogs.

`manifest.json` independently fingerprints the source and normalized files.
Immutable raw content could not be fetched, so observed master rows are not
asserted byte-identical to the recorded branch-history revision. Old weights
are not verified official acquisition probabilities; upstream's
[README](https://github.com/Igoorx/godfield-flash#known-issues) reports distribution
uncertainty. Current official-client compatibility is unverified.

Regenerate and verify from the repository root:

```sh
python3 data/legacy-offensive-miracles-v1/generate.py
python3 data/legacy-offensive-miracles-v1/generate.py --check
```
