#pragma once

#include "Singularity/Foreign/Web/DomMirrorProtocol.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Relation/Relation.hpp"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace Singularity {
namespace Foreign {
namespace Web {

// Relation type vocabulary (authored strings, not C++ enums)
namespace DomRelationType {
    constexpr const char* kChildOf         = "dom-child-of";
    constexpr const char* kNextSibling     = "dom-next-sibling";
    constexpr const char* kHasAttribute    = "dom-has-attribute";
    constexpr const char* kHasValue        = "dom-has-value";
    constexpr const char* kShadowRootOf    = "dom-shadow-root-of";
    constexpr const char* kFrameBoundaryOf = "dom-frame-boundary-of";
}

// DomMirrorTranslator translates between the wire DomMirrorProtocol
// and Earthcall native Lexemes, Relations, and Formations.
class DomMirrorTranslator {
public:
    DomMirrorTranslator();
    ~DomMirrorTranslator();

    // Prevent accidental copying to preserve identity boundaries
    DomMirrorTranslator(const DomMirrorTranslator&) = delete;
    DomMirrorTranslator& operator=(const DomMirrorTranslator&) = delete;

    // Translate a coherent snapshot into native Lexemes, Relations, and Formations.
    // Retires any existing session before admitting a new one.
    bool admitSnapshot(const DomSnapshot& snapshot, std::string* outError = nullptr);

    // Apply incremental mutation edges to the live mirror
    bool applyDelta(const DomDelta& delta, std::string* outError = nullptr);

    // Atomic retirement of current document mirror (e.g. on navigation)
    void retire();

    // Accessors
    bool hasActiveSession() const { return !_pageSessionId.empty(); }
    const std::string& getPageSessionId() const { return _pageSessionId; }
    const std::string& getUrl() const { return _url; }
    uint64_t getCurrentSequence() const { return _currentSequence; }

    // Lookups by DOM node token (e.g. "node.137")
    std::shared_ptr<Singularity::Language::Lexeme> findNodeLexeme(const std::string& nodeToken) const;
    std::shared_ptr<Formation> findNodeFormation(const std::string& nodeToken) const;

    // Document-level Formation gathering the entire live document
    std::shared_ptr<Formation> getDocumentFormation() const { return _documentFormation; }

    // Check if an operation ID was confirmed by a delta
    bool isOperationConfirmed(const std::string& operationId) const;
    const std::vector<std::string>& getConfirmedOperations() const { return _confirmedOperations; }

    // Stable ID generators
    static std::string makeNodeLexemeId(const std::string& sessionId, const std::string& nodeToken);
    static std::string makeAttrLexemeId(const std::string& sessionId, const std::string& nodeToken, size_t attrIndex);
    static std::string makeAttrValLexemeId(const std::string& sessionId, const std::string& nodeToken, size_t attrIndex);
    static std::string makeNodeFormationId(const std::string& sessionId, const std::string& nodeToken);
    static std::string makeDocumentFormationId(const std::string& sessionId);

private:
    std::string _pageSessionId;
    std::string _url;
    std::string _rootNodeToken;
    uint64_t _currentSequence = 0;

    // Native graph storage for the active session
    std::shared_ptr<Formation> _documentFormation;
    std::unordered_map<std::string, std::shared_ptr<Singularity::Language::Lexeme>> _nodeLexemes;
    std::unordered_map<std::string, std::shared_ptr<Formation>> _nodeFormations;
    std::unordered_map<std::string, std::vector<std::shared_ptr<Relation>>> _nodeRelations;
    std::vector<std::shared_ptr<Singularity::Language::Lexeme>> _allSessionLexemes;
    std::vector<std::shared_ptr<Relation>> _allSessionRelations;
    std::vector<std::string> _confirmedOperations;

    // Internal helpers
    void registerNode(const DomNodeRecord& node);
    void unregisterNode(const std::string& nodeToken);
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
