# Keyboard Handler Architecture & Refactoring

This document explains the centralized keyboard handling architecture in `src/Singularity/Input/Keyboard/KeyboardHandler.{hpp,cpp}` and its integration with `Core::Engine`.

## Architecture Overview

Keyboard functionality is isolated into the `Singularity/Input` modality rather than scattered across the core simulation loop.

### Core Responsibilities
- Centralized keyboard state tracking in `KeyboardHandler` (`src/Singularity/Input/Keyboard/KeyboardHandler.hpp`).
- Clean separation of concerns with callback-based key bindings.
- Owned and initialized directly by `Core::Engine` (`_keyboardHandler`), accessible via `Engine::getKeyboardHandler()`.

## Key Bindings Setup

`KeyboardHandler` categorizes key bindings into modular setup routines:
- `setupMenuBindings()` — Menu and UI toggles
- `setupCameraBindings()` — Camera movement and modifier controls
- `setupToolBindings()` — Creation tool and perspective switching
- `setupUtilityBindings()` — Utility shortcuts and developer tools
- `updateGameInput(GLFWwindow* window)` — Per-frame key state transition polling

## How to Use the System

### 1. Adding Key Bindings
```cpp
auto* keyboard = engine.getKeyboardHandler();
if (keyboard) {
    keyboard->bindKey(GLFW_KEY_N, "new_feature", []() {
        // Feature action callback
    });
}
```

### 2. Checking Key States
```cpp
if (keyboard->getMPressedLast()) {
    // Handle M key transition
}
```

### 3. Managing Handlers Dynamically
```cpp
keyboard->bindKey(GLFW_KEY_X, "action", callback);
keyboard->removeHandler("action");
keyboard->clearAllHandlers();
```

## Preserved Functionality
- Menu toggle (`M` key)
- Cursor lock/unlock (`Escape` key)
- Chat window toggle (`H` key)
- Integration UI toggle (`I` key)
- Toolbar toggle (`T` key)
- Perspective switching (`1`/`2`/`3` keys)
- Flight toggle (`F` key)
- Character zone switch (`C` key)
- Law trigger (`L` key)
- Undo/Redo (`Ctrl+Z` / `Ctrl+Y`)
- Camera movement (`WASD` + `Shift`/`Space`)
