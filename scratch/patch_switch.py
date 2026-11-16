import sys

with open("src/Singularity/Network/WebSocketServer.cpp", "r") as f:
    code = f.read()

code = code.replace(
    'if (type == "switch_zone" || type == "change_zone") {',
    'if (type == "switch_zone" || type == "change_zone") {\n                std::cout << "[WebSocketServer] RECEIVED switch_zone name=" << j.value("name", "") << std::endl;'
)

with open("src/Singularity/Network/WebSocketServer.cpp", "w") as f:
    f.write(code)

print("Patched!")
