# Batch analysis summary

**The mistake/swing heuristic below is crude by design** — it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.

Games processed: 200  |  skipped (truncated/incomplete): 0  |  skipped (unparseable): 0
Mean turns (overall): 15.055

## Validity

OK — no frozen-state bursts (longest identical run 2, threshold 10).

## Win rate by deck and seat (Wilson 95%)

| Deck | Seat | Wins | Games | Win rate | 95% CI |
|---|---|---|---|---|---|
| rengar_test | P1 | 95 | 200 | 47.5% | 40.7% - 54.4% |
| kennen_tyler | P2 | 105 | 200 | 52.5% | 45.6% - 59.3% |

## Point-tempo curve (mean score / margin by turn)

- **rengar_test::P1** (turns 1..21): score=[0.0, 0.145, 0.39, 0.68, 1.03, 1.465, 1.87, 2.315, 2.79, 3.259, 3.827, 4.44, 4.975, 5.468, 5.837, 6.377, 6.043, 5.222, 5.0, 6.0, 6.0] margin=[0.0, -0.215, -0.385, -0.635, -0.82, -0.845, -0.93, -1.015, -1.11, -1.137, -1.01, -0.742, -0.553, -0.331, -0.265, 0.245, -0.478, -1.556, -2.0, -1.0, -1.0]
- **kennen_tyler::P2** (turns 1..21): score=[0.0, 0.36, 0.775, 1.315, 1.85, 2.31, 2.8, 3.33, 3.9, 4.396, 4.838, 5.181, 5.528, 5.798, 6.102, 6.132, 6.522, 6.778, 7.0, 7.0, 7.0] margin=[0.0, 0.215, 0.385, 0.635, 0.82, 0.845, 0.93, 1.015, 1.11, 1.137, 1.01, 0.742, 0.553, 0.331, 0.265, -0.245, 0.478, 1.556, 2.0, 1.0, 1.0]

## Action-family mix per phase per deck

- **kennen_tyler**
  - Mulligan: {'setup': 200}
  - MainPhase: {'pass': 16419, 'play': 3424, 'choice': 10397, 'move_to_battlefield': 2333, 'activate': 2557, 'combat_damage': 1151, 'move_to_base': 716}
  - ScoringStep: {'activate': 48, 'pass': 122, 'play': 2, 'choice': 19}
- **rengar_test**
  - Mulligan: {'setup': 200}
  - MainPhase: {'play': 2532, 'choice': 9896, 'pass': 17180, 'combat_damage': 1151, 'move_to_base': 960, 'move_to_battlefield': 1812, 'activate': 596}
  - ScoringStep: {'pass': 122}

## Per-game markers

- Flow plays (printed): {'kennen_tyler': 492}
- Flow plays (granted): {'kennen_tyler': 27}
- Tomb-restricted plays: {'rengar_test': 26, 'kennen_tyler': 47}
- Conquers (score increments attributed to actor): {'rengar_test': 749, 'kennen_tyler': 859}
- Reaction plays in the closed state: {'rengar_test': 310, 'kennen_tyler': 191}
- Burns/empowers: OMITTED (log schema carries no marker for these)

## Mistake candidates (losing deck, top 20)

- drop=3 game=game_105_seed_3105.json idx=183 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': 'Fizz, Trickster', 'destination': {'player': 'P2', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=184 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=185 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=186 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=187 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=188 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=190 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Closed scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=191 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=193 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': 'Ride the Wind', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=194 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=195 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=196 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=197 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=199 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Closed scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=200 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None] P1:1u/5m P2:0u/0m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=201 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateActionAbility'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None*] P1:1u/5m P2:1u/3m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=202 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 10 MainPhase/Closed scores P1=4 P2=1 | Star Spring[None*] P1:1u/5m P2:1u/3m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=204 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Closed scores P1=4 P2=1 | Star Spring[None*] P1:1u/5m P2:1u/3m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=206 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None*] P1:1u/5m P2:1u/3m; Minefield[None] P1:0u/0m P2:0u/0m
- drop=3 game=game_105_seed_3105.json idx=207 turn=10 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[4, 1] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'AssignCombatDamage'}
  board: turn 10 MainPhase/Open scores P1=4 P2=1 | Star Spring[None*] P1:1u/5m P2:1u/3m; Minefield[None] P1:0u/0m P2:0u/0m

