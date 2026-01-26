#pragma once

#include <vulkan/vulkan.h>
#include <array>
#include <stdexcept>
#include <iostream>
#include "../initBuffer.hpp"
/**
 * Verwaltet Cubemap RenderTargets für render-To-texture
 * Erstellt Cubemap-Textur aus Bildern von ReflectionProbe
 */
class CubemapRenderTarget {
public:
    CubemapRenderTarget(VkDevice device, VkPhysicalDevice physicalDevice, 
                       uint32_t resolution)
        : _device(device)
        , _physicalDevice(physicalDevice)
        , _resolution(resolution)
    {
        createCubemapImage();
        createCubemapViews();
        createDepthResources();
        createRenderPass();
        createFramebuffers();
        createSampler();
    }

    ~CubemapRenderTarget() {
        cleanup();
    }

    VkFramebuffer getFramebuffer(uint32_t faceIndex) const {
        return _framebuffers[faceIndex];
    }

    VkImageView getCubemapView() const {
        return _cubemapView;
    }

    VkSampler getSampler() const {
        return _sampler;
    }

    VkRenderPass getRenderPass() const {
        return _renderPass;
    }

    uint32_t getResolution() const {
        return _resolution;
    }
    //Zerstört alle Ressourcen, wird durch Destruktor aufgerufen
    void cleanup() ;

private:
    VkDevice _device;
    VkPhysicalDevice _physicalDevice;
    InitBuffer initB;
    uint32_t _resolution;
    //Cubemap ressourcen
    VkImage _cubemapImage = VK_NULL_HANDLE;
    VkDeviceMemory _cubemapMemory = VK_NULL_HANDLE;
    std::array<VkImageView, 6> _faceViews;
    VkImageView _cubemapView = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;
    //depth Kram
    VkImage _depthImage = VK_NULL_HANDLE;
    VkDeviceMemory _depthMemory = VK_NULL_HANDLE;
    VkImageView _depthView = VK_NULL_HANDLE;
    //RenderPass & Framebuffers
    std::array<VkFramebuffer, 6> _framebuffers;
    VkRenderPass _renderPass = VK_NULL_HANDLE;

    //Erstellt Cubemap-Image mit 6 Array-Layers
    void createCubemapImage();

    //Erstllt Image-Views (6*2D, 1*Cube)
    void createCubemapViews() ;

    //Erstellt depth-Buffer für alle Cubemap-Faces
    void createDepthResources();

    //Erstellt simplen Renderpass für Cubemap rendering
    // Renderpass::createRenderpass ist zu komplex
    void createRenderPass();

    //erstellt 6 passende Framebuffers
    void createFramebuffers();

    //Sampler für die Cubemap
    void createSampler();
};