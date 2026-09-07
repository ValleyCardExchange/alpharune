# Task 12 — corpus heuristic evaluator for MCTS + A/B proof

Branch: `kennen-tyler-deck`. Start HEAD: `c561d2e` (clean tree, verified).
Spec: `docs/superpowers/specs/2026-09-07-corpus-evaluator-design.md`.
Plan: `docs/superpowers/plans/2026-09-07-corpus-evaluator.md`.

Baseline before any change:

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests 2>&1 | tail -5
[==========] 1143 tests from 123 test suites ran. (1991 ms total)
[  PASSED  ] 1143 tests.

  YOU HAVE 1 DISABLED TEST
```

Commits (three, as planned):

| Commit | Subject |
|---|---|
| `d25584c` | Corpus evaluator: pure function + unit tests |
| `90f39c0` | Corpus evaluator: eval=score\|corpus agent option |
| _(pending)_ | Corpus evaluator: A/B results |

---

## Task 12.1 — the pure function + unit tests

### Files

- **new** `src/agents/corpus_evaluator.h` — the six weights and caps as
  named `constexpr`s, each carrying its corpus evidence line from the
  spec, plus the EXCLUDED list with reasons; declares
  `std::pair<double,double> corpusEvaluate(const GameState&)`.
- **new** `src/agents/corpus_evaluator.cpp` — the implementation.
- **new** `tests/test_corpus_evaluator.cpp` — 37 tests.
- **mod** `CMakeLists.txt` — `src/agents/corpus_evaluator.cpp` added to
  the `riftbound_core` source list (that list is explicit, not globbed —
  only `src/cards/**` is globbed; test sources are globbed, so the new
  test file needed no CMake change).

### Why the function lives where it does

`src/agents/mcts_agent.cpp` is compiled into the `riftbound` **executable**
(`CMakeLists.txt` ~:305-320) and not into `riftbound_core` (~:251-278),
because it needs the OpenSpiel `riftbound_game.cpp` / `riftbound_state.cpp`
translation units whose `OPEN_SPIEL_REGISTER_GAME` static initialisers the
linker strips out of a static library. So nothing in `mcts_agent.cpp` is
linkable from `riftbound_tests`. Putting the arithmetic in a free function
that depends only on `core/` headers, compiled into `riftbound_core`, is
what makes every term testable; the OpenSpiel `Evaluator` wrapper stays in
`mcts_agent.cpp` as a thin delegation.

### TDD evidence

**RED 1 — symbol missing.** Test file written first, before any source:

```
$ cmake --build build --target riftbound_tests -j4
FAILED: CMakeFiles/riftbound_tests.dir/tests/test_corpus_evaluator.cpp.o
tests/test_corpus_evaluator.cpp:16:10: fatal error: agents/corpus_evaluator.h: No such file or directory
```

**RED 2 — wrong values.** Header + a stub `corpusEvaluate` returning
`{0.0, 0.0}`, wired into `riftbound_core`:

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='CorpusEvaluator.*'
[==========] 33 tests from 1 test suite ran.
[  PASSED  ] 11 tests.
[  FAILED  ] 22 tests, listed below:
[  FAILED  ] CorpusEvaluator.ScoreTermIsTheDifferenceOverVictoryScore
[  FAILED  ] CorpusEvaluator.ScoreTermIsSignedTowardTheLeader
[  FAILED  ] CorpusEvaluator.ScoreTermRespectsANonDefaultVictoryScore
[  FAILED  ] CorpusEvaluator.UncontestedBattlefieldIsHalfAPointOfTempo
[  FAILED  ] CorpusEvaluator.OpponentUncontestedBattlefieldIsNegative
[  FAILED  ] CorpusEvaluator.BattlefieldTermSumsAcrossBattlefields
[  FAILED  ] CorpusEvaluator.BoardWidthDiminishesAndCapsAtFourUnits
[  FAILED  ] CorpusEvaluator.BoardWidthCountsUnitsAtBattlefieldsToo
[  FAILED  ] CorpusEvaluator.BoardWidthIsSymmetricForTheOpponent
[  FAILED  ] CorpusEvaluator.HeldReactionSpellsCapAtTwo
[  FAILED  ] CorpusEvaluator.OneHeldReactionSpellCountsOnce
[  FAILED  ] CorpusEvaluator.HeldInteractionIsSymmetric
[  FAILED  ] CorpusEvaluator.PrintedFlowSpellInTrashCounts
[  FAILED  ] CorpusEvaluator.GrantedFlowStampedThisTurnCounts
[  FAILED  ] CorpusEvaluator.TrashResourcesCapAtThree
[  FAILED  ] CorpusEvaluator.TrashResourcesAreSymmetric
[  FAILED  ] CorpusEvaluator.EmpoweredLegendCounts
[  FAILED  ] CorpusEvaluator.OpponentEmpoweredLegendIsNegative
[  FAILED  ] CorpusEvaluator.ValueIsAntisymmetricAcrossTerms
[  FAILED  ] CorpusEvaluator.WildlyLopsidedStateStaysInRange
[  FAILED  ] CorpusEvaluator.WildlyLopsidedStateClampsTheOtherWayToo
[  FAILED  ] CorpusEvaluator.OnePointOfScoreOutweighsEveryOtherTermCombined
```

The 11 that already passed are exactly the ones asserting `0.0` — the
"symmetric state", "unempowered", "non-Flow", "Action-only", "granted
flow stamped last turn", "uncontrolled battlefield" cases. That split is
itself useful: it proves those tests were not passing for the right
reason yet, and they are re-checked green after each term lands.

**GREEN, term by term.** Each term implemented and its filter run:

| Term | Filter | Result |
|---|---|---|
| 1 score | `CorpusEvaluator.Score*` + clamp + zero-victory | 5 passed |
| 2 point tempo | `*Battlefield*` | all battlefield tests passed (only the not-yet-implemented board-width test still red) |
| 3 board width | `CorpusEvaluator.BoardWidth*` | 3 passed |
| 4+5 held interaction, trash | `*Held*:*Trash*:*Flow*:*Action*` | 105 passed |
| 6 empowered legend | `CorpusEvaluator.*` | 32/33 passed |

### Finding at term 6 — the score term is NOT dominant over the SUM

The one test still red after all six terms was my own extra guard,
`OnePointOfScoreOutweighsEveryOtherTermCombined`:

```
tests/test_corpus_evaluator.cpp:489: Failure
Expected: (v.first) > (0.0), actual: -0.16562500000000002 vs 0
```

That guard encoded a stronger claim than the spec actually ratifies. The
spec says the score term is "dominant so the bot still chases points";
against the ratified weights that is true **term by term** (weight 1.0 vs
fractions) but not **of the sum**. On the standard two-battlefield board,
every proxy term maxed for one side totals

```
2*(0.5/8) + 2.5*(0.25/8) + 2*(0.15/8) + 3*(0.1/8) + (0.1/8) = 0.290625
```

i.e. **2.325 points of score-equivalent** at `victory_score` 8.

I did not retune, and I did not weaken or delete the guard to make it
pass. I replaced it with characterization tests that pin the measured
number and its consequences:

- `MaxedProxyTermsAreWorthExactlyTwoPointThreeTwoFivePoints` — the swing
  is exactly `-0.290625`, i.e. 2.325 points.
- `TwoPointLeadDoesNotOutweighAMaximalOpposingBoard` — red if the sum
  ever shrinks below 2 points.
- `ThreePointLeadDoesOutweighAMaximalOpposingBoard`.
- `ScoreStillDominatesTermByTerm` — one point beats any single maxed term.
- `TwoOpposingBattlefieldsExactlyTieOnePointOfScore` — the most
  load-bearing consequence of `w_bf = 0.5/victory`: "the opponent holds
  both battlefields" reads as **exactly** one point of score. That
  equality follows from the spec's own "one held battlefield ≈ half a
  point" and is now pinned rather than accidental.

Carried to the Concerns section below. Retuning is the owner's call.

### Final 12.1 state

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='CorpusEvaluator.*'
[==========] 37 tests from 1 test suite ran. (0 ms total)
[  PASSED  ] 37 tests.

$ RIFTBOUND_ROOT=. ./build/riftbound_tests 2>&1 | tail -5
[==========] 1180 tests from 124 test suites ran. (2007 ms total)
[  PASSED  ] 1180 tests.

  YOU HAVE 1 DISABLED TEST

$ python3 scripts/card_coverage.py | tail -3
TOTAL                                          792

INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```

1143 → 1180 (+37). Coverage gate unchanged.

### Spec test coverage map

| Spec test | Covered by |
|---|---|
| 1 empty symmetric → {0,0} | `EmptySymmetricStateIsZero`, `SymmetricNonEmptyStateIsZero` |
| 2 score 4-0 / victory 8 → 0.5 | `ScoreTermIsTheDifferenceOverVictoryScore` (+2 more) |
| 3 battlefield ±0.5/8, contested nets 0 | `UncontestedBattlefieldIsHalfAPointOfTempo`, `OpponentUncontestedBattlefieldIsNegative`, `ContestedBattlefieldSplitsTheTempoAndNetsZero`, `UncontrolledBattlefieldContributesNothing`, `BattlefieldTermSumsAcrossBattlefields` |
| 4 board 1/2/4/6 → 0.25/8 × {1,1.75,2.5,2.5} | `BoardWidthDiminishesAndCapsAtFourUnits` (table-driven, all four cases) + 2 more |
| 5 3 Reactions capped at 2; Action-only 0 | `HeldReactionSpellsCapAtTwo`, `ActionOnlySpellsInHandCountZero` (+2) |
| 6 printed Flow / granted this turn / granted last turn / non-Flow / cap 3 | 5 tests, one per clause |
| 7 empowered legend +0.1/8, else 0 | `EmpoweredLegendCounts`, `UnempoweredLegendCountsZero`, `OpponentEmpoweredLegendIsNegative`, `MissingLegendZoneIsSafe` |
| 8 antisymmetry | `ValueIsAntisymmetricAcrossTerms`, `MirroredStateFlipsTheSign` |
| 9 clamp | `WildlyLopsidedStateStaysInRange`, `…ClampsTheOtherWayToo`, `ZeroVictoryScoreDoesNotDivideByZero` |
| 10 spec parsing | Task 12.2 below |

Note on antisymmetry: the implementation accumulates the non-score terms
into per-side totals and returns `{v, -v}`, so `second == -first` holds
by construction rather than by coincidence.

Constraints honoured: no card-name logic anywhere in the evaluator; no
information read beyond the existing evaluator's perfect-information
contract; no edits to `src/core/*`.

---

## Task 12.2 — the `eval=score|corpus` selector

### Files

- **new** `src/agents/agent_spec.h/.cpp` — `EvaluatorKind {Score, Corpus}`,
  `toString(EvaluatorKind)`, `AgentSpec`, `parseAgentSpec`. Added to the
  `riftbound_core` source list.
- **new** `tests/test_agent_spec.cpp` — 16 tests.
- **mod** `src/agents/mcts_agent.h` — includes `agent_spec.h`; both
  `MctsAgent` and `IsMctsAgent` constructors gain
  `EvaluatorKind eval = EvaluatorKind::Score`.
- **mod** `src/agents/mcts_agent.cpp` — `RiftboundEvaluatorBase` +
  two subclasses + `makeEvaluator`; stale comment corrected.
- **mod** `src/main.cpp` — local `AgentSpec` / `parseAgentSpec` deleted,
  `agents/agent_spec.h` included, `buildAgent` forwards `spec.eval`, help
  text updated in three places (file header, agent-spec block,
  `--agent1` / `--agent2` option strings).

### The extraction

`parseAgentSpec` lived inline in `src/main.cpp`, which the test binary
cannot link, so it had no unit tests at all. It was moved verbatim into
`riftbound_core` first (behaviour-preserving), then extended with `eval`.
Two pre-existing quirks are now pinned as **characterization** tests with
comments saying so, rather than silently fixed:

- `mcts` with no colon parses to `sims=0` instead of throwing — the
  `sims > 0` guard sits inside the "has a colon" branch. Tightening it
  would reject command lines that work today; that is a separate change.
- Unrecognised `key=value` pairs are ignored rather than rejected.

`eval` is validated wherever it appears, on any agent kind — silently
ignoring an `eval=` typo would let an A/B batch quietly measure the wrong
thing. The error is the same `std::runtime_error` shape a bad `sims` gets.

### The evaluator refactor

`Prior()` (the strategic action bias) and the terminal short-circuit to
`Returns()` moved to a shared `RiftboundEvaluatorBase`; the two concrete
evaluators supply only `evaluateEngineState(const GameState&)`. `Prior()`
itself is byte-for-byte unchanged — it was explicitly out of scope.

The stale comment block above the old evaluator (it still advertised a
"total on-board might" secondary term and a "hand size advantage"
tertiary term, both removed long ago) is corrected. The empirical numbers
it recorded — multi-signal heuristic dropped sims=5 from 37% to 20% vs
random — are kept, restated as the bar `eval=corpus` has to clear.

### TDD evidence

**RED — symbol missing:**

```
$ cmake --build build --target riftbound_tests -j4
tests/test_agent_spec.cpp:25:10: fatal error: agents/agent_spec.h: No such file or directory
```

**GREEN:**

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='AgentSpecParse.*:EvaluatorKindToString.*'
[==========] 16 tests from 2 test suites ran. (0 ms total)
[  PASSED  ] 16 tests.
```

**Proof the new guard can fail** (house rule: a new test must be able to
fail). The `eval` validation branch was temporarily disabled
(`else {` → `else if (false) {`), rebuilt, and the guards went red:

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='AgentSpecParse.UnknownEval*:AgentSpecParse.EvalError*'
[  FAILED  ] AgentSpecParse.UnknownEvalValueThrows
[  FAILED  ] AgentSpecParse.UnknownEvalValueThrowsRegardlessOfAgentKind
[  FAILED  ] AgentSpecParse.EvalErrorMentionsTheOfferedValues
[==========] 3 tests from 1 test suite ran.
[  PASSED  ] 0 tests.
```

Reverted, rebuilt, back to 15/15 green on `AgentSpecParse.*`.

### Spec test 10, the half unit tests cannot reach

`buildAgent` is in `main.cpp` and `MctsAgent` is in the `riftbound`
executable, so "the spec string actually selects the corpus evaluator
inside the search" is not unit-testable. Verified at the binary instead:

```
$ ./build/riftbound --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt \
    --agent1 mcts:sims=5,eval=bogus --agent2 random --games 1
Error: Unknown eval 'bogus' in agent spec 'mcts:sims=5,eval=bogus'. Expected one of: score, corpus.
```

```
$ for e in score corpus; do ./build/riftbound --deck1 decks/kennen_tyler.txt \
    --deck2 decks/rengar_test.txt --agent1 mcts:sims=20,eval=$e \
    --agent2 mcts:sims=20 --seed 4242 --games 1; done
=== eval=score ===
P1: Heart of the Tempest / Kennen, Storm of Shuriken  (mcts:sims=20,eval=score)
Game over: winner=1 | reason=P1 reached 8 points | turns=14
=== eval=corpus ===
P1: Heart of the Tempest / Kennen, Storm of Shuriken  (mcts:sims=20,eval=corpus)
Game over: winner=1 | reason=P1 reached 8 points | turns=20
```

Same seed, same opponent, same deck — 14 turns vs 20. The selector
reaches the search, not just the parser.

### Final 12.2 state

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests 2>&1 | tail -5
[==========] 1196 tests from 126 test suites ran. (2007 ms total)
[  PASSED  ] 1196 tests.

  YOU HAVE 1 DISABLED TEST

$ python3 scripts/card_coverage.py | tail -3
TOTAL                                          792

INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```

1180 → 1196 (+16). Coverage gate unchanged. Backwards compatible: `eval`
defaults to `Score`, so every pre-existing command line and test picks
the historical evaluator.

---

## Task 12.3 — the A/B

_(pending — filled in when the batches finish)_
