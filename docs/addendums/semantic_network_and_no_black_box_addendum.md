# Integrating Semantic Network Vision and No Black Box

This addendum synthesizes the architectural directives established in `docs/architecture/migration/SEMANTIC_NETWORK_VISION.md` with the ontological constraints set forth in `docs/architecture/ontology/NO_BLACK_BOX.md`.

## Integration Analysis

The Semantic Network Vision outlines an ambition to create "neural-network like" semantic mapping, where relations decay or strengthen over time. A naive implementation of this might introduce hidden "weight" variables inside a C++ `Relation` class, accessible only to internal engine logic.

However, the No Black Box refusal expressly forbids this: "A field a Person cannot address is a field a Person cannot govern, and Earthcall does not ship state that governs Persons from behind their backs."

Therefore, integrating these two concepts means that the semantic graph's plasticity—its weights, its decay rates, and its inferred connections—cannot be hidden engine mechanics. They must manifest as explicit, authored Laws and queryable properties on `Relation` objects. The system must update a registered float property (`weight`) on a structural `Singular`, ensuring that a Person can inspect, alter, and author Laws against these exact values. Subconscious inference must likewise occur at the Authored Law level, not as a hidden background thread.

## Reflection

Model: Jules
Harness: default
Session ID: 6037580766551911344

The alignment of the Semantic Network Vision with the No Black Box principle is a testament to Earthcall's commitment to unopinionated, legible systems. By enforcing that semantic weight is a tangible, addressable property, Earthcall prevents the engine from becoming a black box that dictates meaning to its users. It empowers Persons to be the true authors of their world's semantics.
