# Turn 46: the Bifrost strands are instanced meshes; the energy material must allow that usage or it renders as
# the default grid material in the packaged/-game run.
import unreal
m = unreal.load_asset("/Game/Valdoria/Materials/M_Valdoria_Energia")
m.set_editor_property("used_with_instanced_static_meshes", True)
unreal.MaterialEditingLibrary.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log("energy material: instanced usage = %s" % m.get_editor_property("used_with_instanced_static_meshes"))
