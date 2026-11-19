# Combat rules - owner decisions (turn 46, 2026-11-19). Numbers are the owner's unless marked "Claude".

## Levels
- 20 levels per match, 1 ability point per level.
- Only abilities **1, 2, 3 and 4** are upgraded. LMB, RMB, V, C and the passive are never upgraded.
- Abilities 1-3: unlock + 4 upgrades each (5 ranks). Supreme (4): unlocks at level **5**, upgrades at **9, 13,
  16 and 20**. 15 + 5 = 20 points: everyone is maxed at level 20, the choice is the ORDER.
- Upgrades: a little more damage/buff and a little less cooldown.
- **Elevação**: 2 points, at levels 10 and 20; each ability elevated once. Big changes: area control, single
  target -> area, damage reduction / control immunity / damage immunity while casting, heal cut, etc.

## Cooldown reduction (CDR)
- Items: up to **30%**. Specific abilities or items can lift the cap to **50%**, never more.
- CDR also speeds up the supreme charge.
- With levels and CDR items some abilities need longer base cooldowns (review hero by hero).

## Healing from damage
- **Vampirismo**: heals from basic attack (LMB) and **V** damage.
- **Roubo de vida**: heals from abilities and passives.
- Kept separate, but hybrids are allowed (an effect can give both, or temporarily boost one of them).

## V - melee (new for every hero)
- Every hero has a melee attack on **V**: a punch (per-hero style later).

## Damage types
- Physical and magical exist. **Hybrid**: weapons/ammo forged or enchanted with mystic stones deal both.
- **True damage** (ignores defence and damage reduction): later.

## Health (owner: raise HP a lot)
- Claude's proposal: everything x10 (HP, damage, heals). To do as one batch with tests.
- Out of combat: 1 HP every 0.5 s. A whole minion wave cleared: +10% of CURRENT HP. Overheal only at full HP.

## Implemented in turn 46 (code) - numbers provisional
- **HP x10**: `ValdoriaStatScale = 10` (ValdoriaTypes.h) multiplies every health/armour/shield/damage/heal at runtime;
  the Data Assets keep their numbers. Out-of-combat regen 2 HP/s (1 every 0.5 s), heroes only.
- **Levels (MOBA only)**: start at level 1 with 1 point. **Ctrl + 1/2/3/4** spends a point. Abilities 1-3 rank up
  to (level + 1) / 2; supreme at 5/9/13/16/20. Locked abilities (rank 0) show "BLOQ.". Each upgrade: +6% damage/heal
  (magnitude) and -5% cooldown. Bots spend points on their own (supreme first, then the lowest of 1-3).
- **XP**: 120 + 60 x (level - 1) per level (turn 47: doubled and growing; level 20 after ~12500). Passive 2/s; minion: killer 40, allied heroes
  within 15 m 20; hero kill 200, assist 100; guardian credits x1.5 as XP.
- **Gold (scoreboard)**: same as the shop credits (4/s, 0.5 per damage point in design units, 100 per kill,
  50 per assist, spot credits).
- **Respawn (MOBA, turn 47)**: (6 s + 2.5 s x (level - 1)) x (1 + 5% per minute after minute 8); cap 60 s before minute 18, 90 s from 18, 120 s from 25. Deaths and farm
  are not counted on their own (farm already becomes level; counting deaths punishes twice).
- **CDR**: shop CDR capped at 30%.
- **Keys**: V punch (camera on the **mouse wheel click** since turn 47; F keys are taken by the official client), **Tab** scoreboard (K/D/A, gold, damage, healing,
  tank = damage taken, objectives = seals), **Y** last death recap (who, what source, how much, last 10 s).

## Turn 47
- **Head bars**: no numbers on any bar above heads; floating damage numbers (mine) and green "+N" heals (mine).
- **Vampirism vs life steal**: vampirism = basic attack + V (weapon LifeSteal + buffs, max 40%); life steal = abilities
  and passives (shop + buffs, max 30%). Caps provisional.
- **Neblinas**: `AValdoriaFogArea` (radius 5.5 m, 4 m high). Inside = invisible to everyone outside (render, bars,
  minimap, bots); seen only by someone in the same fog, or by reveals/clairvoyance. Sight inside 8 m. MOBA: 4 areas on
  the side lanes at 35% and 65%.
