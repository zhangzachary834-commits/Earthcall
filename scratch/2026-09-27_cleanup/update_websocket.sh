sed -i '' 's/const Identity::SingularId\* mover = auth.moverFor(connection);/const Identity::SingularId* mover = auth.moverFor(connection);\n        if (!mover \&\& Relation::s_developerMode) {\n            static Identity::SingularId legacyMover = Identity::SingularId::opaque("legacy-person-session");\n            return legacyMover;\n        }/' src/Singularity/Network/WebSocketServer.cpp

sed -i '' 's/ok = (res == PropertyPath::PathResult::Ok || res == PropertyPath::PathResult::Unchanged);/ok = (res == PropertyPath::PathResult::Ok || res == PropertyPath::PathResult::Unchanged);\n                            if (ok) {\n                                std::string onBehalfOf = j.value("onBehalfOf", "");\n                                std::string lawId = onBehalfOf.empty() ? "mcp" : "mcp (on behalf of " + onBehalfOf + ")";\n                                targetBeing->addStakeholder(prop, mover->toString(), lawId, std::time(nullptr));\n                            }/' src/Singularity/Network/WebSocketServer.cpp

sed -i '' '/obj->addZoneDesignation(mgr.active().getIdentifier());/a\
                std::string onBehalfOf = j.value("onBehalfOf", "");\
                std::string lawId = onBehalfOf.empty() ? "mcp" : "mcp (on behalf of " + onBehalfOf + ")";\
                obj->addStakeholder("spawn", mover->toString(), lawId, std::time(nullptr));
' src/Singularity/Network/WebSocketServer.cpp
