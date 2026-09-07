#!/usr/bin/env python3
"""L2 — batch analysis over per-game decision-log JSON files (L1 schema 1).

Usage:
    python3 scripts/analyze_batch.py <log_dir> [--top N]

Writes ``summary.json`` and ``summary.md`` into ``<log_dir>``.

Run the tests for this script with:
    python3 -m unittest discover -s tests/analysis -v

--------------------------------------------------------------------------
ON-DISK SCHEMA — cross-checked 2026-09-07 against the real
``src/io/decision_log_writer.cpp`` (read only; the C++ was NOT built or
run for this work — L2 and L1 were built in parallel per the plan). If
that writer's shape drifts again before L3 runs this script for real,
re-diff against it; every lookup below still degrades to null/0/"unknown"
rather than raising on an unexpected shape.

Per game file (one JSON object), verified field names:
    {
      "schema": 1,
      "decks": {"p1": {"path", "legend", "champion"}, "p2": {...}},
      "agents": {"p1": "<agent spec>", "p2": "<agent spec>"},
      "seed": 2000,
      "seats": {"<deck file basename, WITH extension>": "P1" | "P2", ...},
      "engine_version": "...",
      "log": [                                  // NOTE: the decision list
                                                  // is keyed "log", not
                                                  // "decisions" (that name
                                                  // is reused below for the
                                                  // footer's decision COUNT).
        {
          "idx": 0, "turn": 1,
          "phase": {"phase": "Main", "ns_state": "Neutral"|"Showdown",
                    "oc_state": "Open"|"Closed"},
          "actor": "P1" | "P2",
          "scores": [p1_score, p2_score],
          "players": [ {"seat": "P1", "hand", "deck", "trash", "banishment",
                         "runes_ready", "runes_exhausted", "legend_empowered"},
                        {"seat": "P2", ...} ],     // positional P1 then P2
          "battlefields": [ {"id", "name", "controller", "contested",
                              "units": {"p1": {"count","might"},
                                        "p2": {"count","might"}}}, ...],
          "legal_count": 8,
          "chosen": {"type": "PlayCard", "card": "..."|null,
                     "source_zone": "Hand"|"Trash"|"Banishment"|"ChampionZone"
                                    |"Hidden"|"ChainZone",
                     "flow_source": "None" | "Printed" | "Granted",
                     "restricted_bf": null | <battlefield id>,
                     "destination": null
                                   | {"type": "battlefield", "id": <id>}
                                   | {"type": "base", "player": "P1"|"P2"}},
          "root_value": null | <float>
        }, ...
      ],
      // On a clean finish, these five keys are spliced onto the TOP-LEVEL
      // object (there is no nested "footer" key):
      "winner": {"deck": "<legend> / <champion>", "seat": "P1"|"P2"},
      "reason": "...", "turns": 6, "final_scores": [p1, p2],
      "decisions": 20,                            // the COUNT, an int
      // On a crash, none of the above five run; instead only:
      "truncated": true                           // best-effort partial file
    }

For robustness this script ALSO accepts the spec's more generic English
description as a fallback shape (a "decisions" list, a nested "footer"
object, "turn_phase"/"ns"/"oc", uppercase "P1"/"P2" battlefield-unit keys,
a bare battlefield id or "base" string as destination) in case the real
writer's shape drifts further — see get_decisions_list/get_footer/
phase_name/is_closed/classify_family/get_ci below. Any missing optional
key is treated as null/0/empty rather than raising. A game is skipped
(counted, not analysed) if it fails to parse as JSON, if a
"truncated": true marker is found (top level or in a nested "footer"), or
if no winner/footer information is present at all (an incomplete file we
cannot trust the ending state of).

FAMILY MAPPING (chosen.type + destination -> one of the 9 families the spec
names): see INTENT_FAMILY below. StandardMove is split into
move_to_battlefield / move_to_base by whether chosen.destination names a
battlefield. Any type string this script does not recognise is bucketed as
"unknown" and reported, rather than silently dropped or raising.

MISTAKE / SWING HEURISTIC (crude by design, per the spec):
For the LOSING deck: for every decision made by the losing seat, take the
margin (loser's score - winner's score) at that decision. Find that seat's
NEXT own turn (turn ownership is itself a crude heuristic: the actor of the
first non-Closed-state decision seen in a turn "owns" that turn, falling
back to the first decision's actor if every decision in the turn is
Closed). Compare the margin at the decision to the margin at the LAST
decision of that next own turn. A candidate qualifies if the margin fell by
>= 2. Top N (--top, default 20) by size of the drop. "Swing decisions" are
the symmetric case for the winning seat (margin ROSE by >= 2 by the end of
its next own turn).

Burns / empowers: the L1 field contract given to this script carries no
explicit burn/empower marker (only players[].legend_empowered, a status,
not an action count). This script looks for an optional "burn" or
"empower" flag on a decision or its "chosen" object; if no game in the
input carries either, the section is OMITTED from the output with a note,
per the spec ("if the log carries them (else omit with a note)").
"""

