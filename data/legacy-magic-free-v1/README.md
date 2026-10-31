# Legacy MAGIC_FREE v1

This independent catalog contains all five positive-weight MAGIC_FREE artifacts.
All twenty-two earlier catalog hashes and indices remain fixed. The dedicated
membership preserves the fee-waiver effect alongside every ordinary role.

| Old ID / local slot | Source type | Ordinary role | Old price / weight |
| --- | --- | --- | --- |
| 47 / 0 | WEAPON | NE ATK12 | 20 / 1 |
| 153 / 1 | PROTECTOR | NE DFS6 | 20 / 1 |
| 172 / 2 | PROTECTOR | NE DFS9 | 20 / 1 |
| 180 / 3 | PROTECTOR | NE DFS12 | 20 / 1 |
| 239 / 4 | SUNDRY | MAGIC_FREE only, no standalone use | 5 / 2 |

All static MP costs, boosts, hit rates and other effects are zero. Armor and SUNDRY
are discardable; WEAPON47 is not. Appending Kind::sundry=4 preserves old enum values
and prevents labeling239 as HP or MP recovery. None is a retained miracle itself.

[Attack prescan][attack] and [defense prescan][defense] choose only the immediate
predecessor of the first non-base MAGIC_FREE. They do not find the last MAGIC or
waive all miracles. If a MAGIC was already used, a reached FREE card is consumed,
gives one pending gift, then skips its own ATK/DFS/attribute handling. Base47 alone
remains ATK12; a FREE at base index0 does not waive a later miracle.

On defense, a preceding210/215 can execute and break before reaching the FREE
suffix. That suffix is then not consumed, although the miracle's fee was waived.
This holds for47 as well as239 and the armor variants. Retained defensive chains
are therefore not bounded by available MP; decision limits, pending gifts,
restoration and unused suffix provenance must be modeled explicitly. Charging or
consuming the suffix early would change source behavior.

`source-rows.csv` retains all columns, while `catalog.csv` stores normalized facts.
The generator verifies both hashes, generated C++, header fingerprint, old22hashes
and ID disjointness. Historical IDs, labels and weights are not verified official
identifiers, names or acquisition probabilities. Master was observed2026-10-31;
immutable retrieval was unavailable, so the history-head commit does not prove
snapshot identity. Exact official UI acceptability of these free defensive chains
is not established.

```sh
python3 data/legacy-magic-free-v1/generate.py
python3 data/legacy-magic-free-v1/generate.py --check
```

[attack]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L96-L139
[defense]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L521-L581
