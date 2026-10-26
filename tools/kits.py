"""Factory kit definitions for Thudi (sparse: unspecified params use defaults).
Run:  python3 tools/kits.py <outdir>   -> writes <outdir>/<Kit>.thudikit (sparse JSON)
The C++ tool `thudi_render --normalize` rewrites them as full files.
"""
import json, sys, copy

# Model indices (must match ThudiParams.h)
KICK, B808, SNARE, CLAP, CHAT, OHAT, TOM, PERC, RIM, CYM, ZAP, CUSTOM = range(12)

def pad(label, model, **p):
    d = {"model": model}; d.update(p); return {"label": label, "params": d}

# ---------------------------------------------------------------- RHYNO kit
# Values derived from measurements of the reference clip (see docs/RHYNO-analysis.md)
RHYNO_KICK = pad("KICK", KICK, tone_pitch=99, sweep_amt=16, sweep_time=22, sweep_curve=0.25,
                 tone_attack=0.5, tone_hold=2, tone_decay=300, tone_curve=0.55, tone_lp=850,
                 drive=0.35, asym=0.1, link=2, vel_sens=0.4, drift=0.1)
RHYNO_808 = pad("808", B808, tone_pitch=50.6, sweep_amt=4.3, sweep_time=40, sweep_curve=0.0,
                growl=0.8, tone_attack=30, tone_hold=80, tone_decay=420, tone_curve=-0.55, tone_lp=4500,
                drive=0.42, asym=0.62, play_mode=1, release=35, poly=0, glide=70,
                crush_mix=0.62, crush_rate=6840, crush_bits=8, crush_xover=157,
                width=0.9, chorus_rate=0.9, chorus_depth=2.75, mono_below=157, drift=0.45, vel_sens=0.25)
RHYNO_SNARE = pad("SNARE", SNARE, tone_level=0.55, tone_pitch=228, sweep_amt=4, sweep_time=10, tone_decay=160, tone_curve=0.3,
                  noise_level=0.9, noise_filter=1, noise_cutoff=1350, noise_reso=0.08, noise_decay=230, noise_curve=0.3,
                  body_level=0.35, body_freq=87, body_ratio=2.62, body_decay=120, body_excite=0.3,
                  drive=0.35, asym=0.2, width=0.3, vel_sens=0.5)
RHYNO_SNAP = pad("SNAP", PERC, tone_level=0.3, tone_pitch=99, tone_decay=40, sweep_amt=0,
                 noise_level=1.0, noise_filter=1, noise_cutoff=2600, noise_reso=0.35, noise_decay=60, noise_curve=0.4,
                 bursts=3, burst_space=6, burst_decay=4, burst_rand=0.2, drive=0.3, asym=0.2, width=0.35)
RHYNO_CHAT = pad("HAT", CHAT, tone_level=0, metal_level=0.9, metal_set=0, metal_pitch=1.0, metal_tone=6500, metal_q=0.35,
                 metal_decay=70, noise_level=0.3, noise_filter=2, noise_cutoff=6000, noise_decay=40, noise_curve=0.3,
                 choke=1, drive=0.2, width=0.45, high_cut=9500, vel_sens=0.7)
RHYNO_OHAT = copy.deepcopy(RHYNO_CHAT); RHYNO_OHAT["label"] = "OPEN HAT"; RHYNO_OHAT["params"].update(model=OHAT, metal_decay=320, noise_decay=280)
RHYNO_CLAP = pad("CLAP", CLAP, tone_level=0, noise_level=1.0, noise_filter=1, noise_cutoff=1500, noise_reso=0.2,
                 bursts=4, burst_space=8.5, burst_decay=4.2, burst_rand=0.25, noise_decay=110, noise_curve=0.35,
                 drive=0.3, asym=0.2, width=0.35)
RHYNO_RIM = pad("RIM", RIM, tone_level=0.6, tone_pitch=1750, sweep_amt=3, sweep_time=4, tone_decay=30, tone_curve=0.5,
                noise_level=0.4, noise_filter=1, noise_cutoff=3200, noise_reso=0.4, noise_decay=18,
                body_level=0.4, body_freq=480, body_ratio=1.48, body_decay=60, body_excite=0.1, drive=0.25)
RHYNO_808L = copy.deepcopy(RHYNO_808); RHYNO_808L["label"] = "808 LONG"; RHYNO_808L["params"].update(tone_hold=300, tone_decay=1500, tone_curve=-0.3)
RHYNO_808S = copy.deepcopy(RHYNO_808); RHYNO_808S["label"] = "808 SHORT"; RHYNO_808S["params"].update(tone_hold=20, tone_decay=220, tone_curve=0.1, play_mode=0)
RHYNO_TOM = pad("TOM", TOM, tone_pitch=140, sweep_amt=7, sweep_time=60, tone_decay=420, tone_curve=0.3,
                body_level=0.3, body_freq=140, body_ratio=1.5, body_decay=300, noise_level=0.15, noise_cutoff=900, noise_decay=60,
                drive=0.3, width=0.2)
