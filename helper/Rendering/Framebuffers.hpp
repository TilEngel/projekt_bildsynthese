#pragma once
#include <vulkan/vulkan_core.h>
#include<iostream>
#include <vector>
#include <stdexcept>
#include <array>
#include "Swapchain.hpp"
#include "Depthbuffer.hpp"

class Framebuffers {
public:
    Framebuffers(VkDevice device, VkPhysicalDevice physicalDevice, SwapChain* swapChain, 
                 DepthBuffer* depthBuffer, VkRenderPass renderPass)
        : _device(device), _physicalDevice(physicalDevice), _swapChain(swapChain), 
          _depthBuffer(depthBuffer), _renderPass(renderPass) {
        createGBufferResources();
        create();
    }

    ~Framebuffers() {
        cleanup();
        cleanupGBufferResources();
    }

    void recreate() {
        cleanup();
        cleanupGBufferResources();
        createGBufferResources();
        create();
    }

    VkFramebuffer getFramebuffer(size_t index) const {
        if (index >= _framebuffers.size()) return VK_NULL_HANDLE;
        return _framebuffers[index];
    }

    size_t getCount() const { return _framebuffers.size(); }

    VkImageView getGBufferNormalView() const { return _gBufferNormalView; }
    VkImageView getGBufferAlbedoView() const { return _gBufferAlbedoView; }

private:
    VkDevice _device;
    VkPhysicalDevice _physicalDevice;
    SwapChain* _swapChain;
    DepthBuffer* _depthBuffer;
    VkRenderPass _renderPass;
    
    std::vector<VkFramebuffer> _framebuffers;
    
    // G-Buffer resources
    VkImage _gBufferNormalImage = VK_NULL_HANDLE;
    VkDeviceMemory _gBufferNormalMemory = VK_NULL_HANDLE;
    VkImageView _gBufferNormalView = VK_NULL_HANDLE;
    //albedo G-Buffer
    VkImage _gBufferAlbedoImage = VK_NULL_HANDLE;
    VkDeviceMemory _gBufferAlbedoMemory = VK_NULL_HANDLE;
    VkImageView _gBufferAlbedoView = VK_NULL_HANDLE;

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && 
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type!");
    }
    //ruft createSingleGBuffer() für beide GBuffers auf
    void createGBufferResources();

    // Helper zum Erstellen eines G-Buffers
    void createSingleGBuffer(VkExtent2D extent, VkFormat format,
                            VkImage& image, VkDeviceMemory& memory, VkImageView& view);

    void cleanupGBufferResources();


    void create();

    void cleanup();

};