"""Tests for scripts/analyze_batch.py.

Run with:
    python3 -m unittest discover -s tests/analysis -v
"""

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT_PATH = REPO_ROOT / "scripts" / "analyze_batch.py"

_spec = importlib.util.spec_from_file_location("analyze_batch", SCRIPT_PATH)
analyze_batch = importlib.util.module_from_spec(_spec)
sys.modules["analyze_batch"] = analyze_batch
_spec.loader.exec_module(analyze_batch)


def _phase(turn_phase="Main", ns="Neutral", oc="Open"):
    # Real writer's field names (src/io/decision_log_writer.cpp): the
    # turn-phase name is nested under a key ALSO called "phase".
    return {"phase": turn_phase, "ns_state": ns, "oc_state": oc}


def _decision(idx, turn, actor, scores, chosen, phase=None, battlefields=None):
    return {
        "idx": idx,
        "turn": turn,
        "phase": phase or _phase(),
        "actor": actor,
        "scores": list(scores),
        "players": [
            {"seat": "P1", "hand": 5, "deck": 30, "trash": 0, "banishment": 0,
             "runes_ready": 1, "runes_exhausted": 0, "legend_empowered": False},
            {"seat": "P2", "hand": 5, "deck": 30, "trash": 0, "banishment": 0,
             "runes_ready": 1, "runes_exhausted": 0, "legend_empowered": False},
        ],
        "battlefields": battlefields if battlefields is not None else [
            {"id": 1, "name": "BF1", "controller": "None", "contested": False,
             "units": {"p1": {"count": 0, "might": 0}, "p2": {"count": 0, "might": 0}}},
        ],
        "legal_count": 8,
        "chosen": chosen,
        "root_value": None,
    }


def _chosen(type_, card=None, source_zone="Hand", flow_source="None",
            restricted_bf=None, destination=None):
    return {
        "type": type_,
        "card": card,
        "source_zone": source_zone,
        "flow_source": flow_source,
        "restricted_bf": restricted_bf,
        "destination": destination,
    }


def _dest_bf(bf_id):
    return {"type": "battlefield", "id": bf_id}


def _dest_base(player):
    return {"type": "base", "player": player}


