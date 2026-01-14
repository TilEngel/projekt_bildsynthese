#include "Swapchain.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <iostream>

void SwapChain::create() {
    //Query Surface Information
    auto formats       = _surface->queryFormats(_physicalDevice);
    auto presentModes  = _surface->queryPresentModes(_physicalDevice);
    auto capabilities  = _surface->queryCapabilities(_physicalDevice);

    if (formats.empty() || presentModes.empty()) {
        throw std::runtime_error("SwapChain creation failed: no valid formats or present modes.");
    }
    // Surface Format wählen
    VkSurfaceFormatKHR selectedFormat = formats[0];

    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM &&
            f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            selectedFormat = f;
            break;
        }
    }
    // Fallback auf SRGB, falls UNORM nicht verfügbar
    if (selectedFormat.format != VK_FORMAT_B8G8R8A8_UNORM) {
        for (const auto& f : formats) {
            if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
                f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                selectedFormat = f;
                break;
            }
        }
    }

    _imageFormat = selectedFormat.format;

    //Present Mode wählen
    VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& pm : presentModes) {
        if (pm == VK_PRESENT_MODE_MAILBOX_KHR) {
            selectedPresentMode = pm;
            break;
        }
    }

    //Choose Swap Extent

    if (capabilities.currentExtent.width != UINT32_MAX) {
        // Vulkan darf Extent vorgeben
        _extent = capabilities.currentExtent;
    } else {
        //Fenstergröße benutzen
        VkExtent2D windowExtent = _surface->getExtent();

        _extent.width  = std::clamp(windowExtent.width,
                                    capabilities.minImageExtent.width,
                                    capabilities.maxImageExtent.width);

        _extent.height = std::clamp(windowExtent.height,
                                    capabilities.minImageExtent.height,
                                    capabilities.maxImageExtent.height);
    }

    //Image Count wählen
    uint32_t imageCount = capabilities.minImageCount + 1;

    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    //Sharing Mode auswählen

    uint32_t qIndices[2] = { _graphicsQueueFamilyIndex, _presentQueueFamilyIndex };
    bool sameFamily = (_graphicsQueueFamilyIndex == _presentQueueFamilyIndex);

    VkSwapchainCreateInfoKHR info{};
    info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface          = _surface->getSurface();
    info.minImageCount    = imageCount;
    info.imageFormat      = selectedFormat.format;
    info.imageColorSpace  = selectedFormat.colorSpace;
    info.imageExtent      = _extent;
    info.imageArrayLayers = 1;
    info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (sameFamily) {
        info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount = 0;
        info.pQueueFamilyIndices   = nullptr;
    } else {
        info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices   = qIndices;
    }

    info.preTransform   = capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode    = selectedPresentMode;
    info.clipped        = VK_TRUE;
    info.oldSwapchain   = VK_NULL_HANDLE;

    //SwapChain erstellen
    if (vkCreateSwapchainKHR(_device, &info, nullptr, &_swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create SwapChain!");
    }


    // SwapChain Images holen
    uint32_t count = 0;
    vkGetSwapchainImagesKHR(_device, _swapChain, &count, nullptr);

    _images.resize(count);
    vkGetSwapchainImagesKHR(_device, _swapChain, &count, _images.data());


    std::cout << "SwapChain created with " << count
              << " images, format=" << selectedFormat.format
              << " extent=" << _extent.width << "x" << _extent.height << "\n";
}


// Destroy
void SwapChain::cleanup() {
    if (_swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_device, _swapChain, nullptr);
        _swapChain = VK_NULL_HANDLE;
    }

    _images.clear();
}


//image views für swapchain images erstellen
void SwapChain::createImageViews() {
    cleanupImageViews(); //sicherstellen dass keine alten Views existieren

    _swapChainImageViews.reserve(_images.size());

    for (size_t i = 0; i < _images.size(); ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = _imageFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView = VK_NULL_HANDLE;
        if (vkCreateImageView(_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views for swap chain!");
        }

        _swapChainImageViews.push_back(imageView);
    }

    std::cout << "Created " << _swapChainImageViews.size() << " swapchain image views\n";
}


//semaphores ür swapchain images erstellen
void SwapChain::createSemaphores() {
    cleanupSemaphores(); // alte Semaphores zerstören

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.flags = 0;

    _presentationSemaphores.reserve(_images.size());

    for (size_t i = 0; i < _images.size(); ++i) {
        VkSemaphore sem = VK_NULL_HANDLE;
        if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &sem) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create presentation semaphore!");
        }
        _presentationSemaphores.push_back(sem);
    }

    std::cout << "Created " << _presentationSemaphores.size() << " presentation semaphores\n";
}

//image views für swap chain zerstören
void SwapChain::cleanupImageViews() {
    for (VkImageView view : _swapChainImageViews) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(_device, view, nullptr);
        }
    }
    _swapChainImageViews.clear();
}


// destroy semaphores
void SwapChain::cleanupSemaphores() {
    for (VkSemaphore sem : _presentationSemaphores) {
        if (sem != VK_NULL_HANDLE) {
            vkDestroySemaphore(_device, sem, nullptr);
        }
    }
    _presentationSemaphores.clear();
}



//nächstes Image aus Swapchain holen
uint32_t SwapChain::acquireNextImage(VkSemaphore semaphore, VkFence fence) {
    if (_swapChain == VK_NULL_HANDLE) {
        throw std::runtime_error("AcquireNextImage called but swapchain is not created.");
    }

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(_device,
                                            _swapChain,
                                            UINT64_MAX,
                                            semaphore,
                                            fence,
                                            &imageIndex);

    if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) {
        return imageIndex;
    } else {
        throw std::runtime_error("Failed to acquire next swap chain image (vkAcquireNextImageKHR returned error).");
    }
}


// present Image mit index imageIndex
bool SwapChain::presentImage(uint32_t imageIndex) {
    if (_swapChain == VK_NULL_HANDLE) {
        throw std::runtime_error("PresentImage called but swapchain is not created.");
    }

    // choose the semaphore associated with this image as wait semaphore
    if (imageIndex >= _presentationSemaphores.size()) {
        throw std::runtime_error("PresentImage: imageIndex out of range for presentation semaphores.");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_presentationSemaphores[imageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapChain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(_presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // swapchain muss möglicherweise neu erstellt werden
        return true;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image (vkQueuePresentKHR returned error).");
    }

    return false;
}

