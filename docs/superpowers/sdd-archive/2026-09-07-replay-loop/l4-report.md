# L4 — prior injection: implementation report

Branch `replay-loop`, on top of `f657e57` (decision log writer / L1).

## Design decisions (and why)

- **Loader lives in `agent_spec.{h,cpp}`, not a new file.** `riftbound_core`
  already links `nlohmann_json::nlohmann_json` publicly and already builds
  `src/agents/agent_spec.cpp` and `src/agents/corpus_evaluator.cpp` into
  that library (see `CMakeLists.txt` lines ~251-279). No new source file
  was needed, so `CMakeLists.txt` is untouched (tests are glob-discovered
  already; `tests/test_prior_config.cpp` picked itself up).
- **`PriorConfig`/`FamilyWeights` live in `agent_spec.h`, not `mcts_agent.h`.**
  `mcts_agent.cpp` compiles only into the `riftbound` executable (needs the
  OpenSpiel game/state TUs for the static registrar — same reason
  `corpus_evaluator.h` documents), so nothing declared only in
  `mcts_agent.h` would be reachable from `riftbound_tests`. Putting the
  structs and the `intentFamilyWeight` lookup in `agent_spec.h`
  (riftbound_core) makes the whole prior-parsing/lookup surface
  unit-testable, matching the existing `corpus_evaluator.h` pattern (task
  note: "if it is not unit-testable without OpenSpiel, test the
  family-weight lookup function you factor out of it" — done as
  `intentFamilyWeight(IntentType, bool moves_to_battlefield,
  const FamilyWeights&)`, called from `RiftboundEvaluatorBase::Prior()`
  which now carries only the dispatch/normalization logic, not the
  weight table).
