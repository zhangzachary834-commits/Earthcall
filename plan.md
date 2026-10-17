1.  **Analyze the Bug:** The issue lies in `Core::EventHandler`, which wraps `Core::EventBus` for convenience. `EventHandler::registerHandler` calls `EventBus::subscribe`, which returns an `EventBus::SubscriptionToken`. However, `EventHandler` *never saves this token*. It only saves a lambda `std::function<void()>` in `_handlers[handlerName]` that *re-subscribes* when executed.
2.  **Verify the Consequence:** Because `EventHandler` doesn't store the `SubscriptionToken`, calling `EventHandler::removeHandler` simply erases the re-subscription lambda from the `_handlers` map, but it *does not* call `EventBus::unsubscribe(token)`. This leaves a dangling listener in `EventBus` that points to the captured handler. Since `EventHandler` is a global singleton, this isn't a dangling pointer to `EventHandler` itself, but the *callback* (often a lambda capturing local state) is kept alive forever in `EventBus`. `clearAllHandlers` has the exact same bug.
3.  **Fix `Core::EventHandler`:**
    *   Change the `_handlers` map type to store the `SubscriptionToken`. Wait, `SubscriptionToken` is returned by `EventBus::subscribe`.
    *   Actually, `registerHandler` stores `[this, handler, priority]() { EventBus::instance().subscribe<Event>(handler, priority); }` in `_handlers[handlerName]`.
    *   And it immediately calls `_handlers[handlerName]()`. So it ignores the token, and calling the lambda again would subscribe *again*. This design is broken.
    *   We need to refactor `EventHandler` to store the `SubscriptionToken` returned by `EventBus::subscribe`.
    *   Since `SubscriptionToken` is not tied to the Event type, we can store it directly: `std::unordered_map<std::string, EventBus::SubscriptionToken> _tokens;`.
    *   Or keep `_handlers` for something else? Wait, `registerHandler` seems intended to store the registration *logic*? No, it just registers immediately: `_handlers[handlerName](); // Register immediately`.
    *   Wait, maybe it stored the lambda so it could re-register later? No, there is no code that loops over `_handlers` to call them later. It's just a broken wrapper.
4.  **Refactor `Core::EventHandler`:**
    *   `std::unordered_map<std::string, EventBus::SubscriptionToken> _subscriptions;`
    *   In `registerHandler`:
        ```cpp
        template<typename Event>
        void registerHandler(const std::string& handlerName,
                            const std::function<void(const Event&)>& handler,
                            int priority = 0) {
            _subscriptions[handlerName] = EventBus::instance().subscribe<Event>(handler, priority);
        }
        ```
    *   In `removeHandler`:
        ```cpp
        void EventHandler::removeHandler(const std::string& handlerName) {
            auto it = _subscriptions.find(handlerName);
            if (it != _subscriptions.end()) {
                EventBus::instance().unsubscribe(it->second);
                _subscriptions.erase(it);
            }
        }
        ```
    *   In `clearAllHandlers`:
        ```cpp
        void EventHandler::clearAllHandlers() {
            for (const auto& [name, token] : _subscriptions) {
                EventBus::instance().unsubscribe(token);
            }
            _subscriptions.clear();
        }
        ```
5.  **Test:** Add a unit test to verify that `removeHandler` and `clearAllHandlers` actually reduce the listener count in `EventBus`, and confirm the fix.
