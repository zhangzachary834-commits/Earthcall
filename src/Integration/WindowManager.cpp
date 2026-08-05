#include "Integration/WindowManager.hpp"
#include <iostream>

namespace Integration {

ExternalWindow::ExternalWindow(const Config& config) : _config(config) {
    std::cout << "🪟 ExternalWindow created: " << config.name << std::endl;
}

ExternalWindow::~ExternalWindow() {
    shutdown();
}

bool ExternalWindow::init() {
    std::cout << "🪟 Initializing external window: " << _config.name << std::endl;
    return true;
}

void ExternalWindow::update() {
    if (_attached) {
        // Update window state
    }
}

void ExternalWindow::render() {
    if (_attached && _overlayMode) {
        // Render overlay
    }
}

void ExternalWindow::shutdown() {
    if (_attached) {
        _detachFromWindow();
    }
    std::cout << "🪟 ExternalWindow shutdown: " << _config.name << std::endl;
}

bool ExternalWindow::findAndAttach() {
    if (_findWindow()) {
        return _attachToWindow();
    }
    return false;
}

void ExternalWindow::setOverlayMode(bool overlay) {
    if (overlay && _config.allow_overlay) {
        _overlayMode = true;
        _setWindowStyle();
        std::cout << "🪟 Overlay mode enabled for: " << _config.name << std::endl;
    } else {
        _overlayMode = false;
        _restoreWindowStyle();
    }
}

void ExternalWindow::setTransparency(float alpha) {
    _transparency = std::clamp(alpha, 0.0f, 1.0f);
    std::cout << "🪟 Transparency set to " << _transparency << " for: " << _config.name << std::endl;
}

void ExternalWindow::setPosition(int x, int y) {
    std::cout << "🪟 Setting position to (" << x << ", " << y << ") for: " << _config.name << std::endl;
    // TODO: Actually set window position
}

void ExternalWindow::setSize(int width, int height) {
    std::cout << "🪟 Setting size to " << width << "x" << height << " for: " << _config.name << std::endl;
    // TODO: Actually set window size
}

void ExternalWindow::bringToFront() {
    std::cout << "🪟 Bringing to front: " << _config.name << std::endl;
    // TODO: Actually bring window to front
}

void ExternalWindow::minimize() {
    std::cout << "🪟 Minimizing: " << _config.name << std::endl;
    // TODO: Actually minimize window
}

void ExternalWindow::maximize() {
    std::cout << "🪟 Maximizing: " << _config.name << std::endl;
    // TODO: Actually maximize window
}

void ExternalWindow::captureInput(bool capture) {
    _inputCaptured = capture;
    std::cout << "🪟 Input capture " << (capture ? "enabled" : "disabled") << " for: " << _config.name << std::endl;
}

void ExternalWindow::forwardMouseEvent(int x, int y, int button, bool pressed) {
    (void)x; (void)y; (void)button; (void)pressed; // Suppress unused parameter warnings
    if (_inputCaptured) {
        std::cout << "🪟 Forwarding mouse event to: " << _config.name << std::endl;
        // TODO: Actually forward mouse event
    }
}

void ExternalWindow::forwardKeyEvent(int key, bool pressed) {
    (void)key; (void)pressed; // Suppress unused parameter warnings
    if (_inputCaptured) {
        std::cout << "🪟 Forwarding key event to: " << _config.name << std::endl;
        // TODO: Actually forward key event
    }
}

bool ExternalWindow::_findWindow() {
    std::cout << "🪟 Searching for window: " << _config.window_title << std::endl;
    // TODO: Actually find the window
    return true; // Simulate success
}

bool ExternalWindow::_attachToWindow() {
    std::cout << "🪟 Attaching to window: " << _config.name << std::endl;
    _attached = true;
    return true;
}

void ExternalWindow::_detachFromWindow() {
    std::cout << "🪟 Detaching from window: " << _config.name << std::endl;
    _attached = false;
}

void ExternalWindow::_setWindowStyle() {
    std::cout << "🪟 Setting overlay window style for: " << _config.name << std::endl;
    // TODO: Set window to overlay style
}

void ExternalWindow::_restoreWindowStyle() {
    std::cout << "🪟 Restoring normal window style for: " << _config.name << std::endl;
    // TODO: Restore normal window style
}

// WindowManager implementation
WindowManager& WindowManager::instance() {
    static WindowManager s_instance;
    return s_instance;
}

bool WindowManager::registerWindow(const ExternalWindow::Config& config) {
    if (_windows.find(config.name) != _windows.end()) {
        std::cerr << "❌ External window '" << config.name << "' already registered" << std::endl;
        return false;
    }
    
    auto window = std::make_unique<ExternalWindow>(config);
    if (!window->init()) {
        std::cerr << "❌ Failed to initialize external window '" << config.name << "'" << std::endl;
        return false;
    }
    
    _windows[config.name] = std::move(window);
    std::cout << "✅ External window '" << config.name << "' registered successfully" << std::endl;
    return true;
}

void WindowManager::unregisterWindow(const std::string& name) {
    auto it = _windows.find(name);
    if (it != _windows.end()) {
        it->second->shutdown();
        _windows.erase(it);
        std::cout << "🗑️ External window '" << name << "' unregistered" << std::endl;
    }
}

ExternalWindow* WindowManager::getWindow(const std::string& name) {
    auto it = _windows.find(name);
    return (it != _windows.end()) ? it->second.get() : nullptr;
}

void WindowManager::enableGlobalOverlay(bool enabled) {
    _globalOverlayEnabled = enabled;
    for (auto& [name, window] : _windows) {
        window->setOverlayMode(enabled);
    }
}

void WindowManager::setGlobalTransparency(float alpha) {
    _globalTransparency = std::clamp(alpha, 0.0f, 1.0f);
    for (auto& [name, window] : _windows) {
        window->setTransparency(alpha);
    }
}

void WindowManager::setOverlayZOrder(const std::vector<std::string>& order) {
    _overlayZOrder = order;
    std::cout << "🪟 Overlay Z-order updated" << std::endl;
}

void WindowManager::setInputCaptureMode(bool capture) {
    _inputCaptureMode = capture;
    for (auto& [name, window] : _windows) {
        window->captureInput(capture);
    }
}

void WindowManager::forwardInputToOverlays(bool forward) {
    _inputForwarding = forward;
    std::cout << "🪟 Input forwarding " << (forward ? "enabled" : "disabled") << std::endl;
}

void WindowManager::enableCompositor(bool enabled) {
    _compositorEnabled = enabled;
    std::cout << "🪟 Compositor " << (enabled ? "enabled" : "disabled") << std::endl;
}

void WindowManager::setCompositorMode(const std::string& mode) {
    _compositorMode = mode;
    std::cout << "🪟 Compositor mode set to: " << mode << std::endl;
}

void WindowManager::update() {
    for (auto& [name, window] : _windows) {
        window->update();
    }
}

void WindowManager::render() {
    for (auto& [name, window] : _windows) {
        window->render();
    }
}

void WindowManager::shutdown() {
    for (auto& [name, window] : _windows) {
        window->shutdown();
    }
    _windows.clear();
}

std::vector<std::string> WindowManager::getAttachedWindows() const {
    std::vector<std::string> attached;
    for (const auto& [name, window] : _windows) {
        if (window->isAttached()) {
            attached.push_back(name);
        }
    }
    return attached;
}

bool WindowManager::isAnyWindowOverlayed() const {
    for (const auto& [name, window] : _windows) {
        if (window->isOverlayMode()) {
            return true;
        }
    }
    return false;
}

} // namespace Integration 