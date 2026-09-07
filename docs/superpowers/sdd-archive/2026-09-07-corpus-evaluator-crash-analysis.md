# Crash analysis — `eval=corpus` faults at `--threads > 1`

Investigation of the crash recorded in
`.superpowers/sdd/2026-09-07-corpus-evaluator/task-12-report.md` and
`docs/superpowers/smoke/2026-09-08-corpus-ab.md`. Read-only: nothing was
built, nothing was run, the working tree is untouched (only this file is
new). Every claim below is from reading the tree at `90f39c0`.

**Verdict up front:** the corpus evaluator did not introduce the bug. It
is a **pre-existing memory-safety defect exposed** — a `BattlefieldState&`
/ range-`for` reference into `GameState::battlefields` (a
`std::vector`) held across code that can `push_back` to that same vector.
The reallocation frees the buffer out from under the live reference; the
next read of `bf.id` yields a garbage 32-bit word, which is fed straight
back into `GameEngine::getBattlefield` — producing literally the observed
`Battlefield not found: 3691939024`. Confidence and the one-command
experiment that settles it are in §4.

---

## 1. Shared-state inventory

The first job was the obvious one: find the cross-thread mutable state.
**There essentially isn't any.** That is itself the finding — it is what
rules out the "plain data race" reading and forces the analysis toward a
heap-lifetime defect. What was checked, and what it holds:

### 1.1 Genuinely shared across worker threads

| What | Where | Verdict |
|---|---|---|
| `CardDB` (one, from `main`) | `src/main.cpp:542` → `BatchRunner` ctor `src/engine/batch_runner.cpp:9-13` → `GameRunner` `src/engine/game_runner.cpp:14-21` | `const&`, immutable after `buildFromClasses`. Safe. |
| `CardRegistry` (one, from `main`) | same chain | `const&`. `CardRegistry::get()` is `const` but hands back a non-const `Card*` (`src/cards/card_registry.h:24`, `card_registry.cpp:19-22`) — checked: **every `Card` subclass is stateless**. Their only data member is `const CardDef def_` (e.g. `src/cards/units/0678_poppy_paragon.cpp:35`). A sweep of all `: public Card` TUs found no non-const member and no function-local mutable static. Safe, and the claim in `CLAUDE.md:54` is accurate. |
| `AggregateResults` | `src/engine/game_runner.cpp:142-196` | atomics for counters, `console_mutex` for the timing fields and `std::cout`. Safe. |
| `GameConfig::agent_factory` | `src/main.cpp:521-539`, copied per task at `src/engine/batch_runner.cpp:45` | `std::function` copied, captures are by value (`factory_spec1/2`, deck path strings). Safe. |
| `open_spiel::GameRegisterer::factories()` | `build/_deps/open_spiel-src/open_spiel/spiel.h:1235` | function-local static `std::map`, written only by `REGISTER_SPIEL_GAME` static initializers (`src/openspiel/riftbound_game.cpp:83-88`) before `main`; read-only afterwards. `LoadGame` does **not** cache — every call `new`s a fresh `RiftboundGame` (`spiel.cc:282-297`, `riftbound_game.cpp:42-45`). Safe. |
| `open_spiel::Game::defaulted_parameters_` | `spiel.h:1195-1197` | `mutable`, but `GUARDED_BY(mutex_defaulted_parameters_)` and per-`Game` instance. Safe. |
| `open_spiel::algorithms::MIN_GC_LIMIT` | `algorithms/mcts.cc:37` | non-const global, but only ever read (`mcts.cc:215,354,451`). Safe. |

### 1.2 Per-thread / per-game (confirmed not shared)

- One `MctsAgent` **per game per seat** (`src/main.cpp:521-539` factory →
  `src/engine/game_runner.cpp:102-122`), each with its own
  `LoadGame(...)`-created `RiftboundGame`, its own `CardRegistry` +
  `CardDB` copies (`riftbound_game.cpp:47-58`), its own `MCTSBot` and its
  own evaluator (`src/agents/mcts_agent.cpp:296-313`).
- Evaluators are stateless: `RiftboundHeuristicEvaluator` and
  `RiftboundCorpusEvaluator` have no members and their
  `evaluateEngineState` is `const` (`mcts_agent.cpp:177-201`). Sharing one
  would be harmless anyway.
