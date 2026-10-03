#pragma once

#include "Singularity/Foreign/Web/DomMirrorProtocol.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "Relation/Formation/Formation.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace Singularity {
namespace Foreign {
namespace Web {

// Compiles one already-authored HTML Lexeme Formation into the INSERT Acts
// that are currently justified by known browser identity.
//
// It does not issue Acts, mint browser tokens, infer HTML from semantic source
// beings, or guess parent identity. Confirmation deltas extend the binding
// table; calling readyInsertions again then reveals the next dependency layer.
class HtmlFormationActPlanner {
public:
    using Bindings = std::unordered_map<std::string, std::string>;

    struct Kinds {
        Singularity::Language::Lexeme* childOf = nullptr;
        Singularity::Language::Lexeme* nextSibling = nullptr;
    };

    struct Request {
        const Formation* desired = nullptr;
        Kinds kinds;

        std::string pageSessionId;
        std::string operationPrefix;

        // Where an unbound desired root should be born in the already-live
        // browser graph. The root itself has no browser token yet.
        std::string externalParentToken;
        int rootSiblingIndex = 0;
    };

    struct PlannedAct {
        std::string desiredId;
        DomAct act;
    };

    struct Ready {
        bool ok = false;
        std::string refusal;
        std::vector<PlannedAct> acts;
        bool complete = false;
        explicit operator bool() const { return ok; }
    };

    static Ready readyInsertions(
        const Request& request,
        const Bindings& bindings);

    // Accept only the confirmation for this exact planned insertion and bind
    // the desired occurrence to the browser-minted node token.
    static bool confirmInsertion(
        const PlannedAct& planned,
        const DomDelta& confirmation,
        Bindings& bindings,
        std::string* outError = nullptr);

private:
    static bool sameKind(
        const Relation& relation,
        const Singularity::Language::Lexeme* kind);
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
