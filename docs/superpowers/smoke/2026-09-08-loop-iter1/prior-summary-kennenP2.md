# Batch analysis summary

**The mistake/swing heuristic below is crude by design** — it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.

Games processed: 200  |  skipped (truncated/incomplete): 0  |  skipped (unparseable): 0
Mean turns (overall): 15.38

## Validity

OK — no frozen-state bursts (longest identical run 2, threshold 10).

## Win rate by deck and seat (Wilson 95%)

| Deck | Seat | Wins | Games | Win rate | 95% CI |
|---|---|---|---|---|---|
| rengar_test | P1 | 114 | 200 | 57.0% | 50.1% - 63.7% |
| kennen_tyler | P2 | 86 | 200 | 43.0% | 36.3% - 49.9% |

## Point-tempo curve (mean score / margin by turn)

- **rengar_test::P1** (turns 1..20): score=[0.0, 0.145, 0.42, 0.695, 1.12, 1.52, 1.96, 2.39, 2.875, 3.404, 4.0, 4.401, 4.963, 5.468, 5.653, 6.152, 6.167, 6.684, 6.727, 6.0] margin=[0.0, -0.075, -0.13, -0.195, -0.215, -0.225, -0.23, -0.25, -0.255, -0.157, 0.09, 0.062, 0.226, 0.447, 0.242, 0.485, 0.361, 0.947, 0.364, -1.0]
- **kennen_tyler::P2** (turns 1..20): score=[0.0, 0.22, 0.55, 0.89, 1.335, 1.745, 2.19, 2.64, 3.13, 3.561, 3.91, 4.339, 4.738, 5.021, 5.411, 5.667, 5.806, 5.737, 6.364, 7.0] margin=[0.0, 0.075, 0.13, 0.195, 0.215, 0.225, 0.23, 0.25, 0.255, 0.157, -0.09, -0.062, -0.226, -0.447, -0.242, -0.485, -0.361, -0.947, -0.364, 1.0]

## Action-family mix per phase per deck

- **kennen_tyler**
  - Mulligan: {'setup': 200}
  - MainPhase: {'pass': 15407, 'play': 3362, 'choice': 10501, 'activate': 2404, 'move_to_battlefield': 2071, 'combat_damage': 1000, 'move_to_base': 676}
  - ScoringStep: {'play': 5, 'choice': 39, 'pass': 160, 'activate': 64}
- **rengar_test**
  - Mulligan: {'setup': 200}
  - MainPhase: {'play': 2559, 'choice': 10033, 'pass': 15349, 'move_to_battlefield': 1930, 'combat_damage': 1000, 'move_to_base': 1122, 'activate': 602}
  - ScoringStep: {'pass': 160}

## Per-game markers

- Flow plays (printed): {'kennen_tyler': 408}
- Flow plays (granted): {'kennen_tyler': 18}
- Tomb-restricted plays: {'kennen_tyler': 39, 'rengar_test': 16}
- Conquers (score increments attributed to actor): {'kennen_tyler': 819, 'rengar_test': 819}
- Reaction plays in the closed state: {'rengar_test': 337, 'kennen_tyler': 202}
- Burns/empowers: OMITTED (log schema carries no marker for these)

## Mistake candidates (losing deck, top 20)

