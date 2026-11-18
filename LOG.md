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

**Ainda não feito / limitações conhecidas:**
- Agachar, deitar, deslizar e escalar ainda **não têm animação** (a malha só abaixa/sobe). O
  ABP do modelo só tem andar/correr/pular.
- Efeitos visuais são linhas de depuração (só aparecem em builds de desenvolvimento).
- Subir em beirada e escalar parede só foram testados por código, não à mão.
- Sem Data Assets de heróis reais ainda; sem menu de admin, menu gráfico, DLSS, seleção de herói.

## Próximos passos sugeridos
1. Dono testar no editor (Play) e ajustar sensação de movimento.
2. Menu de administrador (trocar herói, imortal, sem recarga, dano, teleporte, editar alvo mirado).
3. Menu de configurações gráficas + plugin DLSS da NVIDIA (download precisa de aprovação do dono).
4. Primeiros heróis reais (1 por função) como Data Assets, com a aprovação do dono.
5. Animações de agachar/deitar/escalar e efeitos visuais de verdade (assets livres aprovados).

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
