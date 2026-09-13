#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <glm/glm.hpp>
#include <string>

namespace Singularity {
namespace Core {

class CreationChannel : public Law {
public:
    CreationChannel();

    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "creation-channel"; }
    const std::string& name() const { return _name; }

    static void syncRegister(LawManager& laws);
    static CreationChannel* find(LawManager& laws);

    void writeLiveSelection(const std::string& tool,
                            const std::string& mode,
                            int shapeKind,
                            const glm::vec3& spawnRot,
                            const glm::vec3& spawnScale,
                            bool gridSnap,
                            float gridSnapSize,
                            const glm::vec3& color);

    std::string activeTool;
    std::string active3DMode;
    bool spawnLawArmed{false};
    int activeShapeKind = 0;
    glm::vec3 cursorHitPos{0.0f, 0.0f, 0.0f};
    glm::vec3 cursorHitNormal{0.0f, 1.0f, 0.0f};
    glm::vec3 cursorSpawnPos{0.0f, 0.0f, 0.0f};
    glm::vec3 cursorSpawnRot{0.0f, 0.0f, 0.0f};
    glm::vec3 cursorSpawnScale{1.0f, 1.0f, 1.0f};
    std::string placementMode{"InFront"};
    bool gridSnap{false};
    float gridSnapSize{1.0f};
    float inFrontDistance{2.0f};
    glm::vec3 manualOffset{0.0f, 0.0f, 2.0f};
    bool manualAnchorValid{false};
    glm::vec3 manualAnchorPos{0.0f};
    glm::vec3 manualAnchorRight{1.0f, 0.0f, 0.0f};
    glm::vec3 manualAnchorUp{0.0f, 1.0f, 0.0f};
    glm::vec3 manualAnchorForward{0.0f, 0.0f, -1.0f};
    std::string cursorHoveredBodyPart;
    glm::vec3 activeColor{1.0f, 1.0f, 1.0f};
    std::string activeImplicitExpr;

    glm::vec3 computeSpawnPosition(const glm::vec3& cameraPos, const glm::vec3& cameraForward) const;
    float spawnSurfaceOffset(const glm::vec3& normal) const;
    void updatePlacement(const glm::vec3& cameraPos, const glm::vec3& cameraForward);
    glm::mat4 getCursorSpawnTransform() const;

private:
    void buildProperties() override;
    std::string _name{"creation-channel"};
};

std::shared_ptr<Law> createShapeGenerator3DLaw(Singular& author);
void syncRegisterCreatorTools(LawManager& laws, Singular& author);

// Restore the old hold-to-nudge ManualDistance controls as authored first-mover
// Laws. Input sensing stays on InteractionChannel; these laws own what a held
// arrow/PageUp/PageDown means for creation placement.
void syncRegisterManualDistanceKeyLaws(LawManager& laws, Singular& author);

const char* creatorToolLawIdForMode(const std::string& active3DMode);

} // namespace Core
} // namespace Singularity
