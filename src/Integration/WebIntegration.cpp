#include "Integration/WebIntegration.hpp"
#include "Util/SaveSystem.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

// Real web view implementation using native WebKit
#ifndef __EMSCRIPTEN__
#include "Integration/RealWebView.hpp"
#else
#include <emscripten.h>
#endif

namespace Integration {

// WebView implementation using PIMPL pattern
class WebView::WebViewImpl {
public:
    WebViewImpl(const std::string& url) {
#ifndef __EMSCRIPTEN__
        RealWebView::Config config;
        config.url = url;
        config.width = 800;
        config.height = 600;
        config.allow_javascript = true;
        config.allow_navigation = true;
        
        _realWebView = std::make_unique<RealWebView>(config);
#else
        std::cout << "🌐 Wasm WebView initialized (Host browser is the view): " << url << std::endl;
#endif
    }
    
    bool init() {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            return _realWebView->init();
        }
#endif
        return true;
    }
    
    void update() {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->update();
        }
#endif
    }
    
    void render() {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->render();
        }
#endif
    }
    
    void shutdown() {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->shutdown();
        }
#endif
    }
    
    void navigate(const std::string& url) {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->navigate(url);
        }
#else
        // Let the host JS handle navigation if needed
        EM_ASM({
            console.log("C++ requested navigation to: " + UTF8ToString($0));
            // window.location.href = UTF8ToString($0);
        }, url.c_str());
#endif
    }
    
    void executeJavaScript(const std::string& script) {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->executeJavaScript(script);
        }
#else
        emscripten_run_script(script.c_str());
#endif
    }
    
    void showWindow() {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->showWindow();
        }
#endif
    }
    
    void hideWindow() {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->hideWindow();
        }
#endif
    }
    
    void setWindowVisible(bool visible) {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->setWindowVisible(visible);
        }
#endif
    }
    
    // Web interaction methods
    void injectCSS(const std::string& css) {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->injectCSS(css);
        }
#else
        EM_ASM({
            var style = document.createElement('style');
            style.innerHTML = UTF8ToString($0);
            document.head.appendChild(style);
        }, css.c_str());
#endif
    }
    
    void modifyElement(const std::string& selector, const std::string& property, const std::string& value) {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->modifyElement(selector, property, value);
        }
#endif
    }
    
    void addElement(const std::string& parentSelector, const std::string& html) {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->addElement(parentSelector, html);
        }
#endif
    }
    
    void removeElement(const std::string& selector) {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->removeElement(selector);
        }
#endif
    }
    
    void setElementText(const std::string& selector, const std::string& text) {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->setElementText(selector, text);
        }
#endif
    }
    
    void setElementHTML(const std::string& selector, const std::string& html) {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->setElementHTML(selector, html);
        }
#endif
    }
    
    void enableDeveloperMode(bool enable) {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->enableDeveloperMode(enable);
        }
#endif
    }
    
    void captureScreenshot() {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->captureScreenshot();
        }
#endif
    }
    
    void enableLiveEditing(bool enable) {
#ifndef __EMSCRIPTEN__
        if (_realWebView) {
            _realWebView->enableLiveEditing(enable);
        }
#endif
    }

private:
#ifndef __EMSCRIPTEN__
    std::unique_ptr<RealWebView> _realWebView;
#endif
};

// Bridge for communication between web apps and Earthcall
class WebBridge {
public:
    WebBridge(WebApp* parent) : _parent(parent) {}
    
    void sendToWebApp(const std::string& message) {
        if (_parent && _parent->isActive()) {
            // Send message to web app
            std::cout << "🌉 Bridge -> WebApp: " << message << std::endl;
#ifdef __EMSCRIPTEN__
            EM_ASM({
                if (window.earthcall_receive) {
                    window.earthcall_receive(UTF8ToString($0));
                }
            }, message.c_str());
#endif
        }
    }
    
    void receiveFromWebApp(const std::string& message) {
        // Handle incoming messages from web app
        std::cout << "🌉 WebApp -> Bridge: " << message << std::endl;
        
        // Parse message and route to appropriate handler
        _parseAndRouteMessage(message);
    }
    
    void registerEarthcallAPI() {
        // Register Earthcall features that web apps can access
        std::cout << "🌉 Earthcall API registered" << std::endl;
    }

private:
    WebApp* _parent;
    
