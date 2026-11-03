# Timeline and Moment execution weave

**Status:** Architecture proposal prepared; execution unimplemented.  
**Human origin:** Zach requested granular authoring of how each Timeline's Moments execute relative to one another, including Moments in other Timelines, through the main tick loop (2026-10-09). His follow-ups distinguished consuming A's output from ordering alone and named three temporal grounds: a Timeline starting, an Event on state change, and an Event defined by clock/tick time. The proposal carries those grounds into one execution weave.  
**Prepared by:** Codex · GPT-6 · session `01a12443-380e-75f2-bccb-224fe01c5dc8` · 2026-10-09 22:31:07 PDT.

The [architecture proposal](../../../../../plans/AUTHORED_TIMELINE_MOMENT_WEAVE_2026-10-09.md) maps the actual Engine/LawManager/channel seams, independent temporal coordinates, individually qualified occurrences, precedence and output dependencies, contention, safe continuation, and phased displacement of C++ ordering.

This integrates the existing [Law execution order task](../../Law%20and%20Reasoning/Law_execution_ORDER_is_undefined_and_unauthorable/Law_execution_ORDER_is_undefined_and_unauthorable.md), [Time ontology](../../../../../architecture/ontology/TIME_AND_MOMENT.md), [Adaptive Compute Moments](../../../../../architecture/ADAPTIVE_COMPUTE_MOMENTS.md), and [AD-27 direction](../../../For%20Zach/Zach%20Author%20Decisions.md). It introduces no new Timeline kinds, activation kinds, or independent scheduling ontology.

## Proposed first runtime consumer

Two independent Timelines, with authored applications ordered `A₁ → B₁ → A₂ → C₁`; B consumes a selected A output, and C exposes the result through direct Screen. An unrelated D continues while B's unresolved input keeps B pending. Change the authored order without C++ edits and witness the causal trace and actual native pixels.

Exercise the arrival grounds independently: a declared Timeline start admits A₁, a qualifying state transition grounds B₁, and a clock-only boundary admits D. Each must join the common frontier without adding bespoke Event or Timeline kinds, and an unchanged holding condition must not republish its transition each frame.

## Work remaining

- Author the grounded Law/Moment/Timeline association, occurrence binding, durable identity, and per-channel completion semantics; preserve existing authority and consent gates.
- Establish legible and reactive temporal progress, including direct and reflected Moment edits and restoring nested temporal context.
- Compile scoped occurrence topology and combined-edge cycle checks; implement ready-frontier admission through existing Law guards and conservative effect handling.
- Provide continuation that retains pending work and truthful completion without duplicating Drive samples, level applications, or clock advancement.
- Separate safe opportunities inside Engine update, LawManager passes, language/audio work, native EventBus results, independent maintenance, and presentation lifecycle.
- Author allocation/contention defaults and establish any required coherent commit support before promising same-presentation atomicity.
- Prove isolated ordering, production causal execution, native effects, persistence, and then Person acceptance as distinct rungs.

## Evidence boundary

This pass inspected canonical source at `f0194f3f9d2bc51b0be715ea1d848d059e860ed8` with existing working-tree changes and checked documentation structure/links. It changed no engine code or authored saves and ran no production execution tests. No current UI or native behavior is claimed. Add concrete Person verification steps when a runtime authoring surface or observable weave is implemented.
