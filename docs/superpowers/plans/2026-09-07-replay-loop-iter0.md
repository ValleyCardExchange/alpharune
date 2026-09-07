# Replay-analysis loop, Iteration 0 — implementation plan

> TDD per task; RED observed then GREEN; no literal code in this plan
> (house rule). Spec: `docs/superpowers/specs/2026-09-07-replay-loop-iter0-design.md`.

**Global constraints:** branch `replay-loop` off `master` `8006d38`;
baseline suite 1199 passed / 1 pre-existing disabled; coverage 792 / 0.
No card or rules-engine edits. No core-header edits unless the spec names
them (none expected: `game_runner`, `main`, `agent_spec`, `mcts_agent`,
`corpus_evaluator` and new `src/io` files only). Commit trailers as on
this fork. Existing command lines and results unchanged when the new
flags are absent (guarded by the existing tests and by re-running one
seed of the Task 10 smoke command).

### L1 — decision log writer + `--decision-log`
Files: create `src/io/decision_log_writer.h/.cpp`, `tests/test_decision_log.cpp`;
modify `src/engine/game_runner.h/.cpp` (config field + hookup beside the
replay writer), `src/main.cpp` (flag parse + help text), `CMakeLists.txt`
only if `src/io` sources are listed rather than globbed.
Steps: spec tests (1)(2) RED → writer → GREEN; test (3) RED → runner/flag
→ GREEN; full suite; run one Task-10 smoke seed with and without the flag
and diff the game outcome (must be identical); commit
`Loop: decision log writer and --decision-log`.

### L2 — analysis script
Files: create `scripts/analyze_batch.py`, `tests/analysis/test_analyze_batch.py`.
Steps: unittest fixture + failing tests → implement aggregates → mistake
candidates → summary.md renderer; run against the L1 end-to-end output
of 2 random games as a smoke; commit `Loop: batch analysis script`.

### L3 — first batch + playbook (controller session)
Run the two 200-game batches with `--decision-log`; run the script;
commit the summaries under `docs/superpowers/smoke/2026-09-08-loop-iter0/`;
Claude reads `summary.md` + the top mistake candidates and writes the
vault playbook + priors JSON in personal-ai (its own commit there).

### L0b — engine fixes surfaced by the loop (spec addenda #15 and #16)
Addendum #15 (closed-state ability activations) landed as `8b26d29`. This
task is addendum #16 — equip legality.
Files: `src/cards/card.h` (a `canEquip` legality predicate on `Card`,
const, taking the game state and the controller; default `true` so the
registry guard below names every gear that forgets it), `src/cards/gear/
equip_base.h` (extract the standard pre-check into a shared predicate
used by BOTH `standardEquip` and `SimpleEquipGear::canEquip`; a sibling
predicate for `UniversalEquipGear` — ready runes cover the energy AND at
least one rune remains in base to recycle, exhausted allowed), `src/cards/
card_helpers.h` (`payOnePower` loses its ready-only condition per CR
164.2.b and prefers an exhausted rune, mirroring the engine's canonical
payer; a matching `canPayOnePower` predicate), every hand-written equip
gear (`0471` Last Rites, `0508`, `0601`, `0412`, `0498`, `0720`, `0460`,
`0748`, `0382`, `0365`) gains `canEquip` mirroring its existing pre-check
and calls it first in `onEquip` (single source of truth per card — no
duplicated conditions), `src/engine/game_engine.cpp`
(`generateMainPhaseActions` equip block skips gear whose `canEquip` is
false; `executeIntent` logs a warning naming the gear when `onEquip`
returns false), new `tests/cards/test_equip_legality.cpp`.
Behaviors: an unpayable equip is never offered; a payable one is offered
exactly as before; `onEquip` never mutates state when it returns false;
an exhausted rune can be recycled for power.
Tests (RED first, each): (1) registry guard — every registered card with
`hasEquipAbility` reports `canEquip == false` on a state with no runes in
base, empty trash, 0 XP, and the failure message names the gear; (2)
registry guard — on that state `onEquip` returns false for each and a
snapshot of trash/main-deck/rune-deck/rune exhaustion/XP is unchanged;
(3) Last Rites: two trash cards + only an EXHAUSTED Chaos rune → offered,
equips, trash −2, the Chaos rune recycled, gear attached; two trash + no
Chaos rune → not offered, `onEquip` false, trash unchanged; one trash +
Chaos rune → not offered; (4) generator: an unattached unpayable Last
Rites yields no ActivateAbility intent with that ability source; payable
yields one per friendly unit; (5) faithfulness: Soul Sword equips with
every Calm rune exhausted; (6) executor: a hand-built unpayable equip
intent through `testHook_executeIntent` emits a warning log event and
leaves the snapshot unchanged (capture pattern as in
`test_closed_state_abilities.cpp`).
Existing suites `test_equipment.cpp` and the audit-fix suites must stay
green unchanged. Coverage gate unchanged. Commit subject: `Engine: equip
offers gated by Card::canEquip; rejected equips warn; power recycle per
CR 164.2.b`.

### L4 — `prior=` injection
Files: modify `src/agents/agent_spec.h/.cpp`, `src/agents/mcts_agent.h/.cpp`,
`src/agents/corpus_evaluator.h/.cpp`, `src/main.cpp`; tests in
`tests/test_agent_spec.cpp` (exists from Task 12), `tests/test_corpus_evaluator.cpp`,
new `tests/test_prior_config.cpp`.
Steps: parse/validate tests RED → GREEN; `CorpusWeights` refactor with the
defaults test proving byte-identical values; `Prior()` override test;
one game with `prior=` runs; full suite; commit `Loop: prior=<file>
injection for MCTS priors and evaluator weights`.

### L5 — acceptance (controller session)
Seed-matched batches new-vs-frozen; Wilson; verdict per spec; commit
`Loop: iteration 0 acceptance results`; personal-ai playbook updated with
CONFIRMED / NOT CONFIRMED per lesson.
