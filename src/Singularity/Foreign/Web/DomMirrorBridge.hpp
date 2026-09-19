#pragma once

#include "Singularity/Foreign/Web/DomMirrorProtocol.hpp"
#include "Singularity/Foreign/Web/DomMirrorTranslator.hpp"

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace Integration {
class RealWebView;
}

namespace Singularity {
namespace Foreign {
namespace Web {

// Callback types
using SnapshotAdmittedCallback = std::function<void(const DomSnapshot&)>;
using DeltaAppliedCallback = std::function<void(const DomDelta&)>;
using ActConfirmedCallback = std::function<void(const std::string& operationId)>;
using ErrorCallback = std::function<void(const std::string& error)>;

// DomMirrorBridge manages the lifecycle of the bidirectional DOM mirror
// between WebKit (RealWebView) and Earthcall native Formations/Lexemes.
class DomMirrorBridge {
public:
    explicit DomMirrorBridge(Integration::RealWebView* webView = nullptr);
    ~DomMirrorBridge();

    // Prevent copying
    DomMirrorBridge(const DomMirrorBridge&) = delete;
    DomMirrorBridge& operator=(const DomMirrorBridge&) = delete;

    // Attach to a RealWebView instance and register message handlers
    void attachWebView(Integration::RealWebView* webView);
    void detachWebView();

    // Inject the dom_mirror.js script resource into the webview
    bool injectMirrorScript();

    // Process incoming JSON wire message from dom_mirror.js
    bool handleWireMessage(const std::string& jsonMessage, std::string* outError = nullptr);

    // Issue a structured DOM Act from Earthcall into the browser
    bool issueAct(const DomAct& act, std::string* outError = nullptr);

    // Helper to issue a setText Act targeting an exact node token
    bool issueSetText(const std::string& nodeToken, const std::string& text, const std::string& operationId, std::string* outError = nullptr);

    // Helper to issue a setAttribute Act targeting an exact node token
    bool issueSetAttribute(const std::string& nodeToken, const std::string& name, const std::string& value, const std::string& operationId, std::string* outError = nullptr);

    // Helper to issue a removeNode Act targeting an exact node token
    bool issueRemoveNode(const std::string& nodeToken, const std::string& operationId, std::string* outError = nullptr);

    // Notification of full document navigation: retires old session atomically
    void onNavigationStarted();

    // Access to the underlying translator and native graph
    DomMirrorTranslator& translator() { return _translator; }
    const DomMirrorTranslator& translator() const { return _translator; }

    bool hasActiveSession() const { return _translator.hasActiveSession(); }
    const std::string& getPageSessionId() const { return _translator.getPageSessionId(); }

    // Event hooks
    void onSnapshotAdmitted(SnapshotAdmittedCallback cb) { _onSnapshotAdmitted = std::move(cb); }
    void onDeltaApplied(DeltaAppliedCallback cb) { _onDeltaApplied = std::move(cb); }
    void onActConfirmed(ActConfirmedCallback cb) { _onActConfirmed = std::move(cb); }
    void onError(ErrorCallback cb) { _onError = std::move(cb); }

private:
    Integration::RealWebView* _webView = nullptr;
    DomMirrorTranslator _translator;

    // Track pending acts awaiting confirmation delta
    std::unordered_map<std::string, DomAct> _pendingActs;

    SnapshotAdmittedCallback _onSnapshotAdmitted;
    DeltaAppliedCallback _onDeltaApplied;
    ActConfirmedCallback _onActConfirmed;
    ErrorCallback _onError;

    void notifyError(const std::string& err);
};

} // namespace Web
} // namespace Foreign
} // namespace Singularity
