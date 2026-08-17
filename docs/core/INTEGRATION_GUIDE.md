# 🌟 Earthcall Integration System Guide

## Overview

The Earthcall Integration System allows external applications and websites to interact with Earthcall's creative features at a deep level. This creates a powerful platform where other apps can overlay on top of Earthcall and use its brush system, design system, avatar system, and world creation capabilities.

## 🎯 What This Enables

### **Web Application Integration**
- Embed web apps directly into Earthcall
- Allow web apps to use Earthcall's creative tools
- Real-time communication between web and Earthcall
- Overlay web content on top of Earthcall's 3D world

### **External Application Overlays**
- Put other applications "on top of" Earthcall
- Use Earthcall's creative features with external apps
- Transparent overlays with full interaction
- Window composition and management

### **Creative Platform Capabilities**
- Brush system access for external apps
- Design system integration
- Avatar customization from external sources
- World object creation and manipulation
- Real-time collaboration features

## 🚀 Getting Started

### 1. **Enable Integration System**
Press `I` in Earthcall to open the Integration Manager UI.

### 2. **Register Web Applications**
In the Integration Manager:
1. Go to the "🌐 Web Apps" tab
2. Enter the app name and URL
3. Click "Add Web App"
4. The web app will be embedded and can access Earthcall's features

### 3. **Register External Windows**
In the Integration Manager:
1. Go to the "🪟 External Windows" tab
2. Enter window details (name, process, title)
3. Click "Add External Window"
4. The external app can now overlay on Earthcall

## 🔧 API Reference

### **Web Application API**

Web applications can communicate with Earthcall using JavaScript:

```javascript
// Connect to Earthcall
earthcall.connect();

// Request permissions
earthcall.requestPermissions();

// Create a brush stroke
earthcall.createBrushStroke({
    type: 'brush_stroke',
    settings: {
        size: 10,
        opacity: 0.8,
        color: '#ff0000',
        brush_type: 'default'
    },
    layer_name: 'web_integration_layer'
});

// Create a design element
earthcall.createDesignElement({
    type: 'design_element',
    name: 'My Design',
    design_type: 'shape',
    position: { x: 0, y: 0, z: 0 },
    properties: {
        color: '#00ff00',
        size: 5
    }
});

// Modify avatar
earthcall.modifyAvatar({
    type: 'avatar_modification',
    part: 'head',
    modification_type: 'color',
    parameters: {
        color: '#ff00ff',
        intensity: 0.8
    }
});
```

### **External Application API**

External applications can use the C++ API:

```cpp
#include "Singularity/Foreign/EarthcallAPI.hpp"

// Get the API instance
auto& api = Integration::getEarthcallAPI();

// Request permissions
api.requestPermission("brush_system");
api.requestPermission("design_system");
api.requestPermission("avatar_system");

// Create a brush stroke
Integration::EarthcallAPI::BrushStroke stroke;
stroke.settings.size = 2.0f;
stroke.settings.opacity = 0.8f;
stroke.settings.color = glm::vec3(1.0f, 0.0f, 0.0f);
stroke.points = {{0,0,0}, {1,1,1}, {2,0,2}};
api.createBrushStroke(stroke);

// Create a design element
Integration::EarthcallAPI::DesignElement element;
element.name = "My Design";
element.type = "shape";
element.position = glm::vec3(0.0f, 0.0f, 0.0f);
api.createDesignElement(element);
```

## 🎨 Creative Features Available

### **Brush System**
- Create brush strokes with custom settings
- Multiple brush types (soft, hard, airbrush, chalk, smudge, clone)
- Layer management
- Export artwork

### **Design System**
- Create geometric shapes
- Apply textures and patterns
- Design templates
- Transform and modify elements

### **Avatar System**
- Modify avatar parts (head, body, arms, legs, etc.)
- Change colors, shapes, and textures
- Import/export avatars
- Clothing and appearance customization

### **World System**
- Create objects in the 3D world
- Modify object positions and properties
- Camera control
- Physics interactions

## 🔒 Security & Governance

The integration system combines transport-level security with native Earthcall property governance:

