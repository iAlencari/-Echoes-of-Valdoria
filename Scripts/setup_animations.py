# Deixa as animações do plugin MoverExamples (Epic) tocarem no Manny do projeto.
# O esqueleto do projeto passa a considerar o esqueleto do plugin "compatível"
# (mesma hierarquia de ossos do UE5 Mannequin). Os heróis gerados por IA que usarem
# o esqueleto padrão da Unreal também aproveitam essas animações.
import unreal

OUR_SKELETON = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
COMPATIBLE = [
    "/MoverExamples/Characters/Mannequins/Meshes/SK_Mannequin",
]

skel = unreal.load_asset(OUR_SKELETON)
current = list(skel.get_editor_property("compatible_skeletons"))
paths = [str(s.get_path_name()) if hasattr(s, "get_path_name") else str(s) for s in current]
changed = False
for path in COMPATIBLE:
    other = unreal.load_asset(path)
    if other is None:
        unreal.log_error("VALDORIA esqueleto nao encontrado: " + path)
        continue
    if other.get_path_name() not in paths:
        current.append(other)
        changed = True
if changed:
    skel.set_editor_property("compatible_skeletons", current)
    unreal.EditorAssetLibrary.save_loaded_asset(skel)
unreal.log("VALDORIA setup_animations: compativeis = {}".format(
    [s.get_path_name() for s in skel.get_editor_property("compatible_skeletons")]))
