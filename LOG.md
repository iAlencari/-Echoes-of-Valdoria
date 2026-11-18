# LOG — Echoes of Valdoria (Unreal Engine 5)

> Documento de continuidade entre agentes (Claude Code e OpenCode). **Só acrescente**, nunca
> apague histórico. Atualize a seção "Estado atual" a cada turno e registre um bloco novo no fim.
> O histórico da fase Godot (action RPG, turnos 0 a 6) está em
> `C:\Users\Pichau\Documents\Projeto Padrão\EcosDeValdoria\log.md`.

## Estado atual (18/11/2026)

**Gênero:** hero shooter em terceira pessoa (câmera por cima do ombro). Multiplayer previsto
(código já usa replicação da Unreal, servidor autoritativo); por enquanto se testa sozinho
contra bonecos de treino.

**Funciona e foi testado (teste automático + print):**
- Projeto UE 5.8.4 C++ em `C:\Dev\EchoesOfValdoria`, criado do modelo Third Person, compilando.
- Herói definido por Data Asset (`UValdoriaHeroDefinition`). Sem Data Asset, entra o
  "Recruta" de teste criado em código.
- Vida, armadura e escudo (GAS). Dano: escudo absorve primeiro, armadura reduz 30% de cada
  acerto até acabar, depois vida. Escudo regenera após 3 s sem dano.
- Arma hitscan (rifle automático do Recruta): cadência, pente, recarga (R), dispersão, queda
  de dano por distância, acerto na cabeça x2.
- Habilidades GAS: pulso de cura (E), onda de choque como ultimate (Q). Carga de ultimate por
  dano causado e por tempo. Impulso (`UValdoriaGA_Dash`) existe como habilidade para heróis
  que o queiram numa tecla de habilidade (F = Habilidade 1, livre no Recruta).
- Movimento por personagem (`FValdoriaMovementRules`, dentro do Data Asset):
  - **Shift**: modo 0 nada, 1 correr (segurar), 2 impulso, 3 segurar = correr e dois toques =
    impulso. Cargas de impulso de 1 a 5, tempo de recarga por carga, força e janela do toque duplo.
  - **Ctrl** agachar (segurar); Ctrl correndo = **deslizar**.
  - **X** deitar (alterna).
  - **Espaço**: pulo, pulo duplo (até 5 pulos), **subir em beirada** até a altura máxima do
    herói, e **escalar parede** segurando Espaço (velocidade e tempo máximo por herói).
- 4 bonecos de treino na frente do ponto de nascimento: básico, com armadura, com escudo e um
  que anda de lado. Voltam à vida cheia 3 s depois de pararem de levar dano; renascem 3 s após morrer.
- HUD em código: mira com marcador de acerto (vermelho na cabeça), números de dano, vida/
  armadura/escudo, munição, habilidades com recarga, ultimate, cargas de impulso, estado do
  movimento, FPS / ms / ping (ping mostra "local" sem servidor).
- O mapa `Lvl_ThirdPerson` usa o `ValdoriaGameMode` (ajustado por `Scripts/set_gamemode.py`).
- Desempenho no PC do dono: ~197 FPS no mapa de teste (RTX 5070), Lumen e ray tracing ativos
  (configuração padrão do modelo).

