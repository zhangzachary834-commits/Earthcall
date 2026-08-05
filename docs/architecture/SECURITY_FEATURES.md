# 🔒 Earthcall Security Features

This document describes the comprehensive security system implemented in Earthcall to protect against web-based attacks and malicious content.

## 🛡️ Security Overview

The Earthcall web integration system now includes multiple layers of security protection:

- **URL Validation & Whitelisting**
- **Permission Management**
- **Content Security Policy (CSP)**
- **JavaScript Sanitization**
- **Rate Limiting**
- **Threat Detection**
- **Security Logging**
- **Sandboxing**

## 🔧 Security Components

### 1. SecurityManager

The central security component that coordinates all security features:

```cpp
auto& security = SecurityManager::instance();
security.setSecurityLevel(SecurityLevel::MEDIUM);
```

**Security Levels:**
- `LOW`: Minimal restrictions (development only)
- `MEDIUM`: Standard restrictions (recommended)
- `HIGH`: Strict restrictions
- `PARANOID`: Maximum security (production)

### 2. URL Validation

All URLs are validated before loading:

```cpp
auto result = security.validateURL(url);
if (!result.isValid) {
    // URL blocked - handle error
}
```

**Protection Features:**
- ✅ HTTPS protocol enforcement
- ✅ Local file access blocking
- ✅ JavaScript/data URL blocking
- ✅ Domain whitelisting/blacklisting
- ✅ URL sanitization
- ✅ Rate limiting

### 3. Permission System

Granular permission control for different system features:

```cpp
// Request permission
bool granted = security.requestPermission(PermissionType::BRUSH_SYSTEM, source);

// Check permission
bool hasAccess = security.hasPermission(PermissionType::FILE_SYSTEM, source);
```

**Available Permissions:**
- `BRUSH_SYSTEM`: Access to brush/art tools
- `DESIGN_SYSTEM`: Access to design tools
- `AVATAR_SYSTEM`: Access to avatar management
- `WORLD_ACCESS`: Access to 3D world modification
- `FILE_SYSTEM`: Access to file operations
- `NETWORK_ACCESS`: Access to network resources
- `UI_CONTROL`: Access to UI modification
- `DATA_ACCESS`: Access to user data

### 4. Content Security Policy

Automatic CSP generation to prevent XSS attacks:

```cpp
std::string csp = security.generateCSP(source);
// Apply to web view
```

**CSP Features:**
- Script source restrictions
- Style source restrictions
- Frame restrictions
- Object source restrictions
- Upgrade insecure requests

### 5. JavaScript Security

JavaScript code is validated and sanitized:

```cpp
// Validate JavaScript
if (!security.validateJavaScript(script, source)) {
    // Script blocked
}

// Sanitize JavaScript
std::string safeScript = security.sanitizeJavaScript(script);
```

**Protection Against:**
- `eval()` function calls
- `document.write()` calls
- `innerHTML` assignments
- `setTimeout()` with strings
- Script injection attempts

### 6. Message Validation

All messages from web content are validated:

```cpp
auto result = security.validateMessage(message, source);
if (!result.isValid) {
    // Message blocked
}
```

**Validation Features:**
- JSON format validation
- Suspicious content detection
- Rate limiting
- Source blocking

### 7. Rate Limiting

Automatic rate limiting to prevent abuse:

- **Default Limit**: 100 requests per minute per source
- **Automatic Blocking**: Sources exceeding limits are temporarily blocked
- **Configurable**: Limits can be adjusted per security level

### 8. Threat Detection

Automatic detection of suspicious activity:

```cpp
if (security.detectSuspiciousActivity(source)) {
    security.blockSource(source);
}
```

**Detection Criteria:**
- High event frequency (>100 events/minute)
- Multiple blocked events (>10 blocked events)
- Suspicious patterns in content
- Malicious JavaScript patterns

### 9. Security Logging

Comprehensive logging of all security events:

```cpp
security.logEvent(SecurityEventType::URL_ACCESS, "URL accessed", source);
```

**Logged Events:**
- URL access attempts
- Permission requests
- API calls
- JavaScript execution
- Suspicious activity
- Blocked content

## 🚀 Usage Examples

### Basic Security Setup

```cpp
#include "Integration/SecurityManager.hpp"

// Initialize security
auto& security = SecurityManager::instance();
security.setSecurityLevel(SecurityLevel::MEDIUM);

// Configure whitelisted domains
SecurityConfig config;
config.whitelistedDomains = {
    "https://trusted.earthcall.com",
    "https://api.earthcall.com"
};
security.setConfig(config);
```

### URL Validation

```cpp
std::string url = "https://example.com";
auto result = security.validateURL(url);

if (result.isValid) {
    // Load the URL safely
    loadWebPage(result.sanitizedURL);
} else {
    std::cout << "URL blocked: " << result.reason << std::endl;
}
```

