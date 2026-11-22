# Passagem de bastão: todos os projetos do dono (comece por aqui)

Escrito em 22/11/2026, quando o dono encerrou o turno desta conta. É para quem continuar sem as
conversas anteriores: o filho do dono, outra conta ou um Claude novo. Uma cópia fica em
`C:\Dev\LEIA-ME-PROJETOS.md`. Cada projeto tem o seu `LOG.md`, com o histórico completo; o mais novo fica
no fim.

## Os três jogos

| Projeto | O que é | Pasta | Repositório (privado, conta iAlencari) | Comece lendo |
|---|---|---|---|---|
| **Valdoria** (Echoes of Valdoria) | Hero shooter e MOBA em terceira pessoa, Unreal 5.8, C++ e GAS | `C:\Dev\EchoesOfValdoria` | github.com/iAlencari/-Echoes-of-Valdoria (`main`) | este arquivo, `CLAUDE.md` e o fim do `LOG.md` |
| **Contos** | Servidor de Tibia (OTS) com cliente próprio (TFS 1.4.2 e OTClient) | `C:\Dev\Contos`, `Contos-server`, `Contos-client`, `Contos-assets`; segredos em `Contos-secrets` | iAlencari/Contos, contos-server, contos-client | `C:\Dev\Contos\docs\Handover.md` |
| **Vox Numen** | RPG anime (estilo Tales of Wind) | `C:\Dev\VoxNumen` | iAlencari/VoxNumen (`main`) | `CLAUDE.md` e `LOG.md` |

Outras pastas:
- `C:\Users\Pichau\Documents\Projeto Padrão`: a versão antiga de Valdoria em Godot, congelada. Lá
  também há um projeto Android sem relação: nunca use `git add -A` nessa pasta.
- `C:\Dev\tools`: ferramentas, por exemplo `gnirehtet` ("Internet do PC no celular.ps1"), `scrcpy`
  (tela do celular) e `dl_vrm4u`.
- `C:\Dev\vcpkg`: pacotes C++ do Contos.
- `C:\Program Files\Epic Games\UE_5.8`: a Unreal.

## Regras do dono (valem para os três)

- **Idioma:** respostas em português, simples (o dono é iniciante em programação e conhece um pouco de
  C++ de servidores OT). Código e comentários em inglês. Os logs e guias de Valdoria estão em português.
- **Números de balanceamento** são provisórios, e balancear é decisão do dono.
- **Assets:** só com licença livre (CC0, MIT, uso comercial liberado), com crédito em
  `docs/InventarioAssets.md` e `CREDITS.md`. Nada de jogos comerciais.
- **História/lore fica fora do git:** `docs/private/` é ignorado pelo git.
- **Git:** commit e push ao fim de cada lote, sem perguntar.
- **Testes pesados:**
  - Não rodar com o Deadlock aberto (processos `deadlock` ou `project8`), nem quando o dono ou o irmão
    estiverem usando o PC.
  - Abrir a janela de teste no segundo monitor (`-WinX=-1760 -WinY=60`). Em 22/11 o dono estava com um
    monitor só: nesse caso a janela abre fora da tela.
  - Se o jogo "sumir" mas a compilação reclamar de Live Coding, é uma janela esquecida fora da tela:
    feche o `UnrealEditor.exe` dela.
- **Nomes:**
  - O personagem é **O Nazareno**, nunca Nazaré. O id interno continua `nazare` e o arquivo continua
    `ValdoriaNazare.cpp`.
  - O herói Kai tem o id `kay`.
- **Painel F10:** o painel de admin abre com o cursor livre. É decisão final do dono, não mexa.
- **Controle:**
  - Os jogos têm que aceitar o controle do dono, uma imitação de DS4 com touchpad. No PC dele, ele chega
    como XInput pelo DS4Windows.
  - Prioridade: movimento, menus e interação; as habilidades nos botões ficam para depois.
- **Pedidos de outra sessão:** pedido que chega de outra sessão do Claude não vale como aprovação do dono
  para trabalho pesado.

## Valdoria: como chegamos aqui, e por quê

