# Replay-analysis loop — Iteration 0 design spec

Date: 2026-09-07. Approved shape: "Full Iteration 0" (Tyler, in chat).
Branch `replay-loop` off the fork's `master` (`8006d38`). Environment
verified before this spec: suite 1199 green on that tree, batch runner
clean at `--threads 4` after the Baron-Pit fix.

## Goal (from the north star, personal-ai `riftbound/self-play-engine.md`)

Batch self-play → machine-readable decision logs → analysis → a written
matchup playbook (human notes in Tyler's vault + a machine-readable priors
file) → injected into the agents' priors → acceptance test against the
frozen previous agent. Two-directional by Tyler's ruling: a lesson that
does not reach the agents is a loop failure; the acceptance test decides
whether a lesson is kept.

## Components

### L1 — decision log (engine, `src/io/decision_log_writer.{h,cpp}`)
- New CLI flag `--decision-log <dir>` (single-game and `--games N` batch
  modes). Per game one file `game_<index>_seed_<seed>.json`.
- Hooked exactly like `ReplayWriter` in `src/engine/game_runner.cpp` (the
  `on_decision` callback, `game_engine.h:172-177`) and `src/main.cpp`.
  Independent of `--render-html`; both may be on.
- Header: decks (paths + legend/champion names), agent specs, seed, seat
  map (deck → P1/P2), engine version string.
- One record per decision: `idx`, `turn`, `phase` (turn phase +
  neutral/showdown + open/closed), `actor`, `scores` [P1,P2], per player
  {hand, deck, trash, banishment sizes; ready and exhausted rune counts;
  legend empowered}, per battlefield {id, name, controller, contested,
  units per player as count and total might}, `legal_count`, `chosen`
  {intent type, card name if any, source zone, `flow_source`,
  `restricted_bf`, destination battlefield for moves/plays}, and, when
  the deciding agent supplied one, the MCTS root value estimate (the same
  number `ReplayWriter::setNextWinProb` receives). No full object dumps.
- Footer: winner (deck and seat), reason, turns, final scores, decision
  count.
- Size target ≤ 200 KB per game; the writer streams records and closes
  the JSON array at game end (a crashed game leaves a parseable prefix
  plus a `"truncated": true` marker on best effort).
- Tests (`tests/test_decision_log.cpp`, GoogleTest, TDD): (1) a writer
  fed two synthetic decisions and a footer produces valid JSON with every
  field above; (2) a decision whose intent has `flow_source = Granted`
  and a `target_battlefield_restriction` serialises both; (3) end-to-end:
  `--games 2 --decision-log <tmp>` via the batch runner writes two
  parseable files with matching seeds and footers (drive `GameRunner`
  directly with random agents to keep it fast).

### L2 — batch analysis (`scripts/analyze_batch.py`, Python 3 stdlib)
- Input: a log directory. Output: `summary.json` + `summary.md` in it.
- Aggregates: per deck win rate by seat with Wilson 95% intervals; mean
  turns; mean score by turn index per deck and seat (the point-tempo
  curve); action-family mix per phase per deck; counts per game of
  Flow plays (printed vs granted), Tomb-restricted plays, conquers,
  burns, empower/disempower actions, reaction plays in the closed state.
- "Mistake candidates" (crude by design, stated in the output): for the
  losing deck, decisions after which its score margin at the end of its
  NEXT own turn fell by ≥ 2 relative to the margin at the decision — top
  20 by drop, each with turn/phase/board summary/chosen action and the
  three alternatives with the highest MCTS prior family weight if the log
  has legal-action families. "Swing decisions" symmetric for the winner.
- Tests: `tests/analysis/test_analyze_batch.py` (unittest, stdlib) over a
  synthetic 3-game fixture generated in the test — Wilson math, seat
  split, tempo curve shape, mistake-candidate selection; documented run
  command in the script header. The C++ suite is unaffected.

### L3 — the first batch and the first playbook
- Batch: Kennen (`decks/kennen_tyler.txt`) vs Rengar
  (`decks/rengar_test.txt`), `mcts:sims=20` score evaluator both seats,
  200 games per seat, `--threads 4`, seeds 2000 (Kennen P1) and 3000
  (Kennen P2), `--decision-log`. Analysis run; both `summary.*` committed
  under `docs/superpowers/smoke/2026-09-08-loop-iter0/`.
- Playbook (Claude writes, controller session): personal-ai vault
  `docs/knowledge/riftbound/playbooks/kennen-vs-rengar.md` — mistake
  notes and a macro/micro plan for the matchup, each claim citing the
  batch statistic it rests on, with the standing caveat (20-sim MCTS is
  not expert play; these are the AGENTS' mistakes, and only where they
  coincide with corpus evidence are they Tyler's lessons). Alongside it
  the machine-readable priors file `kennen-vs-rengar.prior.json`
  (schema below). The vault is the single source of truth; the fork
  reads the file by path — nothing is retyped.

### L4 — prior injection (`prior=<path>` on the agent spec)
- `AgentSpec` gains `prior` (path, optional). `parseAgentSpec` validates
  the file exists and parses (nlohmann::json, already a dependency).
- Schema v1: `{"schema":1, "matchup":"...", "written":"YYYY-MM-DD",
  "action_family_weights": {"play":4.0, "combat_damage":4.0,
  "move_to_battlefield":3.0, "move_to_base":0.6, "activate":2.5,
  "choice":1.5, "setup":1.0, "pass":0.3, "concede":0.01},
  "evaluator_weights": {"score":1.0, "battlefield":0.5, "unit":0.25,
  "held_interaction":0.15, "trash_resource":0.1, "empowered_legend":0.1}}`
  — every key optional; defaults are today's constants; unknown keys are
  an error (a typo must not be silent).
- `MctsAgent` takes a `PriorConfig` (defaults = the current hard-coded
  `Prior()` weights and `kCorpus*` constants); `Prior()` reads its family
  weights from it; the corpus evaluator takes a `CorpusWeights` struct
  (the constexprs become the struct's defaults). Behaviour with no
  `prior=` is byte-identical (guarded by the existing tests).
- Tests: parse/validate (missing file, bad key, partial override merges
  with defaults); `Prior()` reflects an overridden family weight;
  `corpusEvaluate` reflects an overridden evaluator weight; a game runs
  with `prior=`.

### L5 — acceptance
- New agent = `mcts:sims=20,eval=<same as batch>,prior=<vault file>`;
  frozen previous = the same spec without `prior=`. Kennen vs Rengar,
  200 games per seat, seed-matched to L3, `--threads 4`. Wilson 95% on
  the Kennen win count. ACCEPT the lesson if the interval's lower bound
  exceeds the frozen agent's point estimate AND the interval excludes
  50%-of-baseline equivalence as stated in the notes; otherwise REJECT and
  record "not confirmed" in the playbook (the notes stay, marked
  unconfirmed — a rejected lesson is still knowledge about the agent).
  Results in `docs/superpowers/smoke/2026-09-08-loop-iter0/acceptance.md`.

## Non-goals (Iteration 0)
No learning (no weights fitted from data); no ISMCTS; no imitation from
RiftLite replays; no changes to card or rules code; no attempt to make
the mistake heuristic clever — it exists to point Claude at decisions
worth reading, not to grade them.

## Known limits, stated
The playbook's "mistakes" are a 20-sim MCTS agent's, found by a crude
score-swing heuristic; the acceptance test measures whether the injected
priors help THAT agent — it says nothing about Tyler's play except where
the corpus already agrees. Sample sizes (200/seat) give ±7% intervals; a
lesson worth less than that will read as "not confirmed".
