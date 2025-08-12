#include "AvatarManager.hpp"
#include "Util/SaveSystem.hpp"
#include "Soul/Soul.hpp"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>

Person* AvatarManager::createAvatar(const std::string& name, const std::string& artStyle) {
    // Check if avatar already exists
    for (auto* avatar : avatars) {
        if (avatar->getSoulName() == name) {
            std::cout << "Avatar '" << name << "' already exists!" << std::endl;
            return avatar;
        }
    }
    
    // Create new avatar
    Body body = Body::createBasicAvatar(artStyle);
    Soul soul(name);  // Create a soul for this person
    Person* newAvatar = new Person(soul, body);
    avatars.push_back(newAvatar);
    
    std::cout << "Created avatar: " << name << std::endl;
    return newAvatar;
}

Person* AvatarManager::createCustomAvatar(const std::string& name, Body::BodyType bodyType, Body::Proportions props) {
    // Check if avatar already exists
    for (auto* avatar : avatars) {
        if (avatar->getSoulName() == name) {
            std::cout << "Avatar '" << name << "' already exists!" << std::endl;
            return avatar;
        }
    }
    
    // Create custom avatar
    Body body = Body::createCustomAvatar("Voxel", bodyType, props);
    Soul soul(name);  // Create a soul for this person
    Person* newAvatar = new Person(soul, body);
    avatars.push_back(newAvatar);
    
    std::cout << "Created custom avatar: " << name << std::endl;
    return newAvatar;
}

Person* AvatarManager::createChildAvatar(const std::string& name) {
    return createCustomAvatar(name, Body::BodyType::Humanoid, Body::Proportions::Child);
}

Person* AvatarManager::createElderAvatar(const std::string& name) {
    return createCustomAvatar(name, Body::BodyType::Humanoid, Body::Proportions::Elder);
}

void AvatarManager::removeAvatar(const std::string& name) {
    auto it = std::remove_if(avatars.begin(), avatars.end(),
        [&name](Person* avatar) {
            return avatar->getSoulName() == name;
        });
    
    if (it != avatars.end()) {
        delete *it;  // Clean up memory
        avatars.erase(it);
        std::cout << "Removed avatar: " << name << std::endl;
    }
}

Person* AvatarManager::getAvatar(const std::string& name) {
    for (auto* avatar : avatars) {
        if (avatar->getSoulName() == name) {
            return avatar;
        }
    }
    return nullptr;
}

void AvatarManager::updateAllAvatars(float deltaTime) {
    // Update each avatar
    for (auto* avatar : avatars) {
        avatar->update(deltaTime);
    }
    
    // Update nearby avatar lists
    updateAvatarNearbyLists();
    
    // Process interactions
    processAvatarInteractions();
    
    // Update AI
    updateAvatarAI(deltaTime);
}

void AvatarManager::updateNearbyAvatars() {
    updateAvatarNearbyLists();
}

void AvatarManager::updateAvatarNearbyLists() {
    // Clear all nearby lists
    for (auto* avatar : avatars) {
        avatar->nearbyAvatars.clear();
    }
    
    // Check distances between all avatars
    for (size_t i = 0; i < avatars.size(); ++i) {
        for (size_t j = i + 1; j < avatars.size(); ++j) {
            Person* avatar1 = avatars[i];
            Person* avatar2 = avatars[j];
            
            if (avatar1->isNearby(avatar2)) {
                avatar1->addNearbyAvatar(avatar2);
                avatar2->addNearbyAvatar(avatar1);
            }
        }
    }
}

void AvatarManager::processAvatarInteractions() {
    // Random interactions between nearby avatars
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    for (auto* avatar : avatars) {
        for (auto* nearby : avatar->nearbyAvatars) {
            // 5% chance of interaction per frame
            if (dis(gen) < 0.05f) {
                avatar->interactWith(nearby);
            }
        }
    }
}

