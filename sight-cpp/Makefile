# Makefile for Earthcall C++ Project

# Compiler and flags
CXX = g++

# Build type. `make` builds optimized; `make BUILD=debug` builds for stepping
# through in a debugger. IMPORTANT: switching between them requires `make clean`
# first — make only tracks file timestamps, NOT which flags an .o was built with,
# so it will happily keep stale -O0 objects when you ask for a release build.
#
#   -O2  turns on inlining, register allocation, and vectorization. On the tight
#        numeric loops in the geometry kernels this is ~35x (measured: a sphere
#        tessellation went from ~590us to ~17us per call). Off by default is what
#        every no-flag build gets, and it was silently costing that 35x everywhere.
#   -g   emits debug symbols. Zero runtime cost — only a bigger binary — so we keep
#        it in release too, which makes any crash produce a readable backtrace.
#   NOTE: we intentionally do NOT pass -DNDEBUG. That would strip every assert(),
#        and the test suite is built with these same flags and checks via assert().
BUILD ?= release
ifeq ($(BUILD),debug)
    OPTFLAGS = -O0 -g
else
    OPTFLAGS = -O2 -g
endif

CXXFLAGS = -std=c++17 -Wall -Wextra $(OPTFLAGS) \
           -I./src \
           -I./src/Form \
           -I./src/ZonesOfEarth \
           -I./src/Person/Body \
           -I../imgui \
           -I/opt/homebrew/Cellar/glfw/3.4/include \
           -I/opt/homebrew/include \
           -DGL_SILENCE_DEPRECATION
LDFLAGS = -L/opt/homebrew/Cellar/lib \
          -L/opt/homebrew/lib \
          -L/opt/homebrew/opt/openssl/lib \
          -lglfw \
          -framework OpenGL \
          -framework WebKit \
          -framework Cocoa \
          -framework CoreAudio \
          -framework AudioToolbox \
          -framework Security \
          -lz -lssl -lcrypto

# Directories
SRC_DIR = src
IMGUI_DIR = ../imgui
BUILD_DIR = build

# --- WebGPU (Milestone 5) — opt-in; wgpu-native vendored in third_party/wgpu. ---
# The macOS frameworks are what wgpu-native's Metal backend needs; the static .a
# keeps the binary self-contained (the .dylib is gitignored). Not linked into the
# default app yet — the WebGPU sources live under src/Rendering/WebGPU and are
# excluded from SOURCES below until the WebGpuRenderer is wired in.
WGPU_DIR = third_party/wgpu
WGPU_INC = -I$(WGPU_DIR)/include
WGPU_LIB = $(WGPU_DIR)/lib/libwgpu_native.a
WGPU_FRAMEWORKS = -framework Metal -framework QuartzCore -framework CoreFoundation \
                  -framework IOKit -framework Foundation -framework AppKit -framework Security

# Source files. WebGPU sources are excluded here (opt-in via the webgpu-* targets)
# so the default OpenGL build never needs the wgpu library.
# MigrateSaves.cpp is a standalone utility with its own main() (see the
# migrate-saves target below); linking it into the app is a duplicate _main.
SOURCES = $(shell find $(SRC_DIR) -name "*.cpp" \! -name "main.cpp" \! -name "MigrateSaves.cpp" \! -path "*/WebGPU/*") \
          $(IMGUI_DIR)/imgui.cpp \
          $(IMGUI_DIR)/imgui_demo.cpp \
          $(IMGUI_DIR)/imgui_draw.cpp \
          $(IMGUI_DIR)/imgui_tables.cpp \
          $(IMGUI_DIR)/imgui_widgets.cpp \
          $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
          $(IMGUI_DIR)/backends/imgui_impl_opengl2.cpp

# Object files
OBJECTS = $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)

# Target executable
TARGET = earthcall

# Default target
all: $(TARGET)

# Create build directory structure
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/$(SRC_DIR)
	mkdir -p $(BUILD_DIR)/$(IMGUI_DIR)

# Header-dependency tracking: -MMD writes a .d file next to each .o listing the
# headers it included, -MP adds phony targets so renamed/removed headers don't
# break the build. Without this, editing a header (e.g. Game.hpp) would NOT
# recompile the .cpp files that include it, producing ODR / stale-layout bugs.
DEPFLAGS = -MMD -MP

