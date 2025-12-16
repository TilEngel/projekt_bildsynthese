// ObjectFactory.cpp
#include "ObjectFactory.hpp"
#include "helper/loadObj.hpp"

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
