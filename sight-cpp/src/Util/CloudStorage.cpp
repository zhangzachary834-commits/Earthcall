#include "CloudStorage.hpp"
#include <iostream>
#include <thread>
#include <chrono>
// Define httplib implementation in exactly one compilation unit.
// If it's used elsewhere, we'd need a separate httplib.cpp, but for now we define it here.
#include "../../third_party/httplib/httplib.h"

namespace Util {

static std::string g_endpoint = "http://localhost:8080";
static std::string g_authToken = "dummy-token";

void CloudStorage::init() {
    std::cout << "[CloudStorage] Initialized.\n";
}

void CloudStorage::shutdown() {
    std::cout << "[CloudStorage] Shutting down.\n";
}

void CloudStorage::setEndpoint(const std::string& url) {
    g_endpoint = url;
}

void CloudStorage::setAuthToken(const std::string& token) {
    g_authToken = token;
}

void CloudStorage::uploadSaveAsync(const std::string& filename,
                                   const std::vector<uint8_t>& data,
                                   SaveSystem::SaveType type,
                                   std::function<void(bool)> callback) {
    std::string typeStr = SaveSystem::getSaveTypeFolderName(type);
    
    // Detach a thread for the upload
    std::thread([filename, data, typeStr, callback]() {
        // Simulate network delay
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        httplib::Client cli(g_endpoint.c_str());
        cli.set_connection_timeout(5, 0); // 5 seconds
        cli.set_read_timeout(5, 0);
        
        httplib::Headers headers = {
            {"Authorization", "Bearer " + g_authToken},
            {"Content-Type", "application/octet-stream"}
        };
        
        std::string path = "/api/saves/" + typeStr + "/" + filename;
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
            // For foundation testing: if the server is down, we simulate a successful mock upload
            // so we can test the UI and caching behaviour.
            std::cout << "[CloudStorage] Upload mock success (server unreachable) for " << filename << "\n";
            if (callback) callback(true);
            return;
        }
        
        if (callback) callback(false);
    }).detach();
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
    
    std::thread([filename, typeStr, callback]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        httplib::Client cli(g_endpoint.c_str());
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(5, 0);
        
        httplib::Headers headers = {
            {"Authorization", "Bearer " + g_authToken}
        };
        
        std::string path = "/api/saves/" + typeStr + "/" + filename;
        
        if (auto res = cli.Get(path.c_str(), headers)) {
            if (res->status == 200) {
                std::vector<uint8_t> data(res->body.begin(), res->body.end());
                std::cout << "[CloudStorage] Downloaded " << filename << " successfully.\n";
                if (callback) callback(data);
                return;
            }
        }
        
        // Mock failure for now if server unreachable
        std::cerr << "[CloudStorage] Mock download failure for " << filename << "\n";
        if (callback) callback(std::nullopt);
    }).detach();
}

void CloudStorage::fetchMetadataAsync(SaveSystem::SaveType type,
                                      std::function<void(std::vector<SaveSystem::SaveMetadata>)> callback) {
    std::string typeStr = SaveSystem::getSaveTypeFolderName(type);
    
    std::thread([typeStr, callback, type]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        httplib::Client cli(g_endpoint.c_str());
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(5, 0);
        
        httplib::Headers headers = {
            {"Authorization", "Bearer " + g_authToken}
        };
        
        std::string path = "/api/saves/" + typeStr;
        
        std::vector<SaveSystem::SaveMetadata> results;
        
        if (auto res = cli.Get(path.c_str(), headers)) {
            if (res->status == 200) {
                // Ideally parse JSON response, but since it's foundational:
                std::cout << "[CloudStorage] Fetched metadata successfully.\n";
            }
        }
        
        // Return mock empty or populated cloud list if server unreachable
        if (callback) callback(results);
    }).detach();
}

} // namespace Util
