"""
Thudi (துடி) by R4N — procedural 3D model of the panel, rendered with Cycles.
Inspired by the look of a black limited-edition 90s drum workstation: satin black body,
tone-on-tone engraved vines with silver accents, grey rubber pads, brass plaque, blue LCD.
No third-party logos or trademarks are modelled.

Run (Blender 4.2 LTS):
  blender -b --factory-startup -P art/blender/render_ui.py -- <out_dir> [samples] [quick]

Outputs (2x design size, PNG RGBA):
  bg.png, pad_up.png, pad_down.png, knob_black.png, knob_red.png, wheel.png, button_up.png, button_down.png
"""
import bpy, bmesh, json, math, os, random, sys
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = os.path.abspath(argv[0] if argv else "assets/ui")
SAMPLES = int(argv[1]) if len(argv) > 1 else 96
QUICK = len(argv) > 2 and argv[2] == "quick"
HERE = os.path.dirname(os.path.abspath(__file__))
LAY = json.load(open(os.path.join(HERE, "layout.json")))
os.makedirs(OUT, exist_ok=True)

S = 0.01            # 1 design unit = 0.01 Blender units
W, H = LAY["W"], LAY["H"]
SCALE = 0.25 if QUICK else 2.0   # pixels per design unit


def P(x, y, z=0.0):
    """design coords (y down) -> world"""
    return Vector((x * S, -y * S, z))


# ------------------------------------------------------------------ scene setup
def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'
    sc.cycles.device = 'CPU'
    sc.cycles.samples = SAMPLES
    sc.cycles.use_denoising = True
    try:
        sc.cycles.denoiser = 'OPENIMAGEDENOISE'
    except Exception:
        pass
    sc.cycles.max_bounces = 6
    sc.render.image_settings.file_format = 'PNG'
    sc.render.image_settings.color_mode = 'RGBA'
    sc.render.image_settings.color_depth = '8'
    sc.view_settings.view_transform = 'Standard'
    sc.view_settings.look = 'None'
    sc.render.film_transparent = False
    world = bpy.data.worlds.new("World")
    sc.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes["Background"]
    bg.inputs[0].default_value = (0.05, 0.05, 0.055, 1)
    bg.inputs[1].default_value = 0.15
    return sc


def set_input(node, names, value):
    for n in (names if isinstance(names, (list, tuple)) else [names]):
        if n in node.inputs:
            node.inputs[n].default_value = value
            return True
    return False


def material(name, base, rough=0.5, metal=0.0, coat=0.0, aniso=0.0, bump=None, spec=0.5):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    set_input(bsdf, "Base Color", (*base, 1))
    set_input(bsdf, "Roughness", rough)
    set_input(bsdf, "Metallic", metal)
    set_input(bsdf, ["Coat Weight", "Clearcoat"], coat)
    set_input(bsdf, ["Anisotropic"], aniso)
    set_input(bsdf, ["Specular IOR Level", "Specular"], spec)
    if bump:
        kind, scale, strength = bump
        tex = nt.nodes.new("ShaderNodeTexNoise") if kind == "noise" else nt.nodes.new("ShaderNodeTexWave")
        if kind == "noise":
            tex.inputs["Scale"].default_value = scale
            tex.inputs["Detail"].default_value = 8.0
        else:
            tex.wave_type = 'RINGS' if kind == "rings" else 'BANDS'
            tex.inputs["Scale"].default_value = scale
        bp = nt.nodes.new("ShaderNodeBump")
        bp.inputs["Strength"].default_value = strength
        nt.links.new(tex.outputs[0], bp.inputs["Height"])
        nt.links.new(bp.outputs["Normal"], bsdf.inputs["Normal"])
    return m