void AvatarManager::processGroupInteractions() {
    for (const auto& group : avatarGroups) {
        auto members = getGroupMembers(group.first);
        if (members.size() >= 2) {
            // Group members interact more frequently
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dis(0.0f, 1.0f);
            
            for (size_t i = 0; i < members.size(); ++i) {
                for (size_t j = i + 1; j < members.size(); ++j) {
                    if (dis(gen) < 0.1f) {  // 10% chance for group members
                        members[i]->interactWith(members[j]);
                    }
                }
            }
        }
    }
}

void AvatarManager::updateAvatarAI(float deltaTime) {
    // Simple AI behavior
    for (auto* avatar : avatars) {
        // Random movement
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
        
        if (avatar->getMode() == Person::GameMode::Survival) {
            // Survival mode - more active
            glm::vec3 randomForce(dis(gen), 0.0f, dis(gen));
            avatar->applyForce(randomForce * 10.0f);
        }
    }
}

float AvatarManager::getAverageHealth() const {
    if (avatars.empty()) return 0.0f;
    
    float totalHealth = 0.0f;
    for (const auto* avatar : avatars) {
        totalHealth += avatar->state.health;
    }
    return totalHealth / avatars.size();
}

float AvatarManager::getAverageLevel() const {
    if (avatars.empty()) return 0.0f;
    
    float totalLevel = 0.0f;
    for (const auto* avatar : avatars) {
        totalLevel += avatar->state.level;
    }
    return totalLevel / avatars.size();
}

int AvatarManager::getTotalExperience() const {
    int totalExp = 0;
    for (const auto* avatar : avatars) {
        totalExp += static_cast<int>(avatar->state.experience);
    }
    return totalExp;
}

void AvatarManager::createAvatarGroup(const std::string& groupName, const std::vector<std::string>& memberNames) {
    avatarGroups[groupName] = memberNames;
    std::cout << "Created group '" << groupName << "' with " << memberNames.size() << " members" << std::endl;
}

void AvatarManager::addAvatarToGroup(const std::string& groupName, const std::string& avatarName) {
    auto it = avatarGroups.find(groupName);
    if (it != avatarGroups.end()) {
        it->second.push_back(avatarName);
        std::cout << "Added '" << avatarName << "' to group '" << groupName << "'" << std::endl;
    }
}

void AvatarManager::removeAvatarFromGroup(const std::string& groupName, const std::string& avatarName) {
    auto it = avatarGroups.find(groupName);
    if (it != avatarGroups.end()) {
        auto& members = it->second;
        auto memberIt = std::remove(members.begin(), members.end(), avatarName);
        members.erase(memberIt, members.end());
        std::cout << "Removed '" << avatarName << "' from group '" << groupName << "'" << std::endl;
    }
}

std::vector<Person*> AvatarManager::getGroupMembers(const std::string& groupName) const {
    std::vector<Person*> members;
    auto it = avatarGroups.find(groupName);
    if (it != avatarGroups.end()) {
        for (const auto& name : it->second) {
            for (auto* avatar : avatars) {
                if (avatar->getSoulName() == name) {
                    members.push_back(avatar);
                    break;
                }
            }
        }
    }
    return members;
}

void AvatarManager::organizeEvent(const std::string& eventName, const std::vector<std::string>& participants) {
    std::cout << "Organizing event: " << eventName << std::endl;
    
    for (const auto& name : participants) {
        Person* avatar = getAvatar(name);
        if (avatar) {
            avatar->modifyMood(15.0f);  // Events boost mood
            avatar->addExperience(10.0f);
            std::cout << "  " << name << " is participating in " << eventName << std::endl;
        }
    }
}

void AvatarManager::startConversation(const std::string& avatar1, const std::string& avatar2) {
    Person* a1 = getAvatar(avatar1);
    Person* a2 = getAvatar(avatar2);
    
    if (a1 && a2) {
        a1->interactWith(a2);
        std::cout << avatar1 << " and " << avatar2 << " are having a conversation" << std::endl;
    }
}

