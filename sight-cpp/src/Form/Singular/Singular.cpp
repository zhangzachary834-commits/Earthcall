#include "Form/Singular/Singular.hpp"

Formation* Singular::singular_properties() {
    return _property_formation;
}

const Formation* Singular::singular_properties() const {
    return _property_formation;
}
