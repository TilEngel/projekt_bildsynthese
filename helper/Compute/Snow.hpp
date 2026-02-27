#pragma once

#include <cstdint>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>

//Postition und geschw. der einzelnen Schneeflocken
struct alignas(32) Particle {
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 velocity;
};

const uint32_t NUMBER_PARTICLES = 256;

/**
 * Verwaltet Partikelsimulation mit Compute-Shader,
 * um Schneeflocken in der Szene darzustellen
 */
class Snow {
public:
    Snow(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t queueIndex);
    
    //gibt CommandBuffer mit Compute-Operationen zurück
    VkCommandBuffer getCommandBuffer() { return _commandBuffer; }
    //Aktueller Buffer mit Partikelpositionen
    VkBuffer getCurrentBuffer() { return _currBuffer; }
    //Gibt Fence für synchonisierung zurück
    VkFence getComputeFence() { return _computeFence; } 
    void waitForCompute();
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

    VkFence _computeFence = VK_NULL_HANDLE;
    //Erstellt Fence für synchro
    void createComputeFence();
    //Erstellt DSL für StorageBuffers
    void createDescriptorSetLayout();
    //Erstellt PipelineLO für comp-shader
    void createPipelineLayout();
    //läd comp-shader und erstellt Pipeline
    void createPipeline();
    //Erstellt SBs mit rndm Werten für Schneeflocken
    void createStorageBuffers();
    // Erstellt DP für Storage Buffer Bindings
    void createDescriptorPool();
    //Allokiert DS aus Pool
    void allocateDescriptorSet();
    //Verbindet SBs mit DSs
    void updateDescriptorSet();
    //Erstellt CP für compute-Befehle
    void createCommandPool();
    //allokiert CB aus Pool
    void allocateCommandBuffer();
    //Zeichnet Compute-Befehle in CB auf
    void recordCommandBuffer();
};