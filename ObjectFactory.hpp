// ObjectFactory.hpp
#pragma once

#include <string>
#include <glm/glm.hpp>
#include "helper/GraphicsPipeline.hpp"
#include "helper/Scene.hpp"
#include "helper/initBuffer.hpp"
#include "helper/loadObj.hpp"

// Benötigte Vulkan-Handles / Helper-Referenzen werden per Konstruktor übergeben
class ObjectFactory {
public:
    ObjectFactory(VkPhysicalDevice physicalDevice,
                  VkDevice device,
                  VkCommandPool commandPool,
                  VkQueue graphicsQueue,
                  VkFormat colorFormat,
                  VkFormat depthFormat,
                  VkDescriptorSetLayout descriptorSetLayout)
        : _physicalDevice(physicalDevice),
          _device(device),
          _commandPool(commandPool),
          _graphicsQueue(graphicsQueue),
          _colorFormat(colorFormat),
          _depthFormat(depthFormat),
          _descriptorSetLayout(descriptorSetLayout) {}

    //erstellt RenderObject für beliebige Objekte ohne bestimmten Shader o.Ä.
    RenderObject createGenericObject(const char* modelPath,
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

    RenderObject createGround(const glm::mat4& modelMatrix,VkRenderPass renderPass);

    RenderObject createSkybox(VkRenderPass renderPass, const std::array<const char*, 6>& cubemapFaces);
private:
    LoadObj _loader;
    InitBuffer _buff;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VkCommandPool _commandPool;
    VkQueue _graphicsQueue;
    VkFormat _colorFormat;
    VkFormat _depthFormat;
    VkDescriptorSetLayout _descriptorSetLayout;
};
