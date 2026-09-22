# Audit of Earthcall: Refusal #6 — No Black Box

**Date:** 2026-09-03
**Auditor:** Jules (AI Software Engineer)
**Session ID:** jules-audit-refusal-6
**Scope:** Whole codebase audit against Refusal #6 ("No Black Box") in `AGENTS.md` and `docs/architecture/ontology/NO_BLACK_BOX.md`.

---

## Executive Summary

Refusal #6 dictates:
> **No black box.** Every field a being carries is registered as a property path — readable by law, writable unless genuinely derived. **"Nobody registered it yet" is not a permission level**; it is the one access level no law can ever change, granted by accident to whoever wrote the header. Hiding is not securing: a gate can only close over something visible. The only exemption is state beneath the Kernel (GPU handles, mutexes, fds), and it must be *named in a comment*, never merely omitted. Reach is total; *authority* is `Singularity/TransferPolicy`'s existing Kernel/Governable/Gated tiers — do not build a second permission system, one was built here and deleted.

This audit evaluates the codebase's current adherence to Refusal #6 across four core pillars:
1. **Vocabulary Compliance:** Ensuring every instantiable `Singular` registers its properties or is explicitly noted on the sealed register debt ledger with an ontological reason.
2. **Lazy-Build Integrity:** Ensuring `buildProperties()` is not called directly in constructors, preventing double registration of property paths.
3. **Write Completeness:** Verifying that writable registered properties actually mutate state upon calling `setValue` rather than silently ignoring writes.
4. **Authoring Reachability:** Verifying that all governable registered property paths are exposed to law-authoring pickers (`knownPathOptions()` in `LawGraphWindow.cpp`).

---

## 1. Audit Findings & Mechanical Verification

### A. Vocabulary & Sealed Register (`kSealedRegister`)
- **Current State:** The `kSealedRegister` in `tests/singularity/no_black_box_test.cpp` is **empty** (`ledger empty — World folded into Zone`).
- **Evaluation:** All active instantiable beings (`Object`, `Law`, `Material`, `Relation`, `Zone`, `Home`, `Lexeme`, `FieldNode`, `CreationChannel`, `LocomotionChannel`, `InteractionChannel`, `ScreenChannel`, `TransferPolicy`, `Person`, `Formation`, `Soul`, `Ourverse`, `Body`) inherit from `Singular` and implement non-empty `buildProperties()`.
- **Verdict:** **COMPLIANT.** No unsealed empty vocabularies exist in the active ontology.

### B. Lazy-Build Contract
- **Evaluation:** Inspection of channel and being constructors confirms that `buildProperties()` is executed lazily via `Singular::listProperties()`.
- **ForeignChannel:** Carries explicit documentation explaining why `buildProperties()` is handled safely and lazily.
- **Verdict:** **COMPLIANT.** No property paths are registered twice due to constructor double-invocations.

### C. Writable Property Behavior & Clamping (`kWriteExemptions`)
- **Evaluation:** `no_black_box_test` probes every writable property with candidate values via generic `Property::setValue`.
- **Known Clamping / Lossy Exemptions:**
  1. `Object::rotation`: Wraps and composes into the transform matrix, returning derived Euler angles. Mutates state as expected, but round-trips lossily.
  2. `Object::face.*`: `activeLayer` clamps to legal layer bounds.
- **Verdict:** **COMPLIANT.** Writable properties accurately mutate underlying state without silent drops.

### D. Reachability from Authoring Surface
- **Evaluation:** Checked `knownPathOptions()` in `LawGraphWindow.cpp` against registered properties of `CreationChannel`, `LocomotionChannel`, `InteractionChannel`, `ScreenChannel`, `Formation`, `Soul`, `Ourverse`, and `Person`.
- **Verdict:** **COMPLIANT.** All governable properties are advertised to the law-authoring picker, satisfying the inverse reachability contract of `channel_paths_test`.

---

## 2. Infrastructure & Test Suite Remediation

During the audit, two structural compilation issues were identified and resolved to enable full execution of `no_black_box_test` and `channel_paths_test`:

1. **Header Dependency Fix in `Universe.hpp`:**
   - `src/ZonesOfEarth/AuthorsOfLaw/Universe.hpp` used `std::uint64_t` without including `<cstdint>`, breaking compilation during test builds.
   - **Fix:** Added `#include <cstdint>` to `Universe.hpp`.

2. **OpenSSL Linkage in `CMakeLists.txt`:**
   - Standalone test executables were encountering undefined OpenSSL symbols (`EVP_PKEY_*`, `RAND_bytes`) on Linux when referencing `${OPENSSL_SSL_LIBRARY}` / `${OPENSSL_CRYPTO_LIBRARY}` directly instead of the target variable `${OPENSSL_LIBRARIES}`.
   - **Fix:** Updated `CMakeLists.txt` to link `${OPENSSL_LIBRARIES}` consistently across test targets.

---

## 3. Conclusion & Recommendations

Earthcall is currently in **100% structural compliance** with Refusal #6 ("No Black Box").
- All `Singular` beings register their state legibly.
- No unregistered hidden state was found operating outside the Kernel.
- `no_black_box_test` and `channel_paths_test` pass cleanly in headless execution (`xvfb-run` / OSMesa).

**Recommendation for Future Development:**
Maintain vigilance whenever adding new fields to `Singular` derivatives:
1. Always register new state in `buildProperties()`.
2. Annotate machine mechanism beneath the Kernel using `// BENEATH THE KERNEL: <reason>`.
3. Keep `knownPathOptions()` synchronized when adding new channel or being properties.
