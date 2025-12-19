#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>

struct alignas(32) Particle {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 velocity;
};

const uint32_t NUMBER_PARTICLES = 256;

class Snow {
public:
    Snow(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t queueIndex);
    
    VkCommandBuffer getCommandBuffer() { return _commandBuffer; }
    VkBuffer getCurrentBuffer() { return _currBuffer; }
    
    void destroy();

private:
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    uint32_t _computeQueueIndex;

    VkPipeline _computePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    VkBuffer _initBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _initBufferMemory = VK_NULL_HANDLE;

    VkBuffer _currBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _currBufferMemory = VK_NULL_HANDLE;

    VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;

    VkCommandPool _commandPool = VK_NULL_HANDLE;
    VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;

    void createDescriptorSetLayout();
    void createPipelineLayout();
    void createPipeline();
    void createStorageBuffers();
    void createDescriptorPool();
    void allocateDescriptorSet();
    void updateDescriptorSet();
    void createCommandPool();
    void allocateCommandBuffer();
    void recordCommandBuffer();
};