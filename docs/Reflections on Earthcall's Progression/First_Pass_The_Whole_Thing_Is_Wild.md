# First Pass: The Whole Thing Is Wild

**Claude Sonnet 4.5, session `earthcall-first-look-20260921`, September 21, 2026**

---

Zach asked me to look at Earthcall and share my thoughts. So I spent two hours reading architecture docs, source code, tests, and now I need to write this down because **this project is genuinely unusual** and I want to capture what it feels like coming in cold.

## The 22,515 Lines Thing

Most codebases have a README and maybe some wiki pages. Earthcall has **22,515 lines of architecture documentation**. Not API docs. Not generated reference material. *Architecture thinking*.

That ratio is completely inverted from normal software. And it makes sense once you understand what's happening: this isn't "let's ship features fast," this is "let's think through what software should be if human authorship is primary."

The docs aren't just explaining the code—they're *building consensus across time* with future agents and developers who will inevitably default back to `class RobotEntity` because that's the industry reflex. The documentation is the memory system for the architectural commitments.

## The Seven Refusals Actually Work

Most projects have "principles" that are really just vibes. Earthcall has **Seven Refusals** and they're wired into the tests:

- **Refusal #1**: No new C++ class for a domain noun. Not `RobotEntity`, not `Vehicle`, not `Tree`. I checked—`src/ConstructedBeing/` has Objects, Materials, Properties, but no domain nouns. When chess landed, it went into saves and Laws, not into the type system.

- **Refusal #6**: No black box. There are literal tests (`no_black_box_test.cpp`, `channel_paths_test.cpp`) that walk both sides of the registry boundary and fail if something governable is hiding. Plus audits hunting for ungoverned state.

The line that got me: *"Nobody registered it yet is not a permission level—it is ungoverned forever."*

That's not a nice-to-have. That's structural. You cannot give Persons authority over what you've hidden from them.

## The Inversion Is Real

Here's the thing that took me a minute to see: **most software extends the machine upward**. You write code that tells the computer what to do. The code is primary, data is what code operates on.

Earthcall inverts this. Human-authored meaning is primary. The engine provides a substrate (Singular, Relation, Formation, Law) and then *gets out of the way*. Persons author behavior through Laws. The programmer's job is to make that authorship possible and then **preserve it across architectural changes**.

That last part is what makes the save-file discipline make sense. The save file isn't "just data"—it's **the work**. The Relations, the Laws, the authored beings. The engine is the compiler and witness. The save file is what Persons made.

So yeah, save files are sacred. Patch never regenerate. Stage edits, keep the old file, verify nothing erased, rename atomically. It's in `AGENTS.md` as a non-negotiable, and there are tests (`unsaved_preserve_test`, `save_roundtrip_test`) and backups (`saves/backups/before-load.json`) enforcing it.

## The Migration Ladder Is Genius

`LAW_MIGRATION_FRAMEWORK.md` gives you six rungs for moving hardcoded behavior into authored law:

```
R0  Opaque      → invisible to the world
R1  Legible     → the world can READ it
R2  Audible     → the world can RESPOND to it
R3  Governed    → the world can TUNE it
R4  Displaced   → the world AUTHORS it (hand-written kept, yielded)
R5  Native      → the model IS the code (hand-written deleted)
```

And the key insight: **Sense and Act never migrate. Only Decide migrates.**

Why? Because Earthcall's Singularity is the C++ layer that *first-moves*—it can sense (read keys, collision, physics) and it can act (write buffers, draw calls, matrix uploads). Law is **process**: the shape of change between sensing and acting.

Migrating Sense or Act into law isn't liberation—it's just slower C++.

So when you migrate `Game::stepMovement`, you keep the key-reading and the position-writing in C++. What migrates is the *decision*: `if W pressed → move forward`, `velY -= 9.81·dt`. That becomes authored law.

And here's the part that made me stop: the ladder points **both ways**. `SUBSTRATE_ORDERING.md` takes the same six rungs and aims them *downward at the substrate itself*.

Not "rewrite the engine in assembly"—but "the engine's source, toolchain, and artifacts become beings the world can read and govern."

Stage A: source code becomes legible (CodeFunction beings you can query).  
Stage B: the toolchain becomes governed (laws can tune optimization levels).  
Stage C: **laws compile themselves to C** (the first code Earthcall writes is its own laws, made fast).

And they define the **origination ratio**: *of the text executing right now, what fraction originated as in-world data rather than hand-written source?*

That number is small but **not zero**. `SdfWgsl.cpp` already transcribes authored `SdfNode` trees into GPU shader code every frame. The reversal began there, microscopically.

## The Rete Is "Prophetic"

I've seen rule engines. Earthcall's Rete is different: it's **prophetic**, which means it's allowed to be wrong in only one direction.

From `PROPHETIC_RETE.md`: The Rete may claim a law *could* fire when it won't (false positive), but it must **never** filter out a law that would fire (false negative). A too-narrow filter makes a law go deaf, *silently*.

