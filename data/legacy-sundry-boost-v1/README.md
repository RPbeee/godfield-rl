# Legacy physical SUNDRY boost v1

Only238 is added: SUNDRY/INCREASE_ATK, source value blank and subValue10, price15,
weight2, neutral and discardable. Card has base attack0, boost10 and DEF/HP/MP0;
there is no MP cost, rate, status or holding. The physical extra is not a weapon
base or standalone USE. The combined pool is225/weight479. All34 prior catalogs,
hashes, Kind values, retained IDs and indices remain fixed.

[Item.getAtk][value] uses subValue for SUNDRY extras. [Attack validation][valid]
allows this extra after a supported guaranteed WEAPON base, subject to existing
PESTLE/percentage restrictions. [Sequential processing][sequence] adds10 at its
position: later DOUBLE scales it, and later paid MAGIC on57 can reset the current
amount. A physical238 pays no MP and does not itself reset57 or learn a miracle.
A FREE adjacent to238 cannot be redirected to waive a different MAGIC.

Canonical defense excludes no-DEF238. The source [weak response guard][defense]
can consume it as neutral zero-DEF/prefix defense. Response-time hand cycling may
have strategic value, so this acceptance is not described as redundant. It is
recorded separately rather than added as a defense role; official UI acceptance
remains unverified.

The generator checks source/catalog hashes, generated C++, header fingerprint,
all old34 catalog hashes and ID disjointness, plus the frozen216 price table hash.
The runtime price helper adds15 only when enabled and preserves existing fallbacks.
Boost10 remains within old15 limits; max merchandise price30 and raw/counter bounds
stay unchanged. Historical IDs/labels/prices/weights are not verified current official
identifiers, names or probabilities. Master was observed2026-11-01; immutable
retrieval was unavailable, so history head does not establish content identity.

```sh
python3 data/legacy-sundry-boost-v1/generate.py
python3 data/legacy-sundry-boost-v1/generate.py --check
```

[value]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/item.py#L111-L126
[valid]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/attackData.py#L78-L94
[sequence]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L226-L286
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L534-L587
