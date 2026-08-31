#ifndef __EMSCRIPTEN__

#include "WebSocketServer.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "json.hpp"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <algorithm>
#include <iostream>
#include <mutex>
#include <vector>
#include <ctime>

extern ZoneManager mgr;
extern MaterialManager materials;

namespace Singularity {
namespace Network {

using ServerType = websocketpp::server<websocketpp::config::asio>;

static Object::ShapeKind parseShapeKind(const std::string& str, int intVal = 0) {
    if (str == "Cube" || str == "cube") return Object::ShapeKind::Cube;
    if (str == "Sphere" || str == "sphere") return Object::ShapeKind::Sphere;
    if (str == "Cylinder" || str == "cylinder") return Object::ShapeKind::Cylinder;
    if (str == "Cone" || str == "cone") return Object::ShapeKind::Cone;
    if (str == "Ellipsoid" || str == "ellipsoid") return Object::ShapeKind::Ellipsoid;
    if (str == "Ovoid" || str == "ovoid") return Object::ShapeKind::Ovoid;
    if (str == "Paraboloid" || str == "paraboloid") return Object::ShapeKind::Paraboloid;
    if (str == "Torus" || str == "torus") return Object::ShapeKind::Torus;
    if (str == "RoundedBox" || str == "rounded_box" || str == "roundedBox") return Object::ShapeKind::RoundedBox;
    if (str == "Polyhedron" || str == "polyhedron") return Object::ShapeKind::Polyhedron;
    if (str == "Shape2D" || str == "shape2d") return Object::ShapeKind::Shape2D;
    if (str == "Text2D" || str == "text2d") return Object::ShapeKind::Text2D;
    if (str == "Field" || str == "field") return Object::ShapeKind::Field;
    if (str == "Patch" || str == "patch") return Object::ShapeKind::Patch;
    
    if (intVal >= 0 && intVal <= 13) {
        return static_cast<Object::ShapeKind>(intVal);
    }
    return Object::ShapeKind::Cube;
}

static nlohmann::json buildWorldSnapshotJson() {
    nlohmann::json root;
    root["type"] = "state_sync";
    root["timestamp"] = static_cast<double>(std::time(nullptr));

    // Zone Manager
    root["active_zone_index"] = mgr.currentIndex();
    if (mgr.currentIndex() < mgr.zones().size() && mgr.zones()[mgr.currentIndex()]) {
        root["active_zone_name"] = mgr.active().name();
        root["active_zone_id"] = mgr.active().getIdentifier();
        root["active_zone_owner"] = mgr.active().owner();
    } else {
        root["active_zone_name"] = "Default Zone";
        root["active_zone_id"] = "zone-default";
        root["active_zone_owner"] = "Player";
    }

    auto& zones = mgr.zones();
    nlohmann::json zonesList = nlohmann::json::array();
    for (size_t i = 0; i < zones.size(); ++i) {
        if (zones[i]) {
            nlohmann::json zj;
            zj["index"] = i;
            zj["name"] = zones[i]->name();
            zj["identifier"] = zones[i]->getIdentifier();
            zj["owner"] = zones[i]->owner();
            zj["object_count"] = zones[i]->objects().size();
            zj["scope"] = zones[i]->scopeName();
            zonesList.push_back(zj);
        }
    }
    root["zones"] = zonesList;

    // Active Zone Objects
    nlohmann::json objList = nlohmann::json::array();
    for (const auto& objPtr : mgr.active().objects()) {
        if (!objPtr) continue;
        nlohmann::json oj;
        oj["id"] = objPtr->getObjectID();
        oj["name"] = objPtr->getIdentifier();
        oj["type"] = objPtr->getObjectType();
        oj["shapeKind"] = static_cast<int>(objPtr->getShapeKind());
        oj["spatialKind"] = static_cast<int>(objPtr->getSpatialKind());
        
        glm::vec3 pos = objPtr->getPosition();
        oj["position"] = {pos.x, pos.y, pos.z};
        
        glm::vec3 rot = objPtr->getRotationEulerDegrees();
        oj["rotation"] = {rot.x, rot.y, rot.z};
        
        oj["dimensions"] = objPtr->getDimensions();
        oj["materialId"] = objPtr->materialId();
        oj["color"] = {objPtr->faceColors[0][0], objPtr->faceColors[0][1], objPtr->faceColors[0][2]};
        oj["renderMode"] = objPtr->getRenderModeProp();
        objList.push_back(oj);
    }
    root["objects"] = objList;

    // Player & Camera Status
    Core::Engine& eng = Core::Engine::instance();
    Person* p = eng.getPerson();
    if (p) {
        nlohmann::json pj;
        pj["name"] = p->getIdentifier();
        glm::vec3 pPos = p->position();
        pj["position"] = {pPos.x, pPos.y, pPos.z};
        
        Core::Camera* cam = eng.getCamera();
        if (cam) {
            glm::vec3 camFront = cam->getFront();
            pj["camera_forward"] = {camFront.x, camFront.y, camFront.z};
            glm::vec3 camPos = cam->getPos();
            pj["camera_position"] = {camPos.x, camPos.y, camPos.z};
            pj["yaw"] = cam->yaw;
            pj["pitch"] = cam->pitch;
        }
        pj["flying"] = Physics::getFlying();
        root["player"] = pj;
    }

    // Laws
    LawManager* lm = eng.getLawManager();
    if (lm) {
        nlohmann::json lawsList = nlohmann::json::array();
        for (const auto& lawPtr : lm->getAll()) {
            if (!lawPtr) continue;
            nlohmann::json lj;
            lj["identifier"] = lawPtr->getIdentifier();
            lj["name"] = lawPtr->name();
            lj["enabled"] = lawPtr->isEnabled();
            lj["activation"] = static_cast<int>(lawPtr->activation());
            lj["expression"] = lawPtr->name();
            lawsList.push_back(lj);
        }
        root["laws"] = lawsList;
    }

    // Physics
    nlohmann::json phys;
    phys["flying"] = Physics::getFlying();
    phys["gravity_viz"] = Physics::getGravityVisualization();
    phys["legacy_engine"] = Physics::getLegacyEngineEnabled();
    root["physics"] = phys;

    return root;
}

struct WebSocketServer::Impl {
    ServerType server;
    std::thread worker;
    bool running = false;
    
