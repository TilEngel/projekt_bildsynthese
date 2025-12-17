// Texture.cpp

#include "Texture.hpp"


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

void Texture::createImageBuffer(const char* filename) {
    
    //bild laden
    int texChannels;
    stbi_uc* pixels = stbi_load(filename, &_texWidth, &_texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error(std::string("failed to load texture image: ") + filename);
    }

    //Mipmap-level berechnen
    _mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(_texWidth, _texHeight)))) + 1;


    VkDeviceSize imageSize = static_cast<VkDeviceSize>(_texWidth) * static_cast<VkDeviceSize>(_texHeight) * 4;

    //staging buffer erstellen
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &_imageBuffer) != VK_SUCCESS) {
        stbi_image_free(pixels);
        throw std::runtime_error("failed to create image staging buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, _imageBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(_physicalDevice, memRequirements.memoryTypeBits,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_imageBufferMemory) != VK_SUCCESS) {
        stbi_image_free(pixels);
        throw std::runtime_error("failed to allocate image staging buffer memory!");
    }

    vkBindBufferMemory(_device, _imageBuffer, _imageBufferMemory, 0);

    // copy pixel data into mapped memory
    void* data;
    vkMapMemory(_device, _imageBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(_device, _imageBufferMemory);

    stbi_image_free(pixels);
    
}

void Texture::destroyImageBuffer() {
    
    if (_imageBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(_device, _imageBuffer, nullptr);
        _imageBuffer = VK_NULL_HANDLE;
    }
    if (_imageBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _imageBufferMemory, nullptr);
        _imageBufferMemory = VK_NULL_HANDLE;
    }
}

void Texture::createTextureImage() {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(_texWidth);
    imageInfo.extent.height = static_cast<uint32_t>(_texHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = _mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = IMAGE_FORMAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;                            //Für Mipmaps \/
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0;

    if (vkCreateImage(_device, &imageInfo, nullptr, &_textureImage) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image!");
    }
}

void Texture::allocateTextureImageMemory() {
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_device, _textureImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(_physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_textureImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate texture image memory!");
    }

    vkBindImageMemory(_device, _textureImage, _textureImageMemory, 0);
}

InitBuffer buf;
void Texture::copyBufferToImage() {
    //commandBuffer anlegen
    VkCommandBuffer commandBuffer = buf.beginSingleTimeCommands(_device,_commandPool);
  

    // transition: undefined -> transfer dst optimal
    VkImageMemoryBarrier barrierToTransfer{};
    barrierToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrierToTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierToTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierToTransfer.image = _textureImage;
    barrierToTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrierToTransfer.subresourceRange.baseMipLevel = 0;
    barrierToTransfer.subresourceRange.levelCount = 1;
    barrierToTransfer.subresourceRange.baseArrayLayer = 0;
    barrierToTransfer.subresourceRange.layerCount = 1;
    barrierToTransfer.srcAccessMask = 0; // as oldLayout is undefined
    barrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrierToTransfer
    );

    // copy buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        static_cast<uint32_t>(_texWidth),
        static_cast<uint32_t>(_texHeight),
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        _imageBuffer,
        _textureImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    int32_t mipWidth = _texWidth;
    int32_t mipHeight = _texHeight;
//Für alle Mipmap Levels:
    for (uint32_t i = 1; i < _mipLevels; i++) {

        // Transition prev level (i-1) to SRC
        VkImageMemoryBarrier barrierBlit{};
        barrierBlit.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierBlit.image = _textureImage;
        barrierBlit.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrierBlit.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrierBlit.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierBlit.subresourceRange.baseArrayLayer = 0;
        barrierBlit.subresourceRange.layerCount = 1;
        barrierBlit.subresourceRange.levelCount = 1;
        barrierBlit.subresourceRange.baseMipLevel = i - 1;
        barrierBlit.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrierBlit.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrierBlit.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrierBlit.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrierBlit
        );

        // Blit i-1 -> i
        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;

        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {
            mipWidth > 1 ? mipWidth / 2 : 1,
            mipHeight > 1 ? mipHeight / 2 : 1,
            1
        };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(
            commandBuffer,
            _textureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            _textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR
        );

        // Transition previous level to SHADER_READ_ONLY_OPTIMAL
        VkImageMemoryBarrier barrierRead{};
        barrierRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierRead.image = _textureImage;
        barrierRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrierRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrierRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierRead.subresourceRange.baseArrayLayer = 0;
        barrierRead.subresourceRange.layerCount = 1;
        barrierRead.subresourceRange.levelCount = 1;
        barrierRead.subresourceRange.baseMipLevel = i - 1;
        barrierRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrierRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrierRead.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrierRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrierRead
        );

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    } //Ende "Für alle Mipmap-Levels"

    // Für letztes MipMap-Level zu SHADER_READ_ONLY_OPTIMAL
    VkImageMemoryBarrier barrierLast = barrierToTransfer;
    barrierLast.subresourceRange.baseMipLevel = _mipLevels - 1;
    barrierLast.subresourceRange.levelCount = 1;
    barrierLast.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrierLast.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrierLast.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrierLast.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrierLast
    );

    
    buf.endSingleTimeCommands(_device,_commandPool,_queue,commandBuffer);
}

void Texture::createTextureImageView() {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = IMAGE_FORMAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = _mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(_device, &viewInfo, nullptr, &_textureImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
}

void Texture::createTextureSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // Geräteeigenschaften und Features abfragen
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(_physicalDevice, &props);

    VkPhysicalDeviceFeatures features{};
    vkGetPhysicalDeviceFeatures(_physicalDevice, &features);

    samplerInfo.anisotropyEnable = features.samplerAnisotropy ? VK_TRUE : VK_FALSE;
    samplerInfo.maxAnisotropy = features.samplerAnisotropy
        ? props.limits.maxSamplerAnisotropy
        : 1.0f;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(_mipLevels - 1);

    if (vkCreateSampler(_device, &samplerInfo, nullptr, &_textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

}