void AvatarManager::startActivity(const std::string& activityName, const std::vector<std::string>& participants) {
    std::cout << "Starting activity: " << activityName << std::endl;
    
    for (const auto& name : participants) {
        Person* avatar = getAvatar(name);
        if (avatar) {
            avatar->modifyEnergy(-5.0f);  // Activities use energy
            avatar->addExperience(15.0f);
            avatar->modifyMood(10.0f);
            std::cout << "  " << name << " is doing " << activityName << std::endl;
        }
    }
}

void AvatarManager::healAllAvatars(float amount) {
    for (auto* avatar : avatars) {
        avatar->modifyHealth(amount);
    }
    std::cout << "Healed all avatars by " << amount << " health points" << std::endl;
}

void AvatarManager::damageAllAvatars(float amount) {
    for (auto* avatar : avatars) {
        avatar->modifyHealth(-amount);
    }
    std::cout << "Damaged all avatars by " << amount << " health points" << std::endl;
}

void AvatarManager::restoreAllAvatars() {
    for (auto* avatar : avatars) {
        avatar->state.health = avatar->state.maxHealth;
        avatar->state.energy = avatar->state.maxEnergy;
        avatar->state.hunger = 0.0f;
        avatar->state.thirst = 0.0f;
        avatar->state.mood = 50.0f;
    }
    std::cout << "Restored all avatars to full health and energy" << std::endl;
}

void AvatarManager::applyPreset(const std::string& presetName, Person* avatar) {
    auto it = avatarPresets.find(presetName);
    if (it != avatarPresets.end() && it->second) {
        // Apply preset settings to avatar
        avatar->state.hairStyle = it->second->state.hairStyle;
        avatar->state.eyeColor = it->second->state.eyeColor;
        avatar->state.skinTone = it->second->state.skinTone;
        avatar->setHeight(it->second->state.height);
        avatar->setWeight(it->second->state.weight);
        std::cout << "Applied preset '" << presetName << "' to " << avatar->getSoulName() << std::endl;
    }
}

void AvatarManager::createPreset(const std::string& presetName, Person* avatar) {
    avatarPresets[presetName] = avatar;
    std::cout << "Created preset '" << presetName << "' from " << avatar->getSoulName() << std::endl;
}

void AvatarManager::listPresets() const {
    std::cout << "Available presets:" << std::endl;
    for (const auto& preset : avatarPresets) {
        std::cout << "  - " << preset.first << std::endl;
    }
}

void AvatarManager::enableAvatarAI(const std::string& avatarName) {
    Person* avatar = getAvatar(avatarName);
    if (avatar) {
        avatar->setMode(Person::GameMode::Survival);
        std::cout << "Enabled AI for " << avatarName << std::endl;
    }
}

void AvatarManager::disableAvatarAI(const std::string& avatarName) {
    Person* avatar = getAvatar(avatarName);
    if (avatar) {
        avatar->setMode(Person::GameMode::Creative);
        std::cout << "Disabled AI for " << avatarName << std::endl;
    }
}

void AvatarManager::setAvatarBehavior(const std::string& avatarName, const std::string& behavior) {
    Person* avatar = getAvatar(avatarName);
    if (avatar) {
        // Could implement different behavior patterns here
        std::cout << "Set behavior '" << behavior << "' for " << avatarName << std::endl;
    }
}

