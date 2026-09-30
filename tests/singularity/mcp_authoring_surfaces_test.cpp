// What a foreign First Mover (Claude Sonnet 4.5, through MCP) can AUTHOR, each
// witnessed end to end rather than assumed:
//
//   1. SDFs     spawn_field over the real socket -> an Object whose signed
//               distance is right on the CPU AND compiles through the real
//               production WGSL emitter (a field that evaluates perfectly and
//               cannot be drawn is the failure gyroid_reliquary_test caught).
//   2. Volumes  author_field_node -> a FieldNode the Zone owns, discovered as a
//               participating medium, density correct on the CPU, accepted by
//               the WGSL density/extinction emitters, persisted in zone.json,
//               re-editable through property_write, refused out of scope.
//   3. Law Line the Terminal's natural-language Law grammar, authored AS the
//               mover (never as the Person at the keyboard), with preview,
//               search and refusal; and the socket gate in front of it.
//
// Zach, 2026-09-30: "MAKE SURE THE SDFS WORK VERIFY THEM WITH TESTS MAKE SURE
// VOLUMETRICS CAN BE AUTHORED VERIFY WITH TESTS MAKE SURE SONNET CAN USE THE
// NEW LAW AUTHORING CLI TOO". Written by Claude Opus 5.5.
#ifndef __EMSCRIPTEN__
#include "Singularity/Network/WebSocketServer.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Terminal/TerminalChannel.hpp"
#include "Singularity/Foreign/ForeignActuationGuard.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Relation/Relation.hpp"
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
#include <cmath>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

using Client = websocketpp::client<websocketpp::config::asio_client>;
using namespace Identity;
extern ZoneManager mgr;

