# Legacy revival v1

Source244, technical label `legacy-244`, is the only positive-weight REVIVE
artifact: SUNDRY, passive amount10, price10, weight1 and discardable. Its ordinary
ATK/boost/DEF/HP/MP are0; Card.hp is deliberately not10 because this is not an active
heal. All twenty-five older catalogs, hashes, Kind values and indices remain fixed.

[The death scan][death] visits fixed room.players order for HP0/deadfalse. If at
least2 seats are not.dead and the owner has244, [playerDyingAttack][dying] directly
discards the first244, adds10HP and continues without marking the owner dead.
There is no replacement gift, MP cost, RNG, new decision or retained ability.
Disease, harms and worseChance persist. Already finalized dead seats cannot return.

The alive count uses not.dead, not positive HP. With both HP0, an earlier seat that
dies can prevent a later seat's244 from activating. A successful earlier revival
preserves the later seat's opportunity. This order must not use shuffled turn order.

[beforeEndInning][finish] scans deaths before the next queued attack and owner
disease tick. Tick death recursively scans again but does not repeat that tick.
Provisional HP0 inside an unresolved reflection chain waits for the item boundary.
Multiple244 cards can activate at separate boundaries, one card per owner per scan.

Standalone USE is invalid because its SUNDRY attack kind is empty. Manual discard
is accepted by the source; DISCARD excludes MORTAR245, not REVIVE244. DYING_ATTACK117
is excluded: a successful revival skips its branch, and a finalized duel death
leaves fewer than2 alive seats. Adding244 does not make117's death attack reachable.

`source-rows.csv` retains every original column; `catalog.csv` normalizes Card
fields and passive amount. The generator verifies fingerprints, generated C++,
header hash, old25hashes and ID disjointness. IDs/labels/weights are not verified
current official identifiers, names or acquisition probabilities. Master was
observed2026-10-31; immutable raw retrieval was unavailable, so the history-head
commit is context, not proof of snapshot identity.

```sh
python3 data/legacy-revival-v1/generate.py
python3 data/legacy-revival-v1/generate.py --check
```

[death]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L369-L395
[dying]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L35-L48
[finish]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/room.py#L426-L449
