#pragma once

#include <vulkan/vulkan.h>
#include <array>

class GBuffer {
public:
    GBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkExtent2D extent);
    ~GBuffer();
    
    void recreate(VkExtent2D extent);
    void destroy();
    
    VkImageView getAlbedoImageView() const { return _albedoImageView; }
    VkImageView getNormalImageView() const { return _normalImageView; }
    VkImageView getPositionImageView() const { return _positionImageView; }
    
    VkFormat getAlbedoFormat() const { return VK_FORMAT_R8G8B8A8_UNORM; }
    VkFormat getNormalFormat() const { return VK_FORMAT_R16G16B16A16_SFLOAT; }
    VkFormat getPositionFormat() const { return VK_FORMAT_R16G16B16A16_SFLOAT; }

private:
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VkExtent2D _extent;
    
    // Albedo (diffuse color)
    VkImage _albedoImage = VK_NULL_HANDLE;
    VkDeviceMemory _albedoMemory = VK_NULL_HANDLE;
    VkImageView _albedoImageView = VK_NULL_HANDLE;
    
    // Normal
    VkImage _normalImage = VK_NULL_HANDLE;
    VkDeviceMemory _normalMemory = VK_NULL_HANDLE;
    VkImageView _normalImageView = VK_NULL_HANDLE;
    
    // Position
    VkImage _positionImage = VK_NULL_HANDLE;
    VkDeviceMemory _positionMemory = VK_NULL_HANDLE;
    VkImageView _positionImageView = VK_NULL_HANDLE;
    
    void createGBufferImages();
    void createImageViews();
    
    void createImage(VkFormat format, VkImageUsageFlags usage,
                    VkImage& image, VkDeviceMemory& memory);
    VkImageView createImageView(VkImage image, VkFormat format);
    
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};