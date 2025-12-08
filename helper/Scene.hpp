#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "GraphicsPipeline.hpp"


class Scene {
public:

    void setRenderObject(GraphicsPipeline* pipeline, VkBuffer vertexBuffer, uint32_t vertexCount, VkImageView textureImageView, VkSampler textureSampler) {
        _pipeline = pipeline;
        _vertexBuffer = vertexBuffer;
        _vertexCount = vertexCount;
        _textureImageView = textureImageView;
        _textureSampler = textureSampler;
    }

    VkBuffer getVertexBuffer() {
        return _vertexBuffer;
    }
     void setVertexBuffer(VkBuffer vertexBuffer, int32_t vertexCount) {
        _vertexBuffer = vertexBuffer;
        _vertexCount = vertexCount;
    }

    uint32_t getVertexCount() {
        return _vertexCount;
    }

    VkImageView getImageView() {
        return _textureImageView;
    }

    VkSampler getSampler() {
        return _textureSampler;
    }

    VkRenderPass getRenderPass() {
        return _pipeline->getRenderPass();
    }

    VkPipeline getPipeline() {
        return _pipeline->getPipeline();
    }

    VkPipelineLayout getPipelineLayout() {
        return _pipeline->getPipelineLayout();
    }

private:
    GraphicsPipeline* _pipeline = nullptr;
    VkBuffer _vertexBuffer = VK_NULL_HANDLE;
    uint32_t _vertexCount = UINT32_MAX;
    VkImageView _textureImageView = VK_NULL_HANDLE;
    VkSampler _textureSampler = VK_NULL_HANDLE;

};