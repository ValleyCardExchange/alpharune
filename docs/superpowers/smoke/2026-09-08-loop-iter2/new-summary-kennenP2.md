# Batch analysis summary

**The mistake/swing heuristic below is crude by design** — it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.

Games processed: 200  |  skipped (truncated/incomplete): 0  |  skipped (unparseable): 0
Mean turns (overall): 15.25

## Validity

OK — no frozen-state bursts (longest identical run 2, threshold 10).

## Win rate by deck and seat (Wilson 95%)

| Deck | Seat | Wins | Games | Win rate | 95% CI |
|---|---|---|---|---|---|
| kennen_tyler | P2 | 108 | 200 | 54.0% | 47.1% - 60.8% |
| rengar_test | P1 | 92 | 200 | 46.0% | 39.2% - 52.9% |

## Point-tempo curve (mean score / margin by turn)

- **rengar_test::P1** (turns 1..21): score=[0.0, 0.145, 0.37, 0.665, 1.025, 1.42, 1.81, 2.255, 2.715, 3.213, 3.793, 4.228, 4.887, 5.364, 5.767, 6.27, 6.333, 6.273, 6.125, 6.8, 7.0] margin=[0.0, -0.21, -0.32, -0.55, -0.715, -0.81, -0.935, -1.01, -1.105, -1.091, -0.99, -1.011, -0.66, -0.419, -0.35, 0.079, 0.1, 0.182, -0.5, 0.2, 0.5]
- **kennen_tyler::P2** (turns 1..21): score=[0.0, 0.355, 0.69, 1.215, 1.74, 2.23, 2.745, 3.265, 3.82, 4.305, 4.782, 5.239, 5.547, 5.783, 6.117, 6.19, 6.233, 6.091, 6.625, 6.6, 6.5] margin=[0.0, 0.21, 0.32, 0.55, 0.715, 0.81, 0.935, 1.01, 1.105, 1.091, 0.99, 1.011, 0.66, 0.419, 0.35, -0.079, -0.1, -0.182, 0.5, -0.2, -0.5]

## Action-family mix per phase per deck

- **kennen_tyler**
  - Mulligan: {'setup': 200}
  - MainPhase: {'pass': 16606, 'play': 3449, 'choice': 10479, 'move_to_battlefield': 2357, 'activate': 2601, 'combat_damage': 1153, 'move_to_base': 729}
  - ScoringStep: {'pass': 147, 'activate': 60, 'choice': 15, 'play': 2}
- **rengar_test**
  - Mulligan: {'setup': 200}
  - MainPhase: {'play': 2591, 'choice': 10071, 'pass': 17314, 'combat_damage': 1153, 'move_to_base': 1013, 'move_to_battlefield': 1868, 'activate': 613}
  - ScoringStep: {'pass': 147}

## Per-game markers

- Flow plays (printed): {'kennen_tyler': 496}
- Flow plays (granted): {'kennen_tyler': 18}
- Tomb-restricted plays: {'kennen_tyler': 50, 'rengar_test': 21}
- Conquers (score increments attributed to actor): {'rengar_test': 757, 'kennen_tyler': 890}
- Reaction plays in the closed state: {'rengar_test': 333, 'kennen_tyler': 202}
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

