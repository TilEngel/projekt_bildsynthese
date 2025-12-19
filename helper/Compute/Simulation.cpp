// Simulation.cpp
#include "../Compute/Simulation.hpp"
#include "../initBuffer.hpp"
#include <vector>
#include <fstream>
#include <random>
#include <iostream>
#include <stdexcept>
#include <cstring> 



// helper: read file
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

// create buffer + allocate memory
static void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProps,
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
    allocInfo.memoryTypeIndex = _buff.findMemoryType(memProps, memReq.memoryTypeBits, physicalDevice);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

// ----------------- class member implementations -----------------

void Simulation::createDescriptorSetLayout() {
    //Zwei Bindings anlegen
    VkDescriptorSetLayoutBinding b0{};
    b0.binding = 0;
    b0.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    b0.descriptorCount = 1;
    b0.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    b0.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding b1{};
    b1.binding = 1;
    b1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    b1.descriptorCount = 1;
    b1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    b1.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { b0, b1 };

    //Layout info
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout");
    }
}

void Simulation::createPipelineLayout() {
    VkPipelineLayoutCreateInfo pli{};
    pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &_descriptorSetLayout;
    pli.pushConstantRangeCount = 0;
    pli.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(_device, &pli, nullptr, &_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }
}

void Simulation::createPipeline() {
    // daten aus Shader holen
    auto code = readFile("shaders/testapp.comp.spv");
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

void Simulation::createStorageBuffers() {
    //partikel mit geschwindigkeit speichern
    std::vector<Particle> particles(NUMBER_PARTICLES);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> vxvz(-0.25f, 0.25f);
    std::uniform_real_distribution<float> vy(2.0f, 2.5f);

    for (uint32_t i = 0; i < NUMBER_PARTICLES; ++i) {
        particles[i].position = glm::vec3(0.0f);
        particles[i].velocity = glm::vec3(vxvz(gen), vy(gen), vxvz(gen));
    }

    VkDeviceSize bufSize = sizeof(Particle) * NUMBER_PARTICLES;

   
    createBuffer(_physicalDevice, _device, bufSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 _initBuffer, _initBufferMemory);

    // map & copy initial data
    void* data;
    vkMapMemory(_device, _initBufferMemory, 0, bufSize, 0, &data);
    std::memcpy(data, particles.data(), static_cast<size_t>(bufSize));
    vkUnmapMemory(_device, _initBufferMemory);

    // create curr buffer (also host visible; could be device local with staging)
    createBuffer(_physicalDevice, _device, bufSize,
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 _currBuffer, _currBufferMemory);

    // copy initial content into current buffer as well
    vkMapMemory(_device, _currBufferMemory, 0, bufSize, 0, &data);
    std::memcpy(data, particles.data(), static_cast<size_t>(bufSize));
    vkUnmapMemory(_device, _currBufferMemory);
}

void Simulation::createDescriptorPool(){

    std::array<VkDescriptorPoolSize, 1> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[0].descriptorCount = 2; // two storage buffers

    VkDescriptorPoolCreateInfo dpci{};
    dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.maxSets = 1;
    dpci.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    dpci.pPoolSizes = poolSizes.data();


    if (vkCreateDescriptorPool(_device, &dpci, nullptr, &_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool");
    }
}

void Simulation::allocateDescriptorSet(){
    VkDescriptorSetAllocateInfo dsai{};
    dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.descriptorPool = _descriptorPool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &_descriptorSetLayout;

    if (vkAllocateDescriptorSets(_device, &dsai, &_descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor set");
    }
}

void Simulation::updateDescriptorSet() {
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

void Simulation::createCommandPool() {

    VkCommandPoolCreateInfo cpci{};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.queueFamilyIndex = _computeQueueIndex;
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(_device, &cpci, nullptr, &_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool for compute");
    }
}

void Simulation::allocateCommandBuffer() {
    VkCommandBufferAllocateInfo cbai{};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = _commandPool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(_device, &cbai, &_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer");
    }
}

void Simulation::recordCommandBuffer() {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    if (vkBeginCommandBuffer(_commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer");
    }

    // bind compute pipeline & descriptor set
    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _computePipeline);
    vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);

    // we have local_size_x = 256 and NUMBER_PARTICLES = 256 -> a single workgroup suffices
    vkCmdDispatch(_commandBuffer, 1, 1, 1);

    if (vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }
}

void Simulation::destroy() {
    if (_commandPool != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device); // ensure device idle
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
