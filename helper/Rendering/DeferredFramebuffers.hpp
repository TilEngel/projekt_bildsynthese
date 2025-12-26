#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "Swapchain.hpp"
#include "Depthbuffer.hpp"
#include "GBuffer.hpp"

class DeferredFramebuffers {
public:
    DeferredFramebuffers(VkDevice device, 
                        SwapChain* swapChain, 
                        DepthBuffer* depthBuffer,
                        GBuffer* gBuffer,
                        VkRenderPass renderPass);
    ~DeferredFramebuffers();
    
    void recreate();
    VkFramebuffer getFramebuffer(size_t index) const { return _framebuffers[index]; }

private:
    VkDevice _device;
    SwapChain* _swapChain;
    DepthBuffer* _depthBuffer;
    GBuffer* _gBuffer;
    VkRenderPass _renderPass;
    
    std::vector<VkFramebuffer> _framebuffers;
    
    void createFramebuffers();
    void destroyFramebuffers();
};