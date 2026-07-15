#pragma once
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include "Body.hpp"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "Singular.hpp"
#include "Soul/Soul.hpp"
#include "Core/EventBus.hpp"
#include "json.hpp"

// Forward declarations for Person events
struct PersonCreatedEvent;
struct PersonJoinedEvent;
struct PersonLoginEvent;
struct PersonLogoutEvent;
struct PersonCustomEvent;

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
    
    // Enhanced avatar system
    AvatarState state;
    std::vector<Animation> animations;
    Animation* currentAnimation = nullptr;
    
    // Interaction system
    std::vector<Person*> nearbyAvatars;
    float interactionRange = 3.0f;
    
    // Inventory system
    std::vector<std::string> inventory;
    int maxInventorySize = 20;

    // Constructors
    Person(Soul& soul, Body& body);
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

    // ------------------------------------------------------------------
    // Custom Events
    // ------------------------------------------------------------------
    // Lets a Person (or whatever's driving it - script, quest system,
    // UI) define event *kinds* that don't exist as their own struct in
    // the codebase, without adding a new C++ type per event. All custom
    // events funnel through the single PersonCustomEvent struct on
    // Core::EventBus and are distinguished by `name` at the listener
    // side (compare against event.name).
    //
    // A condition, if given, is a plain predicate over the Person's own
    // state - no separate ConditionNode type. re-checked once per
    // update() and edge-triggered (fires on the false->true transition,
    // not every tick the condition holds). Composing multiple
    // conditions (AND/OR/NOT) is just composing lambdas:
    //   auto both = [a, b](const Person& p) { return a(p) && b(p); };
    // If that composition needs to be introspectable or built/edited at
    // runtime (a quest editor UI, save/load of condition trees), THEN a
    // small ConditionNode hierarchy earns its keep - but EventCondition
    // stays std::function<bool(const Person&)> either way, so it's a
    // drop-in swap later, not a redesign now.
    using EventCondition = std::function<bool(const Person&)>;

    // Registers a named event kind for this Person. `condition` is
    // optional - omit it (or pass nullptr) to define an event that's
    // only ever fired manually via raiseEvent().
    void defineEvent(const std::string& name, EventCondition condition = nullptr);

    // Stops watching a defined event's condition (manual raiseEvent()
    // calls for that name still work).
    void undefineEvent(const std::string& name);

    // Publishes a PersonCustomEvent immediately, bypassing any
    // registered condition. `name` doesn't need to have been
    // defineEvent()'d first - ad hoc one-off events are fine.
    void raiseEvent(const std::string& name, const nlohmann::json& data = {});

    // Singular interface implementation
    std::string getIdentifier() const override { return soulName; }

private:
    Soul& _soul;
    Body& body;  // Body member variable

    // Helper method for creating default animations
    void createDefaultAnimations();

    // Session and zone state
    bool _isLoggedIn = false;
    std::string _currentSession;
    std::vector<std::string> _joinedZones;

    // Custom event state
    struct CustomEventDefinition {
        std::string name;
        EventCondition condition; // may be null (manual-only event)
        bool wasTrue = false;     // edge-detection state
    };
    std::vector<CustomEventDefinition> _customEvents;

    // Re-evaluates registered conditions and raises events on the
    // false->true edge. Called once per update().
    void checkCustomEventConditions();
};
