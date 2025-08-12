#include "HomesOfEarth/Home.hpp"
#include <vector>
#include <string>
#include <iostream>

Home::Home(std::vector<std::string> owners)
    : Zone("Home"), owners(owners) {}

void Home::welcome() const {
    std::cout << "Welcome to " << owners[0] << "'s home." << std::endl;
}
