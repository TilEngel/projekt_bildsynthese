// ObjectFactory.hpp
#pragma once

#include <string>
#include <glm/glm.hpp>
#include "helper/GraphicsPipeline.hpp"
#include "helper/Scene.hpp"

// Benötigte Vulkan-Handles / Helper-Referenzen werden per Konstruktor übergeben
class ObjectFactory {
public:
    ObjectFactory(VkPhysicalDevice physicalDevice,
                  VkDevice device,
                  VkCommandPool commandPool,
                  VkQueue graphicsQueue,
                  VkFormat colorFormat,
                  VkFormat depthFormat)
        : _physicalDevice(physicalDevice),
          _device(device),
          _commandPool(commandPool),
          _graphicsQueue(graphicsQueue),
          _colorFormat(colorFormat),
          _depthFormat(depthFormat) {}

    //erstellt RenderObject für Teapot
    RenderObject createTeapot(const char* modelPath,
                              const char* vertShaderPath,
                              const char* fragShaderPath,
                              const char* texturePath,
                              const glm::mat4& modelMatrix,
                            VkRenderPass renderPass);

    //erstellt RenderObject für Flying Dutchman
    RenderObject createFlyingDutchman(const char* modelPath,
                              const char* vertShaderPath,
                              const char* fragShaderPath,
                              const char* texturePath,
                              const glm::mat4& modelMatrix,
                              VkRenderPass renderPass);

private:
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VkCommandPool _commandPool;
    VkQueue _graphicsQueue;
    VkFormat _colorFormat;
    VkFormat _depthFormat;
};
