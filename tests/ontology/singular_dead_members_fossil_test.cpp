#include "ConstructedBeing/Singular/Singular.hpp"
#include "Person/Person.hpp"

#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

// The removed Formation vectors were protected. An outside-class expression
// cannot distinguish an inaccessible inherited member from a missing member.
// Probe through a derived class, using the project's C++17 dialect.
template <typename T>
struct ProtectedFormationProbe : T {
private:
    template <typename U>
    static auto detectParent(int) -> decltype(
        (void)std::declval<U&>().parentFormationInstances, std::true_type{});
    template <typename>
    static auto detectParent(...) -> std::false_type;

    template <typename U>
    static auto detectChild(int) -> decltype(
        (void)std::declval<U&>().childFormationInstances, std::true_type{});
    template <typename>
    static auto detectChild(...) -> std::false_type;

public:
    static constexpr bool hasParent =
        decltype(detectParent<ProtectedFormationProbe>(0))::value;
    static constexpr bool hasChild =
        decltype(detectChild<ProtectedFormationProbe>(0))::value;
};

template <typename T, typename = void>
struct HasKernelBounds : std::false_type {};

template <typename T>
struct HasKernelBounds<T, std::void_t<
    decltype(std::declval<const T&>().satisfiesKernelBounds())>>
    : std::true_type {};

// Positive controls prove the probes detect protected inherited members.
struct FossilPositiveControl {
protected:
    std::vector<int> parentFormationInstances;
    std::vector<int> childFormationInstances;
public:
    bool satisfiesKernelBounds() const { return true; }
};

static_assert(ProtectedFormationProbe<FossilPositiveControl>::hasParent);
static_assert(ProtectedFormationProbe<FossilPositiveControl>::hasChild);
static_assert(HasKernelBounds<FossilPositiveControl>::value);

// The actual structural contract, including the Person inheritance path.
static_assert(!ProtectedFormationProbe<Singular>::hasParent);
static_assert(!ProtectedFormationProbe<Singular>::hasChild);
static_assert(!ProtectedFormationProbe<Person>::hasParent);
static_assert(!ProtectedFormationProbe<Person>::hasChild);
static_assert(!HasKernelBounds<Singular>::value);
static_assert(!HasKernelBounds<Person>::value);

int main() {
    std::cout << "singular_dead_members_fossil_test: protected Formation fossils and "
                 "unused Kernel-bounds placeholder absent; positive controls passed\n";
    return 0;
}
