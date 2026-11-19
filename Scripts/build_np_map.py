# Nova Paulicéia (NP) — blockout jogável a partir da arte conceitual isométrica do dono.
# Formas simples (cubos e cilindros da Unreal), com escala e cobertura de hero shooter.
#   Spawn esquerdo: estação de metrô subterrânea (X negativo).
#   Spawn direito: prédio-torre com heliporto (X positivo).
#   Rota A (Y negativo): elevado tipo Minhocão + telhados com heliportos e passarelas.
#   Meio: avenida larga com veículo, praça amarela (objetivo) sob viaduto curvo.
#   Rota C (Y positivo): mercado coberto e becos com grafite.
#   Marco: arranha-céu com antena. Clima: anoitecer roxo/laranja, neon.
# Rodar: UnrealEditor-Cmd.exe <projeto> -run=pythonscript -script="Scripts/build_np_map.py"
import unreal, math, traceback, os

# Map scale (owner 2026-11-18: NP was too small). Horizontal positions and big horizontal sizes
# (>= 5 m) are multiplied; heights, props and characters keep their size.
#   VALDORIA_NP_SCALE (default 2.0) and VALDORIA_NP_MAP (default /Game/Maps/NP_NovaPauliceia).
#   MOBA version: VALDORIA_NP_SCALE=2.6 VALDORIA_NP_MAP=/Game/Maps/NP_Moba
S = float(os.environ.get("VALDORIA_NP_SCALE", "2.0"))
# Turn 45 (owner): the MOBA starts at lane level. VALDORIA_NP_METRO=0 covers the metro stair and puts the left base on
# the street (default: metro on for the 2.0 map, off for the MOBA 2.6 map).
METRO = os.environ.get("VALDORIA_NP_METRO", "1" if S < 2.5 else "0") == "1"
MAP = os.environ.get("VALDORIA_NP_MAP", "/Game/Maps/NP_NovaPauliceia")
MAT_DIR = "/Game/Maps/NP_Materials"
LOG = unreal.Paths.project_saved_dir() + "build_np_map_" + MAP.split("/")[-1] + ".txt"
log_lines = []

def log(m):
    log_lines.append(str(m))

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube")         # 100 cm
CYL = unreal.load_asset("/Engine/BasicShapes/Cylinder")      # 100 cm
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

# ---------------------------------------------------------------- materiais
def base_material():
    path = MAT_DIR + "/M_NP_Blockout"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    m = tools.create_asset("M_NP_Blockout", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    color = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -600, 0)
    color.set_editor_property("parameter_name", "Cor")
    color.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1))
    glow = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, 200)
    glow.set_editor_property("parameter_name", "Brilho")
    glow.set_editor_property("default_value", 0.0)
    rough = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -600, 300)
    rough.set_editor_property("parameter_name", "Aspereza")
    rough.set_editor_property("default_value", 0.7)
    mul = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 150)
    mel.connect_material_expressions(color, "", mul, "A")
    mel.connect_material_expressions(glow, "", mul, "B")
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m

