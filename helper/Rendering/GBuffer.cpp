#include "GBuffer.hpp"
#include <stdexcept>

GBuffer::GBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkExtent2D extent)
    : _physicalDevice(physicalDevice)
    , _device(device)
    , _extent(extent) {
    createGBufferImages();
    createImageViews();
}

GBuffer::~GBuffer() {
    destroy();
}

void GBuffer::recreate(VkExtent2D extent) {
    _extent = extent;
    destroy();
    createGBufferImages();
    createImageViews();
}

void GBuffer::destroy() {
    if (_albedoImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, _albedoImageView, nullptr);
        _albedoImageView = VK_NULL_HANDLE;
    }
    if (_normalImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, _normalImageView, nullptr);
        _normalImageView = VK_NULL_HANDLE;
    }
    if (_positionImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, _positionImageView, nullptr);
        _positionImageView = VK_NULL_HANDLE;
    }
    
    if (_albedoImage != VK_NULL_HANDLE) {
        vkDestroyImage(_device, _albedoImage, nullptr);
        _albedoImage = VK_NULL_HANDLE;
    }
    if (_normalImage != VK_NULL_HANDLE) {
        vkDestroyImage(_device, _normalImage, nullptr);
        _normalImage = VK_NULL_HANDLE;
    }
    if (_positionImage != VK_NULL_HANDLE) {
        vkDestroyImage(_device, _positionImage, nullptr);
        _positionImage = VK_NULL_HANDLE;
    }
    
    if (_albedoMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _albedoMemory, nullptr);
        _albedoMemory = VK_NULL_HANDLE;
    }
    if (_normalMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _normalMemory, nullptr);
        _normalMemory = VK_NULL_HANDLE;
    }
    if (_positionMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _positionMemory, nullptr);
        _positionMemory = VK_NULL_HANDLE;
    }
}

void GBuffer::createGBufferImages() {
    // Albedo
    createImage(VK_FORMAT_R8G8B8A8_UNORM,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
               _albedoImage, _albedoMemory);
    
    // Normal
    createImage(VK_FORMAT_R16G16B16A16_SFLOAT,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
               _normalImage, _normalMemory);
    
    // Position
    createImage(VK_FORMAT_R16G16B16A16_SFLOAT,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
               _positionImage, _positionMemory);
}

void GBuffer::createImageViews() {
    _albedoImageView = createImageView(_albedoImage, VK_FORMAT_R8G8B8A8_UNORM);
    _normalImageView = createImageView(_normalImage, VK_FORMAT_R16G16B16A16_SFLOAT);
    _positionImageView = createImageView(_positionImage, VK_FORMAT_R16G16B16A16_SFLOAT);
}

void GBuffer::createImage(VkFormat format, VkImageUsageFlags usage,
                         VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = _extent.width;
    imageInfo.extent.height = _extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create G-buffer image!");
    }
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_device, image, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate G-buffer image memory!");
    }
    
    vkBindImageMemory(_device, image, memory, 0);
}

VkImageView GBuffer::createImageView(VkImage image, VkFormat format) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    VkImageView imageView;
    if (vkCreateImageView(_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create G-buffer image view!");
    }
    
    return imageView;
}

uint32_t GBuffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("failed to find suitable memory type!");
}