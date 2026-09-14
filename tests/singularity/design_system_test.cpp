#include <cassert>
#include <iostream>
#include <stdexcept>
#include "Singularity/FirstMoverOntology/Legacy/DesignSystem.hpp"

// We want to test that when exception is caught, BrushSystem's undo is called.

int main() {
    std::cout << "Testing DesignSystem undo exception handling..." << std::endl;

    DesignSystem ds;
    ds.initialize(nullptr);

    // Valid entry
    ds.addText("Hello", glm::vec2(0, 0));

    // Inject unescaped quotes which will definitely break JSON parse
    // editText internally does: saveHistoryEntry("edit_text", "{\"id\":\"" + id + "\",\"text\":\"" + newText + "\"}");
    // If we pass a string with an unescaped double quote, it creates invalid JSON.
    ds.editText("text_id_1", "\"invalid_json_format\"");

    bool undoCompleted = false;
    try {
        ds.undo(); // This should trigger the parse exception, catch it, and then call _brushSystem->undo()
        undoCompleted = true;
    } catch (...) {
        // Should not reach here
    }

    assert(undoCompleted && "Undo should handle exceptions without propagating them");
    std::cout << "Test passed: undo successfully handled invalid JSON exception." << std::endl;
    return 0;
}
