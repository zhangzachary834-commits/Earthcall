// SecurityManager — regression tests.
//
// This class had no test of any kind, and shipped a defect that killed the
// process: logEvent() called blockSource(), blockSource() called logEvent(),
// and nothing in the cycle lowered the count that entered it, so the 101st
// event from any source recursed until the stack was gone. The first test
// below is that bug. The rest cover the checks that were wrong in ways a
// reader would not notice, because they returned plausible answers.

#include "Integration/SecurityManager.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

using Integration::PermissionType;
using Integration::SecurityConfig;
using Integration::SecurityEventType;
using Integration::SecurityLevel;
using Integration::SecurityManager;

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool cond, const std::string& what) {
    ++g_checks;
    if (cond) return;
    ++g_failures;
    std::cerr << "  FAIL: " << what << "\n";
}

// The manager is a singleton and logs loudly to stdout. Reset it between
// sections and keep the console readable.
struct Quiet {
    std::streambuf* saved;
    std::ostringstream sink;
    Quiet() : saved(std::cout.rdbuf(sink.rdbuf())) {}
    ~Quiet() { std::cout.rdbuf(saved); }
};

void reset(SecurityManager& sm, const std::string& source) {
    sm.unblockSource(source);
    sm.clearSecurityLog();
}

// ---------------------------------------------------------------------------

void testLoggingTerminates() {
    std::cout << "logEvent does not recurse\n";
    SecurityManager& sm = SecurityManager::instance();
    const std::string source = "https://example.com";
    Quiet quiet;
    reset(sm, source);

    // Pre-fix this segfaulted on iteration ~101 with a 40,000-frame stack of
    // logEvent -> blockSource -> logEvent. Reaching the assertion at all is
    // most of the test.
    for (int i = 0; i < 500; ++i) {
        sm.logEvent(SecurityEventType::API_CALL, "ping", source);
    }

    check(sm.getTotalEvents() > 0, "events were recorded");
    reset(sm, source);
}

void testActivityWindowRolls() {
    std::cout << "activity count is windowed, not cumulative\n";
    SecurityManager& sm = SecurityManager::instance();
    const std::string source = "https://steady.example";
    Quiet quiet;
    reset(sm, source);

    // A 1-second window so the roll-over is observable; the real policy is 60s.
    SecurityConfig cfg;
    cfg.activityWindowSeconds = 1;
    cfg.maxEventsPerWindow = 10;
    sm.setConfig(cfg);
    reset(sm, source);

    // Three bursts, each under the threshold, separated by more than one
    // window. Cumulatively this is 27 events against a limit of 10 — under the
    // old monotonic counter the source would have been condemned during the
    // second burst and could never recover. Windowed, it is never suspicious.
    for (int burst = 0; burst < 3; ++burst) {
        for (int i = 0; i < 9; ++i) {
            sm.logEvent(SecurityEventType::API_CALL, "tick", source);
        }
        check(!sm.isSourceBlocked(source),
              "a source under the per-window threshold stays unblocked across windows");
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    }

    // And the threshold still bites when it is genuinely exceeded in one window.
    for (int i = 0; i < 40; ++i) {
        sm.logEvent(SecurityEventType::API_CALL, "flood", source);
    }
    check(sm.isSourceBlocked(source), "a burst over the per-window threshold IS blocked");

    reset(sm, source);
    sm.setConfig(SecurityConfig{});
}

void testUnblockActuallyUnblocks() {
    std::cout << "unblockSource clears what caused the block\n";
    SecurityManager& sm = SecurityManager::instance();
    const std::string source = "https://noisy.example";
    Quiet quiet;
    reset(sm, source);

    sm.blockSource(source);
    check(sm.isSourceBlocked(source), "blockSource blocks");

    sm.unblockSource(source);
    check(!sm.isSourceBlocked(source), "unblockSource unblocks and it sticks");
    reset(sm, source);
}

void testWhitelistIsHostBased() {
    std::cout << "whitelist matches hosts, not string prefixes\n";
    SecurityManager& sm = SecurityManager::instance();
    Quiet quiet;

    SecurityConfig cfg;
    cfg.level = SecurityLevel::HIGH;
    cfg.whitelistedDomains = {"earthcall.com"};
    sm.setConfig(cfg);

    check(sm.isURLWhitelisted("https://earthcall.com/index.html"), "exact host matches");
    check(sm.isURLWhitelisted("https://app.earthcall.com/x"), "subdomain matches");
    check(sm.isURLWhitelisted("https://EarthCall.COM/x"), "host comparison is case-insensitive");
    check(sm.isURLWhitelisted("https://earthcall.com.:443/x"), "trailing dot and port are ignored");

    // Every one of these passed the old url.find(domain) == 0 prefix test.
    check(!sm.isURLWhitelisted("https://earthcall.com.attacker.test/"),
          "attacker domain that merely STARTS with ours is rejected");
    check(!sm.isURLWhitelisted("https://earthcall.com-evil.test/"),
          "adjacent-label lookalike is rejected");
    check(!sm.isURLWhitelisted("https://notearthcall.com/"),
          "suffix without a label boundary is rejected");
    check(!sm.isURLWhitelisted("https://earthcall.com@attacker.test/"),
          "userinfo confusion resolves to the real host and is rejected");

    // Configs written before host matching stored whole URLs; both spellings
    // must keep working.
    cfg.whitelistedDomains = {"https://earthcall.com"};
    sm.setConfig(cfg);
    check(sm.isURLWhitelisted("https://app.earthcall.com/x"), "URL-form whitelist entry still matches");
    check(!sm.isURLWhitelisted("https://earthcall.com.attacker.test/"),
          "URL-form whitelist entry rejects the lookalike too");

    sm.setConfig(SecurityConfig{});
}

