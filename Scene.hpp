// Scene.hpp
#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>
#include <vector>
#include <unordered_set>
#include <glm/glm.hpp>
#include "helper/Rendering/GraphicsPipeline.hpp"

struct RenderObject {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
    GraphicsPipeline* pipeline = nullptr;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
};

class Scene {
public:
    void setRenderObject(const RenderObject& obj) {
        _objects.push_back(obj);
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
    
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
};
