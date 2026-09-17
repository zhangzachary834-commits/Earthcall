#include <iostream>
#include <variant>

using PropertyValue = std::variant<double, bool>;

void test(const PropertyValue& in, PropertyValue& out) {
    out = in;
}

int main() {
    PropertyValue v = 1.0;
    test(v, v);
    if (std::holds_alternative<double>(v)) {
        std::cout << "OK, holds double: " << std::get<double>(v) << std::endl;
    } else {
        std::cout << "FAILED, empty or something else!" << std::endl;
    }
    return 0;
}
