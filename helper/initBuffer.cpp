// initBuffer.cpp
#include "initBuffer.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

//Passenden MemoryType für dieses Device unter berücksichtigung der Properties
uint32_t InitBuffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) && ( (memProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
            return i;
        }
    }
    throw std::runtime_error("InitBuffer::findMemoryType: failed to find suitable memory type!");
}

VkCommandBuffer InitBuffer::beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("InitBuffer::beginSingleTimeCommands: Failed to allocate command buffer");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("InitBuffer::beginSingleTimeCommands: Failed to begin command buffer");
    }

    return commandBuffer;
}

void InitBuffer::endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer) {
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("InitBuffer::endSingleTimeCommands: Failed to end command buffer");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("InitBuffer::endSingleTimeCommands: Failed to submit command buffer");
    }

    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void InitBuffer::copyBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue,
                            VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(device, commandPool, queue, commandBuffer);
}

VkBuffer InitBuffer::createVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
                                        VkCommandPool commandPool, VkQueue graphicsQueue,
                                        const std::vector<Vertex>& vertices,
                                        VkDeviceMemory* outMemory) {
    if (vertices.empty()) {
        throw std::runtime_error("InitBuffer::createVertexBuffer: vertices is empty");
    }

    VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

    //Staging Buffer erstellen
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo stagingInfo{};
    stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingInfo.size = bufferSize;
    stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult res = vkCreateBuffer(device, &stagingInfo, nullptr, &stagingBuffer);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("InitBuffer::createVertexBuffer: failed to create staging buffer");
    }

    VkMemoryRequirements stagingMemReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingMemReq);

    VkMemoryAllocateInfo stagingAlloc{};
    stagingAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingAlloc.allocationSize = stagingMemReq.size;
    stagingAlloc.memoryTypeIndex = findMemoryType(stagingMemReq.memoryTypeBits,
                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                 physicalDevice);

    if (vkAllocateMemory(device, &stagingAlloc, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        throw std::runtime_error("InitBuffer::createVertexBuffer: failed to allocate staging buffer memory");
    }

    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    //vertex Daten -> staging buffer
    void* data = nullptr;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, stagingBufferMemory);

    //device local VertexBuffer
    VkBufferCreateInfo vertexInfo{};
    vertexInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexInfo.size = bufferSize;
    vertexInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    res = vkCreateBuffer(device, &vertexInfo, nullptr, &_vertexBuffer);
    if (res != VK_SUCCESS) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        throw std::runtime_error("InitBuffer::createVertexBuffer: failed to create vertex buffer");
    }

    VkMemoryRequirements vertexMemReq;
    vkGetBufferMemoryRequirements(device, _vertexBuffer, &vertexMemReq);

    VkMemoryAllocateInfo vertexAlloc{};
    vertexAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vertexAlloc.allocationSize = vertexMemReq.size;
    vertexAlloc.memoryTypeIndex = findMemoryType(vertexMemReq.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                physicalDevice);

    if (vkAllocateMemory(device, &vertexAlloc, nullptr, &_vertexBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, _vertexBuffer, nullptr);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        throw std::runtime_error("InitBuffer::createVertexBuffer: failed to allocate vertex buffer memory");
    }

    vkBindBufferMemory(device, _vertexBuffer, _vertexBufferMemory, 0);

    // 3) staging -> device lokal
    copyBuffer(device, commandPool, graphicsQueue, stagingBuffer, _vertexBuffer, bufferSize);

    // Cleanup staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    // Memory zurückgeben, falls der Pointer nicht null ist
    if (outMemory != nullptr) {
        *outMemory = _vertexBufferMemory;
    }

    std::cout << "[DEBUG] Vertex buffer created via staging buffer (" << vertices.size() << " vertices)" << std::endl;
    return _vertexBuffer;
}

void InitBuffer::destroyVertexBuffer(VkDevice device) {
    if (_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, _vertexBuffer, nullptr);
        _vertexBuffer = VK_NULL_HANDLE;
    }
    if (_vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, _vertexBufferMemory, nullptr);
        _vertexBufferMemory = VK_NULL_HANDLE;
    }
}

VkBuffer InitBuffer::createImageBuffer(VkPhysicalDevice physicalDevice, VkDevice device, const char* imagePath) {
    // rgba Bild laden
    int texChannels = 0;
    stbi_uc* pixels = stbi_load(imagePath, &_texWidth, &_texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error(std::string("InitBuffer::createImageBuffer: failed to load texture image: ") + imagePath);
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(_texWidth) * static_cast<VkDeviceSize>(_texHeight) * 4u; // 4 bytes per pixel

    //host-visible Buffer für Pixel
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &_imageBuffer) != VK_SUCCESS) {
        stbi_image_free(pixels);
        throw std::runtime_error("InitBuffer::createImageBuffer: failed to create image buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, _imageBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                              physicalDevice);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &_imageBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, _imageBuffer, nullptr);
        stbi_image_free(pixels);
        throw std::runtime_error("InitBuffer::createImageBuffer: failed to allocate image buffer memory!");
    }

    vkBindBufferMemory(device, _imageBuffer, _imageBufferMemory, 0);

    // pixel Daten -> buffer
    void* data = nullptr;
    vkMapMemory(device, _imageBufferMemory, 0, imageSize, 0, &data);
    std::memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, _imageBufferMemory);

    stbi_image_free(pixels);

    return _imageBuffer;
}

void InitBuffer::destroyImageBuffer(VkDevice device) {
    if (_imageBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, _imageBuffer, nullptr);
        _imageBuffer = VK_NULL_HANDLE;
    }
    if (_imageBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, _imageBufferMemory, nullptr);
        _imageBufferMemory = VK_NULL_HANDLE;
    }
}

int InitBuffer::getTexWidth() const {
    return _texWidth;
}

int InitBuffer::getTexHeight() const {
    return _texHeight;
}