    // Connection handles
    std::vector<websocketpp::connection_hdl> connections;
    std::mutex connections_mutex;

    static bool isAllowedOrigin(const std::string& origin) {
        if (origin.empty()) return true;
        if (origin.find("localhost") != std::string::npos ||
            origin.find("127.0.0.1") != std::string::npos ||
            origin.find("[::1]") != std::string::npos) {
            return true;
        }
        static const std::vector<std::string> allowed = {
            "http://localhost:5005",  "http://127.0.0.1:5005",
            "http://localhost:5000",  "http://127.0.0.1:5000",
            "http://localhost:3000",  "http://127.0.0.1:3000",
            "http://localhost:8080",  "http://127.0.0.1:8080",
            "https://localhost:5005", "https://127.0.0.1:5005",
            "https://localhost:3000", "https://127.0.0.1:3000",
            "https://trusted.earthcall.com"
        };
        return std::find(allowed.begin(), allowed.end(), origin) != allowed.end();
    }

    bool on_validate(websocketpp::connection_hdl hdl) {
        auto con = server.get_con_from_hdl(hdl);
        std::string origin = con->get_request_header("Origin");

        if (origin.empty()) return true;

        if (!isAllowedOrigin(origin)) {
            std::cout << "[WebSocketServer] Rejected connection from origin: " << origin << std::endl;
            return false;
        }
        return true;
    }

    void broadcast(const std::string& jsonPayload) {
        if (!running) return;
        std::lock_guard<std::mutex> lock(connections_mutex);
        for (auto& hdl : connections) {
            websocketpp::lib::error_code ec;
            server.send(hdl, jsonPayload, websocketpp::frame::opcode::text, ec);
        }
    }

    void sendTo(websocketpp::connection_hdl hdl, const std::string& jsonPayload) {
        websocketpp::lib::error_code ec;
        server.send(hdl, jsonPayload, websocketpp::frame::opcode::text, ec);
    }

