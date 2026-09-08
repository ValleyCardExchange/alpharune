# Batch analysis summary

**The mistake/swing heuristic below is crude by design** — it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.

Games processed: 200  |  skipped (truncated/incomplete): 0  |  skipped (unparseable): 0
Mean turns (overall): 15.01

## Validity

OK — no frozen-state bursts (longest identical run 2, threshold 10).

## Win rate by deck and seat (Wilson 95%)

| Deck | Seat | Wins | Games | Win rate | 95% CI |
|---|---|---|---|---|---|
| kennen_tyler | P1 | 119 | 200 | 59.5% | 52.6% - 66.1% |
| rengar_test | P2 | 81 | 200 | 40.5% | 33.9% - 47.4% |

## Point-tempo curve (mean score / margin by turn)

- **kennen_tyler::P1** (turns 1..21): score=[0.0, 0.355, 0.705, 1.285, 1.745, 2.295, 2.765, 3.335, 3.87, 4.426, 4.859, 5.277, 5.578, 5.823, 5.989, 6.158, 6.3, 6.316, 6.8, 6.667, 7.0] margin=[0.0, 0.225, 0.255, 0.59, 0.72, 0.915, 0.945, 1.12, 1.175, 1.244, 1.146, 1.046, 0.734, 0.411, 0.114, 0.158, -0.1, -0.263, 0.8, 0.333, 0.0]
- **rengar_test::P2** (turns 1..21): score=[0.0, 0.13, 0.45, 0.695, 1.025, 1.38, 1.82, 2.215, 2.695, 3.183, 3.714, 4.231, 4.844, 5.411, 5.875, 6.0, 6.4, 6.579, 6.0, 6.333, 7.0] margin=[0.0, -0.225, -0.255, -0.59, -0.72, -0.915, -0.945, -1.12, -1.175, -1.244, -1.146, -1.046, -0.734, -0.411, -0.114, -0.158, 0.1, 0.263, -0.8, -0.333, 0.0]

## Action-family mix per phase per deck

- **kennen_tyler**
  - Mulligan: {'setup': 200}
  - MainPhase: {'play': 3492, 'choice': 10471, 'pass': 16416, 'activate': 2489, 'move_to_battlefield': 2431, 'move_to_base': 795, 'combat_damage': 1129}
  - ScoringStep: {'activate': 55, 'pass': 159, 'choice': 45, 'play': 6}
- **rengar_test**
  - Mulligan: {'setup': 200}
  - MainPhase: {'pass': 17148, 'play': 2558, 'choice': 9802, 'move_to_battlefield': 1832, 'move_to_base': 979, 'combat_damage': 1129, 'activate': 618}
  - ScoringStep: {'pass': 159}

## Per-game markers

- Flow plays (printed): {'kennen_tyler': 513}
- Flow plays (granted): {'kennen_tyler': 40}
- Tomb-restricted plays: {'kennen_tyler': 52, 'rengar_test': 26}
- Conquers (score increments attributed to actor): {'rengar_test': 754, 'kennen_tyler': 895}
- Reaction plays in the closed state: {'rengar_test': 325, 'kennen_tyler': 193}
- Burns/empowers: OMITTED (log schema carries no marker for these)

## Mistake candidates (losing deck, top 20)

- drop=3 game=game_124_seed_2124.json idx=25 turn=2 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 0] chosen={'card': 'Irresistible Faefolk', 'destination': {'player': 'P2', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 2 MainPhase/Open scores P1=0 P2=0 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_124_seed_2124.json idx=26 turn=2 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 2 MainPhase/Open scores P1=0 P2=0 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_124_seed_2124.json idx=27 turn=2 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 2 MainPhase/Open scores P1=0 P2=0 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_124_seed_2124.json idx=28 turn=2 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[0, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 2 MainPhase/Closed scores P1=0 P2=0 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_124_seed_2124.json idx=30 turn=2 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 0] chosen={'card': None, 'destination': {'id': 1, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 2 MainPhase/Open scores P1=0 P2=0 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_124_seed_2124.json idx=31 turn=2 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[0, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 2 MainPhase/Closed scores P1=0 P2=0 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None*] P1:0u/0m P2:1u/1m
- drop=3 game=game_124_seed_2124.json idx=33 turn=2 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[0, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 2 MainPhase/Closed scores P1=0 P2=0 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None*] P1:0u/0m P2:1u/1m
- drop=3 game=game_124_seed_2124.json idx=34 turn=2 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 2 MainPhase/Open scores P1=0 P2=0 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None*] P1:1u/4m P2:1u/1m
- drop=3 game=game_124_seed_2124.json idx=36 turn=2 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 2 MainPhase/Open scores P1=0 P2=0 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None*] P1:1u/4m P2:1u/1m
- drop=3 game=game_170_seed_2170.json idx=125 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': {'id': 1, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_170_seed_2170.json idx=126 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': 'Star-Crossed', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2*] P1:1u/1m P2:1u/6m
- drop=3 game=game_170_seed_2170.json idx=127 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2*] P1:1u/1m P2:1u/6m
- drop=3 game=game_170_seed_2170.json idx=128 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2*] P1:1u/1m P2:1u/6m
- drop=3 game=game_170_seed_2170.json idx=129 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2*] P1:1u/1m P2:1u/6m
- drop=3 game=game_170_seed_2170.json idx=130 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2*] P1:1u/1m P2:1u/6m
- drop=3 game=game_170_seed_2170.json idx=131 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': 'Hard Bargain', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayReaction'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2*] P1:1u/1m P2:1u/6m
- drop=3 game=game_170_seed_2170.json idx=132 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2*] P1:1u/1m P2:1u/6m
- drop=3 game=game_170_seed_2170.json idx=133 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2*] P1:1u/1m P2:1u/6m
- drop=3 game=game_170_seed_2170.json idx=134 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2*] P1:1u/1m P2:1u/6m
- drop=3 game=game_170_seed_2170.json idx=135 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2*] P1:1u/1m P2:1u/6m

## Swing decisions (winning deck, top 20)

- rise=4 game=game_133_seed_2133.json idx=152 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2] P1:0u/0m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=153 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': 'The Harrowing', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=154 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=155 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=156 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=157 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=158 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=159 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=160 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=161 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=162 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=164 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=165 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=168 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': 'Switcheroo', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=169 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=170 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=171 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=172 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=173 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_133_seed_2133.json idx=175 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P2*] P1:1u/1m P2:1u/6m; Emperor's Dais[None] P1:0u/0m P2:0u/0m

## Notes

- The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.
- Turn ownership is inferred (first non-Closed decision's actor per turn), not read from an explicit field.
- Burns/empowers OMITTED: no decision in this batch carried a burn/empower marker in the assumed schema; nothing to count.
