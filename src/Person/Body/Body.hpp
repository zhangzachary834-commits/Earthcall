#pragma once
#include <string>
#include <vector>
#include <map>
#include "ConstructedBeing/Singular/Singular.hpp"
#include "Relation/Formation/Formation.hpp"
#include "BodyPart/BodyPart.hpp"

/* The same class that will create the characters will also be used for all meta-bodies. 
Or even if not the same class, it will be the same structure */

class Body : public Singular {

public:
    Body(std::string shape = "humanoid", std::string artStyle = "default");
    Body(Body&&) = default;
    Body& operator=(Body&&) = default;
    Body(const Body&) = default;
    Body& operator=(const Body&) = default;
    
    std::string shape;
    std::string artStyle;
    std::vector<std::string> adornments;
    // Physical characteristics (can be overridden by properties later if needed)
    float height = 1.8f;  // meters

    // Collection of body parts (owned elsewhere)
    std::vector<BodyPart*> parts;
    Formation           formation;  // group managing body parts as objects


    // Simple hitbox dimensions (height). Width/depth can be added later.
    float hitboxHeight = 1.0f; // meters

    float getHeight() const { return hitboxHeight; }
    float getEyeHeight() const { return hitboxHeight * 0.9f; } // eye slightly below top
    float getNametagHeight() const { return hitboxHeight + 0.2f; }

    void describe() const;
    void addAdornment(const std::string& item);

    // Add a body part to this body (and formation)
    void addPart(BodyPart* part);

    // Draw the body (simply draws each part)
    void draw() const;

    // Factory: build a simple humanoid body composed of basic parts
    static Body createBasicAvatar(const std::string& artStyle = "Voxel");
    
    // Factory: build an authored body from an ObjectConcept
    static Body createFromConcept(const class ObjectConcept& concept);
    // Body customization
    void setHeight(float h);
    void scaleBodyParts();
    
    // Body part management
    BodyPart* getBodyPart(const std::string& name) const;
    std::vector<BodyPart*> getBodyPartsByType(BodyPart::Type type) const;
    void removeBodyPart(const std::string& name);


    // Singular interface
    std::string getIdentifier() const override { return shape + "_body"; }

protected:
    void buildProperties() override;
};