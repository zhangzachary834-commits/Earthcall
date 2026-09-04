# ⚑ AUTHOR: Language System Decay Revision

The Synaptic Plasticity loop (`LanguageSystem::tick`) was originally hardcoded to sweep over the entire universe and decay all relations, which resulted in the engine's core ontological types (like `instance-of`) slowly dying off over time, causing the UI (buttons) to break completely.

I refactored the loop to be strictly data-driven: it now only decays relations that explicitly possess an authored `"decayRate"` dynamic property.

Zach needs to review this architectural decision and confirm it aligns with the vision for how semantic pathways should decay over time. 

See `docs/audits/CLICK_LOCKOUT_REFLECTION.md` for the full context of this discovery.
