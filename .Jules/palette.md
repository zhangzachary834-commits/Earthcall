## 2023-10-27 - Dynamic Status Container Accessibility
**Learning:** For containers where text changes dynamically without user interaction (like `#connection-status` displaying "Connecting...", "Connected", "Disconnected"), screen readers will not announce the new text by default. This makes critical state invisible to visually impaired users.
**Action:** Always add `role="status"` and `aria-live="polite"` to dynamically updated status containers that provide non-critical but useful context, so screen readers will announce changes naturally during pauses in speech.
## 2024-11-20 - [Added WASM Start Overlay Keyboard Accessibility]
**Learning:** Adding `tabindex="0"`, `role="button"`, and `aria-label` to custom `div` overlays is crucial for keyboard navigation. However, the `keydown` event listener for `Enter` and `Space` needs `event.preventDefault()` specifically for the spacebar to prevent page scrolling, ensuring a smooth activation experience.
**Action:** Always include `event.preventDefault()` on spacebar `keydown` handlers for custom interactive elements.