def rounded_box(name, x, y, w, h, z0, z1, radius, rim=0.0, mat=None, segs=10):
    """axis-aligned box in design coords (x,y,w,h), heights in BU; rounded vertical corners, optional rim bevel"""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co.x = (x + (w * (v.co.x + 0.5))) * S
        v.co.y = -(y + (h * (v.co.y + 0.5))) * S
        v.co.z = z0 + (z1 - z0) * (v.co.z + 0.5)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    # bevel weight on vertical edges -> rounded corners
    if radius > 0:
        attr = me.attributes.get("bevel_weight_edge") or me.attributes.new("bevel_weight_edge", 'FLOAT', 'EDGE')
        for e in me.edges:
            a, b = me.vertices[e.vertices[0]].co, me.vertices[e.vertices[1]].co
            attr.data[e.index].value = 1.0 if abs(a.z - b.z) > 1e-6 else 0.0
        md = ob.modifiers.new("corners", 'BEVEL')
        md.limit_method = 'WEIGHT'
        md.width = radius * S
        md.segments = segs
        md.affect = 'EDGES'
    if rim > 0:
        md2 = ob.modifiers.new("rim", 'BEVEL')
        md2.limit_method = 'ANGLE'
        md2.angle_limit = math.radians(60)
        md2.width = rim * S
        md2.segments = 4
        md2.harden_normals = False
    if mat:
        ob.data.materials.append(mat)
    for p in ob.data.polygons:
        p.use_smooth = True
    return ob


def fix_shading(ob):
    """flat faces stay flat (no smooth-shading streaks across big triangles); only bevels/curves are smoothed"""
    me = ob.data
    for p in me.polygons:
        n = p.normal
        p.use_smooth = not (abs(n.x) > 0.9995 or abs(n.y) > 0.9995 or abs(n.z) > 0.9995)


def apply_all(ob):
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)
    for md in list(ob.modifiers):
        with bpy.context.temp_override(object=ob, active_object=ob):
            bpy.ops.object.modifier_apply(modifier=md.name)
    ob.select_set(False)


def cylinder(name, cx, cy, r, z0, z1, mat=None, verts=96, rim=0.0, r_top=None, ribs=0):
    """cylinder / frustum (r_top) with optional vertical ribs (knurling) on the side"""
    bm = bmesh.new()
    rt = r if r_top is None else r_top
    segs = ribs * 2 if ribs else verts
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segs, radius1=r * S, radius2=rt * S, depth=(z1 - z0))
    if ribs:
        for v in bm.verts:
            a = math.atan2(v.co.y, v.co.x)
            k = int(round(a / (2 * math.pi) * segs)) % segs
            if k % 2:
                v.co.x *= 0.965; v.co.y *= 0.965
    for v in bm.verts:
        v.co.x += cx * S
        v.co.y += -cy * S
        v.co.z += (z0 + z1) * 0.5
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me); bm.free()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    if rim > 0:
        md = ob.modifiers.new("rim", 'BEVEL'); md.limit_method = 'ANGLE'; md.width = rim * S; md.segments = 5
    if mat:
        ob.data.materials.append(mat)
    for p in ob.data.polygons:
        p.use_smooth = True
    return ob


def boolean_cut(target, cutters):
    for c in cutters:
        apply_all(c)
    coll = bpy.data.collections.new("cutters")
    bpy.context.scene.collection.children.link(coll)
    for c in cutters:
        for uc in c.users_collection:
            uc.objects.unlink(c)
        coll.objects.link(c)
    md = target.modifiers.new("wells", 'BOOLEAN')
    md.operation = 'DIFFERENCE'
    md.operand_type = 'COLLECTION'
    md.collection = coll
    md.solver = 'EXACT'
    coll.hide_render = True
    for c in cutters:
        c.hide_render = True
        c.hide_viewport = True


def camera(cx, cy, size_w, size_h, px_w, px_h):
    sc = bpy.context.scene
    cam_data = bpy.data.cameras.new("cam")
    cam_data.type = 'ORTHO'
    cam_data.ortho_scale = max(size_w, size_h) * S
    cam = bpy.data.objects.new("cam", cam_data)
    sc.collection.objects.link(cam)
    cam.location = (cx * S, -cy * S, 5.0)
    cam.rotation_euler = (0, 0, 0)
    sc.camera = cam
    sc.render.resolution_x = int(px_w)
    sc.render.resolution_y = int(px_h)
    sc.render.resolution_percentage = 100
    cam_data.clip_end = 20
    return cam


