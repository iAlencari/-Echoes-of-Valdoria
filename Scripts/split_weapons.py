# Junta as 158 peças importadas do FBX do Meshy em uma malha por arma.
# As peças que se tocam (caixas próximas) viram um grupo = uma arma.
# Resultado: /Game/Valdoria/Weapons/Merged/SM_Arma_XX, com o pivô no centro de cada arma.
import unreal, traceback

SRC = "/Game/Valdoria/Weapons"
OUT = "/Game/Valdoria/Weapons/Merged"
LOG = unreal.Paths.project_saved_dir() + "split_weapons.txt"
GAP = 0.0  # cm: peças a menos disso uma da outra pertencem à mesma arma
out = []

# Acha as funções pelo nome (os nomes das classes mudam entre versões da Unreal).
def find_fn(method):
    for cls_name in dir(unreal):
        if cls_name.startswith("GeometryScript") and cls_name.endswith(("Functions", "Library")) or cls_name.startswith("GeometryScript_"):
            cls = getattr(unreal, cls_name)
            if hasattr(cls, method):
                out.append("%s -> %s" % (method, cls_name))
                return getattr(cls, method)
    out.append("nao achei " + method)
    return None
APPEND = find_fn("append_mesh")
TRANSLATE = find_fn("translate_mesh")
CREATE = find_fn("create_new_static_mesh_asset_from_mesh")

parts = []
for path in unreal.EditorAssetLibrary.list_assets(SRC, recursive=False):
    a = unreal.load_asset(path)
    if isinstance(a, unreal.StaticMesh):
        b = a.get_bounding_box()
        parts.append({"mesh": a, "min": b.min, "max": b.max})

# Regiões de cada arma na vista de frente (x, z em cm) do FBX do Meshy, tiradas do desenho
# das peças. Cada peça vai para a região que contém o centro dela.
REGIONS = [
    ("Rifle",          (-52, -4), (36, 60)),
    ("PecaPequena",    (-5, 1.5), (38, 60)),
    ("LaminasA",       (9, 36.5), (38, 57)),
    ("LaminasB",       (36.5, 46), (33, 60)),
    ("Martelo",        (-52, -18), (15, 31)),
    ("EscudoA",        (-18, 3), (8, 37)),
    ("Bastao",         (4, 24.5), (8, 37)),
    ("EscudoB",        (24.5, 45), (8, 37)),
    ("CajadoA",        (-52, 4), (-1, 11)),
    ("CajadoB",        (4, 48), (-1, 11)),
]
groups = {}
for p in parts:
    cx = (p["min"].x + p["max"].x) * 0.5
    cz = (p["min"].z + p["max"].z) * 0.5
    sx = p["max"].x - p["min"].x
    name = "Extras"
    if sx < 40:  # placas grandes que ligam tudo ficam de fora
        for rname, (x0, x1), (z0, z1) in REGIONS:
            if x0 <= cx <= x1 and z0 <= cz <= z1:
                name = rname
                break
    groups.setdefault(name, []).append(p)
clusters = sorted(groups.items())
out.append("grupos: %s" % [(k, len(v)) for k, v in clusters])
unreal.EditorAssetLibrary.make_directory(OUT)
mat = unreal.load_asset(SRC + "/Material")
for idx, (gname, group) in enumerate(clusters):
    mn = unreal.Vector(min(p["min"].x for p in group), min(p["min"].y for p in group), min(p["min"].z for p in group))
    mx = unreal.Vector(max(p["max"].x for p in group), max(p["max"].y for p in group), max(p["max"].z for p in group))
    center = (mn + mx) * 0.5
    size = mx - mn
    name = "SM_Arma_" + gname
    try:
        merged = unreal.DynamicMesh()
        for p in group:
            tmp = unreal.DynamicMesh()
            res = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(
                p["mesh"], tmp, unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD())
            src_mesh = res[0] if isinstance(res, tuple) else tmp
            APPEND(merged, src_mesh, unreal.Transform())
        TRANSLATE(merged, center * -1.0)
        res = CREATE(
            merged, OUT + "/" + name, unreal.GeometryScriptCreateNewStaticMeshAssetOptions())
        asset = res[0] if isinstance(res, tuple) else res
        if asset and mat:
            asset.set_material(0, mat)
            unreal.EditorAssetLibrary.save_loaded_asset(asset)
        out.append("%s: %d pecas, tamanho %.0f x %.0f x %.0f cm, centro (%.0f, %.0f, %.0f)" % (
            name, len(group), size.x, size.y, size.z, center.x, center.y, center.z))
    except Exception:
        out.append("%s FALHOU: %s" % (name, traceback.format_exc()))

with open(LOG, "w", encoding="utf-8") as fh:
    fh.write("\n".join(out))
