import os
import re

def fix_engine_hpp():
    path = "src/Singularity/Core/Engine.hpp"
    with open(path, 'r') as f:
        content = f.read()

    # Replace the int dummy vars with proper structs
    content = content.replace("int _brush = 0;", "struct DummyBrush { bool showCursor=false; bool cursorVisible=false; float previewSize=1.0f; }; DummyBrush _brush;")
    content = content.replace("int _faceBrush = 0;", "struct DummyFaceBrush { float radius=1.0f; bool soft=false; }; DummyFaceBrush _faceBrush;")
    
    # Remove Chat from Engine.hpp
    content = re.sub(r'class Chat;\n', '', content)
    content = re.sub(r'std::unique_ptr<Chat> _chat;\n', '', content)

    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.hpp")
    
def fix_engine_cpp():
    path = "src/Singularity/Core/Engine.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    if "OurVerse/ElementalToolHandler.hpp" not in content:
        content = '#include "OurVerse/ElementalToolHandler.hpp"\n' + content
        
    # Remove _chat usage
    content = content.replace('_chat = std::make_unique<Chat>();', '')
    
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Engine.cpp")

if __name__ == '__main__':
    fix_engine_hpp()
    fix_engine_cpp()
