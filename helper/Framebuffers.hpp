#include "Swapchain.hpp"
#include "Depthbuffer.hpp"
#include <cstddef>
#include <vulkan/vulkan_core.h>

class Framebuffers {
public:

    Framebuffers(VkDevice device, SwapChain* swapChain, DepthBuffer* depthBuffer, VkRenderPass renderPass)
    : _device(device)
    , _swapChain(swapChain)
    , _depthBuffer(depthBuffer)
    , _renderPass(renderPass) {
        create();
    }

    ~Framebuffers() {
        cleanup();
    }

    VkFramebuffer getFramebuffer(size_t imageIndex) {
        return _framebuffers.at(imageIndex);
    }

    void recreate() {
        cleanup();
        create();
    }

private:

    VkDevice _device = VK_NULL_HANDLE;
    SwapChain* _swapChain = nullptr;
    DepthBuffer* _depthBuffer = nullptr;
    VkRenderPass _renderPass = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> _framebuffers;

    // create framebuffer objects for all images in the swap chain
    // - framebuffers have to match configuration of _renderPass
    //   (i.e. one color attachment and one depth attachment)
    // - use the image view from _swapChain
    // - use the image view from _depthBuffer
    void create();

    // destroy all framebuffer objects
    void cleanup();

};
