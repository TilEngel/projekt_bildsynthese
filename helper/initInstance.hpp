/*
* Erstellung der Instances, Devices und Command Pools
*/
#pragma once

#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>
#include <set>
#include <cstring>
#include <stdexcept>

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "Surface.hpp"


class InitInstance {
public:
    VkInstance createInstance(std::vector<const char*> extensions);
    void destroyInstance(VkInstance instance);

    VkPhysicalDevice pickPhysicalDevice(
        VkInstance instance,
        Surface* surface,
        uint32_t* graphicsQueueFamilyIndex,
        uint32_t* presentQueueFamilyIndex
    );

    VkDevice createLogicalDevice(
        VkPhysicalDevice physicalDevice,
        uint32_t graphicsQueueFamilyIndex,
        uint32_t presentQueueFamilyIndex
    );

    void destroyDevice(VkDevice device);

    VkCommandPool createCommandPool(VkDevice device, uint32_t graphicsQueueFamilyIndex);
    void destroyCommandPool(VkDevice device, VkCommandPool commandPool);

    VkDescriptorPool createDescriptorPool(VkDevice device, uint32_t framesInFlight);
    void destroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool);
};
