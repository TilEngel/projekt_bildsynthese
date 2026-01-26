#include "CubemapRenderTarget.hpp"

void CubemapRenderTarget::cleanup() {
    if (_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(_device, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    }

    for (auto fb : _framebuffers) {
        if (fb != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(_device, fb, nullptr);
        }
    }

    if (_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(_device, _renderPass, nullptr);
        _renderPass = VK_NULL_HANDLE;
    }

    if (_depthView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, _depthView, nullptr);
        _depthView = VK_NULL_HANDLE;
    }

    if (_depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(_device, _depthImage, nullptr);
        _depthImage = VK_NULL_HANDLE;
    }

    if (_depthMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _depthMemory, nullptr);
        _depthMemory = VK_NULL_HANDLE;
    }

    for (auto view : _faceViews) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(_device, view, nullptr);
        }
    }

    if (_cubemapView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, _cubemapView, nullptr);
        _cubemapView = VK_NULL_HANDLE;
    }

    if (_cubemapImage != VK_NULL_HANDLE) {
        vkDestroyImage(_device, _cubemapImage, nullptr);
        _cubemapImage = VK_NULL_HANDLE;
    }

    if (_cubemapMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _cubemapMemory, nullptr);
        _cubemapMemory = VK_NULL_HANDLE;
    }
}

void CubemapRenderTarget::createCubemapImage() {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent.width = _resolution;
    imageInfo.extent.height = _resolution;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;  // 6 Faces für Cubemap
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    if (vkCreateImage(_device, &imageInfo, nullptr, &_cubemapImage) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create cubemap image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_device, _cubemapImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = initB.findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _physicalDevice
    );

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_cubemapMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate cubemap memory!");
    }

    vkBindImageMemory(_device, _cubemapImage, _cubemapMemory, 0);

    std::cout << "Cubemap image created: " << _resolution << "x" << _resolution << std::endl;
}

void CubemapRenderTarget::createCubemapViews() {
    // Individual Face Views für Framebuffer
    for (uint32_t i = 0; i < 6; i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _cubemapImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = i;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(_device, &viewInfo, nullptr, &_faceViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create cubemap face view!");
        }
    }

    // Kompletter Cubemap View für Shader
    VkImageViewCreateInfo cubemapViewInfo{};
    cubemapViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    cubemapViewInfo.image = _cubemapImage;
    cubemapViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    cubemapViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    cubemapViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    cubemapViewInfo.subresourceRange.baseMipLevel = 0;
    cubemapViewInfo.subresourceRange.levelCount = 1;
    cubemapViewInfo.subresourceRange.baseArrayLayer = 0;
    cubemapViewInfo.subresourceRange.layerCount = 6;

    if (vkCreateImageView(_device, &cubemapViewInfo, nullptr, &_cubemapView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create cubemap view!");
    }

    std::cout << "Cubemap views created" << std::endl;
}

void CubemapRenderTarget::createDepthResources() {
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = depthFormat;
    imageInfo.extent.width = _resolution;
    imageInfo.extent.height = _resolution;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;  // Für alle 6 Faces
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(_device, &imageInfo, nullptr, &_depthImage) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_device, _depthImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = initB.findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,_physicalDevice
    );

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_depthMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate depth memory!");
    }

    vkBindImageMemory(_device, _depthImage, _depthMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    if (vkCreateImageView(_device, &viewInfo, nullptr, &_depthView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth view!");
    }

    std::cout << "Depth resources created" << std::endl;
}

void CubemapRenderTarget::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }

    std::cout << "Cubemap render pass created" << std::endl;
}

void CubemapRenderTarget::createFramebuffers() {
    for (uint32_t i = 0; i < 6; i++) {
        // Für jeden Face ein eigenes Framebuffer mit dem entsprechenden Layer
        VkImageView attachments[2];
        attachments[0] = _faceViews[i];  // Color attachment für diesen Face
        attachments[1] = _depthView;     // Shared depth (alle Layers)

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = _resolution;
        framebufferInfo.height = _resolution;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }

    std::cout << "Cubemap framebuffers created" << std::endl;
}

void CubemapRenderTarget::createSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(_device, &samplerInfo, nullptr, &_sampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create cubemap sampler!");
    }

    std::cout << "Cubemap sampler created" << std::endl;
}