# Batch analysis summary

**The mistake/swing heuristic below is crude by design** — it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.

Games processed: 200  |  skipped (truncated/incomplete): 0  |  skipped (unparseable): 0
Mean turns (overall): 15.805

## Validity

OK — no frozen-state bursts (longest identical run 2, threshold 10).

## Win rate by deck and seat (Wilson 95%)

| Deck | Seat | Wins | Games | Win rate | 95% CI |
|---|---|---|---|---|---|
| kennen_tyler | P2 | 90 | 200 | 45.0% | 38.3% - 51.9% |
| rengar_test | P1 | 110 | 200 | 55.0% | 48.1% - 61.7% |

## Point-tempo curve (mean score / margin by turn)

- **rengar_test::P1** (turns 1..21): score=[0.0, 0.18, 0.47, 0.795, 1.12, 1.525, 1.96, 2.41, 2.875, 3.398, 3.943, 4.465, 5.039, 5.48, 5.814, 6.074, 6.28, 6.655, 6.875, 6.75, 7.0] margin=[0.0, -0.065, -0.105, -0.175, -0.27, -0.315, -0.33, -0.32, -0.35, -0.214, -0.104, -0.005, 0.123, 0.393, 0.434, 0.506, 0.58, 0.621, 0.688, 0.75, 0.0]
- **kennen_tyler::P2** (turns 1..21): score=[0.0, 0.245, 0.575, 0.97, 1.39, 1.84, 2.29, 2.73, 3.225, 3.612, 4.047, 4.471, 4.916, 5.087, 5.381, 5.568, 5.7, 6.034, 6.188, 6.0, 7.0] margin=[0.0, 0.065, 0.105, 0.175, 0.27, 0.315, 0.33, 0.32, 0.35, 0.214, 0.104, 0.005, -0.123, -0.393, -0.434, -0.506, -0.58, -0.621, -0.688, -0.75, 0.0]

## Action-family mix per phase per deck

- **kennen_tyler**
  - Mulligan: {'setup': 200}
  - MainPhase: {'pass': 16511, 'play': 3591, 'choice': 10900, 'move_to_battlefield': 2226, 'activate': 2681, 'combat_damage': 1352, 'move_to_base': 585}
  - ScoringStep: {'activate': 83, 'pass': 202, 'choice': 59, 'play': 9}
- **rengar_test**
  - Mulligan: {'setup': 200}
  - MainPhase: {'play': 2613, 'choice': 10165, 'pass': 16435, 'move_to_battlefield': 2336, 'combat_damage': 1352, 'move_to_base': 1218, 'activate': 690}
  - ScoringStep: {'pass': 201, 'play': 1, 'choice': 6}

## Per-game markers

- Flow plays (printed): {'kennen_tyler': 457}
- Flow plays (granted): {'kennen_tyler': 14}
- Tomb-restricted plays: {'kennen_tyler': 29, 'rengar_test': 23}
- Conquers (score increments attributed to actor): {'rengar_test': 870, 'kennen_tyler': 882}
- Reaction plays in the closed state: {'rengar_test': 318, 'kennen_tyler': 210}
- Burns/empowers: OMITTED (log schema carries no marker for these)

## Mistake candidates (losing deck, top 25)

- drop=3 game=game_110_seed_3110.json idx=237 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=239 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Closed scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=240 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': 'Ferrous Forerunner', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=241 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=242 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=243 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=244 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=245 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=246 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=247 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=248 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=250 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': "Kai'Sa, Survivor", 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/5m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=251 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/5m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=252 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/5m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=253 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/5m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=254 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/5m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=255 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/5m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=257 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:1u/6m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_110_seed_3110.json idx=258 turn=12 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'EndTurn'}
  board: turn 12 MainPhase/Open scores P1=4 P2=4 | Emperor's Dais[P1] P1:2u/9m P2:0u/0m; Minefield[P2] P1:0u/0m P2:2u/14m
- drop=3 game=game_137_seed_3137.json idx=107 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 2] chosen={'card': 'Treasure Hunter', 'destination': {'player': 'P2', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 7 MainPhase/Open scores P1=1 P2=2 | Star Spring[None] P1:1u/4m P2:0u/0m; Sandswept Tomb[P1] P1:1u/6m P2:0u/0m
- drop=3 game=game_137_seed_3137.json idx=108 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 MainPhase/Open scores P1=1 P2=2 | Star Spring[None] P1:1u/4m P2:0u/0m; Sandswept Tomb[P1] P1:1u/6m P2:0u/0m
- drop=3 game=game_137_seed_3137.json idx=109 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 MainPhase/Open scores P1=1 P2=2 | Star Spring[None] P1:1u/4m P2:0u/0m; Sandswept Tomb[P1] P1:1u/6m P2:0u/0m
- drop=3 game=game_137_seed_3137.json idx=110 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 2] chosen={'card': 'Lightning Rush', 'destination': None, 'flow_source': 'Printed', 'restricted_bf': None, 'source_zone': 'Trash', 'type': 'PlayCard'}
  board: turn 7 MainPhase/Open scores P1=1 P2=2 | Star Spring[None] P1:1u/4m P2:0u/0m; Sandswept Tomb[P1] P1:1u/6m P2:0u/0m
- drop=3 game=game_137_seed_3137.json idx=111 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 7 MainPhase/Closed scores P1=1 P2=2 | Star Spring[None] P1:1u/4m P2:0u/0m; Sandswept Tomb[P1] P1:1u/6m P2:0u/0m
- drop=3 game=game_137_seed_3137.json idx=112 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 7 MainPhase/Closed scores P1=1 P2=2 | Star Spring[None] P1:1u/4m P2:0u/0m; Sandswept Tomb[P1] P1:1u/6m P2:0u/0m

## Swing decisions (winning deck, top 25)

- rise=4 game=game_104_seed_3104.json idx=147 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': 'Star-Crossed', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=148 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=149 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=150 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=151 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=152 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=154 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=155 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=156 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': 'Traveling Merchant', 'destination': {'player': 'P2', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=157 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=158 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=159 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': 'Ride the Wind', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=160 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=161 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=162 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=163 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=165 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=166 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=168 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': 'Ride the Wind', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=169 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=170 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=171 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=172 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=173 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_104_seed_3104.json idx=175 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=1 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m

## Notes

- The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.
- Turn ownership is inferred (first non-Closed decision's actor per turn), not read from an explicit field.
- Burns/empowers OMITTED: no decision in this batch carried a burn/empower marker in the assumed schema; nothing to count.
