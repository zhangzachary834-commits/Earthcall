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
## 2024-11-25 - [Prevent Silent Data Loss on Disconnect]
**Learning:** Leaving form inputs enabled when a backend connection is broken creates silent failure scenarios where users type and submit data that is lost. Disabling inputs during disconnected states prevents this, but you must manually trigger events (`dispatchEvent(new Event('input'))`) to update dependent button states and manually restore focus (`.focus()`) upon reconnection to preserve navigation flow.
**Action:** Always disable inputs during disconnected states, update placeholders dynamically, trigger update events, and focus the input once the connection returns.
## 2024-11-26 - [Dynamic Dependent Button State Under Connection Loss]
**Learning:** Even if you dispatch an `input` event when a form input gets disabled due to a system disconnect, the dependent buttons (like a Submit button) won't automatically disable unless the input event handler specifically checks the `disabled` state of the input field. This oversight can leave a Submit button visually enabled while the system is disconnected, leading to silent data loss if clicked.
**Action:** Always verify both the content (e.g., `value.trim() === ''`) AND the accessibility state (`disabled`) of the input when computing the dependent state of submission buttons in `input` handlers.
## 2024-11-26 - [Three-State Network UI Model vs Boolean]
**Learning:** Using a single boolean flag (`isConnected`) to drive network UI states often forces intermediate states (like "Connecting...") to incorrectly inherit the visual styling of the "disconnected" state (e.g., a red indicator). This confuses users who expect a distinct visual signal for in-progress operations.
**Action:** When designing connection status indicators, always use an explicit three-state model (e.g., `connecting`, `connected`, `disconnected`) or infer the intermediate state explicitly from parameters, rather than falling back to binary boolean logic.
## 2024-11-27 - Canvas-to-Input Keyboard Focus Flow
**Learning:** In hybrid interfaces where a primary interaction layer is a 3D canvas but there is also a persistent command input (like `#utterance-input`), screen reader users and keyboard power users need a fast, predictable way to switch contexts. Pressing `Enter` on the body/canvas to instantly focus the input, and pressing `Escape` to clear and `.blur()` the input (returning focus to the body), creates a seamless, bidirectional keyboard workflow without requiring manual tabbing through UI layers.
**Action:** Always provide explicit global hotkeys (like `Enter` on body/canvas) to jump into critical inputs, and `Escape` to gracefully exit focus back to the primary spatial context.
## 2024-11-20 - Accessible Disabled Buttons and Tooltips
**Learning:** Using the native `disabled` attribute on HTML `<button>` elements removes them from the tab order, completely hiding them from keyboard navigation and screen readers. When a button is disabled, users often need to know *why* it is disabled. If we use the native `disabled` attribute, any tooltips (like `title` attributes) explaining the state will be inaccessible to keyboard users, leading to a frustrating experience.
**Action:** Instead of `disabled`, use `aria-disabled="true"` to communicate the state to screen readers while keeping the button focusable. Then, use CSS (`[aria-disabled="true"]`) to visually style it as disabled, and handle the logic in JavaScript to prevent the action from firing when `aria-disabled="true"`. This ensures tooltips explaining *why* the button is disabled remain accessible to all users.
## 2024-11-27 - Accessible Keyboard Shortcut Hints
**Learning:** When adding visual keyboard shortcut hints (like `<kbd>Enter</kbd>` or `↵`) inside interactive elements like buttons, screen readers will often append or interject the literal text of the `<kbd>` element into the button's accessible name (e.g., reading "Emit Return button" instead of "Emit button"). This creates a confusing auditory experience for users relying on assistive technologies.
**Action:** Always include `aria-hidden="true"` on visual `<kbd>` hints embedded within buttons or links to hide them from screen readers, since the button's primary action is already understood semantically.
## 2024-11-28 - Composite Input Focus Styling
**Learning:** When styling a composite form input (a container with an input and a button, like `#logos-interface`), applying `:focus-visible` solely to the inner `<input>` element creates a visually disjointed highlight. Using `:focus-within` on the parent container provides a much more cohesive, aesthetically pleasing, and accessible focus indicator for keyboard users.
**Action:** Always prefer `:focus-within` on the container for composite input designs rather than outlining just the inner text field.
## 2024-11-28 - [Accessible Button Contrast Overlays]
**Learning:** When layering semi-transparent backgrounds (like `.kbd-hint` over a primary `#emit-btn`), alpha-blended white (`rgba(255,255,255,0.15)`) on top of a light blue button background (`#4a90e2`) causes both the text and the hint background to fail WCAG AA contrast ratios (falling below 4.5:1).
**Action:** Always verify contrast ratios for primary action buttons. Deepen the base button color (`#2563eb`) to provide sufficient contrast for white text, and use black with alpha (`rgba(0,0,0,0.2)`) instead of white for nested hint backgrounds to maintain legibility without blowing out the lightness.
## 2025-01-20 - [Screen Reader Feedback on Form Submission]
**Learning:** When a form submission simply clears the input field without navigating to a new page or displaying a persistent visual success message (like in a chat or command interface), screen reader users receive no auditory confirmation that their action succeeded. The typed text silently disappears, which can leave the user wondering if the submission worked or failed.
**Action:** Always provide an `aria-live="polite"` visually hidden region (e.g., using a `.sr-only` class) and update its text content upon successful submission to explicitly announce the outcome (e.g., "Emitted: [word]") to assistive technologies.
## 2026-09-20 - [HUD Text Contrast over Dynamic Canvas]
**Learning:** Text placed directly over a 3D canvas (like a WebGL or WebGPU viewport) can become entirely illegible if the scene renders light colors behind it. Relying only on text color is insufficient for accessibility when the background is dynamic.
**Action:** Always wrap floating HUD text elements (like connection statuses) in a semi-transparent dark container (e.g., `background: rgba(0,0,0,0.6)`) to ensure WCAG-compliant contrast regardless of the underlying rendered scene.

## 2026-09-20 - [Reduced Motion for Infinite Status Animations]
**Learning:** Infinite CSS animations (like a pulsing status dot) can trigger vestibular disorders and violate WCAG 2.2.2 (Pause, Stop, Hide).
**Action:** Always wrap infinite status animations in a `@media (prefers-reduced-motion: reduce)` query that sets `animation: none`.

## 2024-11-29 - [Aria-Live Consecutive Announcements]
**Learning:** When using an `aria-live="polite"` region for dynamic announcements (like form submissions), if the new text is identical to the previous text, screen readers will not announce it again. This creates a silent failure when a user repeats an action.
**Action:** Always briefly clear the text content of the live region (e.g., using `setTimeout` for a few milliseconds) before setting the new text to guarantee reliable announcements of consecutive identical strings.
