import os

with open("src/Singularity/Network/WebSocketServer.cpp", "r") as f:
    code = f.read()

# 1. include
if '#include "Relation/Relation.hpp"' not in code:
    code = code.replace('#include "Singularity/Network/WebSocketServer.hpp"', '#include "Singularity/Network/WebSocketServer.hpp"\n#include "Relation/Relation.hpp"')

# 2. admit
code = code.replace(
'''    std::optional<Identity::SingularId> admit(websocketpp::connection_hdl hdl,
                                              const std::string& connection,
                                              const std::string& ackType,
                                              const std::string& resource,
                                              const std::string& property = "",
                                              const std::string& unmappedReason = "",
                                              const nlohmann::json& context = nlohmann::json::object()) {
        auto& reg = Identity::FirstMoverRegister::instance();
        const Identity::SingularId* mover = auth.moverFor(connection);
        const auto decision = Foreign::authorizeForeignActuation(''',
'''    std::optional<Identity::SingularId> admit(websocketpp::connection_hdl hdl,
                                              const std::string& connection,
                                              const std::string& ackType,
                                              const std::string& resource,
                                              const std::string& property = "",
                                              const std::string& unmappedReason = "",
                                              const nlohmann::json& context = nlohmann::json::object()) {
        auto& reg = Identity::FirstMoverRegister::instance();
        const Identity::SingularId* mover = auth.moverFor(connection);

        if (!mover && Relation::s_developerMode) {
            static Identity::SingularId legacyMover = Identity::SingularId::mintOpaque();
            return legacyMover;
        }

        const auto decision = Foreign::authorizeForeignActuation(''')

# 3. property_write
code = code.replace(
'''                    std::optional<Identity::FirstMoverSession> moverSession;
                    if (targetBeing) {
                        auto mover = admit(hdl, clientId, "property_write_ack", resource, prop, unmapped,
                                           {{"target", target}});
                        if (!mover) return;
                        moverSession.emplace(Identity::FirstMoverRegister::instance(), *mover);
                    }''',
'''                    std::optional<Identity::SingularId> mover;
                    std::optional<Identity::FirstMoverSession> moverSession;
                    if (targetBeing) {
                        mover = admit(hdl, clientId, "property_write_ack", resource, prop, unmapped,
                                           {{"target", target}});
                        if (!mover) return;
                        moverSession.emplace(Identity::FirstMoverRegister::instance(), *mover);
                    }''')

code = code.replace(
'''                            ok = (res == PropertyPath::PathResult::Ok || res == PropertyPath::PathResult::Unchanged);
                        }''',
'''                            ok = (res == PropertyPath::PathResult::Ok || res == PropertyPath::PathResult::Unchanged);
                            if (ok) {
                                std::string onBehalfOf = j.value("onBehalfOf", "");
                                std::string lawId = onBehalfOf.empty() ? "mcp" : "mcp (on behalf of " + onBehalfOf + ")";
                                targetBeing->addStakeholder(prop, mover->toString(), lawId, std::time(nullptr));
                            }
                        }''')

# 4. spawn_object
code = code.replace(
'''                obj->addZoneDesignation(mgr.active().getIdentifier());
                mgr.active().addObject(obj);''',
'''                obj->addZoneDesignation(mgr.active().getIdentifier());
                std::string onBehalfOf = j.value("onBehalfOf", "");
                std::string lawId = onBehalfOf.empty() ? "mcp" : "mcp (on behalf of " + onBehalfOf + ")";
                obj->addStakeholder("spawn", mover->toString(), lawId, std::time(nullptr));
                mgr.active().addObject(obj);''')

with open("src/Singularity/Network/WebSocketServer.cpp", "w") as f:
    f.write(code)

