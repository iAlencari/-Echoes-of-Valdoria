# Cria (ou atualiza) os arquivos dos heróis prontos e o que eles usam:
#  - ícones do HUD (recortados da imagem do dono, em RawContent/Icons) -> /Game/Valdoria/UI/Icons
#  - material de energia translúcido (barreira, zonas, projéteis) -> /Game/Valdoria/Materials
#  - Data Assets DA_Brasa, DA_Kael, DA_Iara, DA_Vex, DA_Lume, DA_Kay, DA_Ariel, DA_ArielFaisca -> /Game/Valdoria/Heroes
# Rodar:  UnrealEditor-Cmd.exe EchoesOfValdoria.uproject -run=pythonscript -script="Scripts/create_heroes.py"
# Se um DA_ já existir ele NÃO é sobrescrito (para não apagar o balanceamento feito no editor),
# a não ser que a variável de ambiente VALDORIA_RESET_HEROES=1 esteja definida.
import os
import unreal

LOG = unreal.Paths.project_saved_dir() + "create_heroes.txt"
lines = []
eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary

# --- 1) Ícones
icon_dir = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "RawContent", "Icons")
tasks = []
for name in sorted(os.listdir(icon_dir)):
    if not name.lower().endswith(".png"):
        continue
    t = unreal.AssetImportTask()
    t.filename = os.path.join(icon_dir, name)
    t.destination_path = "/Game/Valdoria/UI/Icons"
    t.replace_existing = True
    t.automated = True
    t.save = True
    tasks.append(t)
tools.import_asset_tasks(tasks)
for t in tasks:
    for p in t.imported_object_paths:
        tex = unreal.load_asset(p)
        if tex:
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
            tex.set_editor_property("never_stream", True)
            eal.save_loaded_asset(tex)
    lines.append("icone: %s -> %s" % (os.path.basename(t.filename), list(t.imported_object_paths)))

