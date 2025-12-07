#pragma once

#include "Window.hpp"

class Surface {
public:
    Surface(Window* window, VkInstance instance)
    : _window(window)
    ,_instance(instance) {
        _surface = _window->createSurface(instance);
    }

    ~Surface() {
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
    }

    VkExtent2D getExtent() {
        return _window->getExtent();
    }

    VkSurfaceKHR getSurface() {
        return _surface;
    }

    // call vkGetPhysicalDeviceSurfaceSupportKHR and return the result
    bool canQueueFamilyPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex);

    // call queryFormats and queryPresentModes
    // return true if both are not empty, otherwise return false
    bool isAdequate(VkPhysicalDevice physicalDevice);

    // return the surface capabilities for the physical device
    VkSurfaceCapabilitiesKHR queryCapabilities(VkPhysicalDevice physicalDevice);

    // return the surface formats for the physical device
    std::vector<VkSurfaceFormatKHR> queryFormats(VkPhysicalDevice physicalDevice);

    // return the present modes for the physical device
    std::vector<VkPresentModeKHR> queryPresentModes(VkPhysicalDevice physicalDevice);

private:
    Window* _window = nullptr;
    VkInstance _instance = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
};
