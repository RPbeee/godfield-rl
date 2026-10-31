# Legacy pestle and mortar v1

Positive-weight PESTLE99 is a LIGHT WEAPON, ATK30, price5, weight1. Its paired
MORTAR245 is a SUNDRY with blank ordinary attribute metadata, price0, weight1 and
all other numeric fields0. Neither has MP cost, hit-rate roll, boost, DEF, recovery,
harm or disease. Dedicated catalog membership preserves each extra effect.

[Pestle processing][select] accepts exactly one piece. It searches not.dead seats
in fixed participant order for the first245 holder. Finding one sets raw999 and
that target without consuming245; otherwise it selects a random alive seat,
including self, and retains raw30. The original LIGHT attribute remains active.
The initial selection becomes historical public information only once it happens;
unselected opponent hand contents remain private.

[Responses][response] still use ordinary and special validation, counter enqueue
and damage. The mortar log/early return occurs after these operations and is not
a defense prohibition. Reflection/bounce preserves amount and mortar provenance.
Current source/target may change while the initial-selection history stays fixed.

MORTAR has no main-use command. [DISCARD][discard] may select it but skips its
actual consumption. `Card.discardable=false` records that effective rule, without
claiming the server rejects a mixed discard command containing245. Forced hand
removal240 can still remove it, without a replacement gift. PESTLE is an ordinary
non-discardable weapon and does not accept additions, including FREE.

`source-rows.csv` retains every source column; `catalog.csv` normalizes Card fields
and the extra-specific metadata. The generator checks pinned source/catalog hashes,
generated C++, header fingerprint, all29 prior catalog hashes and ID disjointness.
The combined pool becomes215 artifacts with total weight429. Historical IDs,
labels and weights are not verified current official identifiers, names or acquisition
probabilities. Master was observed2026-11-01; immutable retrieval was unavailable,
so the recorded history head does not establish content identity.

```sh
python3 data/legacy-pestle-mortar-v1/generate.py
python3 data/legacy-pestle-mortar-v1/generate.py --check
```

[select]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L249-L286
[response]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L583-L609
[discard]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L389-L395
