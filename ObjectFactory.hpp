// ObjectFactory.hpp 
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

class ObjectFactory {
public:
    ObjectFactory(VkPhysicalDevice physicalDevice, VkDevice device,
                 VkCommandPool commandPool, VkQueue graphicsQueue,
                 VkFormat colorFormat, VkFormat depthFormat,
                 VkDescriptorSetLayout descriptorSetLayout,
                 VkDescriptorSetLayout litDescriptorSetLayout)
        : _physicalDevice(physicalDevice), _device(device),
          _commandPool(commandPool), _graphicsQueue(graphicsQueue),
          _colorFormat(colorFormat), _depthFormat(depthFormat),
          _descriptorSetLayout(descriptorSetLayout),
          _litDescriptorSetLayout(litDescriptorSetLayout) {}
    
    //erstellt generische Objekte (Keine Beleuchtung, keine sonstigen gimmicks)
    RenderObject createGenericObject(const char* modelPath,
                                         const char* texturePath,
                                         const glm::mat4& modelMatrix, 
                                         VkRenderPass renderPass);

    // Neue Methode für deferred gerenderte Objekte
    DeferredRenderObject createDeferredObject(
        const char* modelPath,
        const char* texturePath,
        const glm::mat4& modelMatrix,
        VkRenderPass renderPass);

    //Erstellt die Skybox
    RenderObject createSkybox(VkRenderPass renderPass,const std::array<const char*, 6>& cubemapFaces);
    //Schnee (Compute-Shader)
    RenderObject createSnowflake(const char* texturePath,
                                VkRenderPass renderPass,
                                VkBuffer particleBuffer,
                                VkDescriptorSetLayout snowDescriptorSetLayout);
    //Spiegel (Stencil-Buffer)
    RenderObject createMirror(const glm::mat4& modelMatrix,
                             VkRenderPass renderPass,
                             PipelineType pipelineType);
    
                       
    //Erstellt Punktlichter 
    LightSourceObject createLightSource(const glm::vec3& position,
                                       const glm::vec3& color,
                                       float intensity,
                                       float radius,
                                       VkRenderPass renderPass);
    //Objekte, die von den Lichtern beleuchtet werden
    RenderObject createLitObject(const char* modelPath, const char* texturePath, const glm::mat4& modelMatrix, VkRenderPass renderPass);

    // Fullscreen Quad für Lighting Pass
    RenderObject createLightingQuad(VkRenderPass renderPass,
                                   VkDescriptorSetLayout lightingLayout);

    RenderObject createReflectiveObject(const char* modelPath, ReflectionProbe* probe, const glm::mat4& modelMatrix, VkRenderPass renderPass);

    RenderObject createGraffitti(glm::mat4& modelMatrix, VkRenderPass renderPass);
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
    std::array<const char*, 6> _graffittiTextures = {
        "textures/graffitti/161.png",
        "textures/graffitti/cg1.png",
        "textures/graffitti/fcknzs.png",
        "textures/graffitti/hsh.png",
        "textures/graffitti/ln.png",
        "textures/graffitti/sonne.png",
        
    };
};