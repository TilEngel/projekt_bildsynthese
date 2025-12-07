# ------------------------------------------------------------
#   Portable Vulkan + GLFW Makefile (macOS / Linux / Windows)
# ------------------------------------------------------------

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# Detect OS
UNAME_S := $(shell uname -s)

# Vulkan SDK base path detection
# NOTE: Vulkan SDK should set VULKAN_SDK environment variable automatically.
VULKAN_SDK ?= /usr/local # fallback, if not set

# Platform-specific flags
ifeq ($(UNAME_S), Darwin)
    # macOS (MoltenVK) – GLM is inside VulkanSDK/.../macOS/include
    GLM_PATH = $(VULKAN_SDK)/include
    CXXFLAGS  += $(shell pkg-config --cflags glfw3) -I$(GLM_PATH)
    LDFLAGS   = $(shell pkg-config --libs glfw3) \
                -lvulkan \
                -framework Cocoa -framework IOKit -framework CoreFoundation -framework CoreVideo
else ifeq ($(UNAME_S), Linux)
    # Linux – GLM comes with Vulkan SDK or system include
    GLM_PATH = $(VULKAN_SDK)/include
    CXXFLAGS += $(shell pkg-config --cflags glfw3) -I$(GLM_PATH)
    LDFLAGS  = $(shell pkg-config --libs glfw3) -lvulkan -ldl -lpthread
else
    # Windows (MinGW)
    GLM_PATH = C:/VulkanSDK/Include
    CXXFLAGS += -I$(GLM_PATH) -I"C:/glfw/include"
    LDFLAGS  = -L"C:/VulkanSDK/Lib" -lvulkan-1 \
               -L"C:/glfw/lib" -lglfw3 \
               -lgdi32 -luser32 -lshell32
endif

# -----------------------------
# Files
# -----------------------------
SRC = \
    main.cpp \
    ./helper/initInstance.cpp \
    ./helper/initBuffer.cpp \
    ./helper/loadObj.cpp \
    ./helper/Texture.cpp \
    ./helper/Window.cpp \
    ./helper/Surface.cpp \
    ./helper/Swapchain.cpp \
    ./helper/Depthbuffer.cpp \
    ./helper/GraphicsPipeline.cpp \
    ./helper/Framebuffers.cpp

OBJ = $(SRC:.cpp=.o)
TARGET = projekt

# -----------------------------
# Build
# -----------------------------
$(TARGET): $(OBJ) shaders/testapp.vert.spv shaders/testapp.frag.spv HelloVulkan17.hpp helper/Texture.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# -----------------------------
# Shader Compilation
# -----------------------------
%.vert.spv: %.vert
	glslangValidator -V -o $@ $<

%.frag.spv: %.frag
	glslangValidator -V -o $@ $<

# -----------------------------
# Utility
# -----------------------------
.PHONY: test clean

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)
	rm -f shaders/*.spv
