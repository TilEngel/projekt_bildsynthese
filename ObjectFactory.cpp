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
                                         PipelineType type,
                                        uint32_t subpassIndex)
{
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        vertShaderPath, 
        fragShaderPath,
        renderPass,
        _descriptorSetLayout,
        type,
        subpassIndex
    );

    std::vector<Vertex> vertices;
    _loader.objLoader(modelPath, vertices);

    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device, _commandPool, _graphicsQueue, vertices);

    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, texturePath);

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = tex->getImageView();
    obj.textureSampler = tex->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = modelMatrix;
    obj.texture = tex;

    return obj;
}


RenderObject ObjectFactory::createSkybox(VkRenderPass renderPass, const std::array<const char*, 6>& cubemapFaces) {
    std::vector<Vertex> vertices = {
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},

        // Left face (X-)
        {{-1.0f, -1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

        // Right face (X+)
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

        // Front face (Z+)
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

        // Top face (Y+)
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},

        // Bottom face (Y-)
        {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}
    };

    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/skybox.vert.spv",
        "shaders/skybox.frag.spv",
        renderPass,
        _descriptorSetLayout,
        PipelineType::SKYBOX,
        2
    );

    InitBuffer buff;

    VkBuffer vertexBuffer = buff.createVertexBuffer(_physicalDevice, _device, _commandPool, _graphicsQueue, vertices);

    CubeMap* cubemap = new CubeMap(_physicalDevice, _device,
                                   _commandPool, _graphicsQueue, cubemapFaces);

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
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
        // Quad in XY-Ebene, Normale zeigt in +Z
        {{-0.1f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.1f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.1f,  0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        
        {{ 0.1f,  0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.1f,  0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.1f, -0.1f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}
    };

    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/snow.vert.spv",
        "shaders/snow.frag.spv",
        renderPass,
        snowDescriptorSetLayout,
        PipelineType::STANDARD,
        2
    );

    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device, _commandPool, _graphicsQueue, vertices);

    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, texturePath);

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
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
        PipelineType::STANDARD,2
    );
   
    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device,
                                                    _commandPool, _graphicsQueue, sphereVertices);
    
    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, 
                              "textures/white.png");
    
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.02f));
    
    light.renderObject.vertexBuffer = vertexBuffer;
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
        PipelineType::STANDARD,
        2
    );
    
    std::vector<Vertex> vertices;
    _loader.objLoader(modelPath, vertices);
    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device,
                                                    _commandPool, _graphicsQueue, vertices);
    
    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, texturePath);
    
    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
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
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        
        {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
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
        pipelineType,
        2
    );

    VkBuffer vertexBuffer = _buff.createVertexBuffer(
        _physicalDevice, _device, _commandPool, _graphicsQueue, vertices);

    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, 
                               _graphicsQueue, "textures/mirror.jpg");

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = tex->getImageView();
    obj.textureSampler = tex->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = modelMatrix;
    obj.texture = tex;

    return obj;
}
DeferredRenderObject ObjectFactory::createDeferredObject(const char* modelPath,const char* texturePath,const glm::mat4& modelMatrix,VkRenderPass renderPass){
    DeferredRenderObject deferredObj{};

    // Load geometry
    std::vector<Vertex> vertices;
    _loader.objLoader(modelPath, vertices);
    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device,
                                                     _commandPool, _graphicsQueue, vertices);

    // Load texture
    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, texturePath);

    // Pipeline for Depth Prepass (Subpass 0)
    GraphicsPipeline* depthPipeline = new GraphicsPipeline(
        _device, _colorFormat, _depthFormat,
        "shaders/depth_only.vert.spv",
        "shaders/depth_only.frag.spv",
        renderPass,
        _descriptorSetLayout,
        PipelineType::DEPTH_ONLY,
        0  // Subpass 0
    );

    deferredObj.depthPass.vertexBuffer = vertexBuffer;
    deferredObj.depthPass.vertexCount = static_cast<uint32_t>(vertices.size());
    deferredObj.depthPass.textureImageView = tex->getImageView();
    deferredObj.depthPass.textureSampler = tex->getSampler();
    deferredObj.depthPass.pipeline = depthPipeline;
    deferredObj.depthPass.modelMatrix = modelMatrix;
    deferredObj.depthPass.instanceCount = 1;
    deferredObj.depthPass.isDeferred =true;

    // Pipeline for G-Buffer Pass (Subpass 1)
    GraphicsPipeline* gbufferPipeline = new GraphicsPipeline(
        _device, _colorFormat, _depthFormat,
        "shaders/gbuffer.vert.spv",
        "shaders/gbuffer.frag.spv",
        renderPass,
        _descriptorSetLayout,
        PipelineType::GBUFFER,
        1  // Subpass 1
    );

    deferredObj.gbufferPass.vertexBuffer = vertexBuffer;
    deferredObj.gbufferPass.vertexCount = static_cast<uint32_t>(vertices.size());
    deferredObj.gbufferPass.textureImageView = tex->getImageView();
    deferredObj.gbufferPass.textureSampler = tex->getSampler();
    deferredObj.gbufferPass.pipeline = gbufferPipeline;
    deferredObj.gbufferPass.modelMatrix = modelMatrix;
    deferredObj.gbufferPass.instanceCount = 1;
    deferredObj.gbufferPass.isDeferred =true;

    return deferredObj;
}


RenderObject ObjectFactory::createLightingQuad(VkRenderPass renderPass,
                                              VkDescriptorSetLayout lightingLayout)
{
    // Fullscreen quad - vertices generated in shader
    std::vector<Vertex> vertices = {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
    };

    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device, _colorFormat, _depthFormat,
        "shaders/lighting.vert.spv",
        "shaders/lighting.frag.spv",
        renderPass,
        lightingLayout,
        PipelineType::LIGHTING,
        2  // Subpass 2
    );

    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device,
                                                    _commandPool, _graphicsQueue, vertices);

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.pipeline = pipeline;
    obj.modelMatrix = glm::mat4(1.0f);
    obj.instanceCount = 1;

    return obj;
}

RenderObject ObjectFactory::createReflectiveObject(
    const char* modelPath,
    ReflectionProbe* probe,
    const glm::mat4& modelMatrix,
    VkRenderPass renderPass)
{
    // Pipeline
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/renderToTexture.vert.spv",
        "shaders/renderToTexture.frag.spv",
        renderPass,
        _descriptorSetLayout,
        PipelineType::STANDARD,
        2
    );

    // Model laden
    std::vector<Vertex> vertices;
    _loader.objLoader(modelPath, vertices);

    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device, _commandPool, _graphicsQueue, vertices);
   
    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = probe->getCubemapView();
    obj.textureSampler = probe->getCubemapSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = modelMatrix;
    obj.instanceCount = 1;
    obj.texture = nullptr;

    std::cout << "Reflective object created with cubemap" << std::endl;

    return obj;
}