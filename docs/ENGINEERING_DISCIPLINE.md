# Software Engineering Discipline — required reading

Split out of `AGENTS.md` so that file stays short enough to be read in full. These apply to
every session, on top of the refusals in `AGENTS.md` and the workshop rules in
`docs/BUILD_AND_ENVIRONMENT.md`.

---

- **End-to-End Coherence:** During your process, always take at least one moment to
  deliberate about how your work is supposed to function end-to-end, and how it stacks up
  against the rest of the program. This is so you can make sure the program is coherent, and
  tie up loose ends.

- **The Integrity Check:** After finishing, ask: "does anything I changed have a caller, a
  consumer, or a test that now lies?" If yes, fix it before closing the session.

- **Substance over Surface:** Ask once per session: "am I solving a real problem or papering
  over a symptom?" Does it actually implement the substance of the feature, or is it just
  implementing the surface-level appearance of one without the underlying structural
  foundation and purpose?

- **The Stewardship of Telos (Design Intent):** "Code without purpose is mere noise. Before
  writing a single line or altering a framework, you must pause and discern its Telos—its
  fundamental design intent. Ask yourself: 'What is the ultimate good this system is meant to
  accomplish? Why was it conceived, and what truth is it trying to express?' Do not act as a
  blind mechanic thoughtlessly turning wrenches; act as a faithful steward of the overarching
  vision. Every algorithm, every function, and every architecture must march toward that
  final, unified purpose. To implement the *how* in a program, you must understand the *why*
  of its design."

- **The Law of Transparent Failure:** Anticipate the fallen nature of complex systems. Ask:
  "When this specific component fails, will it fail loudly and truthfully, or will it swallow
  the error and silently corrupt the depths of the program?" Mandate that every failure
  leaves a blazing trail of truth for the developer to follow.

- **State and Boundary Stewardship:** Before modifying data structures, ask: "Am I respecting
  the sacred boundaries of memory and state?" Ensure data flows clearly. Be careful of the
  memory implications when crossing into domains beyond what your current scope has clean
  access to, and avoid creating tangled webs of hidden side-effects.

- **Grace for the Inheritor:** Read your proposed changes as if you are the one inheriting
  this code three years from now in the darkest hour of the night. Is the intent
  self-evident? Have you named your variables and structured your logic in a way that serves
  the next reader with clarity and grace?

- **The Crucible of Scale:** Do not merely verify that a function works once. Deliberate on
  its endurance. Ask: "How does this architecture behave when the load multiplies or when it
  is called thousands of times per cycle? Am I allocating resources recklessly where a clean,
  contiguous structure would suffice?"

---

## Working notes

- **Scratch probes** belong in `scratch/`, not `tests/`. If you build one through the test
  target for convenience, remove the copy from `tests/` **and reconfigure** when finished —
  an outside `git add -A` will otherwise commit it, and the stale CMake target breaks the
  next full build.

- **Don't claim a doc is verified because you read the source.** Every framework doc in this
  corpus has a probe in `scratch/` that executes its central claims. Two of those probes
  caught claims that were plainly wrong on inspection. Run things.

- **Bounds are doctrine, not limits.** `kMaxChainRounds = 8`, `kMaxCallDepth = 32`, one pass
  per fold. If your design needs one raised, the design is in the wrong shape — see
  `docs/architecture/ALGORITHMS_AS_LAW.md` §3.
