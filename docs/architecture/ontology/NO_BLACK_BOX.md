# No Black Box

*The sixth refusal. Companion to `LAW_AND_CREATION_SYSTEM.md` (what a Law is),
`AUTHORED_CATEGORIES.md` (what a kind is), and `Singularity/TransferPolicy` (the one gate).*

---

## 0. The refusal, in one sentence

**A field a Person cannot address is a field a Person cannot govern, and Earthcall does not
ship state that governs Persons from behind their backs.**

Every piece of state a being carries is either

1. **registered** — it has a property path, laws can read it, and the gate decides who writes it; or
2. **beneath the Kernel** — it is machine mechanism rather than a being's state, and it is
   *named as such in writing*.

There is no third case. In particular, **"nobody has registered it yet" is not the second
case.** That is the black box, and it is refused.

---

## 1. Why this is a refusal and not a style preference

The other five refusals stop the type system from deciding *what things are*. This one stops
it from deciding *what may be known and changed about them*.

They are the same principle at two depths. A `RobotEntity` class asserts that "robot" is a
fact about the world that Persons may not revise. A private `float _decayRate` asserts the
same thing about decay — more quietly, and therefore worse. Nobody argues with it, because
nobody can see it. The world simply behaves a certain way and no authored law can reach the
reason.

The refusal is not "expose everything because transparency is nice." It is structural:

> **Governance requires legibility first.** A gate can only close over something visible. An
> unregistered field is not "maximally protected" — it is *ungoverned*, permanently, in
> whatever state the last C++ author left it. Hiding is not securing. It is the one access
> level no law can ever change, granted by accident, to whoever wrote the header.

This is why the refusal names the black box rather than the leak. Earthcall's failure mode
is not that a Person sees too much; it is that the substrate quietly keeps a lever to itself.

---

## 2. The bound: Kernel / Singularity

The refusal is total in its *reach* and bounded in its *authority*. Those are different
axes, and conflating them is the usual mistake.

**Reach** — what is visible — is total. Every field of a being is registered.

**Authority** — who may write it — is decided by `Singularity/TransferPolicy`, which already
exists and is already the only gate. **Do not build a second one.** A `PropertyGovernance`
enum sat in `Property.hpp` for exactly this purpose and was deleted; the comment at the top
of that file is its headstone, and it explains why two permission systems that can disagree
are not twice the governance but the absence of it. Ask TransferPolicy.

Its three tiers are the bound this refusal operates inside:

| Tier | Meaning | Who may close it |
|---|---|---|
| **Kernel** | Universally transferable. Registered **read-only**. The anti-tyranny ceiling. | Nobody. Laws cannot close the floor. |
| **Governable** | Open by default; a law may close and reopen it. The default for unlisted paths. | Any authored law with standing |
| **Gated** | Closed by default; a law must open it. | Any authored law with standing |

So a field's *standing* is a first-class, authored, persisted, legible decision — the policy
is itself a Singular whose gates register as ordinary bool properties
(`@transfer-policy.gate.shape := false`). A field's *existence* is not a decision at all.
It is registered, always.

### 2a. Kernel guards are not black boxes — they are the opposite

The one thing that legitimately refuses a Person in C++, unconditionally, is a kernel guard
on the path to a Person's body: the audio channel's infrasound floor
(`ONTOMATH_FRAMEWORK.md` §7a) is the worked example.

A guard is not a hidden field. It is a **loud refusal at the channel**:

- It **names what it refused and why** — "this channel does not sound infrasound at 7 Hz" —
  rather than silently filtering.
- It **constrains the path to the body, never the mathematics.** A Person may still author,
  integrate, inspect, and render a 7 Hz field anywhere else. What is refused is the speaker,
  not the thought.

A guard that achieved its refusal by *hiding the frequency field* would violate this
refusal, and would also be worse at guarding — the Person would conclude the field did
nothing rather than learn where the boundary is. **Refuse out loud; never refuse by
concealment.**

### 2b. What is actually beneath the Kernel

