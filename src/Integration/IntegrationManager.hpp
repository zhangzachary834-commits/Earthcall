#pragma once

#include "Integration/WebIntegration.hpp"
#include "Integration/WindowManager.hpp"
#include "Integration/EarthcallAPI.hpp"
#include <memory>

namespace Integration {

// Main integration manager that coordinates all integration systems
class IntegrationManager {
public:
    static IntegrationManager& instance();

    // Lifecycle
    bool init();
    void update();
    void render();
    void shutdown();
    
    // Save/Load
    void saveIntegrationData();
    void loadIntegrationData();

    // Web Integration
    bool registerWebApp(const WebApp::Config& config);
    void unregisterWebApp(const std::string& name);
    WebApp* getWebApp(const std::string& name);

    // Window Integration
    bool registerExternalWindow(const ExternalWindow::Config& config);
    void unregisterExternalWindow(const std::string& name);
    ExternalWindow* getExternalWindow(const std::string& name);

    // API Access
    EarthcallAPI& getAPI() { return _api; }

    // Global Settings
    void enableIntegration(bool enabled);
    void setIntegrationMode(const std::string& mode); // "web", "window", "both"
    void setSecurityLevel(const std::string& level); // "low", "medium", "high"

    // Communication
    void broadcastToAllIntegrations(const std::string& message);
    void sendToIntegration(const std::string& type, const std::string& name, const std::string& message);

    // UI
    void renderIntegrationUI();
    void showIntegrationSettings();

    // Getters
    bool isEnabled() const { return _enabled; }
    std::string getIntegrationMode() const { return _integrationMode; }
    std::string getSecurityLevel() const { return _securityLevel; }

private:
    IntegrationManager() = default;
    ~IntegrationManager() = default;

    // Sub-systems
    WebIntegrationManager& _webManager = WebIntegrationManager::instance();
    WindowManager& _windowManager = WindowManager::instance();
    // Use function-local static to avoid static init order issues
    EarthcallAPI _api;

    // State
    bool _enabled = false;
    std::string _integrationMode = "both";
    std::string _securityLevel = "medium";

    // UI state
    bool _showSettings = false;
    bool _showWebApps = false;
    bool _showExternalWindows = false;

    // Internal methods
    void _renderWebIntegrationUI();
    void _renderWindowIntegrationUI();
    void _renderSecuritySettings();
    void _handleIntegrationEvents();
};

} // namespace Integration 