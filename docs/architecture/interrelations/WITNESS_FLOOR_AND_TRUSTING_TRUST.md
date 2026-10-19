# Witness Floor and Trusting Trust

**How Earthcall's requirement for plural, independent toolchains defends against the Thompson attack and prevents self-generated illegibility.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../ontology/SUBSTRATE_ORDERING.md` (The Substrate Ordering and Witness Floor)
*   `../ontology/NO_BLACK_BOX.md` (The refusal of hidden, un-introspectable state)

---

## The Interrelation

As Earthcall moves toward Stage C of its Substrate Ordering vision (where laws compile themselves and the engine generates its own code), it confronts a profound existential risk outlined in Ken Thompson's *Reflections on Trusting Trust*: the threat that a self-hosting system can carry invisible backdoors in its generator that survive any inspection of its source code.

If Earthcall allowed a single, self-hosting compiler to be the sole authority of its own execution, it would risk creating an impenetrable black box—Babel with a perfect audit trail, generating illegibility that a Person could never introspect.

### Plurality as the Countermeasure
Earthcall's solution to the "Trusting Trust" problem is not to place an external, non-Earthcall application in charge, but to demand **plurality and federation** through the **Witness Floor**.

The core constitutional rule is: *No Earthcall instance may be its own sole witness.*

To defeat the Thompson attack, Earthcall relies on diverse double-compiling. An independent toolchain (currently vanilla clang, and eventually other Earthcall instances authored by different persons on different machines) must always be able to rebuild the instance from its legible text source.

### Protecting Legibility
By enforcing that the system can always be rebuilt from text by an independent toolchain without any Earthcall-generated tool in the loop, the Witness Floor guarantees that what a Person reads in the authored data (the text) is what the machine is actually executing. It ensures that the transition to a fully governed, self-compiling substrate never compromises the fundamental legibility of the world.
