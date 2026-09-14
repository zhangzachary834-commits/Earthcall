#pragma once

#include "Identity/SingularId.hpp"
#include "json.hpp"

#include <filesystem>
#include <string>

namespace Singularity::Storage {

struct GenerationRef {
    std::string kind;
    Identity::SingularId singularId;
    std::string generation;
    std::filesystem::path relativePath;
};

struct GenerationResult {
    bool ok = false;
    bool alreadyExisted = false;
    GenerationRef ref;
    std::string error;
};

class ImmutableGenerationStore {
public:
    explicit ImmutableGenerationStore(std::filesystem::path saveRoot);

    GenerationResult writeJson(const std::string& kind,
                               const Identity::SingularId& id,
                               const nlohmann::json& document) const;
    bool readVerifiedJson(const GenerationRef& ref, nlohmann::json& document,
                          std::string& error) const;

    // Compare-and-replace prevents concurrent saves from silently selecting a
    // winner. The previous head is retained beside the current pointer.
    bool publishHead(const std::string& kind, const Identity::SingularId& id,
                     const std::string& expectedOldGeneration,
                     const GenerationRef& next, const std::string& transactionId,
                     std::string& error) const;

    std::filesystem::path headPath(const std::string& kind,
                                   const Identity::SingularId& id) const;

private:
    std::filesystem::path _saveRoot;
};

} // namespace Singularity::Storage
