#pragma once
#include <vulkan/vulkan_core.h>
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

    VkImageView getGBufferView() const { return _gBufferImageView; }

private:
    VkDevice _device;
    VkPhysicalDevice _physicalDevice;
    SwapChain* _swapChain;
    DepthBuffer* _depthBuffer;
    VkRenderPass _renderPass;
    
    std::vector<VkFramebuffer> _framebuffers;
    
    // G-Buffer resources
    VkImage _gBufferImage = VK_NULL_HANDLE;
    VkDeviceMemory _gBufferMemory = VK_NULL_HANDLE;
    VkImageView _gBufferImageView = VK_NULL_HANDLE;

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && 
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type!");
    }

    void createGBufferResources() {
        VkExtent2D extent = _swapChain->getExtent();
        VkFormat gBufferFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

        // Create G-Buffer Image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = gBufferFormat;
        imageInfo.extent.width = extent.width;
        imageInfo.extent.height = extent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(_device, &imageInfo, nullptr, &_gBufferImage) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create G-Buffer image!");
        }

        // Allocate memory
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(_device, _gBufferImage, &memRequirements);

        // Need physical device - get from depth buffer helper
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        // This is a workaround - ideally pass physical device to constructor
        // For now, we'll use device-local memory properties
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        
        // Find memory type (device local)
        VkPhysicalDeviceMemoryProperties memProperties;
        // We need to store physical device reference - for now use a hack
        // TODO: Pass physical device to Framebuffers constructor
        
        allocInfo.memoryTypeIndex = findMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            _physicalDevice
        );

        
        if (vkAllocateMemory(_device, &allocInfo, nullptr, &_gBufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate G-Buffer memory!");
        }

        vkBindImageMemory(_device, _gBufferImage, _gBufferMemory, 0);

        // Create Image View
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _gBufferImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = gBufferFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(_device, &viewInfo, nullptr, &_gBufferImageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create G-Buffer image view!");
        }
    }

    void cleanupGBufferResources() {
        if (_gBufferImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(_device, _gBufferImageView, nullptr);
            _gBufferImageView = VK_NULL_HANDLE;
        }
        if (_gBufferImage != VK_NULL_HANDLE) {
            vkDestroyImage(_device, _gBufferImage, nullptr);
            _gBufferImage = VK_NULL_HANDLE;
        }
        if (_gBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(_device, _gBufferMemory, nullptr);
            _gBufferMemory = VK_NULL_HANDLE;
        }
    }

    void create(); //Implementierung in Framebuffers.cpp

    void cleanup() {
        for (auto fb : _framebuffers) {
            if (fb != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(_device, fb, nullptr);
            }
        }
        _framebuffers.clear();
    }
};