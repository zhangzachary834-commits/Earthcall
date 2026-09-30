#include <iostream>
#include <fstream>
#include "src/json.hpp"

using json = nlohmann::json;

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    std::ifstream f(argv[1], std::ios::binary);
    if (!f.is_open()) return 1;
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    try {
        json j = json::from_msgpack(data);
        if (j.contains("MigrationRoot")) {
            j = json::parse(j["MigrationRoot"].get<std::string>());
        }
        if (j.contains("world") && j["world"].contains("objects")) {
            for (const auto& obj : j["world"]["objects"]) {
                if (obj.value("shapeKind", "") == "Field") {
                    std::cout << "Found Field in " << argv[1] << "\n";
                    std::cout << "ID: " << obj.value("objectID", "") << "\n";
                    std::cout << "Material: " << obj.value("materialId", "") << "\n";
                    std::cout << "FaceColors:\n" << obj.value("faceColors", json::array()).dump() << "\n";
                    std::string mid = obj.value("materialId", "");
                    if (j.contains("materials")) {
                        for (const auto& m : j["materials"]) {
                            if ("material." + m.value("name", "") == mid) {
                                std::cout << "  Material baseColor: " << m.value("baseColor", json::array()).dump() << "\n";
                                std::cout << "  FaceTextures count: " << (m.contains("faceTextures") ? m["faceTextures"].size() : 0) << "\n";
                            }
                        }
                    }
                }
            }
        }
    } catch (...) {}
    return 0;
}
