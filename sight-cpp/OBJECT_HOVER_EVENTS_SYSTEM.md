# Object Hover Events System

This document explains the event system for Object hover detection, which triggers events when the mouse hovers over any Object in the system.

## What are Object Hover Events?

The Object Hover Events system automatically detects when the mouse cursor is hovering over Objects and triggers appropriate events. This allows your code to react when:

- The mouse enters an Object's area
- The mouse is hovering over an Object
- The mouse exits an Object's area

## Available Events

### 1. ObjectHoverEvent
**Triggered when:** The mouse is hovering over an Object
**Contains:**
- `object`: Reference to the Object being hovered
- `hoverPoint`: 3D world coordinates where the mouse is hovering
- `screenPosition`: 2D screen coordinates of the mouse
- `timestamp`: When the event was triggered

### 2. ObjectHoverEnterEvent
**Triggered when:** The mouse first enters an Object's area
**Contains:**
- `object`: Reference to the Object that was entered
- `hoverPoint`: 3D world coordinates where the mouse entered
- `screenPosition`: 2D screen coordinates where the mouse entered
- `timestamp`: When the event was triggered

### 3. ObjectHoverExitEvent
**Triggered when:** The mouse leaves an Object's area
**Contains:**
- `object`: Reference to the Object that was exited
- `lastHoverPoint`: 3D world coordinates where the mouse last hovered
- `lastScreenPosition`: 2D screen coordinates where the mouse last was
- `timestamp`: When the event was triggered

## How to use it

### 1. Subscribe to events

```cpp
#include "Core/EventBus.hpp"
#include "Form/Object/Object.hpp"

// Create handlers for each event type
void handleObjectHover(const ObjectHoverEvent& event) {
    std::cout << "Hovering over: " << event.object.getIdentifier() << std::endl;
    std::cout << "At position: (" << event.hoverPoint.x << ", " << event.hoverPoint.y << ", " << event.hoverPoint.z << ")" << std::endl;
}

void handleObjectEnter(const ObjectHoverEnterEvent& event) {
    std::cout << "Entered object: " << event.object.getIdentifier() << std::endl;
    // Could highlight the object, show tooltip, etc.
}

void handleObjectExit(const ObjectHoverExitEvent& event) {
    std::cout << "Exited object: " << event.object.getIdentifier() << std::endl;
    // Could remove highlight, hide tooltip, etc.
}

// Subscribe to all hover events
Core::EventBus::instance().subscribe<ObjectHoverEvent>(handleObjectHover);
Core::EventBus::instance().subscribe<ObjectHoverEnterEvent>(handleObjectEnter);
Core::EventBus::instance().subscribe<ObjectHoverExitEvent>(handleObjectExit);
```

### 2. Events trigger automatically

The events are triggered automatically when you call the hover detection methods:

```cpp
// Create an object
Object cube;
cube.setObjectID("my_cube");

// Update hover state (triggers events automatically)
cube.updateHoverState(true);   // Triggers ObjectHoverEnterEvent
cube.updateHoverState(true);   // Triggers ObjectHoverEvent
cube.updateHoverState(false);  // Triggers ObjectHoverExitEvent
```

## Object Hover Detection Methods

### Hover Detection
- `isMouseHovering(mousePos, viewMatrix, projectionMatrix, windowWidth, windowHeight)`: Check if mouse is hovering using screen coordinates
- `isMouseHovering(worldMousePos)`: Check if mouse is hovering using world coordinates
- `updateHoverState(isHovering)`: Update hover state and trigger events

### State Queries
- `getIsHovered()`: Check if object is currently being hovered
- `getHoverPoint()`: Get the current hover point in world coordinates

## Example Use Cases

### Object Highlighting
```cpp
void highlightObject(const ObjectHoverEnterEvent& event) {
    // Highlight the object when mouse enters
    event.object.setHighlight(true);
}

void removeHighlight(const ObjectHoverExitEvent& event) {
    // Remove highlight when mouse exits
    event.object.setHighlight(false);
}
```

### Tooltips and Information
```cpp
void showTooltip(const ObjectHoverEvent& event) {
    // Show tooltip with object information
    std::string info = "Object: " + event.object.getIdentifier() + 
                      "\nType: " + event.object.getObjectType() +
                      "\nPosition: (" + std::to_string(event.hoverPoint.x) + 
                      ", " + std::to_string(event.hoverPoint.y) + 
                      ", " + std::to_string(event.hoverPoint.z) + ")";
    ui.showTooltip(info, event.screenPosition);
}
```

### Interactive Selection
```cpp
void handleObjectSelection(const ObjectHoverEnterEvent& event) {
    // Show selection indicator
    if (isSelectionMode) {
        event.object.showSelectionIndicator();
    }
}
```

### Debug Information
```cpp
void debugObjectInfo(const ObjectHoverEvent& event) {
    // Log detailed object information for debugging
    std::cout << "Debug: Hovering over object " << event.object.getIdentifier() << std::endl;
    std::cout << "  Geometry Type: " << static_cast<int>(event.object.getGeometryType()) << std::endl;
    std::cout << "  Transform: " << glm::to_string(event.object.getTransform()) << std::endl;
}
```

## Integration with Mouse System

To integrate with the existing mouse system, you would typically:

1. **Get mouse position** from your input system
2. **Check all objects** for hover detection
3. **Update hover states** which triggers events automatically

```cpp
// In your main update loop
void updateObjectHovering() {
    glm::vec2 mousePos = getMousePosition();
    glm::mat4 viewMatrix = getViewMatrix();
    glm::mat4 projectionMatrix = getProjectionMatrix();
    int windowWidth = getWindowWidth();
    int windowHeight = getWindowHeight();
    
    for (auto& object : allObjects) {
        bool isHovering = object->isMouseHovering(mousePos, viewMatrix, projectionMatrix, windowWidth, windowHeight);
        object->updateHoverState(isHovering);
    }
}
```

## Important Notes

- **Automatic event triggering**: Events are triggered automatically when `updateHoverState()` is called
- **State tracking**: Objects track their own hover state internally
- **Performance**: Hover detection uses the existing collision detection system
- **Thread-safe**: The EventBus system handles concurrent access safely
- **3D and 2D coordinates**: Events provide both world and screen coordinates

## Running the Example

To see Object hover events in action, compile and run the example:

```bash
g++ -o object_hover_events_example examples/object_hover_events_example.cpp src/Form/Object/Object.cpp src/Core/EventBus.cpp
./object_hover_events_example
```

This will demonstrate:
- Object creation
- Hover enter events
- Continuous hover events
- Hover exit events
- State tracking

## Integration with Existing Systems

The Object Hover Events system integrates seamlessly with:
- **Object System**: Uses existing collision detection
- **EventBus**: All events use the existing event system
- **Mouse System**: Can integrate with existing mouse handling
- **UI System**: Perfect for tooltips and highlighting
- **Debug System**: Useful for debugging object interactions

This provides a complete event-driven system for tracking mouse interactions with Objects in your application! 🖱️ 