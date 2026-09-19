# 🔒 Earthcall Security Features and Threat Model

This document describes the security posture, threat model, and implemented safeguards in Earthcall.

*Note: This is a truth-correction document defining the actual system behavior rather than aspirational boundaries. Earthcall's authorization is an in-world ontological property, while isolation remains the responsibility of the host environment.*

## 1. Threat Model & Trust Boundary

### Protected Assets
1. **The Substrate (Saves & Memory):** The literal state of the world, represented as JSON, msgpack, and compressed binary.
2. **Execution Integrity:** The guarantee that the engine only fires laws as authored, and does not execute arbitrary native code injected from the substrate.
3. **Ontological Truth:** The accuracy of the First Mover Register and `TransferPolicy` reflecting who actually authored a change.

### Actors
1. **Persons:** Human operators running the engine. Zach's ontological requirement states that the chain of authority always terminates in a Person.
2. **Models/Agents:** AI operators driving the world via MCP, web sockets, or direct save-file injection. They act via delegated authority from a Person.
3. **Web Contexts:** External JavaScript/HTML running in the UI or web integration tier.

### Trust Assumptions
1. **The Host OS is Trusted:** We assume the operating system, C++ runtime, GPU driver, and file system are non-malicious and functioning correctly.
2. **Persons are Trusted:** The human running the software has full access to the machine. Earthcall does not defend the OS from the Person.
3. **Models are Fallible but not Adversarial to the Host:** Models may write incorrect laws or exceed their scopes, but they are not executing zero-day exploits against the Python interpreter or OS.

### Entry Points
1. **Save Ingestion:** Parsing `.ecsave`, `.ecmatter`, and `.ecform` files from disk.
2. **Network/MCP/WebSocket:** Commands and payloads arriving from agents or web UIs.
3. **Foreign Channel:** IPC with the Python environment.

### Attacker Capabilities
An attacker (e.g., a malicious save file or rogue web context) can:
- Inject arbitrary strings, integers, and nested JSON.
- Forge `authors` fields or identifiers inside the save.
- Spam the WebSocket or MCP endpoints with malformed requests.
- Attempt to exploit parser bugs in `nlohmann::json` or `FlatBuffers`.

### Non-Goals (What Earthcall Does NOT Provide)
- **Capability Security:** We do not implement a robust object-capability model. `TransferPolicy` is an access-control list keyed by path name, not a capability system.
- **Host Sandboxing:** We do not sandbox the C++ or Python process from the OS. A remote code execution bug in the engine is an RCE on the host.
- **Enforced Cryptographic Authentication at Runtime:** The engine does not cryptographically authenticate every runtime action; it verifies signatures on load for the First Mover Register, but runtime property guards are advisory.

### Failure Modes
- **Ontological Forgery:** A model injects a save where it claims a Person authored a law. The engine loads this if not caught by the First Mover Register's quarantine.
- **Denial of Service (DoS):** Malformed geometry or infinite loops in laws crashing the engine or causing OOM.
- **TCB Compromise:** A vulnerability in a dependency (e.g., OpenSSL or Playwright) leading to host access.



### D. Verifying the Posture (Concrete Tests)
To prove these statements, refer to the following concrete tests in the C++ suite:
- **`tests/singularity/foreign_integration_test.cpp`**: Validates the global `TransferPolicy` ACL and access blocking at the integration tier.
- **`tests/identity/identity_test.cpp`**: Verifies cryptographic attestation, key pairs, and the `FirstMoverRegister`.
- **`tests/singularity/substrate_split_test.cpp`**: Shows how the system validates boundaries and authority constraints across the substrate.

---

## 2. The Actual Trusted Computing Base (TCB)

The TCB is the set of components that must function correctly for the security properties to hold. In Earthcall, this is substantial. It includes:

- **Core Rendering & OS Abstraction:** `wgpu-native`, Dawn/WASM, GLFW/window system, ImGui.
- **Math & Serialization:** `FlatBuffers`, `GLM`, `nlohmann::json`.
- **Networking & Media:** `httplib`, `miniaudio`, OpenSSL (including the vendored 3.0.13 and 1.1.1w trees).
- **Python Environment:** The entire vendored Python virtualenv carrying Flask, Werkzeug, requests, and Playwright.
- **Entry Points:** Network/MCP/WebSocket listeners and filesystem/save ingestion logic.
- **Host Environment:** The OS, C++ runtime, and GPU driver.

*None of these components know what a "Person" is. Every Person guard in the C++ tree is advisory, not enforced against the host OS.*

---

## 3. Mechanisms vs. Claims

It is crucial to distinguish between what the architecture aspires to and what the C++ code actually enforces.

### A. The First Mover Register (Cryptographic Claims)
- Located in `src/Identity/FirstMoverRegister.cpp` and `Claim.cpp`.
- Provides **cryptographic attestation** of who injected what into the substrate (save file).
- The chain of recognition must terminate in a **Person** (Zach's ontological requirement).
- Models operate on delegated authority from a Person.
- **Action:** If a grant signature fails, the injected entity is loaded but marked **quarantined** and inert.

### B. TransferPolicy (Global ACL)
- Located in `src/Singularity/TransferPolicy.cpp`.
- It is a **global path-based ACL**, not a capability architecture.
- It consults a map of property paths (e.g., `shape.r`) to determine if set-to-set creation is allowed.
- It is checked during explicit creation operations, but does not authenticate the caller (it does not know *who* is asking).

### C. Kernel Guards & Person Authority
- The highest authority level (`Tier::Kernel`) is immune to lower-order laws.
- The `Person` being (derived from `Singular`) sits at the top of the ontological hierarchy. Zach's explicit requirement is that all substantial changes (e.g., metalaw bounds, spawn creation) must trace back to a Person.
- However, these are **ontological constraints**, not OS-level memory protections.

---

## 4. Web Security Features

*(These features apply to the web integration tier and `SecurityManager`)*

- **URL Validation & Whitelisting:** Enforces HTTPS, blocks local file access, and restricts domains.
- **Permission Management:** Granular permissions (e.g., `BRUSH_SYSTEM`, `FILE_SYSTEM`).
- **Content Security Policy (CSP) & JavaScript Sanitization:** Protects against basic XSS in the web UI.
- **Rate Limiting & Threat Detection:** Temporarily blocks high-frequency request sources to prevent spam DoS.

---
*Signed: Jules (Model Unexposed)*
*Session ID: 3ff9f175f0544eb5ab251e63dc1eca0c*
*Date: 2026-09-10*
*Timestamp: 2026-09-10 06:58:18 UTC*
