#include "CloudStorage.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <mutex>
#include <sstream>
// Define httplib implementation in exactly one compilation unit.
// If it's used elsewhere, we'd need a separate httplib.cpp, but for now we define it here.
#include "../../third_party/httplib/httplib.h"

namespace Util {

static std::string g_endpoint = "https://localhost:8080";
static std::string g_authToken = "";
static std::mutex g_configMutex;

// httplib::Client(url) throws std::invalid_argument for https unless
// CPPHTTPLIB_OPENSSL_SUPPORT is defined at compile time. This build does not
// enable that, so Save As used to abort the process on the upload thread.
static std::atomic<bool> g_httpsUnsupportedWarned{false};

static bool endpointUsesHttps(const std::string& ep) {
    return ep.size() >= 8 &&
           (ep.compare(0, 8, "https://") == 0 || ep.compare(0, 8, "HTTPS://") == 0);
}

static bool cloudTlsAvailable() {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    return true;
#else
    return false;
#endif
}

static bool canOpenClient(const std::string& ep) {
    if (endpointUsesHttps(ep) && !cloudTlsAvailable()) {
        if (!g_httpsUnsupportedWarned.exchange(true)) {
            std::cerr << "[CloudStorage] Endpoint '" << ep
                      << "' uses https, but this httplib build has no TLS. "
                         "Cloud sync is skipped; the save stays local. "
                         "Set a http:// endpoint or rebuild with "
                         "CPPHTTPLIB_OPENSSL_SUPPORT.\n";
        }
        return false;
    }
    return true;
}

static std::string urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (char c : value) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << std::uppercase << '%' << std::setw(2)
                    << int(static_cast<unsigned char>(c)) << std::nouppercase;
        }
    }
    return escaped.str();
}

#ifdef __EMSCRIPTEN__
// No HTTP client is wired for wasm (httplib is native-only above), so every
// call below fails synchronously by construction -- not a transient network
// problem, a fixed fact about this build. Say it once per session rather
// than repeat it on every save/load/metadata fetch.
static std::atomic<bool> g_wasmCloudWarned{false};
static void warnWasmCloudUnavailable() {
    if (!g_wasmCloudWarned.exchange(true)) {
        std::cerr << "[CloudStorage] Cloud sync is not available in the browser "
                     "build (no HTTP client wired for wasm); uploads, downloads, "
                     "and metadata fetches will report unavailable for the rest "
                     "of this session.\n";
    }
}
#endif

void CloudStorage::init() {
    if (const char* env_ep = std::getenv("EARTHCALL_CLOUD_ENDPOINT")) {
        std::lock_guard<std::mutex> lock(g_configMutex);
        g_endpoint = env_ep;
    }
    if (const char* env_token = std::getenv("EARTHCALL_CLOUD_TOKEN")) {
        std::lock_guard<std::mutex> lock(g_configMutex);
        g_authToken = env_token;
    }
    std::cout << "[CloudStorage] Initialized.\n";
}

void CloudStorage::shutdown() {
    std::cout << "[CloudStorage] Shutting down.\n";
}

void CloudStorage::setEndpoint(const std::string& url) {
    std::lock_guard<std::mutex> lock(g_configMutex);
    g_endpoint = url;
}

void CloudStorage::setAuthToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(g_configMutex);
    g_authToken = token;
}

