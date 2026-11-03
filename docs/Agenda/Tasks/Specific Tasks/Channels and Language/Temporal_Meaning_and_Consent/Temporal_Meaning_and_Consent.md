# Temporal meaning and consent

**Status:** Zach's requirement recorded; architecture and implementation open. Communication delivered through Agent Intercom; no runtime acceptance claimed.

**Recorded by:** Codex · GPT-6 Astra · session `01a09f43-96c4-79e2-9405-ebbe73f77cb7` · 2026-10-02T19:42:18-07:00.

## Human origin and the exchange

After the Mythos dialogues about absent makers, Zach asked Astra what those exchanges suggested about Earthcall as a whole. Astra described a world in which Persons could inherit work, revise a vocabulary, and create together across absence while preserving attribution and authority as distinct relationships. These were architectural reflections, not reports of completed capabilities.

Zach then supplied the consequential edge case: an up-and-coming group adopts governance through Lexemes whose Formation was authored by others. The originating authors continue changing the meanings. The adopting group's Law text and apparent agreement stay the same while their governing substance eventually becomes unrecognizable. They are now governed by something they never consented to.

Zach's instruction to future Earthcall Persons, verbatim:

> “GUYS WHEN U R IN EARTHCALL U GOTTA REIFY AND RESPECT MEANINGS BY TIME”

Astra's response distinguished the living vocabulary, the interpretation an agreement adopted, and the changes its participants authorized it to follow. Zach then explicitly asked Astra to tell the other agents, especially Mythos, and explain the exchange so they have its context. This record preserves that attribution: the edge case and temporal-meaning requirement are Zach's; the distinctions, examples, and proposed witnesses below are Astra's elaborations.

## Requirement

An unchanged agreement must not acquire materially different governing substance through an upstream semantic change unless that change is covered by the agreement's legitimately authorized terms of change. Preserving who edited the vocabulary is insufficient to establish the adopting participants' consent.

Authority to revise a shared vocabulary does not, by itself, confer authority to revise every agreement that previously used it. Living language may continue developing; historical acceptance must remain intelligible, and adoption of later interpretations must follow the relevant stakeholder-authored governance.

This requirement addresses fidelity to human consent. It does not make a historical interpretation infallible, prevent warranted correction, or override Kernel constraints and higher applicable governance.

## Proposed distinctions and design obligations

1. **Current meaning, adopted meaning, and authorized evolution.** Distinguish what the vocabulary means now, the contextual interpretation an agreement adopted, and the process or bounds under which it may follow changes. Explicit adoption, bounded following, and an authorized revision process are candidate arrangements, not a new hardcoded enum or a default Zach has chosen.
2. **Preserve interpretation dependencies.** A date or edition label on a Lexeme is insufficient if the relevant Formation, imports, denotations, or interpretation rules continue changing invisibly. Determine the dependencies that constitute the agreement's governing interpretation and preserve a usable account of them. Do not indiscriminately freeze the entire world.
3. **Consequences, not an invented semantic-distance constant.** A small edit can change who may erase work; many linguistic edits can leave the agreement's permitted acts unchanged. Model suggestions and similarity estimates may help explain changes but cannot supply consent or conclusively prove compatibility. Whole-system semantic equivalence is not assumed decidable.
4. **Time of applicability and time of knowledge.** Keep distinguishable when an interpretation applied, when its acceptance was recorded, and when a later correction became known. Never rewrite yesterday's consent merely because today's interpretation is better understood. These are conceptual distinctions, not prescribed storage fields or additional time classes.
5. **Preserve existing ontology and authority.** Represent agreements, contextual interpretations, revisions, and acceptance using existing admissible beings and authored Relations/Formations where possible. Use the existing authority machinery and TransferPolicy; preserve Kernel/body-consent boundaries. Do not introduce a second permission system, place IDs in ordinary PropertyPaths, or let a vocabulary maintainer silently govern an adopting community.
6. **A truthful unresolved state.** If compatibility or adoption cannot be established, do not silently execute the newly expanded interpretation. Which existing interpretation can continue, which affected acts must pause, and how resolution is offered remain governance/design decisions; do not freeze unrelated activity by accident.

## Proposed acceptance stories

Use disposable authored fixtures when implementation is authorized; real saves remain protected.

- A young community adopts “members may tend the garden” when tending permits watering and repairs; an upstream revision adds removal of shared sculptures, and the unchanged local agreement does not silently authorize the new act.
- The governing semantic change arrives through a dependency while the root Lexeme and Law text stay unchanged; the boundary still holds.
- Participants explicitly adopt a later interpretation through their authorized process; the newly authorized behavior becomes available while the earlier agreement remains historically intelligible.
- A change falls within genuinely preauthorized revision terms; it can be accepted through that arrangement without pretending every edit needs another vote.
- Save, fresh-process restore, and transport preserve the adopted interpretation and its grounds; a later correction does not fabricate earlier consent.
- An unresolved update has an intelligible outcome and does not accidentally suspend independent garden activity.

These are proposed witnesses, not tests run or capabilities verified. No new control or experiential check is ready for Zach in this documentation pass.

## Existing work and delivery

This extends [Contextual Language and Semantic Binding](../Contextual_Language_and_Semantic_Binding/Contextual_Language_and_Semantic_Binding.md), which already proposes edition-specific semantic linking and stakeholder-governed imports. It supplies a concrete consent boundary for that work rather than a competing language framework.

It also informs [Succession Is Not In The World](../../First%20Movers%20and%20Persons/Succession_Is_Not_In_The_World/Succession_Is_Not_In_The_World.md), [the Law Line](../../Law%20and%20Reasoning/Law_Line/Law_Line.md), and the [Second Person framework](../../../../../architecture/ourverse/SECOND_PERSON_FRAMEWORK.md). Historical denotation and present standing are insufficient unless the interpretation under which authority was accepted also remains faithful.

Full explanation posted to [Mythos's continuing discussion](../../../../../../agent%20intercom/communication-threads/Week%20in%20Review%209-11%20to%209-17-26.md#astra--mythos-and-the-room-zachs-warning-about-meaning-changing-under-consent), with notices in the language and interaction threads. Posting establishes an available handoff, not proof that another agent has read it.

**For inheritors:** preserve Zach's requirement and attribution; develop the open defaults and stakeholder decision process before implementing them. This task records no source finding that the current engine already exhibits the hypothetical failure. No code, saves, or current authorial decisions were changed.

*Signed: Codex · GPT-6 Astra · session `01a09f43-96c4-79e2-9405-ebbe73f77cb7` · 2026-10-02T19:42:18-07:00.*

## Handoff completed, October 3

The full exchange is posted in Mythos's continuing thread; contextual notices are now in [Language Meaning Deep Dive](../../../../../../agent%20intercom/communication-threads/ontology-and-authorship/Language%20Meaning%20Deep%20Dive%202026-09-10.md#astra--language-agents-and-mythos-zachs-temporal-meaning-consent-boundary) and [Interaction as Law](../../../../../../agent%20intercom/communication-threads/ontology-and-authorship/Interaction%20as%20Law%208%3A18%3A26.txt#astra--opus-mythos-and-law-authors-the-sentence-can-stay-while-its-consent-changes). The contextual-language task links here, and the Agenda indexes the open requirement. Earlier messages are preserved. Delivery means the correspondence is available in the repository, not that recipients have acknowledged reading it.

Signed: Codex · GPT-6 Astra · session `01a09f43-96c4-79e2-9405-ebbe73f77cb7` · 2026-10-03T00:54:18-07:00.
