# Pesquisa de referência — retorno visual, HUD e mapas (18/11/2026)

Pesquisa feita na web a pedido do dono, só como inspiração: nenhum asset ou código de outro jogo
foi baixado ou copiado. As fontes são, em sua maioria, artigos, guias e fóruns, e não estudos
formais.

## 1. Retorno visual (o jogo "responder" ao jogador)
- **Seta de dano direcional**: mostrar de onde veio o golpe. A seta maior indica um golpe mais forte e ela some em cerca de 1 s. Indicadores só de "tela piscando" não dizem a direção. → **Feito** (turno 23): arco vermelho em volta da mira.
- **Marcador de acerto** com versões diferentes para cabeça e abate, aparecendo cerca de 50–100 ms depois do tiro. Atraso entre tiro e resposta enfraquece a sensação. → já temos (branco, amarelo para a cabeça, vermelho para o abate).
- O marcador não deve substituir o alvo reagindo ao golpe: a ideia futura é fazer o boneco "tremer" ou tocar uma animação de impacto.
- **Pouca vida**: bordas vermelhas pulsando. → **Feito**.

## 2. HUD de habilidades
- Recargas devem ficar perto do centro de atenção e com contagem visual; a ultimate precisa de um estado "pronta" inconfundível e do progresso enquanto carrega. → temos a porcentagem, a moldura pulsando e agora o aviso **"ULTIMATE PRONTA (Q)"** no centro.
- Evitar excesso de informação: barras e luzes no lugar de números quando possível.
- Acessibilidade barata: deixar o jogador mover, esconder ou redimensionar partes do HUD. → ideia futura no menu de Configurações.

## 3. Mapas de hero shooter (para a NP e o modo MOBA)
- Os três controles clássicos são os **pontos de estrangulamento** (corredores onde a luta acontece), as **rotas de flanco** (2 a 3 por área, para heróis móveis) e as **linhas de visão longas**. Um mapa que foge disso costuma favorecer um dos lados.
- **Altura** dá vantagem de visão. Por isso cada ponto alto precisa de um caminho de flanco que o ameace.
- **Mobilidade quebra a geometria**: quem voa ou usa gancho ignora muros e pontes. Os mapas precisam ser testados com Kael (gancho), Vex (pulo duplo, escalar) e voo.
- Em modos com fases (escolta, MOBA com rotas), os pontos de luta mudam ao longo da partida. Cada fase precisa do seu próprio estrangulamento e dos seus flancos.

## Fontes
- [A UX Analysis of First-Person Shooter Damage Indicators](https://medium.com/@jasper.stephenson/a-ux-analysis-of-first-person-shooter-damage-indicators-59ac9d41caf8)
- [Directional Hit Indicator (Autodesk Scaleform)](https://help.autodesk.com/cloudhelp/ENU/Scaleform-Help/scaleform_help/hud_kit/hud_view/directional_hit.html)
- [Hit markers are an FPS crutch (PC Gamer)](https://www.pcgamer.com/games/fps/hit-markers-are-an-fps-crutch/)
- [What Makes an FPS Feel Good? (StraySpark)](https://www.strayspark.studio/blog/fps-game-design-fundamentals-ue5)
- [The Invisible Interface (StraySpark)](https://www.strayspark.studio/blog/game-ui-ux-design-principles)
- [7 beginner mistakes with your game's HUD](https://bootcamp.uxdesign.cc/7-obvious-beginner-mistakes-with-your-games-hud-from-a-ui-ux-art-director-d852e255184a)
- [Overwatch maps guide (Hotspawn)](https://www.hotspawn.com/overwatch/guide/the-beginners-guide-to-overwatch-map-types)
- [Marvel Rivals maps guide (Mobalytics)](https://mobalytics.gg/blog/marvel-rivals/maps-game-modes-guide/)
- [Eichenwalde map analysis (Kotaku)](https://kotaku.com/an-in-depth-look-at-overwatchs-eichenwalde-map-1797348568)
- [Designing a playground (cxong)](https://cxong.github.io/2025/06/designing-a-playground)
