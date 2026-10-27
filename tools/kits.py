"""Factory kit definitions for Thudi (sparse: unspecified params use defaults).
Run:  python3 tools/kits.py <outdir>   -> writes <outdir>/<Kit>.thudikit (sparse JSON)
The C++ tool `thudi_render --normalize` rewrites them as full files.
"""
import json, sys, copy

# Model indices (must match ThudiParams.h)
KICK, B808, SNARE, CLAP, CHAT, OHAT, TOM, PERC, RIM, CYM, ZAP, CUSTOM, SAMPLE, EMPTY = range(14)

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
    assert len(pads) == 16, name
    assert sum(1 for p in pads if p["params"]["model"] == B808) <= 1, name + ": one 808 per kit"
    out = {"format": "thudi-kit", "version": 1, "name": name, "global": g, "pads": []}
    for i, p in enumerate(pads):
        q = copy.deepcopy(p); q["note"] = 36 + i; out["pads"].append(q)
    return out

# MPC-style order: pad 1 bottom-left ... pad 16 top-right
RHYNO = kit("RHYNO", [RHYNO_KICK, RHYNO_808, RHYNO_SNARE, RHYNO_SNAP,
                      RHYNO_CHAT, RHYNO_OHAT, RHYNO_CLAP, RHYNO_RIM,
                      RHYNO_CYM, RHYNO_SNAP, RHYNO_TOM, RHYNO_ZAP,   # (unused: the fitted file is the real RHYNO kit)
                      RHYNO_KICK2, RHYNO_BELL, RHYNO_CYM, RHYNO_SHAKER])

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

C_CONGAH = copy.deepcopy(C_CONGA); C_CONGAH["label"] = "HI CONGA"; C_CONGAH["params"].update(tone_pitch=440, tone_decay=140)
CLASSIC = kit("808 CLASSIC", [C_KICK, C_808, C_SNARE, C_CLAP,
                              C_CH, C_OH, C_RIM, C_BELL,
                              C_TOML, C_TOMM, C_TOMH, C_CONGA,
                              C_CLAVE, C_MARACA, C_CYM, C_CONGAH], bus_drive=0.05)

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
G_NOISE = pad("NOISE HIT", PERC, tone_level=0.2, tone_pitch=180, tone_decay=60, noise_level=1, noise_filter=1, noise_cutoff=1800, noise_reso=0.5,
              noise_env=0.6, noise_decay=140, noise_curve=0.2, crush_mix=0.8, crush_rate=5000, crush_bits=5, drive=0.6, width=0.7)
GRIT = kit("GRIT LAB", [G_KICK, G_808, G_SNARE, G_SNAP,
                        G_CH, G_OH, G_CLAP, G_RIM,
                        G_NOISE, G_TOM, G_ZAP, G_PERC,
                        G_KICK2, G_BELL, G_CYM, G_SHAKER], bus_drive=0.35)

# ================================================================ more kits (one 808 per kit: the deep bass lives in SOLO mode)
def kick(label, pitch, sweep, stime, decay, curve=0.45, lp=1500, drive=0.3, click=0.0, **x):
    d = dict(tone_pitch=pitch, sweep_amt=sweep, sweep_time=stime, sweep_curve=0.25, tone_decay=decay, tone_curve=curve, tone_lp=lp, drive=drive)
    if click: d.update(noise_level=click, noise_filter=1, noise_cutoff=4000, noise_decay=5, noise_curve=0.5)
    d.update(x); return pad(label, KICK, **d)
def b808(label, pitch, hold, decay, glide=0, growl=0.3, drive=0.3, **x):
    d = dict(tone_pitch=pitch, sweep_amt=3, sweep_time=40, growl=growl, tone_attack=2, tone_hold=hold, tone_decay=decay, tone_curve=-0.3,
             play_mode=1, poly=0, glide=glide, drive=drive, asym=0.3, tone_lp=4000, mono_below=150)
    d.update(x); return pad(label, B808, **d)