namespace {

constexpr uint16_t kPort = 18193;

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

nlohmann::json await(Peer& p, const std::string& type) {
    auto& server = Singularity::Network::WebSocketServer::instance();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        server.pollMainThread();
        {
            std::lock_guard<std::mutex> l(p.m);
            for (auto it = p.inbox.begin(); it != p.inbox.end(); ++it) {
                if (it->value("type", "") == type) {
                    auto j = *it;
                    p.inbox.erase(it);
                    return j;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    std::cerr << "timed out waiting for " << type << "\n";
    assert(false);
    return {};
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
    auto c = await(p, "first_mover_challenge");
    const auto sig = hexEncode(key.sign(foreignSessionTranscript(
        c["challengeId"], c["nonce"], c["connection"], key.id())));
    p.send({{"type", "first_mover_authenticate"}, {"challengeId", c["challengeId"]},
            {"moverId", key.id().toString()}, {"signature", sig}});
    assert(await(p, "first_mover_authenticate_ack")["status"] == "authenticated");
}

double at(const OntoMath::Piecewise& pw, float x, float y, float z) {
    std::map<std::string, PropertyValue> vars{
        {"x", PropertyValue(double(x))}, {"y", PropertyValue(double(y))},
        {"z", PropertyValue(double(z))}, {"p", PropertyValue(glm::vec3(x, y, z))}};
    auto v = pw.evaluate(vars);
    assert(v.has_value());
    if (const auto* d = std::get_if<double>(&*v)) return *d;
    if (const auto* f = std::get_if<float>(&*v)) return *f;
    if (const auto* i = std::get_if<int>(&*v)) return *i;
    assert(false && "density did not evaluate to a scalar");
    return 0.0;
}

bool near(double a, double b, double eps = 1e-3) { return std::fabs(a - b) < eps; }

geom::FieldNode* fieldIn(Zone& z, const std::string& id) {
    for (const auto& f : z.additionalSpatialFields()) {
        if (f && f->getIdentifier() == id) return f.get();
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// 3a. The Law Line, directly: the grammar is the Terminal's own, the author is
//     the mover. (Headless has no Engine LawManager, so this half drives the
//     channel with its own, exactly as law_line_test does.)
// ---------------------------------------------------------------------------
void lawLineAsMover(FirstMoverRegister& reg, const PrivateKey& sonnet) {
    using Singularity::Terminal::TerminalChannel;
    LawManager laws;
    // Deliberately NOT connectToEventBus(): LawManager never unsubscribes in
    // its destructor, so a short-lived one leaves a dangling EventBus
    // handler that the next published event crashes into (found 2026-09-30,
    // on the To-Do list). Authoring needs no event wiring.
    TerminalChannel::syncRegister(laws);
    TerminalChannel* terminal = TerminalChannel::find(laws);
    assert(terminal);
    terminal->setSink([](const std::string&) {});   // keep stdout clean

    const auto authors = Singularity::Foreign::foreignLawAuthors(reg, sonnet.id());
    assert(authors.size() == 1);

    // Preview ('?') and search ('??') are read-only: no author, nothing added.
    const size_t before = laws.getAll().size();
    auto preview = terminal->authorForeign(laws, "on \"sonnet-bloomed\" then set glow 1?", {}, "");
    std::cout << "  preview: " << preview.status << " | " << preview.preview << "\n";
    assert(preview.status == "preview");
    assert(!preview.preview.empty());
    auto search = terminal->authorForeign(laws, "?? set", {}, "");
    assert(search.status == "search" && !search.candidates.empty());
    assert(laws.getAll().size() == before);
    assert(TerminalChannel::isReadOnlySentence("on tick then set glow 1?"));
    assert(TerminalChannel::isReadOnlySentence("?? glow"));
    assert(!TerminalChannel::isReadOnlySentence("on tick then set glow 1"));

    // Authoring needs an identifier (the scope coordinate).
    auto noId = terminal->authorForeign(laws, "on \"sonnet-bloomed\" then set glow 1", authors, "");
    assert(noId.status == "refused");

    // A sentence the grammar cannot read is refused with its reason.
    auto nonsense = terminal->authorForeign(laws, "on \"sonnet-bloomed\" then frobnicate glow 1",
                                            authors, "sonnet-nonsense");
    assert(nonsense.status == "refused" && !nonsense.error.empty());
    assert(!laws.find("sonnet-nonsense"));

    // The real thing.
    auto made = terminal->authorForeign(laws, "on \"sonnet-bloomed\" then set glow 1",
                                        authors, "sonnet-bloom");
    std::cout << "  authored: " << made.status << " " << made.lawId << " | " << made.detail << "\n";
    assert(made.status == "authored" && made.lawId == "sonnet-bloom");
    Law* law = laws.find("sonnet-bloom");
    assert(law && law->isAuthored() && law->isEnabled());
    assert(law->authors().getMembers().size() == 1);
    assert(law->authors().getMembers().front() == reg.find(sonnet.id()));   // the mover, not a Person
    const auto& triggers = laws.triggersOf("sonnet-bloom");
    assert(std::find(triggers.begin(), triggers.end(), "sonnet-bloomed") != triggers.end());
    assert(law->hasActionModel());
    {
        const ActionNode* act = law->actionModel();
        assert(act->kind == ActionNode::Kind::Set);
        const PropertyValue& v = act->operand;
        const double num = std::holds_alternative<double>(v) ? std::get<double>(v)
                         : std::holds_alternative<int>(v) ? double(std::get<int>(v)) : -1.0;
        std::cout << "  action: " << act->describe() << "\n";
        assert(num == 1.0 && "the sentence's value reached the Law, whatever the preview prints");
    }

    // Same identifier twice is refused, never overwritten.
    auto again = terminal->authorForeign(laws, "on \"sonnet-bloomed\" then set glow 0",
                                         authors, "sonnet-bloom");
    assert(again.status == "refused");
    std::cout << "  Law Line authors as the mover; preview/search read-only; refusals explicit OK\n";
}

} // namespace

int main() {
    // The gate must not lean on developer mode (see first_mover_websocket_test).
    Relation::s_developerMode = true;

    const auto sandbox = std::filesystem::temp_directory_path() / "earthcall_mcp_authoring";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "saves");
    SaveSystem::setSaveRoot((sandbox / "saves").string());

    auto& reg = FirstMoverRegister::instance();
    reg.clear();
    reg.clearAuthenticatedPersons();
    reg.setSaveRoot(sandbox / "saves");
    PrivateKey zach = PrivateKey::generate();
    PrivateKey sonnet = PrivateKey::generate();
    assert(reg.trustAuthenticatedPerson(zach));
    assert(reg.recognize(zach, FirstMover::Kind::Person, sonnet.id(), FirstMover::Kind::Model,
                         "Claude Sonnet 4.5", {"zones/SonnetGarden/**", "laws/sonnet-*/**"}, 1000));

    lawLineAsMover(reg, sonnet);

    auto garden = mgr.authorZone("SonnetGarden", "tester", "");
    auto zachs = mgr.authorZone("ZachsWorkshop", "tester", "");
    assert(garden && zachs);
    size_t gardenIndex = 0, zachsIndex = 0;
    for (size_t i = 0; i < mgr.zones().size(); ++i) {
        if (mgr.zones()[i] == garden) gardenIndex = i;
        if (mgr.zones()[i] == zachs) zachsIndex = i;
    }
    assert(mgr.switchTo(gardenIndex));

    auto& server = Singularity::Network::WebSocketServer::instance();
    server.start(kPort);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Peer a;
    a.connect();
    waitOpen(a);

    // --- 3b. The Law Line's socket gate ------------------------------------
    a.send({{"type", "law_sentence"}, {"text", "on \"sonnet-bloomed\" then set glow 1"},
            {"identifier", "sonnet-bloom"}});
    auto r = await(a, "law_sentence_ack");
    assert(r["status"] == "refused" && r["reasonCode"] == "no-first-mover-session");
    authenticate(a, sonnet);
    a.send({{"type", "law_sentence"}, {"text", "on \"sonnet-bloomed\" then set glow 1"}});
    r = await(a, "law_sentence_ack");
    assert(r["status"] == "refused" && r["reasonCode"] == "unmapped-resource");   // no identifier
    a.send({{"type", "law_sentence"}, {"text", "on \"x\" then set glow 1"}, {"identifier", "law-art-stroke-draw"}});
    r = await(a, "law_sentence_ack");
    assert(r["status"] == "refused" && r["reasonCode"] == "outside-scope");
    a.send({{"type", "law_sentence"}, {"text", "on \"sonnet-bloomed\" then set glow 1"},
            {"identifier", "sonnet-bloom"}});
    r = await(a, "law_sentence_ack");
    // Admitted; headless has no Engine LawManager and says so honestly.
    assert(r["status"] == "law_manager_unavailable" || r["status"] == "authored");
    std::cout << "  Law Line socket gate: unauthenticated / no identifier / out of scope refused, in scope admitted OK\n";

    // --- 1. SDF --------------------------------------------------------------
    a.send({{"type", "spawn_field"}, {"name", "sonnet-orb"}, {"expr", "sqrt(x*x+y*y+z*z) - 0.5"},
            {"position", {0, 1, 0}}, {"extent", 1.0}});
    r = await(a, "spawn_field_ack");
    assert(r["status"] == "success" && r["isField"] == true);
    Object* orb = nullptr;
    for (const auto& o : mgr.active().getOwnedObjects()) if (o && o->getObjectID() == "sonnet-orb") orb = o.get();
    assert(orb && orb->hasField());
    const geom::SdfNode& sdf = orb->getFieldData();
    assert(sdf.mathNode && "the expression must lift exactly into OntoMath, not fall back to RPN");
    assert(near(geom::evalSdf(sdf, glm::vec3(0.0f)), -0.5));
    assert(near(geom::evalSdf(sdf, glm::vec3(1.0f, 0.0f, 0.0f)), 0.5));
    assert(near(geom::evalSdf(sdf, glm::vec3(0.0f, 0.5f, 0.0f)), 0.0));
    auto program = sdfwgsl::compile(sdf);
    if (!program.ok) std::cerr << "SDF WGSL refused: " << program.error << "\n";
    assert(program.ok && !program.wgsl.empty());

    // A torus, and a shape that only exists as an authored expression.
    a.send({{"type", "spawn_field"}, {"name", "sonnet-ring"},
            {"expr", "sqrt((sqrt(x*x+z*z) - 0.6)^2 + y*y) - 0.15"}});
    r = await(a, "spawn_field_ack");
    assert(r["status"] == "success");
    Object* ring = nullptr;
    for (const auto& o : mgr.active().getOwnedObjects()) if (o && o->getObjectID() == "sonnet-ring") ring = o.get();
    assert(ring && near(geom::evalSdf(ring->getFieldData(), glm::vec3(0.6f, 0.0f, 0.0f)), -0.15));
    assert(sdfwgsl::compile(ring->getFieldData()).ok);

    // An expression that does not parse is refused -- it used to spawn an
    // invisible object and answer "success".
    const size_t objectsBefore = mgr.active().getOwnedObjects().size();
    a.send({{"type", "spawn_field"}, {"name", "sonnet-broken"}, {"expr", "sqrt(x*"}});
    r = await(a, "spawn_field_ack");
    assert(r["status"] == "invalid_arguments");
    assert(mgr.active().getOwnedObjects().size() == objectsBefore);
    // The shorthand the MCP tool has always advertised now builds the real
    // SdfNode tree (primitives + CSG ops), not nothing.
    const auto spawnShape = [&](const std::string& name, const std::string& expr) -> Object* {
        a.send({{"type", "spawn_field"}, {"name", name}, {"expr", expr}});
        auto ack = await(a, "spawn_field_ack");
        if (ack["status"] != "success") std::cerr << expr << " -> " << ack.dump() << "\n";
        assert(ack["status"] == "success");
        for (const auto& o : mgr.active().getOwnedObjects()) if (o && o->getObjectID() == name) return o.get();
        assert(false);
        return nullptr;
    };
    Object* s1 = spawnShape("sonnet-sphere", "sphere(0.5)");
    assert(s1->getFieldData().prim == geom::SdfPrim::Sphere);
    assert(near(geom::evalSdf(s1->getFieldData(), glm::vec3(0.0f)), -0.5));
    assert(near(geom::evalSdf(s1->getFieldData(), glm::vec3(0.0f, 0.0f, 1.0f)), 0.5));
    Object* s2 = spawnShape("sonnet-moved-box", "move(box(0.25), 1, 0, 0)");
    assert(near(geom::evalSdf(s2->getFieldData(), glm::vec3(1.0f, 0.0f, 0.0f)), -0.25));
    assert(near(geom::evalSdf(s2->getFieldData(), glm::vec3(1.5f, 0.0f, 0.0f)), 0.25));
    Object* s3 = spawnShape("sonnet-carved", "subtract(sphere(0.5), move(sphere(0.3), 0.4, 0, 0))");
    assert(geom::evalSdf(s3->getFieldData(), glm::vec3(0.4f, 0.0f, 0.0f)) > 0.0f);   // carved out
    assert(geom::evalSdf(s3->getFieldData(), glm::vec3(-0.3f, 0.0f, 0.0f)) < 0.0f);  // still solid
    Object* s4 = spawnShape("sonnet-bloom-shape",
                            "smoothUnion(sphere(0.4), move(torus(0.5, 0.1), 0, 0.3, 0), 0.2)");
    Object* s5 = spawnShape("sonnet-union3", "union(sphere(0.2), move(box(0.1),0.5,0,0), move(cylinder(0.1,0.3),-0.5,0,0))");
    assert(geom::evalSdf(s5->getFieldData(), glm::vec3(0.5f, 0.0f, 0.0f)) < 0.0f);
    assert(geom::evalSdf(s5->getFieldData(), glm::vec3(-0.5f, 0.0f, 0.0f)) < 0.0f);
    for (Object* o : {s1, s2, s3, s4, s5}) {
        auto prog = sdfwgsl::compile(o->getFieldData());
        if (!prog.ok) std::cerr << o->getObjectID() << " WGSL refused: " << prog.error << "\n";
        assert(prog.ok && !prog.wgsl.empty());
    }
    // Not shorthand and not an equation: refused with a reason naming both.
    a.send({{"type", "spawn_field"}, {"name", "sonnet-typo"}, {"expr", "spehre(0.5)"}});
    r = await(a, "spawn_field_ack");
    assert(r["status"] == "invalid_arguments");
    assert(r["reason"].get<std::string>().find("spehre") != std::string::npos);
    std::cout << "  SDF spawn: exact distances on CPU, compiles to WGSL, bad expression refused OK\n";

    // --- 2. Volumes ----------------------------------------------------------
    a.send({{"type", "author_field_node"}, {"identifier", "sonnet-mist"},
            {"origin", {0, 1, 0}}, {"scale", {2, 2, 2}},
            {"density", "1 - sqrt(x*x+y*y+z*z)"},
            {"extinction", "0.5 * (1 - sqrt(x*x+y*y+z*z))"},
            {"scattering", "0.8"}});
    r = await(a, "author_field_node_ack");
    std::cout << "  author_field_node: " << r.dump() << "\n";
    assert(r["status"] == "success" && r["created"] == true && r["isVolume"] == true);
    geom::FieldNode* mist = fieldIn(mgr.active(), "sonnet-mist");
    assert(mist);
    assert(near(at(*mist->volumeDensity, 0, 0, 0), 1.0));
    assert(near(at(*mist->volumeDensity, 0.5f, 0, 0), 0.5));
    assert(near(at(*mist->volumeExtinction, 0, 0, 0), 0.5));
    assert(mist->origin == glm::vec3(0, 1, 0) && mist->scale == glm::vec3(2, 2, 2));

    // The renderer discovers it as a participating medium...
    Rendering::VolumeDensityBinding medium;
    assert(Rendering::readVolumeDensity(*mist, 0.0, 0.0, medium));
    assert(medium.producerId == "sonnet-mist" && medium.densityExpr && medium.extinctionExpr);
    // ...and the production emitters accept every channel.
    auto dl = sdfwgsl::inspectDensityExpression(mist->volumeDensity.get());
    if (!dl.ok) std::cerr << "density WGSL refused: " << dl.error << "\n";
    assert(dl.ok);
    assert(sdfwgsl::inspectExtinctionExpression(mist->volumeExtinction.get()).ok);
    assert(sdfwgsl::inspectScatteringExpression(mist->volumeScattering.get()).ok);
    geom::SdfNode none;
    auto volumeProgram = sdfwgsl::compile(none, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                          mist->volumeDensity.get(), sdfwgsl::DensityInputKind::Authored,
                                          mist->volumeExtinction.get(), mist->volumeScattering.get());
    if (!volumeProgram.ok) std::cerr << "volume WGSL refused: " << volumeProgram.error << "\n";
    assert(volumeProgram.ok);

    // Persisted with the Zone, and it round-trips.
    {
        // Through SaveSystem, not a guessed filename: Zones persist as
        // zone.ecform now, and the reader is the contract.
        nlohmann::json zj = SaveSystem::readZoneIdentity("SonnetGarden");
        assert(zj.is_object() && !zj.empty() && "SonnetGarden was not persisted");
        const nlohmann::json* found = nullptr;
        std::function<void(const nlohmann::json&)> walk = [&](const nlohmann::json& n) {
            if (found) return;
            if (n.is_object()) {
                if (n.value("id", "") == "sonnet-mist" && n.contains("volumeDensity")) { found = &n; return; }
                for (auto& [k, v] : n.items()) walk(v);
            } else if (n.is_array()) {
                for (auto& v : n) walk(v);
            }
        };
        walk(zj);
        assert(found && "volume did not reach zone.json");
        auto reloaded = geom::FieldNode::fromJson(*found);
        assert(reloaded && near(at(*reloaded->volumeDensity, 0.5f, 0, 0), 0.5));
    }

    // Re-edit through the ordinary registered property path, sending the
    // Piecewise document itself.
    auto quarter = geom::makeImplicit("0.25");
    assert(quarter.mathNode);
    a.send({{"type", "property_write"}, {"target", "sonnet-mist"}, {"property", "volume.density.ast"},
            {"value", OntoMath::Piecewise::continuous(quarter.mathNode).toJson()}});
    r = await(a, "property_write_ack");
    assert(r["status"] == "success");
    assert(near(at(*mist->volumeDensity, 0.3f, 0.1f, 0), 0.25));

    // Updating keeps the same being.
    a.send({{"type", "author_field_node"}, {"identifier", "sonnet-mist"}, {"emission", "0.1"}});
    r = await(a, "author_field_node_ack");
    assert(r["status"] == "success" && r["created"] == false);
    assert(fieldIn(mgr.active(), "sonnet-mist") == mist);

    // Honest refusals.
    a.send({{"type", "author_field_node"}, {"identifier", "sonnet-fog"}, {"density", "exp(-(x*x))"}});
    r = await(a, "author_field_node_ack");
    assert(r["status"] == "invalid_arguments");   // compound exp: cannot lift exactly
    assert(!fieldIn(mgr.active(), "sonnet-fog"));
    a.send({{"type", "author_field_node"}, {"identifier", "sonnet-fog"}, {"density", "1 - (x"}});
    r = await(a, "author_field_node_ack");
    assert(r["status"] == "invalid_arguments");
    assert(mgr.switchTo(zachsIndex));
    a.send({{"type", "author_field_node"}, {"identifier", "sonnet-fog"}, {"density", "0.5"}});
    r = await(a, "author_field_node_ack");
    assert(r["status"] == "refused" && r["reasonCode"] == "outside-scope");   // not Sonnet's Zone
    assert(!fieldIn(mgr.active(), "sonnet-fog"));
    // Still Sonnet's field, found in its owning Zone even from elsewhere.
    // A document that is not JSON is refused by the bridge, not half-applied...
    a.send({{"type", "property_write"}, {"target", "sonnet-mist"}, {"property", "volume.density.ast"},
            {"value", "not json {"}});
    r = await(a, "property_write_ack");
    assert(r["status"] == "failed");
    assert(near(at(*mist->volumeDensity, 0, 0, 0), 0.25));
    // ...while an empty Piecewise is an authored act: this field is no longer
    // a medium. The renderer stops discovering it.
    a.send({{"type", "property_write"}, {"target", "sonnet-mist"}, {"property", "volume.density.ast"},
            {"value", "{}"}});
    r = await(a, "property_write_ack");
    assert(r["status"] == "success");
    Rendering::VolumeDensityBinding cleared;
    assert(!Rendering::readVolumeDensity(*mist, 0.0, 0.0, cleared));
    std::cout << "  volumes: authored, discovered, WGSL-accepted, persisted, re-editable, refused out of scope OK\n";

    a.close();
    server.stop();
    reg.clear();
    reg.clearAuthenticatedPersons();
    std::filesystem::remove_all(sandbox);
    std::cout << "mcp_authoring_surfaces_test: ALL OK\n";
    return 0;
}
#else
int main() { return 0; }
#endif
