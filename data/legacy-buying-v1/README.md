# Legacy buying v1

Only BUY4 is added: TRADE, price5, weight20, discardable. All ordinary Card
ATK/boost/DEF/HP/MP values are0, without MP cost, rate, disease or harm. Dedicated
membership preserves BUY. The combined pool is217 artifacts with weight469.
All thirty-one previous catalogs, types, hashes, retained IDs and indices stay fixed.

The old216-row selling price table stays unchanged and still returns-1 for4.
When buying is enabled the private core price helper uses this catalog price5 for4,
then delegates every other ID to that frozen table. BUY4 can therefore be sold as
physical merchandise without changing the old price fingerprint or max price30.

[Initial processing][queue] consumes4 and earns one gift. [Resolution][select]
selects one current target physical hand copy uniformly. Empty consumes no ticket/RNG;
nonempty, including one copy, consumes one. Self selection is disclosed without a
purchase. Insufficient current buyer YEN ends without transfer. Otherwise a purchase
phase gives the current buyer an accept/decline decision.

[Acceptance][purchase] removes the chosen physical item, credits current target
capped99, attempts forced delivery to current source, then charges source YEN.
MP/HP cannot fund buying. The source [generic forced-delivery helper][deal] may fail
at capacity16 without rollback, but that state is unreachable in this closed BUY
subset: the buyer has consumed4 or reflection196 and has at most15 hand cards.
Legal purchases always deliver; impossible full-hand offers or purchased histories
with delivered=false are rejected. Physical MAGICs do not become retained abilities.
Weapons and MORTAR245 remain valid merchandise despite ordinary discard limits.
The selected item/price is public; pre-selection population, hand slot and ticket
remain private. The active BUY payload is None, distinct from blank ordinary metadata.

The generator checks source/catalog hashes, generated C++, header fingerprint,
all old31 catalog hashes and ID disjointness, and the frozen216 price table hash.
Historical IDs/labels/prices/weights are not verified current official identifiers,
names or probabilities. Master was observed2026-11-01; immutable retrieval was
unavailable, so history head does not establish content identity.

```sh
python3 data/legacy-buying-v1/generate.py
python3 data/legacy-buying-v1/generate.py --check
```

[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L154-L156
[select]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L418-L422
[purchase]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L50-L70
[deal]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L205-L227