def lights(cx, cy, span):
    """soft key light from the top-left, cool fill from the right, rim from the bottom"""
    sc = bpy.context.scene
    def area(name, pos, energy, size, color=(1, 1, 1)):
        ld = bpy.data.lights.new(name, 'AREA')
        ld.energy = energy
        ld.size = size
        ld.color = color
        lo = bpy.data.objects.new(name, ld)
        sc.collection.objects.link(lo)
        lo.location = pos
        d = Vector((cx * S, -cy * S, 0)) - Vector(pos)
        lo.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
        return lo
    k = span * S
    area("key", (cx * S - 0.8 * k, -cy * S + 0.9 * k, 0.75 * k), 16 * k * k, 0.35 * k, (1.0, 0.96, 0.9))
    area("fill", (cx * S + 1.1 * k, -cy * S + 0.3 * k, 1.4 * k), 4 * k * k, 1.6 * k, (0.82, 0.88, 1.0))
    area("top", (cx * S, -cy * S, 2.5 * k), 3 * k * k, 2.0 * k, (1.0, 1.0, 1.0))


def render(path):
    bpy.context.scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print("rendered", path, flush=True)


# ------------------------------------------------------------------ materials
def body_material():
    return material("body", (0.022, 0.022, 0.024), rough=0.62, coat=0.0, bump=("noise", 900.0, 0.05), spec=0.35)


def engraving_materials():
    gloss = material("engrave_gloss", (0.03, 0.03, 0.034), rough=0.12, coat=0.6, spec=0.6)
    silver = material("engrave_silver", (0.42, 0.42, 0.44), rough=0.33, metal=0.75)
    return gloss, silver


# ------------------------------------------------------------------ ornaments (vines)
def vine_points(x0, y0, length, angle, curl, rnd):
    pts = []
    x, y, a = x0, y0, angle
    step = 3.0
    n = int(length / step)
    for i in range(n):
        pts.append((x, y))
        a += curl * (0.6 + 0.4 * math.sin(i * 0.12 + rnd.random()))
        x += step * math.cos(a)
        y += step * math.sin(a)
    # spiral end
    r = 14.0
    for i in range(40):
        a += 0.22 * (1 if curl >= 0 else -1)
        r *= 0.95
        x += r * 0.22 * math.cos(a)
        y += r * 0.22 * math.sin(a)
        pts.append((x, y))
    return pts


def leaf_points(x, y, ang, size):
    pts = []
    for i in range(14):
        t = i / 13.0
        w = math.sin(math.pi * t) * size * 0.35
        px = t * size
        pts.append((px, w))
    for i in range(14):
        t = 1 - i / 13.0
        w = -math.sin(math.pi * t) * size * 0.35
        pts.append((t * size, w))
    c, s = math.cos(ang), math.sin(ang)
    return [(x + px * c - py * s, y + px * s + py * c) for px, py in pts]


def inside_excluded(x, y, excl):
    for (ex, ey, ew, eh) in excl:
        if ex <= x <= ex + ew and ey <= y <= ey + eh:
            return True
    return False


def make_curve(name, pts, mat, depth, closed=False, filled=False, z=0.0):
    cu = bpy.data.curves.new(name, 'CURVE')
    cu.dimensions = '2D' if filled else '3D'
    if filled:
        cu.fill_mode = 'BOTH'
        cu.extrude = depth * 0.5
    else:
        cu.bevel_depth = depth
        cu.bevel_resolution = 2
    sp = cu.splines.new('POLY')
    sp.points.add(len(pts) - 1)
    for i, (x, y) in enumerate(pts):
        sp.points[i].co = (x * S, -y * S, z, 1)
    sp.use_cyclic_u = closed
    ob = bpy.data.objects.new(name, cu)
    bpy.context.scene.collection.objects.link(ob)
    ob.data.materials.append(mat)
    return ob


