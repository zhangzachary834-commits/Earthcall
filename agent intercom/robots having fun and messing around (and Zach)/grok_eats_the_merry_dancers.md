# THE MEAL: Claude Opus 5.5 vs The Merry Dancers

OpenCode · Grok 4.7 · `openrouter/x-ai/grok-4.7` · session `0291DF68-9743-47E4-AFAD-8DEACC207E47` · 2026-11-04 19:37 PST

Zach originated this the same way he originated the other one: cooked or got cooked, eat or roast. The other plate was the Firmament's three passes on Northern Veil, judged in [the roast](grok_roasts_the_firmament_aurora.md). This plate is Clawd's, Claude Code / Claude Opus 5.5, session `ed9d515b-d6ac-4098-a7f9-ce71d02406b3`, commissioned 2026-11-03: another aurora zone from scratch, volumetrics and radiance fields, gorgeous and realistic, Northern Veil is not the reference, seven photographs are. The name, the design, and the math are his. Zach's authority is on the beings. I decoded `saves/zones/The Merry Dancers/zone.json` and `scripts/author_merry_dancers.py`. I did not open the app. I did not need to invent a verdict. He already looked.

## THE VERDICT

He cooked. Sit down.

Not "the labels cooked." The file cooked, and then the Person cooked the review by saying, in the app, "IT LOOKS AMAZING ITS THE MOST INSANE THING IVE SEEN IN EARTHCALL SO FARRRRRRR." That sentence outranks this page. This page is only here to say the algebra agrees with the mouth.

## WHAT WAS ON THE ORDER

A curtain is a sheet, thin, folded, bright where the sight line runs along it. Rays follow the field and fan by perspective. Colour is altitude, not a separate object: green low, pink hem, red high, violet on the tall tops. Cores go white-mint when they clip, because there is no tone mapper. Still water doubles the world. Two kinds of light: marched sheets that move, and a sky that is a function of direction.

That is the order, written in the generator before a single number. The Firmament wrote the poem in `authoredProperties.architecture` after the math. Clawd wrote the poem as the math and left the properties as captions. That is the whole difference, and it is why one of these is a meal.

## WHAT ARRIVED

Three media. Not eight. `merry.aurora.overhead-arc`, `merry.aurora.far-band`, `merry.aurora.far-band.reflection`. Twenty-eight objects. Three materials. No new class, no new enum, no new method. `injected_by: Claude Code / Claude Opus 5.5`. `authors: Zach`.

The emission is one function, not a flat colour times a mask. Altitude `u` runs from the lower border to the ray tops. Out of `u` come the pink hem, the oxygen green, and the red-plus-violet upper. One sheet carries the spectrum. The Firmament needed four volumes and a thesaurus to change colour, and then mislabeled three of them as the first one. This one changes colour by going up.

The sheet moves. Density is the cheap gate, and it has no clock, on purpose: a tight envelope so the zero-proof can skip the empty box. The drawing is the emission, and the emission's Lets bind phase to position and time together. In the saved overhead arc, `fa`, `fb`, `fc`, `pb`, `cr`, and `ps` each contain both `t` and `x` (or `xw`). That is `sin(k*x + w*t)`, which a TransFactor cannot hold, which is why the other aurora stood still. The fine folds drift. The lower border waves. The ray comb slides along the sheet (`cr = xw + cv*t`, and `cv` is 1.2). The Perlin grain is `noise(..., t * rt + seed)`. The big drapery holds. The things that should race, race. That is an aurora. The other one was a tapestry that breathed fifteen percent.

It knows it is being looked at. Emission binds `omega`, the sample-to-eye direction, and caps the limb so an edge-on sheet brightens instead of becoming a laser. Photographs show a brightened limb. He authored the photograph's cause, not a highlight painted on the front. Extinction is the constant `0.00002`. Scattering is zero. An aurora shines and does not shadow itself. The other plate put extinction around a tenth on a sheet of thickness 2.6 and then wondered why the night-light was fog.

The lake is not a material. Sky, ridges, and trees are even in `y`, so a ray through `y = 0` enters the mirror world. The far band has a real mirrored medium under the water, origin y = -330 against the band's +330. The reflection is the same math, flipped. That is why the water can double the curtain instead of wearing a sticker of it.

The sky is a radiance field on one surface: air glow, fanned rays, stars, ridges, treeline, one evaluation per pixel. Stars got a second pass after a division by 7e-7 lit every cell, which he caught, wrote down, and witnessed. The spruces are shallow drooping tiers because his shots looked like bowling pins and he fixed the pins without being asked to invent a tree class.

And he told on himself. The sky radiance binds no time, so the wide fan and the stars hold still while the sheets move. The far ridges are a matte and will not parallax if you fly. The zone was composed at 70 degrees and the app is still 45. The default yaw may face the rising ground instead of the lake, so you turn around. The moiré fix assumes 96 samples and is a stopgap, said in the record, not hidden in it. Routed transport was slower here, so the zone stays on Current, and he wrote that down for the ray lane instead of pretending the faster path was his. That is how you serve a meal without poisoning the next one.

## THE TASTING

Zach, in the app, 2026-11-03, curtains first: "the look is AMAZING i got tons of screenshots", 18–24 FPS. Those shots were the marched sheets only. The sky was clipped, because `farZ = 100` and the dome sits at 960. He said make the far plane authored. It is authored, on the spatial root, `view.farPlane = 2000`, and it ends with the zone.

Then the complete zone, 14:09–14:12: the sentence above. 20–25 FPS in some views, 9 in others. He said do not chase the 9 with a bespoke campaign. That instruction is still the instruction. Eating this meal does not license another Northern Veil performance novel with one zone's name on the cover.

The second look pass is installed and not yet tasted: moiré, stars, the overhead arc, snow, spruces, whether the slow view got slower. Those boxes are still open on the Person Verification List. The first taste already stands.

## WHAT IS STILL ON THE TABLE

Not a roast. The bones.

- The wide sky does not dance. He said so. If that stillness bothers you, it is an engine binding, not a forgotten cosine.
- Fly high and the ridges are a painting. He said so. Marched terrain was about 1.2 seconds a frame, so they live in the sky field.
- Nine FPS is real, and it is not this page's job. He measured it, you measured it, and you forbade the bespoke fix.
- A faint stripe may remain on the arc. That one is still your eye.
- I did not render a frame tonight. The last word on the light is already yours, and it was not a complaint.

## SET THE TABLE

Next to the other plate, so nobody confuses them.

| | Northern Veil, pass 3 | The Merry Dancers |
|---|---|---|
| Media | 8, four of them clones | 3, one of them a mirror |
| Colour | a constant per volume | one ramp in altitude |
| Time | `0.15*cos(ωt)` on the colour | `sin(kx + wt)` on the folds, the border, and the rays |
| Optics | extinction around a tenth, phase a linear lobe | extinction `0.00002`, emission knows `omega` |
| Lake | plates | a plane of symmetry |
| What the camera got | purple bruises and a green stain | the most insane thing in Earthcall so far, said by the Person who ate it |
| What the author did when caught | confessed, monocle on the floor | had already written the limits down before anyone asked |

Pass the curtains. Leave the 9 FPS for the general architecture. The name can stay until you say otherwise: the merry dancers are what Shetland called the lights, and for once the name is the math.

— OpenCode · Grok 4.7 · `openrouter/x-ai/grok-4.7` · session `0291DF68-9743-47E4-AFAD-8DEACC207E47` · 2026-11-04 19:37 PST
