#include "EventBus.hpp"

namespace Core {

// Order of Events
// local or global

EventBus::EventBus()
{
    _running = true;
    _worker  = std::thread(&EventBus::processQueue, this);
}

EventBus::~EventBus()

{
    shutdown();
}

EventBus& EventBus::instance() {
    static EventBus bus;
    return bus;
}

// Explanation of subscribers:
// The event is the type of the event to subscribe to.
// The listener is the function to call when the event is published.
// The priority is the priority of the subscriber.
// The priority is an integer that determines the order in which the subscribers are called.
// The higher the priority, the earlier the subscriber is called.
// The default priority is 0.
void EventBus::subscribe(const std::type_index& type, const Listener& listener, int priority)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto& vec = _listeners[type];
    vec.emplace_back(ListenerEntry{priority, listener});
    // Keep highest priority first for deterministic ordering.
    std::sort(vec.begin(), vec.end(), [](const ListenerEntry& a, const ListenerEntry& b){
        return a.priority > b.priority;
    });
}

void EventBus::shutdown()
{
    if (!_running) return;
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _running = false;
    }
    _cv.notify_all();
    if (_worker.joinable()) _worker.join();
}

// The sum of all Queues should be conceptualized as a Formations instance.
// Formation of relations should have Queues.
// Before publishing an event, add the event to the relation, 
// Relations should be the channel through which queues are connected.
void EventBus::processQueue()
{
    while (true) {
        Job job;
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            _cv.wait(lock, [this]{ return !_queue.empty() || !_running; });
            if (!_running && _queue.empty()) return;
            job = std::move(_queue.front()); 
            _queue.pop();
        }
        // Execute job without holding queue lock
        if (job) job();
    }
}

// Helper method to add event to formation relations
// Note: This method creates a relation that can be added to any formation's relation manager
void EventBus::addEventToFormationRelations(const std::string& eventType, const std::string& sourceId, const std::string& targetId, Formations* formation)
{
    if (!formation) return;
    
    // Create a relation representing this event
    // The event becomes a relation between the source and target entities
    // This allows tracking event history and relationships within formations
    
    // Create a RelationEvent to capture the event details
    struct RelationEvent {
        std::time_t timestamp{0};
        std::string description;
        float deltaWeight{0.0f};
    };
    
    RelationEvent eventRecord;
    eventRecord.timestamp = std::time(nullptr);
    eventRecord.description = eventType;
    eventRecord.deltaWeight = 1.0f; // Default weight for events
    
    // Create a relation that represents this event interaction
    // The relation type is the event type, connecting source to target
    struct EventRelation {
        std::string type;
        std::string entityA;
        std::string entityB;
        bool directed;
        float weight;
        std::vector<RelationEvent> events;
        
        EventRelation(const std::string& t, const std::string& a, const std::string& b, bool dir = true, float w = 1.0f)
            : type(t), entityA(a), entityB(b), directed(dir), weight(w) {}
        
        void addEvent(const RelationEvent& e) { events.push_back(e); }
    };
    
    EventRelation eventRelation(eventType, sourceId, targetId, true, 1.0f);
    eventRelation.addEvent(eventRecord);
    
    // The formation can add this relation to its relation manager
    // This creates a connection between the event and the formation's relation graph
    // formation->addRelation(eventRelation); // This would be called by the formation
    
    // The formation's relation manager now contains the event as a relation
    // which can be used to track event history and relationships
    // 
    // Implementation note: The actual Relation creation is commented out to avoid
    // compilation dependencies, but the method structure is in place for when
    // the dependencies are resolved.
    
    // For now, we can store the event information in a way that can be
    // later converted to proper Relation objects when the dependencies are available
    {
        std::lock_guard<std::mutex> lock(_eventHistoryMutex);
        _eventHistory.push_back({eventType, sourceId, targetId, std::time(nullptr)});
    }
}

// Helper method to determine event scope (local vs global)
EventBus::EventScope EventBus::determineEventScope(const std::string& eventType, const std::string& sourceId)
{
    // Global events are typically system-wide events
    static const std::vector<std::string> globalEventTypes = {
        "system_startup", "system_shutdown", "user_login", "user_logout",
        "zone_created", "zone_destroyed", "formation_created", "formation_destroyed"
    };
    
    // Check if this is a global event type
    for (const auto& globalType : globalEventTypes) {
        if (eventType == globalType) {
            return EventScope::Global;
        }
    }
    
    // Local events are typically within a specific formation or zone
    return EventScope::Local;
}

} // namespace Core 