from __future__ import annotations

import argparse
import json
import math
from collections import Counter, defaultdict
from pathlib import Path

# 95% two-sided normal quantile.
Z95 = 1.959963984540054

SEATS = ("P1", "P2")


# --------------------------------------------------------------------------
# Wilson score interval
# --------------------------------------------------------------------------

def wilson_interval(successes: int, n: int, z: float = Z95):
    """Return (p, lo, hi) as proportions in [0, 1]. (0, 0, 0) if n == 0."""
    if n <= 0:
        return (0.0, 0.0, 0.0)
    p = successes / n
    denom = 1.0 + (z * z) / n
    center = (p + (z * z) / (2 * n)) / denom
    margin = (z * math.sqrt((p * (1 - p) / n) + (z * z) / (4 * n * n))) / denom
    lo = max(0.0, center - margin)
    hi = min(1.0, center + margin)
    return (p, lo, hi)


# --------------------------------------------------------------------------
# Action-family classification
# --------------------------------------------------------------------------

INTENT_FAMILY = {
    "playcard": "play",
    "playreaction": "play",
    "playactioncard": "play",
    "hidecard": "play",
    "assigncombatdamage": "combat_damage",
    "activateability": "activate",
    "activatereactionability": "activate",
    "activateactionability": "activate",
    "mulligandecision": "setup",
    "choosebattlefield": "setup",
    "playfirstdecision": "setup",
    "sideboardswap": "setup",
    "makechoice": "choice",
    "placeoptionaltrigger": "choice",
    "declineoptionaltrigger": "choice",
    "paytriggeredcost": "choice",
    "declinetriggeredcost": "choice",
    "endturn": "pass",
    "passpriority": "pass",
    "passfocus": "pass",
    "concede": "concede",
}

FAMILIES = (
    "play", "combat_damage", "move_to_battlefield", "move_to_base",
    "activate", "choice", "setup", "pass", "concede",
)


def normalize_token(value) -> str:
    if value is None:
        return ""
    return "".join(ch for ch in str(value).lower() if ch.isalnum())


def classify_family(chosen: dict) -> str:
    chosen = chosen or {}
    t = normalize_token(chosen.get("type"))
    if t == "standardmove":
        dest = chosen.get("destination")
        if isinstance(dest, dict):
            dtype = normalize_token(dest.get("type"))
            if dtype == "battlefield":
                return "move_to_battlefield"
            if dtype == "base":
                return "move_to_base"
            # Unrecognised destination shape: fall through to the bare
            # value heuristics below rather than guessing wrong silently.
        if dest is None:
            return "move_to_base"
        if isinstance(dest, str) and dest.strip().lower() in ("", "base", "none"):
            return "move_to_base"
        return "move_to_battlefield"
    return INTENT_FAMILY.get(t, "unknown")


# --------------------------------------------------------------------------
# Schema helpers (tolerant of missing/alternate shapes)
# --------------------------------------------------------------------------

def deck_label(value) -> str:
    if isinstance(value, dict):
        return str(value.get("name") or value.get("legend") or value.get("path") or value)
    s = str(value)
    # The real writer's "seats" map is keyed by deck-file basename WITH
    # extension (e.g. "kennen_tyler.txt"); strip a trailing extension so
    # the label matches the deck's plain name for readability. Only do
    # this for bare filenames (no path separator) to avoid mangling a
    # legend/champion string that happens to contain a period.
    if "." in s and "/" not in s and "\\" not in s:
        stem = s.rsplit(".", 1)[0]
        if stem:
            return stem
    return s


