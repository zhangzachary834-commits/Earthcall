#include "WebSocketClient.hpp"

#ifdef EMSCRIPTEN

#include <emscripten/websocket.h>
#include <iostream>

namespace Singularity {
namespace Network {

struct WebSocketClient::Impl {
    EMSCRIPTEN_WEBSOCKET_T socket = 0;
    std::function<void(const std::string&)> messageCallback;

    static EM_BOOL onOpen(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, void *userData) {
        std::cout << "[WebSocketClient] Connected to EngineServer." << std::endl;
        return EM_TRUE;
    }

    static EM_BOOL onError(int eventType, const EmscriptenWebSocketErrorEvent *websocketEvent, void *userData) {
        std::cerr << "[WebSocketClient] Connection error." << std::endl;
        return EM_TRUE;
    }

    static EM_BOOL onClose(int eventType, const EmscriptenWebSocketCloseEvent *websocketEvent, void *userData) {
        std::cout << "[WebSocketClient] Connection closed." << std::endl;
        return EM_TRUE;
    }

    static EM_BOOL onMessage(int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, void *userData) {
        auto* impl = static_cast<Impl*>(userData);
        if (impl->messageCallback && !websocketEvent->isText) {
            // Convert binary payload to string (or handle as needed)
            std::string payload(reinterpret_cast<const char*>(websocketEvent->data), websocketEvent->numBytes);
            impl->messageCallback(payload);
        } else if (impl->messageCallback && websocketEvent->isText) {
            std::string payload(reinterpret_cast<const char*>(websocketEvent->data), websocketEvent->numBytes);
            impl->messageCallback(payload);
        }
        return EM_TRUE;
    }
};

WebSocketClient::WebSocketClient() : _impl(std::make_unique<Impl>()) {}
WebSocketClient::~WebSocketClient() { disconnect(); }

WebSocketClient& WebSocketClient::instance() {
    static WebSocketClient inst;
    return inst;
}

void WebSocketClient::connect(const std::string& url) {
    EmscriptenWebSocketCreateAttributes attr;
    emscripten_websocket_init_create_attributes(&attr);
    attr.url = url.c_str();

    _impl->socket = emscripten_websocket_new(&attr);
    if (_impl->socket <= 0) {
        std::cerr << "[WebSocketClient] Failed to create WebSocket." << std::endl;
        return;
    }

    emscripten_websocket_set_onopen_callback(_impl->socket, _impl.get(), Impl::onOpen);
    emscripten_websocket_set_onerror_callback(_impl->socket, _impl.get(), Impl::onError);
    emscripten_websocket_set_onclose_callback(_impl->socket, _impl.get(), Impl::onClose);
    emscripten_websocket_set_onmessage_callback(_impl->socket, _impl.get(), Impl::onMessage);
}

void WebSocketClient::disconnect() {
    if (_impl->socket > 0) {
        emscripten_websocket_close(_impl->socket, 1000, "Client disconnected");
        emscripten_websocket_delete(_impl->socket);
        _impl->socket = 0;
    }
}

void WebSocketClient::send(const std::string& payload) {
    if (_impl->socket > 0) {
        // Send as text for now
        emscripten_websocket_send_utf8_text(_impl->socket, payload.c_str());
    }
}

void WebSocketClient::onMessage(std::function<void(const std::string&)> callback) {
    _impl->messageCallback = std::move(callback);
}

} // namespace Network
} // namespace Singularity

#endif // EMSCRIPTEN
