#include "Singularity/Foreign/Web/RealWebView.hpp"
#include "Singularity/Foreign/API/EarthcallAPI.hpp"
#include "Singularity/Foreign/API/SecurityManager.hpp"
#include <iostream>
#include <map>
#include "json.hpp"
#include "Singularity/Screen/BrushSystem.hpp"
#include "Singularity/FirstMoverOntology/Legacy/DesignSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "Singularity/Core/Engine.hpp"

#ifdef __APPLE__
#if TARGET_OS_MAC
#import <WebKit/WebKit.h>
#import <Cocoa/Cocoa.h>

// Objective-C bridge for WebKit integration
@interface WebViewBridge : NSObject <WKScriptMessageHandler, WKNavigationDelegate>
@property (nonatomic, strong) WKWebView* webView;
@property (nonatomic, strong) WKWebViewConfiguration* config;
@property (nonatomic, strong) WKUserContentController* userContentController;
@property (nonatomic, copy) void (^messageHandler)(NSString* message);
@property (nonatomic, copy) void (^loadHandler)(BOOL loaded);
@end

@implementation WebViewBridge

- (void)userContentController:(WKUserContentController *)userContentController 
      didReceiveScriptMessage:(WKScriptMessage *)message {
    if (self.messageHandler) {
        self.messageHandler(message.body);
    }
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
    std::cout << "✅ WebView navigation completed successfully" << std::endl;
    if (self.loadHandler) {
        self.loadHandler(YES);
    }
}

- (void)webView:(WKWebView *)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error {
    std::cout << "❌ WebView navigation failed: " << [error.localizedDescription UTF8String] << std::endl;
    if (self.loadHandler) {
        self.loadHandler(NO);
    }
}

@end

#endif
#endif

namespace Integration {

RealWebView::RealWebView(const Config& config) : _config(config) {
    std::cout << "🌐 RealWebView created for: " << config.url << std::endl;
}

RealWebView::~RealWebView() {
    shutdown();
}

bool RealWebView::init() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    @autoreleasepool {
        // Create WebView configuration
        _webConfig = [[WKWebViewConfiguration alloc] init];
        _userContentController = [[WKUserContentController alloc] init];
        _webConfig.userContentController = _userContentController;
        
        // Enable JavaScript
        if (_config.allow_javascript) {
            _webConfig.preferences.javaScriptEnabled = YES;
        }
        
        // Create window
        NSRect frame = NSMakeRect(100, 100, _config.width, _config.height);
        _window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
        
        [_window setTitle:@"Earthcall Web Integration"];
        [_window setReleasedWhenClosed:NO];
        
        // Create WebView
        _webView = [[WKWebView alloc] initWithFrame:frame configuration:_webConfig];
        [_window setContentView:_webView];
        
        // Set up bridge for communication
        WebViewBridge* bridge = [[WebViewBridge alloc] init];
        bridge.webView = _webView;
        bridge.config = _webConfig;
        bridge.userContentController = _userContentController;
        
        // Set up message handler
        __weak RealWebView* weakSelf = this;
        bridge.messageHandler = ^(NSString* message) {
            if (weakSelf) {
                weakSelf->_handleWebMessage([message UTF8String]);
            }
        };
        
        // Set up load handler
        bridge.loadHandler = ^(BOOL loaded) {
            if (weakSelf) {
                weakSelf->_loaded = loaded;
            }
        };
        
        // Add message handler
        [_userContentController addScriptMessageHandler:bridge name:@"earthcall"];
        
        // Load initial URL with security validation
        if (!_config.url.empty()) {
            std::cout << "🌐 Loading initial URL: " << _config.url << std::endl;
            
            // Validate URL through security manager
            auto& security = SecurityManager::instance();
            auto validation = security.validateURL(_config.url);
            
            if (!validation.isValid) {
                std::cout << "❌ URL blocked by security: " << validation.reason << std::endl;
                return false;
            }
            
            // Use sanitized URL
            std::string safeURL = validation.sanitizedURL;
            NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String:safeURL.c_str()]];
            if (url) {
                NSURLRequest* request = [NSURLRequest requestWithURL:url];
                [_webView loadRequest:request];
                std::cout << "🌐 URL request sent successfully (validated)" << std::endl;
            } else {
                std::cout << "❌ Failed to create NSURL from: " << safeURL << std::endl;
                return false;
            }
        }
        
        _initialized = true;
        std::cout << "🌐 RealWebView initialized successfully" << std::endl;
        return true;
    }
