# Makefile for Earthcall C++ Project

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra \
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
          -lglfw \
          -framework OpenGL \
          -framework WebKit \
          -framework Cocoa \
          -framework CoreAudio \
          -framework AudioToolbox

# Directories
SRC_DIR = src
IMGUI_DIR = ../imgui
BUILD_DIR = build

# Source files
SOURCES = $(shell find $(SRC_DIR) -name "*.cpp" \! -name "main.cpp") \
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
	rm -rf $(BUILD_DIR) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

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

zone_being_test: $(TEST_OBJECTS) $(BUILD_DIR)/tests/zone_being_test.o
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

test-zone: zone_being_test
	./zone_being_test

test: test-property test-law test-loop test-recorder test-concept test-graph test-meta test-math test-continuous test-timeflow test-persist test-geomcache test-zone

# Pull in auto-generated header dependencies (.d files next to each .o)
-include $(OBJECTS:.o=.d)
-include $(wildcard $(BUILD_DIR)/tests/*.d)

# Phony targets
.PHONY: all clean run test test-property test-law test-loop test-recorder test-concept test-graph test-meta test-math test-continuous test-timeflow test-persist test-geomcache test-zone