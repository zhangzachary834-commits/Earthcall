#pragma once
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include "Util/SaveSystem.hpp"

namespace Util {

class CloudStorage {
public:
    // Initialize the cloud storage system (e.g. set up HTTP client context)
    static void init();

    // Shut down the cloud storage system (e.g. join background threads)
    static void shutdown();

    // Set the endpoint for the backend
    static void setEndpoint(const std::string& url);

    // Placeholder for authentication token or user ID
    static void setAuthToken(const std::string& token);

    // Upload a save file asynchronously
    // callback will be invoked with true on success, false on failure
    static void uploadSaveAsync(const std::string& filename,
                                const std::vector<uint8_t>& data,
                                SaveSystem::SaveType type,
                                std::function<void(bool success)> callback);

    // Upload a save file from a JSON string asynchronously
    static void uploadSaveAsync(const std::string& filename,
                                const std::string& jsonData,
                                SaveSystem::SaveType type,
                                std::function<void(bool success)> callback);

    // Download a save file asynchronously
    // callback will be invoked with the data on success, or std::nullopt on failure
    static void downloadSaveAsync(const std::string& filename,
                                  SaveSystem::SaveType type,
                                  std::function<void(std::optional<std::vector<uint8_t>>)> callback);

    // Fetch the list of cloud saves asynchronously
    static void fetchMetadataAsync(SaveSystem::SaveType type,
                                   std::function<void(std::vector<SaveSystem::SaveMetadata>)> callback);
};

} // namespace Util