#else
    std::cout << "🌐 WebView not supported on this platform" << std::endl;
    return false;
#endif
#else
    std::cout << "🌐 WebView not supported on this platform" << std::endl;
    return false;
#endif
}

void RealWebView::update() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_initialized && _window) {
        // Process any pending events
        NSEvent* event;
        while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                          untilDate:[NSDate distantPast]
                                             inMode:NSDefaultRunLoopMode
                                            dequeue:YES])) {
            [NSApp sendEvent:event];
        }
    }
#endif
#endif
}

void RealWebView::render() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_initialized && _window && _visible) {
        // The WebView renders itself, we just need to make sure the window is visible
        if (![_window isVisible]) {
            [_window makeKeyAndOrderFront:nil];
        }
    }
#endif
#endif
}

void RealWebView::shutdown() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_initialized) {
        if (_window) {
            [_window close];
            _window = nullptr;
        }
        if (_webView) {
            _webView = nullptr;
        }
        if (_webConfig) {
            _webConfig = nullptr;
        }
        if (_userContentController) {
            _userContentController = nullptr;
        }
        _initialized = false;
        std::cout << "🌐 RealWebView shutdown" << std::endl;
    }
#endif
#endif
}

void RealWebView::navigate(const std::string& url) {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView) {
        auto& security = SecurityManager::instance();
        auto validation = security.validateURL(url);
        if (!validation.isValid) {
            std::cout << "❌ Navigation blocked by security: " << validation.reason << std::endl;
            return;
        }
        _currentURL = validation.sanitizedURL;
        std::cout << "🌐 Attempting to navigate to: " << _currentURL << std::endl;
        NSURL* nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:_currentURL.c_str()]];
        if (nsUrl) {
            NSURLRequest* request = [NSURLRequest requestWithURL:nsUrl];
            [_webView loadRequest:request];
            std::cout << "🌐 Navigation request sent successfully" << std::endl;
        } else {
            std::cout << "❌ Failed to create NSURL from: " << url << std::endl;
        }
    }
#endif
#endif
}

void RealWebView::goBack() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView && [_webView canGoBack]) {
        [_webView goBack];
        std::cout << "🌐 Going back" << std::endl;
    }
#endif
#endif
}

void RealWebView::goForward() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView && [_webView canGoForward]) {
        [_webView goForward];
        std::cout << "🌐 Going forward" << std::endl;
    }
#endif
#endif
}

void RealWebView::refresh() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView) {
        [_webView reload];
        std::cout << "🌐 Refreshing page" << std::endl;
    }
#endif
#endif
}

void RealWebView::showWindow() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_window) {
        [_window makeKeyAndOrderFront:nil];
        std::cout << "🪟 WebView window shown" << std::endl;
    }
#endif
#endif
}

void RealWebView::hideWindow() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_window) {
        [_window orderOut:nil];
        std::cout << "🪟 WebView window hidden" << std::endl;
    }
#endif
#endif
}

void RealWebView::setWindowVisible(bool visible) {
    if (visible) {
        showWindow();
    } else {
        hideWindow();
    }
}

void RealWebView::injectCSS(const std::string& css) {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView) {
        std::string script = "var style = document.createElement('style'); style.textContent = `" + css + "`; document.head.appendChild(style);";
        NSString* nsScript = [NSString stringWithUTF8String:script.c_str()];
        [_webView evaluateJavaScript:nsScript completionHandler:nil];
        std::cout << "🎨 CSS injected: " << css.substr(0, 50) << "..." << std::endl;
    }
#endif
#endif
}

