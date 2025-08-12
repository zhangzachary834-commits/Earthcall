#pragma once

#include "EventBus.hpp"
#include <memory>
#include <unordered_map>
#include <functional>
#include <string>

namespace Core {

// Centralized Event Handler that manages all event subscriptions
// This provides a cleaner interface for handling events across your application
class EventHandler {
public:
    // ------------------------------------------------------------------
    // Singleton access
    // ------------------------------------------------------------------
    static EventHandler& instance();

    // ------------------------------------------------------------------
    // Event registration helpers
    // ------------------------------------------------------------------
    
    // Register a handler for a specific event type
    template<typename Event>
    void registerHandler(const std::string& handlerName, 
                        const std::function<void(const Event&)>& handler,
                        int priority = 0) {
        _handlers[handlerName] = [this, handler, priority]() {
            EventBus::instance().subscribe<Event>(handler, priority);
        };
        _handlers[handlerName](); // Register immediately
    }

    // Register multiple handlers at once
    template<typename Event>
    void registerHandlers(const std::vector<std::pair<std::string, std::function<void(const Event&)>>>& handlers) {
        for (const auto& [name, handler] : handlers) {
            registerHandler<Event>(name, handler);
        }
    }

    // ------------------------------------------------------------------
    // Event publishing helpers
    // ------------------------------------------------------------------
    
    // Publish an event
    template<typename Event>
    void publish(const Event& event) {
        EventBus::instance().publish(event);
    }

    // Publish an event asynchronously
    template<typename Event>
    void publishAsync(const Event& event) {
        EventBus::instance().publishAsync(event);
    }

    // ------------------------------------------------------------------
    // Handler management
    // ------------------------------------------------------------------
    
    // Remove a specific handler
    void removeHandler(const std::string& handlerName);
    
    // Clear all handlers
    void clearAllHandlers();
    
    // Get list of registered handlers
    std::vector<std::string> getRegisteredHandlers() const;

    // ------------------------------------------------------------------
    // Utility methods
    // ------------------------------------------------------------------
    
    // Check if a handler is registered
    bool hasHandler(const std::string& handlerName) const;
    
    // Get handler count
    size_t getHandlerCount() const { return _handlers.size(); }

private:
    std::unordered_map<std::string, std::function<void()>> _handlers;
    
    EventHandler() = default;
    ~EventHandler() = default;
    EventHandler(const EventHandler&) = delete;
    EventHandler& operator=(const EventHandler&) = delete;
};

} // namespace Core 