# Software Engineering Discipline — required reading

Split out of `AGENTS.md` so that file stays short enough to be read in full. These apply to
every session, on top of the refusals in `AGENTS.md` and the workshop rules in
`docs/BUILD_AND_ENVIRONMENT.md`.

---

- **End-to-End Coherence:** During your process, always take at least one moment to
  deliberate about how your work is supposed to function end-to-end, and how it stacks up
  against the rest of the program. This is so you can make sure the program is coherent, and
  tie up loose ends. This rule is first because this tree's most expensive failures were
  *self-agreement*: a test or a "done and verified" that reconstructed the construction it
  was supposed to judge, and so stayed green while the live path was dead.
  - `tests/shape_generator_law_test.cpp` was green while the *booted* law could not fire
    (unauthored; conditioned on a `type` path the channel does not carry). The factory
    test now calls what boot instantiates — that is the point of it.
  - `ontomath_test` and the geometry suite agreed with themselves while the live
    shape-generator spawn sat at the origin (`docs/audits/SHAPE_GENERATOR_LAW_AUDIT_2026-08-18.md`).
  - `agent intercom/conversation_history_injection.py` and `playpen.py` were claimed
    verified while they would not parse (literal newlines inside string literals).
  - `tests/bezier_patch_law_test.cpp`'s first "done and verified" was written against a
    file that had never compiled (the same literal-newline break).
  A check that does not exercise the live path is not end-to-end. It is a second office
  for the same claim, and this repository has already learned what that costs.

- **The Integrity Check:** After finishing, ask: "does anything I changed have a caller, a
  consumer, or a test that now lies?" If yes, fix it before closing the session.

- **Name the latch, name every caller.** A condition, a key, a mode string, or an event
  that more than one office reads or writes is a collision. Before you reuse a bit,
  list every writer and every reader in a comment at the field **and** at each
  consumer, pointing at the others. "Nobody linked it yet" is the same failure as
  "nobody registered it yet" (Refusal 6): the next session cannot see the other
  office, and will treat the bit as theirs.
  Worked example: `shape-generator-3d-law` used to condition on
  `@creation-channel.active3DMode == "Create"`, which is also the Creator
  Console's Create mode. They are two latches now: `active3DMode` is the
  console tool; `spawnLawArmed` is the spawn law (L / "Spawn as law").
  The field comments are the register; every consumer points there.

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

- **Scratch probes** belong in `scratch/probes/`, not `tests/` or loose at the `scratch/` root. If you build one through the test
  target for convenience, remove the copy from `tests/` **and reconfigure** when finished —
  an outside `git add -A` will otherwise commit it, and the stale CMake target breaks the
  next full build.

- **Don't claim a doc is verified because you read the source.** Every framework doc in this
  corpus has a probe in `scratch/probes/` that executes its central claims. Two of those probes
  caught claims that were plainly wrong on inspection. Run things.

- **Bounds are doctrine, not limits.** `kMaxChainRounds` (now authorable via LawManager), `kMaxCallDepth = 32`, one pass
  per fold. If your design needs one raised, the design is in the wrong shape — see
  `docs/architecture/law/ALGORITHMS_AS_LAW.md` §3.
