#pragma once
#include <vector>
#include <map>
#include <memory>
#include <string>
#include "Person.hpp"

class AvatarManager {
public:
    // Avatar creation and management
    Person* createAvatar(const std::string& name, const std::string& artStyle = "Voxel");
    Person* createCustomAvatar(const std::string& name, Body::BodyType bodyType, Body::Proportions props);
    Person* createChildAvatar(const std::string& name);
    Person* createElderAvatar(const std::string& name);
    
    void removeAvatar(const std::string& name);
    Person* getAvatar(const std::string& name);
    const std::vector<Person*>& getAllAvatars() const { return avatars; }
    
    // Avatar interactions
    void updateAllAvatars(float deltaTime);
    void updateNearbyAvatars();
    void processAvatarInteractions();
    
    // Avatar statistics
    int getTotalAvatars() const { return avatars.size(); }
    float getAverageHealth() const;
    float getAverageLevel() const;
    int getTotalExperience() const;
    
    // Avatar groups and relationships
    void createAvatarGroup(const std::string& groupName, const std::vector<std::string>& memberNames);
    void addAvatarToGroup(const std::string& groupName, const std::string& avatarName);
    void removeAvatarFromGroup(const std::string& groupName, const std::string& avatarName);
    std::vector<Person*> getGroupMembers(const std::string& groupName) const;
    
    // Avatar events and activities
    void organizeEvent(const std::string& eventName, const std::vector<std::string>& participants);
    void startConversation(const std::string& avatar1, const std::string& avatar2);
    void startActivity(const std::string& activityName, const std::vector<std::string>& participants);
    
    // Avatar health and status management
    void healAllAvatars(float amount);
    void damageAllAvatars(float amount);
    void restoreAllAvatars();
    
    // Avatar customization presets
    void applyPreset(const std::string& presetName, Person* avatar);
    void createPreset(const std::string& presetName, Person* avatar);
    void listPresets() const;
    
    // Avatar AI and behavior
    void enableAvatarAI(const std::string& avatarName);
    void disableAvatarAI(const std::string& avatarName);
    void setAvatarBehavior(const std::string& avatarName, const std::string& behavior);
    
    // Avatar persistence
    void saveAvatarState(const std::string& avatarName, const std::string& filename);
    void loadAvatarState(const std::string& avatarName, const std::string& filename);
    void saveAllAvatars(const std::string& directory);
    void loadAllAvatars(const std::string& directory);

private:
    std::vector<Person*> avatars;
    std::map<std::string, std::vector<std::string>> avatarGroups;
    std::map<std::string, Person*> avatarPresets;
    
    // Helper methods
    void updateAvatarNearbyLists();
    void processGroupInteractions();
    void updateAvatarAI(float deltaTime);
}; 