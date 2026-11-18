# As armas do Meshy vieram sem textura (FBX sem imagens) e ficavam brancas.
# Este script dá a cada arma um material de cor própria (metal + detalhe em neon),
# usando o material base do blockout (/Game/Maps/NP_Materials/M_NP_Blockout).
import unreal

mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
BASE = unreal.load_asset("/Game/Maps/NP_Materials/M_NP_Blockout")
OUT = "/Game/Valdoria/Weapons/Materials"
LOG = unreal.Paths.project_saved_dir() + "color_weapons.txt"
COLORS = {
    "Rifle": ((0.10, 0.45, 0.42), 0.4),      # verde-azulado (Kael)
    "LaminasA": ((0.45, 0.12, 0.6), 0.6),    # roxo (Vex)
    "LaminasB": ((0.45, 0.12, 0.6), 0.6),
    "Martelo": ((0.75, 0.32, 0.08), 0.3),    # laranja (Brasa)
    "EscudoA": ((0.75, 0.32, 0.08), 0.3),
    "EscudoB": ((0.6, 0.45, 0.15), 0.2),
    "CajadoA": ((0.1, 0.6, 0.6), 0.8),       # turquesa (Iara)
    "CajadoB": ((0.1, 0.6, 0.6), 0.8),
    "Bastao": ((0.35, 0.35, 0.38), 0.0),
    "PecaPequena": ((0.35, 0.35, 0.38), 0.0),
    "Extras": ((0.25, 0.25, 0.27), 0.0),
}
lines = []
unreal.EditorAssetLibrary.make_directory(OUT)
for name, (rgb, glow) in COLORS.items():
    mesh = unreal.load_asset("/Game/Valdoria/Weapons/Merged/SM_Arma_" + name)
    if not mesh:
        lines.append("sem malha: " + name)
        continue
    path = OUT + "/MI_Arma_" + name
    mi = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        tools.create_asset("MI_Arma_" + name, OUT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, BASE)
    mel.set_material_instance_vector_parameter_value(mi, "Cor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1))
    mel.set_material_instance_scalar_parameter_value(mi, "Brilho", glow)
    mel.set_material_instance_scalar_parameter_value(mi, "Aspereza", 0.35)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    mesh.set_material(0, mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    lines.append("ok " + name)
with open(LOG, "w", encoding="utf-8") as fh:
    fh.write("\n".join(lines))