# Compile source files
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Compile Objective-C++ files (for WebKit integration)
$(BUILD_DIR)/src/Integration/RealWebView.o: src/Integration/RealWebView.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -ObjC++ -c $< -o $@

# Compile WebIntegration with Objective-C++ (it includes RealWebView.hpp)
$(BUILD_DIR)/src/Integration/WebIntegration.o: src/Integration/WebIntegration.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -ObjC++ -c $< -o $@

# Link the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(WEBGPU_BUILD_DIR) $(WEBGPU_TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)


# --- The same app, built on WebGPU -------------------------------------------
# A SEPARATE binary rather than a rebuilt one, for two reasons: libwgpu_native.a
# is 33 MB, so linking it into the default build would bloat every compile; and
# the OpenGL binary is the known-good reference this migration is measured
# against, so it must stay reachable without a rebuild. Run ./earthcall and
# ./earthcall_webgpu side by side to compare.
#
# Objects go in their own directory: make tracks timestamps, not flags, so
# sharing build/ with the OpenGL build would silently mix -DEARTHCALL_WEBGPU and
# non-WebGPU objects — exactly the trap the BUILD=debug note at the top warns of.
WEBGPU_BUILD_DIR = build-webgpu
WEBGPU_TARGET    = earthcall_webgpu

# Same sources as the app, plus the WebGPU backend, minus the standalone smoke
# programs (each has its own main()).
WEBGPU_APP_SOURCES = $(shell find $(SRC_DIR) -name "*.cpp" \! -name "main.cpp" \! -name "MigrateSaves.cpp" \! -name "smoke_*.cpp") \
          $(IMGUI_DIR)/imgui.cpp \
          $(IMGUI_DIR)/imgui_demo.cpp \
          $(IMGUI_DIR)/imgui_draw.cpp \
          $(IMGUI_DIR)/imgui_tables.cpp \
          $(IMGUI_DIR)/imgui_widgets.cpp \
          $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
          $(IMGUI_DIR)/backends/imgui_impl_wgpu.cpp

WEBGPU_APP_OBJECTS = $(WEBGPU_APP_SOURCES:%.cpp=$(WEBGPU_BUILD_DIR)/%.o) \
                     $(WEBGPU_BUILD_DIR)/src/Rendering/WebGPU/WebGpuContext.o

# IMGUI_IMPL_WEBGPU_BACKEND_WGPU picks imgui's wgpu-native path (vs Dawn).
WEBGPU_CXXFLAGS = $(CXXFLAGS) $(WGPU_INC) -DEARTHCALL_WEBGPU -DIMGUI_IMPL_WEBGPU_BACKEND_WGPU