    void _parseAndRouteMessage(const std::string& message) {
        // Simple message parsing - in real implementation, use JSON
        if (message.find("BRUSH_CREATE") != std::string::npos) {
            // Handle brush creation request
        } else if (message.find("DESIGN_APPLY") != std::string::npos) {
            // Handle design application request
        } else if (message.find("AVATAR_UPDATE") != std::string::npos) {
            // Handle avatar update request
        }
    }
};

// WebView implementation
WebView::WebView(const std::string& url) : _impl(std::make_unique<WebViewImpl>(url)) {
}

WebView::~WebView() = default;

bool WebView::init() {
    return _impl->init();
}

void WebView::update() {
    _impl->update();
}

void WebView::render() {
    _impl->render();
}

void WebView::shutdown() {
    _impl->shutdown();
}

void WebView::navigate(const std::string& url) {
    _impl->navigate(url);
}

void WebView::executeJavaScript(const std::string& script) {
    _impl->executeJavaScript(script);
}

void WebView::showWindow() {
    _impl->showWindow();
}

void WebView::hideWindow() {
    _impl->hideWindow();
}

void WebView::setWindowVisible(bool visible) {
    _impl->setWindowVisible(visible);
}

void WebView::injectCSS(const std::string& css) {
    _impl->injectCSS(css);
}

void WebView::modifyElement(const std::string& selector, const std::string& property, const std::string& value) {
    _impl->modifyElement(selector, property, value);
}

void WebView::addElement(const std::string& parentSelector, const std::string& html) {
    _impl->addElement(parentSelector, html);
}

void WebView::removeElement(const std::string& selector) {
    _impl->removeElement(selector);
}

void WebView::setElementText(const std::string& selector, const std::string& text) {
    _impl->setElementText(selector, text);
}

void WebView::setElementHTML(const std::string& selector, const std::string& html) {
    _impl->setElementHTML(selector, html);
}

void WebView::enableDeveloperMode(bool enable) {
    _impl->enableDeveloperMode(enable);
}

void WebView::captureScreenshot() {
    _impl->captureScreenshot();
}

void WebView::enableLiveEditing(bool enable) {
    _impl->enableLiveEditing(enable);
}

// WebApp::Config serialization
nlohmann::json WebApp::Config::serialize() const {
    nlohmann::json j;
    j["name"] = name;
    j["url"] = url;
    j["icon_path"] = icon_path;
    j["allow_overlay"] = allow_overlay;
    j["allow_earthcall_features"] = allow_earthcall_features;
    
    // Serialize permissions map
    j["permissions"] = nlohmann::json::object();
    for (const auto& [key, value] : permissions) {
        j["permissions"][key] = value;
    }
    
    return j;
}

void WebApp::Config::deserialize(const nlohmann::json& j) {
    name = j.value("name", "");
    url = j.value("url", "");
    icon_path = j.value("icon_path", "");
    allow_overlay = j.value("allow_overlay", true);
    allow_earthcall_features = j.value("allow_earthcall_features", true);
    
    // Deserialize permissions map
    permissions.clear();
    if (j.contains("permissions") && j["permissions"].is_object()) {
        for (const auto& [key, value] : j["permissions"].items()) {
            permissions[key] = value.get<std::string>();
        }
    }
}

// WebApp implementation
WebApp::WebApp(const Config& config) : _config(config) {
    _webView = std::make_unique<WebView>(config.url);
    _bridge = std::make_unique<WebBridge>(this);
}

WebApp::~WebApp() = default;

bool WebApp::init() {
    if (!_webView->init()) {
        return false;
    }
    
    _bridge->registerEarthcallAPI();
    _active = true;
    
    std::cout << "📱 WebApp '" << _config.name << "' initialized" << std::endl;
    return true;
}

void WebApp::update() {
    if (_active) {
        _webView->update();
    }
}

void WebApp::render() {
    if (_active) {
        _webView->render();
    }
}

void WebApp::shutdown() {
    if (_active) {
        _webView->shutdown();
        _active = false;
        std::cout << "📱 WebApp '" << _config.name << "' shutdown" << std::endl;
    }
}

void WebApp::enableEarthcallFeatures(bool enable) {
    if (enable && _config.allow_earthcall_features) {
        std::cout << "🔧 Earthcall features enabled for '" << _config.name << "'" << std::endl;
    }
}