- rise=4 game=game_134_seed_3134.json idx=89 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'ScoringStep'} scores=[0, 3] chosen={'card': 'Flash', 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayReaction'}
  board: turn 7 ScoringStep/Closed scores P1=0 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/5m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_134_seed_3134.json idx=90 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'ScoringStep'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 ScoringStep/Closed scores P1=0 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/5m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_134_seed_3134.json idx=91 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'ScoringStep'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 ScoringStep/Closed scores P1=0 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/5m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_134_seed_3134.json idx=92 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'ScoringStep'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateReactionAbility'}
  board: turn 7 ScoringStep/Closed scores P1=0 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/5m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_134_seed_3134.json idx=93 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'ScoringStep'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 7 ScoringStep/Closed scores P1=0 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/5m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_134_seed_3134.json idx=95 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'ScoringStep'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 7 ScoringStep/Closed scores P1=0 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/5m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_134_seed_3134.json idx=97 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'ScoringStep'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 ScoringStep/Closed scores P1=0 P2=3 | Star Spring[P2] P1:0u/0m P2:1u/5m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_134_seed_3134.json idx=98 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Closed', 'phase': 'ScoringStep'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 7 ScoringStep/Closed scores P1=0 P2=3 | Star Spring[P2] P1:0u/0m P2:0u/0m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_134_seed_3134.json idx=100 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': 'Treasure Hunter', 'destination': {'player': 'P2', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 7 MainPhase/Open scores P1=0 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_134_seed_3134.json idx=101 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 MainPhase/Open scores P1=0 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_134_seed_3134.json idx=102 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 MainPhase/Open scores P1=0 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_134_seed_3134.json idx=103 turn=7 deck=kennen_tyler phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': None, 'destination': {'id': 1, 'type': 'battlefield'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'StandardMove'}
  board: turn 7 MainPhase/Open scores P1=0 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Zaun Warrens[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_134_seed_3134.json idx=104 turn=7 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'ActivateActionAbility'}
  board: turn 7 MainPhase/Open scores P1=0 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Zaun Warrens[None*] P1:0u/0m P2:1u/5m
- rise=4 game=game_134_seed_3134.json idx=105 turn=7 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassPriority'}
  board: turn 7 MainPhase/Closed scores P1=0 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Zaun Warrens[None*] P1:0u/0m P2:1u/5m
- rise=4 game=game_134_seed_3134.json idx=107 turn=7 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Closed', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 7 MainPhase/Closed scores P1=0 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Zaun Warrens[None*] P1:0u/0m P2:1u/5m
- rise=4 game=game_134_seed_3134.json idx=109 turn=7 deck=kennen_tyler phase={'ns_state': 'Showdown', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[0, 3] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PassFocus'}
  board: turn 7 MainPhase/Open scores P1=0 P2=3 | Star Spring[None] P1:0u/0m P2:0u/0m; Zaun Warrens[None*] P1:0u/0m P2:1u/5m
- rise=4 game=game_170_seed_3170.json idx=187 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 4] chosen={'card': 'Rengar, Pouncing', 'destination': {'player': 'P1', 'type': 'base'}, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'PlayCard'}
  board: turn 10 MainPhase/Open scores P1=3 P2=4 | Star Spring[P2] P1:0u/0m P2:1u/6m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_170_seed_3170.json idx=188 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=3 P2=4 | Star Spring[P2] P1:0u/0m P2:1u/6m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_170_seed_3170.json idx=189 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=3 P2=4 | Star Spring[P2] P1:0u/0m P2:1u/6m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:0u/0m
- rise=4 game=game_170_seed_3170.json idx=190 turn=10 deck=rengar_test phase={'ns_state': 'Neutral', 'oc_state': 'Open', 'phase': 'MainPhase'} scores=[3, 4] chosen={'card': None, 'destination': None, 'flow_source': 'None', 'restricted_bf': None, 'source_zone': 'Hand', 'type': 'MakeChoice'}
  board: turn 10 MainPhase/Open scores P1=3 P2=4 | Star Spring[P2] P1:0u/0m P2:1u/6m; Sandswept Tomb[None] P1:0u/0m P2:0u/0m; Baron Pit[None] P1:0u/0m P2:0u/0m

## Notes

- The mistake/swing heuristic is CRUDE BY DESIGN: it is a score-margin swing detector meant to point a reader at decisions worth reading, not a grading of play quality.
- Turn ownership is inferred (first non-Closed decision's actor per turn), not read from an explicit field.
- Burns/empowers OMITTED: no decision in this batch carried a burn/empower marker in the assumed schema; nothing to count.
