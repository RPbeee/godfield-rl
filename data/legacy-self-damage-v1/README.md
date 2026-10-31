# Legacy self damage v1

Source53, technical label `legacy-053`, is the only positive-weight
DAMAGE_TO_SELF artifact: WEAPON, neutral ATK14, price1, weight1. It has no boost,
DEF, HP/MP recovery, MP cost, hit-rate roll, harm or disease. Ordinary weapon
DISCARD is unavailable. Dedicated catalog membership preserves the extra effect.

[Resolution][effect] uses positive non-DARK residual: current source loses that
raw amount first, then current target loses the same amount. Each actual HP loss
clips independently; a small target HP pool does not cap source loss. Self use
applies both losses in order. Reflection/bounce changes current source/target;
final death processing, REVIVE244 and owner disease tick occur after the pair.
Derived counters do not inherit DAMAGE_TO_SELF. A zero residual diagnostic event
has zero losses; miss/block or a continuing reflection produces no resolution.

The source DARK branch skips this extra, but current legal compositions of neutral53
cannot become DARK. PESTLE99/MORTAR245 remain excluded. No unsupported behavior is
removed to fit the allowlist. All twenty-eight older catalogs, hashes and indices
remain fixed, yielding213 artifacts with total weight427.

`source-rows.csv` retains each original column; `catalog.csv` normalizes Card
fields. The generator checks pinned source/catalog hashes, generated C++, header
fingerprint, all old28 catalog hashes and ID disjointness. IDs/labels/weights are
not verified current official identifiers, names or acquisition probabilities.
Master was observed2026-11-01; immutable retrieval was unavailable, so the recorded
history head does not establish content identity.

```sh
python3 data/legacy-self-damage-v1/generate.py
python3 data/legacy-self-damage-v1/generate.py --check
```

[effect]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L486-L494
