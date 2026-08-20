# tests/ — registered ctest suite

Organized by ontological region, the same way `scratch/` is organized by purpose.
CMake globs `tests/**/*.cpp`; the target and `ctest` name is the **file stem**
(`law_loop_test.cpp` → `law_loop_test`), so moving a file does not rename the
target. **Stems must stay unique.** New tests go in the matching directory, not
loose at this root.

```
tests/
  support/              headers shared by more than one test (not a ctest)
  constructed-being/    Singular, Object, ObjectConcept, Formation, Material, Property
  person/               Person, Soul, Body, First Mover actuation
  zones/                Zone, Home, World, Physics, Ourverse, Time
  law/                  AuthorsOfLaw — Law, metalaw, RETE, persistence, spawn
  singularity/          modality channels, OntoMath, Screen, Storage, Foreign, Input
  identity/             First Mover register, identity ledger
```

`test_save_helper.hpp` lives in `support/`. Law tests include it as
`"test_save_helper.hpp"`; CMake adds `tests/support` to the include path.

**Scratch probes belong in `scratch/probes/`, not here.** If you build a probe
through a test target for convenience, remove the copy and reconfigure.

The tests that guard shipped bugs (`paint_test`, `object_roundtrip_test`,
`channel_paths_test`, `no_black_box_test`, `ground_plane_test`,
`test_observation_load_test`) are described in
`docs/BUILD_AND_ENVIRONMENT.md`. `webgpu_particle_test` is a pending-feature
target (excluded from the default build).
