import re
with open("CMakeLists.txt", "r") as f:
    text = f.read()

text = text.replace("""# FaceTexture test
add_executable(face_texture_test tests/constructed-being/face_texture_test.cpp)
target_include_directories(face_texture_test PRIVATE src third_party/nlohmann_json/single_include third_party/glm third_party/glfw/include third_party/gl3w/include third_party)
target_link_libraries(face_texture_test PRIVATE earthcall_core)
add_test(NAME face_texture_test COMMAND face_texture_test)""", "")

with open("CMakeLists.txt", "w") as f:
    f.write(text)
