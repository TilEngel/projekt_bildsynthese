/*
* Erstellung und Zerstörung der Instances, Devices, Command Pools und DescriptorSetLayouts
* Überbleibsel aus den Übungen - erledigt boilerplate-Code
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
#include "Rendering/Surface.hpp"


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

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    void destroyDevice(VkDevice device);

    VkCommandPool createCommandPool(VkDevice device, uint32_t graphicsQueueFamilyIndex);
    void destroyCommandPool(VkDevice device, VkCommandPool commandPool);

    VkDescriptorPool createDescriptorPool(VkDevice device, uint32_t framesInFlight);
    void destroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool);

    VkDescriptorSetLayout createStandardDescriptorSetLayout(VkDevice device);

    // != snow::createDescriptorSetLayout (für ComputeShader)
    //erstellt DSL für render-shader
    VkDescriptorSetLayout createSnowDescriptorSetLayout(VkDevice device);
    //Für beleuchtete Objekte
    VkDescriptorSetLayout createLitDescriptorSetLayout(VkDevice device);
    //Für deferredShading
    VkDescriptorSetLayout createLightingDescriptorSetLayout(VkDevice device);

    void destroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);
};