### Permission Management

```cpp
std::string website = "https://trusted.earthcall.com";

// Request permission
if (security.requestPermission(PermissionType::BRUSH_SYSTEM, website)) {
    // Permission granted - allow brush access
    enableBrushFeatures();
} else {
    // Permission denied - show error
    showPermissionDeniedMessage();
}
```

### JavaScript Security

```cpp
std::string script = "console.log('Hello World');";

// Validate before execution
if (security.validateJavaScript(script, source)) {
    // Execute safely
    executeJavaScript(security.sanitizeJavaScript(script));
} else {
    // Block execution
    logSecurityViolation("JavaScript blocked", source);
}
```

## 🔍 Security Monitoring

### View Security Statistics

```cpp
auto& security = SecurityManager::instance();

std::cout << "Total Events: " << security.getTotalEvents() << std::endl;
std::cout << "Blocked Events: " << security.getBlockedEvents() << std::endl;

auto events = security.getSecurityLog();
for (const auto& event : events) {
    std::cout << event.description << " from " << event.source << std::endl;
}
```

### Export Security Logs

```cpp
security.exportSecurityLog("security_report.txt");
```

### Monitor Recent Events

```cpp
auto events = security.getSecurityLog();
for (auto it = events.rbegin(); it != events.rend() && count < 10; ++it) {
    const auto& event = *it;
    std::cout << (event.blocked ? "[BLOCKED] " : "") 
              << event.description << " from " << event.source << std::endl;
}
```

## 🛠️ Configuration

### Security Configuration

```cpp
SecurityConfig config;

// Set security level
config.level = SecurityLevel::HIGH;

// Configure domains
config.whitelistedDomains = {"https://trusted.earthcall.com"};
config.blacklistedDomains = {"malicious-site.com"};

// Enable features
config.enableCSP = true;
config.enableSandboxing = true;
config.requireUserConfirmation = true;
config.logAllEvents = true;

// Set default permissions
config.defaultPermissions.insert(PermissionType::BRUSH_SYSTEM);

security.setConfig(config);
```

### Custom Permission Callbacks

```cpp
security.setPermissionCallback([](PermissionType perm, const std::string& source) {
    // Show user permission dialog
    return showPermissionDialog(perm, source);
});

security.setSecurityAlertCallback([](const SecurityEvent& event) {
    // Handle security alerts
    showSecurityAlert(event);
});
```

## 🧪 Testing Security

Run the security test to verify all features:

```bash
cd examples
g++ -o security_test security_test.cpp ../src/Integration/SecurityManager.cpp -I../src
./security_test
```

The test will demonstrate:
- URL validation
- Permission system
- Message validation
- JavaScript security
- Rate limiting
- Threat detection
- API security

## 📊 Security Metrics

The system tracks various security metrics:

- **Total Events**: Number of security events logged
- **Blocked Events**: Number of events that were blocked
- **Block Rate**: Percentage of events that were blocked
- **Event Types**: Breakdown by event type
- **Source Activity**: Activity per source

## 🔒 Best Practices

1. **Always use HTTPS**: Only allow HTTPS URLs
2. **Whitelist domains**: Only allow trusted domains
3. **Require user confirmation**: Ask users before granting permissions
4. **Monitor logs**: Regularly check security logs
5. **Update blacklists**: Keep blacklists updated
6. **Use appropriate security levels**: Match security level to environment
7. **Validate all input**: Never trust web content
8. **Rate limit everything**: Prevent abuse
9. **Log suspicious activity**: Monitor for threats
10. **Regular security audits**: Review security settings

## 🚨 Security Alerts

The system will automatically alert you to:

- Suspicious activity patterns
- Rate limit violations
- Permission abuse
- Malicious content detection
- Source blocking events

## 📝 Security Log Format

Security logs include:

```json
{
  "type": 1,
  "description": "URL blocked",
  "source": "malicious-site.com",
  "details": "Non-HTTPS URL blocked",
  "timestamp": 1640995200,
  "blocked": true
}
```

## 🔧 Integration with Web Views

The security system is automatically integrated with:

- **RealWebView**: Native WebKit integration
- **WebApp**: Web application management
- **IntegrationManager**: Main integration coordinator
- **EarthcallAPI**: API access control

All web content is automatically validated and secured without additional code changes.

## ✅ Security Checklist

- [ ] Security level set appropriately
- [ ] Domain whitelist configured
- [ ] Permission system enabled
- [ ] CSP enabled
- [ ] JavaScript validation active
- [ ] Rate limiting configured
- [ ] Threat detection enabled
- [ ] Security logging active
- [ ] User confirmation required
- [ ] Regular security audits scheduled

This comprehensive security system ensures that Earthcall is protected against web-based attacks while maintaining usability for legitimate web integrations. 