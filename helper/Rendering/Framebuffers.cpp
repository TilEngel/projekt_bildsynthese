#include "Framebuffers.hpp"
#include <stdexcept>
#include <array>



    void Framebuffers::createGBufferResources() {
        VkExtent2D extent = _swapChain->getExtent();
        VkFormat gBufferFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

        std::cout << "\n=== CREATING 2 G-BUFFER RESOURCES ===" << std::endl;

        //G-Buffer 1: Normal
        createSingleGBuffer(extent, gBufferFormat, 
                           _gBufferNormalImage, _gBufferNormalMemory, _gBufferNormalView);
        
        // G-Buffer 2: Albedo
        createSingleGBuffer(extent, gBufferFormat,
                           _gBufferAlbedoImage, _gBufferAlbedoMemory, _gBufferAlbedoView);

        std::cout << "2 G-Buffer resources created!" << std::endl;
    }

    // Helper zum Erstellen eines G-Buffers
    void Framebuffers::createSingleGBuffer(VkExtent2D extent, VkFormat format,
                            VkImage& image, VkDeviceMemory& memory, VkImageView& view) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = format;
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

        if (vkCreateImage(_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create G-Buffer image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(_device, image, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        if (vkAllocateMemory(_device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate G-Buffer memory!");
        }

        vkBindImageMemory(_device, image, memory, 0);

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

        if (vkCreateImageView(_device, &viewInfo, nullptr, &view) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create G-Buffer view!");
        }
    }

void Framebuffers::cleanupGBufferResources() {
    // Normal G-Buffer
    if (_gBufferNormalView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, _gBufferNormalView, nullptr);
        _gBufferNormalView = VK_NULL_HANDLE;
    }
    if (_gBufferNormalImage != VK_NULL_HANDLE) {
        vkDestroyImage(_device, _gBufferNormalImage, nullptr);
        _gBufferNormalImage = VK_NULL_HANDLE;
    }
    if (_gBufferNormalMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _gBufferNormalMemory, nullptr);
        _gBufferNormalMemory = VK_NULL_HANDLE;
    }
     //Albedo G-Buffer
    if (_gBufferAlbedoView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, _gBufferAlbedoView, nullptr);
        _gBufferAlbedoView = VK_NULL_HANDLE;
    }
    if (_gBufferAlbedoImage != VK_NULL_HANDLE) {
        vkDestroyImage(_device, _gBufferAlbedoImage, nullptr);
        _gBufferAlbedoImage = VK_NULL_HANDLE;
    }
    if (_gBufferAlbedoMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _gBufferAlbedoMemory, nullptr);
         _gBufferAlbedoMemory = VK_NULL_HANDLE;
    }
}

void Framebuffers::transitionGBufferLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue) {
    // Create temporary command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = _gBufferNormalImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    
}

void Framebuffers::create() {

    const auto& swapViews = _swapChain->getImageViews();
    VkExtent2D extent = _swapChain->getExtent();
    VkImageView depthView = _depthBuffer->getImageView();

    _framebuffers.resize(swapViews.size());
    std::cout << "\nCreating " << swapViews.size() << " framebuffers..." << std::endl;
    for (size_t i = 0; i < swapViews.size(); i++) {

        std::array<VkImageView, 4> attachments = {
            swapViews[i],   // color attachment
            depthView,      // depth attachment
            _gBufferNormalView,   //gBuffer
            _gBufferAlbedoView  //gbuffer 2
        };
        //alle Attachments prüfen
            if (swapViews[i] == VK_NULL_HANDLE) {
                throw std::runtime_error("Swap chain view " + std::to_string(i) + " is NULL!");
            }
            if (depthView == VK_NULL_HANDLE) {
                throw std::runtime_error("Depth view is NULL!");
            }
            if (_gBufferNormalView == VK_NULL_HANDLE) {
                throw std::runtime_error("G-Buffer view is NULL!");
            }

        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = _renderPass;
        info.attachmentCount = static_cast<uint32_t>(attachments.size());
        info.pAttachments = attachments.data();
        info.width = extent.width;
        info.height = extent.height;
        info.layers = 1;

        if (vkCreateFramebuffer(_device, &info, nullptr, &_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
    std::cout << "All framebuffers created successfully!" << std::endl;
}

void Framebuffers::cleanup() {
    for (auto fb : _framebuffers) {
        if (fb != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(_device, fb, nullptr);
        }
    }
    _framebuffers.clear();
}
