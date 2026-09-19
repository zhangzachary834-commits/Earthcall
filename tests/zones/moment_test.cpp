#include "Time/Moment/Moment.hpp"
#include <cassert>
#include <iostream>
#include <sstream>
#include "json.hpp"

using json = nlohmann::json;

int main() {
    std::cout << "Testing Moment class...\n";

    // Test default constructor
    Moment defaultMoment;
    assert(defaultMoment.isInstant());
    assert(defaultMoment.asSeconds() == 0.0);
    assert(defaultMoment.propKind() == static_cast<int>(Moment::Kind::Instant));

    // Test double constructor
    Moment instantMoment(10.5);
    assert(instantMoment.isInstant());
    assert(instantMoment.asSeconds() == 10.5);
    assert(!instantMoment.isInterval());
    assert(instantMoment.propStart() == 10.5);
    assert(instantMoment.propEnd() == 10.5);

    // Test getIdentifier
    assert(instantMoment.getIdentifier() == "moment.10.5");

    // Test interval
    Moment intervalMoment = Moment::interval(5.0, 15.0);
    assert(intervalMoment.isInterval());
    assert(intervalMoment.asSeconds() == 5.0);
    assert(intervalMoment.endSeconds().has_value());
    assert(intervalMoment.endSeconds().value() == 15.0);
    assert(intervalMoment.propStart() == 5.0);
    assert(intervalMoment.propEnd() == 15.0);

    // Test getIdentifier for interval
    assert(intervalMoment.getIdentifier() == "moment.5-15");

    // Test setStart and setEnd
    Moment m2(2.0);
    m2.setStart(4.0);
    assert(m2.propStart() == 4.0);
    assert(m2.propEnd() == 4.0);
    m2.setEnd(8.0);
    assert(m2.propStart() == 4.0);
    assert(m2.propEnd() == 8.0);
    assert(m2.isInterval());

    // Test property setting
    intervalMoment.setKind(static_cast<int>(Moment::Kind::Instant));
    assert(intervalMoment.isInstant());
    assert(intervalMoment.asSeconds() == 5.0); // Should be start

    // Test operator<<
    std::stringstream ss;
    ss << instantMoment;
    assert(ss.str() == "10.5");

    std::stringstream ss2;
    ss2 << Moment::interval(1.2, 3.4);
    assert(ss2.str() == "1.2..3.4");

    // Test toJson
    json j1 = instantMoment.toJson();
    assert(j1["kind"] == static_cast<int>(Moment::Kind::Instant));
    assert(j1["start"] == 10.5);
    assert(!j1.contains("end"));

    Moment i2 = Moment::interval(1.5, 2.5);
    json j2 = i2.toJson();
    assert(j2["kind"] == static_cast<int>(Moment::Kind::Interval));
    assert(j2["start"] == 1.5);
    assert(j2["end"] == 2.5);

    // Test to_json free function
    json j3;
    to_json(j3, i2);
    assert(j3 == j2);

    std::cout << "Moment tests passed.\n";
    return 0;
}
