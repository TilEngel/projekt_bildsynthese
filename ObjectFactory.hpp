#pragma once

#include <string>
#include <glm/glm.hpp>
#include "helper/Rendering/GraphicsPipeline.hpp"
#include "Scene.hpp"
#include "helper/initBuffer.hpp"
#include "helper/ObjectLoading/loadObj.hpp"
#include "helper/Compute/Snow.hpp"
#include "helper/MirrorSystem.hpp"
#include "helper/renderToTexture/ReflectionProbe.hpp"
#include <array>
#include <random>

/**
 * Verantwortlich für die Erzeugung von RenderObjekts.
 * Erstellt Objekte mit evtl. speziellen Eigenschaften, die der Szene hinzugefügt werden können 
 * anhand der übergebenen ModelMatrix, obj-File, Textur etc..
 */

class ObjectFactory {
public:
    ObjectFactory(VkPhysicalDevice physicalDevice, VkDevice device,VkCommandPool commandPool, VkQueue graphicsQueue,VkFormat colorFormat, VkFormat depthFormat,
                 VkDescriptorSetLayout descriptorSetLayout,VkDescriptorSetLayout litDescriptorSetLayout)
        : _physicalDevice(physicalDevice), _device(device), _commandPool(commandPool), _graphicsQueue(graphicsQueue), _colorFormat(colorFormat),
        _depthFormat(depthFormat), _descriptorSetLayout(descriptorSetLayout), _litDescriptorSetLayout(litDescriptorSetLayout) {}


    //erstellt generische Objekte (Keine Beleuchtung, keine sonstigen gimmicks)
    RenderObject createGenericObject(const char* modelPath,
                                         const char* texturePath,
                                         const glm::mat4& modelMatrix, 
                                         VkRenderPass renderPass);

    // Neue Methode für deferred gerenderte Objekte
    DeferredRenderObject createDeferredObject(const char* modelPath,const char* texturePath,
                                            const glm::mat4& modelMatrix,VkRenderPass renderPass);

    //Erstellt die Skybox
    RenderObject createSkybox(VkRenderPass renderPass,const std::array<const char*, 6>& cubemapFaces);

    //Schnee (Compute-Shader)
    RenderObject createSnowflake(const char* texturePath,VkRenderPass renderPass,
                                VkBuffer particleBuffer,VkDescriptorSetLayout snowDescriptorSetLayout);
    //Spiegel (Stencil-Buffer)
    RenderObject createMirror(const glm::mat4& modelMatrix,VkRenderPass renderPass, PipelineType pipelineType);
    
                       
    //Erstellt Punktlichter 
    LightSourceObject createLightSource(const glm::mat4& model,const glm::vec3& color, float intensity,
                                       float radius, VkRenderPass renderPass);

    //Objekte, die von den Lichtern beleuchtet werden
    RenderObject createLitObject(const char* modelPath, const char* texturePath, const glm::mat4& modelMatrix, VkRenderPass renderPass);

    // Fullscreen Quad für Lighting Pass (deferred Rendering)
    RenderObject createLightingQuad(VkRenderPass renderPass, VkDescriptorSetLayout lightingLayout);

    // Spiegelung der Szene (Render-to-Texture)
    RenderObject createReflectiveObject(const char* modelPath, ReflectionProbe* probe, const glm::mat4& modelMatrix, VkRenderPass renderPass);

    //Erstellt ein 2D Viereck mit zufälliger Graffitti-Textur
    RenderObject createGraffitti(glm::mat4& modelMatrix, VkRenderPass renderPass);

    //Objekt, dass mit tessellation-Shader gerendert wird
    RenderObject createTessellatedObject(const char* modelPath, const char* texturePath, const glm::mat4& modelMatrix, VkRenderPass renderPass);

    //Objekt, dessen Vertices nur als Linien gezeichnet werden (zum Vergleich mit Tessellation)
    RenderObject createPolygonLineObject(const char* modelPath,const char* texturePath,const glm::mat4& modelMatrix, VkRenderPass renderPass);
private:
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VkCommandPool _commandPool;
    VkQueue _graphicsQueue;
    VkFormat _colorFormat;
    VkFormat _depthFormat;
    VkDescriptorSetLayout _descriptorSetLayout;
    VkDescriptorSetLayout _litDescriptorSetLayout;
    InitBuffer _buff;
    LoadObj _loader;

    std::array<const char*, 6> _graffittiTextures = {   //die verschiedenen Grafitti-Texturen
        "textures/graffitti/161.png",
        "textures/graffitti/cg1.png",
        "textures/graffitti/fcknzs.png",
        "textures/graffitti/hsh.png",
        "textures/graffitti/ln.png",
        "textures/graffitti/sonne.png",   
    };
};