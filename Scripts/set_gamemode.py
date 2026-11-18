# Faz o mapa de testes usar o modo de jogo do Echoes of Valdoria (C++).
# Rodar: UnrealEditor-Cmd.exe <projeto> -run=pythonscript -script="<este arquivo>"
import unreal

MAP = "/Game/ThirdPerson/Lvl_ThirdPerson"
GAME_MODE = "/Script/EchoesOfValdoria.ValdoriaGameMode"

world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
settings = world.get_world_settings()
game_mode = unreal.load_class(None, GAME_MODE)
settings.set_editor_property("default_game_mode", game_mode)
ok = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
unreal.log("VALDORIA set_gamemode: {} -> {} salvo={}".format(MAP, GAME_MODE, ok))
