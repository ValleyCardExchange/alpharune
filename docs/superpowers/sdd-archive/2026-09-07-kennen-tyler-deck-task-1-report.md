# Task 1 report — Header scaffolding (one rebuild) + keyword guard test

Branch: `kennen-tyler-deck` (verified with `git branch --show-current` before editing)
Commit: `a040093` — *Kennen: header scaffolding for Empower, Flow, non-hand trigger, Tomb discount*

## What I implemented

Every declaration in the brief's "Interfaces produced" list, plus the one
behavioural change (the Flow keyword and its `toString`) driven test-first.

| Interface | Where |
| --- | --- |
| `Keyword::Flow = 23`, `Count = 24` | `src/core/types.h:106-107` |
| `toString(Keyword::Flow) == "Flow"` | `src/core/types.cpp:136` |
| `GameObject::is_empowered` | `src/core/game_object.h` (beside `is_stunned`) |
| `GameObject::GrantedFlow` + `granted_flow` (`energy`, `power`, `power_domain`, `any_domain`, `valid_on_turn`) | `src/core/game_object.h` (after `string_state`) |
| `ActivationCost::disempower_self` | `src/effects/effect_types.h` |
| `TriggerType::WhenYouPlayFromNonHand` (end of enum) | `src/effects/effect_types.h` |
| `CardPlayedEvent::play_source` (`Intent::PlaySource`, default `Hand`) | `src/core/events.h` |
| `ObjectEmpoweredEvent{object, controller}` + `on_object_empowered` signal + `emit` overload | `src/core/events.h` |
| `Intent::FlowSource {None, Printed, Granted}`, `Intent::flow_source`, `Intent::target_battlefield_restriction` | `src/core/intent.h` (beside `use_alt_play_cost`) |
| `ChainItem::banish_on_leave`, `ChainItem::target_battlefield_restriction` | `src/core/game_state.h` |
| `BattlefieldState::friendly_spell_power_discount` | `src/core/game_state.h` |
| `PlayerState::transient_power_discount` + reset in `resetTurnTracking` | `src/core/game_state.h` |
| `CardDef::flow_energy`, `flow_power`, `flow_any_domain` | `src/core/card_db.h` |
| `Card::FlowCost` + `virtual FlowCost flowCost() const` | `src/cards/card.h` (after `alternativePlayCost`) |
| `EffectExecutor::RestDestination {Recycle, Trash}`; `revealAndChoose(PlayerId, int, RestDestination = Recycle)` | `src/engine/effect_executor.h` / `.cpp:1065` |
| `empowerObject`, `disempowerObject`, `burnCards`, `burnOut` (empty bodies) | `src/engine/effect_executor.h` / end of `.cpp` |
| `GameEngine::FlowOffer`, `liveFlowCosts`, `generateFlowPlayActions`, `playSourceFor` (minimal bodies) | `src/engine/game_engine.h` / `.cpp` (next to `generateTrashReplayActions`) |

`Card::flowCost()`'s default reads `def()`, returns invalid unless
`def().keywords.has(Keyword::Flow)`, and sets `power_domain` to
`def().domains.front()` when the card has any domain.

Per the controller's rulings I did **not** touch `src/cards/card_helpers.h`
(Task 5's, test-first), created no card files, and did not touch `decks/`.

## TDD evidence

### RED

Test #27 appended to `tests/test_types.cpp` (following the
`TEST(TypesTest, KeywordSetBasic)` style at line 16): for every bit below
`Keyword::Count`, `toString` must be non-null, non-empty, not the `"Unknown"`
fallback, and unique across the whole enum; then
`EXPECT_STREQ(toString(Keyword::Flow), "Flow")`.

```
$ cmake --build build --target riftbound_tests -j4
/home/user/chorlick/alpharune/tests/test_types.cpp:123:36: error: 'Flow' is not a member of 'riftbound::Keyword'
  123 |     EXPECT_STREQ(toString(Keyword::Flow), "Flow");
```

Expected: the enumerator does not exist yet, so the RED is a compile failure
naming it, exactly as the brief's Step 1 anticipated. The uniqueness half of
the test also guards the `toString` switch — had I added the enumerator
without a `case`, the switch would fall through to `"Unknown"` and the
`EXPECT_NE(name, "Unknown")` assertion would have caught it at runtime.

### GREEN

```
$ cmake --build build --target riftbound_tests -j4
[878/878] Linking CXX executable riftbound_tests
[exited with code 0]

$ RIFTBOUND_ROOT=. ./build/riftbound_tests --gtest_filter='TypesTest.KeywordToStringCompleteAndUnique'
[       OK ] TypesTest.KeywordToStringCompleteAndUnique (0 ms)
[  PASSED  ] 1 test.
```

## Build and suite result

Final ninja line: `[878/878] Linking CXX executable riftbound_tests`, exit
code 0, **0 errors, 0 warnings surfaced**.

```
$ RIFTBOUND_ROOT=. ./build/riftbound_tests 2>&1 | tail -5
[==========] 1058 tests from 115 test suites ran. (1778 ms total)
[  PASSED  ] 1058 tests.

  YOU HAVE 1 DISABLED TEST
```

