#include "PerspectiveManager.hpp"
#include <algorithm>
#include <stdexcept>

PerspectiveManager::PerspectiveManager() {
    // Constructor - initialize with default state
}

PerspectiveManager::~PerspectiveManager() {
    // Destructor - cleanup is handled by unique_ptr
}

void PerspectiveManager::addPerspective(std::unique_ptr<UserPerspective> perspective) {
    if (!perspective) {
        return;
    }
    
    // Check if perspective with same name already exists
    if (hasPerspective(perspective->getName())) {
        return; // Could throw or log warning
    }
    
    _perspectives.push_back(std::move(perspective));
    
    // If this is the first perspective, make it active
    if (_perspectives.size() == 1) {
        _currentIndex = 0;
        if (_isActive) {
            _perspectives[0]->activate();
        }
    }
}

void PerspectiveManager::switchTo(size_t index) {
    if (index >= _perspectives.size()) {
        return; // Could throw or log warning
    }
    
    if (_currentIndex != index) {
        // Deactivate current perspective
        if (_currentIndex < _perspectives.size() && _perspectives[_currentIndex]) {
            _perspectives[_currentIndex]->deactivate();
        }
        
        _currentIndex = index;
        
        // Activate new perspective
        if (_isActive && _perspectives[_currentIndex]) {
            _perspectives[_currentIndex]->activate();
        }
    }
}

void PerspectiveManager::switchTo(const std::string& name) {
    size_t index = findPerspective(name);
    if (index != static_cast<size_t>(-1)) {
        switchTo(index);
    }
}

void PerspectiveManager::removePerspective(size_t index) {
    if (index >= _perspectives.size()) {
        return;
    }
    
    // Deactivate if it's the current perspective
    if (index == _currentIndex && _perspectives[index]) {
        _perspectives[index]->deactivate();
    }
    
    _perspectives.erase(_perspectives.begin() + index);
    
    // Adjust current index if necessary
    if (_perspectives.empty()) {
        _currentIndex = 0;
    } else if (_currentIndex >= _perspectives.size()) {
        _currentIndex = _perspectives.size() - 1;
        if (_isActive && _perspectives[_currentIndex]) {
            _perspectives[_currentIndex]->activate();
        }
    }
}

void PerspectiveManager::activate() {
    if (!_isActive) {
        _isActive = true;
        if (_currentIndex < _perspectives.size() && _perspectives[_currentIndex]) {
            _perspectives[_currentIndex]->activate();
        }
    }
}

void PerspectiveManager::deactivate() {
    if (_isActive) {
        _isActive = false;
        if (_currentIndex < _perspectives.size() && _perspectives[_currentIndex]) {
            _perspectives[_currentIndex]->deactivate();
        }
    }
}

void PerspectiveManager::update(float deltaTime) {
    if (_isActive && _currentIndex < _perspectives.size() && _perspectives[_currentIndex]) {
        _perspectives[_currentIndex]->update(deltaTime);
    }
}

void PerspectiveManager::render() {
    if (_isActive && _currentIndex < _perspectives.size() && _perspectives[_currentIndex]) {
        _perspectives[_currentIndex]->render();
    }
}

UserPerspective* PerspectiveManager::current() const {
    if (_currentIndex < _perspectives.size()) {
        return _perspectives[_currentIndex].get();
    }
    return nullptr;
}

UserPerspective* PerspectiveManager::get(size_t index) const {
    if (index < _perspectives.size()) {
        return _perspectives[index].get();
    }
    return nullptr;
}

void PerspectiveManager::clear() {
    // Deactivate current perspective
    if (_currentIndex < _perspectives.size() && _perspectives[_currentIndex]) {
        _perspectives[_currentIndex]->deactivate();
    }
    
    _perspectives.clear();
    _currentIndex = 0;
}

bool PerspectiveManager::hasPerspective(const std::string& name) const {
    return findPerspective(name) != static_cast<size_t>(-1);
}

size_t PerspectiveManager::findPerspective(const std::string& name) const {
    for (size_t i = 0; i < _perspectives.size(); ++i) {
        if (_perspectives[i] && _perspectives[i]->getName() == name) {
            return i;
        }
    }
    return static_cast<size_t>(-1);
}
