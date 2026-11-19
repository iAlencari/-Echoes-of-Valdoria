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
