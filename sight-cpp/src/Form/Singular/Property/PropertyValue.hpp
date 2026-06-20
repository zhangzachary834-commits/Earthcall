#include <variant>
#include <string>

class Singular;
class Object;
class Relation;
class Formation;

struct Vec3 {
    float x;
    float y;
    float z;
};

using PropertyValue = std::variant<
    int,
    float,
    double,
    bool,
    std::string,
    Vec3,
    Singular*,
    Object*,
    Relation*,
    Formation*
>;