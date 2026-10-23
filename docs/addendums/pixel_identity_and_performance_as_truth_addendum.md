# Pixel Substrate, Performance as Truth, and No Black Box

*Jules task 17781297787680931547; reconciled with current architectural doctrine by Sun Squid Intake.*

## The connection

[Pixel substrate, process, and authored identity](../architecture/ontology/PIXEL_SUBSTRATE_PROCESS_AND_AUTHORED_IDENTITY.md) allows three **coexisting** perspectives on the same visual region: accessible machine substrate, the result of an authored act, and an identity-bearing Singular deliberately authored by a Person. None is a mandatory stage, and an ordinary framebuffer sample need not become its own Singular. A modality channel may expose sample state as Properties of an appropriate bearer.

[No Black Box](../architecture/ontology/NO_BLACK_BOX.md) requires a being's meaningful state to be legible as registered Properties, while explicitly named driver handles, buffers, and other **beneath-Kernel mechanisms** remain legitimate. Legibility does not require exposing every hardware byte as a being or prohibiting GPU machinery. A Singular's identity, its substrate locator, and a sampled observation remain distinct.

[Performance as Truth](../architecture/ontology/PERFORMANCE_AS_TRUTH.md) gives the optimization goal: evaluate the intended representation without unnecessary shadow abstractions. Shared computation, caches, and native lowering are admissible when their provenance, dependencies, and invalidation are explicit; they are not automatically false because they use machinery. Unknown proofs must retain the exact path.

## Evidence boundary

These doctrines do **not** by themselves guarantee pixel-exact output, a particular frame rate, stable identity across resizing, or automatic correspondence between authored acts and samples. A concrete implementation must define its channel and coordinate domain, preserve relevant temporal and authorship context, test output parity at the required resolution, and measure cost on the actual backend. The current direct Screen witness concerns Earthcall's framebuffer, not physical-monitor control.