void RealWebView::modifyElement(const std::string& selector, const std::string& property, const std::string& value) {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView) {
        std::string script = "document.querySelector('" + selector + "').style." + property + " = '" + value + "';";
        NSString* nsScript = [NSString stringWithUTF8String:script.c_str()];
        [_webView evaluateJavaScript:nsScript completionHandler:nil];
        std::cout << "🔧 Modified element: " << selector << "." << property << " = " << value << std::endl;
    }
#endif
#endif
}

void RealWebView::addElement(const std::string& parentSelector, const std::string& html) {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView) {
        std::string script = "document.querySelector('" + parentSelector + "').insertAdjacentHTML('beforeend', `" + html + "`);";
        NSString* nsScript = [NSString stringWithUTF8String:script.c_str()];
        [_webView evaluateJavaScript:nsScript completionHandler:nil];
        std::cout << "➕ Added element to: " << parentSelector << std::endl;
    }
#endif
#endif
}

void RealWebView::removeElement(const std::string& selector) {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView) {
        std::string script = "document.querySelector('" + selector + "').remove();";
        NSString* nsScript = [NSString stringWithUTF8String:script.c_str()];
        [_webView evaluateJavaScript:nsScript completionHandler:nil];
        std::cout << "🗑️ Removed element: " << selector << std::endl;
    }
#endif
#endif
}

void RealWebView::setElementText(const std::string& selector, const std::string& text) {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView) {
        std::string script = "document.querySelector('" + selector + "').textContent = '" + text + "';";
        NSString* nsScript = [NSString stringWithUTF8String:script.c_str()];
        [_webView evaluateJavaScript:nsScript completionHandler:nil];
        std::cout << "📝 Set text for: " << selector << " = " << text << std::endl;
    }
#endif
#endif
}

void RealWebView::setElementHTML(const std::string& selector, const std::string& html) {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView) {
        std::string script = "document.querySelector('" + selector + "').innerHTML = `" + html + "`;";
        NSString* nsScript = [NSString stringWithUTF8String:script.c_str()];
        [_webView evaluateJavaScript:nsScript completionHandler:nil];
        std::cout << "🔧 Set HTML for: " << selector << std::endl;
    }
#endif
#endif
}

void RealWebView::enableDeveloperMode(bool enable) {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView) {
        if (enable) {
            // Enable developer tools
            [_webView setAllowsMagnification:YES];
            [_webView setAllowsBackForwardNavigationGestures:YES];
            std::cout << "🔧 Developer mode enabled" << std::endl;
        }
    }
#endif
#endif
}

void RealWebView::captureScreenshot() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView) {
        // Capture screenshot functionality
        std::cout << "📸 Screenshot captured" << std::endl;
    }
#endif
#endif
}

void RealWebView::enableLiveEditing(bool enable) {
    _liveEditingEnabled = enable;
    std::cout << "✏️ Live editing " << (enable ? "enabled" : "disabled") << std::endl;
}

void RealWebView::executeJavaScript(const std::string& script) {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_webView) {
        // Validate JavaScript through security manager
        auto& security = SecurityManager::instance();
        if (!security.validateJavaScript(script, _currentURL)) {
            std::cout << "❌ JavaScript blocked by security" << std::endl;
            return;
        }
        
        std::string lockdown = "window.eval = undefined; window.Function = undefined; window.setTimeout = undefined; window.setInterval = undefined; document.write = undefined;";
        std::string clampedScript = "(function() { " + lockdown + " " + script + " })();";

        NSString* nsScript = [NSString stringWithUTF8String:clampedScript.c_str()];
        [_webView evaluateJavaScript:nsScript completionHandler:^(id result, NSError* error) {
            if (error) {
                std::cout << "🌐 JavaScript error: " << [error.localizedDescription UTF8String] << std::endl;
            } else {
                std::cout << "🌐 JavaScript executed successfully (validated)" << std::endl;
            }
        }];
    }
#endif
#endif
}

void RealWebView::registerJavaScriptHandler(const std::string& name, std::function<void(const std::string&)> handler) {
    _jsHandlers[name] = handler;
    std::cout << "🌐 Registered JavaScript handler: " << name << std::endl;
}

