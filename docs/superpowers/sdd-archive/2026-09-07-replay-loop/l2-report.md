# L2 — batch analysis script: report

## Scope

Only touched `scripts/analyze_batch.py` and `tests/analysis/` (new dirs
created as needed). Did not build or run any C++. The parallel L1 agent's
files (`src/io/decision_log_writer.{h,cpp}`, `CMakeLists.txt`,
`src/engine/game_runner.{h,cpp}`, `tests/test_decision_log.cpp`) were left
untouched and unstaged; `git status` before committing confirmed the
staged diff was exactly the two intended paths.

## TDD evidence (RED -> GREEN)

1. Wrote `tests/analysis/test_analyze_batch.py` first (23 tests: Wilson
   interval math, family classification, and a `BatchAnalysisTests` suite
   over a synthetic 3-game fixture built in a temp dir).
2. Wrote `scripts/analyze_batch.py`.
3. To get a genuine RED before trusting GREEN, temporarily moved
   `analyze_batch.py` out of the tree and reran the suite:
   ```
   ImportError: Failed to import test module: test_analyze_batch
   ...
   FileNotFoundError: [Errno 2] No such file or directory:
   '/home/user/chorlick/alpharune/scripts/analyze_batch.py'
   Ran 1 test in 0.000s
   FAILED (errors=1)
   ```
4. Restored the script and ran again: one real failure
   (`test_family_mix`, off-by-one in a hand-counted expectation in the
   test itself — 6 vs actual 7 `PlayCard`/`PlayReaction` decisions for
   `deckA`), fixed the test's expected count, reran: all green.

Final run (`python3 -m unittest discover -s tests/analysis -v`):
```
Ran 23 tests in 0.023s
OK
```
23/23 passing, 0 failures, 0 errors.

## Important finding: aligned to the REAL L1 schema, not just the spec's prose

The parallel L1 agent's `src/io/decision_log_writer.cpp` was already
present in the working tree (uncommitted) by the time I got to
implementation. I read it (did not build/run it) and found several real
field shapes that differ from the plain-English field contract in the
task prompt / design spec:

- The decision list is keyed **`"log"`**, not `"decisions"` — the game
  file's top-level `"decisions"` key is the footer's decision **count**
  (an int), not a list.
- There is no nested `"footer"` object — on a clean finish, `winner`,
  `reason`, `turns`, `final_scores`, `decisions` are spliced directly
  onto the **top-level** object.
- `phase` is `{"phase": "<turn phase>", "ns_state": ..., "oc_state": ...}`
  — the turn-phase name is nested under a key also called `"phase"`, and
  open/closed is `oc_state`, not `oc`.
- Battlefield `units` are keyed lowercase `"p1"`/`"p2"`.
- `chosen.destination` is `{"type": "battlefield", "id": N}` or
  `{"type": "base", "player": "P1"|"P2"}` (or `null`), not a bare
  battlefield id / `"base"` string.
- `seats` is keyed by the deck file's **basename with extension** (e.g.
  `"kennen_tyler.txt"`) mapped to `"P1"`/`"P2"`.

I updated `analyze_batch.py`'s parsing (`get_decisions_list`,
`get_footer`, `phase_name`, `is_closed`, `classify_family`'s destination
handling, `deck_label`'s extension-stripping, a case-insensitive
`get_ci` helper for the `p1`/`p2` unit keys) to match this real shape as
the primary path, while keeping the spec's more generic shape as a
tolerant fallback in case the writer's format still moves before L3 runs
this for real. Rewrote the test fixture to the real shape too, so the
tests exercise what L3 will actually feed the script, not just my
original reading of the prose spec. This is documented in the script's
module docstring under "ON-DISK SCHEMA".

## Files

- `/home/user/chorlick/alpharune/scripts/analyze_batch.py` — the analyzer
  (Wilson interval, action-family classification, tempo curves, per-game
  markers, mistake/swing heuristic, summary.json + summary.md writers,
  CLI).
- `/home/user/chorlick/alpharune/tests/analysis/test_analyze_batch.py` —
  unittest suite with a hand-built 3-game synthetic fixture (two decks,
  both seats represented, one truncated game).

## What the script produces (`summary.json` / `summary.md` in the log dir)

- Win rate by deck+seat with Wilson 95% CIs (self-implemented; verified
  against the spec's 92/200 -> 39.2–52.9 example, `wilson_interval`
  reproduces it exactly).
- Mean turns overall.
- Point-tempo curve: mean score AND mean score margin by turn index, per
  (deck, seat).
- Action-family mix per phase per deck (play / combat_damage /
  move_to_battlefield / move_to_base / activate / choice / setup / pass /
  concede).
- Flow plays split printed/granted, Tomb-restricted plays, conquers
  (derived from consecutive `scores`), reaction plays in the closed
  state — all per deck.
- Mistake candidates (losing deck) and swing decisions (winning deck):
  top N (`--top`, default 20) by size of the score-margin swing between
  a decision and the end of that seat's next own turn, each with game
  file, idx, turn, phase, scores, a one-line board summary, and the
  chosen action. `summary.md` states up front that this heuristic is
  "crude by design."
- Burns/empowers: the given field contract carries no explicit marker
  for these (only `legend_empowered`, a status, not an action count), so
  the script looks for an optional `burn`/`empower`/`effect_tags` marker
  and, finding none in this batch, **omits** the section with a note —
  per the spec's "if the log carries them (else omit with a note)."
- Also noted: "the three alternatives with the highest MCTS prior family
  weight" mentioned in the design spec for mistake candidates has no
  corresponding field in the L1 field contract given to me (only
  `legal_count`, an int, not itemized legal actions/weights) — omitted,
  should be revisited if L1 ever adds per-legal-action detail.

## Concerns / follow-ups for the controller (L3) session

1. **Re-verify against a real 2-game smoke log directory before the full
   200-game batches.** I read `decision_log_writer.cpp` and aligned to
   it, but I never ran the C++ (out of scope for L2) and the L1 agent may
   still be revising it in parallel. If `winner.deck` ends up being a
   `"<legend> / <champion>"` string rather than matching a deck name from
   `seats`/`decks`, that's fine — my code keys everything off `seats`
   (deck-file basename), not `winner.deck`, and only uses `winner.seat`.
2. The mistake/swing heuristic's "turn ownership" (who owns a turn, for
   finding "that seat's next own turn") is inferred from the actor of the
   first non-Closed decision in the turn — there's no explicit
   turn-owner field in the schema. Crude by design, stated as such in
   `summary.md`.
3. `deck_label` strips a trailing extension from bare filename-shaped
   seat values (e.g. `"kennen_tyler.txt"` -> `"kennen_tyler"`) so it
   reads cleanly and matches `decks/kennen_tyler.txt` from the L3 batch
   spec; if `champion`/`legend` names are preferred as deck labels
   instead, that's a small follow-up to `resolve_seat_decks`.
4. `--top N` controls both mistakes and swings list length (shared flag,
   default 20 per spec).

## Commit

`818ef91` "Loop: batch analysis script" on branch `replay-loop`, exactly
two files (`scripts/analyze_batch.py`, `tests/analysis/test_analyze_batch.py`),
1185 insertions, ending with the required
`Co-Authored-By`/`Claude-Session` trailer.
