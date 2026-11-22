# CLAUDE.md — Echoes of Valdoria (Unreal Engine 5)

Hero shooter em **Unreal Engine 5.8 + C++**, com o Gameplay Ability System (GAS). O dono do
projeto é iniciante em programação (conhece um pouco de C++ de servidores OT): explique em
português simples. **Leia `LOG.md` inteiro antes de mexer.**

> **Chegou agora (ex.: o filho do dono)?** Comece por `docs/PassagemDeBastao.md`: pastas dos três
> jogos, sessões, regras e o que ficou começado.

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
- O herói é **O Nazareno** (nunca "Nazaré"); id interno `nazare`. Kai tem id `kay`.
- O jogo tem que funcionar com o **controle** do dono (DS4 genérico com touchpad): veja `docs/Controle.md`.
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
- **Heróis prontos**: Brasa, Kael Rook, Iara, Vex, Lume, Kay e Ariel (2 formas) em `Content/Valdoria/Heroes/DA_*.uasset`
  (criados por `Scripts/create_heroes.py` a partir de `ValdoriaHeroPresets.cpp`; o script **não**
  sobrescreve DA_ existentes, a não ser com `VALDORIA_RESET_HEROES=1`). Balancear = editar o DA_.
- **Teclas padrão para TODOS os heróis (dono, turno 45, sem exceção)**: botão ESQUERDO = ataque básico,
  botão DIREITO = habilidade 5, teclas **1, 2, 3** = habilidades 1, 2, 3, tecla **4** = suprema, passiva sempre
  ativa. Habilidade extra opcional no **C** (preferência; X, Q ou E possíveis). X deita, K mostra as habilidades.
  (O esquema antigo F/E/Q e a escolha "Teclas das habilidades" foram removidos.)
- **HUD (turno 45)**: na tela só 1, 2, 3 e 4; o C (se houver) menor, à esquerda. Passiva e habilidade do botão
  direito: ícones pequenos e transparentes com recarga em relógio + número. O resto fica na aba K.
- **Acúmulos (stacks, turno 45)**: `SetStack`/`AddStack` no herói que RECEBE, guardando quem causou. Seus
  acúmulos: ícone pequeno acima das habilidades; os que um inimigo pôs em você: menor, com marca vermelha;
  os que você pôs em outro herói: ícone ao lado da barra de vida dele, visível só para você. Em uso: Foco
  Crescente da Lume (no alvo, 1-5) e anti-cura da loja (AC, % de cura cortada).
- **Kay [Agent Kay] (herói do dono)**: refira-se SÓ pelo nome (nunca "o"/"a"). Identidade não binária, sem arma,
  controla fogo, terra, água e ar (4 conjuntos de habilidades, `ElementSet` em cada entrada). Toque curto no 4 =
  troca de elemento (recarga 20 s); segurar o 4 por 3 s com a suprema cheia = **Devastação**; soltar antes = cancela
  e dá 3 s de recarga. **As habilidades 1/2/3, o botão direito e a passiva são do dono**: não invente.
- **Ariel (criado pelo Claude, aprovado como ideia)**: bigênero; refira-se pelo nome ou com "de Ariel". C troca de
  forma (Forma Rocha, masculina, corpo a corpo, `DA_Ariel`; Forma Faísca, feminina, à distância, `DA_ArielFaisca`),
  mantendo vida (%), suprema e recargas (`AlternateFormId` + `UValdoriaGA_FormSwap`).
- **Deitar (X) é por herói**: tabela `CAN_PRONE` em `Scripts/create_heroes.py` (ou `VALDORIA_NO_PRONE=lume,vex` /
  `VALDORIA_PRONE=...`), aplicada a cada execução do script. Lume começa sem deitar (flutua).
