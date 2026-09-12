#include <iostream>
#include "json.hpp"
int main() {
    nlohmann::json j1 = nlohmann::json();
    std::cout << "j1 empty=" << j1.empty() << "\n";
    return 0;
}
