# Turn 51: which locomotion loops move their root bone (root motion not extracted = the mesh slides ahead and
# snaps back every cycle). Writes Saved/inspect_loop_root.txt.
import unreal
LOG = unreal.Paths.project_saved_dir() + "inspect_loop_root.txt"
out = []
paths = []
for folder in ("/Game/Free_Animation_Library", "/Game/Free_Crawl_Animation", "/Game/Characters/Mannequins/Anims", "/Game/FreeAnimationLibrary"):
    if unreal.EditorAssetLibrary.does_directory_exist(folder):
        paths += [p for p in unreal.EditorAssetLibrary.list_assets(folder, True, False)
                  if any(k in p for k in ("Crouch", "Prone", "Crawl"))]
for p in paths:
    a = unreal.load_asset(p.split(".")[0])
    if not isinstance(a, unreal.AnimSequence):
        continue
    try:
        n = a.get_editor_property("number_of_sampled_frames") if hasattr(a, "number_of_sampled_frames") else None
    except Exception:
        n = None
    length = a.get_play_length()
    rm = a.get_editor_property("enable_root_motion")
    lock = a.get_editor_property("root_motion_root_lock")
    try:
        t0 = unreal.AnimationLibrary.get_bone_pose_for_time(a, "root", 0.0, False).translation
        t1 = unreal.AnimationLibrary.get_bone_pose_for_time(a, "root", length, False).translation
        move = "%.1f,%.1f,%.1f" % (t1.x - t0.x, t1.y - t0.y, t1.z - t0.z)
    except Exception as e:
        move = "erro " + str(e)
    out.append("%s rootmotion=%s lock=%s len=%.2f root_delta=%s" % (p.split(".")[0], rm, lock, length, move))
open(LOG, "w", encoding="utf-8").write("\n".join(out))
