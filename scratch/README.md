Sc# scratch/ — the workshop floor

One-off probes, migration scripts, and superseded snapshots. Nothing here is linked by
CMake (see root `CMakeLists.txt`: `scratch/.*` is excluded). Move, never discard.

```
scratch/
  probes/       C++ verification probes referenced by architecture docs
  legacy/       superseded Game-era source snapshots (UI migration reference)
  scripts/
    migrate/    save-format migration tools and fixtures
    refactor/   bulk rename / compile-fix scripts from the Engine refactor
  fixtures/     generated artifacts (e.g. probe output WAV)
  audits/       scratch-local audit reports (not `docs/audits/`)
  experiments/  self-contained side projects (puppeteer, mock website)
  attic/        oldest strays from the sight-cpp era — binaries, logs, one-offs
```

**New probes** go in `probes/`, not loose at this root. **New one-off scripts** go under
`scripts/migrate/` or `scripts/refactor/` depending on purpose.
