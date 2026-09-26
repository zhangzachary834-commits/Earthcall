#include "Singularity/Network/WebSocketServer.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <cassert>
#include <iostream>
#include <string>

extern ZoneManager mgr;

#ifndef __EMSCRIPTEN__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <thread>
#include <chrono>

static bool verifyWebSocketHandshakeAndReceiveSnapshot(uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(sock);
        return false;
    }

    std::string request =
        "GET / HTTP/1.1\r\n"
        "Host: 127.0.0.1:" + std::to_string(port) + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n";

    if (send(sock, request.c_str(), request.size(), 0) < 0) {
        close(sock);
        return false;
    }

    char buffer[4096];
    struct timeval tv{};
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    close(sock);

    if (bytes <= 0) return false;
    buffer[bytes] = '\0';
    std::string response(buffer, bytes);
    return response.find("101 Switching Protocols") != std::string::npos;
}
#endif

int main() {
#ifndef __EMSCRIPTEN__
    // WITNESS DOCUMENTATION:
    // What the old test could prove:
    //   That start() and stop() toggled the boolean flag _impl->running on WebSocketServer.
    // What the old test could NOT prove:
    //   That the server thread was actually executing ASIO event processing after a restart.
    //   In fact, because _impl->server.reset() was missing on WebSocketServer::start(), calling
    //   stop() left ASIO in a stopped state. A second start() call resulted in _impl->server.run()
    //   immediately returning and exiting the worker thread, rendering the server completely dead
    //   to real network clients while server.isRunning() falsely returned true.

    auto& server = Singularity::Network::WebSocketServer::instance();

    assert(!server.isRunning());

    // First start
    server.start(18080);
    assert(server.isRunning());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    bool firstHandshake = verifyWebSocketHandshakeAndReceiveSnapshot(18080);
    std::cout << "[Test] First handshake success: " << (firstHandshake ? "YES" : "NO") << std::endl;
    assert(firstHandshake && "WebSocketServer must accept handshakes on first start");

    server.stop();
    assert(!server.isRunning());

    // Second start (restart)
    server.start(18080);
    assert(server.isRunning());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    bool secondHandshake = verifyWebSocketHandshakeAndReceiveSnapshot(18080);
    std::cout << "[Test] Second handshake success: " << (secondHandshake ? "YES" : "NO") << std::endl;
    assert(secondHandshake && "WebSocketServer must accept handshakes on restart after stop()");

    server.stop();

    std::cout << "[Test] WebSocketServer real network path verification succeeded." << std::endl;

    // Test unhydrated ZoneManager safety: clear zones to simulate startup before zone hydration
    std::cout << "[Test] Verifying WebSocketServer behavior when ZoneManager has no active zones..." << std::endl;
    mgr.zones().clear();
    server.broadcastStateSync();
    std::cout << "[Test] Unhydrated ZoneManager state_sync broadcast succeeded without crash." << std::endl;
#else
    std::cout << "[Test] WebSocketServer skipped on Emscripten." << std::endl;
#endif

    std::cout << "web_socket_server_test: OK\n";
    return 0;
}