### **Transport & Sandbox Security (`SecurityManager`)**
- **URL Validation & Whitelisting**: HTTPS enforcement, domain whitelisting/blacklisting
- **Content Security Policy (CSP)**: Prevents XSS and unauthorized frame embedding
- **JavaScript Sanitization**: Restricts unsafe eval, document.write, or script injection
- **Rate Limiting & Threat Detection**: Detects abnormal event floods and automatically blocks rogue sources
- **Activity Logging**: Full audit trail of foreign API calls and permission events

### **Authority & Property Governance (`TransferPolicy`)**
Rather than building an ad-hoc second permission system inside the engine, property mutation and data transfer are governed by `Singularity/TransferPolicy`'s three tiers:
- **Kernel**: Universally transferable and accessible; immune to lower-order law constraints.
- **Governable**: Accessible by default, but can be gated/closed by authored Laws.
- **Gated**: Closed by default; requires explicit Law authorization to access.

## 🌐 Example Web Integration

See `examples/web_integration_example.html` for a complete example of a web application that integrates with Earthcall.

The example includes:
- Connection management
- Brush system integration
- Drawing canvas
- Design system access
- Avatar modification
- Real-time communication

## 🪟 Example External Window Integration

```cpp
#include "Singularity/Foreign/IntegrationManager.hpp"

// Register an external window
Integration::ExternalWindow::Config config;
config.name = "My App";
config.process_name = "myapp.exe";
config.window_title = "My Application";
config.allow_overlay = true;
config.allow_transparency = true;

auto& manager = Integration::IntegrationManager::instance();
manager.registerExternalWindow(config);

// Enable overlay mode
auto* window = manager.getExternalWindow("My App");
if (window) {
    window->setOverlayMode(true);
    window->setTransparency(0.8f);
}
```

## 🔧 Advanced Features

### **Window Composition**
- Multiple overlay modes (blend, overlay, side-by-side)
- Z-order management
- Transparency controls
- Input forwarding

### **Real-time Communication**
- Event-driven messaging
- Callback registration
- Broadcast capabilities
- Bidirectional communication

### **Performance Optimization**
- Efficient rendering
- Memory management
- Update batching
- Resource sharing

## 🚨 Troubleshooting

### **Common Issues**

1. **Integration not working**
   - Check if integration is enabled (press `I`)
   - Verify permissions are granted
   - Check console for error messages

2. **Web app not connecting**
   - Ensure the web app URL is accessible
   - Check browser console for errors
   - Verify Earthcall is running

3. **External window not overlaying**
   - Confirm window process is running
   - Check window title matches exactly
   - Verify overlay permissions

### **Debug Information**
The integration system provides detailed logging:
- Connection status
- Permission requests
- API calls
- Error messages

## 🔮 Future Enhancements

### **Planned Features**
- Plugin system for custom integrations
- Advanced window management
- Multi-user collaboration
- Cloud integration
- AI-powered creative assistance

### **API Extensions**
- More creative tools
- Advanced rendering options
- Performance monitoring
- Analytics and metrics

## 📚 Additional Resources

- **API Documentation**: See header files in `src/Singularity/Foreign/`
- **Examples**: Check `examples/` directory
- **Source Code**: Integration modality code in `src/Singularity/Foreign/`
- **Web Demo**: `examples/web_integration_example.html`

## 🤝 Contributing

To extend the integration system:

1. **Add new API methods** in `src/Singularity/Foreign/EarthcallAPI.hpp`
2. **Implement features** in `src/Singularity/Foreign/EarthcallAPI.cpp`
3. **Update UI** in `src/Singularity/Foreign/IntegrationManager.cpp`
4. **Add examples** in `examples/` directory
5. **Update documentation** in this guide

## 🎉 Conclusion

The Earthcall Integration System transforms Earthcall from a standalone application into a powerful creative platform. By allowing external applications and websites to access Earthcall's features, it creates endless possibilities for creative collaboration and innovation.

Whether you're building a web-based design tool, integrating with existing applications, or creating new creative workflows, the integration system provides the foundation for truly epic creative experiences.

---

*For more information, see the source code in `src/Singularity/Foreign/` or run the example web application.*
