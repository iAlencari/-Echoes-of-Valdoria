# Guardian Spots (GS) and Final Guardian (GF) - review and proposals (turn 44, 2026-11-19)

Status: proposals for the owner. Nothing here is implemented unless marked "done". Numbers are placeholders.

**Owner's answer (turn 44): "every MOBA works like that"** -> done in code (waiting for the build): final guardian
gate (immune until the last spot of one lane is sealed/gone) and credits for semi-seal / seal / reverse / gone.
Still open: Bifrost reward, guardian signature threats with telegraphs, compact SPOTS HUD panel, spot 2/3 buffs.
Current rules: see `CLAUDE.md` (MOBA spots, comeback rules, sealing ritual, final guardian) and `LOG.md` turns 35-41.

## What works today
- 3 lanes x 3 spots per team, the last spot with two guardians (one support mage). Leashed autopilot, defends allies.
- Spot buffs with timed fades and comeback multipliers (early fall, same-lane triple).
- Sealing ritual (semi-sealed -> seal or reverse, group channelling, 30 s timeout, immunity, sphere and energy line).
- GF behind each base inherits health/damage and the kits of its linked standing spots; killing it wins.

## Problems seen (playtest + code reading)
1. **Bots never reach the spots** (no pathfinding; fixed in code, to verify): MOBA matches had ~1 kill in 2 minutes.
2. **No reward for sealing** besides the buff loss of the enemy: no credits, no map change. Players have little reason to
   sit 4 s channelling under fire.
3. **GF can be hit from minute 1** (only stronger). A rush could skip the spots; the lore ("seal the spots first") does
   not show in the rules.
4. **Readability**: the spot buff text stacks in the HUD ("SPOT: -50%...", "+20%", "+10 HP/S") and new players will not
   know which spot gives what. Guardians are all Brasa-shaped; the mage only differs by its heal.
5. **Leash + "defend ally" only**: guardians do nothing to enemies that stay out of their area -> they are safe to
   poke from range forever (no ranged threat).
6. **Comeback triple buff** depends on minutes 5/10 that are long in a 2-minute bot test; hard to verify. Needs a
   time scale option for tests.

## Proposals
**Rewards and pacing**
- Seal (or reverse) gives credits: 150 to each nearby ally (inside the area), 50 to the rest of the team. A spot kill
  (semi-seal) gives 50 to the killer. Placeholders.
- After spots 1 and 2 of a lane are sealed, **the attacking team's Bifrost in that lane reaches the enemy last spot**
  and rides 30% faster (map mobility as a reward, same idea as faster ziplines in shooter MOBAs; our own look).

**Gates for the GF (lore made rule)**
- GF is **immune** (amber shield, IMMUNE on hits, like semi-sealed) until the **last spot of at least one lane is
  sealed**. Option B: until any 4 spots are sealed. Owner picks.
- While immune it still fights (it protects the base); the shield breaks with a map-wide roar.

**Guardian identity (one readable threat each, with a wind-up)**
- Melee guardian: ground slam with a red circle 1 s before (on foot, not only vs Bifrost riders).
- Mage guardian (support): heal pulse with a visible ring + empower beam; add a slow ranged orb so poking is not free.
- Flyer guardian (new model later): dives at the furthest enemy in range, telegraphed by a shadow on the ground.
- Thrower guardian: lobbed rock with an impact marker.
- Give each a different tint/size until real models exist (spot 1 small, spot 2 medium, spot 3 large).

**HUD**
- One compact "SPOTS" panel: 3 lanes x 3 icons per side (alive / semi-sealed / sealed), the active buff as an icon with
  a fading ring instead of three text lines. Spot under attack blinks.

**Testing**
- `-ValdoriaMobaTimeScale=<x>` (or admin "time x4" already exists: use `/tempo 4`) to check minute-5/10 rules quickly.
- Playtest observer: log spot state changes (semi-sealed, sealed, reversed) with time.

## Questions for the owner
1. GF gate: one lane's last spot sealed, or any 4 spots, or none?
2. Credits for sealing: yes/no, and amounts?
3. Should guardians have a ranged attack against long-range pokers?
4. Spot 2 and 3 buffs are still placeholders (+20% damage fading 5-10 min; regen 10/s + 10% damage). Keep?
