#include "Soul.hpp"
#include <string>

Soul::Soul(std::string identity) : _identity(identity) {
    // Constructor
}

Soul::~Soul() {
    // Destructor
}

std::string Soul::getIdentifier() const {
    return _identity;
}