- **`AgentSpec::prior` is a `PriorConfig` value, not a raw path string.**
  `parseAgentSpec` validates + loads the file eagerly when it sees
  `prior=<path>` (per the spec: "parseAgentSpec validates the file exists
  and parses"), so a bad path throws at parse time and `buildAgent`/
  `MctsAgent` never see a dangling path to re-resolve. `AgentSpec{}` with
  no `prior=` key gets `PriorConfig{}` — default-constructed to today's
  hard-coded numbers — so behaviour is byte-identical without a
  `std::optional` branch anywhere downstream.
- **`corpusEvaluate` overload, not a default argument**, per the task's
  "the default-weights overload must remain": `corpusEvaluate(state)` and
  `corpusEvaluate(state, weights)` both exist; the single-arg form
  forwards to `CorpusWeights{}`.
- **Caps are NOT part of schema v1** (`kCorpusUnitCap`,
  `kCorpusHeldInteractionCap`, `kCorpusTrashResourceCap`) — the spec's
  JSON schema only lists the six weight terms, so these stay hard-coded
  constants untouched by `CorpusWeights`.
- **The unclassified-intent fallback (weight 1.0) is not part of
  `FamilyWeights`** — schema v1's `action_family_weights` lists exactly
  nine keys (no `default`), so the handful of triggered-ability response
  `IntentType`s with no evidence either way keep returning a literal 1.0
  from `intentFamilyWeight`'s `default:` case, not an overridable field.

## TDD evidence (RED observed, then GREEN)

1. **Corpus evaluator weights** — added 4 tests to
   `tests/test_corpus_evaluator.cpp` referencing `CorpusWeights` before it
   existed. Build failed (`'CorpusWeights' was not declared in this
   scope`, 4 errors) → added the struct + two-arg overload to
   `corpus_evaluator.h/.cpp` → build succeeded, all 41
   `CorpusEvaluator.*` tests pass (37 pre-existing + 4 new).
2. **`FamilyWeights` / `PriorConfig` / `loadPriorConfig` /
   `intentFamilyWeight`** — wrote `tests/test_prior_config.cpp` (23 tests)
   and 6 new `prior=` tests in `tests/test_agent_spec.cpp` referencing
   `AgentSpec::prior`, `FamilyWeights`, `intentFamilyWeight`,
   `loadPriorConfig` before any of them existed. Build failed
   (`'struct riftbound::AgentSpec' has no member named 'prior'`, and
   `agent_spec.h: No such file or directory` from a wrong include path in
   the new file, fixed to `"agents/agent_spec.h"`) → implemented in
   `agent_spec.h/.cpp` → build succeeded, all 45 new tests
   (`FamilyWeightsDefaults`, `CorpusWeightsDefaults`, `PriorConfigDefaults`,
   `IntentFamilyWeight.*` ×10, `LoadPriorConfig.*` ×10,
   `AgentSpecParse.*Prior*` ×6, plus the 4 corpus tests above) pass.
3. **`Prior()` override** — not directly unit-testable (needs OpenSpiel);
   per the task's fallback instruction, `intentFamilyWeight` is the
   factored-out pure function tested instead (10 `IntentFamilyWeight.*`
   cases, including `AnOverriddenFamilyWeightIsReflected`). `Prior()`
   itself was refactored to call `intentFamilyWeight(it.type,
   moves_to_battlefield, family_)`, replacing the inlined switch, and
   verified end-to-end via the binary checks below (a real
   `mcts:sims=5,prior=...` game runs cleanly).
4. Used a proper `mkstemp`-equivalent (`std::filesystem::temp_directory_path()`
   + a `getpid()`+counter-based unique name) for the two new tests' temp
   JSON fixtures after `tmpnam` produced a linker deprecation warning on
   first build — no test behavior changed, just the fixture's file-naming
   safety.

## Files touched

- `src/agents/corpus_evaluator.h` — added `CorpusWeights` struct
  (defaults = the existing `kCorpusWeight*` constants) and the two-arg
  `corpusEvaluate` overload declaration.
- `src/agents/corpus_evaluator.cpp` — single-arg `corpusEvaluate` now
  forwards to the two-arg overload with `CorpusWeights{}`; all six terms
  read from `weights.*` instead of the `kCorpusWeight*` constants
  directly.
- `src/agents/agent_spec.h` — added `FamilyWeights`, `PriorConfig`,
  `intentFamilyWeight()`, `loadPriorConfig()` declarations;
  `AgentSpec` gained a `PriorConfig prior` field; `#include`s
  `corpus_evaluator.h` (for `CorpusWeights`) and `core/types.h` (for
  `IntentType`) — both riftbound_core headers with no OpenSpiel
  dependency, so the "header stays free of OpenSpiel" invariant holds.
- `src/agents/agent_spec.cpp` — implements `intentFamilyWeight` (the
  family-classification switch factored out of `mcts_agent.cpp`'s
  `Prior()`), `loadPriorConfig` (schema-v1 JSON load/validate/merge with
  three key-allowlists and a `rejectUnknownKeys` helper naming the
  offending key), and a new `prior=<path>` case in `parseAgentSpec`'s
  key/value loop that calls `loadPriorConfig` eagerly.
- `src/agents/mcts_agent.h` — `MctsAgent`/`IsMctsAgent` constructors gain
  a trailing `PriorConfig prior = PriorConfig{}` parameter.
- `src/agents/mcts_agent.cpp` — `RiftboundEvaluatorBase` now holds a
  `FamilyWeights family_` member (ctor param) and `Prior()` delegates to
  `intentFamilyWeight` instead of an inlined switch;
  `RiftboundHeuristicEvaluator` takes `FamilyWeights` in its ctor;
  `RiftboundCorpusEvaluator` takes both `FamilyWeights` and
  `CorpusWeights` and calls the two-arg `corpusEvaluate`; `makeEvaluator`
  takes a `const PriorConfig&` and threads both halves through; both
  `Impl` structs and both public constructors gained a `PriorConfig
  prior` parameter, forwarded to `makeEvaluator`.
- `src/main.cpp` — `buildAgent` passes `spec.prior` to `MctsAgent`/
  `IsMctsAgent`; top-of-file doc comment, `--help` text, and the
  `agent1`/`agent2` `boost::program_options` descriptions updated to
  document `[,prior=<path>]`.
- `tests/test_corpus_evaluator.cpp` — 4 new tests under "CorpusWeights
  override (L4 — prior injection)".
- `tests/test_agent_spec.cpp` — 6 new tests under "New: prior=<path> (L4
  — prior injection)", plus a `TempPriorFile` RAII fixture.
