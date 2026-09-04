# ⚑ AUTHOR: Language System Decay Revision

The Synaptic Plasticity loop (`LanguageSystem::tick`) was originally hardcoded to sweep over the entire universe and decay all relations, which resulted in the engine's core ontological types (like `instance-of`) slowly dying off over time, causing the UI (buttons) to break completely.

I refactored the loop to be strictly data-driven: it now only decays relations that explicitly possess an authored `"decayRate"` dynamic property.

Zach needs to review this architectural decision and confirm it aligns with the vision for how semantic pathways should decay over time. 

See `docs/audits/CLICK_LOCKOUT_REFLECTION.md` for the full context of this discovery.

**2026-09-04 addendum (Claude Sonnet 5):** this is the *second* known incident of the same
failure class, not a one-off — an earlier session separately fixed a bug where Zones dropped
all their Relations on load, which is why chess pieces didn't respond to clicks (same root
mechanism: an identity relation like `instance-of` goes missing, a click Law's condition
silently fails to match). Two different subsystems (save/load path, Language System) have now
independently found the same soft spot. Worth deciding, alongside the decay-loop question
above: should identity-defining Relations (`instance-of`, `subcategory-of`, `authored-by`, …)
get one structural protection at a single choke point (`RelationManager`/`Formation`) instead
of relying on each future Relation-touching subsystem to independently avoid repeating this?
Full reasoning in [Two_Times_The_Relations_Vanished.md](../../../Reflections%20on%20Earthcall%27s%20Progression/Earthcall%20Development%20War%20Stories/Two_Times_The_Relations_Vanished.md).
