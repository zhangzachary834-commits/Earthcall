# The save-tree test guard leaks gigabytes and can truncate saves

**Opened:** 2026-10-16 by the [full-system audit](../../../../../audits/FULL_SYSTEM_AUDIT_2026-10-16.md#4a-incident-caused-by-this-audits-own-test-run-and-its-repair) (F13). Claude Code / Claude Opus 5.5 / session `d6fe31dc-53d9-49c1-a4b7-87479d9d605b`.

`TestSupport::RealSaveTreeGuard` (`tests/support/test_harness.hpp`, 15 users) does three things:
- copies all of the checkout's `saves/` (about 5.3 GB, dominated by the ignored `backups/before-load.ecform`) to `$TMPDIR/earthcall-save-backup-<n>` per test;
- removes the copy only in its destructor, so aborts and ctest timeouts leak it;
- restores with `remove_all(original)` followed by `copy(snapshot)`.

On 2026-10-16 one full `ctest` filled the disk (280 MB free), and a restore left `before-load.ecform` truncated in the checkout. It was repaired from a snapshot; see the audit §4a.

**Done when:**
- The guard snapshots only what the test can write, or uses APFS clones (`clonefile`, `copyfile(COPYFILE_CLONE)`), which cost no space until written.
- Restore stages next to the original and renames atomically, so it never deletes the original first.
- Leftover snapshots from dead processes are reaped at the next guard start, after verifying that their owner PID is gone, and the reaped paths are logged.
- A guard refuses to start when free space is below a snapshot's size, and says so.

**Cleanup only Zach should do:** 18 older leaked folders (about 49 GB, Oct 6–13) remain in `$(getconf DARWIN_USER_TEMP_DIR)`. They are copies of the checkout's `saves/` and contain no live data-home content.

## Status 2026-10-16: mostly done

`cloneOrCopySaveEntry` uses APFS `clonefile()`, falling back to a deep copy. Restore stages `.earthcall-restore-<entry>` beside the original and renames it in. Measured: a guarded test moved free space by about 2 MB instead of 5.3 GB. Still open: an `abort()` in a test skips the destructor, so `saves/` is left dirty (now at no disk cost), and reaping of dead snapshots is not done.
