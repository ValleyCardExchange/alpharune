# Batch analysis summary

**The mistake/swing heuristic below is crude by design** — it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.

Games processed: 200  |  skipped (truncated/incomplete): 0  |  skipped (unparseable): 0
Mean turns (overall): 15.835

## Validity

OK — no frozen-state bursts (longest identical run 2, threshold 10).

## Win rate by deck and seat (Wilson 95%)

| Deck | Seat | Wins | Games | Win rate | 95% CI |
|---|---|---|---|---|---|
| kennen_tyler | P2 | 93 | 200 | 46.5% | 39.7% - 53.4% |
| rengar_test | P1 | 107 | 200 | 53.5% | 46.6% - 60.3% |

## Point-tempo curve (mean score / margin by turn)

- **rengar_test::P1** (turns 1..21): score=[0.0, 0.18, 0.47, 0.8, 1.145, 1.53, 1.98, 2.455, 2.935, 3.432, 3.929, 4.359, 4.965, 5.397, 5.767, 6.112, 6.234, 6.607, 6.765, 6.833, 7.0] margin=[0.0, -0.065, -0.1, -0.135, -0.175, -0.205, -0.235, -0.18, -0.215, -0.211, -0.153, -0.12, 0.104, 0.179, 0.242, 0.45, 0.213, 0.357, 0.471, 0.667, 2.0]
- **kennen_tyler::P2** (turns 1..21): score=[0.0, 0.245, 0.57, 0.935, 1.32, 1.735, 2.215, 2.635, 3.15, 3.643, 4.082, 4.478, 4.861, 5.219, 5.525, 5.662, 6.021, 6.25, 6.294, 6.167, 5.0] margin=[0.0, 0.065, 0.1, 0.135, 0.175, 0.205, 0.235, 0.18, 0.215, 0.211, 0.153, 0.12, -0.104, -0.179, -0.242, -0.45, -0.213, -0.357, -0.471, -0.667, -2.0]

## Action-family mix per phase per deck

- **kennen_tyler**
  - Mulligan: {'setup': 200}
  - MainPhase: {'pass': 16552, 'play': 3561, 'choice': 10906, 'move_to_battlefield': 2253, 'activate': 2597, 'combat_damage': 1347, 'move_to_base': 576}
  - ScoringStep: {'pass': 165, 'choice': 37, 'activate': 71, 'play': 5}
- **rengar_test**
  - Mulligan: {'setup': 200}
  - MainPhase: {'play': 2650, 'choice': 10335, 'pass': 16603, 'move_to_battlefield': 2369, 'combat_damage': 1347, 'move_to_base': 1226, 'activate': 693}
  - ScoringStep: {'pass': 165}

## Per-game markers

- Flow plays (printed): {'kennen_tyler': 469}
- Flow plays (granted): {'kennen_tyler': 16}
- Tomb-restricted plays: {'kennen_tyler': 19, 'rengar_test': 21}
- Conquers (score increments attributed to actor): {'rengar_test': 884, 'kennen_tyler': 880}
- Reaction plays in the closed state: {'rengar_test': 318, 'kennen_tyler': 219}
- Burns/empowers: OMITTED (log schema carries no marker for these)

## Mistake candidates (losing deck, top 20)

- drop=4 game=game_170_seed_3170.json idx=136 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': 'Rengar, Trophy Hunter', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/4m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:1u/12m
- drop=4 game=game_170_seed_3170.json idx=137 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/4m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:1u/12m
- drop=4 game=game_170_seed_3170.json idx=138 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/4m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:1u/12m
- drop=4 game=game_170_seed_3170.json idx=139 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/4m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:1u/12m
- drop=4 game=game_170_seed_3170.json idx=140 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/4m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:1u/12m
- drop=4 game=game_170_seed_3170.json idx=141 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/4m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:1u/12m
- drop=4 game=game_170_seed_3170.json idx=142 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/4m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:1u/12m
- drop=4 game=game_170_seed_3170.json idx=143 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/4m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:1u/12m
- drop=4 game=game_170_seed_3170.json idx=145 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': {'id': 2, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 8 MainPhase/Open scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/5m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:1u/12m
- drop=4 game=game_170_seed_3170.json idx=146 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 8 MainPhase/Open scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/5m P2:0u/0m; Baron Pit[None*] P1:1u/6m P2:1u/12m
- drop=4 game=game_170_seed_3170.json idx=148 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 8 MainPhase/Open scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/5m P2:0u/0m; Baron Pit[None*] P1:1u/6m P2:1u/12m
- drop=3 game=game_170_seed_3170.json idx=150 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': {'id': 2, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 8 MainPhase/Open scores P1=4 P2=4 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/5m P2:0u/0m; Baron Pit[P2] P1:0u/0m P2:1u/12m
- drop=3 game=game_170_seed_3170.json idx=151 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 8 MainPhase/Open scores P1=4 P2=4 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:1u/3m P2:0u/0m; Baron Pit[P2*] P1:1u/4m P2:1u/12m
- drop=3 game=game_170_seed_3170.json idx=153 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 8 MainPhase/Open scores P1=4 P2=4 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:1u/3m P2:0u/0m; Baron Pit[P2*] P1:1u/4m P2:1u/12m
- drop=3 game=game_170_seed_3170.json idx=155 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': {'id': 2, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 8 MainPhase/Open scores P1=4 P2=4 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:1u/3m P2:0u/0m; Baron Pit[P2] P1:0u/0m P2:1u/12m
- drop=3 game=game_170_seed_3170.json idx=156 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 8 MainPhase/Open scores P1=4 P2=4 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Baron Pit[P2*] P1:1u/3m P2:1u/12m
- drop=3 game=game_170_seed_3170.json idx=158 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 8 MainPhase/Open scores P1=4 P2=4 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Baron Pit[P2*] P1:1u/3m P2:1u/12m
- drop=3 game=game_170_seed_3170.json idx=160 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'EndTurn'}
  board: turn 8 MainPhase/Open scores P1=4 P2=4 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Baron Pit[P2] P1:0u/0m P2:1u/12m
