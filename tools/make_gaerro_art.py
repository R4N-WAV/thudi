"""
GÆRRO mode art for Thudi: the graffiti, stickers and tape that get thrown on the machine.
All original (no third-party logos); fonts are OFL / Apache (art/gaerro/fonts, Tamil: Kavivanar OFL).

  python3 tools/make_gaerro_art.py            -> assets/gaerro/*.png + assets/gaerro/gaerro_art.json
  python3 tools/make_gaerro_art.py --preview  -> also writes a preview composite over assets/ui/bg.png

Images are 2x design units, already rotated; the plugin reveals them one by one (see src/plugin/GaerroArt.cpp).
"""
import json, math, os, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONTS = os.path.join(ROOT, "art/gaerro/fonts")
OUT = os.path.join(ROOT, "assets/gaerro")
X2 = 2.0
rng = np.random.default_rng(1987)


def font(name, px):
    path = os.path.join(FONTS, name)
    if not os.path.exists(path):
        path = os.path.join(ROOT, "assets/fonts", name)
    return ImageFont.truetype(path, int(px))


def hexc(h):
    h = h.lstrip("#"); return np.array([int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)])


# ------------------------------------------------------------------ raster helpers (float RGBA, straight alpha)
def canvas(w, h):
    return np.zeros((int(h), int(w), 4), np.float32)


def text_mask(text, fnt, pad=40, spacing=0):
    tmp = Image.new("L", (10, 10)); d = ImageDraw.Draw(tmp)
    if spacing:
        widths = [d.textbbox((0, 0), ch, font=fnt)[2] for ch in text]
        w = sum(widths) + spacing * (len(text) - 1)
        bb = d.textbbox((0, 0), text, font=fnt)
        img = Image.new("L", (int(w + 2 * pad), bb[3] + 2 * pad), 0); dd = ImageDraw.Draw(img)
        x = pad
        for ch, cw in zip(text, widths):
            dd.text((x, pad), ch, font=fnt, fill=255); x += cw + spacing
    else:
        bb = d.textbbox((0, 0), text, font=fnt)
        img = Image.new("L", (bb[2] - bb[0] + 2 * pad, bb[3] - bb[1] + 2 * pad), 0)
        ImageDraw.Draw(img).text((pad - bb[0], pad - bb[1]), text, font=fnt, fill=255)
    return np.asarray(img, np.float32) / 255.0


def place(mask, w, h, x=None, y=None):
    """put mask into a w x h field (centred unless x,y given)"""
    out = np.zeros((int(h), int(w)), np.float32)
    mh, mw = mask.shape
    x = (w - mw) // 2 if x is None else int(x)
    y = (h - mh) // 2 if y is None else int(y)
    xs, ys = max(0, -x), max(0, -y)
    xe, ye = min(mw, int(w) - x), min(mh, int(h) - y)
    out[y + ys:y + ye, x + xs:x + xe] = mask[ys:ye, xs:xe]
    return out


def dilate(m, r):
    if r <= 0: return m.copy()
    d = ndimage.distance_transform_edt(m < 0.5)
    return np.clip(r + 0.5 - d, 0, 1).astype(np.float32) * 1.0 + 0 * m


def erode(m, r):
    d = ndimage.distance_transform_edt(m >= 0.5)
    return np.clip(d - r + 0.5, 0, 1).astype(np.float32)


def shift(m, dx, dy):
    return ndimage.shift(m, (dy, dx), order=1, mode="constant")


def blur(m, s):
    return ndimage.gaussian_filter(m, s) if s > 0 else m


