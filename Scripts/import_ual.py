# Importa os pacotes CC0 da Quaternius (Universal Animation Library 1 e 2, formato GLB)
# para dentro do projeto. Os arquivos ficam fora do repositório (pasta do dono).
# Depois o Scripts/retarget_ual.py converte só algumas animações para o esqueleto do Mannequin.
import unreal

UAL1 = r"C:\Users\Pichau\Documents\Projeto Padrão\Universal Animation Library[Standard]\Universal Animation Library[Standard]\Unreal-Godot\UAL1_Standard.glb"
UAL2 = r"C:\Users\Pichau\Documents\Projeto Padrão\Universal Animation Library 2[Standard]\Universal Animation Library 2[Standard]\Unreal-Godot\UAL2_Standard.glb"

JOBS = [
    (UAL1, "/Game/Valdoria/Animations/UAL1", "UAL1"),
    (UAL2, "/Game/Valdoria/Animations/UAL2", "UAL2"),
]


def log(msg):
    unreal.log("UAL " + str(msg))


for path, dest, name in JOBS:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported = list(task.get_editor_property("imported_object_paths"))
    log("importado %s -> %s (%d objetos)" % (name, dest, len(imported)))
    for obj in imported:
        log("   asset: %s" % obj)

unreal.EditorAssetLibrary.save_directory("/Game/Valdoria", only_if_is_dirty=False, recursive=True)
log("pastas salvas")
