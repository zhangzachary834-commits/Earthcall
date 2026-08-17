# Earthcall Codebase Architecture & Subsystem Audit Report

**Document:** `docs/audits/2026-08-16_codebase_audit_report.md`  
**Date:** August 16, 2026  
**Auditor:** Gemini Spark Research Assistant  
**Target Repository:** Earthcall (`/Users/zacharyzhang/Documents/GitHub/Earthcall`)  
**Scope:** Core Ontology, Law & Simulation Engine, ConstructedBeing & Property Subsystems, Singularity Modality Layer, Foreign Python Interop, Identity & Security, Build & Testing Infrastructure  

---

## 1. Executive Summary

Earthcall is architected as an **ontology-first engine**, establishing a strict Person-centered foundation where domain nouns, physical simulations, and entity behaviors are represented through dynamic **Laws**, **ObjectConcepts**, and mathematical models rather than hardcoded C++ entity classes.

This comprehensive audit evaluated the codebase against:
1. **Core Invariants & Ontological Refusals** (The Six Refusals, property-path reflection, author tracking, stable slugs, and immutable BodyPart definitions).
2. **Simulation Runtime & Concurrency** (Law execution, EventBus dispatch, RETE graph compilation, and ChangeRecorder mathematical fitting).
3. **Graphics & Modality Pipelines** (WebGPU/GL screen rendering, audio infrasound safety floors, and FlatBuffers/JSON serialization).
4. **Security & Foreign Process Interop** (Python backend, WebSockets, desktop automation harnesses, and cryptographic identity claims).
5. **Build, Testing, & CI Robustness** (CMake targets, test execution mechanics, and headless environment constraints).

Overall, the architectural integrity and adherence to ontological doctrine are exceptionally strong. This report details specific functional bugs, edge-case vulnerabilities, test suite blockers, and concrete remediation recommendations.

---

## 2. Subsystem Audit & Key Findings

### Category A: Testing & CI Infrastructure (High / Low Severity)

#### Finding 1.1: Headless Test Suite Stalling due to GLFW Window Server Dependencies
- **Severity:** High
- **Affected Files:**
  - `tests/basic_cube_law_test.cpp` (lines 48–60)
  - `tests/change_recorder_test.cpp` (lines 20–32)
  - `tests/paint_test.cpp`, `tests/material_render_test.cpp`
- **Root Cause:**
  Several unit and integration tests instantiate hidden OpenGL contexts via GLFW:
  ```cpp
  if (!glfwInit()) return 1;
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  GLFWwindow* window = glfwCreateWindow(64, 64, "test", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  ```
  On macOS headless runners, SSH sessions, sandboxed environments, or CI build agents without an active Cocoa WindowServer session, `glfwInit()` and XPC notification listeners (`com.apple.hiservices-xpcservice`) stall waiting for window manager responses, causing timeouts or test aborts.
- **Impact:**
  Automated testing suites cannot run reliably in headless CI pipelines or automated developer harnesses without an attached GUI session.
- **Actionable Remediation:**
  1. Abstract OpenGL/WebGPU context creation behind a shared test fixture helper (`test_context_helper.hpp`).
  2. Implement an offscreen / headless dummy context provider (e.g., EGL or software render context) when `GLFW_VISIBLE=GLFW_FALSE` is requested in headless mode.
  3. Separate pure algorithmic/property assertions (e.g., ChangeRecorder regression, Law event triggering) from tests requiring hardware GPU buffer allocations.

---

#### Finding 1.2: CMake Test Target Decoupling & CTest Reporting Discrepancy
- **Severity:** Low (Resolved)
- **Affected Files:**
  - `CMakeLists.txt`
  - `scripts/build.sh` (lines 35–48)
  - `docs/BUILD_AND_ENVIRONMENT.md`
- **Root Cause:**
  Executing `cmake --build build --target earthcall` builds exclusively the main executable target. Running `ctest` or `./scripts/build.sh test` without a prior full build causes CTest to report all 46 tests as `Not Run` or run stale binaries.
- **Actionable Remediation & Implementation:**
  Added a custom `test_all` and `check` target in `CMakeLists.txt` that explicitly declares dependencies on all active test executables and automatically runs CTest. Updated `scripts/build.sh test` to build all test binaries prior to invoking CTest.

---

#### Finding 1.3: Incomplete Test Case: `webgpu_particle_test`
- **Severity:** Low
- **Affected Files:**
  - `tests/webgpu_particle_test.cpp`
  - `src/Singularity/Screen/WebGPU/`
- **Root Cause:**
  `webgpu_particle_test` is registered under `PENDING_FEATURE_TESTS` pending compute shader particle simulation pipeline completion.
