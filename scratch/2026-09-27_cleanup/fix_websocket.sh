sed -i '' 's/Identity::SingularId::opaque("legacy-person-session")/Identity::SingularId::mintOpaque()/' src/Singularity/Network/WebSocketServer.cpp