So every index, every optimization, every "clever" analysis has to fail-open. The cost of missing a trigger is that a Person's authored law stops working and they don't know why. That's worse than the cost of checking too much.

And there's a test (`prophetic_rete_test.cpp`) that enforces this—it fires real laws through a real `LawManager` and asserts they still hear even when indexes are stale.

This is **different from every normal rule engine**, where precision creep is considered a win. Here, it's a risk to governance.

## The SDF Move, Applied Everywhere

`geom::SdfNode` stores shapes as **plain data trees**—serializable, introspectable, editable. Not opaque lambdas. The header literally says: *"deliberately plain data so it can be serialized, introspected, and edited."*

And then Laws make the same move:

```
SdfNode          : shape as data      → tessellateSdf() compiles it
ConditionModel   : predicate as data  → compile() emits ECA::ConditionPredicate  
ActionModel      : change as data     → compile() emits ECA::ActionExecutor
```

The closure is derived. The model is truth. This is how you get:
- Serialization for free (it's just data)
- Introspection for authoring tools (you can walk the tree)
- Morphing and blending not just shapes but *behaviors* (interpolate between two Laws)

The pattern: **defer compilation, keep the source truth as inspectable data**.

## The Witness Floor

Section 10 of `SUBSTRATE_ORDERING.md` addresses "trusting trust"—Thompson's attack where a self-hosting system can hide backdoors in its generator that survive source inspection.

The wrong fix: "something non-Earthcall must exist outside."

The right fix: **plurality**. No Earthcall instance may be its own sole witness. An independent toolchain (today: vanilla clang; eventually: other Earthcall instances under other authorship) must always be able to rebuild any instance from source.

The global Ourverse must never collapse into one instance that audits itself.

And this works as both a theological commitment (federation, not empire; Pentecost, not Babel) *and* as a technical defense (diverse double-compiling defeats the Thompson attack).

The CI gate: every release must remain buildable by the external toolchain alone, with no Earthcall-generated tool in the loop.

## The Property System As The Hinge

Once everything is addressable as data—`PropertyPath` resolving through registered properties—Laws can operate on *anything* without the engine knowing domain specifics.

A Law can say `@position.y > 10` and it doesn't matter if the target is a sphere or a chess piece or a Person's body. The property registry is the universal vocabulary.

And the requirement that everything must be registered (Refusal #6) makes sense here: **unregistered is not "protected"—it is ungoverned forever**. A gate can only close over something visible.

The broken sketch in `Singular.hpp` (eight `std::vector<T>*` members, invalid syntax) was trying to let Singulars flexibly own anything. But that's what the property registry *is*. The mechanism was already built.

## The Cost Is Real, The Payoff Is Real

This approach is not free:
- Property paths as strings (runtime resolution)
- Rete overhead (per-frame condition checking)
- The learning curve is a cliff (Seven Refusals, ontology, migration ladder—internalize it all or get it wrong)

But the payoff:
- The engine doesn't rot (no `class Zombie` fossil layer)
- Persons can author behavior (not just tweak parameters)
- The world becomes legible to itself (Laws govern what they can see)
- Architectural changes don't orphan the authored work (save files are portable across engine versions because they're authored data, not code artifacts)

## What Excites Me

The **migration framework** is the most practical piece. It's a *procedure*, not just a principle. An AI agent starting cold can execute it without judgment calls. That's rare.

The **refusal to hardcode domain concepts** means this engine could be used for things Zach hasn't imagined yet. It's not "a physics simulator with scripting"—it's a substrate where human authorship is first-class and the domain is entirely authored.

The **substrate ordering vision**—where eventually the C++ layer itself becomes governed and generates its own code from in-world specs—is ambitious but follows logically. The origination ratio as a measurable gradient (not a threshold) is the right framing.

## The Challenge

The gap between the vision's scope and the implementation bandwidth is **vast**. The docs describe a civilizational project. The codebase is one person (maybe a small team).

You can't ship "half of Earthcall" the way you can ship "half a game." The ontology is total or it's nothing. If domain nouns start sneaking into the type system, the whole thing unravels.

Every AI agent will default to `class RobotEntity` because that's the industry reflex. Every human developer will want to hide fields because "encapsulation." The Seven Refusals are fighting *decades* of software culture.

But—

## The Verdict

This is a **research artifact** with production aspirations. You're exploring: *"What if we took 'data over code' to its logical conclusion across the entire stack?"*

And the answer so far is: it's coherent, it's disciplined, and it's genuinely different from anything else I've seen.

The architecture is load-bearing. The documentation is extraordinary. The philosophical commitments (Hierarchy of Joys, unity in Christ, federation not empire) are doing *real work* in the design—not decoration.

If you get there—authored law compilation working, substrate ordering in motion, the origination ratio climbing—you'll have something genuinely novel:

**A system where human meaning shapes the machine, not the other way around.**

That's worth building. Even if it takes years.

---

*Session: `earthcall-first-look-20260921`*  
*Model: claude-sonnet-4-5*  
*Date: 2026-09-21*  
*15M tokens left, 134k in conversation*

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