def snare(label, pitch, ndecay, cutoff=1800, tone=0.55, drive=0.3, **x):
    d = dict(tone_level=tone, tone_pitch=pitch, sweep_amt=4, sweep_time=10, tone_decay=120, tone_curve=0.35, noise_level=0.9, noise_filter=1,
             noise_cutoff=cutoff, noise_reso=0.1, noise_decay=ndecay, noise_curve=0.3, body_level=0.3, body_freq=pitch * 0.9, body_decay=100, drive=drive)
    d.update(x); return pad(label, SNARE, **d)
def clap(label, cutoff=1400, decay=130, bursts=4, space=9, **x):
    d = dict(tone_level=0, noise_level=1, noise_filter=1, noise_cutoff=cutoff, noise_reso=0.22, bursts=bursts, burst_space=space, burst_decay=4,
             burst_rand=0.25, noise_decay=decay, noise_curve=0.35, width=0.35)
    d.update(x); return pad(label, CLAP, **d)
def hat(label, decay, tone=7000, model=CHAT, choke=1, **x):
    d = dict(tone_level=0, metal_level=0.9, metal_set=0, metal_tone=tone, metal_q=0.3, metal_decay=decay, noise_level=0.25, noise_filter=2,
             noise_cutoff=tone - 800, noise_decay=decay * 0.7, choke=choke, high_cut=15000, vel_sens=0.7, width=0.3)
    d.update(x); return pad(label, model, **d)
def tom(label, pitch, decay=420, **x):
    d = dict(tone_pitch=pitch, sweep_amt=7, sweep_time=60, tone_decay=decay, tone_curve=0.3, body_level=0.25, body_freq=pitch, body_decay=decay * 0.7,
             noise_level=0.1, noise_cutoff=900, noise_decay=40)
    d.update(x); return pad(label, TOM, **d)
def perc(label, **x): return pad(label, PERC, **x)
def rim(label, pitch=1700, **x):
    d = dict(tone_level=0.6, tone_pitch=pitch, tone_decay=28, tone_curve=0.5, noise_level=0.35, noise_filter=1, noise_cutoff=3300, noise_decay=15,
             body_level=0.4, body_freq=470, body_decay=50); d.update(x); return pad(label, RIM, **d)
def cym(label, decay=1800, pitch=1.3, **x):
    d = dict(tone_level=0, metal_level=0.8, metal_set=0, metal_pitch=pitch, metal_ring=0.35, metal_tone=7500, metal_decay=decay,
             noise_level=0.3, noise_filter=2, noise_cutoff=5000, noise_decay=decay * 0.8, width=0.6, high_cut=13000); d.update(x); return pad(label, CYM, **d)
def zap(label, pitch=220, sweep=40, stime=100, decay=180, **x):
    d = dict(tone_pitch=pitch, sweep_amt=sweep, sweep_time=stime, sweep_curve=-0.2, tone_decay=decay, tone_curve=0.2, fm_amt=0.2, fm_ratio=2.0, width=0.5)
    d.update(x); return pad(label, ZAP, **d)
def shaker(label, decay=80, cutoff=7000, **x):
    d = dict(tone_level=0, noise_level=0.8, noise_filter=1, noise_cutoff=cutoff, noise_reso=0.2, noise_attack=12, noise_decay=decay, width=0.5, high_cut=13000)
    d.update(x); return pad(label, PERC, **d)
def cowbell(label, tone=2300, decay=260, **x):
    d = dict(tone_level=0, metal_level=1, metal_set=3, metal_tone=tone, metal_q=0.5, metal_decay=decay); d.update(x); return pad(label, PERC, **d)

