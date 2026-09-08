# Batch analysis summary

**The mistake/swing heuristic below is crude by design** — it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.

Games processed: 200  |  skipped (truncated/incomplete): 0  |  skipped (unparseable): 0
Mean turns (overall): 15.365

## Validity

OK — no frozen-state bursts (longest identical run 2, threshold 10).

## Win rate by deck and seat (Wilson 95%)

| Deck | Seat | Wins | Games | Win rate | 95% CI |
|---|---|---|---|---|---|
| rengar_test | P2 | 98 | 200 | 49.0% | 42.2% - 55.9% |
| kennen_tyler | P1 | 102 | 200 | 51.0% | 44.1% - 57.8% |

## Point-tempo curve (mean score / margin by turn)

- **kennen_tyler::P1** (turns 1..22): score=[0.0, 0.23, 0.51, 0.905, 1.265, 1.78, 2.185, 2.76, 3.205, 3.721, 4.099, 4.611, 4.925, 5.323, 5.558, 6.0, 6.242, 6.471, 6.556, 6.6, 7.0, 7.0] margin=[0.0, 0.07, 0.03, 0.16, 0.1, 0.23, 0.24, 0.42, 0.4, 0.472, 0.361, 0.383, 0.233, 0.218, -0.077, 0.127, 0.485, 0.471, 0.333, 0.4, 0.0, 0.0]
- **rengar_test::P2** (turns 1..22): score=[0.0, 0.16, 0.48, 0.745, 1.165, 1.55, 1.945, 2.34, 2.805, 3.249, 3.738, 4.229, 4.692, 5.105, 5.635, 5.873, 5.758, 6.0, 6.222, 6.2, 7.0, 7.0] margin=[0.0, -0.07, -0.03, -0.16, -0.1, -0.23, -0.24, -0.42, -0.4, -0.472, -0.361, -0.383, -0.233, -0.218, 0.077, -0.127, -0.485, -0.471, -0.333, -0.4, 0.0, 0.0]

## Action-family mix per phase per deck

- **kennen_tyler**
  - Mulligan: {'setup': 200}
  - MainPhase: {'play': 3466, 'choice': 10446, 'pass': 15735, 'activate': 2526, 'move_to_battlefield': 2222, 'combat_damage': 1018, 'move_to_base': 786}
  - ScoringStep: {'activate': 99, 'pass': 247, 'choice': 48, 'play': 7}
- **rengar_test**
  - Mulligan: {'setup': 200}
  - MainPhase: {'pass': 15769, 'play': 2591, 'choice': 9999, 'move_to_battlefield': 1955, 'move_to_base': 1110, 'combat_damage': 1018, 'activate': 635}
  - ScoringStep: {'pass': 246, 'play': 1, 'choice': 6}

## Per-game markers

- Flow plays (printed): {'kennen_tyler': 460}
- Flow plays (granted): {'kennen_tyler': 22}
- Tomb-restricted plays: {'kennen_tyler': 30, 'rengar_test': 22}
- Conquers (score increments attributed to actor): {'rengar_test': 791, 'kennen_tyler': 853}
- Reaction plays in the closed state: {'rengar_test': 392, 'kennen_tyler': 206}
- Burns/empowers: OMITTED (log schema carries no marker for these)

## Mistake candidates (losing deck, top 20)

- drop=3 game=game_149_seed_2149.json idx=190 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': 'Stacked Deck', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 9 MainPhase/Open scores P1=1 P2=4 | Minefield[None] P1:0u/0m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=191 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=4 | Minefield[None] P1:0u/0m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=192 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=4 | Minefield[None] P1:0u/0m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=193 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=4 | Minefield[None] P1:0u/0m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=195 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=4 | Minefield[None] P1:0u/0m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=197 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=4 | Minefield[None] P1:0u/0m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=198 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': 'Last Rites', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 9 MainPhase/Open scores P1=1 P2=4 | Minefield[None] P1:0u/0m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=199 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=4 | Minefield[None] P1:0u/0m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=200 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=4 | Minefield[None] P1:0u/0m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=201 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Open scores P1=1 P2=4 | Minefield[None] P1:0u/0m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=202 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 9 MainPhase/Open scores P1=1 P2=4 | Minefield[None] P1:0u/0m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=203 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=4 | Minefield[None*] P1:1u/2m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=205 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 9 MainPhase/Closed scores P1=1 P2=4 | Minefield[None*] P1:1u/2m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=206 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 9 MainPhase/Open scores P1=1 P2=4 | Minefield[None*] P1:1u/2m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_149_seed_2149.json idx=208 turn=9 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 9 MainPhase/Open scores P1=1 P2=4 | Minefield[None*] P1:1u/2m P2:1u/6m; Emperor's Dais[P2] P1:0u/0m P2:1u/1m
- drop=3 game=game_169_seed_2169.json idx=106 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': 'Treasure Hunter', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 7 MainPhase/Open scores P1=0 P2=3 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2] P1:0u/0m P2:2u/7m
- drop=3 game=game_169_seed_2169.json idx=107 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 MainPhase/Open scores P1=0 P2=3 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2] P1:0u/0m P2:2u/7m
- drop=3 game=game_169_seed_2169.json idx=108 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 MainPhase/Open scores P1=0 P2=3 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2] P1:0u/0m P2:2u/7m
- drop=3 game=game_169_seed_2169.json idx=109 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': 'Ride the Wind', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 7 MainPhase/Open scores P1=0 P2=3 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2] P1:0u/0m P2:2u/7m
- drop=3 game=game_169_seed_2169.json idx=110 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 MainPhase/Open scores P1=0 P2=3 | Minefield[None] P1:0u/0m P2:1u/4m; Star Spring[P2] P1:0u/0m P2:2u/7m

