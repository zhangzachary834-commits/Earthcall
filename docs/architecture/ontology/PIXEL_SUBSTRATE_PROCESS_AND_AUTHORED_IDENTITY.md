# Pixels: substrate, process, and authored identity

**Status:** Architectural doctrine commissioned by Zach on 2026-10-17. The principle
governs representation choices; it does not declare every correspondence implemented.

**Human origin.** After the [property-only Screen pipeline witness](../../audits/SINGULAR_SCREEN_PROPERTY_PIPELINE_2026-10-17.md),
Zach distinguished pixels/regions as raw machine substrate with Properties, as results
of actions by authored Laws or external First Movers, and as vessels of authored
identity carrying a machine-addressable substrate reference and other pixel Properties.
He explicitly rejected binding Earthcall to one representation and emphasized that it
can move fluidly between them and “hold them all together.” He then requested this
account be canonized. These three perspectives and their coexistence are Zach's.

**Recorded and formalized by:** Codex / GPT-6.1 Sol / session
`01a10a2b-a247-7c11-9d5f-7a8b89df6cfc` / 2026-10-17T16:41:45-07:00.
Codex supplies the organization and correspondence obligations below in service of
Zach's account. This continues the [direct Screen thread](../../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md#canonization-of-coexisting-pixel-perspectives--2026-10-17).

## 1. Governing principle

> Earthcall may represent the same machine substrate simultaneously through its
> accessible state, the authored acts that produce its manifestations, and Singulars
> whose authored identities refer to it. Persons author the meaning and connections
> of those representations; the substrate does not impose one exclusive ontology.

These are coexisting perspectives, not three C++ kinds, enum variants, mandatory
pipeline stages, or successive levels every sample must climb. A design may use one,
several, or all three as its human intention requires. Their connections can change
through authored acts while their distinctions remain legible.

## 2. The three perspectives

| Perspective | What it expresses | Earthcall representation |
| --- | --- | --- |
| Substrate with Properties | An addressable machine sample or region and the characteristics exposed about it | A modality channel projects state through legible Properties on a bearer; dense samples need no individual Singular allocation. |
| Result of an authored process | What an act produces, with its source, authorship and temporal context | Authored Laws or authorized external First Movers act through the modality; a field, stroke, transformation or sampled result can be the relevant whole. |
| Vessel of authored pixel identity | A particular pixel/region/mark given identity and a meaningful role | An appropriately authored Singular bears substrate-reference, mathematical-bound, color, observation and provenance Properties, as applicable; Relations connect it to other beings. |

### Substrate

The raw machine need not become a being per sample before it is addressable. A channel
can expose a sample's coordinates and observed state as aspects of that channel or
another bearer. The [No Black Box](NO_BLACK_BOX.md) rule governs legibility. Driver
handles and transfer buffers remain explicitly named Kernel machinery; that exemption
does not hide the authored meaning or accessible observations of a pixel.

Distinguish texture texels, framebuffer samples and physical display elements. Their
address spaces and properties differ. Current direct Screen witnesses concern the
Earthcall framebuffer; they do not establish physical-monitor photometry or control
over another application's display.

### Process

A region may be meaningful because it is the result of an act: a Person's stroke,
an evolving mathematical field, a Law's change, or an authorized First Mover's output.
Its extent and colors can be consequences of that process rather than an inventory
of independently identified pixel beings. Process identity is itself an authoring
choice. Representing a result does not automatically create a stroke being, Event,
durable causal history, or complete mapping from acts to displayed samples.

An authored field, the act sampling it, and a dated observation of the final image
carry different information. Keep those distinctions when relating them. Foreign acts
retain the existing First Mover authorization boundary; calling something a process
does not supply authority to perform it.

### Authored identity

A Person may give a particular sample, region or mark its own Singular identity.
That vessel can carry a machine-addressable substrate reference as one Property and
the exposed pixel characteristics as other Properties. A Property remains a predicate
of its bearer: see [Property as Predication](PROPERTY_AS_PREDICATION_NOT_BEING.md).
The reference, color or region dictionary does not itself become another Singular
merely because it is stored in a Property.

The identity-bearing vessel and its substrate locator remain distinguishable. A fixed
sample on one surface, a mark maintained through viewport resizing, and a stroke
manifesting over time imply different correspondence rules. Persons author that
meaning; this doctrine chooses no universal identity-lifetime or remapping policy.
Use existing concrete Singular kinds and authored Properties/Relations, rather than
adding a Pixel class, region-kind enum or compulsory ShapeKind carrier.

## 3. Fluidity preserves the connections

A named mark can have identity, be the result of an authored act, and manifest through
many substrate samples at once. An otherwise anonymous sample may acquire significance
through a later authoring act. A single field can describe millions of samples while
only selected meaningful regions receive independent identity. Authored Relations and
Properties can hold these perspectives together without collapsing them into one.

This permits multiple representations of the same substrate; it does not assert a
universal one-to-one mapping or automatic interchangeability. Multiple references
alone settle neither ownership nor simultaneous-write arbitration.

Any implemented correspondence must preserve the information its meaning needs:

1. **Address context:** identify the relevant channel/surface and coordinate domain;
   retain extent and validity context where needed. Reused coordinates alone do not
   establish the same substrate referent. The exact general locator format remains open.
2. **Temporal context:** keep an observation's frame/time and sampled definition;
   subsequent source edits cannot reinterpret it as a new observation.
3. **Authored causation:** retain an explicit producing source/act when known. A final
   composited color cannot establish a unique inverse to the act or source that made it.
4. **Identity intent:** retain the authored distinction between a vessel's identity and
   its current manifestation. Remapping needs the intended identity semantics, not an
   assumed equivalence between a stable being identifier and a transient pixel address.
5. **Authority and definedness:** carrying a reference grants no additional permission;
   existing channel guards and authority mechanisms still apply. Missing mappings,
   unsupported expressions and unavailable observations must remain explicit.

Fluidity therefore means authored, intelligible movement and coexistence. It promises
neither a lossless conversion between every view nor an inverse for every process.

## 4. Implemented mechanisms and open correspondences

| Current mechanism | What remains distinct or open |
| --- | --- |
| Qualified typed fields on Singular bearers; direct Screen sampling and source edits | One explicitly composed output; no automatic discovery/composition of every region-bearing Singular. |
| Authored ScreenRegion dictionary with color and selector fields | The dictionary has no independent identity. Color Piece guards draw; the separate selector chooses observed samples. |
| Bounded new-token observations of actual completed-viewport pixels | Dated read-only snapshots; no universal live framebuffer l-value, inverse compositor or general durable substrate-locator scheme. |
| Native property-only proof for a non-Object Singular and plain Object | No proof of every concrete kind's birth/restart, automatic pixel-identity promotion, complete action-to-sample provenance or physical-monitor control. |

The [direct Screen task](../../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md)
owns the implementation frontier. This canonization changes documentation only;
historical/native evidence keeps its recorded scope. General correspondence machinery,
identity defaults, composition and multi-Person decisions must be specified explicitly
before implementation. The doctrine admits the expressive space without inventing
those decisions on a Person's behalf.

## 5. Agent rule

Before building pixel/region functionality, identify the intended perspectives, their
bearers, the authored mathematics/acts, and any required correspondence. Reuse the
existing modality and Property/Law mechanisms. Do not force every sample into a being,
every meaningful region into an anonymous buffer, or every visual form into ShapeKind
primitives. Preserve identity, process and substrate connections wherever the Person
has authored them, and report which connections are implemented versus proposed.
