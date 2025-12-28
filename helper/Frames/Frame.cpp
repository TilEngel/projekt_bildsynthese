// Frame.cpp
#include "Frame.hpp"

#include <stdexcept>
#include <array>
#include <cstring>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "../Compute/Snow.hpp"



void Frame::createUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    //buffer erstellen
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &_uniformBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create uniform buffer!");
    }

    //Speicher allokieren 
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, _uniformBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    //MemoryType finden
    allocInfo.memoryTypeIndex = _buff.findMemoryType( memRequirements.memoryTypeBits,
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _physicalDevice);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_uniformBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate uniform buffer memory!");
    }

    vkBindBufferMemory(_device, _uniformBuffer, _uniformBufferMemory, 0);

    //Speicher mappen
    void* data = nullptr;
    vkMapMemory(_device, _uniformBufferMemory, 0, bufferSize, 0, &data);
    _uniformBufferMapped = static_cast<UniformBufferObject*>(data);
}

void Frame::allocateDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, size_t objectCount) {
    _descriptorSets.resize(objectCount);

    std::vector<VkDescriptorSetLayout> layouts(objectCount, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(objectCount);
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(_device, &allocInfo, _descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets for frame!");
    }
}


void Frame::allocateCommandBuffer(VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(_device, &allocInfo, &_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffer!");
    }
}

void Frame::createSyncObjects() {
    // semaphore
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(_device, &semInfo, nullptr, &_renderSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render semaphore!");
    }

    // fence
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to create fence!");
    }
}

void Frame::cleanup() {
    if (_uniformBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(_device, _uniformBuffer, nullptr);
        _uniformBuffer = VK_NULL_HANDLE;
    }
    if (_uniformBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _uniformBufferMemory, nullptr);
        _uniformBufferMemory = VK_NULL_HANDLE;
    }
    if (_renderSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(_device, _renderSemaphore, nullptr);
        _renderSemaphore = VK_NULL_HANDLE;
    }
    if (_inFlightFence != VK_NULL_HANDLE) {
        vkDestroyFence(_device, _inFlightFence, nullptr);
        _inFlightFence = VK_NULL_HANDLE;
    }
   
}

void Frame::waitForFence() {
    //auf vorherigen Frame warten
    if (_inFlightFence != VK_NULL_HANDLE) {
        vkWaitForFences(_device, 1, &_inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(_device, 1, &_inFlightFence);
    }

}



void Frame::updateDescriptorSet(Scene* scene) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    // NUR Lichtquellen-RenderObjects!
    size_t normalObjIdx = 0;
    for (const auto& light : scene->getLights()) {
        if (normalObjIdx >= _descriptorSets.size()) {
            std::cerr << "ERROR: Too many lights for descriptor sets\n";
            break;
        }
        
        const auto& obj = light.renderObject;

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = obj.textureImageView;
        imageInfo.sampler = obj.textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = _descriptorSets[normalObjIdx];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = _descriptorSets[normalObjIdx];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(_device, 2, descriptorWrites.data(), 0, nullptr);
        
        normalObjIdx++;
    }
}

void Frame::updateUniformBuffer(Camera* camera) {
    if (!_uniformBufferMapped) return;

    UniformBufferObject ubo{};

    // View-Matrix von der Kamera
    ubo.view = camera->getViewMatrix();

    // Projection
    VkExtent2D extent = _swapChain->getExtent();
    float width = static_cast<float>(extent.width);
    float height = static_cast<float>(extent.height);

    ubo.proj = glm::perspective(
        glm::radians(camera->getZoom()),
        width / height,
        0.1f,       //Near-Plane
        100.0f      //Far-Plane
    );
    ubo.proj[1][1] *= -1.0f;

    std::memcpy(_uniformBufferMapped, &ubo, sizeof(ubo));
}


