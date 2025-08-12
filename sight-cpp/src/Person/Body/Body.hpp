#pragma once
#include <string>
#include <vector>
#include <map>
#include "Form/Object/Object.hpp"
#include "Form/Object/Formation/Formations.hpp"
#include "Singular.hpp"
#include "BodyPart/BodyPart.hpp"

/* The same class that will create the characters will also be used for all meta-bodies. 
Or even if not the same class, it will be the same structure */

class Body : public Object {
public:
    enum class BodyType {
        Humanoid,
        Quadruped,
        Avian,
        Aquatic,
        Mechanical,
        Ethereal
    };

    enum class Proportions {
        Child,
        Teen,
        Adult,
        Elder
    };

    struct Clothing {
        std::string name;
        std::string slot;  // head, torso, legs, feet, etc.
        std::string material;
        float protection = 0.0f;
        float warmth = 0.0f;
        bool isEquipped = false;
    };

    std::string shape;
    std::string artStyle;
    std::vector<std::string> adornments;
    
    // Enhanced body system
    BodyType bodyType = BodyType::Humanoid;
    Proportions proportions = Proportions::Adult;
    float height = 1.8f;  // meters
    float weight = 70.0f; // kg
    float muscleMass = 0.5f;  // 0.0 to 1.0
    float bodyFat = 0.2f;     // 0.0 to 1.0
    
    // Body measurements
    struct Measurements {
        float chest = 0.9f;
        float waist = 0.7f;
        float hips = 0.9f;
        float shoulders = 0.5f;
        float armLength = 0.7f;
        float legLength = 0.9f;
    } measurements;

    // Collection of body parts (owned elsewhere)
    std::vector<BodyPart*> parts;
    Formations           formation;  // group managing body parts as objects

    // Clothing system
    std::map<std::string, Clothing> clothing;
    std::vector<std::string> equippedItems;

    // Simple hitbox dimensions (height). Width/depth can be added later.
    float hitboxHeight = 1.0f; // meters

    float getHeight() const { return hitboxHeight; }
    float getEyeHeight() const { return hitboxHeight * 0.9f; } // eye slightly below top
    float getNametagHeight() const { return hitboxHeight + 0.2f; }

    Body(std::string shape, std::string artStyle);

    void describe() const;
    void addAdornment(const std::string& item);

    // Add a body part to this body (and formation)
    void addPart(BodyPart* part);

    // Draw the body (simply draws each part)
    void draw() const;

    // Factory: build a simple humanoid body composed of basic parts
    static Body createBasicAvatar(const std::string& artStyle = "Voxel");
    
    // Enhanced factory methods
    static Body createCustomAvatar(const std::string& artStyle, BodyType type, Proportions props);
    static Body createChildAvatar(const std::string& artStyle = "Voxel");
    static Body createElderAvatar(const std::string& artStyle = "Voxel");
    
    // Body customization
    void setBodyType(BodyType type);
    void setProportions(Proportions props);
    void setHeight(float h);
    void setWeight(float w);
    void setMuscleMass(float mass);
    void setBodyFat(float fat);
    void updateMeasurements();
    void scaleBodyParts();
    
    // Clothing system
    void addClothing(const Clothing& item);
    bool equipClothing(const std::string& itemName);
    bool unequipClothing(const std::string& itemName);
    const Clothing* getEquippedClothing(const std::string& slot) const;
    float getTotalProtection() const;
    float getTotalWarmth() const;
    
    // Body part management
    BodyPart* getBodyPart(const std::string& name) const;
    std::vector<BodyPart*> getBodyPartsByType(BodyPart::Type type) const;
    void removeBodyPart(const std::string& name);
    
    // Health and status
    bool isInjured() const;
    bool isHealthy() const;
    float getOverallHealth() const;

    // Singular interface
    std::string getIdentifier() const override { return shape + "_body"; }
};