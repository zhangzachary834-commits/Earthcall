#include "WebSocketClient.hpp"

#ifndef EMSCRIPTEN

#include <iostream>

namespace Singularity {
namespace Network {

// Desktop Mock Implementation
// On desktop, we avoid compiling heavy networking libraries for now since the 
// primary multiplayer target is WASM. This acts as a stub so the engine compiles.

struct WebSocketClient::Impl {
    std::function<void(const std::string&)> messageCallback;
};

WebSocketClient::WebSocketClient() : _impl(std::make_unique<Impl>()) {}
WebSocketClient::~WebSocketClient() = default;

WebSocketClient& WebSocketClient::instance() {
    static WebSocketClient inst;
    return inst;
}

void WebSocketClient::connect(const std::string& url) {
    std::cout << "[WebSocketClient] (Desktop Mock) Connecting to " << url << std::endl;
}

void WebSocketClient::disconnect() {
    std::cout << "[WebSocketClient] (Desktop Mock) Disconnecting." << std::endl;
}

void WebSocketClient::send(const std::string& payload) {
    std::cout << "[WebSocketClient] (Desktop Mock) Sent: " << payload << std::endl;
}

void WebSocketClient::onMessage(std::function<void(const std::string&)> callback) {
    _impl->messageCallback = std::move(callback);
}

} // namespace Network
} // namespace Singularity

#endif // EMSCRIPTEN
