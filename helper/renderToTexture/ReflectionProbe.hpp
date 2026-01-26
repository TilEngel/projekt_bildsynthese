#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <memory>
#include "CubemapRenderTarget.hpp"
/*
* Liefert Bilder für die Render-To-Texture Cubemap
* Platziert Quasi die 6 Kameras und schießt die Fotos
*/
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
    std::array<glm::mat4, 6> getCubeFaceViews() const ;

    // 90Grad FOV Projection für Cubemap
    glm::mat4 getProjection() const ;

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

    void createCommandBuffer();
};