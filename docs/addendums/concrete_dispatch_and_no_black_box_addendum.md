# Concrete Dispatch, Kernel Refusals, and Visible Transfer Gates

Earthcall's Singular birth path must not mistake an authored property for authority to synthesize a real Person. Concrete type checks enforce a non-negotiable structural boundary, while the transfer gate examines exposed source properties. These are complementary safeguards, not proof that every native field is Law-governed.

## What the current implementation actually does

In `src/ConstructedBeing/Singular/Creation/SingularSetToSetCreation.cpp`, `derive()` calls `kernelRefusal(request.prototype)` **before** considering property transfer. The refusal rejects `Person`, `Body`, `BodyPart`, `Soul`, and `PersonPerspective` prototypes, along with `Identity::FirstMover`, `Identity::FirstMoverRegister`, and other kernel-only kinds. These checks **refuse synthesis**; they do not construct a valid Body or certify a newborn Person.

After the refusal checks, `derive()` calls `TransferPolicy::instance().canTransfer(PropertyPath::parse(...))` for each property returned by `listProperties()` and for each dynamic property. If a source gate is closed, whole-prototype birth is refused rather than silently dropping the field. Only then does the code choose its concrete Law, Object, ObjectConcept, or stored-Singular codec. Zone birth is explicitly not yet wired.

## Connection to the No Black Box principle

The kernel type boundary protects Person identity and trusted authorship from being fabricated through an arbitrary prototype. The property gate makes the **enumerated source property transfer decision** explicit. Neither mechanism establishes that all native state is exposed through property paths, that all mutations are authored Law, or that no other C++ checks exist.

The engineering obligation is therefore twofold: preserve these kernel refusals, and continue making state and authority legible through appropriate authored interfaces without claiming the transfer gate is a universal transparency proof.
