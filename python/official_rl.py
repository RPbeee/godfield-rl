"""Legal-command enumeration for RL on top of ``official_match.Match``.

``Match`` validates one command at a time (``legal``) and can draw one uniformly
random legal command (``sample_action``), but has no "list all legal commands" call.
This helper generates candidate commands from the observation and keeps those that
``Match.legal`` accepts, so a policy can be masked over a finite action list.

Scope and limits (experimental ruleset, not official-server equivalence):
  * Item sequences are ordered (composition order matters) and capped at
    ``max_items`` (default 2; each extra item multiplies the candidates by ~8). Commands that need longer sequences are not enumerated.
  * ``exchange`` is enumerated only when ``exchange_step`` is given, with allocations
    stepping by that amount over the (hp, mp, cp) totals; it is skipped by default.
  * Special entry points (Fate 207, confusion, grants, guardians, phenomena, devils,
    redraw/summon responses, ...) have their own ``*_legal`` / ``*_step`` methods and are
    not enumerated here. ``enumerate_commands`` returns [] when the match is waiting
    on one of them (see ``pending_kind``).
"""

from __future__ import annotations

from itertools import permutations

from official_match import Command, Match

# Kinds validated through Match.legal / Match.step.
_MAIN_KINDS = ("play", "discard", "pray", "sell", "buy", "exchange")
_RESPONSE_KINDS = ("defend",)
_PURCHASE_KINDS = ("accept", "decline")


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
    for token in tokens:
        yield Command(actor, "discard", (token,))
    for n in range(1, max_items + 1):
        for seq in permutations(tokens, n):
            for target in targets:
                yield Command(actor, "play", seq, target)
    for seller_pair in permutations(tokens, 2):
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


def enumerate_commands(match: Match, max_items: int = 2, exchange_step: int = 0):
    """Return the list of ``Command`` objects that ``match.legal`` accepts now."""
    if pending_kind(match) is not None:
        return []
    observation = match.observe(match.status.actor)
    return [c for c in _candidates(match, observation, max_items, exchange_step) if match.legal(c)]
