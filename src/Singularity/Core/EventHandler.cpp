#include "EventHandler.hpp"
#include <algorithm>

namespace Core {

EventHandler& EventHandler::instance() {
    static EventHandler handler;
    return handler;
}

void EventHandler::removeHandler(const std::string& handlerName) {
    auto it = _subscriptions.find(handlerName);
    if (it != _subscriptions.end()) {
        EventBus::instance().unsubscribe(it->second);
        _subscriptions.erase(it);
    }
}

void EventHandler::clearAllHandlers() {
    for (const auto& [name, token] : _subscriptions) {
        EventBus::instance().unsubscribe(token);
    }
    _subscriptions.clear();
}

std::vector<std::string> EventHandler::getRegisteredHandlers() const {
    std::vector<std::string> handlerNames;
    handlerNames.reserve(_subscriptions.size());
    
    for (const auto& [name, _] : _subscriptions) {
        handlerNames.push_back(name);
    }
    
    return handlerNames;
}

bool EventHandler::hasHandler(const std::string& handlerName) const {
    return _subscriptions.find(handlerName) != _subscriptions.end();
}

} // namespace Core 