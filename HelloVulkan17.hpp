#pragma once

#include <cstdio>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <cstddef>
#include <cassert>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#include"/Users/tilengelbrecht/VulkanSDK/1.4.321.0/macOS/include/glm/glm.hpp"
#include "/Users/tilengelbrecht/VulkanSDK/1.4.321.0/macOS/include/glm/gtc/matrix_transform.hpp"
#define GLFW_INCLUDE_VULKAN
#include <glfw/3.4/include/GLFW/glfw3.h>

#include "./helper/Window.hpp"
#include "./helper/Swapchain.hpp"
#include "./helper/Depthbuffer.hpp"
#include "./helper/GraphicsPipeline.hpp"
#include "./helper/Framebuffers.hpp"


// ----------------------------------------------------------------------------
// HelloVulkanApplication
// ----------------------------------------------------------------------------

class HelloVulkanApplication {
public:
    HelloVulkanApplication() {
    }

    ~HelloVulkanApplication() {
        cleanup();
    }

    void setWindow(Window* window) {
        _window = window;
    }

    void setPhysicalDevice(VkPhysicalDevice physicalDevice) {
        _physicalDevice = physicalDevice;
    }

    void setGraphicsQueue(VkQueue queue) {
        _graphicsQueue = queue;
    }

    void setLogicalDevice(VkDevice device) {
        _device = device;
    }

    void setSwapChain(SwapChain* swapChain) {
        _swapChain = swapChain;
    }

    void setDepthBuffer(DepthBuffer* depthBuffer) {
        _depthBuffer = depthBuffer;
    }

    void setGraphicsPipeline(GraphicsPipeline* pipeline) {
        _graphicsPipeline = pipeline;
    }

    void setFramebuffers(Framebuffers* framebuffers) {
        _framebuffers = framebuffers;
    }

    void setCommandPool(VkCommandPool commandPool) {
        _commandPool = commandPool;
    }

    void setVertexBuffer(VkBuffer vertexBuffer, int32_t vertexCount) {
        _vertexBuffer = vertexBuffer;
        _vertexCount = vertexCount;
    }

    void setTexture(VkImageView imageView, VkSampler sampler) {
        _textureImageView = imageView;
        _textureSampler = sampler;
    }

