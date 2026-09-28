#include <iostream>
#include <fstream>
#include "src/json.hpp"

int main() {
    std::string path = "saves/zones/Chess/zone.json";
    std::ifstream in(path);
    if (!in) return 1;
    nlohmann::json root;
    in >> root;
    in.close();
    
    bool fixed = false;
    for (auto& obj : root["world"]["objects"]) {
        if (obj["objectID"] == "object.chess.board") {
            obj["textureResolution"] = 1024;
            fixed = true;
        }
    }
    
    std::ofstream out(path);
    out << root.dump(2);
    std::cout << (fixed ? "Fixed JSON!" : "Not found") << std::endl;
    return 0;
}
