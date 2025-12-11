#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>
#include <vector>

#include "GraphicsPipeline.hpp"

struct RenderObject {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
    // optional: per-object user data (z.B. transform index)
};

class Scene {
public:
    void setRenderObject(GraphicsPipeline* pipeline,
                         VkBuffer vertexBuffer,
                         uint32_t vertexCount,
                         VkImageView textureImageView,
                         VkSampler textureSampler)
    {
        if (!_pipeline)
            _pipeline = pipeline;

        RenderObject obj{};
        obj.vertexBuffer = vertexBuffer;
        obj.vertexCount = vertexCount;
        obj.textureImageView = textureImageView;
        obj.textureSampler = textureSampler;
        _objects.push_back(obj);
    }

    size_t getObjectCount() const { return _objects.size(); }
    const RenderObject& getObject(size_t idx) const { return _objects[idx]; }

    VkRenderPass getRenderPass() const { return _pipeline->getRenderPass(); }
    VkPipeline getPipeline() const { return _pipeline->getPipeline(); }
    VkPipelineLayout getPipelineLayout() const { return _pipeline->getPipelineLayout(); }

private:
    GraphicsPipeline* _pipeline = nullptr;
    std::vector<RenderObject> _objects;
};
