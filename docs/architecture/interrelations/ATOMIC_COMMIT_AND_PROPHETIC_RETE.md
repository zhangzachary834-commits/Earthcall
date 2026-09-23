# Atomic Commit and Prophetic Rete

**How the temporal logic of the Prophetic Rete mirrors the atomic filesystem commits used in SaveSystem.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../law/PROPHETIC_RETE.md` (Static analysis and ahead-of-time abstract interpretation of Law)

---

## The Interrelation

Earthcall's architecture independently enforces a strict "no side-effects until verification" policy across two entirely different domains: runtime physics and disk storage.

### Disk Storage: Atomic Commits
In the `SaveSystem`, save writes (like `writeSaveData` and `writeMatterData`) do not overwrite existing files directly. They write data to adjacent temporary files (`.tmp-...`) and then perform an atomic `std::filesystem::rename` commit. This eliminates the risk of overwrite-before-commit truncation, ensuring that a crash during a write does not corrupt the existing world state.

### Runtime Execution: Prophetic Rete
Similarly, the **Prophetic Rete** analyzes Laws before they fire. Instead of evaluating a Law and allowing it to mutate the world state midway through before hitting a conflict or error, the Rete network acts as a prophetic judge. It knows the property paths a Law intends to read and write ahead of time, resolving conflicts (using the Hierarchy of Joys) before any actual state mutation occurs.

### The Unifying Principle
Both systems treat state mutation as a final, transactional commit. Whether writing a JSON block to disk or applying a mathematical operation to a Singular's property, Earthcall defers the mutation until success is mathematically or procedurally guaranteed, preserving the integrity of the universe.