## Swing decisions (winning deck, top 20)

- rise=4 game=game_137_seed_2137.json idx=334 turn=15 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 4] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 15 MainPhase/Open scores P1=3 P2=4 | Minefield[P2] P1:0u/0m P2:1u/4m; Star Spring[P2] P1:0u/0m P2:2u/6m
- rise=4 game=game_137_seed_2137.json idx=335 turn=15 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 15 MainPhase/Open scores P1=3 P2=4 | Minefield[P2*] P1:1u/10m P2:1u/4m; Star Spring[P2] P1:0u/0m P2:2u/6m
- rise=4 game=game_137_seed_2137.json idx=337 turn=15 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 15 MainPhase/Open scores P1=3 P2=4 | Minefield[P2*] P1:1u/10m P2:1u/4m; Star Spring[P2] P1:0u/0m P2:2u/6m
- rise=4 game=game_41_seed_2041.json idx=107 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None] P1:0u/0m P2:0u/0m; Emperor's Dais[P2] P1:0u/0m P2:1u/4m
- rise=4 game=game_41_seed_2041.json idx=108 turn=6 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Minefield[None*] P1:1u/6m P2:0u/0m; Emperor's Dais[P2] P1:0u/0m P2:1u/4m
- rise=4 game=game_70_seed_2070.json idx=237 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'ScoringStep'} scores=[3, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 ScoringStep/Closed scores P1=3 P2=3 | Sandswept Tomb[P1] P1:2u/5m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=239 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': 'Lightning Rush', 'destination': None, 'flow_source': 'Printed', 'restricted_bf': None, 'source_zone': 'Trash', 'type': 'PlayCard'}
  board: turn 12 MainPhase/Open scores P1=3 P2=3 | Sandswept Tomb[P1] P1:2u/9m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=240 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=3 P2=3 | Sandswept Tomb[P1] P1:2u/9m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=242 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Closed scores P1=3 P2=3 | Sandswept Tomb[P1] P1:2u/9m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=243 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=3 P2=3 | Sandswept Tomb[P1] P1:2u/9m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=245 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=3 P2=3 | Sandswept Tomb[P1] P1:2u/9m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=247 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': 'Stacked Deck', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 12 MainPhase/Open scores P1=3 P2=3 | Sandswept Tomb[P1] P1:2u/9m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=248 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=3 P2=3 | Sandswept Tomb[P1] P1:2u/9m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=249 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=3 P2=3 | Sandswept Tomb[P1] P1:2u/9m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=251 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Closed scores P1=3 P2=3 | Sandswept Tomb[P1] P1:2u/9m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=252 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 12 MainPhase/Open scores P1=3 P2=3 | Sandswept Tomb[P1] P1:2u/9m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=253 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 12 MainPhase/Open scores P1=3 P2=3 | Sandswept Tomb[P1] P1:3u/17m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=254 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 12 MainPhase/Open scores P1=3 P2=3 | Sandswept Tomb[P1] P1:4u/25m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=255 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 12 MainPhase/Open scores P1=3 P2=3 | Sandswept Tomb[P1] P1:5u/31m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m
- rise=4 game=game_70_seed_2070.json idx=256 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 3] chosen={'card': None, 'destination': {'id': 2, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 12 MainPhase/Open scores P1=3 P2=3 | Sandswept Tomb[P1] P1:6u/36m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:1u/12m P2:0u/0m

## Notes

- The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.
- Turn ownership is inferred (first non-Closed decision's actor per turn), not read from an explicit field.
- Burns/empowers OMITTED: no decision in this batch carried a burn/empower marker in the assumed schema; nothing to count.
