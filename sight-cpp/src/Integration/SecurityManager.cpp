#include "Integration/SecurityManager.hpp"
#include "Util/SaveSystem.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
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
    j["activityWindowSeconds"] = activityWindowSeconds;
    j["maxEventsPerWindow"] = maxEventsPerWindow;
    j["maxBlockedEvents"] = maxBlockedEvents;

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
    // Saves predating these fields get the standard policy.
    activityWindowSeconds = j.value("activityWindowSeconds", 60);
    maxEventsPerWindow = j.value("maxEventsPerWindow", 100);
    maxBlockedEvents = j.value("maxBlockedEvents", 10);

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

    // Compare HOSTS, never raw URL text. A prefix test on the whole URL let
    // "https://trusted.example.attacker.test/" pass a whitelist of
    // "https://trusted.example" — the attacker picks a domain that starts
    // with yours and walks straight through.
    const std::string host = _extractHost(url);
    if (host.empty()) return false;

    for (const auto& domain : _config.whitelistedDomains) {
        if (_hostMatchesDomain(host, domain)) {
            return true;
        }
    }
    return false;
}

bool SecurityManager::isURLBlacklisted(const std::string& url) {
    // Host comparison here too: a substring test blocked innocent URLs that
    // merely mentioned a blacklisted domain (in a query parameter, say) while
    // a trailing dot was enough to evade it.
    const std::string host = _extractHost(url);
    if (host.empty()) return false;

    for (const auto& domain : _config.blacklistedDomains) {
        if (_hostMatchesDomain(host, domain)) {
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
    
    // Basic JSON validation (if it's supposed to be JSON). accept() reports
    // well-formedness without building the document or throwing — we only ever
    // wanted the verdict, and parse() forced us to discard a whole DOM to get it.
    if (message.find('{') != std::string::npos || message.find('[') != std::string::npos) {
        if (!nlohmann::json::accept(message)) {
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

std::string SecurityManager::generateCSP(const std::string& /*source*/) {
    // The policy is the same for every source today; the parameter is kept so
    // per-origin policies can land without touching every call site.
    if (!_config.enableCSP) {
        return "";
    }
    
    // No 'unsafe-inline' in script-src. It used to be here, and it re-permits
    // exactly the inline-script injection the policy exists to stop — with it
    // present the directive bought almost nothing. Inline STYLE is left
    // permitted: it is a far smaller exposure and the embedded pages rely on it.
    std::string csp = "default-src 'self'; ";
    csp += "script-src 'self'; ";
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
    
    // allow-same-origin is deliberately absent. Paired with allow-scripts it
    // is self-defeating: a frame granted both can reach into its parent's
    // origin and strip its own sandbox attribute, so the pair sandboxes
    // nothing. allow-popups goes too — a popup escapes the sandbox entirely
    // unless allow-popups-to-escape-sandbox is withheld, and we have no need
    // to open one.
    return "allow-scripts allow-forms";
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
    if (blocked) _blockedEventCount[source]++;

    // Roll the activity window forward before counting this event, so the
    // threshold means "maxEventsPerWindow events in activityWindowSeconds"
    // — which is what the policy has always claimed.
    // ">=", not ">": an event landing exactly activityWindowSeconds after the
    // window opened is outside a window of that length. std::time_t is whole
    // seconds, so the distinction is real at small window sizes.
    std::time_t& windowStart = _activityWindowStart[source];
    if (windowStart == 0 ||
        event.timestamp - windowStart >= static_cast<std::time_t>(_config.activityWindowSeconds)) {
        windowStart = event.timestamp;
        _sourceActivityCount[source] = 0;
    }
    _sourceActivityCount[source]++;

    // Keep log size manageable. The per-source blocked tallies are maintained
    // incrementally, so anything dropped here has to be subtracted back out.
    if (_securityLog.size() > kMaxLogEntries) {
        auto last = _securityLog.begin() + static_cast<std::ptrdiff_t>(kLogTrimCount);
        for (auto it = _securityLog.begin(); it != last; ++it) {
            if (!it->blocked) continue;
            auto tally = _blockedEventCount.find(it->source);
            if (tally != _blockedEventCount.end() && --tally->second <= 0) {
                _blockedEventCount.erase(tally);
            }
        }
        _securityLog.erase(_securityLog.begin(), last);
    }

    // Check for suspicious activity. blockSource() logs, and logging lands
    // back here — it is only safe to call because blockSource returns without
    // logging when the source is already blocked. Do not remove that guard:
    // nothing in this cycle lowers the activity count, so re-entry would
    // recurse until the stack is exhausted.
    if (!isSourceBlocked(source) && detectSuspiciousActivity(source)) {
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
    _activityWindowStart.clear();
    _blockedEventCount.clear();
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
    // Too many events inside the current rolling window.
    auto it = _sourceActivityCount.find(source);
    if (it != _sourceActivityCount.end() && it->second > _config.maxEventsPerWindow) {
        return true;
    }

    // Or too many events we actually refused. Read from the running tally
    // rather than rescanning the log.
    auto blocked = _blockedEventCount.find(source);
    return blocked != _blockedEventCount.end() && blocked->second > _config.maxBlockedEvents;
}

void SecurityManager::blockSource(const std::string& source) {
    // Already blocked: return WITHOUT logging. logEvent calls back into
    // blockSource when a source looks suspicious, and nothing in that cycle
    // lowers the activity count — so re-logging here recurses forever and
    // exhausts the stack. This early return is what terminates it.
    if (!_blockedSources.insert(source).second) return;
    logEvent(SecurityEventType::SUSPICIOUS_ACTIVITY, "Source blocked due to suspicious activity", source);
}

void SecurityManager::unblockSource(const std::string& source) {
    if (_blockedSources.erase(source) == 0) return;
    // Clear what got it blocked. Otherwise the very next logEvent — including
    // the one below — sees the old counts, decides the source is still
    // suspicious, and blocks it straight back, making unblocking a no-op.
    _sourceActivityCount.erase(source);
    _activityWindowStart.erase(source);
    _blockedEventCount.erase(source);
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
    // Tripwire, not a boundary — see the note on the declaration. The scripts
    // reaching here are the ones Earthcall itself injects, so a hit means
    // something upstream is building a script it should not be.
    for (const auto& pattern : _jsHighRiskPatterns) {
        if (std::regex_search(script, pattern)) {
            logEvent(SecurityEventType::JAVASCRIPT_EXECUTION, "High-risk JavaScript blocked", source, script, true);
            return false;
        }
    }

    for (const auto& pattern : _jsNoteworthyPatterns) {
        if (std::regex_search(script, pattern)) {
            logEvent(SecurityEventType::SUSPICIOUS_ACTIVITY, "Noteworthy JavaScript construct", source, script);
        }
    }

    return true;
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

std::string SecurityManager::_extractHost(const std::string& url) {
    const std::size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) return "";

    // The authority runs from after "://" to the first '/', '?' or '#'.
    const std::size_t authStart = schemeEnd + 3;
    std::size_t authEnd = url.size();
    for (std::size_t i = authStart; i < url.size(); ++i) {
        if (url[i] == '/' || url[i] == '?' || url[i] == '#') { authEnd = i; break; }
    }
    std::string authority = url.substr(authStart, authEnd - authStart);

    // Drop "user:pass@". The LAST '@' wins: "https://good.example@evil.test/"
    // is a request to evil.test, and taking the first '@' would read it as
    // good.example — the classic userinfo confusion.
    const std::size_t at = authority.rfind('@');
    if (at != std::string::npos) authority = authority.substr(at + 1);

    // Drop ":port". Guard against IPv6 literals, whose brackets contain colons.
    if (!authority.empty() && authority.front() == '[') {
        const std::size_t close = authority.find(']');
        if (close != std::string::npos) authority = authority.substr(0, close + 1);
    } else {
        const std::size_t colon = authority.find(':');
        if (colon != std::string::npos) authority = authority.substr(0, colon);
    }

    // A trailing dot is a fully-qualified name and still resolves, so
    // "evil.test." and "evil.test" must compare equal.
    while (!authority.empty() && authority.back() == '.') authority.pop_back();

    std::transform(authority.begin(), authority.end(), authority.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return authority;
}

bool SecurityManager::_hostMatchesDomain(const std::string& host, const std::string& domain) {
    // Configs predating host matching stored whole URLs
    // ("https://trusted.earthcall.com"); accept both spellings.
    std::string d = domain.find("://") != std::string::npos ? _extractHost(domain) : domain;
    while (!d.empty() && d.back() == '.') d.pop_back();
    std::transform(d.begin(), d.end(), d.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (d.empty() || host.empty()) return false;

    if (host == d) return true;
    // Subdomains only, and only on a label boundary: "app.earthcall.com" is
    // under "earthcall.com"; "notearthcall.com" and
    // "earthcall.com.attacker.test" are not.
    return host.size() > d.size() &&
           host.compare(host.size() - d.size(), d.size(), d) == 0 &&
           host[host.size() - d.size() - 1] == '.';
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
    _jsHighRiskPatterns.clear();
    _jsNoteworthyPatterns.clear();

    // ---- HTML/markup patterns, for message bodies -------------------------
    // Note [\s\S] rather than '.': in std::regex's ECMAScript grammar '.' does
    // not match a newline, so ".*?" could not span a multi-line <script> block
    // and the check was trivially sidestepped by pressing Enter.
    _suspiciousPatterns.push_back(std::regex(R"(<script)", std::regex::icase));
    _suspiciousPatterns.push_back(std::regex(R"(javascript:)", std::regex::icase));
    _suspiciousPatterns.push_back(std::regex(R"(on\w+\s*=)", std::regex::icase));
    _suspiciousPatterns.push_back(std::regex(R"(eval\s*\()", std::regex::icase));
    _suspiciousPatterns.push_back(std::regex(R"(document\.write)", std::regex::icase));

    _maliciousPatterns.push_back(std::regex(R"(<script[^>]*>[\s\S]*?</script>)", std::regex::icase));
    _maliciousPatterns.push_back(std::regex(R"(javascript:[^;]*;)", std::regex::icase));
    _maliciousPatterns.push_back(std::regex(R"(onload\s*=)", std::regex::icase));
    _maliciousPatterns.push_back(std::regex(R"(onerror\s*=)", std::regex::icase));
    _maliciousPatterns.push_back(std::regex(R"(<iframe)", std::regex::icase));

    // ---- JavaScript patterns, for validateJavaScript -----------------------
    // The old code fed the HTML patterns above to validateJavaScript, so it was
    // scanning JavaScript source for "<script>" and "<iframe" tags and passing
    // essentially every real payload. These are at least about the language.
    // They still only catch the literal spelling — see the header note.
    _jsHighRiskPatterns.push_back(std::regex(R"(\beval\s*\()", std::regex::icase));
    _jsHighRiskPatterns.push_back(std::regex(R"(\bnew\s+Function\s*\()", std::regex::icase));
    _jsHighRiskPatterns.push_back(std::regex(R"(\bdocument\s*\.\s*cookie\b)", std::regex::icase));
    _jsHighRiskPatterns.push_back(std::regex(R"(\b(localStorage|sessionStorage|indexedDB)\b)", std::regex::icase));
    _jsHighRiskPatterns.push_back(std::regex(R"(\bdocument\s*\.\s*write(ln)?\s*\()", std::regex::icase));
    _jsHighRiskPatterns.push_back(std::regex(R"(\bwindow\s*\.\s*(open|top|parent|opener)\b)", std::regex::icase));

    _jsNoteworthyPatterns.push_back(std::regex(R"(\b(fetch|XMLHttpRequest|WebSocket|sendBeacon)\b)", std::regex::icase));
    _jsNoteworthyPatterns.push_back(std::regex(R"(\b(setTimeout|setInterval)\s*\(\s*['"])", std::regex::icase));
    _jsNoteworthyPatterns.push_back(std::regex(R"(\.innerHTML\s*=)", std::regex::icase));
    _jsNoteworthyPatterns.push_back(std::regex(R"(\bimport\s*\()", std::regex::icase));
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