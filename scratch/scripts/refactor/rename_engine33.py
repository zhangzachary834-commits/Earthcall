import os

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        content = f.read()
        
    content = content.replace("engine->", "engine.")
            
    with open(path, 'w') as f:
        f.write(content)
    print("Fixed Tool.cpp")
    
if __name__ == '__main__':
    fix_tool_cpp()
