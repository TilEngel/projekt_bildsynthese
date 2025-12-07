// Surface.cpp
#include "Surface.hpp"

#include <vector>
#include <stdexcept>
#include <iostream>

bool Surface::canQueueFamilyPresent(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) {
    VkBool32 presentSupport = VK_FALSE;
    VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, _surface, &presentSupport);
    if (res != VK_SUCCESS) {
        std::cerr << "vkGetPhysicalDeviceSurfaceSupportKHR failed (result " << res << ")\n";
        return false;
    }
    return presentSupport == VK_TRUE;
}

bool Surface::isAdequate(VkPhysicalDevice physicalDevice) {
    // prüfe, ob es Formate und Present-Modes gibt
    auto formats = queryFormats(physicalDevice);
    auto presentModes = queryPresentModes(physicalDevice);

    bool ok = !formats.empty() && !presentModes.empty();
    if (!ok) {
        std::cout << "Surface ist nicht adequate: formats.size()=" << formats.size()
                  << " presentModes.size()=" << presentModes.size() << "\n";
    }
    return ok;
}

VkSurfaceCapabilitiesKHR Surface::queryCapabilities(VkPhysicalDevice physicalDevice) {
    VkSurfaceCapabilitiesKHR caps{};
    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, &caps);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
    }
    return caps;
}

std::vector<VkSurfaceFormatKHR> Surface::queryFormats(VkPhysicalDevice physicalDevice) {
    uint32_t count = 0;
    VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &count, nullptr);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("vkGetPhysicalDeviceSurfaceFormatsKHR (count) failed");
    }

    if (count == 0) {
        return {};
    }

    std::vector<VkSurfaceFormatKHR> formats(count);
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &count, formats.data());
    if (res != VK_SUCCESS) {
        throw std::runtime_error("vkGetPhysicalDeviceSurfaceFormatsKHR (fetch) failed");
    }

    return formats;
}

std::vector<VkPresentModeKHR> Surface::queryPresentModes(VkPhysicalDevice physicalDevice) {
    uint32_t count = 0;
    VkResult res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &count, nullptr);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("vkGetPhysicalDeviceSurfacePresentModesKHR (count) failed");
    }

    if (count == 0) {
        return {};
    }

    std::vector<VkPresentModeKHR> modes(count);
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &count, modes.data());
    if (res != VK_SUCCESS) {
        throw std::runtime_error("vkGetPhysicalDeviceSurfacePresentModesKHR (fetch) failed");
    }

    return modes;
}
