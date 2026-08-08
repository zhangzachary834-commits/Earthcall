#pragma once

#ifndef __EMSCRIPTEN__

#include <memory>
#include <string>
#include <thread>

namespace Singularity {
namespace Network {

// A lightweight WebSocket server for Earthcall.
// Listens on a specified port (default 8080) and ingests events like Utterance
// into the core EventBus.
class WebSocketServer {
public:
    static WebSocketServer& instance();

    // Starts the WebSocket server in a background thread.
    void start(uint16_t port = 8080);

    // Stops the background thread and closes all connections.
    void stop();

    // Sends a JSON string payload to all connected clients.
    void broadcast(const std::string& jsonPayload);

private:
    WebSocketServer();
    ~WebSocketServer();
    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    struct Impl;
    std::unique_ptr<Impl> _impl;
};

} // namespace Network
} // namespace Singularity

#endif // __EMSCRIPTEN__
