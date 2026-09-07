# Batch analysis summary

**The mistake/swing heuristic below is crude by design** — it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.

Games processed: 200  |  skipped (truncated/incomplete): 0  |  skipped (unparseable): 0
Mean turns (overall): 15.985

## Validity

OK — no frozen-state bursts (longest identical run 2, threshold 10).

## Win rate by deck and seat (Wilson 95%)

| Deck | Seat | Wins | Games | Win rate | 95% CI |
|---|---|---|---|---|---|
| rengar_test | P2 | 104 | 200 | 52.0% | 45.1% - 58.8% |
| kennen_tyler | P1 | 96 | 200 | 48.0% | 41.2% - 54.9% |

## Point-tempo curve (mean score / margin by turn)

- **kennen_tyler::P1** (turns 1..23): score=[0.0, 0.22, 0.515, 0.87, 1.195, 1.645, 2.03, 2.54, 2.965, 3.492, 3.934, 4.401, 4.705, 4.901, 5.034, 5.265, 5.217, 5.367, 5.545, 5.833, 6.0, 5.333, 7.0] margin=[0.0, 0.08, 0.09, 0.185, 0.105, 0.15, 0.05, 0.17, 0.11, 0.176, 0.112, 0.146, -0.165, -0.483, -0.807, -0.783, -0.87, -0.8, -0.773, -0.417, -0.167, -1.333, 0.0]
- **rengar_test::P2** (turns 1..23): score=[0.0, 0.14, 0.425, 0.685, 1.09, 1.495, 1.98, 2.37, 2.855, 3.317, 3.822, 4.255, 4.869, 5.384, 5.84, 6.048, 6.087, 6.167, 6.318, 6.25, 6.167, 6.667, 7.0] margin=[0.0, -0.08, -0.09, -0.185, -0.105, -0.15, -0.05, -0.17, -0.11, -0.176, -0.112, -0.146, 0.165, 0.483, 0.807, 0.783, 0.87, 0.8, 0.773, 0.417, 0.167, 1.333, 0.0]

## Action-family mix per phase per deck

- **kennen_tyler**
  - Mulligan: {'setup': 200}
  - MainPhase: {'play': 3675, 'choice': 11138, 'pass': 16945, 'activate': 2720, 'move_to_battlefield': 2308, 'combat_damage': 1382, 'move_to_base': 604}
  - ScoringStep: {'play': 10, 'choice': 62, 'pass': 189, 'activate': 70}
- **rengar_test**
  - Mulligan: {'setup': 200}
  - MainPhase: {'pass': 16865, 'play': 2699, 'choice': 10446, 'move_to_battlefield': 2375, 'move_to_base': 1275, 'combat_damage': 1382, 'activate': 716}
  - ScoringStep: {'pass': 189}

## Per-game markers

- Flow plays (printed): {'kennen_tyler': 493}
- Flow plays (granted): {'kennen_tyler': 23}
- Tomb-restricted plays: {'rengar_test': 27, 'kennen_tyler': 23}
- Conquers (score increments attributed to actor): {'rengar_test': 867, 'kennen_tyler': 887}
- Reaction plays in the closed state: {'rengar_test': 373, 'kennen_tyler': 223}
- Burns/empowers: OMITTED (log schema carries no marker for these)

## Mistake candidates (losing deck, top 25)

- drop=3 game=game_179_seed_2179.json idx=178 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': 'Fresh Beans', 'destination': {'player': 'P2', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=179 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=180 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=181 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': 'Inferna', 'destination': {'player': 'P2', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=182 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=183 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=184 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=186 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=188 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': 'Noxus Hopeful', 'destination': {'player': 'P2', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=189 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=190 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=191 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=193 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=195 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': 'Challenge', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=196 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=197 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=198 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=199 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=201 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=202 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Closed scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=203 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=204 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1*] P1:1u/8m P2:1u/5m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=206 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1*] P1:1u/8m P2:1u/5m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=208 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1] P1:1u/8m P2:0u/0m; Emperor's Dais[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_179_seed_2179.json idx=209 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 10 MainPhase/Open scores P1=2 P2=3 | Sandswept Tomb[P1*] P1:1u/8m P2:1u/4m; Emperor's Dais[None] P1:0u/0m P2:0u/0m

## Swing decisions (winning deck, top 25)

- rise=4 game=game_102_seed_2102.json idx=105 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': 'Last Rites', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=106 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=107 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=108 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=109 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': 'Lightning Rush', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=110 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=111 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=113 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=114 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': {'id': 1, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 8 MainPhase/Open scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=115 turn=8 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': 'Stacked Deck', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayActionCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None*] P1:1u/6m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=116 turn=8 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None*] P1:1u/6m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=117 turn=8 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None*] P1:1u/6m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=119 turn=8 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None*] P1:1u/6m P2:0u/0m
- rise=4 game=game_102_seed_2102.json idx=121 turn=8 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 8 MainPhase/Open scores P1=2 P2=0 | Zaun Warrens[None] P1:0u/0m P2:0u/0m; Star Spring[None*] P1:1u/6m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=134 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': 'Fizz, Trickster', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=135 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=136 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=137 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=138 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=139 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=141 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=142 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=144 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': 'Up from the Deep', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=145 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=146 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m

## Notes

- The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.
- Turn ownership is inferred (first non-Closed decision's actor per turn), not read from an explicit field.
- Burns/empowers OMITTED: no decision in this batch carried a burn/empower marker in the assumed schema; nothing to count.
