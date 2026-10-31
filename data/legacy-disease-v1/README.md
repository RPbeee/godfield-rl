# Legacy disease/cure catalog v1

This separately versioned pool retains eight historical records, with total
legacy weight 24. It does not alter any previous catalog or fingerprint.

| Historical ID | Base effect | Disease/cure field | Weight |
| --- | --- | --- | --- |
| 30 | Neutral ATK8 | Hell attack extra | 1 |
| 34 | Neutral ATK9 | Cold attack extra | 1 |
| 49 | Neutral ATK13 | Cold attack extra | 1 |
| 61 | Stone ATK1 | Cold attack extra | 1 |
| 175 | Fire DEF10 | Fever defense extra | 1 |
| 234 | MP+20 | Heaven attack extra | 1 |
| 235 | No HP/MP delta | Lower-harms removal | 12 |
| 236 | No HP/MP delta | All-harms removal | 6 |

**The Heaven MP item is ID234. ID233 is the already cataloged plain MP+15 item.**
MP gain and Heaven are preserved together; no secondary effect is stripped.

IDs are zero-based rows from the historical emulator's
[ItemData.CSV](https://github.com/Igoorx/godfield-flash/blob/master/server-src/ItemData.CSV).
`legacy-NNN` names are technical labels, not verified current official names or
IDs. `source-rows.csv` retains original fields; `catalog.csv` normalizes them.
Both have independent SHA-256 fingerprints in `manifest.json`.

`Disease` preserves the rank order none/cold/fever/hell/heaven as 0..4.
`on_defense` records a defense-extra location; the engine still needs to apply
the right target and timing. Natural worsening probabilities, disease ticks,
stacking and death resolution are not inferred by this table.

The two cure records use `Card::kind=heal` only to route them as consumable
utilities; `hp=0` and the separate `Cure::lower/all` fields carry their meaning.
They do not secretly grant HP. The historical operations remove other harms as
well as diseases. Their full lower/all category is retained, so a profile that
does not implement Fog/Flash/etc. must record that restriction and must not
claim complete behavior for those states. Once those harms are supported,
the cure engine must also clear the corresponding harms.

Miracles, chance attacks, Light/Dark, absorption, other harms and special
reactions are excluded. Unsupported IDs return nullptr. Existing-card aliases
and plain HP/MP records are not duplicated.

The branch-history revision is recorded, but immutable raw content was
unavailable; observed master rows are not asserted byte-identical to it.
Historical weights are not verified official acquisition probabilities; the
upstream [README](https://github.com/Igoorx/godfield-flash#known-issues) reports
distribution uncertainty. Current-client compatibility is unverified.

Regenerate and verify from the repository root:

```sh
python3 data/legacy-disease-v1/generate.py
python3 data/legacy-disease-v1/generate.py --check
```
