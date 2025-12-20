// Frame.cpp
#include "Frame.hpp"

#include <stdexcept>
#include <array>
#include <cstring>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>



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
    // bufferInfo 
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    size_t objectCount = scene->getObjectCount();
    //Für jedes Objekt
    for (size_t i = 0; i < objectCount; ++i) {
        const auto& obj = scene->getObject(i);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = obj.textureImageView;
        imageInfo.sampler = obj.textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = _descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = _descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(_device,
                               static_cast<uint32_t>(descriptorWrites.size()),
                               descriptorWrites.data(),
                               0, nullptr);
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

void Frame::recordCommandBuffer(Scene* scene, uint32_t imageIndex) {
    vkResetCommandBuffer(_commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    if (vkBeginCommandBuffer(_commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = scene->getRenderPass();
    renderPassInfo.framebuffer = _framebuffers->getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = _swapChain->getExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};  // Depth=1, Stencil=0

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
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

    // ========== PASS 0: Normale Szene (ohne Spiegel-Objekte) ==========
    for (size_t i = 0; i < scene->getObjectCount(); i++) {
        if (scene->isMirrorObject(i)) continue;

        const auto& obj = scene->getObject(i);
        renderObject(obj, i);
    }

    // ========== PASS 1: Spiegel-Markierung im Stencil Buffer ==========
    size_t mirrorMarkIndex = scene->getMirrorMarkIndex();
    if (mirrorMarkIndex != SIZE_MAX) {
        const auto& mirrorMark = scene->getObject(mirrorMarkIndex);
        renderObject(mirrorMark, mirrorMarkIndex);
    }

    // ========== PASS 2: Gespiegelte Objekte rendern ==========
    // WICHTIG: Hier wird der Stencil Reference Value gesetzt!
    for (size_t i = 0; i < scene->getReflectedObjectCount(); i++) {
        const auto& reflected = scene->getReflectedObject(i);
        size_t descriptorIndex = scene->getReflectedDescriptorIndex(i);
        
        // RENDER MIT STENCIL REFERENCE = 1
        renderObjectWithStencil(reflected, descriptorIndex, 1);
    }

    // ========== PASS 3: Spiegel selbst (mit Transparenz) ==========
    size_t mirrorBlendIndex = scene->getMirrorBlendIndex();
    if (mirrorBlendIndex != SIZE_MAX) {
        const auto& mirrorBlend = scene->getObject(mirrorBlendIndex);
        renderObject(mirrorBlend, mirrorBlendIndex);
    }

    vkCmdEndRenderPass(_commandBuffer);

    if (vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void Frame::renderObject(const RenderObject& obj, size_t descriptorIndex) {
    if (obj.vertexCount == 0 || obj.vertexBuffer == VK_NULL_HANDLE) {
        return;
    }

    VkPipeline pipelineHandle = obj.pipeline->getPipeline();
    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineHandle);

    VkPipelineLayout pipelineLayout = obj.pipeline->getPipelineLayout();
    vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &_descriptorSets[descriptorIndex], 
                            0, nullptr);

    VkBuffer vertexBuffers[] = {obj.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdPushConstants(_commandBuffer, pipelineLayout, 
                       VK_SHADER_STAGE_VERTEX_BIT, 0, 
                       sizeof(glm::mat4), &obj.modelMatrix);

    vkCmdDraw(_commandBuffer, obj.vertexCount, 1, 0, 0);
}

// NEUE METHODE: Render mit explizitem Stencil Reference
void Frame::renderObjectWithStencil(const RenderObject& obj, size_t descriptorIndex, uint32_t stencilRef) {
    if (obj.vertexCount == 0 || obj.vertexBuffer == VK_NULL_HANDLE) {
        return;
    }

    VkPipeline pipelineHandle = obj.pipeline->getPipeline();
    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineHandle);

    // KRITISCH: Stencil Reference dynamisch setzen!
    // Dies sorgt dafür, dass nur Pixel mit Stencil=1 gerendert werden
    vkCmdSetStencilReference(_commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, stencilRef);

    VkPipelineLayout pipelineLayout = obj.pipeline->getPipelineLayout();
    vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &_descriptorSets[descriptorIndex], 
                            0, nullptr);

    VkBuffer vertexBuffers[] = {obj.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdPushConstants(_commandBuffer, pipelineLayout, 
                       VK_SHADER_STAGE_VERTEX_BIT, 0, 
                       sizeof(glm::mat4), &obj.modelMatrix);

    vkCmdDraw(_commandBuffer, obj.vertexCount, 1, 0, 0);
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