Exempt state is machine mechanism, not a being's state. It has no meaning to a Person
because it has no referent in the world:

- OS and driver handles — GPU buffers, textures, pipelines, socket fds, file descriptors
- Synchronization primitives — mutexes, condition variables, atomics
- Raw memory the being does not own semantically — scratch buffers, staging arenas
- Per-frame derived caches that are recomputed from registered state anyway

The test for exemption is **not** "is it a pointer" or "is it hard to expose." It is:

> **Could a Person mean something by changing this?**

If a Person could mean something by it, it is state, and it is registered — *even if it is
a pointer, even if it is expensive, even if the only sane tier is Kernel read-only.*

`World::_playerEyeHeight` is a float that decides where a Person's viewpoint sits relative to
their feet. There is no reading of "beneath the Kernel" that covers it. It is unregistered
today because `World::buildProperties()` is `{}` — the accident this refusal exists to name.

---

## 3. The admission test — four questions before you add a field

Adding a field to a being is an ontological act, the same as adding a class. Ask, in order:

1. **Is this the being's state, or the machine's mechanism?**
   Machine mechanism → §4 sealed register, with a reason. Being's state → continue.

2. **Could a Person mean something by reading it?**
   If yes it must be readable — no exceptions, this is the whole refusal.
   If you believe the answer is no, you are almost certainly describing something derived;
   register it as a `ComputedProperty` and move on.

3. **Could a Person mean something by writing it?**
   If no → register **read-only** (`ComputedProperty` with a null setter). Read-only is a
   real answer and a common one; it is not a black box, because the law can still see it and
   reason from it.
   If yes → continue.

4. **What is its tier?**
   Default is Governable — say nothing and it is Governable-open, which is usually right.
   Declare `Kernel` only for an anti-tyranny floor no law may close. Declare `Gated` only
   when the default posture is genuinely closed.

Note what is missing from this list: *"is it convenient to expose?"* and *"will anyone use
it?"*. Neither is a question. `Object::faceColors` was write-only for a month and
`propSetColor` was an empty function for a month, both because nobody was going to use them
*yet*.

---

## 4. The sealed register

A field exempt under §2b is not simply left out. Silence is indistinguishable from
forgetting, and forgetting is what this refusal is about. It is **named**, at its
declaration, with a reason:

```cpp
private:
    // BENEATH THE KERNEL: a wgpu buffer handle. Not the being's state — the
    // machine's mechanism for holding it. A Person can mean nothing by it;
    // the geometry it holds is registered as "shape.*".
    WGPUBuffer _vertexBuffer = nullptr;
```

And `buildProperties()` is never empty for a being that has state. If a being genuinely has
nothing to register, that fact is written down too:

```cpp
    // Nothing to register: Perspective holds no state of its own. Its viewpoint
    // is the Person's, addressed through @person.position.
    void buildProperties() override {}
```

An empty `buildProperties()` with no comment is a black box, and `no_black_box_test` will
say so by name.

---

## 5. The procedure

Adding governable state to a being:

1. Add the field.
2. Register it in that being's `buildProperties()` —
   `PropertyRef` over a plain member, `ComputedProperty` over getters when the truth is
   derived (`Object::position` lives in the transform, not in a vec3).
3. Give it a **stable name.** Law text addresses it forever; it is as permanent as an enum
   value. Dotted names group (`shape.r`, `face.count`).
4. If the being's paths are offered in the authoring picker, add it to `knownPathOptions()`
   in `LawGraphWindow.cpp` — a registered property the picker never offers is governable in
   principle and unreachable in practice.
5. Declare a tier in `TransferPolicy` **only if** it is not Governable-open.
6. If `to_json` writes it, add it to `object_roundtrip_test`. A property that does not
   survive a save was never granted.

---

## 6. The two failure modes, both of which have shipped here

This refusal has teeth because both halves of it have already failed silently in this
repository.

**The unregistered field.** `CreationChannel::activeShapeKind` existed, the picker offered
it, and `buildProperties()` never registered it. Every law that spawned a shape from the
author's live selection silently fell back to the concept's template. The law did something
subtly other than what it said, for as long as it took someone to write a test that *used*
the path. Guarded now by `channel_paths_test`.

