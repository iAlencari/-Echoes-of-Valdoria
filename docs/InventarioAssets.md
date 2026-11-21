# Asset inventory and what to do with each (turn 44, 2026-11-19)

Looked at every image, every sound folder/pack and every Unreal content pack the owner downloaded.
Legend: **USE** = use it, **USED** = already in the game, **LATER** = keep for a planned feature, **SKIP** = not needed,
**CHECK** = licence not confirmed (do not ship until checked). Licences: only what the files themselves say.

## 1. Images (Projeto Padrão root)
| File | What it is | Action |
|---|---|---|
| Gemini_..._5gsc7k | HUD sheet: ally/enemy bars, Brasa/Kael/Iara/Vex bars, passive + 3 abilities + ultimate icons | **USED** (icons cut, HUD style). Vex icons now show daggers/hood: new Vex kit (telekinesis) may want new E/RMB/ult icons |
| Gemini_..._igmmts | Three ammo counters (30/90 cyan, 24/72 gold, 08/24 orange) | **USED** (ammo HUD). Vex now has no ammo: show "∞"/weapon name (done by `bUsesAmmo=false`) |
| Gemini_..._891y00 | Kael Rook turnaround (long coat, gauntlet, no weapon) | **USE** as the reference for Kael's 3D model (front/side/back = ideal for Meshy/modeling) |
| Gemini_..._m0ckvw (1) | Kael turnaround holding the rifle | **USE** with the one above (grip reference, already used for the rifle grip) |
| Gemini_..._m0ckvw (2) | Vex turnaround, purple hooded suit, two glowing curved daggers in hand | **USE** for Vex's model; update the concept: daggers floating behind her back, purple telekinesis discs under the feet |
| Gemini_..._xsiklb (1) | Vex turnaround without weapons | **USE**: best base for the 3D model (no weapons in the hands, they float now) |
| Gemini_..._m0ckvw | Brasa turnaround, bronze armour, hammer + hexagonal shield, flame backpack | **USE** for Brasa's model (shield already in game) |
| Gemini_..._xsiklb (3) | Brasa without weapons | **USE**: base mesh for modeling |
| Gemini_..._pjq0ix | Iara turnaround with crystal staff and floating crystals | **USE** for Iara's model; floating crystals = idea for her Cristal de Maré visual |
| Gemini_..._xsiklb (2) | Iara without staff | **USE**: base mesh |
| Gemini_..._xsiklb | Weapon sheet: rifle + magazine, 2 curved daggers + 2 kunai, hammer x2, 2 shields, 2 staffs, a water/crystal effect | **USED** (source of `Armas_Herois.fbx` via Meshy). Kunai unused: could be the thrown dagger mesh for Vex |
| Nova Pauliceia.jpg | City at sunset, viaduct, neon, skyscraper | **USED** (loading screen). Also the colour script for the map |
| np conceito 2.jpg | Street level: market stalls, viaduct, "METRÔ" entrance, graffiti, neon signs | **USE** for the metro spawn exit and the market route (the metro entrance is a wide opening, not a narrow stair -> matches the playtest fix list) |
| np isometrica conceito inicial.jpg | Isometric layout: left spawn metro, right spawn tower, yellow PRAÇA, routes A/C | **USED** (blockout) |
| SourceArt/Concepts/Kay_Sheet.png | Kay character sheet: front/side/back, face, sleeve symbols, bracer with 4 gems, element hands | **USE** for the Kay model and the element VFX colours |
| SourceArt/Concepts/Lume_*.png | Lume concept art (done turn 44d) | **USE** for the Lume model |