$(WEBGPU_BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(WEBGPU_CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Objective-C++ units: the Cocoa/CAMetalLayer surface, and the WebKit integration
# the app already had.
$(WEBGPU_BUILD_DIR)/src/Rendering/WebGPU/WebGpuContext.o: src/Rendering/WebGPU/WebGpuContext.mm
	@mkdir -p $(dir $@)
	$(CXX) $(WEBGPU_CXXFLAGS) $(DEPFLAGS) -ObjC++ -c $< -o $@

$(WEBGPU_BUILD_DIR)/src/Integration/RealWebView.o: src/Integration/RealWebView.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(WEBGPU_CXXFLAGS) $(DEPFLAGS) -ObjC++ -c $< -o $@

$(WEBGPU_BUILD_DIR)/src/Integration/WebIntegration.o: src/Integration/WebIntegration.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(WEBGPU_CXXFLAGS) $(DEPFLAGS) -ObjC++ -c $< -o $@

webgpu-app: $(WEBGPU_TARGET)

$(WEBGPU_TARGET): $(WEBGPU_APP_OBJECTS)
	$(CXX) $(WEBGPU_APP_OBJECTS) $(WGPU_LIB) $(LDFLAGS) $(WGPU_FRAMEWORKS) -o $@
	@echo "Built ./$(WEBGPU_TARGET) — the app on WebGPU. ./earthcall is still OpenGL."

run-webgpu: $(WEBGPU_TARGET)
	./$(WEBGPU_TARGET)

# Headless WebGPU test over the REAL Object draw path (not a hand-fed fixture).
# Links the app's objects, minus the entry point, against the WebGPU backend.
WEBGPU_TEST_OBJECTS = $(filter-out $(WEBGPU_BUILD_DIR)/$(SRC_DIR)/entry.o,$(WEBGPU_APP_OBJECTS))

$(WEBGPU_BUILD_DIR)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(WEBGPU_CXXFLAGS) $(DEPFLAGS) -c $< -o $@

webgpu_object_test: $(WEBGPU_TEST_OBJECTS) $(WEBGPU_BUILD_DIR)/tests/webgpu_object_test.o
	$(CXX) $^ $(WGPU_LIB) $(LDFLAGS) $(WGPU_FRAMEWORKS) -o $@

test-webgpu-object: webgpu_object_test
	./webgpu_object_test

# --- WebGPU verification (Milestone 5) --------------------------------------
# Standalone: brings up a wgpu-native device, clears an offscreen frame, and reads
# the pixels back. Proves the vendored library renders on this machine. Not part
# of the app; opt-in so the default build stays OpenGL-only.
webgpu-smoke:
	$(CXX) -std=c++17 $(WGPU_INC) src/Rendering/WebGPU/smoke_offscreen.cpp \
	  $(WGPU_LIB) $(WGPU_FRAMEWORKS) -o webgpu_smoke
	./webgpu_smoke

# Mesh pipeline: a WGSL shader + vertex buffer + a uniform (the material colour)
# render a triangle offscreen; the read-back pixel must equal the uniform. Proves
# the substance of drawMesh before wiring the on-screen surface.
webgpu-mesh:
	$(CXX) -std=c++17 $(WGPU_INC) src/Rendering/WebGPU/smoke_mesh.cpp \
	  $(WGPU_LIB) $(WGPU_FRAMEWORKS) -o webgpu_mesh
	./webgpu_mesh

# The real WebGpuRenderer class, driven the way the app will (setCamera/beginFrame/
# setModel/drawMesh/endFrame) rendering a lit TessMesh offscreen, verified by readback.
webgpu-renderer:
	$(CXX) -std=c++17 -I./src -I./src/Form $(WGPU_INC) -I/opt/homebrew/include \
	  src/Rendering/WebGPU/WebGpuRenderer.cpp src/Rendering/DrawAdapters.cpp \
	  src/Rendering/WebGPU/SdfWgsl.cpp src/Form/Object/Geometry/Sdf.cpp \
	  src/Rendering/WebGPU/smoke_renderer.cpp \
	  $(WGPU_LIB) $(WGPU_FRAMEWORKS) -o webgpu_renderer
	./webgpu_renderer

# ON-SCREEN demo (needs a display — run on your Mac): opens a window with a
# CAMetalLayer surface and renders a spinning lit cube through WebGpuRenderer.
# The .mm is Objective-C++ (CAMetalLayer + Cocoa). Builds it but does NOT auto-run
# it (a window can't open headless); run ./webgpu_window yourself.
webgpu-window:
	$(CXX) -std=c++17 -I./src -I./src/Form $(WGPU_INC) -I/opt/homebrew/include \
	  -I/opt/homebrew/Cellar/glfw/3.4/include \
	  src/Rendering/WebGPU/WebGpuRenderer.cpp src/Rendering/DrawAdapters.cpp \
	  src/Rendering/WebGPU/SdfWgsl.cpp src/Form/Object/Geometry/Sdf.cpp \
	  src/Rendering/WebGPU/smoke_window.mm \
	  $(WGPU_LIB) $(WGPU_FRAMEWORKS) -L/opt/homebrew/lib -lglfw -o webgpu_window
	@echo "Built ./webgpu_window — run it on your Mac to see the window."

# ---------------------------------------------------------------
# Tests: link everything except the app entry point.
TEST_OBJECTS = $(filter-out $(BUILD_DIR)/$(SRC_DIR)/entry.o,$(OBJECTS))

$(BUILD_DIR)/tests/%.o: tests/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

property_bridge_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/property_bridge_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

law_model_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/law_model_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

law_loop_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/law_loop_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

change_recorder_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/change_recorder_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

object_concept_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/object_concept_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

law_graph_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/law_graph_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

metalaw_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/metalaw_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

ontomath_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/ontomath_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

continuous_law_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/continuous_law_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

time_flow_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/time_flow_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

law_persistence_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/law_persistence_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

geometry_cache_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/geometry_cache_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

glu_compat_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/glu_compat_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

zone_being_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/zone_being_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

formation_merge_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/formation_merge_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

material_being_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/material_being_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

material_render_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/material_render_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

primitive_render_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/primitive_render_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

action_spawn_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/action_spawn_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

serialization_compat_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/serialization_compat_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

law_creation_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/law_creation_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

test-property: property_bridge_test
	./property_bridge_test

test-law: law_model_test
	./law_model_test

test-loop: law_loop_test
	./law_loop_test

test-recorder: change_recorder_test
	./change_recorder_test

test-concept: object_concept_test
	./object_concept_test

test-graph: law_graph_test
	./law_graph_test

test-meta: metalaw_test
	./metalaw_test

test-math: ontomath_test
	./ontomath_test

test-continuous: continuous_law_test
	./continuous_law_test

test-timeflow: time_flow_test
	./time_flow_test

test-persist: law_persistence_test
	./law_persistence_test

test-geomcache: geometry_cache_test
	./geometry_cache_test

test-glucompat: glu_compat_test
	./glu_compat_test

test-zone: zone_being_test
	./zone_being_test

test-creation: law_creation_test
	./law_creation_test

test-material: material_being_test
	./material_being_test

test-material-render: material_render_test
	./material_render_test

test-primitive-render: primitive_render_test
	./primitive_render_test

test-action-spawn: action_spawn_test
	./action_spawn_test

binary_packing_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/binary_packing_test.o
	$(CXX) $^ $(LDFLAGS) -o $@

migrate-saves: $(BUILD_DIR)/src/Util/SaveSystem.o $(BUILD_DIR)/src/Util/CloudStorage.o $(BUILD_DIR)/src/Util/MigrateSaves.o
	$(CXX) $^ $(LDFLAGS) -o migrate_saves

read-laws: $(BUILD_DIR)/src/Util/SaveSystem.o $(BUILD_DIR)/src/Util/CloudStorage.o scratch/read_laws.cpp
	$(CXX) scratch/read_laws.cpp $(BUILD_DIR)/src/Util/SaveSystem.o $(BUILD_DIR)/src/Util/CloudStorage.o -I./src -I./src/Form -I./src/ZonesOfEarth -I./src/Person/Body -I../imgui $(LDFLAGS) -o read_laws

test-serialization: serialization_compat_test
	./serialization_compat_test

test-binary-packing: binary_packing_test
	./binary_packing_test

test: test-property test-law test-creation test-loop test-recorder test-concept test-graph test-meta test-math test-continuous test-timeflow test-persist test-geomcache test-glucompat test-zone test-material test-material-render test-primitive-render test-action-spawn test-serialization test-binary-packing

# Pull in auto-generated header dependencies (.d files next to each .o)
-include $(OBJECTS:.o=.d)
-include $(wildcard $(BUILD_DIR)/tests/*.d)
# The WebGPU build needs these just as much. Without them, editing a header does
# NOT recompile the objects that include it, so a binary can be linked from units
# compiled against DIFFERENT versions of the same class — mismatched member
# offsets, reads landing at the wrong address, and crashes that vanish on a clean
# rebuild. That is not hypothetical: it is what made ./earthcall_webgpu segfault
# inside WebGpuRenderer::uploadTexture.
-include $(WEBGPU_APP_OBJECTS:.o=.d)
-include $(wildcard $(WEBGPU_BUILD_DIR)/tests/*.d)

# Phony targets
.PHONY: all clean run webgpu-smoke webgpu-mesh webgpu-renderer webgpu-window test test-property test-law test-loop test-recorder test-concept test-graph test-meta test-math test-continuous test-timeflow test-persist test-geomcache test-glucompat test-zone test-material test-material-render test-primitive-render