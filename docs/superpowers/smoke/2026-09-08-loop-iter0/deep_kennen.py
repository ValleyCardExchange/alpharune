#!/usr/bin/env python3
"""Deeper, Kennen-centred read of iteration-0 decision logs (stdlib only).
Usage: deep_kennen.py DIR [DIR...]  -- each DIR holds game_*.json"""
import json, glob, sys, math, collections, statistics as st

def wilson(k, n, z=1.96):
    if n == 0: return (0, 0, 0)
    p = k/n; d = 1+z*z/n; c = (p+z*z/(2*n))/d; h = z*math.sqrt(p*(1-p)/n+z*z/(4*n*n))/d
    return (p, c-h, c+h)

def fmt(k, n):
    p, lo, hi = wilson(k, n); return f"{k}/{n} = {100*p:.1f}% [{100*lo:.0f}–{100*hi:.0f}]"

games = []
for D in sys.argv[1:]:
    for f in sorted(glob.glob(D + '/game_*.json')):
        g = json.load(open(f)); g['_file'] = f
        seats = g['seats']; ks = [s for d, s in seats.items() if 'kennen' in d.lower()][0]
        g['_k'] = ks; g['_r'] = 'P2' if ks == 'P1' else 'P1'
        g['_win'] = (g['winner'] or {}).get('seat') == ks
        games.append(g)
N = len(games); W = sum(g['_win'] for g in games)
print(f"games {N}  Kennen wins {fmt(W, N)}")
si = {'P1': 0, 'P2': 1}

def bucketize(pairs, edges, label):
    """pairs: list of (value, win). edges: sorted cut points -> buckets [<e0, e0..e1, ..., >=e_last]"""
    b = collections.defaultdict(lambda: [0, 0])
    for v, w in pairs:
        name = None
        for i, e in enumerate(edges):
            if v < e: name = (f"<{e}" if i == 0 else f"{edges[i-1]}–{e-1}"); break
        if name is None: name = f"≥{edges[-1]}"
        b[name][0] += w; b[name][1] += 1
    order = sorted(b, key=lambda s: (s.startswith('≥'), int(''.join(ch for ch in s.split('–')[0] if ch.isdigit() or ch=='-') or 0)))
    print(f"  {label}: " + " | ".join(f"{k}: {fmt(*b[k])}" for k in order))

feat = collections.defaultdict(list)
card_win = collections.defaultdict(lambda: [0, 0])      # Kennen card played -> [wins, games with >=1]
opp_card_win = collections.defaultdict(lambda: [0, 0])  # Rengar card played -> Kennen [wins, games]
bf_pair = collections.defaultdict(lambda: [0, 0])
margin_at = collections.defaultdict(lambda: collections.defaultdict(lambda: [0, 0]))
turn_bucket = []
first_score = collections.defaultdict(lambda: [0, 0])
for g in games:
    k, r, win = g['_k'], g['_r'], g['_win']; ki, ri = si[k], si[r]
    log = g['log']
    flow = sum(1 for d in log if d['actor'] == k and d['chosen'].get('flow_source') in ('Printed', 'Granted'))
    feat['flow'].append((flow, win))
    kplays = collections.Counter(); rplays = collections.Counter()
    first_kennen = None; seals = 0; retreats = 0; reactions_closed = 0; tomb = 0
    empowered_turns = set(); kennen_turns = set()
    per_turn_last = {}
    for d in log:
        c = d['chosen']; t = d['turn']
        per_turn_last[t] = d
        if d['actor'] == k:
            kennen_turns.add(t) if d['phase']['phase'] == 'MainPhase' and d['phase'].get('oc_state') == 'Open' else None
            if d['players'][ki]['legend_empowered']: empowered_turns.add(t)
            if c.get('card') and c['type'] in ('PlayCard', 'PlayReaction', 'PlayActionCard'):
                kplays[c['card']] += 1
                if c['card'].startswith('Kennen') and first_kennen is None: first_kennen = t
                if c['card'] == 'Seal of Discord': seals += 1
                if d['phase'].get('oc_state') == 'Closed': reactions_closed += 1
            if c['type'] == 'StandardMove' and (c.get('destination') or {}).get('type') == 'base': retreats += 1
            if c.get('restricted_bf') is not None: tomb += 1
        elif d['actor'] == r and c.get('card') and c['type'] in ('PlayCard', 'PlayReaction', 'PlayActionCard'):
            rplays[c['card']] += 1
    feat['first_kennen'].append((first_kennen if first_kennen else 99, win))
    feat['seals'].append((seals, win)); feat['retreats'].append((retreats, win))
    feat['closed_reactions'].append((reactions_closed, win)); feat['tomb'].append((tomb, win))
    feat['empowered_turns'].append((len(empowered_turns), win))
    feat['turns'].append((g['turns'], win))
    for card in kplays: card_win[card][0] += win; card_win[card][1] += 1
    for card in rplays: opp_card_win[card][0] += win; opp_card_win[card][1] += 1
    names = tuple(sorted(b['name'] for b in log[-1]['battlefields']))
    bf_pair[names][0] += win; bf_pair[names][1] += 1
    # margin at end of turn T (Kennen minus Rengar)
    for T in (4, 6, 8, 10, 12):
        if T in per_turn_last:
            s = per_turn_last[T]['scores']; m = s[ki] - s[ri]
            key = '≤-2' if m <= -2 else ('-1' if m == -1 else ('0' if m == 0 else ('+1' if m == 1 else '≥+2')))
            margin_at[T][key][0] += win; margin_at[T][key][1] += 1
    # who scored first
    fs = None
    for d in log:
        s = d['scores']
        if s[ki] > 0 and fs is None: fs = 'kennen'
        if s[ri] > 0 and fs is None: fs = 'rengar'
        if fs: break
    first_score[fs or 'none'][0] += win; first_score[fs or 'none'][1] += 1
    # end-of-own-turn ready runes and hand size, board width
    ready = []; hands = []; widths = []; zero_boards = 0
    for t in sorted(kennen_turns):
        d = per_turn_last[t]; p = d['players'][ki]
        ready.append(p['runes_ready']); hands.append(p['hand'])
        w = sum(b['units'][k.lower()]['count'] for b in d['battlefields']); widths.append(w)
        if w == 0 and t >= 4: zero_boards += 1
    feat['mean_ready_end'].append((round(st.mean(ready)) if ready else 0, win))
    feat['mean_hand_end'].append((round(st.mean(hands)) if hands else 0, win))
    feat['mean_width_end'].append((round(st.mean(widths)) if widths else 0, win))
    feat['zero_boards'].append((zero_boards, win))

    # (a) re-conquer loop: score increments by Kennen attributed to the battlefield Kennen controls at that moment
    conq_by_bf = collections.Counter(); prev = None
    for d in log:
        s = d['scores']
        if prev is not None and s[ki] > prev[ki]:
            held = [b['name'] for b in d['battlefields'] if b['controller'] == k]
            for name in held: conq_by_bf[name] += 1
        prev = s
    feat['max_same_bf_conquers'].append((max(conq_by_bf.values()) if conq_by_bf else 0, win))
    feat['uftd'].append((kplays.get('Up from the Deep', 0), win))
    early_w = [sum(b['units'][k.lower()]['count'] for b in per_turn_last[t]['battlefields']) for t in sorted(kennen_turns) if 3 <= t <= 6]
    feat['early_width'].append((round(st.mean(early_w)) if early_w else 0, win))
    early_retreats = sum(1 for d in log if d['actor'] == k and d['turn'] <= 8 and d['chosen']['type'] == 'StandardMove' and (d['chosen'].get('destination') or {}).get('type') == 'base')
    feat['early_retreats'].append((early_retreats, win))