def resolve_seat_decks(game: dict) -> dict:
    """Return {"P1": deck_label, "P2": deck_label}, best-effort."""
    seats_raw = game.get("seats")
    decks_raw = game.get("decks")
    result = {"P1": None, "P2": None}

    if isinstance(seats_raw, dict):
        keys = set(seats_raw.keys())
        if keys & set(SEATS):
            for seat in SEATS:
                if seat in seats_raw and result[seat] is None:
                    result[seat] = deck_label(seats_raw[seat])
        else:
            for name, seat in seats_raw.items():
                if seat in SEATS and result[seat] is None:
                    result[seat] = deck_label(name)

    if (result["P1"] is None or result["P2"] is None) and isinstance(decks_raw, dict):
        keys = set(decks_raw.keys())
        if keys & set(SEATS):
            for seat in SEATS:
                if seat in decks_raw and result[seat] is None:
                    result[seat] = deck_label(decks_raw[seat])

    if (result["P1"] is None or result["P2"] is None) and isinstance(decks_raw, list) and len(decks_raw) >= 2:
        if result["P1"] is None:
            result["P1"] = deck_label(decks_raw[0])
        if result["P2"] is None:
            result["P2"] = deck_label(decks_raw[1])

    for seat in SEATS:
        if result[seat] is None:
            result[seat] = seat
    return result


def get_scores(decision: dict):
    s = (decision or {}).get("scores")
    if isinstance(s, dict):
        return [get_ci(s, "P1") or 0, get_ci(s, "P2") or 0]
    if isinstance(s, (list, tuple)) and len(s) >= 2:
        return [s[0] or 0, s[1] or 0]
    return [0, 0]


def seat_index(seat: str) -> int:
    return 0 if seat == "P1" else 1


def other_seat(seat: str) -> str:
    return "P2" if seat == "P1" else "P1"


def get_ci(d, *keys):
    """Case-insensitive dict lookup; returns the first key that matches."""
    if not isinstance(d, dict):
        return None
    lower_map = {str(k).lower(): v for k, v in d.items()}
    for k in keys:
        if k.lower() in lower_map:
            return lower_map[k.lower()]
    return None


def is_closed(decision: dict) -> bool:
    phase = (decision or {}).get("phase") or {}
    # Real writer: "oc_state". Fallback (generic schema): "oc".
    val = phase.get("oc_state")
    if val is None:
        val = phase.get("oc")
    return normalize_token(val) == "closed"


def phase_name(decision: dict) -> str:
    phase = (decision or {}).get("phase") or {}
    # Real writer nests the turn-phase name under a key ALSO called
    # "phase" (e.g. {"phase": "Main", "oc_state": ...}). Fallback:
    # "turn_phase" per the generic schema description.
    return phase.get("phase") or phase.get("turn_phase") or "Unknown"


def board_summary(decision: dict) -> str:
    scores = get_scores(decision)
    phase = (decision or {}).get("phase") or {}
    bfs = (decision or {}).get("battlefields") or []
    parts = []
    for bf in bfs:
        name = bf.get("name") or bf.get("id") or "?"
        ctrl = bf.get("controller") or "-"
        mark = "*" if bf.get("contested") else ""
        units = bf.get("units") or {}
        u1 = get_ci(units, "P1") or {}
        u2 = get_ci(units, "P2") or {}
        parts.append(
            f"{name}[{ctrl}{mark}] P1:{u1.get('count', 0)}u/{u1.get('might', 0)}m "
            f"P2:{u2.get('count', 0)}u/{u2.get('might', 0)}m"
        )
    header = (
        f"turn {decision.get('turn')} {phase_name(decision)}/"
        f"{'Closed' if is_closed(decision) else 'Open'} "
        f"scores P1={scores[0]} P2={scores[1]}"
    )
    if parts:
        return header + " | " + "; ".join(parts)
    return header


def has_burn_or_empower(decision: dict) -> bool:
    if not isinstance(decision, dict):
        return False
    if decision.get("burn") or decision.get("empower"):
        return True
    chosen = decision.get("chosen") or {}
    tags = chosen.get("effect_tags")
    if isinstance(tags, (list, tuple)) and ("burn" in tags or "empower" in tags):
        return True
    return False


# --------------------------------------------------------------------------
# Loading / truncation detection
# --------------------------------------------------------------------------

