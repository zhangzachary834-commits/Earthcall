cmake_minimum_required(VERSION 3.15)
project(Earthcall VERSION 1.0 LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set compile options
add_compile_options(-Wall -Wextra)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif()

# We need OpenGL, GLFW, OpenSSL, ZLIB
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
# glfw3 is sometimes named glfw3, sometimes glfw
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
  # fallback using pkg-config if needed, or simple link
  find_library(GLFW_LIB NAMES glfw3 glfw HINTS ${CMAKE_CURRENT_SOURCE_DIR}/local_deps/glfw_install/lib)
else()
  set(GLFW_LIB glfw)
endif()

# macOS specific frameworks
if(APPLE)
    find_library(COCOA_FW Cocoa)
    find_library(WEBKIT_FW WebKit)
    find_library(COREAUDIO_FW CoreAudio)
    find_library(AUDIOTOOLBOX_FW AudioToolbox)
    find_library(SECURITY_FW Security)
    find_library(METAL_FW Metal)
    find_library(QUARTZCORE_FW QuartzCore)
    find_library(COREFOUNDATION_FW CoreFoundation)
    find_library(IOKIT_FW IOKit)
    find_library(FOUNDATION_FW Foundation)
    find_library(APPKIT_FW AppKit)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Form
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ZonesOfEarth
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Person/Body
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/local_deps/include
    ${CMAKE_CURRENT_SOURCE_DIR}/local_deps/glfw_install/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/flatbuffers/include
)

# Fetch standalone ASIO and WebSocketPP for native networking
include(FetchContent)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-28-0
)
FetchContent_MakeAvailable(asio)

FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
    GIT_TAG 0.8.2
)
FetchContent_MakeAvailable(websocketpp)

include_directories(
    ${asio_SOURCE_DIR}/asio/include
    ${websocketpp_SOURCE_DIR}
)
add_definitions(-DASIO_STANDALONE -D_WEBSOCKETPP_CPP11_STL_)

add_definitions(-DGL_SILENCE_DEPRECATION)

# --- Core App Sources ---
file(GLOB_RECURSE SRC_FILES 
    "src/*.cpp"
)