**Ainda nao feito / limitacoes conhecidas:**
- Deitar/rastejar e escalar parede: pacotes CC0 do Quaternius (Universal Animation Library 1 e 2) aprovados pelo dono e importados em /Game/Valdoria/Animations/UAL1 e UAL2 (86 animacoes). Conversao para o esqueleto do Mannequin em andamento via IK Retargeter (IK rigs e retargeters criados com sucesso; passo final de batch retarget em depuracao). Fallback sem download desenhado (pose deitado + sway procedural / zipline loop) caso a conversao falhe.
- Agachar, deslizar e escalar ainda sem animacao propria ate o retarget ser concluido (a malha so abaixa/sobe). Efeitos visuais sao linhas de depuracao (so aparecem em builds de desenvolvimento).
- Subir em beirada e escalar parede so foram testados por codigo, nao a mao.
- Sem Data Assets de herois reais ainda; sem menu de admin, menu grafico, DLSS, selecao de heroi.
- HP do jogador e dos inimigos (bonecos de treino) ainda com barra simples de depuracao; o dono pediu barra mais bonita e HP do inimigo tambem (imagens de referencia nao puderam ser lidas por este agente - descrever em texto).
- Ataques variados: hitscan nao sera o unico tipo. Havera habilidades no estilo de Wuyang e Moira (Overwatch), alem de armas com recuo e spread. Essas configuracoes devem ficar no script (Data Asset) de cada personagem (FValdoriaWeaponStats / habilidades GAS).
- Modelos 3D das armas e imagens de conceito (Gemini) ainda nao integrados; o dono tem um pacote de armas FBX/GLB pronto para exportar.
- Sem menu de admin, menu grafico, DLSS, selecao de heroi (pendente).
## Proximos passos sugeridos
1. Dono testar no editor (Play) e ajustar sensacao de movimento.
2. Finalizar o retarget das animacoes do Quaternius (LayToIdle, ClimbUp_1m, Swim_Fwd_Loop, Slide_*) e aplicar a logica de tocar/reter no UpdateMovementAnimation() (pose deitado ao entrar, anim de levantar ao sair, loop de rastejar com sway, loop de escalada). Testar com -ValdoriaAutoTest + prints.
3. Barra de HP do jogador e dos inimigos (bonecos de treino) mais bonita (aguardando descricao/imagens do dono; este agente nao consegue ler imagens).
4. Menu de administrador (trocar heroi, imortal, sem recarga, dano, teleporte, editar alvo mirado).
5. Menu de configuracoes graficas + plugin DLSS da NVIDIA (download precisa de aprovacao do dono).
6. Primeiros herois reais (1 por funcao) como Data Assets, com a aprovacao do dono; ataques variados (Wuyang/Moira-like, recuo, spread) configurados por personagem.
7. Integrar modelos 3D das armas (FBX/GLB do dono) e imagens de conceito.
8. Efeitos visuais de verdade (assets livres aprovados).
## Controles atuais
WASD mover · Mouse mirar · Botão esquerdo atirar · Botão direito secundário · R recarregar ·
Shift correr/impulso · Ctrl agachar (correndo = deslizar) · X deitar · Espaço pular/subir/escalar ·
F habilidade 1 · E habilidade 2 · Q ultimate

---

## Histórico de turnos

### Turno 7 — Claude Code — 18/11/2026 (início do projeto Unreal)
**Falas do dono:**
> "pronto" (Unreal instalada) · "fechei" (editor fechado para mover a pasta)
>
> "gostei falta agachar no ctrl, correr no shift, escalar paredes com altura suficiente pra escalar usando o espaço, deitar no x, dentre outras funçoes, nem todos os personagens vao usar shift e ter a mesma altura para escalar, alguns nao vao deitar e outros vão escalar paredes então é bom deixar essas funções prontas e depois eu ajeito por personagem o que cada pode fazer no script base"
>
> "na verdade deixe assim 0 para nenhum 1 para correr, 2 para impulso e 3 para segurar shift correr e shift 2x impulso(quantidades de até 5 a serem estabelecidades pelo script por personagem)"

- **O que mudou**
  - Projeto criado pelo dono em `Projeto Padrão\EchoesOfValdoria`; a compilação falhou por causa
    do "ã" no caminho (o MSVC não abre o `.pch`). Movido para `C:\Dev\EchoesOfValdoria`.
  - Código base do hero shooter em `Source/EchoesOfValdoria/Valdoria/` (ver tabela no `CLAUDE.md`).
  - Plugin GameplayAbilities ligado; módulos GameplayAbilities/GameplayTags/GameplayTasks/NetCore.
  - `DefaultEngine.ini`: modo de jogo global = `ValdoriaGameMode`. Mapa de teste também ajustado
    (`Scripts/set_gamemode.py`), porque o mapa do modelo forçava o modo de jogo Blueprint.
  - Movimento avançado configurável por herói (modo do Shift 0–3, cargas de impulso 1–5,
    agachar, deslizar, deitar, pulo múltiplo, subir em beirada, escalar parede).
  - Teste automático `-ValdoriaAutoTest`.
  - Git iniciado com LFS, baseado no repositório do GitHub (que só tinha `LICENSE`, GPL-3.0).