RHYNO_ZAP = pad("ZAP", ZAP, tone_pitch=200, sweep_amt=48, sweep_time=110, sweep_curve=-0.2, tone_decay=160, tone_curve=0.2,
                fm_amt=0.25, fm_ratio=2.0, fm_decay=40, drive=0.3, width=0.5)
RHYNO_KICK2 = copy.deepcopy(RHYNO_KICK); RHYNO_KICK2["label"] = "KICK HARD"; RHYNO_KICK2["params"].update(
                link=0, tone_lp=2500, drive=0.55, noise_level=0.25, noise_filter=1, noise_cutoff=3000, noise_decay=6, sweep_amt=24)
RHYNO_BELL = pad("COWBELL", PERC, tone_level=0, metal_level=1.0, metal_set=3, metal_tone=2400, metal_q=0.5, metal_decay=260, drive=0.2, width=0.3)
RHYNO_CYM = pad("CRASH", CYM, tone_level=0, metal_level=0.8, metal_set=0, metal_pitch=1.3, metal_ring=0.4, metal_tone=7500,
                metal_decay=1800, noise_level=0.35, noise_filter=2, noise_cutoff=5000, noise_decay=1500, width=0.7, high_cut=12000)
RHYNO_SHAKER = pad("SHAKER", PERC, tone_level=0, noise_level=0.8, noise_filter=1, noise_cutoff=7000, noise_reso=0.2,
                   noise_attack=15, noise_decay=90, width=0.5, high_cut=12000)

def kit(name, pads, **glob):
    g = {"master": 0, "bus_drive": 0.15, "oversampling": 1, "keys_pad": 0, "keys_root": 60}; g.update(glob)
    out = {"format": "thudi-kit", "version": 1, "name": name, "global": g, "pads": []}
    for i, p in enumerate(pads):
        q = copy.deepcopy(p); q["note"] = 36 + i; out["pads"].append(q)
    return out

# MPC-style order: pad 1 bottom-left ... pad 16 top-right
RHYNO = kit("RHYNO", [RHYNO_KICK, RHYNO_808, RHYNO_SNARE, RHYNO_SNAP,
                      RHYNO_CHAT, RHYNO_OHAT, RHYNO_CLAP, RHYNO_RIM,
                      RHYNO_808L, RHYNO_808S, RHYNO_TOM, RHYNO_ZAP,
                      RHYNO_KICK2, RHYNO_BELL, RHYNO_CYM, RHYNO_SHAKER], keys_pad=2)

# ---------------------------------------------------------------- 808 CLASSIC (clean, roomy TR-808 flavour)
C_KICK = pad("BOOM KICK", KICK, tone_pitch=48, sweep_amt=14, sweep_time=35, sweep_curve=0.2, tone_decay=900, tone_curve=0.25,
             tone_lp=600, drive=0.1, vel_sens=0.5)
C_808 = pad("808 BASS", B808, tone_pitch=49, sweep_amt=3, sweep_time=50, growl=0.15, tone_attack=2, tone_hold=200, tone_decay=1400,
            tone_curve=-0.2, play_mode=1, poly=0, glide=80, drive=0.12, asym=0.2, tone_lp=3000)
C_SNARE = pad("SNARE", SNARE, tone_level=0.7, tone_pitch=185, sweep_amt=3, sweep_time=8, tone_decay=120, tone_curve=0.4,
              noise_level=0.7, noise_filter=2, noise_cutoff=1800, noise_reso=0.1, noise_decay=180, noise_curve=0.4,
              body_level=0.35, body_freq=330, body_ratio=1.0, body_decay=90, body_excite=0.2)
C_CLAP = pad("CLAP", CLAP, noise_level=1, noise_filter=1, noise_cutoff=1100, noise_reso=0.25, bursts=4, burst_space=10,
             burst_decay=3.5, burst_rand=0.1, noise_decay=180, noise_curve=0.3, tone_level=0, width=0.25)
C_CH = pad("CLOSED HAT", CHAT, tone_level=0, metal_level=1, metal_set=0, metal_tone=8500, metal_q=0.3, metal_decay=45,
           noise_level=0.15, noise_filter=2, noise_cutoff=8000, noise_decay=30, choke=1, high_cut=16000, vel_sens=0.7)
C_OH = pad("OPEN HAT", OHAT, tone_level=0, metal_level=1, metal_set=0, metal_tone=8500, metal_q=0.3, metal_decay=520,
           noise_level=0.15, noise_filter=2, noise_cutoff=8000, noise_decay=380, choke=1, high_cut=16000)