void CloudStorage::uploadSaveAsync(const std::string& filename,
                                   const std::vector<uint8_t>& data,
                                   SaveSystem::SaveType type,
                                   std::function<void(bool)> callback) {
    std::string typeStr = SaveSystem::getSaveTypeFolderName(type);
    
#ifndef __EMSCRIPTEN__
    // Detach a thread for the upload
    std::thread([filename, data, typeStr, callback]() {
        // Simulate network delay
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        std::string ep, token;
        {
            std::lock_guard<std::mutex> lock(g_configMutex);
            ep = g_endpoint;
            token = g_authToken;
        }
        
        if (!canOpenClient(ep)) {
            if (callback) callback(false);
            return;
        }

        try {
            httplib::Client cli(ep.c_str());
            cli.set_connection_timeout(5, 0); // 5 seconds
            cli.set_read_timeout(5, 0);

            httplib::Headers headers = {
                {"Authorization", "Bearer " + token},
                {"Content-Type", "application/octet-stream"}
            };

            std::string path = "/api/saves/" + typeStr + "/" + urlEncode(filename);
            std::string body(reinterpret_cast<const char*>(data.data()), data.size());

            if (auto res = cli.Post(path.c_str(), headers, body, "application/octet-stream")) {
                if (res->status == 200 || res->status == 201) {
                    std::cout << "[CloudStorage] Uploaded " << filename << " successfully.\n";
                    if (callback) callback(true);
                    return;
                } else {
                    std::cerr << "[CloudStorage] Upload failed with status: " << res->status << "\n";
                }
            } else {
                std::cout << "[CloudStorage] Server unreachable for " << filename << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "[CloudStorage] Upload aborted: " << e.what() << "\n";
        }

        if (callback) callback(false);
    }).detach();
#else
    warnWasmCloudUnavailable();
    if (callback) callback(false);
#endif
}

void CloudStorage::uploadSaveAsync(const std::string& filename,
                                   const std::string& jsonData,
                                   SaveSystem::SaveType type,
                                   std::function<void(bool)> callback) {
    std::vector<uint8_t> data(jsonData.begin(), jsonData.end());
    uploadSaveAsync(filename, data, type, callback);
}

void CloudStorage::downloadSaveAsync(const std::string& filename,
                                     SaveSystem::SaveType type,
                                     std::function<void(std::optional<std::vector<uint8_t>>)> callback) {
    std::string typeStr = SaveSystem::getSaveTypeFolderName(type);
    
#ifndef __EMSCRIPTEN__
    std::thread([filename, typeStr, callback]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        std::string ep, token;
        {
            std::lock_guard<std::mutex> lock(g_configMutex);
            ep = g_endpoint;
            token = g_authToken;
        }
        
        if (!canOpenClient(ep)) {
            if (callback) callback(std::nullopt);
            return;
        }

        try {
            httplib::Client cli(ep.c_str());
            cli.set_connection_timeout(5, 0);
            cli.set_read_timeout(5, 0);

            httplib::Headers headers = {
                {"Authorization", "Bearer " + token}
            };

            std::string path = "/api/saves/" + typeStr + "/" + urlEncode(filename);

            if (auto res = cli.Get(path.c_str(), headers)) {
                if (res->status == 200) {
                    std::vector<uint8_t> data(res->body.begin(), res->body.end());
                    std::cout << "[CloudStorage] Downloaded " << filename << " successfully.\n";
                    if (callback) callback(data);
                    return;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[CloudStorage] Download aborted: " << e.what() << "\n";
        }

        std::cerr << "[CloudStorage] Download failure for " << filename << "\n";
        if (callback) callback(std::nullopt);
    }).detach();
#else
    warnWasmCloudUnavailable();
    if (callback) callback(std::nullopt);
#endif
}

void CloudStorage::fetchMetadataAsync(SaveSystem::SaveType type,
                                      std::function<void(std::vector<SaveSystem::SaveMetadata>)> callback) {
    std::string typeStr = SaveSystem::getSaveTypeFolderName(type);
    
#ifndef __EMSCRIPTEN__
    std::thread([typeStr, callback, type]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        std::string ep, token;
        {
            std::lock_guard<std::mutex> lock(g_configMutex);
            ep = g_endpoint;
            token = g_authToken;
        }
        
        std::vector<SaveSystem::SaveMetadata> results;
        if (!canOpenClient(ep)) {
            if (callback) callback(results);
            return;
        }

        try {
            httplib::Client cli(ep.c_str());
            cli.set_connection_timeout(5, 0);
            cli.set_read_timeout(5, 0);

            httplib::Headers headers = {
                {"Authorization", "Bearer " + token}
            };

            std::string path = "/api/saves/" + typeStr;

            if (auto res = cli.Get(path.c_str(), headers)) {
                if (res->status == 200) {
                    std::cout << "[CloudStorage] Fetched metadata successfully.\n";
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "[CloudStorage] Metadata fetch aborted: " << e.what() << "\n";
        }

        if (callback) callback(results);
    }).detach();
#else
    warnWasmCloudUnavailable();
    if (callback) callback({});
#endif
}

} // namespace Util
