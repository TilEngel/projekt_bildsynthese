// ObjectFactory.cpp
#include "ObjectFactory.hpp"
#include "helper/loadObj.hpp"
#include "helper/initBuffer.hpp"
#include "helper/Texture.hpp"

RenderObject ObjectFactory::createTeapot(const char* modelPath,
                                         const char* vertShaderPath,
                                         const char* fragShaderPath,
                                         const char* texturePath,
                                         const glm::mat4& modelMatrix)
{
    // --- 1) Pipeline erstellen (mit eigenen Shaderpfaden) ---
    // Ändere ggf. GraphicsPipeline so, dass Shaderpfade als Parameter übergeben werden können.
    // Wenn deine GraphicsPipeline zurzeit feste shader-Dateinamen verwendet,
    // erweitere den Konstruktor um vert/frag path arguments.
    GraphicsPipeline* pipeline = new GraphicsPipeline(
        _device,
        _colorFormat,
        _depthFormat,
        vertShaderPath, // neu: vertex shader pfad
        fragShaderPath  // neu: fragment shader pfad
    );

    // --- 2) Model laden und VertexBuffer erzeugen ---
    LoadObj loader;
    std::vector<Vertex> vertices;
    loader.objLoader(modelPath, vertices);
    InitBuffer buff;
    VkBuffer vertexBuffer = buff.createVertexBuffer(_physicalDevice, _device, _commandPool, _graphicsQueue, vertices);
    // WICHTIG: buff muss so verwaltet werden, dass VertexBuffer-Lifetime bis Cleanup besteht.
    // Du kannst alternativ ownership in RenderObject dokumentieren bzw. globalen InitBuffer verwenden.

    // --- 3) Texture laden ---
    Texture* tex = new Texture(_physicalDevice, _device, _commandPool, _graphicsQueue, texturePath);

    // --- 4) Build RenderObject ---
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
                                         const glm::mat4& modelMatrix)
{
    // für dieses Beispiel identisch zur createTeapot-Implementierung
    return createTeapot(modelPath, vertShaderPath, fragShaderPath, texturePath, modelMatrix);
}
