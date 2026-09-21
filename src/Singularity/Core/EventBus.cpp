#include "EventBus.hpp"

namespace Core {

// Order of Events
// local or global

EventBus::EventBus()
{
    _running = true;
#ifndef __EMSCRIPTEN__
    _worker  = std::thread(&EventBus::processQueue, this);
#endif
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
EventBus::SubscriptionId EventBus::subscribe(const std::type_index& type,
                                                   const Listener& listener,
                                                   int priority)
{
    std::lock_guard<std::mutex> lock(_mutex);
    const SubscriptionId id = _nextSubscriptionId++;
    auto state = std::make_shared<SubscriptionState>();
    auto it = _listeners.find(type);
    auto newVec = std::make_shared<std::vector<ListenerEntry>>();
    if (it != _listeners.end() && it->second) {
        *newVec = *it->second;
    }
    newVec->emplace_back(ListenerEntry{priority, id, std::move(state), listener});
    // Keep highest priority first for deterministic ordering.
    std::sort(newVec->begin(), newVec->end(), [](const ListenerEntry& a, const ListenerEntry& b){
        return a.priority > b.priority;
    });
    _listeners[type] = newVec;
    return id;
}

bool EventBus::unsubscribe(SubscriptionId id)
{
    if (id == 0) return false;

    // Do not wait on a per-road gate while holding _mutex: a listener is
    // allowed to subscribe or close another road while it is being delivered.
    std::shared_ptr<SubscriptionState> state;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& [type, listeners] : _listeners) {
            (void)type;
            if (!listeners) continue;
            auto it = std::find_if(listeners->begin(), listeners->end(),
                                   [&](const ListenerEntry& entry) { return entry.id == id; });
            if (it != listeners->end()) {
                state = it->state;
                break;
            }
        }
    }
    if (!state) return false;

    {
        std::lock_guard<std::recursive_mutex> gate(state->mutex);
        if (!state->active) return false;
        state->active = false;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    for (auto it = _listeners.begin(); it != _listeners.end(); ++it) {
        const auto& listeners = it->second;
        if (!listeners) continue;
        const bool contains = std::any_of(listeners->begin(), listeners->end(),
                                          [&](const ListenerEntry& entry) { return entry.id == id; });
        if (!contains) continue;

        auto newVec = std::make_shared<std::vector<ListenerEntry>>();
        newVec->reserve(listeners->size());
        for (const auto& entry : *listeners) {
            if (entry.id != id) newVec->push_back(entry);
        }
        if (newVec->empty()) {
            _listeners.erase(it);
        } else {
            it->second = std::move(newVec);
        }
        return true;
    }

    // The road was already removed after we closed its shared state.
    return true;
}

void EventBus::clear()
{
    std::vector<std::shared_ptr<SubscriptionState>> states;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& [type, listeners] : _listeners) {
            (void)type;
            if (!listeners) continue;
            for (const auto& entry : *listeners) {
                if (entry.state) states.push_back(entry.state);
            }
        }
        _listeners.clear();
    }

    // Invalidate already-copied listener snapshots too.
    for (const auto& state : states) {
        std::lock_guard<std::recursive_mutex> gate(state->mutex);
        state->active = false;
    }
}

void EventBus::shutdown()
{
    if (!_running) return;
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        _running = false;
    }
#ifndef __EMSCRIPTEN__
    _cv.notify_all();
    if (_worker.joinable()) _worker.join();
#endif
}

// The sum of all Queues should be conceptualized as a Formation instance.
// Formation of relations should have Queues.
// Before publishing an event, add the event to the relation,
// Relations should be the channel through which queues are connected.
void EventBus::processQueue()
{
#ifndef __EMSCRIPTEN__
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
#endif
}

void EventBus::tick() {
    // Process all currently queued jobs
    std::queue<Job> currentJobs;
    {
        std::lock_guard<std::mutex> lock(_queueMutex);
        std::swap(currentJobs, _queue);
    }
    while (!currentJobs.empty()) {
        Job job = std::move(currentJobs.front());
        currentJobs.pop();
        if (job) job();
    }
}

// Helper method to add event to formation relations
// Note: This method creates a relation that can be added to any formation's relation manager
void EventBus::addEventToFormationRelations(const std::string& eventType, const std::string& sourceId, const std::string& targetId, Formation* formation)
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