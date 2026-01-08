#pragma once
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include <string>

struct Vertex {
    glm::vec3 pos;
    glm::vec2 tex;
};

enum class PipelineType {
    STANDARD,          // Normal forward rendering
    MIRROR_MARK,       // Stencil marking pass
    MIRROR_REFLECT,    // Reflected objects
    MIRROR_BLEND,      // Transparent mirror
    DEPTH_ONLY,        // Depth prepass (subpass 0)
    GBUFFER,          // G-Buffer generation (subpass 1)
    LIGHTING,          // Deferred lighting (subpass 2)
    SKYBOX              //Extra für die Skybox
};

enum class SubpassIndex {
    DEPTH = 0,
    GBUFFER = 1,
    LIGHTING = 2
};

class GraphicsPipeline {
public:
    GraphicsPipeline(VkDevice device,
                     VkFormat colorFormat,
                     VkFormat depthFormat,
                     const char* vertexShaderPath,
                     const char* fragmentShaderPath,
                     VkRenderPass renderPass,
                     VkDescriptorSetLayout descriptorSetLayout,
                     PipelineType pipelineType = PipelineType::STANDARD,
                     uint32_t subpassIndex = 0)
        : _device(device),
          _colorFormat(colorFormat),
          _depthFormat(depthFormat),
          _vertexShaderPath(vertexShaderPath),
          _fragmentShaderPath(fragmentShaderPath),
          _renderPass(renderPass),
          _descriptorSetLayout(descriptorSetLayout),
          _pipelineType(pipelineType),
          _subpassIndex(subpassIndex) {
        createPipelineLayout();
        createPipeline();
    }

    ~GraphicsPipeline() {
        cleanupPipeline();
        cleanupPipelineLayout();
    }

    VkPipeline getPipeline() const { return _graphicsPipeline; }
    VkPipelineLayout getPipelineLayout() const { return _pipelineLayout; }
    VkRenderPass getRenderPass() const { return _renderPass; }

    VkDevice getDevice() const { return _device; }
    VkFormat getColorFormat() const { return _colorFormat; }
    VkFormat getDepthFormat() const { return _depthFormat; }

private:
    VkDevice _device;
    VkFormat _colorFormat;
    VkFormat _depthFormat;
    const char* _vertexShaderPath;
    const char* _fragmentShaderPath;
    VkRenderPass _renderPass;
    VkDescriptorSetLayout _descriptorSetLayout;
    PipelineType _pipelineType;
    uint32_t _subpassIndex;

    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkPipeline _graphicsPipeline = VK_NULL_HANDLE;

    void createPipelineLayout();
    void createPipeline();
    void cleanupPipelineLayout();
    void cleanupPipeline();
};

