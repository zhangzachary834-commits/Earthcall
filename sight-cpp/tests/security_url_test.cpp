// URL gate test.
//
// The whitelist is the only thing standing between an embedded WebView and the
// open internet, so it has to match on the HOST — not on a raw string prefix.
// A prefix test says yes to "https://trusted.earthcall.com.attacker.test/",
// which is a domain the attacker owns outright.

#include "Integration/SecurityManager.hpp"

#include <cassert>
#include <cstdio>

using Integration::SecurityConfig;
using Integration::SecurityManager;

int main() {
    auto& security = SecurityManager::instance();

    SecurityConfig config;
    config.whitelistedDomains = {"https://trusted.earthcall.com"};
    security.setConfig(config);

    // The host itself, and genuine subdomains of it.
    assert(security.isURLWhitelisted("https://trusted.earthcall.com"));
    assert(security.isURLWhitelisted("https://trusted.earthcall.com/apps/index.html"));
    assert(security.isURLWhitelisted("https://trusted.earthcall.com:8443/x"));
    assert(security.isURLWhitelisted("https://cdn.trusted.earthcall.com/lib.js"));
    std::printf("  allow:  exact host, paths, ports, subdomains\n");

    // Suffix-glued lookalikes: the classic prefix-match bypass.
    assert(!security.isURLWhitelisted("https://trusted.earthcall.com.attacker.test/"));
    assert(!security.isURLWhitelisted("https://trusted.earthcall.commercial.test/"));
    // Userinfo smuggling: the real host is after the '@'.
    assert(!security.isURLWhitelisted("https://trusted.earthcall.com@attacker.test/"));
    // The whitelisted name appearing only in the path proves nothing.
    assert(!security.isURLWhitelisted("https://attacker.test/https://trusted.earthcall.com"));
    // A different host entirely.
    assert(!security.isURLWhitelisted("https://earthcall.com/"));
    std::printf("  deny:   suffix lookalikes, userinfo smuggling, path echoes\n");

    // An entry written as a bare domain works the same way.
    SecurityConfig bare;
    bare.whitelistedDomains = {"trusted.earthcall.com"};
    security.setConfig(bare);
    assert(security.isURLWhitelisted("https://trusted.earthcall.com/x"));
    assert(!security.isURLWhitelisted("https://trusted.earthcall.com.attacker.test/"));
    std::printf("  bare:   scheme-less whitelist entries match by host too\n");

    // No whitelist means no host restriction (the documented default).
    SecurityConfig open;
    open.whitelistedDomains.clear();
    security.setConfig(open);
    assert(security.isURLWhitelisted("https://anything.test/"));
    std::printf("  empty:  no whitelist leaves the gate open, as documented\n");

    std::printf("security_url_test: ALL OK\n");
    return 0;
}
