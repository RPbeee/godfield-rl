"""Read-only current official catalog and public client composition rules.

This is partial source conformance, NOT a playable environment. IDs belong to
the official catalog, independently of every legacy profile. compose() computes
display functions; it does not validate an action or simulate server outcomes.
Usage: python3 python/official_data.py [path/to/libgodfield.dylib]
"""
from __future__ import annotations

import ctypes as c
import os
from dataclasses import dataclass
from pathlib import Path
import sys


class _Card(c.Structure):
    _fields_ = [("id", c.c_uint16), ("category", c.c_uint8), ("element", c.c_int8),
                ("ability", c.c_uint8), ("curse", c.c_uint8), ("guardian", c.c_uint8),
                ("is_plus_atk", c.c_uint8)] + [
                    (name, c.c_int32) for name in ("atk", "hit_rate", "defense", "ability_value",
                        "price", "cost", "gift_rate", "appearance_rate", "guardian_attack_rate")]


class _Composition(c.Structure):
    _fields_ = [("attack_power", c.c_int64), ("magic_cost", c.c_int64),
                ("attack_element", c.c_int8), ("defense_element", c.c_int8),
                ("reserved", c.c_uint8 * 6)]


@dataclass(frozen=True)
class Card:
    id: int
    name: str
    image_name: str
    category: str
    element: str | None
    ability: str
    curse: str
    guardian: str
    is_plus_atk: bool
    atk: int
    hit_rate: int
    defense: int
    ability_value: int
    price: int
    cost: int
    gift_rate: int
    appearance_rate: int
    guardian_attack_rate: int


@dataclass(frozen=True)
class Composition:
    attack_power: int
    magic_cost: int
    attack_element: str | None
    defense_element: str | None


class OfficialData:
    """Immutable catalog entries and ordered composition, without game handles."""

    def __init__(self, library: str | Path | None = None):
        ext = "dylib" if sys.platform == "darwin" else "so"
        # Search order: explicit argument, GODFIELD_LIBRARY, then <repo>/build.
        override = os.environ.get("GODFIELD_LIBRARY")
        if library is not None:
            path = Path(library)
        elif override:
            path = Path(override)
        else:
            path = Path(__file__).resolve().parents[1] / "build" / f"libgodfield.{ext}"
        lib = self._lib = c.CDLL(str(path))
        for name in ("abi_version", "catalog_count", "gift_rate"):
            fn = getattr(lib, "gfo_" + name)
            fn.argtypes, fn.restype = [], c.c_int
        for name in ("ruleset", "scope", "catalog_sha256", "client_sha256"):
            fn = getattr(lib, "gfo_" + name)
            fn.argtypes, fn.restype = [], c.c_char_p
        for name in ("card_size", "composition_size"):
            fn = getattr(lib, "gfo_" + name)
            fn.argtypes, fn.restype = [], c.c_size_t
        if (lib.gfo_abi_version() != 1 or lib.gfo_card_size() != c.sizeof(_Card)
                or lib.gfo_composition_size() != c.sizeof(_Composition)
                or c.sizeof(_Card) != 44 or c.sizeof(_Composition) != 24
                or lib.gfo_catalog_count() != 296 or lib.gfo_gift_rate() != 500):
            raise RuntimeError("unsupported official catalog ABI or dimensions")
        for name, argtype in (("card_name", c.c_uint16), ("card_image_name", c.c_uint16),
                              ("category_name", c.c_uint8), ("element_name", c.c_int8),
                              ("ability_name", c.c_uint8), ("curse_name", c.c_uint8),
                              ("guardian_name", c.c_uint8)):
            fn = getattr(lib, "gfo_" + name)
            fn.argtypes, fn.restype = [argtype], c.c_char_p
        lib.gfo_get_card.argtypes = [c.c_uint16, c.POINTER(_Card)]
        lib.gfo_get_card.restype = c.c_int
        lib.gfo_compose.argtypes = [c.POINTER(c.c_uint16), c.c_size_t, c.c_int64, c.POINTER(_Composition)]
        lib.gfo_compose.restype = c.c_int

    @property
    def ruleset(self) -> str:
        return self._lib.gfo_ruleset().decode("utf-8")

    @property
    def scope(self) -> str:
        return self._lib.gfo_scope().decode("utf-8")

    @property
    def catalog_sha256(self) -> str:
        return self._lib.gfo_catalog_sha256().decode("ascii")

    @property
    def client_sha256(self) -> str:
        return self._lib.gfo_client_sha256().decode("ascii")

    @property
    def gift_rate(self) -> int:
        return self._lib.gfo_gift_rate()

    def __len__(self) -> int:
        return self._lib.gfo_catalog_count()

    def __iter__(self):
        return (self.get_card(i) for i in range(1, len(self) + 1))

    def _name(self, kind: str, value: int) -> str:
        result = getattr(self._lib, f"gfo_{kind}_name")(value)
        if result is None:
            raise RuntimeError(f"invalid native {kind} value {value}")
        return result.decode("utf-8")

    def _id(self, value: int) -> int:
        if type(value) is not int or not 1 <= value <= len(self):
            raise ValueError("official model ID must be an integer in [1,296]")
        return value

    def get_card(self, card_id: int) -> Card:
        self._id(card_id)
        out = _Card()
        status = self._lib.gfo_get_card(card_id, c.byref(out))
        if status:
            raise ValueError(f"native official catalog error {status}")
        return Card(out.id, self._name("card", out.id), self._lib.gfo_card_image_name(out.id).decode("utf-8"),
                    self._name("category", out.category), self._name("element", out.element) or None,
                    self._name("ability", out.ability), self._name("curse", out.curse),
                    self._name("guardian", out.guardian), bool(out.is_plus_atk), out.atk,
                    out.hit_rate, out.defense, out.ability_value, out.price, out.cost,
                    out.gift_rate, out.appearance_rate, out.guardian_attack_rate)

    def compose(self, ids, base_override: int = 0) -> Composition:
        """Compute ordered display composition, without action-legality claims.

        A positive base_override supplies the displayed base ATK (e.g. the
        caller's MP-derived value);0 uses catalog ATK. No hidden state is read.
        """
        if type(base_override) is not int or not 0 <= base_override <= 0x7FFFFFFFFFFFFFFF:
            raise ValueError("base_override must be a nonnegative int64")
        values = tuple(ids)
        if not values:
            raise ValueError("composition must not be empty")
        for value in values:
            self._id(value)
        native = (c.c_uint16 * len(values))(*values)
        out = _Composition()
        status = self._lib.gfo_compose(native, len(values), base_override, c.byref(out))
        if status == -3:
            raise OverflowError("native official composition overflow or allocation failure")
        if status:
            raise ValueError(f"native official composition error {status}")
        return Composition(out.attack_power, out.magic_cost,
                           self._name("element", out.attack_element) or None,
                           self._name("element", out.defense_element) or None)


if __name__ == "__main__":
    data = OfficialData(sys.argv[1] if len(sys.argv) > 1 else None)
    print(f"ruleset={data.ruleset} scope={data.scope}")
    print(f"cards={len(data)} gift_rate={data.gift_rate} catalog_sha256={data.catalog_sha256}")
    print(data.get_card(66))
    print(data.compose([13, 66]))