void Frame::recordCommandBuffer(Scene* scene, uint32_t imageIndex, bool useDeferred) {
    vkResetCommandBuffer(_commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;

    if (vkBeginCommandBuffer(_commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPass rp = scene->getRenderPass();
    VkFramebuffer fb = _framebuffers->getFramebuffer(imageIndex);
    
    if (rp == VK_NULL_HANDLE || fb == VK_NULL_HANDLE) {
        throw std::runtime_error("invalid renderpass or framebuffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = rp;
    renderPassInfo.framebuffer = fb;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = _swapChain->getExtent();

    // Clear values für alle Attachments
    std::array<VkClearValue, 5> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} }; // Swapchain
    clearValues[1].depthStencil = { 1.0f, 0 };            // Depth
    clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} }; // Albedo
    clearValues[3].color = { {0.0f, 0.0f, 0.0f, 1.0f} }; // Normal
    clearValues[4].color = { {0.0f, 0.0f, 0.0f, 1.0f} }; // Position

    renderPassInfo.clearValueCount = useDeferred ? static_cast<uint32_t>(clearValues.size()) : 2;
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapChain->getExtent().width);
    viewport.height = static_cast<float>(_swapChain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _swapChain->getExtent();
    vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);

    if (useDeferred) {
        // === SUBPASS 0: G-Buffer Generation ===
        size_t deferredObjIdx = 0;
        
        for (size_t i = 0; i < scene->getObjectCount(); i++) {
            // Skip Schnee und Lichtquellen-RenderObjects
            if (scene->isSnowObject(i)) continue;
            
            // Lichtquellen-RenderObjects überspringen (sie haben keine Beleuchtung)
            bool isLightRenderObject = false;
            for (const auto& light : scene->getLights()) {
                // Vergleiche Vertex Buffer (primitiv aber funktioniert)
                if (scene->getObject(i).vertexBuffer == light.renderObject.vertexBuffer) {
                    isLightRenderObject = true;
                    break;
                }
            }
            if (isLightRenderObject) continue;
            
            const auto& obj = scene->getObject(i);
            
            if (obj.vertexCount == 0 || obj.vertexBuffer == VK_NULL_HANDLE) {
                continue;
            }

            VkPipeline pipelineHandle = obj.pipeline->getPipeline();
            vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineHandle);

            VkPipelineLayout pipelineLayout = obj.pipeline->getPipelineLayout();
            
            if (deferredObjIdx >= _deferredDescriptorSets.size()) {
                std::cerr << "ERROR: Deferred descriptor set index " << deferredObjIdx 
                         << " out of range (max: " << _deferredDescriptorSets.size() << ")\n";
                continue;
            }
            
            vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   pipelineLayout, 0, 1, &_deferredDescriptorSets[deferredObjIdx], 0, nullptr);

            VkBuffer vertexBuffers[] = { obj.vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdPushConstants(_commandBuffer, pipelineLayout, 
                              VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &obj.modelMatrix);

            vkCmdDraw(_commandBuffer, obj.vertexCount, 1, 0, 0);
            
            deferredObjIdx++;
        }

        // === SUBPASS 1: Lighting Pass ===
        vkCmdNextSubpass(_commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        
        // Viewport/Scissor nochmal setzen (für manche Treiber wichtig)
        vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
        
        // Lighting Pipeline
        if (_lightingPipeline != VK_NULL_HANDLE) {
            vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _lightingPipeline);
            
            vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   _lightingPipelineLayout, 0, 1, 
                                   &_deferredLightingDescriptorSet, 0, nullptr);
            
            // Fullscreen Triangle (3 vertices, kein Vertex Buffer)
            vkCmdDraw(_commandBuffer, 3, 1, 0, 0);
        }
        
        // Lichtquellen-Visualisierung (kleine Kugeln)
        size_t normalObjIdx = 0;
        for (const auto& light : scene->getLights()) {
            const auto& obj = light.renderObject;
            
            if (normalObjIdx >= _descriptorSets.size()) {
                std::cerr << "ERROR: Not enough descriptor sets for light sources\n";
                break;
            }
            
            VkPipeline pipelineHandle = obj.pipeline->getPipeline();
            vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineHandle);

            VkPipelineLayout pipelineLayout = obj.pipeline->getPipelineLayout();
            
            vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   pipelineLayout, 0, 1, &_descriptorSets[normalObjIdx], 0, nullptr);
            
            VkBuffer vertexBuffers[] = { obj.vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdPushConstants(_commandBuffer, pipelineLayout, 
                              VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &obj.modelMatrix);

            vkCmdDraw(_commandBuffer, obj.vertexCount, 1, 0, 0);
            normalObjIdx++;
        }
        
        // Schnee rendern
        size_t snowObjIdx = 0;
        for (size_t i = 0; i < scene->getObjectCount(); i++) {
            if (!scene->isSnowObject(i)) continue;
            
            const auto& obj = scene->getObject(i);
            
            VkPipeline pipelineHandle = obj.pipeline->getPipeline();
            vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineHandle);

            VkPipelineLayout pipelineLayout = obj.pipeline->getPipelineLayout();
            
            if (snowObjIdx >= _snowDescriptorSets.size()) {
                continue;
            }
            
            vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   pipelineLayout, 0, 1, &_snowDescriptorSets[snowObjIdx], 0, nullptr);

            VkBuffer vertexBuffers[] = { obj.vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets);

            vkCmdPushConstants(_commandBuffer, pipelineLayout, 
                              VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &obj.modelMatrix);

            if (obj.instanceCount > 1 && obj.instanceBuffer != VK_NULL_HANDLE) {
                vkCmdDraw(_commandBuffer, obj.vertexCount, obj.instanceCount, 0, 0);
            }
            
            snowObjIdx++;
        }
    }

    vkCmdEndRenderPass(_commandBuffer);

    if (vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void Frame::submitCommandBuffer(uint32_t imageIndex) {
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  
    VkSemaphore waitSemaphores[] = { _renderSemaphore };
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    VkSemaphore signalSemaphores[] = { _swapChain->getPresentationSemaphore(imageIndex) };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffer;

    if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
}

//Schnee(Command-Shader) braucht andere DescriptorSets
void Frame::allocateSnowDescriptorSets(VkDescriptorPool descriptorPool, 
                                      VkDescriptorSetLayout descriptorSetLayout, 
                                      size_t snowObjectCount) {
    if (snowObjectCount == 0) return;
    
    _snowDescriptorSets.resize(snowObjectCount);

    std::vector<VkDescriptorSetLayout> layouts(snowObjectCount, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(snowObjectCount);
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(_device, &allocInfo, _snowDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate snow descriptor sets!");
    }
}

void Frame::updateSnowDescriptorSet(size_t index, VkBuffer particleBuffer,
                                   VkImageView imageView, VkSampler sampler) {
    // UBO
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    // Storage Buffer (Particles)
    VkDescriptorBufferInfo storageInfo{};
    storageInfo.buffer = particleBuffer;
    storageInfo.offset = 0;
    storageInfo.range = sizeof(Particle) *NUMBER_PARTICLES;

    // Texture
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

    // Binding 0: UBO
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = _snowDescriptorSets[index];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    // Binding 1: Storage Buffer
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = _snowDescriptorSets[index];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &storageInfo;

    // Binding 2: Texture
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = _snowDescriptorSets[index];
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(_device, 
                          static_cast<uint32_t>(descriptorWrites.size()),
                          descriptorWrites.data(), 0, nullptr);
}


void Frame::createLitUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(LitUniformBufferObject);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &_litUniformBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create lit uniform buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, _litUniformBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = _buff.findMemoryType(memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _physicalDevice);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_litUniformBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate lit uniform buffer memory!");
    }

    vkBindBufferMemory(_device, _litUniformBuffer, _litUniformBufferMemory, 0);

    void* data = nullptr;
    vkMapMemory(_device, _litUniformBufferMemory, 0, bufferSize, 0, &data);
    _litUniformBufferMapped = static_cast<LitUniformBufferObject*>(data);
}

