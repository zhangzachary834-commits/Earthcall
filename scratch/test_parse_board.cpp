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
        if (j.contains("materials")) {
            for (const auto& m : j["materials"]) {
                if (m.value("name", "") == "board") {
                    std::cout << "Board material found\n";
                    if (m.contains("faceTextures")) {
                        for (size_t i = 0; i < m["faceTextures"].size(); ++i) {
                            auto& ft = m["faceTextures"][i];
                            std::cout << "  Face " << i << ": width=" << ft.value("width", -1) 
                                      << " height=" << ft.value("height", -1) 
                                      << " b64_length=" << ft.value("pixels_b64", "").size() << "\n";
                        }
                    }
                }
            }
        }
    } catch (...) {}
    return 0;
}
