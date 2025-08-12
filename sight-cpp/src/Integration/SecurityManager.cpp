#include "Integration/SecurityManager.hpp"
#include "Util/SaveSystem.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>

namespace Integration {

// SecurityEvent serialization
nlohmann::json SecurityEvent::serialize() const {
    nlohmann::json j;
    j["type"] = static_cast<int>(type);
    j["description"] = description;
    j["source"] = source;
    j["details"] = details;
    j["timestamp"] = timestamp;
    j["blocked"] = blocked;
    return j;
}

void SecurityEvent::deserialize(const nlohmann::json& j) {
    type = static_cast<SecurityEventType>(j.value("type", 0));
    description = j.value("description", "");
    source = j.value("source", "");
    details = j.value("details", "");
    timestamp = j.value("timestamp", std::time(nullptr));
    blocked = j.value("blocked", false);
}

// SecurityConfig serialization
nlohmann::json SecurityConfig::serialize() const {
    nlohmann::json j;
    j["level"] = static_cast<int>(level);
    j["whitelistedDomains"] = whitelistedDomains;
    j["blacklistedDomains"] = blacklistedDomains;
    j["enableCSP"] = enableCSP;
    j["enableSandboxing"] = enableSandboxing;
    j["logAllEvents"] = logAllEvents;
    j["requireUserConfirmation"] = requireUserConfirmation;
    
    // Serialize default permissions
    j["defaultPermissions"] = nlohmann::json::array();
    for (const auto& perm : defaultPermissions) {
        j["defaultPermissions"].push_back(static_cast<int>(perm));
    }
    
    return j;
}

void SecurityConfig::deserialize(const nlohmann::json& j) {
    level = static_cast<SecurityLevel>(j.value("level", static_cast<int>(SecurityLevel::MEDIUM)));
    whitelistedDomains = j.value("whitelistedDomains", std::vector<std::string>());
    blacklistedDomains = j.value("blacklistedDomains", std::vector<std::string>());
    enableCSP = j.value("enableCSP", true);
    enableSandboxing = j.value("enableSandboxing", true);
    logAllEvents = j.value("logAllEvents", true);
    requireUserConfirmation = j.value("requireUserConfirmation", true);
    
    // Deserialize default permissions
    defaultPermissions.clear();
    if (j.contains("defaultPermissions") && j["defaultPermissions"].is_array()) {
        for (const auto& perm : j["defaultPermissions"]) {
            defaultPermissions.insert(static_cast<PermissionType>(perm.get<int>()));
        }
    }
}

// SecurityManager implementation
SecurityManager& SecurityManager::instance() {
    static SecurityManager s_instance;
    return s_instance;
}

void SecurityManager::setSecurityLevel(SecurityLevel level) {
    _config.level = level;
    
    // Configure based on security level
    switch (level) {
        case SecurityLevel::LOW:
            _config.enableCSP = false;
            _config.enableSandboxing = false;
            _config.requireUserConfirmation = false;
            break;
        case SecurityLevel::MEDIUM:
            _config.enableCSP = true;
            _config.enableSandboxing = true;
            _config.requireUserConfirmation = true;
            break;
        case SecurityLevel::HIGH:
            _config.enableCSP = true;
            _config.enableSandboxing = true;
            _config.requireUserConfirmation = true;
            _config.logAllEvents = true;
            break;
        case SecurityLevel::PARANOID:
            _config.enableCSP = true;
            _config.enableSandboxing = true;
            _config.requireUserConfirmation = true;
            _config.logAllEvents = true;
            // Add strict domain restrictions
            if (_config.whitelistedDomains.empty()) {
                _config.whitelistedDomains = {"https://trusted.earthcall.com"};
            }
            break;
    }
    
    _initializePatterns();
    logEvent(SecurityEventType::SUSPICIOUS_ACTIVITY, "Security level changed", "system", 
             "Level set to " + std::to_string(static_cast<int>(level)));
}

void SecurityManager::setConfig(const SecurityConfig& config) {
    _config = config;
    _initializePatterns();
    logEvent(SecurityEventType::SUSPICIOUS_ACTIVITY, "Security configuration updated", "system");
}

URLValidationResult SecurityManager::validateURL(const std::string& url) {
    URLValidationResult result;
    result.isValid = false;
    result.isWhitelisted = false;
    
    // Check if source is blocked
    if (isSourceBlocked(url)) {
        result.reason = "Source is blocked due to suspicious activity";
        logEvent(SecurityEventType::URL_ACCESS, "Blocked URL access", url, result.reason, true);
        return result;
    }
    
    // Basic format validation
    if (!_isValidURLFormat(url)) {
        result.reason = "Invalid URL format";
        logEvent(SecurityEventType::URL_ACCESS, "Invalid URL format", url, result.reason, true);
        return result;
    }
    
    // Check for local file access (blocked for security)
    if (_isLocalFile(url)) {
        result.reason = "Local file access is not allowed for security reasons";
        logEvent(SecurityEventType::URL_ACCESS, "Local file access blocked", url, result.reason, true);
        return result;
    }
    
    // Check for secure protocol
    if (!_isSecureProtocol(url)) {
        result.reason = "Only HTTPS URLs are allowed for security";
        logEvent(SecurityEventType::URL_ACCESS, "Non-HTTPS URL blocked", url, result.reason, true);
        return result;
    }
    
    // Check blacklist
    if (isURLBlacklisted(url)) {
        result.reason = "URL is in blacklist";
        logEvent(SecurityEventType::URL_ACCESS, "Blacklisted URL blocked", url, result.reason, true);
        return result;
    }
    
    // Check whitelist (if enabled)
    if (!_config.whitelistedDomains.empty()) {
        if (!isURLWhitelisted(url)) {
            result.reason = "URL not in whitelist";
            logEvent(SecurityEventType::URL_ACCESS, "Non-whitelisted URL blocked", url, result.reason, true);
            return result;
        }
        result.isWhitelisted = true;
    }
    
    // Rate limiting
    if (_isRateLimited(url)) {
        result.reason = "Rate limit exceeded";
        logEvent(SecurityEventType::URL_ACCESS, "Rate limit exceeded", url, result.reason, true);
        return result;
    }
    
    // Sanitize URL
    result.sanitizedURL = sanitizeURL(url);
    result.isValid = true;
    
    logEvent(SecurityEventType::URL_ACCESS, "URL validated successfully", url);
    _updateRateLimit(url);
    
    return result;
}

bool SecurityManager::isURLWhitelisted(const std::string& url) {
    if (_config.whitelistedDomains.empty()) {
        return true; // No whitelist means all allowed
    }
    
    for (const auto& domain : _config.whitelistedDomains) {
        if (url.find(domain) == 0) {
            return true;
        }
    }
    return false;
}

bool SecurityManager::isURLBlacklisted(const std::string& url) {
    for (const auto& domain : _config.blacklistedDomains) {
        if (url.find(domain) != std::string::npos) {
            return true;
        }
    }
    return false;
}

std::string SecurityManager::sanitizeURL(const std::string& url) {
    std::string sanitized = url;
    
    // Remove any potential script injection
    size_t pos = sanitized.find("javascript:");
    if (pos != std::string::npos) {
        sanitized = sanitized.substr(0, pos);
    }
    
    // Remove data URLs
    pos = sanitized.find("data:");
    if (pos != std::string::npos) {
        sanitized = sanitized.substr(0, pos);
    }
    
    // Ensure it starts with https://
    if (sanitized.find("https://") != 0) {
        sanitized = "https://" + sanitized;
    }
    
    return sanitized;
}

bool SecurityManager::requestPermission(PermissionType permission, const std::string& source) {
    // Check if already granted
    if (hasPermission(permission, source)) {
        return true;
    }
    
    // Check if source is blocked
    if (isSourceBlocked(source)) {
        logEvent(SecurityEventType::PERMISSION_DENIED, "Permission denied - source blocked", source, 
                 "Permission: " + std::to_string(static_cast<int>(permission)), true);
        return false;
    }
    
    // Check default permissions
    if (_config.defaultPermissions.find(permission) != _config.defaultPermissions.end()) {
        grantPermission(permission, source);
        return true;
    }
    
    // Require user confirmation if configured
    if (_config.requireUserConfirmation) {
        if (_permissionCallback) {
            bool granted = _permissionCallback(permission, source);
            if (granted) {
                grantPermission(permission, source);
                return true;
            } else {
                logEvent(SecurityEventType::PERMISSION_DENIED, "Permission denied by user", source,
                         "Permission: " + std::to_string(static_cast<int>(permission)), true);
                return false;
            }
        }
    }
    
    // Default deny
    logEvent(SecurityEventType::PERMISSION_DENIED, "Permission denied - no user confirmation", source,
             "Permission: " + std::to_string(static_cast<int>(permission)), true);
    return false;
}

bool SecurityManager::hasPermission(PermissionType permission, const std::string& source) const {
    auto it = _grantedPermissions.find(source);
    if (it != _grantedPermissions.end()) {
        return it->second.find(permission) != it->second.end();
    }
    return false;
}

void SecurityManager::grantPermission(PermissionType permission, const std::string& source) {
    _grantedPermissions[source].insert(permission);
    logEvent(SecurityEventType::PERMISSION_GRANTED, "Permission granted", source,
             "Permission: " + std::to_string(static_cast<int>(permission)));
}

void SecurityManager::revokePermission(PermissionType permission, const std::string& source) {
    auto it = _grantedPermissions.find(source);
    if (it != _grantedPermissions.end()) {
        it->second.erase(permission);
        if (it->second.empty()) {
            _grantedPermissions.erase(it);
        }
    }
    logEvent(SecurityEventType::PERMISSION_DENIED, "Permission revoked", source,
             "Permission: " + std::to_string(static_cast<int>(permission)));
}

void SecurityManager::revokeAllPermissions(const std::string& source) {
    _grantedPermissions.erase(source);
    logEvent(SecurityEventType::PERMISSION_DENIED, "All permissions revoked", source);
}

std::set<PermissionType> SecurityManager::getGrantedPermissions(const std::string& source) const {
    auto it = _grantedPermissions.find(source);
    if (it != _grantedPermissions.end()) {
        return it->second;
    }
    return std::set<PermissionType>();
}

MessageValidationResult SecurityManager::validateMessage(const std::string& message, const std::string& source) {
    MessageValidationResult result;
    result.isValid = false;
    
    // Check if source is blocked
    if (isSourceBlocked(source)) {
        result.reason = "Source is blocked";
        logEvent(SecurityEventType::INVALID_MESSAGE, "Message blocked - source blocked", source, result.reason, true);
        return result;
    }
    
    // Check for suspicious content
    if (_containsSuspiciousContent(message)) {
        result.reason = "Message contains suspicious content";
        logEvent(SecurityEventType::SUSPICIOUS_ACTIVITY, "Suspicious message detected", source, message, true);
        return result;
    }
    
    // Rate limiting
    if (_isRateLimited(source)) {
        result.reason = "Rate limit exceeded";
        logEvent(SecurityEventType::INVALID_MESSAGE, "Message blocked - rate limit", source, result.reason, true);
        return result;
    }
    
    // Basic JSON validation (if it's supposed to be JSON)
    if (message.find('{') != std::string::npos || message.find('[') != std::string::npos) {
        try {
            nlohmann::json::parse(message);
        } catch (const std::exception& e) {
            result.reason = "Invalid JSON format";
            logEvent(SecurityEventType::INVALID_MESSAGE, "Invalid JSON message", source, message, true);
            return result;
        }
    }
    
    result.sanitizedMessage = message; // Could add more sanitization here
    result.isValid = true;
    
    logEvent(SecurityEventType::API_CALL, "Message validated", source);
    _updateRateLimit(source);
    
    return result;
}

bool SecurityManager::isMessageAllowed(const std::string& message, const std::string& source) {
    return validateMessage(message, source).isValid;
}

std::string SecurityManager::generateCSP(const std::string& source) {
    if (!_config.enableCSP) {
        return "";
    }
    
    std::string csp = "default-src 'self'; ";
    csp += "script-src 'self' 'unsafe-inline'; ";
    csp += "style-src 'self' 'unsafe-inline'; ";
    csp += "img-src 'self' data: https:; ";
    csp += "connect-src 'self' https:; ";
    csp += "frame-src 'none'; ";
    csp += "object-src 'none'; ";
    csp += "base-uri 'self'; ";
    csp += "form-action 'self'; ";
    csp += "frame-ancestors 'none'; ";
    csp += "upgrade-insecure-requests;";
    
    return csp;
}

std::string SecurityManager::generateSandboxPolicy() {
    if (!_config.enableSandboxing) {
        return "";
    }
    
    return "allow-scripts allow-same-origin allow-forms allow-popups";
}

void SecurityManager::logEvent(SecurityEventType type, const std::string& description, 
                               const std::string& source, const std::string& details, bool blocked) {
    SecurityEvent event;
    event.type = type;
    event.description = description;
    event.source = source;
    event.details = details;
    event.timestamp = std::time(nullptr);
    event.blocked = blocked;
    
    _securityLog.push_back(event);
    _sourceActivityCount[source]++;
    
    // Keep log size manageable
    if (_securityLog.size() > 10000) {
        _securityLog.erase(_securityLog.begin(), _securityLog.begin() + 1000);
    }
    
    // Check for suspicious activity
    if (detectSuspiciousActivity(source)) {
        blockSource(source);
        if (_securityAlertCallback) {
            _securityAlertCallback(event);
        }
    }
    
    // Output to console for debugging
    std::cout << "🔒 Security: " << description << " from " << source;
    if (blocked) {
        std::cout << " [BLOCKED]";
    }
    std::cout << std::endl;
}

void SecurityManager::clearSecurityLog() {
    _securityLog.clear();
    _sourceActivityCount.clear();
}

void SecurityManager::exportSecurityLog(const std::string& filename) {
    try {
        std::ofstream file(filename);
        if (file.is_open()) {
            file << "Earthcall Security Log\n";
            std::time_t now = std::time(nullptr);
            file << "Generated: " << std::ctime(&now) << "\n\n";
            
            for (const auto& event : _securityLog) {
                file << std::put_time(std::localtime(&event.timestamp), "%Y-%m-%d %H:%M:%S");
                file << " [" << static_cast<int>(event.type) << "] ";
                file << event.description << " from " << event.source;
                if (event.blocked) {
                    file << " [BLOCKED]";
                }
                if (!event.details.empty()) {
                    file << " - " << event.details;
                }
                file << "\n";
            }
            file.close();
            std::cout << "🔒 Security log exported to: " << filename << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to export security log: " << e.what() << std::endl;
    }
}

bool SecurityManager::detectSuspiciousActivity(const std::string& source) {
    auto it = _sourceActivityCount.find(source);
    if (it != _sourceActivityCount.end()) {
        // Block if more than 100 events in 1 minute
        if (it->second > 100) {
            return true;
        }
    }
    
    // Check for blocked events
    int blockedCount = 0;
    for (const auto& event : _securityLog) {
        if (event.source == source && event.blocked) {
            blockedCount++;
        }
    }
    
    // Block if more than 10 blocked events
    return blockedCount > 10;
}

void SecurityManager::blockSource(const std::string& source) {
    _blockedSources.insert(source);
    logEvent(SecurityEventType::SUSPICIOUS_ACTIVITY, "Source blocked due to suspicious activity", source);
}

void SecurityManager::unblockSource(const std::string& source) {
    _blockedSources.erase(source);
    logEvent(SecurityEventType::SUSPICIOUS_ACTIVITY, "Source unblocked", source);
}

bool SecurityManager::isSourceBlocked(const std::string& source) const {
    return _blockedSources.find(source) != _blockedSources.end();
}

bool SecurityManager::validateAPICall(const std::string& api, const std::string& source) {
    // Map API names to required permissions
    std::map<std::string, PermissionType> apiPermissions = {
        {"brush", PermissionType::BRUSH_SYSTEM},
        {"design", PermissionType::DESIGN_SYSTEM},
        {"avatar", PermissionType::AVATAR_SYSTEM},
        {"world", PermissionType::WORLD_ACCESS},
        {"file", PermissionType::FILE_SYSTEM},
        {"network", PermissionType::NETWORK_ACCESS},
        {"ui", PermissionType::UI_CONTROL},
        {"data", PermissionType::DATA_ACCESS}
    };
    
    for (const auto& [apiPrefix, permission] : apiPermissions) {
        if (api.find(apiPrefix) != std::string::npos) {
            return hasPermission(permission, source);
        }
    }
    
    return false;
}

bool SecurityManager::isAPICallAllowed(const std::string& api, const std::string& source) {
    return validateAPICall(api, source);
}

bool SecurityManager::validateJavaScript(const std::string& script, const std::string& source) {
    // Check for malicious patterns
    for (const auto& pattern : _maliciousPatterns) {
        if (std::regex_search(script, pattern)) {
            logEvent(SecurityEventType::JAVASCRIPT_EXECUTION, "Malicious JavaScript detected", source, script, true);
            return false;
        }
    }
    
    // Check for suspicious patterns
    for (const auto& pattern : _suspiciousPatterns) {
        if (std::regex_search(script, pattern)) {
            logEvent(SecurityEventType::SUSPICIOUS_ACTIVITY, "Suspicious JavaScript detected", source, script);
        }
    }
    
    return true;
}

std::string SecurityManager::sanitizeJavaScript(const std::string& script) {
    std::string sanitized = script;
    
    // Remove potentially dangerous functions
    std::vector<std::string> dangerousFunctions = {
        "eval(", "Function(", "setTimeout(", "setInterval(", 
        "document.write(", "document.writeln(", "innerHTML ="
    };
    
    for (const auto& func : dangerousFunctions) {
        size_t pos = 0;
        while ((pos = sanitized.find(func, pos)) != std::string::npos) {
            sanitized.replace(pos, func.length(), "// BLOCKED: " + func);
            pos += 15; // Length of "// BLOCKED: "
        }
    }
    
    return sanitized;
}

void SecurityManager::saveSecurityData() {
    try {
        nlohmann::json j = serialize();
        std::string filename = SaveSystem::writeJson(j, "security_data", SaveSystem::SaveType::INTEGRATION);
        std::cout << "🔒 Security data saved to: " << filename << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to save security data: " << e.what() << std::endl;
    }
}

void SecurityManager::loadSecurityData() {
    try {
        auto files = SaveSystem::listFiles(SaveSystem::SaveType::INTEGRATION);
        for (const auto& file : files) {
            if (file.find("security_data.json") != std::string::npos) {
                std::ifstream f(file);
                if (f.is_open()) {
                    nlohmann::json j;
                    f >> j;
                    deserialize(j);
                    std::cout << "🔒 Security data loaded from: " << file << std::endl;
                    break;
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to load security data: " << e.what() << std::endl;
    }
}

nlohmann::json SecurityManager::serialize() const {
    nlohmann::json j;
    j["config"] = _config.serialize();
    j["securityLog"] = nlohmann::json::array();
    j["blockedSources"] = nlohmann::json::array();
    j["grantedPermissions"] = nlohmann::json::object();
    
    // Serialize security log
    for (const auto& event : _securityLog) {
        j["securityLog"].push_back(event.serialize());
    }
    
    // Serialize blocked sources
    for (const auto& source : _blockedSources) {
        j["blockedSources"].push_back(source);
    }
    
    // Serialize granted permissions
    for (const auto& [source, permissions] : _grantedPermissions) {
        j["grantedPermissions"][source] = nlohmann::json::array();
        for (const auto& perm : permissions) {
            j["grantedPermissions"][source].push_back(static_cast<int>(perm));
        }
    }
    
    return j;
}

void SecurityManager::deserialize(const nlohmann::json& j) {
    if (j.contains("config")) {
        _config.deserialize(j["config"]);
    }
    
    _securityLog.clear();
    if (j.contains("securityLog") && j["securityLog"].is_array()) {
        for (const auto& eventJson : j["securityLog"]) {
            SecurityEvent event;
            event.deserialize(eventJson);
            _securityLog.push_back(event);
        }
    }
    
    _blockedSources.clear();
    if (j.contains("blockedSources") && j["blockedSources"].is_array()) {
        for (const auto& source : j["blockedSources"]) {
            _blockedSources.insert(source.get<std::string>());
        }
    }
    
    _grantedPermissions.clear();
    if (j.contains("grantedPermissions") && j["grantedPermissions"].is_object()) {
        for (const auto& [source, permissions] : j["grantedPermissions"].items()) {
            if (permissions.is_array()) {
                for (const auto& perm : permissions) {
                    _grantedPermissions[source].insert(static_cast<PermissionType>(perm.get<int>()));
                }
            }
        }
    }
    
    _initializePatterns();
}

int SecurityManager::getBlockedEvents() const {
    int count = 0;
    for (const auto& event : _securityLog) {
        if (event.blocked) {
            count++;
        }
    }
    return count;
}

std::map<std::string, int> SecurityManager::getEventCounts() const {
    std::map<std::string, int> counts;
    for (const auto& event : _securityLog) {
        std::string typeName = std::to_string(static_cast<int>(event.type));
        counts[typeName]++;
    }
    return counts;
}

void SecurityManager::setPermissionCallback(std::function<bool(PermissionType, const std::string&)> callback) {
    _permissionCallback = callback;
}

void SecurityManager::setSecurityAlertCallback(std::function<void(const SecurityEvent&)> callback) {
    _securityAlertCallback = callback;
}

// Private helper methods
bool SecurityManager::_isValidURLFormat(const std::string& url) {
    // Basic URL format validation
    std::regex urlPattern(R"(^https?://[^\s/$.?#].[^\s]*$)");
    return std::regex_match(url, urlPattern);
}

bool SecurityManager::_isSecureProtocol(const std::string& url) {
    return url.find("https://") == 0;
}

bool SecurityManager::_isLocalFile(const std::string& url) {
    return url.find("file://") == 0 || url.find("data:") == 0;
}

bool SecurityManager::_containsSuspiciousContent(const std::string& content) {
    for (const auto& pattern : _suspiciousPatterns) {
        if (std::regex_search(content, pattern)) {
            return true;
        }
    }
    return false;
}

bool SecurityManager::_isRateLimited(const std::string& source) {
    return _checkRateLimit(source);
}

void SecurityManager::_initializePatterns() {
    _suspiciousPatterns.clear();
    _maliciousPatterns.clear();
    
    // Suspicious patterns
    _suspiciousPatterns.push_back(std::regex(R"(<script)", std::regex::icase));
    _suspiciousPatterns.push_back(std::regex(R"(javascript:)", std::regex::icase));
    _suspiciousPatterns.push_back(std::regex(R"(on\w+\s*=)", std::regex::icase));
    _suspiciousPatterns.push_back(std::regex(R"(eval\s*\()", std::regex::icase));
    _suspiciousPatterns.push_back(std::regex(R"(document\.write)", std::regex::icase));
    
    // Malicious patterns
    _maliciousPatterns.push_back(std::regex(R"(<script[^>]*>.*?</script>)", std::regex::icase));
    _maliciousPatterns.push_back(std::regex(R"(javascript:[^;]*;)", std::regex::icase));
    _maliciousPatterns.push_back(std::regex(R"(onload\s*=)", std::regex::icase));
    _maliciousPatterns.push_back(std::regex(R"(onerror\s*=)", std::regex::icase));
    _maliciousPatterns.push_back(std::regex(R"(<iframe)", std::regex::icase));
}

bool SecurityManager::_checkRateLimit(const std::string& source) {
    auto it = _rateLimits.find(source);
    if (it != _rateLimits.end()) {
        std::time_t now = std::time(nullptr);
        if (now - it->second.lastReset > 60) { // Reset every minute
            it->second.count = 0;
            it->second.lastReset = now;
        }
        return it->second.count > 100; // Max 100 requests per minute
    }
    return false;
}

void SecurityManager::_updateRateLimit(const std::string& source) {
    auto& rateLimit = _rateLimits[source];
    rateLimit.count++;
    if (rateLimit.lastReset == 0) {
        rateLimit.lastReset = std::time(nullptr);
    }
}

} // namespace Integration 