- `EventBus` is per game / per clone-engine; `TriggerManager` and
  `GameEngine` both disconnect their slots in their destructors
  (`src/engine/trigger_manager.cpp:50-53`, `game_engine.cpp:31-39`), and
  `RiftboundState` declares `event_bus_` before `engine_`
  (`riftbound_state.h:212-213`) so the bus outlives the engine. No
  dangling-slot hazard found.
- `StepDriver` is a `boost::context::fiber`, i.e. **cooperative and
  single-threaded** (`src/engine/step_driver.h:7-23`) — no OS thread. The
  header comment at `step_driver.cpp:43-49` says yields route "through a
  thread-local"; that comment is stale (`yield_target_` is a plain
  member, `step_driver.h:86`) but the member is per-driver, so it is
  wrong-but-harmless.
- The only function-local statics in the agent/OpenSpiel layer are
  `static const bool dbg = std::getenv(...)` (`mcts_agent.cpp:364`) —
  magic-static, thread-safe.
- `encodeAction` / `decodeAction` (`src/openspiel/action_vocab.cpp`) are
  pure; every vocab size is `constexpr` (`action_vocab.h:50,65,74`).
- `GameState` contains **no raw pointers** (grep over
  `src/core/game_state.h`, `game_object.h`), so a `GameState` copy carries
  nothing that can dangle across the copy. `allUnitsControlledBy`
  (`src/core/game_state.cpp:61-71`) caches nothing and returns by value.

**Conclusion of §1:** apart from an immutable card database, threads in
this program share nothing. A classic data race is not available as an
explanation.

---

## 2. Root cause

### 2.1 The defect

`GameState::battlefields` is a `std::vector<BattlefieldState>`
(`src/core/game_state.h:534` region; `BattlefieldState` at
`game_state.h:247-330`, with `BattlefieldId id` at **offset 0**).

`GameEngine::setupBattlefields` builds it with exactly **two**
`push_back`s and no `reserve` (`src/engine/game_engine.cpp:400-434`,
calls at `:432-433`). So for the whole game
`battlefields.size() == capacity() == 2` — **the next `push_back`
is guaranteed to reallocate and free the old buffer.**

There is exactly one post-setup `push_back` into that vector:

- `EffectExecutor::addBattlefieldToken` — `src/engine/effect_executor.cpp:853`

and exactly one caller of it:

- `BaronNashor::onPlay` — `src/cards/units/0709_baron_nashor.cpp:44`
  ("As you play me, add the Baron Pit battlefield token to the board")

**`1 Baron Nashor` is in `decks/kennen_tyler.txt`.** It is not in
`decks/rengar_test.txt`. Kennen is in every crashing batch.

Meanwhile, three sites hold a reference *into that vector* across calls
that resolve cards:

1. **`src/engine/game_engine.cpp:857-874`** — the main-phase staged-
   battlefield driver:
   ```
   for (auto& bf : state_.battlefields) {
       if (bf.showdown_staged && ...) { runShowdown(bf.id); ... bf.showdown_staged = false; }
       if (bf.combat_staged   && ...) { runCombat(bf.id);   ... bf.combat_staged   = false; }
   }
   ```
   A range-`for` whose body runs entire showdowns and combats. Both the
   loop's iterators and `bf` die on a reallocation; the following
   `bf.*_staged = false` is a **write** into freed memory and the next
   iteration's `bf.id` is a **read** of freed memory.
2. **`GameEngine::runShowdown`** — `game_engine.cpp:3799-3836`:
   `auto& bf = getBattlefield(bf_id);` is taken at the top and then read
   and written *after* `runShowdownLoop(bf_id)` (`bf.showdown_in_progress
   = false`, `bf.controller`, `bf.is_contested`) and after
   `scoreConquer`.
3. **`GameEngine::runCombat`** — `game_engine.cpp:3963-4046`:
   `auto& bf = getBattlefield(bf_id);` written across
   `combatDamageStep(bf_id)` / `combatResolutionStep(bf_id)`
   (`bf.combat_phase = ...` at `:4040` and `:4045`).

`getBattlefield` itself hands out exactly these references
(`game_engine.cpp:6476-6489`) and is called from 17 sites in that file.

