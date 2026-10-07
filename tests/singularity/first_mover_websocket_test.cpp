// End to end over the REAL WebSocket path the MCP bridge uses: a foreign
// client must prove it holds a Person-granted First Mover's key before it may
// change anything, and every act is then held to that mover's scope.
//
// A green unit test of the guard is not a witness that the socket consults
// it; this test is. Plan section 15.5 of
// docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md.
// Written 2026-09-24 (Claude Opus 5.5).
#ifndef __EMSCRIPTEN__
#include "Singularity/Network/WebSocketServer.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Identity/FirstMoverRegister.hpp"
#include "Identity/KeyPair.hpp"
#include "json.hpp"

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <cassert>
#include <chrono>
#include <deque>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

using Client = websocketpp::client<websocketpp::config::asio_client>;
extern ZoneManager mgr;
using namespace Identity;

namespace {

constexpr uint16_t kPort = 18187;

struct Peer {
    Client client;
    websocketpp::connection_hdl hdl;
    std::thread worker;
    std::mutex m;
    std::deque<nlohmann::json> inbox;
    bool open = false;

    void connect() {
        client.clear_access_channels(websocketpp::log::alevel::all);
        client.clear_error_channels(websocketpp::log::elevel::all);
        client.init_asio();
        client.set_open_handler([this](websocketpp::connection_hdl h) {
            std::lock_guard<std::mutex> l(m);
            hdl = h;
            open = true;
        });
        client.set_message_handler([this](websocketpp::connection_hdl, Client::message_ptr msg) {
            auto j = nlohmann::json::parse(msg->get_payload(), nullptr, false);
            if (j.is_discarded()) return;
            std::lock_guard<std::mutex> l(m);
            inbox.push_back(std::move(j));
        });
        websocketpp::lib::error_code ec;
        auto con = client.get_connection("ws://127.0.0.1:" + std::to_string(kPort), ec);
        assert(!ec);
        client.connect(con);
        worker = std::thread([this] { client.run(); });
    }

    void send(const nlohmann::json& j) {
        websocketpp::lib::error_code ec;
        client.send(hdl, j.dump(), websocketpp::frame::opcode::text, ec);
        assert(!ec);
    }

