import re

with open("src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleWindow.cpp", "r") as f:
    text = f.read()

def replace_func(m):
    return """#include "Singularity/Core/Engine.hpp"
#include "Singularity/Core/CreationChannel.hpp"
#include "Singularity/Screen/Renderer.hpp"

namespace Rendering {

    void renderCreatorConsole3DPreviews(Person* player, Object* selected, Core::Engine* engine) {
        (void)player;
        (void)selected;
        auto& state = getCreatorConsoleState();
        
        Singularity::Core::CreationChannel* channel = nullptr;
        if (engine) {
            if (auto* lm = engine->getLawManager()) {
                channel = Singularity::Core::CreationChannel::find(*lm);
            }
        }
        
        if (channel && state.current3DMode == Mode3D::BrushCreate) {
            glm::mat4 t = channel->getCursorSpawnTransform();
            
            Object ghost;
            ghost.setShapeKind(static_cast<ObjectTypes::ShapeKind>(channel->activeShapeKind));
            ghost.setTransform(t);
            ghost.propSetColor(channel->activeColor);
            
            currentRenderer().setModel(t);
            currentRenderer().setWireframe(true);
            ghost.drawObject();
            currentRenderer().setWireframe(false);
            currentRenderer().setModel(glm::mat4(1.0f));
        }
    }

} // namespace Rendering"""

text = re.sub(r'void renderCreatorConsole3DPreviews\(Person\* player, Object\* selected\).*?\} // namespace Rendering', replace_func, text, flags=re.DOTALL)

with open("src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleWindow.cpp", "w") as f:
    f.write(text)
