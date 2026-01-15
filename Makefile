# ------------------------------------------------------------
#   Portable Vulkan + GLFW Makefile (macOS / Linux / Windows)
# ------------------------------------------------------------

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
UNAME_S := $(shell uname -s)
BUILD_DIR = build
# Vulkan SDK env var is used if available
VULKAN_SDK ?=

# ------------------------------------------------------------
# Platform-specific settings
# ------------------------------------------------------------

ifeq ($(UNAME_S), Darwin)
    # macOS
    GLM_PATH = $(VULKAN_SDK)/include
    CXXFLAGS += $(shell pkg-config --cflags glfw3) -I$(GLM_PATH)
    LDFLAGS  = $(shell pkg-config --libs glfw3)
    LDFLAGS += -lvulkan \
               -framework Cocoa -framework IOKit -framework CoreFoundation -framework CoreVideo
    MKDIR = mkdir -p

else ifeq ($(UNAME_S), Linux)
    # Linux with apt-installed libs (GLM included automatically)
    CXXFLAGS += $(shell pkg-config --cflags glfw3 vulkan)
    LDFLAGS  = $(shell pkg-config --libs glfw3 vulkan) -ldl -lpthread
    MKDIR = mkdir -p

else
    # Windows (MinGW)
    GLM_PATH = C:/VulkanSDK/Include
    CXXFLAGS += -I$(GLM_PATH) -I"C:/glfw/include"
    LDFLAGS  = -L"C:/VulkanSDK/Lib" -lvulkan-1 \
               -L"C:/glfw/lib" -lglfw3 \
               -lgdi32 -luser32 -lshell32
    MKDIR = mkdir
endif

# ------------------------------------------------------------
# Files
# ------------------------------------------------------------

SRC = \
    main.cpp \
    ObjectFactory.cpp \
    helper/initInstance.cpp \
    helper/initBuffer.cpp \
    helper/ObjectLoading/loadObj.cpp \
    helper/Texture/Texture.cpp \
    helper/Rendering/Window.cpp \
    helper/Rendering/Surface.cpp \
    helper/Rendering/Swapchain.cpp \
    helper/Rendering/Depthbuffer.cpp \
    helper/Rendering/GraphicsPipeline.cpp \
    helper/Rendering/Framebuffers.cpp \
    helper/Frames/Frame.cpp \
    helper/Texture/CubeMap.cpp\
    helper/Compute/Snow.cpp\
    helper/MirrorSystem.cpp
    

#source-paths zu build-Ordner-paths 
OBJ = $(SRC:%.cpp=$(BUILD_DIR)/%.o)
TARGET = projekt

# ------------------------------------------------------------
# Build
# -----------------------------
.PHONY: all clean run
all: $(TARGET)
$(TARGET): $(OBJ) shaders/testapp.vert.spv shaders/testapp.frag.spv shaders/mirror.frag.spv helper/Texture/Texture.hpp shaders/test.vert.spv shaders/skybox.vert.spv shaders/skybox.frag.spv shaders/snow.vert.spv shaders/snow.frag.spv shaders/snow.comp.spv shaders/lit.vert.spv shaders/lit.frag.spv shaders/depth_only.frag.spv shaders/depth_only.vert.spv shaders/gbuffer.frag.spv shaders/gbuffer.vert.spv shaders/lighting.frag.spv shaders/lighting.vert.spv shaders/renderToTexture.vert.spv shaders/renderToTexture.frag.spv
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS)

# build Ordner erstellen
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@
# ------------------------------------------------------------
# Shader compilation
# ------------------------------------------------------------

%.vert.spv: %.vert
	glslangValidator -V $< -o $@

%.frag.spv: %.frag
	glslangValidator -V $< -o $@
	
%.comp.spv: %.comp
	glslangValidator -V $< -o $@
# ------------------------------------------------------------
# Utilities
# ------------------------------------------------------------


run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
	rm -f shaders/*.spv
	rm -rf $(BUILD_DIR)

