#include "../src/Integration/SecurityManager.hpp"
#include <iostream>

using namespace Integration;

int main() {
    std::cout << "🔒 Earthcall Security Test" << std::endl;
    std::cout << "=========================" << std::endl;
    
    // Initialize security manager
    auto& security = SecurityManager::instance();
    security.setSecurityLevel(SecurityLevel::MEDIUM);
    
    // Test 1: URL Validation
    std::cout << "\n1. Testing URL Validation:" << std::endl;
    
    std::vector<std::string> testURLs = {
        "https://trusted.earthcall.com",
        "http://malicious-site.com",
        "file:///etc/passwd",
        "javascript:alert('hack')",
        "data:text/html,<script>alert('hack')</script>",
        "https://api.earthcall.com/safe"
    };
    
    for (const auto& url : testURLs) {
        auto result = security.validateURL(url);
        std::cout << "  " << url << " -> " 
                  << (result.isValid ? "✅ ALLOWED" : "❌ BLOCKED") 
                  << " (" << result.reason << ")" << std::endl;
    }
    
    // Test 2: Permission System
    std::cout << "\n2. Testing Permission System:" << std::endl;
    
    std::string testSource = "test-website.com";
    
    // Try to request permissions
    bool brushPerm = security.requestPermission(PermissionType::BRUSH_SYSTEM, testSource);
    bool filePerm = security.requestPermission(PermissionType::FILE_SYSTEM, testSource);
    
    std::cout << "  Brush System Permission: " << (brushPerm ? "✅ GRANTED" : "❌ DENIED") << std::endl;
    std::cout << "  File System Permission: " << (filePerm ? "✅ GRANTED" : "❌ DENIED") << std::endl;
    
    // Test 3: Message Validation
    std::cout << "\n3. Testing Message Validation:" << std::endl;
    
    std::vector<std::string> testMessages = {
        "{\"type\": \"brush_create\", \"data\": {\"color\": \"red\"}}",
        "<script>alert('hack')</script>",
        "javascript:eval('malicious code')",
        "{\"type\": \"api_call\", \"method\": \"delete_all_files\"}",
        "normal message"
    };
    
    for (const auto& message : testMessages) {
        auto result = security.validateMessage(message, testSource);
        std::cout << "  Message: " << message.substr(0, 30) << "... -> "
                  << (result.isValid ? "✅ VALID" : "❌ BLOCKED")
                  << " (" << result.reason << ")" << std::endl;
    }
    
    // Test 4: JavaScript Validation
    std::cout << "\n4. Testing JavaScript Validation:" << std::endl;
    
    std::vector<std::string> testScripts = {
        "console.log('Hello World');",
        "eval('malicious code');",
        "document.write('<script>alert(1)</script>');",
        "setTimeout(function() { alert('hack'); }, 1000);",
        "document.body.innerHTML = '<h1>Safe content</h1>';"
    };
    
    for (const auto& script : testScripts) {
        bool isValid = security.validateJavaScript(script, testSource);
        std::cout << "  Script: " << script.substr(0, 30) << "... -> "
                  << (isValid ? "✅ SAFE" : "❌ BLOCKED") << std::endl;
    }
    
    // Test 5: Rate Limiting
    std::cout << "\n5. Testing Rate Limiting:" << std::endl;
    
    std::string spamSource = "spam-bot.com";
    int blockedCount = 0;
    
    for (int i = 0; i < 150; i++) {
        auto result = security.validateMessage("spam message", spamSource);
        if (!result.isValid) {
            blockedCount++;
        }
    }
    
    std::cout << "  Sent 150 messages, " << blockedCount << " were blocked by rate limiting" << std::endl;
    
    // Test 6: Security Statistics
    std::cout << "\n6. Security Statistics:" << std::endl;
    
    std::cout << "  Total Events: " << security.getTotalEvents() << std::endl;
    std::cout << "  Blocked Events: " << security.getBlockedEvents() << std::endl;
    
    auto eventCounts = security.getEventCounts();
    std::cout << "  Event Breakdown:" << std::endl;
    for (const auto& [type, count] : eventCounts) {
        std::cout << "    Type " << type << ": " << count << " events" << std::endl;
    }
    
    // Test 7: Content Security Policy
    std::cout << "\n7. Content Security Policy:" << std::endl;
    
    std::string csp = security.generateCSP(testSource);
    std::cout << "  Generated CSP: " << csp << std::endl;
    
    std::string sandbox = security.generateSandboxPolicy();
    std::cout << "  Sandbox Policy: " << sandbox << std::endl;
    
    // Test 8: Threat Detection
    std::cout << "\n8. Threat Detection:" << std::endl;
    
    std::string maliciousSource = "malicious-site.com";
    
    // Simulate suspicious activity
    for (int i = 0; i < 50; i++) {
        security.logEvent(SecurityEventType::SUSPICIOUS_ACTIVITY, "Suspicious behavior", maliciousSource);
    }
    
    bool isSuspicious = security.detectSuspiciousActivity(maliciousSource);
    std::cout << "  Malicious source detected: " << (isSuspicious ? "✅ YES" : "❌ NO") << std::endl;
    
    if (isSuspicious) {
        security.blockSource(maliciousSource);
        std::cout << "  Source has been blocked" << std::endl;
    }
    
    // Test 9: API Security
    std::cout << "\n9. API Security:" << std::endl;
    
    std::vector<std::string> testAPIs = {
        "brush_create",
        "file_delete",
        "world_modify",
        "avatar_control",
        "unknown_api"
    };
    
    for (const auto& api : testAPIs) {
        bool allowed = security.isAPICallAllowed(api, testSource);
        std::cout << "  API '" << api << "': " << (allowed ? "✅ ALLOWED" : "❌ BLOCKED") << std::endl;
    }
    
    // Final Statistics
    std::cout << "\n📊 Final Security Report:" << std::endl;
    std::cout << "=========================" << std::endl;
    std::cout << "Total Security Events: " << security.getTotalEvents() << std::endl;
    std::cout << "Blocked Events: " << security.getBlockedEvents() << std::endl;
    std::cout << "Block Rate: " << (security.getTotalEvents() > 0 ? 
                                   (float)security.getBlockedEvents() / security.getTotalEvents() * 100 : 0) 
                                   << "%" << std::endl;
    
    // Export security log
    security.exportSecurityLog("security_test_log.txt");
    std::cout << "\nSecurity log exported to: security_test_log.txt" << std::endl;
    
    std::cout << "\n✅ Security test completed successfully!" << std::endl;
    return 0;
} 