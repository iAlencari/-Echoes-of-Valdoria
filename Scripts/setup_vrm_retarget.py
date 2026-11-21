# Turn 49: IK Rig + IK Retargeter so a VRoid model (imported with the VRM4U plugin) copies the
# Mannequin's animation at runtime (the hero keeps the hidden Mannequin; the VRM is the visible body).
# Run:  UnrealEditor-Cmd.exe EchoesOfValdoria.uproject -run=pythonscript -script="Scripts/setup_vrm_retarget.py"
# Models: VALDORIA_VRM="HairSampleMale/SK_HairSample_Male,..." (paths under /Game/Valdoria/Characters/VRoid).
import os
import traceback
import unreal

IK_MANNEQUIN = "/MoverExamples/Characters/Mannequins/Rigs/IK_Mannequin"
BASE = "/Game/Valdoria/Characters/VRoid/"
LOG = unreal.Paths.project_saved_dir() + "setup_vrm_retarget.txt"
lines = []
VROID_CHAINS = [
    ("Spine", "J_Bip_C_Spine", "J_Bip_C_UpperChest"), ("Neck", "J_Bip_C_Neck", "J_Bip_C_Neck"), ("Head", "J_Bip_C_Head", "J_Bip_C_Head"),
] + [c for s, S in (("Left", "L"), ("Right", "R")) for c in (
    (s + "Clavicle", "J_Bip_%s_Shoulder" % S, "J_Bip_%s_Shoulder" % S),
    (s + "Arm", "J_Bip_%s_UpperArm" % S, "J_Bip_%s_Hand" % S),
    (s + "Leg", "J_Bip_%s_UpperLeg" % S, "J_Bip_%s_Foot" % S),
    (s + "Foot", "J_Bip_%s_ToeBase" % S, "J_Bip_%s_ToeBase" % S),
    (s + "Thumb", "J_Bip_%s_Thumb1" % S, "J_Bip_%s_Thumb3" % S),
    (s + "Index", "J_Bip_%s_Index1" % S, "J_Bip_%s_Index3" % S),
    (s + "Middle", "J_Bip_%s_Middle1" % S, "J_Bip_%s_Middle3" % S),
    (s + "Ring", "J_Bip_%s_Ring1" % S, "J_Bip_%s_Ring3" % S),
    (s + "Pinky", "J_Bip_%s_Little1" % S, "J_Bip_%s_Little3" % S))]


def log(msg):
    lines.append(str(msg))
    unreal.log("VRMRTG " + str(msg))


def asset(path, cls, factory):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    folder, name = path.rsplit("/", 1)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, cls, factory)


def setup(mesh_rel):
    mesh_path = BASE + mesh_rel
    folder = mesh_path.rsplit("/", 1)[0]
    name = mesh_path.rsplit("/", 1)[1].replace("SK_", "")
    mesh = unreal.load_asset(mesh_path)
    if not mesh:
        log("malha não encontrada: " + mesh_path)
        return
    rig = asset(folder + "/IK_" + name, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    ctl = unreal.IKRigController.get_controller(rig)
    ctl.set_skeletal_mesh(mesh)
    ok = ctl.apply_auto_generated_retarget_definition()
    if not ctl.get_retarget_chains():
        # VRoid bone names (J_Bip_*) are not recognised by the auto rig: build the chains by hand, with the
        # same names as IK_Mannequin so the retargeter maps them one to one.
        ctl.set_retarget_root("J_Bip_C_Hips")
        for chain, start, end in VROID_CHAINS:
            ctl.add_retarget_chain(chain, start, end, "")
        ok = "manual"
    log("IK %s auto=%s chains=%s" % (name, ok, ctl.get_retarget_chains()))
    unreal.EditorAssetLibrary.save_loaded_asset(rig)

    src = unreal.load_asset(IK_MANNEQUIN)
    rtg = asset(folder + "/RTG_Manny_" + name, unreal.IKRetargeter, unreal.IKRetargetFactory())
    rc = unreal.IKRetargeterController.get_controller(rtg)
    rc.remove_all_ops()
    rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src)
    rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, rig)
    rc.add_default_ops()
    rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, src)
    rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, rig)
    try:
        rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    except Exception:
        rc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
    # Manny is in A-pose, VRoid in T-pose: align the target bones to the source pose.
    for args in ((unreal.RetargetSourceOrTarget.TARGET,),
                 (unreal.RetargetSourceOrTarget.TARGET, unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)):
        try:
            rc.auto_align_all_bones(*args)
            break
        except Exception:
            log("auto_align falhou: " + traceback.format_exc().splitlines()[-1])
    try:
        log("mapa: %s" % [(str(c.target_chain), str(rc.get_source_chain(c.target_chain))) for c in rc.get_all_chain_settings()])
    except Exception:
        log("mapa: ?")
    unreal.EditorAssetLibrary.save_loaded_asset(rtg)
    log("ok " + rtg.get_path_name())


try:
    for m in os.environ.get("VALDORIA_VRM", "HairSampleMale/SK_HairSample_Male").split(","):
        setup(m.strip())
except Exception:
    log(traceback.format_exc())
open(LOG, "w", encoding="utf-8").write("\n".join(lines))
