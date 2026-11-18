# Referência de padrões de habilidades (hero shooters e MOBAs)

Resumo do texto que o dono trouxe (gerado pelo Gemini, 18/11/2026) com os padrões de habilidade
que se repetem em Overwatch, Valorant, Deadlock, Marvel Rivals, Smite e Paladins. Os jogos citados
servem **só como referência de mecânica**: nomes, visuais e kits do Echoes of Valdoria são originais.

Ao lado de cada padrão está a classe da nossa biblioteca (`Source/.../ValdoriaHeroAbilities.h`)
que já faz isso, ou "a fazer".

## 1. Mobilidade
| Padrão | O que é | Nossa biblioteca |
|---|---|---|
| Investida / dash | projeta o herói para frente ou na direção do movimento | Shift (impulso, até 5 cargas), `UValdoriaGA_Dash`, `UValdoriaGA_LungeSlash` |
| Teleporte | de A para B na hora, às vezes com marca prévia ou retorno | `UValdoriaGA_Teleport` (volta sozinho se Duration > 0 e SecondaryMagnitude = 1) |
| Salto / voo / escalada | vencer a altura (telhados da NP) | pulo múltiplo, escalar parede, subir beirada, planar, voo por combustível ou habilidade |
| Gancho | puxa o herói até um ponto do cenário | `UValdoriaGA_GrappleSelf` |
| Ir até / puxar aliado | dash, puxão ou teleporte ligado a um aliado | `UValdoriaGA_AllyLink` (0 = vai, 1 = puxa, 2 = teleporta) |
| Rastro para deslizar | cria um caminho (gelo, tinta, luz) que acelera | `UValdoriaGA_SlideTrail` (rampa no ar: a fazer) |
| Portal | liga dois pontos; só time, todos ou inimigos também | a fazer |

## 2. Dano e ofensiva
| Padrão | Nossa biblioteca |
|---|---|
| Projéteis e explosões | `UValdoriaGA_FireProjectile`, `UValdoriaGA_ThrowZone` |
| Dano contínuo (fogo, veneno, sangramento) | zona de dano por segundo (`AValdoriaZone`); DoT preso no alvo: a fazer |
| Corpo a corpo (às vezes com atordoamento) | `UValdoriaGA_FireMelee`, `UValdoriaGA_GroundSlam` (+ `ApplyStatus`) |
| Feixe contínuo que trava no alvo, dano por segundo e cura de aliado | a fazer (5º herói) |
| Raio que atravessa | `UValdoriaGA_PiercingBolt` |

## 3. Controle de grupo
| Padrão | Nossa biblioteca |
|---|---|
| Atordoar, silenciar, enraizar | `ApplyStatus(Stunned / Silenced / Rooted)` |
| Lentidão | `ApplySlow` |
| Empurrar / puxar inimigo | `UValdoriaGA_ConeBlast`, `UValdoriaGA_HookPull`, ultimate da Brasa |

## 4. Defesa e sobrevivência
| Padrão | Nossa biblioteca |
|---|---|
| Barreira (bloqueia tiros inimigos, aliados atiram através) | `UValdoriaGA_Barrier` |
| Absorver / aparar / refletir | a fazer (refletir projéteis, absorver dano virando escudo) |
| Invencível ou intangível por pouco tempo | a fazer (intangível: imune a físico, mágico acerta, `CannotAttack` em si) |
| Proteger aliados por perto | passiva da Brasa (`AllyDamageShare`, `DamageReductionPerAlly`) |

## 5. Utilidade e suporte
| Padrão | Nossa biblioteca |
|---|---|
| Cura (projétil, área, aura, feixe) | orbe da Iara (`AllyHeal`), zonas, `AuraHealPerSecond`, `UValdoriaGA_HealPulse` |
| Revelar inimigos através da parede | a fazer |
| Buffs e debuffs (velocidade, dano, resistência) | `ApplySpeedBuff`, `UValdoriaGA_Stim`; dano/resistência: a fazer junto com a loja |
| Invisibilidade parcial | `UValdoriaGA_Stealth` (aparece a menos de Radius cm de um inimigo) |

## 6. Ideias de sistema citadas
- Habilidades em dupla entre heróis específicos (combinações de time).
- Itens comprados durante a partida que fazem as habilidades crescerem (nossa loja no B).
- Cartas ou talentos que mudam o comportamento de uma habilidade (ex.: mobilidade vira ataque).
- Heróis com poucos itens e poder constante versus heróis que escalam com a partida.