def noise(h, w, scale, octaves=3):
    out = np.zeros((h, w), np.float32); amp = 1.0; tot = 0
    for o in range(octaves):
        s = max(1, int(scale / (2 ** o)))
        small = rng.random((h // s + 2, w // s + 2)).astype(np.float32)
        big = np.asarray(Image.fromarray((small * 255).astype(np.uint8)).resize(((w // s + 2) * s, (h // s + 2) * s), Image.BICUBIC), np.float32)[:h, :w] / 255.0
        out += amp * big; tot += amp; amp *= 0.5
    return out / tot


def paint(dst, mask, color, alpha=1.0):
    """composite a flat colour through mask onto dst (over)"""
    a = np.clip(mask * alpha, 0, 1)[..., None]
    c = color if np.ndim(color) == 3 else np.broadcast_to(np.asarray(color, np.float32), dst[..., :3].shape)
    da = dst[..., 3:4]
    out_a = a + da * (1 - a)
    rgb = (c * a + dst[..., :3] * da * (1 - a)) / np.maximum(out_a, 1e-6)
    dst[..., :3] = rgb; dst[..., 3:4] = out_a


def gradient(h, w, top, bottom):
    t = np.linspace(0, 1, h, dtype=np.float32)[:, None, None]
    return np.broadcast_to(hexc(top) * (1 - t) + hexc(bottom) * t, (h, w, 3)).astype(np.float32)


def rotate(rgba, deg):
    if abs(deg) < 1e-3: return rgba
    im = Image.fromarray((np.clip(rgba, 0, 1) * 255).astype(np.uint8), "RGBA")
    # rotate premultiplied to avoid dark fringes
    arr = np.asarray(im, np.float32) / 255.0
    pm = arr.copy(); pm[..., :3] *= pm[..., 3:4]
    imp = Image.fromarray((pm * 255).astype(np.uint8), "RGBA").rotate(deg, resample=Image.BICUBIC, expand=True)
    out = np.asarray(imp, np.float32) / 255.0
    a = out[..., 3:4]
    out[..., :3] = np.where(a > 1e-4, out[..., :3] / np.maximum(a, 1e-4), 0)
    return out


def drop_shadow(rgba, dx=2.5, dy=3.5, s=2.5, alpha=0.55, pad=14):
    h, w = rgba.shape[:2]
    big = canvas(w + 2 * pad, h + 2 * pad)
    sh = np.zeros((h + 2 * pad, w + 2 * pad), np.float32)
    sh[pad:pad + h, pad:pad + w] = rgba[..., 3]
    sh = blur(shift(sh, dx, dy), s)
    paint(big, sh, (0, 0, 0), alpha)
    top = canvas(w + 2 * pad, h + 2 * pad); top[pad:pad + h, pad:pad + w] = rgba
    over(big, top)
    return big


def over(dst, src):
    a = src[..., 3:4]; da = dst[..., 3:4]
    oa = a + da * (1 - a)
    dst[..., :3] = (src[..., :3] * a + dst[..., :3] * da * (1 - a)) / np.maximum(oa, 1e-6)
    dst[..., 3:4] = oa


def paper_finish(rgba, grain=0.05, gloss=0.10, wear=0.0):
    """printed vinyl sticker: grain, a soft gloss band, light wear at the edges"""
    h, w = rgba.shape[:2]
    n = noise(h, w, 6, 2)
    rgba[..., :3] *= (1 - grain / 2 + grain * n)[..., None]
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    band = np.exp(-((xx / w + yy / h - 0.7) ** 2) / 0.02) * gloss
    rgba[..., :3] = np.clip(rgba[..., :3] + band[..., None], 0, 1)
    if wear > 0:
        edge = ndimage.distance_transform_edt(rgba[..., 3] > 0.5)
        scratch = (noise(h, w, 3, 2) > 1 - wear) & (edge < 10)
        rgba[..., 3] *= np.where(scratch, 0.15, 1.0)
    return rgba


def spray_finish(rgba, soft=1.1, surface=0.10):
    """sprayed paint on a satin body: soft edge, a little of the surface showing through"""
    h, w = rgba.shape[:2]
    rgba[..., 3] = blur(rgba[..., 3], soft)
    rgba[..., 3] *= (1 - surface + surface * noise(h, w, 4, 3))
    return rgba


def overspray(mask, color, amount=0.05, reach=16):
    h, w = mask.shape
    band = np.clip(dilate(mask, reach) - mask, 0, 1)
    d = ndimage.distance_transform_edt(mask < 0.5)
    fall = np.clip(1 - d / reach, 0, 1) ** 2
    dots = (rng.random((h, w)) < amount * fall) * band
    dots = blur(dots.astype(np.float32), 0.6) * 1.6
    return np.clip(dots, 0, 1)


def drips(mask, n, color_mask_target, length=(18, 70), width=(3, 6), seed=0):
    """paint running down from the lower edges"""
    r = np.random.default_rng(seed)
    h, w = mask.shape
    out = np.zeros_like(mask)
    lower = (mask > 0.5) & (np.roll(mask, -1, axis=0) < 0.5)
    ys, xs = np.nonzero(lower)
    if len(xs) == 0: return out
    for _ in range(n):
        i = r.integers(len(xs)); x0, y0 = xs[i], ys[i]
        L = r.uniform(*length); wd = r.uniform(*width)
        img = Image.new("L", (w, h), 0); d = ImageDraw.Draw(img)
        steps = int(L)
        for k in range(steps):
            t = k / max(1, steps)
            ww = wd * (1 - 0.35 * t)
            d.ellipse([x0 - ww / 2 + math.sin(k * 0.15) * 0.6, y0 + k - ww / 2, x0 + ww / 2, y0 + k + ww / 2], fill=255)
        bl = wd * 0.85
        d.ellipse([x0 - bl, y0 + L - bl, x0 + bl, y0 + L + bl * 1.2], fill=255)
        out = np.maximum(out, np.asarray(img, np.float32) / 255.0)
    return out


def save(rgba, name):
    os.makedirs(OUT, exist_ok=True)
    a = np.clip(rgba, 0, 1)
    ys, xs = np.nonzero(a[..., 3] > 0.004)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    a = a[y0:y1, x0:x1]
    Image.fromarray((a * 255 + 0.5).astype(np.uint8), "RGBA").save(os.path.join(OUT, name), optimize=True)
    return a.shape[1], a.shape[0], x0, y0


ELEMENTS = []


def add(rgba, name, cx, cy, kind, t0, dur, dx=1.0, dy=0.0):
    """cx, cy: where the element's centre sits on the panel (design units)"""
    w, h, ox, oy = save(rgba, name)
    full_h, full_w = rgba.shape[:2]
    # centre of the trimmed image relative to the untrimmed centre
    ccx = (ox + w / 2 - full_w / 2) / X2
    ccy = (oy + h / 2 - full_h / 2) / X2
    ELEMENTS.append({"file": name, "x": round(cx + ccx - w / X2 / 2, 2), "y": round(cy + ccy - h / X2 / 2, 2),
                     "w": round(w / X2, 2), "h": round(h / X2, 2), "kind": kind, "t0": t0, "dur": dur, "dx": dx, "dy": dy})


# ------------------------------------------------------------------ pieces
def throwup():
    """GÆRRO piece across the knob band: pink bubble letters, black outline, lime 3D block, white shine"""
    W, H = 1560, 470
    letters = text_mask("GÆRRO", font("SedgwickAveDisplay-Regular.ttf", 330), pad=0, spacing=-18)
    # lean it a little and squash: throw-ups sit low and wide
    sx = 1340.0 / letters.shape[1]
    li = Image.fromarray((letters * 255).astype(np.uint8)).resize((int(letters.shape[1] * sx), int(letters.shape[0] * sx * 0.82)), Image.BICUBIC)
    li = li.transform(li.size, Image.AFFINE, (1, 0.12, -0.12 * li.size[1] * 0.5, 0, 1, 0), resample=Image.BICUBIC)
    M = place(np.asarray(li, np.float32) / 255.0, W, H)
    fill = dilate(M, 12)
    line = dilate(fill, 9)
    block = np.clip(shift(line, 16, 19), 0, 1)
    img = canvas(W, H)
    paint(img, overspray(dilate(block, 2), None, 0.05, 18), hexc("#9fe21f"), 0.55)
    paint(img, dilate(block, 2), hexc("#0e0e10"))
    paint(img, block, gradient(H, W, "#c8ff3c", "#7fcf12"))
    paint(img, line, hexc("#0e0e10"))
    paint(img, fill, gradient(H, W, "#ff5cb6", "#b3166f"))
    # second colour fade inside the letters (classic two-tone fill)
    yy = np.mgrid[0:H, 0:W][0].astype(np.float32)
    band = np.clip(1 - np.abs(yy / H - 0.62) / 0.08, 0, 1) * erode(fill, 3)
    paint(img, blur(band, 3), hexc("#ffd23a"), 0.55)
    # shine: crescents on the upper-left inner edge, plus a few dashes
    inner = erode(fill, 6)
    cres = np.clip(inner - shift(inner, 5, 6), 0, 1)
    paint(img, blur(cres, 0.8), (1, 1, 1), 0.92)
    r = np.random.default_rng(4)
    ys, xs = np.nonzero(erode(fill, 18) > 0.5)
    sp = Image.new("L", (W, H), 0); d = ImageDraw.Draw(sp)
    for _ in range(9):
        i = r.integers(len(xs)); x, y = xs[i], ys[i]
        d.line([x, y, x + 10, y - 8], fill=255, width=5)
        d.ellipse([x + 15, y - 15, x + 21, y - 9], fill=255)
    paint(img, np.asarray(sp, np.float32) / 255 * erode(fill, 8), (1, 1, 1), 0.9)
    paint(img, drips(line, 7, None, (20, 80), (4, 7), seed=3), hexc("#0e0e10"))
    paint(img, drips(fill, 4, None, (12, 40), (4, 6), seed=5) * (1 - line + fill), hexc("#c21a7b"))
    paint(img, overspray(line, None, 0.06, 20), hexc("#0e0e10"), 0.75)
    img = spray_finish(img, 1.0, 0.08)
    return rotate(img, 1.5)


def marker_tag(text, fnt_name, px, color, deg, drip_n=2, outline=None, seed=1):
    m = text_mask(text, font(fnt_name, px), pad=60)
    h, w = m.shape
    img = canvas(w, h + 60)
    m = place(m, w, h + 60, 0, 0)
    if outline:
        paint(img, dilate(m, 3), hexc(outline))
    paint(img, m, hexc(color))
    paint(img, drips(m, drip_n, None, (10, 38), (2.5, 4), seed=seed), hexc(color))
    img = spray_finish(img, 0.6, 0.12)
    return rotate(img, deg)


def sticker_hello():
    W, H = 300, 196
    img = canvas(W, H)
    body = Image.new("L", (W, H), 0); ImageDraw.Draw(body).rounded_rectangle([2, 2, W - 3, H - 3], 18, fill=255)
    bm = np.asarray(body, np.float32) / 255
    paint(img, bm, hexc("#d5232b"))
    white = Image.new("L", (W, H), 0); ImageDraw.Draw(white).rectangle([10, 76, W - 11, H - 24], fill=255)
    paint(img, np.asarray(white, np.float32) / 255, hexc("#fbfaf6"))
    hello = text_mask("HELLO", font("Bangers-Regular.ttf", 54), 0, spacing=3)
    paint(img, place(hello, W, H, None, 6), (1, 1, 1))
    small = text_mask("my name is", font("Bangers-Regular.ttf", 21), 0, spacing=1)
    paint(img, place(small, W, H, None, 52), (1, 1, 1))
    name = text_mask("GÆRRO", font("PermanentMarker-Regular.ttf", 66), 4)
    nm = place(name, W, H, None, 70)
    paint(img, nm * np.asarray(white, np.float32) / 255 * 1.0, hexc("#141414"), 0.95)
    img = paper_finish(img, 0.05, 0.08, 0.02)
    return drop_shadow(rotate(img, 6.0), 2.5, 3.5, 2.6, 0.6)


def sticker_round():
    D = 140
    img = canvas(D, D)
    disc = Image.new("L", (D, D), 0); ImageDraw.Draw(disc).ellipse([2, 2, D - 3, D - 3], fill=255)
    ring = Image.new("L", (D, D), 0); ImageDraw.Draw(ring).ellipse([12, 12, D - 13, D - 13], fill=255)
    paint(img, np.asarray(disc, np.float32) / 255, hexc("#e8e4da"))
    paint(img, np.asarray(ring, np.float32) / 255, hexc("#111113"))
    t = text_mask("R4N", font("Bangers-Regular.ttf", 58), 0, spacing=2)
    paint(img, place(t, D, D, None, 30), hexc("#ff3b2a"))
    s = text_mask("THUDI", font("Bangers-Regular.ttf", 20), 0, spacing=3)
    paint(img, place(s, D, D, None, 92), hexc("#e8e4da"))
    img = paper_finish(img, 0.06, 0.10, 0.03)
    return drop_shadow(rotate(img, -12), 2.0, 3.0, 2.2, 0.6)


def sticker_nosamples():
    W, H = 236, 84
    img = canvas(W, H)
    b = Image.new("L", (W, H), 0); ImageDraw.Draw(b).rectangle([1, 1, W - 2, H - 2], fill=255)
    paint(img, np.asarray(b, np.float32) / 255, hexc("#fbfaf6"))
    inner = Image.new("L", (W, H), 0); ImageDraw.Draw(inner).rectangle([5, 5, W - 6, H - 6], outline=255, width=4)
    paint(img, np.asarray(inner, np.float32) / 255, hexc("#111111"))
    l1 = text_mask("NO SAMPLES", font("Bangers-Regular.ttf", 36), 0, spacing=2)
    l2 = text_mask("CLEARED", font("Bangers-Regular.ttf", 30), 0, spacing=4)
    paint(img, place(l1, W, H, None, 6), hexc("#111111"))
    paint(img, place(l2, W, H, None, 44), hexc("#d5232b"))
    img = paper_finish(img, 0.05, 0.08, 0.04)
    return drop_shadow(rotate(img, -4), 2, 3, 2.2, 0.55)


def sticker_lofi():
    W, H = 112, 92
    img = canvas(W, H)
    b = Image.new("L", (W, H), 0); ImageDraw.Draw(b).rounded_rectangle([1, 1, W - 2, H - 2], 8, fill=255)
    paint(img, np.asarray(b, np.float32) / 255, hexc("#ffd21f"))
    t = text_mask("LO-FI", font("Bangers-Regular.ttf", 44), 0, spacing=1)
    paint(img, place(t, W, H, None, 10), hexc("#111111"))
    s = text_mask("CERTIFIED", font("Bangers-Regular.ttf", 15), 0, spacing=2)
    paint(img, place(s, W, H, None, 62), hexc("#111111"))
    img = paper_finish(img, 0.06, 0.10, 0.05)
    return drop_shadow(rotate(img, 9), 2, 3, 2, 0.55)


def sticker_88():
    D = 124
    img = canvas(D, D)
    disc = Image.new("L", (D, D), 0); ImageDraw.Draw(disc).ellipse([1, 1, D - 2, D - 2], fill=255)
    paint(img, np.asarray(disc, np.float32) / 255, hexc("#ff3fa4"))
    t = text_mask("88", font("Bangers-Regular.ttf", 62), 0, spacing=2)
    paint(img, place(t, D, D, None, 20), (1, 1, 1))
    s = text_mask("BPM", font("Bangers-Regular.ttf", 22), 0, spacing=3)
    paint(img, place(s, D, D, None, 84), (1, 1, 1))
    img = paper_finish(img, 0.05, 0.12, 0.04)
    return drop_shadow(rotate(img, 14), 2, 3, 2, 0.55)


def gaffer_tape(text, length, deg):
    W, H = int(length), 62
    img = canvas(W + 20, H + 20)
    m = np.zeros((H + 20, W + 20), np.float32)
    r = np.random.default_rng(11)
    # torn ends: jagged polygon
    pts = [(10, 10)]
    for y in range(10, H + 10, 4): pts.append((10 + r.uniform(-4, 4), y))
    pts.append((10, H + 10)); pts.append((W + 10, H + 10))
    for y in range(H + 10, 10, -4): pts.append((W + 10 + r.uniform(-4, 4), y))
    pts.append((W + 10, 10))
    im = Image.new("L", (W + 20, H + 20), 0); ImageDraw.Draw(im).polygon(pts, fill=255)
    m = np.asarray(im, np.float32) / 255
    hh, ww = m.shape
    weave = (np.sin(np.mgrid[0:hh, 0:ww][1] * 1.9) * np.sin(np.mgrid[0:hh, 0:ww][0] * 1.9)) * 0.03
    base = np.broadcast_to(hexc("#2d2d30"), (hh, ww, 3)).copy()
    base += (weave + (noise(hh, ww, 2, 2) - 0.5) * 0.06)[..., None]
    paint(img, m, base)
    # cloth threads at the torn ends
    paint(img, np.clip(m - erode(m, 2), 0, 1) * 0.6, hexc("#4a4a4e"))
    t = text_mask(text, font("PermanentMarker-Regular.ttf", 27), 0)
    tm = place(t, W + 20, H + 20)
    paint(img, tm * m, hexc("#e9e7e1"), 0.92)
    img = paper_finish(img, 0.04, 0.07, 0.0)
    return drop_shadow(rotate(img, deg), 1.5, 2.5, 1.8, 0.5)


def spin_it():
    img = marker_tag("SPIN IT", "PermanentMarker-Regular.ttf", 44, "#f2f0ea", -8, drip_n=1, seed=8)
    h, w = img.shape[:2]
    big = canvas(w + 60, h + 40)
    big[0:h, 0:w] = img
    # arrow curling to the wheel
    arr = Image.new("L", (w + 60, h + 40), 0); d = ImageDraw.Draw(arr)
    x0, y0 = w - 30, h // 2
    d.arc([x0 - 10, y0 - 30, x0 + 60, y0 + 50], -60, 80, fill=255, width=6)
    d.line([x0 + 44, y0 + 40, x0 + 30, y0 + 52], fill=255, width=6)
    d.line([x0 + 44, y0 + 40, x0 + 50, y0 + 22], fill=255, width=6)
    paint(big, np.asarray(arr, np.float32) / 255, hexc("#f2f0ea"), 0.95)
    return spray_finish(big, 0.6, 0.1)


def bumper(text, bg, fg, w, h, deg, fnt="Bangers-Regular.ttf", px=None, spacing=3):
    img = canvas(w, h)
    b = Image.new("L", (w, h), 0); ImageDraw.Draw(b).rounded_rectangle([1, 1, w - 2, h - 2], 6, fill=255)
    paint(img, np.asarray(b, np.float32) / 255, hexc(bg))
    t = text_mask(text, font(fnt, px or h * 0.62), 0, spacing=spacing)
    paint(img, place(t, w, h), hexc(fg))
    img = paper_finish(img, 0.05, 0.09, 0.05)
    return drop_shadow(rotate(img, deg), 2, 3, 2, 0.55)


def round_sticker(lines, bg, fg, d, deg, ring=None):
    img = canvas(d, d)
    disc = Image.new("L", (d, d), 0); ImageDraw.Draw(disc).ellipse([1, 1, d - 2, d - 2], fill=255)
    paint(img, np.asarray(disc, np.float32) / 255, hexc(bg))
    if ring:
        rr = Image.new("L", (d, d), 0); ImageDraw.Draw(rr).ellipse([6, 6, d - 7, d - 7], outline=255, width=4)
        paint(img, np.asarray(rr, np.float32) / 255, hexc(ring))
    y = d * 0.5 - sum(px for _, px in lines) * 0.55
    for text, px in lines:
        t = text_mask(text, font("Bangers-Regular.ttf", px), 0, spacing=2)
        paint(img, place(t, d, d, None, y), hexc(fg)); y += px * 1.05
    img = paper_finish(img, 0.05, 0.10, 0.04)
    return drop_shadow(rotate(img, deg), 2, 3, 2, 0.55)


def record_sticker(d, deg):
    img = canvas(d, d)
    yy, xx = np.mgrid[0:d, 0:d].astype(np.float32)
    r = np.hypot(xx - d / 2, yy - d / 2) / (d / 2)
    disc = np.clip((1 - r) * d / 2, 0, 1)
    grooves = 0.10 + 0.05 * np.sin(r * 160) + 0.04 * np.exp(-((np.arctan2(yy - d / 2, xx - d / 2) - 0.8) ** 2) / 0.05)
    paint(img, disc, np.repeat(grooves[..., None], 3, 2))
    lab = np.clip((0.36 - r) * d / 2, 0, 1)
    paint(img, lab, hexc("#d5232b"))
    t = text_mask("R4N", font("Bangers-Regular.ttf", d * 0.13), 0, spacing=1)
    paint(img, place(t, d, d, None, d * 0.40), hexc("#f2efe6"))
    hole = np.clip((0.035 - r) * d / 2, 0, 1)
    img[..., 3] *= 1 - hole
    img = paper_finish(img, 0.03, 0.12, 0.0)
    return drop_shadow(rotate(img, deg), 2, 3, 2, 0.55)


def cassette_sticker(w, h, deg):
    img = canvas(w, h)
    b = Image.new("L", (w, h), 0); d = ImageDraw.Draw(b); d.rounded_rectangle([1, 1, w - 2, h - 2], 8, fill=255)
    paint(img, np.asarray(b, np.float32) / 255, hexc("#f0ece2"))
    lab = Image.new("L", (w, h), 0); ImageDraw.Draw(lab).rounded_rectangle([10, 10, w - 11, h * 0.62], 5, fill=255)
    paint(img, np.asarray(lab, np.float32) / 255, hexc("#ff8a1f"))
    win = Image.new("L", (w, h), 0); dw = ImageDraw.Draw(win)
    dw.rounded_rectangle([w * 0.28, h * 0.26, w * 0.72, h * 0.52], 6, fill=255)
    paint(img, np.asarray(win, np.float32) / 255, hexc("#2a2a2c"))
    reels = Image.new("L", (w, h), 0); dr = ImageDraw.Draw(reels)
    for cx in (w * 0.36, w * 0.64):
        dr.ellipse([cx - h * 0.09, h * 0.30, cx + h * 0.09, h * 0.48], outline=255, width=3)
    paint(img, np.asarray(reels, np.float32) / 255, hexc("#e8e4da"))
    t = text_mask("SIDE A  GÆRRO TAPE", font("PermanentMarker-Regular.ttf", h * 0.13), 0)
    paint(img, place(t, w, h, None, h * 0.66), hexc("#1a1a1a"))
    img = paper_finish(img, 0.05, 0.10, 0.05)
    return drop_shadow(rotate(img, deg), 2, 3, 2, 0.55)


def star(size, color, deg):
    img = canvas(size, size)
    pts = []
    for k in range(10):
        a = -math.pi / 2 + k * math.pi / 5
        rr = size * (0.42 if k % 2 == 0 else 0.18)
        pts.append((size / 2 + rr * math.cos(a), size / 2 + rr * math.sin(a)))
    im = Image.new("L", (size, size), 0); ImageDraw.Draw(im).polygon(pts, fill=255)
    m = np.asarray(im, np.float32) / 255
    paint(img, overspray(m, None, 0.08, 10), hexc(color), 0.6)
    paint(img, m, hexc(color))
    paint(img, drips(m, 2, None, (8, 26), (2.5, 4), seed=9), hexc(color))
    return rotate(spray_finish(img, 0.9, 0.1), deg)


def grime():
    """scuffs, dust and old sticker residue over the whole body (very subtle)"""
    W, H = 940, 1210   # 1x is enough for this
    img = canvas(W, H)
    dust = (rng.random((H, W)) < 0.004).astype(np.float32)
    dust = blur(dust, 0.5) * 2
    paint(img, np.clip(dust, 0, 1), hexc("#bdb8ae"), 0.35)
    sc = Image.new("L", (W, H), 0); d = ImageDraw.Draw(sc)
    r = np.random.default_rng(21)
    for _ in range(70):
        x, y = r.uniform(0, W), r.uniform(0, H)
        a = r.uniform(-0.5, 0.5) + (math.pi / 2 if r.random() < 0.3 else 0)
        L = r.uniform(10, 60)
        d.line([x, y, x + L * math.cos(a), y + L * math.sin(a)], fill=int(r.uniform(60, 140)), width=1)
    paint(img, np.asarray(sc, np.float32) / 255, hexc("#d8d4cc"), 0.25)
    # residue of a peeled sticker
    res = Image.new("L", (W, H), 0); ImageDraw.Draw(res).rounded_rectangle([800, 1188, 880, 1206], 4, fill=255)
    rm = np.asarray(res, np.float32) / 255 * (noise(H, W, 3, 2) > 0.55)
    paint(img, blur(rm, 0.6), hexc("#d9d2bf"), 0.22)
    return img


def main():
    os.makedirs(OUT, exist_ok=True)
    for f in os.listdir(OUT):
        if f.endswith(".png"): os.remove(os.path.join(OUT, f))
    g = grime()
    Image.fromarray((np.clip(g, 0, 1) * 255).astype(np.uint8), "RGBA").save(os.path.join(OUT, "g00_grime.png"), optimize=True)
    ELEMENTS.append({"file": "g00_grime.png", "x": 0, "y": 0, "w": 940, "h": 1210, "kind": "fade", "t0": 0.0, "dur": 0.5, "dx": 1, "dy": 0})
    add(throwup(), "g01_throwup.png", 556, 372, "tag", 0.03, 0.36)
    add(sticker_round(), "g02_r4n.png", 40, 40, "sticker", 0.30, 0.05)
    add(sticker_hello(), "g03_hello.png", 872, 302, "sticker", 0.35, 0.06)
    add(sticker_nosamples(), "g04_nosamples.png", 212, 250, "sticker", 0.40, 0.05)
    add(sticker_lofi(), "g05_lofi.png", 176, 332, "sticker", 0.44, 0.05)
    add(sticker_88(), "g06_88.png", 555, 812, "sticker", 0.48, 0.05)
    add(record_sticker(150, 20), "g11_record.png", 928, 706, "sticker", 0.52, 0.05)
    add(round_sticker([("100%", 30), ("VINYL", 26)], "#111113", "#b8ff2e", 104, -10, ring="#b8ff2e"), "g12_vinyl.png", 920, 1150, "sticker", 0.55, 0.05)
    add(bumper("STAY GÆRRO", "#111113", "#b8ff2e", 250, 52, -2), "g13_stay.png", 470, 1196, "sticker", 0.58, 0.05)
    add(bumper("CRATE DIGGER", "#ff8a1f", "#111111", 220, 50, 84), "g14_crate.png", 12, 800, "sticker", 0.61, 0.05)
    add(cassette_sticker(150, 96, 7), "g15_tape_cassette.png", 150, 1046, "sticker", 0.64, 0.05)
    add(gaffer_tape("DON'T TOUCH MY CHOPS", 372, -3), "g07_tape.png", 100, 1182, "tape", 0.68, 0.10)
    add(marker_tag("துடி", "Kavivanar-Regular.ttf", 96, "#d7dade", -5, drip_n=2, outline="#141416", seed=2), "g08_thudi.png", 100, 560, "tag", 0.74, 0.12)
    add(marker_tag("R4N", "SedgwickAve-Regular.ttf", 80, "#ff3fa4", -7, drip_n=3, outline="#101012", seed=6), "g09_r4n_tag.png", 876, 34, "tag", 0.82, 0.08)
    add(rotate(marker_tag("GÆRRO", "SedgwickAve-Regular.ttf", 60, "#f2f0ea", 0, drip_n=1, seed=12), 90), "g16_gap_tag.png", 368, 735, "tag", 0.86, 0.07, 0.0, -1.0)
    add(star(70, "#b8ff2e", 12), "g17_star.png", 928, 470, "tag", 0.90, 0.04)
    add(marker_tag("gærro was here", "PermanentMarker-Regular.ttf", 30, "#e9e7e1", -2, drip_n=0, seed=14), "g18_was_here.png", 760, 1197, "tag", 0.93, 0.06)
    json.dump({"elements": ELEMENTS}, open(os.path.join(OUT, "gaerro_art.json"), "w"), indent=1)
    print("wrote", len(ELEMENTS), "elements to", OUT)
    if "--preview" in sys.argv:
        bg = Image.open(os.path.join(ROOT, "assets/ui/bg.png")).convert("RGBA").resize((940 * 2, 1210 * 2))
        for e in ELEMENTS:
            im = Image.open(os.path.join(OUT, e["file"])).convert("RGBA").resize((int(e["w"] * 2), int(e["h"] * 2)))
            bg.alpha_composite(im, (int(e["x"] * 2), int(e["y"] * 2)))
        bg.save(os.path.join(sys.argv[sys.argv.index("--preview") + 1] if len(sys.argv) > sys.argv.index("--preview") + 1 else "gaerro_preview.png"))


if __name__ == "__main__":
    main()
