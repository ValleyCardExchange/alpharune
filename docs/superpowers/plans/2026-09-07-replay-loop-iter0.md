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

### L0c — review follow-ups on L0b and the activation fix (spec addendum #17)
Files: `src/cards/card.h` (a per-target legality hook — `canEquipTarget`
taking state, controller and the candidate unit — defaulting to the
gear-level `canEquip`; only gear whose cost depends on the target
override it), `src/cards/gear/equip_base.h` (the standard predicate and
payer choose the power rune FIRST — preferring an exhausted rune — and
count energy only among the OTHER ready runes; the universal variant the
same way; one shared scan feeds both check and payment), `src/cards/
gear/0748_hextech_gauntlets.cpp` (`canEquipTarget` with the target's
Might; `canEquip` = exists a friendly unit for which it holds), `src/
cards/gear/0498_blade_of_the_ruined_king.cpp` (pay power, then kill),
`src/engine/game_engine.cpp` (generator gates each per-unit equip intent
on `canEquipTarget`; `executeIntent`'s activation path returns whether
it executed — a bool on the private method, `game_engine.h` may change
for that — and rejects an underpaid energy cost before any state
change; the closed-state callback uses that status instead of the
fingerprint), `src/agents/agent_spec.cpp` (non-numeric prior values
rethrown as `runtime_error` naming the key), tests in `tests/cards/
test_equip_legality.cpp` (extend), `tests/cards/test_closed_state_
abilities.cpp` (extend), `tests/test_prior_config.cpp` (extend).
Tests (RED first): (1) Boneshiver-shape gear with exactly one ready
domain rune and no other rune → not offered, `onEquip` false, nothing
paid; with one ready domain rune plus one other ready rune → equips,
exactly one rune exhausted and a different one recycled; (2) universal
gear with energy 1 and a single rune → not offered; (3) Hextech
Gauntlets with two friendly units of different Might and runes enough
only for the Mightier → exactly one offer (that unit), and the
hand-built intent for the other is rejected with the warning and no
state change; (4) Blade of the Ruined King: when power cannot be paid
after `canEquip` held (drain the Order rune between check and pay
through the test hook), no unit is killed; (5) closed-state activation
callback: a hand-built underpaid-energy activation is rejected, pays
nothing, and does NOT reset the pass set; a legitimate one still resets
it (existing tests stay green); (6) `prior=` with a string where a
number belongs throws `runtime_error` whose message names the key.
Constraints: full suite green (baseline 1246/1), coverage gate
unchanged, `test_equipment.cpp` untouched, no card-name logic in the
engine. Commit subject: `Engine: equip legality per target, no rune
double-spend, activation status reported explicitly`.

### L0d — QUEUED: activation power costs and recycled-rune exhaustion (spec addendum #18)
Files: `src/engine/game_engine.cpp` (activation generators gate on the
ability's power cost via the engine's canonical power-availability check;
`executeIntent` pays it through the canonical payer, before any state
change, alongside the energy pre-check), `src/cards/gear/equip_base.h`
and `src/cards/card_helpers.h` (every equip payer clears `is_exhausted`
on the rune it recycles, exactly as `payAdditionalCost` does — one shared
recycle helper so the three payers cannot drift), tests in
`tests/cards/test_equip_legality.cpp` and `tests/cards/
test_closed_state_abilities.cpp` (extend) plus a registry-wide guard that
every shipped ability with a power cost is neither offered nor executed
with no matching power available.
Tests (RED first): Treasure Trove with no Chaos rune → not offered,
hand-built intent rejected, gear not killed; with one → offered, pays,
executes; a rune recycled while exhausted returns to the rune deck ready
and, when channeled back, is usable that turn; the guard names any
power-cost ability that escapes the gate.
Not started in iteration 0 — Tyler chooses when.

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
