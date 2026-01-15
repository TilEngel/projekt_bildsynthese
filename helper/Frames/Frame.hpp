// Frame.hpp
#pragma once

#include <vulkan/vulkan_core.h>
#include <vector>
#include <glm/glm.hpp>
#include "../Rendering/Swapchain.hpp"
#include "../Rendering/Framebuffers.hpp"
#include "../../Scene.hpp"
#include "Camera.hpp"
#include "../initBuffer.hpp"
#include "../renderToTexture/ReflectionProbe.hpp"

struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 cameraPos;
};

//UBO für deferred Shading
struct LightingUniformBufferObject {
    alignas(16) glm::mat4 invView;
    alignas(16) glm::mat4 invProj;
    alignas(16) glm::vec3 viewPos;
    alignas(4)  int32_t numLights;
    alignas(8)  glm::vec2 screenSize;  
    alignas(8)  glm::vec2 _padding;
    
    struct Light {
        alignas(16) glm::vec3 position;
        alignas(4)  float intensity;
        alignas(16) glm::vec3 color;
        alignas(4)  float radius;
    } lights[4];
};

class Frame {
public:
    Frame(VkPhysicalDevice physicalDevice, VkDevice device, SwapChain* swapChain,
          Framebuffers* framebuffers, VkQueue graphicsQueue, VkCommandPool commandPool)
        : _physicalDevice(physicalDevice), _device(device), _swapChain(swapChain),
          _framebuffers(framebuffers), _graphicsQueue(graphicsQueue) {
        createUniformBuffer();
        createLitUniformBuffer();
        createLightingUniformBuffer();
        allocateCommandBuffer(commandPool);
        createSyncObjects();
    }

    ~Frame() {
        cleanup();
    }

    // Uniform Buffers
    void createUniformBuffer();
    void createLitUniformBuffer();
    void createLightingUniformBuffer();
    void updateUniformBuffer(Camera* camera);
    void updateLitUniformBuffer(Camera* camera, Scene* scene);
    void updateLightingUniformBuffer(Camera* camera, Scene* scene);

    // Descriptor Sets
    void allocateDescriptorSets(VkDescriptorPool descriptorPool, 
                                VkDescriptorSetLayout descriptorSetLayout, 
                                size_t objectCount);
    void allocateSnowDescriptorSets(VkDescriptorPool descriptorPool, 
                                    VkDescriptorSetLayout descriptorSetLayout, 
                                    size_t snowObjectCount);
    void allocateLitDescriptorSets(VkDescriptorPool descriptorPool, 
                                   VkDescriptorSetLayout descriptorSetLayout, 
                                   size_t objectCount);
    void allocateLightingDescriptorSets(VkDescriptorPool descriptorPool, 
                                          VkDescriptorSetLayout descriptorSetLayout, 
                                          size_t count);

    void updateDescriptorSet(Scene* scene);
    void updateLitDescriptorSet(Scene* scene);
    void updateSnowDescriptorSet(size_t index, VkBuffer particleBuffer,
                                 VkImageView imageView, VkSampler sampler);
    void updateLightingDescriptorSet(VkImageView gBufferNormalView,VkImageView gBufferAlbedoView, VkImageView depthView);

    // Command Buffer
    void allocateCommandBuffer(VkCommandPool commandPool);
    void recordCommandBuffer(Scene* scene, uint32_t imageIndex);

    // Deferred Rendering Passes
    void renderDeferredDepthPass(Scene* scene);
    void renderDeferredGBufferPass(Scene* scene);
    void renderDeferredLightingPass(Scene* scene);
    void renderForwardObjects(Scene* scene);
    void renderCubemap(Scene* scene, ReflectionProbe* probe);
    //Helper: Rendert Objekte für ein Cubemap-Face
    void renderObjectsForCubemap(VkCommandBuffer cmd, Scene* scene, 
                                 size_t reflectiveObjectIndex);

    // Helper Methods
    void renderSingleObject(const RenderObject& obj, size_t normalIdx = 0, 
                           size_t snowIdx = 0, size_t litIdx = 0);
   

    // Sync Objects
    void createSyncObjects();
    void waitForFence();
    void submitCommandBuffer(uint32_t imageIndex);

    // Rendering
    bool render(Scene* scene, ReflectionProbe* probe = nullptr) {
        waitForFence();

        static uint32_t frameCounter = 0;
        if (probe&& (frameCounter % scene->getReflectionUpdateInterval() == 0)) {
            renderCubemap(scene, probe);
        }
        frameCounter++;

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(_device, _swapChain->getSwapchain(),
                                                UINT64_MAX, _renderSemaphore, VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return true;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        recordCommandBuffer(scene, imageIndex);
        submitCommandBuffer(imageIndex);

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        VkSemaphore signalSemaphores[] = { _swapChain->getPresentationSemaphore(imageIndex) };
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = { _swapChain->getSwapchain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(_graphicsQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            return true;
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        return false;
    }

    void cleanup();

private:
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    SwapChain* _swapChain;
    Framebuffers* _framebuffers;
    VkQueue _graphicsQueue;

    // Uniform Buffers
    VkBuffer _uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _uniformBufferMemory = VK_NULL_HANDLE;
    UniformBufferObject* _uniformBufferMapped = nullptr;

    VkBuffer _litUniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _litUniformBufferMemory = VK_NULL_HANDLE;
    LitUniformBufferObject* _litUniformBufferMapped = nullptr;

    //Uniform Buffers für deferred
    VkBuffer _lightingUniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _lightingUniformBufferMemory = VK_NULL_HANDLE;
    LightingUniformBufferObject* _lightingUniformBufferMapped = nullptr;

    // Descriptor Sets
    std::vector<VkDescriptorSet> _descriptorSets;
    std::vector<VkDescriptorSet> _snowDescriptorSets;
    std::vector<VkDescriptorSet> _litDescriptorSets;
    std::vector<VkDescriptorSet> _lightingDescriptorSets;
    // Command Buffer
    VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;

    // Sync Objects
    VkSemaphore _renderSemaphore = VK_NULL_HANDLE;
    VkFence _inFlightFence = VK_NULL_HANDLE;

    // Helper
    InitBuffer _buff;
};