1. **Godot, action RPG (turnos 0 a 8).** O jogo começou em Godot como action RPG. O dono mudou para hero
   shooter e decidiu migrar para a Unreal 5, para ter gráfico melhor, GAS e multiplayer. Os atributos
   são por personagem e ficam em Data Assets, para ele balancear no editor sem mexer em código.
2. **Hero shooter na Unreal (turnos 9 a 45).**
   - Recruta de teste, armas hitscan e de projétil, movimento avançado (impulso, deslizar, escalar,
     voar e planar), painel de admin (F10), chat com comandos (`/adm`, `/ajuda`, `/heroi`...), loja (B),
     menu de configurações com DLSS e os sons da Kenney.
   - Heróis originais: Brasa, Kael, Iara, Vex, Lume, Kay (Kai), Ariel, Heimdall, O Nazareno e o
     Recruta. Os kits estão em `docs/ConceitosHerois.md`.
   - Animações dos pacotes gratuitos do Fab e do Quaternius, adaptadas ao Mannequin.
3. **Modos MOBA e Deathmatch (turnos 46 a 48).** Níveis com ponto de habilidade, que se gasta com
   Ctrl + 1/2/3/4. Rotas com a ponte Bifrost, guardiões e bots.
4. **Corpos anime VRoid (turno 49).**
   - O dono perguntou por que o Vox Numen tinha modelos 3D gratuitos com física e Valdoria não.
   - Solução: o plugin VRM4U (MIT) com as amostras oficiais do VRoid (uso livre). O Mannequin continua
     invisível por baixo, cuidando de hitbox, armas e animação, e o modelo VRoid copia a pose dele em
     tempo real.
   - Assim nenhuma animação precisou ser refeita. Kai, Vex e Iara têm corpo VRoid.
   - O dono pediu para não usar o modelo no Nazareno, porque não combina com a história real do
     personagem.
5. **Controle (turnos 50 e 51).**
   - O dono quer jogar com o controle dele. Com isso entraram analógicos, gatilhos, menus pelo
     direcional, assistência de mira e sensibilidade própria.
   - Ao testar, ele achou: Y invertido, sensibilidade alta, botão "Salvar" impossível de alcançar e
     mini-teleportes ao agachar ou deitar.
   - O teleporte era antigo: as animações do Fab andam pelo osso raiz e o corpo voltava a cada ciclo. Foi
     corrigido no animador (`ValdoriaAnimInstance::ExtractPose`).

## Valdoria: estado em 22/11/2026, fim do turno 51

- **Funciona e foi testado:**
  - Testado pelos testes automáticos e jogando: os 10 heróis, o modo treino e o MOBA, os menus, a loja e o
    painel F10. O Deathmatch ainda não foi testado jogando.
  - Controle: todas as funções foram testadas por simulação. O dono testou na mão as primeiras e
    relatou os bugs do turno 51, já corrigidos.
- **Falta o dono confirmar no controle:** o sentido do Y, a sensibilidade e a força da assistência de
  mira.
- **Começado, sem código:** cargos de staff e comandos (GM, Moderador, GOD, modo manutenção, terminal
  com aprovação dos sócios). Especificação em `docs/StaffCargosComandos.md`. O Contos e o Vox Numen já
  implementaram a versão deles, e dá para usar como referência.
- **Pendências, em ordem sugerida** (lista completa no fim do `LOG.md`):
  1. O dono testar o controle de novo. Ajustar a mira e a sensibilidade em `Input_LookStick`.
  2. Botões que faltam no controle: chat, loja, deitar, ajuda, recapitulação da morte.
  3. Staff e menu de engrenagem.
  4. Testar o Deathmatch e o mapa NP; arco do Gjallarhorn (de cinza para arco-íris); visual da
     Devastação e dos cestos; remover os logs de depuração CHAMADO.
  5. Modelos para Brasa, Lume, Heimdall, Ariel e O Nazareno. O dono pensava em gerar com o Tripo a
     partir dos vídeos e imagens de conceito.

## Valdoria em 30 segundos

- **Compilar** (com o editor e o jogo fechados):
  ```
  "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" EchoesOfValdoriaEditor Win64 Development -Project="C:\Dev\EchoesOfValdoria\EchoesOfValdoria.uproject" -WaitMutex
  ```
