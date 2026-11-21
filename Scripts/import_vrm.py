# Turn 49: imports VRoid/VRM models with the VRM4U plugin into /Game/Valdoria/Characters/VRoid/<Name>/.
# Must run in the FULL editor (the importer opens windows, the commandlet crashes):
#   UnrealEditor.exe EchoesOfValdoria.uproject -ExecutePythonScript="C:/Dev/EchoesOfValdoria/Scripts/import_vrm.py" -unattended -nosplash
# Files: ArtSource/VRoid/*.vrm (or VALDORIA_VRM_FILES="A.vrm,B.vrm"). Existing models are skipped.
# Then run Scripts/setup_vrm_retarget.py for the retargeter.
import glob
import os
import traceback
import unreal

SRC = r"C:/Dev/EchoesOfValdoria/ArtSource/VRoid"
out = []
files = os.environ.get("VALDORIA_VRM_FILES")
paths = [os.path.join(SRC, f.strip()) for f in files.split(",")] if files else sorted(glob.glob(SRC + "/*.vrm"))
for p in paths:
    name = os.path.splitext(os.path.basename(p))[0]
    folder = "/Game/Valdoria/Characters/VRoid/" + name.replace("_", "")
    if unreal.EditorAssetLibrary.does_asset_exist(folder + "/SK_" + name):
        out.append("já existe: " + name)
        continue
    try:
        r = unreal.VrmImporterBPFunctionLibrary.import_vrm_file_with_options(p.replace("\\", "/"), folder + "/" + name, unreal.ImportOptionData())
        out.append("importado %s -> %s" % (name, r.get_editor_property("skeletal_mesh") if r else None))
    except Exception:
        out.append("ERRO %s: %s" % (name, traceback.format_exc()))
out.append("save: %s" % unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True))
open(unreal.Paths.project_saved_dir() + "import_vrm.txt", "w", encoding="utf-8").write("\n".join(out))
unreal.SystemLibrary.quit_editor()
