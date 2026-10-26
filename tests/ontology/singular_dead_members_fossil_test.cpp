#include "ConstructedBeing/Singular/Singular.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"

#include <cassert>
#include <iostream>
#include <type_traits>

namespace {

// SFINAE checks to detect if parentFormationInstances or childFormationInstances exist on Singular
template <typename T, typename = void>
struct has_parent_formation_instances : std::false_type {};

template <typename T>
struct has_parent_formation_instances<T, std::void_t<decltype(std::declval<T>().parentFormationInstances)>> : std::true_type {};

template <typename T, typename = void>
struct has_child_formation_instances : std::false_type {};

template <typename T>
struct has_child_formation_instances<T, std::void_t<decltype(std::declval<T>().childFormationInstances)>> : std::true_type {};

// SFINAE check to detect if satisfiesKernelBounds exists on Singular
template <typename T, typename = void>
struct has_satisfies_kernel_bounds : std::false_type {};

template <typename T>
struct has_satisfies_kernel_bounds<T, std::void_t<decltype(std::declval<T>().satisfiesKernelBounds())>> : std::true_type {};

} // namespace

int main() {
    static_assert(!has_parent_formation_instances<Singular>::value,
                  "Singular must not contain dead parentFormationInstances vector");
    static_assert(!has_child_formation_instances<Singular>::value,
                  "Singular must not contain dead childFormationInstances vector");
    static_assert(!has_satisfies_kernel_bounds<Singular>::value,
                  "Singular must not contain placeholder satisfiesKernelBounds() method");

    // Runtime sanity check on derived Singulars
    Body body("Humanoid", "Voxel");
    Person person(Soul("TestUser"), body, "");

    assert(!has_parent_formation_instances<Person>::value);
    assert(!has_child_formation_instances<Person>::value);
    assert(!has_satisfies_kernel_bounds<Person>::value);

    std::cout << "singular_dead_members_fossil_test: Singular is clean of dead formation vectors and placeholder satisfiesKernelBounds\n";
    return 0;
}
