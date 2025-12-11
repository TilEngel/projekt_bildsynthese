// Frame.cpp
#include "Frame.hpp"

#include <stdexcept>
#include <array>
#include <cstring>
#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "initBuffer.hpp"

extern InitBuffer buff; // in main.cpp you have a global InitBuffer named 'buff'

/*
  Hinweis:
  - UniformBufferObject muss in einem deiner Header (z.B. Scene.hpp) definiert sein.
  - Scene wird hier verwendet, es muss Methoden wie getImageView(), getSampler(),
    getRenderPass(), getPipeline(), getPipelineLayout(), getVertexBuffer(), getVertexCount() bereitstellen.
  - SwapChain muss getExtent() und getPresentationSemaphore(imageIndex) und acquireNextImage/presentImage implementieren.
*/

static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void Frame::createUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    // create buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &_uniformBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create uniform buffer!");
    }

    // allocate memory
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, _uniformBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryTypeIndex(_physicalDevice, memRequirements.memoryTypeBits,
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_uniformBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate uniform buffer memory!");
    }

    vkBindBufferMemory(_device, _uniformBuffer, _uniformBufferMemory, 0);

    // map memory
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

    // fence (start signaled so first wait doesn't stall)
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
    // The command buffer will be freed by the owner of the command pool at destruction time,
    // but it's safe to free it explicitly here if command pool is still valid.
    // We don't have commandPool stored; it's freed elsewhere -- assume owner frees it.
}

void Frame::waitForFence() {
    // wait for previous frame to finish
    if (_inFlightFence != VK_NULL_HANDLE) {
        vkWaitForFences(_device, 1, &_inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(_device, 1, &_inFlightFence);
    }

}



void Frame::updateDescriptorSet(Scene* scene) {
    // bufferInfo (gleich für alle sets: UBO pro Frame)
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    size_t objectCount = scene->getObjectCount();
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

void Frame::updateUniformBuffer() {
    if (!_uniformBufferMapped) return;

    UniformBufferObject ubo{};

    // time-based rotation
    float time = static_cast<float>(glfwGetTime());
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // view: move back along z so object is visible
    glm::mat4 view = glm::lookAtRH(glm::vec3(4.0f, 4.0f, 4.0f), // eye
                                   glm::vec3(0.0f, 0.0f, 0.0f), // center
                                   glm::vec3(0.0f, 1.0f, 0.0f)); // up

    VkExtent2D extent = _swapChain->getExtent();
    float width = static_cast<float>(extent.width);
    float height = static_cast<float>(extent.height);

    // projection: perspective FOV. Using glm::perspective and flip Y.
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 10.0f);
    proj[1][1] *= -1.0f; // invert Y for Vulkan's coordinate system with GLM

    // assume UniformBufferObject contains model, view, proj as glm::mat4 named model/view/proj
    ubo.model = model;
    ubo.view = view;
    ubo.proj = proj;

    // copy to mapped memory
    std::memcpy(_uniformBufferMapped, &ubo, sizeof(ubo));
}

void Frame::recordCommandBuffer(Scene* scene, uint32_t imageIndex) {
    // reset command buffer
    vkResetCommandBuffer(_commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(_commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // begin render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = scene->getRenderPass();
    renderPassInfo.framebuffer = _framebuffers->getFramebuffer(imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = _swapChain->getExtent();

    // clear values: color + depth
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.1f, 0.1f, 0.1f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // bind pipeline
    vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scene->getPipeline());
    // viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapChain->getExtent().width);
    viewport.height = static_cast<float>(_swapChain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);

    // scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _swapChain->getExtent();
    vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);

    // bind descriptor set
    VkPipelineLayout pipelineLayout = scene->getPipelineLayout();
        //Draw object(s) in scene
    for (size_t i = 0; i < scene->getObjectCount(); ++i) {
        const auto& obj = scene->getObject(i);
        vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, 0, 1, &_descriptorSets[i], 0, nullptr);

        // bind vertex buffer des aktuellen Objekts
        VkBuffer vertexBuffers[] = { obj.vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets);


        // Push constant: per-object model matrix (falls du PushConstants verwendest)
        // Beispiel-Modelmatrix: einfache Translation je nach i
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(float(i * 2), 0.0f, 0.0f));
        model = glm::rotate(model, (float)glfwGetTime() * glm::radians(45.0f), glm::vec3(0,1,0));
        vkCmdPushConstants(_commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);


        // draw call
        vkCmdDraw(_commandBuffer, obj.vertexCount, 1, 0, 0);
    }
    // end render pass
    vkCmdEndRenderPass(_commandBuffer);

    if (vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void Frame::submitCommandBuffer(uint32_t imageIndex) {
    // reset fence already done in waitForFence() before we start rendering
    // prepare submission
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // We follow your comments:
    // - wait semaphore: _renderSemaphore (this is a bit unusual naming-wise,
    //   but follows the instructions you gave)
    // - signal semaphore: swapChain->getPresentationSemaphore(imageIndex)
    VkSemaphore waitSemaphores[] = { _renderSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
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
