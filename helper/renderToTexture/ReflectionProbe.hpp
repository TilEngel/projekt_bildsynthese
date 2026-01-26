#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <memory>
#include "CubemapRenderTarget.hpp"

class ReflectionProbe {
public:
    ReflectionProbe(VkDevice device, 
                   VkPhysicalDevice physicalDevice,
                   VkCommandPool commandPool,
                   VkQueue graphicsQueue,
                   const glm::vec3& position,
                   uint32_t resolution = 512)
        : _device(device)
        , _physicalDevice(physicalDevice)
        , _commandPool(commandPool)
        , _graphicsQueue(graphicsQueue)
        , _position(position)
        , _resolution(resolution)
    {
        _renderTarget = std::make_unique<CubemapRenderTarget>(
            device, physicalDevice, resolution
        );
        
        createCommandBuffer();
        std::cout << "ReflectionProbe created at position (" 
                  << position.x << ", " << position.y << ", " << position.z 
                  << ")" << std::endl;
    }

    ~ReflectionProbe() {
        cleanup();
    }

    //View-Matrizen für Cubemap
    std::array<glm::mat4, 6> getCubeFaceViews() const {
        return {
            // -X
            glm::lookAt(_position, _position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
            
            // +X
            glm::lookAt(_position, _position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
            
            // -Y Up-Vektor nach -Z
            glm::lookAt(_position, _position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            
            // +Y  Up-Vektor nach +Z
            glm::lookAt(_position, _position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            
            // -Z
            glm::lookAt(_position, _position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
            
            // +Z
            glm::lookAt(_position, _position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f))
        };
    }

    // 90° FOV Projection für Cubemap
    glm::mat4 getProjection() const {
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
        proj[1][1] *= -1.0f; // Vulkan Y-Flip
        return proj;
    }

    VkCommandBuffer getCommandBuffer() const {
        return _commandBuffer;
    }

    CubemapRenderTarget* getRenderTarget() const {
        return _renderTarget.get();
    }

    VkFramebuffer getFramebuffer(uint32_t faceIndex) const {
        return _renderTarget->getFramebuffer(faceIndex);
    }

    VkRenderPass getRenderPass() const {
        return _renderTarget->getRenderPass();
    }

    VkImageView getCubemapView() const {
        return _renderTarget->getCubemapView();
    }

    VkSampler getCubemapSampler() const {
        return _renderTarget->getSampler();
    }

    uint32_t getResolution() const {
        return _resolution;
    }

    glm::vec3 getPosition() const {
        return _position;
    }

    void setPosition(const glm::vec3& position) {
        _position = position;
    }

    void cleanup() {
        if (_commandBuffer != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(_device, _commandPool, 1, &_commandBuffer);
            _commandBuffer = VK_NULL_HANDLE;
        }
        
        if (_renderTarget) {
            _renderTarget->cleanup();
        }
    }

private:
    VkDevice _device;
    VkPhysicalDevice _physicalDevice;
    VkCommandPool _commandPool;
    VkQueue _graphicsQueue;
    
    glm::vec3 _position;
    uint32_t _resolution;
    
    std::unique_ptr<CubemapRenderTarget> _renderTarget;
    VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;

    void createCommandBuffer() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = _commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(_device, &allocInfo, &_commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate reflection probe command buffer!");
        }
    }
};