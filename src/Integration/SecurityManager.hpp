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
    
    // JavaScript security
    bool validateJavaScript(const std::string& script, const std::string& source);
    std::string sanitizeJavaScript(const std::string& script);
    
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
    std::map<std::string, int> _sourceActivityCount;
    
    std::function<bool(PermissionType, const std::string&)> _permissionCallback;
    std::function<void(const SecurityEvent&)> _securityAlertCallback;
    
    // Internal validation methods
    bool _isValidURLFormat(const std::string& url);
    bool _isSecureProtocol(const std::string& url);
    bool _isLocalFile(const std::string& url);
    bool _containsSuspiciousContent(const std::string& content);
    bool _isRateLimited(const std::string& source);
    
    // Pattern matching
    std::vector<std::regex> _suspiciousPatterns;
    std::vector<std::regex> _maliciousPatterns;
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