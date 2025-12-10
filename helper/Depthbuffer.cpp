#include "Depthbuffer.hpp"

#include <stdexcept>
#include <vector>
#include <iostream>

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties,
                        VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ( (typeFilter & (1 << i)) &&
             (memProperties.memoryTypes[i].propertyFlags & properties) == properties )
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type for depth buffer!");
}


// ------------------------------------------------------------
// create depth image with memory
// ------------------------------------------------------------
void DepthBuffer::createDepthImage(VkExtent2D extent)
{
    // ---------------------------------------------
    // 1. Find suitable depth format
    // ---------------------------------------------
    const std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    _depthImageFormat = VK_FORMAT_UNDEFINED;

    for (VkFormat f : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(_physicalDevice, f, &props);

        if (props.optimalTilingFeatures &
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            _depthImageFormat = f;
            break;
        }
    }

    if (_depthImageFormat == VK_FORMAT_UNDEFINED) {
        throw std::runtime_error("No suitable depth format found!");
    }


    // ---------------------------------------------
    // 2. Create depth image
    // ---------------------------------------------
    VkImageCreateInfo imgInfo{};
    imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.imageType = VK_IMAGE_TYPE_2D;
    imgInfo.format = _depthImageFormat;
    imgInfo.extent.width = extent.width;
    imgInfo.extent.height = extent.height;
    imgInfo.extent.depth = 1;
    imgInfo.mipLevels = 1;
    imgInfo.arrayLayers = 1;
    imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(_device, &imgInfo, nullptr, &_depthImage) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image!");
    }


    // ---------------------------------------------
    // 3. Allocate depth image memory
    // ---------------------------------------------
    VkMemoryRequirements memReq{};
    vkGetImageMemoryRequirements(_device, _depthImage, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memReq.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                       _physicalDevice);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_depthImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate depth image memory!");
    }

    // ---------------------------------------------
    // 4. Bind memory to image
    // ---------------------------------------------
    vkBindImageMemory(_device, _depthImage, _depthImageMemory, 0);
}


// ------------------------------------------------------------
// create view for depth image
// ------------------------------------------------------------
void DepthBuffer::createDepthImageView()
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = _depthImageFormat;

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(_device, &viewInfo, nullptr, &_depthImageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image view!");
    }
}


// ------------------------------------------------------------
// destroy all depth buffer resources
// ------------------------------------------------------------
void DepthBuffer::cleanupDepthRessources()
{
    if (_depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, _depthImageView, nullptr);
        _depthImageView = VK_NULL_HANDLE;
    }

    if (_depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(_device, _depthImage, nullptr);
        _depthImage = VK_NULL_HANDLE;
    }

    if (_depthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _depthImageMemory, nullptr);
        _depthImageMemory = VK_NULL_HANDLE;
    }
}
