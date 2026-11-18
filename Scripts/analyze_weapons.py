# Analisa as peças importadas do FBX das armas (onde cada uma está), para separar por arma.
import unreal, json
DEST = "/Game/Valdoria/Weapons"
rows = []
for path in unreal.EditorAssetLibrary.list_assets(DEST, recursive=False):
    a = unreal.load_asset(path)
    if isinstance(a, unreal.StaticMesh):
        b = a.get_bounding_box()
        c = (b.min + b.max) * 0.5
        s = b.max - b.min
        rows.append({"p": path.split(".")[0], "c": [c.x, c.y, c.z], "s": [s.x, s.y, s.z], "t": a.get_num_triangles(0)})
with open(unreal.Paths.project_saved_dir() + "weapons_parts.json", "w") as fh:
    json.dump(rows, fh)
plugins = [p for p in dir(unreal) if p.startswith("GeometryScript")]
with open(unreal.Paths.project_saved_dir() + "geoscript.txt", "w") as fh:
    fh.write("\n".join(plugins[:50]))
