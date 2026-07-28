#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <memory>
#include <regex>
#include "../json.hpp"

namespace Integration {

// Security levels for different operations
enum class SecurityLevel {
    LOW,        // Minimal restrictions (development only)
    MEDIUM,     // Standard restrictions
    HIGH,       // Strict restrictions
    PARANOID    // Maximum security (production)
};

// Types of permissions that can be requested
enum class PermissionType {
    BRUSH_SYSTEM,
    DESIGN_SYSTEM,
    AVATAR_SYSTEM,
    WORLD_ACCESS,
    FILE_SYSTEM,
    NETWORK_ACCESS,
    UI_CONTROL,
    DATA_ACCESS
};

// Security event types for logging
enum class SecurityEventType {
    URL_ACCESS,
    PERMISSION_REQUEST,
    PERMISSION_GRANTED,
    PERMISSION_DENIED,
    INVALID_MESSAGE,
    SUSPICIOUS_ACTIVITY,
    API_CALL,
    JAVASCRIPT_EXECUTION
};

// Security event structure
struct SecurityEvent {
    SecurityEventType type;
    std::string description;
    std::string source;
    std::string details;
    std::time_t timestamp;
    bool blocked;
    
    nlohmann::json serialize() const;
    void deserialize(const nlohmann::json& j);
};

// URL validation result
struct URLValidationResult {
    bool isValid;
    bool isWhitelisted;
    std::string reason;
    std::string sanitizedURL;
};

// Message validation result
struct MessageValidationResult {
    bool isValid;
    std::string reason;
    std::string sanitizedMessage;
};

// Security configuration
struct SecurityConfig {
    SecurityLevel level = SecurityLevel::MEDIUM;
    std::vector<std::string> whitelistedDomains;
    std::vector<std::string> blacklistedDomains;
    std::set<PermissionType> defaultPermissions;
    bool enableCSP = true;
    bool enableSandboxing = true;
    bool logAllEvents = true;
    bool requireUserConfirmation = true;

    // Suspicious-activity thresholds. These are policy, so they live with the
    // rest of the policy rather than as constants compiled into the detector.
    // The window is what makes "events per minute" mean that: without it the
    // count was cumulative and every busy source was eventually condemned.
    int activityWindowSeconds = 60;
    int maxEventsPerWindow = 100;
    int maxBlockedEvents = 10;

    nlohmann::json serialize() const;
    void deserialize(const nlohmann::json& j);
};

class SecurityManager {
public:
    static SecurityManager& instance();
    
    // Configuration
    void setSecurityLevel(SecurityLevel level);
    void setConfig(const SecurityConfig& config);
    SecurityConfig getConfig() const { return _config; }
    
    // URL validation
    URLValidationResult validateURL(const std::string& url);
    bool isURLWhitelisted(const std::string& url);
    bool isURLBlacklisted(const std::string& url);
    std::string sanitizeURL(const std::string& url);
    
    // Permission management
    bool requestPermission(PermissionType permission, const std::string& source);
    bool hasPermission(PermissionType permission, const std::string& source) const;
    void grantPermission(PermissionType permission, const std::string& source);
    void revokePermission(PermissionType permission, const std::string& source);
    void revokeAllPermissions(const std::string& source);
    std::set<PermissionType> getGrantedPermissions(const std::string& source) const;
    
    // Message validation
    MessageValidationResult validateMessage(const std::string& message, const std::string& source);
    bool isMessageAllowed(const std::string& message, const std::string& source);
    
    // Content Security Policy
    std::string generateCSP(const std::string& source);
    std::string generateSandboxPolicy();
    
    // Security logging
    void logEvent(SecurityEventType type, const std::string& description, 
                  const std::string& source, const std::string& details = "", bool blocked = false);
    std::vector<SecurityEvent> getSecurityLog() const { return _securityLog; }
    void clearSecurityLog();
    void exportSecurityLog(const std::string& filename);
    
    // Threat detection
    bool detectSuspiciousActivity(const std::string& source);
    void blockSource(const std::string& source);
    void unblockSource(const std::string& source);
    bool isSourceBlocked(const std::string& source) const;
    
    // API security
    bool validateAPICall(const std::string& api, const std::string& source);
    bool isAPICallAllowed(const std::string& api, const std::string& source);
    