print("\n== Game length and tempo ==")
bucketize(feat['turns'], [13, 17], "win rate by game length (turns)")
for T in (4, 6, 8, 10, 12):
    print(f"  margin at end of turn {T}: " + " | ".join(f"{k}: {fmt(*margin_at[T][k])}" for k in ('≤-2', '-1', '0', '+1', '≥+2') if k in margin_at[T]))
print("  first to score: " + " | ".join(f"{k}: {fmt(*v)}" for k, v in first_score.items()))
print("\n== Kennen behaviours vs win ==")
bucketize(feat['flow'], [1, 3, 5], "Flow plays per game")
bucketize(feat['first_kennen'], [4, 6, 8, 99], "turn Kennen champion first played (99 = never)")
bucketize(feat['seals'], [1, 2, 3], "Seals of Discord played")
bucketize(feat['empowered_turns'], [1, 3, 5], "turns with legend Empowered")
bucketize(feat['closed_reactions'], [1, 3, 5], "closed-state plays by Kennen")
bucketize(feat['retreats'], [1, 3, 6], "moves to base (retreats)")
bucketize(feat['tomb'], [1, 2], "Tomb-restricted plays")
bucketize(feat['mean_ready_end'], [1, 2, 3], "mean ready runes at own end of turn")
bucketize(feat['mean_hand_end'], [2, 4, 6], "mean hand size at own end of turn")
bucketize(feat['mean_width_end'], [1, 2, 3], "mean units on battlefields at own end of turn")
bucketize(feat['zero_boards'], [1, 3, 5], "own turns (≥4) ending with NO units on battlefields")
bucketize(feat['max_same_bf_conquers'], [2, 3, 5], "most conquers of ONE battlefield (re-conquer loop)")
bucketize(feat['uftd'], [1, 2, 3], "Up from the Deep plays")
bucketize(feat['early_width'], [1, 2, 3], "mean units on battlefields at own end of turns 3–6 (EARLY)")
bucketize(feat['early_retreats'], [1, 3, 5], "retreats before turn 9")
print("\n== Kennen win rate when a card was played at least once ==")
for card, (w, n) in sorted(card_win.items(), key=lambda kv: -kv[1][1]):
    print(f"  {card:28s} {fmt(w, n)}")
print("\n== Kennen win rate when RENGAR played a card at least once ==")
for card, (w, n) in sorted(opp_card_win.items(), key=lambda kv: -kv[1][1]):
    print(f"  {card:28s} {fmt(w, n)}")
print("\n== battlefield pairs ==")
for names, (w, n) in sorted(bf_pair.items(), key=lambda kv: -kv[1][1]):
    print(f"  {' + '.join(names):40s} {fmt(w, n)}")