### 2.2 Why the symptom looks the way it does

`BattlefieldState::id` is the **first** member, at offset 0. A dangling
`bf.id` therefore reads the first 4 bytes of whatever now owns that freed
chunk, and that value is handed straight to
`getBattlefield(bf.id)` → `throw std::runtime_error("Battlefield not
found: " + ...)`.

Both observed values fall out of exactly that:

- `Battlefield not found: 3691939024` (`0xDC0F_xxxx`) — the low half of a
  heap pointer sitting at offset 0 of the reused chunk.
- `Battlefield not found: 37` (the `--threads 4`, 100-game run) — a small
  integer, i.e. a `GameObjectId`: the reused chunk now holds a
  `std::vector<GameObjectId>` buffer. **This is the detail that makes the
  diagnosis specific.** A garbage-pointer theory predicts only large
  garbage; a torn-int theory predicts near-miss values; only "the freed
  battlefield buffer got reused by an id array" predicts `37`.

The SIGSEGVs are the same defect on its write side: `bf.showdown_in_progress
= false`, `bf.combat_phase = ...`, `bf.controller = sole_player` are
stores into freed memory, which corrupt whatever allocation now lives
there and blow up later.

### 2.3 Why `eval=score` is unaffected, and why threads matter

This is a **use-after-free whose observable effect depends on what
reuses the freed block.** Two independent things change that:

- **The evaluator changes the game that gets played, not just the
  numbers.** The corpus heuristic pays for battlefields controlled (term
  2, `corpus_evaluator.cpp:32-40`) and for board width (term 3,
  `:46-54`). The score evaluator pays for nothing but the score delta
  (`mcts_agent.cpp:179-188`). So the corpus bot deploys units to
  battlefields and contests them far more often — it drives the engine
  into `runShowdown` / `runCombat` and into unit plays (Baron included,
  and every Baron play in every MCTS *simulation* counts, not just the
  real one) at a much higher rate. Task 12.2's own verification records
  the same seed running **14 turns with `score` and 20 turns with
  `corpus`** (task-12-report.md, "Spec test 10"). The corpus evaluator
  opens the dangerous window far more often.
- **The corpus evaluator is the first thing in the MCTS loop that
  allocates.** `corpusEvaluate` walks `objects` via
  `allUnitsControlledBy` (two `std::vector<GameObjectId>` per leaf,
  `game_state.cpp:61-71`) plus both `hand` and `trash`; the score
  evaluator reads two `int`s and allocates nothing. Freed memory that
  nothing reuses still holds its old bytes — a stale `bf.id` reads back
  `0` or `1` and the game finishes as if nothing happened. That is what
  `eval=score` is doing: **surviving the same bug, silently.**
- **Thread count changes the allocator, not the logic.** At
  `--threads 1` `BatchRunner` takes the fast path and runs every game on
  the main thread in glibc's `brk`-backed main arena, where freed chunks
  are never returned to the OS and reuse order is a stable LIFO. At
  `--threads N` each game runs on a `boost::asio::thread_pool` worker
  (`batch_runner.cpp:41-54`) with its own `mmap`-backed arena, different
  reuse order, and the possibility of the region being released
  entirely — garbage instead of stale-but-plausible bytes, or an
  unmapped page (SIGSEGV). This is exactly the reported pattern: **same
  decisions, same per-game results, crash only above one thread.**

### 2.4 Pre-existing or introduced by Task 12 — the evidence

**Pre-existing.** Three independent lines:

1. `git show 90f39c0 -- src/agents/mcts_agent.cpp` introduces no new
   shared object, no new lifetime and no new mutable state:
   `makeEvaluator` returns a stateless `shared_ptr` in the exact spot
   `std::make_shared<RiftboundHeuristicEvaluator>()` occupied before,
   `Prior()` moved to the base class byte-for-byte, and the terminal
   short-circuit is unchanged.
2. `corpusEvaluate` (`src/agents/corpus_evaluator.cpp:11-108`) writes
   nothing, keeps no reference past the call, and guards **every**
   `getObject` with `objectExists` (`:64`, `:82`, `:99`). It is
   memory-safe on any well-formed `GameState`. The corruption is
   therefore upstream of it.
