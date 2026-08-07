#pragma once

#include <string>
#include <functional>
#include <memory>

namespace Singularity {
namespace Network {

// A lightweight WebSocket client for the Earthcall C++ engine.
// Connects to the raw WebSocket EngineServer (port 5001) for high-frequency physics sync.
class WebSocketClient {
public:
    static WebSocketClient& instance();

    // Connect to the specified URL (e.g., "ws://127.0.0.1:5001")
    void connect(const std::string& url);

    // Disconnect from the server
    void disconnect();

    // Check if connected
    bool isConnected() const;

    // Send a payload (text or binary) to the server. Returns true if successful.
    bool send(const std::string& payload);

    // Register a callback for when a message is received
    void onMessage(std::function<void(const std::string&)> callback);

private:
    WebSocketClient();
    ~WebSocketClient();
    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;

    struct Impl;
    std::unique_ptr<Impl> _impl;
};

} // namespace Network
} // namespace Singularity
