## 2023-10-27 - Dynamic Status Container Accessibility
**Learning:** For containers where text changes dynamically without user interaction (like `#connection-status` displaying "Connecting...", "Connected", "Disconnected"), screen readers will not announce the new text by default. This makes critical state invisible to visually impaired users.
**Action:** Always add `role="status"` and `aria-live="polite"` to dynamically updated status containers that provide non-critical but useful context, so screen readers will announce changes naturally during pauses in speech.
## 2024-11-20 - [Added WASM Start Overlay Keyboard Accessibility]
**Learning:** Adding `tabindex="0"`, `role="button"`, and `aria-label` to custom `div` overlays is crucial for keyboard navigation. However, the `keydown` event listener for `Enter` and `Space` needs `event.preventDefault()` specifically for the spacebar to prevent page scrolling, ensuring a smooth activation experience.
**Action:** Always include `event.preventDefault()` on spacebar `keydown` handlers for custom interactive elements.
## 2024-11-21 - [Focus dropping on disabled buttons]
**Learning:** If a user clicks or presses a button that modifies state in a way that causes the button to become disabled (such as submitting an utterance that clears the input), focus is dropped to the document `<body>`. This completely disrupts the keyboard navigation flow.
**Action:** When disabling the currently focused element due to a state change, explicitly shift focus to the next logical element (e.g., the input field) using `.focus()`.
## 2024-03-24 - Overlay Dismissal Focus Drop
**Learning:** When dismissing full-screen overlays (like `#start-overlay` in `web_ui/wasm.html`), if the focused element is hidden (e.g., `display: none`), keyboard focus silently drops back to the document `<body>`. This breaks keyboard navigation flow for screen readers and power users.
**Action:** Always add `tabindex="-1"` to the primary application container or canvas that replaces the overlay, and explicitly call `.focus()` on it immediately after hiding the overlay to preserve logical navigation state.
## 2026-09-09 - Semantic Form Transformation
**Learning:** Wrapping single-input interfaces in semantic forms is crucial for both mobile keyboard behavior (providing a native 'Go'/'Submit' button) and standardizing screen reader submission.
**Action:** Always prefer forms over loose inputs with enter-key listeners.
