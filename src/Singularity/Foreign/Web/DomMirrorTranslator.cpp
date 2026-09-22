#include "Singularity/Foreign/Web/DomMirrorTranslator.hpp"
#include <iostream>
#include <algorithm>

namespace Singularity {
namespace Foreign {
namespace Web {

// -----------------------------------------------------------------------------
// ID formatting helpers
// -----------------------------------------------------------------------------
std::string DomMirrorTranslator::makeNodeLexemeId(const std::string& sessionId, const std::string& nodeToken) {
    return sessionId + "." + nodeToken;
}

std::string DomMirrorTranslator::makeAttrLexemeId(const std::string& sessionId, const std::string& nodeToken, size_t attrIndex) {
    return sessionId + ".attr." + nodeToken + "." + std::to_string(attrIndex);
}

std::string DomMirrorTranslator::makeAttrValLexemeId(const std::string& sessionId, const std::string& nodeToken, size_t attrIndex) {
    return sessionId + ".attrval." + nodeToken + "." + std::to_string(attrIndex);
}

std::string DomMirrorTranslator::makeNodeFormationId(const std::string& sessionId, const std::string& nodeToken) {
    return sessionId + "." + nodeToken + ".formation";
}

std::string DomMirrorTranslator::makeDocumentFormationId(const std::string& sessionId) {
    return sessionId + ".doc.formation";
}

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------
DomMirrorTranslator::DomMirrorTranslator() = default;

DomMirrorTranslator::~DomMirrorTranslator() {
    retire();
}

void DomMirrorTranslator::retire() {
    if (_pageSessionId.empty()) return;

    auto& language = Singularity::Language::LanguageSystem::instance();

    // Release all members from Formations so no raw Singular* dangle
    if (_documentFormation) {
        _documentFormation->clear();
        _documentFormation.reset();
    }

    for (auto& [token, form] : _nodeFormations) {
        if (form) {
            form->clear();
        }
    }
    _nodeFormations.clear();
    _nodeRelations.clear();

    // Remove occurrence Lexemes from LanguageSystem
    for (const auto& lex : _allSessionLexemes) {
        if (lex) {
            language.remove("@" + lex->getIdentifier());
        }
    }
    _allSessionLexemes.clear();
    _nodeLexemes.clear();
    _allSessionRelations.clear();
    _confirmedOperations.clear();

    _pageSessionId.clear();
    _url.clear();
    _rootNodeToken.clear();
    _currentSequence = 0;
}

// -----------------------------------------------------------------------------
// Snapshot Admission
// -----------------------------------------------------------------------------
bool DomMirrorTranslator::admitSnapshot(const DomSnapshot& snapshot, std::string* outError) {
    ValidationResult v = snapshot.validate();
    if (!v.valid) {
        if (outError) *outError = "Snapshot validation failed: " + v.error;
        return false;
    }

    // Atomic replacement: retire any previous session before admitting the new one
    retire();

    _pageSessionId = snapshot.pageSessionId;
    _url = snapshot.url;
    _rootNodeToken = snapshot.rootNodeToken;
    _currentSequence = snapshot.sequenceBase;

    auto& language = Singularity::Language::LanguageSystem::instance();

    // 1. Intern all node Lexemes
    for (const auto& node : snapshot.nodes) {
        std::string lexemeId = makeNodeLexemeId(_pageSessionId, node.nodeToken);
        std::string symbol = node.symbol;
        if (node.nodeType == "text" && !node.textContent.empty()) {
            symbol = node.textContent;
        }

        auto lexeme = language.intern(symbol, lexemeId);
        if (!lexeme) {
            if (outError) *outError = "Failed to intern node Lexeme: " + lexemeId;
            retire();
            return false;
        }
        _nodeLexemes[node.nodeToken] = lexeme;
        _allSessionLexemes.push_back(lexeme);
    }

    // 2. Create document Formation
    _documentFormation = std::make_shared<Formation>();
    _documentFormation->setIdentifier(makeDocumentFormationId(_pageSessionId));

    auto rootLexemeIt = _nodeLexemes.find(_rootNodeToken);
    if (rootLexemeIt != _nodeLexemes.end() && rootLexemeIt->second) {
        _documentFormation->setRoot(rootLexemeIt->second.get());
    }

    for (const auto& [token, lexeme] : _nodeLexemes) {
        _documentFormation->addMember(lexeme.get());
    }

    // 3. Register each node, its attributes, and internal relations
    for (const auto& node : snapshot.nodes) {
        registerNode(node);
    }

    // 4. Create structural Relations (parent-child and sibling ordering)
    std::unordered_map<std::string, std::vector<std::string>> childrenByParent;
    for (const auto& node : snapshot.nodes) {
        if (!node.parentToken.empty()) {
            childrenByParent[node.parentToken].push_back(node.nodeToken);

            auto childLexeme = _nodeLexemes[node.nodeToken];
            auto parentLexeme = _nodeLexemes[node.parentToken];
            if (childLexeme && parentLexeme) {
                auto childRel = std::make_shared<Relation>(
                    DomRelationType::kChildOf,
                    *childLexeme,
                    *parentLexeme,
                    true, // directed: child points to parent
                    1.0f  // settled initialWeight
                );
                _documentFormation->addRelation(childRel);
                _allSessionRelations.push_back(childRel);
            }
        }
    }

    // Sibling order relations
    for (const auto& [parentTok, childTokens] : childrenByParent) {
        for (size_t i = 1; i < childTokens.size(); ++i) {
            auto prevLexeme = _nodeLexemes[childTokens[i - 1]];
            auto nextLexeme = _nodeLexemes[childTokens[i]];
            if (prevLexeme && nextLexeme) {
                auto sibRel = std::make_shared<Relation>(
                    DomRelationType::kNextSibling,
                    *prevLexeme,
                    *nextLexeme,
                    true, // directed: prev points to next
                    1.0f  // settled initialWeight
                );
                _documentFormation->addRelation(sibRel);
                _allSessionRelations.push_back(sibRel);
            }
        }
    }

    return true;
}

void DomMirrorTranslator::registerNode(const DomNodeRecord& node) {
    auto nodeLexemeIt = _nodeLexemes.find(node.nodeToken);
    if (nodeLexemeIt == _nodeLexemes.end() || !nodeLexemeIt->second) return;
    auto nodeLexeme = nodeLexemeIt->second;

    auto& language = Singularity::Language::LanguageSystem::instance();

    // Create rooted node formation
    auto nodeForm = std::make_shared<Formation>();
    nodeForm->setIdentifier(makeNodeFormationId(_pageSessionId, node.nodeToken));
    nodeForm->setRoot(nodeLexeme.get());
    nodeForm->addMember(nodeLexeme.get());

    std::vector<std::shared_ptr<Relation>> nodeRels;

    for (size_t i = 0; i < node.attributes.size(); ++i) {
        const auto& attr = node.attributes[i];

        std::string attrId = makeAttrLexemeId(_pageSessionId, node.nodeToken, i);
        std::string valId = makeAttrValLexemeId(_pageSessionId, node.nodeToken, i);

        auto attrLexeme = language.intern(attr.name, attrId);
        auto valLexeme = language.intern(attr.value, valId);

        if (attrLexeme && valLexeme) {
            _allSessionLexemes.push_back(attrLexeme);
            _allSessionLexemes.push_back(valLexeme);

            nodeForm->addMember(attrLexeme.get());
            nodeForm->addMember(valLexeme.get());

            auto hasAttrRel = std::make_shared<Relation>(
                DomRelationType::kHasAttribute,
                *nodeLexeme,
                *attrLexeme,
                true, // directed: node has attribute
                1.0f  // settled initialWeight
            );
            auto hasValRel = std::make_shared<Relation>(
                DomRelationType::kHasValue,
                *attrLexeme,
                *valLexeme,
                true, // directed: attribute has value
                1.0f  // settled initialWeight
            );

            nodeForm->addRelation(hasAttrRel);
            nodeForm->addRelation(hasValRel);

            nodeRels.push_back(hasAttrRel);
            nodeRels.push_back(hasValRel);
            _allSessionRelations.push_back(hasAttrRel);
            _allSessionRelations.push_back(hasValRel);
        }
    }

    _nodeFormations[node.nodeToken] = nodeForm;
    _nodeRelations[node.nodeToken] = nodeRels;
}

void DomMirrorTranslator::unregisterNode(const std::string& nodeToken) {
    auto lexemeIt = _nodeLexemes.find(nodeToken);
    if (lexemeIt != _nodeLexemes.end() && lexemeIt->second) {
        auto lexeme = lexemeIt->second;

        if (_documentFormation) {
            _documentFormation->releaseMember(lexeme.get());
        }

        auto formIt = _nodeFormations.find(nodeToken);
        if (formIt != _nodeFormations.end() && formIt->second) {
            formIt->second->clear();
            _nodeFormations.erase(formIt);
        }

        _nodeRelations.erase(nodeToken);

        Singularity::Language::LanguageSystem::instance().remove("@" + lexeme->getIdentifier());
        _nodeLexemes.erase(lexemeIt);
    }
}

// -----------------------------------------------------------------------------
// Delta Application
// -----------------------------------------------------------------------------
bool DomMirrorTranslator::applyDelta(const DomDelta& delta, std::string* outError) {
    if (_pageSessionId.empty() || delta.pageSessionId != _pageSessionId) {
        if (outError) *outError = "Delta session mismatch: expected '" + _pageSessionId + "', got '" + delta.pageSessionId + "'";
        return false;
    }

    ValidationResult v = delta.validate();
    if (!v.valid) {
        if (outError) *outError = "Delta validation failed: " + v.error;
        return false;
    }

    // Sequence check: deltas must arrive in sequence without gaps
    if (delta.sequence != _currentSequence + 1) {
        if (outError) *outError = "Sequence gap: current sequence is " + std::to_string(_currentSequence) +
                                 ", delta arrived with " + std::to_string(delta.sequence);
        return false;
    }
    _currentSequence = delta.sequence;

    // Operation provenance: record confirmed Earthcall-authored Act
    if (!delta.originOperationId.empty()) {
        _confirmedOperations.push_back(delta.originOperationId);
    }

    auto& language = Singularity::Language::LanguageSystem::instance();

    for (const auto& rec : delta.records) {
        switch (rec.kind) {
            case DomDeltaKind::TextChange: {
                auto lexeme = findNodeLexeme(rec.targetNodeToken);
                if (lexeme) {
                    // Update symbol by removing old binding and re-interning under the same stable ID
                    std::string stableId = lexeme->getIdentifier();
                    language.remove("@" + stableId);
                    auto updated = language.intern(rec.textContent, stableId);
                    _nodeLexemes[rec.targetNodeToken] = updated;
                }
                break;
            }

            case DomDeltaKind::AttributeSet: {
                auto nodeForm = findNodeFormation(rec.targetNodeToken);
                auto nodeLexeme = findNodeLexeme(rec.targetNodeToken);
                if (nodeForm && nodeLexeme) {
                    size_t newIndex = _nodeRelations[rec.targetNodeToken].size() / 2;
                    std::string attrId = makeAttrLexemeId(_pageSessionId, rec.targetNodeToken, newIndex);
                    std::string valId = makeAttrValLexemeId(_pageSessionId, rec.targetNodeToken, newIndex);

                    auto attrLexeme = language.intern(rec.attributeName, attrId);
                    auto valLexeme = language.intern(rec.attributeValue, valId);

                    _allSessionLexemes.push_back(attrLexeme);
                    _allSessionLexemes.push_back(valLexeme);

                    nodeForm->addMember(attrLexeme.get());
                    nodeForm->addMember(valLexeme.get());

                    auto hasAttrRel = std::make_shared<Relation>(
                        DomRelationType::kHasAttribute,
                        *nodeLexeme,
                        *attrLexeme,
                        true,
                        1.0f
                    );
                    auto hasValRel = std::make_shared<Relation>(
                        DomRelationType::kHasValue,
                        *attrLexeme,
                        *valLexeme,
                        true,
                        1.0f
                    );

                    nodeForm->addRelation(hasAttrRel);
                    nodeForm->addRelation(hasValRel);

                    _nodeRelations[rec.targetNodeToken].push_back(hasAttrRel);
                    _nodeRelations[rec.targetNodeToken].push_back(hasValRel);
                    _allSessionRelations.push_back(hasAttrRel);
                    _allSessionRelations.push_back(hasValRel);
                }
                break;
            }

            case DomDeltaKind::AttributeRemove: {
                auto nodeForm = findNodeFormation(rec.targetNodeToken);
                if (nodeForm) {
                    // Search for attribute member matching attributeName
                    Singular* memberToRemove = nullptr;
                    for (Singular* mem : nodeForm->getMembers()) {
                        auto* lex = dynamic_cast<Singularity::Language::Lexeme*>(mem);
                        if (lex && lex->getSymbol() == rec.attributeName) {
                            memberToRemove = lex;
                            break;
                        }
                    }
                    if (memberToRemove) {
                        nodeForm->releaseMember(memberToRemove);
                    }
                }
                break;
            }

            case DomDeltaKind::Insert: {
                if (rec.node.has_value()) {
                    std::string lexemeId = makeNodeLexemeId(_pageSessionId, rec.node->nodeToken);
                    std::string symbol = rec.node->symbol;
                    if (rec.node->nodeType == "text" && !rec.node->textContent.empty()) {
                        symbol = rec.node->textContent;
                    }

                    auto lexeme = language.intern(symbol, lexemeId);
                    _nodeLexemes[rec.node->nodeToken] = lexeme;
                    _allSessionLexemes.push_back(lexeme);

                    if (_documentFormation) {
                        _documentFormation->addMember(lexeme.get());
                    }

                    registerNode(*rec.node);

                    if (!rec.parentToken.empty()) {
                        auto parentLexeme = findNodeLexeme(rec.parentToken);
                        if (parentLexeme) {
                            auto childRel = std::make_shared<Relation>(
                                DomRelationType::kChildOf,
                                *lexeme,
                                *parentLexeme,
                                true,
                                1.0f
                            );
                            if (_documentFormation) {
                                _documentFormation->addRelation(childRel);
                            }
                            _allSessionRelations.push_back(childRel);
                        }
                    }
                }
                break;
            }

            case DomDeltaKind::Remove: {
                unregisterNode(rec.targetNodeToken);
                break;
            }

            case DomDeltaKind::Move: {
                auto nodeLexeme = findNodeLexeme(rec.targetNodeToken);
                auto parentLexeme = findNodeLexeme(rec.parentToken);
                if (nodeLexeme && parentLexeme && _documentFormation) {
                    auto childRel = std::make_shared<Relation>(
                        DomRelationType::kChildOf,
                        *nodeLexeme,
                        *parentLexeme,
                        true,
                        1.0f
                    );
                    _documentFormation->addRelation(childRel);
                    _allSessionRelations.push_back(childRel);
                }
                break;
            }
        }
    }

    return true;
}

// -----------------------------------------------------------------------------
// Query / Lookup
// -----------------------------------------------------------------------------
std::shared_ptr<Singularity::Language::Lexeme> DomMirrorTranslator::findNodeLexeme(const std::string& nodeToken) const {
    auto it = _nodeLexemes.find(nodeToken);
    return (it != _nodeLexemes.end()) ? it->second : nullptr;
}

std::shared_ptr<Formation> DomMirrorTranslator::findNodeFormation(const std::string& nodeToken) const {
    auto it = _nodeFormations.find(nodeToken);
    return (it != _nodeFormations.end()) ? it->second : nullptr;
}

bool DomMirrorTranslator::isOperationConfirmed(const std::string& operationId) const {
    return std::find(_confirmedOperations.begin(), _confirmedOperations.end(), operationId) != _confirmedOperations.end();
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
