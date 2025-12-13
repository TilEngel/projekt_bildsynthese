// ObjectFactory.cpp
#include "ObjectFactory.hpp"
#include "helper/loadObj.hpp"
#include "helper/initBuffer.hpp"
#include "helper/Texture.hpp"

RenderObject ObjectFactory::createTeapot(const char* modelPath,
                                         const char* vertShaderPath,
                                         const char* fragShaderPath,
                                         const char* texturePath,
                                         const glm::mat4& modelMatrix, VkRenderPass renderPass)
{
    //eigene Pipeline erstellen
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        vertShaderPath, 
        fragShaderPath,
        renderPass
    );

    //Model laden & Vertexbuffer erzeugen
    LoadObj loader;
    std::vector<Vertex> vertices;
    loader.objLoader(modelPath, vertices);
    InitBuffer buff;
    VkBuffer vertexBuffer = buff.createVertexBuffer(_physicalDevice, _device, _commandPool, _graphicsQueue, vertices);


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

RenderObject ObjectFactory::createFlyingDutchman(const char* modelPath,
                                         const char* vertShaderPath,
                                         const char* fragShaderPath,
                                         const char* texturePath,
                                         const glm::mat4& modelMatrix,
                                        VkRenderPass renderPass){
    //theoretisch kann man jetzt was neues machen (eigener Shader oder Textur und so)
    return createTeapot(modelPath, vertShaderPath, fragShaderPath, texturePath, modelMatrix,renderPass);
}
