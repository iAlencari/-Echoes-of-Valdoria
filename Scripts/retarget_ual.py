# Converte animações CC0 da Quaternius (Universal Animation Library 1 e 2) para o
# esqueleto do Mannequin da Unreal, usando o IK Retargeter do próprio motor.
#
# Rodar depois de Scripts/import_ual.py:
#   UnrealEditor-Cmd.exe EchoesOfValdoria.uproject -run=pythonscript -script="Scripts/retarget_ual.py"
#
# Resultado: animações em /Game/Valdoria/Animations/Retarget, no esqueleto do Mannequin.
# Os pacotes originais (Content/Valdoria/Animations/UAL1 e UAL2) podem ser apagados depois:
# as animações convertidas já são independentes.
import unreal
import traceback

# Malhas/esqueletos importados do GLB (fonte) e malha do jogo (destino).
SRC_MESH_1 = "/Game/Valdoria/Animations/UAL1/UAL1_Standard/SkeletalMeshes/UAL1"
SRC_MESH_2 = "/Game/Valdoria/Animations/UAL2/UAL2_Standard/SkeletalMeshes/UAL2"
TGT_MESH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"

IK_MANNEQUIN = "/MoverExamples/Characters/Mannequins/Rigs/IK_Mannequin"
RIG_DIR = "/Game/Valdoria/Rigs"
OUT_DIR = "/Game/Valdoria/Animations/Retarget"

# O que queremos converter: (caminho da animação, mesh fonte)
WANTED = [
    ("/Game/Valdoria/Animations/UAL2/UAL2_Standard/SkeletalMeshes/UAL2LayToIdle", SRC_MESH_2),
    ("/Game/Valdoria/Animations/UAL2/UAL2_Standard/SkeletalMeshes/UAL2ClimbUp_1m", SRC_MESH_2),
    ("/Game/Valdoria/Animations/UAL2/UAL2_Standard/SkeletalMeshes/UAL2NinjaJump_Idle_Loop", SRC_MESH_2),
    ("/Game/Valdoria/Animations/UAL2/UAL2_Standard/SkeletalMeshes/UAL2Idle_Rail_Loop", SRC_MESH_2),
    ("/Game/Valdoria/Animations/UAL2/UAL2_Standard/SkeletalMeshes/UAL2Slide_Start", SRC_MESH_2),
    ("/Game/Valdoria/Animations/UAL2/UAL2_Standard/SkeletalMeshes/UAL2Slide_Loop", SRC_MESH_2),
    ("/Game/Valdoria/Animations/UAL1/UAL1_Standard/SkeletalMeshes/UAL1Swim_Fwd_Loop", SRC_MESH_1),
    ("/Game/Valdoria/Animations/UAL1/UAL1_Standard/SkeletalMeshes/UAL1Roll", SRC_MESH_1),
    ("/Game/Valdoria/Animations/UAL2/UAL2_Standard/SkeletalMeshes/UAL2Slide_Exit", SRC_MESH_2),
    ("/Game/Valdoria/Animations/UAL1/UAL1_Standard/SkeletalMeshes/UAL1Swim_Idle_Loop", SRC_MESH_1),
]


def log(msg):
    unreal.log("RETARGET " + str(msg))
    with open(unreal.Paths.project_saved_dir() + "retarget_log.txt", "a", encoding="utf-8") as fh:
        fh.write(str(msg) + "\n")


def step(name, fn):
    try:
        out = fn()
        log("OK %s -> %s" % (name, out))
        return out
    except Exception:
        log("FALHOU %s: %s" % (name, traceback.format_exc()))
        return None


def make_source_rig(mesh_path, rig_path):
    """IK Rig novo para a malha da Quaternius, com as chains geradas automaticamente."""
    if unreal.EditorAssetLibrary.does_asset_exist(rig_path):
        rig = unreal.load_asset(rig_path)
    else:
        rig = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            rig_path.rsplit("/", 1)[1], rig_path.rsplit("/", 1)[0],
            unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    if not rig:
        return None
    ctl = unreal.IKRigController.get_controller(rig)
    mesh = unreal.load_asset(mesh_path)
    if not ctl.set_skeletal_mesh(mesh):
        log("set_skeletal_mesh falhou em " + rig_path)
    ok = ctl.apply_auto_generated_retarget_definition()
    log("chains em %s (auto=%s): %s" % (rig_path, ok, ctl.get_retarget_chains()))
    unreal.EditorAssetLibrary.save_loaded_asset(rig)
    return rig


