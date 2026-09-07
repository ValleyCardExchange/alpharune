## Verdict: Approved with notes

### 1. [Moderate — soundness, currently unreachable] "did it execute" fingerprint is a guess, not a proof
`src/engine/game_engine.cpp:1216-1244` (`setActivateAbility` lambda). `resourceFingerprint`
inspects only the ACTIVATING player's own `hand`/`main_deck` size, `xp`, and `rune_pool`
fields, plus the source's `is_exhausted`/`is_empowered`. It never looks at the OPPONENT's
`PlayerState`, at board/unit/battlefield state, or at individual rune-OBJECT exhaustion —
the energy-cost payment loop (`game_engine.cpp:1339-1349`) exhausts rune *objects* directly
and never touches `rune_pool.energy`/`power[]`, so an energy-only cost is invisible to the
fingerprint too.

Concrete failing case: a future zero-`exhaust` `[Reaction]` ability whose only cost is
`energy` and whose only effect targets the OPPONENT (e.g. "target opponent discards a
card") and that resolves without a chain item would be misreported as "executed nothing".
That drives `ChainManager::stepExecuteAndPass`'s false-rejection branch (`logWarn` +
`continue`, `chain_manager.cpp:1075-1078`), which does **not** clear
`players_passed_priority` for an action that actually happened (violates CR 337.1.b.3) and
leaves the same intent legal, so a repeat-choosing agent could re-execute it (bounded only
by `kMaxPriorityPasses`=10).

Not currently exploitable: `executeIntent`'s activation case unconditionally reaches
`addAbility()`+`runChain()` (a no-op under `isProcessing()`) for every activation that
survives the two early rejects, so chain-size-changed already catches every real case in
today's registry — confirmed by the commit message itself ("no card in the registry has a
`[Reaction]` ability that is not an `[Add]` ability") and by `ability-fix-report.md` §6
concern 2, which names this exact gap. Minimal fix: have `executeIntent`'s activation case
report success/failure explicitly (a return value or member flag) instead of inferring it
from state diffs — needs a `game_engine.h` signature change, correctly deferred, but should
be tracked rather than left as permanent trust in "no card does this yet."

### 2. [Low — pre-existing, newly exposed] underpaid energy cost still executes
`src/engine/game_engine.cpp:1339-1349`. The loop decrements `needed` per exhausted ready
rune but never checks `needed == 0` before falling through to `addAbility`. A hand-built
`ActivateReactionAbility`/`ActivateAbility` intent for a real ability costing `energy=3`
with only 1 ready rune pays 1 and still fully executes. Pre-existing (same code already
served `ActivateAbility`/`ActivateActionAbility`), but this diff is what makes
`ActivateReactionAbility` reachable through `ChainManager` again, and `l4`/ability-fix
report both flag it as newly relevant. `generateClosedStateActions` itself gates
affordability correctly, so this only bites hand-built intents (MCTS/OpenSpiel
bridge/replay) — same caveat the implementer's report already states (§6 concern 4).
Minimal fix: `if (needed > 0) break;` after the payment loop, mirroring the disempower
re-check just above it.

### 3. [Low — doc/contract] `loadPriorConfig` can throw the wrong exception type
`src/agents/agent_spec.cpp:189-220`. Every `.get<double>()` call is unguarded; a non-numeric
value (e.g. `{"action_family_weights": {"play": "high"}}`) throws
`nlohmann::json::type_error`, which is **not** a `std::runtime_error` — contradicting the
header's `@throws std::runtime_error` contract (`agent_spec.h:365`). `main.cpp`'s top-level
`catch (const std::exception&)` survives it in practice, so this is not a live bug, but no
test covers the path and a narrower catch site would slip through. Minimal fix: wrap each
field read (or the two per-section blocks) in try/catch and rethrow as `std::runtime_error`
naming the key, matching `rejectUnknownKeys`'s style.

### Checks that passed
- **Timing (ask #2):** `generateClosedStateActions` is the only generator emitting
  `ActivateReactionAbility`, gated on `ab.is_reaction`; main-phase/showdown generators only
  emit `ActivateAbility`/`ActivateActionAbility` (`game_engine.cpp:3797-3841`, `:3805-3838`).
  `resolveShowdownDecision`'s `ActivateReactionAbility` case is a pre-existing defensive
  catch-all for hand-built intents, not generator output — no new Open-state leak.
- **L4 parsing:** unknown keys rejected and named at all three levels; partial files merge
  with `PriorConfig{}` defaults (field-by-field tests); no `prior=` is byte-identical
  (`FamilyWeightsDefaults`/`CorpusWeightsDefaults` guard tests + same-seed binary A/B in
  `l4-report.md`). `intentFamilyWeight` matches the removed inline switch case-for-case,
  including the `StandardMove` battlefield/base split and `default: return 1.0`. No header
  here pulls OpenSpiel into `riftbound_core` — `agent_spec.h` only adds
  `corpus_evaluator.h`/`core/types.h`, both already OpenSpiel-free.
- **House rules:** no new card-name conditionals (the one `name == "Brush"` hit is
  pre-existing, untouched); header edits match the plan's explicit scope
  (`chain_manager.h`/`game_engine.cpp` for the ability fix; `agent_spec.h`/`mcts_agent.h`/
  `corpus_evaluator.h` for L4 — the last one explicitly named in the L4 plan section, not an
  overreach); both commits carry `Co-Authored-By`/`Claude-Session` trailers; new tests assert
  concrete state (exhaust flags, power counts, finalize/resolve event counts) and are not
  tautological.