- **Bugs achados e corrigidos no caminho:** habilidades não eram concedidas quando o jogador era
  possuído antes do `BeginPlay` (concessão agora também no fim de `ApplyHeroDefinition`);
  parâmetro `Instigator` sombreava membro (erro C4458).
- **Testes:** `Build.bat` OK. Teste automático: 12 tiros → boneco 250 → 30; correr 550 → 798;
  deitar OK (cápsula 42); impulso consome carga; ultimate mata o boneco; print em
  `Saved/Screenshots/AutoTest.png`; ~197 FPS.
- **Pendente / atenção:** o `LICENSE` do GitHub é **GPL-3.0**. Código GPL não combina bem com a
  licença da Unreal na hora de distribuir o jogo; vale o dono decidir a licença (ex.: MIT para o
  código próprio). Nada foi enviado ao GitHub ainda.

### Turno 8 — Claude Code — 18/11/2026
**Fala do dono:** > "sim pode trocar ja aceitei o uac coloca um atalho pra eu testar o projeto na area de trabalho"
- Licença trocada de GPL-3.0 para **MIT** (código original do projeto), com nota de que a Unreal e o conteúdo da Epic seguem o EULA da Epic.
- Atalhos na Área de Trabalho: **Echoes of Valdoria - Jogar** (abre direto no jogo, janela 1600x900) e **Echoes of Valdoria - Editor** (abre o projeto no editor). Testado: o jogo abre com o ValdoriaGameMode.
- Ainda não enviado ao GitHub: aguardando confirmação do dono.

### Turno 9 — Claude Code — 18/11/2026
**Fala do dono:** > "abre no segundo monitor por favor? adicionar um menu no esc ou alt pra eu poder tirar o cursor de dedntro e arrastar a janela"
- Atalho **Jogar** agora abre no monitor 2 (que fica à esquerda: X=-1920), janela 1600x900 em -WinX=-1760 -WinY=60.
- **Esc** abre/fecha o menu de pausa (Slate em C++, sem assets): Continuar, Reiniciar treino, Sair do jogo. O jogo pausa e o cursor aparece.
- **Segurar Alt** solta o cursor (para arrastar a janela); soltar o Alt volta a prender. Com o cursor solto, cliques e movimento do mouse não atiram nem giram a câmera.
- Módulo SlateCore adicionado. Teste automático agora também abre o menu de pausa (passou: menu aberto=1, jogo pausado=1).
- Obs.: no editor (Play), o Esc encerra a sessão por padrão da Unreal; o menu de pausa vale no jogo aberto pelo atalho. No editor, Shift+F1 solta o mouse.

### Turno 10 — Claude Code — 18/11/2026 (animações de movimento)
**Fala do dono:** > "notei que não tem animação de agachar ou deitar o boneco fica parado e a camera abaixa escalar também não tem animação"
- Novo **FValdoriaAnimationSet** no Data Asset do herói (Visual > Animations): agachar parado e andando nas 4 direções, deslizar, deitar parado/rastejando, subir em beirada, escalar parede, impulso nas 4 direções. Tocam por cima do Animation Blueprint no slot DefaultSlot, sem root motion; servem para qualquer malha no esqueleto padrão UE5 Mannequin (os heróis de IA devem usar esse esqueleto).
- Animações gratuitas da Epic ligadas pelo plugin **MoverExamples** (vem com a Unreal): agachar (4 direções), deslizar (entrada do agachar), subir em beirada (VaultOver), impulso (4 direções).
- Scripts/setup_animations.py: marca o esqueleto do plugin como compatível com o SK_Mannequin do projeto (alterou o asset do esqueleto).
- **Deitar/rastejar e escalar parede seguem sem animação**: a Epic não tem gratuitas. Proposto ao dono o pacote CC0 Quaternius Universal Animation Library 1 e 2; aguardando aprovação para baixar.
- Teste automático: agachar toca MM_Unarmed_Crouch_Walk_Fwd; print em Saved/Screenshots/AutoTest_Agachado.png.

