# Dead members in `Singular` cost memory

**Status:** done and verified
**Section in the To-Do list:** Housekeeping / Findings from Clawd MYTHOS Audit

---

## Description
In the Clawd MYTHOS (Claude Fable 5.1) Audit (2026-09-01), two dead members were identified in `Singular` (`parentFormationInstances` and `childFormationInstances`), which were vectors of `Formation` instances that were never read or written, but added overhead to every `Singular` instance and during copy/move operations. Additionally, `virtual bool satisfiesKernelBounds() const` was an uncalled placeholder method on `Singular`.

## Actions Taken
1. Removed `parentFormationInstances` and `childFormationInstances` from `src/ConstructedBeing/Singular/Singular.hpp`.
2. Removed `satisfiesKernelBounds()` from `src/ConstructedBeing/Singular/Singular.hpp`.
3. Added `#include <cstdint>` to `src/ZonesOfEarth/AuthorsOfLaw/Universe.hpp` for `std::uint64_t`.

## Verification
- Built `earthcall_core` and all C++ test targets using `cmake --build build`.
- Ran `./build/singular_copy_move_test` (50/50 checks passed).
- Ran Python unit test suite `PYTHONPATH=. python3 -m unittest discover -s tests/singularity/py` (100% passed).
