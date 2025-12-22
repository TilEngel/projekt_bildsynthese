#include "initInstance.hpp"


VkInstance InitInstance::createInstance(std::vector<const char*> extensions){
    //Validation Layers (nur falls vorhanden)
    std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef __APPLE__
    // macOS benötigt diese Erweiterungen
    extensions.push_back("VK_KHR_portability_enumeration");
    extensions.push_back("VK_KHR_get_physical_device_properties2");
#endif
    // Prüfe Layer 
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool found = false;

        for (auto& l : availableLayers)
            if (strcmp(layerName, l.layerName) == 0){
                found = true;
            }
        if (!found){
            std::cerr << "[WARN] Validation Layer fehlt: " << layerName << "\n";
        }
    }

    // Prüfe extensions
    uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, availableExtensions.data());

    for (auto* extName : extensions) {
        bool found = false;
        for (auto& ext : availableExtensions)
            if (strcmp(extName, ext.extensionName) == 0)
                found = true;

        if (!found)
            std::cerr << "[WARN] Fehlende Extension: " << extName << "\n";
    }


    // Application Info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Instance";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;


    // Instance Create Info
    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &appInfo;

    info.enabledExtensionCount = extensions.size();
    info.ppEnabledExtensionNames = extensions.data();

#ifdef __APPLE__
    //portability-flag notwendig unter macOS
    info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // Keine Layers aktivieren für moltenVK
    info.enabledLayerCount = 0;
    info.ppEnabledLayerNames = nullptr;

    // Instance erstellen
    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&info, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("vkCreateInstance failed.");

    std::cout << "Vulkan Instance erfolgreich erstellt.\n";
    return instance;
}

//Instance zerstören
void InitInstance::destroyInstance(VkInstance instance){
    if (instance != VK_NULL_HANDLE){
        vkDestroyInstance(instance, nullptr);
    }
}


//Physical Device wählen
VkPhysicalDevice InitInstance::pickPhysicalDevice(VkInstance instance,Surface* surface,uint32_t* graphicsQueueFamilyIndex, uint32_t* presentQueueFamilyIndex){
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (count == 0){
        throw std::runtime_error("Keine Vulkan-kompatible GPU gefunden.");
    }

    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data());

    for (auto dev : devices) {

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);

        // Prüfe Swapchain Support
        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);

        std::vector<VkExtensionProperties> exts(extCount);
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, exts.data());

        bool hasSwapchain = false;
        for (auto& ext : exts)
            if (strcmp(ext.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0){
                hasSwapchain = true;
            }

            if (!hasSwapchain)continue;
            // Prüfe Surface
            if (!surface->isAdequate(dev)) continue;
            //Queuefamily finden
            uint32_t qCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, nullptr);

            std::vector<VkQueueFamilyProperties> qfam(qCount);
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &qCount, qfam.data());

            uint32_t g = UINT32_MAX, p = UINT32_MAX;

            for (uint32_t i = 0; i < qCount; i++){
                if (qfam[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
                    g = i;
                }

                if (surface->canQueueFamilyPresent(dev, i)){
                    p = i;
                }

                if (g != UINT32_MAX && p != UINT32_MAX){
                    break;
                }
            }

            if (g == UINT32_MAX || p == UINT32_MAX)
                continue;

            *graphicsQueueFamilyIndex = g;
            *presentQueueFamilyIndex  = p;

            std::cout << "GPU ausgewählt: " << props.deviceName << "\n";
            return dev;
    }
    throw std::runtime_error("Keine geeignete GPU gefunden.");
}


// Logical Device
VkDevice InitInstance::createLogicalDevice(VkPhysicalDevice physicalDevice,uint32_t gQueue,uint32_t pQueue){
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
    extensions.push_back("VK_KHR_portability_subset");
#endif

    VkPhysicalDeviceFeatures features{};
    features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = queues.size();
    info.pQueueCreateInfos = queues.data();
    info.enabledExtensionCount = extensions.size();
    info.ppEnabledExtensionNames = extensions.data();
    info.pEnabledFeatures = &features;

    info.enabledLayerCount = 0;

    VkDevice device = VK_NULL_HANDLE;
    if (vkCreateDevice(physicalDevice, &info, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("vkCreateDevice failed.");

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