def get_decisions_list(game: dict) -> list:
    """The decision records. Real writer keys this "log"; the generic
    schema description uses "decisions" (a list there, NOT the footer's
    decision count, which only ever appears as an int)."""
    log = game.get("log")
    if isinstance(log, list):
        return log
    d = game.get("decisions")
    if isinstance(d, list):
        return d
    return []


def get_footer(game: dict) -> dict:
    """Outcome fields, normalised to one dict regardless of nesting.

    Real writer: winner/reason/turns/final_scores/decisions are spliced
    onto the TOP-LEVEL object. Generic schema fallback: a nested "footer"
    dict. Returns {} if neither is present (crashed/incomplete game)."""
    nested = game.get("footer")
    if isinstance(nested, dict):
        return nested
    if "winner" in game:
        count = game.get("decisions")
        return {
            "winner": game.get("winner"),
            "reason": game.get("reason"),
            "turns": game.get("turns"),
            "final_scores": game.get("final_scores"),
            "decisions": count if not isinstance(count, list) else None,
        }
    return {}


def is_truncated_or_incomplete(game: dict) -> bool:
    if game.get("truncated"):
        return True
    footer = get_footer(game)
    if footer.get("truncated"):
        return True
    if not footer or not isinstance(footer.get("winner"), dict):
        return True
    return False


def load_games(log_dir: Path):
    """Return (games, skipped_unparseable, skipped_truncated).

    games is a list of (filename, game_dict) for usable games, in filename
    (== recording) order.
    """
    games = []
    skipped_unparseable = 0
    skipped_truncated = 0
    for path in sorted(log_dir.glob("*.json")):
        if path.name in ("summary.json",):
            continue
        try:
            with path.open() as f:
                game = json.load(f)
        except (OSError, json.JSONDecodeError):
            skipped_unparseable += 1
            continue
        if not isinstance(game, dict):
            skipped_unparseable += 1
            continue
        if is_truncated_or_incomplete(game):
            skipped_truncated += 1
            continue
        games.append((path.name, game))
    return games, skipped_unparseable, skipped_truncated


# --------------------------------------------------------------------------
# Per-game accumulation
# --------------------------------------------------------------------------

def turn_owners(decisions):
    owners = {}
    for d in decisions:
        t = d.get("turn")
        if t in owners:
            continue
        if not is_closed(d):
            owners[t] = d.get("actor")
    for d in decisions:
        t = d.get("turn")
        if t not in owners:
            owners[t] = d.get("actor")
    return owners


def last_decision_per_turn(decisions):
    last = {}
    for d in decisions:
        last[d.get("turn")] = d
    return last


def find_candidates(decisions, owners, last_per_turn, target_seat, direction):
    """direction: 'drop' (mistakes, want margin to fall) or 'rise' (swings)."""
    idx_t = seat_index(target_seat)
    idx_o = 1 - idx_t
    out = []
    for d in decisions:
        if d.get("actor") != target_seat:
            continue
        turn_d = d.get("turn")
        scores_d = get_scores(d)
        margin_d = scores_d[idx_t] - scores_d[idx_o]

        next_turn = None
        for t, owner in owners.items():
            if owner == target_seat and t is not None and turn_d is not None and t > turn_d:
                if next_turn is None or t < next_turn:
                    next_turn = t
        if next_turn is None:
            continue
        end_decision = last_per_turn.get(next_turn)
        if end_decision is None:
            continue
        scores_end = get_scores(end_decision)
        margin_end = scores_end[idx_t] - scores_end[idx_o]

        if direction == "drop":
            delta = margin_d - margin_end
        else:
            delta = margin_end - margin_d
        if delta >= 2:
            out.append({
                "delta": delta,
                "decision": d,
                "margin_at_decision": margin_d,
                "margin_at_turn_end": margin_end,
                "next_turn": next_turn,
            })
    return out


def count_conquers(decisions, deck_by_seat):
    conquers = Counter()
    prev = None
    for d in decisions:
        cur = get_scores(d)
        if prev is not None:
            actor = d.get("actor")
            i = seat_index(actor) if actor in SEATS else None
            if i is not None and cur[i] > prev[i]:
                conquers[deck_by_seat.get(actor, actor)] += 1
        prev = cur
    return conquers


