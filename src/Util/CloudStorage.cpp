#include "CloudStorage.hpp"
#include <iostream>
#include <thread>
#include <chrono>
// Define httplib implementation in exactly one compilation unit.
// If it's used elsewhere, we'd need a separate httplib.cpp, but for now we define it here.
#include "../../third_party/httplib/httplib.h"

namespace Util {

#include <mutex>

static std::string g_endpoint = "https://localhost:8080";
static std::string g_authToken = "";
static std::mutex g_configMutex;

void CloudStorage::init() {
    if (const char* env_token = std::getenv("EARTHCALL_CLOUD_TOKEN")) {
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
        
        httplib::Client cli(ep.c_str());
        cli.set_connection_timeout(5, 0); // 5 seconds
        cli.set_read_timeout(5, 0);
        
        httplib::Headers headers = {
            {"Authorization", "Bearer " + token},
            {"Content-Type", "application/octet-stream"}
        };
        
        auto url_encode = [](const std::string& value) {
            std::ostringstream escaped;
            escaped.fill('0');
            escaped << std::hex;
            for (char c : value) {
                if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
                    escaped << c;
                } else {
                    escaped << std::uppercase << '%' << std::setw(2) << int((unsigned char)c) << std::nouppercase;
                }
            }
            return escaped.str();
        };
        
        std::string path = "/api/saves/" + typeStr + "/" + url_encode(filename);
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
        
        std::string ep, token;
        {
            std::lock_guard<std::mutex> lock(g_configMutex);
            ep = g_endpoint;
            token = g_authToken;
        }
        
        httplib::Client cli(ep.c_str());
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(5, 0);
        
        httplib::Headers headers = {
            {"Authorization", "Bearer " + token}
        };
        
        auto url_encode = [](const std::string& value) {
            std::ostringstream escaped;
            escaped.fill('0');
            escaped << std::hex;
            for (char c : value) {
                if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
                    escaped << c;
                } else {
                    escaped << std::uppercase << '%' << std::setw(2) << int((unsigned char)c) << std::nouppercase;
                }
            }
            return escaped.str();
        };
        
        std::string path = "/api/saves/" + typeStr + "/" + url_encode(filename);
        
        if (auto res = cli.Get(path.c_str(), headers)) {
            if (res->status == 200) {
                std::vector<uint8_t> data(res->body.begin(), res->body.end());
                std::cout << "[CloudStorage] Downloaded " << filename << " successfully.\n";
                if (callback) callback(data);
                return;
            }
        }
        
        std::cerr << "[CloudStorage] Download failure for " << filename << "\n";
        if (callback) callback(std::nullopt);
    }).detach();
}

void CloudStorage::fetchMetadataAsync(SaveSystem::SaveType type,
                                      std::function<void(std::vector<SaveSystem::SaveMetadata>)> callback) {
    std::string typeStr = SaveSystem::getSaveTypeFolderName(type);
    
    std::thread([typeStr, callback, type]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        std::string ep, token;
        {
            std::lock_guard<std::mutex> lock(g_configMutex);
            ep = g_endpoint;
            token = g_authToken;
        }
        
        httplib::Client cli(ep.c_str());
        cli.set_connection_timeout(5, 0);
        cli.set_read_timeout(5, 0);
        
        httplib::Headers headers = {
            {"Authorization", "Bearer " + token}
        };
        
        std::string path = "/api/saves/" + typeStr;
        
        std::vector<SaveSystem::SaveMetadata> results;
        
        if (auto res = cli.Get(path.c_str(), headers)) {
            if (res->status == 200) {
                // Ideally parse JSON response, but since it's foundational:
                std::cout << "[CloudStorage] Fetched metadata successfully.\n";
            }
        }
        
        if (callback) callback(results);
    }).detach();
}

} // namespace Util