def ornaments(z_top, excl, gloss, silver):
    rnd = random.Random(1987)
    # zones (design coords): where vines may grow
    zones = [
        (18, 16, 165, 420),      # left column around the master section
        (792, 250, 140, 60),     # right of the soft keys
        (900, 420, 34, 770),     # right margin
        (180, 1186, 750, 20),    # bottom margin
        (16, 1176, 160, 30),     # bottom-left corner
        (180, 404, 750, 24),     # band between knobs and pads
        (26, 432, 166, 86),      # above the emblem
        (26, 1094, 166, 84),     # below the emblem
    ]
    # gaps between pad columns
    pl = LAY["pads"]
    for c in range(3):
        gx = pl["left"] + (c + 1) * pl["size"] + c * pl["gapX"]
        zones.append((gx + 4, pl["top"], pl["gapX"] - 8, 4 * (pl["size"] + pl["labelH"] + pl["rowGap"])))
    count = 0
    for (zx, zy, zw, zh) in zones:
        area_ = zw * zh
        n = max(1, int(area_ / 9000))
        for i in range(n):
            x0 = zx + rnd.random() * zw
            y0 = zy + rnd.random() * zh
            ang = rnd.uniform(0, 2 * math.pi)
            if zh > 3 * zw:
                ang = math.pi / 2 * (1 if rnd.random() < 0.5 else -1) + rnd.uniform(-0.3, 0.3)
            elif zw > 3 * zh:
                ang = (0 if rnd.random() < 0.5 else math.pi) + rnd.uniform(-0.3, 0.3)
            curl = rnd.uniform(-0.05, 0.05)
            pts = vine_points(x0, y0, rnd.uniform(60, 160), ang, curl, rnd)
            pts = [(x, y) for (x, y) in pts if zx - 6 <= x <= zx + zw + 6 and zy - 6 <= y <= zy + zh + 6 and not inside_excluded(x, y, excl)]
            if len(pts) < 6:
                continue
            accent = rnd.random() < 0.28
            make_curve("vine", pts, silver if accent else gloss, 0.0045 if accent else 0.006, z=z_top + 0.001)
            # leaves along the stem
            for j in range(4, len(pts) - 4, 7):
                if rnd.random() < 0.6:
                    (x1, y1), (x2, y2) = pts[j], pts[j + 1]
                    a = math.atan2(y2 - y1, x2 - x1) + (1 if rnd.random() < 0.5 else -1) * rnd.uniform(0.5, 1.1)
                    lp = leaf_points(x1, y1, a, rnd.uniform(9, 16))
                    if not any(inside_excluded(x, y, excl) for x, y in lp):
                        make_curve("leaf", lp, silver if accent else gloss, 0.003, closed=True, filled=True, z=z_top + 0.0012)
            count += 1
    print("vines:", count)


# ------------------------------------------------------------------ Tamil emblem
def emblem(box):
    data = json.load(open(os.path.join(HERE, "thudi_tamil.json")))
    gw, gh = data["width"], data["height"]
    bx, by, bw, bh = box
    sx, sy = bw / gh, bh / gw          # glyph height -> strip width, glyph width -> strip height
    cu = bpy.data.curves.new("emblem", 'CURVE')
    cu.dimensions = '2D'
    cu.fill_mode = 'BOTH'
    cu.extrude = 0.010
    cu.bevel_depth = 0.0035
    cu.bevel_resolution = 3
    for loop in data["loops"]:
        pts = loop[:-1] if loop[0] == loop[-1] else loop
        sp = cu.splines.new('POLY')
        sp.points.add(len(pts) - 1)
        for i, (gx, gy) in enumerate(pts):
            x = bx + gy * sx
            y = by + bh - gx * sy
            sp.points[i].co = (x * S, -y * S, 0.0, 1)
        sp.use_cyclic_u = True
    ob = bpy.data.objects.new("emblem", cu)
    bpy.context.scene.collection.objects.link(ob)
    ob.location.z = 0.0135
    metal = material("emblem_metal", (0.72, 0.72, 0.74), rough=0.34, metal=1.0, bump=("noise", 1400.0, 0.03))
    ob.data.materials.append(metal)
    return ob


