// Phase 3: the Person root codec preserves the profile schema while making
// Person a first-class session serialization root.

#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Storage/Serialization/Person/PersonSerialization.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

Person makePerson(const char* name) {
    Person person(Soul(name), Body("Humanoid", "Voxel"), "default");
    person.setDisplayName(name);
    return person;
}

bool near(float a, float b) {
    return std::fabs(a - b) < 0.0001f;
}

} // namespace

int main() {
    Person original = makePerson("Phase 3 Person");
    original.position() = {1.25f, -2.5f, 3.75f};
    original.velocity() = {-4.0f, 5.5f, 6.25f};

    const nlohmann::json saved = personToJson(original);
    assert(saved.contains("displayName"));
    assert(saved.contains("soulName"));
    assert(saved.contains("position"));
    assert(saved.contains("velocity"));
    assert(saved.contains("body"));
    assert(original.serialize() == saved);

    Person restored = makePerson("Temporary");
    personFromJson(saved, restored);
    assert(restored.getDisplayName() == "Phase 3 Person");
    assert(near(restored.position().x, 1.25f));
    assert(near(restored.position().y, -2.5f));
    assert(near(restored.position().z, 3.75f));
    assert(near(restored.velocity().x, -4.0f));
    assert(near(restored.velocity().y, 5.5f));
    assert(near(restored.velocity().z, 6.25f));

    std::puts("person_serialization_test: ALL OK");
    return 0;
}