def count_flags(decisions, deck_by_seat):
    flow_printed = Counter()
    flow_granted = Counter()
    tomb = Counter()
    reactions_closed = Counter()
    for d in decisions:
        chosen = d.get("chosen") or {}
        actor = d.get("actor")
        deck = deck_by_seat.get(actor, actor)
        fs = normalize_token(chosen.get("flow_source"))
        if fs == "printed":
            flow_printed[deck] += 1
        elif fs == "granted":
            flow_granted[deck] += 1
        if chosen.get("restricted_bf") is not None:
            tomb[deck] += 1
        if is_closed(d) and classify_family(chosen) == "play":
            reactions_closed[deck] += 1
    return flow_printed, flow_granted, tomb, reactions_closed


FROZEN_BURST_MIN_RUN = 10


def _state_fingerprint(decision: dict) -> str:
    """Everything the log records about a decision EXCEPT its index: the
    actor, what was chosen, the scores, both players' resource lines, the
    battlefields and the phase. Two consecutive decisions with the same
    fingerprint mean the engine executed nothing in between."""
    keys = ("actor", "chosen", "scores", "players", "battlefields", "phase",
            "legal_count")
    return json.dumps({k: decision.get(k) for k in keys}, sort_keys=True)


def find_frozen_bursts(decisions, min_run: int = FROZEN_BURST_MIN_RUN):
    """Batch VALIDITY check. Returns (bursts, max_run) where `bursts` lists
    every run of >= min_run consecutive decisions whose full fingerprint is
    identical — the signature of an offer the executor silently drops (the
    same intent is legal again, the agent picks it again, nothing moves).
    Iteration 0 found two such holes by hand (closed-state ability
    activations; unpayable equip offers) and they invalidated whole
    batches, so every summary now leads with this. `max_run` is the longest
    identical run in the game, even when below the threshold."""
    bursts = []
    max_run = 0
    run_start = 0
    prev_fp = None
    n = len(decisions)
    for i in range(n + 1):
        fp = _state_fingerprint(decisions[i]) if i < n else None
        if i < n and fp == prev_fp:
            continue
        length = i - run_start
        if prev_fp is not None:
            max_run = max(max_run, length)
            if length >= min_run:
                first = decisions[run_start]
                bursts.append({
                    "start_idx": first.get("idx", run_start),
                    "turn": first.get("turn"),
                    "actor": first.get("actor"),
                    "type": (first.get("chosen") or {}).get("type"),
                    "length": length,
                })
        run_start = i
        prev_fp = fp
    return bursts, max_run


def accumulate_family_mix(decisions, deck_by_seat, mix):
    for d in decisions:
        chosen = d.get("chosen") or {}
        actor = d.get("actor")
        deck = deck_by_seat.get(actor, actor)
        family = classify_family(chosen)
        mix[deck][phase_name(d)][family] += 1


def accumulate_tempo(decisions, deck_by_seat, last_per_turn, tempo, margin_curve):
    for t, d in last_per_turn.items():
        if t is None:
            continue
        scores = get_scores(d)
        for seat in SEATS:
            i = seat_index(seat)
            o = 1 - i
            deck = deck_by_seat.get(seat, seat)
            key = (deck, seat)
            tempo[key][t].append(scores[i])
            margin_curve[key][t].append(scores[i] - scores[o])


# --------------------------------------------------------------------------
# Top-level orchestration
# --------------------------------------------------------------------------

