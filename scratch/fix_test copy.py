with open("src/test_save_edge.cpp", "r") as f:
    content = f.read()
content = content.replace('readSaveData', 'SaveSystem::readSaveData')
with open("src/test_save_edge.cpp", "w") as f:
    f.write(content)
