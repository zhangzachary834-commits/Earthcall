#include "../src/Core/EventBus.hpp"
#include "../src/Form/Object/Object.hpp"
#include "../src/Form/Object/Formation/Formations.hpp"
#include <iostream>
#include <vector>
#include <memory>

// Event handlers for Object hover events
void handleObjectHover(const ObjectHoverEvent& event) {
    std::cout << "\n🖱️ === Object Hovering ===" << std::endl;
    std::cout << "Object: " << event.object.getIdentifier() << std::endl;
    std::cout << "Hover Point: (" << event.hoverPoint.x << ", " << event.hoverPoint.y << ", " << event.hoverPoint.z << ")" << std::endl;
    std::cout << "Screen Position: (" << event.screenPosition.x << ", " << event.screenPosition.y << ")" << std::endl;
    std::cout << "Timestamp: " << event.timestamp << std::endl;
    std::cout << "=========================" << std::endl;
}

void handleObjectHoverEnter(const ObjectHoverEnterEvent& event) {
    std::cout << "\n🎯 === Object Hover Enter ===" << std::endl;
    std::cout << "Object: " << event.object.getIdentifier() << std::endl;
    std::cout << "Enter Point: (" << event.hoverPoint.x << ", " << event.hoverPoint.y << ", " << event.hoverPoint.z << ")" << std::endl;
    std::cout << "Screen Position: (" << event.screenPosition.x << ", " << event.screenPosition.y << ")" << std::endl;
    std::cout << "Timestamp: " << event.timestamp << std::endl;
    std::cout << "=============================" << std::endl;
}

void handleObjectHoverExit(const ObjectHoverExitEvent& event) {
    std::cout << "\n👋 === Object Hover Exit ===" << std::endl;
    std::cout << "Object: " << event.object.getIdentifier() << std::endl;
    std::cout << "Exit Point: (" << event.lastHoverPoint.x << ", " << event.lastHoverPoint.y << ", " << event.lastHoverPoint.z << ")" << std::endl;
    std::cout << "Screen Position: (" << event.lastScreenPosition.x << ", " << event.lastScreenPosition.y << ")" << std::endl;
    std::cout << "Timestamp: " << event.timestamp << std::endl;
    std::cout << "============================" << std::endl;
}

int main() {
    std::cout << "=== Object Hover Events Demo ===" << std::endl;
    
    // Subscribe to all Object hover events
    Core::EventBus::instance().subscribe<ObjectHoverEvent>(handleObjectHover);
    Core::EventBus::instance().subscribe<ObjectHoverEnterEvent>(handleObjectHoverEnter);
    Core::EventBus::instance().subscribe<ObjectHoverExitEvent>(handleObjectHoverExit);
    
    // Create some test objects
    std::vector<std::unique_ptr<Object>> objects;
    
    std::cout << "\n--- Creating Objects ---" << std::endl;
    
    // Create a cube
    auto cube = std::make_unique<Object>();
    cube->setObjectID("test_cube_001");
    cube->setObjectType("Cube");
    cube->setX(0);
    cube->setY(0);
    cube->setZ(0);
    objects.push_back(std::move(cube));
    
    // Create a sphere
    auto sphere = std::make_unique<Object>();
    sphere->setObjectID("test_sphere_001");
    sphere->setObjectType("Sphere");
    sphere->setGeometryType(Object::GeometryType::Sphere);
    sphere->setX(2);
    sphere->setY(0);
    sphere->setZ(0);
    objects.push_back(std::move(sphere));
    
    // Create a polyhedron
    auto polyhedron = std::make_unique<Object>();
    polyhedron->setObjectID("test_polyhedron_001");
    polyhedron->setObjectType("Polyhedron");
    polyhedron->createOctahedron();
    polyhedron->setX(-2);
    polyhedron->setY(0);
    polyhedron->setZ(0);
    objects.push_back(std::move(polyhedron));
    
    std::cout << "Created " << objects.size() << " objects" << std::endl;
    
    std::cout << "\n--- Simulating Hover Events ---" << std::endl;
    
    // Simulate hover events for each object
    for (auto& obj : objects) {
        // Simulate mouse entering the object
        std::cout << "Simulating hover enter for: " << obj->getIdentifier() << std::endl;
        obj->updateHoverState(true);
        
        // Simulate continuous hovering
        std::cout << "Simulating continuous hover for: " << obj->getIdentifier() << std::endl;
        obj->updateHoverState(true);
        
        // Simulate mouse exiting the object
        std::cout << "Simulating hover exit for: " << obj->getIdentifier() << std::endl;
        obj->updateHoverState(false);
        
        std::cout << std::endl;
    }
    
    std::cout << "\n--- Checking Object States ---" << std::endl;
    
    // Check final states
    for (auto& obj : objects) {
        std::cout << "Object: " << obj->getIdentifier() << std::endl;
        std::cout << "  Is Hovered: " << (obj->getIsHovered() ? "Yes" : "No") << std::endl;
        std::cout << "  Hover Point: (" << obj->getHoverPoint().x << ", " << obj->getHoverPoint().y << ", " << obj->getHoverPoint().z << ")" << std::endl;
    }
    
    std::cout << "\n=== Demo Complete ===" << std::endl;
    
    return 0;
} 