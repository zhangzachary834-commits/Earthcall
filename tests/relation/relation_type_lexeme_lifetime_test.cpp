// A Lexeme-grounded Relation must not retain a raw type-kind pointer after
// LanguageSystem releases that Lexeme. The Relation itself remains live and
// keeps the stable kind identifier; only the dead grounding pointer must go.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "Singularity/Language/LanguageSystem.hpp"

#include <cstdio>
#include <memory>
#include <string>

namespace {
int g_failures = 0;
void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("  FAILED: %s\n", what.c_str());
    } else {
        std::printf("  ok: %s\n", what.c_str());
    }
}
}

int main() {
    using Singularity::Language::LanguageSystem;

    auto& language = LanguageSystem::instance();
    language.clear();

    Object a;
    Object b;
    a.setObjectID("type-lifetime-a");
    b.setObjectID("type-lifetime-b");

    RelationManager relations;
    auto typeLexeme =
        language.intern("lifetime-kind", "lexeme.test.lifetime-kind");
    std::weak_ptr<Singularity::Language::Lexeme> typeLifetime = typeLexeme;

    auto relation =
        std::make_shared<Relation>(*typeLexeme, a, b, true);
    relations.add(relation);

    check(relation->hasGroundedType(),
          "Relation begins grounded in its live type Lexeme");
    check(relation->type == "lexeme.test.lifetime-kind",
          "Relation stores the type Lexeme's stable identity");

    typeLexeme.reset();
    language.remove("lexeme.test.lifetime-kind");

    check(typeLifetime.expired(),
          "LanguageSystem removal released the type Lexeme owner");
    check(relations.getAll().size() == 1,
          "Relation itself remains live after its kind-being is removed");
    check(!relation->hasGroundedType(),
          "live Relation no longer retains the released Lexeme pointer");
    check(relation->type == "lexeme.test.lifetime-kind",
          "ungrounding preserves stable Relation-kind identity");

    language.clear();

    std::printf("%s\n",
                g_failures ? "relation_type_lexeme_lifetime_test: FAILURES"
                           : "relation_type_lexeme_lifetime_test: OK");
    return g_failures ? 1 : 0;
}
