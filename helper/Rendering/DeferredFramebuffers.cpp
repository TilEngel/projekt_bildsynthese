#include "DeferredFramebuffers.hpp"
#include <stdexcept>
#include <array>

DeferredFramebuffers::DeferredFramebuffers(VkDevice device, 
                                         SwapChain* swapChain, 
                                         DepthBuffer* depthBuffer,
                                         GBuffer* gBuffer,
                                         VkRenderPass renderPass)
    : _device(device)
    , _swapChain(swapChain)
    , _depthBuffer(depthBuffer)
    , _gBuffer(gBuffer)
    , _renderPass(renderPass) {
    createFramebuffers();
}

DeferredFramebuffers::~DeferredFramebuffers() {
    destroyFramebuffers();
}

void DeferredFramebuffers::recreate() {
    destroyFramebuffers();
    createFramebuffers();
}

void DeferredFramebuffers::createFramebuffers() {
    auto imageViews = _swapChain->getImageViews();
    _framebuffers.resize(imageViews.size());
    
    for (size_t i = 0; i < imageViews.size(); i++) {
        std::array<VkImageView, 5> attachments = {
            imageViews[i],                      // Swapchain
            _depthBuffer->getImageView(),       // Depth
            _gBuffer->getAlbedoImageView(),     // Albedo
            _gBuffer->getNormalImageView(),     // Normal
            _gBuffer->getPositionImageView()    // Position
        };
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = _swapChain->getExtent().width;
        framebufferInfo.height = _swapChain->getExtent().height;
        framebufferInfo.layers = 1;
        
        if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create deferred framebuffer!");
        }
    }
}

void DeferredFramebuffers::destroyFramebuffers() {
    for (auto framebuffer : _framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(_device, framebuffer, nullptr);
        }
    }
    _framebuffers.clear();
}