# Addendum: Integrating Prophetic Rete and Split-Substrate Serialization

*(Original synthesis: Claude 3.5 Sonnet / Jules, session 13284209740648546535. Correctness and status clarification: Sun Squid Intake III, 2026-10-27.)*

**Status: conceptual interrelation, not an implemented matter scheduler.**

## Two distinct forms of legibility

[Prophetic Rete](../architecture/law/PROPHETIC_RETE.md) conservatively analyzes what authored Laws *could* read or write before they execute. Its soundness rule is an **over-approximation**: an unknown effect must fail open rather than suppress a real Law. It is not an oracle for the exact future state.

[Split-substrate persistence](split_substrate_matter_generation_addendum.md) separates the semantic `.ecform` save root from derived `.ecmatter` physical sidecars. The latter are generation-addressed and checked for existence, size, schema, and SHA-256 before admission. The semantic root is not intrinsically a human-readable text file.

These boundaries are complementary: authored dependencies can describe *why* physical material may need regeneration, while a verified generation identifies *which* physical bytes actually correspond to a committed semantic root. Neither boundary makes the other automatic.

## A possible dependency-to-generation bridge

A future integration could map a conservative Law write effect to an invalidation frontier for a derived matter artifact. Such a bridge would have to prove that every actual change affecting the artifact is included, including First-Mover/foreign writes and unknown dependencies. Only then could it avoid redundant generation without silently missing updates.

Scheduling expensive generation asynchronously is a **proposal**, not a shipped capability of Prophetic Rete. A scheduler would need versioned inputs, cancellation or discard of obsolete results, an explicit publication boundary, and tests that stale results cannot overwrite newer semantic truth. Advancing the semantic graph before physical fulfillment is safe only when consumers distinguish pending material from a committed, verified generation; it must not silently display old bytes as if they represented the new state.

The existing save protocol writes a new sidecar before committing the semantic root and retires the predecessor only after the root commit. Its filesystem fallback and lack of explicit durable `fsync` do **not** establish universal crash-proof atomicity. See the [split-substrate audit](split_substrate_matter_generation_addendum.md).

**Engineering gate:** prove conservative invalidation, version-correct publication, verified readback, and failure/restart behavior with executable witnesses before promoting this synthesis into runtime claims.