### Turno 11 - OpenCode/Beam - 18/11/2026 (anotacao de escopo, sem codigo)
**Falas do dono:**
> "sim ignore, voce consegue arrumar a animacao de deitar e escalar? as imagens sao conceitos para
> criar o modelo 3D se quiser pode olhar elas e aqui ja temos o modelo 3d das armas" (imagem anexada)
> "eu baixei os 2 universal packs do quaternius estao na pasta"
> "falta esses menu de admin, menu grafico + DLSS, selecao de heroi, herois reais em Data Assets,
> se conseguir arrumar e uma boa, sobre o hitscan nao sera o unico tipo teremos ataques variados como
> os personagen wuiang de ow(exemplo) e moira(exemplo) alem de alguns ter recuo e spread, essas
> configuracoes vao ficar no script de cada personagem de preferencia"
> "falta a barra de hp mais bonita e hp do inimigo tambem, vou mandar imagens pra referencia,
> no momento nao faca nada so leia o projeto e me diga o que voce entendeu"
> "nao no momento so anota nos logs e claude md o que eu e o chat aqui falamos/fizemos pro claude
> ler depois"

- **O que foi feito (sem alterar codigo do jogo):**
  - Aprovado o uso dos packs CC0 da Quaternius (Universal Animation Library 1 e 2). Ambos baixados
    pelo dono em "Projeto Padrao/Universal Animation Library[Standard]" e "... 2[Standard]".
  - Importados no Unreal em /Game/Valdoria/Animations/UAL1 e UAL2 (86 animacoes, 96 arquivos,
    ~37 MB). Criados IK Rigs (IK_UAL1, IK_UAL2) e retargeters (RTG_UAL1, RTG_UAL2) com chains
    geradas automaticamente (Spine, Neck, Head, LeftArm, RightArm, LeftLeg, RightLeg, dedos).
    Passo final de batch retarget em depuracao (Scripts/retarget_ual.py).
  - Fallback sem download desenhado: pose deitado (anim de morte segurada + sway procedural) e
    MM_Zipline_Loop para escalar. A ser usado se o retarget falhar.
  - Lido o projeto inteiro (codigo, HUD, bonecos de treino, tipos) para entender o escopo.
- **Novos pedidos registrados (decisao do dono, a implementar quando autorizado):**
  1. Barra de HP mais bonita para o jogador e para os inimigos (bonecos de treino). O dono vai
     mandar imagens de referencia; este agente nao consegue ler imagens, entao descrever em texto.
  2. Ataques variados (nao so hitscan): habilidades no estilo de Wuyang e Moira (Overwatch) e
     armas com recuo e spread; configuracoes por personagem, nos Data Assets (FValdoriaWeaponStats
     e habilidades GAS).
  3. Menu de administrador (F1): trocar heroi, imortal, sem recarga, dano, teleporte, editar alvo
     mirado.
  4. Menu de configuracoes graficas + plugin NVIDIA DLSS (download precisa de aprovacao do dono).
  5. Selecao de heroi.
  6. Herois reais em Data Assets (1 por funcao, com aprovacao do dono).
  7. Integracao dos modelos 3D das armas (FBX/GLB do dono, de ferramenta no navegador) e das
     imagens de conceito (Gemini).
- **Observacao:** este agente nao consegue ler imagens (erro ao abrir os arquivos
  Gemini_Generated_Image_*.jpg). Descrever referencias em texto.
- **Arquivos tocados:** LOG.md, CLAUDE.md (Unreal) e, para descoberta, log.md/CLAUDE.md (Godot).
  Nenhum codigo do jogo foi alterado neste turno; as mudancas de animacao estao pendentes de
  finalizacao.
