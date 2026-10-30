"""Collects everything the browser version needs into web/app/ (the wasm itself: web/engine/build.sh).

  python3 tools/build_web.py
"""
import json, os, shutil
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = os.path.join(ROOT, "web/app")
A = os.path.join(APP, "assets")


def copy(src, dst):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copyfile(src, dst)


def main():
    os.makedirs(A, exist_ok=True)
    ui = os.path.join(ROOT, "assets/ui")
    for f in ["pad_up.png", "pad_down.png", "pad_lit.png", "pad_lit_down.png", "pad_lit_g.png", "pad_lit_g_down.png",
              "knob_black.png", "knob_red.png", "button_up.png", "button_down.png"]:
        if os.path.exists(os.path.join(ui, f)):
            Image.open(os.path.join(ui, f)).save(os.path.join(A, f), optimize=True)
    bg = Image.open(os.path.join(ui, "bg.png")).convert("RGB")
    s = bg.size[0] / 940.0
    crop = lambda x, y, w, h: bg.crop((int(x * s), int(y * s), int((x + w) * s), int((y + h) * s)))
    crop(42, 356, 116, 44).save(os.path.join(A, "plaque.jpg"), quality=92)
    # the துடி emblem, turned to read left -> right for the header
    crop(34, 580, 150, 450).rotate(-90, expand=True).save(os.path.join(A, "emblem.jpg"), quality=90)
    # a clean piece of the satin body for the page background
    crop(16, 1040, 160, 140).save(os.path.join(A, "body.jpg"), quality=88)
    for f in ["VT323-Regular.ttf", "BarlowCondensed-SemiBold.ttf", "BarlowCondensed-Medium.ttf"]:
        copy(os.path.join(ROOT, "assets/fonts", f), os.path.join(A, "fonts", f))
    g = os.path.join(ROOT, "assets/gaerro")
    for f in os.listdir(g):
        if f.endswith(".png"):
            copy(os.path.join(g, f), os.path.join(A, "gaerro", f))
    # presets: every factory kit and pad preset in one file
    kits, pads = [], []
    kd = os.path.join(ROOT, "presets/factory/kits")
    for f in sorted(os.listdir(kd)):
        if f.endswith(".thudikit"):
            kits.append(json.load(open(os.path.join(kd, f))))
    order = {"RHYNO": 0}
    kits.sort(key=lambda k: (order.get(k["name"], 1), k["name"]))
    pd = os.path.join(ROOT, "presets/factory/pads")
    for f in sorted(os.listdir(pd)):
        if f.endswith(".thudipad"):
            cat, name = f[:-9].split("__", 1)
            p = json.load(open(os.path.join(pd, f)))
            pads.append({"category": cat, "name": name, "label": p["label"], "params": p["params"]})
    json.dump({"kits": kits, "pads": pads}, open(os.path.join(APP, "presets.json"), "w"), separators=(",", ":"))
    # app icon from the knob render
    icon = Image.new("RGB", (512, 512), (22, 22, 24))
    k = Image.open(os.path.join(ui, "knob_red.png")).convert("RGBA").resize((400, 400), Image.LANCZOS)
    icon.paste(k, (56, 56), k)
    icon.save(os.path.join(A, "icon-512.png"))
    icon.resize((192, 192), Image.LANCZOS).save(os.path.join(A, "icon-192.png"))
    print("web assets:", len(kits), "kits,", len(pads), "pad presets")


if __name__ == "__main__":
    main()
