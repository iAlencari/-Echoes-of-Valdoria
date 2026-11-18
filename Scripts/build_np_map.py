# Nova Paulicéia (NP) — blockout jogável a partir da arte conceitual isométrica do dono.
# Formas simples (cubos e cilindros da Unreal), com escala e cobertura de hero shooter.
#   Spawn esquerdo: estação de metrô subterrânea (X negativo).
#   Spawn direito: prédio-torre com heliporto (X positivo).
#   Rota A (Y negativo): elevado tipo Minhocão + telhados com heliportos e passarelas.
#   Meio: avenida larga com veículo, praça amarela (objetivo) sob viaduto curvo.
#   Rota C (Y positivo): mercado coberto e becos com grafite.
#   Marco: arranha-céu com antena. Clima: anoitecer roxo/laranja, neon.
# Rodar: UnrealEditor-Cmd.exe <projeto> -run=pythonscript -script="Scripts/build_np_map.py"
import unreal, math, traceback

MAP = "/Game/Maps/NP_NovaPauliceia"
MAT_DIR = "/Game/Maps/NP_Materials"
LOG = unreal.Paths.project_saved_dir() + "build_np_map.txt"
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

# ---------------------------------------------------------------- nível
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.EditorAssetLibrary.delete_asset(MAP)
# Mapa em branco (sem World Partition) carregado como mundo do editor; salvo no fim com save_map.
NEW_WORLD = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)

def box(name, center, size, material, rot_yaw=0.0, mesh=None):
    """Cubo de 'size' (cm) com o centro em 'center'."""
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
    sx, sy, sz = start
    ex, ey, ez = end
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
    l = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*pos), unreal.Rotator(0, 0, 0))
    l.set_actor_label(name)
    lc = l.light_component
    lc.set_light_color(unreal.LinearColor(*color, 1))
    lc.set_intensity(intensity)
    lc.set_attenuation_radius(radius)
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

    # ---------------- chão geral e limites (mapa ~ 120 x 80 m)
    floor_box("Chao_Avenidas", -3900, 6000, -4000, 4000, 0, 50, asfalto)
    # faixa entre o metrô e a avenida, com o vão da escada do metrô
    floor_box("Chao_Faixa_N", -4400, -3900, -4000, -850, 0, 50, asfalto)
    floor_box("Chao_Faixa_S", -4400, -3900, -350, 4000, 0, 50, asfalto)
    floor_box("Chao_Oeste_N", -6000, -4400, -4000, -1200, 0, 50, asfalto)
    floor_box("Chao_Oeste_S", -6000, -4400, 1200, 4000, 0, 50, asfalto)
    for i, (y, h) in enumerate(((-4050, 900), (4050, 900))):
        box("Limite_%d" % i, (0, y, h / 2), (12400, 100, h), predio)
    box("Limite_Oeste", (-6050, 0, 450), (100, 8200, 900), predio)
    box("Limite_Leste", (6050, 0, 450), (100, 8200, 900), predio)

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
        box("Quarteirao_%d" % i, (x, y, h / 2), (sx, sy, h), predio if i % 2 else predio2)

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
        box("Beco_Parede_%d" % i, (x, 3750, 400), (900, 120, 800), predio2)
        box("Grafite_%d" % i, (x, 3685, 250), (700, 10, 300), [rotaC, neonRosa, rotaA][i % 3])
    box("Caixa_Agua", (2600, 2600, 120), (200, 200, 240), concreto, mesh=CYL)

    # ---------------- marco: arranha-céu com antena
    box("Arranha_Ceu", (2600, -1200, 4500), (1100, 1100, 9000), predio)
    box("Antena", (2600, -1200, 10000), (40, 40, 2000), concreto, mesh=CYL)
    light("Luz_Antena", (2600, -1200, 11000), (1.0, 0.1, 0.1), 50000, 3000)
    for k in range(6):
        box("Janelas_Neon_%d" % k, (2600, -645, 1500 + k * 1200), (900, 10, 120), [rotaA, rotaC][k % 2])

    # ---------------- SPAWN ESQUERDO: estação de metrô subterrânea
    floor_box("Metro_Piso", -6000, -4400, -1200, 1200, -500, 50, calcada)
    box("Metro_Trilho", (-5200, 900, -480), (1600, 300, 40), mat("Trilho", (0.1, 0.1, 0.1)))
    box("Metro_Faixa", (-5200, 650, -470), (1600, 40, 10), spawnA)
    ramp("Escada_Metro", (-4500, -600, -500), (-3900, -600, 0), 500, concreto)
    box("Saida_Metro_Placa", (-3950, -900, 300), (60, 300, 120), spawnA)
    light("Luz_Metro_1", (-5500, 0, -200), (0.4, 1.0, 0.6), 6000, 1200)
    light("Luz_Metro_2", (-4800, 0, -200), (0.4, 1.0, 0.6), 6000, 1200)
    # teto do metrô = chão da rua; a saída é pela escada a leste
    floor_box("Metro_Teto", -6000, -4400, -1200, 1200, 0, 50, concreto)
    box("Metro_Parede_N", (-5200, -1225, -250), (1600, 50, 500), concreto)
    box("Metro_Parede_S", (-5200, 1225, -250), (1600, 50, 500), concreto)
    box("Metro_Parede_L_N", (-4400, -1050, -250), (50, 400, 500), concreto)
    box("Metro_Parede_L_S", (-4400, 400, -250), (50, 1600, 500), concreto)
    ps = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-5600, -300, -380), unreal.Rotator(0, 0, 0))
    ps.set_actor_label("Spawn_Esquerdo_Metro")

    # ---------------- SPAWN DIREITO: prédio-torre com heliporto
    floor_box("Torre_Base", 4400, 6000, 600, 2600, 300, 300, predio2)
    box("Torre_Corpo", (5400, 1600, 1100), (900, 900, 1600), predio2)
    box("Torre_Heliporto", (5400, 1600, 1910), (800, 800, 20), spawnB, mesh=CYL)
    ramp("Torre_Rampa", (4100, 400, 0), (4700, 1000, 300), 500, concreto)
    box("Torre_Faixa", (4420, 1600, 320), (40, 2000, 20), spawnB)
    ps2 = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(5000, 1000, 420), unreal.Rotator(0, 180, 0))
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
