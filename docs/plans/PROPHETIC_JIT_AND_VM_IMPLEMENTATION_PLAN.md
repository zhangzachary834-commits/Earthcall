# Prophetic JIT & Native Bytecode VM Implementation Plan

## 1. Overview and Telos
This document outlines the step-by-step technical implementation plan for Earthcall's **Execution Modality** — the engine that physically runs authored Laws. It actualizes the theoretical mathematics proven in `docs/Analysis/LAW_EXECUTION_TRADEOFFS_ANALYSIS.md`.

The goal is to implement the `src/Singularity/Execution/` skeleton, migrating Law execution from C++ AST-walking to a dual-state **Bytecode VM** and **Prophetic JIT Compiler**.

## 2. Phase 1: The Native Bytecode VM (The Authoritative Fallback)
Before building the JIT, we must build the safe, W^X-independent fallback that handles dynamic shapes natively.

### Step 1.1: Define the Instruction Set Architecture (ISA)
*   **Action:** Fleshing out `NativeBytecodeVM::Opcode`.
*   **Design:** A register-based VM (faster than stack-based for property manipulation). 
*   **Instructions needed:**
    *   `LoadProperty r_dest, r_target, StringId_path`
    *   `StoreProperty r_target, StringId_path, r_src`
    *   `MathAdd r_dest, r_lhs, r_rhs`, `MathSub`, `MathMul`
    *   `CmpEq`, `CmpGt`, `BranchIfFalse`

### Step 1.2: AST to Bytecode Compiler
*   **Action:** Implement `BytecodeVM::emit(const Law& law)`.
*   **Design:** Traverse the existing `ConditionNode` and `ActionNode` ASTs. Instead of generating C++ closures (`std::function`), generate a contiguous `std::vector<Instruction>`.
*   **String Interning Integration:** Ensure all property paths are resolved to their `StringId`s at compile-time and baked directly into the bytecode operands.

### Step 1.3: The Execution Loop
*   **Action:** Implement `BytecodeVM::execute()`.
*   **Design:** A massive, flat `switch(instruction.op)` loop. 
*   **Performance:** Uses computed gotos (if compiler supported) or tightly optimized switch statements. `LoadProperty` uses the new 1-cycle SoA `findProperty(StringId)` lookup we just merged.

## 3. Phase 2: The Prophetic Index
The mathematical shield that enables the JIT to drop Bailout Guards.

### Step 2.1: Rete Graph Disjointness Query
*   **Action:** Implement `PropheticIndex::queryStructuralDisjointness()`.
*   **Design:** When the JIT asks "Does anything mutate `@position` on `Archetype X`?", the Index walks the Prophetic Rete. 
*   **Return Values:**
    *   `Disjoint`: No laws in the current universe structural-write to this path.
    *   `Intersects`: A law exists that *might* mutate this structure.

### Step 2.2: The Invalidation Hook
*   **Action:** Wire `Universe::setApplicationOnset` and `Zone` structural changes to `PropheticIndex::invalidate()`.
*   **Design:** If a player authors a new law that mutates structure, the Index drops, and `ExecutionChannel` is signaled to flush the JIT cache.

## 4. Phase 3: The Prophetic JIT Compiler (The 1.0x Engine)
Translating ASTs directly into unguarded x86_64/ARM assembly.

### Step 4.1: LLVM ORC JIT Setup
*   **Action:** Initialize LLVM context, module, and `ExecutionEngine` inside `PropheticJIT`.
*   **Design:** Create a fast, thread-safe JIT instance using LLVM's ORC (On-Request Compilation) API.

### Step 4.2: AST to LLVM IR Generation
*   **Action:** Map Earthcall operations to LLVM IR Builder.
*   **Design:** 
    *   Walk the Law AST.
    *   Query `PropheticIndex`.
    *   **If Disjoint:** Emit raw GEP (GetElementPtr) instructions to calculate the exact byte offset of the property. Emit a raw `load`/`store` instruction. (0 allocations, 0 guards).
    *   **If Intersects:** Emit a call to the C++ `lawGetValue` fallback function.

### Step 4.3: Compilation & Caching
*   **Action:** Implement `PropheticJIT::compileUnguarded()`.
*   **Design:** Compile the IR module to a raw function pointer `void(*)(Singular& target)`. Cache it in an `std::unordered_map<LawId, NativeLawClosure>`.

## 5. Phase 4: The Execution Channel (Orchestration)
Wiring it all together seamlessly.

### Step 5.1: The Routing Logic
*   **Action:** Flesh out `ExecutionChannel::executeLaw`.
*   **Design:** 
    ```cpp
    if (isJITActive && jitCache.contains(law)) {
        jitCache[law](target);
    } else {
        vm.execute(vmCache[law], target);
    }
    ```

### Step 5.2: Asynchronous Warm-Up
*   **Action:** Implement `ExecutionChannel::warmCaches()`.
*   **Design:** When a Zone loads, instantly compile all Laws to Bytecode (takes microseconds, unblocks the main thread). Spin up a background thread to feed the LLVM JIT. As each law finishes JIT-compiling, seamlessly hot-swap the pointer in the execution cache.

## 6. Verification Plan
*   **Unit Tests:** Write bytecode emission tests proving ASTs map to correct Opcode sequences.
*   **JIT Tests:** Write tests proving that LLVM emits unguarded offsets when `PropheticIndex` returns `Disjoint`.
*   **Performance Benchmark:** Run `frame_lag_test` to prove JIT execution hits the 1.0x native C++ baseline (matching a hardcoded `struct` modification).
