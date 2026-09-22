#include "JoyHierarchy.hpp"

#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "Relation/Formation/Formation.hpp"

#include <cstdio>

namespace Singularity {
namespace Language {

void seedJoyHierarchy(Formation& dest, Lexeme* foundation) {
    dest.markJoyHierarchy();

    if (!foundation) {
        dest.clearRoot();
        std::fprintf(stderr,
            "[WARNING] Joy hierarchy seeded with no foundation. "
            "The being has no worship-ordering (HIERARCHY_OF_JOYS.md).\n");
        return;
    }

    dest.addMember(foundation);
    dest.setRoot(foundation);
}

} // namespace Language
} // namespace Singularity