### Turno 12 — Claude Code — 18/11/2026 (animações Quaternius, revisão do turno do OpenCode)
**Falas do dono:**
> "o deeitar ficou uma bosta pq é animaçao de nado mas pode manter ela quem sabe a gente não adiciona um ambiente com agua futuramente"
>
> "está na pasta continua ai vou deitar vai arrumando o que sem mim"

- Conferido o turno 11 (OpenCode): só anotações. A importação dos pacotes UAL1/UAL2 em `Content/Valdoria` e os scripts `import_ual.py`/`retarget_ual.py` estavam sem commit; o retarget falhava (passava objetos em vez de AssetData). O `CLAUDE.md` tinha lixo no fim: corrigido.
- `retarget_ual.py` corrigido (AssetData, log em `Saved/retarget_log.txt`, recria as operações, auto-align). Gera 10 animações em `/Game/Valdoria/Animations/Retarget`, **mas o resultado ainda deforma o corpo**: o mapeamento de chains entre o rig da Quaternius e o `IK_Mannequin` sai vazio pela API Python da 5.8. Pendente: abrir `RTG_UAL1`/`RTG_UAL2` no editor, conferir o mapeamento de chains e o Auto Align da pose, e rodar o script de novo.
- Por isso o Recruta usa animações da Epic, sem deformação: deslizar = `MM_Unarmed_Crouch_Entry`, subir em beirada = `VaultOver`, escalar parede = `MM_Zipline_Loop`. Deitar = `UAL1Swim` (natação, mantida a pedido do dono para uma futura área com água).
- `CREDITS.md` criado (Epic, Quaternius CC0, licença MIT do projeto).
- Armas do Meshy: pasta `SourceArt\Armas` criada, mas o arquivo `Armas_Herois.fbx` ainda não apareceu nela.
- Teste automático completo passou (tiro, correr, deitar, impulso, ultimate, agachar, deitado, escalar, pausa).

### Turno 13 — Claude Code — 18/11/2026 (HUD no estilo das referências do dono)
Referências do dono (imagens, descritas em texto): barra de aliado ciano e de inimigo vermelha com caveira; barra de cada herói no estilo dele (Brasa laranja com escudo e 600 de vida, Kael verde-azulado 200, Iara turquesa com conchas 200, Vex roxa 150); ícone de passiva, 3 habilidades e ultimate com moldura; munição em três estilos (30/90 ciano, 24/72 dourado, 08/24 laranja) com ícones de balas.

- `ValdoriaHUD` reescrito (ainda desenhado em código, sem imagens):
  - Barra de vida **segmentada** (divisão a cada 25), vida na cor do herói (`ThemeColor`), armadura laranja e escudo azul em sequência, moldura na cor do herói, nome grande, funções, número atual / máximo.
  - **Barras sobre a cabeça** de todos os personagens: inimigo em vermelho com caveira, aliado em ciano, com o total de vida.
  - Fileira central: passiva (P), botão direito, F, E e **ultimate com moldura dupla**, preenchimento pela carga e pulsando quando pronta; recarga escurece o quadrado e mostra os segundos.
  - **Munição** grande na cor do herói (`18 / ∞`), ícones de balas que apagam ao atirar, nome da arma.
  - Cargas de impulso e estado do movimento ao lado da barra.
- Novo campo `ReserveAmmo` na arma (0 = infinita; por enquanto só visual).
- Teste automático ganhou o print `Saved/Screenshots/AutoTest_HUD.png`.
- Pendente: ícones de verdade (imagens) para passiva/habilidades/heróis quando houver arte; Data Assets dos heróis reais (Brasa, Kael, Iara, Vex) com as cores e vidas das referências, quando o dono aprovar os números.

### Turno 14 — Claude Code — 18/11/2026 (painel de administrador e tela de ajuda)
**Falas do dono:**
> "se tiver como abrir todas as janelas no monitor 2 seria melhor pra mim, tem coisas sem arrumar desde o inicio como o painel de adm com todas as configurações de testes necessarias entre outros tá guardado lá no md, as armas tao na pasta do projeto"
>
> "tecla K pressionada aparece o mouse e abre uma tela semi transparente explicando o que cada habilidade e passiva faz"