1058 passed (1057 pre-existing + #27), 1 disabled (pre-existing) — matches the
brief exactly.

## Files changed (14, all explicitly `git add`-ed; no `git add -A`)

```
src/cards/card.h               | 24 +++++++++++
src/core/card_db.h             |  7 +++++
src/core/events.h              | 18 ++++++++++
src/core/game_object.h         | 20 ++++++++++
src/core/game_state.h          | 19 ++++++++++
src/core/intent.h              | 21 +++++++++++
src/core/types.cpp             |  1 +
src/core/types.h               |  3 +-
src/effects/effect_types.h     |  8 ++++++
src/engine/effect_executor.cpp | 19 ++++++++-
src/engine/effect_executor.h   | 27 +++++++++++-
src/engine/game_engine.cpp     | 15 ++++++++
src/engine/game_engine.h       | 29 ++++++++++++++
tests/test_types.cpp           | 21 +++++++++++
14 files changed, 229 insertions(+), 3 deletions(-)
```

`git status` is clean post-commit; `.superpowers/` is gitignored, so no
workspace or build-log files were staged.

## Self-review findings

Checks I ran against the diff rather than assuming:

1. **Aggregate-initialization safety.** I inserted
   `target_battlefield_restriction` and `banish_on_leave` into the *middle* of
   `ChainItem`, which would break any positional brace-init. `grep -rn
   "ChainItem{" src/ tests/` returns **no hits** — every construction is
   field-by-field. Safe.
2. **`CardPlayedEvent` call sites.** All six emit sites plus two test sites
   pass at most 5 positional arguments; `play_source` is the 6th member with a
   default, so none needed changing.
3. **`ActivationCost` call sites** all use designated initializers
   (`ActivationCost{.exhaust = true, ...}`), so appending `disempower_self` is
   inert.
4. **`Keyword::Count` consumers.** Every use in the tree
   (`game_engine.cpp:968/4204`, `0434_kato_the_arm.cpp:34`, and the
   `Keyword::Count`-as-sentinel fields) is a loop bound or a "none" sentinel
   that scales with the new value automatically. No hardcoded `23` anywhere.
5. **Exhaustive `switch` over `Keyword`.** Only `src/core/types.cpp` switches
   on `Keyword`, and I added the `Flow` case there — no other TU can go
   non-exhaustive.
6. **`revealAndChoose` back-compat.** The default `RestDestination::Recycle`
   plus the unchanged body means the two existing call sites compile and
   behave identically; the `rest` parameter is deliberately unused
   (`/*rest*/`) until Task 7.
7. **Names verified verbatim** against the brief's interface list — all 20
   present and spelled exactly as listed.
8. **No extra behaviour.** All six new executor/engine bodies are empty or
   return empty/`Hand`; the only semantic change in the diff is the Flow
   enumerator and its `toString`.

## Judgment call the controller should confirm

**I added `flow_source` and `target_battlefield_restriction` to
`Intent::operator==`** (`src/core/intent.h`). This is not in the brief's
interface list, so flagging it explicitly.

Reason: `operator==` is documented in-file as "full structural equality … used
by serializers to locate the chosen action's index within the legal-action
list", and warns that a partial comparison "would conflate 'twin' intents …
and bias the recorded chosen_idx toward earlier indices". Spec §3/addendum #1
emits two intents that differ **only** in `flow_source` (test #29), and §4
emits a restricted variant that differs **only** in
`target_battlefield_restriction`. Without these two lines those twins compare
equal and `chosen_idx` silently resolves to the wrong one.

It is a zero-behaviour-change edit today (both fields hold their defaults
everywhere in the current tree, which the green 1058 confirms). Revert the two
lines if the controller would rather Task 3/Task 8 own them alongside the
tests that exercise them.

## Concerns

1. **`events.h` now includes `intent.h`.** The brief anticipated a possible
   cycle. There is none: `intent.h` includes only `types.h`, so no
   forward-declaration workaround was needed and `Intent::PlaySource` is used
   directly as the field type. Noted because it widens what every TU including
   `events.h` pulls in — negligible here (`intent.h` is small and header-only),
   but it is a real edge added to the include graph.
2. **Pre-existing gap I did not touch:** `Intent::operator==` still omits
   `use_alt_play_cost` and `granted_ability_def`. Same conflation hazard as
   above, for Jhin-style alt-cost twins and aura-granted abilities. Out of
   scope for this task; worth a separate look, since a serializer picking the
   wrong `chosen_idx` fails silently rather than loudly.
3. **`liveFlowCosts` returns `std::vector<FlowOffer>` by value** as specified.
   It will be called per trash spell inside the action generator; if that
   shows up in self-play profiling, Task 3 may want an out-parameter. Not a
   correctness issue.
4. **`GameEngine::playSourceFor` currently returns `Hand` unconditionally.**
   Nothing calls it yet, so nothing is wrong today, but this stub is the kind
   that reads as finished code. Task 2/3 must replace it before any
   `WhenYouPlayFromNonHand` test can pass — a test that trusted it now would
   pass for the wrong reason.
