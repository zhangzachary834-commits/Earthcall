import glob

def process(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    orig = content
    content = content.replace('class Game;', 'class Engine;')
    content = content.replace('Core::Game', 'Core::Engine')
    content = content.replace('Core::Engine*', 'Core::Engine* engine') # Oops, if there was no parameter name
    # Let's just fix the function signatures in the cpp files:
    content = content.replace('update(Core::Engine& game)', 'update(Core::Engine& engine)')
    content = content.replace('update(float dt, Core::Engine* game', 'update(float dt, Core::Engine* engine')
    content = content.replace('tool_status_update(Core::Engine* game', 'tool_status_update(Core::Engine* engine')
    content = content.replace('buildMouseRay(GLFWwindow* window, Core::Engine* game', 'buildMouseRay(GLFWwindow* window, Core::Engine* engine')
    content = content.replace('use(GLFWwindow *window, ZoneManager &mgr, Zone &zone, Type type, Core::Engine &game)', 'use(GLFWwindow *window, ZoneManager &mgr, Zone &zone, Type type, Core::Engine &engine)')
    content = content.replace('use(GLFWwindow* window, ZoneManager& mgr, Zone& zone, Type type, Core::Engine& game)', 'use(GLFWwindow* window, ZoneManager& mgr, Zone& zone, Type type, Core::Engine& engine)')
    content = content.replace('Pottery3D(GLFWwindow* window, Core::Engine* game', 'Pottery3D(GLFWwindow* window, Core::Engine* engine')
    content = content.replace('Pottery3D(GLFWwindow *window, Core::Engine *game', 'Pottery3D(GLFWwindow *window, Core::Engine *engine')
    content = content.replace('Rotate3D(GLFWwindow* window, Core::Engine* game', 'Rotate3D(GLFWwindow* window, Core::Engine* engine')
    content = content.replace('Rotate3D(GLFWwindow *window, Core::Engine *game', 'Rotate3D(GLFWwindow *window, Core::Engine *engine')
    content = content.replace('FacePaint(GLFWwindow* window, Core::Engine* game', 'FacePaint(GLFWwindow* window, Core::Engine* engine')
    content = content.replace('FacePaint(GLFWwindow *window, Core::Engine *game', 'FacePaint(GLFWwindow *window, Core::Engine *engine')
    content = content.replace('FaceBrush(GLFWwindow* window, Core::Engine* game', 'FaceBrush(GLFWwindow* window, Core::Engine* engine')
    content = content.replace('FaceBrush(GLFWwindow *window, Core::Engine *game', 'FaceBrush(GLFWwindow *window, Core::Engine *engine')
    content = content.replace('Selection3D(GLFWwindow* window, Core::Engine* game', 'Selection3D(GLFWwindow* window, Core::Engine* engine')
    content = content.replace('Selection3D(GLFWwindow *window, Core::Engine *game', 'Selection3D(GLFWwindow *window, Core::Engine *engine')
    content = content.replace('PickObject3D(GLFWwindow* window, Core::Engine* game', 'PickObject3D(GLFWwindow* window, Core::Engine* engine')
    content = content.replace('PickObject3D(GLFWwindow *window, Core::Engine *game', 'PickObject3D(GLFWwindow *window, Core::Engine *engine')
    content = content.replace('pickObjectAtCursor3D(Core::Engine& game)', 'pickObjectAtCursor3D(Core::Engine& engine)')
    
    # game-> getCameraModelview() -> engine->getCamera()->getModelview()
    content = content.replace('engine->getCameraModelview()', 'engine->getCamera()->getModelview()')
    content = content.replace('engine->getCameraProjection()', 'engine->getCamera()->getProjection()')
    content = content.replace('engine->getCameraViewport()', 'engine->getCamera()->getViewport()')
    
    if orig != content:
        with open(filepath, 'w') as f:
            f.write(content)
        print("Updated", filepath)

for f in glob.glob('src/OurVerse/*.*'):
    process(f)
