#include <string>
#include <sstream>
#include <iostream>
#include "json.hpp"

// Minimalist ECGraph Parser
nlohmann::json ecgraphToJson(const std::string& input) {
    nlohmann::json root = nlohmann::json::array();
    std::istringstream iss(input);
    std::string line;
    nlohmann::json currentObj;
    
    while (std::getline(iss, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        if (line[0] == '[') {
            if (!currentObj.empty()) {
                root.push_back(currentObj);
                currentObj.clear();
            }
            // Parse [Singular "id"] or [Relation "id"]
            size_t space = line.find(' ');
            size_t end = line.find(']');
            if (space != std::string::npos && end != std::string::npos) {
                std::string type = line.substr(1, space - 1);
                std::string id = line.substr(space + 2, end - space - 3);
                currentObj["type"] = type;
                currentObj["id"] = id;
            }
        } else {
            // Parse key = value
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                std::string val = line.substr(eq + 1);
                // trim
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                val.erase(0, val.find_first_not_of(" \t"));
                val.erase(val.find_last_not_of(" \t") + 1);
                currentObj[key] = val;
            }
        }
    }
    if (!currentObj.empty()) {
        root.push_back(currentObj);
    }
    return root;
}