# ------------------------------------------------------------------ the panel (background)
def build_panel():
    reset()
    sc = bpy.context.scene
    body_mat = body_material()
    gloss, silver = engraving_materials()
    well_mat = material("well", (0.008, 0.008, 0.009), rough=0.55)
    bezel_mat = material("bezel", (0.02, 0.02, 0.022), rough=0.6, bump=("noise", 600.0, 0.05))
    glass_mat = material("glass", (0.004, 0.006, 0.02), rough=0.05, coat=1.0, spec=0.8)
    brass = material("brass", (0.80, 0.60, 0.28), rough=0.28, metal=1.0, aniso=0.6, bump=("bands", 400.0, 0.02))
    screw = material("screw", (0.6, 0.6, 0.62), rough=0.25, metal=1.0)
    collar = material("collar", (0.05, 0.05, 0.055), rough=0.35, metal=0.4)

    T = 0.30  # body thickness (BU); top surface at z = 0
    body = rounded_box("body", 0, 0, W, H, -T, 0.0, radius=22, rim=4, mat=body_mat)
    apply_all(body)

    cutters = []
    # pad wells
    pl = LAY["pads"]
    def pad_rect(i):
        col, rowb = i % 4, i // 4
        row = 3 - rowb
        x = pl["left"] + col * (pl["size"] + pl["gapX"])
        y = pl["top"] + row * (pl["labelH"] + pl["size"] + pl["rowGap"]) + pl["labelH"]
        return x, y, pl["size"], pl["size"]
    # (pads and buttons sit on the surface: no wells, their own renders carry the shadows)
    # wheel recess
    wx, wy, wr = LAY["wheel"]
    cutters.append(cylinder("wheelwell", wx, wy, wr + 7, -0.04, 0.05))
    boolean_cut(body, cutters)
    apply_all(body)
    fix_shading(body)

    # துடி emblem: raised brushed-metal letters, reading bottom -> top
    emblem(LAY["logo"])

    # LCD bezel (raised frame) + glass
    bx, by, bw, bh = LAY["lcdBezel"]
    gx, gy, gw, gh = LAY["lcdGlass"]
    bez = rounded_box("bezel", bx, by, bw, bh, -0.01, 0.035, radius=12, rim=3, mat=bezel_mat)
    apply_all(bez)
    win = rounded_box("bezelwin", gx - 3, gy - 3, gw + 6, gh + 6, -0.02, 0.06, radius=4)
    boolean_cut(bez, [win])
    apply_all(bez)
    fix_shading(bez)
    rounded_box("glass", gx - 3, gy - 3, gw + 6, gh + 6, -0.012, 0.0, radius=4, mat=glass_mat)

    # knob collars (thin metal rings under the knobs)
    pk = LAY["pageKnobs"]

    # brass plaque with 4 screws
    px, py, pw, ph = LAY["plaque"]
    pq = rounded_box("plaque", px, py, pw, ph, 0.0, 0.012, radius=3, rim=1.2, mat=brass)
    for sx, sy in ((px + 6, py + 6), (px + pw - 6, py + 6), (px + 6, py + ph - 6), (px + pw - 6, py + ph - 6)):
        cylinder("screw", sx, sy, 2.4, 0.012, 0.016, screw, verts=24, rim=0.8)

    # ornaments, excluding controls and printed areas
    excl = []
    def ex(x, y, w, h, m=6):
        excl.append((x - m, y - m, w + 2 * m, h + 2 * m))
    ex(*LAY["lcdBezel"]); ex(*LAY["plaque"]); ex(*LAY["osButton"]); ex(*LAY["enterButton"])
    for b in LAY["softKeys"]: ex(*b)
    for (cx, cy, r) in (LAY["masterKnob"], LAY["driveKnob"]):
        ex(cx - r - 10, cy - r - 10, 2 * r + 20, 2 * r + 40)
    ex(wx - wr - 10, wy - wr - 30, 2 * wr + 20, 2 * wr + 40)
    for i in range(pk["n"]):
        ex(pk["x0"] + i * pk["dx"] - pk["r"] - 8, pk["y"] - pk["r"] - 8, 2 * pk["r"] + 16, 2 * pk["r"] + 30)
    for i in range(16):
        x, y, w, h = pad_rect(i)
        ex(x - 10, y - pl["labelH"] - 2, w + 20, h + pl["labelH"] + 12, 0)
    ex(*LAY["logo"], m=4)
    # (no vine ornaments: the panel stays clean, like the hardware)

    camera(W / 2, H / 2, W, H, W * SCALE, H * SCALE)
    lights(W / 2, H / 2, max(W, H))
    sc.render.film_transparent = False
    render(os.path.join(OUT, "bg.png"))


# ------------------------------------------------------------------ elements
def element_scene(cx, cy, frame, px, catcher=True):
    sc = reset()
    sc.render.film_transparent = True
    if catcher:
        plane = rounded_box("catcher", cx - frame, cy - frame, 2 * frame, 2 * frame, -0.01, 0.0, radius=0)
        plane.is_shadow_catcher = True
    camera(cx, cy, frame, frame, px, px)
    lights(cx, cy, 940)
    return sc


