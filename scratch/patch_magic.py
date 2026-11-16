with open("src/Singularity/Storage/SaveSystem.cpp", "r") as f:
    content = f.read()
    
# Change to little endian representation of 0x4543464D: 0x4D, 0x46, 0x43, 0x45
content = content.replace("bytes[0] == 0x45 && bytes[1] == 0x43 && bytes[2] == 0x46 && bytes[3] == 0x4D",
                          "bytes[0] == 0x4D && bytes[1] == 0x46 && bytes[2] == 0x43 && bytes[3] == 0x45")

with open("src/Singularity/Storage/SaveSystem.cpp", "w") as f:
    f.write(content)
