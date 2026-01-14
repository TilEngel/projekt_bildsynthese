#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <array>
#include "../initBuffer.hpp"

/*
* Klasse für die Darstellung der CubeMap Textur aka Skybox
*/

class CubeMap{
public:
    CubeMap(VkPhysicalDevice physicalDevice,
                   VkDevice device,
                   VkCommandPool commandPool,
                   VkQueue queue,
                   const std::array<const char*, 6>& faces) //Je ein Bild für jede Seite
        : _physicalDevice(physicalDevice)
        , _device(device)
        , _commandPool(commandPool)
        , _queue(queue)
    {
        //speichert 6 Images aus faces
        loadCubeMap(faces);

        //sollte klar sein
        createTextureImage();
        allocateTextureImageMemory();
        copyBufferToImage();
        destroyImageBuffer();
        createTextureImageView();
        createTextureSampler();
    }

    ~CubeMap() {
        if (_textureSampler != VK_NULL_HANDLE)
            vkDestroySampler(_device, _textureSampler, nullptr);
        if (_textureImageView != VK_NULL_HANDLE)
            vkDestroyImageView(_device, _textureImageView, nullptr);
        if (_textureImage != VK_NULL_HANDLE)
            vkDestroyImage(_device, _textureImage, nullptr);
        if (_textureImageMemory != VK_NULL_HANDLE)
            vkFreeMemory(_device, _textureImageMemory, nullptr);
    }

    VkImageView getImageView() const { return _textureImageView; }
    VkSampler getSampler() const { return _textureSampler; }

private:
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VkCommandPool _commandPool;
    VkQueue _queue;

    VkImage _textureImage = VK_NULL_HANDLE;
    VkDeviceMemory _textureImageMemory = VK_NULL_HANDLE;
    VkImageView _textureImageView = VK_NULL_HANDLE;
    VkSampler _textureSampler = VK_NULL_HANDLE;

    VkBuffer _imageBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _imageBufferMemory = VK_NULL_HANDLE;
    InitBuffer _buff;

    int _texWidth = 0;
    int _texHeight = 0;
    uint32_t _mipLevels = 1;

    static constexpr VkFormat IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;

    void loadCubeMap(const std::array<const char*, 6>& faces);
    void createTextureImage();
    void allocateTextureImageMemory();
    void copyBufferToImage();
    void destroyImageBuffer();
    void createTextureImageView();
    void createTextureSampler();
};