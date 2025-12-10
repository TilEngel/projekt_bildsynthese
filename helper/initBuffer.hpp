/*
* Verantwortlich für Buffer-bezogene Aufgaben
*/
#pragma once

#include <vulkan/vulkan.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>

// GLFW and STB should be available using your include paths configured in the Makefile.
// Do NOT use absolute paths here.
#include <GLFW/glfw3.h>
// stb_image.h sollte sich im Projektverzeichnis befinden oder via -I erreichbar sein.
#include "../stb_image.h"
#include "GraphicsPipeline.hpp"

class InitBuffer {
public:
    // buffer handles
    VkBuffer _vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer _imageBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _imageBufferMemory = VK_NULL_HANDLE;
    int _texWidth = 0;
    int _texHeight = 0;

    //Finde memory Type index auf physical device, dass zu typeFilter und properties passt
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice);

    // Helpers für single-time command buffers
    VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
    void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer);

    //Selbsterklärend 
    void copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    VkBuffer createVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, const std::vector<Vertex>& vertices);
    void destroyVertexBuffer(VkDevice device);
    VkBuffer createImageBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const char* imagePath);
    void destroyImageBuffer(VkDevice device);

    int getTexWidth() const;
    int getTexHeight() const;
};
