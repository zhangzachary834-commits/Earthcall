## 2026-09-23 - Add visual and audio feedback to Emit Utterance button
**Learning:** Silently failing an action due to a disconnected state causes user frustration and confusion. Even when disabled, interactive elements should provide clear feedback about *why* they cannot be used, especially for keyboard and screen reader users. In this app, users could press 'Enter' or click Emit while disconnected, and nothing would visibly or audibly happen.
**Action:** Always provide a clear, immediate error indication (like a shake animation combined with a screen reader announcement) when a primary action is blocked by application state. Additionally, always provide success feedback (like a flash and announcement) when an action completes successfully without immediate visual UI changes in the DOM.

## 2026-09-24 - Add clear button to utterance input
**Learning:** In flex layouts with dynamic inputs, toggling element visibility using `display: none` / `block` can cause visually jarring layout shifts (e.g., adjacent buttons jump sideways).
**Action:** Use `visibility: hidden` and `visibility: visible` to toggle elements inside flex layouts when you want to reserve their space, preventing sibling elements from shifting when the element appears or disappears.
