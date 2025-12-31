// ObjectFactory.hpp 
#pragma once

#include <string>
#include <glm/glm.hpp>
#include "helper/Rendering/GraphicsPipeline.hpp"
#include "Scene.hpp"
#include "helper/initBuffer.hpp"
#include "helper/ObjectLoading/loadObj.hpp"
#include "helper/Compute/Snow.hpp"
#include "helper/MirrorSystem.hpp"
#include <array>


class ObjectFactory {
public:
    ObjectFactory(VkPhysicalDevice physicalDevice, VkDevice device,
                 VkCommandPool commandPool, VkQueue graphicsQueue,
                 VkFormat colorFormat, VkFormat depthFormat,
                 VkDescriptorSetLayout descriptorSetLayout,
                 VkDescriptorSetLayout litDescriptorSetLayout)
        : _physicalDevice(physicalDevice), _device(device),
          _commandPool(commandPool), _graphicsQueue(graphicsQueue),
          _colorFormat(colorFormat), _depthFormat(depthFormat),
          _descriptorSetLayout(descriptorSetLayout),
          _litDescriptorSetLayout(litDescriptorSetLayout) {}

    RenderObject createGenericObject(const char* modelPath,
                                         const char* vertShaderPath,
                                         const char* fragShaderPath,
                                         const char* texturePath,
                                         const glm::mat4& modelMatrix, 
                                         VkRenderPass renderPass,
                                         PipelineType type, uint32_t subpassIndex);

    // Neue Methode für deferred gerenderte Objekte
    DeferredRenderObject createDeferredObject(
        const char* modelPath,
        const char* texturePath,
        const glm::mat4& modelMatrix,
        VkRenderPass renderPass);

    DeferredRenderObject createDeferredLitObject(
        const char* modelPath,
        const char* texturePath,
        const glm::mat4& modelMatrix,
        VkRenderPass renderPass);

    // Existing methods bleiben für forward-rendered objects
    RenderObject createSkybox(VkRenderPass renderPass,
                             const std::array<const char*, 6>& cubemapFaces,
                             uint32_t subpassIndex = 2);

    RenderObject createSnowflake(const char* texturePath,
                                VkRenderPass renderPass,
                                VkBuffer particleBuffer,
                                VkDescriptorSetLayout snowDescriptorSetLayout,
                                uint32_t subpassIndex = 2);

    RenderObject createMirror(const glm::mat4& modelMatrix,
                             VkRenderPass renderPass,
                             PipelineType pipelineType,
                             uint32_t subpassIndex = 2);

    LightSourceObject createLightSource(const glm::vec3& position,
                                       const glm::vec3& color,
                                       float intensity,
                                       float radius,
                                       VkRenderPass renderPass);

    // Fullscreen Quad für Lighting Pass
    RenderObject createLightingQuad(VkRenderPass renderPass,
                                   VkDescriptorSetLayout lightingLayout);

private:
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VkCommandPool _commandPool;
    VkQueue _graphicsQueue;
    VkFormat _colorFormat;
    VkFormat _depthFormat;
    VkDescriptorSetLayout _descriptorSetLayout;
    VkDescriptorSetLayout _litDescriptorSetLayout;

    InitBuffer _buff;
    LoadObj _loader;
};