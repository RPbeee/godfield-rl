# Legacy HP gamble v1

Only237 is added: SUNDRY/INCREASE_OR_DECREASE_HP, amount10, price1, weight2,
discardable. All ordinary Card ATK/boost/DEF/HP/MP values are0, without MP cost,
hit rate, disease, harm or special defense. Dedicated membership preserves the
random signed HP effect rather than an unconditional heal. The combined pool is
218 artifacts with weight471. All32 previous catalogs, types, hashes, retained IDs
and indices stay fixed. MYSTERY243 and unrelated random multi-effects stay excluded.

[Launch][launch] requires one physical piece and selects a single sign:0 means-10,
1 means+10. It earns one source gift, permits self/opponent targeting, and accepts
no additions/FREE. The signed result is public before a response and remains
unchanged through reflection. The active payload is None, distinct from blank
ordinary attribute metadata; only the existing first196 bypass can reflect it.

[Resolution][resolve] changes current target HP by signed10, capped0/99. The actual
HP change is recorded before244/death processing and owner natural disease tick.
It is not ordinary raw damage and generates no counters or elemental defense.
The old216 price table/hash remain fixed and still exclude237. With hp_gamble
enabled the core uses catalog price1, retaining the independent BUY4 price5 fallback.
The supported merchandise maximum remains30.

The generator checks source/catalog hashes, generated C++, header fingerprint,
all old32 catalog hashes and ID disjointness, and the frozen216 price table hash.
Historical IDs/labels/prices/weights are not verified current official identifiers,
names or probabilities. Master was observed2026-11-01; immutable retrieval was
unavailable, so history head does not establish content identity.

```sh
python3 data/legacy-hp-gamble-v1/generate.py
python3 data/legacy-hp-gamble-v1/generate.py --check
```

[launch]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L219-L222
[resolve]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L471-L476
