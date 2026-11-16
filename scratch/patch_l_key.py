import re

with open("src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreationTools.cpp", "r") as f:
    text = f.read()

def replace_func(m):
    return """void handleCreateLawKey(GLFWwindow* window, Core::Engine& engine,
                        Singularity::Core::CreationChannel& channel) {
    if (!window) return;
    if (engine.getMainMenu().isOpen()) return;
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    static bool lawModeKeyDownLast = false;
    const bool lawModeKeyDown = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
    if (lawModeKeyDown && !lawModeKeyDownLast) {
        auto& state = getCreatorConsoleState();
        if (state.current3DMode == Mode3D::BrushCreate) {
            state.current3DMode = Mode3D::None;
        } else {
            state.current3DMode = Mode3D::BrushCreate;
        }
    }
    lawModeKeyDownLast = lawModeKeyDown;
}"""

text = re.sub(r'void handleCreateLawKey.*?\n}', replace_func, text, flags=re.DOTALL)

with open("src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreationTools.cpp", "w") as f:
    f.write(text)
