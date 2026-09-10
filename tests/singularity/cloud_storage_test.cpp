#define CPPHTTPLIB_IMPLEMENTATION
#include "../../third_party/httplib/httplib.h"
#include "Singularity/Storage/CloudStorage.hpp"
#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <atomic>
#include <cstdlib>

int main() {
    httplib::Server svr;
    std::atomic<bool> upload_called{false};
    std::atomic<bool> download_called{false};
    std::atomic<bool> metadata_called{false};
    std::atomic<bool> auth_failed{false};

    svr.Post("/api/saves/worlds/test_save.json", [&](const httplib::Request& req, httplib::Response& res) {
        if (req.get_header_value("Authorization") != "Bearer test-token") {
            auth_failed = true;
            res.status = 401;
            return;
        }
        if (req.body == "test data") {
            res.status = 200;
            upload_called = true;
        } else {
            res.status = 400;
        }
    });

    svr.Get("/api/saves/worlds/test_save.json", [&](const httplib::Request& req, httplib::Response& res) {
        if (req.get_header_value("Authorization") != "Bearer test-token") {
            auth_failed = true;
            res.status = 401;
            return;
        }
        res.status = 200;
        res.set_content("download data", "application/octet-stream");
        download_called = true;
    });

    svr.Get("/api/saves/worlds", [&](const httplib::Request& req, httplib::Response& res) {
        if (req.get_header_value("Authorization") != "Bearer test-token") {
            auth_failed = true;
            res.status = 401;
            return;
        }
        res.status = 200;
        metadata_called = true;
    });

    // Bind to any ephemeral port
    int port = svr.bind_to_any_port("localhost");

    // Start server in background thread
    auto server_thread = std::thread([&svr]() {
        svr.listen_after_bind();
    });

    // Wait for server to start correctly using httplib
    svr.wait_until_ready();

    Util::CloudStorage::init();
    Util::CloudStorage::setEndpoint("http://localhost:" + std::to_string(port));
    Util::CloudStorage::setAuthToken("test-token");

    int checks = 0;
    int failures = 0;
    auto check = [&](bool condition, const std::string& desc) {
        checks++;
        if (!condition) {
            failures++;
            std::cerr << "  FAILED: " << desc << "\n";
        } else {
            std::cout << "  ok: " << desc << "\n";
        }
    };

    auto wait_future_bool = [&](auto& future, const std::string& desc) -> bool {
        auto status = future.wait_for(std::chrono::seconds(5));
        check(status == std::future_status::ready, desc + " did not timeout");
        if (status != std::future_status::ready) return false;
        return future.get();
    };

    // Test Upload
    std::promise<bool> upload_promise;
    auto upload_future = upload_promise.get_future();
    Util::CloudStorage::uploadSaveAsync("test_save.json", "test data", SaveSystem::SaveType::WORLD, [&](bool success) {
        upload_promise.set_value(success);
    });
    bool upload_result = wait_future_bool(upload_future, "Upload operation");
    check(upload_result, "Upload should return true on success");
    check(upload_called.load(), "Server should have received the correct upload data");

    // Test Download
    std::promise<std::optional<std::vector<uint8_t>>> download_promise;
    auto download_future = download_promise.get_future();
    Util::CloudStorage::downloadSaveAsync("test_save.json", SaveSystem::SaveType::WORLD, [&](std::optional<std::vector<uint8_t>> data) {
        download_promise.set_value(data);
    });
    auto download_status = download_future.wait_for(std::chrono::seconds(5));
    check(download_status == std::future_status::ready, "Download operation did not timeout");
    std::optional<std::vector<uint8_t>> download_result;
    if (download_status == std::future_status::ready) download_result = download_future.get();

    check(download_result.has_value(), "Download should succeed");
    if (download_result.has_value()) {
        std::string downloaded_str(download_result->begin(), download_result->end());
        check(downloaded_str == "download data", "Downloaded data should match server response");
    }
    check(download_called.load(), "Server should have received the download request");

    // Test Metadata Fetch
    std::promise<std::vector<SaveSystem::SaveMetadata>> metadata_promise;
    auto metadata_future = metadata_promise.get_future();
    Util::CloudStorage::fetchMetadataAsync(SaveSystem::SaveType::WORLD, [&](std::vector<SaveSystem::SaveMetadata> data) {
        metadata_promise.set_value(data);
    });
    auto metadata_status = metadata_future.wait_for(std::chrono::seconds(5));
    check(metadata_status == std::future_status::ready, "Metadata operation did not timeout");
    std::vector<SaveSystem::SaveMetadata> metadata_result;
    if (metadata_status == std::future_status::ready) metadata_result = metadata_future.get();
    check(metadata_called.load(), "Server should have received the metadata request");

    // Test auth failure
    Util::CloudStorage::setAuthToken("wrong-token");
    std::promise<bool> upload_fail_promise;
    auto upload_fail_future = upload_fail_promise.get_future();
    Util::CloudStorage::uploadSaveAsync("test_save.json", "test data", SaveSystem::SaveType::WORLD, [&](bool success) {
        upload_fail_promise.set_value(success);
    });
    bool upload_fail_result = wait_future_bool(upload_fail_future, "Upload failure operation");
    check(!upload_fail_result, "Upload should fail with wrong auth token");
    check(auth_failed.load(), "Server should have rejected the wrong auth token");

    // Test invalid endpoint
    Util::CloudStorage::setEndpoint("http://this-does-not-exist.local"); // invalid hostname
    std::promise<bool> upload_unreach_promise;
    auto upload_unreach_future = upload_unreach_promise.get_future();
    Util::CloudStorage::uploadSaveAsync("test_save.json", "test data", SaveSystem::SaveType::WORLD, [&](bool success) {
        upload_unreach_promise.set_value(success);
    });
    bool upload_unreach_result = wait_future_bool(upload_unreach_future, "Upload unreachable operation");
    check(!upload_unreach_result, "Upload should fail when server is unreachable");

    svr.stop();
    server_thread.join();

    std::cout << "------------------------------------------------------------\n";
    std::cout << checks - failures << "/" << checks << " checks passed\n";
    if (failures > 0) {
        std::cout << "cloud_storage_test: FAILED\n";
        return 1;
    }
    std::cout << "cloud_storage_test: ALL OK\n";
    return 0;
}
