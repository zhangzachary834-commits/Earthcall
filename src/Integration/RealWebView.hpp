#pragma once

#include <string>
#include <memory>
#include <functional>
#include <map>
#include "../json.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#import <WebKit/WebKit.h>
#import <Cocoa/Cocoa.h>
#endif
#endif

namespace Integration {

// Real web view implementation using native WebKit
class RealWebView {
public:
    struct Config {
        std::string url;
        int width = 800;
        int height = 600;
        bool allow_javascript = true;
        bool allow_navigation = true;
    };

    RealWebView(const Config& config);
    ~RealWebView();

    // Lifecycle
    bool init();
    void update();
    void render();
    void shutdown();

    // Navigation
    void navigate(const std::string& url);
    void goBack();
    void goForward();
    void refresh();
    
    // Window management
    void showWindow();
    void hideWindow();
    void setWindowVisible(bool visible);

    // JavaScript interaction
    void executeJavaScript(const std::string& script);
    void registerJavaScriptHandler(const std::string& name, std::function<void(const std::string&)> handler);
    
    // Web page modification
    void injectCSS(const std::string& css);
    void modifyElement(const std::string& selector, const std::string& property, const std::string& value);
    void addElement(const std::string& parentSelector, const std::string& html);
    void removeElement(const std::string& selector);
    void setElementText(const std::string& selector, const std::string& text);
    void setElementHTML(const std::string& selector, const std::string& html);
    
    // Interactive features
    void enableDeveloperMode(bool enable);
    void captureScreenshot();
    void enableLiveEditing(bool enable);

    // Window management
    void setSize(int width, int height);
    void setPosition(int x, int y);
    void show();
    void hide();
    void focus();

    // Communication
    void sendMessageToWeb(const std::string& message);
    void setMessageHandler(std::function<void(const std::string&)> handler);

    // Getters
    bool isLoaded() const { return _loaded; }
    std::string getCurrentURL() const { return _currentURL; }
    bool isVisible() const { return _visible; }

private:
    Config _config;
    bool _initialized = false;
    bool _loaded = false;
    bool _visible = true;
    bool _liveEditingEnabled = false;
    std::string _currentURL;

#ifdef __APPLE__
#if TARGET_OS_MAC
    NSWindow* _window = nullptr;
    WKWebView* _webView = nullptr;
    WKWebViewConfiguration* _webConfig = nullptr;
    WKUserContentController* _userContentController = nullptr;
#endif
#endif

    std::function<void(const std::string&)> _messageHandler;
    std::map<std::string, std::function<void(const std::string&)>> _jsHandlers;

    // Internal methods
    void _setupWebView();
    void _setupJavaScriptHandlers();
    void _handleWebMessage(const std::string& message);
    
    // Integration handlers
    void _handleBrushCreate(const nlohmann::json& data);
    void _handleBrushSetActive(const nlohmann::json& data);
    void _handleBrushPaint(const nlohmann::json& data);
    void _handleBrushGetAll();
    
    void _handleDesignCreateShape(const nlohmann::json& data);
    void _handleDesignCreateText(const nlohmann::json& data);
    void _handleDesignApplyEffect(const nlohmann::json& data);
    void _handleDesignGetAll();
    
    void _handleAvatarCreate(const nlohmann::json& data);
    void _handleAvatarAnimate(const nlohmann::json& data);
    void _handleAvatarSetPosition(const nlohmann::json& data);
    void _handleAvatarGetAll();
    
    void _handleWorldCreateZone(const nlohmann::json& data);
    void _handleWorldAddObject(const nlohmann::json& data);
    void _handleWorldSetTheme(const nlohmann::json& data);
    void _handleWorldGetAll();
    
    void _handleUINotification(const nlohmann::json& data);
    void _handleUIOpenPanel(const nlohmann::json& data);
    void _handleUISetCursor(const nlohmann::json& data);
    
    void _handleDataSave(const nlohmann::json& data);
    void _handleDataLoad(const nlohmann::json& data);
    void _handleDataGetKeys();
};

} // namespace Integration 