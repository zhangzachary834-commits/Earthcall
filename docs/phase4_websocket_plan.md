# Phase 4: Browser Interface & WebSocket Ingestion Plan

## Goal
Establish a robust, real-time bridge between the Earthcall C++ engine and a browser-based HTML/JS frontend. This shifts Earthcall away from its native ImGui chat UI into a proper web-driven interface, allowing users to emit `Lexeme`s (words) from a clean browser environment.

## Context & The "Medium Agnostic" Shift
As we established, a `Lexeme` is a structural entity that inherits the medium of the `Zone` it enters. To feed these entities into the engine, we need an entry point that is not hard-locked to ImGui. WebSockets provide a fast, bidirectional pipeline for this. 

## Architectural Decision: Dual-Path UI Ingestion

To maintain maximum generativity and flexibility, we will support both ingestion paths simultaneously. The engine will abstract the source of the `Utterance`, allowing us to defer the final architectural lockdown:

1. **Native Server Mode**: When compiled as a native executable (`wgpu-native`), Earthcall runs a WebSocket server. The browser connects via `ws://` over the network.
2. **WASM Browser Mode**: When compiled to WebAssembly via Emscripten, Earthcall runs *inside* the browser. The HTML/JS UI passes strings directly into C++ via `Embind` (zero network overhead).

Both paths will push a unified `Event::Utterance` to the `EventBus`.

## Proposed Changes

### 1. Unified EventBus Ingestion
- **[MODIFY] `src/Singularity/Core/EventBus.hpp`**:
  - Define `Event::Utterance` containing the string payload and the originating client ID.
  - The main game loop (`Engine::tick`) processes these events.

### 2. Path A: Native WebSocket Server (C++ Backend)
- **[NEW] Dependency**: Add `websocketpp` and `asio` (or a lightweight alternative) to `CMakeLists.txt` (only built if `EMSCRIPTEN` is false).
- **[NEW] `src/Singularity/Network/WebSocketServer.hpp / .cpp`**: 
  - Runs a background thread listening on `ws://localhost:8080`.
  - Parses incoming JSON (`{"type": "utterance", "payload": "Joy"}`) and pushes `Event::Utterance` to the `EventBus`.

### 3. Path B: Emscripten WASM Bindings
- **[NEW] `src/Singularity/Foreign/WebBindings.cpp`**:
  - Uses `#ifdef EMSCRIPTEN` and `<emscripten/bind.h>`.
  - Exposes a global C++ function `Earthcall_EmitUtterance(std::string text)` to JavaScript.
  - Pushes `Event::Utterance` to the `EventBus` directly.

### 4. Frontend Interface (HTML/JS Client)
- **[NEW] `web_ui/index.html` & `app.js`**:
  - A clean HTML5 interface for Earthcall.
  - Detects if Earthcall is running locally in WASM (calls `Module.Earthcall_EmitUtterance()`) or remotely (opens a WebSocket to `ws://localhost:8080`).

## Verification Plan

1. **Compilation**: Verify the new WebSocket library compiles alongside `wgpu-native`.
2. **Connection**: Open `web_ui/index.html` in Chrome and verify it successfully connects to the C++ Earthcall engine.
3. **Transmission**: Type "Joy" into the HTML interface, hit enter, and verify the C++ terminal logs `[Network] Received Utterance: Joy`.
