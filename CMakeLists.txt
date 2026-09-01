cmake_minimum_required(VERSION 3.5)
if(APPLE)
    project(Earthcall VERSION 1.0 LANGUAGES CXX OBJCXX)
else()
    project(Earthcall VERSION 1.0 LANGUAGES CXX)
endif()

set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/local_deps" CACHE PATH "" FORCE)
set(CMAKE_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/local_deps/include" CACHE PATH "" FORCE)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/local_deps/include)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Emit build/compile_commands.json — the compilation database clangd reads to
# learn each translation unit's real include paths and flags. Without it clangd
# guesses, and on a tree with vendored OpenSSL, imgui, glm and wgpu headers it
# guesses badly: every include resolves to nothing and the whole file reports as
# errors, so symbol lookup silently falls back to text search.
#
# This matters beyond the editor. An agent that can ask "where is
# ObjectConcept::apply defined" gets one answer from the language server; without
# the database it greps, reads several candidate files, and pays for all of them.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set compile options
add_compile_options(-Wall -Wextra)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build" FORCE)
endif()

# We need OpenGL, OpenSSL, ZLIB
find_package(OpenGL)

if(NOT APPLE AND NOT EXISTS "/usr/include/GL/gl.h")
    set(OpenGL_FOUND FALSE CACHE BOOL "" FORCE)
endif()
find_library(UUID_LIB uuid)
find_package(Threads REQUIRED)

if (NOT EMSCRIPTEN)
    find_package(ZLIB REQUIRED)
    find_package(OpenSSL REQUIRED)
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

list(APPEND CMAKE_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/local_deps/include")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/src/s
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ZonesOfEarth
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Person/Body
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/local_deps/include
	${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniaudio
)

# Fetch standalone ASIO, WebSocketPP, and GLFW
include(FetchContent)

list(APPEND CMAKE_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/local_deps/include")

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RANDR_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/local_deps/include" CACHE PATH "" FORCE)
set(XINERAMA_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/local_deps/include" CACHE PATH "" FORCE)
set(XCURSOR_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/local_deps/include" CACHE PATH "" FORCE)
set(XI_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/local_deps/include" CACHE PATH "" FORCE)
if(NOT APPLE AND NOT EMSCRIPTEN)
    set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
endif()
if (NOT EMSCRIPTEN)
    if(APPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/local_deps/glfw_install/lib/libglfw3.a")
        add_library(glfw STATIC IMPORTED)
        set_target_properties(glfw PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/local_deps/glfw_install/lib/libglfw3.a")
        target_include_directories(glfw INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/local_deps/glfw_install/include")
        set(GLFW_LIB glfw)
    else()
        FetchContent_MakeAvailable(glfw)
        set(GLFW_LIB glfw)
    endif()

    include_directories(${glfw_SOURCE_DIR}/include)

endif()

FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY https://github.com/google/flatbuffers.git
    GIT_TAG v25.12.19
)
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(flatbuffers)
include_directories(${flatbuffers_SOURCE_DIR}/include)

if (NOT EMSCRIPTEN)
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
endif()

FetchContent_Declare(
    vhacd
    GIT_REPOSITORY https://github.com/kmammou/v-hacd.git
    GIT_TAG v4.1.0
)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(vhacd)

include_directories(
    ${vhacd_SOURCE_DIR}/include
)

add_definitions(-DGL_SILENCE_DEPRECATION)

# --- Core App Sources ---
file(GLOB_RECURSE SRC_FILES 
    "src/*.cpp"
)

# Exclude standalone main files and WebGPU from core sources
list(FILTER SRC_FILES EXCLUDE REGEX "src/main.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "src/Singularity/Storage/MigrateSaves.cpp")
# Standalone tools carry their own main(); they must not enter the core lib.
list(FILTER SRC_FILES EXCLUDE REGEX "src/Identity/MigrateIdentitiesTool.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "src/Singularity/Screen/WebGPU/.*")
list(FILTER SRC_FILES EXCLUDE REGEX "src/entry.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "smoke_.*")
list(FILTER SRC_FILES EXCLUDE REGEX "scratch/.*")
list(FILTER SRC_FILES EXCLUDE REGEX "src/Legacy Depricated/.*")
list(FILTER SRC_FILES EXCLUDE REGEX "src/Singularity/Foreign/py/.*")

set(IMGUI_DIR "imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)
if(NOT EMSCRIPTEN AND OpenGL_FOUND)
    list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp)
endif()

# Compile WebIntegration and RealWebView as Objective-C++ since they use macOS APIs on Apple
if(APPLE)
    set_source_files_properties(src/Singularity/Foreign/Web/RealWebView.cpp PROPERTIES LANGUAGE OBJCXX)
    set_source_files_properties(src/Singularity/Foreign/Web/WebIntegration.cpp PROPERTIES LANGUAGE OBJCXX)
endif()

# Object library for shared code
add_library(earthcall_core OBJECT ${SRC_FILES} ${IMGUI_SOURCES})
if(NOT EMSCRIPTEN AND OpenGL_FOUND)
    target_link_libraries(earthcall_core PUBLIC OpenGL::GL)
else()
    target_compile_definitions(earthcall_core PUBLIC NO_OPENGL_RENDERER GLFW_INCLUDE_NONE)
endif()
if (NOT EMSCRIPTEN)
    target_link_libraries(earthcall_core PUBLIC
        ${GLFW_LIB}
        ZLIB::ZLIB
        OpenSSL::SSL
        OpenSSL::Crypto
        ${CMAKE_DL_LIBS}
        Threads::Threads
    )
    if (NOT APPLE)
        # <uuid/uuid.h>'s symbols (uuid_generate/uuid_unparse, used by
        # Lexeme.cpp) ship in libSystem on Apple platforms, so UUID_LIB is
        # NOTFOUND there and must not be linked (or even referenced
        # unconditionally — CMake errors at generate time on a NOTFOUND
        # variable used in target_link_libraries). Elsewhere they need
        # util-linux's separate libuuid.
        if (NOT UUID_LIB)
            message(FATAL_ERROR "libuuid not found (required by Lexeme.cpp's uuid_generate on non-Apple platforms)")
        endif()
        target_link_libraries(earthcall_core PUBLIC ${UUID_LIB})
    endif()
endif()
if(APPLE AND NOT EMSCRIPTEN)
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

if(NOT EMSCRIPTEN)
    # --- Main Executable ---
    add_executable(earthcall src/entry.cpp)
    target_link_libraries(earthcall PRIVATE earthcall_core)

    # --- wgpu-native (vendored) ---
    set(WGPU_NATIVE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/wgpu)
    include_directories(${WGPU_NATIVE_DIR}/include)

    # --- WebGPU App ---
    set(WEBGPU_APP_SOURCES ${SRC_FILES})
    list(FILTER WEBGPU_APP_SOURCES EXCLUDE REGEX "src/Singularity/Screen/WebGPU/smoke_.*")

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
        src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp
        src/Singularity/Screen/WebGPU/SdfWgsl.cpp
        src/Singularity/Screen/WebGPU/GpuBufferPool.cpp
        src/Singularity/Screen/WebGPU/GpuMeshCache.cpp
    )
    if(APPLE)
        list(APPEND WEBGPU_APP_SOURCES src/Singularity/Screen/WebGPU/WebGpuContext.mm)
        set_source_files_properties(src/Singularity/Screen/WebGPU/WebGpuContext.mm PROPERTIES LANGUAGE OBJCXX)
    endif()

    add_executable(earthcall_webgpu src/entry.cpp ${WEBGPU_APP_SOURCES} ${IMGUI_WEBGPU_SOURCES})
    target_include_directories(earthcall_webgpu PRIVATE ${WGPU_NATIVE_DIR}/include)
    target_compile_definitions(earthcall_webgpu PRIVATE EARTHCALL_WEBGPU IMGUI_IMPL_WEBGPU_BACKEND_WGPU)

    if(APPLE)
        target_link_libraries(earthcall_webgpu PRIVATE
            $<$<BOOL:${OPENGL_FOUND}>:OpenGL::GL>
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
    add_executable(migrate_identities src/Identity/MigrateIdentitiesTool.cpp)
    target_link_libraries(migrate_identities PRIVATE earthcall_core)

    # --- Tests ---
    enable_testing()
    file(GLOB_RECURSE TEST_FILES "tests/*.cpp")

    # Tests written against an API that does not exist yet. They are KEPT as the
    # specification of a feature still to be built, and are still configured as
    # targets you can ask for by name — but they are excluded from the default
    # target, so `cmake --build build` is not red for work nobody has started.
    # ctest reports them "Not Run". Take a name off this list the moment the
    # feature it describes lands.
    #
    #   (none right now — webgpu_particle_test landed 2026-08-24: drawParticles
    #    added to WebGpuRenderer, see WebGpuRenderer.cpp)
    set(PENDING_FEATURE_TESTS "")
    set(ALL_ACTIVE_TEST_TARGETS "")

    foreach(TEST_SOURCE ${TEST_FILES})
        get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

        if (${TEST_NAME} STREQUAL "brush_system_test")
            add_executable(${TEST_NAME} ${TEST_SOURCE})
            target_include_directories(${TEST_NAME} PRIVATE ${GLFW_SOURCE_DIR}/include ${GLFW_SOURCE_DIR}/deps)
            target_compile_definitions(${TEST_NAME} PRIVATE GLFW_INCLUDE_NONE)
            target_link_libraries(${TEST_NAME} PRIVATE earthcall_core)
        elseif (${TEST_NAME} MATCHES "webgpu_.*" OR ${TEST_NAME} STREQUAL "gpu_mastery_test")
            set(WEBGPU_TEST_EXTRA_SRCS
                src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp
                src/Singularity/Screen/WebGPU/SdfWgsl.cpp
                src/Singularity/Screen/WebGPU/GpuBufferPool.cpp
                src/Singularity/Screen/WebGPU/GpuMeshCache.cpp)
            if(APPLE)
                list(APPEND WEBGPU_TEST_EXTRA_SRCS src/Singularity/Screen/WebGPU/WebGpuContext.mm)
                set_source_files_properties(src/Singularity/Screen/WebGPU/WebGpuContext.mm PROPERTIES LANGUAGE OBJCXX)
            endif()
            add_executable(${TEST_NAME} ${TEST_SOURCE} ${WEBGPU_TEST_EXTRA_SRCS})
            target_include_directories(${TEST_NAME} PRIVATE ${WGPU_NATIVE_DIR}/include)
            target_compile_definitions(${TEST_NAME} PRIVATE EARTHCALL_WEBGPU IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
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
                target_link_libraries(${TEST_NAME} PRIVATE earthcall_core ${GLFW_LIB} ${OPENSSL_LIBRARIES} ZLIB::ZLIB ${CMAKE_DL_LIBS} Threads::Threads)
            endif()
        else()
            add_executable(${TEST_NAME} ${TEST_SOURCE})
            # uuid (non-Apple only, and NOTFOUND-guarded) already comes in
            # transitively via earthcall_core's PUBLIC link libraries above.
            target_link_libraries(${TEST_NAME} PRIVATE earthcall_core ${GLFW_LIB} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} ZLIB::ZLIB ${CMAKE_DL_LIBS} Threads::Threads)
        endif()

        # Shared fixtures (test_save_helper.hpp). Source files live under
        # tests/<region>/; ctest target names stay NAME_WE of the .cpp.
        target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/tests/support)
        target_compile_options(${TEST_NAME} PRIVATE -UNDEBUG)
        if (${TEST_NAME} IN_LIST PENDING_FEATURE_TESTS)
            set_target_properties(${TEST_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE)
        else()
            list(APPEND ALL_ACTIVE_TEST_TARGETS ${TEST_NAME})
        endif()
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

        # ctest's default WORKING_DIRECTORY for an add_test() target is the
        # build tree, not the source tree. synthesis_studio_app_test opens an
        # authored save by a bare "saves/..." path (test_observation_load_test's
        # equivalent fixture is optional and skips when absent; this one is the
        # test's actual subject, so it must resolve), so it needs the source
        # root as its CWD like the `lag` custom target above already gets.
        if (${TEST_NAME} STREQUAL "synthesis_studio_app_test")
            set_tests_properties(${TEST_NAME} PROPERTIES
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
        endif()

        # The lag probe measures durations, so it must not be measured while
        # three other tests are fighting it for the machine: RUN_SERIAL makes
        # ctest give it the box to itself. (It survives contention either way
        # — it detects a moving machine and stops enforcing its timings — but
        # a run where nothing is enforced is a run that checked nothing.)
        if (${TEST_NAME} STREQUAL "frame_lag_test" OR
            ${TEST_NAME} STREQUAL "webgpu_micro_mastery_lag_test")
            set_tests_properties(${TEST_NAME} PROPERTIES
                RUN_SERIAL TRUE
                TIMEOUT 300
                LABELS "lag")
        endif()
    endforeach()

    # `cmake --build build --target lag` — the frame-cost probe on its own.
    # It is in the ordinary ctest run too, but it is the one test that takes
    # tens of seconds and prints a report worth reading, so it gets a name you
    # can ask for directly. Add --rebaseline by hand when re-recording
    # tests/singularity/frame_lag_baseline.txt.
    add_custom_target(lag
        COMMAND frame_lag_test
        DEPENDS frame_lag_test
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        USES_TERMINAL
        COMMENT "Measuring what a frame of Earthcall costs"
    )

    add_custom_target(test_all
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS ${ALL_ACTIVE_TEST_TARGETS}
        COMMENT "Building and running all active Earthcall test suites"
    )
    add_custom_target(check DEPENDS test_all)
endif()

# --- WebAssembly / Emscripten Build ---
if (EMSCRIPTEN)
    set(WASM_APP_SOURCES ${SRC_FILES})
    list(FILTER WASM_APP_SOURCES EXCLUDE REGEX "src/Singularity/Screen/WebGPU/smoke_.*")
    # Exclude macOS specific webviews
    list(FILTER WASM_APP_SOURCES EXCLUDE REGEX "src/Singularity/Foreign/Web/RealWebView.*")
    list(FILTER WASM_APP_SOURCES EXCLUDE REGEX "src/Singularity/Foreign/Web/WebIntegration.*")
    # Exclude Legacy OpenGL renderer but keep GluCompat
    list(FILTER WASM_APP_SOURCES EXCLUDE REGEX "src/Singularity/Screen/GL/OpenGLRenderer.cpp")
    # Also exclude native AudioToolbox/CoreAudio dependencies if needed

    # Add back WebGPU sources since they were excluded from SRC_FILES globally
    file(GLOB_RECURSE WEBGPU_SOURCES "src/Singularity/Screen/WebGPU/*.cpp")
    list(FILTER WEBGPU_SOURCES EXCLUDE REGEX "smoke_.*")
    list(APPEND WASM_APP_SOURCES ${WEBGPU_SOURCES})

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
    target_compile_definitions(earthcall_wasm PRIVATE EARTHCALL_WEBGPU IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
    target_compile_options(earthcall_wasm PRIVATE "--use-port=emdawnwebgpu" "-sUSE_GLFW=3" "-sUSE_ZLIB=1")
    
    # Emscripten linker flags for WebGPU, asyncify, glfw, and embind
    set_target_properties(earthcall_wasm PROPERTIES LINK_FLAGS
        "--use-port=emdawnwebgpu -s ASYNCIFY=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s USE_GLFW=3 -s USE_ZLIB=1 --bind -O3 -s ASSERTIONS=1"
    )
    # Output name
    set_target_properties(earthcall_wasm PROPERTIES OUTPUT_NAME "earthcall")
    set_target_properties(earthcall_wasm PROPERTIES SUFFIX ".js")
endif()