MATS = {}
def mat(name, rgb, glow=0.0, rough=0.7):
    if name in MATS:
        return MATS[name]
    path = MAT_DIR + "/MI_NP_" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mi = unreal.load_asset(path)
    else:
        mi = tools.create_asset("MI_NP_" + name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, base_material())
    mel.set_material_instance_vector_parameter_value(mi, "Cor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1))
    mel.set_material_instance_scalar_parameter_value(mi, "Brilho", glow)
    mel.set_material_instance_scalar_parameter_value(mi, "Aspereza", rough)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    MATS[name] = mi
    return mi

# Facade material (owner: plain boxes looked like Roblox). Windows from world position, so they keep
# their size on any box; random lit windows, roofs without windows. Instances vary the colours.
def facade_material():
    path = MAT_DIR + "/M_NP_Fachada"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    m = tools.create_asset("M_NP_Fachada", MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    def E(cls, x, y, **props):
        e = mel.create_material_expression(m, cls, x, y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e
    def C(a, b, pin="", out=""):
        mel.connect_material_expressions(a, out, b, pin)
    def P(name, x, y, value, vector=False):
        e = E(unreal.MaterialExpressionVectorParameter if vector else unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name)
        e.set_editor_property("default_value", unreal.LinearColor(*value, 1) if vector else value)
        return e
    wp = E(unreal.MaterialExpressionWorldPosition, -2200, 0)
    mx = E(unreal.MaterialExpressionComponentMask, -2000, -100, r=True, g=False, b=False, a=False); C(wp, mx)
    my = E(unreal.MaterialExpressionComponentMask, -2000, 0, r=False, g=True, b=False, a=False); C(wp, my)
    mz = E(unreal.MaterialExpressionComponentMask, -2000, 150, r=False, g=False, b=True, a=False); C(wp, mz)
    hsum = E(unreal.MaterialExpressionAdd, -1800, -50); C(mx, hsum, "A"); C(my, hsum, "B")
    u = E(unreal.MaterialExpressionMultiply, -1600, -50, const_b=1.0 / 350.0); C(hsum, u, "A")
    v = E(unreal.MaterialExpressionMultiply, -1600, 150, const_b=1.0 / 380.0); C(mz, v, "A")
    def stripe(src, width, y):
        fr = E(unreal.MaterialExpressionFrac, -1400, y); C(src, fr)
        sb = E(unreal.MaterialExpressionSubtract, -1250, y, const_b=0.5); C(fr, sb, "A")
        ab = E(unreal.MaterialExpressionAbs, -1100, y); C(sb, ab)
        s2 = E(unreal.MaterialExpressionSubtract, -950, y, const_b=width); C(ab, s2, "A")
        mu = E(unreal.MaterialExpressionMultiply, -800, y, const_b=-25.0); C(s2, mu, "A")
        sa = E(unreal.MaterialExpressionSaturate, -650, y); C(mu, sa)
        return sa
    wu = stripe(u, 0.32, -50)
    wv = stripe(v, 0.27, 150)
    win = E(unreal.MaterialExpressionMultiply, -450, 50); C(wu, win, "A"); C(wv, win, "B")
    # roofs and floors: no windows
    nrm = E(unreal.MaterialExpressionVertexNormalWS, -800, 350)
    nz = E(unreal.MaterialExpressionComponentMask, -650, 350, r=False, g=False, b=True, a=False); C(nrm, nz)
    naz = E(unreal.MaterialExpressionAbs, -500, 350); C(nz, naz)
    side = E(unreal.MaterialExpressionOneMinus, -350, 350); C(naz, side)
    winside = E(unreal.MaterialExpressionMultiply, -250, 150); C(win, winside, "A"); C(side, winside, "B")
    # random lit windows: hash(floor(u), floor(v))
    fu = E(unreal.MaterialExpressionFloor, -1400, 500); C(u, fu)
    fv = E(unreal.MaterialExpressionFloor, -1400, 600); C(v, fv)
    hu = E(unreal.MaterialExpressionMultiply, -1250, 500, const_b=12.9898); C(fu, hu, "A")
    hv = E(unreal.MaterialExpressionMultiply, -1250, 600, const_b=78.233); C(fv, hv, "A")
    hs = E(unreal.MaterialExpressionAdd, -1100, 550); C(hu, hs, "A"); C(hv, hs, "B")
    sn = E(unreal.MaterialExpressionSine, -950, 550); C(hs, sn)
    big = E(unreal.MaterialExpressionMultiply, -800, 550, const_b=43758.5453); C(sn, big, "A")
    rnd = E(unreal.MaterialExpressionFrac, -650, 550); C(big, rnd)
    thr = P("Acesas", -800, 700, 0.5)
    rs = E(unreal.MaterialExpressionSubtract, -500, 600); C(rnd, rs, "A"); C(thr, rs, "B")
    rm = E(unreal.MaterialExpressionMultiply, -350, 600, const_b=40.0); C(rs, rm, "A")
    lit = E(unreal.MaterialExpressionSaturate, -200, 600); C(rm, lit)
    wl = E(unreal.MaterialExpressionMultiply, -50, 300); C(winside, wl, "A"); C(lit, wl, "B")
    wcol = P("CorJanela", -400, 800, (1.0, 0.75, 0.4), True)
    glow = P("Brilho", -400, 950, 6.0)
    cg = E(unreal.MaterialExpressionMultiply, -200, 850); C(wcol, cg, "A"); C(glow, cg, "B")
    em = E(unreal.MaterialExpressionMultiply, 100, 400); C(wl, em, "A"); C(cg, em, "B")
    fcol = P("CorFachada", -400, -350, (0.16, 0.15, 0.19), True)
    gcol = P("CorVidro", -400, -200, (0.03, 0.04, 0.06), True)
    base = E(unreal.MaterialExpressionLinearInterpolate, 100, -250); C(fcol, base, "A"); C(gcol, base, "B"); C(winside, base, "Alpha")
    rough = E(unreal.MaterialExpressionLinearInterpolate, 100, 0, const_a=0.85, const_b=0.12); C(winside, rough, "Alpha")
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m

def facade(name, window_rgb, facade_rgb, lit=0.5):
    if name in MATS:
        return MATS[name]
    path = MAT_DIR + "/MI_NP_" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mi = unreal.load_asset(path)
    else:
        mi = tools.create_asset("MI_NP_" + name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, facade_material())
    mel.set_material_instance_vector_parameter_value(mi, "CorJanela", unreal.LinearColor(*window_rgb, 1))
    mel.set_material_instance_vector_parameter_value(mi, "CorFachada", unreal.LinearColor(*facade_rgb, 1))
    mel.set_material_instance_scalar_parameter_value(mi, "Acesas", lit)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    MATS[name] = mi
    return mi

# ---------------------------------------------------------------- nívelles = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.EditorAssetLibrary.delete_asset(MAP)
# Mapa em branco (sem World Partition) carregado como mundo do editor; salvo no fim com save_map.
NEW_WORLD = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)

def box(name, center, size, material, rot_yaw=0.0, mesh=None):
    """Cubo de 'size' (cm) com o centro em 'center'."""
    center = (center[0] * S, center[1] * S, center[2])
    size = tuple((s * S if (i < 2 and s >= 500) else s) for i, s in enumerate(size))
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*center), unreal.Rotator(0, 0, rot_yaw))
    a.set_actor_label(name)
    c = a.static_mesh_component
    c.set_static_mesh(mesh or CUBE)
    a.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    c.set_material(0, material)
    return a

