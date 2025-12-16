#include "CubeMap.hpp"
#include "../stb_image.h"
#include "initBuffer.hpp"
#include <stdexcept>
#include <cstring>

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void CubeMap::loadCubeMap(const std::array<const char*, 6>& faces) {
    // Lade die erste Textur um Größe zu bestimmen
    int texChannels;
    stbi_uc* firstPixels = stbi_load(faces[0], &_texWidth, &_texHeight, &texChannels, STBI_rgb_alpha);
    if (!firstPixels) {
        throw std::runtime_error(std::string("Failed to load cubemap face: ") + faces[0]);
    }

    VkDeviceSize layerSize = static_cast<VkDeviceSize>(_texWidth) * _texHeight * 4;
    VkDeviceSize imageSize = layerSize * 6; // 6 Faces

    // Staging Buffer erstellen
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &_imageBuffer) != VK_SUCCESS) {
        stbi_image_free(firstPixels);
        throw std::runtime_error("Failed to create cubemap staging buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, _imageBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(_physicalDevice, memRequirements.memoryTypeBits,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_imageBufferMemory) != VK_SUCCESS) {
        stbi_image_free(firstPixels);
        throw std::runtime_error("Failed to allocate cubemap staging buffer memory!");
    }

    vkBindBufferMemory(_device, _imageBuffer, _imageBufferMemory, 0);

    // Map memory und kopiere alle 6 Faces
    void* data;
    vkMapMemory(_device, _imageBufferMemory, 0, imageSize, 0, &data);

    // Kopiere erste Textur
    memcpy(data, firstPixels, static_cast<size_t>(layerSize));
    stbi_image_free(firstPixels);

    // Lade und kopiere restliche 5 Faces
    for (int i = 1; i < 6; ++i) {
        int w, h, channels;
        stbi_uc* pixels = stbi_load(faces[i], &w, &h, &channels, STBI_rgb_alpha);
        if (!pixels) {
            vkUnmapMemory(_device, _imageBufferMemory);
            throw std::runtime_error(std::string("Failed to load cubemap face: ") + faces[i]);
        }
        if (w != _texWidth || h != _texHeight) {
            stbi_image_free(pixels);
            vkUnmapMemory(_device, _imageBufferMemory);
            throw std::runtime_error("Cubemap faces have different dimensions!");
        }
        
        memcpy(static_cast<char*>(data) + layerSize * i, pixels, static_cast<size_t>(layerSize));
        stbi_image_free(pixels);
    }

    vkUnmapMemory(_device, _imageBufferMemory);
}

void CubeMap::createTextureImage() {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(_texWidth);
    imageInfo.extent.height = static_cast<uint32_t>(_texHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = _mipLevels;
    imageInfo.arrayLayers = 6; // CubeMap hat 6 Layers
    imageInfo.format = IMAGE_FORMAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // WICHTIG für CubeMap

    if (vkCreateImage(_device, &imageInfo, nullptr, &_textureImage) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create cubemap image!");
    }
}

void CubeMap::allocateTextureImageMemory() {
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_device, _textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(_physicalDevice, memRequirements.memoryTypeBits, 
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_textureImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate cubemap image memory!");
    }

    vkBindImageMemory(_device, _textureImage, _textureImageMemory, 0);
}

void CubeMap::copyBufferToImage() {
    InitBuffer buf;
    VkCommandBuffer commandBuffer = buf.beginSingleTimeCommands(_device, _commandPool);

    // Transition: UNDEFINED -> TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _textureImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = _mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 6; // Alle 6 Layers
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy buffer to image (alle 6 Faces)
    VkDeviceSize layerSize = static_cast<VkDeviceSize>(_texWidth) * _texHeight * 4;
    
    for (uint32_t face = 0; face < 6; ++face) {
        VkBufferImageCopy region{};
        region.bufferOffset = layerSize * face;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = face;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            static_cast<uint32_t>(_texWidth),
            static_cast<uint32_t>(_texHeight),
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, _imageBuffer, _textureImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    // Transition: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    buf.endSingleTimeCommands(_device, _commandPool, _queue, commandBuffer);
}

void CubeMap::destroyImageBuffer() {
    if (_imageBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(_device, _imageBuffer, nullptr);
        _imageBuffer = VK_NULL_HANDLE;
    }
    if (_imageBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _imageBufferMemory, nullptr);
        _imageBufferMemory = VK_NULL_HANDLE;
    }
}

void CubeMap::createTextureImageView() {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE; // CUBE statt 2D!
    viewInfo.format = IMAGE_FORMAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = _mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6; // 6 Faces

    if (vkCreateImageView(_device, &viewInfo, nullptr, &_textureImageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create cubemap image view!");
    }
}

void CubeMap::createTextureSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(_mipLevels);

    if (vkCreateSampler(_device, &samplerInfo, nullptr, &_textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create cubemap sampler!");
    }
}