- drop=3 game=game_174_seed_3174.json idx=57 turn=4 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': {'id': 1, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 4 MainPhase/Open scores P1=2 P2=0 | Star Spring[P1] P1:1u/1m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_174_seed_3174.json idx=58 turn=4 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 4 MainPhase/Closed scores P1=2 P2=0 | Star Spring[P1] P1:1u/1m P2:0u/0m; Minefield[None*] P1:1u/1m P2:0u/0m

## Swing decisions (winning deck, top 20)

- rise=4 game=game_170_seed_3170.json idx=114 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 7 MainPhase/Open scores P1=3 P2=2 | Star Spring[None] P1:0u/0m P2:0u/0m; Sandswept Tomb[P1] P1:2u/4m P2:0u/0m
- rise=4 game=game_170_seed_3170.json idx=115 turn=7 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 7 MainPhase/Open scores P1=3 P2=2 | Star Spring[None*] P1:0u/0m P2:1u/6m; Sandswept Tomb[P1] P1:2u/4m P2:0u/0m
- rise=4 game=game_170_seed_3170.json idx=144 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/4m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:1u/12m
- rise=4 game=game_170_seed_3170.json idx=147 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 8 MainPhase/Open scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/5m P2:0u/0m; Baron Pit[None*] P1:1u/6m P2:1u/12m
- rise=4 game=game_170_seed_3170.json idx=149 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 8 MainPhase/Open scores P1=4 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/8m; Sandswept Tomb[P1] P1:2u/5m P2:0u/0m; Baron Pit[None*] P1:1u/6m P2:1u/12m
- rise=4 game=game_194_seed_3194.json idx=127 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': 'Nidalee, Cat Form', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=3 P2=2 | Star Spring[P2] P1:0u/0m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=128 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=3 P2=2 | Star Spring[P2] P1:0u/0m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=129 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=3 P2=2 | Star Spring[P2] P1:0u/0m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=130 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=3 P2=2 | Star Spring[P2] P1:0u/0m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=131 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=3 P2=2 | Star Spring[P2] P1:0u/0m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=132 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': 'Rengar, Trophy Hunter', 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayReaction'}
  board: turn 8 MainPhase/Closed scores P1=3 P2=2 | Star Spring[P2] P1:0u/0m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=133 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=3 P2=2 | Star Spring[P2] P1:0u/0m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=134 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=3 P2=2 | Star Spring[P2] P1:0u/0m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=135 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=3 P2=2 | Star Spring[P2] P1:0u/0m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=136 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=3 P2=2 | Star Spring[P2] P1:0u/0m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=137 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=3 P2=2 | Star Spring[P2] P1:0u/0m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=138 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=3 P2=2 | Star Spring[P2] P1:0u/0m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=139 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=3 P2=2 | Star Spring[P2*] P1:1u/6m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=141 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=3 P2=2 | Star Spring[P2*] P1:1u/7m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_194_seed_3194.json idx=143 turn=8 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 8 MainPhase/Open scores P1=3 P2=2 | Star Spring[P2*] P1:1u/8m P2:2u/7m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m

## Notes

- The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.
- Turn ownership is inferred (first non-Closed decision's actor per turn), not read from an explicit field.
- Burns/empowers OMITTED: no decision in this batch carried a burn/empower marker in the assumed schema; nothing to count.