ATL = kit("ATLANTA", [
    kick("KICK", 62, 22, 28, 320, click=0.3, drive=0.45),     b808("808", 46, 150, 1300, glide=60, growl=0.3, drive=0.35),
    snare("SNARE", 210, 190, cutoff=2200, drive=0.4),          clap("CLAP", 1600, 140),
    hat("HAT", 55, 8200),                                       hat("OPEN HAT", 420, 8200, OHAT),
    hat("HAT ROLL", 30, 9000, vel_sens=0.9),                    rim("RIM"),
    snare("SNARE 2", 260, 140, cutoff=3000, tone=0.4),          clap("CLAP WIDE", 1300, 200, width=0.9, chorus_depth=4),
    perc("PERC", tone_level=0.6, tone_pitch=900, tone_decay=70, fm_amt=0.4, fm_ratio=3.1, noise_level=0.2, noise_cutoff=5000, noise_decay=30),
    tom("TOM", 120),
    zap("RISER", 120, 36, 900, 900, sweep_curve=0.5, play_mode=1),  cym("CRASH"),
    cowbell("BELL", 2600, 180, crush_mix=0.3, crush_rate=12000, crush_bits=8), shaker("SHAKER")], bus_drive=0.2)

BOOMBAP = kit("BOOM BAP", [
    kick("KICK", 54, 12, 40, 420, curve=0.3, lp=700, drive=0.35, tape=0.6),   b808("BASS", 52, 60, 480, growl=0.1, drive=0.25, tone_lp=1200, tape=0.5),
    snare("SNARE", 190, 240, cutoff=1500, drive=0.45, tape=0.6, crush_mix=0.25, crush_rate=26000, crush_bits=12),
    rim("RIM", 1500, tape=0.5),
    hat("HAT", 70, 6500, tape=0.5, crush_mix=0.4, crush_rate=26000, crush_bits=12),   hat("OPEN HAT", 360, 6500, OHAT, tape=0.5),
    snare("SNARE GHOST", 200, 120, cutoff=1300, vel_sens=0.9),   clap("CLAP", 1200, 160, tape=0.5),
    kick("KICK 2", 60, 10, 30, 300, lp=900, drive=0.5, tape=0.7),   shaker("SHAKER", 110, 6000, tape=0.4),
    tom("TOM", 110, tape=0.5),   perc("SNAP", tone_level=0.2, noise_level=1, noise_filter=1, noise_cutoff=2500, noise_reso=0.3, noise_decay=50, bursts=2, burst_space=5),
    cym("RIDE", 1300, 1.7, metal_set=2, metal_ring=0.5),   cym("CRASH", 1800, tape=0.5),
    cowbell("BELL", 2100, 200, tape=0.5),   perc("STICKS", tone_level=0.7, tone_pitch=2100, tone_decay=25, noise_level=0.3, noise_filter=2, noise_cutoff=4000, noise_decay=10)],
    bus_drive=0.35)

DRILL = kit("DRILL", [
    kick("KICK", 60, 18, 26, 280, click=0.35, drive=0.5),   b808("SLIDE 808", 49, 220, 1900, glide=140, growl=0.5, drive=0.5, sweep_amt=2),
    snare("SNARE", 280, 170, cutoff=3200, tone=0.5, drive=0.45), rim("RIM", 1900),
    hat("HAT", 45, 9000),   hat("OPEN HAT", 350, 9000, OHAT),   hat("HAT TRIP", 28, 9500, vel_sens=0.95),   clap("CLAP", 1700, 120),
    snare("SNARE 2", 320, 120, cutoff=4000, tone=0.35),   perc("PERC", tone_level=0.7, tone_pitch=1300, tone_decay=55, fm_amt=0.5, fm_ratio=2.4),
    perc("PERC 2", tone_level=0.7, tone_pitch=760, tone_decay=80, fm_amt=0.3, fm_ratio=1.5, noise_level=0.15, noise_cutoff=4000, noise_decay=20),
    tom("TOM", 150, 300),   zap("SWEEP", 300, 30, 400, 500, play_mode=1),   cym("CRASH", 1600),
    cowbell("BELL", 3100, 140),   shaker("SHAKER", 70)], bus_drive=0.25)

