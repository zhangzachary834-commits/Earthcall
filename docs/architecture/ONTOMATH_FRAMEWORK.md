# OntoMath Framework

The Earthcall ontology separates the underlying mathematical rules of reality from the engine logic that executes them. `OntoMath` is the multi-class module that encapsulates pure mathematics, separating mathematical constructs from any particular rendering framework or API like WebGPU or Audio.

## 1. Why OntoMath?

In most traditional game engines, mathematical behaviors like noise fields, velocity, or volumetric densities are tightly coupled with the physics engine or the rendering shaders. In Earthcall, this coupling is an ontological violation. What something *is* mathematically must be distinct from how a *specific channel* renders it.

If a `FieldNode` in Geometry needs a procedural density map, it does not define a custom WebGPU node. Instead, it maintains a reference to an `OntoMath::ScalarField` or `OntoMath::VectorField`. The WebGPU modality compiles the WGSL shader by reading the `OntoMath` structures. The Physics modality reads the exact same `OntoMath` structures to apply wind or gravity.

## 2. Representation: The `MathNode` AST and `Piecewise` Functions

The foundation of `OntoMath` is exact symbolic math, avoiding approximations where possible.

### `ScalarForm`
`ScalarForm` represents a continuous algebraic expression: a sum of terms where a term is `coefficient * Π var^exp * Π trans(scale·var + shift)`. Within it, algebra (+, ×) and basic calculus (derivatives) are exact.

### `MathNode` (AST)
When we need complex vector calculations (Dot products, Cross products) or structured combinations of scalars, we construct a tree of `MathNode`s. 

Supported Operations:
- `ScalarLeaf`: Contains a `ScalarForm`.
- `VectorConstruct`: Builds a vector from scalar children.
- Math ops: `Add`, `Sub`, `Scale`, `Dot`, `Cross`, `Normalize`, `Length`, `Map`.

### `Piecewise`
Mathematical laws in reality often only apply under certain conditions (e.g., gravity changes beyond the atmosphere). `Piecewise` functions define the interval or *condition* bounds where a specific `MathNode` AST applies.

## 3. Shader Integration (The Dual Path)

`OntoMath` is designed to power the Law system. When generating highly parallelized code like WGSL for WebGPU, we rely on two paths based on the `Field`'s `EvaluationMode`:

1. **Path A (Procedural/Hardcoded):**
   The field's properties (like `baseDensity` or `frequency`) are bound as dynamic uniform variables in the shader. The Law system can modify these properties via `PropertyPath` in real-time without recompiling the shader. This is ideal for continuous modulation.

2. **Path B (AST Compilation):**
   If the `Field` requires a custom mathematical equation authored by a Person via a Law, the `mode` is set to `AST`. The shader compiler (`SdfWgsl.cpp`) traverses the `OntoMath::Piecewise` tree and transpiles it into literal WGSL string operations. Changing the AST triggers a shader recompile, providing infinite mathematical flexibility.

## 4. Sub-Modules of OntoMath
- `ScalarForm`: Multivariate signomial algebra and exact calculus.
- `Field`: Continuous Scalar and Vector fields.
- `CurveModel`: 1D time-based parameter curves.
- `ProbabilityForm`: Stochastic modeling.

By centralizing these in `OntoMath`, Earthcall ensures that any mathematical law authored by a `Person` can be reliably executed by any sub-system.

## 5. Declared but not implemented

`MathNode::Op::Raycast` (16) and `LineIntegral` (19) exist in the append-only op
enum and nowhere else. Both need machinery nothing in this tree authors — a marching
budget for one, a curve parameterization and quadrature rule for the other — so both
evaluate to `nullopt` on the CPU and **refuse to compile** to WGSL rather than emitting
something that merely looks like an answer. The enum values are spent and stay spent;
implementing them later needs no renumbering.

## 6. Exact integration, and reading the world backwards

`ScalarForm::derivative` had no counterpart above the term level: `antiderivative` existed
on a `ScalarForm` but nothing integrated a `MathNode` tree or a `Piecewise` model. Those
are now in `ScalarForm.hpp` as free functions:

