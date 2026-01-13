#pragma once
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include <string>

struct Vertex {
    glm::vec3 pos;
    glm::vec2 tex;
};

enum class PipelineType {
    STANDARD,          // Normales forward rendering
    MIRROR_MARK,       // Stencil marking pass
    MIRROR_REFLECT,    // Reflektierte objekte
    MIRROR_BLEND,      // Transparenter Spiegel
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
    void destroy();

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

     // create a pipeline layout object
    // - contains descriptor set layout from the previous function
    // - store object in _pipeline layout
    void createPipelineLayout();

    // create a graphics pipeline object
    // - create vertex shader module from file shaders/testapp.vert.spv
    // - create fragment shader module from file shaders/testapp.frag.spv
    // - create corresponding shader stage create info structures
    // - create vertex input state create info structure
    //   - binding is 0
    //   - see locations in the vertex shader
    //   - use sizeof and offsetof with Vertex structure
    // - create input assemply state create info structure
    //   - vertex data consists of a triangle list
    // - create viewport state create info structure
    //   - viewport and scissor will be dynamic state
    // - create rasterization state create info structure
    //   - set polygon mode to fill
    // - create multisample state create info structure
    //   - disable multisampling
    // - create depth-stencil state create info structure
    //   - enable depth test
    //   - disable stencil test
    // - create color blend state create info structure
    //   - disable color blending
    // - create dynamic state create info
    //   - viewport and scissor are dynamic state
    // - create graphics pipeline object
    //   - use all previously created create infos
    //   - use _pipelineLyout
    //   - use _renderPass
    // - store object in _graphicsPipeline
    // - destroy the shader modules
    void createPipeline();

    // destroy the pipeline layout object
    void cleanupPipelineLayout();

    // destroy the pipeline object
    void cleanupPipeline();
};

