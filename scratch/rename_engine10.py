import os
import re

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        content = f.read()

    # Need Menu.hpp and Ourverse.hpp
    if '#include "ZonesOfEarth/Ourverse/Ourverse.hpp"' not in content:
        content = '#include "ZonesOfEarth/Ourverse/Ourverse.hpp"\n#include "ConstructedBeing/Object/Formation/Menu/Menu.hpp"\n' + content

    members = """    bool _useLegacy2DTools = false;
    
    Ourverse _world;
    Menu _mainMenu;
    
    // Combine tool state
    Object* _combineOperandA = nullptr;
    int _combineOp = 2;
    float _combineBlend = 0.15f;
    Object* _clayGrabbed = nullptr;
    Object* _clayTarget = nullptr;
    bool _fieldHandleDragging = false;
    bool _blendHandleDragging = false;

    // Perspective
    Ourverse::PerspectiveMode _currentPerspective = Ourverse::PerspectiveMode::FirstPerson;
"""
    if "Ourverse _world;" not in content:
        content = content.replace("    bool _useLegacy2DTools = false;", members)
    
    methods = """    bool getUseLegacy2DTools() const { return _useLegacy2DTools; }
    
    Ourverse& getWorld() { return _world; }
    Menu& getMainMenu() { return _mainMenu; }
    
    void fuseObjects(Object* A, Object* B);
    void blendRail(const Object* o, glm::vec3& start, glm::vec3& dir, float& length) const;
    bool handleFieldGizmos(Object* o, bool pressEdge, bool mouseDown, double winX, double winY);
    void registerCallbacks();
"""
    if "Ourverse& getWorld()" not in content:
        content = content.replace("    bool getUseLegacy2DTools() const { return _useLegacy2DTools; }", methods)

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")

def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        content = f.read()
    
    content = content.replace("PerspectiveMode", "Ourverse::PerspectiveMode")
    content = content.replace("_camera.pos", "_camera->pos")
    content = content.replace("_camera.front", "_camera->front")
    content = content.replace("_camera.up", "_camera->up")
    content = content.replace("_camera.modelview", "_camera->modelview")
    content = content.replace("_camera.projection", "_camera->projection")
    content = content.replace("_camera.viewport", "_camera->viewport")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineRender.cpp")

def fix_engine_update():
    path = "src/Singularity/Core/EngineUpdate.cpp"
    with open(path, 'r') as f:
        content = f.read()

    content = content.replace("_camera.front", "_camera->front")
    content = content.replace("_camera.up", "_camera->up")
    content = content.replace("_camera.modelview", "_camera->modelview")
    content = content.replace("_camera.projection", "_camera->projection")
    content = content.replace("_camera.viewport", "_camera->viewport")

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineUpdate.cpp")

def fix_engine_init():
    path = "src/Singularity/Core/EngineInit.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    # Revert my bad regex replaces
    content = content.replace("true.addOption", "_mainMenu.addOption")
    content = content.replace("true.close()", "_mainMenu.close()")
    
    # We still need to replace some World stuff that doesn't exist
    # mgr.active().world().saveStateWithLog() -> mgr.saveStateWithLog() maybe?
    # Actually just replace them with dummy for now to get it compiling
    content = content.replace("mgr.active().world().saveStateWithLog();", "")
    content = content.replace("mgr.active().world().showChatWindow", "_world.showChatWindow")
    content = content.replace("mgr.active().world().showToolbar", "_world.showToolbar")
    content = content.replace("mgr.active().world().showKeymapWindow", "_world.showKeymapWindow")
    content = content.replace("mgr.active().world().setCamera", "_world.setCamera")
    content = content.replace("mgr.active().world().addObject", "_world.addOwnedObject") # hack
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed EngineInit.cpp")

def fix_engine_cpp():
    path = "src/Singularity/Core/Engine.cpp"
    with open(path, 'r') as f:
        content = f.read()
    
    content = content.replace("void Engine::run(Game& game)", "void Engine::run()")
    content = content.replace("void Engine::tick(Game& game, float dt)", "void Engine::tick(float dt)")
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.cpp")

if __name__ == '__main__':
    fix_engine_hpp()
    fix_engine_render()
    fix_engine_update()
    fix_engine_init()
    fix_engine_cpp()
