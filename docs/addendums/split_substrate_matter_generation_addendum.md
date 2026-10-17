# Addendum: Integrating Split-Substrate Serialization, Matter Generation, and Atomic Save Swaps

## Semantic root and physical sidecar

Earthcall separates the semantic save root (`.ecform`) from physical matter bytes (`.ecmatter`). In [ZoneManager.cpp](../../src/ZonesOfEarth/ZoneManager.cpp), the file-local `commitMatterGeneration` helper handles **nonempty** matter bytes: it computes a SHA-256 digest, uses its first 16 hexadecimal characters as `snapshotId`, and writes a `<stem>.<snapshotId>.ecmatter` sidecar. It records `snapshotId`, full `sha256`, `byteLength`, and `schemaVersion` in the semantic JSON object as `matterGeneration`.

The helper reads any prior generation metadata from the existing `.ecform` using `SaveSystem::readSaveData` and returns the predecessor's sidecar path when different. The caller is responsible for committing the semantic root **after** the sidecar and retiring the predecessor only after successful root commit. This is a generation-coupling protocol, not a guarantee that every possible save or filesystem failure is automatically recoverable.

## Read-side checks and format

`readVerifiedMatterGeneration` checks the named generation's existence, schema version, byte length, and SHA-256 before admitting its physical bytes. Legacy saves without `matterGeneration` can use a separate fixed-name sidecar compatibility path. The `.ecform` semantic root is encoded using MessagePack (including a JSON-string wrapper in relevant save paths); it is **not** inherently a human-readable text file.

## Atomicity boundaries that still matter

The local `atomicWriteFile` helper writes a temporary file, flushes its C++ output stream, and attempts a filesystem rename. If rename fails, its fallback uses `copy_file(..., overwrite_existing)` followed by temporary-file removal. The code shown does not perform an explicit durable `fsync` of file and directory. Therefore the strongest blanket claims—an indivisible whole-world macro-moment, guaranteed crash-proof durability, or perfect reversibility on every platform—are **not established** by this helper alone.

## Verification anchors

- [ZoneManager.cpp](../../src/ZonesOfEarth/ZoneManager.cpp): `atomicWriteFile`, `commitMatterGeneration`, `readVerifiedMatterGeneration`, and predecessor cleanup.
- [SaveSystem.cpp](../../src/Singularity/Storage/SaveSystem.cpp): semantic save encoding/decoding and save-path behavior.