## Swing decisions (winning deck, top 20)

- rise=4 game=game_138_seed_3138.json idx=37 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': {'id': 0, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 4 MainPhase/Open scores P1=1 P2=0 | Star Spring[P1] P1:1u/2m P2:0u/0m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=38 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': 'Up from the Deep', 'destination': None, 'flow_source': 'Printed', 'restricted_bf': None, 'source_zone': 'Trash', 'type': 'PlayCard'}
  board: turn 4 MainPhase/Open scores P1=1 P2=0 | Star Spring[P1*] P1:1u/2m P2:1u/1m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=39 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 4 MainPhase/Closed scores P1=1 P2=0 | Star Spring[P1*] P1:1u/2m P2:1u/1m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=41 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 4 MainPhase/Closed scores P1=1 P2=0 | Star Spring[P1*] P1:1u/2m P2:1u/1m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=44 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': 'Switcheroo', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 4 MainPhase/Open scores P1=1 P2=0 | Star Spring[P1*] P1:1u/2m P2:1u/1m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=45 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 4 MainPhase/Open scores P1=1 P2=0 | Star Spring[P1*] P1:1u/2m P2:1u/1m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=46 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 4 MainPhase/Open scores P1=1 P2=0 | Star Spring[P1*] P1:1u/2m P2:1u/1m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=47 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 4 MainPhase/Open scores P1=1 P2=0 | Star Spring[P1*] P1:1u/2m P2:1u/1m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=48 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 4 MainPhase/Open scores P1=1 P2=0 | Star Spring[P1*] P1:1u/2m P2:1u/1m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=49 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 4 MainPhase/Closed scores P1=1 P2=0 | Star Spring[P1*] P1:1u/2m P2:1u/1m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=51 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 4 MainPhase/Closed scores P1=1 P2=0 | Star Spring[P1*] P1:1u/2m P2:1u/1m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=52 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 4 MainPhase/Closed scores P1=1 P2=0 | Star Spring[P1*] P1:1u/2m P2:1u/1m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=54 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': 'Stacked Deck', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 4 MainPhase/Open scores P1=1 P2=0 | Star Spring[P1*] P1:1u/1m P2:1u/2m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=55 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 4 MainPhase/Open scores P1=1 P2=0 | Star Spring[P1*] P1:1u/1m P2:1u/2m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=56 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 4 MainPhase/Closed scores P1=1 P2=0 | Star Spring[P1*] P1:1u/1m P2:1u/2m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=58 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 4 MainPhase/Closed scores P1=1 P2=0 | Star Spring[P1*] P1:1u/1m P2:1u/2m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=60 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateActionAbility'}
  board: turn 4 MainPhase/Open scores P1=1 P2=0 | Star Spring[P1*] P1:1u/1m P2:1u/2m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=61 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 4 MainPhase/Closed scores P1=1 P2=0 | Star Spring[P1*] P1:1u/1m P2:1u/2m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=63 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 4 MainPhase/Closed scores P1=1 P2=0 | Star Spring[P1*] P1:1u/1m P2:1u/2m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_138_seed_3138.json idx=65 turn=4 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[1, 0] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 4 MainPhase/Open scores P1=1 P2=0 | Star Spring[P1*] P1:1u/1m P2:1u/2m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m

## Notes

- The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.
- Turn ownership is inferred (first non-Closed decision's actor per turn), not read from an explicit field.
- Burns/empowers OMITTED: no decision in this batch carried a burn/empower marker in the assumed schema; nothing to count.
