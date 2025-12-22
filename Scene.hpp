// Scene.hpp
#pragma once
#include <cstdint>
#include <vulkan/vulkan_core.h>
#include <vector>
#include <unordered_set>
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
    
    RenderObject& getObjectMutable(size_t idx) { 
        return _objects[idx]; 
    }

    size_t getObjectCount() const { return _objects.size(); }
    const RenderObject& getObject(size_t idx) const { return _objects[idx]; }

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

    // Mirror-spezifische Methoden
    void setMirrorMarkObject(const RenderObject& obj) {
        _mirrorMarkIndices.push_back(_objects.size());
        _objects.push_back(obj);
    }

    void setMirrorBlendObject(const RenderObject& obj) {
        _mirrorBlendIndices.push_back(_objects.size());
        _objects.push_back(obj);
    }

    void addReflectedObject(const RenderObject& obj, size_t originalIndex) {
        _reflectedObjects.push_back(obj);
        _reflectedDescriptorIndices.push_back(originalIndex);
    }

    const std::vector<size_t>& getMirrorMarkIndices() const { 
        return _mirrorMarkIndices; 
    }

    const std::vector<size_t>& getMirrorBlendIndices() const { 
        return _mirrorBlendIndices; 
    }

    // Deprecated - für Backwards-Kompatibilität
    size_t getMirrorMarkIndex() const { 
        return _mirrorMarkIndices.empty() ? SIZE_MAX : _mirrorMarkIndices[0]; 
    }

    size_t getMirrorBlendIndex() const { 
        return _mirrorBlendIndices.empty() ? SIZE_MAX : _mirrorBlendIndices[0]; 
    }
    
    size_t getReflectedObjectCount() const { 
        return _reflectedObjects.size(); 
    }

    const RenderObject& getReflectedObject(size_t idx) const { 
        return _reflectedObjects[idx]; 
    }

    size_t getReflectedDescriptorIndex(size_t idx) const {
        return _reflectedDescriptorIndices[idx];
    }

    bool isMirrorObject(size_t idx) const {
        for (size_t markIdx : _mirrorMarkIndices) {
            if (idx == markIdx) return true;
        }
        for (size_t blendIdx : _mirrorBlendIndices) {
            if (idx == blendIdx) return true;
        }
        return false;
    }

    bool isReflectedObject(size_t idx) const {
        return _reflectableObjectIndices.find(idx) != _reflectableObjectIndices.end();
    }

    void markObjectAsReflectable(size_t idx) {
        _reflectableObjectIndices.insert(idx);
    }

private:
    std::vector<RenderObject> _objects;
    std::vector<RenderObject> _reflectedObjects;
    std::vector<size_t> _reflectedDescriptorIndices;
    
    std::vector<size_t> _mirrorMarkIndices;
    std::vector<size_t> _mirrorBlendIndices;
    std::unordered_set<size_t> _reflectableObjectIndices;
    
    std::vector<LightSourceObject> _lights;
    std::vector<size_t> _snowObjectIndices;
    std::vector<size_t> _litObjectIndices;
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
};