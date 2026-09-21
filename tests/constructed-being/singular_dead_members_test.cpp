#include "ConstructedBeing/Singular/Singular.hpp"
#include <iostream>

class TestSingular : public Singular {
public:
    TestSingular(std::string id = "test-singular") : _id(id) {}
    std::string getIdentifier() const override { return _id; }
protected:
    void buildProperties() override {}
private:
    std::string _id;
};

int main() {
    std::cout << "Running Singular dead members cleanup verification test...\n";

    TestSingular s1("singular-1");
    if (s1.getIdentifier() != "singular-1") {
        std::cerr << "FAIL: getIdentifier mismatch\n";
        return 1;
    }

    s1.setDynamicProperty("custom_prop", 42.0);
    PropertyValue val;
    if (!s1.getDynamicProperty("custom_prop", val) || std::get<double>(val) != 42.0) {
        std::cerr << "FAIL: dynamic property get/set failed\n";
        return 1;
    }

    TestSingular s2 = s1;
    PropertyValue val2;
    if (!s2.getDynamicProperty("custom_prop", val2) || std::get<double>(val2) != 42.0) {
        std::cerr << "FAIL: copy constructor failed to preserve properties\n";
        return 1;
    }

    TestSingular s3 = std::move(s2);
    PropertyValue val3;
    if (!s3.getDynamicProperty("custom_prop", val3) || std::get<double>(val3) != 42.0) {
        std::cerr << "FAIL: move constructor failed to preserve properties\n";
        return 1;
    }

    std::cout << "All Singular dead members cleanup tests passed.\n";
    return 0;
}
