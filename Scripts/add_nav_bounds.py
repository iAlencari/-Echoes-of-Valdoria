# Adds (or replaces) a NavMeshBoundsVolume covering the whole map, so the runtime navmesh
# (RecastNavMesh RuntimeGeneration=Dynamic in DefaultEngine.ini) gives the autopilot real paths.
# Run:  set VALDORIA_MAP=/Game/ThirdPerson/Lvl_ThirdPerson
#       UnrealEditor-Cmd.exe EchoesOfValdoria.uproject -run=pythonscript -script="Scripts/add_nav_bounds.py"
# build_np_map.py calls add_nav_bounds() itself for Nova Pauliceia.
import os
import unreal


def add_nav_bounds(log=print):
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = eas.get_all_level_actors()
    for a in actors:
        if isinstance(a, unreal.NavMeshBoundsVolume):
            eas.destroy_actor(a)
    lo = [1e9, 1e9, 1e9]
    hi = [-1e9, -1e9, -1e9]
    count = 0
    for a in eas.get_all_level_actors():
        if not isinstance(a, unreal.StaticMeshActor):
            continue
        origin, extent = a.get_actor_bounds(False)
        if max(extent.x, extent.y, extent.z) > 100000:
            continue
        for i, (o, e) in enumerate(((origin.x, extent.x), (origin.y, extent.y), (origin.z, extent.z))):
            lo[i] = min(lo[i], o - e)
            hi[i] = max(hi[i], o + e)
        count += 1
    if count == 0:
        log("add_nav_bounds: no static meshes, nothing done")
        return None
    centre = unreal.Vector((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, (lo[2] + hi[2]) / 2)
    half = [(hi[i] - lo[i]) / 2 + 300 for i in range(3)]
    vol = eas.spawn_actor_from_class(unreal.NavMeshBoundsVolume, centre)
    vol.set_actor_label("NavBounds_Valdoria")
    # The default volume brush is a 200 cm cube (half extent 100).
    vol.set_actor_scale3d(unreal.Vector(half[0] / 100.0, half[1] / 100.0, half[2] / 100.0))
    o, e = vol.get_actor_bounds(False)
    log("add_nav_bounds: volume centre={} half={} -> bounds extent={}".format(centre, half, e))
    return vol


if __name__ == "__main__":
    MAP = os.environ.get("VALDORIA_MAP", "")
    if MAP:
        les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        les.load_level(MAP)
        add_nav_bounds(unreal.log)
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        unreal.log("add_nav_bounds: saved " + MAP)
