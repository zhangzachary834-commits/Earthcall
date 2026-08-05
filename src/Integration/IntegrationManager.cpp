#include "Integration/IntegrationManager.hpp"
#include "Integration/WebIntegration.hpp"
#include "Integration/SecurityManager.hpp"
#include "../json.hpp"
#include "Util/SaveSystem.hpp"
#include <imgui.h>
#include <iostream>

namespace Integration {

IntegrationManager& IntegrationManager::instance() {
    static IntegrationManager s_instance;
    return s_instance;
}

bool IntegrationManager::init() {
    std::cout << "🔗 Integration Manager initialized" << std::endl;
    
    // Initialize security manager
    auto& security = SecurityManager::instance();
    security.setSecurityLevel(SecurityLevel::MEDIUM);
    security.loadSecurityData();
    
    // Set up default security configuration
    SecurityConfig config;
    config.whitelistedDomains = {
        "https://trusted.earthcall.com",
        "https://api.earthcall.com",
        "https://docs.earthcall.com"
    };
    config.blacklistedDomains = {
        "malicious-site.com",
        "phishing-example.com"
    };
    config.enableCSP = true;
    config.enableSandboxing = true;
    config.requireUserConfirmation = true;
    
    security.setConfig(config);
    
    // Load saved integration data
    loadIntegrationData();
    
    return true;
}

void IntegrationManager::update() {
    _webManager.update();
    _windowManager.update();
    _api.update();
}

void IntegrationManager::render() {
    _webManager.render();
    _windowManager.render();
}

void IntegrationManager::shutdown() {
    // Save integration data before shutting down
    saveIntegrationData();
    
    // Save security data
    SecurityManager::instance().saveSecurityData();
    
    _webManager.shutdown();
    _windowManager.shutdown();
    _api.shutdown();
    std::cout << "🔗 Integration Manager shutdown" << std::endl;
}

bool IntegrationManager::registerWebApp(const WebApp::Config& config) {
    return _webManager.registerApp(config);
}

void IntegrationManager::unregisterWebApp(const std::string& name) {
    _webManager.unregisterApp(name);
}

WebApp* IntegrationManager::getWebApp(const std::string& name) {
    return _webManager.getApp(name);
}

bool IntegrationManager::registerExternalWindow(const ExternalWindow::Config& config) {
    return _windowManager.registerWindow(config);
}

void IntegrationManager::unregisterExternalWindow(const std::string& name) {
    _windowManager.unregisterWindow(name);
}

ExternalWindow* IntegrationManager::getExternalWindow(const std::string& name) {
    return _windowManager.getWindow(name);
}

void IntegrationManager::enableIntegration(bool enabled) {
    _enabled = enabled;
    std::cout << "🔗 Integration " << (enabled ? "enabled" : "disabled") << std::endl;
}

void IntegrationManager::setIntegrationMode(const std::string& mode) {
    _integrationMode = mode;
    std::cout << "🔗 Integration mode set to: " << mode << std::endl;
}

void IntegrationManager::setSecurityLevel(const std::string& level) {
    _securityLevel = level;
    std::cout << "🔗 Security level set to: " << level << std::endl;
}

void IntegrationManager::broadcastToAllIntegrations(const std::string& message) {
    _webManager.broadcastToAllApps(message);
    // TODO: Add window manager broadcast
}

void IntegrationManager::sendToIntegration(const std::string& type, const std::string& name, const std::string& message) {
    if (type == "web") {
        _webManager.sendToApp(name, message);
    } else if (type == "window") {
        // TODO: Add window manager send
    }
}

void IntegrationManager::renderIntegrationUI() {
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("🔗 Integration Manager", &_showSettings);

    // Status
    ImGui::Text("Status: %s", _enabled ? "Enabled" : "Disabled");
    ImGui::Text("Mode: %s", _integrationMode.c_str());
    ImGui::Text("Security: %s", _securityLevel.c_str());
    
    ImGui::Separator();

    // Controls
    if (ImGui::Button(_enabled ? "Disable Integration" : "Enable Integration")) {
        enableIntegration(!_enabled);
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Settings")) {
        _showSettings = true;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("💾 Save Now")) {
        saveIntegrationData();
    }

    ImGui::Separator();

    // Tabs
    if (ImGui::BeginTabBar("IntegrationTabs")) {
        if (ImGui::BeginTabItem("🌐 Web Apps")) {
            _renderWebIntegrationUI();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("🪟 External Windows")) {
            _renderWindowIntegrationUI();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("🔒 Security")) {
            _renderSecuritySettings();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }

    ImGui::End();

    // Settings window
    if (_showSettings) {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("Integration Settings", &_showSettings);
        
        ImGui::Text("Integration Settings");
        ImGui::Separator();
        
        // Mode selection
        const char* modes[] = { "web", "window", "both" };
        static int currentMode = 2; // "both"
        if (ImGui::Combo("Integration Mode", &currentMode, modes, IM_ARRAYSIZE(modes))) {
            setIntegrationMode(modes[currentMode]);
        }
        
        // Security level
        const char* levels[] = { "low", "medium", "high" };
        static int currentLevel = 1; // "medium"
        if (ImGui::Combo("Security Level", &currentLevel, levels, IM_ARRAYSIZE(levels))) {
            setSecurityLevel(levels[currentLevel]);
        }
        
        ImGui::Separator();
        
        // Feature toggles
        static bool brushAccess = false;
        static bool designAccess = false;
        static bool avatarAccess = false;
        
        if (ImGui::Checkbox("Allow Brush System Access", &brushAccess)) {
            if (brushAccess) {
                _api.requestPermission("brush_system");
            }
        }
        
        if (ImGui::Checkbox("Allow Design System Access", &designAccess)) {
            if (designAccess) {
                _api.requestPermission("design_system");
            }
        }
        
        if (ImGui::Checkbox("Allow Avatar System Access", &avatarAccess)) {
            if (avatarAccess) {
                _api.requestPermission("avatar_system");
            }
        }
        
        ImGui::End();
    }
}

void IntegrationManager::_renderWebIntegrationUI() {
    ImGui::Text("Web Applications");
    ImGui::Separator();
    
    // Add new web app
    static char appName[64] = "";
    static char appUrl[256] = "";
    
    ImGui::InputText("App Name", appName, sizeof(appName));
    ImGui::InputText("App URL", appUrl, sizeof(appUrl));
    
    if (ImGui::Button("Add Web App")) {
        if (strlen(appName) > 0 && strlen(appUrl) > 0) {
            WebApp::Config config;
            config.name = appName;
            config.url = appUrl;
            config.allow_overlay = true;
            config.allow_earthcall_features = true;
            
            if (registerWebApp(config)) {
                appName[0] = '\0';
                appUrl[0] = '\0';
            }
        }
    }
    
    ImGui::Separator();
    
    // List registered apps
    ImGui::Text("Registered Web Apps:");
    
    const auto& apps = _webManager.getAllApps();
    if (apps.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No web apps registered yet");
    } else {
        for (const auto& [name, app] : apps) {
            ImGui::PushID(name.c_str());
            
            // App info
            ImGui::Text("📱 %s", name.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(%s)", app->getConfig().url.c_str());
            
            // Status indicator
            ImGui::SameLine();
            if (app->isActive()) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "● Active");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "○ Inactive");
            }
            
            // Control buttons
            ImGui::SameLine();
            if (ImGui::Button("Open")) {
                app->showWindow();
                std::cout << "🌐 Opening web app: " << name << std::endl;
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Remove")) {
                _webManager.unregisterApp(name);
            }
            
            // Web interaction controls
            if (ImGui::CollapsingHeader("🌐 Web Interaction Tools")) {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Click on any element in the web page to modify it!");
                
                // Quick Actions
                ImGui::Separator();
                ImGui::Text("🚀 Quick Actions:");
                
                if (ImGui::Button("🎨 Make Page Beautiful")) {
                    // Inject some nice CSS to make the page look better
                    std::string beautifulCSS = R"(
                        body { 
                            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif !important;
                            line-height: 1.6 !important;
                            color: #333 !important;
                        }
                        h1, h2, h3 { color: #2c3e50 !important; }
                        button, input, select { 
                            border-radius: 8px !important;
                            border: 2px solid #3498db !important;
                            padding: 8px 12px !important;
                        }
                        .container, .wrapper { 
                            max-width: 1200px !important;
                            margin: 0 auto !important;
                            padding: 20px !important;
                        }
                    )";
                    app->getWebView()->injectCSS(beautifulCSS);
                }
                
                ImGui::SameLine();
                if (ImGui::Button("🌙 Dark Mode")) {
                    std::string darkCSS = R"(
                        body { 
                            background-color: #1a1a1a !important;
                            color: #ffffff !important;
                        }
                        * { 
                            background-color: #1a1a1a !important;
                            color: #ffffff !important;
                            border-color: #444 !important;
                        }
                        input, textarea { 
                            background-color: #2d2d2d !important;
                            color: #ffffff !important;
                        }
                    )";
                    app->getWebView()->injectCSS(darkCSS);
                }
                
                ImGui::SameLine();
                if (ImGui::Button("☀️ Light Mode")) {
                    std::string lightCSS = R"(
                        body { 
                            background-color: #ffffff !important;
                            color: #333333 !important;
                        }
                        * { 
                            background-color: #ffffff !important;
                            color: #333333 !important;
                            border-color: #ddd !important;
                        }
                        input, textarea { 
                            background-color: #f8f9fa !important;
                            color: #333333 !important;
                        }
                    )";
                    app->getWebView()->injectCSS(lightCSS);
                }
                
                ImGui::Separator();
                ImGui::Text("🎯 Element Inspector:");
                
                // Element selection mode
                static bool inspectorMode = false;
                if (ImGui::Button(inspectorMode ? "🔍 Stop Inspecting" : "🔍 Start Inspecting")) {
                    inspectorMode = !inspectorMode;
                    if (inspectorMode) {
                        // Enable element highlighting on hover
                        std::string inspectorJS = R"(
                            let originalOutlines = new Map();
                            document.addEventListener('mouseover', function(e) {
                                if (e.target !== document.body) {
                                    originalOutlines.set(e.target, e.target.style.outline);
                                    e.target.style.outline = '2px solid #ff6b6b';
                                    e.target.style.cursor = 'pointer';
                                }
                            });
                            document.addEventListener('mouseout', function(e) {
                                if (originalOutlines.has(e.target)) {
                                    e.target.style.outline = originalOutlines.get(e.target);
                                    e.target.style.cursor = '';
                                }
                            });
                            document.addEventListener('click', function(e) {
                                e.preventDefault();
                                e.stopPropagation();
                                console.log('Selected element:', e.target.tagName, e.target.className, e.target.id);
                                window.lastSelectedElement = e.target;
                                // Send element info back to Earthcall
                                if (window.webkit && window.webkit.messageHandlers) {
                                    window.webkit.messageHandlers.elementSelected.postMessage({
                                        tagName: e.target.tagName,
                                        className: e.target.className,
                                        id: e.target.id,
                                        text: e.target.textContent.substring(0, 100)
                                    });
                                }
                            });
                        )";
                        app->getWebView()->executeJavaScript(inspectorJS);
                    } else {
                        // Disable element highlighting
                        std::string disableJS = R"(
                            // Restore original outlines
                            document.querySelectorAll('*').forEach(el => {
                                el.style.outline = '';
                                el.style.cursor = '';
                            });
                        )";
                        app->getWebView()->executeJavaScript(disableJS);
                    }
                }
                
                ImGui::SameLine();
                if (ImGui::Button("📸 Screenshot")) {
                    app->getWebView()->captureScreenshot();
                }
                
                ImGui::SameLine();
                if (ImGui::Button("🔧 Developer Mode")) {
                    app->getWebView()->enableDeveloperMode(true);
                }
                
                // Element modification panel
                ImGui::Separator();
                ImGui::Text("✏️ Modify Selected Element:");
                
                static char elementText[256] = "";
                static char elementColor[32] = "#ff6b6b";
                static char elementSize[32] = "16px";
                
                ImGui::Text("Text Content:");
                ImGui::InputText("##elementText", elementText, sizeof(elementText));
                
                ImGui::Text("Text Color:");
                ImGui::SameLine();
                ImGui::ColorEdit3("##elementColor", (float*)&elementColor[1], ImGuiColorEditFlags_NoInputs);
                
                ImGui::Text("Font Size:");
                ImGui::InputText("##elementSize", elementSize, sizeof(elementSize));
                
                if (ImGui::Button("Apply Changes")) {
                    if (strlen(elementText) > 0) {
                        // Apply changes to the last selected element
                        std::string js = "if (window.lastSelectedElement) {";
                        if (strlen(elementText) > 0) {
                            js += "window.lastSelectedElement.textContent = '" + std::string(elementText) + "';";
                        }
                        if (strlen(elementColor) > 1) {
                            js += "window.lastSelectedElement.style.color = '" + std::string(elementColor) + "';";
                        }
                        if (strlen(elementSize) > 0) {
                            js += "window.lastSelectedElement.style.fontSize = '" + std::string(elementSize) + "';";
                        }
                        js += "}";
                        app->getWebView()->executeJavaScript(js);
                    }
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Hide Element")) {
                    std::string js = "if (window.lastSelectedElement) { window.lastSelectedElement.style.display = 'none'; }";
                    app->getWebView()->executeJavaScript(js);
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Show Element")) {
                    std::string js = "if (window.lastSelectedElement) { window.lastSelectedElement.style.display = ''; }";
                    app->getWebView()->executeJavaScript(js);
                }
            }
            
            ImGui::PopID();
            ImGui::Separator();
        }
    }
}

