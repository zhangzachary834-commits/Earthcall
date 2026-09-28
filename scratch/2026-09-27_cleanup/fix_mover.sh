sed -i '' 's/std::optional<Identity::FirstMoverSession> moverSession;/std::optional<Identity::FirstMoverSession> moverSession;\n                    std::optional<Identity::SingularId> mover;/' src/Singularity/Network/WebSocketServer.cpp

sed -i '' 's/auto mover = admit/mover = admit/' src/Singularity/Network/WebSocketServer.cpp
