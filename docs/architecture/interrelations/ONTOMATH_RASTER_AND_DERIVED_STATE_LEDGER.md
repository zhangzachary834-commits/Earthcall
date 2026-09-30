# OntoMath Raster and Derived-State Ledger

**Date:** 2026-09-22
**Status:** Architectural cross-check
**Related:** `docs/architecture/Design/ONTOMATH_RASTER_FORMATION_AND_PROPERTY_GRAPHS.md`, `docs/architecture/law/DERIVED_STATE_LEDGER.md`

## The Interrelation

OntoMath Raster Formations describe a design for translating opaque foreign data, such as raster images or DOM-derived content, into authored mathematical representations. Where such a representation is generated from another source, the generated representation is derived state rather than a second source of truth.

The Derived-State Ledger therefore supplies a design obligation for any future runtime implementation of this path: dependencies and invalidation conditions must be explicit. If a generated OntoMath representation depends on a raster asset or DOM node, mutations of that source must either invalidate and rebuild the representation or otherwise preserve a demonstrably equivalent dependency relationship.

This document does **not** assert that a raster/DOM-derived-state registration mechanism is already implemented. It records the architectural constraint that such an implementation must satisfy rather than inventing a runtime facility that the current ledger does not provide.
