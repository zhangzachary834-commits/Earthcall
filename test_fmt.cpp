#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

int main() {
    std::string line = "singular \"entity 123\" \"@person\" \"@owner\" \"@zone.home\" 0";
    std::stringstream ss(line);
    std::string type, entityId, kindLex, ownerLex, zoneLex;
    uint32_t flags;
    
    ss >> type;
    ss >> std::quoted(entityId) >> std::quoted(kindLex) >> std::quoted(ownerLex) >> std::quoted(zoneLex) >> flags;
    std::cout << type << " | " << entityId << " | " << kindLex << "\n";
}
