// Frame.cpp - Vollständige Implementation mit Deferred Rendering
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

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &_uniformBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create uniform buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, _uniformBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = _buff.findMemoryType(memRequirements.memoryTypeBits,
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _physicalDevice);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_uniformBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate uniform buffer memory!");
    }

    vkBindBufferMemory(_device, _uniformBuffer, _uniformBufferMemory, 0);

    void* data = nullptr;
    vkMapMemory(_device, _uniformBufferMemory, 0, bufferSize, 0, &data);
    _uniformBufferMapped = static_cast<UniformBufferObject*>(data);
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

void Frame::createLightingUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(LightingUniformBufferObject);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &_lightingUniformBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create lighting uniform buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_device, _lightingUniformBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = _buff.findMemoryType(memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _physicalDevice);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_lightingUniformBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate lighting uniform buffer memory!");
    }

    vkBindBufferMemory(_device, _lightingUniformBuffer, _lightingUniformBufferMemory, 0);

    void* data = nullptr;
    vkMapMemory(_device, _lightingUniformBufferMemory, 0, bufferSize, 0, &data);
    _lightingUniformBufferMapped = static_cast<LightingUniformBufferObject*>(data);
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


// Frame.cpp - recordCommandBuffer - KOMPLETT ERSETZT

void Frame::recordCommandBuffer(Scene* scene, uint32_t imageIndex) {
    vkResetCommandBuffer(_commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(_commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPass rp = scene->getRenderPass();
    VkFramebuffer fb = _framebuffers->getFramebuffer(imageIndex);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = rp;
    renderPassInfo.framebuffer = fb;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = _swapChain->getExtent();

    std::array<VkClearValue, 3> clearValues{};
    clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};  // Back buffer
    clearValues[1].depthStencil = {1.0f, 0};              // Depth
    clearValues[2].color = {{0.0f, 0.0f, 0.0f, 0.0f}};   // G-Buffer

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

    // ============================================
    // SUBPASS 0: DEPTH PREPASS
    // ============================================    
    size_t deferredDescriptorIdx = 0;  // Zähler für deferred descriptor sets
    
    for (size_t i = 0; i < scene->getDeferredObjectCount(); ++i) {
        const auto& obj = scene->getDepthPassObject(i);
        
        if (obj.vertexCount == 0 || obj.vertexBuffer == VK_NULL_HANDLE) {
            deferredDescriptorIdx++;  // Auch bei skip hochzählen!
            continue;
        }

        VkPipeline pipeline = obj.pipeline->getPipeline();
        vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        VkPipelineLayout layout = obj.pipeline->getPipelineLayout();

        // Descriptor set binden (Index = deferredDescriptorIdx * 2 + 0)
        size_t setIndex = deferredDescriptorIdx * 2;
        if (setIndex < _descriptorSets.size()) {
            vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   layout, 0, 1, &_descriptorSets[setIndex], 0, nullptr);
        }

        VkBuffer vb[] = {obj.vertexBuffer};
        VkDeviceSize off[] = {0};
        vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vb, off);
        vkCmdPushConstants(_commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 
                          0, sizeof(glm::mat4), &obj.modelMatrix);
        vkCmdDraw(_commandBuffer, obj.vertexCount, 1, 0, 0);
        
        deferredDescriptorIdx++;
    }

    // ============================================
    // SUBPASS 1: G-BUFFER PASS
    // ============================================
    vkCmdNextSubpass(_commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);



    deferredDescriptorIdx = 0;

    for (size_t i = 0; i < scene->getDeferredObjectCount(); ++i) {
        const auto& obj = scene->getGBufferPassObject(i);
        
        if (obj.vertexCount == 0 || obj.vertexBuffer == VK_NULL_HANDLE) {
            std::cout << "  [GBUFFER] Skipping object " << i << " (invalid)" << std::endl;
            deferredDescriptorIdx++;
            continue;
        }    
        if (obj.pipeline) {
            VkPipeline pipeline = obj.pipeline->getPipeline();
        }

        VkPipeline pipeline = obj.pipeline->getPipeline();
        vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        VkPipelineLayout layout = obj.pipeline->getPipelineLayout();

        // Descriptor set binden
        size_t setIndex = deferredDescriptorIdx * 2 + 1;
        
        if (setIndex < _descriptorSets.size()) {
            vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                layout, 0, 1, &_descriptorSets[setIndex], 0, nullptr);
        } else {
            std::cout << "    ERROR: Descriptor set index out of range!" << std::endl;
        }

        VkBuffer vb[] = {obj.vertexBuffer};
        VkDeviceSize off[] = {0};
        vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vb, off);
        vkCmdPushConstants(_commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 
                        0, sizeof(glm::mat4), &obj.modelMatrix);
        
      
        vkCmdDraw(_commandBuffer, obj.vertexCount, 1, 0, 0);

        
        deferredDescriptorIdx++;
    }
    // ============================================
    // SUBPASS 2: LIGHTING + FORWARD RENDERING
    // ============================================
    vkCmdNextSubpass(_commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
    
//    std::cout << "\n=== SUBPASS 2: LIGHTING + FORWARD ===" << std::endl;
    // 1. LIGHTING QUAD
    if (scene->hasLightingQuad() && !_lightingDescriptorSets.empty()) {
//        std::cout << "Rendering lighting quad..." << std::endl;
        
        const auto& lightingQuad = scene->getLightingQuad();
        
        if (lightingQuad.vertexCount > 0 && lightingQuad.vertexBuffer != VK_NULL_HANDLE) {
            VkPipeline pipeline = lightingQuad.pipeline->getPipeline();
            vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            VkPipelineLayout layout = lightingQuad.pipeline->getPipelineLayout();
            
            vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                layout, 0, 1, &_lightingDescriptorSets[0], 0, nullptr);
            
            VkBuffer vb[] = {lightingQuad.vertexBuffer};
            VkDeviceSize off[] = {0};
            vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vb, off);
            
            vkCmdDraw(_commandBuffer, lightingQuad.vertexCount, 1, 0, 0);
        }
    }

    // 2. FORWARD OBJECTS (normale, snow, lit)
    // ✅ WICHTIG: Viewport/Scissor nochmal setzen!
    vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
    
