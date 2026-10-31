# Legacy physical MAGIC bounce v1

All positive-weight physical FLICK_MAGIC artifacts are included:21 is a neutral
WEAPON with ATK5 and boost5, price10;119/121/127/134/140 are neutral armors with
DEF0/1/2/3/4, price5. Every card has weight1, no MP cost, hit-rate roll, disease or
harm. The weapon is non-discardable; the armors are discardable. Dedicated catalog
membership preserves the MAGIC-only bounce, without stripping21's base/boost or
the ordinary armor roles. Each stable local index follows ascending ID0..5.

[Matching response][response] consumes the reached physical card and earns one
gift, with no MP payment, retained learning or held-cast role. It changes current
source to the responder and chooses an alive target including self via the existing
bounce-target mechanism. Self resolves immediately; another target continues
response. Amount, utility kind, original miracle and other payload markers remain
unchanged, with no chance reroll or MP repayment. The existing [final attribute
check][validation] still applies. Non-MAGIC payloads do not trigger this effect.
The six physical cards are separate from retained MAGIC210.

`source-rows.csv` retains every original column; `catalog.csv` preserves Card roles
and prices. The generator checks source/catalog hashes, generated C++, header
fingerprint, all old33 catalog hashes and ID disjointness, plus the frozen216 price
table. Runtime price fallbacks add these six only when enabled while retaining4/237.
The pool becomes224/weight477; merchandise max30 and attack/counter bounds stay
unchanged. Historical IDs/labels/prices/weights are not verified current official
identifiers, names or probabilities. Master was observed2026-11-01; immutable
retrieval was unavailable, so history head does not establish content identity.

```sh
python3 data/legacy-magic-bounce-v1/generate.py
python3 data/legacy-magic-bounce-v1/generate.py --check
```

[response]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L534-L581
[validation]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L630-L643