def analyze_directory(log_dir: Path, top_n: int = 20) -> dict:
    games, skipped_unparseable, skipped_truncated = load_games(log_dir)

    win_stats = defaultdict(lambda: defaultdict(lambda: {"wins": 0, "games": 0}))
    turns_all = []
    tempo = defaultdict(lambda: defaultdict(list))
    margin_curve = defaultdict(lambda: defaultdict(list))
    family_mix = defaultdict(lambda: defaultdict(Counter))
    flow_printed_total = Counter()
    flow_granted_total = Counter()
    tomb_total = Counter()
    reactions_closed_total = Counter()
    conquers_total = Counter()
    per_game = []
    mistakes = []
    swings = []
    burn_empower_seen = False
    frozen_bursts = []
    max_run_overall = 0
    games_with_bursts = 0

    for filename, game in games:
        decisions = get_decisions_list(game)
        footer = get_footer(game)
        deck_by_seat = resolve_seat_decks(game)

        turns = footer.get("turns")
        if turns is not None:
            turns_all.append(turns)

        winner = footer.get("winner") or {}
        winner_seat = winner.get("seat")
        if winner_seat not in SEATS:
            # Can't determine a winner/loser split for this game; still
            # fold its non-outcome-dependent stats in, but skip win-rate
            # and mistake/swing bookkeeping for it.
            winner_seat = None
        else:
            loser_seat = other_seat(winner_seat)
            winner_deck = deck_by_seat[winner_seat]
            loser_deck = deck_by_seat[loser_seat]
            win_stats[winner_deck][winner_seat]["wins"] += 1
            win_stats[winner_deck][winner_seat]["games"] += 1
            win_stats[loser_deck][loser_seat]["games"] += 1

        owners = turn_owners(decisions)
        last_per_turn = last_decision_per_turn(decisions)

        accumulate_family_mix(decisions, deck_by_seat, family_mix)
        accumulate_tempo(decisions, deck_by_seat, last_per_turn, tempo, margin_curve)

        fp, fg, tb, rc = count_flags(decisions, deck_by_seat)
        flow_printed_total.update(fp)
        flow_granted_total.update(fg)
        tomb_total.update(tb)
        reactions_closed_total.update(rc)

        cq = count_conquers(decisions, deck_by_seat)
        conquers_total.update(cq)

        bursts, max_run = find_frozen_bursts(decisions)
        max_run_overall = max(max_run_overall, max_run)
        if bursts:
            games_with_bursts += 1
            for b in bursts:
                b = dict(b)
                b["file"] = filename
                b["deck"] = deck_by_seat.get(b["actor"], b["actor"])
                frozen_bursts.append(b)

        if any(has_burn_or_empower(d) for d in decisions):
            burn_empower_seen = True

        per_game.append({
            "file": filename,
            "decks": deck_by_seat,
            "turns": turns,
            "flow_printed": dict(fp),
            "flow_granted": dict(fg),
            "tomb_restricted": dict(tb),
            "reactions_closed": dict(rc),
            "conquers": dict(cq),
        })

        if winner_seat is not None:
            for c in find_candidates(decisions, owners, last_per_turn, loser_seat, "drop"):
                mistakes.append({
                    "game": filename,
                    "deck": loser_deck,
                    "idx": c["decision"].get("idx"),
                    "turn": c["decision"].get("turn"),
                    "phase": (c["decision"].get("phase") or {}),
                    "scores": get_scores(c["decision"]),
                    "margin_at_decision": c["margin_at_decision"],
                    "margin_at_turn_end": c["margin_at_turn_end"],
                    "delta": c["delta"],
                    "next_turn": c["next_turn"],
                    "board_summary": board_summary(c["decision"]),
                    "chosen": c["decision"].get("chosen"),
                })
            for c in find_candidates(decisions, owners, last_per_turn, winner_seat, "rise"):
                swings.append({
                    "game": filename,
                    "deck": winner_deck,
                    "idx": c["decision"].get("idx"),
                    "turn": c["decision"].get("turn"),
                    "phase": (c["decision"].get("phase") or {}),
                    "scores": get_scores(c["decision"]),
                    "margin_at_decision": c["margin_at_decision"],
                    "margin_at_turn_end": c["margin_at_turn_end"],
                    "delta": c["delta"],
                    "next_turn": c["next_turn"],
                    "board_summary": board_summary(c["decision"]),
                    "chosen": c["decision"].get("chosen"),
                })

    mistakes.sort(key=lambda m: m["delta"], reverse=True)
    swings.sort(key=lambda m: m["delta"], reverse=True)

    win_rates = {}
    for deck, by_seat in win_stats.items():
        win_rates[deck] = {}
        for seat, rec in by_seat.items():
            p, lo, hi = wilson_interval(rec["wins"], rec["games"])
            win_rates[deck][seat] = {
                "wins": rec["wins"],
                "games": rec["games"],
                "win_rate_pct": round(p * 100, 1),
                "ci95_low_pct": round(lo * 100, 1),
                "ci95_high_pct": round(hi * 100, 1),
            }

    tempo_out = {}
    for (deck, seat), by_turn in tempo.items():
        length = max(by_turn.keys())
        scores_curve = [
            round(sum(by_turn[t]) / len(by_turn[t]), 3) if by_turn.get(t) else None
            for t in range(1, length + 1)
        ]
        margins = margin_curve[(deck, seat)]
        margin_out = [
            round(sum(margins[t]) / len(margins[t]), 3) if margins.get(t) else None
            for t in range(1, length + 1)
        ]
        tempo_out[f"{deck}::{seat}"] = {
            "deck": deck,
            "seat": seat,
            "length": length,
            "mean_score_by_turn": scores_curve,
            "mean_margin_by_turn": margin_out,
        }

    family_mix_out = {
        deck: {phase: dict(counts) for phase, counts in phases.items()}
        for deck, phases in family_mix.items()
    }

    notes = [
        "The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin "
        "swing detector meant to point a reader at decisions worth reading, "
        "not a grading of play quality.",
        "Turn ownership is inferred (first non-Closed decision's actor per "
        "turn), not read from an explicit field.",
    ]
    if not burn_empower_seen:
        notes.append(
            "Burns/empowers OMITTED: no decision in this batch carried a "
            "burn/empower marker in the assumed schema; nothing to count."
        )
    if skipped_unparseable:
        notes.append(f"{skipped_unparseable} file(s) failed to parse as JSON and were skipped.")

    if frozen_bursts:
        notes.append(
            f"VALIDITY: {games_with_bursts} game(s) contain frozen-state "
            f"bursts (longest run {max_run_overall}). A run of identical "
            "decisions with no state change is an engine no-op, not play — "
            "treat the affected deck's numbers as engine evidence until "
            "the offer/executor mismatch is fixed."
        )

    result = {
        "schema": 1,
        "games_processed": len(games),
        "skipped_truncated": skipped_truncated,
        "skipped_unparseable": skipped_unparseable,
        "mean_turns_overall": round(sum(turns_all) / len(turns_all), 3) if turns_all else None,
        "win_rates": win_rates,
        "tempo": tempo_out,
        "family_mix": family_mix_out,
        "flow_plays": {
            "printed": dict(flow_printed_total),
            "granted": dict(flow_granted_total),
        },
        "tomb_restricted": dict(tomb_total),
        "conquers": dict(conquers_total),
        "reactions_in_closed_state": dict(reactions_closed_total),
        "burn_empower": None if not burn_empower_seen else "present (see per-decision data; no aggregate wired yet)",
        "per_game": per_game,
        "mistakes": mistakes[:top_n],
        "swings": swings[:top_n],
        "validity": {
            "frozen_bursts": frozen_bursts,
            "games_with_bursts": games_with_bursts,
            "max_run": max_run_overall,
            "min_run_flagged": FROZEN_BURST_MIN_RUN,
        },
        "notes": notes,
    }
    return result