    // JavaScript screening.
    //
    // READ THIS BEFORE RELYING ON IT. This is a tripwire over the scripts
    // Earthcall injects into its own WebView, not a boundary against hostile
    // JavaScript. Pattern-matching cannot contain a language that can spell
    // eval() as window['ev'+'al'](); anyone treating a `true` here as "this
    // script is safe" will be wrong. The defences that actually hold are the
    // URL allowlist (validateURL) and the page's CSP.
    //
    // Returns false for constructs first-party script has no business
    // containing, and logs — the point is to notice when something upstream
    // starts building scripts it should not be.
    bool validateJavaScript(const std::string& script, const std::string& source);
    // sanitizeJavaScript() was REMOVED, not renamed. It spliced "// BLOCKED: "
    // into the middle of expressions, which corrupted legitimate first-party
    // scripts (any script containing setTimeout( came out broken) while
    // stopping no attacker who could concatenate two strings. There is no
    // replacement because sanitising JavaScript by substring rewriting does
    // not work; screen with validateJavaScript and rely on CSP.
    
    // Save/Load
    void saveSecurityData();
    void loadSecurityData();
    nlohmann::json serialize() const;
    void deserialize(const nlohmann::json& j);
    
    // Statistics
    int getTotalEvents() const { return _securityLog.size(); }
    int getBlockedEvents() const;
    std::map<std::string, int> getEventCounts() const;
    
    // Callbacks for user interaction
    void setPermissionCallback(std::function<bool(PermissionType, const std::string&)> callback);
    void setSecurityAlertCallback(std::function<void(const SecurityEvent&)> callback);

private:
    SecurityManager() = default;
    ~SecurityManager() = default;
    
    SecurityConfig _config;
    std::map<std::string, std::set<PermissionType>> _grantedPermissions;
    std::set<std::string> _blockedSources;
    std::vector<SecurityEvent> _securityLog;

    // Activity is counted inside a ROLLING WINDOW, not for the process
    // lifetime. _activityWindowStart[src] is when the current window opened;
    // _sourceActivityCount[src] is how many events have landed inside it.
    // Without the window this was a monotonic counter, so every long-lived
    // source eventually crossed the threshold and could never come back.
    std::map<std::string, int> _sourceActivityCount;
    std::map<std::string, std::time_t> _activityWindowStart;

    // Running tally of blocked events per source, kept in step with
    // _securityLog (including when the log is trimmed). detectSuspiciousActivity
    // used to rescan the whole 10,000-entry log on EVERY logEvent call, which
    // made logging quadratic in the number of events.
    std::map<std::string, int> _blockedEventCount;

    static constexpr std::size_t kMaxLogEntries = 10000;
    static constexpr std::size_t kLogTrimCount = 1000;

    std::function<bool(PermissionType, const std::string&)> _permissionCallback;
    std::function<void(const SecurityEvent&)> _securityAlertCallback;
    
    // Internal validation methods
    bool _isValidURLFormat(const std::string& url);
    bool _isSecureProtocol(const std::string& url);
    bool _isLocalFile(const std::string& url);
    // Host of an absolute URL, lowercased, userinfo and port stripped.
    // "" when the string has no scheme separator.
    static std::string _extractHost(const std::string& url);
    // Does `host` fall under `domain`? Matches only on a label boundary, so
    // "earthcall.com" covers "app.earthcall.com" but never
    // "earthcall.com.attacker.example". `domain` may be a bare host or a full
    // URL — configs written before this existed stored the latter.
    static bool _hostMatchesDomain(const std::string& host, const std::string& domain);
    bool _containsSuspiciousContent(const std::string& content);
    bool _isRateLimited(const std::string& source);
    
    // Pattern matching. The first pair is markup, matched against message
    // bodies; the second is JavaScript, matched against injected scripts.
    // They used to be the same two vectors, which is why validateJavaScript
    // was scanning JavaScript for "<iframe".
    std::vector<std::regex> _suspiciousPatterns;
    std::vector<std::regex> _maliciousPatterns;
    std::vector<std::regex> _jsHighRiskPatterns;
    std::vector<std::regex> _jsNoteworthyPatterns;
    void _initializePatterns();
    
    // Rate limiting
    struct RateLimitInfo {
        int count = 0;
        std::time_t lastReset = 0;
    };
    std::map<std::string, RateLimitInfo> _rateLimits;
    bool _checkRateLimit(const std::string& source);
    void _updateRateLimit(const std::string& source);
};

} // namespace Integration 