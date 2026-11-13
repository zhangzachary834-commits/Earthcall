#pragma once

#include "Singularity/Screen/WebGPU/FieldVmDependencies.hpp"

#include <glm/glm.hpp>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace sdfwgsl {

// This is NOT a visibility result and NOT a cache entry.
// It is the exact premise tuple under which a later visibility proof may be
// retained/reused. Knownness belongs to the proof produced from these premises,
// never to this identity by itself.
enum class VisibilityExecutorFrame : unsigned {
    MeshRaster = 1,
    SdfRaymarch = 2,
};

enum class VisibilityProofScope : unsigned {
    LocalReceiverGeometry = 1,
    CompleteScene = 2,
};

struct VisibilityProofPremises {
    VisibilityExecutorFrame executorFrame = VisibilityExecutorFrame::MeshRaster;
    VisibilityProofScope scope = VisibilityProofScope::CompleteScene;

    // Exact caller-owned identities. Parallax composes them; it never invents
    // their authority or hashes them down to a smaller key.
    std::string admittedProgramIdentity;
    std::string receiverEvidenceIdentity;
    std::string sceneEvidenceIdentity;
    std::string traversalPolicyIdentity;

    // Lattice-owned exact dependency evidence. A missing/invalid snapshot means
    // there is no reusable Parallax premise identity.
    FieldVmDependencySnapshot dependencySnapshot;

    // Exact query segment endpoints in the execution precision of the current
    // visibility kernel. Signed-zero / bit-level differences conservatively
    // invalidate reuse rather than being normalized by this layer.
    glm::vec3 sourcePosition{0.0f};
    glm::vec3 receiverPosition{0.0f};
};

struct VisibilityProofPremiseIdentity {
    bool valid = false;
    std::string identity;
    std::vector<std::string> missing;
    std::vector<std::string> nonFinite;
    std::vector<std::string> unsupported;
};

namespace detail {

inline uint32_t visibilityFloatBits(float value) {
    uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

inline std::string visibilityHex32(uint32_t value) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(8, '0');
    for (int i = 7; i >= 0; --i) {
        out[static_cast<std::size_t>(i)] = kHex[value & 0xfu];
        value >>= 4u;
    }
    return out;
}

inline void appendVisibilityIdentityPart(
    std::string& out, const std::string& value) {
    out += std::to_string(value.size());
    out += ":";
    out += value;
    out += ";";
}

inline bool finiteVec3(const glm::vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

inline std::string vec3Bits(const glm::vec3& v) {
    return visibilityHex32(visibilityFloatBits(v.x)) + ":" +
           visibilityHex32(visibilityFloatBits(v.y)) + ":" +
           visibilityHex32(visibilityFloatBits(v.z));
}

} // namespace detail

inline VisibilityProofPremiseIdentity
makeVisibilityProofPremiseIdentity(const VisibilityProofPremises& p) {
    VisibilityProofPremiseIdentity out;

    const auto require = [&](const std::string& value, const char* label) {
        if (value.empty()) out.missing.emplace_back(label);
    };
    require(p.admittedProgramIdentity, "admitted-program");
    require(p.receiverEvidenceIdentity, "receiver-evidence");
    require(p.sceneEvidenceIdentity, "scene-evidence");
    require(p.traversalPolicyIdentity, "traversal-policy");

    if (!p.dependencySnapshot.valid || p.dependencySnapshot.identity.empty()) {
        out.missing.emplace_back("dependency-snapshot");
    }
    if (p.executorFrame != VisibilityExecutorFrame::MeshRaster &&
        p.executorFrame != VisibilityExecutorFrame::SdfRaymarch) {
        out.unsupported.emplace_back("executor-frame");
    }
    if (p.scope != VisibilityProofScope::LocalReceiverGeometry &&
        p.scope != VisibilityProofScope::CompleteScene) {
        out.unsupported.emplace_back("proof-scope");
    }
    if (!detail::finiteVec3(p.sourcePosition)) {
        out.nonFinite.emplace_back("source-position");
    }
    if (!detail::finiteVec3(p.receiverPosition)) {
        out.nonFinite.emplace_back("receiver-position");
    }
    if (!out.missing.empty() || !out.nonFinite.empty() ||
        !out.unsupported.empty()) return out;

    std::string id;
    const auto add = [&](const std::string& part) {
        detail::appendVisibilityIdentityPart(id, part);
    };

    add("visibility-proof-premises:v1");
    add(std::to_string(static_cast<unsigned>(p.executorFrame)));
    add(std::to_string(static_cast<unsigned>(p.scope)));
    add(p.admittedProgramIdentity);
    add(p.dependencySnapshot.identity);
    add(p.receiverEvidenceIdentity);
    add(p.sceneEvidenceIdentity);
    add(p.traversalPolicyIdentity);
    add(detail::vec3Bits(p.sourcePosition));
    add(detail::vec3Bits(p.receiverPosition));

    out.valid = true;
    out.identity = std::move(id);
    return out;
}

} // namespace sdfwgsl
