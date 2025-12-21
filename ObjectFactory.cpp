// ObjectFactory.cpp
#include "ObjectFactory.hpp"
#include "helper/ObjectLoading/loadObj.hpp"
#include "helper/Texture/CubeMap.hpp"

#include "helper/Texture/Texture.hpp"

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
        _descriptorSetLayout
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
        _descriptorSetLayout
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
        _descriptorSetLayout
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
        "shaders/snow.vert.spv",
        "shaders/snow.frag.spv",
        renderPass,
        snowDescriptorSetLayout
    );

    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device,
                                                    _commandPool, _graphicsQueue, vertices);

    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, texturePath);

    RenderObject obj{};
    obj.vertexBuffer = vertexBuffer;
    obj.vertexCount = static_cast<uint32_t>(vertices.size());
    obj.textureImageView = tex->getImageView();
    obj.textureSampler = tex->getSampler();
    obj.pipeline = pipeline;
    obj.modelMatrix = glm::mat4(1.0f);
    obj.instanceBuffer = particleBuffer;    //Particle Buffer
    obj.instanceCount = NUMBER_PARTICLES;              //Anzahl Instanzen
    obj.isSnow= true;

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
    
    // Erstelle kleine Kugel zur Visualisierung
    std::vector<Vertex> sphereVertices;
    _loader.objLoader("models/teapot.obj", sphereVertices);
    
    
    // Pipeline für Lichtquelle (unlit, emissive)
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/testapp.vert.spv",    // Einfacher Vertex Shader
        "shaders/testapp.frag.spv",    // Emissive Fragment Shader
        renderPass,
        _descriptorSetLayout
    );
    
    VkBuffer vertexBuffer = _buff.createVertexBuffer(_physicalDevice, _device,
                                                    _commandPool, _graphicsQueue, sphereVertices);
    
    // Weiße Textur für Lichtquelle (oder eigene Glow-Textur)
    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, 
                              "textures/white.png");
    
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.2f));  // Kleine Kugel
    
    light.renderObject.vertexBuffer = vertexBuffer;
    light.renderObject.vertexCount = static_cast<uint32_t>(sphereVertices.size());
    light.renderObject.textureImageView = tex->getImageView();
    light.renderObject.textureSampler = tex->getSampler();
    light.renderObject.pipeline = pipeline;
    light.renderObject.modelMatrix = modelMatrix;
    light.renderObject.instanceCount = 1;
    light.renderObject.isSnow = false;
    light.renderObject.isLit = false;  // Lichtquelle selbst ist nicht beleuchtet
    
    return light;
}

RenderObject ObjectFactory::createLitObject(const char* modelPath,
                                          const char* texturePath,
                                          const glm::mat4& modelMatrix,
                                          VkRenderPass renderPass,
                                          VkDescriptorSetLayout litDescriptorSetLayout) {
    // Pipeline mit Lighting-Layout
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        "shaders/lit.vert.spv",
        "shaders/lit.frag.spv",
        renderPass,
        litDescriptorSetLayout
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
    
    return obj;
}