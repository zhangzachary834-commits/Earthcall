#include "Singularity/Foreign/Web/DomMirrorBridge.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
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
    std::cout << "Running foreign_web_dom_bridge_test..." << std::endl;

    auto& language = Singularity::Language::LanguageSystem::instance();
    language.clear();

    DomMirrorBridge bridge;

    bool snapshotAdmitted = false;
    bridge.onSnapshotAdmitted([&](const DomSnapshot& s) {
        snapshotAdmitted = true;
        assert(s.pageSessionId == "page-session.a91f4b2c");
    });

    bool deltaApplied = false;
    bridge.onDeltaApplied([&](const DomDelta& d) {
        deltaApplied = true;
        assert(d.sequence == 1);
    });

    bool actConfirmed = false;
    std::string confirmedOpId;
    bridge.onActConfirmed([&](const std::string& opId) {
        actConfirmed = true;
        confirmedOpId = opId;
    });

    // 1. Deliver Snapshot Wire Message
    {
        std::string snapshotJsonStr = readFixture("tests/fixtures/foreign_web/dom_snapshot_basic.json");
        nlohmann::json wireMsg{
            {"type", "snapshot"},
            {"payload", nlohmann::json::parse(snapshotJsonStr)}
        };

        std::string err;
        bool ok = bridge.handleWireMessage(wireMsg.dump(), &err);
        assert(ok);
        assert(err.empty());
        assert(snapshotAdmitted);
        assert(bridge.hasActiveSession());
        assert(bridge.getPageSessionId() == "page-session.a91f4b2c");

        // Verify native graph through translator
        auto divNode = bridge.translator().findNodeLexeme("node.6");
        assert(divNode != nullptr);
        assert(divNode->getSymbol() == "div");
        assert(divNode->getIdentifier() == "page-session.a91f4b2c.node.6");
    }

    // 2. Issue Structured Act from Earthcall
    {
        std::string err;
        bool ok = bridge.issueSetText("node.9", "Updated First Link Text", "op.act.001", &err);
        assert(ok);
        assert(err.empty());

        // Target validation
        assert(!bridge.issueSetText("", "text", "op.act.002", &err)); // empty target fails
    }

    // 3. Deliver Confirmation Delta with matching originOperationId
    {
        std::string deltaJsonStr = readFixture("tests/fixtures/foreign_web/dom_delta_basic.json");
        nlohmann::json wireMsg{
            {"type", "delta"},
            {"payload", nlohmann::json::parse(deltaJsonStr)}
        };

        std::string err;
        bool ok = bridge.handleWireMessage(wireMsg.dump(), &err);
        assert(ok);
        assert(err.empty());
        assert(deltaApplied);
        assert(actConfirmed);
        assert(confirmedOpId == "op.act.001");

        // Prove text was updated in native mirror
        auto node9 = bridge.translator().findNodeLexeme("node.9");
        assert(node9 != nullptr);
        assert(node9->getSymbol() == "Updated First Link Text");
    }

    // 4. Navigation Lifecycle
    {
        bridge.onNavigationStarted();
        assert(!bridge.hasActiveSession());
        assert(bridge.translator().findNodeLexeme("node.6") == nullptr);
    }

    std::cout << "foreign_web_dom_bridge_test PASSED!" << std::endl;
    return 0;
}