def build_fixture(tmpdir: Path):
    """Writes three synthetic game files (game1, game2 usable; game3 truncated)."""

    # ---------------- game 1: deckA=P1 (winner), deckB=P2 (loser) --------
    decisions1 = []
    # turn 1 (P1) - one of every family, plus Flow printed/granted + tomb.
    decisions1.append(_decision(0, 1, "P1", [0, 0], _chosen("PlayCard", "UnitA")))
    decisions1.append(_decision(1, 1, "P1", [0, 0], _chosen("PlayCard", "FlowUnit", flow_source="Printed")))
    decisions1.append(_decision(2, 1, "P1", [0, 0], _chosen(
        "PlayCard", "GrantedFlowUnit", flow_source="Granted", restricted_bf=2)))
    decisions1.append(_decision(3, 1, "P1", [0, 0], _chosen("StandardMove", destination=_dest_bf(1))))
    decisions1.append(_decision(4, 1, "P1", [0, 0], _chosen("StandardMove", destination=_dest_base("P1"))))
    decisions1.append(_decision(5, 1, "P1", [0, 0], _chosen("AssignCombatDamage")))
    decisions1.append(_decision(6, 1, "P1", [0, 0], _chosen("ActivateAbility")))
    decisions1.append(_decision(7, 1, "P1", [0, 0], _chosen("MakeChoice")))
    decisions1.append(_decision(8, 1, "P1", [0, 0], _chosen("ChooseBattlefield")))
    decisions1.append(_decision(9, 1, "P1", [0, 0], _chosen("EndTurn")))  # last of turn 1

    # turn 2 (P2) - two conquers of different size (drives the mistake test)
    decisions1.append(_decision(10, 2, "P2", [0, 1], _chosen("PlayCard", "SmallUnit")))
    decisions1.append(_decision(11, 2, "P2", [0, 2], _chosen("EndTurn")))  # last of turn 2

    # turn 3 (P1) - a Closed-state reaction play (also the constructed swing),
    # then P1 conquers to 3.
    decisions1.append(_decision(
        12, 3, "P1", [0, 2], _chosen("PlayReaction", "Trick"), phase=_phase(oc="Closed"), ))
    decisions1.append(_decision(13, 3, "P1", [3, 2], _chosen("PlayCard", "BigUnit")))
    decisions1.append(_decision(14, 3, "P1", [3, 2], _chosen("EndTurn")))  # last of turn 3

    # turn 4 (P2) - P2's next own turn after turn 2; no further movement.
    decisions1.append(_decision(15, 4, "P2", [3, 2], _chosen("ActivateAbility")))
    decisions1.append(_decision(16, 4, "P2", [3, 2], _chosen("EndTurn")))  # last of turn 4

    # turn 5 (P1) - P1's next own turn after turn 3; no further movement.
    decisions1.append(_decision(17, 5, "P1", [3, 2], _chosen("PlayCard", "SmallUnit2")))
    decisions1.append(_decision(18, 5, "P1", [3, 2], _chosen("EndTurn")))  # last of turn 5

    # turn 6 (P2) - P2's next own turn after turn 4; game ends here.
    decisions1.append(_decision(19, 6, "P2", [3, 2], _chosen("PlayCard", "LateUnit")))
    decisions1.append(_decision(20, 6, "P2", [3, 2], _chosen("EndTurn")))  # last of turn 6

    game1 = {
        "schema": 1,
        "decks": {"p1": {"path": "decks/deckA.txt", "legend": "Alpha", "champion": "Alpha"},
                  "p2": {"path": "decks/deckB.txt", "legend": "Beta", "champion": "Beta"}},
        "agents": {"p1": "mcts:sims=20", "p2": "mcts:sims=20"},
        "seed": 2000,
        # Real writer keys "seats" by deck-file basename WITH extension.
        "seats": {"deckA.txt": "P1", "deckB.txt": "P2"},
        "engine_version": "test-0.0.1",
        "log": decisions1,
        # Real writer splices winner/reason/turns/final_scores/decisions
        # directly onto the top-level object (no nested "footer" key).
        "winner": {"deck": "Alpha / Alpha", "seat": "P1"},
        "reason": "turns_exhausted",
        "turns": 6,
        "final_scores": [3, 2],
        "decisions": len(decisions1),
    }

    # ---------------- game 2: deckA=P2 (winner), deckB=P1 (loser) --------
    decisions2 = [
        _decision(0, 1, "P1", [0, 0], _chosen("PlayCard", "UnitC")),
        _decision(1, 1, "P1", [0, 0], _chosen("EndTurn")),
        _decision(2, 2, "P2", [1, 3], _chosen("PlayCard", "UnitD")),
        _decision(3, 2, "P2", [1, 3], _chosen("EndTurn")),
    ]
    game2 = {
        "schema": 1,
        "decks": {"p1": {"path": "decks/deckB.txt", "legend": "Beta", "champion": "Beta"},
                  "p2": {"path": "decks/deckA.txt", "legend": "Alpha", "champion": "Alpha"}},
        "agents": {"p1": "mcts:sims=20", "p2": "mcts:sims=20"},
        "seed": 3000,
        "seats": {"deckA.txt": "P2", "deckB.txt": "P1"},
        "engine_version": "test-0.0.1",
        "log": decisions2,
        "winner": {"deck": "Alpha / Alpha", "seat": "P2"},
        "reason": "conquest",
        "turns": 2,
        "final_scores": [1, 3],
        "decisions": len(decisions2),
    }

    # ---------------- game 3: truncated (best-effort partial) ------------
    # Mirrors DecisionLogWriter's destructor-on-crash output: only "schema"
    # through the partial "log" plus "truncated": true -- no winner/reason/
    # turns/final_scores/decisions (finish() never ran).
    game3 = {
        "schema": 1,
        "decks": {"p1": {"path": "decks/deckA.txt"}, "p2": {"path": "decks/deckB.txt"}},
        "agents": {"p1": "mcts:sims=20", "p2": "mcts:sims=20"},
        "seed": 4000,
        "seats": {"deckA.txt": "P1", "deckB.txt": "P2"},
        "engine_version": "test-0.0.1",
        "log": [_decision(0, 1, "P1", [0, 0], _chosen("PlayCard", "Whatever"))],
        "truncated": True,
    }

    (tmpdir / "game_001_seed_2000.json").write_text(json.dumps(game1))
    (tmpdir / "game_002_seed_3000.json").write_text(json.dumps(game2))
    (tmpdir / "game_003_seed_4000.json").write_text(json.dumps(game3))


