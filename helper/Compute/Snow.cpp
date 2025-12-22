#include "Snow.hpp"

#include <vector>
#include <fstream>
#include <random>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include "../initBuffer.hpp"
#include <array>

// Helper: read file
static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("failed to open file: " + filename);
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

// Helper: Buffer erstellen & Speicher allokieren
static void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, 
                         VkBufferUsageFlags usage, VkMemoryPropertyFlags memProps,
                         VkBuffer &buffer, VkDeviceMemory &bufferMemory) {
    VkBufferCreateInfo bci{};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = size;
    bci.usage = usage;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bci, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer");
    }

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, buffer, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    InitBuffer buff;
    allocInfo.memoryTypeIndex = buff.findMemoryType(memReq.memoryTypeBits, memProps, physicalDevice);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

Snow::Snow(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t queueIndex) 
    : _physicalDevice(physicalDevice)
    , _device(device)
    , _computeQueueIndex(queueIndex) {
    createDescriptorSetLayout();
    createPipelineLayout();
    createPipeline();
    createStorageBuffers();
    createDescriptorPool();
    allocateDescriptorSet();
    updateDescriptorSet();
    createCommandPool();
    allocateCommandBuffer();
    recordCommandBuffer();
    createComputeFence();
}


void Snow::createComputeFence() {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled

    if (vkCreateFence(_device, &fenceInfo, nullptr, &_computeFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute fence");
    }
}

void Snow::waitForCompute() {
    if (_computeFence != VK_NULL_HANDLE) {
        vkWaitForFences(_device, 1, &_computeFence, VK_TRUE, UINT64_MAX);
        vkResetFences(_device, 1, &_computeFence);
    }
}

//Layout für den ComputeShader
void Snow::createDescriptorSetLayout() {
    //StorageBuffer für die beiden Bindings
    VkDescriptorSetLayoutBinding b0{};
    b0.binding = 0;
    b0.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    b0.descriptorCount = 1;
    b0.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding b1{};
    b1.binding = 1;
    b1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    b1.descriptorCount = 1;
    b1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { b0, b1 };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout");
    }
}

