"""Bounded legal Command enumeration for RL.

Uses native Match.legal_commands, with a Python oracle fallback only for older
libraries missing that symbol. Ordered item sequences are capped at 1..4 items;
exchange_step=0 excludes exchange. sample_action is a proposal policy, not a
uniform draw over all legal Commands. Dedicated phases raise MatchError.
"""

from __future__ import annotations

from itertools import permutations

from official_match import Command, Match, InvalidMatchError, UnsupportedMatchError, _integer

_KINDS = ("play", "defend", "discard", "pray", "exchange", "sell", "buy", "accept", "decline", "sacrifice")
_PURCHASE_KINDS = ("accept", "decline")


def _command_order(command):
    return (_KINDS.index(command.kind), command.items,
            -1 if command.target is None else command.target,
            command.allocation or (-1, -1, -1))


def _alive_players(observation):
    return [p["id"] for p in observation["board"]["players"] if not p["dead"]]


def _hand_tokens(observation):
    # Used miracles stay in the hand and can be reused, so Match.legal decides.
    return [item["token"] for item in observation["board"]["hand"]]


def pending_kind(match: Match) -> str | None:
    """Return the phase name when it needs a non-Command entry point, else None."""
    status = match.status
    if status.done:
        return "finished"
    if status.phase == "main":
        observation = match.observe(status.actor)
        actor = next(p for p in observation["board"]["players"] if p["id"] == status.actor)
        if actor["status"]["confused"]:
            return "confusion"
    if status.phase in ("main", "response", "purchase"):
        return None
    return status.phase


def _candidates(match, observation, max_items, exchange_step):
    status = match.status
    actor = status.actor
    tokens = _hand_tokens(observation)
    targets = [None] + _alive_players(observation)
    phase = status.phase

    if phase == "purchase":
        for kind in _PURCHASE_KINDS:
            yield Command(actor, kind)
        return

    if phase == "response":
        yield Command(actor, "defend")
        for n in range(1, max_items + 1):
            for seq in permutations(tokens, n):
                yield Command(actor, "defend", seq)
        return

    # main phase
    yield Command(actor, "pray")
    for n in range(1, max_items + 1):
        for seq in permutations(tokens, n):
            yield Command(actor, "discard", seq)
            yield Command(actor, "sacrifice", seq)
    for n in range(1, max_items + 1):
        for seq in permutations(tokens, n):
            for target in targets:
                yield Command(actor, "play", seq, target)
    for seller_pair in permutations(tokens, 2) if max_items >= 2 else ():
        for target in targets[1:]:
            yield Command(actor, "sell", seller_pair, target)
    for token in tokens:
        for target in targets[1:]:
            yield Command(actor, "buy", (token,), target)
    if exchange_step:
        me = next(p for p in observation["board"]["players"] if p["id"] == actor)["status"]
        total = me["hp"] + me["mp"] + me["cp"]
        for token in tokens:
            for hp in range(0, total + 1, exchange_step):
                for mp in range(0, total - hp + 1, exchange_step):
                    yield Command(actor, "exchange", (token,), None, (hp, mp, total - hp - mp))


def _enumerate_commands_python(match, max_items=2, exchange_step=0):
    """Independent exhaustive candidate oracle, also used for old libraries."""
    _integer(max_items, "max_items", 1, 4)
    _integer(exchange_step, "exchange_step", 0, 2**31 - 1)
    pending = pending_kind(match)
    if pending == "finished":
        raise InvalidMatchError("cannot enumerate a finished match")
    if pending is not None:
        raise UnsupportedMatchError("dedicated entry required: " + pending)
    observation = match.observe(match.status.actor)
    return sorted((c for c in _candidates(match, observation, max_items, exchange_step) if match.legal(c)), key=_command_order)


def enumerate_commands(match: Match, max_items: int = 2, exchange_step: int = 0):
    """Return native legal Commands; fall back only if the library lacks the API."""
    if hasattr(match._lib, "gfo_match_legal_commands"):
        return match.legal_commands(max_items, exchange_step)
    return _enumerate_commands_python(match, max_items, exchange_step)
