#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>

int main() {
    std::string line = "formation \"root_123\" \"member_1\" \"member_2\"";
    std::stringstream ss(line);
    std::string type, rootId;
    ss >> type >> std::quoted(rootId);
    std::string member;
    while (ss >> std::quoted(member)) {
        std::cout << "Member: " << member << "\n";
    }
}
