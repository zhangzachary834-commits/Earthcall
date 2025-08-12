#pragma once
#include <string>
#include <map>
#include <vector>
#include <memory>
#include "Body.hpp"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "Singular.hpp"
#include "Soul/Soul.hpp"
#include "Core/EventBus.hpp"

// Forward declarations for Person events
struct PersonCreatedEvent;
struct PersonJoinedEvent;
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
    

};
