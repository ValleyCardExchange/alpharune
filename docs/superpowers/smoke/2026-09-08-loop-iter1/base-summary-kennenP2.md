# Batch analysis summary

**The mistake/swing heuristic below is crude by design** — it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.

Games processed: 200  |  skipped (truncated/incomplete): 0  |  skipped (unparseable): 0
Mean turns (overall): 15.595

## Validity

OK — no frozen-state bursts (longest identical run 2, threshold 10).

## Win rate by deck and seat (Wilson 95%)

| Deck | Seat | Wins | Games | Win rate | 95% CI |
|---|---|---|---|---|---|
| kennen_tyler | P2 | 95 | 200 | 47.5% | 40.7% - 54.4% |
| rengar_test | P1 | 105 | 200 | 52.5% | 45.6% - 59.3% |

## Point-tempo curve (mean score / margin by turn)

- **rengar_test::P1** (turns 1..21): score=[0.0, 0.145, 0.415, 0.68, 1.095, 1.505, 1.93, 2.33, 2.825, 3.344, 3.912, 4.419, 5.042, 5.423, 5.748, 5.946, 6.178, 6.409, 6.5, 6.667, 6.0] margin=[0.0, -0.075, -0.125, -0.2, -0.22, -0.23, -0.235, -0.325, -0.305, -0.159, -0.031, 0.091, 0.388, 0.444, 0.346, 0.243, 0.333, 0.318, 0.0, -0.333, -1.0]
- **kennen_tyler::P2** (turns 1..21): score=[0.0, 0.22, 0.54, 0.88, 1.315, 1.735, 2.165, 2.655, 3.13, 3.503, 3.943, 4.328, 4.655, 4.979, 5.402, 5.703, 5.844, 6.091, 6.5, 7.0, 7.0] margin=[0.0, 0.075, 0.125, 0.2, 0.22, 0.23, 0.235, 0.325, 0.305, 0.159, 0.031, -0.091, -0.388, -0.444, -0.346, -0.243, -0.333, -0.318, 0.0, 0.333, 1.0]

## Action-family mix per phase per deck

- **kennen_tyler**
  - Mulligan: {'setup': 200}
  - MainPhase: {'pass': 15643, 'play': 3446, 'choice': 10623, 'activate': 2514, 'move_to_battlefield': 2176, 'move_to_base': 765, 'combat_damage': 1020}
  - ScoringStep: {'activate': 81, 'pass': 183, 'play': 9, 'choice': 50}
- **rengar_test**
  - Mulligan: {'setup': 200}
  - MainPhase: {'play': 2587, 'choice': 10082, 'pass': 15605, 'move_to_battlefield': 2039, 'combat_damage': 1020, 'move_to_base': 1179, 'activate': 646}
  - ScoringStep: {'pass': 182, 'activate': 1}

## Per-game markers

- Flow plays (printed): {'kennen_tyler': 426}
- Flow plays (granted): {'kennen_tyler': 17}
- Tomb-restricted plays: {'kennen_tyler': 33, 'rengar_test': 19}
- Conquers (score increments attributed to actor): {'kennen_tyler': 849, 'rengar_test': 816}
- Reaction plays in the closed state: {'rengar_test': 318, 'kennen_tyler': 192}
- Burns/empowers: OMITTED (log schema carries no marker for these)

## Mistake candidates (losing deck, top 20)