void Frame::allocateLitDescriptorSets(VkDescriptorPool descriptorPool, 
                                     VkDescriptorSetLayout descriptorSetLayout, 
                                     size_t objectCount) {
    if (objectCount == 0) return;
    
    _litDescriptorSets.resize(objectCount);

    std::vector<VkDescriptorSetLayout> layouts(objectCount, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(objectCount);
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(_device, &allocInfo, _litDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate lit descriptor sets!");
    }
}

void Frame::updateLitUniformBuffer(Camera* camera, Scene* scene) {
    if (!_litUniformBufferMapped) return;

    LitUniformBufferObject ubo{};
    ubo.view = camera->getViewMatrix();
    
    VkExtent2D extent = _swapChain->getExtent();
    ubo.proj = glm::perspective(
        glm::radians(camera->getZoom()),
        static_cast<float>(extent.width) / static_cast<float>(extent.height),
        0.1f, 100.0f
    );
    ubo.proj[1][1] *= -1.0f;
    
    ubo.viewPos = camera->getPosition();
    ubo.numLights = static_cast<int>(scene->getLightCount());
    
    // Kopiere Licht-Daten
    const auto& lights = scene->getLights();
    for (size_t i = 0; i < lights.size() && i < 4; ++i) {
        ubo.lights[i].position = lights[i].position;
        ubo.lights[i].color = lights[i].color;
        ubo.lights[i].intensity = lights[i].intensity;
        ubo.lights[i].radius = lights[i].radius;
    }
    
    std::memcpy(_litUniformBufferMapped, &ubo, sizeof(ubo));
}

void Frame::updateLitDescriptorSet(Scene* scene) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _litUniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(LitUniformBufferObject);

    size_t litIndex = 0;
    for (size_t i = 0; i < scene->getObjectCount(); ++i) {
        if (!scene->isLitObject(i)) continue;
        
        const auto& obj = scene->getObject(i);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = obj.textureImageView;
        imageInfo.sampler = obj.textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = _litDescriptorSets[litIndex];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = _litDescriptorSets[litIndex];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(_device, 2, descriptorWrites.data(), 0, nullptr);
        
        litIndex++;
    }
}