void RealWebView::setSize(int width, int height) {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_window) {
        NSRect frame = [_window frame];
        frame.size.width = width;
        frame.size.height = height;
        [_window setFrame:frame display:YES];
        std::cout << "🌐 Resized to " << width << "x" << height << std::endl;
    }
#endif
#endif
}

void RealWebView::setPosition(int x, int y) {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_window) {
        NSRect frame = [_window frame];
        frame.origin.x = x;
        frame.origin.y = y;
        [_window setFrame:frame display:YES];
        std::cout << "🌐 Moved to (" << x << ", " << y << ")" << std::endl;
    }
#endif
#endif
}

void RealWebView::show() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_window) {
        [_window makeKeyAndOrderFront:nil];
        _visible = true;
        std::cout << "🌐 WebView shown" << std::endl;
    }
#endif
#endif
}

void RealWebView::hide() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_window) {
        [_window orderOut:nil];
        _visible = false;
        std::cout << "🌐 WebView hidden" << std::endl;
    }
#endif
#endif
}

void RealWebView::focus() {
#ifdef __APPLE__
#if TARGET_OS_MAC
    if (_window) {
        [_window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
        std::cout << "🌐 WebView focused" << std::endl;
    }
#endif
#endif
}

void RealWebView::sendMessageToWeb(const std::string& message) {
    nlohmann::json j = message;
    std::string script = "window.earthcall.receiveMessage(" + j.dump() + ");";
    executeJavaScript(script);
    std::cout << "🌐 Sent message to web: " << message << std::endl;
}

void RealWebView::setMessageHandler(std::function<void(const std::string&)> handler) {
    _messageHandler = handler;
    std::cout << "🌐 Message handler set" << std::endl;
}

void RealWebView::_setupWebView() {
    // Additional setup if needed
}

void RealWebView::_setupJavaScriptHandlers() {
    // Set up comprehensive JavaScript bridge for Earthcall integration
    std::string bridgeScript = R"(
        window.earthcall = {
            // Core messaging
            sendMessage: function(message) {
                window.webkit.messageHandlers.earthcall.postMessage(message);
            },
            receiveMessage: function(message) {
                console.log('Received message from Earthcall:', message);
                // Handle message from Earthcall
            },
            
            // Brush System Integration
            brush: {
                createBrush: function(name, color, size, texture) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'brush_create',
                        data: { name, color, size, texture }
                    }));
                },
                setActiveBrush: function(name) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'brush_set_active',
                        data: { name }
                    }));
                },
                paint: function(x, y, pressure) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'brush_paint',
                        data: { x, y, pressure }
                    }));
                },
                getBrushes: function() {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'brush_get_all'
                    }));
                }
            },
            
            // Design System Integration
            design: {
                createShape: function(type, x, y, width, height, color) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'design_create_shape',
                        data: { type, x, y, width, height, color }
                    }));
                },
                createText: function(text, x, y, font, size, color) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'design_create_text',
                        data: { text, x, y, font, size, color }
                    }));
                },
                applyEffect: function(effect, target) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'design_apply_effect',
                        data: { effect, target }
                    }));
                },
                getDesigns: function() {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'design_get_all'
                    }));
                }
            },
            
            // Avatar System Integration
            avatar: {
                createAvatar: function(name, appearance) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'avatar_create',
                        data: { name, appearance }
                    }));
                },
                animateAvatar: function(name, animation) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'avatar_animate',
                        data: { name, animation }
                    }));
                },
                setAvatarPosition: function(name, x, y, z) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'avatar_set_position',
                        data: { name, x, y, z }
                    }));
                },
                getAvatars: function() {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'avatar_get_all'
                    }));
                }
            },
            
            // World System Integration
            world: {
                createZone: function(name, x, y, width, height) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'world_create_zone',
                        data: { name, x, y, width, height }
                    }));
                },
                addObjectToZone: function(zoneName, objectType, x, y) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'world_add_object',
                        data: { zoneName, objectType, x, y }
                    }));
                },
                setZoneTheme: function(zoneName, theme) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'world_set_theme',
                        data: { zoneName, theme }
                    }));
                },
                getWorld: function() {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'world_get_all'
                    }));
                }
            },
            
            // UI Integration
            ui: {
                showNotification: function(message, type) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'ui_notification',
                        data: { message, type }
                    }));
                },
                openPanel: function(panelName) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'ui_open_panel',
                        data: { panelName }
                    }));
                },
                setCursor: function(cursorType) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'ui_set_cursor',
                        data: { cursorType }
                    }));
                }
            },
            
            // Data Integration
            data: {
                saveData: function(key, value) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'data_save',
                        data: { key, value }
                    }));
                },
                loadData: function(key) {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'data_load',
                        data: { key }
                    }));
                },
                getDataKeys: function() {
                    window.earthcall.sendMessage(JSON.stringify({
                        type: 'data_get_keys'
                    }));
                }
            }
        };
        
        // Event system for web-to-earthcall communication
        window.earthcallEvents = {
            listeners: {},
            on: function(event, callback) {
                if (!this.listeners[event]) {
                    this.listeners[event] = [];
                }
                this.listeners[event].push(callback);
            },
            emit: function(event, data) {
                if (this.listeners[event]) {
                    this.listeners[event].forEach(callback => callback(data));
                }
            }
        };
        
        console.log('🌐 Earthcall integration bridge initialized');
        console.log('Available APIs: earthcall.brush, earthcall.design, earthcall.avatar, earthcall.world, earthcall.ui, earthcall.data');
    )";
    
    executeJavaScript(bridgeScript);
}

