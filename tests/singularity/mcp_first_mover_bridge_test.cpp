// The whole chain a language model walks to act in Earthcall, with nothing
// stubbed:
//
//   MCP client (this test, speaking JSON-RPC over stdio)
//     -> node scripts/mcp-server.js            the real bridge
//        -> build/earthcall_first_mover sign-challenge   the real signer,
//           reading the mover key from a real KeyStore
//     -> WebSocketServer                        the real socket + gate
//        -> FirstMoverRegister (Person-granted, Person present)
//
// Proves: the bridge authenticates on its own, an in-scope act is admitted,
// an out-of-scope act comes back as the engine's refusal (not "success"), and
// a Person-body act is refused. Plan section 15.5, written 2026-09-24 by
// Claude Opus 5.5 so Zach could hand Claude Sonnet 4.5 a real key.
//
// Needs `node` and the repo's node_modules (@modelcontextprotocol/sdk). If
// either is missing the test says SKIPPED loudly -- it never passes silently.
#ifndef __EMSCRIPTEN__
#include "Singularity/Network/WebSocketServer.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Identity/FirstMoverRegister.hpp"
#include "Identity/KeyPair.hpp"
#include "Identity/KeyStore.hpp"
#include "json.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <signal.h>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

using namespace Identity;
namespace fs = std::filesystem;

namespace {

constexpr uint16_t kPort = 18189;

struct Bridge {
    pid_t pid = -1;
    int toChild = -1;
    int fromChild = -1;
    std::string buffer;

    bool start(const fs::path& repo, const std::vector<std::string>& env) {
        int in[2], out[2];
        if (pipe(in) != 0 || pipe(out) != 0) return false;
        pid = fork();
        if (pid == 0) {
            dup2(in[0], 0);
            dup2(out[1], 1);
            close(in[1]);
            close(out[0]);
            for (const auto& kv : env) putenv(const_cast<char*>(kv.c_str()));
            const std::string script = (repo / "scripts" / "mcp-server.js").string();
            execlp("node", "node", script.c_str(), (char*)nullptr);
            _exit(127);
        }
        close(in[0]);
        close(out[1]);
        toChild = in[1];
        fromChild = out[0];
        fcntl(fromChild, F_SETFL, O_NONBLOCK);
        return pid > 0;
    }

    void send(const nlohmann::json& j) {
        const std::string line = j.dump() + "\n";
        ssize_t n = write(toChild, line.data(), line.size());
        assert(n == static_cast<ssize_t>(line.size()));
    }