void WebApp::setOverlayMode(bool overlay) {
    if (overlay && _config.allow_overlay) {
        _overlayMode = true;
        std::cout << "🖼️ Overlay mode enabled for '" << _config.name << "'" << std::endl;
    } else {
        _overlayMode = false;
    }
}

void WebApp::setTransparency(float alpha) {
    _transparency = std::clamp(alpha, 0.0f, 1.0f);
}

void WebApp::showWindow() {
    if (_active && _webView) {
        _webView->showWindow();
        std::cout << "🪟 Showing window for '" << _config.name << "'" << std::endl;
    }
}

void WebApp::hideWindow() {
    if (_active && _webView) {
        _webView->hideWindow();
        std::cout << "🪟 Hiding window for '" << _config.name << "'" << std::endl;
    }
}

void WebApp::setWindowVisible(bool visible) {
    if (visible) {
        showWindow();
    } else {
        hideWindow();
    }
}

void WebApp::sendMessage(const std::string& message) {
    _bridge->sendToWebApp(message);
}

void WebApp::registerMessageHandler(const std::string& type, std::function<void(const std::string&)> handler) {
    _messageHandlers[type] = handler;
}

// WebIntegrationManager implementation
WebIntegrationManager& WebIntegrationManager::instance() {
    static WebIntegrationManager s_instance;
    return s_instance;
}

bool WebIntegrationManager::registerApp(const WebApp::Config& config) {
    if (_apps.find(config.name) != _apps.end()) {
        std::cerr << "❌ WebApp '" << config.name << "' already registered" << std::endl;
        return false;
    }
    
    // Validate URL
    if (config.url.empty() || config.url.length() < 4) {
        std::cerr << "❌ Invalid URL for WebApp '" << config.name << "': " << config.url << std::endl;
        return false;
    }
    
    auto app = std::make_unique<WebApp>(config);
    if (!app->init()) {
        std::cerr << "❌ Failed to initialize WebApp '" << config.name << "'" << std::endl;
        return false;
    }
    
    _apps[config.name] = std::move(app);
    std::cout << "✅ WebApp '" << config.name << "' registered successfully" << std::endl;
    
    // Save immediately after registering
    saveWebApps();
    
    return true;
}

void WebIntegrationManager::unregisterApp(const std::string& name) {
    auto it = _apps.find(name);
    if (it != _apps.end()) {
        it->second->shutdown();
        _apps.erase(it);
        std::cout << "🗑️ WebApp '" << name << "' unregistered" << std::endl;
    }
}

WebApp* WebIntegrationManager::getApp(const std::string& name) {
    auto it = _apps.find(name);
    return (it != _apps.end()) ? it->second.get() : nullptr;
}

void WebIntegrationManager::setGlobalOverlayMode(bool enabled) {
    _globalOverlayMode = enabled;
    for (auto& [name, app] : _apps) {
        app->setOverlayMode(enabled);
    }
}

void WebIntegrationManager::setGlobalTransparency(float alpha) {
    _globalTransparency = std::clamp(alpha, 0.0f, 1.0f);
    for (auto& [name, app] : _apps) {
        app->setTransparency(alpha);
    }
}

void WebIntegrationManager::enableBrushSystemAccess(bool enable) {
    _brushSystemAccess = enable;
    std::cout << "🎨 Brush system access " << (enable ? "enabled" : "disabled") << std::endl;
}

void WebIntegrationManager::enableDesignSystemAccess(bool enable) {
    _designSystemAccess = enable;
    std::cout << "🎨 Design system access " << (enable ? "enabled" : "disabled") << std::endl;
}

void WebIntegrationManager::enableAvatarSystemAccess(bool enable) {
    _avatarSystemAccess = enable;
    std::cout << "👤 Avatar system access " << (enable ? "enabled" : "disabled") << std::endl;
}

void WebIntegrationManager::broadcastToAllApps(const std::string& message) {
    for (auto& [name, app] : _apps) {
        app->sendMessage(message);
    }
}

void WebIntegrationManager::sendToApp(const std::string& appName, const std::string& message) {
    if (auto* app = getApp(appName)) {
        app->sendMessage(message);
    }
}

void WebIntegrationManager::update() {
    for (auto& [name, app] : _apps) {
        app->update();
    }
}

void WebIntegrationManager::render() {
    for (auto& [name, app] : _apps) {
        app->render();
    }
}

void WebIntegrationManager::shutdown() {
    // Save web apps before shutting down
    saveWebApps();
    
    for (auto& [name, app] : _apps) {
        app->shutdown();
    }
    _apps.clear();
}