3. The three dangling sites (§2.1) and the `push_back` all predate this
   branch's Task 12 commits; `git log` shows them untouched by `d25584c`
   and `90f39c0`.

### 2.5 A second, independent landmine found on the way

`GameState::getObject` is guarded **only by `assert`**
(`src/core/game_state.h:589-599`), and the batch binary is built
`Release` = `-O3 -DNDEBUG` (`build/CMakeCache.txt:84,107`). In the
shipped binary a missing id therefore dereferences `objects.end()`
instead of failing. There are 812 `getObject(` call sites against 814
`objectExists(` calls, so most are guarded — but the ones that aren't
(e.g. `game_engine.cpp:2474`, `:4274`) turn any stale id into UB rather
than a named error. This is not the cause of *this* crash, but it is why
the engine has no way to tell you when a stale id appears, and fixing it
is cheap.

Also noted, not implicated:
`boost::context::protected_fixedsize_stack::allocate()` ignores the
`mprotect` return value under `NDEBUG`
(`/usr/include/boost/context/posix/protected_fixedsize_stack.hpp`, the
`boost::ignore_unused(result); BOOST_ASSERT(0 == result);` pair), so a
fiber could in principle get no guard page. Checked and largely ruled
out: an `objdump` scan of `build/riftbound` shows the largest stack frame
in the whole binary is 4096 bytes — equal to the guard page — so an
overflow of the 256 KB fiber stack (`step_driver.cpp:16`) would hit the
guard rather than skip it. Worth a line in the file, not a fix.

---

## 3. Minimal proposed fix

No literal code; files, behaviours and test cases only.

**Fix A — stop holding references across resolution (the actual fix).**

- `src/engine/game_engine.cpp`, main-phase staged-battlefield driver
  (≈`:857-874`): iterate a **snapshot of ids** (collect `bf.id` into a
  local `std::vector<BattlefieldId>` first, then loop over that),
  re-fetching `getBattlefield(id)` immediately before each read and each
  write. No iterator and no reference survives a `runShowdown` /
  `runCombat` call. Skip ids that no longer resolve.
- `GameEngine::runShowdown` (`:3799`): drop the function-scope
  `auto& bf`. Re-fetch after `runShowdownLoop(bf_id)` and again after
  `scoreConquer`.
- `GameEngine::runCombat` (`:3963`) and `combatDamageStep` /
  `combatResolutionStep`: same — re-fetch after every nested call that
  can resolve a card.
- Add a one-line contract comment on `GameEngine::getBattlefield`
  (`:6476`) stating that the returned reference is invalidated by any
  card resolution, because `state_.battlefields` can grow
  (`EffectExecutor::addBattlefieldToken`).

**Fix B — kill the class, not just the instance (recommended, cheap).**
Change `GameState::battlefields` from `std::vector<BattlefieldState>` to
`std::deque<BattlefieldState>` in `src/core/game_state.h`. `deque`
keeps references valid across `push_back`, which makes every one of the
17 `getBattlefield` sites and the ~25 range-`for`s over `battlefields`
correct by construction. Needs a check that nothing relies on contiguity
or on `&battlefields[0]` (grep found none). Do **not** "fix" this with
`reserve()` in `setupBattlefields` — that hides the bug behind a capacity
number and the next BF-token card re-opens it.

**Fix C — make the next occurrence say so (independent, do it anyway).**
`GameState::getObject` (`src/core/game_state.h:589-599`): replace the
`assert` with an unconditional throw naming the id, so Release and Debug
fail identically. `getBattlefield` already throws — that is the only
reason this crash produced a message at all instead of silent garbage.

### Tests that pin it

1. **Deterministic unit test** (`tests/`, links `riftbound_core`):
   set up a 2-battlefield game so `battlefields.size()==capacity()==2`,
   drive a staged showdown, and have the showdown resolution call
   `EffectExecutor::addBattlefieldToken`. Assert afterwards that
   `state.battlefields` has 3 slots with ids `0,1,2`, that the staged
   flags were cleared on the *right* slots, and that no `getBattlefield`
   throws. Under ASan this is red before Fix A and green after.
   House rule "a new test must be able to fail": prove it by reverting
   Fix A, watching it go red, and re-applying.
