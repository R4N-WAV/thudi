"""Tune the RHYNO factory kit against the reference measurements (tests/rhyno_targets.json).
usage: python3 tools/fit_rhyno.py <thudi_render binary> <workdir> [iterations]
Writes <workdir>/RHYNO_fit.json (params per pad) and prints the result.
"""
import copy, json, os, subprocess, sys
import numpy as np
sys.path.insert(0, os.path.dirname(__file__))
import kits, rhyno_metrics

BIN, WORK = sys.argv[1], sys.argv[2]
ITERS = int(sys.argv[3]) if len(sys.argv) > 3 else 150
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
T = json.load(open(os.path.join(ROOT, 'tests/rhyno_targets.json')))
PATTERN = os.path.join(ROOT, 'tests/rhyno_pattern.txt')

# (pad index, param, lo, hi, log?)
SPACE = [
    (0, 'level', -12, 6, 0), (0, 'drive', 0, 0.8, 0), (0, 'tone_lp', 300, 3000, 1), (0, 'tone_decay', 120, 600, 1), (0, 'tone_curve', 0, 1, 0),
    (1, 'level', -12, 6, 0), (1, 'growl', 0.2, 1.0, 0), (1, 'drive', 0.1, 0.8, 0), (1, 'asym', 0.1, 1.0, 0),
    (1, 'crush_mix', 0.2, 1.0, 0), (1, 'crush_rate', 3000, 14000, 1), (1, 'crush_bits', 4, 12, 0), (1, 'crush_xover', 100, 600, 1),
    (1, 'width', 0.3, 1.0, 0), (1, 'mono_below', 100, 400, 1), (1, 'tone_decay', 200, 900, 1), (1, 'tone_curve', -1, 0.3, 0), (1, 'tone_lp', 1500, 9000, 1),
    (2, 'level', -18, 6, 0), (3, 'level', -18, 6, 0),
    (4, 'level', -24, 3, 0), (4, 'width', 0, 1, 0), (4, 'high_cut', 4000, 16000, 1), (4, 'metal_tone', 3000, 10000, 1),
    (5, 'level', -24, 3, 0), (2, 'width', 0, 1, 0), (3, 'width', 0, 1, 0), (2, 'drive', 0, 0.8, 0),
]


def get(k, s):
    p, name, *_ = s
    return k['pads'][p]['params'].get(name, None)


def apply(k, x):
    k = copy.deepcopy(k)
    for (p, name, lo, hi, lg), v in zip(SPACE, x):
        k['pads'][p]['params'][name] = float(v)
    # keep the open hat in the same family as the closed hat
    for name in ('width', 'high_cut', 'metal_tone'):
        k['pads'][5]['params'][name] = k['pads'][4]['params'][name]
    # keep 808 variants in sync with the main 808 character
    for v in (8, 9):
        for name in ('growl', 'drive', 'asym', 'crush_mix', 'crush_rate', 'crush_bits', 'crush_xover', 'width', 'mono_below', 'tone_lp', 'level'):
            k['pads'][v]['params'][name] = k['pads'][1]['params'][name]
    return k


def evaluate(k, tag='cand'):
    kp = os.path.join(WORK, tag + '.thudikit'); wp = os.path.join(WORK, tag + '.wav')
    json.dump(k, open(kp, 'w'))
    subprocess.run([BIN, kp, PATTERN, wp], check=True)
    m = rhyno_metrics.measure(wp)
    return rhyno_metrics.loss(m, T), m


def norm(v, s):
    _, _, lo, hi, lg = s
    return (np.log(v / lo) / np.log(hi / lo)) if lg else (v - lo) / (hi - lo)


def denorm(u, s):
    _, _, lo, hi, lg = s
    u = min(1, max(0, u))
    return lo * (hi / lo) ** u if lg else lo + u * (hi - lo)


if __name__ == '__main__':
    os.makedirs(WORK, exist_ok=True)
    base = json.load(open(os.environ['START'])) if os.environ.get('START') else kits.RHYNO
    defaults = {'level': 0.0, 'drive': 0.0, 'width': 0.0, 'high_cut': 22000, 'metal_tone': 7000, 'tone_lp': 20000,
                'tone_decay': 400, 'tone_curve': 0.0, 'mono_below': 120, 'crush_bits': 16}
    u = np.array([norm(min(max(get(base, s) if get(base, s) is not None else defaults.get(s[1], s[2]), s[2]), s[3]), s) for s in SPACE])
    best_l, best_m = evaluate(apply(base, [denorm(a, s) for a, s in zip(u, SPACE)]))
    print('start', round(best_l, 2), flush=True)
    rng = np.random.default_rng(3); scale = 0.25
    for it in range(ITERS):
        cand = u.copy()
        idx = rng.choice(len(SPACE), size=rng.integers(1, 5), replace=False)
        cand[idx] += rng.normal(0, scale, len(idx))
        cand = np.clip(cand, 0, 1)
        l, m = evaluate(apply(base, [denorm(a, s) for a, s in zip(cand, SPACE)]))
        if l < best_l:
            best_l, best_m, u = l, m, cand
            print(f'it {it} loss {best_l:.2f}', flush=True)
        if it % 60 == 59:
            scale *= 0.7
    best = apply(base, [denorm(a, s) for a, s in zip(u, SPACE)])
    json.dump(best, open(os.path.join(WORK, 'RHYNO_fit.json'), 'w'), indent=1)
    print('final loss', round(best_l, 2))
    for key in best_m:
        print(f"{key:14s} target {T[key]}\n{'':14s} thudi  {best_m[key]}")
    for (p, name, *_), a, s in zip(SPACE, u, SPACE):
        print(p + 1, name, round(denorm(a, s), 3))
