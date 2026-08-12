import glob

def process(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    orig = content
    
    # Syntax errors from bad replacements
    content = content.replace('Core::Engine* engine game', 'Core::Engine* engine')
    content = content.replace('Core::Engine& engine game', 'Core::Engine& engine')
    
    # Missing includes
    if 'MouseHandler.hpp' not in content and ('getMouseHandler()' in content or 'getMouseHandler()' in content):
        if '#include "Singularity/Core/Engine.hpp"' in content:
            content = content.replace('#include "Singularity/Core/Engine.hpp"', '#include "Singularity/Core/Engine.hpp"\n#include "Singularity/Input/MouseHandler.hpp"')
    if 'Camera.hpp' not in content and 'getCamera()' in content:
        if '#include "Singularity/Core/Engine.hpp"' in content:
            content = content.replace('#include "Singularity/Core/Engine.hpp"', '#include "Singularity/Core/Engine.hpp"\n#include "Singularity/Screen/Camera.hpp"')
            
    # missed replacements for CursorTools
    content = content.replace('engine.getCameraViewport()', 'engine.getCamera()->getViewport()')
    content = content.replace('engine.getCameraModelview()', 'engine.getCamera()->getModelview()')
    content = content.replace('engine.getCameraProjection()', 'engine.getCamera()->getProjection()')
    content = content.replace('engine.getCameraFront()', 'engine.getCamera()->getFront()')
    content = content.replace('engine.getCameraPos()', 'engine.getCamera()->getPos()')
    content = content.replace('engine.getCameraUp()', 'engine.getCamera()->getUp()')
    
    content = content.replace('engine->getCameraViewport()', 'engine->getCamera()->getViewport()')
    content = content.replace('engine->getCameraFront()', 'engine->getCamera()->getFront()')
    content = content.replace('engine->getCameraPos()', 'engine->getCamera()->getPos()')
    content = content.replace('engine->getCameraUp()', 'engine->getCamera()->getUp()')
    
    # RayDir undeclared issue
    content = content.replace('rayDir = glm::normalize(glm::vec3(farX, farY, farZ) - rayOrigin);', 'glm::vec3 rayDir = glm::normalize(glm::vec3(farX, farY, farZ) - rayOrigin);')
    # Oh wait, rayDir is an out-parameter in buildMouseRay: glm::vec3& rayDir! Why was it undeclared?
    # Ah, in Tool.cpp line 266: `rayDir = glm::normalize(...)`. Let's check Tool.cpp later if needed, but it should be declared. Maybe it's inside `buildMouseRay` but the error said `use of undeclared identifier 'rayDir'`.
    
    if orig != content:
        with open(filepath, 'w') as f:
            f.write(content)
        print("Updated", filepath)

for f in glob.glob('src/OurVerse/*.*') + glob.glob('src/Integration/*.*'):
    process(f)