- **Painel de administrador (F1)**, em Slate/C++ (`ValdoriaAdminPanel.cpp`), com o jogo rodando e o cursor solto:
  - Herói: trocar para o Recruta ou para qualquer Data Asset `UValdoriaHeroDefinition` do projeto (lista automática); imortal; sem recarga; munição infinita; impulso infinito; multiplicador de dano; multiplicador de velocidade; ultimate cheia; vida cheia; zerar recargas.
  - Alvo na mira: pegar o alvo (raio pelo centro da tela ou o mais alinhado com a mira), ver nome/vida/armadura/escudo/distância, editar vida, vida máxima, armadura, armadura máxima, escudo e escudo máximo, matar, reviver/curar, imortal, congelar, ir até o alvo.
  - Inimigos: matar todos, reviver todos, criar boneco (básico, armadura, escudo, andando) onde a mira aponta.
  - Mapa: teleportar para a mira, voltar ao início, abrir qualquer mapa de `/Game` (lista automática).
  - Jogo: velocidade do jogo (câmera lenta/rápida), mostrar FPS e ping.
  - Hoje o jogo é local; com multiplayer essas ações viram RPCs para o servidor.
- **Tela de ajuda (segurar K)**: fundo semitransparente com o cursor solto; mostra nome, descrição, passiva e cada habilidade (tecla, nome, recarga, explicação). Os textos vêm do Data Asset: novos campos `PassiveName`, `PassiveDescription` e `Description` em cada habilidade. O Recruta já tem os textos preenchidos.
- Novos campos de admin no personagem: `bInfiniteDashes`, `AdminSpeedMultiplier`; `AValdoriaGameMode::SpawnDummyAt` público.
- Armas: o dono salvou `Armas_Herois.fbx` em `Projeto Padrão`; copiado para `SourceArt\Armas` (import ainda pendente).
- Monitor 2: o atalho Jogar já abre lá; o editor da Unreal e o Visual Studio lembram a última posição da janela, então basta arrastá-los uma vez para o monitor 2.
- Teste automático passou com o painel aberto e com a tela de ajuda (prints `AutoTest_Admin.png` e `AutoTest_Ajuda.png`).

### Turno 15 — Claude Code — 18/11/2026 (configurações e modo administrador no menu)
**Fala do dono:**
> "Podia ter deixado o painel no menu pra ativar, faltam as configurações de grafico, audio e tudo mais, e essas funções são poucas tá faltando mais da uma pesquisada se não souber quais mais adicionar"

- **Menu de pausa (Esc)**: novos botões **Configurações** e **Modo administrador: ligado/desligado**. O F1 só abre o painel com o modo administrador ligado; ligar o modo pelo menu já abre o painel.
- **Configurações** (`ValdoriaSettingsMenu.cpp`, abas Gráficos / Áudio / Controles, "Salvar e voltar"):
  - Gráficos: qualidade geral (Baixa → Cinemática), modo de tela, resolução (lista do monitor), V-Sync, limite de FPS (sem limite, 30–240), escala de resolução (50–100%), antisserrilhado/upscaling (desligado, FXAA, TAA, TSR), FOV (70–120), Lumen, ray tracing por hardware, desfoque de movimento, mostrar FPS e ping. Nota sobre DLSS (precisa do plugin da NVIDIA).
  - Áudio: volume geral (aplicado), música e efeitos (salvos para quando houver sons).
  - Controles: sensibilidade do mouse, inverter Y, tamanho e cor da mira, lista de teclas.
