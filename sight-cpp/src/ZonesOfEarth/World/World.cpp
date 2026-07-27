#include "World.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/glm.hpp"
#include "Form/Object/Object.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "Rendering/HighlightSystem.hpp"
#include "Rendering/Renderer.hpp"

void World::update(float dt){
    // NOTE: the player/camera is no longer simulated here. All player movement,
    // gravity, ground/support contact and collision live in Game::stepMovement
    // so there is a single authoritative writer of the camera position. World
    // simulates only the world's own objects (rotations, automations, physics).

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

    // Sub-step large delta times to avoid physics explosions during blocking operations (e.g., saving).
    // Anti-"spiral of death": never simulate more than maxFrameTime of physics in a single frame.
    // Without this clamp, one slow frame -> larger dt -> more substeps -> even slower frame, runaway
    // (measured: steps 1 -> 17 -> 105 as dt grew 0.0 -> 0.33 -> 2.08s, effectively freezing the app).
    // Clamping dt drops the excess wall-clock time instead of trying to "catch up" all at once, so the
    // substep count is bounded (here <= 0.1/0.02 = 5) and the sim recovers on the next frame.
    const float maxStep = 0.02f;      // 50 FPS equivalent per sub-step
    const float maxFrameTime = 0.1f;  // cap simulated time per frame (anti-spiral)
    if (dt > maxFrameTime) dt = maxFrameTime;
    int steps = std::max(1, (int)std::ceil(dt / maxStep));
    float stepDt = dt / steps;

    for (int s = 0; s < steps; ++s) {
        if(mode==Mode::Survival && Physics::getFlying()) Physics::setFlying(false);
        for (const auto& up : _objects) {
            if (!up) continue;
            if (up->hasPendingRotation()) {
                up->updateRotation(stepDt);
            }
            if (up->hasAutomations()) {
                up->updateAutomations(stepDt);
            }
        }
        if(physicsEnabled){
            for(const auto& up: _objects) if(up) Physics::getBodyFor(up.get());
            Physics::updateBodies(_objects, stepDt, 9.81f, 0.1f, groundY);
        }
    }
}

void World::drawGround(){
    // Draw ground quad separately (simple green plane)
    // --------------------------------------------------------------
    // Unlike the gizmos, this quad carries a normal and IS lit, so it goes through
    // drawMesh with a material rather than the unlit drawSolid path.
    static const geom::TessMesh mesh = [] {
        const float s = 100.0f;
        // The ground placeholder cube has a height of 1.0 after scaling. Its top
        // surface sits at +0.5 in world space. Render the quad at this height so it
        // visually matches the physics collision plane.
        const float y = 0.5f;
        const glm::vec3 corners[4] = {
            {-s, y, -s}, { s, y, -s}, { s, y,  s}, {-s, y,  s}};
        // Quad -> two triangles, same winding GL_QUADS used.
        const int order[6] = {0, 1, 2, 0, 2, 3};
        geom::TessMesh m;
        for (int i : order) {
            geom::TessVertex v;
            v.pos = corners[i];
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            // UVs so a future ground texture tiles rather than stretching.
            v.uv = glm::vec2((corners[i].x / s + 1.0f) * 0.5f,
                             (corners[i].z / s + 1.0f) * 0.5f);
            m.tris.push_back(v);
        }
        return m;
    }();

    RenderMaterial mat;
    mat.baseColor = glm::vec3(0.4f, 0.7f, 0.5f);

    currentRenderer().setModel(glm::mat4(1.0f));
    currentRenderer().drawMesh(mesh, mat);
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
