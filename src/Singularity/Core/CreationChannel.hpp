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

    // Push the console's live selection onto the registered paths spawn
    // laws already read. Called from the first-mover step
    // (Rendering::stepCreationTools), never from a render function — the
    // six fields used to be copied only while render3DConsole was on
    // screen in BrushCreate, so collapsing the window froze the channel.
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

    glm::vec3 computeSpawnPosition(const glm::vec3& cameraPos, const glm::vec3& cameraForward) const;
    float spawnSurfaceOffset(const glm::vec3& normal) const;
    void updatePlacement(const glm::vec3& cameraPos, const glm::vec3& cameraForward);
    glm::mat4 getCursorSpawnTransform() const;

private:
    void buildProperties() override;
    std::string _name{"creation-channel"};
};

// The Shape Generator 3D tool, as law.
//
// A FACTORY rather than a block inlined in Engine::initLogic, for the reason
// Physics::createDefaultPhysicsLaws() is one: a law built inside the engine's
// boot path can only be exercised by booting a window, so nothing tested it,
// and it shipped for a day unable to fire at all -- unauthored (Law::applyTo
// refuses with Unauthored) and conditioned on a "type" property the
// CreationChannel does not carry. tests/shape_generator_law_test.cpp now
// exercises exactly what boot instantiates.
//
// The author is the Person the law is made for: "Nothing enters the world
// without an author" is structural, not decorative, and a first mover is not
// exempt -- isFirstMover() governs SERIALIZATION (engine truth is not written
// into world saves), never the authorship gate.
//
// Registers concept "concept-shape-3d" into the ConceptRegistry as a side
// effect if it is not already there; the law's Spawn action resolves it by id.
std::shared_ptr<Law> createShapeGenerator3DLaw(Singular& author);

// The rest of the Creator Console 3D tools, as first movers. The console
// stays hardcoded chrome — it is the reference gesture that writes
// @creation-channel.active3DMode. Each tool is a named FirstMoverLaw so a
// Person can set it down, and so Law Author lists it with Shape Generator
// rather than as anonymous C++. Sense/Act stay in Tool::*; enabled is the
// gate stepCreationTools already honours for the spawn law.
void syncRegisterCreatorTools(LawManager& laws, Singular& author);

// Stable identifier for the first-mover law that owns this active3DMode
// string ("Create", "Select", ...). Empty when no tool is armed.
const char* creatorToolLawIdForMode(const std::string& active3DMode);

} // namespace Core
} // namespace Singularity