- `FValdoriaOptions` (`ValdoriaSettings.h/.cpp`): opções salvas em `GameUserSettings.ini` seção `[Valdoria]`; resolução/janela/V-Sync/FPS/qualidade no `UGameUserSettings` da Unreal. Personagem usa sensibilidade/inversão/FOV; HUD usa cor/tamanho da mira e "mostrar FPS".
- Teste automático abre as Configurações no fim (print `AutoTest.png`).
- Armas: `Scripts/split_weapons.py` (Geometry Scripting, plugin ligado só no editor) junta as 158 peças do FBX em 7 malhas (`/Game/Valdoria/Weapons/Merged/SM_Arma_00..06`), mas duas ou mais armas ficaram grudadas na `SM_Arma_05` (109 peças). Pendente: separar melhor (ex.: exportar uma arma por arquivo no Meshy).

### Turno 16 — Claude Code — 18/11/2026 (painel de admin ampliado; deitar e escalar sem animação)
**Fala do dono:**
> "tem que arrumar o x pra tirar a animação de nado e so deixar sem ação por enquanto, e a de escalar que estica o torço ao inves de ter animação"

- **Painel de administrador** ganhou, a partir do que campos de treino de hero shooters costumam ter:
  - Movimento e física: voar/atravessar paredes (noclip; anda para onde a câmera aponta), gravidade, força do pulo, controle no ar, salvar/ir para 3 posições.
  - Bots e treino de mira: bonecos que **atiram em você** (parados, fácil 6 de dano/30% de acerto, médio 10/50%, difícil 16/75%, com linha de visão e alcance), todos andando de lado, sem regenerar, imortais, "Treino de mira: 6 alvos" andando na sua frente, apagar bonecos criados, trocar o time do alvo (aliado ciano / inimigo vermelho).
  - Depuração: medidor de dano (DPS dos últimos 5 s, total e média) no HUD, zerar medidor, mostrar colisões (hitboxes), câmera livre, tempos de CPU/GPU, tirar print.
- Personagem: `SetNoclip`, `DealDamageTo` (dano via GAS, usado pelos bots). Boneco: `BotDifficulty`, `bForceStrafe`, `bNoAutoHeal`.
- **Deitar (X) sem ação** por enquanto (`bCanProne = false` no Recruta) e sem animação de natação; **escalar parede sem animação** (a anterior esticava o corpo). Os campos continuam no Data Asset para quando houver animações boas.
- Teste automático passou com o painel novo (print `AutoTest_Admin.png`).

### Turno 17 — Claude Code — 18/11/2026 (chat /adm, painel livre, dano em aliado, mapa Nova Paulicéia, armas)
**Falas do dono:**
> "deixa ativar o modo de adm por comando de chat /adm por exemplo, e deixa mexer o boneco e a mira mesmo com o menu aberto pra eu poder mirar, tá faltando uma opções pq eu n consigo tirar vida do aliado pra testar a cura, tem uma parte verde do mapa que tá com barra de hp, imagens de conceito do mapa gemini sugeriu Nova Paulicéia, eu abreviei pra np"
>
> "n esquece de arrumar as armas, eu n consigo n"

- **Chat (Enter)** com comandos (`ValdoriaChat.cpp`): `/adm` liga/desliga o modo administrador e abre o painel; `/ajuda`; com admin: `/imortal`, `/recarga`, `/municao`, `/voar`, `/bots 0-3`, `/dano N`, `/vida`, `/ult`, `/tempo N`, `/mapa np|treino`. Linhas do chat aparecem no HUD.
- **Painel de admin não prende o mouse**: com ele aberto você anda e mira; **segure Alt** para clicar nele. Pausa, configurações, ajuda (K) e chat continuam soltando o cursor.
- **Testar cura**: "Tirar 50 de vida de mim", "Tirar 50 de vida (aliado ou inimigo)" no alvo, e "+ Aliado ferido" (boneco do seu time com 30% de vida e sem regenerar). `/dano N` no chat.
- **Bug da barra na parte verde**: barras de inimigo atrás de parede não aparecem mais (teste de linha de visão); aliados continuam visíveis através das paredes.
- **Mapa Nova Paulicéia (NP)** — blockout jogável gerado por `Scripts/build_np_map.py` a partir da arte conceitual isométrica do dono (`Projeto Padrão/np isometrica conceito inicial.jpg`): `/Game/Maps/NP_NovaPauliceia`, ~120 x 80 m.
  - Spawn esquerdo (time 1): estação de metrô subterrânea com escada para a avenida. Spawn direito (`Time2`): prédio-torre com heliporto e rampa.
  - Rota A: elevado tipo Minhocão (6 m) com pilares, rampas nas pontas, postes; telhados com heliportos ligados por passarelas.
  - Meio: avenida larga com faixas, veículo (cobertura), canteiro, barracas de comida, **praça amarela (objetivo)** sob viaduto curvo com pilares, barreiras de concreto e caixotes.
  - Rota C: mercado coberto com bancas e lonas, becos com painéis de grafite neon, caixa d'água.
  - Marco: arranha-céu de 90 m com antena e luz vermelha. Clima: sol poente laranja, neblina roxa, neons azul/roxo/rosa, chão de asfalto "molhado".
  - Materiais `M_NP_Blockout` + instâncias em `/Game/Maps/NP_Materials`. O modo de jogo escolhe o spawn sem etiqueta (time 1). Abre pelo painel (Mapa) ou `/mapa np`.
