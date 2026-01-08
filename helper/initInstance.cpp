#include "initInstance.hpp"

#include <vector>
#include <set>
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>

#ifndef NDEBUG
static const bool enableValidationLayers = false;
#endif

static const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

/* ============================================================
   Debug Callback
   ============================================================ */
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* userData) {

    std::cerr << "[VALIDATION] " << data->pMessage << std::endl;
    return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT makeDebugCreateInfo() {
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debugCallback;
    return info;
}

/* ============================================================
   Instance
   ============================================================ */
VkInstance InitInstance::createInstance(std::vector<const char*> extensions) {

#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    /* ---- Layer Check ---- */
    if (enableValidationLayers) {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> layers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

        for (const char* name : validationLayers) {
            bool found = false;
            for (auto& l : layers)
                if (strcmp(name, l.layerName) == 0)
                    found = true;

            if (!found)
                 throw std::runtime_error("Validation Layer fehlt!");
        }
    }

    /* ---- App Info ---- */
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Instance";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    /* ---- Debug Create Info ---- */
    VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
    if (enableValidationLayers)
        debugInfo = makeDebugCreateInfo();

    /* ---- Instance Create ---- */
    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &appInfo;
    info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();

#ifdef __APPLE__
    info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    if (enableValidationLayers) {
        info.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        info.ppEnabledLayerNames = validationLayers.data();
        info.pNext = &debugInfo;
    } else {
        info.enabledLayerCount = 0;
        info.ppEnabledLayerNames = nullptr;
    }

    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&info, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("vkCreateInstance failed");

    /* ---- Debug Messenger ---- */
    if (enableValidationLayers) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance,
                "vkCreateDebugUtilsMessengerEXT");

        if (func) {
            func(instance, &debugInfo, nullptr, &debugMessenger);
        }
    }

    std::cout << "Vulkan Instance erstellt\n";
    return instance;
}

void InitInstance::destroyInstance(VkInstance instance) {

    if (enableValidationLayers && debugMessenger != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance,
                "vkDestroyDebugUtilsMessengerEXT");
        if (func)
            func(instance, debugMessenger, nullptr);
    }

    if (instance != VK_NULL_HANDLE)
        vkDestroyInstance(instance, nullptr);
}

/* ============================================================
   Physical Device
   ============================================================ */
VkPhysicalDevice InitInstance::pickPhysicalDevice(
    VkInstance instance,
    Surface* surface,
    uint32_t* graphicsQueueFamilyIndex,
    uint32_t* presentQueueFamilyIndex) {

    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0)
        throw std::runtime_error("Keine GPU gefunden");

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    for (auto dev : devices) {

        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> exts(extCount);
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, exts.data());

        bool hasSwapchain = false;
        for (auto& e : exts)
            if (strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
                hasSwapchain = true;

        if (!hasSwapchain || !surface->isAdequate(dev))
            continue;

        uint32_t qCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, nullptr);
        std::vector<VkQueueFamilyProperties> qfam(qCount);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, qfam.data());

        uint32_t g = UINT32_MAX, p = UINT32_MAX;
        for (uint32_t i = 0; i < qCount; i++) {
            if (qfam[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                g = i;
            if (surface->canQueueFamilyPresent(dev, i))
                p = i;
        }

        if (g != UINT32_MAX && p != UINT32_MAX) {
            *graphicsQueueFamilyIndex = g;
            *presentQueueFamilyIndex = p;
            return dev;
        }
    }

    throw std::runtime_error("Keine geeignete GPU");
}

/* ============================================================
   Logical Device
   ============================================================ */