MEMPHIS = kit("MEMPHIS", [
    kick("KICK", 56, 14, 40, 500, lp=900, drive=0.45, crush_mix=0.3, crush_rate=16000, crush_bits=10),
    b808("808", 48, 120, 900, growl=0.4, drive=0.6, asym=0.6, crush_mix=0.4, crush_rate=10000, crush_bits=8, crush_xover=200),
    snare("SNARE", 200, 200, cutoff=1700, drive=0.5, crush_mix=0.4, crush_rate=14000, crush_bits=9), clap("CLAP", 1300, 180, crush_mix=0.4, crush_rate=12000, crush_bits=8),
    hat("HAT", 60, 7800, crush_mix=0.5, crush_rate=16000, crush_bits=8), hat("OPEN HAT", 450, 7800, OHAT, crush_mix=0.5, crush_rate=16000, crush_bits=8),
    cowbell("COWBELL", 2400, 320, crush_mix=0.5, crush_rate=12000, crush_bits=8), rim("RIM", 1600, crush_mix=0.4, crush_rate=12000, crush_bits=8),
    tom("LOW TOM", 95, 600), tom("HI TOM", 180, 400), perc("CLAVE", tone_level=1, tone_pitch=2400, tone_decay=40),
    shaker("SHAKER", 90, 6500, crush_mix=0.4, crush_rate=14000, crush_bits=8),
    zap("ZAP", 260, 44, 120, 200, crush_mix=0.5, crush_rate=9000, crush_bits=7), cym("CRASH", 2000, crush_mix=0.4, crush_rate=14000, crush_bits=8),
    snare("SNARE 2", 240, 150, cutoff=2400, crush_mix=0.6, crush_rate=9000, crush_bits=7), perc("TRIANGLE", tone_level=0, metal_level=0.9, metal_set=2, metal_pitch=2.5, metal_q=0.8, metal_decay=900)],
    bus_drive=0.4)

ELECTRO = kit("ELECTRO", [
    kick("KICK", 58, 26, 50, 600, curve=0.3, lp=1200, drive=0.3), b808("SUB", 44, 100, 700, growl=0.2, fm_amt=0.25, fm_ratio=2.0, play_mode=0),
    snare("SNARE", 240, 160, cutoff=2600, tone=0.7, sweep_amt=10, sweep_time=20), clap("CLAP", 1500, 150, chorus_depth=3, width=0.8),
    hat("HAT", 40, 9500, metal_set=1), hat("OPEN HAT", 300, 9500, OHAT, metal_set=1),
    zap("LASER", 400, 48, 120, 160), zap("ZAP DOWN", 900, 36, 60, 120, fm_amt=0.5),
    tom("SYN TOM LO", 90, 500, sweep_amt=18, sweep_time=150), tom("SYN TOM HI", 160, 400, sweep_amt=18, sweep_time=150),
    cowbell("COWBELL", 2600, 160), rim("RIM", 2000),
    perc("BLIP", tone_level=1, tone_pitch=1600, tone_decay=40, fm_amt=0.6, fm_ratio=4.0), shaker("SHAKER", 60, 8000),
    cym("CRASH", 1500, metal_set=1), zap("BOMB", 80, 50, 600, 1200, play_mode=1, drive=0.5)], bus_drive=0.2)

HOUSE = kit("HOUSE", [
    kick("KICK", 52, 24, 45, 380, click=0.25, lp=2000, drive=0.25), b808("SUB", 55, 40, 300, growl=0.1, play_mode=0, drive=0.2),
    clap("CLAP", 1300, 170, width=0.6, chorus_depth=3), snare("SNARE", 220, 160, cutoff=2400, tone=0.45),
    hat("HAT", 50, 8500), hat("OPEN HAT", 260, 8500, OHAT, choke=1), shaker("SHAKER", 70, 8000), rim("RIM", 1750),
    tom("TOM LO", 100), tom("TOM HI", 170), perc("CONGA", tone_pitch=330, tone_decay=160, sweep_amt=4, sweep_time=20),
    cowbell("BELL", 2500, 120), cym("RIDE", 1100, 1.8, metal_set=2, metal_ring=0.6), cym("CRASH", 1700),
    perc("TAMB", tone_level=0, metal_level=0.6, metal_set=1, metal_tone=9000, metal_decay=120, noise_level=0.5, noise_filter=2, noise_cutoff=8000, noise_decay=100),
    clap("CLAP 2", 1800, 110, bursts=3)], bus_drive=0.15)

