import os
import glob

def process_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    original = content
    # Type replacements
    content = content.replace('Core::Game*', 'Core::Engine*')
    content = content.replace('Core::Game&', 'Core::Engine&')
    content = content.replace('Core::Game ', 'Core::Engine ')
    content = content.replace('Core::Game::PotteryTool', 'Core::PotteryTool')
    content = content.replace('Core::Game::RotationAxisMode', 'Core::RotationAxisMode')
    content = content.replace('Core::Game::PublicBrushType', 'Core::PublicBrushType')
    
    # Variable rename (only safe if it's not a common word, but game is used as a parameter)
    # We will do a simple text replace of 'game->' and 'game.' but we must be careful.
    content = content.replace('game->', 'engine->')
    content = content.replace('game.', 'engine.')
    
    # specific method names that were mapped
    content = content.replace('engine->getMouseLeftPressedLast()', 'engine->getMouseHandler().isLeftPressedLast()')
    content = content.replace('engine.getMouseLeftPressedLast()', 'engine.getMouseHandler().isLeftPressedLast()')
    
    content = content.replace('engine->getCursorX()', 'engine->getMouseHandler().getCursorX()')
    content = content.replace('engine.getCursorX()', 'engine.getMouseHandler().getCursorX()')
    content = content.replace('engine->getCursorY()', 'engine->getMouseHandler().getCursorY()')
    content = content.replace('engine.getCursorY()', 'engine.getMouseHandler().getCursorY()')
    
    # For Ourverse variables in Tool.cpp, they need to access the world:
    # engine->getRotationAxisMode() => mgr.active().world().getRotationAxisMode() 
    # Let's replace 'engine->' to 'mgr.active().world().' for those Ourverse UI accessors if mgr is available.
    # Actually wait, I added these UI accessors to Engine in previous script? No, they are in Ourverse.
    # Let's check if Tool.cpp has access to `mgr`. Yes, most Tool methods take `ZoneManager &mgr` and `Core::Engine* engine`.
    
    if content != original:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Updated {filepath}")

files = glob.glob('src/OurVerse/*.cpp') + glob.glob('src/OurVerse/*.hpp')
files += glob.glob('src/Integration/*.cpp') + glob.glob('src/Integration/*.hpp')
for f in files:
    process_file(f)