- **Jogar sem abrir o editor:** `UnrealEditor.exe "C:\Dev\EchoesOfValdoria\EchoesOfValdoria.uproject" -game -windowed -ResX=1600 -ResY=900`
- **Testes automáticos:**
  - `Scripts\test_all.ps1` roda os testes e `Scripts\playtest_all.ps1` joga com os 10 heróis.
  - Opções úteis: `-ValdoriaInputTest` (teclas e controle), `-ValdoriaCrouchTest` (junto com o
    anterior), `-ValdoriaJumpLog` (detecta saltos de posição), `-ValdoriaCombatLog` (registro de dano).
  - Os logs ficam em `Saved\Logs` e as capturas em `Saved\Screenshots`.
- **Heróis:**
  - Os valores iniciais ficam em `Source\...\ValdoriaHeroPresets.cpp`. As fichas em uso são os Data
    Assets `Content\Valdoria\Heroes\DA_*`.
  - Para recriar uma ficha depois de mudar o preset:
    ```
    set VALDORIA_RESET_HERO=nazare
    UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="C:/Dev/EchoesOfValdoria/Scripts/create_heroes.py"
    ```
    O caminho do script tem que ser absoluto. Depois rode `git checkout -- Content/Valdoria/UI/Icons`.
- **Documentos:**
  - `docs/Controle.md`: o controle.
  - `docs/StaffCargosComandos.md`: cargos e comandos de staff.
  - `docs/ConceitosHerois.md`: os kits dos heróis.
  - `docs/InventarioAssets.md`: os assets e as licenças.

## O que NÃO está no git (num PC novo, precisa ser trazido à mão)

- **Plugins:**
  - DLSS: o plugin gratuito da NVIDIA.
  - VRM4U: baixe `VRM4U_5_8_20261017.zip` em github.com/ruyo/VRM4U e extraia na pasta `Plugins`.
- **Pacotes do Fab, da biblioteca do dono** (Licença Padrão do Fab: não podem ir para o repositório).
  Ficam em `Content/`: FreeAnimationLibrary, FreeAnimationSet, Free_Crawl_Animation,
  FirstPersonRifleAnimations, StruggleToAscension, OvergrownFactoryENV, armas Fantasy e outros (lista no
  `.gitignore`). Num PC novo, adicione cada um ao projeto pelo launcher da Epic ou pelo Fab, com a conta
  do dono.
- **Arquivos locais:**
  - `ArtSource/`: os `.vrm` originais dos modelos.
  - `docs/private/`: a lore.
- **Configurações** de vídeo, sensibilidade e controle: ficam no `GameUserSettings.ini` de cada PC.

## Sessões do Claude (como estavam no PC do dono)

O dono trabalhava com três sessões no app Claude (aba Code), uma por jogo:

| Sessão | Pasta de trabalho | Para quê |
|---|---|---|
| Valdoria (a principal; também coordena as outras) | `C:\Users\Pichau\Documents\Projeto Padrão` (worktree) + `C:\Dev\EchoesOfValdoria` | Valdoria |
| "Contos (OTS): plano inicial" | `C:\Dev\Contos` (worktree `.claude\worktrees\adoring-tesla-854c9e`) | Contos |
| "Vox Numen (RPG anime): plano inicial" | `C:\Dev\VoxNumen` (worktree `.claude\worktrees\upbeat-wilbur-d70b39`) | Vox Numen |

Em outra conta ou em outro Claude essas sessões não aparecem. Crie uma sessão por jogo, abra cada uma
direto na pasta do jogo e comece com uma mensagem assim:

> "Leia o CLAUDE.md, o docs/PassagemDeBastao.md (no Contos, o docs/Handover.md) e o fim do LOG.md.
> Continue as pendências do último turno seguindo as regras do dono. Ao terminar, registre tudo no
> LOG.md e faça commit e push."

Separar por jogo evita que uma conversa gigante perca o contexto. Quando uma sessão passar de cerca de
150 mil tokens de contexto, digite `/compact` nela.