void AvatarManager::saveAvatarState(const std::string& avatarName, const std::string& filename) {
    Person* avatar = getAvatar(avatarName);
    if (avatar) {
        nlohmann::json j;
        j["avatarName"] = avatar->getSoulName();
        j["health"] = avatar->state.health;
        j["energy"] = avatar->state.energy;
        j["level"] = avatar->state.level;
        j["experience"] = avatar->state.experience;
        j["height"] = avatar->state.height;
        j["weight"] = avatar->state.weight;
        j["hairStyle"] = avatar->state.hairStyle;
        j["eyeColor"] = avatar->state.eyeColor;
        j["skinTone"] = avatar->state.skinTone;
        j["mode"] = static_cast<int>(avatar->getMode());
        
        // Save body parts if available
        if (avatar->getBody().parts.size() > 0) {
            nlohmann::json partsArray = nlohmann::json::array();
            for (auto* part : avatar->getBody().parts) {
                if (part) {
                    nlohmann::json partJson;
                    partJson["name"] = part->getName();
                    auto dims = part->getGeometry().getDimensions();
                    partJson["dimensions"] = {dims.x, dims.y, dims.z};
                    auto col = part->getColor();
                    partJson["color"] = {col[0], col[1], col[2]};
                    const glm::mat4& T = part->getTransform();
                    std::vector<float> tvals(16);
                    const float* ptrT = &T[0][0];
                    for(int k=0;k<16;++k) tvals[k]=ptrT[k];
                    partJson["transform"] = tvals;
                    partsArray.push_back(partJson);
                }
            }
            j["bodyParts"] = partsArray;
        }
        
        // Use SaveSystem to write the file
        SaveSystem::writeJson(j, avatarName, SaveSystem::SaveType::AVATAR);
        std::cout << "Saved avatar state for " << avatarName << std::endl;
    }
}

void AvatarManager::loadAvatarState(const std::string& avatarName, const std::string& filename) {
    Person* avatar = getAvatar(avatarName);
    if (avatar) {
        try {
            std::ifstream file(filename);
            if (file.is_open()) {
                nlohmann::json j;
                file >> j;
                
                avatar->state.health = j.value("health", 100.0f);
                avatar->state.energy = j.value("energy", 100.0f);
                avatar->state.level = j.value("level", 1);
                avatar->state.experience = j.value("experience", 0.0f);
                avatar->state.height = j.value("height", 1.7f);
                avatar->state.weight = j.value("weight", 70.0f);
                avatar->state.hairStyle = j.value("hairStyle", "");
                avatar->state.eyeColor = j.value("eyeColor", "");
                avatar->state.skinTone = j.value("skinTone", "");
                
                if (j.contains("mode")) {
                    avatar->setMode(static_cast<Person::GameMode>(j["mode"].get<int>()));
                }
                
                // Load body parts if available
                if (j.contains("bodyParts")) {
                    const auto& partsArray = j["bodyParts"];
                    for (size_t i = 0; i < partsArray.size() && i < avatar->getBody().parts.size(); ++i) {
                        auto* part = avatar->getBody().parts[i];
                        if (part && partsArray[i].contains("dimensions")) {
                            auto dims = partsArray[i]["dimensions"];
                            if (dims.size() >= 3) {
                                part->getGeometry().setDimensions(glm::vec3(dims[0], dims[1], dims[2]));
                            }
                        }
                        if (part && partsArray[i].contains("color")) {
                            auto col = partsArray[i]["color"];
                            if (col.size() >= 3) {
                                part->setColor(col[0], col[1], col[2]);
                            }
                        }
                        if (part && partsArray[i].contains("transform")) {
                            auto tvals = partsArray[i]["transform"].get<std::vector<float>>();
                            if (tvals.size() == 16) {
                                glm::mat4 T;
                                std::memcpy(glm::value_ptr(T), tvals.data(), sizeof(float) * 16);
                                part->setTransform(T);
                            }
                        }
                    }
                }
                
                file.close();
                std::cout << "Loaded avatar state for " << avatarName << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading avatar state: " << e.what() << std::endl;
        }
    }
}

void AvatarManager::saveAllAvatars(const std::string& directory) {
    for (auto* avatar : avatars) {
        saveAvatarState(avatar->getSoulName(), ""); // Use SaveSystem's automatic naming
    }
}

void AvatarManager::loadAllAvatars(const std::string& directory) {
    // This would need to be implemented based on the file structure
    std::cout << "Load all avatars from " << directory << " (not implemented)" << std::endl;
} 