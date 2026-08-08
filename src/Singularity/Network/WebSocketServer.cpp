#ifndef __EMSCRIPTEN__

#include "WebSocketServer.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "json.hpp"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <algorithm>
#include <iostream>
#include <mutex>
#include <vector>

namespace Singularity {
namespace Network {

using ServerType = websocketpp::server<websocketpp::config::asio>;

struct WebSocketServer::Impl {
    ServerType server;
    std::thread worker;
    bool running = false;
    
    // Connection handles
    std::vector<websocketpp::connection_hdl> connections;
    std::mutex connections_mutex;

    // Exact-match allowlist. A substring test is not usable here: an attacker
    // can register a host that merely *contains* "localhost"
    // (https://localhost.example.com), and since a WebSocket handshake is not
    // subject to the same-origin policy, any page the user visits could then
    // drive the engine. Serve web_ui over one of these origins rather than
    // opening it from file:// -- a file:// page sends "Origin: null", which is
    // also what sandboxed iframes send, so "null" is deliberately not listed.
    static bool isAllowedOrigin(const std::string& origin) {
        static const std::vector<std::string> allowed = {
            "http://localhost:3000",  "http://127.0.0.1:3000",
            "http://localhost:8080",  "http://127.0.0.1:8080",
            "https://localhost:3000", "https://127.0.0.1:3000",
        };
        return std::find(allowed.begin(), allowed.end(), origin) != allowed.end();
    }

    bool on_validate(websocketpp::connection_hdl hdl) {
        auto con = server.get_con_from_hdl(hdl);
        std::string origin = con->get_request_header("Origin");

        // No Origin header means no browser sent this. Browsers always attach
        // one to a WS handshake, so absence cannot be a hostile web page --
        // it is a local native client, already scoped by the loopback bind.
        if (origin.empty()) return true;

        if (!isAllowedOrigin(origin)) {
            std::cout << "[WebSocketServer] Rejected connection from origin: " << origin << std::endl;
            return false;
        }
        return true;
    }

    void on_message(websocketpp::connection_hdl hdl, ServerType::message_ptr msg) {
        std::string payload = msg->get_payload();
        try {
            auto j = nlohmann::json::parse(payload);
            if (!j.is_object()) return;

            if (j.value("type", "") == "utterance") {
                // Types are checked rather than coerced: value<std::string>
                // hands back "" for a nested object, which would admit
                // {"payload": {...}} as a silently empty utterance instead of
                // rejecting it (cybersecurity policy 1, "Type Checking").
                auto it = j.find("payload");
                if (it == j.end() || !it->is_string()) return;

                Core::Event::Utterance evt;
                evt.payload = it->get<std::string>();

                // Very basic pointer-to-string cast to get a unique client ID
                evt.sourceClient = std::to_string(reinterpret_cast<uintptr_t>(hdl.lock().get()));
                
                // Publish to the main engine bus
                Core::EventBus::instance().publish(evt);
                std::cout << "[WebSocketServer] Received utterance: " << evt.payload << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[WebSocketServer] Failed to parse message: " << e.what() << std::endl;
        }
    }

    void on_open(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(connections_mutex);
        connections.push_back(hdl);
        std::cout << "[WebSocketServer] Client connected." << std::endl;
    }

    void on_close(websocketpp::connection_hdl hdl) {
        std::lock_guard<std::mutex> lock(connections_mutex);
        auto it = std::find_if(connections.begin(), connections.end(),
            [&hdl](const websocketpp::connection_hdl& c) {
                return !c.owner_before(hdl) && !hdl.owner_before(c);
            });
        if (it != connections.end()) {
            connections.erase(it);
        }
        std::cout << "[WebSocketServer] Client disconnected." << std::endl;
    }
};

WebSocketServer& WebSocketServer::instance() {
    static WebSocketServer instance;
    return instance;
}

WebSocketServer::WebSocketServer() : _impl(std::make_unique<Impl>()) {
    _impl->server.clear_access_channels(websocketpp::log::alevel::all);
    _impl->server.set_access_channels(websocketpp::log::alevel::access_core);
    
    _impl->server.init_asio();
    _impl->server.set_max_message_size(1024 * 1024); // 1 MB limit
    
    _impl->server.set_validate_handler(std::bind(&Impl::on_validate, _impl.get(), std::placeholders::_1));
    _impl->server.set_message_handler(std::bind(&Impl::on_message, _impl.get(), std::placeholders::_1, std::placeholders::_2));
    _impl->server.set_open_handler(std::bind(&Impl::on_open, _impl.get(), std::placeholders::_1));
    _impl->server.set_close_handler(std::bind(&Impl::on_close, _impl.get(), std::placeholders::_1));
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start(uint16_t port) {
    if (_impl->running) return;
    _impl->running = true;
    
    _impl->server.listen("127.0.0.1", std::to_string(port));
    _impl->server.start_accept();
    
    _impl->worker = std::thread([this]() {
        websocketpp::lib::error_code ec;
        std::cout << "[WebSocketServer] Listening on ws://localhost:" << _impl->server.get_local_endpoint(ec).port() << std::endl;
        _impl->server.run();
    });
}

void WebSocketServer::stop() {
    if (!_impl->running) return;
    _impl->running = false;
    
    _impl->server.stop_listening();
    {
        std::lock_guard<std::mutex> lock(_impl->connections_mutex);
        for (auto& hdl : _impl->connections) {
            websocketpp::lib::error_code ec;
            _impl->server.close(hdl, websocketpp::close::status::normal, "Server shutting down", ec);
        }
    }
    _impl->server.stop();
    
    if (_impl->worker.joinable()) {
        _impl->worker.join();
    }
}

void WebSocketServer::broadcast(const std::string& jsonPayload) {
    if (!_impl->running) return;
    std::lock_guard<std::mutex> lock(_impl->connections_mutex);
    for (auto& hdl : _impl->connections) {
        websocketpp::lib::error_code ec;
        _impl->server.send(hdl, jsonPayload, websocketpp::frame::opcode::text, ec);
    }
}

} // namespace Network
} // namespace Singularity

#endif // __EMSCRIPTEN__
