// ObjectFactory.cpp
#include "ObjectFactory.hpp"
#include "helper/ObjectLoading/loadObj.hpp"
#include "helper/Rendering/GraphicsPipeline.hpp"
#include "helper/Texture/CubeMap.hpp"

#include "helper/Texture/Texture.hpp"

RenderObject ObjectFactory::createGenericObject(const char* modelPath,
                                         const char* vertShaderPath,
                                         const char* fragShaderPath,
                                         const char* texturePath,
                                         const glm::mat4& modelMatrix, 
                                         VkRenderPass renderPass,
                                         PipelineType type)
{
    //eigene Pipeline erstellen
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

    //Model laden & Vertexbuffer erzeugen
    std::vector<Vertex> vertices;
    _loader.objLoader(modelPath, vertices);
    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device, _commandPool, _graphicsQueue, vertices);


    //Textur laden
    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, texturePath);

    //build RenderObject
    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = tex->getImageView();
    obj.textureSampler = tex->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = modelMatrix;



    return obj;
}

//Erstellt den Boden. Weniger Parameter und theoretisch mehr Freiheit für besondere techniken (wird stand jetzt nur nicht ausgenutzt)
RenderObject ObjectFactory::createGround(const glm::mat4& modelMatrix, VkRenderPass renderPass){
    //eigene Pipeline erstellen
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

    //Model laden & Vertexbuffer erzeugen
    std::vector<Vertex> vertices;
    _loader.objLoader("models/wooden_bowl.obj", vertices);
   
    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device, _commandPool, _graphicsQueue, vertices);

    //Textur laden
    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, "textures/wooden_bowl.jpg");

    //build RenderObject
    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = tex->getImageView();
    obj.textureSampler = tex->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = modelMatrix;

    return obj;
}

RenderObject ObjectFactory::createSkybox(VkRenderPass renderPass, 
                                         const std::array<const char*, 6>& cubemapFaces) {
    // Skybox ist ein Würfel
    std::vector<Vertex> vertices = {
        // Positionen für einen Würfel (nur Position, keine Textur-Koordinaten nötig)
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

    // Pipeline für Skybox
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

    // Vertex Buffer
    InitBuffer buff;
    VkBuffer vertexBuffer = buff.createVertexBuffer(_physicalDevice, _device,
                                                    _commandPool, _graphicsQueue, vertices);

    // CubeMap Textur laden
    CubeMap* cubemap = new CubeMap(_physicalDevice, _device,
                                                  _commandPool, _graphicsQueue, cubemapFaces);

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = cubemap->getImageView();
    obj.textureSampler = cubemap->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = glm::mat4(1.0f); // Keine Transformation nötig

    return obj;
}

RenderObject ObjectFactory::createMirror(const glm::mat4& modelMatrix, 
                                         VkRenderPass renderPass,
                                         PipelineType pipelineType) {
    // Einfache Quad-Geometrie für Spiegel
    std::vector<Vertex> vertices = {
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}},  // Oben links
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},  // Unten links
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},  // Unten rechts
        
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},  // Unten rechts
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}},  // Oben rechts
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}}   // Oben links
    };

    const char* fragShader;
    if (pipelineType == PipelineType::MIRROR_BLEND) {
        fragShader = "shaders/mirror.frag.spv";  // Transparenter Shader
    } else {
        fragShader = "shaders/testapp.frag.spv";
    }

RenderObject ObjectFactory::createSnowflake(const char* texturePath, 
                                           VkRenderPass renderPass,
                                           VkBuffer particleBuffer, VkDescriptorSetLayout snowDescriptorSetLayout) {
    // Einfaches Quad für Schneeflocke
    std::vector<Vertex> vertices = {
        {{-0.1f, -0.1f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.1f, -0.1f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.1f,  0.1f, 0.0f}, {1.0f, 1.0f}},
        
        {{ 0.1f,  0.1f, 0.0f}, {1.0f, 1.0f}},
        {{-0.1f,  0.1f, 0.0f}, {0.0f, 1.0f}},
        {{-0.1f, -0.1f, 0.0f}, {0.0f, 0.0f}}
    };

    // Pipeline für Schneeflocken (mit Instancing)
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/testapp.vert.spv",
        fragShader,
        renderPass,
        _descriptorSetLayout,
        pipelineType  // Pipeline-Typ übergeben
    );

    VkBuffer vertexBuffer = _buff.createVertexBuffer(
        _physicalDevice, _device, _commandPool, _graphicsQueue, vertices
    );

    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, 
                               _graphicsQueue, "textures/mirror.jpg");

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = tex->getImageView();
    obj.textureSampler = tex->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = modelMatrix;

    return obj;
}
