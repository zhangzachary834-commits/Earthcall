#pragma once
#include <vector>
#include <memory>
#include <string>
#include "UserPerspective.hpp"

class PerspectiveManager {
    std::vector<std::unique_ptr<UserPerspective>> _perspectives;
    size_t _currentIndex = 0;
    bool _isActive = false;

public:
    PerspectiveManager();
    ~PerspectiveManager();

    // Core management functions
    void addPerspective(std::unique_ptr<UserPerspective> perspective);
    void switchTo(size_t index);
    void switchTo(const std::string& name);
    void removePerspective(size_t index);
    
    // State management
    void activate();
    void deactivate();
    bool isActive() const { return _isActive; }
    
    // Update and render
    void update(float deltaTime);
    void render();
    
    // Accessors
    UserPerspective* current() const;
    UserPerspective* get(size_t index) const;
    size_t currentIndex() const { return _currentIndex; }
    size_t count() const { return _perspectives.size(); }
    
    // Utility
    void clear();
    bool hasPerspective(const std::string& name) const;
    size_t findPerspective(const std::string& name) const;
};
