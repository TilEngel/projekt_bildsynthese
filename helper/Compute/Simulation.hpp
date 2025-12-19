//Simulation.hpp
#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../initBuffer.hpp"
#include <vulkan/vulkan.h>


struct alignas(32) Particle {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 velocity;
};

const uint32_t NUMBER_PARTICLES = 200;


class Simulation {
public:
    Simulation(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t queueIndex) 
        : _physicalDevice(physicalDevice)
        , _device(device)
        , _computeQueueIndex(queueIndex) {
        createDescriptorSetLayout();
        createPipelineLayout();
        createPipeline();
        createStorageBuffers();
        createDescriptorPool();
        allocateDescriptorSet();
        updateDescriptorSet();
        createCommandPool();
        allocateCommandBuffer();
        recordCommandBuffer();
    }

    VkCommandBuffer getCommandBuffer() {
        return _commandBuffer;
    }

    VkDescriptorBufferInfo getBufferInfo() {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = _currBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(Particle) * NUMBER_PARTICLES;
        return bufferInfo;
    }

    // Cleanup everything
    // - Destroy created/allocated objects
    // - Free allocated memory
    void destroy();
    InitBuffer _buff;
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

    // Create a descriptor set layout for the compute pipeline
    // - The compute shader needs access to two storage buffers.
    // - Save the layout in the member variable _descriptorSetLayout.
    void createDescriptorSetLayout();

    // Create the pipeline layout
    // - Use the descriptor set layout from _descriptorSetLayout.
    // - Save the layout in _pipelineLayout.
    void createPipelineLayout();

    // Create a compute pipeline
    // - Load the compute shader from file shaders/testapp.comp.spv.
    // - Create the shader module.
    // - Create the compute pipeline using the shader module and
    //   the layout from _pipelineLayout.
    // - Save the pipeline in the member variable _computePipeline.
    // - Destroy the shader module.
    void createPipeline();

    // Create storage buffers used by compute shader
    // - Fill std::vector<Particle> with 256 particles.
    // - Each particle should have position (0, 0, 0) and a random
    //   velocity in ([-0.25, 0.25], [2, 2.5], [-0.25, 0.25]).
    // - Create two storage buffers and fill them with the particle
    //   data.
    // - Use _initBuffer and _initBufferMemory for the first buffer.
    // - Use _currBuffer and _currBufferMemory for the second buffer.
    void createStorageBuffers();

    // Create a descriptor pool for both storage buffers
    // - Save the descriptor pool in _descriptorPool.
    void createDescriptorPool();

    // Allocate a descriptor set for the descriptor set layout
    // - Use _descriptorPool and _descriptorSetLayout.
    // - Save the descriptor set in _descriptorSet.
    void allocateDescriptorSet();

    // Update descriptor set with buffer data
    // - Use descriptor set from _descriptorSet.
    // - Write descriptor set from both storage buffers
    //  (_initBuffer and _currBuffer).
    void updateDescriptorSet();

    // Create a command pool for the compute queue
    // - Use the queue family index from _computeQueueIndex.
    // - Save the command pool in _commandPool.
    void createCommandPool();

    // Allocate a command buffer
    // - Save the command buffer in _commandBuffer.
    void allocateCommandBuffer();

    // Record the command buffer
    // - Set flag VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT.
    // - Bind the compute pipeline from _computePipeline.
    // - Bind the descriptor set from _descriptorSet with layout
    //   from _pipelineLayout.
    // - Send a dispatch command with group counts 1, 1, 1.
    void recordCommandBuffer();

};