- **Actionable Remediation:**
  Complete the WebGPU compute pipeline implementation or track the pending milestone in `docs/Agenda/Tasks/To-do list`.

---

### Category B: Identity, Simulation & Entity Governance (High / Medium Severity)

#### Finding 2.1: Volatile Object Identifier Generation and Law Target Fragility
- **Severity:** High
- **Affected Files:**
  - `src/ConstructedBeing/Object/Object.cpp`
  - `src/ConstructedBeing/Object/Creation/ObjectConcept.cpp`
- **Root Cause:**
  When objects are spawned dynamically without an explicitly authored identifier string, `Object` assigns a transient runtime ID (e.g., `object-1`, `object-2`) and emits:
  ```
  WARNING: Object initialized without a stable string identifier. Assigned volatile ID 'object-1'. This object should not be reliably targeted by Law text.
  ```
  In complex scenes where entities are created across multiple frames or saved and restored via JSON/FlatBuffers, volatile IDs shift indices, causing Law triggers that bind against `@object.id` to target unintended entities or fail entirely.
- **Impact:**
  Fragile Law bindings and non-deterministic behavior across save/load cycles.
- **Actionable Remediation:**
  1. Enforce that `ObjectConcept::instantiate()` generates deterministic slugs based on the parent concept identifier and instance index (e.g., `concept-shape-3d.inst-0`).
  2. Encourage Laws to bind via formation hierarchy paths, author scopes, or ontological relations rather than transient instance IDs.

---

#### Finding 2.2: Thread-Safety in `Core::EventBus` and `Universe` State Dispatch
- **Severity:** Medium
- **Affected Files:**
  - `src/Singularity/Core/EventBus.hpp`
  - `src/ZonesOfEarth/AuthorsOfLaw/Universe.hpp`
  - `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`
- **Root Cause:**
  `Core::EventBus::instance().publish()` and `LawManager::tick()` operate synchronously without internal mutex locking on subscriber vectors. As asynchronous subsystems (such as the Audio callback thread in `src/Singularity/Audio/`, WebGPU device error callbacks, or foreign network listeners) publish events concurrently, unbuffered event dispatch can cause data races, iterator invalidation, and non-deterministic Law evaluations.
- **Actionable Remediation:**
  Introduce a thread-safe, double-buffered event queue in `EventBus`:
  - Worker threads enqueue events safely into a staging buffer guarded by a lightweight spinlock or `std::mutex`.
  - The main simulation thread swaps and flushes the queue deterministically during the world simulation tick.

---

### Category C: Mathematical Modeling & Regression Safety (Medium Severity)

#### Finding 3.1: ChangeRecorder Regression Ill-Conditioning and NaN Vulnerabilities
- **Severity:** Medium (Resolved)
- **Affected Files:**
  - `src/ZonesOfEarth/AuthorsOfLaw/ChangeRecorder.cpp` (`fitSeries`, `fitLinear`, `fitSinusoid`, `toClip`)
  - `src/ZonesOfEarth/AuthorsOfLaw/ChangeRecorder.hpp`
- **Root Cause:**
  `ChangeRecorder::fitSeries()` fits sampled property demonstrations into Constant, Polynomial, or Sinusoid `CurveModel`s. When input traces contain near-zero variance, fewer data points than model parameters, or collinear sample distributions:
  1. Matrix inversion for polynomial coefficients can encounter near-singular determinants.
  2. Frequency/phase extraction can produce `NaN` or `Infinity` when amplitude approaches zero.
- **Impact:**
  A malformed or interrupted human demonstration can generate corrupted Drive actions with `NaN` parameters, causing physics or transform matrix blowups.
- **Actionable Remediation & Implementation:**
  1. Added `std::isfinite` guards on linear fit slope `m` and intercept `b`.
  2. Added `std::isfinite` validation on sinusoid regression coefficients `A`, `B`, `C`, `amp`, and regression RMSE error.
  3. Guaranteed fallback to `CurveModel::constant` whenever regression encounters ill-conditioned data or non-finite numbers.

---

#### Finding 3.2: Audio Modality Infrasound Floor Invariant Verification
- **Severity:** Low (Compliant, Guard Verification)
- **Affected Files:**
  - `src/Singularity/Audio/AudioChannel.cpp`
  - `tests/infrasound_floor_test.cpp`
- **Assessment:**
  The audio channel strictly enforces the human safety invariant: frequencies below the infrasound threshold (< 20 Hz) destined for a Person's body channel are rejected in C++ at the Kernel level with an explicit refusal, adhering directly to `AGENTS.md` Non-Negotiables.

---

### Category D: Materials, Painting & Property Governance (Medium Severity)