# Exclude standalone main files and WebGPU from core sources
list(FILTER SRC_FILES EXCLUDE REGEX "src/main.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "src/Util/MigrateSaves.cpp")
# Standalone tools carry their own main(); they must not enter the core lib.
list(FILTER SRC_FILES EXCLUDE REGEX "src/Identity/MigrateIdentitiesTool.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "src/Rendering/WebGPU/.*")
list(FILTER SRC_FILES EXCLUDE REGEX "src/entry.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "smoke_.*")
list(FILTER SRC_FILES EXCLUDE REGEX "scratch/.*")
list(FILTER SRC_FILES EXCLUDE REGEX "src/Legacy Depricated/.*")

set(IMGUI_DIR "imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp
)

# Compile WebIntegration and RealWebView as Objective-C++ since they use macOS APIs
set_source_files_properties(src/Integration/RealWebView.cpp PROPERTIES LANGUAGE OBJCXX)
set_source_files_properties(src/Integration/WebIntegration.cpp PROPERTIES LANGUAGE OBJCXX)

# Object library for shared code
add_library(earthcall_core OBJECT ${SRC_FILES} ${IMGUI_SOURCES})
target_link_libraries(earthcall_core PUBLIC 
    OpenGL::GL
    ZLIB::ZLIB
    OpenSSL::SSL
    OpenSSL::Crypto
)
if(APPLE)
    target_link_libraries(earthcall_core PUBLIC 
        ${GLFW_LIB}
        ${COCOA_FW}
        ${WEBKIT_FW}
        ${COREAUDIO_FW}
        ${AUDIOTOOLBOX_FW}
        ${SECURITY_FW}
        ${IOKIT_FW}
        ${COREFOUNDATION_FW}
        ${FOUNDATION_FW}
    )
endif()

# --- Main Executable ---
add_executable(earthcall src/entry.cpp)
target_link_libraries(earthcall PRIVATE earthcall_core)

# --- wgpu-native (vendored) ---
# third_party/wgpu bundles a matching webgpu.h (new-style WGPUStringView /
# CallbackInfo API, v29.0.1.1) plus a prebuilt libwgpu_native.a for macOS.
# WebGpuRenderer.cpp and friends are written against this API generation, so
# native and Emscripten builds must share this exact header rather than an
# independently-fetched wgpu-native release (older releases ship an
# incompatible pre-StringView ABI and will crash at runtime).
set(WGPU_NATIVE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/wgpu)
include_directories(${WGPU_NATIVE_DIR}/include)

# --- WebGPU App ---
# Exclude OpenGL specific imgui backend and include WebGPU ones
set(WEBGPU_APP_SOURCES ${SRC_FILES})
list(FILTER WEBGPU_APP_SOURCES EXCLUDE REGEX "src/Rendering/WebGPU/smoke_.*")

set(IMGUI_WEBGPU_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
)
# Add WebGPU specific sources
list(APPEND WEBGPU_APP_SOURCES 
    src/Rendering/WebGPU/WebGpuContext.mm
    src/Rendering/WebGPU/WebGpuRenderer.cpp
    src/Rendering/WebGPU/SdfWgsl.cpp
)

add_executable(earthcall_webgpu src/entry.cpp ${WEBGPU_APP_SOURCES} ${IMGUI_WEBGPU_SOURCES})
target_include_directories(earthcall_webgpu PRIVATE ${WGPU_NATIVE_DIR}/include)
target_compile_definitions(earthcall_webgpu PRIVATE EARTHCALL_WEBGPU IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
set_source_files_properties(src/Rendering/WebGPU/WebGpuContext.mm PROPERTIES LANGUAGE OBJCXX)

if(APPLE)
    target_link_libraries(earthcall_webgpu PRIVATE
        OpenGL::GL
        ${WGPU_NATIVE_DIR}/lib/libwgpu_native.a
        ${GLFW_LIB}
        ${COCOA_FW}
        ${WEBKIT_FW}
        ${COREAUDIO_FW}
        ${AUDIOTOOLBOX_FW}
        ${SECURITY_FW}
        ${METAL_FW}
        ${QUARTZCORE_FW}
        ${COREFOUNDATION_FW}
        ${IOKIT_FW}
        ${FOUNDATION_FW}
        ${APPKIT_FW}
        ZLIB::ZLIB
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

# --- Tools ---
# Identity migration is an explicit, one-time act (see PersonMigration.hpp,
# TRUST BOUNDARY), so it ships as a tool a Person runs rather than a step
# that fires on load.
add_executable(migrate_identities src/Identity/MigrateIdentitiesTool.cpp)
target_link_libraries(migrate_identities PRIVATE earthcall_core)

# --- Tests ---
enable_testing()
file(GLOB_RECURSE TEST_FILES "tests/*.cpp")

foreach(TEST_SOURCE ${TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    # Skip WebGPU test on macOS if it's too complex to link for now, or link properly
    if (${TEST_NAME} MATCHES "webgpu_.*")
        add_executable(${TEST_NAME} ${TEST_SOURCE} src/Rendering/WebGPU/WebGpuRenderer.cpp src/Rendering/WebGPU/SdfWgsl.cpp src/Rendering/WebGPU/WebGpuContext.mm)
        target_include_directories(${TEST_NAME} PRIVATE ${WGPU_NATIVE_DIR}/include)
        target_compile_definitions(${TEST_NAME} PRIVATE EARTHCALL_WEBGPU IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
        set_source_files_properties(src/Rendering/WebGPU/WebGpuContext.mm PROPERTIES LANGUAGE OBJCXX)
        if(APPLE)
            target_link_libraries(${TEST_NAME} PRIVATE
                earthcall_core
                ${WGPU_NATIVE_DIR}/lib/libwgpu_native.a
                ${GLFW_LIB}
                ${COCOA_FW}
                ${WEBKIT_FW}
                ${COREAUDIO_FW}
                ${AUDIOTOOLBOX_FW}
                ${SECURITY_FW}
                ${METAL_FW}
                ${QUARTZCORE_FW}
                ${COREFOUNDATION_FW}
                ${IOKIT_FW}
                ${FOUNDATION_FW}
                ${APPKIT_FW}
                ZLIB::ZLIB
                OpenSSL::SSL
                OpenSSL::Crypto
                OpenGL::GL
            )
        else()
            target_link_libraries(${TEST_NAME} PRIVATE earthcall_core)
        endif()
    else()
        add_executable(${TEST_NAME} ${TEST_SOURCE})
        target_link_libraries(${TEST_NAME} PRIVATE earthcall_core)
    endif()

    # The suite checks via assert(). A Release build passes -DNDEBUG, which
    # compiles every one of them out and leaves tests that pass unconditionally
    # -- see the same note in Makefile.legacy. Undefine it for test targets so
    # a failing assertion actually fails the test.
    target_compile_options(${TEST_NAME} PRIVATE -UNDEBUG)

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# --- WebAssembly / Emscripten Build ---
if (EMSCRIPTEN)
    set(WASM_APP_SOURCES ${SRC_FILES})
    list(FILTER WASM_APP_SOURCES EXCLUDE REGEX "src/Rendering/WebGPU/smoke_.*")
    # Exclude macOS specific webviews
    list(FILTER WASM_APP_SOURCES EXCLUDE REGEX "src/Integration/RealWebView.*")
    list(FILTER WASM_APP_SOURCES EXCLUDE REGEX "src/Integration/WebIntegration.*")
    # Also exclude native AudioToolbox/CoreAudio dependencies if needed

    set(IMGUI_WASM_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp
    )

    add_executable(earthcall_wasm src/entry.cpp ${WASM_APP_SOURCES} ${IMGUI_WASM_SOURCES})
    target_compile_definitions(earthcall_wasm PRIVATE EARTHCALL_WEBGPU IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
    
    # Emscripten linker flags for WebGPU, asyncify, glfw, and embind
    set_target_properties(earthcall_wasm PROPERTIES LINK_FLAGS
        "-s USE_WEBGPU=1 -s ASYNCIFY=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s USE_GLFW=3 --bind -O3"
    )
    # Output name
    set_target_properties(earthcall_wasm PROPERTIES OUTPUT_NAME "earthcall")
    set_target_properties(earthcall_wasm PROPERTIES SUFFIX ".js")
endif()