    void on_message(websocketpp::connection_hdl hdl, ServerType::message_ptr msg) {
        std::string payload = msg->get_payload();
        try {
            auto j = nlohmann::json::parse(payload);
            if (!j.is_object()) return;

            std::string type = j.value("type", "");
            std::string clientId = std::to_string(reinterpret_cast<uintptr_t>(hdl.lock().get()));

            // 1. Query State / Get State
            if (type == "get_state" || type == "query_state") {
                nlohmann::json snapshot = buildWorldSnapshotJson();
                sendTo(hdl, snapshot.dump());
                return;
            }

            // 2. Utterance Event
            if (type == "utterance") {
                auto it = j.find("payload");
                if (it != j.end() && it->is_string()) {
                    Core::Event::Utterance evt;
                    evt.payload = it->get<std::string>();
                    evt.sourceClient = j.value("sourceClient", clientId);
                    evt.targetSingularId = j.value("targetSingularId", "");

                    Core::EventBus::instance().publish(evt);
                    std::cout << "[WebSocketServer] Received utterance: \"" << evt.payload << "\" from " << evt.sourceClient << std::endl;

                    nlohmann::json reply;
                    reply["type"] = "engine_event";
                    reply["event"] = "utterance";
                    reply["payload"] = evt.payload;
                    reply["source"] = evt.sourceClient;
                    broadcast(reply.dump());
                }
                return;
            }

            // 3. Property Write
            if (type == "property_write" || type == "PropertyWrite") {
                std::string target = j.value("target", "");
                std::string prop = j.value("property", "");
                auto valIt = j.find("value");

                if (!target.empty() && !prop.empty() && valIt != j.end()) {
                    Singular* targetBeing = nullptr;
                    
                    if (target == "@player" || target == "player" || target == "Player") {
                        targetBeing = Core::Engine::instance().getPerson();
                    } else if (target == "@active_zone" || target == "active_zone" || target == "zone") {
                        targetBeing = &mgr.active();
                    } else {
                        for (auto* being : Universe::instance().beings()) {
                            if (being && (being->getIdentifier() == target || (dynamic_cast<Object*>(being) && dynamic_cast<Object*>(being)->getObjectID() == target))) {
                                targetBeing = being;
                                break;
                            }
                        }
                    }

                    if (targetBeing) {
                        PropertyValue val = propertyValueFromJson(*valIt);
                        PropertyPath path = PropertyPath::parse(prop);
                        auto res = path.setValue(*targetBeing, val);
                        bool ok = (res == PropertyPath::PathResult::Ok || res == PropertyPath::PathResult::Unchanged);

                        nlohmann::json reply;
                        reply["type"] = "property_write_ack";
                        reply["target"] = target;
                        reply["property"] = prop;
                        reply["status"] = ok ? "success" : "failed";
                        sendTo(hdl, reply.dump());

                        if (ok) {
                            broadcast(buildWorldSnapshotJson().dump());
                        }
                    } else {
                        nlohmann::json reply;
                        reply["type"] = "property_write_ack";
                        reply["target"] = target;
                        reply["property"] = prop;
                        reply["status"] = "target_not_found";
                        sendTo(hdl, reply.dump());
                    }
                }
                return;
            }

            // 4. Spawn / Create Object
            if (type == "spawn_object" || type == "create_object") {
                std::string shapeStr = j.value("shape", j.value("shapeKind", "Cube"));
                int shapeInt = j.value("shapeKindInt", -1);
                Object::ShapeKind shape = parseShapeKind(shapeStr, shapeInt);

                auto obj = std::make_shared<Object>();
                
                std::string name = j.value("name", j.value("id", ""));
                if (!name.empty()) {
                    obj->setObjectID(name);
                }

                obj->setShape(shape);

                if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3) {
                    float px = j["position"][0].get<float>();
                    float py = j["position"][1].get<float>();
                    float pz = j["position"][2].get<float>();
                    obj->setPosition(glm::vec3(px, py, pz));
                } else {
                    Person* p = Core::Engine::instance().getPerson();
                    if (p) {
                        glm::vec3 pPos = p->position();
                        glm::vec3 fwd = p->cameraForward;
                        obj->setPosition(pPos + fwd * 3.0f + glm::vec3(0, 0.5f, 0));
                    }
                }

                if (j.contains("rotation") && j["rotation"].is_array() && j["rotation"].size() >= 3) {
                    float rx = j["rotation"][0].get<float>();
                    float ry = j["rotation"][1].get<float>();
                    float rz = j["rotation"][2].get<float>();
                    obj->setRotationEulerDegrees(glm::vec3(rx, ry, rz));
                }

                if (j.contains("dimensions")) {
                    float dim = j["dimensions"].get<float>();
                    if (dim > 0) obj->setDimensions(static_cast<int>(dim));
                }

                if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 3) {
                    float cr = j["color"][0].get<float>();
                    float cg = j["color"][1].get<float>();
                    float cb = j["color"][2].get<float>();
                    if (cr > 1.0f || cg > 1.0f || cb > 1.0f) { cr /= 255.0f; cg /= 255.0f; cb /= 255.0f; }
                    for (int f = 0; f < 6; ++f) {
                        obj->setFaceColor(f, cr, cg, cb);
                    }
                }

                std::string mat = j.value("materialId", "");
                if (!mat.empty()) {
                    obj->setMaterialId(mat);
                }

                mgr.active().addObject(obj);
                std::cout << "[WebSocketServer] Spawned object " << obj->getObjectID() << " (" << shapeStr << ") in " << mgr.active().name() << std::endl;

                nlohmann::json reply;
                reply["type"] = "spawn_object_ack";
                reply["status"] = "success";
                reply["id"] = obj->getObjectID();
                reply["name"] = obj->getIdentifier();
                sendTo(hdl, reply.dump());

                broadcast(buildWorldSnapshotJson().dump());
                return;
            }

