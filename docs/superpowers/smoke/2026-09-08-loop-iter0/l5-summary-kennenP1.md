# Batch analysis summary

**The mistake/swing heuristic below is crude by design** — it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.

Games processed: 200  |  skipped (truncated/incomplete): 0  |  skipped (unparseable): 0
Mean turns (overall): 15.805

## Validity

OK — no frozen-state bursts (longest identical run 2, threshold 10).

## Win rate by deck and seat (Wilson 95%)

| Deck | Seat | Wins | Games | Win rate | 95% CI |
|---|---|---|---|---|---|
| rengar_test | P2 | 100 | 200 | 50.0% | 43.1% - 56.9% |
| kennen_tyler | P1 | 100 | 200 | 50.0% | 43.1% - 56.9% |

## Point-tempo curve (mean score / margin by turn)

- **kennen_tyler::P1** (turns 1..22): score=[0.0, 0.215, 0.5, 0.865, 1.2, 1.645, 2.03, 2.56, 2.995, 3.54, 3.974, 4.389, 4.789, 4.96, 5.366, 5.506, 5.786, 5.963, 6.583, 6.5, 6.0, 7.0] margin=[0.0, 0.075, 0.075, 0.17, 0.105, 0.13, 0.045, 0.155, 0.105, 0.152, 0.046, -0.086, -0.2, -0.544, -0.602, -0.646, -0.571, -0.444, -0.083, -0.5, -1.0, 0.0]
- **rengar_test::P2** (turns 1..22): score=[0.0, 0.14, 0.425, 0.695, 1.095, 1.515, 1.985, 2.405, 2.89, 3.389, 3.929, 4.476, 4.989, 5.503, 5.967, 6.152, 6.357, 6.407, 6.667, 7.0, 7.0, 7.0] margin=[0.0, -0.075, -0.075, -0.17, -0.105, -0.13, -0.045, -0.155, -0.105, -0.152, -0.046, 0.086, 0.2, 0.544, 0.602, 0.646, 0.571, 0.444, 0.083, 0.5, 1.0, 0.0]

## Action-family mix per phase per deck

- **kennen_tyler**
  - Mulligan: {'setup': 200}
  - MainPhase: {'play': 3611, 'choice': 11034, 'pass': 16813, 'activate': 2678, 'move_to_battlefield': 2337, 'combat_damage': 1358, 'move_to_base': 609}
  - ScoringStep: {'pass': 225, 'activate': 93, 'choice': 45, 'play': 9}
- **rengar_test**
  - Mulligan: {'setup': 200}
  - MainPhase: {'pass': 16889, 'play': 2678, 'choice': 10365, 'move_to_battlefield': 2370, 'move_to_base': 1305, 'combat_damage': 1358, 'activate': 697}
  - ScoringStep: {'pass': 225}

## Per-game markers

- Flow plays (printed): {'kennen_tyler': 490}
- Flow plays (granted): {'kennen_tyler': 22}
- Tomb-restricted plays: {'rengar_test': 15, 'kennen_tyler': 29}
- Conquers (score increments attributed to actor): {'rengar_test': 845, 'kennen_tyler': 881}
- Reaction plays in the closed state: {'rengar_test': 367, 'kennen_tyler': 197}
- Burns/empowers: OMITTED (log schema carries no marker for these)

## Mistake candidates (losing deck, top 20)

- drop=3 game=game_147_seed_2147.json idx=346 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': 'Stacked Deck', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 13 MainPhase/Open scores P1=6 P2=4 | Minefield[None] P1:0u/0m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=347 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 13 MainPhase/Open scores P1=6 P2=4 | Minefield[None] P1:0u/0m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=348 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 13 MainPhase/Closed scores P1=6 P2=4 | Minefield[None] P1:0u/0m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=349 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 13 MainPhase/Closed scores P1=6 P2=4 | Minefield[None] P1:0u/0m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=351 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 13 MainPhase/Closed scores P1=6 P2=4 | Minefield[None] P1:0u/0m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=353 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 13 MainPhase/Closed scores P1=6 P2=4 | Minefield[None] P1:0u/0m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=354 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': 'Up from the Deep', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 13 MainPhase/Open scores P1=6 P2=4 | Minefield[None] P1:0u/0m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=355 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 13 MainPhase/Open scores P1=6 P2=4 | Minefield[None] P1:0u/0m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=356 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 13 MainPhase/Open scores P1=6 P2=4 | Minefield[None] P1:0u/0m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=357 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 13 MainPhase/Open scores P1=6 P2=4 | Minefield[None] P1:0u/0m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=358 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 13 MainPhase/Closed scores P1=6 P2=4 | Minefield[None] P1:0u/0m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=360 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 13 MainPhase/Open scores P1=6 P2=4 | Minefield[None] P1:0u/0m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=361 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 13 MainPhase/Closed scores P1=6 P2=4 | Minefield[None*] P1:1u/2m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=363 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 13 MainPhase/Closed scores P1=6 P2=4 | Minefield[None*] P1:1u/2m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=364 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': 'Lightning Rush', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 13 MainPhase/Open scores P1=6 P2=4 | Minefield[None*] P1:1u/2m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=365 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 13 MainPhase/Open scores P1=6 P2=4 | Minefield[None*] P1:1u/2m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=366 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 13 MainPhase/Closed scores P1=6 P2=4 | Minefield[None*] P1:1u/2m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=368 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 13 MainPhase/Closed scores P1=6 P2=4 | Minefield[None*] P1:1u/2m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=370 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 13 MainPhase/Open scores P1=6 P2=4 | Minefield[None*] P1:1u/2m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m
- drop=3 game=game_147_seed_2147.json idx=371 turn=13 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[6, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 13 MainPhase/Open scores P1=6 P2=4 | Minefield[None*] P1:1u/2m P2:1u/4m; Treasure Hoard[P2] P1:1u/4m P2:3u/9m

## Swing decisions (winning deck, top 20)

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
- rise=4 game=game_154_seed_2154.json idx=147 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=148 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=150 turn=8 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': {'id': 1, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=151 turn=8 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateActionAbility'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None*] P1:1u/6m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=152 turn=8 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None*] P1:1u/6m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=154 turn=8 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 8 MainPhase/Closed scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None*] P1:1u/6m P2:0u/0m
- rise=4 game=game_154_seed_2154.json idx=156 turn=8 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[2, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 8 MainPhase/Open scores P1=2 P2=1 | Minefield[None] P1:0u/0m P2:0u/0m; Treasure Hoard[None*] P1:1u/6m P2:0u/0m
- rise=4 game=game_156_seed_2156.json idx=215 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[5, 2] chosen={'card': 'Rengar, Trophy Hunter', 'destination': {'player': 'P2', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 10 MainPhase/Open scores P1=5 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Emperor's Dais[P1] P1:1u/2m P2:0u/0m
- rise=4 game=game_156_seed_2156.json idx=216 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[5, 2] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=5 P2=2 | Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Emperor's Dais[P1] P1:1u/2m P2:0u/0m

## Notes

- The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.
- Turn ownership is inferred (first non-Closed decision's actor per turn), not read from an explicit field.
- Burns/empowers OMITTED: no decision in this batch carried a burn/empower marker in the assumed schema; nothing to count.