- drop=3 game=game_173_seed_3173.json idx=141 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': 'Rengar, Trophy Hunter', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[P1] P1:1u/3m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_173_seed_3173.json idx=142 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[P1] P1:1u/3m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_173_seed_3173.json idx=143 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[P1] P1:1u/3m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_173_seed_3173.json idx=144 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[P1] P1:1u/3m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_173_seed_3173.json idx=145 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[P1] P1:1u/3m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_173_seed_3173.json idx=146 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[P1] P1:1u/3m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_173_seed_3173.json idx=147 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[P1] P1:1u/3m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_173_seed_3173.json idx=148 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 10 MainPhase/Closed scores P1=4 P2=1 | Star Spring[P1] P1:1u/3m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_173_seed_3173.json idx=149 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=4 P2=1 | Star Spring[P1] P1:1u/3m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_173_seed_3173.json idx=151 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=4 P2=1 | Star Spring[P1] P1:1u/3m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_173_seed_3173.json idx=153 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=4 P2=1 | Star Spring[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_173_seed_3173.json idx=155 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[P1] P1:1u/4m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_173_seed_3173.json idx=156 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'EndTurn'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None] P1:0u/0m P2:0u/0m; Minefield[P2] P1:0u/0m P2:1u/6m
- drop=3 game=game_178_seed_3178.json idx=157 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 0] chosen={'card': 'Nocturne, Horrifying', 'destination': {'player': 'P2', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 9 MainPhase/Open scores P1=3 P2=0 | Emperor's Dais[None] P1:1u/2m P2:0u/0m; Minefield[P1] P1:1u/4m P2:0u/0m
- drop=3 game=game_178_seed_3178.json idx=158 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=3 P2=0 | Emperor's Dais[None] P1:1u/2m P2:0u/0m; Minefield[P1] P1:1u/4m P2:0u/0m
- drop=3 game=game_178_seed_3178.json idx=159 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=3 P2=0 | Emperor's Dais[None] P1:1u/2m P2:0u/0m; Minefield[P1] P1:1u/4m P2:0u/0m
- drop=3 game=game_178_seed_3178.json idx=160 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=3 P2=0 | Emperor's Dais[None] P1:1u/2m P2:0u/0m; Minefield[P1] P1:1u/4m P2:0u/0m
- drop=3 game=game_178_seed_3178.json idx=161 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=3 P2=0 | Emperor's Dais[None] P1:1u/2m P2:0u/0m; Minefield[P1] P1:1u/4m P2:0u/0m
- drop=3 game=game_178_seed_3178.json idx=162 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=3 P2=0 | Emperor's Dais[None] P1:1u/2m P2:0u/0m; Minefield[P1] P1:1u/4m P2:0u/0m
- drop=3 game=game_178_seed_3178.json idx=163 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 0] chosen={'card': 'Switcheroo', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 9 MainPhase/Open scores P1=3 P2=0 | Emperor's Dais[None] P1:1u/2m P2:0u/0m; Minefield[P1] P1:1u/4m P2:0u/0m

## Swing decisions (winning deck, top 20)

- rise=4 game=game_121_seed_3121.json idx=73 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': 'Tideturner', 'destination': {'player': 'P2', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 5 MainPhase/Open scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1] P1:1u/1m P2:0u/0m
- rise=4 game=game_121_seed_3121.json idx=74 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 5 MainPhase/Open scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1] P1:1u/1m P2:0u/0m
- rise=4 game=game_121_seed_3121.json idx=75 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 5 MainPhase/Open scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1] P1:1u/1m P2:0u/0m
- rise=4 game=game_121_seed_3121.json idx=76 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 5 MainPhase/Closed scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1] P1:1u/1m P2:0u/0m
- rise=4 game=game_121_seed_3121.json idx=78 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': 'Lightning Rush', 'destination': None, 'flow_source': 'Printed', 'restricted_bf': None, 'source_zone': 'Trash', 'type': 'PlayCard'}
  board: turn 5 MainPhase/Open scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1] P1:1u/1m P2:0u/0m
- rise=4 game=game_121_seed_3121.json idx=79 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 5 MainPhase/Closed scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1] P1:1u/1m P2:0u/0m
- rise=4 game=game_121_seed_3121.json idx=81 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 5 MainPhase/Closed scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1] P1:1u/1m P2:0u/0m
- rise=4 game=game_121_seed_3121.json idx=82 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 5 MainPhase/Closed scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1] P1:1u/1m P2:0u/0m
- rise=4 game=game_121_seed_3121.json idx=84 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 5 MainPhase/Closed scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1] P1:1u/1m P2:0u/0m
- rise=4 game=game_121_seed_3121.json idx=86 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': {'id': 1, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 5 MainPhase/Open scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1] P1:1u/1m P2:0u/0m
- rise=4 game=game_121_seed_3121.json idx=87 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateActionAbility'}
  board: turn 5 MainPhase/Open scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1*] P1:1u/1m P2:1u/6m
- rise=4 game=game_121_seed_3121.json idx=88 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 5 MainPhase/Closed scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1*] P1:1u/1m P2:1u/6m
- rise=4 game=game_121_seed_3121.json idx=90 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 5 MainPhase/Closed scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1*] P1:1u/1m P2:1u/6m
- rise=4 game=game_121_seed_3121.json idx=92 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 5 MainPhase/Open scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1*] P1:1u/1m P2:1u/6m
- rise=4 game=game_121_seed_3121.json idx=93 turn=5 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 5 MainPhase/Open scores P1=1 P2=0 | Emperor's Dais[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1*] P1:1u/1m P2:1u/6m
- rise=4 game=game_127_seed_3127.json idx=213 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[5, 3] chosen={'card': 'Lightning Rush', 'destination': None, 'flow_source': 'Printed', 'restricted_bf': None, 'source_zone': 'Trash', 'type': 'PlayCard'}
  board: turn 10 MainPhase/Open scores P1=5 P2=3 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_127_seed_3127.json idx=214 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[5, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 10 MainPhase/Closed scores P1=5 P2=3 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_127_seed_3127.json idx=215 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[5, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=5 P2=3 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_127_seed_3127.json idx=217 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[5, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=5 P2=3 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_127_seed_3127.json idx=219 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[5, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Closed scores P1=5 P2=3 | Treasure Hoard[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m

## Notes

- The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.
- Turn ownership is inferred (first non-Closed decision's actor per turn), not read from an explicit field.
- Burns/empowers OMITTED: no decision in this batch carried a burn/empower marker in the assumed schema; nothing to count.