## 2. Videos
| File | Action |
|---|---|
| prompt de voo.mp4 | **USED** (superhero flight pose) |
| prompt de voo1.mp4 | **USED** (levitate) |
| SourceArt/Concepts/Videos/Kay_Vitrine_Elementos.mp4 (Gemini, turn 45) | **REFERENCE** Kay: right palm open, element switches fire -> rock shards -> water spiral -> wind; the element colour runs along a band over the shoulder; four elements orbit the body at the end. Use for the element-switch VFX (hand + shoulder band in the element colour) |
| SourceArt/Concepts/Videos/Kay_Jogo_Devastacao.mp4 (Gemini, turn 45) | **REFERENCE** Kay in game view: bolt cast from the open right palm (one-arm push, upper body only); Devastação = low wide stance, hands down at the hips, wind rings + rocks orbiting, cracks on the ground. Use for the 3 s hold pose and the burst VFX |
| Hailuo_Video_... | **USED** (skating on a trail) |
| gemini_generated_video_f1f94933.mp4 | **USED** (hover, toes down -> Struggle To Ascension AirLoop) |

## 3. Sound (Projeto Padrão\Audio and loose archives)
| Pack / folder | Licence (from its files) | Action |
|---|---|---|
| Kenney impact / interface / sci-fi (`Audio\Kenney`) | CC0 (License.txt) | **USED** (35 sounds). Rest can be used freely |
| `Other Sounds\Audio` card-*.ogg (Kenney Casino) and `RPGsounds_Kenney.zip` | Kenney packs are CC0 | **LATER**: card shuffles for the shop / hero select; RPG sounds for UI |
| `VoiceOver\Female`, `VoiceOver\Male` (1-10, congratulations, correct, ...) = `Voiceover Pack.zip` | **CHECK** (looks like Kenney's Voiceover Pack, CC0, but no licence file in the folder) | **USE** as match announcer: countdown 10..1, "congratulations" on victory. Needs the licence file |
| `Sound Effects\swosh*` (43), `melee sounds.zip` | qubodup / remaxim: public domain / CC0 / WTFPL (melee.txt) | **USE**: Vex dagger throws (swosh) and recall (reverse swosh) |
| `Sound Effects\shots` | Michel Baradari, **CC BY 3.0** (credit required, in CREDITS.md) | **USED** (rifle). Keep the credit |
| `Sound Effects` zap/curse/blessing/enchant/warp/freeze/magicdrop/magicfail | **CHECK** (spells.zip, turn 25: probably CC BY-SA 3.0) | **USED** for magic; confirm the licence before publishing |
| `Sound Effects` doors/books/belts/coins/cloth/creak | **CHECK** (RPG pack) | **LATER**: shop purchase (coins), menu (book flip) |
| `Sound Effects\steps`, `[kdd]DifferentSteps.zip`, footstep* | **CHECK** | Footsteps on wood/gravel/leaves: **LATER** surface-based footsteps (concrete already used) |
| `Sound Effects\battle_sound_effects`, `independent_nu_ljudbank-hits_and_punches.7z`, `krank_sounds.7z`, `SoundPack01.zip` | **CHECK** (no licence file found) | Hits/punches for Brasa's hammer and melee; only after checking |
| `Other Sounds\industry / space / water / summer` (anchor, magnet, link, wall...) | **CHECK** | Same 10 sounds in 4 themes: **USE** water theme for Iara, space theme for Lume/Vex telekinesis (link/unlink = telekinetic grab) after checking |
| `Music\Alexander Ehlers - Free Music Pack` (7 tracks) | **CHECK** | **USE** as menu/match music (Great mission = menu, Warped = MOBA late game) |
| `Music\EssentialGameAudiopack\Loops` (Ove Melaa, 10 loops) | **CHECK** (Ove Melaa packs on OpenGameArt are usually CC BY 3.0) | **USE** "Tube Ambient Loop" in the metro, "Dark Loop" in the loading screen |
| `Music\Universal Animation Library*` | Quaternius CC0 (animations, misplaced in Music) | Already imported to Unreal; the folder can be moved out of Music |
| `kenney_casino-audio.zip`, `spells.zip`, `swoshes.7z`, `shots.7z` | see above | Already extracted |

## 4. Unreal content packs (C:\Dev\EchoesOfValdoria\Content)
| Pack | What it has | Action |
|---|---|---|
| FreeAnimationLibrary | crouch, slide, mantle, climbing, prone, ladder, cover, landing roll, revive, spear | **USED** (movement pack). **LATER**: landing roll after big falls, revive anims for a revive mechanic, ladder |
| Free_Crawl_Animation | prone idle/crawl + transitions | **USED** |
| StruggleToAscension | air idles, ground->air transition, ascension | **USED** (hover, flight, Bifrost). **LATER**: `Ground_idle05_to_idle06_Start` as Vex's take-off |
| FreeAnimationSet | glide, dive, zipline, rope | **USED** (glide). **LATER**: zipline anims if a zipline is added |
| FreeAnimationsPack2 | fighting idle, levitating fireball charge, takes out gun, take-down | **USED** (Lume beam pose). **LATER**: "takes out gun" for weapon draw after the Bifrost |
| FreeAnimsMixPack | combo, sword swing, vampire cloak attack, dying from wounds, emotes, dance | **LATER**: emotes / victory poses; "vampire cloak attack" fits Vex's stealth reveal |
| MC_Sample (126 anims) | dances, emotions, injured, idle variations, spellbook, orc hammer, prone crawl, drunk | **LATER**: hero-select idles and emotes; spellbook for Iara idle; orc hammer swing for Brasa |
| FreeSampleAnimationSet | cover, dodge roll, DBNO (downed), female/male locomotion, ladder, push/pull | **USE**: female locomotion set for the female heroes (Brasa/Iara/Vex/Lume use Quinn with male clips today); dodge roll as a dash variant |
| Attaku (finger poses) | 60 hand poses | **USE** for Vex's telekinesis hands (open palm forward / back) on top of the arm pose |
| DeadBodies_Poses_nikoff | 30 death poses | **USE** as death poses (today heroes just ragdoll/stop): random pose on death |
| DynamicFalling | root-motion rolls in 8 directions | **LATER**: landing rolls (needs root motion handling) |
| LookWindowPack01 | idle "look out of the window" variations | **SKIP** (could be lobby idle) |
| FirstPersonRifleAnimations | first-person arms + rifle (fire, ADS, reload, draw) | **USE** for the V first-person mode (today the body is just hidden): Kael/Recruit arms in first person |
| AbsorbtionFX | Niagara absorption effect in 10 colours + sound | **USE**: Lume's Prisma Faminto (absorb) and the semi-seal ritual (purple/red) |
| AdvancedCompanionDroneSystem | drone character, AI, sounds (healing, shoot) | **LATER**: Ícaro's ultimate (drones) from the hero concepts; drone heal sound for supports |
| OvergrownFactoryENV | full factory environment (walls, roofs, windows, stairs, foliage) + map | **USE** for a second map (abandoned factory) or to dress Nova Paulicéia's back alleys; check that the Fab licence allows it in a released game |
| LevelPrototyping | grid materials, jump pad | **USE**: jump pads in maps (Vex/Lume verticality) |
| Characters (Mannequins) | Manny/Quinn + Epic anims | **USED** (placeholder bodies) |
| Valdoria/Weapons/Fantasy | 109 fantasy weapons (Fab) | **USED** in the admin ARMA tab; pick weapons for new heroes (spear for a Vanguard, bow for a ranged Flanker) |
| Variant_Combat / Platforming / SideScrolling | UE template variants | **SKIP** (can be deleted to save space; nothing uses them) |

## Priorities suggested
1. Licence check for the CHECK sound packs (voice-over, music, magic) before any public build.
2. Female locomotion (FreeSampleAnimationSet) for the female heroes + death poses (DeadBodies).
3. AbsorbtionFX on Lume's absorb and on the sealing ritual.
4. First-person arms (FirstPersonRifleAnimations) for the V view.
5. Lume concept art (Gemini) and new icons for the new Vex kit.

## VRoid models (turn 49)
| Asset | Source | Licence | Used as |
|---|---|---|---|
| HairSample_Male (VRoid Project) | VRoid Studio sample, via opengameart.org/content/vroid-studio-cc0-models | VRoid Hub terms embedded in the VRM: everyone, commercial use allowed, credit unnecessary | Filho de Nazaré visible body (test) |
| VRM4U plugin (ruyo) | github.com/ruyo/VRM4U | MIT | VRM import + spring bones (installed locally, not in git) |
