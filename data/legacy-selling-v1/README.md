# Legacy selling v1

Only SELL3 is added: TRADE, price5, weight20, discardable. All ordinary Card
ATK/boost/DEF/HP/MP values are0, with no MP cost, rate, disease or harm. Dedicated
membership preserves SELL. BUY4 is excluded, so the combined pool is216 artifacts
with total weight449. All thirty previous catalogs, types, hashes and indices stay
fixed.

The separate `sale-prices.csv` covers exactly the older215 IDs plus3, sorted by ID.
`price-source-rows.csv` retains every original column for all216 source rows.
Every price is checked, including22 MAGIC types whose old runtime structs lack a
price field. `sale_price()` returns-1 for unknown/unsupported IDs; zero is a valid
price for MORTAR245. The maximum is30 at56/186; MAGIC220 costs25, and SELL3 costs5.

[Queue processing][queue] consumes3 and directly discards the chosen physical
merchandise. Selling a physical MAGIC neither uses nor learns it. [Resolution][sale]
charges current target YEN→MP→HP, attempts forced physical delivery, then credits
current source capped99. The original merchandise is gone even if delivery fails.
[Forced deal][deal] bypasses ordinary illusion handling but still refuses hand
capacity16; failure does not roll back charges, credit or original item removal.
The receiver gets a physical card, not a retained ability, with no compensation gift.

Ordinary attribute metadata is blank, while the active SELL payload is None.
The generator checks source/catalog/price hashes, exact216 source prices, generated
C++, header fingerprints, all old30 catalog hashes and ID disjointness. Historical
IDs/labels/prices/weights are not verified current official identifiers, names or
probabilities. Master was observed2026-11-01; immutable retrieval was unavailable,
so history head does not establish content identity.

```sh
python3 data/legacy-selling-v1/generate.py
python3 data/legacy-selling-v1/generate.py --check
```

[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L148-L156
[sale]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L404-L411
[deal]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L205-L227
