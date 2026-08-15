with open("CMakeLists.txt", "r") as f:
    content = f.read()

content = content.replace("src/Rendering/", "src/Singularity/Screen/")
content = content.replace("src/Integration/", "src/Singularity/Foreign/")
content = content.replace("src/Perspective/", "src/Person/Perspective/")
content = content.replace("src/Util/", "src/Singularity/Storage/")

with open("CMakeLists.txt", "w") as f:
    f.write(content)