void Snow::createPipelineLayout() {
    VkPipelineLayoutCreateInfo pli{};
    pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &_descriptorSetLayout;

    if (vkCreatePipelineLayout(_device, &pli, nullptr, &_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }
}

void Snow::createPipeline() {
    auto code = readFile("shaders/snow.comp.spv");
    
    VkShaderModuleCreateInfo smci{};
    smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smci.codeSize = code.size();
    smci.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_device, &smci, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pci{};
    pci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pci.stage = stageInfo;
    pci.layout = _pipelineLayout;

    if (vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &pci, nullptr, &_computePipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(_device, shaderModule, nullptr);
        throw std::runtime_error("failed to create compute pipeline");
    }

    vkDestroyShaderModule(_device, shaderModule, nullptr);
}

//Erstellt Buffer für die Daten der Partikel
void Snow::createStorageBuffers() {
    std::vector<Particle> particles(NUMBER_PARTICLES);
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Schneeflocken verteilen sich über die Szene
    std::uniform_real_distribution<float> posX(-2.8f, 0.8f);
    std::uniform_real_distribution<float> posY(5.0f, 5.5f);  // Starten oben
    std::uniform_real_distribution<float> posZ(-2.0f, 1.0f);
    
    // Langsame Fallgeschwindigkeit mit leichter Variation
    std::uniform_real_distribution<float> velX(-0.008f, 0.008f);
    std::uniform_real_distribution<float> velY(-0.01f, -0.0005f);  // Langsam nach unten
    std::uniform_real_distribution<float> velZ(-0.008f, 0.008f);

    for (uint32_t i = 0; i < NUMBER_PARTICLES; ++i) {
        particles[i].position = glm::vec3(posX(gen), posY(gen), posZ(gen));
        particles[i].velocity = glm::vec3(velX(gen), velY(gen), velZ(gen));
    }

    VkDeviceSize bufSize = sizeof(Particle) * NUMBER_PARTICLES;

    // Buffer mit Initialdaten der Schneeflocken
    createBuffer(_physicalDevice, _device, bufSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 _initBuffer, _initBufferMemory);

    void* data;
    vkMapMemory(_device, _initBufferMemory, 0, bufSize, 0, &data);
    std::memcpy(data, particles.data(), static_cast<size_t>(bufSize));
    vkUnmapMemory(_device, _initBufferMemory);

    //Buffer mit aktuellen Daten
    createBuffer(_physicalDevice, _device, bufSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 _currBuffer, _currBufferMemory);

    vkMapMemory(_device, _currBufferMemory, 0, bufSize, 0, &data);
    std::memcpy(data, particles.data(), static_cast<size_t>(bufSize));
    vkUnmapMemory(_device, _currBufferMemory);
}

void Snow::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 1> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = 2;

    VkDescriptorPoolCreateInfo dpci{};
    dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.maxSets = 1;
    dpci.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    dpci.pPoolSizes = poolSizes.data();

    if (vkCreateDescriptorPool(_device, &dpci, nullptr, &_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool");
    }
}

void Snow::allocateDescriptorSet() {
    VkDescriptorSetAllocateInfo dsai{};
    dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.descriptorPool = _descriptorPool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &_descriptorSetLayout;

    if (vkAllocateDescriptorSets(_device, &dsai, &_descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set");
    }
}

void Snow::updateDescriptorSet() {
    VkDescriptorBufferInfo initInfo{};
    initInfo.buffer = _initBuffer;
    initInfo.offset = 0;
    initInfo.range = sizeof(Particle) * NUMBER_PARTICLES;

    VkDescriptorBufferInfo currInfo{};
    currInfo.buffer = _currBuffer;
    currInfo.offset = 0;
    currInfo.range = sizeof(Particle) * NUMBER_PARTICLES;

    VkWriteDescriptorSet w0{};
    w0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w0.dstSet = _descriptorSet;
    w0.dstBinding = 0;
    w0.dstArrayElement = 0;
    w0.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    w0.descriptorCount = 1;
    w0.pBufferInfo = &initInfo;

    VkWriteDescriptorSet w1{};
    w1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w1.dstSet = _descriptorSet;
    w1.dstBinding = 1;
    w1.dstArrayElement = 0;
    w1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    w1.descriptorCount = 1;
    w1.pBufferInfo = &currInfo;

    std::array<VkWriteDescriptorSet, 2> writes = { w0, w1 };
    vkUpdateDescriptorSets(_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void Snow::createCommandPool() {
    VkCommandPoolCreateInfo cpci{};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.queueFamilyIndex = _computeQueueIndex;
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(_device, &cpci, nullptr, &_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool for compute");
    }
}

void Snow::allocateCommandBuffer() {
    VkCommandBufferAllocateInfo cbai{};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = _commandPool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(_device, &cbai, &_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer");
    }
}

void Snow::recordCommandBuffer() {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;

    if (vkBeginCommandBuffer(_commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer");
    }

    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _computePipeline);
    vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, 
                           _pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);

    vkCmdDispatch(_commandBuffer, 1, 1, 1);

    if (vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }
}

void Snow::destroy() {
    if (_computeFence != VK_NULL_HANDLE) {  // NEU
        vkDestroyFence(_device, _computeFence, nullptr);
        _computeFence = VK_NULL_HANDLE;
    }
    if (_commandPool != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);
        vkDestroyCommandPool(_device, _commandPool, nullptr);
        _commandPool = VK_NULL_HANDLE;
    }
    if (_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
        _descriptorPool = VK_NULL_HANDLE;
    }
    if (_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_device, _descriptorSetLayout, nullptr);
        _descriptorSetLayout = VK_NULL_HANDLE;
    }
    if (_computePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_device, _computePipeline, nullptr);
        _computePipeline = VK_NULL_HANDLE;
    }
    if (_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
    if (_initBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(_device, _initBuffer, nullptr);
        _initBuffer = VK_NULL_HANDLE;
    }
    if (_initBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _initBufferMemory, nullptr);
        _initBufferMemory = VK_NULL_HANDLE;
    }
    if (_currBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(_device, _currBuffer, nullptr);
        _currBuffer = VK_NULL_HANDLE;
    }
    if (_currBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _currBufferMemory, nullptr);
        _currBufferMemory = VK_NULL_HANDLE;
    }
}