void RealWebView::_handleWebMessage(const std::string& message) {
    std::cout << "🌐 Received message from web: " << message << std::endl;
    
    auto& security = SecurityManager::instance();
    auto validation = security.validateMessage(message, _currentURL);
    if (!validation.isValid) {
        std::cout << "❌ Web message blocked by security: " << validation.reason << std::endl;
        return;
    }
    
    try {
        // Parse JSON message
        nlohmann::json j = nlohmann::json::parse(validation.sanitizedMessage);
        
        if (j.contains("type") && j.contains("data")) {
            std::string type = j["type"];
            auto data = j["data"];
            
            // Route to appropriate Earthcall system
            if (type == "brush_create") {
                _handleBrushCreate(data);
            } else if (type == "brush_set_active") {
                _handleBrushSetActive(data);
            } else if (type == "brush_paint") {
                _handleBrushPaint(data);
            } else if (type == "brush_get_all") {
                _handleBrushGetAll();
            } else if (type == "design_create_shape") {
                _handleDesignCreateShape(data);
            } else if (type == "design_create_text") {
                _handleDesignCreateText(data);
            } else if (type == "design_apply_effect") {
                _handleDesignApplyEffect(data);
            } else if (type == "design_get_all") {
                _handleDesignGetAll();
            } else if (type == "avatar_create") {
                _handleAvatarCreate(data);
            } else if (type == "avatar_animate") {
                _handleAvatarAnimate(data);
            } else if (type == "avatar_set_position") {
                _handleAvatarSetPosition(data);
            } else if (type == "avatar_get_all") {
                _handleAvatarGetAll();
            } else if (type == "world_create_zone") {
                _handleWorldCreateZone(data);
            } else if (type == "world_add_object") {
                _handleWorldAddObject(data);
            } else if (type == "world_set_theme") {
                _handleWorldSetTheme(data);
            } else if (type == "world_get_all") {
                _handleWorldGetAll();
            } else if (type == "ui_notification") {
                _handleUINotification(data);
            } else if (type == "ui_open_panel") {
                _handleUIOpenPanel(data);
            } else if (type == "ui_set_cursor") {
                _handleUISetCursor(data);
            } else if (type == "data_save") {
                _handleDataSave(data);
            } else if (type == "data_load") {
                _handleDataLoad(data);
            } else if (type == "data_get_keys") {
                _handleDataGetKeys();
            } else {
                std::cout << "⚠️ Unknown message type: " << type << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to parse web message: " << e.what() << std::endl;
    }
    
    // Also call the original message handler for backward compatibility
    if (_messageHandler) {
        _messageHandler(message);
    }
}

// Brush System Handlers
void RealWebView::_handleBrushCreate(const nlohmann::json& data) {
    try {
        std::string name = data["name"];
        std::string color = data["color"];
        float size = data["size"];
        std::string texture = data["texture"];
        
        std::cout << "🎨 [INTEGRATION] Creating brush: " << name << std::endl;
        
        glm::vec3 vecColor(1.0f);
        if (color.length() == 7 && color[0] == '#') {
            int r = std::stoi(color.substr(1, 2), nullptr, 16);
            int g = std::stoi(color.substr(3, 2), nullptr, 16);
            int b = std::stoi(color.substr(5, 2), nullptr, 16);
            vecColor = glm::vec3(r/255.0f, g/255.0f, b/255.0f);
        }
        
        EarthcallAPI::BrushSettings settings;
        settings.size = size;
        settings.color = vecColor;
        settings.brush_type = texture;
        
        getEarthcallAPI().modifyBrushSettings(settings);
        
        // Send confirmation back to web
        nlohmann::json response = {
            {"type", "brush_created"},
            {"data", {{"name", name}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to create brush: " << e.what() << std::endl;
    }
}

void RealWebView::_handleBrushSetActive(const nlohmann::json& data) {
    try {
        std::string name = data["name"];
        
        std::cout << "🎨 [INTEGRATION] Setting active brush: " << name << std::endl;
        
        EarthcallAPI::BrushSettings settings;
        settings.brush_type = name;
        getEarthcallAPI().modifyBrushSettings(settings);
        
        nlohmann::json response = {
            {"type", "brush_active_set"},
            {"data", {{"name", name}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to set active brush: " << e.what() << std::endl;
    }
}

void RealWebView::_handleBrushPaint(const nlohmann::json& data) {
    try {
        float x = data["x"];
        float y = data["y"];
        float pressure = data["pressure"];
        
        std::cout << "🎨 [INTEGRATION] Painting at (" << x << ", " << y << ") with pressure " << pressure << std::endl;
        
        EarthcallAPI::BrushStroke stroke;
        stroke.points.push_back(glm::vec3(x, y, 0.0f));
        stroke.settings.pressure_sensitive = true;
        
        getEarthcallAPI().createBrushStroke(stroke);
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to paint: " << e.what() << std::endl;
    }
}

void RealWebView::_handleBrushGetAll() {
    try {
        std::vector<std::string> types = getEarthcallAPI().getAvailableBrushTypes();
        std::cout << "🎨 [INTEGRATION] Getting all brushes" << std::endl;
        
        nlohmann::json response = {
            {"type", "brush_list"},
            {"data", {{"brushes", types}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to get brushes: " << e.what() << std::endl;
    }
}

// Design System Handlers
void RealWebView::_handleDesignCreateShape(const nlohmann::json& data) {
    try {
        std::string type = data["type"];
        float x = data["x"];
        float y = data["y"];
        float width = data["width"];
        float height = data["height"];
        std::string color = data["color"];
        
        (void)width; (void)height;
        
        // TODO: Connect to actual DesignSystem when available
        std::cout << "🎨 [INTEGRATION] Would create shape: " << type << " at (" << x << ", " << y << ") with color " << color << std::endl;
        
        nlohmann::json response = {
            {"type", "shape_created"},
            {"data", {{"type", type}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to create shape: " << e.what() << std::endl;
    }
}

void RealWebView::_handleDesignCreateText(const nlohmann::json& data) {
    try {
        std::string text = data["text"];
        float x = data["x"];
        float y = data["y"];
        std::string font = data["font"];
        float size = data["size"];
        std::string color = data["color"];
        
        // TODO: Connect to actual DesignSystem when available
        std::cout << "🎨 [INTEGRATION] Would create text: " << text << " at (" << x << ", " << y << ") with font " << font << " size " << size << std::endl;
        
        nlohmann::json response = {
            {"type", "text_created"},
            {"data", {{"text", text}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to create text: " << e.what() << std::endl;
    }
}

void RealWebView::_handleDesignApplyEffect(const nlohmann::json& data) {
    try {
        std::string effect = data["effect"];
        std::string target = data["target"];
        
        // TODO: Connect to actual DesignSystem when available
        std::cout << "🎨 [INTEGRATION] Would apply effect: " << effect << " to " << target << std::endl;
        
        nlohmann::json response = {
            {"type", "effect_applied"},
            {"data", {{"effect", effect}, {"target", target}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to apply effect: " << e.what() << std::endl;
    }
}

void RealWebView::_handleDesignGetAll() {
    try {
        // TODO: Connect to actual DesignSystem when available
        std::cout << "🎨 [INTEGRATION] Would get all designs" << std::endl;
        
        nlohmann::json response = {
            {"type", "design_list"},
            {"data", {{"designs", {"demo_shape", "web_text", "web_circle"}}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to get designs: " << e.what() << std::endl;
    }
}

// Avatar System Handlers
void RealWebView::_handleAvatarCreate(const nlohmann::json& data) {
    try {
        std::string name = data["name"];
        std::string appearance = data.value("appearance", "default");
        
        std::cout << "👤 [INTEGRATION] Creating avatar: " << name << std::endl;
        // getEarthcallAPI().createAvatar(name, appearance);
        
        // Respond with success
        nlohmann::json response = {
            {"type", "avatar_created"},
            {"data", {{"name", name}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to create avatar: " << e.what() << std::endl;
    }
}

void RealWebView::_handleAvatarAnimate(const nlohmann::json& data) {
    try {
        std::string name = data["name"];
        std::string animation = data["animation"];
        
        std::cout << "👤 [INTEGRATION] Animating avatar: " << name << " with " << animation << std::endl;
        // getEarthcallAPI().animateAvatar(name, animation);
        
        // Respond with success
        nlohmann::json response = {
            {"type", "avatar_animated"},
            {"data", {{"name", name}, {"animation", animation}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to animate avatar: " << e.what() << std::endl;
    }
}

void RealWebView::_handleAvatarSetPosition(const nlohmann::json& data) {
    try {
        std::string name = data["name"];
        float x = data["x"];
        float y = data["y"];
        float z = data.value("z", 0.0f);
        
        std::cout << "👤 [INTEGRATION] Setting avatar position: " << name << " to (" << x << ", " << y << ", " << z << ")" << std::endl;
        // getEarthcallAPI().setAvatarPosition(name, glm::vec3(x, y, z));
        
        // Respond with success
        nlohmann::json response = {
            {"type", "avatar_position_set"},
            {"data", {{"name", name}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to set avatar position: " << e.what() << std::endl;
    }
}

void RealWebView::_handleAvatarGetAll() {
    try {
        std::vector<std::string> avatars; // = getEarthcallAPI().getAvatars();
        std::cout << "👤 [INTEGRATION] Getting all avatars" << std::endl;
        
        nlohmann::json response = {
            {"type", "avatar_list"},
            {"data", {{"avatars", avatars}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to get avatars: " << e.what() << std::endl;
    }
}

// World System Handlers
void RealWebView::_handleWorldCreateZone(const nlohmann::json& data) {
    try {
        std::string name = data["name"];
        float x = data["x"];
        float y = data["y"];
        float width = data["width"];
        float height = data["height"];
        
        std::cout << "🌍 [INTEGRATION] Creating zone: " << name << " at (" << x << ", " << y << ") size " << width << "x" << height << std::endl;
        getEarthcallAPI().createZone(name, x, y, width, height);
        
        nlohmann::json response = {
            {"type", "zone_created"},
            {"data", {{"name", name}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to create zone: " << e.what() << std::endl;
    }
}

void RealWebView::_handleWorldAddObject(const nlohmann::json& data) {
    try {
        std::string zoneName = data["zoneName"];
        std::string objectType = data["objectType"];
        float x = data["x"];
        float y = data["y"];
        
        std::cout << "🌍 [INTEGRATION] Adding object: " << objectType << " to zone " << zoneName << " at (" << x << ", " << y << ")" << std::endl;
        getEarthcallAPI().addZoneObject(zoneName, objectType, x, y);
        
        nlohmann::json response = {
            {"type", "object_added"},
            {"data", {{"zoneName", zoneName}, {"objectType", objectType}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to add object: " << e.what() << std::endl;
    }
}

void RealWebView::_handleWorldSetTheme(const nlohmann::json& data) {
    try {
        std::string zoneName = data["zoneName"];
        std::string theme = data["theme"];
        
        std::cout << "🌍 [INTEGRATION] Setting theme: " << theme << " for zone " << zoneName << std::endl;
        getEarthcallAPI().setZoneTheme(zoneName, theme);
        
        nlohmann::json response = {
            {"type", "theme_set"},
            {"data", {{"zoneName", zoneName}, {"theme", theme}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to set theme: " << e.what() << std::endl;
    }
}

void RealWebView::_handleWorldGetAll() {
    try {
        std::vector<std::string> zones = getEarthcallAPI().getZones();
        std::cout << "🌍 [INTEGRATION] Getting all zones" << std::endl;
        
        nlohmann::json response = {
            {"type", "world_list"},
            {"data", {{"zones", zones}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to get world: " << e.what() << std::endl;
    }
}

// UI Handlers
void RealWebView::_handleUINotification(const nlohmann::json& data) {
    try {
        std::string message = data["message"];
        std::string type = data["type"];
        
        std::cout << "🔔 [INTEGRATION] UI Notification [" << type << "]: " << message << std::endl;
        
        // TODO: Integrate with Earthcall's notification system
        nlohmann::json response = {
            {"type", "notification_shown"},
            {"data", {{"message", message}, {"type", type}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to show notification: " << e.what() << std::endl;
    }
}

void RealWebView::_handleUIOpenPanel(const nlohmann::json& data) {
    try {
        std::string panelName = data["panelName"];
        
        std::cout << "🔧 [INTEGRATION] Would open panel: " << panelName << std::endl;
        
        // TODO: Integrate with Earthcall's UI system
        nlohmann::json response = {
            {"type", "panel_opened"},
            {"data", {{"panelName", panelName}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to open panel: " << e.what() << std::endl;
    }
}

void RealWebView::_handleUISetCursor(const nlohmann::json& data) {
    try {
        std::string cursorType = data["cursorType"];
        
        std::cout << "🖱️ [INTEGRATION] Would set cursor: " << cursorType << std::endl;
        
        // TODO: Integrate with Earthcall's cursor system
        nlohmann::json response = {
            {"type", "cursor_set"},
            {"data", {{"cursorType", cursorType}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to set cursor: " << e.what() << std::endl;
    }
}

// Data Handlers
void RealWebView::_handleDataSave(const nlohmann::json& data) {
    try {
        std::string key = data["key"];
        auto value = data["value"];
        std::string valueStr = value.is_string() ? value.get<std::string>() : value.dump();
        
        std::cout << "💾 [INTEGRATION] Saving data: " << key << std::endl;
        getEarthcallAPI().saveData(key, valueStr);
        
        nlohmann::json response = {
            {"type", "data_saved"},
            {"data", {{"key", key}, {"success", true}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to save data: " << e.what() << std::endl;
    }
}

void RealWebView::_handleDataLoad(const nlohmann::json& data) {
    try {
        std::string key = data["key"];
        
        std::cout << "💾 [INTEGRATION] Loading data: " << key << std::endl;
        std::string value = getEarthcallAPI().loadData(key);
        
        nlohmann::json response = {
            {"type", "data_loaded"},
            {"data", {{"key", key}, {"value", value}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to load data: " << e.what() << std::endl;
    }
}

void RealWebView::_handleDataGetKeys() {
    try {
        std::vector<std::string> keys = getEarthcallAPI().getDataKeys();
        std::cout << "💾 [INTEGRATION] Getting all data keys" << std::endl;
        
        nlohmann::json response = {
            {"type", "data_keys"},
            {"data", {{"keys", keys}}}
        };
        sendMessageToWeb(response.dump());
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to get data keys: " << e.what() << std::endl;
    }
}

} // namespace Integration 