//    std::cout << "\nRendering forward objects..." << std::endl;
    
    // Zähler für jede Descriptor Set Kategorie
    size_t normalForwardIdx = 0;  // Normale forward objects (NACH deferred)
    size_t snowIdx = 0;
    size_t litIdx = 0;
    
    // Berechne Offset für normale forward objects (nach deferred sets)
    size_t normalForwardOffset = scene->getDeferredObjectCount() * 2;
    
    for (size_t i = 0; i < scene->getObjectCount(); i++) {
        const auto& obj = scene->getObject(i);
        
        // Skip deferred objects - die wurden schon gerendert
        if (obj.isDeferred) {
            continue;
        }
        
        // Skip mirror objects erstmal
        if (scene->isMirrorObject(i)) {
            if (!obj.isSnow && !obj.isLit) normalForwardIdx++;
            continue;
        }
        
        if (obj.vertexCount == 0 || obj.vertexBuffer == VK_NULL_HANDLE) {
            if (obj.isSnow) snowIdx++;
            else if (obj.isLit) litIdx++;
            else normalForwardIdx++;
            continue;
        }

        VkPipeline pipeline = obj.pipeline->getPipeline();
        vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        VkPipelineLayout layout = obj.pipeline->getPipelineLayout();

        // Descriptor Set binden basierend auf Typ
        if (obj.isSnow) {
//            std::cout << "  Snow object " << i << " (descriptor " << snowIdx << ")" << std::endl;
            if (snowIdx < _snowDescriptorSets.size()) {
                vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       layout, 0, 1, &_snowDescriptorSets[snowIdx], 0, nullptr);
            }
            snowIdx++;
        } else if (obj.isLit) {
 //           std::cout << "  Lit object " << i << " (descriptor " << litIdx << ")" << std::endl;
            if (litIdx < _litDescriptorSets.size()) {
                vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       layout, 0, 1, &_litDescriptorSets[litIdx], 0, nullptr);
            }
            litIdx++;
        } else {
            // Normale forward object
            size_t setIndex = normalForwardOffset + normalForwardIdx;
//            std::cout << "  Normal forward object " << i << " (descriptor " << setIndex << ")" << std::endl;
            if (setIndex < _descriptorSets.size()) {
                vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       layout, 0, 1, &_descriptorSets[setIndex], 0, nullptr);
            }
            normalForwardIdx++;
        }

        VkBuffer vb[] = {obj.vertexBuffer};
        VkDeviceSize off[] = {0};
        vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vb, off);
        vkCmdPushConstants(_commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 
                          0, sizeof(glm::mat4), &obj.modelMatrix);

        if (obj.instanceCount > 1 && obj.instanceBuffer != VK_NULL_HANDLE) {
            vkCmdDraw(_commandBuffer, obj.vertexCount, obj.instanceCount, 0, 0);
        } else {
            vkCmdDraw(_commandBuffer, obj.vertexCount, 1, 0, 0);
        }
    }

    // 3. MIRROR SYSTEM (bleibt vorerst auskommentiert bis Basics funktionieren)
    // TODO: Mirror rendering...

    vkCmdEndRenderPass(_commandBuffer);

    if (vkEndCommandBuffer(_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
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

// Frame.cpp - updateDescriptorSet - ERSETZT

void Frame::updateDescriptorSet(Scene* scene) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    size_t descriptorSetIndex = 0;
    
    // 1. DEFERRED OBJECTS (2 descriptor sets pro object)
    for (size_t i = 0; i < scene->getDeferredObjectCount(); ++i) {
        const auto& depthObj = scene->getDepthPassObject(i);
        const auto& gbufferObj = scene->getGBufferPassObject(i);
        
        // Depth Pass Descriptor
        VkDescriptorImageInfo depthImageInfo{};
        depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        depthImageInfo.imageView = depthObj.textureImageView;
        depthImageInfo.sampler = depthObj.textureSampler;

        std::array<VkWriteDescriptorSet, 2> depthWrites{};
        depthWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        depthWrites[0].dstSet = _descriptorSets[descriptorSetIndex];
        depthWrites[0].dstBinding = 0;
        depthWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        depthWrites[0].descriptorCount = 1;
        depthWrites[0].pBufferInfo = &bufferInfo;

        depthWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        depthWrites[1].dstSet = _descriptorSets[descriptorSetIndex];
        depthWrites[1].dstBinding = 1;
        depthWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        depthWrites[1].descriptorCount = 1;
        depthWrites[1].pImageInfo = &depthImageInfo;

        vkUpdateDescriptorSets(_device, 2, depthWrites.data(), 0, nullptr);
        descriptorSetIndex++;
        
        // G-Buffer Pass Descriptor
        VkDescriptorImageInfo gbufferImageInfo{};
        gbufferImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        gbufferImageInfo.imageView = gbufferObj.textureImageView;
        gbufferImageInfo.sampler = gbufferObj.textureSampler;

        std::array<VkWriteDescriptorSet, 2> gbufferWrites{};
        gbufferWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        gbufferWrites[0].dstSet = _descriptorSets[descriptorSetIndex];
        gbufferWrites[0].dstBinding = 0;
        gbufferWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        gbufferWrites[0].descriptorCount = 1;
        gbufferWrites[0].pBufferInfo = &bufferInfo;

        gbufferWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        gbufferWrites[1].dstSet = _descriptorSets[descriptorSetIndex];
        gbufferWrites[1].dstBinding = 1;
        gbufferWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gbufferWrites[1].descriptorCount = 1;
        gbufferWrites[1].pImageInfo = &gbufferImageInfo;

        vkUpdateDescriptorSets(_device, 2, gbufferWrites.data(), 0, nullptr);
        descriptorSetIndex++;
    }
    
    // 2. NORMAL FORWARD OBJECTS
    for (size_t i = 0; i < scene->getObjectCount(); ++i) {
        const auto& obj = scene->getObject(i);
        
        // Skip deferred, snow, lit
        if (obj.isDeferred || obj.isSnow || obj.isLit) {
            continue;
        }
        
        if (descriptorSetIndex >= _descriptorSets.size()) {
            std::cerr << "ERROR: Descriptor set index out of range!" << std::endl;
            break;
        }

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = obj.textureImageView;
        imageInfo.sampler = obj.textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = _descriptorSets[descriptorSetIndex];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = _descriptorSets[descriptorSetIndex];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(_device, 2, descriptorWrites.data(), 0, nullptr);
        
        descriptorSetIndex++;
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

// ============================================
// LIGHTING DESCRIPTOR SETS
// ============================================

void Frame::allocateLightingDescriptorSets(VkDescriptorPool descriptorPool, 
                                          VkDescriptorSetLayout descriptorSetLayout, 
                                          size_t count) {
    if (count == 0) {
        std::cout << "No lighting descriptor sets to allocate" << std::endl;
        return;
    }
    
    _lightingDescriptorSets.resize(count);

    std::vector<VkDescriptorSetLayout> layouts(count, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(count);
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(_device, &allocInfo, _lightingDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate lighting descriptor sets!");
    }
    
    std::cout << "Allocated " << count << " lighting descriptor sets" << std::endl;
}

void Frame::updateLightingDescriptorSet(VkImageView gBufferView, VkImageView depthView) {
    if (_lightingDescriptorSets.empty()) {
        std::cerr << "WARNING: No lighting descriptor sets allocated!" << std::endl;
        return;
    }

    std::array<VkDescriptorImageInfo, 2> imageInfos{};
    
    // Binding 0: G-Buffer Input Attachment
    imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfos[0].imageView = gBufferView;
    imageInfos[0].sampler = VK_NULL_HANDLE;
    
    // Binding 1: Depth Input Attachment
    imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    imageInfos[1].imageView = depthView;
    imageInfos[1].sampler = VK_NULL_HANDLE;

    // ✅ Binding 2: Lighting UBO
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = _lightingUniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(LightingUniformBufferObject);

    std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

    //GBuffer
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = _lightingDescriptorSets[0];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfos[0];

    //Depth
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = _lightingDescriptorSets[0];
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfos[1];

    //UBO
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = _lightingDescriptorSets[0];
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(_device, 
                          static_cast<uint32_t>(descriptorWrites.size()),
                          descriptorWrites.data(), 0, nullptr);
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
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(_device, &semInfo, nullptr, &_renderSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render semaphore!");
    }

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
    if (_litUniformBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(_device, _litUniformBuffer, nullptr);
        _litUniformBuffer = VK_NULL_HANDLE;
    }
    if (_litUniformBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _litUniformBufferMemory, nullptr);
        _litUniformBufferMemory = VK_NULL_HANDLE;
    }
    if(_lightingUniformBuffer != VK_NULL_HANDLE){
        vkDestroyBuffer(_device, _lightingUniformBuffer, nullptr);
        _lightingUniformBuffer = VK_NULL_HANDLE;
    }
    if(_lightingUniformBufferMemory != VK_NULL_HANDLE){
        vkFreeMemory(_device, _lightingUniformBufferMemory,nullptr);
        _lightingUniformBufferMemory = VK_NULL_HANDLE;
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
    if (_inFlightFence != VK_NULL_HANDLE) {
        vkWaitForFences(_device, 1, &_inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(_device, 1, &_inFlightFence);
    }
}

void Frame::updateUniformBuffer(Camera* camera) {
    if (!_uniformBufferMapped) return;

    UniformBufferObject ubo{};
    ubo.view = camera->getViewMatrix();

    VkExtent2D extent = _swapChain->getExtent();
    float width = static_cast<float>(extent.width);
    float height = static_cast<float>(extent.height);

    ubo.proj = glm::perspective(
        glm::radians(camera->getZoom()),
        width / height,
        0.1f, 100.0f
    );
    ubo.proj[1][1] *= -1.0f;

    std::memcpy(_uniformBufferMapped, &ubo, sizeof(ubo));
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
    
    const auto& lights = scene->getLights();
    for (size_t i = 0; i < lights.size() && i < 4; ++i) {
        ubo.lights[i].position = lights[i].position;
        ubo.lights[i].color = lights[i].color;
        ubo.lights[i].intensity = lights[i].intensity;
        ubo.lights[i].radius = lights[i].radius;
    }
    
    std::memcpy(_litUniformBufferMapped, &ubo, sizeof(ubo));
}

void Frame::updateLightingUniformBuffer(Camera* camera, Scene* scene) {
    if (!_lightingUniformBufferMapped) return;

    LightingUniformBufferObject ubo{};
    
    //view und proj Matrizen
    glm::mat4 view = camera->getViewMatrix();
    VkExtent2D extent = _swapChain->getExtent();
    glm::mat4 proj = glm::perspective(
        glm::radians(camera->getZoom()),
        static_cast<float>(extent.width) / static_cast<float>(extent.height),
        0.1f, 100.0f
    );
    proj[1][1] *= -1.0f;
    
    ubo.invView = glm::inverse(view);
    ubo.invProj = glm::inverse(proj);
    ubo.viewPos = camera->getPosition();
    ubo.numLights = static_cast<int32_t>(scene->getLightCount());
    
    //screen-Size für UV-Berechnung
    ubo.screenSize = glm::vec2(extent.width, extent.height);
    
    // Lichtquellen kopieren
    const auto& lights = scene->getLights();
    for (size_t i = 0; i < lights.size() && i < 4; ++i) {
        ubo.lights[i].position = lights[i].position;
        ubo.lights[i].color = lights[i].color;
        ubo.lights[i].intensity = lights[i].intensity;
        ubo.lights[i].radius = lights[i].radius;
    }
    
    std::memcpy(_lightingUniformBufferMapped, &ubo, sizeof(ubo));
}

// Frame.cpp - Teil 4: Mirror System (bleibt wie im Original)

void Frame::renderMirrorSystem(Scene* scene, size_t& normalIdx, 
                               size_t& snowIdx, size_t& litIdx) {
    const auto& mirrorMarkIndices = scene->getMirrorMarkIndices();
    const auto& mirrorBlendIndices = scene->getMirrorBlendIndices();

    // ========================================
    // PASS 1: Spiegel-Markierung (Stencil Buffer schreiben)
    // ========================================
    for (size_t markIdx : mirrorMarkIndices) {
        const auto& obj = scene->getObject(markIdx);
        
        if (obj.vertexCount == 0 || obj.vertexBuffer == VK_NULL_HANDLE) {
            if (!obj.isSnow && !obj.isLit && !obj.isDeferred) normalIdx++;
            continue;
        }

        VkPipeline pipelineHandle = obj.pipeline->getPipeline();
        vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineHandle);
        VkPipelineLayout pipelineLayout = obj.pipeline->getPipelineLayout();

        if (normalIdx < _descriptorSets.size()) {
            vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   pipelineLayout, 0, 1, &_descriptorSets[normalIdx], 0, nullptr);
        }
        normalIdx++;

        VkBuffer vertexBuffers[] = {obj.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdPushConstants(_commandBuffer, pipelineLayout,
                          VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &obj.modelMatrix);

        vkCmdDraw(_commandBuffer, obj.vertexCount, 1, 0, 0);
    }

    // ========================================
    // PASS 2: Gespiegelte Objekte rendern (nur wo Stencil == 1)
    // ========================================
    for (size_t i = 0; i < scene->getReflectedObjectCount(); i++) {
        const auto& reflObj = scene->getReflectedObject(i);
        
        if (reflObj.vertexCount == 0 || reflObj.vertexBuffer == VK_NULL_HANDLE) {
            continue;
        }

        VkPipeline pipelineHandle = reflObj.pipeline->getPipeline();
        vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineHandle);
        VkPipelineLayout pipelineLayout = reflObj.pipeline->getPipelineLayout();

        // Stencil Reference auf 1 setzen
        vkCmdSetStencilReference(_commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 1);

        // Descriptor Set vom Original-Objekt verwenden
        size_t originalIdx = scene->getReflectedDescriptorIndex(i);
        const auto& originalObj = scene->getObject(originalIdx);
        
        if (originalObj.isSnow) {
            size_t snowIdxTmp = 0;
            for (size_t j = 0; j < originalIdx; j++) {
                if (scene->isSnowObject(j)) snowIdxTmp++;
            }
            if (snowIdxTmp < _snowDescriptorSets.size()) {
                vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       pipelineLayout, 0, 1, &_snowDescriptorSets[snowIdxTmp], 0, nullptr);
            }
        } else if (originalObj.isLit) {
            size_t litIdxTmp = 0;
            for (size_t j = 0; j < originalIdx; j++) {
                if (scene->isLitObject(j)) litIdxTmp++;
            }
            if (litIdxTmp < _litDescriptorSets.size()) {
                vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       pipelineLayout, 0, 1, &_litDescriptorSets[litIdxTmp], 0, nullptr);
            }
        } else {
            size_t normIdxTmp = 0;
            for (size_t j = 0; j < originalIdx; j++) {
                if (!scene->isSnowObject(j) && !scene->isLitObject(j) && 
                    !scene->isMirrorObject(j) && !scene->isDeferredObject(j)) {
                    normIdxTmp++;
                }
            }
            if (normIdxTmp < _descriptorSets.size()) {
                vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                       pipelineLayout, 0, 1, &_descriptorSets[normIdxTmp], 0, nullptr);
            }
        }

        VkBuffer vertexBuffers[] = {reflObj.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdPushConstants(_commandBuffer, pipelineLayout,
                          VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &reflObj.modelMatrix);

        vkCmdDraw(_commandBuffer, reflObj.vertexCount, 1, 0, 0);
    }

    // ========================================
    // PASS 3: Transparenten Spiegel rendern (MIRROR_BLEND)
    // ========================================
    for (size_t blendIdx : mirrorBlendIndices) {
        const auto& obj = scene->getObject(blendIdx);
        
        if (obj.vertexCount == 0 || obj.vertexBuffer == VK_NULL_HANDLE) {
            if (!obj.isSnow && !obj.isLit && !obj.isDeferred) normalIdx++;
            continue;
        }

        VkPipeline pipelineHandle = obj.pipeline->getPipeline();
        vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineHandle);
        VkPipelineLayout pipelineLayout = obj.pipeline->getPipelineLayout();

        if (normalIdx < _descriptorSets.size()) {
            vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   pipelineLayout, 0, 1, &_descriptorSets[normalIdx], 0, nullptr);
        }
        normalIdx++;

        VkBuffer vertexBuffers[] = {obj.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(_commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdPushConstants(_commandBuffer, pipelineLayout,
                          VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &obj.modelMatrix);

        vkCmdDraw(_commandBuffer, obj.vertexCount, 1, 0, 0);
    }
}