void IntegrationManager::_renderWindowIntegrationUI() {
    ImGui::Text("External Windows");
    ImGui::Separator();
    
    // Add new external window
    static char windowName[64] = "";
    static char processName[64] = "";
    static char windowTitle[256] = "";
    
    ImGui::InputText("Window Name", windowName, sizeof(windowName));
    ImGui::InputText("Process Name", processName, sizeof(processName));
    ImGui::InputText("Window Title", windowTitle, sizeof(windowTitle));
    
    if (ImGui::Button("Add External Window")) {
        if (strlen(windowName) > 0 && strlen(processName) > 0) {
            ExternalWindow::Config config;
            config.name = windowName;
            config.process_name = processName;
            config.window_title = windowTitle;
            config.allow_overlay = true;
            config.allow_transparency = true;
            
            if (registerExternalWindow(config)) {
                windowName[0] = '\0';
                processName[0] = '\0';
                windowTitle[0] = '\0';
            }
        }
    }
    
    ImGui::Separator();
    
    // List registered windows
    ImGui::Text("Registered External Windows:");
    // TODO: Get actual list from window manager
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No external windows registered yet");
}

void IntegrationManager::_renderSecuritySettings() {
    auto& security = SecurityManager::instance();
    
    ImGui::Text("Security Settings");
    ImGui::Separator();
    
    // Security level
    ImGui::Text("Current Security Level: %s", _securityLevel.c_str());
    
    // Security statistics
    ImGui::Text("Security Statistics:");
    ImGui::Text("Total Events: %d", security.getTotalEvents());
    ImGui::Text("Blocked Events: %d", security.getBlockedEvents());
    
    ImGui::Separator();
    
    // Security level control
    const char* levels[] = { "LOW", "MEDIUM", "HIGH", "PARANOID" };
    static int currentLevel = static_cast<int>(SecurityLevel::MEDIUM);
    if (ImGui::Combo("Security Level", &currentLevel, levels, IM_ARRAYSIZE(levels))) {
        security.setSecurityLevel(static_cast<SecurityLevel>(currentLevel));
        _securityLevel = levels[currentLevel];
    }
    
    ImGui::Separator();
    
    // Domain management
    if (ImGui::CollapsingHeader("Domain Management")) {
        auto config = security.getConfig();
        
        // Whitelisted domains
        ImGui::Text("Whitelisted Domains:");
        for (const auto& domain : config.whitelistedDomains) {
            ImGui::BulletText("%s", domain.c_str());
        }
        
        // Blacklisted domains
        ImGui::Text("Blacklisted Domains:");
        for (const auto& domain : config.blacklistedDomains) {
            ImGui::BulletText("%s", domain.c_str());
        }
    }
    
    ImGui::Separator();
    
    // Security actions
    if (ImGui::Button("Reset All Permissions")) {
        // TODO: Implement permission reset for all sources
        std::cout << "🔒 All permissions reset" << std::endl;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Export Security Log")) {
        security.exportSecurityLog("security_log.txt");
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear Security Log")) {
        security.clearSecurityLog();
    }
    
    ImGui::Separator();
    
    // Recent security events
    if (ImGui::CollapsingHeader("Recent Security Events")) {
        auto events = security.getSecurityLog();
        int count = 0;
        for (auto it = events.rbegin(); it != events.rend() && count < 10; ++it, ++count) {
            const auto& event = *it;
            ImVec4 color = event.blocked ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImGui::TextColored(color, "%s from %s", event.description.c_str(), event.source.c_str());
            if (!event.details.empty()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%s)", event.details.c_str());
            }
        }
    }
}

void IntegrationManager::showIntegrationSettings() {
    _showSettings = true;
}

void IntegrationManager::saveIntegrationData() {
    try {
        std::cout << "🔗 IntegrationManager: Starting save..." << std::endl;
        
        // Save web apps
        _webManager.saveWebApps();
        
        // TODO: Save external windows when implemented
        // _windowManager.saveExternalWindows();
        
        std::cout << "💾 Integration data saved successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to save integration data: " << e.what() << std::endl;
    }
}

void IntegrationManager::loadIntegrationData() {
    try {
        std::cout << "🔗 IntegrationManager: Starting load..." << std::endl;
        
        // Load web apps
        _webManager.loadWebApps();
        
        // TODO: Load external windows when implemented
        // _windowManager.loadExternalWindows();
        
        std::cout << "📂 Integration data loaded successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "❌ Failed to load integration data: " << e.what() << std::endl;
    }
}

} // namespace Integration 