# Batch analysis summary

**The mistake/swing heuristic below is crude by design** — it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.

Games processed: 200  |  skipped (truncated/incomplete): 0  |  skipped (unparseable): 0
Mean turns (overall): 15.04

## Validity

OK — no frozen-state bursts (longest identical run 2, threshold 10).

## Win rate by deck and seat (Wilson 95%)

| Deck | Seat | Wins | Games | Win rate | 95% CI |
|---|---|---|---|---|---|
| rengar_test | P2 | 86 | 200 | 43.0% | 36.3% - 49.9% |
| kennen_tyler | P1 | 114 | 200 | 57.0% | 50.1% - 63.7% |

## Point-tempo curve (mean score / margin by turn)

- **kennen_tyler::P1** (turns 1..20): score=[0.0, 0.385, 0.7, 1.27, 1.725, 2.295, 2.795, 3.42, 3.97, 4.485, 4.963, 5.29, 5.541, 5.777, 6.082, 6.214, 6.324, 6.476, 6.778, 6.667] margin=[0.0, 0.255, 0.245, 0.56, 0.69, 0.85, 0.965, 1.23, 1.3, 1.347, 1.34, 1.195, 0.77, 0.587, 0.32, 0.375, 0.054, -0.048, 0.0, -0.333]
- **rengar_test::P2** (turns 1..20): score=[0.0, 0.13, 0.455, 0.71, 1.035, 1.445, 1.83, 2.19, 2.67, 3.138, 3.623, 4.095, 4.77, 5.19, 5.763, 5.839, 6.27, 6.524, 6.778, 7.0] margin=[0.0, -0.255, -0.245, -0.56, -0.69, -0.85, -0.965, -1.23, -1.3, -1.347, -1.34, -1.195, -0.77, -0.587, -0.32, -0.375, -0.054, 0.048, 0.0, 0.333]

## Action-family mix per phase per deck

- **kennen_tyler**
  - Mulligan: {'setup': 200}
  - MainPhase: {'play': 3480, 'choice': 10407, 'pass': 16447, 'activate': 2505, 'move_to_battlefield': 2319, 'move_to_base': 696, 'combat_damage': 1137}
  - ScoringStep: {'play': 2, 'choice': 16, 'activate': 67, 'pass': 154}
- **rengar_test**
  - Mulligan: {'setup': 200}
  - MainPhase: {'pass': 17154, 'play': 2558, 'choice': 9809, 'move_to_battlefield': 1843, 'move_to_base': 970, 'combat_damage': 1137, 'activate': 620}
  - ScoringStep: {'pass': 154}

## Per-game markers

- Flow plays (printed): {'kennen_tyler': 539}
- Flow plays (granted): {'kennen_tyler': 41}
- Tomb-restricted plays: {'kennen_tyler': 52, 'rengar_test': 31}
- Conquers (score increments attributed to actor): {'rengar_test': 742, 'kennen_tyler': 926}
- Reaction plays in the closed state: {'rengar_test': 326, 'kennen_tyler': 184}
- Burns/empowers: OMITTED (log schema carries no marker for these)

## Mistake candidates (losing deck, top 20)

- drop=3 game=game_112_seed_2112.json idx=102 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': 'Ride the Wind', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=103 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=104 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=105 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=106 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=108 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=109 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=111 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': 'Nocturne, Horrifying', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=112 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=113 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=114 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=115 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=116 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=117 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=118 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Minefield[None*] P1:1u/1m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=119 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Minefield[None*] P1:1u/1m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=121 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Minefield[None*] P1:1u/1m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=123 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None*] P1:1u/1m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_112_seed_2112.json idx=125 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None*] P1:1u/1m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_124_seed_2124.json idx=25 turn=2 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 0] chosen={'card': 'Irresistible Faefolk', 'destination': {'player': 'P2', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 2 MainPhase/Open scores P1=0 P2=0 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m

## Swing decisions (winning deck, top 20)

- rise=4 game=game_133_seed_2133.json idx=148 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2] P1:0u/0m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=149 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': 'Lightning Rush', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/2m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=150 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/2m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=151 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/2m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=153 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/2m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=155 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': 'Switcheroo', 'destination': None, 'flow_source': 'None', 'restricted_bf': 0, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/2m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=156 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/2m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=157 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/2m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=158 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/2m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=159 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/2m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=161 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/2m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=162 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/2m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=164 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': 'Lightning Rush', 'destination': None, 'flow_source': 'Printed', 'restricted_bf': None, 'source_zone': 'Trash', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/6m P2:1u/2m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=165 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/6m P2:1u/2m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=167 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/6m P2:1u/2m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=168 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/6m P2:1u/2m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=171 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': 'Lightning Rush', 'destination': None, 'flow_source': 'Printed', 'restricted_bf': None, 'source_zone': 'Trash', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/6m P2:1u/2m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=172 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/6m P2:1u/2m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=174 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/6m P2:1u/2m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=175 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/6m P2:1u/2m; Emperor's Dais[None] P1:0u/0m P2:0u/0m

## Notes

- The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.
- Turn ownership is inferred (first non-Closed decision's actor per turn), not read from an explicit field.
- Burns/empowers OMITTED: no decision in this batch carried a burn/empower marker in the assumed schema; nothing to count.
