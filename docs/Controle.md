# Controle (gamepad / DualShock 4) em Valdoria

Pedido do dono (22/11/2026): o jogo tem que aceitar o controle dele, uma imitação de controle de PS4
(DualShock 4) com touchpad funcionando. Por enquanto a prioridade é movimentação, menus e interação;
a distribuição das habilidades nos botões pode mudar depois.

## Como o controle chega ao jogo

A Unreal lê controles **XInput** (padrão Xbox) sem nenhum plugin. Um DS4 genérico conectado pelo
Bluetooth aparece no Windows como "Wireless Controller" (HID/DirectInput), e a Unreal não lê esse tipo
direto. Ele precisa ser "traduzido" para XInput por um destes programas. No PC do dono já estão
instalados o ViGEmBus (Nefarius), o x360ce (em `Downloads\x360ce`) e a Steam:

1. **Steam Input** (mais simples, se a Steam estiver aberta). Na Steam, use "Adicionar um jogo que não
   é da Steam" e escolha o `UnrealEditor.exe` ou o jogo empacotado. Ative a Entrada Steam para
   controles PlayStation nas configurações do controle. A Steam já mapeia o touchpad como mouse.
2. **DS4Windows** (grátis, código aberto: github.com/schmaldeo/DS4Windows). Ele usa o ViGEmBus, que já
   está instalado, e emula um controle de Xbox 360. Ative "Use Touchpad as Mouse" para o touchpad
   mexer o cursor nos menus.
3. **x360ce** (já baixado): o mesmo papel, configurando os botões à mão.

Para confirmar que está funcionando, abra a pausa com o botão Options: ela tem que abrir com o
botão "Continuar" em dourado.

> A Unreal 5.8 também tem o plugin `GameInputWindows` (beta), que poderia ler o DS4 direto, e o
> `WinDualShock`, que depende do SDK da Sony e não pode ser usado aqui. Nenhum dos dois foi ligado,
> para não arriscar entrada duplicada. Fica para testar depois, com o controle na mão.

## Botões (nomes do PS4; entre parênteses o nome XInput no código)

| Botão | Jogo | Menus |
|---|---|---|
| Analógico esquerdo (Left2D) | andar | navegar |
| Analógico direito (Right2D) | mirar/câmera: 120 graus/s no máximo × sensibilidade; assistência de mira ligável em Configurações > Controles | - |
| L3 (LeftThumbstick) | correr / impulso (Shift) | - |
| R3 (RightThumbstick) | soco (V) | - |
| X (FaceButton_Bottom) | pular / voar (Espaço) | confirmar |
| O (FaceButton_Right) | agachar (Ctrl) | voltar / fechar menu |
| Quadrado (FaceButton_Left) | recarregar (R) | - |
| Triângulo (FaceButton_Top) | habilidade 3 | - |
| R2 (RightTrigger) | tiro principal | - |
| L2 (LeftTrigger) | tiro secundário | - |
| L1 / R1 (shoulders) | habilidade 1 / habilidade 2 | - |
| Direcional cima | ultimate | navegar |
| Direcional esquerda | habilidade extra (C) | navegar |
| Direcional direita | interagir, segurar (G) | navegar |
| Direcional baixo | 1ª / 3ª pessoa (botão do meio do mouse) | navegar |
| Options (Special_Right) | pausa (Esc) | fecha a pausa |
| Share (Special_Left) | placar (Tab), segurar. Share + habilidade = gastar ponto de nível (Ctrl + 1/2/3/4) | - |
| Touchpad | cursor do mouse, via Steam ou DS4Windows | clicar |

Ainda sem botão no controle: chat (Enter), loja (B), deitar (X do teclado), recapitulação da
morte (Y), ajuda (K), painel de admin (F10) e cursor livre (Alt). O touchpad como mouse cobre os menus
que abrem com o cursor.

## Onde está no código

- `ValdoriaHeroCharacter.cpp`:
  - `BuildDefaultInput` e `MapAbilityKeys` registram os botões do controle no mesmo contexto do
    teclado (`IMC_Valdoria`).
  - `Input_LookStick` gira a câmera: 120 graus/s × `GamepadSensitivity`, com resposta quadrática para
    mirar com mais precisão perto do centro do analógico. O eixo Y foi invertido no turno 51, porque
    a primeira versão subia ao contrário.
  - **Assistência de mira** (`bAimAssist`, ligada por padrão, só para o controle). Vale para um
    inimigo visível a até 45 m e a até 8° do centro da mira:
    - **Atrito:** a câmera fica até 45% mais lenta sobre o alvo.
    - **Puxão:** puxa a mira para o peito do alvo enquanto o analógico se mexe.
- `ValdoriaPlayerController.cpp`:
  - `SetupInputComponent` liga Options, O, Share, R3 e o direcional para baixo.
  - `MenuBackPressed` é o O, que fecha o menu de cima.
  - `ApplyCursorInput` e `FindMenuFocus` dão o foco ao primeiro botão do menu aberto (na pausa, o
    "Continuar"), para o direcional navegar. O botão em foco fica dourado.
  - O painel F10 não recebe foco de propósito: o analógico continua andando com ele aberto.
- `ValdoriaSettings` guarda a `GamepadSensitivity` e a `bAimAssist` no GameUserSettings.ini.
- **Configurações:**
  - "Salvar e voltar" fica na linha das abas: no fim da lista, o direcional pulava de volta para as
    abas.
  - As barras deslizantes mudam com o direcional para os lados, sem travar com X (`RequiresControllerLock(false)`),
    em 20 passos.
- **Teste automático:** `-ValdoriaInputTest` (`ValdoriaGameMode.cpp`, `RunInputTestStep`). Os passos
  "controle" simulam Options, o direcional, o O e o analógico esquerdo.
