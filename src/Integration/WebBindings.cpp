#include "Singularity/Core/EventBus.hpp"
#include <string>

// emcc defines __EMSCRIPTEN__; plain EMSCRIPTEN is only a build-system
// environment variable, never a preprocessor macro. This guard never
// matched, so this whole translation unit compiled to nothing: --bind
// exported no symbols and web_ui/app.js's `Module.Earthcall_EmitUtterance`
// feature test was always false. See AUDIT_2026-08-10.md §2.6.
#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>

// Expose a global function to JS so the HTML frontend can push utterances
// directly into the Earthcall event bus when running in WebAssembly.
void Earthcall_EmitUtterance(std::string text, std::string clientId) {
    Core::Event::Utterance evt;
    evt.payload = text;
    evt.sourceClient = clientId;
    
    // Publish the utterance to the native C++ event bus.
    // The main loop (Engine::tick) or LanguageSystem will pick this up.
    Core::EventBus::instance().publish(evt);
}

EMSCRIPTEN_BINDINGS(earthcall_module) {
    emscripten::function("Earthcall_EmitUtterance", &Earthcall_EmitUtterance);
}
#endif
