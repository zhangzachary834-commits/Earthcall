import glob

def process(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    orig = content
    
    # Missing includes
    if 'getMouseHandler()' in content and 'MouseHandler.hpp' not in content:
        content = '#include "Singularity/Input/Mouse/MouseHandler.hpp"\n' + content
    if 'getCamera()' in content and 'Camera.hpp' not in content:
        content = '#include "Singularity/Screen/Camera.hpp"\n' + content
            
    content = content.replace('engine.getMouseHandler().', 'engine.getMouseHandler()->')
    content = content.replace('engine->getMouseHandler().', 'engine->getMouseHandler()->')
    
    content = content.replace('engine.isCursorLocked()', 'engine.getMouseHandler()->isCursorLocked()')
    content = content.replace('engine->isCursorLocked()', 'engine->getMouseHandler()->isCursorLocked()')
    
    # getAdvanced2DBrush might not exist in Engine. Wait, in original Game.hpp, was it getAdvanced2DBrush()?
    # Let me comment it out for now or just replace it with something if I can't find it.
    # actually let's see what getAdvanced2DBrush is. It might be in Ourverse.
    # I'll let it be for now and see if I need to move it to Ourverse.
    
    if orig != content:
        with open(filepath, 'w') as f:
            f.write(content)
        print("Updated", filepath)

for f in glob.glob('src/OurVerse/*.*') + glob.glob('src/Integration/*.*'):
    process(f)