void WebIntegrationManager::saveWebApps() {
    try {
        std::cout << "💾 Attempting to save web apps..." << std::endl;
        std::cout << "💾 Number of apps to save: " << _apps.size() << std::endl;
        
        if (_apps.empty()) {
            std::cout << "💾 No web apps to save" << std::endl;
            return;
        }
        
        nlohmann::json j = serializeWebApps();
        std::cout << "💾 Serialized JSON: " << j.dump(2) << std::endl;
        
        std::string filename = SaveSystem::writeSaveData(j, "web_apps", SaveSystem::SaveType::INTEGRATION);
        std::cout << "💾 Web apps saved to: " << filename << std::endl;
        
        // Verify the file was created
        std::ifstream checkFile(filename);
        if (checkFile.good()) {
            std::cout << "✅ Save file verified successfully" << std::endl;
        } else {
            std::cout << "❌ Save file verification failed" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to save web apps: " << e.what() << std::endl;
    }
}

void WebIntegrationManager::loadWebApps() {
    try {
        // First, try to find any existing integration save files
        auto files = SaveSystem::listFiles(SaveSystem::SaveType::INTEGRATION);
        std::cout << "📂 Found " << files.size() << " integration save files" << std::endl;
        
        if (files.empty()) {
            std::cout << "📂 No saved web apps found, starting fresh" << std::endl;
            return;
        }
        
        // Look for web_apps.json specifically, or use the most recent file
        std::string filename;
        for (const auto& file : files) {
            if (file.find("web_apps.json") != std::string::npos) {
                filename = file;
                break;
            }
        }
        
        if (filename.empty() && !files.empty()) {
            filename = files.back(); // Use most recent if web_apps.json not found
        }
        
        std::cout << "📂 Attempting to load web apps from: " << filename << std::endl;
        
        std::ifstream file(filename);
        if (file.is_open()) {
            std::cout << "📂 File opened successfully" << std::endl;
            nlohmann::json j;
            file >> j;
            std::cout << "📂 Loaded JSON: " << j.dump(2) << std::endl;
            deserializeWebApps(j);
            std::cout << "📂 Web apps loaded from: " << filename << std::endl;
        } else {
            std::cout << "📂 Failed to open file: " << filename << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to load web apps: " << e.what() << std::endl;
    }
}

nlohmann::json WebIntegrationManager::serializeWebApps() const {
    nlohmann::json j;
    j["version"] = "1.0";
    j["timestamp"] = std::time(nullptr);
    j["web_apps"] = nlohmann::json::array();
    
    for (const auto& [name, app] : _apps) {
        nlohmann::json appJson;
        appJson["name"] = name;
        appJson["config"] = app->getConfig().serialize();
        appJson["active"] = app->isActive();
        appJson["overlay_mode"] = app->isOverlayMode();
        j["web_apps"].push_back(appJson);
    }
    
    return j;
}

void WebIntegrationManager::deserializeWebApps(const nlohmann::json& j) {
    if (!j.contains("web_apps") || !j["web_apps"].is_array()) {
        std::cout << "📂 No web apps found in save file" << std::endl;
        return;
    }
    
    for (const auto& appJson : j["web_apps"]) {
        try {
            WebApp::Config config;
            config.deserialize(appJson["config"]);
            
            // Validate URL before creating web app
            if (config.url.empty() || config.url.length() < 4) {
                std::cout << "⚠️ Skipping web app with invalid URL: " << config.url << std::endl;
                continue;
            }
            
            auto app = std::make_unique<WebApp>(config);
            if (app->init()) {
                std::string name = appJson["name"];
                _apps[name] = std::move(app);
                std::cout << "📂 Loaded web app: " << name << std::endl;
            } else {
                std::cout << "⚠️ Failed to initialize web app: " << config.name << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "❌ Failed to load web app: " << e.what() << std::endl;
        }
    }
}

} // namespace Integration 

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>

// Global C-API wrapper for JS to easily call without C++ object instantiation
extern "C" {
    void emscripten_receive_from_js(const char* appName, const char* message) {
        Integration::WebIntegrationManager::instance().sendToApp(appName, message);
    }
}

EMSCRIPTEN_BINDINGS(earthcall_integration) {
    emscripten::function("receive_from_js", &emscripten_receive_from_js);
}
#endif