C_RIM = pad("RIMSHOT", RIM, tone_level=0.7, tone_pitch=1700, tone_decay=22, tone_curve=0.6, noise_level=0.2, noise_filter=1,
            noise_cutoff=3500, noise_decay=12, body_level=0.5, body_freq=460, body_ratio=1.48, body_decay=40, body_excite=0.05)
C_BELL = pad("COWBELL", PERC, tone_level=0, metal_level=1, metal_set=3, metal_tone=2200, metal_q=0.55, metal_decay=300)
C_TOML = pad("LOW TOM", TOM, tone_pitch=95, sweep_amt=6, sweep_time=80, tone_decay=600, tone_curve=0.2, body_level=0.2, body_freq=95, body_decay=400,
             noise_level=0.1, noise_cutoff=700, noise_decay=40)
C_TOMM = copy.deepcopy(C_TOML); C_TOMM["label"] = "MID TOM"; C_TOMM["params"].update(tone_pitch=140, body_freq=140, tone_decay=480)
C_TOMH = copy.deepcopy(C_TOML); C_TOMH["label"] = "HI TOM"; C_TOMH["params"].update(tone_pitch=200, body_freq=200, tone_decay=380)
C_CONGA = pad("CONGA", TOM, tone_pitch=330, sweep_amt=4, sweep_time=20, tone_decay=180, tone_curve=0.35)
C_CLAVE = pad("CLAVE", RIM, tone_level=1, tone_pitch=2500, tone_decay=35, tone_curve=0.3, noise_level=0, body_level=0.3, body_freq=2500, body_decay=50)
C_MARACA = pad("MARACAS", PERC, tone_level=0, noise_level=0.8, noise_filter=2, noise_cutoff=6500, noise_decay=45, noise_attack=3, high_cut=14000)
C_CYM = pad("CYMBAL", CYM, tone_level=0, metal_level=0.9, metal_set=0, metal_pitch=1.1, metal_ring=0.25, metal_tone=8000, metal_decay=2500,
            noise_level=0.3, noise_filter=2, noise_cutoff=6000, noise_decay=2000, width=0.4)
C_808L = copy.deepcopy(C_808); C_808L["label"] = "808 LONG"; C_808L["params"].update(tone_hold=400, tone_decay=3000)

CLASSIC = kit("808 CLASSIC", [C_KICK, C_808, C_SNARE, C_CLAP,
                              C_CH, C_OH, C_RIM, C_BELL,
                              C_TOML, C_TOMM, C_TOMH, C_CONGA,
                              C_CLAVE, C_MARACA, C_CYM, C_808L], keys_pad=2, bus_drive=0.05)

# ---------------------------------------------------------------- GRIT LAB (crushed, driven, taped)
def grit(p, **extra):
    q = copy.deepcopy(p); q["params"].update(extra); return q
G_KICK = grit(RHYNO_KICK, label=None, drive=0.7, tape=0.5, crush_mix=0.35, crush_rate=11000, crush_bits=7, crush_xover=300, link=2); G_KICK["label"] = "DIRT KICK"
G_808 = grit(RHYNO_808, drive=0.75, asym=0.8, crush_mix=0.85, crush_rate=4800, crush_bits=6, crush_xover=180, tape=0.4, width=1.0); G_808["label"] = "FUZZ 808"
G_SNARE = grit(RHYNO_SNARE, drive=0.6, crush_mix=0.6, crush_rate=9000, crush_bits=6, crush_xover=200, tape=0.6, width=0.5); G_SNARE["label"] = "CRUSH SNARE"
G_CLAP = grit(RHYNO_CLAP, drive=0.5, crush_mix=0.7, crush_rate=7000, crush_bits=5, tape=0.5, width=0.6); G_CLAP["label"] = "LOFI CLAP"
G_CH = grit(RHYNO_CHAT, crush_mix=0.5, crush_rate=16000, crush_bits=6, tape=0.4); G_CH["label"] = "DUST HAT"
G_OH = grit(RHYNO_OHAT, crush_mix=0.5, crush_rate=16000, crush_bits=6, tape=0.4); G_OH["label"] = "DUST OPEN"
G_SNAP = grit(RHYNO_SNAP, crush_mix=0.6, crush_rate=8000, crush_bits=6, drive=0.5); G_SNAP["label"] = "GRIT SNAP"
G_RIM = grit(RHYNO_RIM, crush_mix=0.7, crush_rate=6000, crush_bits=5, drive=0.5); G_RIM["label"] = "BIT RIM"
G_ZAP = grit(RHYNO_ZAP, crush_mix=0.8, crush_rate=5000, crush_bits=5, drive=0.6, width=0.8); G_ZAP["label"] = "LASER"
G_TOM = grit(RHYNO_TOM, drive=0.6, tape=0.6, crush_mix=0.3, crush_rate=10000, crush_bits=7); G_TOM["label"] = "TAPE TOM"
G_808S = grit(RHYNO_808S, drive=0.8, asym=0.9, crush_mix=0.9, crush_rate=4000, crush_bits=5, crush_xover=150); G_808S["label"] = "BITE 808"
G_BELL = grit(RHYNO_BELL, crush_mix=0.7, crush_rate=6000, crush_bits=5); G_BELL["label"] = "BIT BELL"
G_CYM = grit(RHYNO_CYM, crush_mix=0.6, crush_rate=12000, crush_bits=6, tape=0.5); G_CYM["label"] = "TRASH CRASH"
G_SHAKER = grit(RHYNO_SHAKER, crush_mix=0.6, crush_rate=9000, crush_bits=6); G_SHAKER["label"] = "SAND"
G_KICK2 = grit(RHYNO_KICK2, drive=0.8, tape=0.7, crush_mix=0.5, crush_rate=8000, crush_bits=6); G_KICK2["label"] = "BLOWN KICK"
G_PERC = pad("METAL PERC", PERC, tone_level=0.5, tone_pitch=420, fm_amt=0.6, fm_ratio=3.5, fm_decay=30, tone_decay=90, metal_level=0.4,
             metal_set=2, metal_decay=120, crush_mix=0.6, crush_rate=7000, crush_bits=6, drive=0.4, width=0.6)
