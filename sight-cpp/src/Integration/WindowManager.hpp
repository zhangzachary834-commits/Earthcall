#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace Integration {

// Represents an external application window that can overlay on Earthcall
class ExternalWindow {
public:
    struct Config {
        std::string name;
        std::string process_name;
        std::string window_title;
        bool allow_overlay = true;
        bool allow_transparency = true;
        float default_alpha = 0.8f;
        bool capture_input = false;
        std::vector<std::string> permissions;
    };

    ExternalWindow(const Config& config);
    ~ExternalWindow();

    // Lifecycle
    bool init();
    void update();
    void render();
    void shutdown();

    // Window management
    bool findAndAttach();
    void setOverlayMode(bool overlay);
    void setTransparency(float alpha);
    void setPosition(int x, int y);
    void setSize(int width, int height);
    void bringToFront();
    void minimize();
    void maximize();

    // Input handling
    void captureInput(bool capture);
    void forwardMouseEvent(int x, int y, int button, bool pressed);
    void forwardKeyEvent(int key, bool pressed);

    // Getters
    const Config& getConfig() const { return _config; }
    bool isAttached() const { return _attached; }
    bool isOverlayMode() const { return _overlayMode; }
    float getTransparency() const { return _transparency; }

private:
    Config _config;
    bool _attached = false;
    bool _overlayMode = false;
    float _transparency = 1.0f;
    bool _inputCaptured = false;
    
#ifdef _WIN32
    HWND _hwnd = nullptr;
    WNDPROC _originalWndProc = nullptr;
    bool _inputCaptured = false;
#elif defined(__APPLE__)
    void* _windowRef = nullptr;
#endif

    // Platform-specific methods
    bool _findWindow();
    bool _attachToWindow();
    void _detachFromWindow();
    void _setWindowStyle();
    void _restoreWindowStyle();
};

// Manages all external windows and their overlays
class WindowManager {
public:
    static WindowManager& instance();

    // Window management
    bool registerWindow(const ExternalWindow::Config& config);
    void unregisterWindow(const std::string& name);
    ExternalWindow* getWindow(const std::string& name);

    // Global overlay settings
    void enableGlobalOverlay(bool enabled);
    void setGlobalTransparency(float alpha);
    void setOverlayZOrder(const std::vector<std::string>& order);

    // Input management
    void setInputCaptureMode(bool capture);
    void forwardInputToOverlays(bool forward);

    // Composition
    void enableCompositor(bool enabled);
    void setCompositorMode(const std::string& mode); // "blend", "overlay", "side-by-side"

    // Lifecycle
    void update();
    void render();
    void shutdown();

    // Utility
    std::vector<std::string> getAttachedWindows() const;
    bool isAnyWindowOverlayed() const;

private:
    WindowManager() = default;
    ~WindowManager() = default;

    std::map<std::string, std::unique_ptr<ExternalWindow>> _windows;
    bool _globalOverlayEnabled = false;
    float _globalTransparency = 1.0f;
    bool _inputCaptureMode = false;
    bool _inputForwarding = false;
    bool _compositorEnabled = false;
    std::string _compositorMode = "blend";
    std::vector<std::string> _overlayZOrder;
};

} // namespace Integration 