- **Vigor**: só a Lume por enquanto (`bUsesVigor`); Shift corre e dá impulso gastando vigor.
- **Lobby (turno 45)**: abre uma vez ao iniciar o jogo (nunca em testes: qualquer `-Valdoria...` pula; também
  `-ValdoriaNoLobby`) e pelo menu de pausa. Abas: Jogar (modos), Heróis, Loja (visuais "em breve"),
  Inventário (vazio), Treino, Configurações, Sair. Código: `ValdoriaLobby.cpp`.
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
- **MOBA spots (owner's decision, 2026-11-18)**: instead of towers, each lane has 3 **spots** = guardian monsters/bosses that move inside their area and focus the enemy hero who attacks an allied hero there. Each spot gives its team a different buff while alive. Spot 1: 50% damage reduction during the first minute, fading to 0% at minute 5 (configurable). Spots 2 and 3: buffs to be decided by the owner (proposed: spot 2 +20% damage fading, spot 3 regeneration). Name: **Guardian Spots**. Comeback rules: if a team's spot 1 falls before minute 5, that team keeps the spot-1 buff doubled and constant until minute 5; spot 2 buff fades from full to 0 between minutes 5 and 10; if spots 1 and 2 of a lane fall before minute 5, that lane's last spot buffs triple until minute 10. One of the two last-spot guardians is a support mage (area heal on cooldown that heals the other guardian too, and an empower on the lowest-HP ally: damage reduction or reflection). Total damage reduction is capped at 75%. The triple bonus only counts when spots 1 and 2 fall in the **same lane** (confirmed). **Final guardian (GF)** in each base behind the spawn, very high damage; it inherits health and damage (later abilities/passives) from every linked spot guardian of its team and loses each share when that spot falls; bringing it down wins. **Sealing ritual (lore)**: a defeated spot guardian becomes semi-sealed (no health, helpless). An enemy out of passive combat holds the seal (open hand, Latin incantation, subtitles for now) and it is sealed for good (seal mark, rage roar across the map). Its team can reverse it with the reverse incantation (7-10 s): it returns with little health and regenerates to 50% out of combat (+1% per minion killed, TODO); a second defeat makes it fade away (roar of pain). Roars/shockwaves are cosmetic. Semi-sealed: immune (hits show IMMUNE), ignored by minions/summons/bots' attacks; 30 s timeout -> comes back by itself with 50% health, no further regen, 2.5 s immune recovery; several heroes channel together faster; damage cancels. Visual: ~30% protection sphere (enemy view amber -> bright red over time, ally view blue -> bright green), energy line on the ground from the final guardian through every spot of the lane up to the semi-sealed one, with pulses. Bifrost pose: not lying; flying pose leaning forward looking slightly down, arms back, weapon on the back, knees slightly bent; quick weapon draw when getting off (per hero: summon, telekinesis floating weapon, melee guard, charged attack while landing). Healing items: more healing done, more healing received, anti-heal (cuts the target's healing). The **3rd (last) spot has two guardians**, each doing something different. Lane layout: **side lanes** = longer distance from the base to the 1st spot, spots closer to each other; **middle lane** = shorter distance to the 1st spot, more space between spots. Use this when laying out the MOBA on Nova Paulicéia. Code: AValdoriaGuardian (ValdoriaGuardian.h), /spot to test.
- **Bifrost (owner chose the Norse-mythology name; turn 46 look = owner concept image SourceArt/Concepts/Bifrost_Conceito.png: wide bundle of loose rainbow strands, smooth curves, no 90-degree corners, round pad at the base, ring portal at the end, riders float inside)**: a calm light trail per lane from each base (vertical at the spawn, then horizontal over the lane), reaching the middle without minions and growing/shrinking with minion pushes (TODO with minions). Enemy trails have a red accent. NP lanes are named after Brazilian avenues (Ipiranga, Paulista, Rebouças). Mount: look at it + hold Space 0.4 s (generous hitbox). Ride lying down; the camera is free: looking does not change the way; to turn around (e.g. go home) look back AND press forward (W). Space again gets off. Basic attacks only, no abilities; falls off when hit by an enemy; locked for 2 s after **passive combat** (taking damage or control from an enemy). **General combat** = any attack/ability that hits enemies or allies (heals count). The trail is intangible to everything. CanRideAnyRail for a future surfer hero (allied and enemy trails). Enemy guardians bring riders down by style: melee leaps and knocks down, mage pulls and roots from the ground, flyer grabs and stuns, thrower throws something.
- **MOBA standard rules (owner, turn 44: "every MOBA works like that")**: the final guardian is immune until the last spot of at least one lane is sealed or gone (`bGateUntilLaneBroken`); defeating, sealing and reversing spots give shop credits to the team (`Spot|Rewards`). Follow common MOBA conventions unless the owner says otherwise.
- **Vex (turn 44)**: telekinetic flanker. Daggers float on her back; primary throws one and recalls the previous (no ammo); RMB piercing telekinetic line; F partial stealth; E telekinetic dash to the aimed enemy (push + damage + brief stagger) or ally (small round shield behind her until she arrives); ultimate: up to 5 daggers at once; unlimited flight but no attacks/abilities while flying (arms down, levitating); passive: silent movement because she walks on small telekinetic floors she creates under her feet.
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

## Turn 46 rules (owner)
- Commit and push every batch without asking. Story/lore stays OUT of git (docs/private/, git-ignored).
- HP x10 via ValdoriaStatScale (data keeps design units). Levels/XP/respawn formula/CDR caps: docs/RegrasCombate.md.
- Keys: V = punch (every hero), F5 = camera, Tab = scoreboard, Y = death recap, Ctrl+1..4 = spend level point (MOBA).
- Minimap top-left with team vision (UValdoriaVisionSubsystem). Admin panel 100% opaque.

## Name (owner, 19/11/2026)
For now the game is called just **Valdoria** (repo and folder names stay). Side projects: `C:\Dev\Contos` (OTS) and `C:\Dev\VoxNumen` (anime RPG), each with its own private repo.

## Admin panel (owner, 19/11/2026, final - do not change)
F10 opens/closes the admin panel over the screen with the mouse cursor free (clickable at will). No Alt needed.