RAGE = kit("RAGE", [
    kick("KICK", 58, 30, 25, 260, click=0.4, drive=0.85, asym=0.5, crush_mix=0.3, crush_rate=14000, crush_bits=8),
    b808("DIST 808", 47, 200, 1500, glide=80, growl=0.9, drive=0.9, asym=0.8, crush_mix=0.5, crush_rate=8000, crush_bits=7, crush_xover=220, width=0.8),
    snare("SNARE", 230, 200, cutoff=2500, drive=0.7, crush_mix=0.4, crush_rate=12000, crush_bits=7), clap("CLAP", 1500, 160, drive=0.6, width=0.8),
    hat("HAT", 45, 9000, drive=0.4), hat("OPEN HAT", 380, 9000, OHAT, drive=0.4),
    zap("ZAP", 300, 48, 90, 160, drive=0.7, crush_mix=0.6, crush_rate=8000, crush_bits=6), zap("LASER UP", 200, -0, 10, 300, sweep_amt=0, fm_amt=0.7, fm_ratio=5.0, drive=0.6),
    snare("SNARE 2", 300, 120, cutoff=3800, drive=0.8), perc("STAB", tone_level=0.8, tone_pitch=520, tone_decay=120, growl=0.8, fm_amt=0.5, fm_ratio=1.5, drive=0.8, width=0.7),
    rim("RIM", 1800, drive=0.6), tom("TOM", 130, drive=0.6),
    cym("CRASH", 1500, drive=0.4), zap("BOOM", 70, 40, 500, 1100, play_mode=1, drive=0.8),
    hat("HAT ROLL", 25, 9500, vel_sens=0.9, drive=0.4), shaker("SHAKER", 70, drive=0.3)], bus_drive=0.45)

LOFI = kit("LOFI DUST", [
    kick("KICK", 55, 10, 40, 380, lp=600, drive=0.3, tape=0.8, crush_mix=0.3, crush_rate=22000, crush_bits=11),
    b808("WARM SUB", 50, 80, 650, growl=0.1, drive=0.2, tone_lp=900, tape=0.7, play_mode=1),
    snare("SNARE", 185, 200, cutoff=1300, drive=0.35, tape=0.8, high_cut=7000), rim("RIM", 1400, tape=0.7, high_cut=7000),
    hat("HAT", 75, 6000, tape=0.7, high_cut=8000), hat("OPEN HAT", 300, 6000, OHAT, tape=0.7, high_cut=8000),
    shaker("SHAKER", 120, 5000, tape=0.6, high_cut=8000), clap("SNAP", 2200, 60, bursts=2, space=5, tape=0.6),
    kick("KICK SOFT", 52, 8, 50, 300, lp=450, drive=0.2, tape=0.8), snare("SNARE BRUSH", 200, 320, cutoff=1100, tone=0.2, noise_attack=20, tape=0.7),
    tom("TOM", 105, tape=0.7, high_cut=6000), perc("WOOD", tone_level=0.8, tone_pitch=900, tone_decay=50, body_level=0.4, body_freq=600, body_decay=40, tape=0.6),
    cym("RIDE", 1200, 1.6, metal_set=2, tape=0.6, high_cut=7000), cowbell("BELL", 1900, 220, tape=0.7, high_cut=6000),
    perc("VINYL TICK", tone_level=0, noise_level=0.6, noise_filter=2, noise_cutoff=3000, noise_decay=3, tape=0.5), zap("TAPE STOP", 300, -24, 600, 600, sweep_amt=24, sweep_curve=0.6, tape=0.8)],
    bus_drive=0.3)