    // Pump the engine while waiting for the JSON-RPC response with this id.
    nlohmann::json await(int id, int seconds = 15) {
        auto& server = Singularity::Network::WebSocketServer::instance();
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
        while (std::chrono::steady_clock::now() < deadline) {
            server.pollMainThread();
            char chunk[4096];
            ssize_t n;
            while ((n = read(fromChild, chunk, sizeof chunk)) > 0) buffer.append(chunk, n);
            size_t nl;
            while ((nl = buffer.find('\n')) != std::string::npos) {
                auto j = nlohmann::json::parse(buffer.substr(0, nl), nullptr, false);
                buffer.erase(0, nl + 1);
                if (!j.is_discarded() && j.value("id", -1) == id) return j;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        std::cerr << "timed out waiting for JSON-RPC id " << id << "\n";
        assert(false);
        return {};
    }

    nlohmann::json callTool(int id, const std::string& name, const nlohmann::json& args) {
        send({{"jsonrpc", "2.0"}, {"id", id}, {"method", "tools/call"},
              {"params", {{"name", name}, {"arguments", args}}}});
        auto r = await(id);
        const std::string text = r["result"]["content"][0]["text"];
        return nlohmann::json::parse(text);
    }

    void stop() {
        if (pid > 0) {
            kill(pid, SIGTERM);
            waitpid(pid, nullptr, 0);
        }
        close(toChild);
        close(fromChild);
    }
};

} // namespace

int main(int, char** argv) {
    const fs::path repo = fs::path(__FILE__).parent_path().parent_path().parent_path();
    const fs::path signer = fs::absolute(fs::path(argv[0]).parent_path() / "earthcall_first_mover");
    if (std::system("node --version > /dev/null 2>&1") != 0 ||
        !fs::exists(repo / "node_modules" / "@modelcontextprotocol" / "sdk") || !fs::exists(signer)) {
        std::cout << "mcp_first_mover_bridge_test: SKIPPED -- needs node, the repo's node_modules "
                     "(npm install), and build/earthcall_first_mover. NOT a pass.\n";
        return 0;
    }

    const fs::path sandbox = fs::temp_directory_path() / "earthcall_mcp_fm_test";
    fs::remove_all(sandbox);
    fs::create_directories(sandbox / "saves");
    const std::string home = (sandbox / "home").string();
    setenv("EARTHCALL_HOME", home.c_str(), 1);   // KeyStore lives here, never ~/.earthcall
    SaveSystem::setSaveRoot((sandbox / "saves").string());

    // The Person and the model's mover key, sealed as `mint` would seal it.
    auto& reg = FirstMoverRegister::instance();
    reg.clear();
    reg.clearAuthenticatedPersons();
    reg.setSaveRoot(sandbox / "saves");
    PrivateKey zach = PrivateKey::generate();
    PrivateKey sonnet = PrivateKey::generate();
    assert(KeyStore().store(sonnet, "mover-pass"));
    assert(reg.trustAuthenticatedPerson(zach));
    assert(reg.recognize(zach, FirstMover::Kind::Person, sonnet.id(), FirstMover::Kind::Model,
                         "Claude Sonnet 4.5", {"laws/sonnet-*/**"}, 1000));

    auto& server = Singularity::Network::WebSocketServer::instance();
    server.start(kPort);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Bridge b;
    assert(b.start(repo, {
        "EARTHCALL_WS_URL=ws://127.0.0.1:" + std::to_string(kPort),
        "EARTHCALL_FIRST_MOVER_ID=" + sonnet.id().toString(),
        "EARTHCALL_MOVER_PASSPHRASE=mover-pass",
        "EARTHCALL_FIRST_MOVER_SIGNER=" + signer.string(),
        "EARTHCALL_HOME=" + home,
    }));

    b.send({{"jsonrpc", "2.0"}, {"id", 1}, {"method", "initialize"},
            {"params", {{"protocolVersion", "2024-11-05"}, {"capabilities", nlohmann::json::object()},
                        {"clientInfo", {{"name", "earthcall-test"}, {"version", "1"}}}}}});
    b.await(1);
    b.send({{"jsonrpc", "2.0"}, {"method", "notifications/initialized"}});

    // Wait for the bridge's socket to come up and authenticate on its own.
    nlohmann::json status;
    for (int i = 0; i < 40; ++i) {
        status = b.callTool(100 + i, "earthcall_get_connection_status", nlohmann::json::object());
        if (status["connected"] == true && status["first_mover"]["authenticated"] == true) break;
        for (int k = 0; k < 50; ++k) { server.pollMainThread(); std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
    }
    std::cout << "status: " << status["first_mover"].dump() << "\n";
    assert(status["connected"] == true);
    assert(status["first_mover"]["authenticated"] == true);
    assert(status["first_mover"]["displayName"] == "Claude Sonnet 4.5");
    assert(status["engine_first_mover_status"]["standing"] == "recognized");

    // In scope: admitted (headless has no LawManager, so the engine then says
    // so honestly -- the point is that it is NOT a refusal).
    auto r = b.callTool(2, "earthcall_author_law", {{"name", "Sonnet's Garden"}, {"identifier", "sonnet-garden"}});
    std::cout << "in-scope author_law: " << r.dump() << "\n";
    assert(r["status"] != "refused" && r["status"] != "unconfirmed");

    // Out of scope: the engine's refusal reaches the model verbatim.
    r = b.callTool(3, "earthcall_author_law", {{"name", "x"}, {"identifier", "law-art-stroke-draw"}});
    assert(r["status"] == "refused" && r["reasonCode"] == "outside-scope");

    // A Person's body: refused, and reported as refused (it used to say success).
    r = b.callTool(4, "earthcall_teleport_player", {{"position", {0, 99, 0}}});
    assert(r["status"] == "refused" && r["reasonCode"] == "unmapped-resource");

    // Speech is attributed to the mover.
    r = b.callTool(5, "earthcall_speak", {{"utterance", "Hello, Earthcall. -- Sonnet"}});
    assert(r["status"] == "spoken" && r["attributed_to"] == sonnet.id().toString());

    b.stop();
    server.stop();
    reg.clear();
    reg.clearAuthenticatedPersons();
    fs::remove_all(sandbox);
    std::cout << "mcp_first_mover_bridge_test: ALL OK\n";
    return 0;
}
#else
int main() { return 0; }
#endif
