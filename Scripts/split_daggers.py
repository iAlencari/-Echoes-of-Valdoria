# Splits the curved dagger pair (LaminasA region of the Meshy FBX) into one mesh per dagger,
# so Vex can hold one in each hand (owner's concept art). Output: SM_Arma_AdagaA (top), SM_Arma_AdagaB (bottom).
import unreal, traceback
SRC = "/Game/Valdoria/Weapons"
OUT = "/Game/Valdoria/Weapons/Merged"
out = []
try:
    lib = unreal.GeometryScript_AssetUtils
    def fn(name):
        for c in dir(unreal):
            if c.startswith("GeometryScript"):
                k = getattr(unreal, c)
                if hasattr(k, name):
                    return getattr(k, name)
    APPEND, TRANSLATE, CREATE = fn("append_mesh"), fn("translate_mesh"), fn("create_new_static_mesh_asset_from_mesh")
    parts = []
    for path in unreal.EditorAssetLibrary.list_assets(SRC, recursive=False):
        a = unreal.load_asset(path)
        if isinstance(a, unreal.StaticMesh):
            b = a.get_bounding_box()
            cx, cz = (b.min.x + b.max.x) * 0.5, (b.min.z + b.max.z) * 0.5
            if (b.max.x - b.min.x) < 40 and 9 <= cx <= 36.5 and 38 <= cz <= 57:
                parts.append({"mesh": a, "min": b.min, "max": b.max, "cz": cz})
    mid = (min(p["min"].z for p in parts) + max(p["max"].z for p in parts)) * 0.5
    mat = unreal.load_asset(OUT + "/SM_Arma_LaminasA").get_material(0)
    for name, group in (("AdagaA", [p for p in parts if p["cz"] >= mid]), ("AdagaB", [p for p in parts if p["cz"] < mid])):
        mn = unreal.Vector(min(p["min"].x for p in group), min(p["min"].y for p in group), min(p["min"].z for p in group))
        mx = unreal.Vector(max(p["max"].x for p in group), max(p["max"].y for p in group), max(p["max"].z for p in group))
        merged = unreal.DynamicMesh()
        for p in group:
            tmp = unreal.DynamicMesh()
            res = lib.copy_mesh_from_static_mesh(p["mesh"], tmp, unreal.GeometryScriptCopyMeshFromAssetOptions(), unreal.GeometryScriptMeshReadLOD())
            APPEND(merged, res[0] if isinstance(res, tuple) else tmp, unreal.Transform())
        TRANSLATE(merged, ((mn + mx) * 0.5) * -1.0)
        path = OUT + "/SM_Arma_" + name
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.EditorAssetLibrary.delete_asset(path)
        res = CREATE(merged, path, unreal.GeometryScriptCreateNewStaticMeshAssetOptions())
        asset = res[0] if isinstance(res, tuple) else res
        asset.set_material(0, mat)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        s = mx - mn
        out.append("%s: %d pecas, %.0f x %.0f x %.0f" % (name, len(group), s.x, s.y, s.z))
except Exception:
    out.append(traceback.format_exc())
open(unreal.Paths.project_saved_dir() + "split_daggers.txt", "w").write("\n".join(out))