//Deferred shading

void Frame::createDeferredLightingUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(LitUniformBufferObject);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &_deferredLightingUniformBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create deferred lighting uniform buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, _deferredLightingUniformBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = _buff.findMemoryType(memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _physicalDevice);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_deferredLightingUniformBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate deferred lighting uniform buffer memory!");
    }

    vkBindBufferMemory(_device, _deferredLightingUniformBuffer, _deferredLightingUniformBufferMemory, 0);

    void* data = nullptr;
    vkMapMemory(_device, _deferredLightingUniformBufferMemory, 0, bufferSize, 0, &data);
    _deferredLightingUniformBufferMapped = static_cast<LitUniformBufferObject*>(data);
}

void Frame::allocateDeferredDescriptorSets(VkDescriptorPool descriptorPool, 
                                          VkDescriptorSetLayout descriptorSetLayout, 
                                          size_t objectCount) {
    if (objectCount == 0) return;
    
    _deferredDescriptorSets.resize(objectCount);

    std::vector<VkDescriptorSetLayout> layouts(objectCount, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(objectCount);
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(_device, &allocInfo, _deferredDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate deferred descriptor sets!");
    }
}

void Frame::allocateDeferredLightingDescriptorSet(VkDescriptorPool descriptorPool,
                                                  VkDescriptorSetLayout layout,
                                                  GBuffer* gBuffer) {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    if (vkAllocateDescriptorSets(_device, &allocInfo, &_deferredLightingDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate deferred lighting descriptor set!");
    }
    
    // Update mit G-Buffer Attachments
    VkDescriptorImageInfo albedoInfo{};
    albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    albedoInfo.imageView = gBuffer->getAlbedoImageView();
    albedoInfo.sampler = VK_NULL_HANDLE;
    
    VkDescriptorImageInfo normalInfo{};
    normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    normalInfo.imageView = gBuffer->getNormalImageView();
    normalInfo.sampler = VK_NULL_HANDLE;
    
    VkDescriptorImageInfo positionInfo{};
    positionInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    positionInfo.imageView = gBuffer->getPositionImageView();
    positionInfo.sampler = VK_NULL_HANDLE;
    
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _deferredLightingUniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(LitUniformBufferObject);
    
    std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
    
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = _deferredLightingDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &albedoInfo;
    
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = _deferredLightingDescriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &normalInfo;
    
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = _deferredLightingDescriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pImageInfo = &positionInfo;
    
    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = _deferredLightingDescriptorSet;
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pBufferInfo = &bufferInfo;
    
    vkUpdateDescriptorSets(_device, 4, descriptorWrites.data(), 0, nullptr);
}

void Frame::updateDeferredDescriptorSet(Scene* scene) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    size_t deferredIndex = 0;
    for (size_t i = 0; i < scene->getObjectCount(); ++i) {
        if (!scene->isDeferredObject(i)) continue;
        
        const auto& obj = scene->getObject(i);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = obj.textureImageView;
        imageInfo.sampler = obj.textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = _deferredDescriptorSets[deferredIndex];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = _deferredDescriptorSets[deferredIndex];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(_device, 2, descriptorWrites.data(), 0, nullptr);
        
        deferredIndex++;
    }
}

void Frame::updateDeferredLightingUniformBuffer(Camera* camera, Scene* scene) {
    if (!_deferredLightingUniformBufferMapped) return;

    LitUniformBufferObject ubo{};
    ubo.viewPos = camera->getPosition();
    ubo.numLights = static_cast<int>(scene->getLightCount());
    
    const auto& lights = scene->getLights();
    for (size_t i = 0; i < lights.size() && i < 4; ++i) {
        ubo.lights[i].position = lights[i].position;
        ubo.lights[i].color = lights[i].color;
        ubo.lights[i].intensity = lights[i].intensity;
        ubo.lights[i].radius = lights[i].radius;
    }
    
    std::memcpy(_deferredLightingUniformBufferMapped, &ubo, sizeof(ubo));
}