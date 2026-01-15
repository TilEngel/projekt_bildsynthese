// Scene.hpp
#pragma once
#include <cstdint>
#include <vulkan/vulkan_core.h>
#include <vector>
#include <glm/glm.hpp>
#include "helper/Rendering/GraphicsPipeline.hpp"
#include "helper/Texture/Texture.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <unordered_set>

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
    alignas(16) glm::vec3 viewPos;
    alignas(4)  int numLights;
    PointLight lights[4];
};

struct RenderObject {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
    Texture* texture = nullptr;
    GraphicsPipeline* pipeline = nullptr;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    VkBuffer instanceBuffer = VK_NULL_HANDLE;
    uint32_t instanceCount = 1;
    bool isSnow = false;
    bool isLit = false;
    bool isDeferred = false; 
};

// Deferred Render Object - hat 2 Pipelines
struct DeferredRenderObject {
    RenderObject depthPass;    // Subpass 0
    RenderObject gbufferPass;  // Subpass 1
    size_t originalIndex;      // Index des ursprünglichen Objekts
};

// Lichtquellen-Objekt
struct LightSourceObject {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float radius;
    RenderObject renderObject; 
};
struct DeferredObjectInfo {
    size_t depthPassIndex;
    size_t gbufferPassIndex;
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
        if (obj.isDeferred) {
            _deferredObjectIndices.push_back(_objects.size());
        }
        _objects.push_back(obj);
    }
    
    //Deferred Objekt hinzufügen
    void setDeferredRenderObject(DeferredRenderObject& deferredObj) {
    // Depth Pass Object
    deferredObj.depthPass.isDeferred = true;
    _objects.push_back(deferredObj.depthPass);
    size_t depthIndex = _objects.size() - 1;
    
    // G-Buffer Pass Object
    deferredObj.gbufferPass.isDeferred = true;
    _objects.push_back(deferredObj.gbufferPass);
    size_t gbufferIndex = _objects.size() - 1;
    
    
    // Speichere die Indices
    DeferredObjectInfo info;
    info.depthPassIndex = depthIndex;
    info.gbufferPassIndex = gbufferIndex;
    _deferredObjectInfos.push_back(info);
    
    _deferredObjectIndices.push_back(depthIndex);
    _deferredObjectIndices.push_back(gbufferIndex);
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
            glm::mat4 model = glm::translate(glm::mat4(1.0f), newPos);
            _lights[index].renderObject.modelMatrix = model;
        }
    }
    
    const std::vector<LightSourceObject>& getLights() const { return _lights; }
    size_t getLightCount() const { return _lights.size(); }
    
    size_t getObjectCount() const { return _objects.size(); }
    size_t getSnowObjectCount() const { return _snowObjectIndices.size(); }
    size_t getNormalObjectCount() const { 
        return _objects.size() - _snowObjectIndices.size() - _litObjectIndices.size() 
               - _deferredObjectIndices.size(); 
    }
    size_t getLitObjectCount() const { return _litObjectIndices.size(); }
    size_t getDeferredObjectCount() const { return _deferredObjectInfos.size(); }
    
    size_t getLitDescriptorSetCount() const {
        // Lit objects die NICHT deferred sind
        size_t count = 0;
        for (size_t idx : _litObjectIndices) {
            if (!_objects[idx].isDeferred) {
                count++;
            }
        }
        return count;
    }

    size_t getNormalDescriptorSetCount() const {
        size_t count = 0;
        for (size_t i = 0; i < _objects.size(); i++) {
            if (!_objects[i].isSnow && !_objects[i].isLit && !_objects[i].isDeferred) {
                count++;
            }
        }
        return count;
    }

    size_t getDeferredDescriptorSetCount() const {
        // Deferred objects nutzen normale descriptor sets
        // Jedes deferred object hat 2 render objects (depth + gbuffer)
        return _deferredObjectInfos.size() * 2;
    }

    const RenderObject& getObject(size_t index) const { return _objects[index]; }
    RenderObject& getObjectMutable(size_t idx) { return _objects[idx]; }
    
    bool isSnowObject(size_t index) const {
        return std::find(_snowObjectIndices.begin(), _snowObjectIndices.end(), index) 
               != _snowObjectIndices.end();
    }
    
    bool isLitObject(size_t index) const {
        return std::find(_litObjectIndices.begin(), _litObjectIndices.end(), index) 
               != _litObjectIndices.end();
    }
    
    bool isDeferredObject(size_t index) const {
        return std::find(_deferredObjectIndices.begin(), _deferredObjectIndices.end(), index) 
               != _deferredObjectIndices.end();
    }
    
    // NEU: Hilfsmethoden für Deferred Rendering
    const DeferredObjectInfo& getDeferredInfo(size_t infoIndex) const {
        return _deferredObjectInfos[infoIndex];
    }
    
    const RenderObject& getDepthPassObject(size_t infoIndex) const {
        return _objects[_deferredObjectInfos[infoIndex].depthPassIndex];
    }
    
    const RenderObject& getGBufferPassObject(size_t infoIndex) const {
        return _objects[_deferredObjectInfos[infoIndex].gbufferPassIndex];
    }
    
    void updateObject(size_t idx, const glm::mat4& newModel) {
        if (idx < _objects.size()) {
            _objects[idx].modelMatrix = newModel;
        }
    }
    
    //Update deferred object (beide Passes gleichzeitig)
    void updateDeferredObject(size_t infoIndex, const glm::mat4& newModel) {
        if (infoIndex < _deferredObjectInfos.size()) {
            size_t depthIdx = _deferredObjectInfos[infoIndex].depthPassIndex;
            size_t gbufferIdx = _deferredObjectInfos[infoIndex].gbufferPassIndex;
            _objects[depthIdx].modelMatrix = newModel;
            _objects[gbufferIdx].modelMatrix = newModel;
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

    //Lighting Quad für deferred
    void setLightingQuad(const RenderObject& quad) {
        _lightingQuad = quad;
        _hasLightingQuad = true;
    }
    
    bool hasLightingQuad() const { return _hasLightingQuad; }
    const RenderObject& getLightingQuad() const { return _lightingQuad; }

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
    RenderObject& getReflectedObject(size_t idx){
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

    //Updatet reflektierte Objekte im Spiegel
    void updateReflectedObject(size_t index, const glm::mat4& newModel) {
        if (index < _reflectedObjects.size()) {
            _reflectedObjects[index].modelMatrix = newModel;
        }
    }

    //Render To Texture
    // Markiert ein Objekt als reflektierend (es selbst wird nicht in der Cubemap gerendert)
    void markObjectAsReflective(size_t index) {
        _reflectiveObjectIndices.insert(index);
    }

    bool isReflectiveObject(size_t index) const {
        return _reflectiveObjectIndices.find(index) != _reflectiveObjectIndices.end();
    }

    const std::unordered_set<size_t>& getReflectiveObjectIndices() const {
        return _reflectiveObjectIndices;
    }

    // Update-Frequenz für Reflexionen (nicht jeden Frame)
    void setReflectionUpdateInterval(uint32_t interval) {
        _reflectionUpdateInterval = interval;
    }

    uint32_t getReflectionUpdateInterval() const {
        return _reflectionUpdateInterval;
    }

private:
   

    std::vector<RenderObject> _objects;
    std::vector<LightSourceObject> _lights;
    std::vector<size_t> _snowObjectIndices;
    std::vector<size_t> _litObjectIndices;
    std::vector<size_t> _deferredObjectIndices;
    std::vector<DeferredObjectInfo> _deferredObjectInfos;
    
    // Lighting Quad
    RenderObject _lightingQuad;
    bool _hasLightingQuad = false;
    
    // Mirror data
    std::vector<RenderObject> _reflectedObjects;
    std::vector<size_t> _reflectedDescriptorIndices;
    std::vector<size_t> _mirrorMarkIndices;
    std::vector<size_t> _mirrorBlendIndices;
    std::unordered_set<size_t> _reflectableObjectIndices;

    //Render to texture
    std::unordered_set<size_t> _reflectiveObjectIndices;
    uint32_t _reflectionUpdateInterval = 10; // Alle 10 Frames updaten
    
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
};