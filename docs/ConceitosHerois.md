# New hero concepts (turn 44 drafts, 2026-11-19)

Original characters. Other games only as genre/mechanic reference. Diverse bodies and origins, natural proportions,
no exaggerated sexualisation (owner's rule). Kits use the existing ability library when possible (marked [lib]) so
they cost little code; new mechanics are marked [new]. Numbers are left to the owner.

Current roster: Brasa (Vanguard), Kael Rook (Warrior), Iara (Rearguard), Vex (Flanker, telekinesis), Lume
(Rearguard + Warrior, beam). Gaps: a **second Vanguard**, a **zone/trap Warrior**, a **mobile Rearguard**,
a **ranged Flanker without stealth**, and anyone that uses the **Bifrost/rail** idea (CanRideAnyRail).

---

## 1. Tauá - Vanguard (big, slow, rooted in the ground)
Indigenous-inspired engineer of stone and roots from the outskirts of Nova Paulicéia. Broad, older (50s), grey braid.
- Weapon: root-sling, short-range lobbed seeds that burst (splash) [lib projectile + splash].
- Passive *Raízes*: standing still for 1 s grants armour regeneration [new small passive].
- RMB *Muralha de Pedra*: raises a wall that blocks everyone (not only enemies) for 4 s [lib barrier variant].
- F *Enraizar*: cone that roots enemies 1 s [lib cone + ApplyStatus Rooted].
- E *Galho Puxador*: pulls himself to a wall (anchor) [lib hook self].
- Ult *Floresta Viva*: big zone where allies regenerate and enemies are slowed [lib zone].

## 2. Sombra-de-Neon "Nix" - Warrior (traps and area denial)
Non-binary street artist and hacker; slim, short, neon spray cans.
- Weapon: paint-gun burst (3 shots) [lib hitscan burst = fire rate tuning].
- Passive *Marcado*: enemies hit get a neon mark: you see them through walls for 2 s [new].
- RMB *Mina de Tinta*: sticky mine that explodes in a slow zone [lib zone thrown].
- F *Grafite Holográfico*: fake copy of you that runs forward (decoy) [new].
- E *Escape*: dash backwards [lib dash].
- Ult *Apagão*: city lights go out around you: enemies in 15 m are blinded (vision reduced) for 4 s [new].

## 3. Maré "Kaila" - Rearguard (mobile healer, rides any Bifrost)
Pacific-islander surfer medic; tall, athletic, curvy; board of light.
- Weapon: healing wave projectile that bounces once (heals ally / hurts enemy) [lib projectile + AllyHeal].
- Passive *Surfista*: can ride **any** Bifrost, allied or enemy (`bCanRideAnyRail`) and heals 10/s while riding.
- RMB *Prancha*: summons a short light-trail to slide on (allies can use it too) [lib SlideTrail].
- F *Resgate*: dash to an ally and heal both [lib ally link].
- E *Bolha*: shield bubble on an ally for 2 s [new small, based on overheal/shield].
- Ult *Tsunami de Luz*: a wave crossing the area: heals allies, pushes enemies [lib cone/whirl variant].

## 4. Ferro "Dandara" - Flanker (ranged, no stealth, grappling)
Afro-Brazilian ex-firefighter turned bounty hunter; muscular; prosthetic arm.
- Weapon: lever-action rifle, high damage, slow [lib hitscan].
- Passive *Adrenalina*: each kill resets the grapple and gives 30% speed for 3 s [new small].
- RMB *Gancho Hidráulico*: grapple to walls (already: hook self) [lib].
- F *Granada de Fumaça*: smoke zone that blocks vision (bots ignore targets inside) [new zone flag].
- E *Chute*: melee kick that knocks back [lib melee + launch].
- Ult *Caçada*: marks the lowest-health enemy on the map; you see them and deal +30% to them for 8 s [new].

## 5. Ecos "Ícaro" - Warrior/Flanker (flyer using the sky ceiling)
Young pilot with mechanical wings; short, wiry; asthma inhaler as a character detail.
- Weapon: wing darts (fast projectiles) [lib].
- Passive: glides by default when falling (bCanGlide) and flies with fuel (FlightMode Fuel) [lib movement].
- RMB *Mergulho*: dives to the aim point and explodes on landing [lib leap slam].
- F *Corrente de Ar*: updraft zone: allies inside fly up [new small].
- E *Chaff*: breaks enemy lock-ons (Lume beam, homing) for 2 s [new].
- Ult *Esquadrilha*: 4 drones fly with you and shoot your target for 6 s [new; could reuse the Fab drone system pack the
  owner already added].

---
Next steps (owner): pick 1-2 concepts to build first, approve names, then the concept art (Gemini) for the models.

---

## Turn 45 (2026-11-19): heroes now in the game

Standard keys for every hero: LMB basic attack, RMB ability 5, 1/2/3 abilities, 4 supreme, optional extra on C.

### Kay [Agent Kay] - owner's hero (refer to Kay only by name)
Non-binary, inspired by Clove (Valorant) as a personality reference only; born male, androgynous look, short black hair in Lume's
style, heterochromia. **No weapon**: bends fire, earth, water and air with the hands.
- In game (base only): elemental bolt on LMB (fire/water magical, earth physical, air hybrid); tap 4 = next
  element (20 s cooldown); hold 4 for 3 s with a full supreme = **Devastação** (9 m burst; fire leaves burning
  ground, earth stuns, water slows, air throws up). Releasing early cancels with a 3 s lock.
- **Abilities 1/2/3, RMB and passive: the owner will design them** (entries can be bound to one element with
  `ElementSet` 0-3, so each element can have its own set). Do not invent them.
- 3D concept prompt (sent to the owner in chat, turn 45): full body A-pose / three-quarter turnaround, slim
  androgynous build, short black asymmetrical hair, one amber and one ice-blue eye, fitted agent jacket with
  four small elemental sigils, bare hands with faint elemental glow, no weapon; versions with and without effects.

### Ariel - created by Claude (bigender; refer to Ariel by name or "de Ariel")
Swaps form with **C**, keeping health %, supreme charge and cooldowns.
- **Forma Rocha** (masculine, Vanguard, `DA_Ariel`): stone-skinned fists, 325 HP + 100 armour, 15% physical
  resistance. LMB wide punch; RMB *Investida* (lunge); 1 *Tremor* (slam + slow); 2 *Pele de Pedra* (heal +
  speed); C swap; 4 *Queda de Montanha* (leap slam).
- **Forma Faísca** (feminine, Warrior, `DA_ArielFaisca`): electric sparks from the hands, 200 HP, faster, 2
  dashes, wall climb. LMB spark bolts; RMB *Descarga* (cone); 1 *Relâmpago* (dash); 2 *Arco Voltaico*
  (piercing line); C swap; 4 *Tempestade* (weapon surge).
- All numbers are placeholders for the owner.
- 3D concept prompt: see the turn-45 reply (one character sheet, two forms side by side sharing the same face
  structure, hairstyle and a shared amber-and-blue scar/mark, so the player reads them as one person).

## Turn 46 (2026-11-19): Heimdall - owner's next hero (concept only, no code yet)
Norse guardian of the Bifrost (mythology, public domain). Our own look: no horned helmet, no comic-book design.
Waiting for the owner's feedback on the Gemini images/videos before any code.
- **Passive (owner)**: never falls off a Bifrost while riding it and can attack while riding. Different riding
  animation: he surfs/slides on it (side stance, knees bent, axe low) instead of the lean pose.
- **Supreme option A (owner) - Chamado da Bifrost**: raises the axe, then a straight charge. Passes through
  minions and grabs the FIRST enemy hero. Not a teleport: at the end of the charge the victim is thrown up and
  back, and the nearest Bifrost "calls" and catches the victim (so a well-aimed charge can drop someone on another
  lane's bridge). The victim rides that Bifrost to their own base, unable to shoot, use abilities or get off.
  After arriving, the victim cannot take that same Bifrost back for 8 s (other bridges are fine). Hitting nobody
  wastes the supreme.
- **Supreme option B (owner) - Gjallarhorn**: blows a horn while raising the weapon; a small area appears. The
  closer he is to a Bifrost, the farther the jump reaches (at the base, next to all 3 bridges = maximum range).
  Everyone inside is teleported together along a rainbow arc drawn from where he stands to where they land.
  Excluded: guardians (GS), final guardian (GF) and jungle (JG).
- Decisions (owner left them to Claude, turn 46):
  - Option B carries EVERYONE inside the area when the horn ends: allies, enemies and minions (owner's idea,
    like Hela stepping into the Bifrost). Enemies get the 1 s warning (marker + area) to walk out; an enemy who
    stays lands with Heimdall's team. GS/GF/JG are never carried.
  - Basic attack idea (owner's other inspiration): thrown axes that fly in an arc, ranged.
  - Heimdall can ride any Bifrost, allied or enemy, and the passive works on all of them.
  - Landing point, option B: hold 4. The horn plays (about 1.5 s) and the minimap opens big in the middle of
    the screen with a circle of the maximum range around Heimdall. Move the mouse to place the rainbow marker,
    then confirm with LMB (or release 4). RMB cancels and refunds half the cooldown. A spot inside a building
    snaps to the nearest street; ANY other place is allowed, enemy base and GS/GF/JG areas included (owner:
    strategies and trolling friends into the GF). The GS/GF/JG units themselves are never carried.
    Cast (Claude's pick): the area is centred on Heimdall and moves with him during the horn; he walks at reduced
    speed, cannot shoot, and always travels with the group. The jump happens where he stands when the horn ends. Enemies see the marker
    and the arc 1 s before the jump (counterplay). The jump takes about 1.5 s along the arc and everyone lands
    together.
  - Range (provisional numbers): 40 m base, plus up to 60 m for each Bifrost within 30 m (closer = more).
    Next to the 3 bridges at the base it covers the whole map.
- Bifrost concept image from the owner (Gemini, turn 46): `SourceArt/Concepts/Bifrost_Conceito.png`. Three bridges
  start from round glowing pads at the base and end in big tube portals at the lane ends; flowing rainbow strands,
  smooth curves, cyberpunk Heimdall surfing in the inset. Owner wants it exactly like this: loose strands, riders
  look like they float (as in the films). No solid floor band.

### Heimdall - full kit draft (turn 46, Claude's picks from the owner's suggestions; all numbers provisional)
Lore used (public domain: Poetic Edda, Prose Edda, Rigsthula): watchman of the gods at Himinbjorg by the Bifrost;
needs less sleep than a bird; sees a hundred leagues by day or night; hears grass and wool growing; blows the
Gjallarhorn at Ragnarok; golden teeth ("Gullintanni"); golden-maned horse Gulltoppr; son of nine mothers (the
waves); fought Loki in seal form for the Brisingamen necklace; he and Loki are fated to kill each other.
- **Passive - Guardião da Ponte**:
  - Never falls off a Bifrost and can attack while riding (surf pose). On the Bifrost: -30% damage and range
    (so he does not camp on it).
  - Near a Bifrost but OFF it (within 15 m): 15% damage reduction; allies in his lane ride the Bifrost 20% faster
    and are not stunned when knocked off it.
  - *Ouve a grama crescer*: enemies moving within 20 m show on his minimap even behind walls (uses team vision).
- **LMB - Machados**: thrown axes in an arc, ranged (owner's other inspiration).
- **RMB - Cem Léguas**: aimed long throw, slower, more damage at long range; reveals the target for 3 s.
- **1 - Sentinela**: pulse that reveals every enemy within 40 m for 4 s (silhouettes through walls, on the
  minimap for the whole team).
- **2 - Gulltoppr**: short charge on a spectral golden-maned horse; passes through minions and knocks them aside.
- **3 - Destino de Ragnarök**: marks an enemy hero for 6 s; Heimdall deals +15% damage to that hero and takes
  -15% from it (the fated duel with Loki).
- **C - Chamado da Bifrost** (owner's supreme A, moved to his special key): raise the axe, charge, grab the
  first enemy hero (passes through minions), throw it up and back, the nearest Bifrost catches it and carries it
  helpless toward its base for 3 s (the full ride to the base was supreme power; C has a 40 s cooldown). The
  victim cannot take that same Bifrost for 8 s after. Hitting nobody = full cooldown.
- **4 - Gjallarhorn** (supreme, owner's option B): area on Heimdall that moves with him during the horn (1.5 s);
  he is immune to control and takes 30% less damage while blowing; everyone inside (allies, enemies, minions;
  never GS/GF/JG) jumps along a rainbow arc to the spot picked on the big map (anywhere outside buildings).
  Range: 40 m + up to 60 m per nearby Bifrost (base = whole map).

### Heimdall - kit v2 (turn 46, owner's new ideas; replaces the draft above where they differ)
- **Passive** adds: sees the silhouette of everyone riding ANY Bifrost, at any distance.
- **C - O Olho que Tudo Vê** (owner): long cooldown (~90 s). For 3 s every enemy is revealed on the team's
  minimap, and Heimdall sees all silhouettes through walls, all the way to the enemy base.
- **3 - Chamado da Bifrost** (the old C): 40 s cooldown, victim rides 3 s. Its ELEVATION makes the ride go all
  the way to the victim's base (the owner's original supreme A).
- Destino de Ragnarök becomes the elevation of Gulltoppr (the first hero hit is marked for the duel).
- Elevations (see the system below): LMB Machados -> bleed; RMB Cem Léguas -> heal cut (the "AC" stack);
  1 Sentinela -> also reveals enemies hidden in fog areas; 2 Gulltoppr -> Destino de Ragnarök mark;
  3 Chamado -> ride to the base; 4 Gjallarhorn -> allies land with a short shield.

## System idea: Elevação (turn 46, owner's idea; needs the owner's OK before code)
- During a match the hero gains levels (XP from minions, kills, assists and seals). Every few levels give one
  **elevation point**. Each ability (LMB, RMB, 1, 2, 3, C, 4) can be elevated ONCE, and an elevated ability gains a
  **second passive** (bleed, heal cut, reveal, longer effect...). A match ends with only part of the kit
  elevated, so players build differently.
- Data: one optional `Elevation` entry per ability in the hero Data Asset (text + effect), the owner fills them.

## Map idea: Neblinas (fog/smoke areas, like bushes) (turn 46, owner)
- Units inside are hidden from enemies outside (only seen within ~4 m), and see less themselves (sight cut
  to ~8 m). Fits the team vision system (`UValdoriaVisionSubsystem`). Heimdall's sight and Sentinela elevation
  reveal them.

## Filho de Nazaré - Tank/Support (turn 46, owner's idea; mythology/Gospel stories, public domain)
Approach (Claude's decision, owner agreed to let Claude decide): a **pacifist protector**. His power never kills:
his damage cannot take an enemy hero below 1 HP (assists only), and he wins fights by healing, protecting and
controlling. No crucifixion or sacrifice as a gameplay effect, no mockery, no weapon. White and earth-tone robe,
soft golden light. Rationale: strong identity, and much less chance of the backlash other games got for gods of
living religions (Smite, 2012). The owner keeps the final word on name, look and whether he ships.
- **Passive - Dar a Outra Face**: damage he takes builds Graça; at full Graça his next ability is stronger.
- **LMB - Luz**: short beam that pushes and slows enemies and heals allies it passes through.
- **RMB - Acalmar a Tempestade**: area that removes control effects from allies and makes enemy projectiles
  inside slower.
- **1 - Multiplicação dos Pães**: drops baskets; each ally who takes one heals over time (shared, limited).
- **2 - Caminhar sobre as Águas**: moves over gaps and water, ignores slows, short protection for himself.
- **3 - Cura do Cego**: target ally is cleansed and sees hidden enemies (fog/stealth) for 5 s.
- **C - Bem-aventuranças**: aura for 6 s, allies nearby take 20% less damage.
- **4 - Levanta-te, Lázaro** (supreme): revives the most recently fallen ally where they died, or, if nobody
  fell, a big heal plus a shield for all allies around him.

## Turn 46 - owner decisions round 2 (replaces earlier drafts where they differ)
### Levels and Elevação (owner)
- 20 levels per match. Each ability (LMB, RMB, 1, 2, 3, C, 4) can be upgraded up to 4 times: small damage/buff
  increase and slightly shorter cooldown.
- 2 elevation points: one at level 10, one at level 20. Each ability can be elevated only once. Elevations are the
  big changes: area control, single target becoming area, damage reduction while casting, heal cut, control
  immunity or damage immunity while casting, etc.
- Claude's proposal for the points: 1 upgrade point per level except 10 and 20 (they give elevation) = 18 upgrade
  points for 28 possible upgrades, so nobody maxes everything.
### Neblinas (owner, final rule)
- Inside a fog area you are fully invisible to everyone outside (except clairvoyance abilities: Heimdall etc.).
  To see someone inside you must enter; then both silhouettes show to each other. Sight inside is reduced but OK.
### Health scale (owner: raise HP a lot)
- Claude's proposal: multiply every HP, damage and heal by 10 (Kael 200 -> 2000, Brasa 300 -> 3000). Then
  0.05% of 2000 HP = 1 HP and the owner's percentages become whole numbers. To do as one batch with the tests.
### General health rules (owner)
- Out-of-combat regeneration: 1 HP every 0.5 s.
- Clearing a whole minion wave restores 10% of CURRENT health.
- Overheal only happens when health is full.
### Heimdall: owner keeps Claude's kit (Gjallarhorn supreme; Chamado da Bifrost throws onto the bridge, drops nearby,
  no return on that bridge for 8 s).
### Filho de Nazaré - Tank/Support (owner's kit; title: see below)
- **Passive - Deus do Velho Testamento**: an enemy at 5% HP or less hit by him is executed. Gold and XP go to the
  nearest ally; he gets a reduced share.
- **Cálice e Carne**: gives his own HP to heal an ally; an ally who drinks it and is out of combat regenerates
  3x faster until full.
- **4 - Crucificado** (supreme, max 3.5 s): immune to control and damage, rises into the sky healing allies a
  little (0.05% max HP every 0.5 s), and every 2 s a pulse heals 5% and removes control from allies. Allies under
  10% HP get double healing. While it lasts, a lethal hit only kills an ally if it dealt more than 30% of their
  max HP; otherwise they fall with 1 HP and take 1 s to stand up, immune to damage meanwhile (after that, any
  damage kills). At the end he dies and respawns at the base 3 s later. Presentation with reverence: light and
  ascension, no gore.
- **Elevation of the supreme - Levanta-te e Anda**: for 10 s after an ally falls, getting an assist on or killing
  that ally's killer revives the ally at the base.
### Titles
- Kay: **Harmônico** (owner's choice). Recruta: **Prelúdio de Valdoria**. Ariel: **Amálgama**.
- Claude's proposals: Heimdall **O Vigia**; Filho de Nazaré **O Prometido** (owner's options: O Crucificado or
  O Prometido).

## Turn 46 - round 3 (owner)
- Titles: Filho de Nazaré **O Prometido** (owner: less religious). Heimdall **O Onividente** (Claude's pick from the
  owner's options; his C stays "O Olho que Tudo Vê").
- Levels, CDR, vampirism vs life steal, V melee, hybrid damage: see `docs/RegrasCombate.md`.

### Filho de Nazaré "O Prometido" - Tank/Support, kit v2
- **Passive - Deus do Velho Testamento**: an enemy at 5% HP or less who takes his damage is executed. While an enemy
  is at <=5% and he is near, that enemy shows a blinking cross-like mark (so both know). Gold/XP go to the nearest
  ally, he gets a reduced share. On execution: the nearest ally within 12 m (owner: 10-15 m) heals 35% of missing
  HP; with nobody near, he gets it. Healing above full becomes a temporary green shield; while it lasts, 5% damage
  reduction. Second part (owner) - **Comunhão**: allies near him receive 10% of every heal he receives.
- **LMB - Luz** (Claude): short beam; light damage and slow to enemies, heals allies it passes through.
- **RMB - Acalmar a Tempestade** (Claude): area that removes control from allies and slows enemy projectiles.
- **V**: punch (every hero).
- **1 - Cálice e Carne** (owner): channel, he gives his own HP to an ally: 1% of his max HP every 0.25 s, stops at
  1% (he never kills himself), the player decides when to stop; he can move but not attack while channelling. The
  ally gets **Ungido de Sangue** for 7 s, renewed while still being healed: 10% damage reduction, 10% life steal,
  20% vampirism, 5% speed, 10% CDR. Out of combat the ally regenerates 3x faster to full.
- **2 - Multiplicação dos Pães** (Claude): drops baskets; each ally who takes one heals over time.
- **3 - Caminhar sobre as Águas** (Claude): crosses gaps, ignores slows, short damage reduction for himself.
- **C - Rogai por Nós** (owner): stands still / kneels with hands to the sky and recovers a good amount of HP.
- **4 - Crucificado** (owner): max 3.5 s, immune to control and damage, rises into the sky; heals allies 1.5% max HP
  every 0.5 s (double under 25% max HP); every 2 s a pulse heals 5% and removes control. A lethal hit only kills an
  ally if it dealt more than 30% of their max HP; otherwise they fall with 1 HP, 1 s to stand up, immune meanwhile
  (after that any damage kills). At the end he dies and respawns at the base after 3 s. Light and ascension, no gore.
- Elevations (Claude, except 4): 1 -> the ally is also cleansed; 2 -> baskets reveal hidden enemies nearby (fog,
  stealth); 3 -> allies near him also ignore slows; 4 -> **Levanta-te e Anda** (owner).
