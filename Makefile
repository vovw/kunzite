# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Iinclude -Ilib/imgui -Ilib/imgui/backends $(shell pkg-config --cflags glfw3)
LDFLAGS = $(shell pkg-config --static --libs glfw3) -framework Cocoa -framework OpenGL -framework IOKit

# Add curl and nlohmann/json
CXXFLAGS += -I/opt/homebrew/opt/curl/include -I/opt/homebrew/opt/nlohmann-json/include
LDFLAGS += -L/opt/homebrew/opt/curl/lib -lcurl

# Source files
SRC_DIR = src
IMGUI_DIR = lib/imgui
IMGUI_BACKENDS_DIR = lib/imgui/backends
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
IMGUI_FILES = $(wildcard $(IMGUI_DIR)/*.cpp)
IMGUI_BACKEND_FILES = $(IMGUI_BACKENDS_DIR)/imgui_impl_glfw.cpp $(IMGUI_BACKENDS_DIR)/imgui_impl_opengl3.cpp
OBJECTS = $(SRC_FILES:.cpp=.o) $(IMGUI_FILES:.cpp=.o) $(IMGUI_BACKEND_FILES:.cpp=.o)

# Target executable
TARGET = kunzite

# Build target
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(IMGUI_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(IMGUI_BACKENDS_DIR)/%.o: $(IMGUI_BACKENDS_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install dependencies (for macOS with Homebrew)
install-deps:
	brew install glfw curl

.PHONY: all clean install-deps
