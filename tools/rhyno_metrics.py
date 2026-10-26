"""Measure a rendered drum loop with the same metrics used to analyse the RHYNO reference clip.
usage:
  python3 tools/rhyno_metrics.py measure <wav> [--json out.json]
  python3 tools/rhyno_metrics.py compare <wav> tests/rhyno_targets.json   (exit 1 if far off)
"""
import json, sys
import numpy as np
from scipy.io import wavfile
from scipy import signal

BANDS = [(None, 80), (80, 200), (200, 500), (500, 2000), (2000, 6000), (6000, None)]


def _load(path):
    sr, x = wavfile.read(path)
    x = x.astype(np.float64)
    if x.ndim == 1:
        x = np.stack([x, x], 1)
    x /= np.abs(x).max() + 1e-12
    return sr, x


def _fl(y, lo, hi, sr):
    if lo is None:
        sos = signal.butter(4, hi, 'low', fs=sr, output='sos')
    elif hi is None:
        sos = signal.butter(4, lo, 'high', fs=sr, output='sos')
    else:
        sos = signal.butter(4, [lo, hi], 'band', fs=sr, output='sos')
    return signal.sosfiltfilt(sos, y, axis=-1)


def measure(path, seconds=None):
    sr, x = _load(path)
    if seconds:
        x = x[: int(seconds * sr)]
    L, R = x[:, 0], x[:, 1]
    M, S = (L + R) / 2, (L - R) / 2
    tot = np.sum(M ** 2)
    bal = [10 * np.log10(np.sum(_fl(M, a, b, sr) ** 2) / tot + 1e-20) for a, b in BANDS]
    wid = [10 * np.log10(np.sum(_fl(S, a, b, sr) ** 2) / (np.sum(_fl(M, a, b, sr) ** 2) + 1e-20) + 1e-20) for a, b in BANDS]
    sub = _fl(M, None, 90, sr)
    k = int(0.005 * sr)
    e = np.array([np.abs(sub[i:i + k]).max() for i in range(0, len(sub) - k, k)])
    ons, last = [], -99
    for i in range(4, len(e) - 1):
        if e[i] > e.max() * 0.25 and e[i] > e[i - 4:i].mean() * 1.4 and e[i] >= e[i - 1] and i - last > 18:
            ons.append(i); last = i
    fr = []
    for a, b in zip(ons[:-1], ons[1:]):
        pk = e[a:a + 20].max(); w = np.where(e[a + 5:b] < pk * 0.1)[0]
        fr.append(((w[0] + 5) / (b - a)) if len(w) else 1.0)
    kb = _fl(M, 80, 250, sr)
    rat = [20 * np.log10(np.abs(kb[a * k:a * k + int(0.02 * sr)]).max() / (np.abs(sub[a * k + int(0.08 * sr):a * k + int(0.25 * sr)]).max() + 1e-12)) for a in ons]
    hi = _fl(M, 2000, 10000, sr)
    f, t, Z = signal.stft(hi, sr, nperseg=512, noverlap=384)
    A = np.abs(Z)[(f > 2000) & (f < 10000)]
    lev = 20 * np.log10(np.sqrt((A ** 2).mean(0)) + 1e-12); lev -= lev.max()
    flat = np.exp(np.mean(np.log(A + 1e-12), 0)) / (A.mean(0) + 1e-12)
    fp = [float(np.median(flat[(lev > a) & (lev <= b)])) for a, b in [(-12, 0), (-24, -12), (-36, -24), (-48, -36)]]
    return dict(balance_db=[round(v, 2) for v in bal], side_mid_db=[round(v, 2) for v in wid],
                bounce_share=round(float(np.mean(np.array(fr) < 0.95)), 3), kick_punch_db=round(float(np.median(rat)), 2),
                hf_flatness=[round(v, 3) for v in fp])


def loss(m, t):
    return (np.sum((np.array(m['balance_db']) - t['balance_db']) ** 2) / 6
            + 0.3 * np.sum((np.maximum(m['side_mid_db'], -25) - np.maximum(t['side_mid_db'], -25)) ** 2) / 6
            + 20 * (m['bounce_share'] - t['bounce_share']) ** 2
            + 0.5 * (m['kick_punch_db'] - t['kick_punch_db']) ** 2
            + 300 * np.sum((np.array(m['hf_flatness']) - t['hf_flatness']) ** 2) / 4)


if __name__ == '__main__':
    cmd, wav = sys.argv[1], sys.argv[2]
    m = measure(wav)
    if cmd == 'measure':
        print(json.dumps(m, indent=1))
        if '--json' in sys.argv:
            json.dump(m, open(sys.argv[sys.argv.index('--json') + 1], 'w'), indent=1)
    else:
        t = json.load(open(sys.argv[3]))
        l = loss(m, t)
        for key in m:
            print(f"{key:14s} target {t[key]}\n{'':14s} thudi  {m[key]}")
        print(f"loss {l:.2f} (pass < {t.get('max_loss', 6.0)})")
        sys.exit(0 if l < t.get('max_loss', 6.0) else 1)