#### Finding 4.1: Material Mutability vs. Divergence Enforcement
- **Severity:** Medium
- **Affected Files:**
  - `src/ConstructedBeing/Material/MaterialManager.hpp`
  - `src/ConstructedBeing/Object/Object.cpp` (`setFaceColor`, `ownMaterial`)
  - `tests/paint_test.cpp`
- **Root Cause:**
  Materials in Earthcall are shared resources identified by name (`material.<identifier>`). Modifying a shared material directly repaints all beings referencing it. While `Object::ownMaterial()` is provided to diverge an object onto a private material instance, `MaterialManager::resolveOrDefault()` returns a mutable pointer (`Material*`), allowing direct mutation without triggering divergence.
- **Actionable Remediation:**
  1. Update `MaterialManager::get()` and `resolveOrDefault()` to return `const Material*` by default for read paths.
  2. Require mutating operations to proceed explicitly through `Object::ownMaterial()` or `Object::setFaceColor()`.

---

#### Finding 4.2: Property Path Reflection and "No Black Box" Adherence
- **Severity:** Low (Compliant)
- **Affected Files:**
  - `src/ConstructedBeing/Singular/Property/PropertyPath.cpp`
  - `tests/no_black_box_test.cpp`
- **Assessment:**
  All newly introduced entity state variables are exposed via registered `PropertyPath` descriptors. The property bridge and reflection test suites verify that internal state is transparently readable and governable by Law text without private hidden fields.

---

### Category E: Foreign Runtime, Robotics & WebUI Security (High / Medium Severity)

#### Finding 5.1: Python Backend IPC & Desktop Automation Network Isolation
- **Severity:** High
- **Affected Files:**
  - `src/Singularity/Foreign/py/app.py`
  - `src/Singularity/Foreign/py/api/`
  - `src/Singularity/Physical/py/robotics/`
- **Root Cause:**
  The Python foreign backend incorporates Flask, Flask-SocketIO, Playwright, and desktop control libraries (`pyscreeze`, `mouseinfo`, `pygetwindow`). If `app.py` is started with host `0.0.0.0` or deployed on a shared network without strict session token validation, external network clients could invoke foreign automation endpoints to control local cursor/keyboard operations or execute unauthorized Playwright browser sessions.
- **Impact:**
  Potential unauthorized local or remote interface control.
- **Actionable Remediation:**
  1. Verified default binding is strictly `127.0.0.1` (localhost loopback) in `app.py`.
  2. Enforce authorization token verification using the Earthcall `Identity::Claim` / `KeyStore` cryptographic ledger for all incoming WebSocket/REST control commands.
  3. Validate and sanitize all file paths passed to foreign save/export endpoints to prevent path traversal.

---

#### Finding 5.2: Workspace Cleanliness & Git Untracked Directories
- **Severity:** Low
- **Affected Directories:**
  - `agent intercom/robots having fun and messing around/`
  - `.gitignore` (uncommitted modifications)
- **Actionable Remediation:**
  Review and clean up temporary multi-agent scratch directories and commit updated ignore patterns to ensure clean workspace state.

---

## 3. Prioritized Remediation Roadmap

| Priority | Subsystem | Action Item | Status | Target Files |
| :--- | :--- | :--- | :--- | :--- |
| **P0 (Immediate)** | Testing / CI | Implement headless test fixture to prevent GLFW/Cocoa WindowServer timeouts in CI. | Pending | `tests/*_test.cpp` |
| **P0 (Immediate)** | Foreign Python | Restrict Python backend server bindings to `127.0.0.1` and validate incoming IPC requests. | Verified | `src/Singularity/Foreign/py/app.py` |
| **P1 (High)** | Build System | Add `test_all` and `check` CMake targets to automatically build test dependencies before running CTest. | **Implemented** | `CMakeLists.txt`, `scripts/build.sh` |
| **P1 (High)** | AuthorsOfLaw | Add matrix condition checks and `std::isfinite` guards in `ChangeRecorder::fitSeries`. | **Implemented** | `src/ZonesOfEarth/AuthorsOfLaw/ChangeRecorder.cpp` |
| **P1 (High)** | Object Lifecycle | Generate deterministic slugs for dynamically concept-spawned objects to prevent volatile ID warnings. | In Review | `src/ConstructedBeing/Object/Creation/` |
| **P1 (High)** | Core Concurrency | Add double-buffered thread-safe event queue to `EventBus`. | In Review | `src/Singularity/Core/EventBus.hpp` |
| **P2 (Medium)** | Material System | Enforce `const Material*` return in `MaterialManager` to mandate `ownMaterial()` divergence. | In Review | `src/ConstructedBeing/Material/` |

---
*Report compiled and archived to `docs/audits/2026-08-16_codebase_audit_report.md`.*
