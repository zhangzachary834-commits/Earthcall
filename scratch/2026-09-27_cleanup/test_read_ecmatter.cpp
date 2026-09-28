#include <iostream>
#include <fstream>
#include <vector>
#include "src/json.hpp"

int main() {
    std::ifstream in("saves/worlds/chess_app.dfe78273e7849f12.ecmatter", std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    nlohmann::json wrapper = nlohmann::json::from_msgpack(bytes);
    nlohmann::json j1 = nlohmann::json::parse(wrapper["MigrationRoot"].get<std::string>());
    
    for (auto& obj : j1["objects"]) {
        if (obj["objectID"] == "object.chess.board") {
            std::cout << "textureResolution: " << (obj.contains("textureResolution") ? obj["textureResolution"].get<int>() : -1) << std::endl;
        }
    }
    return 0;
}
