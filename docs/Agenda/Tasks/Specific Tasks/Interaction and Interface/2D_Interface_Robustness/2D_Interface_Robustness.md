# 2D Interface Robustness

*Claude Opus 5.5 · session `b0dcb70f-a02a-4081-8589-0aae3ab30551` · 2026-09-22. From Zach's request to make the Singular/Law-driven 2D interfaces more robust.*

**Status:** Tier 0 model test passes. The native audit reopened edge ordering, click qualification, pick accuracy and Law event data. Tier 1 (B–I) remains proposed.

- Full audit, fixes, proposals, pitfalls: [plan](../../../../../plans/2D_Interface_Robustness_Pass_2026-09-22.md)
- Person checks: [Person Verification List](../../../For%20Zach/Person%20Verification%20List.md) § "2D interface robustness"
- Guards: `tests/singularity/interaction_robustness_test.cpp`, `interaction_channel_test`, `control_patterns_test` §4a
- Zach decided 2026-09-23: click slop stays authorable; no edge is subjectless (the Person is its subject/agent); leaving a Zone is sensed (`object-left-reach`) and cancelling on it is an authored law. See the plan's "Revision 2026-09-23".
- Next, from Zach's direction: when Zone activity is decoupled from the Person's location and reach becomes OntoMath bounds, `InteractionChannel::step`'s reachable-set gathering must follow.

## Native interaction audit · 2026-11-11

Zach asked for the native Singularity path behind click registration. The probe reproduced wrong-target left clicks, short right/middle clicks disappearing, release bypassing slop, a scaled cylinder beating a closer object, and incorrect key dispatch across one Law tick. Restore world-space ray distance independently, then preserve ordered input samples and per-event readings at the Sense-to-Law boundary. Use the audit's probes as regression cases. [Audit and repair order](../../../../../audits/NATIVE_INTERACTION_CHANNEL_AUDIT_2026-11-11.md)

*Codex · GPT-6 · session `01a1cdbf-bb6d-7861-a9a0-696204c370fe` · 2026-11-11T19:22:35-08:00.*
