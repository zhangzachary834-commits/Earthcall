import re

with open('src/Singularity/Foreign/Web/WebIntegration.cpp', 'r') as f:
    content = f.read()

# Add include
content = content.replace('#include "Singularity/Foreign/Web/RealWebView.hpp"',
                          '#include "Singularity/Foreign/Web/RealWebView.hpp"\n#include "Singularity/Foreign/Web/DomMirrorBridge.hpp"\n#include "Singularity/Foreign/Web/DomMirrorTranslator.hpp"\n#include "Singularity/Core/EventBus.hpp"')

# Add member to WebViewImpl
content = re.sub(r'std::unique_ptr<RealWebView> _realWebView;',
                 r'std::unique_ptr<RealWebView> _realWebView;\n    std::unique_ptr<Singularity::Foreign::Web::DomMirrorBridge> _mirrorBridge;',
                 content)

# Initialize mirror bridge in init()
init_replacement = """        if (_realWebView) {
            bool ok = _realWebView->init();
            if (ok) {
                _mirrorBridge = std::make_unique<Singularity::Foreign::Web::DomMirrorBridge>(_realWebView.get());
                
                // When dom_mirror.js connects and sends a snapshot, it's admitted to the local graph.
                _mirrorBridge->onSnapshotAdmitted([](const Singularity::Foreign::Web::DomSnapshot& snapshot) {
                    std::cout << "🌐 [DomMirrorBridge] Snapshot admitted for " << snapshot.url << std::endl;
                    // Provide the document formation to the world, etc.
                });

                _mirrorBridge->injectMirrorScript();
            }
            return ok;"""

content = content.replace('        if (_realWebView) {\n            return _realWebView->init();', init_replacement)

# Wire navigation
nav_replacement = """        if (_realWebView) {
            if (_mirrorBridge) _mirrorBridge->onNavigationStarted();
            _realWebView->navigate(url);"""
content = content.replace('        if (_realWebView) {\n            _realWebView->navigate(url);', nav_replacement)

with open('src/Singularity/Foreign/Web/WebIntegration.cpp', 'w') as f:
    f.write(content)
