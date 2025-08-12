# Makefile for Earthcall C++ Project

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra \
           -I./src \
           -I./src/Form \
           -I./src/ZonesOfEarth \
           -I./src/Person/Body \
           -I./imgui \
           -I/opt/homebrew/Cellar/glfw/3.4/include \
           -I/opt/homebrew/include \
           -DGL_SILENCE_DEPRECATION
LDFLAGS = -L/opt/homebrew/Cellar/lib \
          -L/opt/homebrew/lib \
          -lglfw \
          -framework OpenGL \
          -framework WebKit \
          -framework Cocoa

# Directories
SRC_DIR = src
IMGUI_DIR = imgui
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

# Compile source files
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile Objective-C++ files (for WebKit integration)
$(BUILD_DIR)/src/Integration/RealWebView.o: src/Integration/RealWebView.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -ObjC++ -c $< -o $@

# Compile WebIntegration with Objective-C++ (it includes RealWebView.hpp)
$(BUILD_DIR)/src/Integration/WebIntegration.o: src/Integration/WebIntegration.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -ObjC++ -c $< -o $@

# Link the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Phony targets
.PHONY: all clean run 