for g_ in (G_KICK, G_808, G_SNARE, G_CLAP, G_CH, G_OH, G_SNAP, G_RIM, G_ZAP, G_TOM, G_808S, G_BELL, G_CYM, G_SHAKER, G_KICK2):
    g_["params"].pop("label", None)
GRIT = kit("GRIT LAB", [G_KICK, G_808, G_SNARE, G_SNAP,
                        G_CH, G_OH, G_CLAP, G_RIM,
                        G_808S, G_TOM, G_ZAP, G_PERC,
                        G_KICK2, G_BELL, G_CYM, G_SHAKER], keys_pad=2, bus_drive=0.35)

KITS = {"808 CLASSIC": CLASSIC, "GRIT LAB": GRIT}   # RHYNO is the fitted file in presets/factory/kits

MODEL_CATEGORY = ["Kick", "808", "Snare", "Clap", "Closed Hat", "Open Hat", "Tom", "Perc", "Rim", "Cymbal", "Zap", "Custom"]


def param_defaults(root):
    import re
    txt = open(os.path.join(root, "src/dsp/ThudiParams.h")).read()
    block = txt[txt.index("#define THUDI_PAD_PARAMS"):txt.index("enum class P")]
    d = {}
    for m in re.finditer(r'X\((\w+),\s*"(\w+)",\s*"[^"]*",\s*([-\d.f]+),\s*([-\d.f]+),\s*([-\d.f]+)', block):
        d[m.group(2)] = float(m.group(5).rstrip("f"))
    return d


def normalise_kit(k, defaults):
    for p in k["pads"]:
        full = dict(defaults); full.update(p["params"]); p["params"] = full
    return k

import os

if __name__ == "__main__":
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    kdir = os.path.join(root, "presets/factory/kits"); pdir = os.path.join(root, "presets/factory/pads")
    os.makedirs(kdir, exist_ok=True); os.makedirs(pdir, exist_ok=True)
    defaults = param_defaults(root)
    for name, k in KITS.items():
        json.dump(normalise_kit(copy.deepcopy(k), defaults), open(os.path.join(kdir, name + ".thudikit"), "w"), indent=2)
    # normalise RHYNO (fitted) too and export pad presets from every factory kit
    for f in os.listdir(pdir):
        if f.endswith(".thudipad"): os.remove(os.path.join(pdir, f))
    count = 0
    for f in sorted(os.listdir(kdir)):
        if not f.endswith(".thudikit"): continue
        k = json.load(open(os.path.join(kdir, f)))
        k = normalise_kit(k, defaults)
        json.dump(k, open(os.path.join(kdir, f), "w"), indent=2)
        for p in k["pads"]:
            params = dict(p["params"]); params["link"] = 0; params["output"] = 0
            cat = MODEL_CATEGORY[int(round(params["model"]))]
            name = (k["name"] + " " + p["label"]).title().replace("Rhyno", "RHYNO").replace("808 Classic", "808 Classic")
            fn = cat + "__" + name.replace("/", "-") + ".thudipad"
            json.dump({"format": "thudi-pad", "version": 1, "label": p["label"], "params": params}, open(os.path.join(pdir, fn), "w"), indent=2)
            count += 1
    print("kits:", sorted(os.listdir(kdir)), "pad presets:", count)
