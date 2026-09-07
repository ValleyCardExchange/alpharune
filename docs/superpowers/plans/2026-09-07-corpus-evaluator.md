# Corpus evaluator (Task 12) — implementation plan

> For agentic workers: TDD per task; RED observed then GREEN; no literal
> code in this plan (house rule). Spec: `docs/superpowers/specs/2026-09-07-corpus-evaluator-design.md`.

**Goal:** a selectable `eval=corpus` MCTS evaluator built from the spec's six
evidenced terms, proven against the score-only evaluator by a seat-balanced
A/B with a mirror control.

**Global constraints:** no core-header edits; `mcts_agent.h` may change (its
constructor signature gains the evaluator choice; it is included only by
`main.cpp` and tests — small rebuild). Existing command lines unchanged
(`eval` defaults to `score`). No card-name logic. Commit trailers as on this
branch. Baseline before Task 12: whatever the suite count is after Task 11.

### Task 12.1 — the pure evaluator function + unit tests
Files: create `src/agents/corpus_evaluator.h/.cpp` (free function
`corpusEvaluate(const GameState&)` and the weight constants, documented
with each term's evidence line from the spec); create
`tests/test_corpus_evaluator.cpp` (spec tests 1–9).
Steps: tests first (RED: symbol missing) → implement term by term, one
test at a time → full suite → commit `Corpus evaluator: pure function +
unit tests`.

### Task 12.2 — wire the selector
Files: `src/agents/mcts_agent.h/.cpp` (an `EvaluatorKind {Score, Corpus}`
constructor parameter; the corpus `Evaluator` wrapper delegating to
`corpusEvaluate`, terminal short-circuit preserved), `src/main.cpp`
(`AgentSpec::eval`, `parseAgentSpec` `eval=` handling with validation,
`buildAgent` passes it, help text updated).
Steps: spec-parsing tests (spec test 10; put them where `parseAgentSpec`
is testable — if it is not linkable from tests, extract it to
`src/agents/agent_spec.h/.cpp` first, test-first) → wire → full suite →
commit `Corpus evaluator: eval=score|corpus agent option`.

### Task 12.3 — the A/B
Build `riftbound`; run the three batches from the spec's Proof section
(`--games 100 --threads 4`, sims=20, each seat), capture stdout summaries,
compute Wilson intervals, write
`docs/superpowers/smoke/2026-09-08-corpus-ab.md` with the honest verdict
rule, commit `Corpus evaluator: A/B results`.
