#pragma once

#include <string>
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include "../json.hpp"

namespace Integration {

// Forward declarations
class WebBridge;

// WebView class definition
class WebView {
public:
    WebView(const std::string& url);
    ~WebView();

    // Lifecycle
    bool init();
    void update();
    void render();
    void shutdown();

    // Navigation
    void navigate(const std::string& url);
    void executeJavaScript(const std::string& script);
    
    // Window management
    void showWindow();
    void hideWindow();
    void setWindowVisible(bool visible);
    
    // Web interaction methods
    void injectCSS(const std::string& css);
    void modifyElement(const std::string& selector, const std::string& property, const std::string& value);
    void addElement(const std::string& parentSelector, const std::string& html);
    void removeElement(const std::string& selector);
    void setElementText(const std::string& selector, const std::string& text);
    void setElementHTML(const std::string& selector, const std::string& html);
    void enableDeveloperMode(bool enable);
    void captureScreenshot();
    void enableLiveEditing(bool enable);

private:
    class WebViewImpl;
    std::unique_ptr<WebViewImpl> _impl;
};

// Represents a web application that can be embedded in Earthcall
class WebApp {
public:
    struct Config {
        std::string name;
        std::string url;
        std::string icon_path;
        bool allow_overlay = true;
        bool allow_earthcall_features = true;
        std::map<std::string, std::string> permissions;
        
        // Serialization
        nlohmann::json serialize() const;
        void deserialize(const nlohmann::json& j);
    };

    WebApp(const Config& config);
    ~WebApp();

    // Lifecycle
    bool init();
    void update();
    void render();
    void shutdown();

    // Interaction with Earthcall
    void enableEarthcallFeatures(bool enable);
    void setOverlayMode(bool overlay);
    void setTransparency(float alpha);
    
    // Window management
    void showWindow();
    void hideWindow();
    void setWindowVisible(bool visible);

    // Communication
    void sendMessage(const std::string& message);
    void registerMessageHandler(const std::string& type, std::function<void(const std::string&)> handler);

    // Getters
    const Config& getConfig() const { return _config; }
    bool isActive() const { return _active; }
    bool isOverlayMode() const { return _overlayMode; }
    WebView* getWebView() { return _webView.get(); }

private:
    Config _config;
    std::unique_ptr<WebView> _webView;
    std::unique_ptr<WebBridge> _bridge;
    bool _active = false;
    bool _overlayMode = false;
    float _transparency = 1.0f;
    std::map<std::string, std::function<void(const std::string&)>> _messageHandlers;
};

// Manages all web integrations
class WebIntegrationManager {
public:
    static WebIntegrationManager& instance();

    // App management
    bool registerApp(const WebApp::Config& config);
    void unregisterApp(const std::string& name);
    WebApp* getApp(const std::string& name);
    const std::map<std::string, std::unique_ptr<WebApp>>& getAllApps() const { return _apps; }

    // Global settings
    void setGlobalOverlayMode(bool enabled);
    void setGlobalTransparency(float alpha);

    // Earthcall feature access
    void enableBrushSystemAccess(bool enable);
    void enableDesignSystemAccess(bool enable);
    void enableAvatarSystemAccess(bool enable);

    // Communication
    void broadcastToAllApps(const std::string& message);
    void sendToApp(const std::string& appName, const std::string& message);

    // Lifecycle
    void update();
    void render();
    void shutdown();
    
    // Save/Load
    void saveWebApps();
    void loadWebApps();
    nlohmann::json serializeWebApps() const;
    void deserializeWebApps(const nlohmann::json& j);

private:
    WebIntegrationManager() = default;
    ~WebIntegrationManager() = default;

    std::map<std::string, std::unique_ptr<WebApp>> _apps;
    bool _globalOverlayMode = false;
    float _globalTransparency = 1.0f;
    bool _brushSystemAccess = false;
    bool _designSystemAccess = false;
    bool _avatarSystemAccess = false;
};

} // namespace Integration 