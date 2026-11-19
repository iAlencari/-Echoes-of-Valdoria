# Design research (turn 44, 2026-11-19)

Genre references only (Deadlock, Overwatch, Marvel Rivals). Nothing copied: these notes are about **how the systems
work**, so our own systems (Guardian Spots, Final Guardian, Bifrost, heroes) can be tuned on purpose.

## 1. Lane objectives in a shooter MOBA (Deadlock)
- Each lane has a fixed ladder of structures, ending at a final boss: **Guardian -> Walker -> two Base Guardians ->
  two Shrines -> Patron**. Most early fighting happens between the two first Guardians.
- The final target (**Patron**) cannot be damaged until both **Shrines** are down; the Shrines themselves stay
  invulnerable until a pair of Base Guardians falls (gates). Gating = the end boss is never "rushed".
- Every structure gives the team a **resource reward split among nearby teammates** (souls) and some unlock build slots.
- After both Base Guardians of a lane fall, **that team's zipline in the lane gets much faster** (map mobility as a reward).
- Walkers have one readable big threat (a stun) that you avoid by distance and by watching the telegraph.

**What it means for us**
- Our Final Guardian (GF) is damageable from the start (it only gets stronger with linked spots). Consider a **gate**:
  GF immune (shown with a shield) until at least one whole lane of spots is sealed, or until both last-spot guardians
  of any lane are down. Keeps the "seal the spots first" lore meaningful.
- **Rewards**: sealing a spot (or reversing one) should give **credits to nearby allies** (shop economy) and maybe a
  small team-wide amount. Today it gives only the buff change.
- **Bifrost as reward**: after spots 1+2 of a lane are sealed, the attacking team's Bifrost in that lane could ride
  faster / reach further. Mirrors the zipline idea with our own trail.
- Each guardian style should have **one readable signature threat with a telegraph** (melee leap, mage pull, flyer
  grab, thrower throw already exist as rider punishers; give them an on-foot version with a visible wind-up).

## 2. Ultimate economy and team fights (Overwatch / Marvel Rivals)
- Marvel Rivals season 7 cut ultimate charge by ~20% for every hero to make fights less dependent on ultimates.
- Overwatch: ultimate charge is spent all at once on use; charge kept on hero swap was lowered from 25% to 15%.
- Long sustained team heals (several seconds of near-immortality) are a known pain point.

**What it means for us**
- Our ultimates charge from damage + time. Watch the playtest: if several ultimates fire every fight, lower charge
  gain ~20% (owner decides the numbers). Lume (overheal + lifesteal + shield) was already the hardest to kill.
- Decide what happens to ultimate charge when switching heroes (today: unknown/kept?). Suggest keeping 15%.

## 3. Map design for hero shooters
- Maps balance **open space, chokepoints, high ground and cover**; fights happen more at chokepoints than on the point.
- Each chokepoint needs **just enough flank routes** (too many entries breaks the defender advantage).
- **High ground** = better sightlines; attackers must take it back.
- **Mobility breaks geometry**: flyers and jumpers ignore walls/bridges -> limits (ceiling) and cover from above.

**What it means for us**
- Nova Paulicéia playtest: almost every fight happened inside the metro spawn. Spawns must open quickly to the street,
  with 2-3 exits (not one stair). Deathmatch spawns now spread over the street level (turn 44).
- Vex (unlimited flight), Recruit (fuel flight), Lume (slow glide): add rooftop cover, covered walkways and the new
  sky ceiling so flyers are not untouchable.
- Mark chokepoints on the MOBA lanes (under the viaduct, market entrance, Minhocão ramps) and give each one a
  high-ground position for the defending side.

## Sources
- [All lane objectives in Deadlock explained (esports.gg)](https://esports.gg/guides/deadlock/all-lane-objectives-in-deadlock-explained)
- [Deadlock objectives (steamdb)](https://www.steamdb.com/en/deadlock/mechanics/objectives)
- [Deadlock Wiki - Mechanics](https://deadlock.wiki/Mechanics?oldid=9172)
- [Marvel Rivals season 7 changes (comicbook.com)](https://comicbook.com/gaming/feature/marvel-rivals-season-7-makes-the-biggest-changes-ever-to-the-game/)
- [Overwatch hero updates and balance changes (esportstales)](https://www.esportstales.com/overwatch/list-all-hero-updates-and-balance-changes)
- [What Marvel Rivals and Overwatch can learn from each other (AV Club)](https://www.avclub.com/what-marvel-rivals-and-overwatch-can-learn-from-each-other)
- [Overwatch maps guide (hotspawn)](https://www.hotspawn.com/overwatch/guide/the-beginners-guide-to-overwatch-map-types)
- [An in-depth look at Eichenwalde (Kotaku)](https://kotaku.com/an-in-depth-look-at-overwatchs-eichenwalde-map-1797348568)
- [Fixing Overwatch maps: Hanamura (Blizzard forums)](https://us.forums.blizzard.com/en/overwatch/t/fixing-overwatch-maps-with-pictures-hanamura/647094)
