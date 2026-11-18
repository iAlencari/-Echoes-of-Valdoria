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

## Decisões de design do dono (turno 20, 18/11/2026)
- **Heróis prontos**: Brasa, Kael Rook, Iara e Vex em `Content/Valdoria/Heroes/DA_*.uasset`
  (criados por `Scripts/create_heroes.py` a partir de `ValdoriaHeroPresets.cpp`; o script **não**
  sobrescreve DA_ existentes, a não ser com `VALDORIA_RESET_HEROES=1`). Balancear = editar o DA_.
- **Botões**: botão direito, F e E são as 3 habilidades; Q é a ultimate; a passiva é sempre ativa.
- **Tipos de dano**: Físico (armas, golpes), Mágico (cajados, magias) e Híbrido (parte de cada).
  Cada arma/habilidade declara o tipo no Data Asset; cada herói tem Poder Físico/Mágico e
  Resistência Física/Mágica. A loja e itens vão somar a esses valores.
- **Loja na tecla B** (referência: Paladins, e Deadlock também): poucas categorias (defesa,
  utilidade, cura, dano), itens com ~3 níveis comprados em sequência, limite de slots, créditos
  ganhos na partida, tudo zera a cada partida. Itens num arquivo de dados para o dono balancear.
- **Biblioteca de habilidades reutilizáveis** (`ValdoriaHeroAbilities.h`): cada classe lê
  Magnitude/Radius/Duration/SecondaryMagnitude do Data Asset. Estados genéricos: Silenciado,
  Enraizado, Atordoado, Sem ataque, Sem habilidades (`ApplyStatus`, no alvo ou em si mesmo).
- **Ideias para a biblioteca / próximos heróis** (originais; outros jogos só como referência de
  mecânica): teleporte com retorno, portais (só time / todos / inimigos também), puxar aliado ou ir
  até ele, refletir projéteis, absorver dano virando escudo, intangível (imune a físico, mágico
  ainda acerta, sem atacar), invisibilidade parcial (já existe), rastro para deslizar (já existe,
  inclusive como rampa no ar no futuro), arma de feixe contínuo que trava no alvo e cura aliados
  (5º herói), habilidades em dupla entre heróis, cartas/talentos que modificam habilidades.
  Lista de padrões de habilidades: `docs/HabilidadesReferencia.md`.
- **MOBA spots (owner's decision, 2026-11-18)**: instead of towers, each lane has 3 **spots** = guardian monsters/bosses that move inside their area and focus the enemy hero who attacks an allied hero there. Each spot gives its team a different buff while alive. Spot 1: 50% damage reduction during the first minute, fading to 0% at minute 5 (configurable). Spots 2 and 3: buffs to be decided by the owner (proposed: spot 2 +20% damage fading, spot 3 regeneration). Name: **Guardian Spots**. Comeback rules: if a team's spot 1 falls before minute 5, that team keeps the spot-1 buff doubled and constant until minute 5; spot 2 buff fades from full to 0 between minutes 5 and 10; if spots 1 and 2 of a lane fall before minute 5, that lane's last spot buffs triple until minute 10. One of the two last-spot guardians is a support mage (area heal on cooldown that heals the other guardian too, and an empower on the lowest-HP ally: damage reduction or reflection). Total damage reduction is capped at 75%. Healing items: more healing done, more healing received, anti-heal (cuts the target's healing). The **3rd (last) spot has two guardians**, each doing something different. Lane layout: **side lanes** = longer distance from the base to the 1st spot, spots closer to each other; **middle lane** = shorter distance to the 1st spot, more space between spots. Use this when laying out the MOBA on Nova Paulicéia. Code: AValdoriaGuardian (ValdoriaGuardian.h), /spot to test.
- **Bifrost (owner chose the Norse-mythology name; the look is ours: a simple light ribbon, no Marvel/Loki style)**: a calm light trail per lane from each base (vertical at the spawn, then horizontal over the lane), reaching the middle without minions and growing/shrinking with minion pushes (TODO with minions). Enemy trails have a red accent. NP lanes are named after Brazilian avenues (Ipiranga, Paulista, Rebouças). Mount: look at it + hold Space 0.4 s (generous hitbox). Ride lying down; the camera is free: looking does not change the way; to turn around (e.g. go home) look back AND press forward (W). Space again gets off. Basic attacks only, no abilities; falls off when hit by an enemy; locked for 2 s after **passive combat** (taking damage or control from an enemy). **General combat** = any attack/ability that hits enemies or allies (heals count). The trail is intangible to everything. CanRideAnyRail for a future surfer hero (allied and enemy trails). Enemy guardians bring riders down by style: melee leaps and knocks down, mage pulls and roots from the ground, flyer grabs and stuns, thrower throws something.
- **Modos de jogo**: vários, incluindo **MOBA 6v6 com 3 lanes** (a NP já tem 3 rotas). Plano: um
  GameMode por modo, com as regras do modo num arquivo de dados.
- **Corpo e locomoção**: heróis baixos, altos, largos; `CapsuleRadius`/`CapsuleHalfHeight` =
  hitbox, `MeshScale`, `HoverHeight` (flutua acima do chão, hitbox sobe junto), `Locomotion`
  (Andar / Flutuar sem passos / Andar sobre efeito mágico **ou tecnológico**, visual configurável).
  A câmera acompanha o tamanho.
- **Diversidade**: corpos variados (retos, curvilíneos, combinações), identidades cis e trans,
  todas as etnias, proporções naturais e **sem sexualização exagerada**. Design caso a caso.

## Pendências atuais (decisão do dono) — ver LOG.md (Turno 11)
- Animações Quaternius (CC0): convertidas para o Manny em `/Game/Valdoria/Animations/Retarget`
  por `Scripts/retarget_ual.py` (deslizar, subir/escalar, deitar = natação por enquanto; o dono
  quer manter a natação para uma futura área com água). Para converter mais, acrescente em
  `WANTED` no script e rode-o com o UnrealEditor-Cmd.
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
- Todo turno: atualize o `LOG.md` (estado atual + bloco do turno) e faça commit.
- Commits em português; não envie (`push`) sem o dono pedir.
