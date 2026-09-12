clang++ -std=c++17 -O3 -g -Isrc -Ithird_party/nlohmann_json/include \
    -DGLFW_INCLUDE_NONE -DGLM_ENABLE_EXPERIMENTAL \
    -Ilocal_deps/openssl-3.0.13/include \
    -c scratch/test_laws.cpp -o build/test_laws.o
# Link using existing build artifacts
clang++ -O3 -g build/test_laws.o build/CMakeFiles/earthcall_core.dir/src/**/*.cpp.o \
    -Llocal_deps/openssl-3.0.13 -lssl -lcrypto -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo \
    -o build/test_laws
