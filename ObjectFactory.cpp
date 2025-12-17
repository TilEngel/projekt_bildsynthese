// ObjectFactory.cpp
#include "ObjectFactory.hpp"
#include "helper/GraphicsPipeline.hpp"
#include "helper/loadObj.hpp"
#include "helper/CubeMap.hpp"

#include "helper/Texture.hpp"

RenderObject ObjectFactory::createGenericObject(const char* modelPath,
                                         const char* vertShaderPath,
                                         const char* fragShaderPath,
                                         const char* texturePath,
                                         const glm::mat4& modelMatrix, 
                                         VkRenderPass renderPass)
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
        StencilMode::Disabled
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

    // WICHTIG: Eigentum / Cleanup-Regel festlegen:
    // - ObjectFactory returned RenderObject mit pointer auf pipeline und pointer auf Texture
    // - Du musst sicherstellen, dass main bzw. Scene diese Ressourcen beim Programmende löscht.
    // Alternativ: benutzte SmartPointer (unique_ptr) an den passenden Stellen.

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
        StencilMode::Disabled
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

RenderObject ObjectFactory::createMirror(const glm::mat4& modelMatrix, VkRenderPass renderPass) {
    // 1) Spiegel-Geometrie (Quad)
    std::vector<Vertex> vertices = {
        // Position              // UV (egal)
        {{-1.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f, 0.0f,  1.0f}, {1.0f, 1.0f}},

        {{-1.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, 0.0f,  1.0f}, {1.0f, 1.0f}},
        {{-1.0f, 0.0f,  1.0f}, {0.0f, 1.0f}},
    };

    VkBuffer vertexBuffer =
        _buff.createVertexBuffer(
            _physicalDevice,
            _device,
            _commandPool,
            _graphicsQueue,
            vertices
        );

    // 2) Pipeline: STENCIL WRITE
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/mirror.vert.spv",
        "shaders/mirror.frag.spv",
        renderPass,
        _descriptorSetLayout,
        StencilMode::Write //wichtig
    );

    Texture* dummyTex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, 
                                    "textures/whitepixel.jpg");

    // 3) RenderObject bauen
    RenderObject mirror{};
    mirror.vertexBuffer = vertexBuffer;
    mirror.vertexCount  = static_cast<uint32_t>(vertices.size());
    mirror.pipeline     = pipeline;
    mirror.modelMatrix  = modelMatrix;
    mirror.textureImageView = dummyTex->getImageView();
    mirror.textureSampler   = dummyTex->getSampler();

    return mirror;
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
        StencilMode::Disabled
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