- **Armas**: `Scripts/split_weapons.py` agora separa as 158 peças do FBX por região (vista de frente) em 11 malhas: `SM_Arma_Rifle`, `Martelo`, `EscudoA`, `EscudoB`, `CajadoA`, `CajadoB`, `Bastao`, `LaminasA`, `LaminasB`, `PecaPequena`, `Extras` (uma placa grande). O Recruta segura o **rifle** (novo campo `WeaponMesh`/`WeaponSocket`/`WeaponOffset` no Data Asset; escala 2,2, rotação Pitch 90 no `hand_r`). A pose de segurar ainda é a de mãos livres (falta animação de arma).
- Teste automático completo passou no mapa NP (tiro, correr, deitar sem ação, impulso, ultimate, agachar, admin, ajuda, pausa/configurações).

### Turno 18 — Claude Code — 18/11/2026 (bugs relatados pelo dono, agora testados com teclas de verdade)
**Falas do dono:**
> "bug do hp continua, outro bug a segunda vez que morri meu boneco ficou travado no spawn, enter nao abre chat nenhum, tem certeza que vc fez isso?"
>
> "o botao f1 ativa outros graficos entao tem que trocar o atalho"

- **Honestidade:** no turno 17 o chat, o painel e as correções foram só compilados, sem teste de tecla real. Os três bugs eram reais.
- Novo teste `-ValdoriaInputTest` (`AValdoriaGameMode::RunInputTestStep`): aperta teclas pelo mesmo caminho do teclado (Slate → viewport → jogo): Enter, digita "/adm", Enter, F10, Esc, Esc, K; depois mata o herói **duas vezes** e confere se ele anda depois de cada renascimento. Resultado final: tudo OK (chat abre, /adm liga o modo e abre o painel, F10 alterna, Esc pausa/volta, K abre/fecha, anda 7,6 m depois de cada renascimento).
- **Enter não abria o chat** — duas causas: (1) o Slate usa Enter como "confirmar" e engolia a tecla; agora um `IInputProcessor` (`FValdoriaChatKeyProcessor`) pega o Enter antes; (2) o chat fechava sozinho ao perder o foco; agora só fecha com Enter (envia) ou Esc (cancela).
- **Travado depois da 2ª morte:** o contexto de entrada (WASD) do corpo antigo continuava registrado e "segurava" as teclas; agora `ClearAllMappings()` antes de adicionar o do herói novo.
- **Barra de vida "no cilindro verde":** era o boneco "andando de lado", que nascia em cima do cilindro (1800 cm à frente). Agora cada boneco procura chão na mesma altura do spawn, aproximando-se até achar.
- **Painel de admin passou do F1 para o F10** (no editor, F1 é o atalho da Unreal que muda o modo de visualização).
- Prints: `Saved/Screenshots/Input_Chat.png`, `Input_Adm.png`, `Input_Renasceu1.png`, `Input_Renasceu2.png`.
