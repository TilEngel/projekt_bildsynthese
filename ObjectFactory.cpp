// ObjectFactory.cpp (Merged)
#include "ObjectFactory.hpp"
#include "helper/ObjectLoading/loadObj.hpp"
#include "helper/Texture/CubeMap.hpp"
#include "helper/Texture/Texture.hpp"
#include <vulkan/vulkan_core.h>

RenderObject ObjectFactory::createGenericObject(const char* modelPath,
                                         const char* vertShaderPath,
                                         const char* fragShaderPath,
                                         const char* texturePath,
                                         const glm::mat4& modelMatrix, 
                                         VkRenderPass renderPass,
                                         PipelineType type)
{
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        vertShaderPath, 
        fragShaderPath,
        renderPass,
        _descriptorSetLayout,
        type
    );

    std::vector<Vertex> vertices;
    _loader.objLoader(modelPath, vertices);

    VkDeviceMemory vertexBufferMemory;
    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device, _commandPool, _graphicsQueue, vertices, &vertexBufferMemory);

    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, texturePath);

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexBufferMemory = vertexBufferMemory;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = tex->getImageView();
    obj.textureSampler = tex->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = modelMatrix;
    obj.texture = tex;

    return obj;
}

RenderObject ObjectFactory::createGround(const glm::mat4& modelMatrix, VkRenderPass renderPass){
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/test.vert.spv", 
        "shaders/testapp.frag.spv",
        renderPass,
        _descriptorSetLayout,
        PipelineType::STANDARD
    );

    std::vector<Vertex> vertices;
    _loader.objLoader("models/wooden_bowl.obj", vertices);
   
    VkDeviceMemory vertexBufferMemory;
    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device, _commandPool, _graphicsQueue, vertices, &vertexBufferMemory);

    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, "textures/wooden_bowl.jpg");

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexBufferMemory = vertexBufferMemory;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = tex->getImageView();
    obj.textureSampler = tex->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = modelMatrix;
    obj.texture = tex;

    return obj;
}

RenderObject ObjectFactory::createSkybox(VkRenderPass renderPass, 
                                         const std::array<const char*, 6>& cubemapFaces) {
    std::vector<Vertex> vertices = {
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},

        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f}},

        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},

        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f}},

        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f}},

        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f}}
    };

    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/skybox.vert.spv",
        "shaders/skybox.frag.spv",
        renderPass,
        _descriptorSetLayout,
        PipelineType::STANDARD
    );

    InitBuffer buff;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer vertexBuffer = buff.createVertexBuffer(_physicalDevice, _device, _commandPool, _graphicsQueue, vertices, &vertexBufferMemory);

    CubeMap* cubemap = new CubeMap(_physicalDevice, _device,
                                   _commandPool, _graphicsQueue, cubemapFaces);

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexBufferMemory = vertexBufferMemory;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = cubemap->getImageView();
    obj.textureSampler = cubemap->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = glm::mat4(1.0f);

    return obj;
}

RenderObject ObjectFactory::createSnowflake(const char* texturePath, 
                                           VkRenderPass renderPass,
                                           VkBuffer particleBuffer, 
                                           VkDescriptorSetLayout snowDescriptorSetLayout) {
    std::vector<Vertex> vertices = {
        {{-0.1f, -0.1f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.1f, -0.1f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.1f,  0.1f, 0.0f}, {1.0f, 1.0f}},
        
        {{ 0.1f,  0.1f, 0.0f}, {1.0f, 1.0f}},
        {{-0.1f,  0.1f, 0.0f}, {0.0f, 1.0f}},
        {{-0.1f, -0.1f, 0.0f}, {0.0f, 0.0f}}
    };

    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/snow.vert.spv",
        "shaders/snow.frag.spv",
        renderPass,
        snowDescriptorSetLayout,
        PipelineType::STANDARD
    );

    VkDeviceMemory vertexBufferMemory;
    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device, _commandPool, _graphicsQueue, vertices, &vertexBufferMemory);

    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, texturePath);

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexBufferMemory = vertexBufferMemory;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = tex->getImageView();
    obj.textureSampler = tex->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = glm::mat4(1.0f);
    obj.instanceBuffer = particleBuffer;
    obj.instanceCount = NUMBER_PARTICLES;
    obj.isSnow = true;
    obj.texture = tex;

    return obj;
}

LightSourceObject ObjectFactory::createLightSource(const glm::vec3& position,
                                                const glm::vec3& color,
                                                float intensity,
                                                float radius,
                                                VkRenderPass renderPass) {
    LightSourceObject light;
    light.position = position;
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;
    
    std::vector<Vertex> sphereVertices;
    _loader.objLoader("models/teapot.obj", sphereVertices);
    
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/testapp.vert.spv",
        "shaders/testapp.frag.spv",
        renderPass,
        _descriptorSetLayout,
        PipelineType::STANDARD
    );
    
    VkDeviceMemory vertexBufferMemory;
    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device,
                                                    _commandPool, _graphicsQueue, sphereVertices, &vertexBufferMemory);
    
    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, 
                              "textures/white.png");
    
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.02f));
    
    light.renderObject.vertexBuffer = vertexBuffer;
    light.renderObject.vertexBufferMemory = vertexBufferMemory;
    light.renderObject.vertexCount = static_cast<uint32_t>(sphereVertices.size());
    light.renderObject.textureImageView = tex->getImageView();
    light.renderObject.textureSampler = tex->getSampler();
    light.renderObject.pipeline = pipeline;
    light.renderObject.modelMatrix = modelMatrix;
    light.renderObject.instanceCount = 1;
    light.renderObject.isLit = false;
    light.renderObject.texture = tex;
    
    return light;
}

RenderObject ObjectFactory::createLitObject(const char* modelPath,
                                          const char* texturePath,
                                          const glm::mat4& modelMatrix,
                                          VkRenderPass renderPass) {
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/lit.vert.spv",
        "shaders/lit.frag.spv",
        renderPass,
        _litDescriptorSetLayout,
        PipelineType::STANDARD
    );
    
    std::vector<Vertex> vertices;
    _loader.objLoader(modelPath, vertices);
    VkDeviceMemory vertexBufferMemory;
    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device,
                                                    _commandPool, _graphicsQueue, vertices, &vertexBufferMemory);
    
    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, texturePath);
    
    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexBufferMemory = vertexBufferMemory;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = tex->getImageView();
    obj.textureSampler = tex->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = modelMatrix;
    obj.isLit = true;
    obj.texture = tex;
    
    return obj;
}

RenderObject ObjectFactory::createMirror(const glm::mat4& modelMatrix, 
                                         VkRenderPass renderPass,
                                         PipelineType pipelineType) {
    std::vector<Vertex> vertices = {
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}}
    };

    const char* fragShader;
    if (pipelineType == PipelineType::MIRROR_BLEND) {
        fragShader = "shaders/mirror.frag.spv";
    } else {
        fragShader = "shaders/testapp.frag.spv";
    }

    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/testapp.vert.spv",
        fragShader,
        renderPass,
        _descriptorSetLayout,
        pipelineType
    );

    VkDeviceMemory vertexBufferMemory;
    VkBuffer vertexBuffer = _buff.createVertexBuffer(
        _physicalDevice, _device, _commandPool, _graphicsQueue, vertices, &vertexBufferMemory);

    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, 
                               _graphicsQueue, "textures/mirror.jpg");

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexBufferMemory = vertexBufferMemory;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = tex->getImageView();
    obj.textureSampler = tex->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = modelMatrix;
    obj.texture = tex;

    return obj;
}