**The registered field that does not answer.** `propSetColor` was an empty function for a
month. The property was registered, the picker offered it, the law wrote it, the write
returned success, and nothing changed. Guarded now by `paint_test`.

Note that these are opposite directions of the same promise, and each needed its own guard:

- `channel_paths_test` — **advertised ⊆ registered.** Nothing is offered that cannot answer.
- `no_black_box_test` — **field ⊆ registered**, and **registered ⊆ advertised**. Nothing a
  Person could mean something by is hidden, and nothing registered is unreachable from the
  authoring surface.

---

## 7. What the test can and cannot check

C++ has no reflection, so no test can enumerate a class's private fields and diff them
against the registry. `no_black_box_test` therefore checks the four things that *are*
mechanically checkable, and §4's written reason carries the rest:

1. **No silent empty vocabulary.** Every instantiable being registers at least one property,
   or appears on the test's own sealed register with a stated reason. This catches
   `buildProperties() override {}` added without thought.
2. **The lazy-build contract.** No name is registered twice. `Singular` builds the registry
   lazily behind `_propertiesBuilt`; a constructor that *also* calls `buildProperties()`
   does not set that flag, so the whole vocabulary is registered again on first access.
3. **Writable means it writes.** Every non-read-only registered property, written through
   the generic `Property::setValue` door the Law system uses, reads back what was written.
   This is the `propSetColor` class of failure, generalized.
4. **Registered means reachable.** Every registered property of a picker-backed being is
   offered by `knownPathOptions()` — the inverse direction from `channel_paths_test`.

**Its first run found two live bugs**, which is the argument for the check existing:
`CreationChannel`'s constructor called `buildProperties()`, so all 21 of its properties were
registered twice and the picker offered every creation path twice; and 15 of those 21 —
the entire manual-anchor and spawn frame, grid snapping, placement mode — were governable
but offered nowhere, reachable only by a Person who already knew the path by heart. Both are
fixed; the channel is now *probed* into `knownPathOptions()` rather than hand-listed, so that
particular drift cannot recur.

### The two ledgers

Both lists in that test are **debt ledgers, not allowlists.** Entries are expected to leave.

`kSealedRegister` names beings that register nothing. As of 2026-08-13: `World`, `Ourverse`,
`Formation`, `Soul`. (`Perspective` is *not* on it — `Perspective.cpp` is empty, so its
constructor is declared and never defined. It is an uninstantiable stub, not a being with
hidden state.) Taking a name off is the work; adding one requires a reason that survives §3.

`kWriteExemptions` names properties where check 3 cannot mechanically distinguish "the
setter clamped my request to the only legal value" from "the setter ignored my request,"
because it does not know each property's legal domain. Every entry records what was
*measured*, by probe, not what the setter looks like it does. Two today:

- `Object::rotation` — writes, but round-trips lossily. `(1,2,3)` degrees reads back as
  `(1.104, 1.945, 3.036)`: the setter wraps and composes into the transform, and the getter
  re-derives Euler angles from the matrix. Not a black box — the object does move — but a
  law that writes then reads gets a different number than it wrote.
- `Object::face.*.activeLayer` — clamps to `[0, layers-1]`, and a probe object has one
  layer, so `0` is the only legal value and no perturbation can round-trip.

---

## 8. Relation to the other refusals

| Refusal | Stops the type system from deciding… |
|---|---|
| 1. No class for a domain noun | what kinds of things exist |
| 2. No top-level directory for a subsystem | what regions of being exist |
| 3. No enum value for a kind | what categories exist |
| 4. `Body` is for Persons | what counts as embodiment |
| 5. `Person` means Human | who counts as someone |
| **6. No black box** | **what may be known and changed about any of them** |

The general form of all six is unchanged: **no subsystem may define what a thing IS.**
Refusal 6 adds the corollary that a subsystem may not define what a thing's state *means*
by keeping it where no law can look.