2. **Guard test** (`tests/`): a small test asserting that
   `addBattlefieldToken` invoked while a showdown is in progress leaves
   the engine consistent — this is the guard that must never be weakened
   if another BF-token card lands.
3. **Smoke** (`docs/superpowers/smoke/2026-09-08-corpus-ab.md`): the
   exact failing command,
   `--agent1 mcts:sims=20 --agent2 mcts:sims=20,eval=corpus --deck1
   decks/kennen_tyler.txt --deck2 decks/rengar_test.txt --games 20
   --threads 2 --seed 1040`, **3 consecutive clean runs**, then the same
   at `--threads 4` × 3, then `eval=score --threads 4` × 1 to show no
   regression. Record the crash reproduction *before* the fix in the same
   doc so the entry is a before/after, not an assertion.
4. **CI**: this is the case for the `tests.yml` port that
   `CLAUDE.md` already pre-ratifies — plus an ASan job, because a
   non-sanitized suite cannot see this class of defect at all.

---

## 4. Confidence, and the experiment that confirms it

**Confidence**

- That the dangling-`BattlefieldState&`-across-`battlefields.push_back`
  defect is **real, present at `90f39c0`, and reachable with
  `decks/kennen_tyler.txt`**: **high (~90%)**. The `push_back`, the
  guaranteed reallocation at size==capacity==2, the three holding sites
  and Baron Nashor's presence in the deck were each read directly.
- That it is **the** cause of the two reported aborts: **moderate-high
  (~70%)**. The `Battlefield not found: 37` value is strong
  corroboration, and no competing mechanism survived §1. The gap: I could
  not run the binary, so I have not observed Baron Nashor actually being
  played in these 20 seeds, and the cheapest path from a main-phase Baron
  play to a *live* outer `BattlefieldState&` runs through a Closed-State
  facedown reveal (`chain_manager.cpp:316-323`) rather than a plain main-
  phase play.
- That the corpus evaluator itself is innocent: **high (~95%)** — §2.4.

**The one experiment that needs no rebuild (do this first).**
Copy `decks/kennen_tyler.txt` to `decks/kennen_tyler_nobaron.txt` with
`1 Baron Nashor` removed (drop the line; add a filler to keep the deck
legal if the validator requires the count), then re-run the exact failing
command with `--deck2 decks/kennen_tyler_nobaron.txt`, 3× at
`--threads 2` and 3× at `--threads 4`. If Baron is the trigger the crash
disappears completely. That is a ~2-minute discriminator and it settles
§4's remaining 30% before anyone touches the engine.

**The sanitizer run (separate build dir, later, off the busy box).**

- `cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=RelWithDebInfo
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"` — a new
  directory, so `build/` and the binary the A/B used stay untouched.
- Re-run the failing command under it.
- **The discriminating prediction:** if this analysis is right, ASan
  reports `heap-use-after-free`, the *freed-by* stack names
  `std::vector<BattlefieldState>::push_back` ←
  `EffectExecutor::addBattlefieldToken` ← `BaronNashor::onPlay`, and —
  the important part — **it reproduces at `--threads 1`**, because ASan
  quarantines freed memory instead of letting benign reuse mask the
  stale access. It should also eventually reproduce with `eval=score` at
  `--threads 1` given enough seeds, since the defect is
  evaluator-independent; the corpus evaluator only raises its rate.
- If ASan is clean at `--threads 1` and at `--threads 4`, this analysis
  is wrong and the pre-existing-race reading is back on the table: build
  `build-tsan` with `-fsanitize=thread` and re-run. Note that TSan and
  `boost::context` fibers do not mix cleanly (TSan cannot see the stack
  switch), so expect noise and interpret only reports that name two
  distinct OS threads.

**Consequence for Task 12.3 in the meantime.** The A/B numbers must keep
coming from `--threads 1` runs, as the smoke doc already decided — and
the doc should now say *why*: not "a suspected race", but "a
use-after-free in the battlefield vector that multithreaded allocation
makes fatal". The four single-threaded corpus batches are unaffected by
the thread-count trigger, but they are **not** proof of an uncorrupted
run either — the same UAF is executing there, silently. Re-run the final
A/B once Fix A/B lands.
