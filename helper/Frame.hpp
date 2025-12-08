#pragma once

#include <vulkan/vulkan_core.h>
#include "Framebuffers.hpp"
#include "Scene.hpp"
#include "Swapchain.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Frame {
public:
    Frame(VkPhysicalDevice physicalDevice, VkDevice device,
        SwapChain* swapChain, Framebuffers* framebuffers, VkQueue graphicsQueue,
        VkCommandPool commandPool, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
    : _physicalDevice(physicalDevice)
    , _device(device)
    , _swapChain(swapChain) 
    , _framebuffers(framebuffers)
    , _graphicsQueue(graphicsQueue) {
        createUniformBuffer();
        allocateDescriptorSet(descriptorSetLayout, descriptorPool);
        allocateCommandBuffer(commandPool);
        createSyncObjects();
    }
    
    ~Frame() {
        cleanup();
    }

    bool render(Scene* scene)  {
        // wait for fence
        waitForFence();

        // acquire next image
        uint32_t imageIndex = _swapChain->acquireNextImage(_renderSemaphore, VK_NULL_HANDLE);
        if (imageIndex == UINT32_MAX) {
            return true;
        }
        
        // update descriptor set
        updateDescriptorSet(scene);

        // update uniform buffer
        updateUniformBuffer();

        // record command buffer
        recordCommandBuffer(scene, imageIndex);
        
        // submit command buffer
        submitCommandBuffer(imageIndex);

        // present image
        bool recreate = _swapChain->presentImage(imageIndex);
        return recreate;
    }

private:
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    SwapChain* _swapChain = nullptr;
    Framebuffers* _framebuffers = nullptr;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;

    VkBuffer _uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _uniformBufferMemory = VK_NULL_HANDLE;
    UniformBufferObject* _uniformBufferMapped = nullptr;

    VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;

    VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;

    VkSemaphore _renderSemaphore = VK_NULL_HANDLE;
    VkFence _inFlightFence = VK_NULL_HANDLE;

    // create a buffer for UniformBufferObject
    // - create the buffer object
    //   - use _uniformBuffer to store the buffer object
    //   - size of UniformBufferObject
    //   - usage as uniform buffer)
    // - allocate memory for buffer
    //   - use _uniformBufferMemory to store the memory object
    //   - memory has to be host visible and should be host coherent
    // - bind memory to buffer
    // - map memory
    //   - use _uniformBufferMapped with static_cast to store the pointer
    void createUniformBuffer();

    // allocate a descriptor set
    // - use _descriptorSet to store the descriptor set
    void allocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool);
    
    // allocate a command buffer
    // - use _commandBuffer to store the command buffer
    void allocateCommandBuffer(VkCommandPool commandPool);

    // create the objects needed for synchronization
    // - create a semaphore for rendering
    //   - use _renderSemaphore to store the semaphore
    // - create a fence to synchronize with device
    //   - create the fence signaled
    //   - use _inFlightFence to store the fence
    void createSyncObjects();

    // destroy all created objects
    // - destroy _uniformBuffer
    // - free _uniformBufferMemory
    // - destroy _renderSemaphore
    // - destroy _inFlightFence
    void cleanup();

    // wait for _inFlightFence
    void waitForFence();

    // update _descriptorSet
    // - write _uniformBuffer to binding 0
    //   - descriptor type is "uniform buffer"
    // - write the image view and sampler from the scene object to binding 1
    //   - image layout is "shader read-only optimal"
    //   - descriptor type is "combined image sampler"
    void updateDescriptorSet(Scene* scene);

    // update the uniform buffer
    // - use _uniformBufferMapped to write to the buffer
    // - rotate the object in the model matrix
    //   (animation time could be obtained by calling glfwGetTime())
    // - set a view matrix such that the object is visible
    // - use glm::perspectiveFovRH_ZO(...) to set the projection matrix
    //   (do not forget to proj[1][1] *= -1)
    void updateUniformBuffer();

    // record the command buffer _commandBuffer
    // - reset the command buffer
    // - begin recording (call vkBeginCommandBuffer)
    // - begin a render pass
    //   - get render pass by calling scene->getRenderPass()
    //   - get framebuffer by calling _framebuffers->getFramebuffer(imageIndex)
    // - bind graphics pipeline
    //   - get pipeline by calling scene->getPipeline()
    // - set viewport
    // - set scissor
    // - bind descriptor set _descriptorSet
    //   - get pipeline layout by calling scene->getPipelineLayout()
    // - bind vertex buffer
    //   - get vertex buffer by calling scene->getVertexBuffer()
    // - draw
    //   - get vertex count by calling scene->getVertexCount()
    // - end render pass
    // - end recording (call vkEndCommandBuffer)
    void recordCommandBuffer(Scene* scene, uint32_t imageIndex);

    // submit the command buffer to the graphics queue
    // - reset _inFlightFence before using it as fence
    // - use _swapChain->getPresentationSemaphore(imageIndex) as signal semaphore
    // - use _renderSemaphore as wait semaphore (wait at "top of pipe" stage)
    // - use _commandBuffer as command buffer
    // - use _graphics queue as queue
    // - call vkQueueSubmit
    void submitCommandBuffer(uint32_t imageIndex);
};
