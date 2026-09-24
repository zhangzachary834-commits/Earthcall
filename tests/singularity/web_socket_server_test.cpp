#include "Singularity/Network/WebSocketServer.hpp"
#include <cassert>
#include <iostream>

int main() {
#ifndef __EMSCRIPTEN__
    auto& server = Singularity::Network::WebSocketServer::instance();

    assert(!server.isRunning());

    server.start(18080);
    assert(server.isRunning());

    server.stop();
    assert(!server.isRunning());

    server.stop();

    server.start(18080);
    assert(server.isRunning());
    server.stop();

    std::cout << "[Test] WebSocketServer lifecycle operations succeed." << std::endl;
#else
    std::cout << "[Test] WebSocketServer skipped on Emscripten." << std::endl;
#endif

    std::cout << "web_socket_server_test: OK\n";
    return 0;
}
