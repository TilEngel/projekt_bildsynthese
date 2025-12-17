#pragma once

#include <vulkan/vulkan_core.h>

class DepthBuffer {
public:
    DepthBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkExtent2D extent)
    : _physicalDevice(physicalDevice)
    , _device(device) {
        createDepthImage(extent);
        createDepthImageView();
    }

    ~DepthBuffer() {
        cleanupDepthRessources();
    }

    VkFormat getImageFormat() {
        return _depthImageFormat;
    }

    VkImageView getImageView() {
        return _depthImageView;
    }

    void recreate(VkExtent2D extent) {
        cleanupDepthRessources();
        createDepthImage(extent);
        createDepthImageView();
    }

private:
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
        
    VkFormat _depthImageFormat = VK_FORMAT_UNDEFINED;
    VkImage _depthImage = VK_NULL_HANDLE;
    VkDeviceMemory _depthImageMemory = VK_NULL_HANDLE;
    VkImageView _depthImageView = VK_NULL_HANDLE;


    // create depth image with memory
    // - find suitable format for depth image
    //   - Use vkGetPhysicalDeviceFormatProperties to check, if one of the
    //     following formats can be uses as depth-stencil attachment:
    //     VK_FORMAT_D32_SFLOAT, 
    //     VK_FORMAT_D32_SFLOAT_S8_UINT,
    //     VK_FORMAT_D24_UNORM_S8_UINT
    //   - save format in member variable _depthImageFormat
    //   - throw error if none of the formats can be used
    // - create depth image
    //   - sharing mode can be "exclusive" (only used by graphics queue)
    //   - extent has to be the same as for the swap chain images
    //   - save image to _depthImage
    // - allocate memory for depth image
    //   - memory should be "device local"
    //   - save memory object in _depthImageMemory
    // - bind image memory to image
    void createDepthImage(VkExtent2D extent);

    // create view for depth image
    // - use depth aspect for aspect mask
    void createDepthImageView();

    // destroy the image view for the depth image
    // destroy the depth image
    // free memory used for depth image
    void cleanupDepthRessources();
};