void testBlacklistIsHostBased() {
    std::cout << "blacklist matches hosts, not substrings\n";
    SecurityManager& sm = SecurityManager::instance();
    Quiet quiet;

    SecurityConfig cfg;
    cfg.blacklistedDomains = {"evil.test"};
    sm.setConfig(cfg);

    check(sm.isURLBlacklisted("https://evil.test/path"), "exact host is blacklisted");
    check(sm.isURLBlacklisted("https://cdn.evil.test/x"), "subdomain is blacklisted");
    check(sm.isURLBlacklisted("https://evil.test./x"), "trailing dot does not evade the blacklist");
    check(!sm.isURLBlacklisted("https://good.example/?ref=evil.test"),
          "a blacklisted name in the query does not block an innocent host");

    sm.setConfig(SecurityConfig{});
}

void testValidateURL() {
    std::cout << "validateURL rejects the obvious\n";
    SecurityManager& sm = SecurityManager::instance();
    Quiet quiet;
    sm.setConfig(SecurityConfig{});
    sm.clearSecurityLog();

    check(!sm.validateURL("http://plain.example/").isValid, "plain HTTP is rejected");
    check(!sm.validateURL("file:///etc/passwd").isValid, "file:// is rejected");
    check(!sm.validateURL("not a url").isValid, "malformed input is rejected");
    check(sm.validateURL("https://ok.example/page").isValid, "a well-formed HTTPS URL passes");
}

void testPermissionsDefaultDeny() {
    std::cout << "permissions default to deny\n";
    SecurityManager& sm = SecurityManager::instance();
    const std::string source = "https://asker.example";
    Quiet quiet;
    reset(sm, source);

    SecurityConfig cfg;
    cfg.requireUserConfirmation = true;
    sm.setConfig(cfg);
    sm.revokeAllPermissions(source);

    // No callback installed, nothing in defaultPermissions: must deny.
    check(!sm.requestPermission(PermissionType::FILE_SYSTEM, source),
          "request without a confirmation callback is denied");
    check(!sm.hasPermission(PermissionType::FILE_SYSTEM, source), "and nothing was granted");

    sm.grantPermission(PermissionType::FILE_SYSTEM, source);
    check(sm.hasPermission(PermissionType::FILE_SYSTEM, source), "explicit grant is honoured");
    sm.revokePermission(PermissionType::FILE_SYSTEM, source);
    check(!sm.hasPermission(PermissionType::FILE_SYSTEM, source), "revoke takes it back");

    reset(sm, source);
    sm.setConfig(SecurityConfig{});
}

void testGeneratedPolicies() {
    std::cout << "CSP and sandbox policies do not negate themselves\n";
    SecurityManager& sm = SecurityManager::instance();
    Quiet quiet;
    sm.setSecurityLevel(SecurityLevel::HIGH);

    const std::string csp = sm.generateCSP("https://any.example");
    check(csp.find("script-src 'self';") != std::string::npos, "script-src is present");
    check(csp.find("'unsafe-inline'; img-src") != std::string::npos ||
              csp.find("script-src 'self' 'unsafe-inline'") == std::string::npos,
          "script-src does not carry 'unsafe-inline'");
    check(csp.find("object-src 'none'") != std::string::npos, "object-src is locked down");

    const std::string sandbox = sm.generateSandboxPolicy();
    check(sandbox.find("allow-scripts") != std::string::npos, "scripts are allowed in the frame");
    check(sandbox.find("allow-same-origin") == std::string::npos,
          "allow-same-origin is absent — with allow-scripts it defeats the sandbox");

    sm.setConfig(SecurityConfig{});
}

void testJavaScriptScreening() {
    std::cout << "validateJavaScript screens JavaScript, not HTML\n";
    SecurityManager& sm = SecurityManager::instance();
    const std::string source = "https://page.example";
    Quiet quiet;
    reset(sm, source);

    check(sm.validateJavaScript("document.title = 'hello';", source),
          "ordinary first-party script passes");

    // These are the payloads the old HTML-pattern check waved through.
    check(!sm.validateJavaScript("eval(payload)", source), "eval( is blocked");
    check(!sm.validateJavaScript("new Function(payload)()", source), "new Function( is blocked");
    check(!sm.validateJavaScript("x = document.cookie", source), "document.cookie is blocked");
    check(!sm.validateJavaScript("localStorage.clear()", source), "storage access is blocked");

    reset(sm, source);
}

void testMessageValidation() {
    std::cout << "validateMessage rejects malformed JSON and script injection\n";
    SecurityManager& sm = SecurityManager::instance();
    const std::string source = "https://sender.example";
    Quiet quiet;
    reset(sm, source);
    sm.setConfig(SecurityConfig{});

    check(sm.validateMessage(R"({"ok":true})", source).isValid, "well-formed JSON passes");
    check(!sm.validateMessage(R"({"ok":)", source).isValid, "truncated JSON is rejected");

    // The multi-line case: '.' does not match a newline in std::regex, so the
    // old "<script[^>]*>.*?</script>" could be defeated by pressing Enter.
    check(!sm.validateMessage("<script>\nsteal()\n</script>", source).isValid,
          "multi-line script block is rejected");

    reset(sm, source);
}

} // namespace

int main() {
    std::cout << "=== SecurityManager tests ===\n";

    testLoggingTerminates();
    testActivityWindowRolls();
    testUnblockActuallyUnblocks();
    testWhitelistIsHostBased();
    testBlacklistIsHostBased();
    testValidateURL();
    testPermissionsDefaultDeny();
    testGeneratedPolicies();
    testJavaScriptScreening();
    testMessageValidation();

    std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cerr << g_failures << " FAILED\n";
        return 1;
    }
    std::cout << "All SecurityManager tests passed.\n";
    return 0;
}
