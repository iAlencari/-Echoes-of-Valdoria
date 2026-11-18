# Importa as armas dos heróis (FBX exportado do Meshy) como malhas estáticas separadas.
# Rodar: UnrealEditor-Cmd.exe <projeto> -run=pythonscript -script="Scripts/import_weapons.py"
import unreal

SRC = r"C:/Dev/EchoesOfValdoria/SourceArt/Armas/Armas_Herois.fbx"
DEST = "/Game/Valdoria/Weapons"
LOG = unreal.Paths.project_saved_dir() + "import_weapons.txt"

lines = []
task = unreal.AssetImportTask()
task.filename = SRC
task.destination_path = DEST
task.automated = True
task.replace_existing = True
task.save = True

options = unreal.FbxImportUI()
options.import_mesh = True
options.import_as_skeletal = False
options.import_materials = True
options.import_textures = True
options.import_animations = False
options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
options.static_mesh_import_data.combine_meshes = False
options.static_mesh_import_data.generate_lightmap_u_vs = True
options.static_mesh_import_data.auto_generate_collision = True
task.options = options

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
lines.append("importados: %s" % list(task.imported_object_paths))
for path in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True):
    asset = unreal.load_asset(path)
    info = path
    if isinstance(asset, unreal.StaticMesh):
        box = asset.get_bounding_box()
        size = box.max - box.min
        info += "  tamanho(cm)=%.1f x %.1f x %.1f  triangulos=%d" % (size.x, size.y, size.z, asset.get_num_triangles(0))
    lines.append(info)
with open(LOG, "w", encoding="utf-8") as fh:
    fh.write("\n".join(lines))
