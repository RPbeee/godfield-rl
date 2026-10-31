# Legacy neutral catalog v1

`catalog.csv` is a normalized allowlist of 60 historical artifact records:
35 weapons, 18 armor records, four HP items and three MP items. `catalog.cpp`
is generated from this file, and the fingerprint is recorded in `manifest.json`.

Only neutral, deterministic records without special defense behavior are included.
Weapon boosts and armor boosts remain separate from base attack and defense.
Seven weapons have both attack and defense; four armor records also boost attacks.
All weapon records are non-discardable. Other included records are discardable.

IDs preserve zero-based row positions in the historical
[ItemData.CSV](https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV).
`legacy-NNN` is an internal label, not an official name. The table does not assert
that legacy IDs map to current official IDs. ID 0 is not an artifact in this pool.
Excluded and unknown IDs must be rejected, rather than treated as plain weapons.

The recorded branch-head commit comes from the repository's
[commit history](https://github.com/Igoorx/godfield-flash/commits/master/).
Fetching that immutable raw revision was unavailable, so the manifest explicitly
does not claim the retrieved master data is byte-identical to that commit.
The local normalized facts are pinned by SHA-256 instead. No complete upstream
CSV or third-party game assets are bundled.

Weights are historical emulator values. Its
[README](https://github.com/Igoorx/godfield-flash#known-issues) explicitly reports
uncertainty about artifact distribution. Normalizing weights over this selected
pool creates a restricted curriculum, not official acquisition probabilities.
Current-client compatibility remains unverified.

Regenerate and verify from the repository root:

```sh
python3 data/legacy-neutral-v1/generate.py
python3 data/legacy-neutral-v1/generate.py --check
```

This subset intentionally excludes trade, miracles, elements, conditions,
reflection/counters, guardians, probabilistic attacks, and sundry booster ID238.
