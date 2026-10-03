#include "Singularity/Foreign/Web/HtmlFormationActPlanner.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <unordered_set>

namespace Singularity {
namespace Foreign {
namespace Web {

namespace {

using Singularity::Language::Lexeme;

bool readString(const Singular& being, const char* name, std::string& out) {
    PropertyValue value;
    if (!being.getDynamicProperty(name, value)) return false;
    const auto* text = std::get_if<std::string>(&value);
    if (!text) return false;
    out = *text;
    return true;
}

} // namespace

bool HtmlFormationActPlanner::sameKind(
    const Relation& relation,
    const Lexeme* kind) {
    return kind && relation.getTypeLexeme() == kind;
}

HtmlFormationActPlanner::Ready HtmlFormationActPlanner::readyInsertions(
    const Request& request,
    const Bindings& bindings) {

    Ready out;
    if (!request.desired) {
        out.refusal = "desired HTML Formation is missing";
        return out;
    }
    if (!request.kinds.childOf || !request.kinds.nextSibling) {
        out.refusal = "exact dom-child-of and dom-next-sibling kind Lexemes are required";
        return out;
    }
    if (request.pageSessionId.empty()) {
        out.refusal = "page session identity is empty";
        return out;
    }
    if (request.operationPrefix.empty()) {
        out.refusal = "operation prefix is empty";
        return out;
    }
    if (request.rootSiblingIndex < 0) {
        out.refusal = "root sibling index cannot be negative";
        return out;
    }

    auto* root = dynamic_cast<Lexeme*>(request.desired->root());
    if (!root) {
        out.refusal = "desired HTML Formation needs a Lexeme root";
        return out;
    }

    std::map<std::string, Lexeme*> nodes;
    for (Singular* member : request.desired->getMembers()) {
        auto* lexeme = dynamic_cast<Lexeme*>(member);
        if (!lexeme) {
            out.refusal = "HTML insertion planner only admits Lexeme occurrences";
            return out;
        }
        if (!nodes.emplace(lexeme->getIdentifier(), lexeme).second) {
            out.refusal = "duplicate desired occurrence identity: " + lexeme->getIdentifier();
            return out;
        }

        std::string nodeType;
        if (!readString(*lexeme, "html.nodeType", nodeType)) {
            out.refusal = "desired occurrence lacks html.nodeType: " + lexeme->getIdentifier();
            return out;
        }
        if (nodeType != "element" && nodeType != "text") {
            out.refusal = "unsupported html.nodeType '" + nodeType +
                          "' on " + lexeme->getIdentifier();
            return out;
        }
    }

    if (nodes.find(root->getIdentifier()) == nodes.end()) {
        out.refusal = "Formation root is not one of its desired occurrences";
        return out;
    }

    std::unordered_map<std::string, std::string> parentOf;
    std::unordered_map<std::string, std::vector<std::string>> childrenOf;
    std::vector<const Relation*> siblingEdges;

    for (const auto& relation : request.desired->relations().getAll()) {
        if (!relation || !relation->hasEndpoints()) continue;

        if (sameKind(*relation, request.kinds.childOf)) {
            if (!relation->directed) {
                out.refusal = "dom-child-of must be directed child -> parent";
                return out;
            }
            const std::string child = relation->aId();
            const std::string parent = relation->bId();
            if (!nodes.count(child) || !nodes.count(parent)) {
                out.refusal = "dom-child-of names an occurrence outside the desired Formation";
                return out;
            }
            if (parentOf.count(child)) {
                out.refusal = "desired occurrence has more than one DOM parent: " + child;
                return out;
            }
            parentOf[child] = parent;
            childrenOf[parent].push_back(child);
        } else if (sameKind(*relation, request.kinds.nextSibling)) {
            if (!relation->directed) {
                out.refusal = "dom-next-sibling must be directed previous -> next";
                return out;
            }
            siblingEdges.push_back(relation.get());
        }
    }

    if (parentOf.count(root->getIdentifier())) {
        out.refusal = "desired HTML root cannot also be a child";
        return out;
    }
    for (const auto& [id, node] : nodes) {
        (void)node;
        if (id == root->getIdentifier()) continue;
        if (!parentOf.count(id)) {
            out.refusal = "desired HTML occurrence is disconnected from root: " + id;
            return out;
        }
    }

    // Exact sibling positions. A multi-child parent must carry one unambiguous
    // next-sibling chain; vector/member insertion order is not semantic order.
    std::unordered_map<std::string, int> siblingIndex;
    for (auto& [parent, children] : childrenOf) {
        if (children.size() == 1) {
            siblingIndex[children.front()] = 0;
            continue;
        }

        std::unordered_set<std::string> childSet(children.begin(), children.end());
        std::unordered_map<std::string, std::string> next;
        std::unordered_map<std::string, int> indegree;
        for (const auto& child : children) indegree[child] = 0;

        std::size_t usedEdges = 0;
        for (const Relation* relation : siblingEdges) {
            if (!relation) continue;
            const std::string a = relation->aId();
            const std::string b = relation->bId();
            if (!childSet.count(a) || !childSet.count(b)) continue;
            if (next.count(a) || ++indegree[b] > 1) {
                out.refusal = "dom-next-sibling is not a single chain under parent " + parent;
                return out;
            }
            next[a] = b;
            ++usedEdges;
        }

        if (usedEdges != children.size() - 1) {
            out.refusal = "multiple DOM children require a complete dom-next-sibling chain under " + parent;
            return out;
        }

        std::string head;
        for (const auto& child : children) {
            if (indegree[child] == 0) {
                if (!head.empty()) {
                    out.refusal = "dom-next-sibling has multiple heads under parent " + parent;
                    return out;
                }
                head = child;
            }
        }
        if (head.empty()) {
            out.refusal = "dom-next-sibling chain has no head under parent " + parent;
            return out;
        }

        std::unordered_set<std::string> visited;
        std::string current = head;
        int index = 0;
        while (!current.empty()) {
            if (!visited.insert(current).second) {
                out.refusal = "dom-next-sibling cycle under parent " + parent;
                return out;
            }
            siblingIndex[current] = index++;
            const auto it = next.find(current);
            current = it == next.end() ? std::string{} : it->second;
        }
        if (visited.size() != children.size()) {
            out.refusal = "dom-next-sibling chain does not cover every child under " + parent;
            return out;
        }
    }

    struct Candidate {
        std::string desiredId;
        std::string parentToken;
        int index = 0;
        Lexeme* node = nullptr;
    };
    std::vector<Candidate> ready;

    for (const auto& [id, node] : nodes) {
        if (bindings.count(id)) continue;

        std::string parentToken;
        int index = 0;
        if (id == root->getIdentifier()) {
            parentToken = request.externalParentToken;
            index = request.rootSiblingIndex;
        } else {
            const auto parentIt = parentOf.find(id);
            if (parentIt == parentOf.end()) continue;
            const auto boundParent = bindings.find(parentIt->second);
            if (boundParent == bindings.end()) continue;
            parentToken = boundParent->second;
            index = siblingIndex[id];
        }

        if (parentToken.empty()) {
            if (id == root->getIdentifier()) {
                out.refusal = "unbound desired root needs an external browser parent token";
                return out;
            }
            continue;
        }
        ready.push_back({id, parentToken, index, node});
    }

    std::sort(ready.begin(), ready.end(), [](const Candidate& a, const Candidate& b) {
        if (a.parentToken != b.parentToken) return a.parentToken < b.parentToken;
        if (a.index != b.index) return a.index < b.index;
        return a.desiredId < b.desiredId;
    });

    for (const auto& candidate : ready) {
        std::string nodeType;
        readString(*candidate.node, "html.nodeType", nodeType);

        DomAct act;
        act.protocolVersion = kDomProtocolVersion;
        act.pageSessionId = request.pageSessionId;
        act.operationId = request.operationPrefix + "." + candidate.desiredId;
        act.parentToken = candidate.parentToken;
        act.siblingIndex = candidate.index;

        if (nodeType == "element") {
            act.kind = DomActKind::InsertElement;
            act.tagName = candidate.node->getSymbol();
        } else {
            act.kind = DomActKind::InsertText;
            std::string authoredText;
            act.text = readString(*candidate.node, "html.text", authoredText)
                           ? authoredText
                           : candidate.node->getSymbol();
        }

        const ValidationResult validation = act.validate();
        if (!validation.valid) {
            out.refusal = "planned DOM Act is invalid: " + validation.error;
            out.acts.clear();
            return out;
        }
        out.acts.push_back({candidate.desiredId, std::move(act)});
    }

    out.complete = bindings.size() >= nodes.size();
    out.ok = true;
    return out;
}

bool HtmlFormationActPlanner::confirmInsertion(
    const PlannedAct& planned,
    const DomDelta& confirmation,
    Bindings& bindings,
    std::string* outError) {

    if (planned.desiredId.empty()) {
        if (outError) *outError = "planned desired identity is empty";
        return false;
    }
    if (planned.act.kind != DomActKind::InsertElement &&
        planned.act.kind != DomActKind::InsertText) {
        if (outError) *outError = "planned Act is not an insertion";
        return false;
    }
    if (confirmation.pageSessionId != planned.act.pageSessionId) {
        if (outError) *outError = "confirmation page session does not match planned Act";
        return false;
    }
    if (confirmation.originOperationId != planned.act.operationId) {
        if (outError) *outError = "confirmation operation ID does not match planned Act";
        return false;
    }

    std::string matchedToken;
    for (const auto& record : confirmation.records) {
        if (record.kind != DomDeltaKind::Insert || !record.node) continue;
        if (record.parentToken != planned.act.parentToken) continue;
        if (record.siblingIndex != planned.act.siblingIndex) continue;

        const DomNodeRecord& node = *record.node;
        const bool matches =
            planned.act.kind == DomActKind::InsertElement
                ? (node.nodeType == "element" && node.symbol == planned.act.tagName)
                : (node.nodeType == "text" && node.textContent == planned.act.text);
        if (!matches) continue;

        if (!matchedToken.empty()) {
            if (outError) *outError = "confirmation contains multiple matching newborn nodes";
            return false;
        }
        matchedToken = record.targetNodeToken.empty()
                           ? node.nodeToken
                           : record.targetNodeToken;
    }

    if (matchedToken.empty()) {
        if (outError) *outError = "confirmation contains no matching newborn node";
        return false;
    }

    const auto existing = bindings.find(planned.desiredId);
    if (existing != bindings.end() && existing->second != matchedToken) {
        if (outError) *outError =
            "desired occurrence is already bound to a different browser token";
        return false;
    }
    bindings[planned.desiredId] = matchedToken;
    return true;
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
