// Scene.hpp (angepasst)
#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>
#include <vector>
#include <glm/glm.hpp>
#include "helper/Rendering/GraphicsPipeline.hpp"

struct RenderObject {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
    GraphicsPipeline* pipeline = nullptr; // pipeline für dieses Objekt
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    VkBuffer instanceBuffer = VK_NULL_HANDLE; //für Instancing
    uint32_t instanceCount = 200;
    bool isSnow = false;
};

class Scene {
public:
    void setRenderObject(RenderObject obj) {
        if (obj.isSnow) {
            _snowObjectIndices.push_back(_objects.size());
        }
        _objects.push_back(obj);
    }

    size_t getObjectCount() const { return _objects.size(); }
    size_t getSnowObjectCount() const { return _snowObjectIndices.size(); }
    size_t getNormalObjectCount() const { return _objects.size() - _snowObjectIndices.size(); }
    
    const RenderObject& getObject(size_t index) const { return _objects[index]; }
    const std::vector<size_t>& getSnowIndices() const { return _snowObjectIndices; }
    
    bool isSnowObject(size_t index) const {
        return std::find(_snowObjectIndices.begin(), _snowObjectIndices.end(), index) != _snowObjectIndices.end();
    }

    void updateObject(size_t idx, const glm::mat4& newModel) {
        if (idx < _objects.size()) {
            _objects[idx].modelMatrix = newModel;
        }
    }
    
    RenderObject& getObjectMutable(size_t idx) { 
        return _objects[idx]; 
    }

    // Hilfsfunktionen: falls du noch einen "default" RenderPass/Pipeline brauchst,
    // kannst du z.B. das erste Objekt als Referenz nehmen:
    VkRenderPass getRenderPass() const {
        if (_objects.empty() || !_objects[0].pipeline) return VK_NULL_HANDLE;
        return _objects[0].pipeline->getRenderPass();
    }
    // Für alte Aufrufe: liefert Pipeline des ersten Objekts (nur falls nötig).
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
    std::vector<size_t> _snowObjectIndices;
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
};
