#pragma once

#include <vulkan/vulkan.h>
#include "../../stb_image.h"

#include <vector>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "../initBuffer.hpp"

class Texture {
public:
    Texture(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue queue, const char* filename)
    : _physicalDevice(physicalDevice)
    , _device(device) 
    , _commandPool(commandPool)
    , _queue(queue) {
        createImageBuffer(filename);
        createTextureImage();
        allocateTextureImageMemory();
        copyBufferToImage();
        destroyImageBuffer();
        createTextureImageView();
        createTextureSampler();
    }

    void destroy() {
        vkDestroySampler(_device, _textureSampler, nullptr);
        vkDestroyImageView(_device, _textureImageView, nullptr);
        vkDestroyImage(_device, _textureImage, nullptr);
        vkFreeMemory(_device, _textureImageMemory, nullptr);
    }

    VkImageView getImageView() {
        return _textureImageView;
    }

    VkSampler getSampler() {
        return _textureSampler;
    }

private:
    const VkFormat IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;

    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkCommandPool _commandPool = VK_NULL_HANDLE;
    VkQueue _queue = VK_NULL_HANDLE;

    VkBuffer _imageBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _imageBufferMemory = VK_NULL_HANDLE;
    int _texWidth = 0;
    int _texHeight = 0;
    uint32_t _mipLevels = 0;

    VkImage _textureImage = VK_NULL_HANDLE;
    VkDeviceMemory _textureImageMemory = VK_NULL_HANDLE;
    VkImageView _textureImageView = VK_NULL_HANDLE;
    VkSampler _textureSampler = VK_NULL_HANDLE;

    // create the staging buffer for the texture
    // - load pixel data from file with name filename
    // - save objects in member variables _imageBuffer and _imageBufferMemory
    // - save texture width and height in _texWidth and _texHeight
    // - save number of mipmap levels in _mipLevels
    void createImageBuffer(const char* filename);

    // destroy the staging buffer for the texture and free its memory
    void destroyImageBuffer();

    // create the image object for the texture
    // - use values from _texWidth, _texHeight and _mipLevels
    // - format should be VK_FORMAT_R8G8B8A8_SRGB
    // - tiling shoud be "optimal"
    // - layout has to be "undefined"
    // - usage is "transfer source", "transfer destination" and "sampled"
    // - use member variable _textureImage for the image object
    void createTextureImage();

    // allocate memory for the texture image
    // - memory should be "device local"
    // - consider memory requirements of _textureImage
    // - bind image memory to _textureImage
    // - use member variable _textureImageMemory for the memory object
    void allocateTextureImageMemory();

    // copy image data from _imageBuffer to _textureImage
    // - create a command buffer from _commandPool
    // - begin recording to the command buffer
    // - add command for transition of _textureImage's layout from "undefined" to "transfer destination optimal"
    //   for all mipmap levels (source stage mask is "top of pipe" and destination stage mask "transfer")
    // - add command for copying data from _imageBuffer to _textureImage
    // - check if image format supports linear blitting (VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
    // - generate the mipmap by adding the following commands for each mipmap level i >= 1:
    //     - transition the previous level's (i-1) layout from "transfer destination optimal" to "transfer source optimal"
    //       (source and destination stage masks are "transfer")
    //     - blit the image from previous level to current level (halving width and height)
    //     - transition the previous level's (i-1) layout from "transfer source optimal" to "shader read-only optimal"
    //       (source stage mask is "transfer" and destination stage mask "fragment shader")
    // - transition the last level's layout from "transfer destination optimal" to "shader read-only optimal"
    //   (source stage mask is "transfer" and destination stage mask "fragment shader")
    // - end recording to the command buffer
    // - submit command buffer to _queue
    // - wait for _queue to become idle
    // - free the command buffer
    void copyBufferToImage();

    // create an image view for _textureImage
    // - subresource range has to contain all mipmap levels
    // - save to _textureImageView
    void createTextureImageView();

    // create a sampler
    // - activate linear mipmap mode
    // - save to _textureSampler
    void createTextureSampler();
};

