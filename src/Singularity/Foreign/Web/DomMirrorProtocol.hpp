#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include "json.hpp"

namespace Singularity {
namespace Foreign {
namespace Web {

constexpr int kDomProtocolVersion = 1;

// Resource limits to defend against hostile or pathological web pages
struct DomProtocolBounds {
    size_t maxNodes = 50000;
    size_t maxTextBytesPerNode = 1000000; // 1 MB
    size_t maxAttributesPerNode = 1000;
    size_t maxTotalPayloadBytes = 50000000; // 50 MB
    size_t maxMutationRecordsPerBatch = 10000;
};

// Attribute entry
struct DomAttributeRecord {
    std::string name;
    std::string value;

    nlohmann::json toJson() const;
    static DomAttributeRecord fromJson(const nlohmann::json& j);
};

// Individual node record in a snapshot
struct DomNodeRecord {
    std::string nodeToken;     // Unique token assigned by browser, e.g. "node.1"
    std::string nodeType;      // "element", "text", "comment", "document"
    std::string symbol;        // Visible symbol/tag e.g. "div", "span", "#text"
    std::string parentToken;   // Token of parent, or empty for root
    int siblingIndex = 0;      // Index among siblings
    std::vector<DomAttributeRecord> attributes;
    std::string textContent;   // Present for text nodes
    std::string namespaceUri;  // Optional namespace
    bool isShadowBoundary = false;
    bool isFrameBoundary = false;

    nlohmann::json toJson() const;
    static DomNodeRecord fromJson(const nlohmann::json& j);
};

struct ValidationResult {
    bool valid = true;
    std::string error;

    static ValidationResult ok() { return {true, ""}; }
    static ValidationResult fail(const std::string& err) { return {false, err}; }
};

// Initial document snapshot envelope
struct DomSnapshot {
    int protocolVersion = kDomProtocolVersion;
    std::string pageSessionId; // e.g. "page-session.a91f4b2c"
    uint64_t sequenceBase = 0;
    std::string url;
    std::string rootNodeToken;
    std::vector<DomNodeRecord> nodes;

    ValidationResult validate(const DomProtocolBounds& bounds = DomProtocolBounds()) const;
    nlohmann::json toJson() const;
    static DomSnapshot fromJson(const nlohmann::json& j, std::string* outError = nullptr);
};

// Edge mutation record kinds
enum class DomDeltaKind {
    Insert,
    Remove,
    Move,
    AttributeSet,
    AttributeRemove,
    TextChange
};

std::string deltaKindToString(DomDeltaKind kind);
std::optional<DomDeltaKind> deltaKindFromString(const std::string& s);

// Mutation record within a delta batch
struct DomDeltaRecord {
    DomDeltaKind kind = DomDeltaKind::Insert;
    std::string targetNodeToken;
    std::string parentToken;
    int siblingIndex = 0;
    std::optional<DomNodeRecord> node; // populated on Insert
    std::string attributeName;         // for AttributeSet / AttributeRemove
    std::string attributeValue;        // for AttributeSet
    std::string textContent;           // for TextChange

    nlohmann::json toJson() const;
    static DomDeltaRecord fromJson(const nlohmann::json& j);
};

// Delta batch envelope
struct DomDelta {
    int protocolVersion = kDomProtocolVersion;
    std::string pageSessionId;
    uint64_t sequence = 0;
    std::string originOperationId; // Optional: present when this delta confirms an Earthcall Act
    std::vector<DomDeltaRecord> records;

    ValidationResult validate(const DomProtocolBounds& bounds = DomProtocolBounds()) const;
    nlohmann::json toJson() const;
    static DomDelta fromJson(const nlohmann::json& j, std::string* outError = nullptr);
};

// Actuation kinds from Earthcall to the browser
enum class DomActKind {
    SetText,
    SetAttribute,
    RemoveAttribute,
    RemoveNode,
    InsertElement,
    InsertText,
    MoveNode
};

std::string actKindToString(DomActKind kind);
std::optional<DomActKind> actKindFromString(const std::string& s);

// Act envelope
struct DomAct {
    int protocolVersion = kDomProtocolVersion;
    std::string pageSessionId;
    std::string operationId; // e.g. "op.101"
    DomActKind kind = DomActKind::SetText;
    std::string targetNodeToken;

    // Structured parameters (no raw arbitrary JavaScript strings!)
    std::string text;
    std::string attributeName;
    std::string attributeValue;
    std::string tagName;
    std::string parentToken;
    int siblingIndex = 0;

    ValidationResult validate() const;
    nlohmann::json toJson() const;
    static DomAct fromJson(const nlohmann::json& j, std::string* outError = nullptr);
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