VkDevice InitInstance::createLogicalDevice(
    VkPhysicalDevice physicalDevice,
    uint32_t gQueue,
    uint32_t pQueue) {

    float priority = 1.0f;
    std::set<uint32_t> families = { gQueue, pQueue };
    std::vector<VkDeviceQueueCreateInfo> queues;

    for (uint32_t f : families) {
        VkDeviceQueueCreateInfo q{};
        q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q.queueFamilyIndex = f;
        q.queueCount = 1;
        q.pQueuePriorities = &priority;
        queues.push_back(q);
    }

    std::vector<const char*> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef __APPLE__
    //extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = static_cast<uint32_t>(queues.size());
    info.pQueueCreateInfos = queues.data();
    info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();
    info.pEnabledFeatures = &features;
    info.enabledLayerCount = 0;

    VkDevice device = VK_NULL_HANDLE;
    if (vkCreateDevice(physicalDevice, &info, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("vkCreateDevice failed");

    return device;
}

void InitInstance::destroyDevice(VkDevice device){
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
        vkDestroyDevice(device, nullptr);
    }
}


// Command Pool

VkCommandPool InitInstance::createCommandPool(VkDevice device, uint32_t family){
    VkCommandPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.queueFamilyIndex = family;
    info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkCommandPool pool;
    if (vkCreateCommandPool(device, &info, nullptr, &pool) != VK_SUCCESS)
        throw std::runtime_error("vkCreateCommandPool failed.");

    return pool;
}

void InitInstance::destroyCommandPool(VkDevice device, VkCommandPool pool){
    if (pool != VK_NULL_HANDLE)
        vkDestroyCommandPool(device, pool, nullptr);
}

//Descriptor-Pool
VkDescriptorPool InitInstance::createDescriptorPool(VkDevice device, uint32_t framesInFlight) {
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, framesInFlight },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, framesInFlight }
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = 0;
    poolInfo.maxSets = framesInFlight; // we will allocate one descriptor set per frame
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkResult res = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    return descriptorPool;
}

void InitInstance::destroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool) {
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }
}

VkDescriptorSetLayout InitInstance::createStandardDescriptorSetLayout(VkDevice device) {
    // Binding 0: Uniform Buffer
    VkDescriptorSetLayoutBinding ubo{};
    ubo.binding = 0;
    ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo.descriptorCount = 1;
    ubo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Binding 1: Combined Image Sampler
    VkDescriptorSetLayoutBinding sampler{};
    sampler.binding = 1;
    sampler.descriptorCount = 1;
    sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo, sampler };

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.pBindings = bindings.data();

    VkDescriptorSetLayout layout;
    if (vkCreateDescriptorSetLayout(device, &info, nullptr, &layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor set layout!");

    return layout;
}
VkDescriptorSetLayout InitInstance::createSnowDescriptorSetLayout(VkDevice device) {
    // Binding 0: UBO (model, view, proj)
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Binding 1: Storage Buffer (Particles)
    VkDescriptorSetLayoutBinding storageBinding{};
    storageBinding.binding = 1;
    storageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    storageBinding.descriptorCount = 1;
    storageBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Binding 2: Texture Sampler
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 2;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {
        uboBinding, storageBinding, samplerBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create snow descriptor set layout!");
    }

    return descriptorSetLayout;
}
VkDescriptorSetLayout InitInstance::createLitDescriptorSetLayout(VkDevice device) {
    // Binding 0: UBO mit Licht-Daten
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // Binding 1: Texture Sampler
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
        uboBinding, samplerBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create lit descriptor set layout!");
    }

    return descriptorSetLayout;
}

void InitInstance::destroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout) {
    if (descriptorSetLayout!= VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout,nullptr);
    }
}


VkDescriptorSetLayout InitInstance::createLightingDescriptorSetLayout(VkDevice device) {
    //4 Bindings: Normal, Albedo, Depth, UBO
    
    //Normal
    VkDescriptorSetLayoutBinding normalBinding{};
    normalBinding.binding = 0;
    normalBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    normalBinding.descriptorCount = 1;
    normalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    //Albedo
    VkDescriptorSetLayoutBinding albedoBinding{};
    albedoBinding.binding = 1;
    albedoBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    albedoBinding.descriptorCount = 1;
    albedoBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    //Depth
    VkDescriptorSetLayoutBinding depthBinding{};
    depthBinding.binding = 2;
    depthBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    depthBinding.descriptorCount = 1;
    depthBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    //UBO
    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 3;  // ✅ Jetzt Binding 3!
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
        normalBinding, albedoBinding, depthBinding, uboBinding
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create lighting descriptor set layout!");
    }

    return descriptorSetLayout;
}