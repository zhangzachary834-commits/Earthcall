import os

path1 = "src/Singularity/FirstMoverOntology/FirstMoverWindowTools/PerformanceMetricsWindow.cpp"
with open(path1, "r") as f:
    s1 = f.read()

if "FrameTimings g_frameTimings" not in s1:
    s1 = s1.replace("extern ZoneManager mgr;", "extern ZoneManager mgr;

FrameTimings g_frameTimings{};")

old_law = "        // ---- LawManager tick() timing breakdown ----"
new_breakdown = """        // ---- Engine::tick() Frame Timings Breakdown ----
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Engine::tick()  %.2f ms", g_frameTimings.total_ms);
        ImGui::Text("  Input:           %6.2f ms", g_frameTimings.input_ms);
        ImGui::Text("  Locomotion:      %6.2f ms", g_frameTimings.locomotion_ms);
        ImGui::Text("  Creation Tools:  %6.2f ms", g_frameTimings.creation_ms);
        ImGui::Text("  Interaction:     %6.2f ms", g_frameTimings.interaction_ms);
        ImGui::Text("  Zone Update:     %6.2f ms", g_frameTimings.zone_ms);
        ImGui::Text("  Laws (Rete):     %6.2f ms", g_frameTimings.laws_ms);
        ImGui::Text("  Language:        %6.2f ms", g_frameTimings.language_ms);
        ImGui::Text("  Audio:           %6.2f ms", g_frameTimings.audio_ms);
        ImGui::Text("  3D Render:       %6.2f ms", g_frameTimings.render3d_ms);
        ImGui::Text("  ImGui / Present: %6.2f ms", g_frameTimings.imgui_ms);

        // ---- LawManager tick() timing breakdown ----"""

if old_law in s1 and "Engine::tick()  %.2f ms" not in s1:
    s1 = s1.replace(old_law, new_breakdown)
    with open(path1, "w") as f:
        f.write(s1)
    print("Updated PerformanceMetricsWindow.cpp")

path2 = "src/Singularity/Core/EngineUpdate.cpp"
with open(path2, "r") as f:
    s2 = f.read()

if "PerformanceMetricsWindow.hpp" not in s2:
    inc2_old = '#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp"'
    inc2_new = '#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/PerformanceMetricsWindow.hpp"
#include <chrono>'
    s2 = s2.replace(inc2_old, inc2_new)

old_update = """    void Engine::update(float dt) {
        if (!_keyboardHandler || !_mouseHandler || !_camera || !_player || !_lawManager) return;

        // Update input handlers
        if (_mainMenu.isOpen()) {
            _mainMenu.processInput(_window);
        }
        
        _keyboardHandler->update();
        _mouseHandler->update();

        // Update camera front from mouse handler
        _camera->front = _mouseHandler->calculateCameraFront();

        // Check if any text input is active (ImGui)
        bool anyTextInputActive = ImGui::IsAnyItemActive() || ImGui::IsWindowFocused();

        // Vessel movement — Input first mover, not Person.
        const bool canMove = _mouseHandler->isCursorLocked() && !_mainMenu.isOpen() && !anyTextInputActive;
        const bool flying  = Physics::getFlying();
        if (auto* locomotion = Singularity::Input::LocomotionChannel::find(*_lawManager)) {
            locomotion->step(*_player, *_camera, _window, mgr, dt, flying, canMove);
        }

        // Creation first mover — sense placement, honour L, push the
        // console's live selection onto the channel, actuate the armed
        // tool. Used to run inside render3DConsole / DeveloperToolsWindow,
        // so collapsing the console froze every 3D tool.
        Rendering::stepCreationTools(_window, this, mgr, dt, _creatorConsoleOpen);

        // Interaction first mover — pick the being under the pointer, publish
        // the click/scroll/focus edges, drive hover. Stepped here and not from
        // a render function, for the reason above it: a channel that only runs
        // while a window is on screen is a channel that freezes when the
        // window collapses.
        //
        // WantCaptureMouse is the foreign-surface veto: while an ImGui panel
        // owns the pointer, the world must see no pointer at all, or the
        // Person clicks a menu and a button behind it fires too.
        if (auto* interaction = Singularity::Input::InteractionChannel::find(*_lawManager)) {
            interaction->step(_window, *_camera, mgr, ImGui::GetIO().WantCaptureMouse);
        }

        // Update world (physics etc.)
        mgr.active().update(dt);
        mgr.active().applyFormationRelations();

        // Evict unreferenced smooth tessellation caches from destroyed or modified objects
        Object::gcSmoothTessellationCache();

        // Advance time
        _worldTime += static_cast<double>(dt);
        Universe::instance().setClock(_worldTime, static_cast<double>(dt));
    }"""

new_update = """    void Engine::update(float dt) {
        if (!_keyboardHandler || !_mouseHandler || !_camera || !_player || !_lawManager) return;

        using clock = std::chrono::steady_clock;
        auto getMs = [](clock::time_point start, clock::time_point end) {
            return std::chrono::duration<float, std::milli>(end - start).count();
        };

        // Update input handlers
        auto tInput0 = clock::now();
        if (_mainMenu.isOpen()) {
            _mainMenu.processInput(_window);
        }
        
        _keyboardHandler->update();
        _mouseHandler->update();

        // Update camera front from mouse handler
        _camera->front = _mouseHandler->calculateCameraFront();
        auto tInput1 = clock::now();
        g_frameTimings.input_ms = getMs(tInput0, tInput1);

        // Check if any text input is active (ImGui)
        bool anyTextInputActive = ImGui::IsAnyItemActive() || ImGui::IsWindowFocused();

        // Vessel movement — Input first mover, not Person.
        const bool canMove = _mouseHandler->isCursorLocked() && !_mainMenu.isOpen() && !anyTextInputActive;
        const bool flying  = Physics::getFlying();
        auto tLoco0 = clock::now();
        if (auto* locomotion = Singularity::Input::LocomotionChannel::find(*_lawManager)) {
            locomotion->step(*_player, *_camera, _window, mgr, dt, flying, canMove);
        }
        auto tLoco1 = clock::now();
        g_frameTimings.locomotion_ms = getMs(tLoco0, tLoco1);

        // Creation first mover — sense placement, honour L, push the
        // console's live selection onto the channel, actuate the armed
        // tool. Used to run inside render3DConsole / DeveloperToolsWindow,
        // so collapsing the console froze every 3D tool.
        auto tCreate0 = clock::now();
        Rendering::stepCreationTools(_window, this, mgr, dt, _creatorConsoleOpen);
        auto tCreate1 = clock::now();
        g_frameTimings.creation_ms = getMs(tCreate0, tCreate1);

        // Interaction first mover — pick the being under the pointer, publish
        // the click/scroll/focus edges, drive hover. Stepped here and not from
        // a render function, for the reason above it: a channel that only runs
        // while a window is on screen is a channel that freezes when the
        // window collapses.
        //
        // WantCaptureMouse is the foreign-surface veto: while an ImGui panel
        // owns the pointer, the world must see no pointer at all, or the
        // Person clicks a menu and a button behind it fires too.
        auto tInteract0 = clock::now();
        if (auto* interaction = Singularity::Input::InteractionChannel::find(*_lawManager)) {
            interaction->step(_window, *_camera, mgr, ImGui::GetIO().WantCaptureMouse);
        }
        auto tInteract1 = clock::now();
        g_frameTimings.interaction_ms = getMs(tInteract0, tInteract1);

        // Update world (physics etc.)
        auto tZone0 = clock::now();
        mgr.active().update(dt);
        mgr.active().applyFormationRelations();

        // Evict unreferenced smooth tessellation caches from destroyed or modified objects
        Object::gcSmoothTessellationCache();

        // Advance time
        _worldTime += static_cast<double>(dt);
        Universe::instance().setClock(_worldTime, static_cast<double>(dt));
        auto tZone1 = clock::now();
        g_frameTimings.zone_ms = getMs(tZone0, tZone1);
    }"""

if old_update in s2:
    s2 = s2.replace(old_update, new_update)
    with open(path2, "w") as f:
        f.write(s2)
    print("Updated EngineUpdate.cpp")

path3 = "src/Singularity/Core/Engine.cpp"
with open(path3, "r") as f:
    s3 = f.read()

if "<chrono>" not in s3:
    inc3_old = "#include <climits>"
    inc3_new = "#include <climits>
#include <chrono>"
    s3 = s3.replace(inc3_old, inc3_new)

old_tick_start = """void Engine::tick(float dt) {
        glfwPollEvents();"""

new_tick_start = """void Engine::tick(float dt) {
    using clock = std::chrono::steady_clock;
    auto tickStart = clock::now();
    auto getMs = [](clock::time_point start, clock::time_point end) {
        return std::chrono::duration<float, std::milli>(end - start).count();
    };

    glfwPollEvents();"""

s3 = s3.replace(old_tick_start, new_tick_start)

old_mid = """        // 1. Process physical & logical simulation
        update(dt);

        // Tick language modality
        Singularity::Language::LanguageSystem::instance().tick(dt);
        Core::Audio::AudioSystem::instance().tick();
#ifdef __EMSCRIPTEN__
        Core::EventBus::instance().tick();
#endif"""

new_mid = """        // 1. Process physical & logical simulation
        update(dt);

        // Tick language modality
        auto tLang0 = clock::now();
        Singularity::Language::LanguageSystem::instance().tick(dt);
        auto tLang1 = clock::now();
        g_frameTimings.language_ms = getMs(tLang0, tLang1);

        auto tAudio0 = clock::now();
        Core::Audio::AudioSystem::instance().tick();
#ifdef __EMSCRIPTEN__
        Core::EventBus::instance().tick();
#endif
        auto tAudio1 = clock::now();
        g_frameTimings.audio_ms = getMs(tAudio0, tAudio1);"""

s3 = s3.replace(old_mid, new_mid)

old_law3 = "        if (_lawManager) _lawManager->tick();"
new_law3 = """        auto tLaws0 = clock::now();
        if (_lawManager) _lawManager->tick();
        auto tLaws1 = clock::now();
        g_frameTimings.laws_ms = getMs(tLaws0, tLaws1);"""

s3 = s3.replace(old_law3, new_law3)

old_render = """        // 2. Render 3D scene (must happen before ImGui::Render composites over it)
        render();

        // Render ImGui
        ImGui::Render();
#ifdef EARTHCALL_WEBGPU
        // game.render() already closed the scene pass, but the acquired surface
        // texture is still held — so imgui composites in a second load-op pass on
        // top of it, and only then does the frame get presented.
        _webgpu->renderer.overlayPass([](WGPURenderPassEncoder pass) {
            ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
        });
        _webgpu->renderer.present();
#else
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(_window);
#endif
}"""

new_render = """        // 2. Render 3D scene (must happen before ImGui::Render composites over it)
        auto tRender0 = clock::now();
        render();
        auto tRender1 = clock::now();
        g_frameTimings.render3d_ms = getMs(tRender0, tRender1);

        // Render ImGui
        auto tGui0 = clock::now();
        ImGui::Render();
#ifdef EARTHCALL_WEBGPU
        // game.render() already closed the scene pass, but the acquired surface
        // texture is still held — so imgui composites in a second load-op pass on
        // top of it, and only then does the frame get presented.
        _webgpu->renderer.overlayPass([](WGPURenderPassEncoder pass) {
            ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
        });
        _webgpu->renderer.present();
#else
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(_window);
#endif
        auto tGui1 = clock::now();
        g_frameTimings.imgui_ms = getMs(tGui0, tGui1);

        auto tickEnd = clock::now();
        g_frameTimings.total_ms = getMs(tickStart, tickEnd);
}"""

if old_render in s3:
    s3 = s3.replace(old_render, new_render)
    with open(path3, "w") as f:
        f.write(s3)
    print("Updated Engine.cpp")