# --- 2) Material de energia
MAT = "/Game/Valdoria/Materials/M_Valdoria_Energia"
if not eal.does_asset_exist(MAT):
    eal.make_directory("/Game/Valdoria/Materials")
    mat = tools.create_asset("M_Valdoria_Energia", "/Game/Valdoria/Materials", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    cor = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, 0)
    cor.set_editor_property("parameter_name", "Cor")
    cor.set_editor_property("default_value", unreal.LinearColor(1, 0.5, 0.1, 1))
    forca = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -600, 200)
    forca.set_editor_property("r", 3.0)
    mult = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 0)
    mel.connect_material_expressions(cor, "", mult, "A")
    mel.connect_material_expressions(forca, "", mult, "B")
    mel.connect_material_property(mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    opac = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, 350)
    opac.set_editor_property("parameter_name", "Opacidade")
    opac.set_editor_property("default_value", 0.35)
    mel.connect_material_property(opac, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    eal.save_loaded_asset(mat)
    lines.append("material criado")
else:
    lines.append("material ja existe")

# --- 3) Heróis
factory = unreal.DataAssetFactory()
factory.set_editor_property("data_asset_class", unreal.ValdoriaHeroDefinition)
reset = os.environ.get("VALDORIA_RESET_HEROES") == "1"
# Turn 44: reset only some heroes (comma list, e.g. VALDORIA_RESET_HERO=vex) when their kit changed in
# ValdoriaHeroPresets.cpp; the others keep whatever the owner balanced in the editor.
reset_some = [h.strip().lower() for h in os.environ.get("VALDORIA_RESET_HERO", "").split(",") if h.strip()]
eal.make_directory("/Game/Valdoria/Heroes")
# Turn 45 (owner): not every hero lies down (X). Edit this table, or override with
# VALDORIA_NO_PRONE=lume,vex (turn off) / VALDORIA_PRONE=lume (turn on). Applied on every run.
CAN_PRONE = {"brasa": True, "kael": True, "iara": True, "vex": True, "lume": False,
             "kay": True, "ariel": True, "ariel_faisca": True}
for h in os.environ.get("VALDORIA_NO_PRONE", "").split(","):
    if h.strip():
        CAN_PRONE[h.strip().lower()] = False
for h in os.environ.get("VALDORIA_PRONE", "").split(","):
    if h.strip():
        CAN_PRONE[h.strip().lower()] = True


def apply_prone(asset, hero_id):
    want = CAN_PRONE.get(hero_id, True)
    mv = asset.get_editor_property("movement")
    if mv.get_editor_property("can_prone") != want:
        mv.set_editor_property("can_prone", want)
        asset.set_editor_property("movement", mv)
        lines.append("deitar (X) %s: %s" % (hero_id, want))
    return asset


HEROES = (("brasa", "DA_Brasa"), ("kael", "DA_Kael"), ("iara", "DA_Iara"), ("vex", "DA_Vex"), ("lume", "DA_Lume"),
          ("kay", "DA_Kay"), ("ariel", "DA_Ariel"), ("ariel_faisca", "DA_ArielFaisca"))
for hero_id, asset_name in HEROES:
    path = "/Game/Valdoria/Heroes/" + asset_name
    if eal.does_asset_exist(path) and not reset and hero_id not in reset_some:
        if os.environ.get("VALDORIA_APPLY_LOOKS") == "1":
            existing = unreal.load_asset(path)
            existing.apply_preset_looks(hero_id)
            apply_prone(existing, hero_id)
            eal.save_loaded_asset(existing)
            lines.append("aparencia aplicada (animacoes e corpo): " + path)
        else:
            existing = apply_prone(unreal.load_asset(path), hero_id)
            eal.save_loaded_asset(existing)
        lines.append("ja existe (mantido): " + path)
        continue
    asset = unreal.load_asset(path) if eal.does_asset_exist(path) else \
        tools.create_asset(asset_name, "/Game/Valdoria/Heroes", unreal.ValdoriaHeroDefinition, factory)
    ok = asset.apply_preset(hero_id)
    asset.apply_preset_looks(hero_id)
    apply_prone(asset, hero_id)
    eal.save_loaded_asset(asset)
    lines.append("heroi %s: %s" % (path, ok))

with open(LOG, "w", encoding="utf-8") as fh:
    fh.write("\n".join(lines))

# --- 4) Loja (itens e créditos) - também não sobrescreve se já existir
SHOP = "/Game/Valdoria/Shop/DA_Loja"
if not eal.does_asset_exist(SHOP) or reset:
    eal.make_directory("/Game/Valdoria/Shop")
    shop_factory = unreal.DataAssetFactory()
    shop_factory.set_editor_property("data_asset_class", unreal.ValdoriaShopCatalog)
    shop = unreal.load_asset(SHOP) if eal.does_asset_exist(SHOP) else \
        tools.create_asset("DA_Loja", "/Game/Valdoria/Shop", unreal.ValdoriaShopCatalog, shop_factory)
    shop.apply_default_items()
    eal.save_loaded_asset(shop)
    lines.append("loja criada: " + SHOP)
else:
    lines.append("loja ja existe (mantida)")

with open(LOG, "w", encoding="utf-8") as fh:
    fh.write("\n".join(lines))

# --- 5) Mode rules (deathmatch) - not overwritten if it exists
MODE = "/Game/Valdoria/Modes/DA_Deathmatch"
if not eal.does_asset_exist(MODE):
    eal.make_directory("/Game/Valdoria/Modes")
    mode_factory = unreal.DataAssetFactory()
    mode_factory.set_editor_property("data_asset_class", unreal.ValdoriaModeRules)
    mode = tools.create_asset("DA_Deathmatch", "/Game/Valdoria/Modes", unreal.ValdoriaModeRules, mode_factory)
    eal.save_loaded_asset(mode)
    lines.append("mode rules created: " + MODE)

with open(LOG, "w", encoding="utf-8") as fh:
    fh.write("\n".join(lines))
# --- 6) MOBA rules (Guardian Spots) - not overwritten if it exists
MOBA = "/Game/Valdoria/Modes/DA_Moba"
if not eal.does_asset_exist(MOBA):
    moba_factory = unreal.DataAssetFactory()
    moba_factory.set_editor_property("data_asset_class", unreal.ValdoriaMobaRules)
    moba = tools.create_asset("DA_Moba", "/Game/Valdoria/Modes", unreal.ValdoriaMobaRules, moba_factory)
    eal.save_loaded_asset(moba)
    lines.append("moba rules created: " + MOBA)

with open(LOG, "w", encoding="utf-8") as fh:
    fh.write("\n".join(lines))