#pragma once
#include <string>
#include <map>
#include <vector>
#include <memory>
#include "Body.hpp"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "ConstructedBeing/Singular/Singular.hpp"
#include "Identity/SingularId.hpp"
#include "Soul/Soul.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Screen/Camera.hpp"
#include <json.hpp>

// Forward declarations for Person events (defined in PersonEvents.hpp)
struct PersonJoinedEvent;
struct PersonLeftZoneEvent;
struct PersonLoginEvent;
struct PersonLogoutEvent;

class Person : public Singular {
public:

    /* NOTE: Refactor the "game-like" aspects of Person added by Cursor to a separate Avatar system.
    It's first and foremost a digital metaverse, not a game.
    We need to focus on the Earthcall essentials before adding game-like features. */



    // Animation System
    struct Animation {
        std::string name;
        float duration;
        float currentTime = 0.0f;
        bool isPlaying = false;
        bool isLooping = false;
        std::map<std::string, std::vector<glm::vec3>> keyframes;  // bodyPart -> positions
        std::map<std::string, std::vector<glm::vec3>> rotations; // bodyPart -> rotations
    };

    std::string displayName;

    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 velocity{0.0f, 0.0f, 0.0f};
    glm::vec3 acceleration{0.0f, 0.0f, 0.0f};
    bool grounded = false;  // Whether the person is resting on a surface
    bool wasGrounded = false; // Previous frame's grounded state, for landing detection
    bool wasMoving = false;  // Previous frame's locomotion state, for locomotion events
    bool jumpKeyDownLast = false; // Previous frame's jump key state, for jump edge detection
    // --- Law System Perception Properties --- 
    std::string activeTool;
    std::string active3DMode;
    int activeShapeKind = 0;
    glm::vec3 cursorHitPos{0.0f, 0.0f, 0.0f};
    glm::vec3 cursorHitNormal{0.0f, 1.0f, 0.0f};
    glm::vec3 cursorSpawnPos{0.0f, 0.0f, 0.0f};
    glm::vec3 cursorSpawnRot{0.0f, 0.0f, 0.0f};
    glm::vec3 cursorSpawnScale{1.0f, 1.0f, 1.0f};
    // --- Placement Mode & Grid Snap Properties ---
    // Mirrors of the brush placement state the creation tools run on, so a
    // law can read and override what the hard-coded tool used to consume
    // directly. GameUpdate refreshes these each frame.
    std::string placementMode{"InFront"}; // "InFront", "ManualDistance", "CursorSnap"
    // Grid snap is ORTHOGONAL to the mode, exactly as the brush treats it:
    // it rounds whatever position the mode produced. Folding it into a mode
    // would make surface placement and snapping mutually exclusive.
    bool gridSnap{false};
    float gridSnapSize{1.0f};
    // "InFront" places this far along the camera's forward axis.
    float inFrontDistance{2.0f};
    // "ManualDistance" places relative to a FROZEN anchor, so the shape stays
    // where it was put instead of following the camera. GameUpdate captures
    // the anchor on entering the mode.
    glm::vec3 manualOffset{0.0f, 0.0f, 2.0f};
    bool manualAnchorValid{false};
    glm::vec3 manualAnchorPos{0.0f};
    glm::vec3 manualAnchorRight{1.0f, 0.0f, 0.0f};
    glm::vec3 manualAnchorUp{0.0f, 1.0f, 0.0f};
    glm::vec3 manualAnchorForward{0.0f, 0.0f, -1.0f};
    // The one place placementMode is turned into a position.
    glm::vec3 computeSpawnPosition() const;
    // Half-extent of the spawn box measured along `normal`, so a shape placed
    // against a surface rests ON it rather than half inside it.
    float spawnSurfaceOffset(const glm::vec3& normal) const;
    // Refreshes cursorSpawnPos from computeSpawnPosition(). Called once per
    // frame by GameUpdate; laws may overwrite cursorSpawnPos after it runs.
    void updatePlacement();
    glm::mat4 getCursorSpawnTransform() const;
    std::string cursorHoveredBodyPart;
    glm::vec3 cameraPos{0.0f, 0.0f, 0.0f};
    glm::vec3 cameraForward{0.0f, 0.0f, -1.0f};
    glm::vec3 activeColor{1.0f, 1.0f, 1.0f};


    
    // Serialization
    nlohmann::json serialize() const;
    void deserialize(const nlohmann::json& j);

    // Constructors
    Person(Soul soul, Body body, const std::string& joyOrdering);
    // Person(std::string displayName, Body&& body, glm::vec3 pos = {0.0f,0.0f,0.0f});  // Commented out - needs Soul reference
    void express() const;
    void draw() const;
    void drawNametag() const;
    void update(float deltaTime);

    const std::string& getDisplayName() const { return displayName; }



    // Update all body part world transforms based on current position
    void updatePose();


    
    // Animation System
    void addAnimation(const Animation& anim);
    void playAnimation(const std::string& name, bool loop = false);
    void stopAnimation();
    void updateAnimation(float deltaTime);

