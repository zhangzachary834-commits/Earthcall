# Event Bus vs Event Handler - What's the Difference?

## Simple Explanation

Think of it like a **mail delivery system**:

### Event Bus = Post Office
- **What it does**: Delivers messages (events) to the right people
- **Your EventBus**: Manages who gets what events and when
- **Doesn't**: Read or process the messages itself

### Event Handler = Mail Clerk
- **What it does**: Reads and processes the messages when they arrive
- **Your EventHandler**: Contains the actual code that responds to events
- **Does**: The work when an event happens

## In Your Code

### You Already Have Both!

**Event Bus** (`EventBus.hpp/cpp`):
```cpp
// This is your "post office"
Core::EventBus::instance().publish(PlayerJumped{42});
Core::EventBus::instance().subscribe<PlayerJumped>(handlePlayerJumped);
```

**Event Handlers** (scattered throughout your code):
```cpp
// These are your "mail clerks"
void handlePlayerJumped(const PlayerJumped& event) {
    std::cout << "Player " << event.playerId << " jumped!\n";
}

void handleMouseMove(double x, double y) {
    // Process mouse movement
}
```

## The New EventHandler Class

I created a new `EventHandler` class that makes things **cleaner and easier to manage**:

### Before (scattered):
```cpp
// Handlers scattered everywhere
Core::EventBus::instance().subscribe<PlayerJumped>(handlePlayerJumped);
Core::EventBus::instance().subscribe<ItemCollected>(handleItemCollected);
Core::EventBus::instance().subscribe<GameStateChanged>(handleGameStateChanged);
```

### After (centralized):
```cpp
// All handlers in one place
auto& handler = Core::EventHandler::instance();
handler.registerHandler<PlayerJumped>("player_jump", handlePlayerJumped);
handler.registerHandler<ItemCollected>("item_collected", handleItemCollected);
handler.registerHandler<GameStateChanged>("game_state", handleGameStateChanged);
```

## Benefits of the New EventHandler

1. **Better Organization**: All your event handlers are registered in one place
2. **Easier Management**: You can see all handlers, remove them, or check if they exist
3. **Cleaner Code**: Less repetitive EventBus calls
4. **Named Handlers**: Each handler has a name, making debugging easier

## Do You Need It?

**You don't strictly need it** - your current system works fine! But the new `EventHandler` makes your code:
- More organized
- Easier to maintain
- Cleaner to read
- Better for debugging

## Usage Examples

### Basic Usage:
```cpp
// Register a handler
Core::EventHandler::instance().registerHandler<PlayerJumped>("jump_handler", handlePlayerJumped);

// Publish an event
Core::EventHandler::instance().publish(PlayerJumped{42, 5.5f, "now"});
```

### Advanced Usage:
```cpp
auto& handler = Core::EventHandler::instance();

// Register with priority (higher numbers run first)
handler.registerHandler<PlayerJumped>("high_priority", handleJumpHighPriority, 10);
handler.registerHandler<PlayerJumped>("normal_priority", handleJumpNormal, 0);

// Check what handlers you have
std::cout << "Handlers: " << handler.getHandlerCount() << std::endl;
for (const auto& name : handler.getRegisteredHandlers()) {
    std::cout << "  - " << name << std::endl;
}

// Remove a handler
handler.removeHandler("jump_handler");
```

## Summary

- **EventBus**: Your existing "post office" - delivers events
- **Event Handlers**: Your existing "mail clerks" - process events  
- **New EventHandler**: A "manager" that organizes all your mail clerks in one place

You can use either approach - the new EventHandler just makes things cleaner! 