def floor_box(name, x0, x1, y0, y1, z_top, thick, material):
    return box(name, ((x0 + x1) / 2, (y0 + y1) / 2, z_top - thick / 2), (abs(x1 - x0), abs(y1 - y0), thick), material)

def ramp(name, start, end, width, material):
    """Rampa entre dois pontos (x, y, z do topo da superfície)."""
    sx, sy, sz = start[0] * S, start[1] * S, start[2]
    ex, ey, ez = end[0] * S, end[1] * S, end[2]
    dx, dy, dz = ex - sx, ey - sy, ez - sz
    run = math.sqrt(dx * dx + dy * dy)
    length = math.sqrt(run * run + dz * dz)
    yaw = math.degrees(math.atan2(dy, dx))
    pitch = math.degrees(math.atan2(dz, run))
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector((sx + ex) / 2, (sy + ey) / 2, (sz + ez) / 2 - 15), unreal.Rotator(0, pitch, yaw))
    a.set_actor_label(name)
    a.static_mesh_component.set_static_mesh(CUBE)
    a.set_actor_scale3d(unreal.Vector(length / 100.0, width / 100.0, 0.3))
    a.static_mesh_component.set_material(0, material)
    return a

def light(name, pos, color, intensity=8000.0, radius=1500.0):
    l = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(pos[0] * S, pos[1] * S, pos[2]), unreal.Rotator(0, 0, 0))
    l.set_actor_label(name)
    lc = l.light_component
    lc.set_light_color(unreal.LinearColor(*color, 1))
    lc.set_intensity(intensity)
    lc.set_attenuation_radius(radius * S)
    return l

