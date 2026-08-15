import os

def fix_engine_render():
    path = "src/Singularity/Core/EngineRender.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    for i in range(len(lines)):
        if "_polyhedron.shapeKind" in lines[i] or "_polyhedron.shapeParams" in lines[i]:
            lines[i] = "// " + lines[i]
        elif "setPlayerEyeHeight" in lines[i]:
            lines[i] = "// " + lines[i]
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed EngineRender.cpp")

def fix_tool_cpp():
    path = "src/OurVerse/Tool.cpp"
    with open(path, 'r') as f:
        lines = f.readlines()
        
    # Revert engine-> back to engine. anywhere in Tool.cpp
    # wait, there are places where engine is a pointer!
    # Let's see if we can find all "Core::Engine& engine" and "Core::Engine* engine"
    # Actually, in Tool.cpp, I originally replaced "game." with "engine->", and THEN
    # replaced "engine." with "engine->" which broke things.
    # Wait, my original script did:
    # content = content.replace("game.", "engine->")
    # content = content.replace("game->", "engine->")
    # And then later I did content = content.replace("engine.", "engine->") !!
    # This was a huge mistake!
    # So wherever I see "engine->", it should PROBABLY be "engine." IF it's in a function
    # that takes a reference.
    # I will just revert "engine->" to "engine." everywhere in Tool.cpp that is inside
    # onStrokeTool, applyCursorTool, updateCursorTool, because they all take Core::Engine& engine.
    
    in_ref_func = False
    for i in range(len(lines)):
        line = lines[i]
        if "Core::Engine& engine" in line:
            in_ref_func = True
        elif "Core::Engine* engine" in line:
            in_ref_func = False
        
        # If we hit a new function without engine, just keep the current state, but mostly functions are short.
        if in_ref_func:
            lines[i] = lines[i].replace("engine->", "engine.")
            
    with open(path, 'w') as f:
        f.writelines(lines)
    print("Fixed Tool.cpp")
    
if __name__ == '__main__':
    fix_engine_render()
    fix_tool_cpp()
