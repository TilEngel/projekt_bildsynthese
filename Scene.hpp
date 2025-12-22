#pragma once
#include <cstdint>
#include <vulkan/vulkan_core.h>
#include <vector>
#include <glm/glm.hpp>
#include "helper/Rendering/GraphicsPipeline.hpp"
#include <glm/gtc/matrix_transform.hpp>

// Licht-Daten für Shader
struct PointLight {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 color;     
    alignas(4)  float intensity;  
    alignas(4)  float radius;       
};

// Uniform Buffer Object für beleuchtete Objekte
struct LitUniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 viewPos;      // Kamera-Position für Specular
    alignas(4)  int numLights;          // Anzahl Lichter
    PointLight lights[4];                // Max. 4 Punktlichter
};

struct RenderObject {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
    GraphicsPipeline* pipeline = nullptr;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    VkBuffer instanceBuffer = VK_NULL_HANDLE;
    uint32_t instanceCount = 1;
    bool isSnow = false;
    bool isLit = false;  
};

// Lichtquellen-Objekt
struct LightSourceObject {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float radius;
    RenderObject renderObject; 
};

class Scene {
public:
    void setRenderObject(RenderObject obj) {
        if (obj.isSnow) {
            _snowObjectIndices.push_back(_objects.size());
        }
        if (obj.isLit) {
            _litObjectIndices.push_back(_objects.size());
        }
        _objects.push_back(obj);
    }
    
    void addLightSource(const LightSourceObject& light) {
        if (_lights.size() >= 4) {
            return;
        }
        _lights.push_back(light);
    }
    
    void updateLightPosition(size_t index, const glm::vec3& newPos) {
        if (index < _lights.size()) {
            _lights[index].position = newPos;
            // Update auch das visuelle Objekt
            glm::mat4 model = glm::translate(glm::mat4(1.0f), newPos);
            _lights[index].renderObject.modelMatrix = model;
        }
    }
    
    const std::vector<LightSourceObject>& getLights() const { return _lights; }
    size_t getLightCount() const { return _lights.size(); }
    
    size_t getObjectCount() const { return _objects.size(); }
    size_t getSnowObjectCount() const { return _snowObjectIndices.size(); }
    size_t getNormalObjectCount() const { 
        return _objects.size() - _snowObjectIndices.size() - _litObjectIndices.size(); 
    }
    size_t getLitObjectCount() const { return _litObjectIndices.size(); }
    
    const RenderObject& getObject(size_t index) const { return _objects[index]; }
    
    bool isSnowObject(size_t index) const {
        return std::find(_snowObjectIndices.begin(), _snowObjectIndices.end(), index) 
               != _snowObjectIndices.end();
    }
    
    bool isLitObject(size_t index) const {
        return std::find(_litObjectIndices.begin(), _litObjectIndices.end(), index) 
               != _litObjectIndices.end();
    }
    
    void updateObject(size_t idx, const glm::mat4& newModel) {
        if (idx < _objects.size()) {
            _objects[idx].modelMatrix = newModel;
        }
    }
    
    VkRenderPass getRenderPass() const {
        if (_objects.empty() || !_objects[0].pipeline) return VK_NULL_HANDLE;
        return _objects[0].pipeline->getRenderPass();
    }
    
    VkPipeline getPipeline() const {
        if (_objects.empty() || !_objects[0].pipeline) return VK_NULL_HANDLE;
        return _objects[0].pipeline->getPipeline();
    }
    
    VkPipelineLayout getPipelineLayout(size_t objectIndex = 0) const {
        if (_objects.empty() || !_objects[objectIndex].pipeline) return VK_NULL_HANDLE;
        return _objects[objectIndex].pipeline->getPipelineLayout();
    }
    
    void setDescriptorSetLayout(VkDescriptorSetLayout layout) {
        _descriptorSetLayout = layout;
    }
    
    VkDescriptorSetLayout getDescriptorSetLayout() const {
        return _descriptorSetLayout;
    }

private:
    std::vector<RenderObject> _objects;
    std::vector<LightSourceObject> _lights;
    std::vector<size_t> _snowObjectIndices;
    std::vector<size_t> _litObjectIndices;
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
};