            // 5. Delete / Destroy Object
            if (type == "delete_object" || type == "destroy_object") {
                std::string id = j.value("id", j.value("target", ""));
                if (!id.empty()) {
                    bool removed = mgr.active().removeObjectById(id);
                    nlohmann::json reply;
                    reply["type"] = "delete_object_ack";
                    reply["status"] = removed ? "success" : "not_found";
                    reply["id"] = id;
                    sendTo(hdl, reply.dump());

                    if (removed) {
                        broadcast(buildWorldSnapshotJson().dump());
                    }
                }
                return;
            }

            // 6. Transform Object / Update Object
            if (type == "transform_object" || type == "update_object" || type == "set_transform") {
                std::string id = j.value("id", j.value("target", ""));
                if (!id.empty()) {
                    Object* targetObj = nullptr;
                    for (auto& o : mgr.active().objects()) {
                        if (o && (o->getObjectID() == id || o->getIdentifier() == id)) {
                            targetObj = o.get();
                            break;
                        }
                    }

                    if (targetObj) {
                        if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3) {
                            float px = j["position"][0].get<float>();
                            float py = j["position"][1].get<float>();
                            float pz = j["position"][2].get<float>();
                            targetObj->setPosition(glm::vec3(px, py, pz));
                        }
                        if (j.contains("rotation") && j["rotation"].is_array() && j["rotation"].size() >= 3) {
                            float rx = j["rotation"][0].get<float>();
                            float ry = j["rotation"][1].get<float>();
                            float rz = j["rotation"][2].get<float>();
                            targetObj->setRotationEulerDegrees(glm::vec3(rx, ry, rz));
                        }
                        if (j.contains("dimensions")) {
                            targetObj->setDimensions(static_cast<int>(j["dimensions"].get<float>()));
                        }
                        if (j.contains("color") && j["color"].is_array() && j["color"].size() >= 3) {
                            float cr = j["color"][0].get<float>();
                            float cg = j["color"][1].get<float>();
                            float cb = j["color"][2].get<float>();
                            if (cr > 1.0f || cg > 1.0f || cb > 1.0f) { cr /= 255.0f; cg /= 255.0f; cb /= 255.0f; }
                            for (int f = 0; f < 6; ++f) {
                                targetObj->setFaceColor(f, cr, cg, cb);
                            }
                        }
                        if (j.contains("shape") || j.contains("shapeKind")) {
                            std::string shapeStr = j.value("shape", j.value("shapeKind", ""));
                            if (!shapeStr.empty()) {
                                targetObj->setShape(parseShapeKind(shapeStr));
                            }
                        }
                        if (j.contains("materialId")) {
                            targetObj->setMaterialId(j["materialId"].get<std::string>());
                        }

                        nlohmann::json reply;
                        reply["type"] = "transform_object_ack";
                        reply["status"] = "success";
                        reply["id"] = id;
                        sendTo(hdl, reply.dump());

                        broadcast(buildWorldSnapshotJson().dump());
                    } else {
                        nlohmann::json reply;
                        reply["type"] = "transform_object_ack";
                        reply["status"] = "not_found";
                        reply["id"] = id;
                        sendTo(hdl, reply.dump());
                    }
                }
                return;
            }

            // 7. Toggle Law
            if (type == "toggle_law" || type == "set_law_enabled") {
                std::string identifier = j.value("identifier", j.value("id", ""));
                bool enabled = j.value("enabled", true);
                LawManager* lm = Core::Engine::instance().getLawManager();
                if (lm && !identifier.empty()) {
                    bool found = false;
                    for (auto& law : lm->getAll()) {
                        if (law && (law->getIdentifier() == identifier || law->name() == identifier)) {
                            law->setEnabled(enabled);
                            found = true;
                            break;
                        }
                    }

                    nlohmann::json reply;
                    reply["type"] = "toggle_law_ack";
                    reply["identifier"] = identifier;
                    reply["enabled"] = enabled;
                    reply["status"] = found ? "success" : "not_found";
                    sendTo(hdl, reply.dump());

                    if (found) {
                        broadcast(buildWorldSnapshotJson().dump());
                    }
                }
                return;
            }

