# Batch analysis summary

**The mistake/swing heuristic below is crude by design** — it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.

Games processed: 200  |  skipped (truncated/incomplete): 0  |  skipped (unparseable): 0
Mean turns (overall): 15.35

## Validity

OK — no frozen-state bursts (longest identical run 2, threshold 10).

## Win rate by deck and seat (Wilson 95%)

| Deck | Seat | Wins | Games | Win rate | 95% CI |
|---|---|---|---|---|---|
| rengar_test | P2 | 104 | 200 | 52.0% | 45.1% - 58.8% |
| kennen_tyler | P1 | 96 | 200 | 48.0% | 41.2% - 54.9% |

## Point-tempo curve (mean score / margin by turn)

- **kennen_tyler::P1** (turns 1..22): score=[0.0, 0.225, 0.51, 0.9, 1.28, 1.78, 2.19, 2.73, 3.16, 3.652, 4.032, 4.431, 4.819, 5.086, 5.359, 5.677, 6.0, 6.579, 6.667, 6.667, 6.5, 7.0] margin=[0.0, 0.065, 0.03, 0.165, 0.145, 0.285, 0.26, 0.37, 0.27, 0.328, 0.175, 0.144, 0.075, -0.129, -0.204, -0.062, -0.051, 0.684, 0.333, 1.0, 0.5, 2.0]
- **rengar_test::P2** (turns 1..22): score=[0.0, 0.16, 0.48, 0.735, 1.135, 1.495, 1.93, 2.36, 2.89, 3.323, 3.857, 4.287, 4.744, 5.216, 5.563, 5.738, 6.051, 5.895, 6.333, 5.667, 6.0, 5.0] margin=[0.0, -0.065, -0.03, -0.165, -0.145, -0.285, -0.26, -0.37, -0.27, -0.328, -0.175, -0.144, -0.075, 0.129, 0.204, 0.062, 0.051, -0.684, -0.333, -1.0, -0.5, -2.0]

## Action-family mix per phase per deck

- **kennen_tyler**
  - Mulligan: {'setup': 200}
  - MainPhase: {'play': 3429, 'choice': 10632, 'pass': 15595, 'activate': 2423, 'move_to_battlefield': 2150, 'combat_damage': 1033, 'move_to_base': 697}
  - ScoringStep: {'activate': 90, 'pass': 219, 'choice': 43, 'play': 6}
- **rengar_test**
  - Mulligan: {'setup': 200}
  - MainPhase: {'pass': 15594, 'play': 2560, 'choice': 9848, 'move_to_battlefield': 1974, 'move_to_base': 1138, 'combat_damage': 1033, 'activate': 641}
  - ScoringStep: {'pass': 218, 'play': 1, 'choice': 6}

## Per-game markers

- Flow plays (printed): {'kennen_tyler': 449}
- Flow plays (granted): {'kennen_tyler': 32}
- Tomb-restricted plays: {'kennen_tyler': 28, 'rengar_test': 14}
- Conquers (score increments attributed to actor): {'rengar_test': 805, 'kennen_tyler': 835}
- Reaction plays in the closed state: {'rengar_test': 364, 'kennen_tyler': 203}
- Burns/empowers: OMITTED (log schema carries no marker for these)

## Mistake candidates (losing deck, top 20)

- drop=3 game=game_116_seed_2116.json idx=320 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': 'Kennen, Storm of Shuriken', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 12 MainPhase/Open scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=321 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=322 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=323 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=324 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=325 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 12 MainPhase/Closed scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=326 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=328 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=330 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': 'Up from the Deep', 'destination': None, 'flow_source': 'Printed', 'restricted_bf': None, 'source_zone': 'Trash', 'type': 'PlayCard'}
  board: turn 12 MainPhase/Open scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=331 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=333 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=335 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': 'Rhasa the Sunderer', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 12 MainPhase/Open scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=336 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=337 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': 'Stacked Deck', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 12 MainPhase/Open scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=338 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Open scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=339 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=341 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Closed scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=342 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=344 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 12 MainPhase/Closed scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m
- drop=3 game=game_116_seed_2116.json idx=345 turn=12 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[7, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 12 MainPhase/Closed scores P1=7 P2=4 | Minefield[P2] P1:0u/0m P2:2u/5m; Treasure Hoard[P1] P1:2u/3m P2:0u/0m

## Swing decisions (winning deck, top 20)

- rise=4 game=game_126_seed_2126.json idx=77 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': 'Fizz, Trickster', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=78 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=79 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=80 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=81 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=82 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=84 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=85 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=87 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': 'Stacked Deck', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=88 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=89 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=91 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=92 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': 'Ride the Wind', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=93 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=94 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=95 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Open scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=96 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=98 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=99 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_126_seed_2126.json idx=101 turn=6 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 6 MainPhase/Closed scores P1=2 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Star Spring[None] P1:0u/0m P2:0u/0m

## Notes

- The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.
- Turn ownership is inferred (first non-Closed decision's actor per turn), not read from an explicit field.
- Burns/empowers OMITTED: no decision in this batch carried a burn/empower marker in the assumed schema; nothing to count.