    // Automation System (time-driven body-part motion on top of the rest pose).
    // Advance every body part's automation clocks once per frame; updatePose()
    // then samples them. Keeping advance separate from sampling lets updatePose
    // run several times per frame without over-advancing the clips.
    void updateBodyAutomations(float deltaTime);
    // Author clip libraries onto the body parts.
    void playIdleAutomation();              // gentle breathing / sway
    void playWalkAutomation(float speed);   // swing legs & arms; speed scales tempo
    void stopBodyAutomations();

    // Apply a locomotion intent: walking swaps in the walk cycle (tempo tracks
    // speed), standing still settles into idle. Holds the transition guard so it
    // is safe to call every frame. Normally driven via the LocomotionChanged
    // event (see PersonEvents.hpp) rather than called directly.
    void setLocomotion(bool moving, float speed);

    // Install the single EventBus router that dispatches LocomotionChanged to
    // its target Person. Idempotent; call once at startup.
    static void installLocomotionRouting();
    

    
    // Physics and Movement
    void applyForce(const glm::vec3& force);
    void setVelocity(const glm::vec3& vel);
    void updatePhysics(float deltaTime);
    
    // Movement integration (previously Game::stepMovement)
    // Takes external dependencies: camera, window for input, zone manager for collisions
    void stepMovement(float dt, struct GLFWwindow* window, 
                      Core::Camera* camera, 
                      class ZoneManager* mgr, 
                      bool flying, bool canMove);
    
    // Body access methods
    Body& getBody() { return body; }
    const Body& getBody() const { return body; }

    // Session and Zone Management
    void login(const std::string& sessionId = "");
    void logout(const std::string& sessionId = "");
    void joinZone(const std::string& zoneName);
    void leaveZone(const std::string& zoneName);
    bool isLoggedIn() const { return _isLoggedIn; }
    const std::string& getCurrentSession() const { return _currentSession; }
    const std::vector<std::string>& getJoinedZones() const { return _joinedZones; }

    // AI Network Request
    void requestAIAction(const std::string& context, const std::string& targetObjectId);

    // ------------------------------------------------------------------
    // Identity and personal address.
    //
    // This belongs to Person, not to Soul. Identity is how a Person is
    // addressed, recognised, and held to what they own -- it is a fact about
    // the Person as a whole, and routing it through a member meant the class
    // responsible for a Person's standing in the world did not hold it.
    //
    // Two distinct things, deliberately not one:
    //
    //   personId    what this Person IS. An Ed25519 public key, so holding it
    //               is provable and claiming it is not. Never chosen, never
    //               typed in, never compared against a name.
    //   displayName what this Person is CALLED. Chosen, editable, and free to
    //               collide with anyone else's. It carries no authority --
    //               that is exactly why it is safe to let people pick it.
    // ------------------------------------------------------------------
    const Identity::SingularId& personId() const { return _personId; }
    bool hasIdentity() const { return _personId.canAuthenticate(); }

    // Assigning identity is a distinct act from constructing a Person: a
    // Person may exist in a loaded world before their key is available, and
    // minting one on every construction would hand out a fresh identity to
    // every temporary copy.
    void setPersonId(const Identity::SingularId& id) { _personId = id; }

    // Singular interface implementation. Prefers the cryptographic identity;
    // falls back to the display name only for worlds saved before identities
    // existed, which is what keeps legacy law-author resolution working until
    // migration has run.
    std::string getIdentifier() const override {
        return _personId.canAuthenticate() ? _personId.toString() : displayName;
    }

    // Law-author resolution scans beings for a matching identifier. During
    // migration a saved world may address this Person either way, so accept
    // both -- but never let a display name match a Person who has a real
    // identity, or picking someone's name would again be enough to be them.
    bool matchesIdentifier(const std::string& candidate) const {
        if (_personId.canAuthenticate()) return candidate == _personId.toString();
        return candidate == displayName;
    }

private:
    // A Person is a legible Singular too: laws can ask about (and, where
    // authorized, act on) a person's position; the name is read-only —
    // identity is not a writable slot.
    void buildProperties() override;
    std::string propName() const { return displayName; }

    // A Soul is what a Person worships and orders their joy by. It is
    // deliberately no longer the source of identity: a Soul's identifier was a
    // plain chosen string, so deriving the Person's identity from it meant
    // anyone who typed the same string became the same Person.
    Soul _soul;
    Identity::SingularId _personId;
    Body body;  // Body member variable

    // Helper method for creating default animations
    void createDefaultAnimations();
    
    // Session and zone state
    bool _isLoggedIn = false;
    std::string _currentSession;
    std::vector<std::string> _joinedZones;
    

    
    // Transient animation state
    bool _walkActive = false;
    bool _idleActive = false;
    std::vector<Animation> animations;
    Animation* currentAnimation = nullptr;

};
