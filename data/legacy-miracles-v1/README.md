# Legacy miracles v1

This separately versioned pool adds two simple, neutral historical miracles.
It does not modify the 60-card `legacy-neutral-v1` catalog or its fingerprint.

| Stable kind slot | Historical CSV ID | Technical label | Effect | MP cost | Historical weight |
| --- | --- | --- | --- | --- | --- |
| 0 | 216 | legacy-216 | HP +10 | 6 | 1 |
| 1 | 220 | legacy-220 | Money +15 | 8 | 1 |

Facts come from the corresponding zero-based rows in the historical emulator's
[ItemData.CSV](https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV).
Names are technical labels, not verified official names or current IDs.
`miracle_index()` preserves the slots above and returns -1 for unsupported IDs;
`find_miracle()` returns nullptr for those IDs.

**Legacy ID216 costs 6MP. The contemporary extracted Spring entry costs 7MP.**
The difference is visible in the
[2026-09-20 extraction](https://github.com/BrandenXia/godfield-bot/blob/main/data/snapshots/2026-09-20/bible.json).
This catalog intentionally keeps the old value; it must not be presented as the
current Spring implementation. Effect/ID/name correspondence remains unverified.

Weights come from the old emulator, whose
[README](https://github.com/Igoorx/godfield-flash#known-issues) reports uncertainty
about artifact distribution. A restricted profile using these weights is not
the official distribution.

`manifest.json` records the source, normalized CSV SHA-256, stable slots and
retrieval limits. The history-head commit was observed, but its immutable raw
file was unavailable; the retrieved master rows are not asserted byte-identical
to that revision. Current official-client compatibility is unverified.

Regenerate or check the native table from the repository root:

```sh
python3 data/legacy-miracles-v1/generate.py
python3 data/legacy-miracles-v1/generate.py --check
```
