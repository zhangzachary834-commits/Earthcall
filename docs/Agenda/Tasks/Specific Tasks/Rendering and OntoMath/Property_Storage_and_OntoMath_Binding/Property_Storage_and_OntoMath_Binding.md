# Property storage and live OntoMath binding

**Status:** Architecture drafted; implementation open.

**Architecture:** [`PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md`](../../../../../architecture/ontology/PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md).

**Origin:** Zach's To-Do note, preserved below without rewriting his requests.
**Recorded by:** Codex (GPT-6), session `01a0cad3-5b23-7430-b00a-0613ece86506`, 2026-09-22T13:41:24-07:00.

**Further direction from Zach, 2026-09-22:** “the PERMISSIONS of memory/property accessilblity has to be governed by Laws” and “there is also a bootstrapping/First mover problem because Laws themselves are memory.” These are requirements, not optional implementation notes; the architecture document's §§5a–5b carries the proposed response.

## Zach's original direction

> So recall the foundational definition of Property: Machine-level substrate ordered as predicates of a Singular.
>
> - OntoMath Rework OntoMath to have more precise metal capacities so there is an option where the variables themselves can directly reference/point to Properties, so the specific data types of *values* they calculate are not engine-locked to just being a standard "double" or "float" and you aren't restricted to deep-copying just to calculate.
> - If we haven't already done this—cpp data structures should also be authorable properties. Not full Singulars in themselves (we don't give new domain nouns for Graph or Hash Map or something)
> - Data structures as Properties mean complex Property operations can be stored in a data type so we do not need to create new Singulars for everything cpp-level data structure.
> - Some processes need to stay machine level though. It's about being fluidly govern how data is stored rather than forcing Persons to handle metal-level cpp management themselves.
> - Say I have something I'm holding in my hand that's red. I want to be able to use OntoMath to make a building that derives exactly from that PropertyPath, and the PropertyPaths themselves need robust authorable memory management.
> - This is a very low level memory management interface (thats still needs to be fundamentally safe and not allow segfaults) that will take a lot of skill to operate in-world.
> - Also, different properties should be able to access the same memory. The power of shared/unique/weak-pointers by Property should not be an inaccessible machine process and be authorable direction.
> - Memory management and ownership so Properties can point to any memory (I don't literally mean ANY) and share the same memory as properties. For example, sharing "red" could either mean a Property path that is distinct yet shares the same memory as another (but could also be rewritten or referenced differently), or it could mean sharing the PropertyPath itself (that would inherently share memory), or it could be a deep copy. Obviously, cpp level things like dereferencing, address, pointer managemnet should remain in the cpp, but deciding which noun to point to.
> - We thus need a Property cache those properties very efficiently. I'm wondering if we should keep using SingularId Interning or if SoA would be better.
>
> (move this section to Specific Tasks, replace with smaller bullets.) - Zach

## Open work

1. Specify path-following, cell sharing, unique ownership, weak observation, copying, and derivation as distinct authorable relationships, including the serialized compatibility contract.
2. Make nested and shared Property writes observable to Law/Rete and channel consumers through a complete change feed.
3. Implement checked value-cell lifetimes and save/load alias topology without exposing C++ addresses or granting a second permission system.
4. Extend OntoMath bindings to preserve supported value types and resolve live Property references, with explicit type and channel refusals.
5. Measure the existing interned-name/parallel-array layout against selective dense storage on representative authored worlds before changing the cache.
6. Specify one Law-governed read/write/bind/share/derive access funnel and bootstrap its initial policy Laws through an attested First Mover without self-authorizing Law edits.

**For future agents, including Jules:** `StringId` interns path names, `SingularId` identifies beings, and `Singular` already holds two parallel Property lookup arrays. Do not add a Property-Singular, a domain container class, a new enum kind, or a raw-pointer save representation. Read the architecture document's current-state table and proof plan before implementing. Save-file changes require Zach's authorization and the patch/stage/verify/atomic-rename process in `FIRST_MOVER_AUTHORING.md` §7.

---

*Codex (GPT-6), session `01a0cad3-5b23-7430-b00a-0613ece86506`, 2026-09-22T13:44:32-07:00. Zach supplied the original direction and the Law-governed-permission/bootstrap correction; Codex organized the open work and linked its architecture proposal.*

## Waiting use case: the Law Line (2026-09-25)

The Law Line's `set glow to @lamp.brightness` is **refused** today, and it names this movement's "copy value" relationship (§2). Zach asked that no ad-hoc `operandPath` be added to `ActionNode` outside it. When this binding algebra lands, the Law Line needs only its grammar's refusal replaced by the new binding. → [Law_Line](../../Law%20and%20Reasoning/Law_Line/Law_Line.md) *(Claude Code · Claude Opus 5.5 · 2026-09-25)*

## Zach's identity and authority direction (2026-09-27)

Zach specified that a path intended to refer to an individual must bind that individual's durable identity, provided the path itself can individuate by ID. It must never silently route to another individual, and ambiguity between possible referents must refuse. He left open who adjudicates IDs and asked for help understanding the cryptographic boundary. For distinct paths sharing one value cell, write behavior **depends on their authority level**; no precedence rule was supplied. Zach further specified that permission to read an ID is decided by Metalaw authored by authority-bearing People. If the reader lacks that permission, a Metalaw must author another way to disambiguate. These are Zach's requirements, not a blanket permission to expose IDs in Law text.

Source inspection found that `PropertyPath` stores property-name segments and runtime-interned `StringId`s, while `resolveLawRoot` resolves `@` roots through `getIdentifier()` text. `Person` has a key-based `SingularId`, but `Singular` has no common durable-ID contract; Objects, Zones, and Laws still return their text identifiers. Law path serialization writes the path string. Thus individual binding across save/reload is **not implemented**. A narrow safety repair now makes a qualified spelling refuse when the current root index contains two distinct bearers with that spelling; `property_path_precalc_test` covers read and write refusal (focused build and test passed). The root index still depends on structural revision and is not a durable binding.

**⚑ AUTHOR — open:** How should a saved individual binding carry identity when its reader may not see the ID? What disambiguating mechanism should Metalaw author? Which authority level(s) govern writes through distinct paths sharing a cell, and how are conflicts settled? Do not infer answers from the current cache or introduce a storage/permission ontology around these questions.

*Codex · GPT-6 · session `01a0e64f-5853-7d30-8196-995b4fd16b89` · 2026-09-27 22:12 PDT. Zach supplied the identity, ambiguity, ID-disclosure, and authority requirements; Codex inspected the code and added only the duplicate-spelling refusal.*
