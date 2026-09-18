#include "Singularity/Foreign/Web/DomMirrorBridge.hpp"
#include "Singularity/Foreign/Web/RealWebView.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace Singularity {
namespace Foreign {
namespace Web {

DomMirrorBridge::DomMirrorBridge(Integration::RealWebView* webView)
    : _webView(webView) {
    if (_webView) {
        attachWebView(_webView);
    }
}

DomMirrorBridge::~DomMirrorBridge() {
    detachWebView();
}

void DomMirrorBridge::attachWebView(Integration::RealWebView* webView) {
    _webView = webView;
    if (_webView) {
        _webView->registerJavaScriptHandler("domMirror", [this](const std::string& message) {
            std::string err;
            if (!this->handleWireMessage(message, &err)) {
                this->notifyError("Failed to handle wire message: " + err);
            }
        });
    }
}

void DomMirrorBridge::detachWebView() {
    _translator.retire();
    _pendingActs.clear();
    _webView = nullptr;
}

bool DomMirrorBridge::injectMirrorScript() {
    if (!_webView) return false;

    // Read the script from disk if available
    std::string scriptPath = "src/Singularity/Foreign/Web/dom_mirror.js";
    std::ifstream file(scriptPath);
    std::string scriptContent;
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        scriptContent = buffer.str();
    }

    if (scriptContent.empty()) {
        notifyError("dom_mirror.js not found or empty");
        return false;
    }

    _webView->executeJavaScript(scriptContent);
    return true;
}

bool DomMirrorBridge::handleWireMessage(const std::string& jsonMessage, std::string* outError) {
    nlohmann::json root;
    try {
        root = nlohmann::json::parse(jsonMessage);
    } catch (const std::exception& e) {
        if (outError) *outError = std::string("JSON parse error: ") + e.what();
        return false;
    }

    if (!root.is_object() || !root.contains("type") || !root.contains("payload")) {
        if (outError) *outError = "Malformed envelope: must contain 'type' and 'payload'";
        return false;
    }

    std::string type = root.value("type", "");
    const auto& payload = root["payload"];

    if (type == "snapshot") {
        std::string err;
        DomSnapshot snapshot = DomSnapshot::fromJson(payload, &err);
        if (!err.empty()) {
            if (outError) *outError = "Failed to parse snapshot: " + err;
            return false;
        }

        if (!_translator.admitSnapshot(snapshot, &err)) {
            if (outError) *outError = "Translator refused snapshot: " + err;
            return false;
        }

        if (_onSnapshotAdmitted) {
            _onSnapshotAdmitted(snapshot);
        }
        return true;
    } else if (type == "delta") {
        std::string err;
        DomDelta delta = DomDelta::fromJson(payload, &err);
        if (!err.empty()) {
            if (outError) *outError = "Failed to parse delta: " + err;
            return false;
        }

        if (!_translator.applyDelta(delta, &err)) {
            if (outError) *outError = "Translator refused delta: " + err;
            return false;
        }

        // Check if this delta confirmed a pending Act
        if (!delta.originOperationId.empty()) {
            auto it = _pendingActs.find(delta.originOperationId);
            if (it != _pendingActs.end()) {
                _pendingActs.erase(it);
                if (_onActConfirmed) {
                    _onActConfirmed(delta.originOperationId);
                }
            }
        }

        if (_onDeltaApplied) {
            _onDeltaApplied(delta);
        }
        return true;
    } else {
        if (outError) *outError = "Unknown message type: " + type;
        return false;
    }
}

bool DomMirrorBridge::issueAct(const DomAct& act, std::string* outError) {
    ValidationResult v = act.validate();
    if (!v.valid) {
        if (outError) *outError = "Act validation failed: " + v.error;
        return false;
    }

    if (!_translator.hasActiveSession() || act.pageSessionId != _translator.getPageSessionId()) {
        if (outError) *outError = "Cannot issue act: session mismatch or inactive session";
        return false;
    }

    _pendingActs[act.operationId] = act;

    if (_webView) {
        nlohmann::json actJson = act.toJson();
        std::string script = "window.__earthcall_apply_act && window.__earthcall_apply_act(" + actJson.dump() + ");";
        _webView->executeJavaScript(script);
    }

    return true;
}

bool DomMirrorBridge::issueSetText(const std::string& nodeToken, const std::string& text, const std::string& operationId, std::string* outError) {
    DomAct act;
    act.protocolVersion = kDomProtocolVersion;
    act.pageSessionId = _translator.getPageSessionId();
    act.operationId = operationId;
    act.kind = DomActKind::SetText;
    act.targetNodeToken = nodeToken;
    act.text = text;
    return issueAct(act, outError);
}

bool DomMirrorBridge::issueSetAttribute(const std::string& nodeToken, const std::string& name, const std::string& value, const std::string& operationId, std::string* outError) {
    DomAct act;
    act.protocolVersion = kDomProtocolVersion;
    act.pageSessionId = _translator.getPageSessionId();
    act.operationId = operationId;
    act.kind = DomActKind::SetAttribute;
    act.targetNodeToken = nodeToken;
    act.attributeName = name;
    act.attributeValue = value;
    return issueAct(act, outError);
}

bool DomMirrorBridge::issueRemoveNode(const std::string& nodeToken, const std::string& operationId, std::string* outError) {
    DomAct act;
    act.protocolVersion = kDomProtocolVersion;
    act.pageSessionId = _translator.getPageSessionId();
    act.operationId = operationId;
    act.kind = DomActKind::RemoveNode;
    act.targetNodeToken = nodeToken;
    return issueAct(act, outError);
}

void DomMirrorBridge::onNavigationStarted() {
    _translator.retire();
    _pendingActs.clear();
}

void DomMirrorBridge::notifyError(const std::string& err) {
    if (_onError) {
        _onError(err);
    } else {
        std::cerr << "[DomMirrorBridge Error] " << err << std::endl;
    }
}

} // namespace Web
} // namespace Foreign
} // namespace Singularity
