# Passagem de bastão: todos os projetos do dono (comece por aqui)

Escrito em 22/11/2026 para quem continuar sem as conversas anteriores (o filho do dono, ou um Claude
novo). Uma cópia fica em `C:\Dev\LEIA-ME-PROJETOS.md`.

## Os três jogos

| Projeto | O que é | Pasta | Repositório (privado, conta iAlencari) | Comece lendo |
|---|---|---|---|---|
| **Valdoria** (Echoes of Valdoria) | Hero shooter e MOBA em terceira pessoa, Unreal 5.8, C++ e GAS | `C:\Dev\EchoesOfValdoria` | github.com/iAlencari/-Echoes-of-Valdoria (`main`) | `CLAUDE.md`, depois o fim do `LOG.md` |
| **Contos** | Servidor de Tibia (OTS) com cliente próprio (TFS 1.4.2 e OTClient) | `C:\Dev\Contos`, `Contos-server`, `Contos-client`, `Contos-assets`; segredos em `Contos-secrets` | iAlencari/Contos, contos-server, contos-client | `C:\Dev\Contos\docs\Handover.md` |
| **Vox Numen** | RPG anime (estilo Tales of Wind) | `C:\Dev\VoxNumen` (o trabalho mais novo pode estar no ramo `claude/upbeat-wilbur-d70b39`, na pasta `.claude\worktrees\upbeat-wilbur-d70b39`) | iAlencari/VoxNumen | `CLAUDE.md` e `LOG.md` |

Outras pastas:
- `C:\Users\Pichau\Documents\Projeto Padrão`: a versão antiga de Valdoria em Godot, congelada. Lá
  também há um projeto Android sem relação: nunca use `git add -A` nessa pasta.
- `C:\Dev\tools`: ferramentas, por exemplo `gnirehtet` ("Internet do PC no celular.ps1"), `scrcpy`
  (tela do celular) e `dl_vrm4u`.
- `C:\Dev\vcpkg`: pacotes C++ do Contos.
- `C:\Program Files\Epic Games\UE_5.8`: a Unreal.

## Regras do dono (valem para os três)

- **Idioma:** respostas em português; código e comentários em inglês. Os logs e guias de Valdoria estão
  em português.
- **Números de balanceamento** são provisórios.
- **Assets:** só com licença livre (CC0, MIT, uso comercial liberado), com crédito em
  `docs/InventarioAssets.md` e `CREDITS.md`.
- **História/lore fica fora do git:** `docs/private/` é ignorado pelo git.
- **Git:** commit e push ao fim de cada lote, sem perguntar.
- **Testes pesados:**
  - Não rodar com o Deadlock aberto (processos `deadlock` ou `project8`), nem quando o dono ou o irmão
    estiverem usando o PC.
  - Abrir a janela de teste no segundo monitor (`-WinX=-1760 -WinY=60`). Em 22/11 o dono estava com um
    monitor só e ia instalar o segundo.
- **Nomes:**
  - O personagem é **O Nazareno**, não Nazaré. O id interno continua `nazare` e o arquivo continua
    `ValdoriaNazare.cpp`.
  - O herói Kai tem o id `kay`.
- **Painel F10:** o painel de admin abre com o cursor livre. É decisão final do dono, não mexa.
- **Controle:** os jogos têm que aceitar o controle do dono, uma imitação de DS4 com touchpad. Prioridade:
  movimento, menus e interação; as habilidades nos botões ficam para depois.

## Valdoria em 30 segundos

- **Compilar** (com o editor fechado):
  ```
  "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" EchoesOfValdoriaEditor Win64 Development -Project="C:\Dev\EchoesOfValdoria\EchoesOfValdoria.uproject" -WaitMutex
  ```
- **Jogar sem abrir o editor:** `UnrealEditor.exe "C:\Dev\EchoesOfValdoria\EchoesOfValdoria.uproject" -game -windowed -ResX=1600 -ResY=900`
- **Testes automáticos:**
  - `Scripts\test_all.ps1` roda os testes (`-ValdoriaInputTest`, `-ValdoriaAdminTest` etc.).
  - `Scripts\playtest_all.ps1` joga com os 10 heróis.
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
- **Plugins fora do git** (precisam ser instalados à mão num PC novo):
  - DLSS.
  - VRM4U (modelos VRoid): baixe `VRM4U_5_8_20261017.zip` em github.com/ruyo/VRM4U e extraia na pasta
    `Plugins`.
  - Os `.vrm` originais ficam em `ArtSource/VRoid`, também fora do git.
- **Documentos:**
  - `docs/Controle.md`: o controle.
  - `docs/StaffCargosComandos.md`: cargos e comandos de staff, especificação comum aos três jogos.
  - `docs/ConceitosHerois.md`: os kits dos heróis.
  - `docs/InventarioAssets.md`: os assets e as licenças.

## O que mudou e o que ficou começado (21 e 22/11/2026)

**Valdoria:**
- **Feito:**
  - Corpos VRoid (anime) em Kai, Vex e Iara, com as armas seguindo as mãos.
  - "Nazaré" renomeado para **O Nazareno**.
  - Suporte a controle: andar, câmera, botões e menus pelo direcional, com X para confirmar e O para
    voltar. Testado por simulação no `-ValdoriaInputTest`; ainda não testado com o controle de
    verdade na mão.
- **Começado, sem código ainda:** cargos de staff e comandos (GM, Moderador, GOD, modo manutenção,
  terminal com aprovação dos sócios). A especificação está em `docs/StaffCargosComandos.md` e no fim do
  `LOG.md`.
- **Pendências:** estão no fim do `LOG.md` (turno 50).

**Contos e Vox Numen:**
- Cada um tem a sua sessão do Claude, que atualiza o próprio `LOG.md`.
- O pedido de 22/11 foi o mesmo para todos: retomar o que parou por limite de uso, ligar o controle e
  registrar tudo.

## Sessões do Claude (como estavam no PC do dono)

O dono trabalhava com três sessões no app Claude (aba Code), uma por jogo:

| Sessão | Pasta de trabalho | Para quê |
|---|---|---|
| Valdoria (a principal; também coordena as outras) | `C:\Users\Pichau\Documents\Projeto Padrão` (worktree) + `C:\Dev\EchoesOfValdoria` | Valdoria |
| "Contos (OTS): plano inicial" | `C:\Dev\Contos` (worktree `.claude\worktrees\adoring-tesla-854c9e`) | Contos |
| "Vox Numen (RPG anime): plano inicial" | `C:\Dev\VoxNumen` (worktree `.claude\worktrees\upbeat-wilbur-d70b39`) | Vox Numen |

Se você estiver em outro Claude e não enxergar essas sessões, crie uma sessão por jogo. Abra cada uma
direto na pasta do jogo e comece com uma mensagem assim:

> "Leia o CLAUDE.md, o LOG.md (do fim para o começo) e o docs/PassagemDeBastao.md (ou o
> docs/Handover.md no Contos). Continue as pendências do último turno seguindo as regras do dono.
> Ao terminar, registre tudo no LOG.md e faça commit e push."

Separar por jogo evita que uma conversa gigante perca o contexto. Quando uma sessão passar de cerca de
150 mil tokens de contexto, digite `/compact` nela.
