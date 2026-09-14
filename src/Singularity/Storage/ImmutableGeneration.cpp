#include "Singularity/Storage/ImmutableGeneration.hpp"

#include <openssl/sha.h>

#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

#if !defined(_WIN32)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace Singularity::Storage {
namespace {

bool safeToken(const std::string& value) {
    if (value.empty()) return false;
    for (const unsigned char ch : value) {
        if (!(std::isalnum(ch) || ch == '-' || ch == '_')) return false;
    }
    return true;
}

std::string sha256(const std::string& bytes) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), digest);
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned char byte : digest) out << std::setw(2) << static_cast<int>(byte);
    return out.str();
}

bool durableWrite(const std::filesystem::path& path, const std::string& bytes,
                  std::string& error) {
#if defined(_WIN32)
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.flush();
    if (!output) { error = "write/flush failed: " + path.string(); return false; }
    return true;
#else
    const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0) { error = "open failed: " + path.string(); return false; }
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const ssize_t wrote = ::write(fd, bytes.data() + offset, bytes.size() - offset);
        if (wrote <= 0) { error = "write failed: " + path.string(); ::close(fd); return false; }
        offset += static_cast<std::size_t>(wrote);
    }
    if (::fsync(fd) != 0) { error = "fsync failed: " + path.string(); ::close(fd); return false; }
    if (::close(fd) != 0) { error = "close failed: " + path.string(); return false; }
    return true;
#endif
}

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    std::ostringstream out;
    out << input.rdbuf();
    bytes = out.str();
    return static_cast<bool>(input) || input.eof();
}

std::filesystem::path rootDirectory(const std::filesystem::path& saveRoot,
                                    const std::string& kind,
                                    const Identity::SingularId& id) {
    return saveRoot / "roots" / kind / id.toString();
}

} // namespace

ImmutableGenerationStore::ImmutableGenerationStore(std::filesystem::path saveRoot)
    : _saveRoot(std::move(saveRoot)) {}

GenerationResult ImmutableGenerationStore::writeJson(
    const std::string& kind, const Identity::SingularId& id,
    const nlohmann::json& document) const {
    GenerationResult result;
    if (!safeToken(kind) || !id.isValid()) {
        result.error = "invalid kind or SingularId";
        return result;
    }
    nlohmann::json canonical = document;
    canonical["singularId"] = id.toString();
    const std::string bytes = canonical.dump();
    const std::string digest = sha256(bytes);
    const std::string generation = "sha256:" + digest;
    const auto relative = std::filesystem::path("roots") / kind / id.toString() /
                          "generations" / (digest + ".json");
    const auto finalPath = _saveRoot / relative;
    std::error_code ec;
    std::filesystem::create_directories(finalPath.parent_path(), ec);
    if (ec) { result.error = ec.message(); return result; }
    if (std::filesystem::exists(finalPath, ec)) {
        std::string existing;
        if (!readBytes(finalPath, existing) || sha256(existing) != digest) {
            result.error = "existing immutable generation failed hash verification";
            return result;
        }
        result.ok = true;
        result.alreadyExisted = true;
        result.ref = {kind, id, generation, relative};
        return result;
    }
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto staging = finalPath.string() + ".staging-" + std::to_string(nonce);
    if (!durableWrite(staging, bytes, result.error)) return result;
    std::string staged;
    if (!readBytes(staging, staged) || sha256(staged) != digest) {
        result.error = "staged generation failed reread verification";
        return result;
    }
    std::filesystem::rename(staging, finalPath, ec);
    if (ec) { result.error = ec.message(); return result; }
    result.ok = true;
    result.ref = {kind, id, generation, relative};
    return result;
}

bool ImmutableGenerationStore::readVerifiedJson(const GenerationRef& ref,
                                                nlohmann::json& document,
                                                std::string& error) const {
    if (!ref.singularId.isValid() || ref.generation.rfind("sha256:", 0) != 0) {
        error = "invalid generation reference";
        return false;
    }
    std::string bytes;
    if (!readBytes(_saveRoot / ref.relativePath, bytes)) {
        error = "missing generation: " + ref.relativePath.string();
        return false;
    }
    if ("sha256:" + sha256(bytes) != ref.generation) {
        error = "generation hash mismatch: " + ref.relativePath.string();
        return false;
    }
    try { document = nlohmann::json::parse(bytes); }
    catch (...) { error = "generation JSON malformed"; return false; }
    if (document.value("singularId", std::string{}) != ref.singularId.toString()) {
        error = "generation SingularId mismatch";
        return false;
    }
    return true;
}

std::filesystem::path ImmutableGenerationStore::headPath(
    const std::string& kind, const Identity::SingularId& id) const {
    return rootDirectory(_saveRoot, kind, id) / "head.json";
}

bool ImmutableGenerationStore::publishHead(
    const std::string& kind, const Identity::SingularId& id,
    const std::string& expectedOldGeneration, const GenerationRef& next,
    const std::string& transactionId, std::string& error) const {
    if (!safeToken(kind) || !id.isValid() || next.singularId != id ||
        next.kind != kind || transactionId.empty()) {
        error = "REFUSED: invalid head publication request";
        return false;
    }
    nlohmann::json verified;
    if (!readVerifiedJson(next, verified, error)) return false;
    const auto finalPath = headPath(kind, id);
    std::string priorBytes;
    nlohmann::json prior;
    if (readBytes(finalPath, priorBytes)) {
        try { prior = nlohmann::json::parse(priorBytes); }
        catch (...) { error = "REFUSED: current head is malformed"; return false; }
    }
    const std::string actualOld = prior.is_object()
        ? prior.value("generation", std::string{})
        : std::string{};
    if (actualOld != expectedOldGeneration) {
        error = "REFUSED: current head changed since prepare";
        return false;
    }
    const nlohmann::json nextHead{{"singularId", id.toString()},
                                  {"kind", kind}, {"generation", next.generation},
                                  {"path", next.relativePath.generic_string()},
                                  {"transactionId", transactionId}};
    std::error_code ec;
    std::filesystem::create_directories(finalPath.parent_path(), ec);
    if (ec) { error = ec.message(); return false; }
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto temporary = finalPath.string() + ".staging-" + std::to_string(nonce);
    if (!durableWrite(temporary, nextHead.dump(2), error)) return false;
    if (!priorBytes.empty()) {
        const auto previous = finalPath.parent_path() / "head.previous.json";
        const auto previousTemp = previous.string() + ".staging-" + std::to_string(nonce);
        if (!durableWrite(previousTemp, priorBytes, error)) return false;
        std::filesystem::rename(previousTemp, previous, ec);
        if (ec) { error = ec.message(); return false; }
    }
    std::filesystem::rename(temporary, finalPath, ec);
    if (ec) { error = ec.message(); return false; }
    return true;
}

} // namespace Singularity::Storage
