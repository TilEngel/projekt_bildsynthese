# ------------------------------------------------------------
#   Portable Vulkan + GLFW Makefile (macOS / Linux / Windows)
# ------------------------------------------------------------

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

UNAME_S := $(shell uname -s)

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

else ifeq ($(UNAME_S), Linux)
    # Linux with apt-installed libs (GLM included automatically)
    CXXFLAGS += $(shell pkg-config --cflags glfw3 vulkan)
    LDFLAGS  = $(shell pkg-config --libs glfw3 vulkan) -ldl -lpthread

else
    # Windows (MinGW)
    GLM_PATH = C:/VulkanSDK/Include
    CXXFLAGS += -I$(GLM_PATH) -I"C:/glfw/include"
    LDFLAGS  = -L"C:/VulkanSDK/Lib" -lvulkan-1 \
               -L"C:/glfw/lib" -lglfw3 \
               -lgdi32 -luser32 -lshell32
endif

# ------------------------------------------------------------
# Files
# ------------------------------------------------------------

SRC = \
    main.cpp \
    helper/initInstance.cpp \
    helper/initBuffer.cpp \
    helper/loadObj.cpp \
    helper/Texture.cpp \
    helper/Window.cpp \
    helper/Surface.cpp \
    helper/Swapchain.cpp \
    helper/Depthbuffer.cpp \
    helper/GraphicsPipeline.cpp \
    helper/Framebuffers.cpp

OBJ = $(SRC:.cpp=.o)
TARGET = projekt

# ------------------------------------------------------------
# Build
# ------------------------------------------------------------

$(TARGET): $(OBJ) shaders/testapp.vert.spv shaders/testapp.frag.spv
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ------------------------------------------------------------
# Shader compilation
# ------------------------------------------------------------

%.vert.spv: %.vert
	glslangValidator -V $< -o $@

%.frag.spv: %.frag
	glslangValidator -V $< -o $@

# ------------------------------------------------------------
# Utilities
# ------------------------------------------------------------

.PHONY: clean run

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ) shaders/*.spv

