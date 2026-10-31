# Legacy exchange v1

Source2, technical label `legacy-002`, is TRADE/EXCHANGE: price10, weight20,
ATK/boost/DEF/HP/MP/cost/rate0 and no other extra. Kind::trade=5 is appended, keeping
all old values and twenty-four older catalogs/indices unchanged. SELL3 and BUY4 are
not included.

[Player.usePiece][use] consumes every non-FIXED ordinary artifact, including TRADE.
[discardPiece][discard] and [DISCARD processing][turn] do not exclude TRADE, so this
card is discardable. Use consumes2, earns one pending gift and does not retain a
miracle. Price10 is market metadata, not an exchange fee.

[queueAttack][queue] requires a one-item command and HP/MP/YEN allocations with the
same total as the current three resources, then marks it an action and forces self.
[Resolution][turn] directly assigns the three requested values with no response.
The old session only parses integers, and the turn checks only conservation. This
runtime intentionally accepts canonical values0..99, permits HP0 and rejects other
ranges; official client/UI bounds are unverified. Keep allocation deltas separate
from the ordinary owner disease tick and terminal death handling.

`source-rows.csv` retains all original columns; `catalog.csv` stores normalized
Card fields. The generator verifies both hashes, generated C++, header fingerprint,
old24hashes and ID disjointness. IDs/labels/weights are not verified current official
identifiers, names or acquisition probabilities. Master was observed2026-10-31;
immutable raw retrieval was unavailable, so history-head does not prove content identity.

```sh
python3 data/legacy-exchange-v1/generate.py
python3 data/legacy-exchange-v1/generate.py --check
```

[use]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L276-L291
[discard]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/player.py#L247-L254
[queue]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L126-L147
[turn]: https://github.com/Igoorx/godfield-flash/blob/master/server-src/modules/turn.py#L389-L403
