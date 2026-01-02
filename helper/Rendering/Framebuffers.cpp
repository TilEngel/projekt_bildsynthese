#include "Framebuffers.hpp"
#include <stdexcept>
#include <array>

void Framebuffers::create() {

    const auto& swapViews = _swapChain->getImageViews();
    VkExtent2D extent = _swapChain->getExtent();
    VkImageView depthView = _depthBuffer->getImageView();

    _framebuffers.resize(swapViews.size());

    for (size_t i = 0; i < swapViews.size(); i++) {

        std::array<VkImageView, 3> attachments = {
            swapViews[i],   // color attachment
            depthView,      // depth attachment
            _gBufferImageView
        };

        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = _renderPass;
        info.attachmentCount = attachments.size();
        info.pAttachments = attachments.data();
        info.width = extent.width;
        info.height = extent.height;
        info.layers = 1;

        if (vkCreateFramebuffer(_device, &info, nullptr, &_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}