def make_retargeter(src_rig, tgt_rig, rtg_path):
    """Retargeter novo: fonte = Quaternius, destino = Mannequin do jogo."""
    if unreal.EditorAssetLibrary.does_asset_exist(rtg_path):
        rtg = unreal.load_asset(rtg_path)
    else:
        rtg = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            rtg_path.rsplit("/", 1)[1], rtg_path.rsplit("/", 1)[0],
            unreal.IKRetargeter, unreal.IKRetargetFactory())
    if not rtg:
        return None
    ctl = unreal.IKRetargeterController.get_controller(rtg)
    # Recomeça do zero (rodar o script de novo não duplica as operações).
    ctl.remove_all_ops()
    ctl.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
    ctl.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, tgt_rig)
    ctl.add_default_ops()
    ctl.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
    ctl.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, tgt_rig)
    try:
        ctl.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    except Exception:
        log("auto_map FUZZY falhou: " + traceback.format_exc())
        ctl.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
    # Quaternius está em T-pose e o Manny em A-pose: alinha os ossos do alvo à pose da fonte.
    for args in ((unreal.RetargetSourceOrTarget.TARGET,),
                 (unreal.RetargetSourceOrTarget.TARGET, unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)):
        try:
            ctl.auto_align_all_bones(*args)
            log("auto_align_all_bones OK com %d argumentos" % len(args))
            break
        except Exception:
            log("auto_align_all_bones falhou: " + traceback.format_exc().splitlines()[-1])
    try:
        mapped = [(str(c.target_chain), str(ctl.get_source_chain(c.target_chain))) for c in ctl.get_all_chain_settings()]
    except Exception:
        mapped = "?"
    log("mapeamento: %s" % (mapped,))
    log("retargeter %s: %d chains" % (rtg_path, len(list(ctl.get_all_chain_settings()))))
    unreal.EditorAssetLibrary.save_loaded_asset(rtg)
    return rtg


def retarget_one(anim_path, src_mesh_path, rtg):
    anim = unreal.load_asset(anim_path)
    if not anim:
        log("animação não encontrada: " + anim_path)
        return None
    src_mesh = unreal.load_asset(src_mesh_path)
    tgt_mesh = unreal.load_asset(TGT_MESH)
    result = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        [unreal.EditorAssetLibrary.find_asset_data(anim_path)], src_mesh, tgt_mesh, rtg,
        search="", replace="", prefix="", suffix="",
        target_path=OUT_DIR, use_source_path=False,
        include_referenced_assets=False, overwrite_existing_files=True)
    names = [str(a.package_name) for a in result] if result else []
    log("retarget %s -> %s" % (anim_path.rsplit("/", 1)[-1], names))
    return names


def main():
    log("início")
    unreal.EditorAssetLibrary.make_directory(RIG_DIR)
    rig1 = step("criar IK da UAL1", lambda: make_source_rig(SRC_MESH_1, RIG_DIR + "/IK_UAL1"))
    rig2 = step("criar IK da UAL2", lambda: make_source_rig(SRC_MESH_2, RIG_DIR + "/IK_UAL2"))
    tgt_rig = unreal.load_asset(IK_MANNEQUIN)

    rtg1 = step("criar retargeter UAL1", lambda: make_retargeter(rig1, tgt_rig, RIG_DIR + "/RTG_UAL1"))
    rtg2 = step("criar retargeter UAL2", lambda: make_retargeter(rig2, tgt_rig, RIG_DIR + "/RTG_UAL2"))

    for anim_path, src_mesh_path in WANTED:
        rtg = rtg2 if src_mesh_path == SRC_MESH_2 else rtg1
        if not rtg:
            log("sem retargeter para " + anim_path)
            continue
        step("retarget " + anim_path.rsplit("/", 1)[-1],
             lambda a=anim_path, m=src_mesh_path, r=rtg: retarget_one(a, m, r))

    unreal.EditorAssetLibrary.save_directory(OUT_DIR, only_if_is_dirty=False, recursive=True)
    log("fim")


main()
