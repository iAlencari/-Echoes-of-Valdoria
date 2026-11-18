# Diagnóstico do retargeter (escreve em Saved/inspect_rtg.txt)
import unreal
out = []
ctl_cls = unreal.IKRetargeterController
out.append("METODOS: " + ", ".join(m for m in dir(ctl_cls) if not m.startswith("_")))
rtg = unreal.load_asset("/Game/Valdoria/Rigs/RTG_UAL2")
ctl = unreal.IKRetargeterController.get_controller(rtg)
for name in ["get_ik_rig", "get_retarget_ops", "get_num_retarget_ops"]:
    out.append(name + ": " + str(hasattr(ctl, name)))
try:
    out.append("src rig: " + str(ctl.get_ik_rig(unreal.RetargetSourceOrTarget.SOURCE)))
    out.append("tgt rig: " + str(ctl.get_ik_rig(unreal.RetargetSourceOrTarget.TARGET)))
except Exception as e:
    out.append("erro rig: " + str(e))
try:
    n = ctl.get_num_retarget_ops()
    out.append("ops: %d" % n)
    for i in range(n):
        out.append("  op %d: %s" % (i, ctl.get_op_name(i) if hasattr(ctl, "get_op_name") else "?"))
except Exception as e:
    out.append("erro ops: " + str(e))
tgt = unreal.load_asset("/MoverExamples/Characters/Mannequins/Rigs/IK_Mannequin")
tctl = unreal.IKRigController.get_controller(tgt)
out.append("chains alvo: " + str([c.chain_name for c in tctl.get_retarget_chains()]))
with open(unreal.Paths.project_saved_dir() + "inspect_rtg.txt", "w", encoding="utf-8") as fh:
    fh.write("\n".join(out))
