# CLAUDE.md — Echoes of Valdoria (Unreal Engine 5)

Hero shooter em **Unreal Engine 5.8 + C++**, com o Gameplay Ability System (GAS). O dono do
projeto é iniciante em programação (conhece um pouco de C++ de servidores OT): explique em
português simples. **Leia `LOG.md` inteiro antes de mexer.**

## Regras
- Personagens, nomes, mapas, arte e lore **originais**. Overwatch, Marvel Rivals, Deadlock e
  Valorant são só inspiração de gênero. Nada de Priston Tale/MU.
- Assets externos só com licença livre (CC0 etc.), **com aprovação do dono antes de baixar**,
  e creditados em `CREDITS.md`.
- **Balanceamento é decisão do dono.** Os números ficam nos Data Assets de cada herói
  (`UValdoriaHeroDefinition`); não mude valores por conta própria.
- Atributos são **por personagem**, não por função. Funções (Vanguarda, Guerreiro,
  Retaguarda, Flanqueador) são só etiquetas; um herói pode ter mais de uma.
- Toda mudança vai para o `LOG.md` (estado atual + bloco do turno) e para um commit.
- O caminho do projeto **não pode ter acento** (o compilador da Microsoft quebra): fica em
  `C:\Dev\EchoesOfValdoria`.

## Onde está o código
`Source/EchoesOfValdoria/Valdoria/` (o resto de `Source/` é do modelo Third Person da Unreal).

| Arquivo | O que faz |
|---|---|
| `ValdoriaTypes.h` | Funções (roles), slots de habilidade, modo do Shift, regras de movimento, números da arma |
| `ValdoriaHeroDefinition` | Data Asset de um herói: vida/armadura/escudo, movimento, arma, habilidades, ultimate, malha, câmera |
| `ValdoriaHeroCharacter` | Personagem genérico: lê a definição, entrada (criada em código), arma, movimento avançado, morte |
| `ValdoriaAttributeSet` | Vida, armadura, escudo, ultimate, multiplicador de dano (dano: escudo → armadura → vida) |
| `ValdoriaGameplayAbility` | Base das habilidades + tiro hitscan, impulso, pulso de cura, onda de choque |
| `ValdoriaEffects` | Efeitos de dano e cura (SetByCaller) |
| `ValdoriaTrainingDummy` | Bonecos de treino (básico, armadura, escudo, andando de lado) |
| `ValdoriaGameMode` | Modo de treino, renascimento e teste automático (`-ValdoriaAutoTest`) |
| `ValdoriaHUD` | HUD desenhado em código: mira, vida, munição, habilidades, ultimate, FPS/ping |

## Comandos (PowerShell)
```powershell
# compilar (feche o editor antes)
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" EchoesOfValdoriaEditor Win64 Development -Project="C:\Dev\EchoesOfValdoria\EchoesOfValdoria.uproject" -WaitMutex

# teste automático (atira, corre, deita, impulso, ultimate, tira print e fecha)
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Dev\EchoesOfValdoria\EchoesOfValdoria.uproject" -game -windowed -ResX=1600 -ResY=900 -log -ValdoriaAutoTest
# resultado: linhas "AUTOTEST" em Saved\Logs\EchoesOfValdoria.log; print em Saved\Screenshots\AutoTest.png

# script Python do editor (ex.: mudar o modo de jogo do mapa)
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Dev\EchoesOfValdoria\EchoesOfValdoria.uproject" -run=pythonscript -script="C:/Dev/EchoesOfValdoria/Scripts/set_gamemode.py" -unattended
```

## Git
- Repositório: https://github.com/iAlencari/-Echoes-of-Valdoria (ramo `main`).
- `.uasset`, `.umap`, imagens, áudio e modelos vão pelo **Git LFS** (`.gitattributes`).
- Não versionar `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`.
- Commits em português; não envie (`push`) sem o dono pedir.

## Pendências atuais (decisão do dono) — ver LOG.md (Turno 11)
- Animações de deitar/rastejar e escalar: retarget dos packs CC0 do Quaternius em andamento
  (`/Game/Valdoria/Animations/UAL1` e `UAL2` já importados; IK Rigs e retargeters criados).
- Barra de HP mais bonita para o jogador e para os inimigos (bonecos de treino); o dono vai
  mandar imagens de referência (este agente não consegue ler imagens — descrever em texto).
- Ataques variados (não só hitscan): habilidades no estilo de Wuyang e Moira (Overwatch), armas
  com recuo e spread; configurações por personagem, nos Data Assets.
- Menu de administrador (F1), menu gráfico + plugin NVIDIA DLSS, seleção de herói, heróis reais
  em Data Assets.
- Integração dos modelos 3D das armas (FBX/GLB do dono) e das imagens de conceito (Gemini).
- Efeitos visuais de verdade (assets livres aprovados).

## Lembrete
- **Balanceamento é decisão do dono**; não altere números por conta própria.
- **Andarilha**: ideia do dono, adiada — não descartada; não implemente sem ele pedir.
- Todo turno: atualize o `LOG.md` (estado atual + bloco do turno) e faça commit."oldString">
- Commits em português; não envie (`push`) sem o dono pedir.