class WilsonIntervalTests(unittest.TestCase):
    def test_known_value_92_of_200(self):
        p, lo, hi = analyze_batch.wilson_interval(92, 200)
        self.assertAlmostEqual(round(lo * 100, 1), 39.2)
        self.assertAlmostEqual(round(hi * 100, 1), 52.9)
        self.assertAlmostEqual(round(p * 100, 1), 46.0)

    def test_zero_games(self):
        p, lo, hi = analyze_batch.wilson_interval(0, 0)
        self.assertEqual((p, lo, hi), (0.0, 0.0, 0.0))

    def test_all_wins(self):
        p, lo, hi = analyze_batch.wilson_interval(10, 10)
        self.assertEqual(p, 1.0)
        self.assertLess(hi, 1.0 + 1e-9)
        self.assertGreater(lo, 0.0)


class FamilyClassificationTests(unittest.TestCase):
    def test_play_types(self):
        for t in ("PlayCard", "PlayReaction", "PlayActionCard", "HideCard"):
            self.assertEqual(analyze_batch.classify_family({"type": t}), "play")

    def test_move_split(self):
        self.assertEqual(
            analyze_batch.classify_family({"type": "StandardMove", "destination": 3}),
            "move_to_battlefield")
        self.assertEqual(
            analyze_batch.classify_family({"type": "StandardMove", "destination": None}),
            "move_to_base")

    def test_activate_choice_setup_pass_concede(self):
        self.assertEqual(analyze_batch.classify_family({"type": "ActivateAbility"}), "activate")
        self.assertEqual(analyze_batch.classify_family({"type": "MakeChoice"}), "choice")
        self.assertEqual(analyze_batch.classify_family({"type": "ChooseBattlefield"}), "setup")
        self.assertEqual(analyze_batch.classify_family({"type": "EndTurn"}), "pass")
        self.assertEqual(analyze_batch.classify_family({"type": "Concede"}), "concede")

    def test_unknown_type_does_not_raise(self):
        self.assertEqual(analyze_batch.classify_family({"type": "SomeFutureIntent"}), "unknown")

    def test_missing_chosen_does_not_raise(self):
        self.assertEqual(analyze_batch.classify_family(None), "unknown")


class BatchAnalysisTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmpdir = Path(self._tmp.name)
        build_fixture(self.tmpdir)
        self.summary = analyze_batch.analyze_directory(self.tmpdir, top_n=20)

    def tearDown(self):
        self._tmp.cleanup()

    def test_truncated_game_counted_and_skipped(self):
        self.assertEqual(self.summary["skipped_truncated"], 1)
        self.assertEqual(self.summary["games_processed"], 2)

    def test_mean_turns(self):
        self.assertAlmostEqual(self.summary["mean_turns_overall"], 4.0)

    def test_win_rate_seat_split(self):
        wr = self.summary["win_rates"]
        self.assertEqual(wr["deckA"]["P1"]["wins"], 1)
        self.assertEqual(wr["deckA"]["P1"]["games"], 1)
        self.assertEqual(wr["deckA"]["P2"]["wins"], 1)
        self.assertEqual(wr["deckA"]["P2"]["games"], 1)
        self.assertEqual(wr["deckB"]["P2"]["wins"], 0)
        self.assertEqual(wr["deckB"]["P2"]["games"], 1)
        self.assertEqual(wr["deckB"]["P1"]["wins"], 0)
        self.assertEqual(wr["deckB"]["P1"]["games"], 1)

    def test_tempo_curve_shape_and_values(self):
        curve = self.summary["tempo"]["deckA::P1"]
        self.assertEqual(curve["length"], 6)
        self.assertEqual(curve["mean_score_by_turn"], [0, 0, 3, 3, 3, 3])
        self.assertEqual(curve["mean_margin_by_turn"], [0, -2, 1, 1, 1, 1])

        curve_b = self.summary["tempo"]["deckB::P2"]
        self.assertEqual(curve_b["length"], 6)
        self.assertEqual(curve_b["mean_score_by_turn"], [0, 2, 2, 2, 2, 2])

        curve_a2 = self.summary["tempo"]["deckA::P2"]
        self.assertEqual(curve_a2["length"], 2)
        self.assertEqual(curve_a2["mean_score_by_turn"], [0, 3])

    def test_family_mix(self):
        mix = self.summary["family_mix"]
        deckA_main = mix["deckA"]["Main"]
        self.assertEqual(deckA_main["play"], 7)          # 3 PlayCard + 1 PlayReaction + 2 PlayCard (g1) + 1 PlayCard (g2, P2 seat)
        self.assertEqual(deckA_main["move_to_battlefield"], 1)
        self.assertEqual(deckA_main["move_to_base"], 1)
        self.assertEqual(deckA_main["combat_damage"], 1)
        self.assertEqual(deckA_main["activate"], 1)
        self.assertEqual(deckA_main["choice"], 1)
        self.assertEqual(deckA_main["setup"], 1)
        self.assertEqual(deckA_main["pass"], 4)           # 3 EndTurn (g1) + 1 EndTurn (g2)

        deckB_main = mix["deckB"]["Main"]
        self.assertEqual(deckB_main["play"], 3)           # 2 PlayCard (g1, P2) + 1 PlayCard (g2, P1)
        self.assertEqual(deckB_main["activate"], 1)
        self.assertEqual(deckB_main["pass"], 4)           # 3 EndTurn (g1) + 1 EndTurn (g2)

        # The Closed-state reaction lands in its own phase bucket key by
        # phase name (still "Main" here since only oc differs) and family "play".
        self.assertIn("play", mix["deckA"]["Main"])

    def test_flow_printed_and_granted_counts(self):
        self.assertEqual(self.summary["flow_plays"]["printed"], {"deckA": 1})
        self.assertEqual(self.summary["flow_plays"]["granted"], {"deckA": 1})

    def test_tomb_restricted_counts(self):
        self.assertEqual(self.summary["tomb_restricted"], {"deckA": 1})

    def test_reactions_in_closed_state(self):
        self.assertEqual(self.summary["reactions_in_closed_state"], {"deckA": 1})

    def test_conquers(self):
        self.assertEqual(self.summary["conquers"]["deckA"], 2)
        self.assertEqual(self.summary["conquers"]["deckB"], 2)

    def test_burn_empower_omitted_with_note(self):
        self.assertIsNone(self.summary["burn_empower"])
        self.assertTrue(any("Burns/empowers OMITTED" in n for n in self.summary["notes"]))

    def test_mistake_candidates_selection_and_order(self):
        mistakes = self.summary["mistakes"]
        self.assertEqual(len(mistakes), 2)
        self.assertEqual(mistakes[0]["delta"], 3)
        self.assertEqual(mistakes[0]["idx"], 11)
        self.assertEqual(mistakes[0]["deck"], "deckB")
        self.assertEqual(mistakes[1]["delta"], 2)
        self.assertEqual(mistakes[1]["idx"], 10)
        # ordered by size, descending
        self.assertGreater(mistakes[0]["delta"], mistakes[1]["delta"])

    def test_swing_decisions(self):
        swings = self.summary["swings"]
        self.assertEqual(len(swings), 1)
        self.assertEqual(swings[0]["delta"], 3)
        self.assertEqual(swings[0]["idx"], 12)
        self.assertEqual(swings[0]["deck"], "deckA")

    def test_top_n_limits_candidates(self):
        summary = analyze_batch.analyze_directory(self.tmpdir, top_n=1)
        self.assertEqual(len(summary["mistakes"]), 1)
        self.assertEqual(summary["mistakes"][0]["delta"], 3)

    def test_summary_md_contains_crude_caveat(self):
        md = analyze_batch.render_markdown(self.summary)
        self.assertIn("crude by design", md)

    def test_main_writes_summary_files(self):
        analyze_batch.main([str(self.tmpdir)])
        self.assertTrue((self.tmpdir / "summary.json").exists())
        self.assertTrue((self.tmpdir / "summary.md").exists())
        data = json.loads((self.tmpdir / "summary.json").read_text())
        self.assertEqual(data["games_processed"], 2)
        md_text = (self.tmpdir / "summary.md").read_text()
        self.assertIn("crude by design", md_text)


if __name__ == "__main__":
    unittest.main()
