#include "Singularity/Foreign/Web/DomMirrorProtocol.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace Singularity::Foreign::Web;

static std::string readFixture(const std::string& path) {
    std::ifstream file(path);
    assert(file.is_open());
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    std::cout << "Running foreign_web_dom_protocol_test..." << std::endl;

    // 1. Basic Snapshot Fixture Parsing & Validation
    {
        std::string jsonStr = readFixture("tests/fixtures/foreign_web/dom_snapshot_basic.json");
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        std::string err;
        DomSnapshot snapshot = DomSnapshot::fromJson(j, &err);
        assert(err.empty());
        assert(snapshot.protocolVersion == 1);
        assert(snapshot.pageSessionId == "page-session.a91f4b2c");
        assert(snapshot.rootNodeToken == "node.1");
        assert(snapshot.nodes.size() == 11);

        // Prove duplicate tag strings do NOT collapse protocol identity
        // node.6 and node.7 are both "div" elements
        assert(snapshot.nodes[5].symbol == "div");
        assert(snapshot.nodes[6].symbol == "div");
        assert(snapshot.nodes[5].nodeToken != snapshot.nodes[6].nodeToken);
        assert(snapshot.nodes[5].nodeToken == "node.6");
        assert(snapshot.nodes[6].nodeToken == "node.7");

        // Validate snapshot round-trip
        nlohmann::json roundtripJson = snapshot.toJson();
        assert(roundtripJson["nodes"].size() == 11);
        assert(roundtripJson["pageSessionId"] == "page-session.a91f4b2c");
    }

    // 2. Hostile String & Unicode Preservation
    {
        std::string jsonStr = readFixture("tests/fixtures/foreign_web/dom_snapshot_hostile.json");
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        std::string err;
        DomSnapshot snapshot = DomSnapshot::fromJson(j, &err);
        assert(err.empty());
        assert(snapshot.nodes.size() == 2);

        // Verify quotes, backticks, newlines, and Unicode are preserved verbatim as data
        const auto& textNode = snapshot.nodes[1];
        assert(textNode.textContent.find("`backtick`") != std::string::npos);
        assert(textNode.textContent.find("\"quotes\"") != std::string::npos);
        assert(textNode.textContent.find("\n") != std::string::npos);
        assert(textNode.textContent.find("🌍 Earthcall 敬畏耶和华 🌟") != std::string::npos);
        assert(textNode.textContent.find("${eval(malicious)}") != std::string::npos);

        // Attribute payload with hostile script injection content
        const auto& rootNode = snapshot.nodes[0];
        assert(rootNode.attributes[1].value == "<script>alert('pwned');</script> && rm -rf /");
    }

    // 3. Reject Unknown Protocol Version
    {
        DomSnapshot badVersion;
        badVersion.protocolVersion = 99;
        badVersion.pageSessionId = "page-session.valid123";
        badVersion.rootNodeToken = "node.1";
        DomNodeRecord r;
        r.nodeToken = "node.1";
        r.symbol = "html";
        badVersion.nodes.push_back(r);

        ValidationResult res = badVersion.validate();
        assert(!res.valid);
        assert(res.error.find("protocolVersion") != std::string::npos);
    }

    // 4. Reject Empty or Malformed Session IDs
    {
        DomSnapshot badSession;
        badSession.protocolVersion = 1;
        badSession.pageSessionId = ""; // Empty
        badSession.rootNodeToken = "node.1";
        DomNodeRecord r;
        r.nodeToken = "node.1";
        r.symbol = "html";
        badSession.nodes.push_back(r);

        assert(!badSession.validate().valid);

        badSession.pageSessionId = "invalid_no_prefix";
        assert(!badSession.validate().valid);

        badSession.pageSessionId = "page-session.invalid space";
        assert(!badSession.validate().valid);
    }

    // 5. Reject Duplicate Node Tokens
    {
        DomSnapshot dupTokens;
        dupTokens.protocolVersion = 1;
        dupTokens.pageSessionId = "page-session.test";
        dupTokens.rootNodeToken = "node.1";

        DomNodeRecord r1;
        r1.nodeToken = "node.1";
        r1.symbol = "html";
        dupTokens.nodes.push_back(r1);

        DomNodeRecord r2;
        r2.nodeToken = "node.1"; // Duplicate!
        r2.symbol = "body";
        r2.parentToken = "node.1";
        dupTokens.nodes.push_back(r2);

        ValidationResult res = dupTokens.validate();
        assert(!res.valid);
        assert(res.error.find("Duplicate node token") != std::string::npos);
    }

    // 6. Reject Missing Parent References
    {
        DomSnapshot missingParent;
        missingParent.protocolVersion = 1;
        missingParent.pageSessionId = "page-session.test";
        missingParent.rootNodeToken = "node.1";

        DomNodeRecord r1;
        r1.nodeToken = "node.1";
        r1.symbol = "html";
        missingParent.nodes.push_back(r1);

        DomNodeRecord r2;
        r2.nodeToken = "node.2";
        r2.symbol = "div";
        r2.parentToken = "node.999_does_not_exist"; // Missing!
        missingParent.nodes.push_back(r2);

        ValidationResult res = missingParent.validate();
        assert(!res.valid);
        assert(res.error.find("missing parentToken") != std::string::npos);
    }

    // 7. Reject Cycles in DOM Hierarchy
    {
        DomSnapshot cycleSnapshot;
        cycleSnapshot.protocolVersion = 1;
        cycleSnapshot.pageSessionId = "page-session.cycle";
        cycleSnapshot.rootNodeToken = "node.1";

        DomNodeRecord r1;
        r1.nodeToken = "node.1";
        r1.symbol = "html";
        cycleSnapshot.nodes.push_back(r1);

        // node.2 parent is node.3, node.3 parent is node.2 (Cycle!)
        DomNodeRecord r2;
        r2.nodeToken = "node.2";
        r2.symbol = "div";
        r2.parentToken = "node.3";
        cycleSnapshot.nodes.push_back(r2);

        DomNodeRecord r3;
        r3.nodeToken = "node.3";
        r3.symbol = "span";
        r3.parentToken = "node.2";
        cycleSnapshot.nodes.push_back(r3);

        ValidationResult res = cycleSnapshot.validate();
        assert(!res.valid);
        assert(res.error.find("Cycle detected") != std::string::npos);
    }

    // 8. Enforce Resource Bounds
    {
        DomSnapshot hugeSnapshot;
        hugeSnapshot.protocolVersion = 1;
        hugeSnapshot.pageSessionId = "page-session.bounds";
        hugeSnapshot.rootNodeToken = "node.1";

        DomNodeRecord r1;
        r1.nodeToken = "node.1";
        r1.symbol = "html";
        hugeSnapshot.nodes.push_back(r1);

        DomProtocolBounds strictBounds;
        strictBounds.maxNodes = 1; // Bound to 1 node

        DomNodeRecord r2;
        r2.nodeToken = "node.2";
        r2.symbol = "div";
        r2.parentToken = "node.1";
        hugeSnapshot.nodes.push_back(r2);

        ValidationResult res = hugeSnapshot.validate(strictBounds);
        assert(!res.valid);
        assert(res.error.find("maximum admitted node budget") != std::string::npos);
    }

    // 9. Delta Fixture Parsing, Validation & Round-Trip
    {
        std::string jsonStr = readFixture("tests/fixtures/foreign_web/dom_delta_basic.json");
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        std::string err;
        DomDelta delta = DomDelta::fromJson(j, &err);
        assert(err.empty());
        assert(delta.protocolVersion == 1);
        assert(delta.pageSessionId == "page-session.a91f4b2c");
        assert(delta.sequence == 1);
        assert(delta.originOperationId == "op.act.001");
        assert(delta.records.size() == 4);

        assert(delta.records[0].kind == DomDeltaKind::TextChange);
        assert(delta.records[0].targetNodeToken == "node.9");
        assert(delta.records[0].textContent == "Updated First Link Text");

        assert(delta.records[1].kind == DomDeltaKind::AttributeSet);
        assert(delta.records[1].targetNodeToken == "node.6");
        assert(delta.records[1].attributeName == "data-selected");
        assert(delta.records[1].attributeValue == "true");

        assert(delta.records[2].kind == DomDeltaKind::Insert);
        assert(delta.records[2].targetNodeToken == "node.12");
        assert(delta.records[2].node.has_value());
        assert(delta.records[2].node->symbol == "p");

        assert(delta.records[3].kind == DomDeltaKind::Remove);
        assert(delta.records[3].targetNodeToken == "node.10");

        nlohmann::json roundtripJson = delta.toJson();
        assert(roundtripJson["records"].size() == 4);
        assert(roundtripJson["originOperationId"] == "op.act.001");
    }

    // 10. Act Validation & Round-Trip
    {
        DomAct act;
        act.protocolVersion = 1;
        act.pageSessionId = "page-session.a91f4b2c";
        act.operationId = "op.act.001";
        act.kind = DomActKind::SetText;
        act.targetNodeToken = "node.9";
        act.text = "Changed by Earthcall Law";

        ValidationResult res = act.validate();
        assert(res.valid);

        nlohmann::json actJson = act.toJson();
        assert(actJson["kind"] == "setText");
        assert(actJson["targetNodeToken"] == "node.9");
        assert(actJson["text"] == "Changed by Earthcall Law");

        std::string err;
        DomAct parsedAct = DomAct::fromJson(actJson, &err);
        assert(err.empty());
        assert(parsedAct.operationId == "op.act.001");
        assert(parsedAct.kind == DomActKind::SetText);
        assert(parsedAct.text == "Changed by Earthcall Law");
    }

    std::cout << "foreign_web_dom_protocol_test PASSED!" << std::endl;
    return 0;
}
