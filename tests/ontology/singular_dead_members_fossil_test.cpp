#include "ConstructedBeing/Singular/Singular.hpp"
#include "Person/Person.hpp"

#include <iostream>
#include <vector>

// The removed Formation vectors were PROTECTED, not public. A probe using
// declval<T>().member outside a derived context returns false even when those
// members still exist, making the original witness falsely green.
template <typename T>
struct ProtectedFormationProbe : T {
    static constexpr bool hasParent = requires(ProtectedFormationProbe& probe) {
        probe.parentFormationInstances;
    };
    static constexpr bool hasChild = requires(ProtectedFormationProbe& probe) {
        probe.childFormationInstances;
    };
};

template <typename T>
constexpr bool hasKernelBounds = requires(const T& instance) {
    instance.satisfiesKernelBounds();
};

// Positive controls ensure the probes do detect protected inherited members.
struct FossilPositiveControl {
protected:
    std::vector<int> parentFormationInstances;
    std::vector<int> childFormationInstances;
public:
    bool satisfiesKernelBounds() const { return true; }
};

static_assert(ProtectedFormationProbe<FossilPositiveControl>::hasParent);
static_assert(ProtectedFormationProbe<FossilPositiveControl>::hasChild);
static_assert(hasKernelBounds<FossilPositiveControl>);

// The actual structural contract, including the Person inheritance path.
static_assert(!ProtectedFormationProbe<Singular>::hasParent);
static_assert(!ProtectedFormationProbe<Singular>::hasChild);
static_assert(!ProtectedFormationProbe<Person>::hasParent);
static_assert(!ProtectedFormationProbe<Person>::hasChild);
static_assert(!hasKernelBounds<Singular>);
static_assert(!hasKernelBounds<Person>);

int main() {
    std::cout << "singular_dead_members_fossil_test: protected Formation fossils and "
                 "unused Kernel-bounds placeholder absent; positive controls passed\n";
    return 0;
}