try:
    concreto = mat("Concreto", (0.32, 0.31, 0.33))
    asfalto = mat("Asfalto", (0.05, 0.05, 0.06), rough=0.25)   # chão "molhado"
    calcada = mat("Calcada", (0.22, 0.21, 0.24), rough=0.35)
    predio = mat("Predio", (0.16, 0.15, 0.19))
    predio2 = mat("Predio2", (0.24, 0.20, 0.22))
    praca = mat("Praca", (0.95, 0.75, 0.1), glow=1.5)
    spawnA = mat("SpawnMetro", (0.2, 0.9, 0.4), glow=1.2)
    spawnB = mat("SpawnTorre", (1.0, 0.45, 0.1), glow=1.2)
    rotaA = mat("NeonAzul", (0.2, 0.5, 1.0), glow=8.0)
    rotaC = mat("NeonRoxo", (0.75, 0.2, 1.0), glow=8.0)
    neonRosa = mat("NeonRosa", (1.0, 0.2, 0.6), glow=8.0)
    caixote = mat("Caixote", (0.45, 0.3, 0.15))
    barreira = mat("Barreira", (0.5, 0.5, 0.48))
    veiculo = mat("Veiculo", (0.6, 0.7, 0.8), rough=0.2)
    lona = mat("Lona", (0.6, 0.15, 0.1))
    verde = mat("Grama", (0.1, 0.3, 0.1))
    fachadas = [facade("FachadaQuente", (1.0, 0.72, 0.38), (0.17, 0.15, 0.18)),
                facade("FachadaCiano", (0.3, 0.85, 1.0), (0.13, 0.15, 0.2), 0.55),
                facade("FachadaRosa", (1.0, 0.35, 0.75), (0.2, 0.15, 0.2), 0.6)]

    # ---------------- chão geral e limites (mapa ~ 120 x 80 m)
    floor_box("Chao_Avenidas", -3900, 6000, -4000, 4000, 0, 50, asfalto)
    # faixa entre o metrô e a avenida, com o vão da escada do metrô
    floor_box("Chao_Faixa_N", -4400, -3900, -4000, -850, 0, 50, asfalto)
    floor_box("Chao_Faixa_S", -4400, -3900, -350, 4000, 0, 50, asfalto)
    if not METRO:
        floor_box("Tampa_Escada_Metro", -4400, -3900, -850, -350, 0, 50, asfalto)  # stair covered: no underground
    floor_box("Chao_Oeste_N", -6000, -4400, -4000, -1200, 0, 50, asfalto)
    floor_box("Chao_Oeste_S", -6000, -4400, 1200, 4000, 0, 50, asfalto)
    # Turn 45: the border walls were 100 cm thick (not scaled) and stood 50*S cm away from the floor edge -> a 50-80 cm
    # gap along all four sides. Now 600 wide (scaled), starting exactly at the floor edge, and 12 m tall.
    for i, (y, h) in enumerate(((-4300, 1200), (4300, 1200))):
        box("Limite_%d" % i, (0, y, h / 2), (13200, 600, h), fachadas[i % 3])
    box("Limite_Oeste", (-6300, 0, 600), (600, 9200, 1200), fachadas[2])
    box("Limite_Leste", (6300, 0, 600), (600, 9200, 1200), fachadas[0])

    # ---------------- avenida central (meio) com calçadas e faixas
    floor_box("Calcada_N", -4500, 4500, -1500, -1100, 15, 30, calcada)
    floor_box("Calcada_S", -4500, 4500, 1100, 1500, 15, 30, calcada)
    for i in range(-4, 5):
        box("Faixa_%d" % i, (i * 1000, 0, 2), (400, 30, 4), mat("Faixa", (0.9, 0.9, 0.85), glow=0.3))
    # veículo (cobertura móvel no conceito) e canteiro
    box("Veiculo", (-1800, 300, 150), (900, 300, 300), veiculo)
    box("Veiculo_Teto", (-1800, 300, 330), (700, 260, 60), rotaA)
    floor_box("Canteiro", -3300, -2700, -500, 500, 40, 40, verde)

    # ---------------- PRAÇA (objetivo) sob o viaduto curvo
    floor_box("Praca_Objetivo", 200, 1800, -800, 800, 8, 8, praca)
    light("Luz_Praca", (1000, 0, 350), (1.0, 0.8, 0.3), 12000, 1800)
    # viaduto curvo: arco de segmentos a 8 m de altura, com pilares
    cx, cy, r = 1000.0, 1400.0, 2200.0
    for k in range(9):
        ang = math.radians(200 + k * 17)
        x = cx + r * math.cos(ang)
        y = cy + r * math.sin(ang)
        box("Viaduto_%d" % k, (x, y, 800), (700, 900, 60), concreto, rot_yaw=math.degrees(ang) + 90)
        if k % 2 == 0:
            box("Pilar_Viaduto_%d" % k, (x, y, 400), (160, 160, 780), concreto, mesh=CYL)
    # coberturas na praça: barreiras de concreto e caixotes
    for i, (x, y, yaw) in enumerate(((600, -500, 0), (1400, 500, 0), (1000, 0, 90), (500, 600, 45), (1500, -600, 45))):
        box("Barreira_Praca_%d" % i, (x, y, 60), (300, 80, 120), barreira, rot_yaw=yaw)
    for i, (x, y) in enumerate(((-600, -700), (-650, -600), (2600, 650), (2600, 750), (-1000, 900))):
        box("Caixote_%d" % i, (x, y, 50), (100, 100, 100), caixote)
    # barracas de comida na avenida
    for i, x in enumerate((-3800, -1000, 2900)):
        box("Barraca_%d" % i, (x, -1250, 110), (220, 160, 220), lona)
        light("Neon_Barraca_%d" % i, (x, -1150, 260), (1.0, 0.3, 0.6), 3000, 600)

    # ---------------- quarteirões que separam as rotas (bloqueiam a visão entre elas)
    blocks = [(-3800, -2100, 1200, 900, 1400), (-1800, -2200, 1400, 900, 2000), (1800, -2200, 1200, 900, 1700),
              (3700, -2100, 1300, 900, 1200), (-3900, 2100, 1100, 900, 900), (-1600, 2200, 1300, 900, 1100),
              (3800, 2200, 1200, 900, 1000)]
    for i, (x, y, sx, sy, h) in enumerate(blocks):
        box("Quarteirao_%d" % i, (x, y, h / 2), (sx, sy, h), fachadas[i % 3])

    # ---------------- ROTA A: elevado (Minhocão) + telhados e passarelas
    elev_z = 600
    floor_box("Minhocao", -5200, 5200, -3700, -2900, elev_z, 60, asfalto)
    for i in range(-5, 6):
        box("Pilar_Minhocao_%d" % i, (i * 1000, -3300, elev_z / 2), (150, 150, elev_z), concreto, mesh=CYL)
    box("Mureta_Minhocao_N", (0, -3720, elev_z + 50), (10400, 40, 100), concreto)
    for i in range(-5, 6, 2):
        box("Mureta_Minhocao_S_%d" % i, (i * 1000, -2880, elev_z + 50), (600, 40, 100), concreto)
        light("Poste_Minhocao_%d" % i, (i * 1000, -2900, elev_z + 400), (0.4, 0.6, 1.0), 4000, 900)
    ramp("Rampa_Minhocao_Oeste", (-5600, -2300, 0), (-4600, -3300, elev_z), 500, concreto)
    ramp("Rampa_Minhocao_Leste", (5600, -2300, 0), (4600, -3300, elev_z), 500, concreto)
    # telhados com heliportos ligados ao elevado por passarelas
    roofs = [(-1800, -2200, 2000), (1800, -2200, 1700)]
    for i, (x, y, h) in enumerate(roofs):
        box("Heliporto_%d" % i, (x, y, h + 10), (700, 700, 20), mat("Heliporto", (0.9, 0.4, 0.1), glow=2.0), mesh=CYL)
        ramp("Passarela_%d" % i, (x, -2950, elev_z + 10), (x, y - 450, h), 300, rotaA)
    box("Escada_Telhado", (-200, -2300, 600), (300, 600, 1200), concreto)

    # ---------------- ROTA C: mercado coberto e becos com grafite
    floor_box("Piso_Mercado", -3000, 3000, 2700, 3700, 10, 10, calcada)
    for i in range(-5, 6):
        x = i * 520
        box("Banca_%d" % i, (x, 3000, 60), (300, 150, 120), caixote)
        box("Banca_Teto_%d" % i, (x, 3000, 300), (400, 400, 20), lona if i % 2 else mat("Lona2", (0.1, 0.4, 0.5)))
        if i % 3 == 0:
            light("Neon_Mercado_%d" % i, (x, 3300, 280), (0.75, 0.2, 1.0), 5000, 900)
    box("Mercado_Telhado", (0, 3450, 450), (6000, 500, 30), concreto)
    for i, x in enumerate((-2600, -1200, 400, 2000)):
        box("Beco_Parede_%d" % i, (x, 3750, 400), (900, 120, 800), fachadas[(i + 1) % 3])
        box("Grafite_%d" % i, (x, 3685, 250), (700, 10, 300), [rotaC, neonRosa, rotaA][i % 3])
    box("Caixa_Agua", (2600, 2600, 120), (200, 200, 240), concreto, mesh=CYL)

    # ---------------- marco: arranha-céu com antena
    box("Arranha_Ceu", (2600, -1200, 4500), (1100, 1100, 9000), fachadas[1])
    box("Antena", (2600, -1200, 10000), (40, 40, 2000), concreto, mesh=CYL)
    light("Luz_Antena", (2600, -1200, 11000), (1.0, 0.1, 0.1), 50000, 3000)
    for k in range(6):
        box("Janelas_Neon_%d" % k, (2600, -645, 1500 + k * 1200), (900, 10, 120), [rotaA, rotaC][k % 2])

    # ---------------- SPAWN ESQUERDO: estação de metrô subterrânea (or, for the MOBA, a base on the street)
    if not METRO:
        floor_box("Metro_Teto", -6000, -4400, -1200, 1200, 0, 50, concreto)
        floor_box("Base_Esquerda", -5900, -4500, -1100, 500, 12, 12, spawnA)
        box("Base_Esquerda_Placa", (-4500, -900, 300), (60, 300, 120), spawnA)
        light("Luz_Base_Esquerda", (-5200, -300, 500), (0.78, 0.95, 0.85), 6000, 2000)
        ps = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-5600 * S, -300 * S, 120), unreal.Rotator(0, 0, 0))
        ps.set_actor_label("Spawn_Esquerdo_Rua")
    if METRO:
        floor_box("Metro_Piso", -6000, -4400, -1200, 1200, -500, 50, calcada)
        box("Metro_Trilho", (-5200, 900, -480), (1600, 300, 40), mat("Trilho", (0.1, 0.1, 0.1)))
        box("Metro_Faixa", (-5200, 650, -470), (1600, 40, 10), spawnA)
        # Turn 44: the stair opening in the street is 500*S wide but the ramp was 500 (not scaled): 2.5 m gaps on each side
        # with no floor below -> heroes fell out of the map. Ramp as wide as the opening + a floor under the whole opening.
        ramp("Escada_Metro", (-4500, -600, -500), (-3900, -600, 0), 500 * S, concreto)
        floor_box("Escada_Fosso", -4450, -3900, -850, -350, -500, 50, calcada)
        box("Saida_Metro_Placa", (-3950, -900, 300), (60, 300, 120), spawnA)
        # Turn 44 playtest: the strong saturated green tinted everything and hid enemies; softer, whiter light.
        light("Luz_Metro_1", (-5500, 0, -200), (0.78, 0.95, 0.85), 2500, 1400)
        light("Luz_Metro_2", (-4800, 0, -200), (0.78, 0.95, 0.85), 2500, 1400)
        # teto do metrô = chão da rua; a saída é pela escada a leste
        floor_box("Metro_Teto", -6000, -4400, -1200, 1200, 0, 50, concreto)
        box("Metro_Parede_N", (-5200, -1225, -250), (1600, 50, 500), concreto)
        box("Metro_Parede_S", (-5200, 1225, -250), (1600, 50, 500), concreto)
        # Turn 44: the station had no west wall and the east walls did not follow the map scale (sizes under
        # 500 cm are not scaled), leaving gaps beside the stair: heroes walked off the floor into the void.
        # Walls now computed in world units: west wall closed, east wall closed up to the stair opening.
        def wall_abs(name, x, y0, y1):
            a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, (y0 + y1) / 2, -250), unreal.Rotator(0, 0, 0))
            a.set_actor_label(name)
            a.static_mesh_component.set_static_mesh(CUBE)
            a.set_actor_scale3d(unreal.Vector(0.5, abs(y1 - y0) / 100.0, 5.0))
            a.static_mesh_component.set_material(0, concreto)
        stair_y, stair_half = -600 * S, 250 + 20
        wall_abs("Metro_Parede_O", -6000 * S, -1200 * S - 50, 1200 * S + 50)
        wall_abs("Metro_Parede_L_N", -4400 * S, -1200 * S, stair_y - stair_half)
        wall_abs("Metro_Parede_L_S", -4400 * S, stair_y + stair_half, 1200 * S)
        ps = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-5600 * S, -300 * S, -380), unreal.Rotator(0, 0, 0))
        ps.set_actor_label("Spawn_Esquerdo_Metro")

    # ---------------- SPAWN DIREITO: prédio-torre com heliporto
    floor_box("Torre_Base", 4400, 6000, 600, 2600, 300, 300, predio2)
    box("Torre_Corpo", (5400, 1600, 1100), (900, 900, 1600), fachadas[0])
    box("Torre_Heliporto", (5400, 1600, 1910), (800, 800, 20), spawnB, mesh=CYL)
    ramp("Torre_Rampa", (4100, 400, 0), (4700, 1000, 300), 500, concreto)
    box("Torre_Faixa", (4420, 1600, 320), (40, 2000, 20), spawnB)
    ps2 = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(5000 * S, 1000 * S, 420), unreal.Rotator(0, 180, 0))
    ps2.set_actor_label("Spawn_Direito_Torre_Time2")
    ps2.set_editor_property("player_start_tag", "Time2")

    # ---------------- céu de anoitecer roxo/laranja
    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 3000), unreal.Rotator(0, -8, 160))
    sun.set_actor_label("Sol_Poente")
    sun.light_component.set_intensity(2.5)
    sun.light_component.set_light_color(unreal.LinearColor(1.0, 0.55, 0.3, 1))
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0)).set_actor_label("Ceu")
    sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    sky.set_actor_label("Luz_Ceu")
    sky.light_component.set_editor_property("real_time_capture", True)
    sky.light_component.set_intensity(0.5)
    fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fog.set_actor_label("Neblina")
    fc = fog.component
    fc.set_fog_density(0.006)
    fc.set_fog_inscattering_color(unreal.LinearColor(0.12, 0.05, 0.2, 1))
    try:
        fc.set_editor_property("enable_volumetric_fog", True)
    except Exception:
        log("sem neblina volumetrica")
    ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    ppv.set_editor_property("unbound", True)
    try:
        pps = ppv.get_editor_property("settings")
        pps.set_editor_property("override_auto_exposure_bias", True)
        pps.set_editor_property("auto_exposure_bias", -1.0)
        pps.set_editor_property("override_bloom_intensity", True)
        pps.set_editor_property("bloom_intensity", 1.2)
        ppv.set_editor_property("settings", pps)
    except Exception:
        log("pos-processo: " + traceback.format_exc().splitlines()[-1])

    # Turn 44: navigation bounds for the runtime navmesh (autopilot / bots pathfinding).
    try:
        import sys
        sys.path.append(os.path.dirname(os.path.abspath(__file__)))
        from add_nav_bounds import add_nav_bounds
        add_nav_bounds(log)
    except Exception:
        import traceback as _tb
        log("nav bounds: " + _tb.format_exc().splitlines()[-1])

    # modo de jogo do projeto
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, "/Script/EchoesOfValdoria.ValdoriaGameMode"))
    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
    log("atores no mapa: %d, salvo=%s" % (len(eas.get_all_level_actors()), saved))
    log("OK: mapa salvo em " + MAP)
except Exception:
    log("FALHOU: " + traceback.format_exc())

with open(LOG, "w", encoding="utf-8") as fh:
    fh.write("\n".join(log_lines))