- drop=3 game=game_140_seed_3140.json idx=114 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': 'Ride the Wind', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 7 MainPhase/Open scores P1=3 P2=2 | Emperor's Dais[P2] P1:1u/6m P2:1u/3m; Zaun Warrens[P1] P1:1u/1m P2:0u/0m
- drop=3 game=game_140_seed_3140.json idx=115 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 MainPhase/Open scores P1=3 P2=2 | Emperor's Dais[P2] P1:1u/6m P2:1u/3m; Zaun Warrens[P1] P1:1u/1m P2:0u/0m
- drop=3 game=game_140_seed_3140.json idx=116 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 MainPhase/Open scores P1=3 P2=2 | Emperor's Dais[P2] P1:1u/6m P2:1u/3m; Zaun Warrens[P1] P1:1u/1m P2:0u/0m
- drop=3 game=game_140_seed_3140.json idx=117 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 MainPhase/Open scores P1=3 P2=2 | Emperor's Dais[P2] P1:1u/6m P2:1u/3m; Zaun Warrens[P1] P1:1u/1m P2:0u/0m
- drop=3 game=game_140_seed_3140.json idx=118 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 7 MainPhase/Closed scores P1=3 P2=2 | Emperor's Dais[P2] P1:1u/6m P2:1u/3m; Zaun Warrens[P1] P1:1u/1m P2:0u/0m
- drop=3 game=game_140_seed_3140.json idx=119 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 7 MainPhase/Closed scores P1=3 P2=2 | Emperor's Dais[P2] P1:1u/6m P2:1u/3m; Zaun Warrens[P1] P1:1u/1m P2:0u/0m
- drop=3 game=game_140_seed_3140.json idx=121 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 7 MainPhase/Closed scores P1=3 P2=2 | Emperor's Dais[P2] P1:1u/6m P2:1u/3m; Zaun Warrens[P1] P1:1u/1m P2:0u/0m
- drop=3 game=game_140_seed_3140.json idx=123 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 MainPhase/Closed scores P1=3 P2=2 | Emperor's Dais[P2] P1:1u/6m P2:1u/3m; Zaun Warrens[P1] P1:1u/1m P2:0u/0m
- drop=3 game=game_140_seed_3140.json idx=124 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 7 MainPhase/Open scores P1=3 P2=2 | Emperor's Dais[None] P1:1u/6m P2:0u/0m; Zaun Warrens[P1] P1:1u/1m P2:0u/0m
- drop=3 game=game_140_seed_3140.json idx=125 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 7 MainPhase/Open scores P1=3 P2=2 | Emperor's Dais[None*] P1:1u/6m P2:1u/3m; Zaun Warrens[P1] P1:1u/1m P2:0u/0m
- drop=3 game=game_140_seed_3140.json idx=127 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 7 MainPhase/Open scores P1=3 P2=2 | Emperor's Dais[None*] P1:1u/6m P2:1u/3m; Zaun Warrens[P1] P1:1u/1m P2:0u/0m
- drop=3 game=game_173_seed_3173.json idx=135 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': 'Brynhir Thundersong', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/4m
- drop=3 game=game_173_seed_3173.json idx=136 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/4m
- drop=3 game=game_173_seed_3173.json idx=137 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/4m
- drop=3 game=game_173_seed_3173.json idx=138 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/4m
- drop=3 game=game_173_seed_3173.json idx=139 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/4m
- drop=3 game=game_173_seed_3173.json idx=140 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/4m
- drop=3 game=game_173_seed_3173.json idx=141 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/4m
- drop=3 game=game_173_seed_3173.json idx=142 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/4m
- drop=3 game=game_173_seed_3173.json idx=143 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/4m

## Swing decisions (winning deck, top 20)