    void prepareRendering() {
        createDescriptorPool();
        createUniformBuffers();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void render() {
        drawFrame();
    }


private:

    Window* _window = nullptr;
    SwapChain* _swapChain = nullptr;
    DepthBuffer* _depthBuffer = nullptr;
    Framebuffers* _framebuffers = nullptr;

    GraphicsPipeline* _graphicsPipeline = nullptr;

    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;

    VkQueue _graphicsQueue = VK_NULL_HANDLE;

    VkBuffer _vertexBuffer = VK_NULL_HANDLE;
    uint32_t _vertexCount = 0;

    VkImageView _textureImageView = VK_NULL_HANDLE;
    VkSampler _textureSampler = VK_NULL_HANDLE;

    VkCommandPool _commandPool = VK_NULL_HANDLE;
    VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;

    const size_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t _currentFrame = 0;
  
    std::vector<VkBuffer> _uniformBuffers;              // per frame
    std::vector<VkDeviceMemory> _uniformBuffersMemory;  // per frame
    std::vector<void*> _uniformBuffersMapped;           // per frame

    std::vector<VkDescriptorSet> _descriptorSets;       // per frame
    std::vector<VkCommandBuffer> _commandBuffers;       // per frame

    std::vector<VkSemaphore> _imageAvailableSemaphores; // per frame
    std::vector<VkFence> _inFlightFences;               // per frame


    
    void cleanup() {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(_device, _uniformBuffers[i], nullptr);
            vkFreeMemory(_device, _uniformBuffersMemory[i], nullptr);
        }
        
        vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(_device, _inFlightFences[i], nullptr);
        }
    }


    void recreateSwapChain() {
        assert(_device != VK_NULL_HANDLE);
        assert(_swapChain != nullptr);
        assert(_depthBuffer != nullptr);
        assert(_framebuffers != nullptr);

        vkDeviceWaitIdle(_device);
        _swapChain->recreate();
        _depthBuffer->recreate(_swapChain->getExtent());
        _framebuffers->recreate();
    }

    
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(_device, buffer, bufferMemory, 0);
    }


    void createDescriptorPool() {
        assert(_device != VK_NULL_HANDLE);

        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }


    void createDescriptorSets() {
        assert(_textureImageView != VK_NULL_HANDLE);
        assert(_textureSampler != VK_NULL_HANDLE);

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _graphicsPipeline->getDescriptorSetLayout());
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = _descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        _descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(_device, &allocInfo, _descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = _uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = _descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;
            descriptorWrite.pImageInfo = nullptr; // Optional
            descriptorWrite.pTexelBufferView = nullptr; // Optional

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = _textureImageView;
            imageInfo.sampler = _textureSampler;

            VkWriteDescriptorSet samplerDescriptorWrite{};
            samplerDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            samplerDescriptorWrite.dstSet = _descriptorSets[i];
            samplerDescriptorWrite.dstBinding = 1;
            samplerDescriptorWrite.dstArrayElement = 0;
            samplerDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerDescriptorWrite.descriptorCount = 1;
            samplerDescriptorWrite.pImageInfo = &imageInfo;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites = {descriptorWrite, samplerDescriptorWrite};
            vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }


    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        _uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        _uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        _uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _uniformBuffers[i], _uniformBuffersMemory[i]);

            vkMapMemory(_device, _uniformBuffersMemory[i], 0, bufferSize, 0, &_uniformBuffersMapped[i]);
        }
    }


    void createCommandBuffers() {
        assert(_device != VK_NULL_HANDLE);
        assert(_commandPool != VK_NULL_HANDLE);

        _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = _commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) _commandBuffers.size();

        if (vkAllocateCommandBuffers(_device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }


    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        assert(_swapChain != nullptr);
        assert(_vertexBuffer != VK_NULL_HANDLE);
        
        VkExtent2D extent = _swapChain->getExtent();

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = _graphicsPipeline->getRenderPass();
        renderPassInfo.framebuffer = _framebuffers->getFramebuffer(imageIndex);
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline->getPipeline());

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float) extent.width;
            viewport.height = (float) extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = extent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline->getPipelineLayout(), 0, 1, &_descriptorSets[_currentFrame], 0, nullptr);

            VkBuffer vertexBuffers[] = {_vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdDraw(commandBuffer, _vertexCount, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }


    void createSyncObjects() {
        _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }


    void updateUniformBuffer(uint32_t currentImage) {
        assert(_swapChain != nullptr);

        VkExtent2D extent = _swapChain->getExtent();
        float time = glfwGetTime();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.model = glm::rotate(ubo.model, time * glm::radians(50.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ubo.view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));
        ubo.proj = glm::perspectiveFovRH_ZO(glm::radians(45.0f), (float) extent.width, (float) extent.height, 0.1f, 100.0f);
        ubo.proj[1][1] *= -1;

        memcpy(_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }


    void drawFrame() {
        assert(_device != VK_NULL_HANDLE);
        assert(_graphicsQueue != VK_NULL_HANDLE);
        
        vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(_device, 1, &_inFlightFences[_currentFrame]);
        
        uint32_t imageIndex = _swapChain->acquireNextImage(_imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE);
        if (imageIndex == UINT32_MAX) {
            recreateSwapChain();
            return;
        }
        
        
        updateUniformBuffer(_currentFrame);
        
        vkResetCommandBuffer(_commandBuffers[_currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(_commandBuffers[_currentFrame], imageIndex);
        
        std::vector<VkSemaphore> waitSemaphores = {_imageAvailableSemaphores[_currentFrame]};
        std::vector<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

        VkSemaphore presentationSemaphore = _swapChain->getPresentationSemaphore(imageIndex);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_commandBuffers[_currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &presentationSemaphore;
        if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        
        bool recreate = _swapChain->presentImage(imageIndex);
        if (recreate || _window->wasResized()) {
            recreateSwapChain();
        }

        _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    uint32_t findMemoryType(uint32_t typeFilter,  VkMemoryPropertyFlags properties) {
        assert(_physicalDevice != VK_NULL_HANDLE);

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

};