| Function | Answers |
|---|---|
| `toScalarForm(node, why)` | the linear-multiplicative subset of an AST (leaves, `Add`, `Sub`, `Scale`) flattened into one exact `ScalarForm` |
| `antiderivative(node, var, why)` | ∫ node d(var), via the above plus the audited power/product rules |
| `integrable(model, var, why)` | can ∫ model d(var) be held in closed form — a judgement on the **text**, nothing bound, nothing run |
| `definiteIntegral(model, var, a, b, vars, why)` | ∫[a,b], honouring the piecewise structure with the same first-applicable-piece-wins rule `evaluate` uses |

**Why this matters beyond arithmetic.** A law's `Flow` action authors dp/dt. Integrating
that rate in closed form gives the past *without a log*:

```
p(t − Δ) = p(t) − ∫[t−Δ, t] dp/dt
```

Nothing is recorded and nothing is stored. The law text is the record, and the world can
be read backwards from it exactly. `ActionNode::valueSecondsAgo(subject, secondsAgo)` does
this; `ActionNode::reversibility()` decides in advance whether it can, from the law's text
alone.

**The refusals are the load-bearing half.** Every one of these is a stretch of world that
is genuinely irreversible, and the substrate says so with the reason attached rather than
approximating:

| Obstacle | Why it is real |
|---|---|
| `Set` | the value it overwrote is not in the law text |
| `Add` / `Scale` / `Lerp` | invertible per firing, but the text does not record how many times it fired |
| a rate needing integration by parts | the algebra does not hold it, and will not guess |
| a piece carrying a **world guard** | answering whether it applied then needs the past being computed |
| a piece whose value is a **fold** or a **function call** | reads world state whose past is not in the text |
| a rate that reads what it writes | that is a differential equation, not a quadrature |
| `Destroy` / `Create` / `Spawn` | annihilation and birth are not quantities to integrate |
| bounds cutting a variable other than the integration variable | which piece governed is not decidable here |

`Map` is reversible under a **weaker** condition than `Flow`: it only re-evaluates F at
t−Δ, so it needs no antiderivative at all. `t·sin t` is a reversible `Map` and an
irreversible `Flow`. `Drive` reverses too — a `CurveModel` is symbolic — but only when its
input is the clock; driven by anything else, the past of the *input* is the unknown.

**Scope, stated exactly.** `reversibility()` judges whether one action's own writes can be
carried backwards. It cannot see another law writing the same property; that is a
Zone-level question, answered by folding it over the Zone's whole law set. **That fold is
the irreversibility map**: a Zone that can say which of its regions can be undone and
which cannot, and why — which no engine that reverses by replaying a log can say at all.

**One caveat worth knowing.** `Flow` integrates numerically at runtime (Euler, one step
per tick). The reversal is the *exact continuum* answer, so it recovers what the authored
mathematics says the past was, not the accumulated floating-point path the simulation
actually walked. The two converge as dt shrinks, and their difference is a free measure of
the integrator's own drift.

Verified by `tests/law_reversal_test.cpp`.

## 7. The audio channel reads OntoMath

§1 says the WebGPU modality compiles the WGSL shader *by reading the OntoMath structures*,
and the physics modality reads the same ones. The audio channel now does too:

```cpp
Core::Audio::renderForm(form, timeVariable, seconds, sampleRate,
                        constants, &undefinedSamples, &clampedSamples);
```

The authored expression **is** the waveform, sampled. There is no frequency parameter, no
wave-type preset, and no envelope in the path — those would be the channel deciding what
the sound is, which is exactly what a modality channel may not do. `AudioSystem::playForm`
sounds it through miniaudio; `renderForm` itself is pure, takes no device, and is
therefore testable.

This is what makes "a building you can hear" a structural claim rather than a metaphor. A
`ScalarField::astDefinition` handed to the raymarcher is a density in space; the same
`Piecewise` handed to `renderForm` is a pressure wave in time. The vault and its chord are
not a resemblance between two artifacts — they are one text, read twice, by two channels
neither of which is allowed to know what it means.

Undefined samples — outside every authored piece — are silent **and counted**, reported in
the `SoundingReport` rather than passed off as zero. The silence is a hole in the domain,
not part of the sound.

