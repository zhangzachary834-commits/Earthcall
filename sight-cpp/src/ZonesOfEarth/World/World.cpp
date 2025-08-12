#include "World.hpp"
#include <GLFW/glfw3.h>
#include <OpenGL/glu.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/glm.hpp"
#include "Form/Object/Object.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "Rendering/HighlightSystem.hpp"

void World::update(float dt){
    if(!_cameraPos) return;
    // ground Y based on object tagged as baseline ground if exists; fall back to index 1
    float groundY = 0.0f;
    size_t groundIdx = 1;
    for (size_t i = 0; i < _objects.size(); ++i) {
        if (_objects[i] && _objects[i]->hasAttribute("baseline") && _objects[i]->getAttribute("baseline") == std::string("ground")) { groundIdx = i; break; }
    }
    if(_objects.size()>groundIdx && _objects[groundIdx]){
        const glm::mat4& gT = _objects[groundIdx]->getTransform();
        float scaleY = glm::length(glm::vec3(gT[1]));
        groundY = gT[3][1] + 0.5f*scaleY;
    }

    // Sub-step large delta times to avoid physics explosions during blocking operations (e.g., saving)
    const float maxStep = 0.02f; // 50 FPS equivalent per sub-step
    int steps = std::max(1, (int)std::ceil(dt / maxStep));
    float stepDt = dt / steps;

    for (int s = 0; s < steps; ++s) {
        Physics::applyGravity(*_cameraPos, physicsEnabled, static_cast<Physics::GameMode>(mode), stepDt, groundY);
        if(mode==Mode::Survival && Physics::getFlying()) Physics::setFlying(false);
        if(physicsEnabled){
            for(const auto& up: _objects) if(up) Physics::getBodyFor(up.get());
            Physics::updateBodies(_objects, stepDt, 9.81f, 0.1f, groundY);
            Physics::enforceCollisions(*_cameraPos, _objects);
        }
    }
}

void World::drawGround(){
    // Draw ground quad separately (simple green plane)
    // --------------------------------------------------------------
    glPushMatrix();
    glNormal3f(0.0f, 1.0f, 0.0f);
    glColor3f(0.4f, 0.7f, 0.5f);
    float groundSize = 100.0f;

    // The ground placeholder cube has a height of 1.0 after scaling. Its top surface sits at +0.5 in world space.
    // Render the quad at this height so it visually matches the physics collision plane.
    float groundY = 0.5f;
    glBegin(GL_QUADS);
    glVertex3f(-groundSize, groundY, -groundSize);
    glVertex3f( groundSize, groundY, -groundSize);
    glVertex3f( groundSize, groundY,  groundSize);
    glVertex3f(-groundSize, groundY,  groundSize);
    glEnd();
    glPopMatrix();

}

void World::load() {
    // Load the world
    std::cout << "🌍 World::load() - Starting world initialization..." << std::endl;
    
    mode = Mode::Creative;
    // Initialize physics system
    physicsEnabled = true;
    
    // Create a basic ground plane 
    drawGround();
    
    // TODO: Load world configuration from save file
    // TODO: Load saved objects and their states
    // TODO: Set up environment settings (lighting, atmosphere, etc.)
    // TODO: Initialize any world-specific systems
    
    std::cout << "🌍 World::load() - World loaded successfully with " << _objects.size() << " objects" << std::endl;
}

void World::unload(){
    // Unload the world
    std::cout << "🌍 World::unload() - Unloading world..." << std::endl;
    _objects.clear();
    // Its not enough to just clear the vector, we have to stop the visual generator system and delete the memory
    // Save the objects before storing. Refactor Game.cpp's save system to here.
    std::cout << "🌍 World::unload() - World unloaded successfully" << std::endl;
}

void World::render() const{}
// Hook highlight rendering into World::render (non-invasive): draw after normal render path
// Minimal change: render highlights for all objects if flagged
// Note: For now, World::render is empty; we ensure callers render objects then we overlay outlines here if needed.

void World::addObject(std::unique_ptr<Object> obj){ _objects.push_back(std::move(obj)); }

World::~World() = default; 