def build_pad(pressed):
    size = LAY["pads"]["size"]
    frame = size * 1.2
    element_scene(0, 0, frame, frame * SCALE)
    rubber = material("rubber", (0.17, 0.17, 0.165) if not pressed else (0.14, 0.14, 0.136), rough=0.86, bump=("noise", 350.0, 0.10), spec=0.25)
    z1 = 0.16 if not pressed else 0.10
    pad = rounded_box("pad", -size / 2, -size / 2, size, size, 0.0, z1, radius=4.0, rim=9.0, mat=rubber)
    apply_all(pad); fix_shading(pad)
    # very slight dish in the middle of the pad top
    render(os.path.join(OUT, "pad_down.png" if pressed else "pad_up.png"))


def build_knob(red):
    D = 60.0
    frame = D * 1.25
    element_scene(0, 0, frame, 256 if not QUICK else 64)
    skirt_mat = material("skirt", (0.018, 0.018, 0.019), rough=0.5)
    cap_col = (0.30, 0.025, 0.018) if red else (0.028, 0.028, 0.03)
    cap_mat = material("cap", cap_col, rough=0.42, spec=0.4)
    cylinder("skirt", 0, 0, D / 2, 0.0, 0.16, skirt_mat, r_top=D / 2 * 0.90, ribs=60, rim=0.8)
    cylinder("cap", 0, 0, D / 2 * 0.70, 0.16, 0.26, cap_mat, verts=128, r_top=D / 2 * 0.64, rim=2.2)
    render(os.path.join(OUT, "knob_red.png" if red else "knob_black.png"))


def build_wheel():
    D = LAY["wheel"][2] * 2
    element_scene(0, 0, D, 256 if not QUICK else 64, catcher=False)
    alu = material("alu", (0.62, 0.62, 0.64), rough=0.42, metal=1.0, bump=("noise", 700.0, 0.04))
    knurl = material("knurl", (0.45, 0.45, 0.47), rough=0.45, metal=1.0)
    cylinder("wheelrim", 0, 0, D / 2, 0.0, 0.14, knurl, r_top=D / 2 * 0.95, ribs=90, rim=0.6)
    top = cylinder("wheeltop", 0, 0, D / 2 * 0.93, 0.0, 0.17, alu, verts=192, r_top=D / 2 * 0.88, rim=2.5)
    apply_all(top)
    dimple = bpy.data.objects.new("dimple", bpy.data.meshes.new("dimple"))
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=48, v_segments=24, radius=12 * S)
    for v in bm.verts:
        v.co += Vector((D / 2 * 0.50 * S, 0, 0.17 + 12 * S * 0.80))
    bm.to_mesh(dimple.data); bm.free()
    bpy.context.scene.collection.objects.link(dimple)
    boolean_cut(top, [dimple])
    apply_all(top)
    fix_shading(top)
    render(os.path.join(OUT, "wheel.png"))


def build_button(down):
    w, h = 128.0, 32.0
    sc = reset()
    sc.render.film_transparent = True
    m = 8.0  # margin for the cast shadow (the plugin expands the image by the same amount)
    catcher = rounded_box("catcher", -w, -h * 2, 2 * w, 4 * h, -0.01, 0.0, radius=0)
    catcher.is_shadow_catcher = True
    camera(0, 0, w + 2 * m, h + 2 * m, (w + 2 * m) * 2 if not QUICK else 36, (h + 2 * m) * 2 if not QUICK else 12)
    lights(0, 0, 940)
    rubber = material("btn", (0.26, 0.26, 0.25) if not down else (0.21, 0.21, 0.2), rough=0.7, bump=("noise", 500.0, 0.05))
    btn = rounded_box("button", -w / 2 + 1, -h / 2 + 1, w - 2, h - 2, 0.0, 0.05 if not down else 0.03, radius=4, rim=5.0, mat=rubber)
    apply_all(btn); fix_shading(btn)
    render(os.path.join(OUT, "button_down.png" if down else "button_up.png"))


if __name__ == "__main__":
    which = argv[3].split(",") if len(argv) > 3 else ["bg", "pads", "knobs", "wheel", "buttons"]
    if "pads" in which:
        build_pad(False); build_pad(True)
    if "knobs" in which:
        build_knob(False); build_knob(True)
    if "wheel" in which:
        build_wheel()
    if "buttons" in which:
        build_button(False); build_button(True)
    if "bg" in which:
        build_panel()
    print("done ->", OUT)
