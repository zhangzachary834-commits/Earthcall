#include "EventHandler.hpp"
#include <algorithm>

namespace Core {

EventHandler& EventHandler::instance() {
    static EventHandler handler;
    return handler;
}

EventHandler::~EventHandler() {
    clearAllHandlers();
}

void EventHandler::removeHandler(const std::string& handlerName) {
    auto it = _handlers.find(handlerName);
    if (it != _handlers.end()) {
        EventBus::instance().unsubscribe(it->second);
        _handlers.erase(it);
    }
}

void EventHandler::clearAllHandlers() {
    for (const auto& [name, id] : _handlers) {
        EventBus::instance().unsubscribe(id);
    }
    _handlers.clear();
}

std::vector<std::string> EventHandler::getRegisteredHandlers() const {
    std::vector<std::string> handlerNames;
    handlerNames.reserve(_handlers.size());
    
    for (const auto& [name, _] : _handlers) {
        handlerNames.push_back(name);
    }
    
    return handlerNames;
}

bool EventHandler::hasHandler(const std::string& handlerName) const {
    return _handlers.find(handlerName) != _handlers.end();
}

} // namespace Core 