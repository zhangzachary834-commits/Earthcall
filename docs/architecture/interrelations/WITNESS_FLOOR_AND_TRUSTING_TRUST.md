# Witness Floor and Trusting Trust

**How independent rebuilds and plural witnesses can reduce self-hosting compiler risk without claiming that compilation alone proves trust.**

**Status:** Conceptual interrelation. Substrate Ordering Stage C and a full independent-witness release gate are future work.
**Connected documents:**
- [Substrate Ordering](../ontology/SUBSTRATE_ORDERING.md)
- [No Black Box](../ontology/NO_BLACK_BOX.md)

---

## The interrelation

Ken Thompson's *Reflections on Trusting Trust* shows why a self-hosting compiler can reproduce behavior that is absent from its visible source. A system that eventually generates its own executable substrate must not treat its own output as its only evidence of correctness.

### Plurality, with explicit assumptions

The Witness Floor's constitutional direction is: **no Earthcall instance may be its own sole witness**. Substrate Ordering calls for a path from legible source through an independent toolchain, and eventually plural Earthcall instances under separate authorship. Wheeler-style diverse double-compiling can expose certain compiler-subversion attacks *when the compilers and comparisons satisfy the method's independence and equivalence assumptions*. It is not a universal proof that an executable is benign.

### What exists, and what remains proposed

The current C++ source can be built with an ordinary external toolchain such as clang, without a self-generated Earthcall compiler. That is a useful baseline, **not** evidence that a multi-toolchain comparison or self-hosting witness protocol has been implemented. Stage C's generated-law compiler, reproducible provenance, artifact comparisons, and independent release witnesses require their own implementation and executable tests.

### Protecting legibility

The enduring rule is to retain source, build provenance, a non-self-hosted recovery path, and independently reproducible evidence as the substrate evolves. A successful rebuild demonstrates that a build path exists; only additional, appropriately independent comparisons and behavioral tests can support stronger claims about correspondence between authored source and executed behavior.
