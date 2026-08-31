#pragma once

#ifndef __EMSCRIPTEN__

#include <memory>
#include <string>
#include <thread>
#include <cstdint>

namespace Singularity {
namespace Network {

// A high-performance WebSocket server for Earthcall.
// Listens on a specified port (default 8080) and facilitates bidirectional
// real-time synchronization between the C++ engine and external tools (Python backend, Web UI, Robotics).
class WebSocketServer {
public:
    static WebSocketServer& instance();

    // Starts the WebSocket server in a background thread.
    void start(uint16_t port = 8080);

    // Stops the background thread and closes all connections.
    void stop();

    // Sends a JSON string payload to all connected clients.
    void broadcast(const std::string& jsonPayload);

    // Broadcasts the current live world state snapshot to all connected clients.
    void broadcastStateSync();

    // Checks if the server is running
    bool isRunning() const;

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