### 7a. The infrasound floor — a Kernel boundary on this channel

Letting a Person author an arbitrary waveform means letting them author 7 Hz at full
amplitude. Below roughly 20 Hz a signal stops being heard and starts being *felt*:
sustained high-energy infrasound is associated with nausea, disorientation and chest
pressure; it drives loudspeaker cones toward their excursion limits with no audible
warning; and 0 Hz — a DC offset — is not a sound at all but a constant displacement held
against the driver and the ear.

The manifesto's first Kernel boundary is that nothing may violate fundamental Person
guards, and a Person's body is on the other side of this channel. So the floor is enforced
in the channel itself and is deliberately **unconditional** (no parameter or flag lifts
it), **unauthorable** (no Law or Zone can reach it, because it is not expressed as law text
at all), and a **refusal rather than a filter** — `renderForm` returns an empty buffer and
says which frequency it refused, instead of silently high-passing a Person's mathematics
into something they did not write.

It is enforced twice, because one instrument cannot see everything:

| Pass | Sees | How |
|---|---|---|
| **Symbolic** — on the text, before a sample exists | authored sinusoids, exactly | a `TransFactor` is `kind(scale·var + shift)` and `sinusoid` builds `scale = 2π·f`, so the frequency is `scale/2π` — no FFT, no window, no leakage |
| **Measured** — on the rendered buffer | everything else: DC, ramps, step trains, function calls, folds | 4th-order Butterworth low-pass, run forward and backward (zero phase, ~48 dB/oct), Hann-windowed, RMS against an absolute ceiling |

Two details in the symbolic pass are worth knowing, because a naive reading gets both
backwards. **A term is a product, not a mixture.** By the product-to-sum identities
`sin(a)·sin(b)` is energy at `|a−b|` and `a+b`, and at *neither* `a` nor `b`. So the
frequencies a term reaches are the combinations `±f₁±f₂…`, and the lowest of those is what
counts. That is what distinguishes a 2 Hz **modulator** — `sin(440t)·sin(2t)`, an ordinary
tremolo living at 438 and 442 Hz — from 2 Hz **content**. It also catches the reverse case,
which per-factor reading misses entirely: `sin(440t)·sin(430t)` has every factor
comfortably audible and puts a 10 Hz difference tone in the room.

The measurement corner sits at **12 Hz, deliberately below the 20 Hz doctrine line.** Any
Butterworth passes −3 dB at its own corner, so a filter cornered at 20 Hz reads a
legitimate 20–25 Hz bass note as half infrasonic and the guard would refuse real music.
Measured on a 0.9-amplitude tone, the cascade refuses DC through 18 Hz on energy alone and
passes 19.9 Hz upward — so the two passes together leave no gap, since the text-reader
covers every authored sinusoid below 20 Hz exactly.

The ceiling is **absolute RMS, not a ratio**, because what reaches the body is absolute: an
inaudible 7 Hz term at RMS 0.001 is harmless and passes; the same term at 0.3 is what this
exists to stop.

**What the floor does not constrain is the mathematics.** A Person may author, evaluate,
differentiate, integrate, and render as geometry a 7 Hz field freely — that is the
substrate doing its job. The guard is on the path to the body, not on the thought.
`playProceduralCollisionSound` has always clamped to the same 20 Hz line; that clamp and
this one now share one constant so they cannot drift apart.

**Scope, stated honestly:** this covers the paths where mathematics becomes sound.
`playSound` and `playMusic` decode files a Person chose and are not analyzed.

Verified by `tests/infrasound_floor_test.cpp`, which pins all four properties that make it
a guard rather than a setting: it stops authored infrasound exactly, it stops infrasound
arriving by routes the text-reader cannot see, it does *not* stop ordinary sound (20 Hz
through 15 kHz, quiet infrasonic terms, and tremolo all pass), and it leaves the
mathematics untouched.

Verified by `tests/ontomath_sounding_test.cpp`; demonstrated audibly by
`scratch/probes/one_expression_two_senses.cpp`, which prints one expression as an ASCII arch,
writes it as a playable `.wav`, and reverses a `Flow` governed by its derivative.