            // 8. Switch Zone
            if (type == "switch_zone" || type == "change_zone") {
                if (j.contains("index")) {
                    size_t idx = j["index"].get<size_t>();
                    if (idx < mgr.zones().size()) {
                        mgr.switchTo(idx);
                        broadcast(buildWorldSnapshotJson().dump());
                    }
                } else if (j.contains("name")) {
                    std::string zname = j["name"].get<std::string>();
                    for (size_t i = 0; i < mgr.zones().size(); ++i) {
                        if (mgr.zones()[i] && mgr.zones()[i]->name() == zname) {
                            mgr.switchTo(i);
                            broadcast(buildWorldSnapshotJson().dump());
                            break;
                        }
                    }
                }
                return;
            }

            // 9. Create Zone
            if (type == "create_zone") {
                std::string zname = j.value("name", "New Zone");
                std::string kind = j.value("kind", "zone");
                Person* p = Core::Engine::instance().getPerson();
                std::string owner = p ? p->getIdentifier() : "Player";
                mgr.authorZone(zname, owner, kind);
                broadcast(buildWorldSnapshotJson().dump());
                return;
            }

            // 10. Teleport Player
            if (type == "teleport_player" || type == "teleport") {
                if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3) {
                    float px = j["position"][0].get<float>();
                    float py = j["position"][1].get<float>();
                    float pz = j["position"][2].get<float>();
                    
                    Person* p = Core::Engine::instance().getPerson();
                    if (p) p->position() = glm::vec3(px, py, pz);
                    
                    Core::Camera* cam = Core::Engine::instance().getCamera();
                    if (cam) cam->pos = glm::vec3(px, py + 1.8f, pz);

                    broadcast(buildWorldSnapshotJson().dump());
                }
                return;
            }

            // 11. Physics Controls
            if (type == "set_physics") {
                if (j.contains("flying")) {
                    Physics::setFlying(j["flying"].get<bool>());
                }
                if (j.contains("gravity_viz")) {
                    Physics::setGravityVisualization(j["gravity_viz"].get<bool>());
                }
                broadcast(buildWorldSnapshotJson().dump());
                return;
            }

            // 12. Quick Save
            if (type == "quick_save" || type == "save_world") {
                SaveContext ctx;
                Core::Engine& eng = Core::Engine::instance();
                ctx.camera = eng.getCamera();
                ctx.mouseHandler = eng.getMouseHandler();
                ctx.person = eng.getPerson();
                ctx.lawManager = eng.getLawManager();
                ctx.worldTime = eng.worldTimePtr();
                std::string customName = j.value("name", "");
                mgr.saveStateWithLog(customName, ctx);

                nlohmann::json reply;
                reply["type"] = "save_ack";
                reply["status"] = "success";
                sendTo(hdl, reply.dump());
                return;
            }

            // Echo fallback
            nlohmann::json echo;
            echo["status"] = "received";
            echo["echo"] = j;
            sendTo(hdl, echo.dump());

        } catch (const std::exception& e) {
            std::cerr << "[WebSocketServer] Failed to process message: " << e.what() << std::endl;
        }
    }

    void on_open(websocketpp::connection_hdl hdl) {
        {
            std::lock_guard<std::mutex> lock(connections_mutex);
            connections.push_back(hdl);
        }
        std::cout << "[WebSocketServer] Client connected from " 
                  << server.get_con_from_hdl(hdl)->get_remote_endpoint() << std::endl;
        
        try {
            nlohmann::json snapshot = buildWorldSnapshotJson();
            sendTo(hdl, snapshot.dump());
        } catch (const std::exception& e) {
            std::cerr << "[WebSocketServer] Error sending initial snapshot: " << e.what() << std::endl;
        }
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
    _impl->server.set_max_message_size(4 * 1024 * 1024); // 4 MB limit
    
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
    
    _impl->worker = std::thread([this, port]() {
        std::cout << "[WebSocketServer] Earthcall C++ WebSocket Server listening on ws://127.0.0.1:" << port << std::endl;
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
        _impl->connections.clear();
    }
    _impl->server.stop();
    
    if (_impl->worker.joinable()) {
        _impl->worker.join();
    }
}

void WebSocketServer::broadcast(const std::string& jsonPayload) {
    _impl->broadcast(jsonPayload);
}

void WebSocketServer::broadcastStateSync() {
    if (!_impl->running) return;
    try {
        nlohmann::json snapshot = buildWorldSnapshotJson();
        _impl->broadcast(snapshot.dump());
    } catch (const std::exception& e) {
        std::cerr << "[WebSocketServer] broadcastStateSync error: " << e.what() << std::endl;
    }
}

bool WebSocketServer::isRunning() const {
    return _impl->running;
}

} // namespace Network
} // namespace Singularity

#endif // __EMSCRIPTEN__
