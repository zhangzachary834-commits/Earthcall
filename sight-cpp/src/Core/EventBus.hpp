#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>
#include <memory>
#include <chrono>
#include <algorithm>
#include <string>

// Forward declarations
class Formations;

namespace Core {

// Very lightweight, header-only publish/subscribe Event Bus.
//  - Any struct/class can be used as an event type.
//  - Listeners register with subscribe<T>(handler).
//  - Publishers emit with publish(const T&).
//  - Delivery is synchronous and happens on the thread that calls publish().
//  - Internally stores type-erased function pointers keyed by std::type_index.
//
// Example:
//     struct PlayerJumped { int playerId; };
//     Core::EventBus::instance().subscribe<PlayerJumped>([](const PlayerJumped& e){
//         std::cout << "Player " << e.playerId << " jumped!\n";
//     });
//     Core::EventBus::instance().publish(PlayerJumped{42});
//
class EventBus {
public:
    // ------------------------------------------------------------------
    // Public types
    // ------------------------------------------------------------------
    using Listener = std::function<void(const void*)>;
    struct ListenerEntry { int priority; Listener listener; };
    
    // Lightweight metadata automatically attached to each event. Can be
    // extended later without breaking the templated interface.
    struct Metadata {
        std::chrono::steady_clock::time_point timestamp{};
        const void* source; // optional emitter ptr

        Metadata()
            : timestamp(std::chrono::steady_clock::now()), source(nullptr) {}

        Metadata(const void* src)
            : timestamp(std::chrono::steady_clock::now()), source(src) {}
    };

    // ------------------------------------------------------------------
    // Accessor (singleton)
    // ------------------------------------------------------------------
    static EventBus& instance();

    // ------------------------------------------------------------------
    // Subscription ------------------------------------------------------
    // ------------------------------------------------------------------
    template<typename Event>
    void subscribe(const std::function<void(const Event&)>& handler, int priority = 0)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto& vec = _listeners[typeid(Event)];
        vec.emplace_back(ListenerEntry{priority, [handler](const void* ePtr){
            handler(*static_cast<const Event*>(ePtr));
        }});
        // Keep highest priority first for deterministic ordering.
        std::sort(vec.begin(), vec.end(), [](const ListenerEntry& a, const ListenerEntry& b){
            return a.priority > b.priority;
        });
    }

    // Non-template version for internal use
    void subscribe(const std::type_index& type, const Listener& listener, int priority = 0);

    // ------------------------------------------------------------------
    // Publication (synchronous) -----------------------------------------
    // ------------------------------------------------------------------
    template<typename Event>
    void publish(const Event& event, const Metadata& meta = {})
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _listeners.find(typeid(Event));
        if (it == _listeners.end()) return;
        for (auto& entry : it->second) {
            entry.listener(&event);
        }
    }

    // ------------------------------------------------------------------
    // Publication (asynchronous) ---------------------------------------
    // ------------------------------------------------------------------
    template<typename Event>
    void publishAsync(const Event& event, const Metadata& meta = {})
    {
        // Copy listeners snapshot under lock, then enqueue work item.
        std::vector<ListenerEntry> listenersCopy;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _listeners.find(typeid(Event));
            if (it != _listeners.end()) listenersCopy = it->second;
        }
        if (listenersCopy.empty()) return;

        auto ePtr = std::make_shared<Event>(event); // shared to outlive lambda
        auto job  = [listenersCopy, ePtr]() {
            for (auto& entry : listenersCopy) {
                entry.listener(ePtr.get());
            }
        };

        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            _queue.emplace(std::move(job));
        }
        _cv.notify_one();
    }

    // ------------------------------------------------------------------
    // Formation and Relation Integration ---------------------------------
    // ------------------------------------------------------------------
    // Helper method to add event to formation relations
    void addEventToFormationRelations(const std::string& eventType, const std::string& sourceId, const std::string& targetId, Formations* formation);
    
    // Helper method to determine event scope (local vs global)
    enum class EventScope { Local, Global };
    EventScope determineEventScope(const std::string& eventType, const std::string& sourceId);

    // ------------------------------------------------------------------
    // Lifecycle ---------------------------------------------------------
    // ------------------------------------------------------------------
    void shutdown(); // gracefully stop worker thread

private:
    // Listener registry keyed by event type ---------------------------------
    std::unordered_map<std::type_index, std::vector<ListenerEntry>> _listeners;
    std::mutex   _mutex;

    // Async queue -----------------------------------------------------------
    using Job = std::function<void()>;
    std::queue<Job>                _queue;
    std::mutex                     _queueMutex;
    std::condition_variable        _cv;
    std::thread                    _worker;
    bool                           _running = false;

    // Event history for formation relations ---------------------------------
    struct EventRecord {
        std::string eventType;
        std::string sourceId;
        std::string targetId;
        std::time_t timestamp;
    };
    std::vector<EventRecord>       _eventHistory;
    std::mutex                     _eventHistoryMutex;

    // Internal helpers ------------------------------------------------------
    void processQueue();

    // Singleton plumbing ----------------------------------------------------
    EventBus();
    ~EventBus();
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
};

} // namespace Core 