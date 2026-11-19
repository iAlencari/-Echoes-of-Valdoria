# Turn 45: finds places where a hero can fall out of the map.
# For every point of a grid over the level's static meshes it traces straight down from high above. A point is a HOLE
# when nothing solid is found down to VALDORIA_HOLE_DEPTH (default -700 cm, below the metro floor at -500).
# Run:  set VALDORIA_MAP=/Game/Maps/NP_NovaPauliceia   (comma list allowed)
#       UnrealEditor-Cmd.exe EchoesOfValdoria.uproject -run=pythonscript -script="Scripts/find_holes.py"
# Output: Saved/holes.txt (one line per hole cluster: map, centre, size, count).
import os
import unreal

maps = [m.strip() for m in os.environ.get("VALDORIA_MAP", "/Game/Maps/NP_NovaPauliceia").split(",") if m.strip()]
depth = float(os.environ.get("VALDORIA_HOLE_DEPTH", "-700"))
step = float(os.environ.get("VALDORIA_HOLE_STEP", "150"))
out = []
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for m in maps:
    unreal.EditorLoadingAndSavingUtils.load_map(m)
    world = unreal.EditorLevelLibrary.get_editor_world()
    lo = [1e9, 1e9]
    hi = [-1e9, -1e9]
    solids = []  # (x0, x1, y0, y1, zmin) of every collidable static mesh (axis-aligned bounds)
    for a in eas.get_all_level_actors():
        if not isinstance(a, unreal.StaticMeshActor):
            continue
        origin, ext = a.get_actor_bounds(False)
        if max(ext.x, ext.y) > 100000 or (ext.x < 1 and ext.y < 1):
            continue
        comp = a.static_mesh_component
        if comp.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION:
            continue
        solids.append((origin.x - ext.x, origin.x + ext.x, origin.y - ext.y, origin.y + ext.y, origin.z - ext.z))
        lo[0] = min(lo[0], origin.x - ext.x); lo[1] = min(lo[1], origin.y - ext.y)
        hi[0] = max(hi[0], origin.x + ext.x); hi[1] = max(hi[1], origin.y + ext.y)
    holes = []
    x = lo[0] + step * 0.5
    while x < hi[0]:
        y = lo[1] + step * 0.5
        while y < hi[1]:
            # (line traces do not work in the commandlet: no physics scene) -> geometric check on the bounds;
            # only pieces that reach the street level count (a viaduct above a hole does not close it)
            if not any(s0 <= x <= s1 and s2 <= y <= s3 and depth < sz < 100 for (s0, s1, s2, s3, sz) in solids):
                holes.append((x, y))
            y += step
        x += step
    # cluster neighbours
    seen = set()
    clusters = []
    hs = set(holes)
    for h in holes:
        if h in seen:
            continue
        stack = [h]
        seen.add(h)
        pts = []
        while stack:
            p = stack.pop()
            pts.append(p)
            for dx in (-step, 0, step):
                for dy in (-step, 0, step):
                    q = (round(p[0] + dx, 3), round(p[1] + dy, 3))
                    for cand in ((p[0] + dx, p[1] + dy),):
                        if cand in hs and cand not in seen:
                            seen.add(cand)
                            stack.append(cand)
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        clusters.append((len(pts), (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2, max(xs) - min(xs) + step, max(ys) - min(ys) + step))
    clusters.sort(reverse=True)
    out.append("%s: bounds x %.0f..%.0f y %.0f..%.0f, step %.0f, holes %d in %d clusters" % (m, lo[0], hi[0], lo[1], hi[1], step, len(holes), len(clusters)))
    inner = [h for h in holes if lo[0] + 400 < h[0] < hi[0] - 400 and lo[1] + 200 < h[1] < hi[1] - 200]
    out.append("  inner holes (not on the outer border): %d" % len(inner))
    for h in inner[:300]:
        out.append("    pt (%.0f, %.0f)" % h)
    for c in clusters[:40]:
        out.append("  hole: centre (%.0f, %.0f) size %.0f x %.0f cm, %d points" % (c[1], c[2], c[3], c[4], c[0]))

with open(unreal.Paths.project_saved_dir() + "holes.txt", "w", encoding="utf-8") as fh:
    fh.write("\n".join(out))
