#pragma once

#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "Surface.hpp"

class SwapChain {
public:
    SwapChain(Surface* surface, VkPhysicalDevice physicalDevice, VkDevice device, VkQueue presentQueue, uint32_t graphicsQueueFamilyIndex, uint32_t presentQueueFamilyIndex)
    : _surface(surface)
    , _physicalDevice(physicalDevice)
    , _device(device)
    , _presentQueue(presentQueue)
    , _graphicsQueueFamilyIndex(graphicsQueueFamilyIndex)
    , _presentQueueFamilyIndex(presentQueueFamilyIndex) {
        create();
        createImageViews();
        createSemaphores();
    }

    ~SwapChain() {
        cleanupSemaphores();
        cleanupImageViews();
        cleanup();
    }

    uint32_t getImageCount() {
        return _images.size();
    }

    VkFormat getImageFormat() {
        return _imageFormat;
    }

    VkImageView getImageView(size_t index) {
        return _swapChainImageViews.at(index);
    }

    VkSemaphore getPresentationSemaphore(size_t index) {
        return _presentationSemaphores.at(index);
    }

    VkExtent2D getExtent() {
        return _extent;
    }

    void recreate() {
        cleanupImageViews();
        cleanup();
        create();
        createImageViews();
    }

    // acquire the next image of the swap chain
    // call vkAcquireNextImageKHR
    // return the image index
    // throw error if the result is not "success" or "suboptimal"
    uint32_t acquireNextImage(VkSemaphore semaphore, VkFence fence);

    // present the image with index imageIndex
    // call vkQueuePresentKHR
    // wait for the images semaphore
    // return true, if result is "error out of date" or "suboptimal"
    // else throw error, if the result is not "success"
    // else return false
    // (return value indicates, if swap chain has to be recreated)
    bool presentImage(uint32_t imageIndex);


    public:

    const std::vector<VkImageView>& getImageViews() const {
        return _swapChainImageViews;
    }



private:
    Surface* _surface = nullptr;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkQueue _presentQueue = VK_NULL_HANDLE;
    uint32_t _graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t _presentQueueFamilyIndex = UINT32_MAX;

    VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
    VkExtent2D _extent;
    
    VkFormat _imageFormat = VK_FORMAT_UNDEFINED;;
    std::vector<VkImage> _images;
    std::vector<VkImageView> _swapChainImageViews;

    std::vector<VkSemaphore> _presentationSemaphores;
   
    // create a new swap chain and get all images
    // - query formats, present modes and capabilites from surface
    // - choose a surface format with VK_FORMAT_B8G8R8A8_SRGB and
    //   VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, or just select the first
    //   from the list
    // - set _imageFormat to the selected surface format
    // - choose present mode mailbox if available, otherwise choose
    //   present mode FIFO
    // - choose the image extent and set member variable _extent accordingly:
    //   - if available set _extent to capabilities.currentExtent
    //   - otherwise get extent from surface, but also consider
    //     capabilities.minImageExtent and capabilities.maxImageExtent
    // - choose an image count of capabilities.minImageCount + 1
    //   but also consider capabilities.maxImageCount
    // - choose an image sharing mode
    //   - if graphics and presentation queue family indices are the same,
    //     sharing mode can be "exclusive"
    //   - otherwise the sharing mode has to be "concurrent" and the queue
    //     queue family indices have to be set in the create info
    // - create the swap chain and save it to member variable _swapChain
    // - get all swap chain images and save them to _images
    void create();

    // create image views for all swapchain images
    void createImageViews();

    // create semaphores for all swapchain images
    void createSemaphores();

    // destroy the swap chain
    void cleanup();

    // destroy the image views for the swap chain
    void cleanupImageViews();

    // destroy the semaphores
    void cleanupSemaphores();
};
