# Importa os sons escolhidos (RawContent/Audio/<categoria>) para /Game/Valdoria/Audio/<categoria>
# e as imagens da tela de carregamento (RawContent/Loading) para /Game/Valdoria/UI/Loading.
# Origem dos sons e licenças: CREDITS.md.
# Rodar:  UnrealEditor-Cmd.exe EchoesOfValdoria.uproject -run=pythonscript -script="Scripts/import_audio.py"
import os
import unreal

project = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
tools = unreal.AssetToolsHelpers.get_asset_tools()
lines = []

def import_folder(src, dest):
    tasks = []
    for name in sorted(os.listdir(src)):
        full = os.path.join(src, name)
        if os.path.isdir(full):
            continue
        t = unreal.AssetImportTask()
        t.filename = full
        t.destination_path = dest
        t.replace_existing = True
        t.automated = True
        t.save = True
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    for t in tasks:
        lines.append("%s -> %s" % (os.path.basename(t.filename), list(t.imported_object_paths)))

audio = os.path.join(project, "RawContent", "Audio")
for cat in sorted(os.listdir(audio)):
    if os.path.isdir(os.path.join(audio, cat)):
        import_folder(os.path.join(audio, cat), "/Game/Valdoria/Audio/" + cat)

loading = os.path.join(project, "RawContent", "Loading")
if os.path.isdir(loading):
    import_folder(loading, "/Game/Valdoria/UI/Loading")
    for asset in unreal.EditorAssetLibrary.list_assets("/Game/Valdoria/UI/Loading", recursive=False):
        tex = unreal.load_asset(asset)
        if isinstance(tex, unreal.Texture2D):
            tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
            tex.set_editor_property("never_stream", True)
            unreal.EditorAssetLibrary.save_loaded_asset(tex)

with open(unreal.Paths.project_saved_dir() + "import_audio.txt", "w", encoding="utf-8") as fh:
    fh.write("\n".join(lines))
