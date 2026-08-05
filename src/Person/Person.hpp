#pragma once
#include <string>
#include <map>
#include <vector>
#include <memory>
#include "Body.hpp"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "Form/Singular/Singular.hpp"
#include "Soul/Soul.hpp"
#include "Singularity/Core/EventBus.hpp"
#include <json.hpp>

// Forward declarations for Person events (defined in PersonEvents.hpp)
struct PersonJoinedEvent;
struct PersonLeftZoneEvent;
struct PersonLoginEvent;
struct PersonLogoutEvent;

class Person : public Singular {
public:
    enum class GameMode {
        Creative,
        Survival,
        Spectator
    };

    /* NOTE: Refactor the "game-like" aspects of Person added by Cursor to a separate Avatar system.
    It's first and foremost a digital metaverse, not a game.
    We need to focus on the Earthcall essentials before adding game-like features. */

    // Avatar State System
    struct AvatarState {
        float health = 100.0f;
        float maxHealth = 100.0f;
        float energy = 100.0f;
        float maxEnergy = 100.0f;
        float mood = 50.0f;  // -100 to 100
        float hunger = 0.0f;
        float thirst = 0.0f;
        float temperature = 37.0f;  // Celsius
        float experience = 0.0f;
        int level = 1;
        
        // Skills and abilities
        std::map<std::string, float> skills;
        
        // Status effects
        std::vector<std::string> activeEffects;
        
        // Avatar customization
        std::string hairStyle = "Default";
        std::string eyeColor = "Brown";
        std::string skinTone = "Natural";
        float height = 1.0f;
        float weight = 70.0f;  // kg
        
        // Social stats
        int friends = 0;
        int reputation = 0;
        std::vector<std::string> relationships;
    };

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

    std::string soulName;
    GameMode mode = GameMode::Creative;
    bool physicsEnabled = false;
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 velocity{0.0f, 0.0f, 0.0f};
    glm::vec3 acceleration{0.0f, 0.0f, 0.0f};
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

    // Enhanced avatar system
    AvatarState state;
    std::vector<Animation> animations;
    Animation* currentAnimation = nullptr;
    bool _idleActive = false;
    bool _walkActive = false;
    
    // Interaction system
    std::vector<Person*> nearbyAvatars;
    float interactionRange = 3.0f;
    
    // Inventory system
    std::vector<std::string> inventory;
    int maxInventorySize = 20;

    std::vector<std::string> nicknames;
    
    // Serialization
    nlohmann::json serialize() const;
    void deserialize(const nlohmann::json& j);

    // Constructors
    Person(Soul soul, Body body, const std::string& joyOrdering);
    // Person(std::string soulName, Body&& body, glm::vec3 pos = {0.0f,0.0f,0.0f});  // Commented out - needs Soul reference
    void express() const;
    void draw() const;
    void drawNametag() const;
    void update(float deltaTime);

    const std::string& getSoulName() const { return soulName; }

    void setMode(GameMode m) { mode = m; }
    GameMode getMode() const { return mode; }
    void togglePhysics() { physicsEnabled = !physicsEnabled; }
    bool isPhysicsEnabled() const { return physicsEnabled; }

    // Update all body part world transforms based on current position
    void updatePose();

    // Avatar State Management
    void updateState(float deltaTime);
    void modifyHealth(float amount);
    void modifyEnergy(float amount);
    void modifyMood(float amount);
    void addExperience(float amount);
    void levelUp();
    void addSkill(const std::string& skillName, float value);
    float getSkill(const std::string& skillName) const;
    
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
    bool isWalkAutomationActive() const { return _walkActive; }
    bool isIdleAutomationActive() const { return _idleActive; }

    // Apply a locomotion intent: walking swaps in the walk cycle (tempo tracks
    // speed), standing still settles into idle. Holds the transition guard so it
    // is safe to call every frame. Normally driven via the LocomotionChanged
    // event (see PersonEvents.hpp) rather than called directly.
    void setLocomotion(bool moving, float speed);

    // Install the single EventBus router that dispatches LocomotionChanged to
    // its target Person. Idempotent; call once at startup.
    static void installLocomotionRouting();
    
    // Interaction System
    void interactWith(Person* other);
    void addNearbyAvatar(Person* avatar);
    void removeNearbyAvatar(Person* avatar);
    bool isNearby(Person* other) const;
    
    // Inventory System
    bool addToInventory(const std::string& item);
    bool removeFromInventory(const std::string& item);
    bool hasItem(const std::string& item) const;
    
    // Avatar Customization
    void setHairStyle(const std::string& style);
    void setEyeColor(const std::string& color);
    void setSkinTone(const std::string& tone);
    void setHeight(float h);
    void setWeight(float w);
    
    // Physics and Movement
    void applyForce(const glm::vec3& force);
    void setVelocity(const glm::vec3& vel);
    void updatePhysics(float deltaTime);
    
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

    // Singular interface implementation
    std::string getIdentifier() const override { return soulName; }

private:
    // A Person is a legible Singular too: laws can ask about (and, where
    // authorized, act on) a person's position; the name is read-only —
    // identity is not a writable slot.
    void buildProperties() override;
    std::string propName() const { return soulName; }

    Soul _soul;
    Body body;  // Body member variable

    // Helper method for creating default animations
    void createDefaultAnimations();
    
    // Session and zone state
    bool _isLoggedIn = false;
    std::string _currentSession;
    std::vector<std::string> _joinedZones;
    

};
