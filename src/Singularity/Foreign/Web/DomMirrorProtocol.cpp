#include "Singularity/Foreign/Web/DomMirrorProtocol.hpp"
#include <algorithm>
#include <cctype>

namespace Singularity {
namespace Foreign {
namespace Web {

// -----------------------------------------------------------------------------
// DomAttributeRecord
// -----------------------------------------------------------------------------
nlohmann::json DomAttributeRecord::toJson() const {
    return nlohmann::json{
        {"name", name},
        {"value", value}
    };
}

DomAttributeRecord DomAttributeRecord::fromJson(const nlohmann::json& j) {
    DomAttributeRecord r;
    r.name = j.value("name", "");
    r.value = j.value("value", "");
    return r;
}

// -----------------------------------------------------------------------------
// DomNodeRecord
// -----------------------------------------------------------------------------
nlohmann::json DomNodeRecord::toJson() const {
    nlohmann::json j{
        {"nodeToken", nodeToken},
        {"nodeType", nodeType},
        {"symbol", symbol},
        {"parentToken", parentToken},
        {"siblingIndex", siblingIndex}
    };
    if (!textContent.empty()) {
        j["textContent"] = textContent;
    }
    if (!namespaceUri.empty()) {
        j["namespaceUri"] = namespaceUri;
    }
    if (isShadowBoundary) {
        j["isShadowBoundary"] = true;
    }
    if (isFrameBoundary) {
        j["isFrameBoundary"] = true;
    }
    nlohmann::json attrArray = nlohmann::json::array();
    for (const auto& a : attributes) {
        attrArray.push_back(a.toJson());
    }
    j["attributes"] = attrArray;
    return j;
}

DomNodeRecord DomNodeRecord::fromJson(const nlohmann::json& j) {
    DomNodeRecord r;
    r.nodeToken = j.value("nodeToken", "");
    r.nodeType = j.value("nodeType", "element");
    r.symbol = j.value("symbol", "");
    r.parentToken = j.value("parentToken", "");
    r.siblingIndex = j.value("siblingIndex", 0);
    r.textContent = j.value("textContent", "");
    r.namespaceUri = j.value("namespaceUri", "");
    r.isShadowBoundary = j.value("isShadowBoundary", false);
    r.isFrameBoundary = j.value("isFrameBoundary", false);

    if (j.contains("attributes") && j["attributes"].is_array()) {
        for (const auto& aj : j["attributes"]) {
            r.attributes.push_back(DomAttributeRecord::fromJson(aj));
        }
    }
    return r;
}

// -----------------------------------------------------------------------------
// Helper: Validate session ID format
// -----------------------------------------------------------------------------
static bool isValidSessionId(const std::string& id) {
    if (id.empty() || id.length() < 5) return false;
    // Must start with "page-session."
    const std::string prefix = "page-session.";
    if (id.rfind(prefix, 0) != 0) return false;
    if (id.length() == prefix.length()) return false;
    for (size_t i = prefix.length(); i < id.length(); ++i) {
        char c = id[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_' && c != '.') {
            return false;
        }
    }
    return true;
}

// -----------------------------------------------------------------------------
// DomSnapshot
// -----------------------------------------------------------------------------
ValidationResult DomSnapshot::validate(const DomProtocolBounds& bounds) const {
    if (protocolVersion != kDomProtocolVersion) {
        return ValidationResult::fail("Unknown or unsupported protocolVersion: " + std::to_string(protocolVersion));
    }
    if (!isValidSessionId(pageSessionId)) {
        return ValidationResult::fail("Invalid or malformed pageSessionId: '" + pageSessionId + "'");
    }
    if (rootNodeToken.empty()) {
        return ValidationResult::fail("Snapshot rootNodeToken cannot be empty");
    }
    if (nodes.empty()) {
        return ValidationResult::fail("Snapshot nodes collection cannot be empty");
    }
    if (nodes.size() > bounds.maxNodes) {
        return ValidationResult::fail("Snapshot exceeds maximum admitted node budget: " +
                                     std::to_string(nodes.size()) + " > " + std::to_string(bounds.maxNodes));
    }

    std::unordered_set<std::string> seenTokens;
    std::unordered_map<std::string, std::string> parentMap;
    bool foundRoot = false;
    size_t totalBytes = 0;

    for (const auto& node : nodes) {
        if (node.nodeToken.empty()) {
            return ValidationResult::fail("Node token cannot be empty");
        }
        if (node.symbol.empty()) {
            return ValidationResult::fail("Node symbol cannot be empty for token: " + node.nodeToken);
        }
        if (seenTokens.count(node.nodeToken) > 0) {
            return ValidationResult::fail("Duplicate node token in snapshot: '" + node.nodeToken + "'");
        }
        seenTokens.insert(node.nodeToken);
        parentMap[node.nodeToken] = node.parentToken;

        if (node.attributes.size() > bounds.maxAttributesPerNode) {
            return ValidationResult::fail("Node '" + node.nodeToken + "' exceeds attribute budget: " +
                                         std::to_string(node.attributes.size()) + " > " + std::to_string(bounds.maxAttributesPerNode));
        }
        if (node.textContent.size() > bounds.maxTextBytesPerNode) {
            return ValidationResult::fail("Node '" + node.nodeToken + "' text length exceeds budget: " +
                                         std::to_string(node.textContent.size()) + " > " + std::to_string(bounds.maxTextBytesPerNode));
        }

        totalBytes += node.textContent.size() + node.symbol.size() + node.nodeToken.size();
        for (const auto& a : node.attributes) {
            totalBytes += a.name.size() + a.value.size();
        }

        if (node.nodeToken == rootNodeToken) {
            foundRoot = true;
            if (!node.parentToken.empty()) {
                return ValidationResult::fail("Root node '" + rootNodeToken + "' must have empty parentToken, found: '" + node.parentToken + "'");
            }
        }
    }

    if (!foundRoot) {
        return ValidationResult::fail("Declared rootNodeToken '" + rootNodeToken + "' not found in snapshot nodes");
    }

    if (totalBytes > bounds.maxTotalPayloadBytes) {
        return ValidationResult::fail("Snapshot total payload exceeds budget");
    }

    // Verify parent references and cycle detection
    for (const auto& node : nodes) {
        if (node.nodeToken == rootNodeToken) continue;

        if (node.parentToken.empty()) {
            return ValidationResult::fail("Non-root node '" + node.nodeToken + "' has empty parentToken");
        }
        if (seenTokens.count(node.parentToken) == 0) {
            return ValidationResult::fail("Non-root node '" + node.nodeToken + "' references missing parentToken '" + node.parentToken + "'");
        }

        // Cycle detection: walk upward from this node
        std::string current = node.nodeToken;
        std::unordered_set<std::string> pathSeen;
        while (!current.empty()) {
            if (pathSeen.count(current) > 0) {
                return ValidationResult::fail("Cycle detected in DOM parent hierarchy containing node '" + current + "'");
            }
            pathSeen.insert(current);
            auto it = parentMap.find(current);
            if (it == parentMap.end()) {
                return ValidationResult::fail("Parent reference gap during hierarchy walk: '" + current + "'");
            }
            current = it->second;
            if (pathSeen.size() > nodes.size()) {
                return ValidationResult::fail("Cycle detected in parent chain of node '" + node.nodeToken + "'");
            }
        }
    }

    return ValidationResult::ok();
}

nlohmann::json DomSnapshot::toJson() const {
    nlohmann::json nodeArr = nlohmann::json::array();
    for (const auto& n : nodes) {
        nodeArr.push_back(n.toJson());
    }
    return nlohmann::json{
        {"protocolVersion", protocolVersion},
        {"pageSessionId", pageSessionId},
        {"sequenceBase", sequenceBase},
        {"url", url},
        {"rootNodeToken", rootNodeToken},
        {"nodes", nodeArr}
    };
}

DomSnapshot DomSnapshot::fromJson(const nlohmann::json& j, std::string* outError) {
    DomSnapshot s;
    if (!j.is_object()) {
        if (outError) *outError = "Expected JSON object for DomSnapshot";
        return s;
    }
    s.protocolVersion = j.value("protocolVersion", 0);
    s.pageSessionId = j.value("pageSessionId", "");
    s.sequenceBase = j.value("sequenceBase", 0ULL);
    s.url = j.value("url", "");
    s.rootNodeToken = j.value("rootNodeToken", "");

    if (j.contains("nodes") && j["nodes"].is_array()) {
        for (const auto& nj : j["nodes"]) {
            s.nodes.push_back(DomNodeRecord::fromJson(nj));
        }
    }

    auto v = s.validate();
    if (!v.valid && outError) {
        *outError = v.error;
    }
    return s;
}

// -----------------------------------------------------------------------------
// DomDeltaKind Helpers
// -----------------------------------------------------------------------------
std::string deltaKindToString(DomDeltaKind kind) {
    switch (kind) {
        case DomDeltaKind::Insert: return "insert";
        case DomDeltaKind::Remove: return "remove";
        case DomDeltaKind::Move: return "move";
        case DomDeltaKind::AttributeSet: return "attribute-set";
        case DomDeltaKind::AttributeRemove: return "attribute-remove";
        case DomDeltaKind::TextChange: return "text-change";
    }
    return "unknown";
}

std::optional<DomDeltaKind> deltaKindFromString(const std::string& s) {
    if (s == "insert" || s == "node-inserted") return DomDeltaKind::Insert;
    if (s == "remove" || s == "node-removed") return DomDeltaKind::Remove;
    if (s == "move" || s == "node-moved" || s == "node-reparented") return DomDeltaKind::Move;
    if (s == "attribute-set" || s == "attribute-changed") return DomDeltaKind::AttributeSet;
    if (s == "attribute-remove" || s == "attribute-removed") return DomDeltaKind::AttributeRemove;
    if (s == "text-change" || s == "text-changed") return DomDeltaKind::TextChange;
    return std::nullopt;
}

// -----------------------------------------------------------------------------
// DomDeltaRecord
// -----------------------------------------------------------------------------
nlohmann::json DomDeltaRecord::toJson() const {
    nlohmann::json j{
        {"kind", deltaKindToString(kind)},
        {"targetNodeToken", targetNodeToken}
    };
    if (!parentToken.empty()) j["parentToken"] = parentToken;
    if (siblingIndex > 0) j["siblingIndex"] = siblingIndex;
    if (node.has_value()) j["node"] = node->toJson();
    if (!attributeName.empty()) j["attributeName"] = attributeName;
    if (!attributeValue.empty()) j["attributeValue"] = attributeValue;
    if (!textContent.empty()) j["textContent"] = textContent;
    return j;
}

DomDeltaRecord DomDeltaRecord::fromJson(const nlohmann::json& j) {
    DomDeltaRecord r;
    std::string kindStr = j.value("kind", "insert");
    auto parsedKind = deltaKindFromString(kindStr);
    r.kind = parsedKind.value_or(DomDeltaKind::Insert);
    r.targetNodeToken = j.value("targetNodeToken", "");
    r.parentToken = j.value("parentToken", "");
    r.siblingIndex = j.value("siblingIndex", 0);
    r.attributeName = j.value("attributeName", "");
    r.attributeValue = j.value("attributeValue", "");
    r.textContent = j.value("textContent", "");

    if (j.contains("node") && j["node"].is_object()) {
        r.node = DomNodeRecord::fromJson(j["node"]);
    }
    return r;
}

// -----------------------------------------------------------------------------
// DomDelta
// -----------------------------------------------------------------------------
ValidationResult DomDelta::validate(const DomProtocolBounds& bounds) const {
    if (protocolVersion != kDomProtocolVersion) {
        return ValidationResult::fail("Unknown protocolVersion in Delta: " + std::to_string(protocolVersion));
    }
    if (!isValidSessionId(pageSessionId)) {
        return ValidationResult::fail("Invalid pageSessionId in Delta: '" + pageSessionId + "'");
    }
    if (records.empty()) {
        return ValidationResult::fail("Delta records cannot be empty");
    }
    if (records.size() > bounds.maxMutationRecordsPerBatch) {
        return ValidationResult::fail("Delta records count exceeds batch budget");
    }
    for (const auto& rec : records) {
        if (rec.targetNodeToken.empty()) {
            return ValidationResult::fail("Delta record targetNodeToken cannot be empty");
        }
        if (rec.kind == DomDeltaKind::Insert) {
            if (!rec.node.has_value()) {
                return ValidationResult::fail("Insert record missing node descriptor for target: " + rec.targetNodeToken);
            }
            if (rec.node->symbol.empty()) {
                return ValidationResult::fail("Insert record node symbol cannot be empty");
            }
        } else if (rec.kind == DomDeltaKind::AttributeSet) {
            if (rec.attributeName.empty()) {
                return ValidationResult::fail("AttributeSet record requires non-empty attributeName");
            }
        } else if (rec.kind == DomDeltaKind::AttributeRemove) {
            if (rec.attributeName.empty()) {
                return ValidationResult::fail("AttributeRemove record requires non-empty attributeName");
            }
        }
    }
    return ValidationResult::ok();
}

nlohmann::json DomDelta::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : records) {
        arr.push_back(r.toJson());
    }
    nlohmann::json j{
        {"protocolVersion", protocolVersion},
        {"pageSessionId", pageSessionId},
        {"sequence", sequence},
        {"records", arr}
    };
    if (!originOperationId.empty()) {
        j["originOperationId"] = originOperationId;
    }
    return j;
}

DomDelta DomDelta::fromJson(const nlohmann::json& j, std::string* outError) {
    DomDelta d;
    if (!j.is_object()) {
        if (outError) *outError = "Expected JSON object for DomDelta";
        return d;
    }
    d.protocolVersion = j.value("protocolVersion", 0);
    d.pageSessionId = j.value("pageSessionId", "");
    d.sequence = j.value("sequence", 0ULL);
    d.originOperationId = j.value("originOperationId", "");

    if (j.contains("records") && j["records"].is_array()) {
        for (const auto& rj : j["records"]) {
            d.records.push_back(DomDeltaRecord::fromJson(rj));
        }
    }

    auto v = d.validate();
    if (!v.valid && outError) {
        *outError = v.error;
    }
    return d;
}

// -----------------------------------------------------------------------------
// DomActKind Helpers
// -----------------------------------------------------------------------------
std::string actKindToString(DomActKind kind) {
    switch (kind) {
        case DomActKind::SetText: return "setText";
        case DomActKind::SetAttribute: return "setAttribute";
        case DomActKind::RemoveAttribute: return "removeAttribute";
        case DomActKind::RemoveNode: return "removeNode";
        case DomActKind::InsertElement: return "insertElement";
        case DomActKind::InsertText: return "insertText";
        case DomActKind::MoveNode: return "moveNode";
    }
    return "unknown";
}

std::optional<DomActKind> actKindFromString(const std::string& s) {
    if (s == "setText") return DomActKind::SetText;
    if (s == "setAttribute") return DomActKind::SetAttribute;
    if (s == "removeAttribute") return DomActKind::RemoveAttribute;
    if (s == "removeNode") return DomActKind::RemoveNode;
    if (s == "insertElement") return DomActKind::InsertElement;
    if (s == "insertText") return DomActKind::InsertText;
    if (s == "moveNode") return DomActKind::MoveNode;
    return std::nullopt;
}

// -----------------------------------------------------------------------------
// DomAct
// -----------------------------------------------------------------------------
ValidationResult DomAct::validate() const {
    if (protocolVersion != kDomProtocolVersion) {
        return ValidationResult::fail("Unknown protocolVersion in Act: " + std::to_string(protocolVersion));
    }
    if (!isValidSessionId(pageSessionId)) {
        return ValidationResult::fail("Invalid pageSessionId in Act: '" + pageSessionId + "'");
    }
    if (operationId.empty()) {
        return ValidationResult::fail("Act operationId cannot be empty");
    }
    if (targetNodeToken.empty()) {
        return ValidationResult::fail("Act targetNodeToken cannot be empty");
    }

    if (kind == DomActKind::SetAttribute) {
        if (attributeName.empty()) {
            return ValidationResult::fail("SetAttribute requires non-empty attributeName");
        }
    } else if (kind == DomActKind::RemoveAttribute) {
        if (attributeName.empty()) {
            return ValidationResult::fail("RemoveAttribute requires non-empty attributeName");
        }
    } else if (kind == DomActKind::InsertElement) {
        if (tagName.empty()) {
            return ValidationResult::fail("InsertElement requires non-empty tagName");
        }
        if (parentToken.empty()) {
            return ValidationResult::fail("InsertElement requires non-empty parentToken");
        }
    } else if (kind == DomActKind::InsertText) {
        if (parentToken.empty()) {
            return ValidationResult::fail("InsertText requires non-empty parentToken");
        }
    } else if (kind == DomActKind::MoveNode) {
        if (parentToken.empty()) {
            return ValidationResult::fail("MoveNode requires non-empty parentToken");
        }
    }

    return ValidationResult::ok();
}

nlohmann::json DomAct::toJson() const {
    nlohmann::json j{
        {"protocolVersion", protocolVersion},
        {"pageSessionId", pageSessionId},
        {"operationId", operationId},
        {"kind", actKindToString(kind)},
        {"targetNodeToken", targetNodeToken}
    };
    if (!text.empty()) j["text"] = text;
    if (!attributeName.empty()) j["attributeName"] = attributeName;
    if (!attributeValue.empty()) j["attributeValue"] = attributeValue;
    if (!tagName.empty()) j["tagName"] = tagName;
    if (!parentToken.empty()) j["parentToken"] = parentToken;
    if (siblingIndex > 0) j["siblingIndex"] = siblingIndex;
    return j;
}

DomAct DomAct::fromJson(const nlohmann::json& j, std::string* outError) {
    DomAct a;
    if (!j.is_object()) {
        if (outError) *outError = "Expected JSON object for DomAct";
        return a;
    }
    a.protocolVersion = j.value("protocolVersion", 0);
    a.pageSessionId = j.value("pageSessionId", "");
    a.operationId = j.value("operationId", "");
    auto parsedKind = actKindFromString(j.value("kind", ""));
    a.kind = parsedKind.value_or(DomActKind::SetText);
    a.targetNodeToken = j.value("targetNodeToken", "");
    a.text = j.value("text", "");
    a.attributeName = j.value("attributeName", "");
    a.attributeValue = j.value("attributeValue", "");
    a.tagName = j.value("tagName", "");
    a.parentToken = j.value("parentToken", "");
    a.siblingIndex = j.value("siblingIndex", 0);

    auto v = a.validate();
    if (!v.valid && outError) {
        *outError = v.error;
    }
    return a;
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