- rise=4 game=game_49_seed_3049.json idx=214 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': {'id': 1, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 12 MainPhase/Open scores P1=3 P2=3 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_49_seed_3049.json idx=215 turn=12 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 12 MainPhase/Open scores P1=3 P2=3 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Zaun Warrens[None*] P1:0u/0m P2:1u/3m
- rise=3 game=game_101_seed_3101.json idx=19 turn=2 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 0] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 2 MainPhase/Open scores P1=0 P2=0 | Star Spring[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=3 game=game_101_seed_3101.json idx=20 turn=2 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 2 MainPhase/Open scores P1=0 P2=0 | Star Spring[None*] P1:0u/0m P2:1u/2m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=3 game=game_101_seed_3101.json idx=123 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 6 MainPhase/Closed scores P1=1 P2=4 | Star Spring[P1] P1:1u/4m P2:0u/0m; Sandswept Tomb[P2] P1:0u/0m P2:2u/2m
- rise=3 game=game_101_seed_3101.json idx=125 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 6 MainPhase/Closed scores P1=1 P2=4 | Star Spring[P1] P1:1u/4m P2:0u/0m; Sandswept Tomb[P2] P1:0u/0m P2:2u/2m
- rise=3 game=game_101_seed_3101.json idx=126 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': {'id': 1, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 6 MainPhase/Open scores P1=1 P2=4 | Star Spring[P1] P1:1u/4m P2:0u/0m; Sandswept Tomb[P2] P1:0u/0m P2:2u/2m
- rise=3 game=game_101_seed_3101.json idx=127 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateAbility'}
  board: turn 6 MainPhase/Open scores P1=1 P2=4 | Star Spring[P1] P1:1u/4m P2:0u/0m; Sandswept Tomb[P2] P1:0u/0m P2:3u/4m
- rise=3 game=game_101_seed_3101.json idx=128 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 6 MainPhase/Open scores P1=1 P2=4 | Star Spring[P1] P1:1u/4m P2:0u/0m; Sandswept Tomb[P2] P1:0u/0m P2:3u/4m
- rise=3 game=game_101_seed_3101.json idx=129 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 6 MainPhase/Open scores P1=1 P2=4 | Star Spring[P1*] P1:1u/4m P2:1u/6m; Sandswept Tomb[P2] P1:0u/0m P2:3u/4m
- rise=3 game=game_101_seed_3101.json idx=131 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 6 MainPhase/Open scores P1=1 P2=4 | Star Spring[P1*] P1:1u/4m P2:1u/6m; Sandswept Tomb[P2] P1:0u/0m P2:3u/4m
- rise=3 game=game_106_seed_3106.json idx=254 turn=11 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 5] chosen={'card': None, 'destination': {'id': 1, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 11 MainPhase/Open scores P1=4 P2=5 | Treasure Hoard[P1] P1:1u/5m P2:0u/0m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=3 game=game_106_seed_3106.json idx=255 turn=11 deck=rengar_test phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 5] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 11 MainPhase/Open scores P1=4 P2=5 | Treasure Hoard[P1] P1:1u/5m P2:0u/0m; Zaun Warrens[None*] P1:1u/6m P2:0u/0m
- rise=3 game=game_108_seed_3108.json idx=142 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': {'id': 1, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Star Spring[P1] P1:1u/4m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=3 game=game_108_seed_3108.json idx=143 turn=8 deck=rengar_test phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Star Spring[P1] P1:1u/4m P2:0u/0m; Sandswept Tomb[None*] P1:1u/4m P2:0u/0m
- rise=3 game=game_109_seed_3109.json idx=198 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': 'Up from the Deep', 'destination': None, 'flow_source': 'Printed', 'restricted_bf': None, 'source_zone': 'Trash', 'type': 'PlayCard'}
  board: turn 10 MainPhase/Open scores P1=3 P2=2 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P1] P1:2u/7m P2:0u/0m
- rise=3 game=game_109_seed_3109.json idx=199 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 10 MainPhase/Closed scores P1=3 P2=2 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P1] P1:2u/7m P2:0u/0m
- rise=3 game=game_109_seed_3109.json idx=200 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=3 P2=2 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P1] P1:2u/7m P2:0u/0m
- rise=3 game=game_109_seed_3109.json idx=202 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=3 P2=2 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P1] P1:2u/7m P2:0u/0m
- rise=3 game=game_109_seed_3109.json idx=204 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=3 P2=2 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P1] P1:2u/7m P2:0u/0m

## Notes

- The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.
- Turn ownership is inferred (first non-Closed decision's actor per turn), not read from an explicit field.
- Burns/empowers OMITTED: no decision in this batch carried a burn/empower marker in the assumed schema; nothing to count.
