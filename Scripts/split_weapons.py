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

def touches(p, q):
    return (p["min"].x - GAP <= q["max"].x and q["min"].x - GAP <= p["max"].x and
            p["min"].y - GAP <= q["max"].y and q["min"].y - GAP <= p["max"].y and
            p["min"].z - GAP <= q["max"].z and q["min"].z - GAP <= p["max"].z)

# União de grupos (union-find).
parent = list(range(len(parts)))
def find(i):
    while parent[i] != i:
        parent[i] = parent[parent[i]]
        i = parent[i]
    return i
for i in range(len(parts)):
    for j in range(i + 1, len(parts)):
        if touches(parts[i], parts[j]):
            parent[find(i)] = find(j)
groups = {}
for i in range(len(parts)):
    groups.setdefault(find(i), []).append(parts[i])
clusters = sorted(groups.values(), key=lambda g: (-round(min(p["min"].z for p in g) / 10.0), min(p["min"].x for p in g)))
out.append("grupos: %d" % len(clusters))

unreal.EditorAssetLibrary.make_directory(OUT)
mat = unreal.load_asset(SRC + "/Material")
for idx, group in enumerate(clusters):
    mn = unreal.Vector(min(p["min"].x for p in group), min(p["min"].y for p in group), min(p["min"].z for p in group))
    mx = unreal.Vector(max(p["max"].x for p in group), max(p["max"].y for p in group), max(p["max"].z for p in group))
    center = (mn + mx) * 0.5
    size = mx - mn
    name = "SM_Arma_%02d" % idx
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