# --------------------------------------------------------------------------
# summary.md rendering
# --------------------------------------------------------------------------

def _fmt_pct(x):
    return "n/a" if x is None else f"{x:.1f}%"


def render_markdown(summary: dict) -> str:
    lines = []
    lines.append("# Batch analysis summary")
    lines.append("")
    lines.append(
        "**The mistake/swing heuristic below is crude by design** — it is a "
        "score-margin swing detector meant to point a reader at decisions "
        "worth reading, not a grading of play quality."
    )
    lines.append("")
    lines.append(
        f"Games processed: {summary['games_processed']}  |  "
        f"skipped (truncated/incomplete): {summary['skipped_truncated']}  |  "
        f"skipped (unparseable): {summary['skipped_unparseable']}"
    )
    lines.append(f"Mean turns (overall): {summary['mean_turns_overall']}")
    lines.append("")
    v = summary.get("validity") or {}
    lines.append("## Validity")
    lines.append("")
    if v.get("frozen_bursts"):
        lines.append(
            f"**FROZEN-STATE BURSTS in {v['games_with_bursts']} game(s)** "
            f"(longest run {v['max_run']}, threshold {v['min_run_flagged']}): "
            "the same decision repeated with NO state change — an engine "
            "no-op. The affected deck's numbers below are engine evidence, "
            "not play, until the offer/executor mismatch is fixed."
        )
        for b in v["frozen_bursts"][:20]:
            lines.append(
                f"- {b['file']}: {b['deck']} ({b['actor']}) x{b['length']} "
                f"{b['type']} from decision {b['start_idx']} (turn {b['turn']})"
            )
        if len(v["frozen_bursts"]) > 20:
            lines.append(f"- ... and {len(v['frozen_bursts']) - 20} more")
    else:
        lines.append(
            f"OK — no frozen-state bursts (longest identical run "
            f"{v.get('max_run', 0)}, threshold {v.get('min_run_flagged', FROZEN_BURST_MIN_RUN)})."
        )
    lines.append("")

    lines.append("## Win rate by deck and seat (Wilson 95%)")
    lines.append("")
    lines.append("| Deck | Seat | Wins | Games | Win rate | 95% CI |")
    lines.append("|---|---|---|---|---|---|")
    for deck, by_seat in summary["win_rates"].items():
        for seat, rec in by_seat.items():
            lines.append(
                f"| {deck} | {seat} | {rec['wins']} | {rec['games']} | "
                f"{_fmt_pct(rec['win_rate_pct'])} | "
                f"{_fmt_pct(rec['ci95_low_pct'])} - {_fmt_pct(rec['ci95_high_pct'])} |"
            )
    lines.append("")

    lines.append("## Point-tempo curve (mean score / margin by turn)")
    lines.append("")
    for key, curve in summary["tempo"].items():
        lines.append(f"- **{key}** (turns 1..{curve['length']}): "
                      f"score={curve['mean_score_by_turn']} "
                      f"margin={curve['mean_margin_by_turn']}")
    lines.append("")

    lines.append("## Action-family mix per phase per deck")
    lines.append("")
    for deck, phases in summary["family_mix"].items():
        lines.append(f"- **{deck}**")
        for phase, counts in phases.items():
            lines.append(f"  - {phase}: {counts}")
    lines.append("")

    lines.append("## Per-game markers")
    lines.append("")
    lines.append(f"- Flow plays (printed): {summary['flow_plays']['printed']}")
    lines.append(f"- Flow plays (granted): {summary['flow_plays']['granted']}")
    lines.append(f"- Tomb-restricted plays: {summary['tomb_restricted']}")
    lines.append(f"- Conquers (score increments attributed to actor): {summary['conquers']}")
    lines.append(f"- Reaction plays in the closed state: {summary['reactions_in_closed_state']}")
    if summary["burn_empower"] is None:
        lines.append("- Burns/empowers: OMITTED (log schema carries no marker for these)")
    else:
        lines.append(f"- Burns/empowers: {summary['burn_empower']}")
    lines.append("")

    lines.append(f"## Mistake candidates (losing deck, top {len(summary['mistakes'])})")
    lines.append("")
    if not summary["mistakes"]:
        lines.append("None found.")
    for m in summary["mistakes"]:
        lines.append(
            f"- drop={m['delta']} game={m['game']} idx={m['idx']} turn={m['turn']} "
            f"deck={m['deck']} phase={m['phase']} scores={m['scores']} "
            f"chosen={m['chosen']}\n  board: {m['board_summary']}"
        )
    lines.append("")

    lines.append(f"## Swing decisions (winning deck, top {len(summary['swings'])})")
    lines.append("")
    if not summary["swings"]:
        lines.append("None found.")
    for m in summary["swings"]:
        lines.append(
            f"- rise={m['delta']} game={m['game']} idx={m['idx']} turn={m['turn']} "
            f"deck={m['deck']} phase={m['phase']} scores={m['scores']} "
            f"chosen={m['chosen']}\n  board: {m['board_summary']}"
        )
    lines.append("")

    if summary["notes"]:
        lines.append("## Notes")
        lines.append("")
        for n in summary["notes"]:
            lines.append(f"- {n}")
        lines.append("")

    return "\n".join(lines)


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("log_dir", type=Path, help="directory of per-game decision-log JSON files")
    parser.add_argument("--top", type=int, default=20, help="mistake/swing candidates to keep (default 20)")
    args = parser.parse_args(argv)

    summary = analyze_directory(args.log_dir, top_n=args.top)

    (args.log_dir / "summary.json").write_text(json.dumps(summary, indent=2, sort_keys=False) + "\n")
    (args.log_dir / "summary.md").write_text(render_markdown(summary))

    print(f"Wrote {args.log_dir / 'summary.json'} and {args.log_dir / 'summary.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
