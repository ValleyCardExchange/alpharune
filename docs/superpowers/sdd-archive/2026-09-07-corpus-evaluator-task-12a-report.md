# Task 12a — battlefield use-after-free fix + corpus A/B notes

Branch: `kennen-tyler-deck`. Start HEAD: `90f39c0` (verified; one
untracked file, `docs/superpowers/smoke/2026-09-08-corpus-ab.md`, the
draft finished here). Root-cause analysis followed:
`.superpowers/sdd/2026-09-07-corpus-evaluator/crash-analysis.md`.

| Commit | Subject | Files |
|---|---|---|
| `5d8d7e7` | Engine: stable battlefield references (deque) — fixes use-after-free on Baron Pit spawn | `src/core/game_state.h`, `src/engine/game_engine.cpp`, `tests/test_battlefield_stability.cpp` |
| `f97b678` | Corpus evaluator: A/B results | `docs/superpowers/smoke/2026-09-08-corpus-ab.md` |

**Note on the base commit.** The task named `90f39c0` as HEAD, and it was
when this task started. Partway through, another actor working in the
same checkout committed `220b215` ("Archive the SDD ledger and task
reports"), 15 docs-only files under `docs/superpowers/reports/`. My two
commits sit on top of it. It touches no source, no test and no file this
task edited, so nothing had to be reconciled — but `git diff 90f39c0
HEAD` shows those 15 files as well, and that is why. `git show --stat`
on each of my two commits shows exactly the four files above and nothing
else; no `git add -A` was used.

---

## Part A — the use-after-free

### What was wrong (confirmed against the tree, not taken on faith)

`GameState::battlefields` was `std::vector<BattlefieldState>`
(`src/core/game_state.h:531` pre-fix). `GameEngine::setupBattlefields`
(`src/engine/game_engine.cpp:400-434`) builds it with exactly two
`push_back`s and no `reserve`, so `size() == capacity() == 2` for the
whole game and the next append is a guaranteed reallocation.

The only post-setup append is `EffectExecutor::addBattlefieldToken`
(`src/engine/effect_executor.cpp:853`), whose only caller is
`BaronNashor::onPlay` (`src/cards/units/0709_baron_nashor.cpp:44`).
`1 Baron Nashor` is line 8 of `decks/kennen_tyler.txt` — verified.

Three engine sites held a `BattlefieldState&` across card resolution and
then read and wrote it: the staged-battlefield loop in
`advanceMainPhase`, `runShowdown`, and `runCombat`. Reading the code for
the fix surfaced **two more** of the same shape, both reached from
`runCombat`, and both were fixed:

- `combatDamageStep` — read `*bf.defender` for the second `runOneSide`
  call, i.e. *after* the first one had resolved damage assignments.
- `combatResolutionStep` — held `bf` across `processLethalDamage()` and
  the `moveUnit` recall loop, then wrote `combat_in_progress`,
  `combat_phase`, `is_contested`, `attacker`, `defender`, `controller`
  through it.

### The fix

1. **`GameState::battlefields` is now `std::deque<BattlefieldState>`.**
   Every use of `battlefields` across `src/` and `tests/` was
   enumerated (423 hits over 100+ files) and classified. Only
   `operator[]`, `push_back`, `size()`, `back()`, `front()`, `empty()`
   and range-`for` are used. There is **no** `.data()`, **no**
   `reserve`, **no** pointer arithmetic, and no function anywhere takes
   or returns a `std::vector<BattlefieldState>` — the type name appears
   exactly once in the tree, at the declaration. The one address-of use,
   `&state_.battlefields[bf_id]` in the Brush aura
   (`game_engine.cpp` ~:4795), is a pointer to an element used
   immediately, which deque supports. `DeckList::battlefields` is a
   different member (a `std::vector<CardDefId>`) and is untouched.
   Nothing was blocked; no reconciliation problem arose.
2. **The five sites no longer hold a reference across resolution.** The
   staged-battlefield loop snapshots the ids and re-fetches before every
   read and every write (via a local `find_bf` that returns `nullptr`
   rather than throwing, so a future battlefield removal cannot throw
   out of the main-phase driver). `runShowdown`, `runCombat`,
   `combatDamageStep` and `combatResolutionStep` read the seats into
   locals and re-fetch by id after each nested call that can resolve a
   card. `getBattlefield` carries the lifetime contract as a comment.

Behaviour is otherwise unchanged. The one deliberate difference: a
battlefield appended *during* a pass of the staged loop is handled on
the next call into `advanceMainPhase` rather than in the same pass —
which is what the range-`for` did too (it had already read its end
iterator), only now it is defined behaviour instead of luck.

### RED evidence (pre-fix binary, `90f39c0` tree)

`tests/test_battlefield_stability.cpp` was written and run against the
UNMODIFIED engine first.

```
$ ninja -C build riftbound_tests            # 21s, only the new TU
$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='*BattlefieldReferencesSurvive*'
[ RUN      ] CardTestFixture.BattlefieldReferencesSurviveTokenAppend
tests/test_battlefield_stability.cpp:70: Failure
Expected equality of these values:
  bf0_before
    Which is: 0x56060040e650
  &state.battlefields[0]
    Which is: 0x56060040e9a0
Appending a battlefield token relocated battlefield 0. Every
BattlefieldState& held across card resolution (the staged-battlefield
loop, runShowdown, runCombat) is now dangling.
tests/test_battlefield_stability.cpp:74: Failure
  bf1_before   Which is: 0x56060040e6a8
  &state.battlefields[1]   Which is: 0x56060040e9f8
[  FAILED  ] CardTestFixture.BattlefieldReferencesSurviveTokenAppend (3 ms)
[ RUN      ] CardTestFixture.BattlefieldReferencesSurviveRepeatedAppends
  ... Battlefield 0 moved on append #0 ... (and on #1, #3, #5)
[  FAILED  ] CardTestFixture.BattlefieldReferencesSurviveRepeatedAppends (1 ms)
 2 FAILED TESTS
```

The failure is deterministic, not allocator-luck: the fixture's two
appends leave a vector at `size == capacity == 2`, so the third append
must move every element. The tests assert on element **addresses**
(well-defined) rather than dereferencing freed memory.

### GREEN evidence (post-fix)

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests \
    --gtest_filter='*BattlefieldReferencesSurvive*:BaronPitEngineTest.*'
[       OK ] CardTestFixture.BattlefieldReferencesSurviveTokenAppend (2 ms)
[       OK ] CardTestFixture.BattlefieldReferencesSurviveRepeatedAppends (1 ms)
[       OK ] BaronPitEngineTest.BaronPitSpawnsThenCombatRunsOnThreeBattlefields (1 ms)
[  PASSED  ] 3 tests.
```

### Test (b) — honest status: written, GREEN both sides, NOT a RED

`BaronPitSpawnsThenCombatRunsOnThreeBattlefields` drives the real
engine: Baron Nashor is played through `executePlayCard` →
`Card::onPlay` → `addBattlefieldToken` (asserting the Pit spawns as BF 2
with `accepts_any_inbound`, and that Baron enters it), and the turn then
runs a real combat at BF 0 via `testHook_runCombat` with three
battlefields on the board — asserting nothing throws, all three ids
still resolve, the Pit is still there, and `combatResolutionStep`'s
closing writes landed on the live BF 0.

**It does not go red on the pre-fix build, and I could not make it do
so honestly.** Verified empirically, not assumed: it was built and run
against the unmodified engine and passed. The reason is ordering — the
append happens before the combat, so the references `runCombat` takes
are taken after the reallocation and are valid. The fatal ordering needs
a battlefield-token spawn from *inside* showdown/combat resolution.
Baron Nashor is a plain 10-cost unit with no Ambush and no Flow, so no
legal line plays him in a showdown; a synthetic token-spawning Action
spell plus a scripted agent could force it, but observing the corruption
without ASan would still depend on what reuses the block. Per the task's
allowance, (a) is the deterministic guard and (b) is recorded as an
end-to-end consistency guard, with that limitation written into the test
file itself so nobody later mistakes it for a regression test.

(One assertion in (b) was wrong on the first run — it expected
`combat_phase == ResolutionStep` after `runCombat`, but
`combatResolutionStep` clears the phase to `None` on the way out. The
assertion was corrected to the fields that combat actually leaves
behind; the test was not weakened to pass.)

### Suite, gate, binary

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests
[==========] 1199 tests from 127 test suites ran. (1977 ms total)
[  PASSED  ] 1199 tests.
  YOU HAVE 1 DISABLED TEST
```

1199 = 1196 baseline + 3 new. The 1 disabled test is the pre-existing
one.

```
$ python3 scripts/card_coverage.py | tail -2
TOTAL                                          792
INCOMPLETE (stub+metadata-only): 0   engine-gap flagged: 50
```

`game_state.h` is a core header, so this was a full rebuild: 889 targets,
~50 min on 4 cores. `game_engine.cpp` was edited while that build was in
flight, so `ninja` was re-run to completion afterwards — it rebuilt
`game_engine.cpp.o` and relinked both binaries, so neither binary
contains a stale object.

### The discriminating batch

Segfaulted 3/3 before the fix. Post-fix, run twice:

```
$ ./build/riftbound --agent1 mcts:sims=20 --agent2 mcts:sims=20,eval=corpus \
    --deck1 decks/kennen_tyler.txt --deck2 decks/rengar_test.txt \
    --games 20 --threads 4 --seed 1040
```

| Run | Exit | P1 (`score`) | P2 (`corpus`) | Draws | Avg turns | Avg decisions |
|---|---|---|---|---|---|---|
| 1 | 0 | 9 | 11 | 0 | 14 | 585 |
| 2 | 0 | 9 | 11 | 0 | 14 | 585 |

Clean both times, and identical per-game results.

---

## Part B — the mirror control on the fixed engine

Only the mirror-control cell was re-run, per the task, on the post-fix
binary at `--threads 4` (100 games per seat, seed 1000, sims 20). Both
cells completed clean — no crash at the thread count that used to be
fatal.

| Cell | Corpus seat | Pre-fix corpus wins | Post-fix corpus wins | Agree? | Pre-fix avg decisions | Post-fix avg decisions |
|---|---|---|---|---|---|---|
| Mirror A | P1 | 47 / 100 | 47 / 100 | yes | 701 | 699 |
| Mirror B | P2 | 45 / 100 | 45 / 100 | yes | 697 | 696 |
| **Total** | — | 92 / 200 = 46.0% | **92 / 200 = 46.0%** | **yes** | — | — |

Logs: `abfx-mirror-corpusP1.log`, `abfx-mirror-corpusP2.log`, both
`EXIT=0` (gitignored, like the pre-fix `ab*-*.log` set).

**They agree exactly on win counts, so the verdict does not move.**
Wilson 95% on 92/200: [39.2%, 52.9%] — includes 50%.

Worth stating precisely, because "identical" invites the wrong reading:
the average decision counts did shift slightly (701 → 699, 697 → 696).
That is the fix being visible — writes such as
`bf.showdown_staged = false` and `bf.combat_phase = …` now land on the
live battlefield instead of on freed memory, so a few games take a
marginally different path. None of them changed hands at this sample.
The agreement therefore says the pre-fix numbers happened to be benign,
not that the defect was harmless in general.

### The A/B verdict as written into the smoke doc

**NOT DISTINGUISHABLE at this sample; point estimate slightly below.**
The mirror control is 46.0% (92/200), Wilson 95% [39.2%, 52.9%], which
includes 50%; the point estimate is below it, not above. The full table
(baseline 85/200 = 42.5% [35.9%, 49.4%]; treatment 80/200 = 40.0%
[33.5%, 46.9%]; mirror 92/200 = 46.0% [39.2%, 52.9%]) is in
`docs/superpowers/smoke/2026-09-08-corpus-ab.md`, along with the
standing framing sentence: MCTS at sims=20 plays legal, not expert,
Riftbound, and the evaluator plays the book — it does not learn.

The smoke doc's old line calling the corpus evaluator "unusable in
multithreaded batch mode" is gone, replaced by the root cause, the fix,
and the passing discriminating batch. Its "suspected data race"
hypothesis section is likewise replaced: it was wrong, and saying so is
the point of keeping the doc.


---

## Files

| File | Change |
|---|---|
| `src/core/game_state.h` | `battlefields` → `std::deque<BattlefieldState>`, `#include <deque>`, lifetime rationale as a comment |
| `src/engine/game_engine.cpp` | staged-battlefield loop, `runShowdown`, `runCombat`, `combatDamageStep`, `combatResolutionStep` re-fetch by id; `getBattlefield` lifetime contract comment |
| `tests/test_battlefield_stability.cpp` | **new** — 3 tests (2 deterministic RED→GREEN guards, 1 Baron end-to-end) |
| `docs/superpowers/smoke/2026-09-08-corpus-ab.md` | finished: root cause + fix replace the "unusable in multithreaded batch mode" line; full results table; post-fix mirror re-run; verdict |
| `.superpowers/sdd/2026-09-07-corpus-evaluator/task-12a-report.md` | **new** — this file |

Nothing else was touched. No `git add -A` was used.

---

## Concerns

1. **The `runShowdown` / `runCombat` re-fetches have no test that goes
   red without them.** With the deque in place they are belt and braces,
   and the deque alone makes the guard tests pass. A test that would
   actually pin them needs a battlefield-token spawn from *inside* a
   live showdown, which no shipped card can do (Baron is the only
   `addBattlefieldToken` caller and he is a plain main-phase unit).
   If the container is ever changed back, tests 1 and 2 catch it — but
   nothing catches someone re-introducing a held reference at a new
   site. The `getBattlefield` contract comment is the only defence
   there.

2. **`GameState::getObject` is still guarded only by `assert`**
   (`src/core/game_state.h`, ~:589-599) and the shipped binary is
   `Release` (`-O3 -DNDEBUG`), so a stale id dereferences `objects.end()`
   instead of failing with a name. Fix C from the crash analysis — make
   it throw unconditionally — was NOT done here: it is out of this
   task's scope and would change behaviour in a way that deserves its
   own change and its own test. `getBattlefield` already throws, which
   is the only reason this crash produced a message rather than silent
   garbage. Recommend doing it next.

3. **No ASan run.** The crash analysis proposed a `build-asan` tree as
   the definitive confirmation, and it would also be the way to prove
   the re-fetches in concern 1. Not done: a sanitizer build is a second
   full compile of an 889-target tree, and the deterministic
   address-stability test plus the discriminating batch going from 3/3
   SIGSEGV to 2/2 clean was judged sufficient evidence for this task. An
   ASan CI job remains the right long-term answer for this defect class.

4. **The engine has no CI.** `CLAUDE.md` pre-ratifies a `tests.yml` port
   "from the first test"; the suite is 1199 tests and there is still no
   workflow running it. This defect is the case for it, and for the ASan
   job alongside.

5. **`EffectExecutor::replaceBattlefieldWithToken` and
   `GameEngine::replaceBattlefield` (CR 438) mutate a slot in place**
   rather than appending, so they were not part of this defect — but
   they are the other way a `BattlefieldState` can change identity under
   a held reference. Not audited here.

6. **Only the mirror cell was re-run post-fix**, as instructed. The
   baseline and treatment cells in the results table are still pre-fix
   numbers. Given that the mirror agreed exactly, that is very likely
   fine, but it is an assumption, not a measurement.

