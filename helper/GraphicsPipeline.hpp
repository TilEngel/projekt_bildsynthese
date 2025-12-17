#pragma once

#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>


struct UniformBufferObject {
//    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec2 tex;
};

enum class StencilMode {
    Disabled,
    Write,
    Test
};


class GraphicsPipeline {
public:

    GraphicsPipeline(VkDevice device, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat,const char* vertShaderPath, const char* fragShaderPath, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout, StencilMode stencilMode = StencilMode::Disabled) 
    : _device(device)
        , _vertexShaderPath(vertShaderPath)
        , _fragmentShaderPath(fragShaderPath)
        , _descriptorSetLayout(descriptorSetLayout)
        , _stencilMode(stencilMode){
        _renderPass = renderPass;
       // createRenderPass(colorAttachmentFormat, depthAttachmentFormat);
       // createDescriptorSetLayout();
        createPipelineLayout();
        createPipeline();
    }

    ~GraphicsPipeline() {
        cleanupPipeline();
        cleanupPipelineLayout();
    //    cleanupDescriptorSetLayout();
    //    cleanupRenderPass();
    }

    VkRenderPass getRenderPass() {
        return _renderPass;
    }

    VkDescriptorSetLayout getDescriptorSetLayout() {
        return _descriptorSetLayout;
    }

    VkPipelineLayout getPipelineLayout() {
        return _pipelineLayout;
    }

    VkPipeline getPipeline() {
        return _graphicsPipeline;
    }
    StencilMode getStencilMode() {
        return _stencilMode;
    }


private:

    VkDevice _device = VK_NULL_HANDLE;

    VkRenderPass _renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkPipeline _graphicsPipeline = VK_NULL_HANDLE;
    const char* _vertexShaderPath;
    const char* _fragmentShaderPath;

    StencilMode _stencilMode = StencilMode::Disabled;

    // create a render pass object
    // - with one color and one depth attachment
    // - with one subpass
    // - subpass depends on external subpass (srcSubpass = VK_SUBPASS_EXTERNAL)
    // - store object in _renderpass
    void createRenderPass(VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat);

   
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
    
    // destroy the render pass object    
    void cleanupRenderPass();

    // destroy the pipeline layout object
    void cleanupPipelineLayout();

    // destroy the pipeline object
    void cleanupPipeline();
};
