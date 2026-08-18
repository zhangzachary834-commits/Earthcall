#include "JoyHierarchy.hpp"

#include "Singularity/Language/LanguageSystem.hpp"
#include "ConstructedBeing/Object/Formation/Formation.hpp"
#include "Relation/Relation.hpp"

#include <cstdio>
#include <memory>

namespace Singularity {
namespace Language {

void seedJoyHierarchy(Formation& dest, const std::string& foundationSymbol) {
    dest.markJoyHierarchy();

    if (foundationSymbol.empty()) {
        dest.clearRoot();
        std::fprintf(stderr,
            "[WARNING] Joy hierarchy seeded with no foundation. "
            "The being has no worship-ordering (HIERARCHY_OF_JOYS.md).\n");
        return;
    }

    std::shared_ptr<Lexeme> root;
    if (foundationSymbol == "default" || foundationSymbol == "strict") {
        root = LanguageSystem::instance().foundation();
    } else {
        root = LanguageSystem::instance().resolve(foundationSymbol);
    }
    if (!root) return;

    dest.addMember(root.get());
    dest.setRoot(root.get());
}

} // namespace Language
} // namespace Singularity