    void close() {
        websocketpp::lib::error_code ec;
        client.close(hdl, websocketpp::close::status::normal, "done", ec);
        if (worker.joinable()) worker.join();
    }
};

// Pump the engine's main-thread queue (as Engine::update does) until a reply
// matching `pred` arrives.
nlohmann::json await(Peer& p, const std::function<bool(const nlohmann::json&)>& pred) {
    auto& server = Singularity::Network::WebSocketServer::instance();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        server.pollMainThread();
        {
            std::lock_guard<std::mutex> l(p.m);
            for (auto it = p.inbox.begin(); it != p.inbox.end(); ++it) {
                if (pred(*it)) {
                    auto j = *it;
                    p.inbox.erase(it);
                    return j;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    std::cerr << "timed out waiting for a reply\n";
    assert(false);
    return {};
}

auto ofType(const std::string& t) {
    return [t](const nlohmann::json& j) { return j.value("type", "") == t; };
}

void waitOpen(Peer& p) {
    auto& server = Singularity::Network::WebSocketServer::instance();
    for (int i = 0; i < 500; ++i) {
        server.pollMainThread();
        { std::lock_guard<std::mutex> l(p.m); if (p.open) return; }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    assert(false && "client never connected");
}

void authenticate(Peer& p, const PrivateKey& key) {
    p.send({{"type", "first_mover_challenge"}});
    auto c = await(p, ofType("first_mover_challenge"));
    const auto sig = hexEncode(key.sign(foreignSessionTranscript(
        c["challengeId"], c["nonce"], c["connection"], key.id())));
    p.send({{"type", "first_mover_authenticate"}, {"challengeId", c["challengeId"]},
            {"moverId", key.id().toString()}, {"signature", sig}});
    auto ack = await(p, ofType("first_mover_authenticate_ack"));
    assert(ack["status"] == "authenticated");
    assert(ack["displayName"] == "Claude Sonnet 4.5");

    // Replaying the same answer must fail: the challenge was consumed.
    p.send({{"type", "first_mover_authenticate"}, {"challengeId", c["challengeId"]},
            {"moverId", key.id().toString()}, {"signature", sig}});
    auto replay = await(p, ofType("first_mover_authenticate_ack"));
    assert(replay["status"] == "refused" && replay["reasonCode"] == "unknown-challenge");
}

} // namespace

int main() {
    const auto saves = std::filesystem::temp_directory_path() / "earthcall_fm_ws_saves";
    std::filesystem::remove_all(saves);
    std::filesystem::create_directories(saves);
    SaveSystem::setSaveRoot(saves.string());

    auto& reg = FirstMoverRegister::instance();
    reg.clear();
    reg.clearAuthenticatedPersons();
    reg.setSaveRoot(saves);
    PrivateKey zach = PrivateKey::generate();
    PrivateKey sonnet = PrivateKey::generate();
    assert(reg.trustAuthenticatedPerson(zach));
    assert(reg.recognize(zach, FirstMover::Kind::Person, sonnet.id(), FirstMover::Kind::Model,
                         "Claude Sonnet 4.5", {"laws/sonnet-*/**", "zones/SonnetGarden/**"}, 1000));

    auto& server = Singularity::Network::WebSocketServer::instance();
    server.start(kPort);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Peer a;
        a.connect();
        waitOpen(a);

        // 1. Unauthenticated: reads work, writes are refused with a reason.
        a.send({{"type", "first_mover_status"}});
        auto st = await(a, ofType("first_mover_status"));
        assert(st["authenticated"] == false);
        assert(st["authenticatedPersonPresent"] == true);

        a.send({{"type", "create_law"}, {"identifier", "sonnet-garden"}, {"name", "Sonnet's Garden"}});
        auto r = await(a, ofType("create_law_ack"));
        assert(r["status"] == "refused" && r["reasonCode"] == "no-first-mover-session");

        // 2. A bare mover id is not authentication.
        a.send({{"type", "first_mover_challenge"}});
        auto c = await(a, ofType("first_mover_challenge"));
        a.send({{"type", "first_mover_authenticate"}, {"challengeId", c["challengeId"]},
                {"moverId", sonnet.id().toString()}, {"signature", "00"}});
        auto bad = await(a, ofType("first_mover_authenticate_ack"));
        assert(bad["status"] == "refused" && bad["reasonCode"] == "signature-invalid");

        // 3. Prove the key.
        authenticate(a, sonnet);
        a.send({{"type", "first_mover_status"}});
        st = await(a, ofType("first_mover_status"));
        assert(st["authenticated"] == true && st["standing"] == "recognized");
        assert(st["moverId"] == sonnet.id().toString());

        // 4. Inside scope: authorized. (Headless engine may have no
        //    LawManager; then the act is admitted and reports that honestly.)
        a.send({{"type", "create_law"}, {"identifier", "sonnet-garden"}, {"name", "Sonnet's Garden"},
                {"firstMoverId", zach.id().toString()}, {"authority", 99}});   // ignored
        r = await(a, ofType("create_law_ack"));
        // Engine::initLogic() needs a GL context, so headless there is no
        // LawManager: the act is ADMITTED and then honestly reports that.
        // Authorship itself is witnessed in foreign_actuation_test
        // (foreignLawAuthors, the one seam both socket paths call).
        assert(r["status"] == "success" || r["status"] == "law_manager_unavailable");
        if (r["status"] == "success") {
            LawManager* lm = ::Core::Engine::instance().getLawManager();
            Law* law = lm ? lm->find("sonnet-garden") : nullptr;
            assert(law);
            // Truthful authorship: the mover, not the Person at the screen.
            const auto& members = law->authors().getMembers();
            assert(members.size() == 1 && members.front() == reg.find(sonnet.id()));
        }

        // 4b. A real allowed MUTATION, end to end: spawn into a Zone the
        //     Person granted. (Before any Zone is live, spawn is refused as
        //     unmapped rather than dereferencing an empty ZoneManager.)
        if (mgr.zones().empty()) {
            a.send({{"type", "spawn_object"}, {"name", "early"}});
            r = await(a, ofType("spawn_object_ack"));
            assert(r["status"] == "refused" && r["reasonCode"] == "unmapped-resource");
        }
        auto garden = mgr.authorZone("SonnetGarden", "tester", "");
        assert(garden);
        size_t gardenIndex = 0;
        for (size_t i = 0; i < mgr.zones().size(); ++i) if (mgr.zones()[i] == garden) gardenIndex = i;
        assert(mgr.switchTo(gardenIndex));
        a.send({{"type", "spawn_object"}, {"name", "sonnet-first-stone"}, {"shape", "Sphere"},
                {"position", {0, 1, 0}}});
        r = await(a, ofType("spawn_object_ack"));
        assert(r["status"] == "success");
        bool placed = false;
        for (const auto& o : mgr.active().getOwnedObjects()) placed |= (o && o->getObjectID() == "sonnet-first-stone");
        assert(placed);

        // 5. Outside scope: someone else's Law.
        a.send({{"type", "delete_law"}, {"identifier", "law-art-stroke-draw"}});
        r = await(a, ofType("delete_law_ack"));
        // (Headless: no LawManager, so delete_law answers before admit.
        //  create_law admits first, so it is the probe.)
        a.send({{"type", "create_law"}, {"identifier", "law-art-stroke-draw"}, {"name", "x"}});
        r = await(a, ofType("create_law_ack"));
        assert(r["status"] == "refused" && r["reasonCode"] == "outside-scope");

        // 6. The Person's body and presence have no mover coordinate.
        a.send({{"type", "teleport_player"}, {"position", {0, 50, 0}}});
        r = await(a, ofType("teleport_ack"));
        assert(r["status"] == "refused" && r["reasonCode"] == "unmapped-resource");
        a.send({{"type", "switch_zone"}, {"index", 0}});
        r = await(a, ofType("switch_zone_ack"));
        assert(r["status"] == "refused" && r["reasonCode"] == "unmapped-resource");

        // 7. Speech is attributed to the mover, whatever the payload claims.
        a.send({{"type", "utterance"}, {"payload", "hello, Earth"}, {"sourceClient", "Zach"}});
        auto heard = await(a, [](const nlohmann::json& j) {
            return j.value("type", "") == "engine_event" && j.value("payload", "") == "hello, Earth";
        });
        assert(heard["source"] == sonnet.id().toString());

        // 8. Authentication is per connection.
        Peer b;
        b.connect();
        waitOpen(b);
        b.send({{"type", "create_law"}, {"identifier", "sonnet-garden-2"}, {"name", "y"}});
        r = await(b, ofType("create_law_ack"));
        assert(r["status"] == "refused" && r["reasonCode"] == "no-first-mover-session");
        // ...and an unauthenticated peer may not wear a cryptographic identity.
        b.send({{"type", "utterance"}, {"payload", "i am sonnet"}, {"sourceClient", sonnet.id().toString()}});
        heard = await(b, [](const nlohmann::json& j) {
            return j.value("type", "") == "engine_event" && j.value("payload", "") == "i am sonnet";
        });
        assert(heard["source"].get<std::string>().rfind("foreign-unauthenticated:", 0) == 0);
        b.close();

        // 9. Revocation bites on the very next act of a live connection.
        assert(reg.revoke(zach, sonnet.id()));
        a.send({{"type", "create_law"}, {"identifier", "sonnet-garden-3"}, {"name", "z"}});
        r = await(a, ofType("create_law_ack"));
        assert(r["status"] == "refused" && r["reasonCode"] == "not-registered");
        a.close();
    }

    server.stop();
    reg.clear();
    reg.clearAuthenticatedPersons();
    std::filesystem::remove_all(saves);
    std::cout << "first_mover_websocket_test: ALL OK\n";
    return 0;
}
#else
int main() { return 0; }
#endif