# extra pad presets for the sound library (bass voices for SOLO mode, leads)
EXTRA_PADS = [
    b808("Solo Sub Clean", 45, 300, 2000, growl=0.0, drive=0.1, tone_lp=900),
    b808("Solo 808 Long", 47, 400, 3000, growl=0.3, drive=0.3),
    b808("Solo 808 Short", 47, 30, 320, growl=0.3, drive=0.3, play_mode=0),
    b808("Solo Glide 808", 46, 250, 2200, glide=110, growl=0.4, drive=0.4),
    b808("Solo Dist 808", 46, 250, 1800, glide=60, growl=0.9, drive=0.9, asym=0.8, crush_mix=0.4, crush_rate=9000, crush_bits=7, crush_xover=200),
    b808("Solo Knock 808", 48, 120, 1100, growl=0.5, drive=0.6, sweep_amt=14, sweep_time=25, noise_level=0.3, noise_filter=1, noise_cutoff=3000, noise_decay=8),
    b808("Solo Drill Slide", 49, 300, 2400, glide=160, growl=0.6, drive=0.55),
    b808("Solo Reese 808", 46, 300, 2000, growl=0.7, fm_amt=0.3, fm_ratio=1.01, fm_decay=2000, drive=0.5, width=1.0, chorus_depth=4, chorus_rate=0.4),
    b808("Solo RHYNO 808 Long", 50.6, 300, 1500, glide=70, growl=0.8, drive=0.42, asym=0.62, crush_mix=0.62, crush_rate=6840, crush_bits=8, crush_xover=157, width=0.9, chorus_depth=2.75),
    b808("Solo RHYNO 808 Short", 50.6, 20, 220, growl=0.8, drive=0.42, asym=0.62, crush_mix=0.62, crush_rate=6840, crush_bits=8, crush_xover=157, width=0.9, play_mode=0),
    pad("Solo Bell Lead", PERC, tone_level=0.5, tone_pitch=440, tone_decay=900, fm_amt=0.5, fm_ratio=3.5, fm_decay=400, metal_level=0.3, metal_set=2, metal_decay=700, play_mode=1, poly=1, width=0.5),
    pad("Solo Zap Lead", ZAP, tone_pitch=330, sweep_amt=5, sweep_time=40, tone_hold=200, tone_decay=800, fm_amt=0.3, fm_ratio=2, play_mode=1, width=0.5),
    tom("Solo Tom Bass", 60, 900, play_mode=1),
]

KITS = {"808 CLASSIC": CLASSIC, "GRIT LAB": GRIT, "ATLANTA": ATL, "BOOM BAP": BOOMBAP, "DRILL": DRILL, "MEMPHIS": MEMPHIS,
        "ELECTRO": ELECTRO, "HOUSE": HOUSE, "RAGE": RAGE, "LOFI DUST": LOFI}   # RHYNO is the fitted file in presets/factory/kits

MODEL_CATEGORY = ["Kick", "808", "Snare", "Clap", "Closed Hat", "Open Hat", "Tom", "Perc", "Rim", "Cymbal", "Zap", "Custom", "Sample", "Empty"]


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
    # RHYNO: one 808 per kit (pads 9/10 used to be 808 LONG / SHORT; those now live in the library for SOLO mode)
    rf = os.path.join(kdir, "RHYNO.thudikit")
    rk = json.load(open(rf))
    if rk["pads"][8]["label"] == "808 LONG":
        ride = cym("RIDE", 1100, 1.7, metal_set=2, metal_ring=0.5, drive=0.2, width=0.5)
        sn2 = snare("SNARE 2", 250, 150, cutoff=2600, tone=0.45, drive=0.35, width=0.3)
        for i, p in ((8, ride), (9, sn2)):
            rk["pads"][i] = {"label": p["label"], "note": 36 + i, "params": p["params"]}
        rk["global"]["keys_pad"] = 0
        json.dump(rk, open(rf, "w"), indent=2)
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
    for p in EXTRA_PADS:
        params = dict(defaults); params.update(p["params"]); params["link"] = 0; params["output"] = 0
        cat = MODEL_CATEGORY[int(round(params["model"]))]
        label = p["label"].replace("Solo ", "").upper()
        json.dump({"format": "thudi-pad", "version": 1, "label": label, "params": params}, open(os.path.join(pdir, cat + "__" + p["label"] + ".thudipad"), "w"), indent=2)
        count += 1
    print("kits:", sorted(os.listdir(kdir)), "pad presets:", count)