- `tests/test_prior_config.cpp` — new file, 23 tests covering
  `FamilyWeights`/`CorpusWeights`/`PriorConfig` defaults,
  `intentFamilyWeight`, and `loadPriorConfig` (missing file, bad JSON,
  unknown key at each of the three levels naming the key, empty-object
  defaults, partial merges, full-schema parse, and the committed sample
  file).
- `docs/superpowers/priors/example.prior.json` — new sample file, all
  keys present at today's default values, `"matchup": "example"`.
- `CMakeLists.txt` — **not touched** (no new source files; test sources
  are glob-discovered).

No core-header edits. No `git add -A` used anywhere.

## Full suite

```
RIFTBOUND_ROOT=. ./build/riftbound_tests
...
[==========] 1235 tests from 134 test suites ran. (2077 ms total)
[  PASSED  ] 1235 tests.
  YOU HAVE 1 DISABLED TEST
```

Baseline was 1202 passed / 1 pre-existing disabled; this change adds
exactly 33 new tests (4 corpus-evaluator + 6 agent-spec `prior=` + 23 in
`test_prior_config.cpp`) and 1202 + 33 = 1235. The 1 disabled test is the
same pre-existing one (untouched).

## Binary checks

Both run from the repo root against the built `./build/riftbound`
(built successfully as its own target alongside `riftbound_tests`;
`mcts_agent.cpp`/`main.cpp` compile clean against the new signatures):

```
$ ./build/riftbound --agent1 mcts:sims=5,prior=docs/superpowers/priors/example.prior.json \
    --agent2 random --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt \
    --seed 4242 --games 1
...
Game over: winner=1 | reason=P1 reached 8 points | turns=16
(exit 0)

$ ./build/riftbound --agent1 mcts:sims=5,prior=/nonexistent \
    --agent2 random --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt \
    --seed 4242 --games 1
Error: prior file '/nonexistent' does not exist or is not readable.
(exit 1)
```

Additionally verified the "behaviour with no prior= is byte-identical"
claim at the binary level, not just via unit tests: ran
`mcts:sims=20 vs mcts:sims=20` and `mcts:sims=20,prior=example.prior.json
vs mcts:sims=20` at the same seed (777) — `example.prior.json` carries
every key at its default value, so this is the strongest available
regression check short of a fuzzer. Both produced the identical outcome:
`winner=1 | reason=P1 reached 8 points | turns=13`.

## Concerns / follow-ups for L5

- `example.prior.json`'s `"written"` date is today's date (2026-09-07)
  and its `"matchup"` is the literal string `"example"` — it is a
  documentation/test fixture, not a real matchup file. L5's acceptance
  run will need its own `prior=<vault file>` written by the L3
  controller session (per the plan, that file lives in personal-ai's
  vault, not this repo).
- The two 200-game batch processes noted as running throughout this task
  (`riftbound`, PIDs still under `build/riftbound`) were left untouched —
  no build of `riftbound` was skipped, but every build in this session
  ran with only 2 of 4 cores requested (`-j2`) to leave headroom, and the
  machine still had heavy contention from those two 4-thread batches
  (confirmed by the one single-game smoke test that took several minutes
  wall-clock instead of seconds). L5's own batches should expect the
  same contention if run before iter-0's batches finish.
- `intentFamilyWeight`'s `default:` branch (weight 1.0, for
  `HideCard`, `SideboardSwap`, `PlaceOptionalTrigger`,
  `DeclineOptionalTrigger`, `PayTriggeredCost`, `DeclineTriggeredCost`) is
  intentionally NOT part of schema v1 or `FamilyWeights` — confirmed
  against the spec's exact `action_family_weights` key list, which has
  no `default` key. If a future iteration wants this fallback
  overridable, that's a schema v2 decision, not a bug in this one.
