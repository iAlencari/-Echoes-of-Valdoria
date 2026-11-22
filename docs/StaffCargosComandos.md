# Staff roles and in-game admin tools (spec)

Owner's request, 2026-11-21. Engine-agnostic: the same spec is shared with the sister projects
(Valdoria, Contos) so all games use the same roles and commands.

## 1. Roles (from lowest to highest)

| Role (PT) | Who | Job | Power |
|---|---|---|---|
| **Jogador** | everyone | plays | settings, chat, PM, `/reportar` bugs |
| **Game Master (GM)** | public support staff | attends players, handles bug reports, fixes what can be fixed inside the game | look/locate/teleport/freecam, `/call` and release players, bug tab |
| **Moderador** | moderation staff | moderates the game and the players (conflicts, abuse, punishments) | everything a GM has + mute/kick/ban, broadcast, **maintenance mode** (emergency time freeze) |
| **GOD** | owners and partners (sócios) | decisive power over the game, including code and server config (like GOD on TFS/OTX Open Tibia servers) | everything + spawn/give/edit, **Terminal** |

"Admin" in old texts = Moderador + Game Master. GOD is the new top role.
Rule for the Terminal: changes only take effect with the approval of **more than half of the GODs (sócios)**
or of the **owner (dono)** alone. Everything staff does goes to an **audit log**.

## 2. Settings menu (gear icon)

Tabs: **Gráficos**, **Áudio**, **Câmera** (fixed RPG camera or *live camera*: rotate freely while walking,
including to look at your own character), **Controles**, **Jogo** (UI options, report a bug, save and quit),
and for staff: **Staff** (command buttons), **Jogadores**, **Bugs**, **Terminal** (GOD only), **Auditoria**.

Each staff command is a button. Commands that need a target offer **Texto** (type a name/id) or
**Lista** (pick from a list).

## 3. Maintenance mode (emergency time freeze)

- Moderador/GOD toggles it (`/manutencao on|off` or button).
- For players everything stops in time: characters, monsters, projectiles, skill effects, cooldowns, respawns,
  regen, damage over time. Cosmetic things keep running so the player sees the game did not freeze:
  trees and grass in the wind, water, sky/clouds, shadows, fire, weapon glows, hair physics.
- A low-opacity red band across the middle of the screen: **JOGO EM MANUTENÇÃO**.
- If a player closes the game during maintenance, everything stays exactly as it was, including their character
  (position, HP/MP, cooldowns, buffs, enemies around).
- Staff are not frozen and can work: `/freecam`, `/call`, teleport, inspect.

## 4. Chat commands (`/ajuda` lists them for your role)

Targets (all commands that take a target accept any of these):
`eu`, `jogador:<nome|id>`, `boss[:<nome>]`, `mob:<nome|id>`, `spawn:<nome|id>`, `item:<nome|id|uid>`,
`mapa:<nome|id>`, `masmorra:<nome|id>`, `uid:<n>`, `pos:x,z` or `pos:x,y,z`, or just a name/number
(searched in everything). If more than one entity matches, a window lists them with **tipo, id, nome, uid,
dono, posição, local** (world, inventory of player X, dungeon) to pick from.

| Command | Role | What it does |
|---|---|---|
| `/ajuda [comando]` | all | list commands / help |
| `/pos` | all | your position and map |
| `/pm <jogador> <texto>` | all | private message |
| `/reportar <texto>` | all | report a bug (goes to the staff Bugs tab with your position) |
| `/freecam [alvo]` / `/freecam off` | GM | free camera; with a target it flies there |
| `/tp <alvo>` | GM | teleport yourself to a target |
| `/info <alvo>` | GM | everything about an entity |
| `/buscar <texto>` | GM | search entities by name/id/uid |
| `/call <jogador>` | GM | summon a player next to you; their exact state and place are saved. If they were in a solo boss dungeon, the dungeon freezes until they are released. `/call` again (or `/liberar`) sends them back exactly as they were. They also go back automatically if they disconnect or stay silent (no chat, no PM) for the configured time, with an apology message: the moderation team will review it and may compensate any loss. |
| `/liberar <jogador>` | GM | same as the second `/call` |
| `/bugs` | GM | open the bug tab |
| `/manutencao on|off` | Moderador | emergency time freeze |
| `/avisar <texto>` | Moderador | broadcast to everyone |
| `/mute <jogador> <min>` `/kick <jogador>` `/ban <jogador> <motivo>` | Moderador | punishments (logged) |
| `/congelar <alvo>` | Moderador | freeze one entity |
| `/spawn <tipo> [nível] [qtd]` `/matar <alvo>` `/curar <alvo>` `/nivel <jogador> <n>` `/dar <jogador> <item> [qtd]` `/deus` `/tempo <escala>` `/papel <jogador> <papel>` | GOD | world editing |
| `/terminal` | GOD | open the Terminal |

## 5. Terminal (GOD only)

Config/code-level changes (`config <chave> <valor>`, ...). A change by a non-owner GOD becomes a
**proposal**; it is applied when more than half of the GODs approve, or immediately if the owner approves.
`propostas`, `aprovar <id>`, `rejeitar <id>`, `config` (list), `log`.

## 6. Vox Numen status

The game is single-player for now: other "players" are **simulated players (bots)** used to test `/call`,
lists and dungeons. Security (who is GOD) must be enforced by the server when the online version exists;
offline the local account role is a test setting.
